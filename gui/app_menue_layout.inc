// app_menue_layout.inc - Menue-Editor M5 Punkt 1: Layouts wie in Qt (in app_menue.cpp eingebunden).
// Nebeneinander, untereinander, Raster, zwei Spalten mit Teiler; Abstand, Raender, Spacer,
// Ausrichtung. Ins .menu kommen nur feste rects; das Layout steht als Kommentarzeile
// "// jkstudio:layout ..." im menuDef (bhed/menue/layout.h). Jede Aenderung ist ein
// Rueckgaengig-Schritt (Layoutzeile und alle rects in einer Ersetzung).

namespace {   // wie die Vorwaertserklaerungen oben in app_menue.cpp

// Breitester Text eines Items ueber alle Sprachen der Spieldaten (StringEd je Sprache)
const Texte& texteDerSprache(const std::string& sprache) {
    Zustand& z = Z();
    const std::string schl = sprache + "|" + std::to_string(z.texteVersion);
    auto it = z.layoutTexte.find(schl);
    if (it == z.layoutTexte.end()) { it = z.layoutTexte.emplace(schl, ladeTexte(z.daten, z.spiel, sprache)).first; }
    return it->second;
}

// Grundzustand: eine eigene Laufzeit oeffnet das Menue wie das Spiel (onOpen)
Laufzeit offeneLaufzeit(Dokument& d, int mi) {
    Laufzeit lz;
    lz.start(d.datei);
    lz.basis = umgebung(d);
    lz.oeffne(d.datei.menus[static_cast<std::size_t>(mi)].window.name);
    return lz;
}

// Sichtbarkeit nach onOpen
std::vector<bool> grundSichtbar(Dokument& d, int mi, const Laufzeit& lz) {
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    std::vector<bool> v(m.items.size(), true);
    // die Flags nach onOpen stehen in der Laufzeit (anwenden uebertraegt nur rects und Texte)
    if (static_cast<std::size_t>(mi) < lz.items.size()) {
        for (std::size_t i = 0; i < v.size() && i < lz.items[static_cast<std::size_t>(mi)].size(); ++i) {
            v[i] = (lz.items[static_cast<std::size_t>(mi)][i].flags & WINDOW_VISIBLE) != 0;
        }
    }
    return v;
}

int itemNamens(const Menu& m, const std::string& name) {
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (m.items[i].window.name == name) { return static_cast<int>(i); }
    }
    return -1;
}

struct LayoutBreite {
    float breite = 0;
    std::string sprache;
};

// Fuer jedes Item: breitester Text und in welcher Sprache
std::map<std::string, LayoutBreite> layoutTextBreiten(Dokument& d, int mi) {
    std::map<std::string, LayoutBreite> r;
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    Umgebung u = umgebung(d);
    for (const std::string& sp : sprachenListe()) {
        const Texte& t = texteDerSprache(sp);
        u.texte = &t;
        for (const Item& it : m.items) {
            if (it.text.empty() || it.window.name.empty()) { continue; }
            const Rechteck tr = textRechteck(it, t.imSpiel(it.text), u);
            // links ausgerichtet beginnt der Text bei textalignx; sonst ist textalignx der Ankerpunkt
            const float b = tr.w + (it.textalignment == 0 ? std::max(0.0F, it.textalignx) : 0.0F);
            LayoutBreite& x = r[it.window.name];
            if (b > x.breite) { x = LayoutBreite{b, sp}; }
        }
    }
    return r;
}

LayoutGroessen layoutGroessen(Dokument& d, int mi, const std::map<std::string, Rechteck>& vorab, const std::map<std::string, LayoutBreite>& breiten,
                              const std::vector<bool>& sichtbar) {
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    LayoutGroessen g;
    g.rect = [&m, vorab](const std::string& n, Rechteck& r) {
        const auto v = vorab.find(n);
        if (v != vorab.end()) {
            r = v->second;
            return true;
        }
        const int i = itemNamens(m, n);
        if (i < 0) { return false; }
        r = m.items[static_cast<std::size_t>(i)].window.rectClient;
        return true;
    };
    g.textBreite = [breiten](const std::string& n) {
        const auto it = breiten.find(n);
        return it == breiten.end() ? 0.0F : it->second.breite;
    };
    g.sichtbar = [&m, sichtbar](const std::string& n) {
        const int i = itemNamens(m, n);
        return i < 0 || static_cast<std::size_t>(i) >= sichtbar.size() || sichtbar[static_cast<std::size_t>(i)];
    };
    return g;
}

