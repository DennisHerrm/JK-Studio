// app_menue_m6.inc - Menue-Editor M6: Menuefluss (Punkt 1), Tab-Reihenfolge (Punkt 2),
// Referenzbild (Punkt 7), Rueckfrage beim Schliessen eines geaenderten Reiters (Rest M5b).
// Wird in app_menue.cpp eingebunden (wie app_menue_projekt.inc).

namespace {   // wie die Vorwaertserklaerungen oben in app_menue.cpp

// --- Punkt 1: Menuefluss ----------------------------------------------------------------

const Fluss& flussAktuell() {
    Zustand& z = Z();
    std::string schluessel;
    std::vector<const Datei*> dat;
    for (const auto& dp : z.docs) {
        neuParsen(*dp);
        dat.push_back(&dp->datei);
        schluessel += std::to_string(reinterpret_cast<std::uintptr_t>(dp.get())) + ":" + std::to_string(dp->geparst) + ";";
    }
    if (schluessel != z.flussSchluessel) {
        z.flussSchluessel = schluessel;
        z.fluss = menueFluss(dat);
    }
    return z.fluss;
}

// Das Menue eines Flussknotens zeigen (Dokument-Reiter und Seite)
void flussSpringe(int dok, int menu) {
    Zustand& z = Z();
    if (dok < 0 || static_cast<std::size_t>(dok) >= z.docs.size()) { return; }
    z.aktiv = dok;
    Dokument& d = *z.docs[static_cast<std::size_t>(dok)];
    const std::vector<Seite> s = seitenImMenue(d.datei);
    for (std::size_t k = 0; k < s.size(); ++k) {
        if (s[k].menu == menu && s[k].cvar.empty()) {
            d.seite = static_cast<int>(k);
            break;
        }
    }
    d.auswahl.clear();
}

std::string fmtFluss(Str s, const std::string& name) {
    std::vector<char> b(1024);
    std::snprintf(b.data(), b.size(), tr(s), name.c_str(), name.c_str());
    return b.data();
}

// Was mit dem Menue nicht stimmt, in normaler Sprache ("" = nichts)
std::string flussProblem(const FlussMenue& n) {
    if (n.datei < 0) {
        std::vector<char> b(1024);
        std::snprintf(b.data(), b.size(), tr(Str::MeFlussFehlt), n.name.c_str());
        return b.data();
    }
    if (n.sackgasse) { return fmtFluss(Str::MeFlussSackgasse, n.name); }
    if (!n.erreichbar) { return fmtFluss(Str::MeFlussUnerreichbar, n.name); }
    if (n.ohneZurueck) { return fmtFluss(Str::MeFlussOhneZurueck, n.name); }
    return {};
}

void zeichneFlussFenster(Dokument* aktivDok) {
    Zustand& z = Z();
    if (!z.flussOffen) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 50, fs * 32}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.45F});   // unter der Leiste
    const std::string titel = std::string(tr(Str::MeFluss)) + "###menuefluss";
    if (!ImGui::Begin(titel.c_str(), &z.flussOffen)) {
        ImGui::End();
        return;
    }
    if (z.docs.empty()) {
        ImGui::TextWrapped("%s", tr(Str::MeFlussLeer));
        ImGui::End();
        return;
    }
    const Fluss& f = flussAktuell();
    int unerreichbar = 0, sack = 0, ohne = 0, fehlt = 0, echte = 0, maxEbene = 0;
    for (const FlussMenue& n : f.menues) {
        if (n.datei < 0) {
            ++fehlt;
            continue;
        }
        ++echte;
        unerreichbar += n.erreichbar ? 0 : 1;
        sack += n.sackgasse ? 1 : 0;
        ohne += (n.ohneZurueck && !n.sackgasse) ? 1 : 0;
        maxEbene = std::max(maxEbene, n.ebene);
    }
    {
        std::vector<char> b(512);
        std::snprintf(b.data(), b.size(), tr(Str::MeFlussZusammen), echte, unerreichbar, sack, ohne, fehlt);
        ImGui::TextUnformatted(b.data());
    }
    const bool nurDatei = z.docs.size() == 1 && z.docs[0]->satz < 0;
    if (nurDatei) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.75F, 0.3F, 1});
        ImGui::TextWrapped("%s", tr(Str::MeFlussNurDatei));
        ImGui::PopStyleColor();
        if (ImGui::Button(tr(Str::MeMenuesatzOeffnen))) { (void)oeffneMenuesatzFuer(aktivDok); }
    }
    ImGui::TextDisabled("%s", tr(Str::MeFlussLegende));
    // Knoten: Spalte = Schritte vom Einstieg, die nie erreichten (und fehlenden ohne Ebene) rechts
    const float bw = fs * 10, bh = fs * 1.7F, gx = fs * 4, gy = fs * 0.7F, rand = fs * 0.6F;
    const int nieSpalte = maxEbene + 1;
    std::vector<int> zeilen(static_cast<std::size_t>(nieSpalte + 1), 0);
    std::vector<ImVec2> pos(f.menues.size());
    for (std::size_t k = 0; k < f.menues.size(); ++k) {
        const int sp = f.menues[k].ebene >= 0 ? f.menues[k].ebene : nieSpalte;
        const int zl = zeilen[static_cast<std::size_t>(sp)]++;
        pos[k] = ImVec2{rand + static_cast<float>(sp) * (bw + gx), rand + fs * 1.4F + static_cast<float>(zl) * (bh + gy)};
    }
    const float listeH = std::min(fs * 9, ImGui::GetContentRegionAvail().y * 0.35F);
    ImGui::BeginChild("##flussFlaeche", ImVec2{0, std::max(fs * 6, ImGui::GetContentRegionAvail().y - listeH)}, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o{ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};
    if (unerreichbar + fehlt > 0) {
        dl->AddText(ImVec2{o.x + rand + static_cast<float>(nieSpalte) * (bw + gx), o.y + rand}, IM_COL32(160, 160, 160, 255), tr(Str::MeFlussNie));
    }
    // Kanten zuerst (je Paar einmal), dann die Knoepfe darueber
    std::set<std::pair<int, int>> gezeichnet;
    for (const FlussKante& e : f.kanten) {
        if (e.von == e.nach || !gezeichnet.insert({e.von, e.nach}).second) { continue; }
        const ImVec2 a{o.x + pos[static_cast<std::size_t>(e.von)].x + bw, o.y + pos[static_cast<std::size_t>(e.von)].y + bh * 0.5F};
        const ImVec2 b{o.x + pos[static_cast<std::size_t>(e.nach)].x, o.y + pos[static_cast<std::size_t>(e.nach)].y + bh * 0.5F};
        const ImU32 c = IM_COL32(150, 190, 255, 170);
        const float bogen = b.x > a.x ? (b.x - a.x) * 0.5F : fs * 4;
        dl->AddBezierCubic(a, ImVec2{a.x + bogen, a.y}, ImVec2{b.x - bogen, b.y}, b, c, 1.5F);
        dl->AddTriangleFilled(b, ImVec2{b.x - fs * 0.5F, b.y - fs * 0.3F}, ImVec2{b.x - fs * 0.5F, b.y + fs * 0.3F}, c);
    }
    const int aktivMenue = aktivDok != nullptr ? menuDerSeite(*aktivDok) : -1;
    for (std::size_t k = 0; k < f.menues.size(); ++k) {
        const FlussMenue& n = f.menues[k];
        ImVec4 farbe{0.22F, 0.36F, 0.55F, 1};
        if (n.datei < 0 || n.sackgasse) {
            farbe = ImVec4{0.62F, 0.18F, 0.16F, 1};
        } else if (!n.erreichbar) {
            farbe = ImVec4{0.35F, 0.35F, 0.35F, 1};
        } else if (n.ohneZurueck) {
            farbe = ImVec4{0.72F, 0.42F, 0.08F, 1};
        }
        ImGui::SetCursorScreenPos(ImVec2{o.x + pos[k].x, o.y + pos[k].y});
        ImGui::PushStyleColor(ImGuiCol_Button, farbe);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{farbe.x + 0.12F, farbe.y + 0.12F, farbe.z + 0.12F, 1});
        const std::string l = anzeige(n.name.empty() ? std::string("?") : n.name) + "##fk" + std::to_string(k);
        if (ImGui::Button(l.c_str(), ImVec2{bw, bh}) && n.datei >= 0) { flussSpringe(n.datei, n.menu); }
        ImGui::PopStyleColor(2);
        const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        if (n.einstieg) { dl->AddRect(ImVec2{a.x - 2, a.y - 2}, ImVec2{b.x + 2, b.y + 2}, IM_COL32(60, 220, 90, 255), 3.0F, 0, 2.0F); }
        if (n.datei >= 0 && z.docs[static_cast<std::size_t>(n.datei)].get() == aktivDok && n.menu == aktivMenue) {
            dl->AddRect(ImVec2{a.x - 4, a.y - 4}, ImVec2{b.x + 4, b.y + 4}, IM_COL32(255, 255, 255, 230), 3.0F, 0, 1.5F);
        }
        if (ImGui::IsItemHovered()) {
            std::string t = anzeige(n.name);
            if (n.datei >= 0) { t += "   (" + z.docs[static_cast<std::size_t>(n.datei)]->name + ")"; }
            if (n.einstieg) { t += std::string("\n") + tr(Str::MeFlussEinstieg); }
            const std::string p = flussProblem(n);
            if (!p.empty()) { t += "\n" + p; }
            std::string von, nach;
            for (const FlussKante& e : f.kanten) {
                if (e.nach == static_cast<int>(k)) { von += "\n  " + anzeige(f.menues[static_cast<std::size_t>(e.von)].name) + "  (" + anzeige(e.wo) + ")"; }
                if (e.von == static_cast<int>(k)) { nach += "\n  " + anzeige(f.menues[static_cast<std::size_t>(e.nach)].name) + "  (" + anzeige(e.wo) + ")"; }
            }
            if (!von.empty()) { t += std::string("\n") + tr(Str::MeFlussOeffnetAus) + ":" + von; }
            if (!nach.empty()) { t += std::string("\n") + tr(Str::MeFlussOeffnet) + ":" + nach; }
            ImGui::SetTooltip("%s", t.c_str());
        }
    }
    ImGui::Dummy(ImVec2{1, 1});
    ImGui::EndChild();
    // Probleme als Liste (Klick springt)
    ImGui::BeginChild("##flussListe", ImVec2{0, 0}, ImGuiChildFlags_None);
    bool eines = false;
    for (std::size_t k = 0; k < f.menues.size(); ++k) {
        const std::string p = flussProblem(f.menues[k]);
        if (p.empty()) { continue; }
        eines = true;
        ImGui::PushID(static_cast<int>(k));
        if (ImGui::Selectable(p.c_str(), false) && f.menues[k].datei >= 0) { flussSpringe(f.menues[k].datei, f.menues[k].menu); }
        ImGui::PopID();
    }
    if (!eines) { ImGui::TextColored(ImVec4{0.4F, 0.9F, 0.5F, 1}, "%s", tr(Str::MeFlussKeine)); }
    ImGui::EndChild();
    ImGui::End();
}

