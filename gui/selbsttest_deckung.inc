// selbsttest_deckung.inc - rc609 H2: Kampfpunkt-Vorschlaege hinter Deckung (BHED_EDITORTEST=deckung)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach selbsttest_kampfpunkte.inc: kp::kampfpunkte, kp::eintrag).
//
// Mike (03.11.): point_combat "should be sprinkled around randomly (usually behind cover) ... used by gunners to move to
// cover or get a better angle for shooting". Karte aus BHED_WE_PK3/BHED_WE_KARTE wie kampfpunkte. Mit den Knoepfen des
// Wegpunkt-Fensters, wie ein Anwender:
//   1. Vorschlaege entstehen (Netz erzeugt, dann "Vorschlaege erzeugen"), die Karte bleibt unveraendert
//   2. jeder Vorschlag erfuellt die Deckungsregel (NPC_FindCombatPoint CP_CLEAR|CP_COVER) gegen seine Feindstelle -
//      nachgeprueft mit der Spur, mit der die Figuren des Spiels in der Vorschau rechnen (navWeltSpawn, G_ClearLOS)
//   3. Annehmen einzeln und alle: je EIN Rueckgaengig-Schritt, jeder neue point_combat hat die Zuordnung des Spiels (W4:
//      GetNearestNode, nicht "has no waypoint"); zweimal Strg+Z = wie vorher
//   4. Abbrechen aendert nichts
//   5. Nur im gemalten Bereich: alle Vorschlaege darin
// BHED_DK_VERGLEICH=1 (Originalkarten, Punkt 3): die Kampfpunkte der Karte weg (das Wegnetz bleibt), so viele Vorschlaege
// wie es Kampfpunkte waren; wie viele Originale haben einen Vorschlag in einer Rasterweite? Nur gemessen, keine Schwelle.
namespace dk {

std::string entText() {
    std::string s;
    for (const MapEntity& e : g_app->map.entities) {
        for (const auto& [k, v] : e.keys) { s += k + "=" + v + ";"; }
        s += "\n";
    }
    return s;
}
// Die Regel mit der Spur des Spiels: die Welt der Vorschau (navWeltSpawn: Brush-Entities an ihrem Platz zur Laufzeit),
// G_ClearLOS (gClearLos), der Kasten der Figur mit MASK_NPCSOLID.
bool regelImSpiel(const WegDeckung& d, const NavWelt& w) {
    const auto& ents = g_app->map.entities;
    const NavSpurFn spur = w.spur;
    const WegSichtFn sicht = [&](const std::array<float, 3>& a, const std::array<float, 3>& b) {
        return gClearLos(spur, &ents, NavVec{a[0], a[1], a[2]}, NavVec{b[0], b[1], b[2]});
    };
    const WegKastenFn kasten = [&](const std::array<float, 3>& o) {
        const NavVec p{o[0], o[1], o[2]};
        const NavSpur s = spur(p, {-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 40.0F}, p, kNavKeineEntity, kNavMaskNpcSolid);
        return !s.startsolid && !s.allsolid;
    };
    WegDeckungRegeln r;
    if (navJk2Regeln()) {
        r.jk2 = true;
        r.meiden = 0.0F;
        r.sammeln = 512.0F;
    }
    return wegDeckungGilt(sicht, kasten, {d.ort[0], d.ort[1], d.ort[2] + 0.125F}, d.feind, r);
}
Schritt rolle() {
    return tu("DK: Wegpunkt-Fenster nach unten rollen", [] {
        if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) { ImGui::SetScrollY(f, f->ScrollMax.y); }
    });
}

}  // namespace dk

