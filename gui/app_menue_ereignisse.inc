// app_menue_ereignisse.inc - Menue-Editor M5 Punkt 2: Ereignisse je Element wie in Unreal UMG
// (in app_menue.cpp eingebunden). Bausteine schreiben den Skripttext des Ereignisses; jede
// Aenderung ist ein Rueckgaengig-Schritt und steht sofort im Text-Reiter.

namespace {

const Str kEreignisNamen[] = {Str::MeEvHover, Str::MeEvHoverEnde, Str::MeEvKlick, Str::MeEvFokus, Str::MeEvFokusEnde, Str::MeEvEnter};

std::string skriptVon(const Item& it, int art) {
    switch (art) {
        case 0: return it.mouseEnter;
        case 1: return it.mouseExit;
        case 2: return it.action;
        case 3: return it.onFocus;
        case 4: return it.leaveFocus;
        default: return it.accept;
    }
}

void ereignisSchreiben(Dokument& d, int item, const char* schluessel, const std::vector<Baustein>& b) {
    std::vector<Ersetzung> e;
    if (skriptSetzen(d.datei, menuDerSeite(d), item, schluessel, zusammensetzen(b), e)) { aendere(std::move(e)); }
    else { ausMakroMelden(); }
}

// Probehoeren ueber denselben Weg wie die Klaenge der Vorschau (Ton-Schalter, stumm im Selbsttest)
void probehoeren(const std::string& klang) {
    if (klang.empty()) { return; }
    Z().lauf.klaenge.push_back(klang);
    spieleKlaenge();
}

// Vorgabe-Woerter eines neuen Bausteins
Baustein neuerBaustein(const std::string& befehl, const Item& it, const Menu& m) {
    Baustein b;
    b.befehl = befehl;
    const std::string& n = it.window.name;
    const Rechteck& r = it.window.rectClient;
    if (befehl == "show" || befehl == "hide" || befehl == "fadein" || befehl == "fadeout" || befehl == "setfocus") { b.args = {n}; }
    else if (befehl == "setitemcolor") { b.args = {n, "forecolor", "1", "1", "1", "1"}; }
    else if (befehl == "setcolor") { b.args = {"forecolor", "1", "1", "1", "1"}; }
    else if (befehl == "setitemrect") { b.args = {n, zahlText(r.x), zahlText(r.y), zahlText(r.w), zahlText(r.h)}; }
    else if (befehl == "setitemtext") { b.args = {n, "text"}; }
    else if (befehl == "transition") {
        b.args = {n, zahlText(r.x), zahlText(r.y), zahlText(r.w), zahlText(r.h), zahlText(r.x - 4), zahlText(r.y - 4), zahlText(r.w + 8),
                  zahlText(r.h + 8), "10", "1"};
    } else if (befehl == "play" || befehl == "playlooped") { b.args = {"sound/interface/button1.wav"}; }
    else if (befehl == "open" || befehl == "close") { b.args = {m.window.name}; }
    else if (befehl == "setcvar") { b.args = {"ui_name", "1"}; }
    else if (befehl == "setcvartocvar") { b.args = {"ui_ziel", "ui_quelle"}; }
    else if (befehl == "uiScript") { b.args = {"character"}; }
    else if (befehl == "exec") { b.args = {"echo hallo"}; }
    b.zitiert.assign(b.args.size(), befehl == "play" || befehl == "playlooped" || befehl == "exec");
    return b;
}

bool argIstItem(const std::string& befehl, std::size_t i) {
    static const char* const mitItem[] = {"show", "hide", "fadein", "fadeout", "setfocus", "setitemcolor", "setitemrect", "setitemtext",
                                          "setitembackground", "transition", "transition2", "orbit"};
    return i == 0 && std::any_of(std::begin(mitItem), std::end(mitItem), [&](const char* c) { return befehl == c; });
}

void zeichneEreignisse(Dokument& d) {
    Zustand& z = Z();
    if (!ImGui::CollapsingHeader(tr(Str::MeEreignisse), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.size() != 1) {
        ImGui::TextDisabled("%s", tr(Str::MeEvEinElement));
        return;
    }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    const int i = d.auswahl[0];
    if (static_cast<std::size_t>(i) >= m.items.size()) { return; }
    const Item& it = m.items[static_cast<std::size_t>(i)];
    ImGui::PushID("ereignisse");
    if (it.window.name.empty()) {
        ImGui::TextWrapped("%s", tr(Str::MeLayoutOhneName));
        ImGui::PopID();
        return;
    }
    // fertige Hover-Effekte - erst einfach, dann mehr
    ImGui::TextUnformatted(tr(Str::MeEvEffekte));
    ImGui::ColorEdit4("##hoverfarbe", &z.hoverFarbe.r, ImGuiColorEditFlags_NoInputs);
    tipp(Str::MeEvFarbeTip);
    const Str eff[] = {Str::MeEvLeuchten, Str::MeEvFarbe, Str::MeEvRahmen, Str::MeEvGroesser};
    const Str effTip[] = {Str::MeEvLeuchtenTip, Str::MeEvFarbeEffTip, Str::MeEvRahmenTip, Str::MeEvGroesserTip};
    for (int k = 0; k < 4; ++k) {
        daneben(knopfBreite(tr(eff[k])));
        if (ImGui::Button(tr(eff[k]))) {
            std::vector<Ersetzung> e;
            if (hoverEffekt(d.datei, mi, i, static_cast<HoverEffekt>(k), z.hoverFarbe, e)) {
                aendere(std::move(e));
                // Leuchten setzt das Glow-Element davor: die Auswahl bleibt beim Element selbst
                if (static_cast<HoverEffekt>(k) == HoverEffekt::Leuchten) { d.auswahl.assign(1, i + 1); }
            } else {
                ausMakroMelden();
            }
        }
        tipp(effTip[k]);
    }
    // Klaenge: Hover = focusSound (Item_SetFocus), Klick = play am Anfang von action
    static std::string fokusKlang, klickKlang;
    static int klangItem = -1;
    std::vector<Baustein> klick = zerlege(it.action, d.datei.dialekt, z.spiel);
    if (klangItem != i || !ImGui::IsAnyItemActive()) {
        klangItem = i;
        fokusKlang = it.focusSound;
        klickKlang.clear();
        for (const Baustein& b : klick) {
            if (b.befehl == "play" && !b.args.empty()) {
                klickKlang = b.args[0];
                break;
            }
        }
    }
    // rc608 Punkt 3: Beschriftung, Feld, Knopf vollstaendig; das Feld wird nicht breiter, als ein Pfad braucht
    const auto klangBreite = [] {
        const float rest = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 1.6F - ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize() * 4.0F, std::min(rest, ImGui::GetFontSize() * 24.0F)));
    };
    feldKopf(tr(Str::MeEvHoverKlang));
    klangBreite();
    textFeld("##hoverklang", &fokusKlang, [i](const std::string& t) { eigenschaft(i, "focusSound", {inAnf(t)}); });
    tipp(Str::MeEvHoverKlangTip);
    ImGui::SameLine();
    if (ImGui::Button(">##hk")) { probehoeren(fokusKlang); }
    tipp(Str::MeEvProbe);
    feldKopf(tr(Str::MeEvKlickKlang));
    klangBreite();
    {
        Dokument* dp = &d;
        textFeld("##klickklang", &klickKlang, [dp, i, klick](const std::string& t) {
            std::vector<Baustein> liste = klick;
            bool gesetzt = false;
            for (Baustein& b : liste) {
                if (b.befehl == "play" && !b.args.empty()) {
                    b.args[0] = t;
                    gesetzt = true;
                    break;
                }
            }
            if (!gesetzt) { liste.insert(liste.begin(), Baustein{"play", {t}, {true}}); }
            if (t.empty()) {
                liste.erase(std::remove_if(liste.begin(), liste.end(), [](const Baustein& b) { return b.befehl == "play"; }), liste.end());
            }
            ereignisSchreiben(*dp, i, "action", liste);
        });
    }
    tipp(Str::MeEvKlickKlangTip);
    ImGui::SameLine();
    if (ImGui::Button(">##kk")) { probehoeren(klickKlang); }
    tipp(Str::MeEvProbe);
    // je Ereignis die Bausteine
    const std::vector<Ereignis> ev = ereignisseFuer(d.datei.dialekt, z.spiel);
    bool irgendwas = false;
    for (const Ereignis& e : ev) {
        ImGui::PushID(e.art);
        std::vector<Baustein> b = zerlege(skriptVon(it, e.art), d.datei.dialekt, z.spiel);
        irgendwas = irgendwas || !b.empty();
        const std::string titel = std::string(tr(kEreignisNamen[e.art])) + " (" + e.schluessel + ")" + (b.empty() ? "" : "  [" + std::to_string(b.size()) + "]");
        if (ImGui::TreeNodeEx(titel.c_str(), b.empty() ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
            bool geaendert = false;
            for (std::size_t k = 0; k < b.size(); ++k) {
                ImGui::PushID(static_cast<int>(k));
                Baustein& x = b[k];
                // rc608 Punkt 3 (shank: "die events scalen nicht gut mit"): ein Block. Erste Zeile: Befehl und Ziel, Hoch und Loeschen immer
                // an fester Stelle rechts. Darunter die Werte, die mit der Breite umbrechen.
                const float fs = ImGui::GetFontSize();
                const float kn = ImGui::GetFrameHeight();
                const float luft = ImGui::GetStyle().ItemSpacing.x;
                const float startX = ImGui::GetCursorPosX();
                const float breit = ImGui::GetContentRegionAvail().x;
                const float rechts = kn * 2.0F + luft;
                const bool mitZiel = !x.args.empty() && argIstItem(x.befehl, 0);
                const bool klang = (x.befehl == "play" || x.befehl == "playlooped") && !x.args.empty();
                const bool zielUnten = mitZiel && ImGui::CalcTextSize(x.befehl.c_str()).x + fs * 5.0F + rechts + luft * 2.0F > breit;
                const auto zielFeld = [&](float b0) {
                    ImGui::SetNextItemWidth(b0);
                    if (ImGui::BeginCombo("##it", x.args[0].c_str())) {
                        for (const Item& o : m.items) {
                            if (!o.window.name.empty() && ImGui::Selectable(o.window.name.c_str(), o.window.name == x.args[0])) {
                                x.args[0] = o.window.name;
                                geaendert = true;
                            }
                        }
                        ImGui::EndCombo();
                    }
                };
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(x.befehl.c_str());
                if (mitZiel && !zielUnten) {
                    ImGui::SameLine();
                    zielFeld(std::max(fs * 3.0F, breit - ImGui::CalcTextSize(x.befehl.c_str()).x - rechts - luft * 2.0F));
                }
                ImGui::SameLine(startX + breit - rechts);
                const ImVec2 kq{kn, kn};
                const bool hoch = ImGui::Button("^", kq);
                tipp(Str::MeLayoutHoch);
                ImGui::SameLine();
                const bool weg = ImGui::Button("x", kq);
                tipp(Str::MeEvEntfernen);
                if (hoch && k > 0) {
                    std::swap(b[k], b[k - 1]);
                    geaendert = true;
                }
                if (weg) {
                    b.erase(b.begin() + static_cast<std::ptrdiff_t>(k));
                    geaendert = true;
                    ImGui::PopID();
                    break;
                }
                if (zielUnten) { zielFeld(-FLT_MIN); }
                Dokument* dp = &d;
                const std::string schluessel = e.schluessel;
                // ab: Stelle in den Woertern; ein Zahlenblock schreibt die Woerter zurueck, die Folge bleibt wie im Spiel
                const auto zahlen = [&](const char* id, std::size_t ab, int n, const char* const* teile, float tempo) {
                    float w[4] = {0, 0, 0, 0};
                    for (int q = 0; q < n; ++q) { w[q] = static_cast<float>(std::atof(x.args[ab + static_cast<std::size_t>(q)].c_str())); }
                    zahlenFeld(id, w, n, [&w, id, n, teile, tempo] { return zahlenReihe(id, w, n, teile, tempo, "%g"); },
                               [dp, i, schluessel, b, k, ab, n](const std::vector<float>& wert) {
                                   std::vector<Baustein> liste = b;
                                   for (int q = 0; q < n; ++q) { liste[k].args[ab + static_cast<std::size_t>(q)] = zahlText(wert[static_cast<std::size_t>(q)]); }
                                   ereignisSchreiben(*dp, i, schluessel.c_str(), liste);
                               });
                };
                if ((x.befehl == "transition" || x.befehl == "transition2") && x.args.size() == 11) {
                    // Script_Transition: Name, von (Rect_Parse), nach (Rect_Parse), Zeit, Schrittweite
                    static const char* const kRect[4] = {"x", "y", "w", "h"};
                    ImGui::TextUnformatted(tr(Str::MeEvVon));
                    zahlen("von", 1, 4, kRect, 1.0F);
                    ImGui::TextUnformatted(tr(Str::MeEvNach));
                    zahlen("nach", 5, 4, kRect, 1.0F);
                    const char* const teile[2] = {tr(Str::MeEvZeit), tr(Str::MeEvSchritte)};
                    zahlen("zeit", 9, 2, teile, 1.0F);
                } else if (x.befehl == "setitemcolor" && x.args.size() == 6) {
                    // Script_SetItemColor: Ziel, Eigenschaft, Farbe
                    static const char* const kEigen[3] = {"forecolor", "backcolor", "bordercolor"};
                    static const char* const kRgba[4] = {"R", "G", "B", "A"};
                    ImGui::SetNextItemWidth(std::min(fs * 9.0F, breit));
                    if (ImGui::BeginCombo("##eigenschaft", x.args[1].c_str())) {
                        for (const char* en : kEigen) {
                            if (ImGui::Selectable(en, x.args[1] == en)) {
                                x.args[1] = en;
                                geaendert = true;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    float c[4];
                    for (int q = 0; q < 4; ++q) { c[q] = static_cast<float>(std::atof(x.args[static_cast<std::size_t>(2 + q)].c_str())); }
                    zahlenFeld("##c", c, 4, [&c] {
                        ImGui::BeginGroup();
                        bool neu = ImGui::ColorEdit4("##bild", c, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_Float);
                        ImGui::SameLine();
                        neu |= zahlenReihe("##c", c, 4, kRgba, 0.01F, "%.2f");
                        ImGui::EndGroup();
                        return neu;
                    }, [dp, i, schluessel, b, k](const std::vector<float>& w) {
                        std::vector<Baustein> liste = b;
                        for (std::size_t q = 0; q < 4; ++q) { liste[k].args[2 + q] = zahlText(w[q]); }
                        ereignisSchreiben(*dp, i, schluessel.c_str(), liste);
                    });
                } else {
                    const float zelle = std::max(fs * 7.0F, std::min(breit, fs * 14.0F));
                    bool erstes = true;
                    for (std::size_t a = mitZiel ? 1 : 0; a < x.args.size(); ++a) {
                        ImGui::PushID(static_cast<int>(a));
                        if (!erstes) { daneben(zelle); }
                        erstes = false;
                        const float rest = ImGui::GetContentRegionAvail().x;
                        const float kk = klang ? kn + luft : 0.0F;
                        ImGui::SetNextItemWidth(std::min(rest - kk, x.args.size() - (mitZiel ? 1U : 0U) == 1U ? fs * 24.0F : zelle));
                        textFeld("##a", &x.args[a], [dp, i, schluessel, b, k, a](const std::string& t) {
                            std::vector<Baustein> liste = b;
                            liste[k].args[a] = t;
                            ereignisSchreiben(*dp, i, schluessel.c_str(), liste);
                        });
                        if (klang && a == 0) {
                            ImGui::SameLine();
                            if (ImGui::Button(">", ImVec2{kn, kn})) { probehoeren(x.args[0]); }
                            tipp(Str::MeEvProbe);
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::SetNextItemWidth(std::min(ImGui::GetFontSize() * 10.0F, ImGui::GetContentRegionAvail().x));
            if (ImGui::BeginCombo("##neu", tr(Str::MeEvAktionNeu))) {
                for (const std::string& c : bausteinBefehle(d.datei.dialekt, z.spiel)) {
                    if (ImGui::Selectable(c.c_str())) {
                        b.push_back(neuerBaustein(c, it, m));
                        geaendert = true;
                    }
                }
                ImGui::EndCombo();
            }
            tipp(Str::MeEvAktionNeuTip);
            if (geaendert) { ereignisSchreiben(d, i, e.schluessel, b); }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (!irgendwas) {
        ImGui::TextWrapped("%s", tr(Str::MeEvLeer));
        if (ImGui::Button(tr(Str::MeLayoutZeigMir))) {
            std::vector<Ersetzung> e;
            if (hoverEffekt(d.datei, mi, i, HoverEffekt::Farbe, z.hoverFarbe, e)) {
                aendere(std::move(e));
                z.meldung = tr(Str::MeEvZeigMirTip);
            }
        }
        tipp(Str::MeEvZeigMirTip);
    }
    ImGui::PopID();
}

}  // namespace