// --- Punkt 2: Tab-Reihenfolge ------------------------------------------------------------

// Die Tab-Folge des Menues der Seite (Laufzeit nach onOpen mit den Vorschau-Cvars), zwischengespeichert
const std::vector<int>& tabFolgeVon(Dokument& d, int mi) {
    Zustand& z = Z();
    const std::string schluessel = std::to_string(reinterpret_cast<std::uintptr_t>(&d)) + ":" + std::to_string(d.geparst) + ":" + std::to_string(mi);
    if (schluessel != z.tabSchluessel) {
        z.tabSchluessel = schluessel;
        z.tabAlt.clear();
        if (mi >= 0 && static_cast<std::size_t>(mi) < d.datei.menus.size()) {
            Laufzeit l;
            l.start(d.datei);
            l.basis = umgebung(d);
            for (const auto& [k, v] : z.vorschauCvars) { l.setzeCvar(k, v); }
            l.oeffne(d.datei.menus[static_cast<std::size_t>(mi)].window.name);
            z.tabAlt = l.tabReihenfolge(mi);
        }
    }
    return z.tabAlt;
}

// Leiste ueber der Flaeche, solange die Tab-Reihenfolge an ist
void zeichneTabLeiste(Dokument& d) {
    Zustand& z = Z();
    if (!z.tabFolge || z.interaktion) { return; }
    const int mi = menuDerSeite(d);
    const std::vector<int>& alt = tabFolgeVon(d, mi);
    if (z.tabNeuFuer != z.tabSchluessel) {   // anderes Menue oder geaenderter Text: neu beginnen
        z.tabNeu.clear();
        z.tabNeuFuer = z.tabSchluessel;
    }
    if (alt.empty()) {
        ImGui::TextWrapped("%s", tr(Str::MeTabFolgeLeer));
        return;
    }
    std::vector<char> b(512);
    std::snprintf(b.data(), b.size(), tr(Str::MeTabFolgeKlick), static_cast<int>(z.tabNeu.size()) + 1, static_cast<int>(alt.size()));
    ImGui::TextWrapped("%s", b.data());
    if (!z.tabNeu.empty()) {
        if (ImGui::SmallButton(tr(Str::MeTabFolgeNeu))) { z.tabNeu.clear(); }
        ImGui::SameLine();
    }
    if (ImGui::SmallButton(tr(Str::MeTabFolgeFertig))) { z.tabFolge = false; }
}

