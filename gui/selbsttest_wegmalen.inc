// selbsttest_wegmalen.inc - rc609 W3b: Bereich und Pfad malen, Startpunkt per Klick (BHED_EDITORTEST=wegmalen)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach selbsttest_wegerzeugung.inc: die Messung we::messen).
//
// Mike (03.11.): "more control over where the waypoints are generated ... highlight/draw an area and then click
// generate. Like paint a path around the map." Karte aus BHED_WE_PK3/BHED_WE_KARTE wie wegerzeugung. Alles mit der Maus
// in der Draufsicht, wie ein Anwender:
//   1. Bereich A (Pinsel) und "Hier erzeugen": Punkte nur im Bereich, untereinander verbunden; Bereich B daneben: seine
//      Punkte haengen am Netz von A (Auto-Verbinden, Kantenpruefung des Spiels)
//   2. Sperrbereich (Lasso) und volle Erzeugung: kein neuer Punkt darin
//   3. Pfad: Punkte entlang der Linie, verbunden, wo die Kantenpruefung des Spiels besteht
//   4. Startpunkt per Klick: ein Teil, den das Netz nicht erreichte, ist danach ueber die Wegfindung des Spiels erreichbar
//   5. "Netz pruefen": ein Wegpunkt auf einem Boden aus einer Brush-Entity gilt nicht als schwebend (wie die Engine)
//   6. Nicht angebundene Teile: violett gezaehlt, "Anschliessen" bindet sie an
// Je Aktion genau ein Rueckgaengig-Schritt; die Bereiche kommen mit der Karte wieder.
namespace wm {

std::vector<int> neueWp(std::size_t ab) {
    std::vector<int> aus;
    for (std::size_t i = ab; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname.rfind("wpauto_", 0) == 0) { aus.push_back(static_cast<int>(i)); }
    }
    return aus;
}
std::array<float, 3> bodenVon(int e) {
    float o[3] = {0, 0, 0};
    (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(e)], o);
    return {o[0], o[1], o[2] - 25.0F};
}
// Teile der Wegpunkte (target..target4, targetJump) - wie viele, und ob alle in `liste` in einem liegen.
int teileVon(const std::vector<int>& liste) {
    const std::map<int, std::vector<int>> n = we::netz();
    std::set<int> rest(liste.begin(), liste.end());
    int teile = 0;
    while (!rest.empty()) {
        ++teile;
        std::vector<int> st{*rest.begin()};
        std::set<int> gesehen{st.front()};
        while (!st.empty()) {
            const int i = st.back();
            st.pop_back();
            rest.erase(i);
            const auto it = n.find(i);
            if (it == n.end()) { continue; }
            for (const int j : it->second) {
                if (gesehen.insert(j).second) { st.push_back(j); }
            }
        }
    }
    return teile;
}
// Der Knoten des Spiels (baueNavNetz) an einer Entity.
int knotenVon(const NavNetz& netz, int entity) {
    for (std::size_t k = 1; k < netz.knoten.size(); ++k) {
        if (netz.knoten[k].entity == entity) { return static_cast<int>(k); }
    }
    return -1;
}
ImVec2 schirm(const std::array<float, 3>& b) {
    const float w[3] = {b[0], b[1], b[2] + 1.0F};
    ImVec2 s{0, 0};
    (void)kartenPunktAufSchirm(w, s);
    return s;
}

// Eine gerade Strecke ueber ebenen, zu Fuss erreichbaren Boden: 13 Proben in x im Abstand 64 (768 lang), je 2 Proben
// daneben in y ebenfalls (ein Gang von 256 Breite), alle hoechstens eine Stufe auseinander. Nahe dem Spielerstart zuerst.
struct Strecke {
    std::array<float, 3> a{};   // Boden bei k = 0
    bool da = false;
};
Strecke streckeSuchen(const we::Messung& m) {
    std::map<std::pair<int, int>, std::vector<int>> spalten;
    for (std::size_t i = 0; i < m.proben.size(); ++i) { spalten[{m.proben[i].ix, m.proben[i].iy}].push_back(static_cast<int>(i)); }
    const auto bei = [&](int ix, int iy, float z) -> const we::Probe* {
        const auto it = spalten.find({ix, iy});
        if (it == spalten.end()) { return nullptr; }
        for (const int j : it->second) {
            const we::Probe& p = m.proben[static_cast<std::size_t>(j)];
            if (p.erreichbar && p.normaleZ >= 0.999F && std::fabs(p.boden[2] - z) <= we::kStufe) { return &p; }
        }
        return nullptr;
    };
    float st[3] = {0, 0, 0};
    for (const MapEntity& e : g_app->map.entities) {
        if (e.classname.rfind("info_player", 0) == 0 && entityOrt(e, st)) { break; }
    }
    Strecke best;
    float bestD = 1.0e30F;
    for (const we::Probe& p : m.proben) {
        if (!p.erreichbar || p.normaleZ < 0.999F) { continue; }
        bool gut = true;
        for (int k = 0; k <= 12 && gut; ++k) {
            for (int dy = -2; dy <= 2 && gut; ++dy) { gut = bei(p.ix + k, p.iy + dy, p.boden[2]) != nullptr; }
        }
        if (!gut) { continue; }
        const float d = std::hypot(p.boden[0] - st[0], p.boden[1] - st[1]);
        if (d < bestD) {
            bestD = d;
            best.a = p.boden;
            best.da = true;
        }
    }
    return best;
}