// Layouts rechnen und als Ersetzungen schreiben: geaendert != nullptr ersetzt bzw. ergaenzt das Layout
// gleichen Namens (loeschen: Zeile weg, rects bleiben); vorab = rects, die der Anwender gerade setzt.
void layoutsAnwenden(Dokument& d, int mi, const Layout* geaendert, bool loeschen, const std::map<std::string, Rechteck>& vorab,
                     std::vector<Ersetzung>& e) {
    Zustand& z = Z();
    std::vector<Layout> ls = liesLayouts(d.datei, mi);
    if (geaendert != nullptr) {
        auto it = std::find_if(ls.begin(), ls.end(), [&](const Layout& l) { return l.name == geaendert->name; });
        if (loeschen) {
            if (it != ls.end()) { ls.erase(it); }
        } else if (it != ls.end()) {
            *it = *geaendert;
        } else {
            ls.push_back(*geaendert);
        }
        (void)layoutSchreiben(d.datei, mi, *geaendert, loeschen, e);
    }
    const std::map<std::string, LayoutBreite> breiten = layoutTextBreiten(d, mi);
    const Laufzeit offen = offeneLaufzeit(d, mi);
    const std::vector<bool> sicht = grundSichtbar(d, mi, offen);
    const LayoutErgebnis r = rechneLayouts(ls, layoutGroessen(d, mi, vorab, breiten, sicht));
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    std::map<std::string, Rechteck> ziel = vorab;
    for (const auto& [n, rr] : r.rects) { ziel[n] = rr; }
    for (const auto& [n, rr] : ziel) {
        const int i = itemNamens(m, n);
        if (i < 0) { continue; }
        const Rechteck& alt = m.items[static_cast<std::size_t>(i)].window.rectClient;
        if (alt.x == rr.x && alt.y == rr.y && alt.w == rr.w && alt.h == rr.h) { continue; }
        std::vector<Ersetzung> x;
        if (setzeRect(d.datei, mi, i, rr, x)) { e.insert(e.end(), x.begin(), x.end()); }
        else { z.meldung = tr(Str::MeAusMakro); }
    }
    z.layoutWarnungen.clear();
    for (const std::string& n : r.warnungen) {
        const auto b = breiten.find(n);
        char buf[400];
        std::snprintf(buf, sizeof(buf), tr(Str::MeLayoutTextBreit), n.c_str(), b != breiten.end() ? b->second.sprache.c_str() : "",
                      static_cast<double>(b != breiten.end() ? b->second.breite : 0.0F));
        z.layoutWarnungen.push_back(buf);
    }
    for (const std::string& n : r.fehlend) {
        char buf[300];
        std::snprintf(buf, sizeof(buf), tr(Str::MeLayoutFehlt), n.c_str());
        z.layoutWarnungen.push_back(buf);
    }
    // M5b: show/hide je Zustand - die Skripte, die Kinder umschalten, setzen die Lage danach
    std::vector<std::string> alte;   // Kinder eines gerade aufgeloesten Layouts: ihre setitemrect fallen weg
    if (geaendert != nullptr && loeschen) {
        for (const LayoutKind& k : geaendert->kinder) {
            if (!k.spacer && !k.layout) { alte.push_back(k.name); }
        }
    }
    const std::vector<std::string> befehle = bausteinBefehle(d.datei.dialekt, d.datei.spiel);
    const bool kennt = std::find(befehle.begin(), befehle.end(), "setitemrect") != befehle.end();
    const LayoutZustaende zs = layoutZustaende(d.datei, mi, ls, layoutGroessen(d, mi, ziel, breiten, sicht), offen, kennt, alte);
    e.insert(e.end(), zs.ersetzungen.begin(), zs.ersetzungen.end());
    z.layoutZustaende.clear();
    for (const LayoutZustandSkript& s : zs.skripte) {
        if (s.befehle.empty()) { continue; }
        z.layoutZustaende.push_back((s.item < 0 ? m.window.name : m.items[static_cast<std::size_t>(s.item)].window.name) + ": " + s.schluessel);
    }
    if (zs.lageWechselt && !kennt) {
        char buf[600];
        std::snprintf(buf, sizeof(buf), tr(Str::MeLayoutOhneSetItemRect), d.datei.spiel == Spiel::JK2 ? "Jedi Outcast" : d.datei.spiel == Spiel::MD ? "Movie Duels" : "Jedi Academy");
        z.layoutWarnungen.push_back(buf);
    }
}