// Nummern auf der Flaeche: alte Folge blau, schon neu geklickte gruen
void zeichneTabNummern(ImDrawList* dl, Dokument& d, int mi, const Abbildung& ab, ImVec2 o) {
    Zustand& z = Z();
    if (!z.tabFolge || z.interaktion || mi < 0) { return; }
    const std::vector<int>& alt = tabFolgeVon(d, mi);
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    const float r = ImGui::GetFontSize() * 0.62F;
    for (std::size_t k = 0; k < alt.size(); ++k) {
        const Rechteck& q = m.items[static_cast<std::size_t>(alt[k])].window.rect;
        const auto neu = std::find(z.tabNeu.begin(), z.tabNeu.end(), alt[k]);
        const bool gesetzt = neu != z.tabNeu.end();
        const int nr = gesetzt ? static_cast<int>(neu - z.tabNeu.begin()) + 1 : static_cast<int>(k) + 1;
        const ImVec2 c{o.x + ab.x(q.x + q.w) - r - 2, o.y + ab.y(q.y) + r + 2};   // rechts oben: der Text beginnt meist links
        dl->AddRect(ImVec2{o.x + ab.x(q.x), o.y + ab.y(q.y)}, ImVec2{o.x + ab.x(q.x + q.w), o.y + ab.y(q.y + q.h)},
                    gesetzt ? IM_COL32(70, 220, 110, 200) : IM_COL32(90, 160, 255, 160), 0, 0, 1.5F);
        dl->AddCircleFilled(c, r, gesetzt ? IM_COL32(40, 170, 80, 255) : (z.tabNeu.empty() ? IM_COL32(40, 110, 220, 255) : IM_COL32(90, 90, 90, 255)));
        const std::string t = std::to_string(nr);
        const ImVec2 ts = ImGui::CalcTextSize(t.c_str());
        dl->AddText(ImVec2{c.x - ts.x * 0.5F, c.y - ts.y * 0.5F}, IM_COL32(255, 255, 255, 255), t.c_str());
    }
}