// Draufsicht ueber der Strecke (Kamera um dx versetzt), Raster 128, Pinsel 96.
Schritt draufsicht(std::shared_ptr<Strecke> strecke, float dx) {
    return tu("WM: Draufsicht ueber der Strecke", [=] {
        draufsichtSchalten(false);
        g_app->cam.pos[0] = strecke->a[0] + dx;
        g_app->cam.pos[1] = strecke->a[1];
        g_app->cam.pos[2] = strecke->a[2] + 64.0F;
        draufsichtSchalten(true);
        g_app->wegAbstand = 128.0F;
        g_app->wegPinsel = 96.0F;
        // Das Fenster an den rechten Rand der Ansicht: die Striche liegen links der Mitte (ein Klick auf das Fenster
        // erreicht die Ansicht nicht).
        if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) {
            ImGui::SetWindowPos(f, ImVec2{g_app->kartenAnsicht[2] - f->Size.x - 8.0F, g_app->kartenAnsicht[1] + 8.0F});
        }
    });
}
// Der Bildschirmpunkt des Bodens (dx, dy) neben dem Anfang der Strecke.
std::function<ImVec2()> bei(std::shared_ptr<Strecke> strecke, float dx, float dy) {
    return [=] { return schirm({strecke->a[0] + dx, strecke->a[1] + dy, strecke->a[2]}); };
}
// Ein Strich mit gedrueckter Maustaste von (x0, y0) nach (x1, y1) (Weltkoordinaten relativ zum Anfang der Strecke).
Schritt strich(std::shared_ptr<Strecke> strecke, const std::string& was, float x0, float y0, float x1, float y1) {
    auto von = std::make_shared<ImVec2>();
    return Schritt{"Strich " + was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const ImVec2 bis = bei(strecke, x1, y1)();
                       if (b == 0) {
                           *von = bei(strecke, x0, y0)();
                           setzeMaus(*von);
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(0, true);
                           return false;
                       }
                       if (b >= 3 && b <= 26) {
                           const float f = static_cast<float>(b - 2) / 24.0F;
                           setzeMaus(ImVec2{von->x + (bis.x - von->x) * f, von->y + (bis.y - von->y) * f});
                           return false;
                       }
                       if (b == 28) {
                           io.AddMouseButtonEvent(0, false);
                           return false;
                       }
                       return b >= 32;
                   }};
}

}  // namespace wm

