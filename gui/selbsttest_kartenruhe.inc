// --- rc610 R1: Der Karten-Editor zeichnet nur bei Bedarf (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=ruhen) ---
//
// Wie NRCs queueDraw: ein ruhendes Map-Editor-Fenster zeichnet keine Ansicht neu und zeigt kein neues Bild; jede
// Aenderung zeichnet im naechsten Bild - eine Mausbewegung ueber der Kamera, die Kamera selbst, die Auswahl, ein Befehl
// und sein Rueckgaengig, eine Taste im Editor, die Groesse des Fensters. Das stehen gebliebene Bild ist dasselbe, das
// ein neues Zeichnen ergibt. Streng (zeichnenNachBedarfImTest): auch die Schritte des Tests zaehlen nicht als Eingabe.
namespace kartenruhe {
namespace ke = karteneditor;

struct Zahl {
    std::size_t kamera = 0;
    std::size_t xy[3] = {0, 0, 0};
    unsigned long long fenster = 0;   // gezeigte Bilder des eigenen Fensters
};

Zahl zaehlen() {
    const ke::Editor& e = ke::editor();
    Zahl z;
    z.kamera = e.kamFenster.anlass.bilder;
    for (int i = 0; i < 3; ++i) { z.xy[i] = e.xy[static_cast<std::size_t>(i)].anlass.bilder; }
    if (const ImGuiWindow* w = tabfenster::fenster(3); w != nullptr && w->Viewport != nullptr) { z.fenster = render::viewportBilder(w->Viewport->ID); }
    return z;
}

std::string text(const Zahl& a, const Zahl& b) {
    const ke::Editor& e = ke::editor();
    return "Gruende Kamera \"" + e.kamFenster.anlass.grund + "\" XY \"" + e.xy[0].anlass.grund + "\" - Kamera " + std::to_string(b.kamera - a.kamera) + ", XY " + std::to_string(b.xy[0] - a.xy[0]) + ", XZ " + std::to_string(b.xy[1] - a.xy[1]) +
           ", YZ " + std::to_string(b.xy[2] - a.xy[2]) + ", Fenster " + std::to_string(b.fenster - a.fenster);
}

// Eine Aenderung: vorher zaehlen, `tun`, ein Bild spaeter muessen die Kamera und (falls `auchXy`) die 2D-Ansichten neu
// gezeichnet haben; danach wieder Ruhe (10 Bilder ohne neues Zeichnen).
void aenderung(std::vector<Schritt>& s, const std::string& was, std::function<void()> tun, bool auchXy) {
    auto vor = std::make_shared<Zahl>();
    auto nach = std::make_shared<Zahl>();
    s.push_back(Schritt{"Ruhe: " + was, [=](int b) {
                            if (b == 0) {
                                *vor = zaehlen();
                                tun();
                                return false;
                            }
                            if (b < 3) { return false; }
                            *nach = zaehlen();
                            return true;
                        }});
    s.push_back(pruefe("Ruhe: " + was + " - die Ansichten zeichnen sofort neu", [=] {
        diag::detail("Knopftest: Ruhe: " + was + ": " + text(*vor, *nach));
        bool ok = nach->kamera > vor->kamera;
        // nur die 2D-Ansichten, die die Anordnung zeigt (die anderen zeichnen nie)
        for (int i = 0; i < 3 && auchXy; ++i) { ok = ok && (vor->xy[i] == 0U || nach->xy[i] > vor->xy[i]); }
        return ok;
    }));
    auto ruhig = std::make_shared<Zahl>();
    s.push_back(Schritt{"Ruhe: nach " + was + " 30 Bilder ohne Eingabe", [=](int b) {
                            if (b == 2) { *ruhig = zaehlen(); }
                            if (b < 32) { return false; }
                            *nach = zaehlen();
                            return true;
                        }});
    s.push_back(pruefe("Ruhe: nach " + was + " ruht das Fenster wieder", [=] {
        diag::detail("Knopftest: Ruhe: nach " + was + ": " + text(*ruhig, *nach));
        return nach->kamera == ruhig->kamera && nach->xy[0] == ruhig->xy[0] && nach->xy[1] == ruhig->xy[1] && nach->xy[2] == ruhig->xy[2] &&
               nach->fenster == ruhig->fenster;
    }));
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto pinsel = std::make_shared<std::shared_ptr<karte::Primitiv>>();
    add(tu("Ruhe: Map Editor mit zwei Pinseln, nur bei Bedarf zeichnen (streng)", [pinsel] {
        g_app->leftMode = 3;
        const std::filesystem::path p = std::filesystem::current_path() / "r1_ruhe.map";
        texturliste::schreibe(p, "// entity 0\n{\n\"classname\" \"worldspawn\"\n// brush 0\n" + texturliste::kasten(0, 0, 0, 128, 128, 64, "common/caulk") +
                                     "// brush 1\n" + texturliste::kasten(192, 0, 0, 256, 64, 160, "common/clip") + "}\n");
        (void)ke::karteOeffnen(p.string());
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        ke::auswahlGeaendert();
        for (const auto& k : e.karte.welt()->kinder()) {
            if (k->art() == karte::Primitiv::Art::Pinsel && !*pinsel) { *pinsel = k; }
        }
        e.kamera.ursprung = karte::Vektor3f(-256, -320, 192);
        e.kamera.winkel = karte::Vektor3f(-20, 45, 0);
        ke::zeichnenNachBedarfImTest(true);
    }));
    add(tu("Ruhe: Map Editor in ein eigenes Fenster", [] { (void)tabfenster::loesen(3); }));
    add(pause(1.0));
    add(warteBis("Ruhe: Texturen der Kamera geladen", [] { return ke::kameraBilderFertig(); }, 900));
    add(pause(0.5));
    // --- Ruhe: 120 Bilder ohne Eingabe (das Hauptfenster zeichnet weiter, es taktet die Schleife) ----------------
    auto vor = std::make_shared<Zahl>();
    auto nach = std::make_shared<Zahl>();
    add(Schritt{"Ruhe: 120 Bilder ohne Eingabe", [=](int b) {
                    if (b == 2) { *vor = zaehlen(); }
                    if (b < 122) { return false; }
                    *nach = zaehlen();
                    return true;
                }});
    add(pruefe("Ruhe: das ruhende Map-Editor-Fenster zeichnet keine Ansicht neu und zeigt kein neues Bild (120 Bilder)", [=] {
        std::string gruende;
        if (const ImGuiWindow* w = tabfenster::fenster(3); w != nullptr && w->Viewport != nullptr) { gruende = render::viewportGruende(w->Viewport->ID); }
        diag::detail("Knopftest: Ruhe: 120 Bilder: " + text(*vor, *nach) + " (eigenes Fenster " + (tabfenster::eigenes(3) ? "ja" : "nein") +
                     ", Gruende " + gruende + ")");
        return tabfenster::eigenes(3) && nach->kamera == vor->kamera && nach->xy[0] == vor->xy[0] && nach->xy[1] == vor->xy[1] &&
               nach->xy[2] == vor->xy[2] && nach->fenster == vor->fenster;
    }));
    // --- Das stehende Bild ist das, das ein neues Zeichnen ergibt ----------------------------------------------
    auto bild = std::make_shared<std::vector<unsigned char>>();
    auto bildW = std::make_shared<int>(0);
    auto bildH = std::make_shared<int>(0);
    add(tu("Ruhe: das stehende Kamerabild lesen, dann neu zeichnen lassen", [=] {
        (void)ke::kameraBild(*bild, *bildW, *bildH);
        ke::neuZeichnen();
    }));
    add(pause(0.2));
    add(pruefe("Ruhe: das stehende Kamerabild ist Punkt fuer Punkt das neu gezeichnete", [=] {
        std::vector<unsigned char> neu;
        int w = 0;
        int h = 0;
        const bool da = ke::kameraBild(neu, w, h);
        std::size_t anders = 0;
        for (std::size_t i = 0; i < std::min(neu.size(), bild->size()); ++i) { anders += neu[i] != (*bild)[i] ? 1U : 0U; }
        diag::detail("Knopftest: Ruhe: Kamerabild " + std::to_string(w) + "x" + std::to_string(h) + ", stehend " + std::to_string(*bildW) + "x" +
                     std::to_string(*bildH) + ", " + std::to_string(anders) + " Bytes anders");
        return da && !bild->empty() && w == *bildW && h == *bildH && neu.size() == bild->size() && anders == 0;
    }));
    // --- Jede Aenderung zeichnet sofort -----------------------------------------------------------------------
    aenderung(s, "die Maus bewegt sich ueber der Kamera", [] {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        setzeMaus(ImVec2{f.bildX + static_cast<float>(f.breite) * 0.5F, f.bildY + static_cast<float>(f.hoehe) * 0.5F});
    }, true);
    aenderung(s, "die Kamera faehrt 32 Einheiten (ohne Eingabe)", [] { ke::editor().kamera.ursprung[0] += 32.0F; }, true);
    aenderung(s, "ein Pinsel wird gewaehlt (ohne Eingabe)", [pinsel] {
        ke::Editor& e = ke::editor();
        if (*pinsel) { e.karte.auswahl.waehlen(e.karte.welt(), *pinsel); }
        ke::auswahlGeaendert();
    }, true);
    aenderung(s, "ein Befehl (Patch Cylinder)", [] { (void)ke::ausfuehren("PatchCylinder"); }, true);
    aenderung(s, "Rueckgaengig", [] { (void)ke::ausfuehren("Undo"); }, true);
    aenderung(s, "eine Taste im Editor (Umschalt)", [] {
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(ImGuiKey_LeftShift, true);
        io.AddKeyEvent(ImGuiKey_LeftShift, false);
    }, true);
    aenderung(s, "das Fenster wird groesser", [] {
        if (const ImGuiWindow* w = tabfenster::fenster(3)) { ImGui::SetWindowSize(w->Name, ImVec2{w->Size.x + 40.0F, w->Size.y + 30.0F}); }
    }, true);
    aenderung(s, "neuZeichnen (Ende eines Hintergrundlaufs)", [] { ke::neuZeichnen(); }, true);
    // --- rc610 F23 (B3-2): das Spiel des Editors wird im Hintergrund neu geladen (Reiterwechsel auf eine Mission mit anderen
    // Spielordnern, setzeSitzungsSpielordner) - ohne Eingabe im Editor zeigen die Ansichten danach das neue Spiel ------------
    auto spielVor = std::make_shared<Zahl>();
    auto spielNach = std::make_shared<Zahl>();
    auto kennung = std::make_shared<std::string>();
    add(tu("Ruhe: Spielordner der Sitzung ohne Movie Duels (wie der Reiter einer JKA-Mission; ein leerer base im Laufordner)", [=] {
        *spielVor = zaehlen();
        *kennung = ke::editor().spielKennung;
        const std::filesystem::path jka = std::filesystem::current_path() / "f23_jka" / "base";
        std::error_code ec;
        std::filesystem::create_directories(jka, ec);
        setzeSitzungsSpielordner({jka.string()});
        diag::detail("Knopftest: Ruhe: Sitzung " + jka.string() + ", Spiel " + std::to_string(ke::kartenSpielNummer()));
    }));
    add(warteBis("Ruhe: das Spiel des Editors ist neu geladen", [=] { return ke::editor().spielKennung != *kennung && ke::spielBereit(); }, 3000));
    add(Schritt{"Ruhe: drei Bilder nach dem Laden", [=](int b) {
                    if (b < 3) { return false; }
                    *spielNach = zaehlen();
                    return true;
                }});
    add(pruefe("Ruhe: nach dem Neuladen des Spiels im Hintergrund zeichnen die Ansichten neu (B3-2)", [=] {
        diag::detail("Knopftest: Ruhe: Spiel neu geladen (" + *kennung + " -> " + ke::editor().spielKennung + "): " + text(*spielVor, *spielNach));
        return ke::editor().spielKennung != *kennung && spielNach->kamera > spielVor->kamera && (spielVor->xy[0] == 0U || spielNach->xy[0] > spielVor->xy[0]);
    }));
    add(tu("Ruhe: Spielordner der Sitzung zurueck", [=] {
        *kennung = ke::editor().spielKennung;
        setzeSitzungsSpielordner({});
    }));
    add(warteBis("Ruhe: das Spiel des Editors ist wieder das alte", [=] { return ke::editor().spielKennung != *kennung && ke::spielBereit(); }, 3000));
    add(tu("Ruhe: wieder jedes Bild zeichnen, Fenster zurueck", [] {
        ke::zeichnenNachBedarfImTest(false);
        tabfenster::zurueck(3);
        g_app->leftMode = 0;
    }));
    add(pause(0.5));
    // --- rc610 F23 (B3-1, nicht nachstellbar): das Spielbild (Szene 1) vergisst beim Wechsel der .bsp die Bilder und Lightmaps
    // der vorigen - aufbauen laedt die Bilder ueber loadMapTextures, und das ruft gpu::vergissKarte fuer die gewaehlte Szene 1.
    // Erst duel_jeditemple (viele Shader und Lightmaps, Kamera wie Teil 11), dann maps/mp/duel1: danach liegt im Grafikspeicher
    // der Szene kein Platz mehr, den duel1 nicht hat.
    auto erste = std::make_shared<ke::SpielbildStand>();
    add(tu("Spielbild: duel_jeditemple als Vorlage, Built map as in the game", [] {
        g_app->leftMode = 3;
        ke::Editor& ed = ke::editor();
        ed.echtLicht.spielbild = true;
        ed.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        std::string daten;
        std::string f;
        if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen("maps/duel_jeditemple.bsp", daten) || !ke::vorlageLaden(daten, "duel_jeditemple.bsp", &f)) {
            meldeFehler("Spielbild: maps/duel_jeditemple.bsp nicht geladen " + f);
        }
        ed.kamera.ursprung = karte::Vektor3f(-1250.0F, -20.0F, -1113.75F);
        ed.kamera.winkel = karte::Vektor3f(0.0F, 90.0F, 0.0F);
        ++ed.sichtStand;
    }));
    add(warteBis("Spielbild: Texturen duel_jeditemple", [] { return ke::kameraBilderFertig(); }, 20000));
    add(stufe9c::langeWarten("Spielbild: duel_jeditemple gezeigt", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.0));
    add(tu("Spielbild: Plaetze von duel_jeditemple merken, maps/mp/duel1 als Vorlage", [erste] {
        *erste = ke::spielbildStand();
        ke::Editor& ed = ke::editor();
        std::string daten;
        std::string f;
        if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen("maps/mp/duel1.bsp", daten) || !ke::vorlageLaden(daten, "duel1.bsp", &f)) {
            meldeFehler("Spielbild: maps/mp/duel1.bsp nicht geladen " + f);
        }
        ++ed.sichtStand;
    }));
    add(warteBis("Spielbild: Texturen duel1", [] { return ke::kameraBilderFertig(); }, 20000));
    add(stufe9c::langeWarten("Spielbild: duel1 gezeigt", [erste] { return ke::spielbildStand().nummer != erste->nummer && ke::editor().echtLicht.spielbildGezeigt; },
                             20000));
    add(pause(1.0));
    add(pruefe("Spielbild: nach dem Wechsel liegen nur Plaetze von duel1 im Grafikspeicher der Szene (B3-1)", [erste] {
        const ke::SpielbildStand z = ke::spielbildStand();
        char t[300];
        std::snprintf(t, sizeof t,
                      "Knopftest: Spielbild B3-1 - duel_jeditemple %d Bilder / %d Lightmaps, hoechster Platz %d / %d; duel1 %d / %d, hoechster Platz %d / %d",
                      erste->bildPlaetze, erste->lightmapPlaetze, erste->hoechstesBild, erste->hoechsteLightmap, z.bildPlaetze, z.lightmapPlaetze,
                      z.hoechstesBild, z.hoechsteLightmap);
        diag::detail(t);
        // Vorbedingung: die erste Karte hat Plaetze belegt, die es in der zweiten nicht gibt
        const bool sinnvoll = erste->hoechstesBild >= z.bildPlaetze || erste->hoechsteLightmap >= z.lightmapPlaetze;
        return sinnvoll && z.hoechstesBild < z.bildPlaetze && z.hoechsteLightmap < z.lightmapPlaetze;
    }));
    add(tu("Spielbild: Vorlage weg, Mission-Tab", [] {
        ke::vorlageEntfernen();
        g_app->leftMode = 0;
    }));
    return s;
}
}  // namespace kartenruhe