// Klick auf die Flaeche bei eingeschalteter Tab-Reihenfolge: das vorderste Element der Folge unter
// der Maus kommt als naechstes; mit dem letzten wird umgestellt (ein Rueckgaengig-Schritt).
// true = Klick verbraucht.
bool tabFolgeKlick(Dokument& d, int mi, float vx, float vy, const Umgebung& u) {
    Zustand& z = Z();
    if (!z.tabFolge || z.interaktion || mi < 0) { return false; }
    const std::vector<int> alt = tabFolgeVon(d, mi);
    for (int i : itemsUnterPunkt(d, mi, vx, vy, u)) {
        if (std::find(alt.begin(), alt.end(), i) == alt.end() || std::find(z.tabNeu.begin(), z.tabNeu.end(), i) != z.tabNeu.end()) { continue; }
        z.tabNeu.push_back(i);
        break;
    }
    if (!alt.empty() && z.tabNeu.size() == alt.size()) {
        std::vector<Ersetzung> e;
        if (itemsNeuOrdnen(d.datei, mi, alt, z.tabNeu, e)) {
            if (!e.empty()) { aendere(std::move(e)); }
            z.meldung = tr(Str::MeTabFolgeGesetzt);
        } else {
            z.meldung = tr(Str::MeTabFolgeGesperrt);
        }
        z.tabNeu.clear();
    }
    return true;
}

// --- Punkt 9: Lesbarkeit ---------------------------------------------------------------------------

// Befunde des Menues der Seite (je Stand, Seite und Sprache der Vorschau zwischengespeichert)
const std::vector<LesbarkeitsBefund>& lesbarkeitVon(Dokument& d, int mi) {
    Zustand& z = Z();
    const std::string schluessel = std::to_string(reinterpret_cast<std::uintptr_t>(&d)) + ":" + std::to_string(d.geparst) + ":" + std::to_string(mi) + ":" +
                                   z.sprache + ":" + std::to_string(z.texteVersion);
    if (schluessel != z.lesbarSchluessel) {
        z.lesbarSchluessel = schluessel;
        std::vector<SprachTexte> sp;
        for (const std::string& s : sprachenListe()) { sp.push_back({s, &texteDerSprache(s)}); }
        z.lesbar = lesbarkeit(d.datei, mi, umgebung(d), sp);
    }
    return z.lesbar;
}

std::string lesbarText(const Menu& m, const LesbarkeitsBefund& b) {
    const std::string n = b.item >= 0 && static_cast<std::size_t>(b.item) < m.items.size() && !m.items[static_cast<std::size_t>(b.item)].window.name.empty()
                              ? m.items[static_cast<std::size_t>(b.item)].window.name
                              : "#" + std::to_string(b.item);
    std::vector<char> t(512);
    switch (b.art) {
        case LesbarArt::SchriftKlein:
            std::snprintf(t.data(), t.size(), tr(Str::MeLesbarKlein), n.c_str(), static_cast<double>(b.wert), static_cast<double>(b.grenze));
            break;
        case LesbarArt::KontrastGering:
            std::snprintf(t.data(), t.size(), tr(Str::MeLesbarKontrast), n.c_str(), static_cast<double>(b.wert), static_cast<double>(b.grenze));
            break;
        case LesbarArt::TextZuLang:
            std::snprintf(t.data(), t.size(), tr(Str::MeLesbarLang), n.c_str(), b.sprache.c_str(), static_cast<double>(b.wert), static_cast<double>(b.grenze));
            break;
    }
    return anzeige(t.data());
}

// Teil der Grenzen-Zeile: Zahl der Hinweise, Tooltip mit allen, Klick waehlt das naechste Element
void zeichneLesbarkeit(Dokument& d) {
    Zustand& z = Z();
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const std::vector<LesbarkeitsBefund>& b = lesbarkeitVon(d, mi);
    std::vector<char> t(128);
    if (b.empty()) {
        std::snprintf(t.data(), t.size(), "%s", tr(Str::MeLesbarGut));
    } else {
        std::snprintf(t.data(), t.size(), tr(Str::MeLesbar), static_cast<int>(b.size()));
    }
    ImGui::PushStyleColor(ImGuiCol_Text, b.empty() ? ImVec4{0.6F, 0.6F, 0.6F, 1} : ImVec4{1, 0.7F, 0.25F, 1});
    if (ImGui::SmallButton((std::string(t.data()) + "##lesbar").c_str()) && !b.empty()) {
        z.lesbarNaechster = (z.lesbarNaechster + 1) % static_cast<int>(b.size());
        waehle(b[static_cast<std::size_t>(z.lesbarNaechster)].item, false);
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        std::string tip = tr(Str::MeLesbarTip);
        const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
        for (std::size_t k = 0; k < b.size() && k < 12; ++k) { tip += "\n- " + lesbarText(m, b[k]); }
        ImGui::SetTooltip("%s", tip.c_str());
    }
}

// --- Punkt 10: Live-Grenzen ----------------------------------------------------------------