std::vector<Schritt> wegmalenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    add(tu("WM: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("WM: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    add(warteBis("WM: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto m = std::make_shared<we::Messung>();
    auto strecke = std::make_shared<wm::Strecke>();
    auto tiefe0 = std::make_shared<std::size_t>(0);
    auto tiefe = std::make_shared<std::size_t>(0);
    auto zahl0 = std::make_shared<std::size_t>(0);
    auto vorText = std::make_shared<std::string>();
    add(tu("WM: Boden messen, eine gerade Strecke ueber ebenen Boden suchen", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAutoVerbinden = true;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        *m = we::messen();
        *strecke = wm::streckeSuchen(*m);
        *tiefe0 = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
        diag::detail("Knopftest: WM - Karte " + g_app->map.path + ", " + std::to_string(m->proben.size()) + " Bodenproben, Strecke ab " +
                     (strecke->da ? we::ortText(strecke->a) : std::string("-")));
    }));
    add(pruefe("WM: eine gerade Strecke ueber ebenen Boden gefunden (13 x 5 Proben)", [=] { return strecke->da; }));
    const auto draufsicht = [=](float dx) { return wm::draufsicht(strecke, dx); };
    const auto bei = [=](float dx, float dy) { return wm::bei(strecke, dx, dy); };
    const auto strich = [=](const std::string& was, float x0, float y0, float x1, float y1) { return wm::strich(strecke, was, x0, y0, x1, y1); };
    const auto fertig = [](const std::string& was) {
        return warteBis("WM: " + was + " fertig gerechnet", [] { return !wegAuftragLaeuft(); }, 20000);
    };

    // --- 1. Bereich A und B ----------------------------------------------------------------
    add(draufsicht(684.0F));
    add(pause(0.6));
    add(klick(tr(Str::WegPinsel), "###wegpunkte"));
    add(pause(0.2));
    add(strich("Bereich A", 0.0F, 0.0F, 256.0F, 0.0F));
    add(pause(0.2));
    add(pruefe("WM Bereich: der Pinselstrich ist ein Bereich (Erzeugen, Raster 128), gespeichert", [=] {
        return g_app->wegBereiche.size() == 1U && g_app->wegBereiche[0].art == WegBereich::Art::Erzeugen && !g_app->wegBereiche[0].lasso &&
               g_app->wegBereiche[0].raster == 128.0F && g_app->wegBereiche[0].punkte.size() >= 5U && !g_app->wegBereicheKarte.empty();
    }));
    add(tu("WM Bereich A: Stand", [=] {
        *tiefe = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
    }));
    add(klick(tr(Str::WegHierErzeugen), "###wegpunkte", 0));
    add(fertig("Bereich A"));
    add(pause(0.3));
    auto inA = std::make_shared<std::vector<int>>();
    add(pruefe("WM Bereich A: Punkte nur im Bereich, untereinander verbunden, ein Rueckgaengig-Schritt", [=] {
        *inA = wm::neueWp(*zahl0);
        int draussen = 0;
        for (const int e : *inA) { draussen += imWegBereich(g_app->wegBereiche[0], wm::bodenVon(e)) ? 0 : 1; }
        const int teile = wm::teileVon(*inA);
        diag::detail("Knopftest: WM - Bereich A: " + g_app->wegMeldung + "; " + std::to_string(inA->size()) + " Punkte, ausserhalb " +
                     std::to_string(draussen) + ", Teile " + std::to_string(teile) + ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe));
        return inA->size() >= 3U && draussen == 0 && teile == 1 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("wm_bereich_a"));
    add(pruefe("WM Bereich: der Pinsel bleibt gewaehlt", [] { return g_app->wegWerkzeug == 3; }));
    add(strich("Bereich B", 384.0F, 0.0F, 640.0F, 0.0F));
    add(pause(0.2));
    add(tu("WM Bereich B: Stand", [=] {
        *tiefe = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
    }));
    add(klick(tr(Str::WegHierErzeugen), "###wegpunkte", 1));
    add(fertig("Bereich B"));
    add(pause(0.3));
    add(pruefe("WM Bereich B: Punkte nur in B, haengen am Netz von A (ein Teil), jede neue Kante besteht die Pruefung des Spiels", [=] {
        const std::vector<int> inB = wm::neueWp(*zahl0);
        int draussen = 0;
        for (const int e : inB) { draussen += imWegBereich(g_app->wegBereiche[1], wm::bodenVon(e)) ? 0 : 1; }
        std::vector<int> alle = *inA;
        alle.insert(alle.end(), inB.begin(), inB.end());
        const int teile = wm::teileVon(alle);
        const NavBauHilfe h = navBauHilfeKarte();
        NavKantenPruefer p(g_app->map.entities, h);
        int schlecht = 0;
        for (const int e : inB) {
            for (const char* f : we::kFelder) {
                const std::string* t = g_app->map.entities[static_cast<std::size_t>(e)].find(f);
                if (t == nullptr) { continue; }
                const int z = w2::indexVon(*t);
                schlecht += (z < 0 || !p.pruefe(e, z).begehbar) ? 1 : 0;
            }
        }
        diag::detail("Knopftest: WM - Bereich B: " + g_app->wegMeldung + "; " + std::to_string(inB.size()) + " Punkte, ausserhalb " +
                     std::to_string(draussen) + ", Teile mit A " + std::to_string(teile) + ", Kanten nicht begehbar " + std::to_string(schlecht) +
                     ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe));
        return !inB.empty() && draussen == 0 && teile == 1 && schlecht == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("wm_bereich_b"));
    add(tu("WM Bereich: Rueckgaengig (B, A)", [=] {
        (void)entityRueckgaengig();
        (void)entityRueckgaengig();
    }));
    add(pruefe("WM Bereich: zwei Mal Rueckgaengig - wie vorher", [=] { return g_app->entUndo.size() == *tiefe0 && wm::neueWp(0).empty(); }));
    add(tu("WM Bereich: Karte neu laden - die Bereiche kommen wieder", [=] {
        g_app->wegBereicheKarte = "-";   // als waere eine andere Karte dazwischen geladen worden
        g_app->wegBereiche.clear();
    }));
    add(pause(0.3));
    add(pruefe("WM Bereich: nach dem Laden zwei Bereiche aus der Datei der Karte", [=] { return g_app->wegBereiche.size() == 2U; }));

    // --- 2. Sperrbereich ----------------------------------------------------------------------
    add(tu("WM Sperre: Bereiche weg, Sperrbereich ein", [=] {
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        g_app->wegNeuSperre = true;
    }));
    add(klick(tr(Str::WegLasso), "###wegpunkte"));
    add(pause(0.2));
    // ein Dreieck ueber dem Gang (das Lasso schliesst sich beim Loslassen)
    add(Schritt{"Lasso Sperrbereich", [=](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    const std::array<std::array<float, 2>, 4> ecken{{{192.0F, -128.0F}, {576.0F, -128.0F}, {384.0F, 128.0F}, {192.0F, -128.0F}}};
                    if (b == 0) {
                        setzeMaus(bei(ecken[0][0], ecken[0][1])());
                        return false;
                    }
                    if (b == 2) {
                        io.AddMouseButtonEvent(0, true);
                        return false;
                    }
                    if (b >= 3 && b <= 38) {
                        const int seg = (b - 3) / 12;
                        const float f = static_cast<float>((b - 3) % 12 + 1) / 12.0F;
                        const ImVec2 p0 = bei(ecken[static_cast<std::size_t>(seg)][0], ecken[static_cast<std::size_t>(seg)][1])();
                        const ImVec2 p1 = bei(ecken[static_cast<std::size_t>(seg) + 1][0], ecken[static_cast<std::size_t>(seg) + 1][1])();
                        setzeMaus(ImVec2{p0.x + (p1.x - p0.x) * f, p0.y + (p1.y - p0.y) * f});
                        return false;
                    }
                    if (b == 40) {
                        io.AddMouseButtonEvent(0, false);
                        return false;
                    }
                    return b >= 44;
                }});
    add(pause(0.2));
    add(pruefe("WM Sperre: ein Sperrbereich (Lasso)", [=] {
        return g_app->wegBereiche.size() == 1U && g_app->wegBereiche[0].art == WegBereich::Art::Sperre && g_app->wegBereiche[0].lasso &&
               g_app->wegBereiche[0].punkte.size() >= 3U;
    }));
    add(tu("WM Sperre: Werkzeug aus, Stand", [=] {
        g_app->wegWerkzeug = 0;
        g_app->wegNeuSperre = false;
        *tiefe = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
    }));
    add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    add(fertig("Erzeugen mit Sperrbereich"));
    add(pause(0.3));
    add(pruefe("WM Sperre: volle Erzeugung, kein neuer Punkt im Sperrbereich, aber Punkte ringsum", [=] {
        const std::vector<int> neu = wm::neueWp(0);
        int drin = 0;
        int nah = 0;
        WegBereich weiter = g_app->wegBereiche[0];
        weiter.lasso = false;
        weiter.pinsel = 256.0F;   // ringsum: bis 256 um die Ecken
        for (const int e : neu) {
            drin += imWegBereich(g_app->wegBereiche[0], wm::bodenVon(e)) ? 1 : 0;
            nah += imWegBereich(weiter, wm::bodenVon(e)) ? 1 : 0;
        }
        diag::detail("Knopftest: WM - Sperre: " + g_app->wegMeldung + "; " + std::to_string(neu.size()) + " Punkte, im Sperrbereich " +
                     std::to_string(drin) + ", ringsum " + std::to_string(nah) + ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe));
        return !neu.empty() && drin == 0 && nah > 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("wm_sperre"));
    add(tu("WM Sperre: Rueckgaengig, Sperrbereich weg", [=] {
        (void)entityRueckgaengig();
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
    }));

    // --- 3. Pfad --------------------------------------------------------------------------
    add(klick(tr(Str::WegPfad), "###wegpunkte"));
    add(pause(0.2));
    add(tu("WM Pfad: Stand", [=] {
        *tiefe = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
    }));
    add(strich("Pfad", 0.0F, 0.0F, 768.0F, 0.0F));
    add(pause(0.4));
    add(pruefe("WM Pfad: Punkte entlang der Linie (Raster 128), Nachbarn verbunden, wo die Kantenpruefung des Spiels besteht", [=] {
        const std::vector<int> neu = wm::neueWp(*zahl0);
        int weitAb = 0;
        for (const int e : neu) {
            const std::array<float, 3> b = wm::bodenVon(e);
            weitAb += (std::fabs(b[1] - strecke->a[1]) > 24.0F || b[0] < strecke->a[0] - 24.0F || b[0] > strecke->a[0] + 792.0F) ? 1 : 0;
        }
        // Je zwei Nachbarn auf der Linie: verbunden genau dann, wenn die Kante in beide Richtungen besteht.
        std::vector<int> nachX = neu;
        std::sort(nachX.begin(), nachX.end(), [](int x, int y) { return wm::bodenVon(x)[0] < wm::bodenVon(y)[0]; });
        const NavBauHilfe h = navBauHilfeKarte();
        NavKantenPruefer p(g_app->map.entities, h);
        int falsch = 0;
        for (std::size_t i = 0; i + 1 < nachX.size(); ++i) {
            const int a = nachX[i];
            const int b = nachX[i + 1];
            const bool geht = p.pruefeOrte({wm::bodenVon(a)[0], wm::bodenVon(a)[1], wm::bodenVon(a)[2] + 25.0F},
                                           {wm::bodenVon(b)[0], wm::bodenVon(b)[1], wm::bodenVon(b)[2] + 25.0F}).begehbar &&
                              p.pruefeOrte({wm::bodenVon(b)[0], wm::bodenVon(b)[1], wm::bodenVon(b)[2] + 25.0F},
                                           {wm::bodenVon(a)[0], wm::bodenVon(a)[1], wm::bodenVon(a)[2] + 25.0F}).begehbar;
            falsch += (geht != w2::kante(a, b)) ? 1 : 0;
        }
        diag::detail("Knopftest: WM - Pfad: " + g_app->wegMeldung + "; " + std::to_string(neu.size()) + " Punkte, abseits der Linie " +
                     std::to_string(weitAb) + ", Nachbarn falsch verbunden " + std::to_string(falsch) + ", Schritte " +
                     std::to_string(g_app->entUndo.size() - *tiefe));
        return neu.size() >= 6U && weitAb == 0 && falsch == 0 && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("wm_pfad"));
    add(tu("WM Pfad: Rueckgaengig, Werkzeug aus", [=] {
        (void)entityRueckgaengig();
        g_app->wegWerkzeug = 0;
    }));

    // --- 4. Startpunkt per Klick ------------------------------------------------------------
    // Erst nur Bereich A erzeugen; die Stelle 640 weiter liegt dann ohne Punkt in Reichweite. Ein Klick dort flutet und
    // bindet an.
    add(klick(tr(Str::WegPinsel), "###wegpunkte"));
    add(pause(0.2));
    add(strich("Bereich A noch einmal", 0.0F, 0.0F, 256.0F, 0.0F));
    add(pause(0.2));
    add(klick(tr(Str::WegHierErzeugen), "###wegpunkte", 0));
    add(fertig("Bereich A"));
    add(tu("WM Startpunkt: Bereich weg, Werkzeug Startpunkt", [=] {
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        *tiefe = g_app->entUndo.size();
        *zahl0 = g_app->map.entities.size();
    }));
    add(pruefe("WM Startpunkt: an der Stelle (Anfang + 704) kein Wegpunkt in einer Rasterweite (vorher nicht erreicht)", [=] {
        const std::array<float, 3> ziel{strecke->a[0] + 704.0F, strecke->a[1], strecke->a[2]};
        for (const we::Punkt& w : we::punkte()) {
            if (std::hypot(std::hypot(w.ort[0] - ziel[0], w.ort[1] - ziel[1]), w.ort[2] - 25.0F - ziel[2]) <= g_app->wegAbstand) { return false; }
        }
        return true;
    }));
    add(klick(tr(Str::WegStartpunkt), "###wegpunkte"));
    add(pause(0.2));
    add(klickAn("Startpunkt", bei(704.0F, 0.0F)));
    add(pause(0.2));
    add(pruefe("WM Startpunkt: der Klick startet die Erzeugung im Hintergrund", [] { return wegAuftragLaeuft() || !g_app->wegMeldung.empty(); }));
    add(fertig("Startpunkt"));
    add(pause(0.3));
    add(pruefe("WM Startpunkt: dort jetzt ein Wegpunkt, die Wegfindung des Spiels kommt von Bereich A hin, ein Rueckgaengig-Schritt", [=] {
        const std::array<float, 3> ziel{strecke->a[0] + 704.0F, strecke->a[1], strecke->a[2]};
        const std::shared_ptr<NavNetz> netz = baueNavNetz(g_app->map.entities, navBauHilfeKarte());
        int an = -1;
        float bestD = g_app->wegAbstand;
        for (const we::Punkt& w : we::punkte()) {
            const float d = std::hypot(std::hypot(w.ort[0] - ziel[0], w.ort[1] - ziel[1]), w.ort[2] - 25.0F - ziel[2]);
            if (d <= bestD) {
                bestD = d;
                an = w.entity;
            }
        }
        const int von = inA->empty() ? -1 : wm::knotenVon(*netz, w2::indexVon(g_app->map.entities[static_cast<std::size_t>(inA->front())].targetname));
        const int nach = an >= 0 ? wm::knotenVon(*netz, an) : -1;
        const bool weg = von > 0 && nach > 0 && (von == nach || navAStar(*netz, von, nach));
        diag::detail("Knopftest: WM - Startpunkt: " + g_app->wegMeldung + "; Punkt in Reichweite " + (an >= 0 ? "ja" : "nein") +
                     ", Wegfindung von A " + (weg ? "ja" : "nein") + ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe));
        return an >= 0 && weg && g_app->entUndo.size() == *tiefe + 1;
    }));
    add(foto("wm_startpunkt"));

    // --- 6. Nicht angebundene Teile -------------------------------------------------------------
    auto allein = std::make_shared<int>(-1);
    auto teileVor = std::make_shared<int>(0);
    add(tu("WM Teile: Werkzeug aus, ein einzelner Wegpunkt 1,5 Rasterweiten neben dem Netz (ohne Auto-Verbinden)", [=] {
        g_app->wegWerkzeug = 0;
        g_app->wegAutoVerbinden = false;
        const float b[3] = {strecke->a[0] + 128.0F, strecke->a[1] - 192.0F, strecke->a[2]};
        *allein = wegSetzen(b, -1);
        g_app->wegAutoVerbinden = true;
        *tiefe = g_app->entUndo.size();
    }));
    add(pause(0.3));
    const auto abseits = [=] {
        return *allein >= 0 && static_cast<std::size_t>(*allein) < g_app->wegAbseits.size() && g_app->wegAbseits[static_cast<std::size_t>(*allein)] != 0;
    };
    add(pruefe("WM Teile: der einzelne Punkt ist nicht angebunden (violett), Knopf Anschliessen da", [=] {
        *teileVor = g_app->wegTeileZahl;
        diag::detail("Knopftest: WM - Teile abseits " + std::to_string(g_app->wegTeileZahl));
        return abseits() && g_app->wegTeileZahl >= 1 && finde(tr(Str::WegAnschliessen), "###wegpunkte", 0, false) != nullptr;
    }));
    add(foto("wm_abseits"));
    add(klick(tr(Str::WegAnschliessen), "###wegpunkte"));
    add(fertig("Anschliessen"));
    add(pause(0.3));
    add(pruefe("WM Teile: nach Anschliessen haengt der Punkt am Hauptteil, ein Teil weniger, ein Rueckgaengig-Schritt", [=] {
        diag::detail("Knopftest: WM - Anschliessen: " + g_app->wegMeldung + "; Teile abseits " + std::to_string(g_app->wegTeileZahl) +
                     ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe));
        return !abseits() && g_app->wegTeileZahl < *teileVor && g_app->entUndo.size() == *tiefe + 1;
    }));

    // --- 5. Netz pruefen: Boden aus einer Brush-Entity -------------------------------------------
    auto aufBrush = std::make_shared<std::array<float, 3>>();
    auto brushDa = std::make_shared<bool>(false);
    add(tu("WM Pruefen: eine Stelle auf einer Brush-Entity suchen, die die Flaechen der Welt nicht sehen, die Spur der Engine aber", [=] {
        draufsichtSchalten(false);
        const NavBauHilfe h = navBauHilfeKarte();
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* mdl = e.find("model");
            if (*brushDa || mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*' || e.classname.rfind("func_", 0) != 0 || e.classname == "func_door") { continue; }
            const int nr = std::atoi(mdl->c_str() + 1);
            if (nr <= 0 || nr >= static_cast<int>(g_app->geo.models.size())) { continue; }
            const auto& sm = g_app->geo.models[static_cast<std::size_t>(nr)];
            float o[3] = {0, 0, 0};
            (void)entityOrt(e, o);
            const std::array<float, 3> oben{(sm.mins[0] + sm.maxs[0]) * 0.5F + o[0], (sm.mins[1] + sm.maxs[1]) * 0.5F + o[1], sm.maxs[2] + o[2] + 25.0F};
            const NavSpur sp = h.spur(oben, {0, 0, 0}, {0, 0, 0}, {oben[0], oben[1], oben[2] - 90.0F}, kNavKeineEntity, kNavSolid | kNavTerrain | kNavMonsterClip);
            const TraceTreffer t = traceRay(g_app->geo, oben.data(), std::array<float, 3>{oben[0], oben[1], oben[2] - 90.0F}.data());
            if (sp.fraction < 1.0F && !t.hit && sp.normal[2] >= 0.7F) {
                *aufBrush = {oben[0], oben[1], sp.endpos[2]};
                *brushDa = true;
                diag::detail("Knopftest: WM - Boden aus " + e.classname + " " + *mdl + " bei " + we::ortText(*aufBrush));
            }
        }
        if (*brushDa) {
            g_app->wegAutoVerbinden = false;
            const float b[3] = {(*aufBrush)[0], (*aufBrush)[1], (*aufBrush)[2]};
            (void)wegSetzen(b, -1);
            g_app->wegAutoVerbinden = true;
        }
    }));
    add(pruefe("WM Pruefen: nachgestellt - mit den Flaechen der Welt allein (wie bis W3a) gilt er als schwebend", [=] {
        if (!*brushDa) {
            diag::detail("Knopftest: WM - keine Brush-Entity mit freiem Boden auf dieser Karte - Pruefung entfaellt");
            return true;
        }
        wegNetzAktuell();
        int schwebt = 0;
        for (const WegBefund& b : pruefeWegNetz(g_app->wegNetz, g_app->map.entities, [](const std::array<float, 3>& a, const std::array<float, 3>& b2,
                                                                                        std::array<float, 3>& t, std::array<float, 3>& nn) {
                 const TraceTreffer r = traceRay(g_app->geo, a.data(), b2.data());
                 for (int k = 0; k < 3; ++k) {
                     t[static_cast<std::size_t>(k)] = r.point[k];
                     nn[static_cast<std::size_t>(k)] = r.normal[k];
                 }
                 return r.hit;
             })) {
            schwebt += (b.art == WegBefund::Art::Schwebend && b.knoten >= 0 &&
                        g_app->wegNetz.knoten[static_cast<std::size_t>(b.knoten)].entity == static_cast<int>(g_app->map.entities.size()) - 1)
                           ? 1
                           : 0;
        }
        diag::detail("Knopftest: WM - ohne Spur der Engine schwebend: " + std::to_string(schwebt));
        return schwebt == 1;
    }));
    add(klick(tr(Str::WegPruefen), "###wegpunkte"));
    add(pause(0.3));
    add(pruefe("WM Pruefen: \"Netz pruefen\" meldet ihn nicht als schwebend (Spur der Engine mit allen Entities)", [=] {
        if (!*brushDa) { return true; }
        int schwebt = 0;
        for (const WegBefund& b : g_app->wegBefunde) {
            schwebt += (b.art == WegBefund::Art::Schwebend && b.knoten >= 0 &&
                        g_app->wegNetz.knoten[static_cast<std::size_t>(b.knoten)].entity == static_cast<int>(g_app->map.entities.size()) - 1)
                           ? 1
                           : 0;
        }
        diag::detail("Knopftest: WM - Netz pruefen: " + std::to_string(g_app->wegBefunde.size()) + " Befunde, der Punkt auf der Brush-Entity schwebend " +
                     std::to_string(schwebt));
        return schwebt == 0 && g_app->wegGeprueft;
    }));
    add(tu("WM: aufraeumen", [=] {
        while (g_app->entUndo.size() > *tiefe0) { (void)entityRueckgaengig(); }
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        draufsichtSchalten(false);
    }));
    return s;
}

// rc609 W3b: die Bilder des Abschnitts "Bereiche, Pfad, Startpunkt, Sprungkanten" (Seite NPC-Wegpunkte), BHED_EDITORTEST=handbuch
// mit BHED_HB_NUR=wegmalen; Karte aus BHED_WE_PK3/BHED_WE_KARTE wie wegmalen (Jedi Temple Garden: Dach mit Sprungkanten).
//   h_weg_bereich     Draufsicht: ein gemalter Bereich und die Punkte darin, das Fenster mit den Werkzeugen
//   h_weg_pfad        Draufsicht: ein gemalter Pfad und seine Punkte
//   h_weg_fortschritt das Fenster waehrend der Erzeugung im Hintergrund (Fortschritt, Abbrechen)
//   h_weg_dach        3D: eine Sprungkante vom Boden aufs Dach (orange gestrichelt)
std::vector<Schritt> wegmalenHandbuchSchritte() {
    std::vector<Schritt> t;
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    t.push_back(tu("Handbuch Wegmalen: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("Handbuch Wegmalen: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    t.push_back(warteBis("Handbuch Wegmalen: geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto m = std::make_shared<we::Messung>();
    auto strecke = std::make_shared<wm::Strecke>();
    t.push_back(tu("Handbuch Wegmalen: messen, Fenster auf", [=] {
        g_app->playing = false;
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(-1);
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        *m = we::messen();
        *strecke = wm::streckeSuchen(*m);
    }));
    t.push_back(pause(0.5));
    t.push_back(wm::draufsicht(strecke, 256.0F));
    // Das Fenster links neben die Ansicht (ueber die Liste der Entities): die Karte bleibt frei, Bild = Ansicht und Fenster.
    const auto fensterLinks = [] {
        return tu("Handbuch Wegmalen: Fenster links neben die Ansicht", [] {
            if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) {
                ImGui::SetWindowPos(f, ImVec2{std::max(4.0F, g_app->kartenAnsicht[0] - f->Size.x - 4.0F), g_app->kartenAnsicht[1]});
            }
        });
    };
    const auto bild = [] { return hb::vereint(hb::karte(), hb::fensterRect("###wegpunkte")); };
    t.push_back(tu("Handbuch Wegmalen: naeher heran (600 ueber dem Schnitt statt 1200)", [] { g_app->cam.pos[2] = g_app->draufSchnitt + 600.0F; }));
    t.push_back(fensterLinks());
    t.push_back(pause(0.6));
    t.push_back(klick(tr(Str::WegPinsel), "###wegpunkte"));
    t.push_back(wm::strich(strecke, "Bereich", 0.0F, 0.0F, 512.0F, 64.0F));
    t.push_back(pause(0.2));
    t.push_back(klick(tr(Str::WegHierErzeugen), "###wegpunkte"));
    t.push_back(wegFertig());
    t.push_back(tu("Handbuch Wegmalen: Maus aus dem Bild", [] { setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F}); }));
    t.push_back(pause(0.8));
    hb::markiere("h_weg_bereich", [] {
        return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegPinsel), tr(Str::WegLasso), tr(Str::WegPfad), tr(Str::WegStartpunkt)}, "###wegpunkte"), 1),
                                      hb::Marke(hb::gruppe({tr(Str::WegSperre)}, "###wegpunkte"), 2),
                                      hb::Marke(hb::gruppe({tr(Str::WegHierErzeugen)}, "###wegpunkte"), 3)};
    });
    t.push_back(hb::fotoTeil("h_weg_bereich", bild));
    t.push_back(tu("Handbuch Wegmalen: zurueck, Bereich weg, Pfad", [] {
        (void)entityRueckgaengig();
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        g_app->wegWerkzeug = 5;
    }));
    t.push_back(wm::strich(strecke, "Pfad", 0.0F, 0.0F, 768.0F, 0.0F));
    t.push_back(tu("Handbuch Wegmalen: Maus aus dem Bild", [] { setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F}); }));
    t.push_back(pause(0.8));
    hb::markiere("h_weg_pfad", [] { return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegPfad)}, "###wegpunkte"), 1)}; });
    t.push_back(hb::fotoTeil("h_weg_pfad", bild));
    t.push_back(tu("Handbuch Wegmalen: zurueck, Werkzeug aus, Blick vom Spielerstart", [] {
        (void)entityRueckgaengig();
        g_app->wegWerkzeug = 0;
        draufsichtSchalten(false);
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if (e.classname == "info_player_start" && entityOrt(e, o)) {
                for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = o[k]; }
                g_app->cam.pos[2] += 300.0F;
                break;
            }
        }
        g_app->cam.angles[0] = 35.0F;
        g_app->wegAbstand = 192.0F;
    }));
    t.push_back(pause(0.5));
    t.push_back(klick(tr(Str::WegErzeugen), "###wegpunkte"));
    t.push_back(warteBis("Handbuch Wegmalen: Fortschritt sichtbar", [] { return wegAuftragAnteil() > 0.15F; }, 3000));
    t.push_back(tu("Handbuch Wegmalen: Maus aus dem Bild", [] { setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F}); }));
    hb::markiere("h_weg_fortschritt", [] { return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegAbbrechen)}, "###wegpunkte"), 1)}; });
    t.push_back(hb::fotoTeil("h_weg_fortschritt", bild));
    t.push_back(wegFertig());
    t.push_back(tu("Handbuch Wegmalen: Startpunkt auf dem Dach", [=] {
        const std::vector<int> dach = we::groesstesDach(*m);
        if (dach.empty()) {
            meldeFehler("Handbuch Wegmalen: kein Dach");
            return;
        }
        (void)wegAuftragStarten(WegAuftragArt::Start, {we::dachMitte(*m, dach)});
    }));
    t.push_back(wegFertig());
    t.push_back(tu("Handbuch Wegmalen: Blick von der Seite auf eine Sprungkante", [] {
        std::map<std::string, std::array<float, 3>> ort;
        for (const MapEntity& e : g_app->map.entities) {
            float o[3];
            if (!e.targetname.empty() && entityOrt(e, o)) { ort[e.targetname] = {o[0], o[1], o[2]}; }
        }
        float best = 0.0F;
        std::array<float, 3> a{};
        std::array<float, 3> b{};
        for (const MapEntity& e : g_app->map.entities) {
            const std::string* j = e.find("targetJump");
            const auto ia = ort.find(e.targetname);
            const auto ib = j != nullptr ? ort.find(*j) : ort.end();
            if (ia == ort.end() || ib == ort.end()) { continue; }
            const float dz = std::fabs(ib->second[2] - ia->second[2]);
            if (dz > best) {
                best = dz;
                a = ia->second;
                b = ib->second;
            }
        }
        if (best <= 0.0F) {
            meldeFehler("Handbuch Wegmalen: keine Sprungkante");
            return;
        }
        const float ziel[3] = {(a[0] + b[0]) * 0.5F, (a[1] + b[1]) * 0.5F, (a[2] + b[2]) * 0.5F};
        float qx = -(b[1] - a[1]);
        float qy = b[0] - a[0];
        const float ql = std::max(1.0F, std::hypot(qx, qy));
        qx /= ql;
        qy /= ql;
        g_app->cam.pos[0] = ziel[0] + qx * 330.0F;
        g_app->cam.pos[1] = ziel[1] + qy * 330.0F;
        g_app->cam.pos[2] = ziel[2];
        aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
        g_app->mapDirty = true;
    }));
    t.push_back(tu("Handbuch Wegmalen: Maus aus dem Bild", [] { setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F}); }));
    t.push_back(pause(0.8));
    t.push_back(hb::fotoTeil("h_weg_dach", bild));
    t.push_back(tu("Handbuch Wegmalen: aufraeumen", [] {
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
    }));
    return t;
}