std::vector<Schritt> deckungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    const bool vergleich = std::getenv("BHED_DK_VERGLEICH") != nullptr;
    add(tu("DK: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("DK: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    add(warteBis("DK: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    auto originale = std::make_shared<std::vector<std::array<float, 3>>>();
    add(tu(vergleich ? "DK: Kampfpunkte der Karte weg (Wegnetz bleibt)" : "DK: Wegnetz und Kampfpunkte der Karte weg", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAutoVerbinden = true;
        g_app->wegAbdeckungAn = false;
        g_app->wegDeckNurBereich = false;
        g_app->wegDeckAbstand = 0.0F;
        g_app->wegDeckHoechstens = 0;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        g_app->wegAbstand = 192.0F;
        auto& v = g_app->map.entities;
        for (const MapEntity& e : v) {
            float o[3];
            if (e.classname == "point_combat" && entityOrt(e, o)) { originale->push_back({o[0], o[1], o[2]}); }
        }
        const std::size_t vor = v.size();
        if (vergleich) {
            v.erase(std::remove_if(v.begin(), v.end(), [](const MapEntity& e) { return e.classname == "point_combat"; }), v.end());
            g_app->wegDeckHoechstens = std::max(1, static_cast<int>(originale->size()));
        } else {
            v.erase(std::remove_if(v.begin(), v.end(), [](const MapEntity& e) { return istWegKlasse(e.classname); }), v.end());
        }
        g_app->entUndo.clear();
        int wege = 0;
        for (const MapEntity& e : v) { wege += kp::istWegpunkt(e) ? 1 : 0; }
        diag::detail("Knopftest: DK - Karte " + g_app->map.path + ", " + std::to_string(vor - v.size()) + " Entities entfernt, " +
                     std::to_string(originale->size()) + " Kampfpunkte im Original, " + std::to_string(wege) + " Wegpunkte, Regelwerk " +
                     (navJk2Regeln() ? "Jedi Outcast" : regelwerkMovieDuels() ? "Movie Duels" : "OpenJK"));
    }));
    if (!vergleich) {
        add(klick(tr(Str::WegErzeugen), "###wegpunkte"));
        add(stufe8::warteSek("DK: Erzeugung fertig", [] { return !wegAuftragLaeuft(); }, 600.0));
        add(pause(0.3));
    }
    auto tiefe = std::make_shared<std::size_t>(0);
    auto zahl0 = std::make_shared<std::size_t>(0);
    auto vorher = std::make_shared<std::string>();
    const auto stand = [=](const std::string& was) {
        return tu("DK " + was + ": Stand", [=] {
            g_app->wegWerkzeug = 0;
            *tiefe = g_app->entUndo.size();
            *zahl0 = g_app->map.entities.size();
            *vorher = dk::entText();
        });
    };

    // --- 1. Vorschlaege entstehen -------------------------------------------------------------------
    add(stand("Vorschlaege"));
    add(dk::rolle());
    add(pause(0.3));
    add(klick(tr(Str::WegDeckVorschlagen), "###wegpunkte"));
    add(stufe8::warteSek("DK: Vorschlaege fertig", [] { return !wegDeckungLaeuft(); }, 600.0));
    add(pause(0.3));
    auto erste = std::make_shared<std::vector<WegDeckung>>();
    add(pruefe("DK Vorschlaege: es entstehen Geisterpunkte, die Karte bleibt unveraendert (keine Entity, kein Schritt)", [=] {
        *erste = g_app->wegDeck;
        diag::detail("Knopftest: DK - " + std::to_string(erste->size()) + " Vorschlaege, " + std::to_string(g_app->wegDeckVerworfen) +
                     " verworfen; " + g_app->wegDeckMeldung);
        for (std::size_t i = 0; i < std::min<std::size_t>(erste->size(), 5U); ++i) {
            const WegDeckung& d = (*erste)[i];
            diag::detail("Knopftest: DK - Vorschlag " + std::to_string(i) + " bei " + we::ortText(d.ort) + ": Deckung gegen " + std::to_string(d.feinde) +
                         " Feindstellen, Mitte " + we::ortText(d.feindMitte) + ", Richtung " + std::to_string(static_cast<int>(d.richtung)) +
                         ", Feind " + we::ortText(d.feind));
        }
        return !erste->empty() && g_app->entUndo.size() == *tiefe && dk::entText() == *vorher;
    }));
    if (vergleich) {
        add(pruefe("DK Vergleich: Originale mit einem Vorschlag in einer Rasterweite (nur gemessen)", [=] {
            const float r = g_app->wegAbstand;
            int treffer = 0;
            for (const auto& o : *originale) {
                float best = 1.0e30F;
                for (const WegDeckung& d : *erste) {
                    best = std::min(best, std::hypot(std::hypot(d.ort[0] - o[0], d.ort[1] - o[1]), d.ort[2] - o[2]));
                }
                treffer += best <= r ? 1 : 0;
            }
            char z[300];
            std::snprintf(z, sizeof(z), "Knopftest: DK - Vergleich %s: %d von %zu Originalen mit Vorschlag in %.0f, %zu Vorschlaege", g_app->map.path.c_str(),
                          treffer, originale->size(), static_cast<double>(r), erste->size());
            diag::detail(z);
            return !originale->empty();
        }));
        return s;
    }
    add(pruefe("DK Regel: jeder Vorschlag ist gegen seine Feindstelle gedeckt und schiesst ueber die Deckung - mit der Spur des Spiels", [=] {
        const std::shared_ptr<const NavWelt> w = navWeltSpawn();
        if (!w || !w->spur) { return false; }
        int gut = 0;
        for (const WegDeckung& d : *erste) {
            if (dk::regelImSpiel(d, *w)) {
                ++gut;
            } else {
                diag::detail("Knopftest: DK - Regel verletzt bei " + we::ortText(d.ort) + " gegen " + we::ortText(d.feind));
            }
        }
        diag::detail("Knopftest: DK - Regel mit der Spur des Spiels: " + std::to_string(gut) + " von " + std::to_string(erste->size()));
        return gut == static_cast<int>(erste->size()) && gut > 0;
    }));
    add(foto("dk_vorschlaege"));

    // --- 2. Annehmen: einzeln, dann alle; Zuordnung wie W4 ---------------------------------------------
    add(stand("Annehmen"));
    add(dk::rolle());
    add(pause(0.3));
    add(klick(tr(Str::WegDeckAnnehmen), "###wegpunkte"));
    add(pause(0.4));
    const auto zuordnungGut = [](const std::vector<int>& neu, int& gut) {
        wegKampfAktuell();
        const std::shared_ptr<const NavWelt> w = navWeltSpawn();
        gut = 0;
        if (!w || !w->netz) { return; }
        for (const int e : neu) {
            const NavKampfPunkt* k = kp::eintrag(e);
            if (k == nullptr) { continue; }
            const int nn = navNaechsterKnoten(*w, k->ort, 0, 0);
            const bool ok = !k->ohneWegpunkt && k->zuordnung == nn;
            gut += ok ? 1 : 0;
            if (!ok) {
                diag::detail("Knopftest: DK - Zuordnung Entity " + std::to_string(e) + " bei " + we::ortText(k->ort) + ": Editor " +
                             std::to_string(k->zuordnung) + ", GetNearestNode " + std::to_string(nn) + (k->ohneWegpunkt ? ", has no waypoint" : ""));
            }
        }
    };
    add(pruefe("DK Annehmen: ein point_combat am Ort des ersten Vorschlags, ein Schritt, zugeordnet wie im Spiel (W4)", [=] {
        const std::vector<int> k = kp::kampfpunkte();
        const bool eins = k.size() == 1U && !erste->empty();
        const auto o = eins ? kp::ortVon(k.front()) : std::array<float, 3>{};
        const bool ort = eins && std::fabs(o[0] - (*erste)[0].ort[0]) < 0.5F && std::fabs(o[1] - (*erste)[0].ort[1]) < 0.5F &&
                         std::fabs(o[2] - (*erste)[0].ort[2]) < 0.5F;
        int gut = 0;
        zuordnungGut(k, gut);
        diag::detail("Knopftest: DK - einzeln: " + std::to_string(k.size()) + " Kampfpunkte, Ort " + (ort ? "ja" : "nein") + ", zugeordnet " +
                     std::to_string(gut) + ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe) + ", uebrig " + std::to_string(g_app->wegDeck.size()));
        return eins && ort && gut == 1 && g_app->entUndo.size() == *tiefe + 1 && g_app->wegDeck.size() + 1 == erste->size();
    }));
    add(dk::rolle());
    add(pause(0.3));
    add(klick(tr(Str::WegDeckAlle), "###wegpunkte"));
    add(pause(0.4));
    add(pruefe("DK Alle annehmen: alle uebrigen als point_combat in EINEM Schritt, jeder zugeordnet wie im Spiel", [=] {
        const std::vector<int> k = kp::kampfpunkte();
        int gut = 0;
        zuordnungGut(k, gut);
        diag::detail("Knopftest: DK - alle: " + std::to_string(k.size()) + " Kampfpunkte (" + std::to_string(erste->size()) + " Vorschlaege), zugeordnet " +
                     std::to_string(gut) + ", Schritte " + std::to_string(g_app->entUndo.size() - *tiefe) + ", Geister uebrig " +
                     std::to_string(g_app->wegDeck.size()));
        return k.size() == erste->size() && gut == static_cast<int>(k.size()) && g_app->entUndo.size() == *tiefe + 2 && g_app->wegDeck.empty();
    }));
    add(foto("dk_angenommen"));
    // Fuer den Spielvergleich (BHED_DK_ENT=<Ordner>): die Entities mit den angenommenen Vorschlaegen als .ent und je
    // Kampfpunkt, was der Editor erwartet (Format wie BHED_KP_ENT, W4: kein "has no waypoint").
    if (const char* ordner = std::getenv("BHED_DK_ENT"); ordner != nullptr && *ordner != 0) {
        const std::string o = ordner;
        add(tu("DK Spielvergleich: Entities und Erwartung schreiben", [=] {
            std::string erwartet;
            for (const NavKampfPunkt& k : g_app->wegKampf) {
                char z[160];
                std::snprintf(z, sizeof(z), "%d %d %d %s\n", static_cast<int>(k.ort[0]), static_cast<int>(k.ort[1]), static_cast<int>(k.ort[2]),
                              k.ohneWegpunkt ? "ohne" : "mit");
                erwartet += z;
            }
            const auto schreib = [](const std::string& pfad, const std::string& text) {
                std::ofstream f(::bhed::utf8Pfad(pfad), std::ios::binary | std::ios::trunc);
                f << text;
                return static_cast<bool>(f);
            };
            const bool ok = schreib(o + "\\kp.ent", writeEntities(g_app->map.entities)) && schreib(o + "\\kp_erwartet.txt", erwartet);
            diag::detail("Knopftest: DK - Spielvergleich: " + std::to_string(g_app->wegKampf.size()) + " Kampfpunkte geschrieben (" +
                         (ok ? "ok" : "FEHLER") + ")");
        }));
    }
    add(tu("DK: Maus in die Ansicht", [] { setzeMaus(w2::ansichtMitte()); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("DK Rueckgaengig: zweimal Strg+Z - die Entities wie vor dem Annehmen", [=] {
        return dk::entText() == *vorher && g_app->entUndo.size() == *tiefe && kp::kampfpunkte().empty();
    }));

    // --- 3. Abbrechen aendert nichts -----------------------------------------------------------------
    add(stand("Abbrechen"));
    add(dk::rolle());
    add(pause(0.3));
    add(klick(tr(Str::WegDeckVorschlagen), "###wegpunkte"));
    add(pruefe("DK Abbrechen: die Suche laeuft", [] { return wegDeckungLaeuft(); }));
    add(klick(tr(Str::WegAbbrechen), "###wegpunkte"));
    add(stufe8::warteSek("DK: abgebrochen", [] { return !wegDeckungLaeuft(); }, 600.0));
    add(pause(0.3));
    add(pruefe("DK Abbrechen: Entities bytegleich, kein Schritt, keine Geisterpunkte, Meldung", [=] {
        diag::detail("Knopftest: DK - Abbrechen: " + g_app->wegDeckMeldung);
        return dk::entText() == *vorher && g_app->entUndo.size() == *tiefe && g_app->wegDeck.empty() &&
               g_app->wegDeckMeldung == tr(Str::WegDeckAbgebrochen);
    }));

    // --- 4. Nur im gemalten Bereich ------------------------------------------------------------------
    auto bereich = std::make_shared<WegBereich>();
    add(tu("DK Bereich: ein Pinselstrich (Halbmesser 256) um den ersten Vorschlag, \"Nur in gemalten Bereichen\"", [=] {
        if (erste->empty()) { return; }
        bereich->art = WegBereich::Art::Erzeugen;
        bereich->pinsel = 256.0F;
        bereich->punkte = {(*erste)[0].boden};
        g_app->wegBereiche = {*bereich};
        wegBereicheSpeichern();
    }));
    add(dk::rolle());
    add(pause(0.3));
    add(klick(tr(Str::WegDeckNurBereich), "###wegpunkte"));
    add(pause(0.2));
    add(klick(tr(Str::WegDeckVorschlagen), "###wegpunkte"));
    add(stufe8::warteSek("DK: Vorschlaege im Bereich fertig", [] { return !wegDeckungLaeuft(); }, 600.0));
    add(pause(0.3));
    add(pruefe("DK Bereich: Vorschlaege nur im gemalten Bereich", [=] {
        int aussen = 0;
        for (const WegDeckung& d : g_app->wegDeck) { aussen += imWegBereich(*bereich, d.boden) ? 0 : 1; }
        diag::detail("Knopftest: DK - Bereich: " + std::to_string(g_app->wegDeck.size()) + " Vorschlaege, ausserhalb " + std::to_string(aussen));
        return g_app->wegDeckNurBereich && !g_app->wegDeck.empty() && aussen == 0;
    }));
    add(foto("dk_bereich"));
    add(tu("DK: aufraeumen", [] {
        g_app->wegDeckNurBereich = false;
        wegDeckungVerwerfen();
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
    }));
    return s;
}

// rc609 H2: das Bild des Abschnitts "Vorschlaege hinter Deckung" (Seite NPC-Wegpunkte), BHED_EDITORTEST=handbuch mit
// BHED_HB_NUR=deckung; Karte aus BHED_WE_PK3/BHED_WE_KARTE wie deckung (Jedi Temple Garden).
//   h_weg_deckung    Draufsicht: Geisterpunkte mit dem Pfeil zu ihren Feindstellen, das Fenster mit der Liste
std::vector<Schritt> deckungHandbuchSchritte() {
    std::vector<Schritt> t;
    const char* const kk = std::getenv("BHED_WE_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "jeditemple_garden";
    const char* const pk = std::getenv("BHED_WE_PK3");
    const std::string pk3 = pk != nullptr ? pk : "";
    t.push_back(tu("Handbuch Deckung: Karte " + karte, [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        if (!pk3.empty()) {
            missionenAusArchiv(pk3, karte);
        } else if (!loadMapFromArchive("maps/" + karte + ".bsp")) {
            meldeFehler("Handbuch Deckung: maps/" + karte + ".bsp nicht geladen");
        }
    }));
    t.push_back(warteBis("Handbuch Deckung: geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty(); }, 6000));
    t.push_back(tu("Handbuch Deckung: Netz erzeugen, Vorschlaege suchen", [] {
        g_app->playing = false;
        g_app->throughCamera = false;
        g_app->cameraSelected = false;
        g_app->selectedActor = -1;
        entityAuswahlSetzen(-1);
        g_app->wegFensterOffen = true;
        g_app->zeigeWegnetz = true;
        g_app->wegAbdeckungAn = false;
        g_app->wegDeckNurBereich = false;
        g_app->wegBereiche.clear();
        wegBereicheSpeichern();
        g_app->wegAbstand = 192.0F;
        (void)wegAuftragStarten(WegAuftragArt::Voll, {}, -1, false);
        (void)wegDeckungStarten(false);
        if (g_app->wegDeck.empty()) { meldeFehler("Handbuch Deckung: keine Vorschlaege"); }
    }));
    // Die Draufsicht ueber dem dichtesten Haufen von Vorschlaegen (die meisten Nachbarn in 600).
    t.push_back(tu("Handbuch Deckung: Draufsicht ueber den Vorschlaegen", [] {
        std::size_t best = 0;
        int bestN = -1;
        for (std::size_t i = 0; i < g_app->wegDeck.size(); ++i) {
            int n = 0;
            for (const WegDeckung& d : g_app->wegDeck) {
                n += std::hypot(d.ort[0] - g_app->wegDeck[i].ort[0], d.ort[1] - g_app->wegDeck[i].ort[1]) < 600.0F &&
                             std::fabs(d.ort[2] - g_app->wegDeck[i].ort[2]) < 64.0F
                         ? 1
                         : 0;
            }
            if (n > bestN) {
                bestN = n;
                best = i;
            }
        }
        if (g_app->wegDeck.empty()) { return; }
        kp::ueber(g_app->wegDeck[best].boden);
        g_app->cam.pos[2] = g_app->draufSchnitt + 700.0F;
    }));
    t.push_back(tu("Handbuch Deckung: Fenster links neben die Ansicht, Maus aus dem Bild", [] {
        if (ImGuiWindow* f = ImGui::FindWindowByName("###wegpunkte")) {
            ImGui::SetWindowPos(f, ImVec2{std::max(4.0F, g_app->kartenAnsicht[0] - f->Size.x - 4.0F), g_app->kartenAnsicht[1]});
        }
        setzeMaus(ImVec2{4.0F, ImGui::GetIO().DisplaySize.y - 4.0F});
    }));
    t.push_back(pause(0.5));
    t.push_back(dk::rolle());
    t.push_back(pause(0.5));
    hb::markiere("h_weg_deckung", [] {
        return std::vector<hb::Marke>{hb::Marke(hb::gruppe({tr(Str::WegDeckVorschlagen)}, "###wegpunkte"), 1),
                                      hb::Marke(hb::gruppe({tr(Str::WegDeckAlle)}, "###wegpunkte"), 2)};
    });
    t.push_back(hb::fotoTeil("h_weg_deckung", [] { return hb::vereint(hb::karte(), hb::fensterRect("###wegpunkte")); }));
    return t;
}