// Eine Zeile unter den Seiten: Elemente, Menues, laengstes Skript gegen die Grenzen der Fassung
void zeichneGrenzenZeile(Dokument& d) {
    const Auslastung a = auslastung(d.datei, menuDerSeite(d));
    auto teil = [](Str s, int wert, int grenze) {
        const float anteil = grenze > 0 ? static_cast<float>(wert) / static_cast<float>(grenze) : 0.0F;
        const ImVec4 c = anteil >= 1.0F ? ImVec4{1, 0.35F, 0.3F, 1} : (anteil >= 0.8F ? ImVec4{1, 0.7F, 0.25F, 1} : ImVec4{0.6F, 0.6F, 0.6F, 1});
        std::vector<char> b(128);
        std::snprintf(b.data(), b.size(), tr(s), wert, grenze);
        ImGui::TextColored(c, "%s", b.data());
    };
    ImGui::BeginGroup();
    teil(Str::MeGrenzenItems, a.items, a.g.menuItems);
    ImGui::SameLine(0, ImGui::GetFontSize());
    teil(Str::MeGrenzenMenues, a.menus, a.g.menus);
    ImGui::SameLine(0, ImGui::GetFontSize());
    teil(Str::MeGrenzenSkript, a.skript, a.g.skript);
    ImGui::EndGroup();
    const bool grenzenHover = ImGui::IsItemHovered();
    ImGui::SameLine(0, ImGui::GetFontSize());
    zeichneLesbarkeit(d);   // Punkt 9
    if (grenzenHover) {
        std::string t = tr(Str::MeGrenzenTip);
        if (!a.skriptWo.empty()) {
            std::vector<char> b(512);
            std::snprintf(b.data(), b.size(), tr(Str::MeGrenzenWo), anzeige(a.skriptWo).c_str());
            t += std::string("\n") + b.data();
        }
        ImGui::SetTooltip("%s", t.c_str());
    }
}

// --- Punkt 7: Referenzbild -----------------------------------------------------------------

void refBildLaden(const std::string& platte) {
    Zustand& z = Z();
    z.refFehler.clear();
    std::ifstream f(utf8Pfad(platte), std::ios::binary);
    std::ostringstream t;
    t << f.rdbuf();
    const std::string bytes = t.str();
    const image::Image im = bytes.empty() ? image::Image{} : image::decode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
    if (im.width <= 0 || im.height <= 0) {
        std::vector<char> b(1024);
        std::snprintf(b.data(), b.size(), tr(Str::MeRefFehler), u8(utf8Pfad(platte).filename()).c_str());
        z.refFehler = b.data();
        return;
    }
    z.refTex.tex = render::createTexture(im.rgba.data(), im.width, im.height);
    z.refStand = render::texturStand();
    z.refTex.w = im.width;
    z.refTex.h = im.height;
    z.refPfad = platte;
}

void refBildEntfernen() {
    Zustand& z = Z();
    z.refTex = Textur{};
    z.refPfad.clear();
}

// Das Bild auf die 640x480-Flaeche (unter oder ueber dem Menue)
void zeichneRefBild(ImDrawList* dl, const Abbildung& ab, ImVec2 o, bool oben) {
    Zustand& z = Z();
    if (z.refTex.tex != nullptr && z.refStand != render::texturStand()) { refBildLaden(z.refPfad); }   // Geraet neu: Textur neu
    if (z.refTex.tex == nullptr || z.refOben != oben) { return; }
    const int a = std::clamp(static_cast<int>(z.refDeckkraft * 255.0F + 0.5F), 0, 255);
    dl->AddImage(reinterpret_cast<ImTextureID>(z.refTex.tex), ImVec2{o.x + ab.x(0), o.y + ab.y(0)},
                 ImVec2{o.x + ab.x(kSchirmBreite), o.y + ab.y(kSchirmHoehe)}, ImVec2{0, 0}, ImVec2{1, 1}, IM_COL32(255, 255, 255, a));
}

// Knopf in der Leiste mit Klappfenster (waehlen, Deckkraft, ueber/unter, entfernen)
void zeichneRefKnopf() {
    Zustand& z = Z();
    if (leistenKnopf(tr(Str::MeRefBild), UiIcon::Bildschirm, tr(Str::MeRefBildTip), z.refTex.tex != nullptr)) { ImGui::OpenPopup("##refbild"); }
    if (!ImGui::BeginPopup("##refbild")) { return; }
    if (ImGui::Button(tr(Str::MeRefWaehlen))) {
        const std::string p = platform::openFileDialog(tr(Str::MeRefBild), "Bild (*.png;*.jpg;*.jpeg;*.tga)|*.png;*.jpg;*.jpeg;*.tga|", "");
        if (!p.empty()) { refBildLaden(p); }
    }
    if (!z.refFehler.empty()) { ImGui::TextColored(ImVec4{1, 0.45F, 0.4F, 1}, "%s", z.refFehler.c_str()); }
    if (z.refTex.tex != nullptr) {
        ImGui::TextDisabled("%s", u8(utf8Pfad(z.refPfad).filename()).c_str());
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
        ImGui::SliderFloat(tr(Str::MeRefDeckkraft), &z.refDeckkraft, 0.05F, 1.0F, "%.2f");
        ImGui::Checkbox(tr(Str::MeRefOben), &z.refOben);
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeRefObenTip)); }
        if (ImGui::Button(tr(Str::MeRefEntfernen))) { refBildEntfernen(); }
    }
    ImGui::EndPopup();
}

// --- Rest M5b: Reiter mit ungespeicherten Aenderungen schliessen ------------------------------

void schliesseDokument(int k) {
    Zustand& z = Z();
    if (k < 0 || static_cast<std::size_t>(k) >= z.docs.size()) { return; }
    z.docs.erase(z.docs.begin() + k);
    z.aktiv = std::min(z.aktiv, static_cast<int>(z.docs.size()) - 1);
    z.laufDatei = -1;
}