void layoutAendern(Dokument& d, int mi, const Layout& l, bool loeschen = false) {
    std::vector<Ersetzung> e;
    layoutsAnwenden(d, mi, &l, loeschen, {}, e);
    aendere(std::move(e));
}

// Neues Layout aus der Auswahl: umschliesst ein gewaehltes Item alle anderen (Hintergrund, Rahmen),
// wird es die Flaeche; sonst die gemeinsame Huelle. Reihenfolge der Kinder nach Lage.
void layoutAusAuswahl(Dokument& d, LayoutArt art) {
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.size() < 2) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    std::vector<int> kinder(d.auswahl.begin(), d.auswahl.end());
    int rahmen = -1;
    for (const int i : kinder) {
        // M5b: Flaeche wird nur ein Element ohne eigenen Inhalt (Panel, Hintergrund) - ein Knopf mit
        // Text oder Aktion, der zufaellig die anderen ueberdeckt (eben eingefuegt), bleibt ein Kind
        const Item& ki = m.items[static_cast<std::size_t>(i)];
        if ((!ki.text.empty() || !ki.action.empty()) && (ki.window.flags & WINDOW_DECORATION) == 0) { continue; }
        const Rechteck& a = ki.window.rectClient;
        bool alle = true;
        for (const int k : kinder) {
            if (k == i) { continue; }
            const Rechteck& b = m.items[static_cast<std::size_t>(k)].window.rectClient;
            alle = alle && b.x >= a.x && b.y >= a.y && b.x + b.w <= a.x + a.w && b.y + b.h <= a.y + a.h;
        }
        if (alle) { rahmen = i; }
    }
    if (rahmen >= 0) { kinder.erase(std::find(kinder.begin(), kinder.end(), rahmen)); }
    if (art == LayoutArt::Teiler && kinder.size() != 2) {
        Z().meldung = tr(Str::MeLayoutTeilerZwei);
        return;
    }
    for (const int i : kinder) {
        if (m.items[static_cast<std::size_t>(i)].window.name.empty()) {
            Z().meldung = tr(Str::MeLayoutOhneName);
            return;
        }
    }
    auto rr = [&](int i) { return m.items[static_cast<std::size_t>(i)].window.rectClient; };
    std::stable_sort(kinder.begin(), kinder.end(), [&](int a, int b) {
        if (art == LayoutArt::Senkrecht) { return rr(a).y < rr(b).y; }
        if (art == LayoutArt::Raster && std::fabs(rr(a).y - rr(b).y) > std::min(rr(a).h, rr(b).h) / 2) { return rr(a).y < rr(b).y; }
        return rr(a).x < rr(b).x;
    });
    Layout l;
    l.name = freierLayoutName(liesLayouts(d.datei, mi));
    l.art = art;
    if (rahmen >= 0) {
        l.rahmenItem = m.items[static_cast<std::size_t>(rahmen)].window.name;
        l.rand[0] = l.rand[1] = l.rand[2] = l.rand[3] = 8;
    } else {
        float x0 = 1e9F, y0 = 1e9F, x1 = -1e9F, y1 = -1e9F;
        for (const int i : kinder) {
            x0 = std::min(x0, rr(i).x);
            y0 = std::min(y0, rr(i).y);
            x1 = std::max(x1, rr(i).x + rr(i).w);
            y1 = std::max(y1, rr(i).y + rr(i).h);
        }
        l.rahmen = Rechteck{x0, y0, x1 - x0, y1 - y0};
    }
    l.abstand = 8;
    if (art == LayoutArt::Raster) { l.spalten = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(kinder.size()))))); }
    // M5b: ohne Rahmen mindestens so gross, dass die Kinder mit Abstand hineinpassen (wie die
    // Mindestgroesse eines Qt-Layouts) - sonst schrumpften uebereinander liegende Elemente (etwa
    // drei gerade eingefuegte Knoepfe) auf die Breite eines einzigen
    if (rahmen < 0) {
        float summeW = 0, summeH = 0, maxW = 0, maxH = 0;
        for (const int i : kinder) {
            summeW += rr(i).w;
            summeH += rr(i).h;
            maxW = std::max(maxW, rr(i).w);
            maxH = std::max(maxH, rr(i).h);
        }
        const float n = static_cast<float>(kinder.size());
        float w = l.rahmen.w, h = l.rahmen.h;
        if (art == LayoutArt::Waagrecht) {
            w = std::max(w, summeW + l.abstand * (n - 1));
            h = std::max(h, maxH);
        } else if (art == LayoutArt::Senkrecht) {
            w = std::max(w, maxW);
            h = std::max(h, summeH + l.abstand * (n - 1));
        } else if (art == LayoutArt::Raster) {
            const float sp = static_cast<float>(l.spalten), ze = std::ceil(n / sp);
            w = std::max(w, maxW * sp + l.abstand * (sp - 1));
            h = std::max(h, maxH * ze + l.abstand * (ze - 1));
        } else {
            w = std::max(w, summeW + l.griff);
            h = std::max(h, maxH);
        }
        l.rahmen.w = w;
        l.rahmen.h = h;
        // und im Menue bleiben: was rechts bzw. unten hinausragt, rueckt nach links bzw. oben
        const float mw = m.window.rect.w > 0 ? m.window.rect.w : 640.0F, mh = m.window.rect.h > 0 ? m.window.rect.h : 480.0F;
        if (l.rahmen.x + w > mw) { l.rahmen.x = std::max(0.0F, mw - w); }
        if (l.rahmen.y + h > mh) { l.rahmen.y = std::max(0.0F, mh - h); }
    }
    for (const int i : kinder) { l.kinder.push_back(LayoutKind{m.items[static_cast<std::size_t>(i)].window.name, false, false, 1.0F}); }
    layoutAendern(d, mi, l);
}