void zeichneSchliessenFrage() {
    Zustand& z = Z();
    if (z.schliessenDok >= 0 && !ImGui::IsPopupOpen("###schliessenFrage")) { ImGui::OpenPopup("###schliessenFrage"); }
    if (!ImGui::BeginPopupModal((std::string(tr(Str::MeSpeichern)) + "###schliessenFrage").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    const int k = z.schliessenDok;
    if (k < 0 || static_cast<std::size_t>(k) >= z.docs.size()) {
        z.schliessenDok = -1;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }
    Dokument& d = *z.docs[static_cast<std::size_t>(k)];
    std::vector<char> b(1024);
    std::snprintf(b.data(), b.size(), tr(Str::MeSchliessenFrage), d.name.c_str());
    ImGui::TextUnformatted(b.data());
    if (ImGui::Button(tr(Str::MeSpeichern))) {
        std::string ziel = d.platte;
        if (ziel.empty() || imSpielordner(ziel)) {
            ziel = platform::saveFileDialog(tr(Str::MeSpeichern), "Menu (*.menu)|*.menu|", z.ordnerAusgabe, fs::path(d.name).filename().string());
        }
        if (!ziel.empty()) {
            speichern(d, ziel);
            if (d.puffer.text() == d.gespeichert) { schliesseDokument(k); }
            z.schliessenDok = -1;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MeSchliessenVerwerfen))) {
        schliesseDokument(k);
        z.schliessenDok = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::KeCancel)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        z.schliessenDok = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// --- Punkt 4: Stile zentral -------------------------------------------------------------------

// Einen Wert an allen Stellen ersetzen (eine Aenderung = ein Rueckgaengig-Schritt)
void stilAnwenden(Dokument& d, const StilWert& w, const std::vector<std::string>& neu) {
    std::vector<Ersetzung> e;
    if (stilErsetzen(d.datei, w, neu, e) && !e.empty()) { aendere(std::move(e)); }
}

std::vector<std::string> woerter(const std::string& s) {
    std::vector<std::string> v;
    std::istringstream in(s);
    for (std::string w; in >> w;) { v.push_back(w); }
    return v;
}

void zeichneStileFenster(Dokument* d) {
    Zustand& z = Z();
    if (!z.stileOffen || d == nullptr) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 30, fs * 26}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.45F});   // unter der Leiste
    const std::string titel = std::string(tr(Str::MeStile)) + "###menuestile";
    if (!ImGui::Begin(titel.c_str(), &z.stileOffen)) {
        ImGui::End();
        return;
    }
    ImGui::TextWrapped("%s", tr(Str::MeStileTip));
    const std::vector<StilWert> v = stileImMenue(d->datei);
    if (v.empty()) { ImGui::TextDisabled("%s", tr(Str::MeStileLeer)); }
    int gesperrt = 0;
    const Str kopf[] = {Str::MeStilFarben, Str::MeStilSchriften, Str::MeStilGroessen, Str::MeStilHintergruende};
    for (int art = 0; art < 4; ++art) {
        bool hatArt = false;
        for (const StilWert& w : v) { hatArt = hatArt || static_cast<int>(w.art) == art; }
        if (!hatArt) { continue; }
        ImGui::SeparatorText(tr(kopf[art]));
        for (std::size_t i = 0; i < v.size(); ++i) {
            const StilWert& w = v[i];
            if (static_cast<int>(w.art) != art) { continue; }
            gesperrt += w.gesperrt;
            ImGui::PushID(static_cast<int>(i) * 4 + art);
            const std::string id = std::to_string(art) + "|" + w.wert;
            if (w.art == StilArt::Farbe) {
                float c[4] = {0, 0, 0, 1};
                const std::vector<std::string> t = woerter(w.wert);
                for (std::size_t k = 0; k < 4 && k < t.size(); ++k) { c[k] = static_cast<float>(std::atof(t[k].c_str())); }
                if (z.stilAktiv == id) { std::copy(std::begin(z.stilFarbe), std::end(z.stilFarbe), c); }
                if (ImGui::ColorEdit4("##farbe", c, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float)) {
                    z.stilAktiv = id;
                    std::copy(c, c + 4, z.stilFarbe);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    std::vector<std::string> neu;
                    for (const float x : c) {
                        char b[32];
                        std::snprintf(b, sizeof(b), "%g", static_cast<double>(std::round(x * 1000.0F) / 1000.0F));
                        neu.emplace_back(b);
                    }
                    z.stilAktiv.clear();
                    stilAnwenden(*d, w, neu);
                }
            } else if (w.art == StilArt::Textgroesse) {
                float x = z.stilAktiv == id ? z.stilFarbe[0] : static_cast<float>(std::atof(w.wert.c_str()));
                ImGui::SetNextItemWidth(fs * 6);
                if (ImGui::DragFloat("##groesse", &x, 0.01F, 0.05F, 4.0F, "%.2f")) {
                    z.stilAktiv = id;
                    z.stilFarbe[0] = x;
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    char b[32];
                    std::snprintf(b, sizeof(b), "%g", static_cast<double>(std::round(x * 100.0F) / 100.0F));
                    z.stilAktiv.clear();
                    stilAnwenden(*d, w, {b});
                }
            } else {
                if (z.stilAktiv != id) { z.stilText = anzeige(w.wert); }
                ImGui::SetNextItemWidth(fs * (w.art == StilArt::Schrift ? 6.0F : 14.0F));
                if (eingabe("##text", &z.stilText)) { z.stilAktiv = id; }
                if (ImGui::IsItemDeactivatedAfterEdit() && !z.stilText.empty()) {
                    z.stilAktiv.clear();
                    stilAnwenden(*d, w, {utf8ZuLatin1(z.stilText.c_str())});
                }
            }
            ImGui::SameLine();
            std::string wo;
            for (const std::string& k : w.schluessel) { wo += (wo.empty() ? "" : ", ") + k; }
            std::vector<char> b(256);
            std::snprintf(b.data(), b.size(), tr(Str::MeStilAnzahl), w.anzahl, wo.c_str());
            ImGui::TextDisabled("%s  %s", w.art == StilArt::Farbe ? w.wert.c_str() : "", b.data());
            ImGui::PopID();
        }
    }
    if (gesperrt > 0) {
        std::vector<char> b(256);
        std::snprintf(b.data(), b.size(), tr(Str::MeStilGesperrt), gesperrt);
        ImGui::TextDisabled("%s", b.data());
    }
    ImGui::End();
}

// --- Punkt 6: Einstellungsseiten-Assistent -----------------------------------------------------

// Die Steuerungen aus den Menues des Spiels (Menuesatz des Mod-Profils fuer den Dialekt), je Spiel/Dialekt einmal
const std::vector<CvarSteuerung>& assistentKatalog(Dialekt dl) {
    Zustand& z = Z();
    const std::string fuer = std::to_string(static_cast<int>(dl)) + "|" + aktivesProfil()->name + "|" + std::to_string(static_cast<int>(z.spiel)) + "|" + std::to_string(z.daten.ordner.size());
    if (fuer == z.assistentFuer) { return z.assistentKatalog; }
    z.assistentFuer = fuer;
    std::vector<Datei> dateien;
    for (const std::string& liste : menuelisten(*aktivesProfil(), dl)) {
        std::string t;
        if (!z.daten.lies(liste, t)) { continue; }
        const Datei l = lies(liste, t, leseOptionen(liste));
        for (const std::string& m : l.loadMenus) {
            std::string mt;
            if (z.daten.lies(m, mt)) { dateien.push_back(lies(m, mt, leseOptionen(m))); }
        }
        break;
    }
    std::vector<const Datei*> zeiger;
    for (const Datei& d : dateien) { zeiger.push_back(&d); }
    z.assistentKatalog = cvarSteuerungen(zeiger);
    return z.assistentKatalog;
}

std::string steuerungArt(const CvarSteuerung& c) {
    std::vector<char> b(128);
    if (c.typ == ITEM_TYPE_SLIDER) {
        std::snprintf(b.data(), b.size(), tr(Str::MeAssistentArtSchieber), static_cast<double>(c.min), static_cast<double>(c.max));
    } else if (c.typ == ITEM_TYPE_MULTI) {
        std::snprintf(b.data(), b.size(), tr(Str::MeAssistentArtAuswahl), static_cast<int>(c.liste.size()));
    } else {
        return tr(Str::MeAssistentArtJaNein);
    }
    return b.data();
}

// Seite aus der Auswahl in das Dokument (ein Rueckgaengig-Schritt)
void assistentAnlegen(Dokument& d) {
    Zustand& z = Z();
    std::vector<CvarSteuerung> wahl;
    for (const CvarSteuerung& c : assistentKatalog(d.datei.dialekt)) {
        if (z.assistentWahl.count(kleinPfad(c.cvar)) != 0) { wahl.push_back(c); }
    }
    if (wahl.empty()) { return; }
    const std::string nl = d.puffer.zeilenUmbruch();
    menuEinfuegen(d, "settingsMenu", [&](const std::string& name) { return einstellungsSeite(wahl, name, "\t", nl); });
    z.assistentWahl.clear();
}