// Das Layout, zu dem die Auswahl gehoert (alle gewaehlten im selben), sonst -1
int layoutDerAuswahl(const Dokument& d, const std::vector<Layout>& ls) {
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.empty()) { return -1; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    int li = -2;
    for (const int i : d.auswahl) {
        if (static_cast<std::size_t>(i) >= m.items.size()) { return -1; }
        const int x = layoutVon(ls, m.items[static_cast<std::size_t>(i)].window.name);
        if (x < 0 || (li != -2 && x != li)) { return -1; }
        li = x;
    }
    return li < 0 ? -1 : li;
}

void tipp(Str s) {
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(s)); }
}

void zeichneLayout(Dokument& d) {
    Zustand& z = Z();
    if (!ImGui::CollapsingHeader(tr(Str::MeLayout), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    ImGui::PushID("layout");
    const std::vector<Layout> ls = liesLayouts(d.datei, mi);
    const int li = layoutDerAuswahl(d, ls);
    if (li < 0) {
        // einfach zuerst: vier Knoepfe, die die Auswahl anordnen
        const bool genug = d.auswahl.size() >= 2;
        if (!genug) {
            ImGui::TextWrapped("%s", tr(Str::MeLayoutLeer));
            if (ImGui::Button(tr(Str::MeLayoutZeigMir))) {
                // waehlt die ersten drei sichtbaren Elemente mit Namen - danach sind die Knoepfe frei
                const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
                const std::vector<bool> s = grundSichtbar(d, mi, offeneLaufzeit(d, mi));
                d.auswahl.clear();
                for (std::size_t i = 0; i < m.items.size() && d.auswahl.size() < 3; ++i) {
                    if (s[i] && !m.items[i].window.name.empty() && m.items[i].window.rectClient.w > 0) { d.auswahl.push_back(static_cast<int>(i)); }
                }
                z.meldung = tr(Str::MeLayoutZeigMirTip);
            }
            tipp(Str::MeLayoutZeigMirTip);
        }
        ImGui::BeginDisabled(!genug);
        if (ImGui::Button(tr(Str::MeLayoutWaagrecht))) { layoutAusAuswahl(d, LayoutArt::Waagrecht); }
        tipp(Str::MeLayoutWaagrechtTip);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeLayoutSenkrecht))) { layoutAusAuswahl(d, LayoutArt::Senkrecht); }
        tipp(Str::MeLayoutSenkrechtTip);
        if (ImGui::Button(tr(Str::MeLayoutRaster))) { layoutAusAuswahl(d, LayoutArt::Raster); }
        tipp(Str::MeLayoutRasterTip);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeLayoutTeiler))) { layoutAusAuswahl(d, LayoutArt::Teiler); }
        tipp(Str::MeLayoutTeilerTip);
        ImGui::EndDisabled();
        ImGui::PopID();
        return;
    }
    Layout l = ls[static_cast<std::size_t>(li)];
    bool neu = false;   // nach dem Loslassen der Regler bzw. sofort bei Knoepfen
    ImGui::Text("%s: %s", tr(Str::MeLayout), l.name.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton(tr(Str::MeLayoutAufloesen))) {   // oben: ohne Blaettern erreichbar
        layoutAendern(d, mi, l, true);
        ImGui::PopID();
        return;
    }
    tipp(Str::MeLayoutAufloesenTip);
    const Str arten[] = {Str::MeLayoutWaagrecht, Str::MeLayoutSenkrecht, Str::MeLayoutRaster, Str::MeLayoutTeiler};
    int art = static_cast<int>(l.art);
    if (ImGui::BeginCombo(tr(Str::MeLayoutArt), tr(arten[art]))) {
        for (int a = 0; a < 4; ++a) {
            if (ImGui::Selectable(tr(arten[a]), a == art)) {
                l.art = static_cast<LayoutArt>(a);
                neu = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::DragFloat(tr(Str::MeLayoutAbstand), &l.abstand, 0.5F, 0.0F, 200.0F, "%.0f")) {}
    neu = neu || ImGui::IsItemDeactivatedAfterEdit();
    tipp(Str::MeLayoutAbstandTip);
    if (ImGui::DragFloat4(tr(Str::MeLayoutRaender), l.rand, 0.5F, 0.0F, 200.0F, "%.0f")) {}
    neu = neu || ImGui::IsItemDeactivatedAfterEdit();
    const Str ausr[] = {Str::MeLayoutAnfang, Str::MeLayoutMitte, Str::MeLayoutEnde, Str::MeLayoutFuellen};
    auto ausrichtung = [&](Str titel, LayoutAusrichtung& a) {
        int w = static_cast<int>(a);
        if (ImGui::BeginCombo(tr(titel), tr(ausr[w]))) {
            for (int k = 0; k < 4; ++k) {
                if (ImGui::Selectable(tr(ausr[k]), k == w)) {
                    a = static_cast<LayoutAusrichtung>(k);
                    neu = true;
                }
            }
            ImGui::EndCombo();
        }
    };
    if (l.art != LayoutArt::Teiler) {
        ausrichtung(Str::MeLayoutAusrH, l.h);
        ausrichtung(Str::MeLayoutAusrV, l.v);
    }
    if (l.art == LayoutArt::Raster) {
        if (ImGui::SliderInt(tr(Str::MeLayoutSpalten), &l.spalten, 1, 8)) {}
        neu = neu || ImGui::IsItemDeactivatedAfterEdit();
    }
    if (l.art == LayoutArt::Teiler) {
        if (ImGui::SliderFloat(tr(Str::MeLayoutTeilung), &l.teilung, 0.05F, 0.95F, "%.2f")) {}
        neu = neu || ImGui::IsItemDeactivatedAfterEdit();
        if (ImGui::DragFloat(tr(Str::MeLayoutGriff), &l.griff, 0.5F, 0.0F, 64.0F, "%.0f")) {}
        neu = neu || ImGui::IsItemDeactivatedAfterEdit();
    }
    if (ImGui::Checkbox(tr(Str::MeLayoutVersteckt), &l.versteckteAuslassen)) { neu = true; }
    tipp(Str::MeLayoutVerstecktTip);
    // Kinder: Reihenfolge, Spacer, entfernen
    ImGui::SeparatorText(tr(Str::MeLayoutKinder));
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    for (std::size_t k = 0; k < l.kinder.size(); ++k) {
        ImGui::PushID(static_cast<int>(k));
        const LayoutKind& c = l.kinder[k];
        const std::string zeile = c.spacer ? std::string("<-> ") + tr(Str::MeLayoutSpacer) : c.layout ? "@" + c.name : c.name;
        const int idx = c.spacer || c.layout ? -1 : itemNamens(m, c.name);
        if (ImGui::Selectable(zeile.c_str(), idx >= 0 && std::find(d.auswahl.begin(), d.auswahl.end(), idx) != d.auswahl.end(),
                              ImGuiSelectableFlags_AllowOverlap, ImVec2{ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 4.5F, 0}) &&
            idx >= 0) {
            waehle(idx, ImGui::GetIO().KeyCtrl);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("^") && k > 0) {
            std::swap(l.kinder[k], l.kinder[k - 1]);
            neu = true;
        }
        tipp(Str::MeLayoutHoch);
        ImGui::SameLine();
        if (ImGui::SmallButton("v") && k + 1 < l.kinder.size()) {
            std::swap(l.kinder[k], l.kinder[k + 1]);
            neu = true;
        }
        tipp(Str::MeLayoutRunter);
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            l.kinder.erase(l.kinder.begin() + static_cast<std::ptrdiff_t>(k));
            neu = true;
            ImGui::PopID();
            break;
        }
        tipp(Str::MeLayoutEntfernen);
        ImGui::PopID();
    }
    if (ImGui::Button(tr(Str::MeLayoutSpacerNeu))) {
        // hinter das zuletzt gewaehlte Kind, sonst ans Ende
        std::size_t pos = l.kinder.size();
        for (std::size_t k = 0; k < l.kinder.size(); ++k) {
            const int idx = itemNamens(m, l.kinder[k].name);
            if (idx >= 0 && !d.auswahl.empty() && idx == d.auswahl.back()) { pos = k + 1; }
        }
        l.kinder.insert(l.kinder.begin() + static_cast<std::ptrdiff_t>(pos), LayoutKind{"", true, false, 1.0F});
        neu = true;
    }
    tipp(Str::MeLayoutSpacerTip);
    ImGui::SameLine();
    if (ImGui::Button(tr(Str::MeLayoutNeuRechnen))) { neu = true; }
    tipp(Str::MeLayoutNeuRechnenTip);
    for (const std::string& w : z.layoutWarnungen) { ImGui::TextColored(ImVec4{1, 0.75F, 0.3F, 1}, "%s", w.c_str()); }
    if (!z.layoutZustaende.empty() && l.versteckteAuslassen) {
        ImGui::TextWrapped(trN(Str::MeLayoutZustaende, z.layoutZustaende.size()), static_cast<int>(z.layoutZustaende.size()));
        if (ImGui::IsItemHovered()) {
            std::string liste;
            for (const std::string& x : z.layoutZustaende) { liste += "\n" + x; }
            ImGui::SetTooltip("%s%s", tr(Str::MeLayoutZustaendeTip), liste.c_str());
        }
    }
    if (neu) { layoutAendern(d, mi, l); }
    ImGui::PopID();
}

// Tasten der Flaeche: Strg+Umschalt+1..4 ordnen die Auswahl an
void layoutTasten(Dokument& d) {
    const ImGuiIO& io = ImGui::GetIO();
    if (!io.KeyCtrl || !io.KeyShift) { return; }
    if (ImGui::IsKeyPressed(ImGuiKey_1)) { layoutAusAuswahl(d, LayoutArt::Waagrecht); }
    if (ImGui::IsKeyPressed(ImGuiKey_2)) { layoutAusAuswahl(d, LayoutArt::Senkrecht); }
    if (ImGui::IsKeyPressed(ImGuiKey_3)) { layoutAusAuswahl(d, LayoutArt::Raster); }
    if (ImGui::IsKeyPressed(ImGuiKey_4)) { layoutAusAuswahl(d, LayoutArt::Teiler); }
}

}  // namespace