void zeichneAssistentFenster(Dokument* d) {
    Zustand& z = Z();
    if (!z.assistentOffen || d == nullptr) { return; }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{fs * 44, fs * 30}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.45F});   // unter der Leiste
    const std::string titel = std::string(tr(Str::MeAssistent)) + "###menueassistent";
    if (!ImGui::Begin(titel.c_str(), &z.assistentOffen)) {
        ImGui::End();
        return;
    }
    ImGui::TextWrapped("%s", tr(Str::MeAssistentTip));
    const std::vector<CvarSteuerung>& kat = assistentKatalog(d->datei.dialekt);
    if (kat.empty()) {
        ImGui::TextColored(ImVec4{1, 0.7F, 0.3F, 1}, "%s", tr(Str::MeAssistentLeer));
        ImGui::End();
        return;
    }
    ImGui::SetNextItemWidth(fs * 14);
    eingabe(tr(Str::MeAssistentSuche), &z.assistentSuche);
    std::vector<char> b(128);
    std::snprintf(b.data(), b.size(), tr(Str::MeAssistentAnlegen), static_cast<int>(z.assistentWahl.size()));
    ImGui::BeginDisabled(z.assistentWahl.empty());
    if (ImGui::Button(b.data())) { assistentAnlegen(*d); }
    ImGui::EndDisabled();
    const Texte& tx = vorschauTexte(*d);
    const std::string such = kleinPfad(z.assistentSuche);
    // Tabelle statt fester Abstaende: jede Spalte so breit wie ihr laengster Eintrag, beschnitten an der
    // Spaltengrenze (lange Movie-Duels-Schluessel ragten sonst in die naechste Spalte, in allen Sprachen)
    const ImGuiTableFlags tf = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
                               ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("##assistentListe", 3, tf)) {
        ImGui::End();
        return;
    }
    ImGui::TableSetupScrollFreeze(1, 0);
    // Pruefwert fuer den Selbsttest: ragt der sichtbare Teil einer Zelle ueber den Anfang der naechsten?
    int ueberlappung = 0;
    float zellEnde = 0;
    auto zelle = [&](int spalte) {
        ImGui::TableSetColumnIndex(spalte);
        if (spalte > 0 && zellEnde > ImGui::GetCursorScreenPos().x + 0.5F) { ++ueberlappung; }
    };
    auto merkeEnde = [&] { zellEnde = std::min(ImGui::GetItemRectMax().x, ImGui::GetWindowDrawList()->GetClipRectMax().x); };
    for (const CvarSteuerung& c : kat) {
        const std::string text = anzeige(c.text.empty() || c.text[0] != '@' ? c.text : tx.aufloesen(c.text));
        if (!such.empty() && kleinPfad(c.cvar).find(such) == std::string::npos && kleinPfad(text).find(such) == std::string::npos) { continue; }
        const std::string k = kleinPfad(c.cvar);
        bool an = z.assistentWahl.count(k) != 0;
        const std::string l = c.cvar + "##as";
        ImGui::TableNextRow();
        zelle(0);
        if (ImGui::Checkbox(l.c_str(), &an)) {
            if (an) {
                z.assistentWahl.insert(k);
            } else {
                z.assistentWahl.erase(k);
            }
        }
        if (ImGui::IsItemHovered()) {
            std::vector<char> h(512);
            std::snprintf(h.data(), h.size(), tr(Str::MeAssistentHerkunft), c.herkunft.c_str());
            ImGui::SetTooltip("%s", h.data());
        }
        merkeEnde();
        zelle(1);   // die kurze Art vor dem Text, damit sie ohne Rollen sichtbar bleibt
        ImGui::TextDisabled("%s", steuerungArt(c).c_str());
        merkeEnde();
        zelle(2);
        ImGui::TextUnformatted(text.c_str());
        if (ImGui::GetItemRectMax().x > ImGui::GetWindowDrawList()->GetClipRectMax().x && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", text.c_str()); }
    }
    z.assistentUeberlappung = ueberlappung;
    ImGui::EndTable();
    ImGui::End();
}

// --- Punkt 8: Vergleich mit dem Original ----------------------------------------------------------

// Die Originaldatei des Spiels zum Dokument (nur gelesen, je Name und Spiel zwischengespeichert)
const Datei* originalVon(const Dokument& d) {
    Zustand& z = Z();
    const std::string fuer = d.name + "|" + std::to_string(static_cast<int>(z.spiel)) + "|" + std::to_string(z.daten.ordner.size());
    if (fuer != z.originalFuer) {
        z.originalFuer = fuer;
        z.originalDa = false;
        std::string t;
        if (z.daten.lies(d.name, t)) {
            z.original = lies(d.name, t, leseOptionen(d.name));
            z.originalDa = true;
        }
    }
    return z.originalDa ? &z.original : nullptr;
}

// Abschnitt in den Eigenschaften: gewaehltes Element gegen das Original, Zuruecksetzen
void zeichneOriginal(Dokument& d) {
    if (d.auswahl.size() != 1) { return; }
    const Datei* o = originalVon(d);
    const int mi = menuDerSeite(d);
    if (o == nullptr || mi < 0) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    const int ii = d.auswahl[0];
    if (ii < 0 || static_cast<std::size_t>(ii) >= m.items.size()) { return; }
    const Item& it = m.items[static_cast<std::size_t>(ii)];
    int omi = -1, oii = -1;
    for (std::size_t k = 0; k < o->menus.size() && omi < 0; ++k) {
        if (!gleichOhneFallStr(o->menus[k].window.name, m.window.name)) { continue; }
        omi = static_cast<int>(k);
        for (std::size_t j = 0; j < o->menus[k].items.size(); ++j) {
            if (!it.window.name.empty() && o->menus[k].items[j].window.name == it.window.name) {
                oii = static_cast<int>(j);
                break;
            }
        }
    }
    if (omi < 0) { return; }
    ImGui::SeparatorText(tr(Str::MeOriginal));
    if (oii < 0) {
        ImGui::TextWrapped("%s", tr(Str::MeOriginalNeu));
        return;
    }
    const std::vector<std::string> u = itemUnterschiede(d.datei, it, *o, o->menus[static_cast<std::size_t>(omi)].items[static_cast<std::size_t>(oii)]);
    if (u.empty()) {
        ImGui::TextDisabled("%s", tr(Str::MeOriginalGleich));
        return;
    }
    std::string liste;
    for (const std::string& k : u) { liste += (liste.empty() ? "" : ", ") + k; }
    std::vector<char> b(1024);
    std::snprintf(b.data(), b.size(), tr(Str::MeOriginalAnders), liste.c_str());
    ImGui::TextWrapped("%s", b.data());
    if (ImGui::Button(tr(Str::MeOriginalZuruecksetzen))) {
        std::vector<Ersetzung> e;
        if (itemErsetzen(d.datei, mi, ii, itemText(*o, omi, oii), e)) { aendere(std::move(e)); }
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeOriginalZuruecksetzenTip)); }
}

}  // namespace
