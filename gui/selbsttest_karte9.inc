// selbsttest_karte9.inc - Stufe 9 des Karten-Editors: die restlichen Karten-Werkzeuge.
// Laeuft nach dem Tutorial der Stufe 8c und dem GPU-Backen der Stufe 8g (die Karte meinekarte
// ist gebaut und unveraendert).
//
// Punkt 1: Build > Test in game. Den Menuepunkt anklicken wie ein Mensch: die .bsp ist aktuell
// (kein Bau), das Archiv liegt im eigenen Testordner (Einstellungsordner des Laufs, nie im
// Spielordner) mit maps/meinekarte.bsp Byte fuer Byte, die Befehlszeile hat fs_cdpath bzw.
// fs_homepath mit diesem Ordner und +devmap meinekarte - und wurde NICHT gestartet (nur gemerkt).
//
// Eingebunden in gui/selbsttest_handbuch.inc nach selbsttest_karte8g.inc. Modus karteneditor
// (BHED_KARTE_NUR=stufe9: Tutorial, GPU-Backen und dieser Teil).
namespace stufe9 {
namespace ke = karteneditor;
using stufe3::konsoleText;
std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Im Spiel testen: Konsole leeren, kein alter Befehl", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::editor().letzteUrl.clear();
        ke::editor().bau.testBefehl.clear();
        ke::editor().bau.testPk3.clear();
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(ke::spielTestOrdner()), ec);
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeStMenue)) + "##bauimspiel"));
    add(pause(0.3));
    {   // die Beschriftung haengt an der Engine der Einstellungen: erst beim Klick bilden
        auto innen = std::make_shared<Schritt>();
        add({"Im Spiel testen: Engine aus den Einstellungen anklicken", [innen](int b) {
                 if (b == 0) {
                     char z[512];
                     std::snprintf(z, sizeof(z), ke::trK(Str::KeStEngineStandard), ke::standardTestExe().c_str());
                     *innen = klick(std::string(z) + "##bauimspiel_std");
                 }
                 return innen->tun(b);
             }});
    }
    add(pause(0.5));
    add(pruefe("Im Spiel testen: kein Bau (die .bsp ist aktuell), Archiv im eigenen Testordner mit maps/meinekarte.bsp Byte fuer Byte", [] {
        const ke::Editor& ed = ke::editor();
        std::string tst = ke::spielTestOrdner();
        std::replace(tst.begin(), tst.end(), '\\', '/');
        std::string inhalt;
        bool archivOk = false;
        Pk3 a;
        if (readPk3Directory(ed.bau.testPk3, a)) {
            if (const Pk3Entry* en = a.find("maps/meinekarte.bsp"); en != nullptr) {
                archivOk = readPk3File(a, *en, inhalt) && inhalt == dateiInhalt(stufe8c::stamm() + ".bsp") && !inhalt.empty();
            }
        }
        const std::string k = konsoleText();
        diag::detail("Knopftest: Im Spiel testen - Archiv " + ed.bau.testPk3 + " ok " + std::to_string(archivOk ? 1 : 0));
        return !ke::bauLaeuft() && ed.bau.testPk3.rfind(tst + "/", 0) == 0 && archivOk && k.find("up to date") != std::string::npos;
    }));
    add(pruefe("Im Spiel testen: Befehlszeile mit fs_cdpath/fs_homepath auf den Testordner und +devmap meinekarte, nur gemerkt (nicht gestartet)", [] {
        const ke::Editor& ed = ke::editor();
        std::string tst = ke::spielTestOrdner();
        std::replace(tst.begin(), tst.end(), '\\', '/');
        const std::string& b = ed.bau.testBefehl;
        const bool pfad = b.find("+set fs_cdpath \"" + tst + "\"") != std::string::npos || b.find("+set fs_homepath \"" + tst + "\"") != std::string::npos;
        const bool exe = b.rfind("\"" + ke::enginePfadAnzeige() + ke::standardTestExe() + "\" ", 0) == 0;
        diag::detail("Knopftest: Im Spiel testen - Befehl " + b);
        return pfad && exe && b.find(" +devmap meinekarte") != std::string::npos && b.find("+set sv_pure 0") != std::string::npos &&
               ed.letzteUrl == b && konsoleText().find("(Selbsttest) nicht gestartet") != std::string::npos;
    }));
    add(pruefe("Im Spiel testen: die Wahl ist gespeichert (BhedTestGameExe leer = Engine)", [] {
        return ke::editor().bau.testExe.empty() && g_app->settings.karte.count("BhedTestGameExe") == 1;
    }));
    add(tu("Im Spiel testen: Ende - Dialoge zu", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
    }));
    add(pause(0.2));

    // --- Punkt 2: Geometrie zu Brushes ---------------------------------------------------
    // Eine .obj mit einem L-Koerper (geschlossen, konkav) und einer Platte (offen): Auto macht
    // aus dem L zwei Brushes und aus der Platte Modell + Clip; "Einfuegen" anklicken, dann:
    // die Brushes sind in der Karte und gewaehlt, das misc_model zeigt auf die .obj im
    // Import-Ordner (nicht im Spielordner), der Optimierer ist offen, Undo nimmt alles zurueck.
    auto vorher = std::make_shared<std::size_t>(0);
    add(tu("Geometrie: .obj schreiben und lesen (File > Import geometry as brushes)", [vorher] {
        karteDialogeZu();
        ke::konsoleLeeren();
        std::string l = "o L\n";
        const int gx[6] = {0, 128, 128, 64, 64, 0}, gy[6] = {0, 0, 64, 64, 128, 128};
        for (int zz = 0; zz < 2; ++zz) {
            for (int i = 0; i < 6; ++i) { l += "v " + std::to_string(gx[i] + 1024) + " " + std::to_string(gy[i]) + " " + std::to_string(zz * 32) + "\n"; }
        }
        l += "usemtl mauer\nf 1 6 5 4 3 2\nf 7 8 9 10 11 12\n";
        for (int i = 0; i < 6; ++i) {
            const int a = i + 1, b = (i + 1) % 6 + 1;
            l += "f " + std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(b + 6) + " " + std::to_string(a + 6) + "\n";
        }
        l += "o Platte\nv 1024 256 0\nv 1152 256 0\nv 1152 384 0\nv 1024 384 0\nusemtl holz\nf 13 14 15\nf 13 15 16\n";
        const std::string pfad = platform::settingsDirectory() + "\\geo_test.obj";
        std::ofstream(std::filesystem::path(pfad), std::ios::binary) << l;
        *vorher = ke::editor().karte.welt()->kinder().size();
        ke::editor().geoImport.yOben = false;
        ke::editor().geoImport.skala = 1.0F;
        if (!ke::geoImportLaden(pfad)) { meldeFehler("Geometrie: geo_test.obj nicht lesbar"); }
    }));
    add(pause(0.4));
    add(pruefe("Geometrie: Dialog offen, 2 Teile; Vorschau: L geschlossen -> 2 Brushes, Platte offen -> Modell + Clip", [] {
        const ke::GeoImportZustand& z = ke::editor().geoImport;
        return z.offen && z.netz.teile.size() == 2 && z.vorschau.size() == 2 && z.vorschau[0].geschlossen && z.vorschau[0].art == karte::TeilArt::Pinsel &&
               z.vorschau[0].pinsel.size() == 2 && !z.vorschau[1].geschlossen && z.vorschau[1].art == karte::TeilArt::ModellClip;
    }));
    add(klick(std::string(ke::trK(Str::KeGiEinfuegen)) + "##gieinfuegen", "###geoimport"));
    add(pause(0.5));
    add(pruefe("Geometrie: 3 Brushes eingefuegt und gewaehlt, misc_model auf models/import/geo_test.obj im Import-Ordner, Optimierer offen", [vorher] {
        const ke::Editor& ed = ke::editor();
        bool modell = false;
        for (const auto& en : ed.karte.entitaeten()) {
            modell = modell || (en->wert("classname") == "misc_model" && en->wert("model") == "models/import/geo_test.obj");
        }
        std::error_code ec;
        const bool datei = std::filesystem::is_regular_file(std::filesystem::path(platform::settingsDirectory() + "\\import\\models\\import\\geo_test.obj"), ec);
        const bool imVfs = !ke::importOrdner().empty() && ke::kartenVfsOrdner().front() == ke::importOrdner();
        diag::detail("Knopftest: Geometrie - Brushes " + std::to_string(ed.karte.welt()->kinder().size() - *vorher) + ", Modell " + std::to_string(modell ? 1 : 0) +
                     ", Datei " + std::to_string(datei ? 1 : 0) + ", VFS " + std::to_string(imVfs ? 1 : 0));
        return !ed.geoImport.offen && ed.geoImport.letztes.pinsel == 3 && ed.geoImport.letztes.modelle == 1 &&
               ed.karte.welt()->kinder().size() == *vorher + 3 && modell && datei && imVfs && ke::optimiererOffen() &&
               konsoleText().find("geo_test.obj: 3") != std::string::npos;
    }));
    add(tu("Geometrie: Undo", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Geometrie: ein Undo nimmt Brushes und misc_model zurueck", [vorher] {
        const ke::Editor& ed = ke::editor();
        bool modell = false;
        for (const auto& en : ed.karte.entitaeten()) { modell = modell || en->wert("classname") == "misc_model"; }
        return ed.karte.welt()->kinder().size() == *vorher && !modell;
    }));
    // --- Punkt 3: Modell per Ziehen -------------------------------------------------------
    // Ein Wuerfel als .obj mit .mtl und Bild wird (wie WM_DROPFILES) auf die Mitte der aktiven
    // 2D-Ansicht abgelegt: MD3, Bild und Shader liegen im Import-Ordner, das misc_model steht
    // unter dem Zeiger und ist gewaehlt, readMd3 liest das Modell, "Im Spiel testen" packt den
    // Import-Ordner mit ins Archiv. Undo nimmt das misc_model zurueck.
    auto ziel = std::make_shared<karte::Vektor3f>();
    add(tu("Modell ziehen: Wuerfel.obj + .mtl + Bild schreiben, auf die 2D-Ansicht ablegen", [ziel] {
        karteDialogeZu();
        ke::konsoleLeeren();
        const std::string o = platform::settingsDirectory() + "\\ablage\\";
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(o), ec);
        std::ofstream(std::filesystem::path(o + "Geo Wuerfel.obj"), std::ios::binary)
            << "mtllib wuerfel.mtl\no Wuerfel\nv -32 -32 0\nv 32 -32 0\nv 32 32 0\nv -32 32 0\nv -32 -32 64\nv 32 -32 64\nv 32 32 64\nv -32 32 64\n"
               "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Stein\nf 1/1 4/4 3/3 2/2\nf 5/1 6/2 7/3 8/4\nf 1/1 2/2 6/3 5/4\nf 2/1 3/2 7/3 6/4\nf 3/1 4/2 8/3 7/4\nf 4/1 1/2 5/3 8/4\n";
        std::ofstream(std::filesystem::path(o + "wuerfel.mtl"), std::ios::binary) << "newmtl Stein\nmap_Kd stein.png\n";
        std::ofstream(std::filesystem::path(o + "stein.png"), std::ios::binary) << std::string("\x89PNG\r\n\x1a\nbild", 12);
        ke::Editor& ed = ke::editor();
        ed.geoImport.yOben = false;
        const ke::XyFenster& f = ed.xy[static_cast<std::size_t>(ed.aktivXy)];
        const int mx = f.ansicht.breite / 2, my = f.ansicht.hoehe / 2;
        *ziel = f.ansicht.zuPunkt(mx, my, ed.raster.einrastGroesse());
        dateiAbgelegt(o + "Geo Wuerfel.obj", f.bildX + static_cast<float>(mx), f.bildY + static_cast<float>(my));
    }));
    add(pause(0.5));
    add(pruefe("Modell ziehen: misc_model models/import/geo_wuerfel/geo_wuerfel.md3 unter dem Zeiger, gewaehlt; MD3 lesbar, Bild und Shader im Import-Ordner", [ziel] {
        const ke::Editor& ed = ke::editor();
        const std::string w = platform::settingsDirectory() + "\\import\\";
        bool modell = false;
        for (const auto& en : ed.karte.entitaeten()) {
            if (en->wert("classname") != "misc_model" || en->wert("model") != "models/import/geo_wuerfel/geo_wuerfel.md3") { continue; }
            float o[3] = {0, 0, 0};
            modell = std::sscanf(en->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]) == 3;
            const auto achse = static_cast<std::size_t>(karte::blickachse(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ));
            for (std::size_t i = 0; i < 3; ++i) {
                if (i != achse) { modell = modell && std::fabs(o[i] - (*ziel)[i]) < 0.5F; }
            }
        }
        Md3Model m;
        std::string err;
        const bool md3 = readMd3(dateiInhalt(w + "models\\import\\geo_wuerfel\\geo_wuerfel.md3"), m, &err) && m.surfaces.size() == 1 &&
                         m.surfaces[0].shader == "models/import/geo_wuerfel/stein";
        const bool bild = dateiInhalt(w + "models\\import\\geo_wuerfel\\stein.png").size() == 12;
        const bool shader = dateiInhalt(w + "scripts\\geo_wuerfel.shader").find("map models/import/geo_wuerfel/stein.png") != std::string::npos;
        diag::detail("Knopftest: Modell ziehen - Entitaet " + std::to_string(modell ? 1 : 0) + ", md3 " + std::to_string(md3 ? 1 : 0) + ", Bild " +
                     std::to_string(bild ? 1 : 0) + ", Shader " + std::to_string(shader ? 1 : 0) + ", Auswahl " + std::to_string(ed.karte.auswahl.anzahl()));
        return modell && md3 && bild && shader && ed.karte.auswahl.anzahl() == 1 && konsoleText().find("geo_wuerfel.md3") != std::string::npos;
    }));
    add(tu("Modell ziehen: Im Spiel testen (nur Befehl) - das Archiv nimmt den Import-Ordner mit", [] {
        ke::editor().bau.testPk3.clear();
        ke::imSpielTesten("");
    }));
    add(stufe8::warteSek("Modell ziehen: Bau fertig (die Karte ist geaendert)", [] { return !ke::bauLaeuft(); }, 300));
    add(pause(0.5));
    add(pruefe("Modell ziehen: zzz_jkstudio_test.pk3 enthaelt maps/meinekarte.bsp, die .md3, das Bild und das Shader-Skript", [] {
        Pk3 a;
        const std::string& p = ke::editor().bau.testPk3;
        const bool ok = !p.empty() && readPk3Directory(p, a) && a.find("maps/meinekarte.bsp") != nullptr &&
                        a.find("models/import/geo_wuerfel/geo_wuerfel.md3") != nullptr && a.find("models/import/geo_wuerfel/stein.png") != nullptr &&
                        a.find("scripts/geo_wuerfel.shader") != nullptr;
        diag::detail("Knopftest: Modell ziehen - Archiv " + p + " ok " + std::to_string(ok ? 1 : 0));
        return ok;
    }));
    add(pruefe("Pack map as .pk3 nimmt den Import-Ordner mit (MD3, Bild, Shader)", [] {
        const std::string p = ke::karteAlsPk3();
        Pk3 a;
        return !p.empty() && readPk3Directory(p, a) && a.find("maps/meinekarte.bsp") != nullptr &&
               a.find("models/import/geo_wuerfel/geo_wuerfel.md3") != nullptr && a.find("models/import/geo_wuerfel/stein.png") != nullptr &&
               a.find("scripts/geo_wuerfel.shader") != nullptr;
    }));
    add(tu("Modell ziehen: Undo", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Modell ziehen: Undo nimmt das misc_model zurueck", [] {
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->wert("classname") == "misc_model") { return false; }
        }
        return true;
    }));
    // --- Punkt 4: Sprungweite und Platz ------------------------------------------------------
    // View > Check jumps and space > Check (Machtstufe 2): im Raum des Tutorials gibt es
    // Stehplaetze auf dem Boden, Kanten an den Waenden; die Anzeige hat Linien; Clear leert.
    add(tu("Sprung: Machtstufe 2, Menue View", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::editor().sprungMacht = 2;
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeSjMenue)) + "###sprungpruefung"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeSjPruefen)) + "###sprungpruefen"));
    add(stufe8::warteSek("Sprung: Pruefung fertig", [] { return !ke::editor().sprungLaeuft && ke::editor().sprungBericht != nullptr; }, 120));
    add(pause(0.3));
    add(pruefe("Sprung: Stehplaetze im Raum, Kanten an den Waenden, Linien in der 2D-Ansicht, Konsole", [] {
        const auto& b = ke::editor().sprungBericht;
        if (!b) { return false; }
        std::size_t stehen = 0;
        for (const auto& p : b->punkte) { stehen += p.stehen ? 1 : 0; }
        std::size_t linien = 0;
        for (const auto& pl : ke::pluginLinien2d()) { linien += pl.linien.size(); }
        diag::detail("Knopftest: Sprung - Punkte " + std::to_string(b->punkte.size()) + ", stehen " + std::to_string(stehen) + ", Kanten " +
                     std::to_string(b->kanten) + ", Spruenge " + std::to_string(b->spruenge.size()) + ", Linien " + std::to_string(linien));
        return stehen >= 20 && b->kanten >= 4 && linien >= 2 * stehen && konsoleText().find("Jumps and space:") != std::string::npos;
    }));
    add(tu("Sprung: Clear, Einstellungen speichern", [] {
        ke::sprungPruefungLeeren();
        ke::einstellungenSpeichern();
    }));
    add(pruefe("Sprung: geleert, die Einstellung gespeichert (BhedJumpCheckForce 2)", [] {
        return ke::editor().sprungBericht == nullptr && g_app->settings.karte["BhedJumpCheckForce"] == "2";
    }));

    // --- Punkt 5: NPC-Wege --------------------------------------------------------------------
    // Vier Wegpunkte in einer Reihe am Spielerstart (eine Undo-Stufe), den ersten und letzten
    // waehlen; View > NPC paths: das Netz hat 4 Punkte und Verbindungen, der Testlauf kommt an,
    // die Bahn ist in der 2D-Ansicht; Undo nimmt die Wegpunkte zurueck.
    auto npcVorher = std::make_shared<std::size_t>(0);
    add(tu("NPC-Wege: vier Wegpunkte am Spielerstart, erster und letzter gewaehlt", [npcVorher] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *npcVorher = ed.karte.entitaeten().size();
        karte::Vektor3f s(0, 0, 32);
        for (const auto& en : ed.karte.entitaeten()) {
            if (en->wert("classname") == "info_player_start") { s = karte::entitaetUrsprung(*en); }
        }
        karte::Befehl undo(ed.karte.undo, "entityCreate -class waypoint");
        ed.karte.auswahl.leeren();
        std::vector<std::shared_ptr<karte::Entitaet>> wp;
        for (int i = 0; i < 4; ++i) {
            auto en = ed.karte.neueEntitaet("waypoint", false);
            char o[96];
            std::snprintf(o, sizeof(o), "%g %g %g", -120.0 + 80.0 * i, 0.0, static_cast<double>(s[2]) + 8.0);
            en->setzen("origin", o);
            en->setzen("targetname", "st9wp" + std::to_string(i));
            if (i < 3) { en->setzen("target", "st9wp" + std::to_string(i + 1)); }
            ed.karte.entitaetEinfuegen(en);
            wp.push_back(en);
        }
        ed.karte.auswahl.waehlen(wp.front());
        ed.karte.auswahl.waehlen(wp.back());
        ed.npcNetzZeigen = false;
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeNwMenue)) + "###npcwege"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeNwNetz)) + "###npcnetz"));
    add(pause(0.5));
    add(pruefe("NPC-Wege: Netz an, 4 Punkte, mindestens 3 Verbindungen, Konsole", [] {
        const ke::Editor& ed = ke::editor();
        const int kanten = ed.npcWelt && ed.npcWelt->netz ? ed.npcWelt->netz->zahlKanten() : -1;
        const std::size_t punkte = ed.npcWelt && ed.npcWelt->netz ? ed.npcWelt->netz->knoten.size() - 1 : 0;
        diag::detail("Knopftest: NPC-Wege - Punkte " + std::to_string(punkte) + ", Kanten " + std::to_string(kanten));
        return ed.npcNetzZeigen && punkte == 4 && kanten >= 3 && konsoleText().find("Waypoint network: 4 points") != std::string::npos;
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeNwMenue)) + "###npcwege"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeNwLauf)) + "###npclauf"));
    add(pause(0.5));
    add(pruefe("NPC-Wege: Testlauf kommt an, Bahn und Kasten in der 2D-Ansicht", [] {
        const ke::Editor& ed = ke::editor();
        if (!ed.npcLauf) { return false; }
        std::size_t linien = 0;
        for (const auto& pl : ke::pluginLinien2d()) { linien += pl.linien.size(); }
        diag::detail("Knopftest: NPC-Wege - Ankunft " + std::to_string(ed.npcLauf->ankunftMs) + " ms, Linien " + std::to_string(linien));
        return ed.npcLauf->ankunftMs > 0 && !ed.npcLauf->nie && linien > 12 && konsoleText().find("Test walk: arrives") != std::string::npos;
    }));
    add(tu("NPC-Wege: aus, Testlauf leeren, Undo", [] {
        karteDialogeZu();
        ke::npcTestlaufLeeren();
        ke::npcNetzUmschalten();
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("NPC-Wege: Undo nimmt die Wegpunkte zurueck", [npcVorher] {
        return ke::editor().karte.entitaeten().size() == *npcVorher && !ke::editor().npcNetzZeigen;
    }));

    // --- Punkt 6: verknuepfte Prefabs ---------------------------------------------------------
    // Ein Brush (gewaehlt), View > Linked copies > Make linked copy: zwei Gruppen; ein Brush
    // dazu in der ersten (ein Befehl, wie der Anwender) erscheint auch in der Kopie; ein Undo
    // nimmt beides zurueck; drei weitere Undo stellen die Karte wieder her.
    auto vbVorher = std::make_shared<std::size_t>(0);
    const auto vbGruppe = [](int nr) -> std::shared_ptr<karte::Entitaet> {
        int i = 0;
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->wert(karte::kVerbundName).empty()) { continue; }
            if (i++ == nr) { return en; }
        }
        return nullptr;
    };
    const auto vbQuader = [](float x, float z) {
        auto p = std::make_shared<karte::Pinsel>();
        karte::Kasten b;
        b.origin = karte::Vektor3f(x, 0, z);
        b.extents = karte::Vektor3f(16, 16, 16);
        karte::TexturProjektion t;
        karte::texdefStandard(ke::editor().karte.kontext.art(), t);
        p->quaderBauen(b, "textures/system/caulk", t, ke::editor().karte.kontext);
        return p;
    };
    add(tu("Verbund: ein Brush im Raum, gewaehlt", [vbVorher, vbQuader] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *vbVorher = ed.karte.entitaeten().size();
        karte::Befehl b(ed.karte.undo, "brushCreate");
        auto p = vbQuader(-100, 0);
        ed.karte.welt()->kindEinfuegen(p);
        ed.karte.auswahl.leeren();
        ed.karte.auswahl.waehlen(ed.karte.welt(), p);
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeVbMenue)) + "###verbund"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeVbKopie)) + "###verbundkopie"));
    add(pause(0.4));
    add(pruefe("Verbund: zwei verknuepfte Gruppen mit je einem Brush, Konsole", [vbGruppe] {
        const auto a = vbGruppe(0);
        const auto c = vbGruppe(1);
        return a && c && a->kinder().size() == 1 && c->kinder().size() == 1 && a->wert(karte::kVerbundName) == c->wert(karte::kVerbundName) &&
               konsoleText().find("Linked copy 'link1'") != std::string::npos;
    }));
    add(tu("Verbund: ein zweiter Brush in der ersten Gruppe (ein Befehl)", [vbGruppe, vbQuader] {
        karte::Befehl b(ke::editor().karte.undo, "brushCreate");
        vbGruppe(0)->kindEinfuegen(vbQuader(-100, 64));
    }));
    add(pause(0.3));
    add(pruefe("Verbund: die Kopie folgt (zwei Brushes)", [vbGruppe] { return vbGruppe(1) && vbGruppe(1)->kinder().size() == 2; }));
    add(tu("Verbund: Undo", [] { (void)ke::ausfuehren("Undo"); }));
    add(pause(0.3));
    add(pruefe("Verbund: ein Undo nimmt beides zurueck", [vbGruppe] {
        return vbGruppe(0) && vbGruppe(0)->kinder().size() == 1 && vbGruppe(1)->kinder().size() == 1;
    }));
    add(tu("Verbund: noch zwei Undo (Kopie, Brush)", [] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Verbund: die Karte wie vorher", [vbVorher, vbGruppe] {
        return ke::editor().karte.entitaeten().size() == *vbVorher && !vbGruppe(0);
    }));

    // --- Punkt 7: Kurven-Werkzeug -------------------------------------------------------------
    // Drei info_null im Raum (in dieser Reihenfolge gewaehlt): View > Curve tool > Path -> ein
    // Patch im worldspawn; Undo; Path corners -> eine path_corner-Kette; Undo; Undo (info_null).
    auto kuVorher = std::make_shared<std::size_t>(0);
    auto kuPatches = std::make_shared<std::size_t>(0);
    add(tu("Kurve: drei info_null, der Reihe nach gewaehlt", [kuVorher, kuPatches] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *kuVorher = ed.karte.entitaeten().size();
        *kuPatches = ed.karte.welt()->kinder().size();
        karte::Befehl b(ed.karte.undo, "entityCreate -class info_null");
        ed.karte.auswahl.leeren();
        const char* orte[3] = {"-150 -100 -40", "0 100 -40", "150 -100 -40"};
        for (const char* o : orte) {
            auto en = ed.karte.neueEntitaet("info_null", false);
            en->setzen("origin", o);
            ed.karte.entitaetEinfuegen(en);
            ed.karte.auswahl.waehlen(en);
        }
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeKuMenue)) + "###kurve"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeKuWeg)) + "###kurveweg"));
    add(pause(0.4));
    add(pruefe("Kurve: ein Weg-Patch im worldspawn (3 Spalten), Konsole", [kuPatches] {
        const auto& kinder = ke::editor().karte.welt()->kinder();
        if (kinder.size() != *kuPatches + 1 || !kinder.back() || kinder.back()->art() != karte::Primitiv::Art::Patch) { return false; }
        const auto& p = static_cast<const karte::Patch&>(*kinder.back());
        return p.breite() == 3 && p.hoehe() >= 5 && konsoleText().find("Curve tool: curve through 3 points - 1 new") != std::string::npos;
    }));
    add(tu("Kurve: Undo, dann die Auswahl der drei info_null zurueck und path_corner", [] {
        (void)ke::ausfuehren("Undo");
        ke::Editor& ed = ke::editor();
        ed.karte.auswahl.leeren();
        for (const auto& en : ed.karte.entitaeten()) {
            if (en->wert("classname") == "info_null") { ed.karte.auswahl.waehlen(en); }
        }
        (void)ke::kurveBefehl(karte::KurvenArt::PfadEcken);
    }));
    add(pause(0.3));
    add(pruefe("Kurve: path_corner-Kette kurve1_1 -> kurve1_2 ...", [] {
        int n = 0;
        bool erster = false;
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->wert("classname") != "path_corner") { continue; }
            ++n;
            if (en->wert("targetname") == "kurve1_1") { erster = en->wert("target") == "kurve1_2"; }
        }
        diag::detail("Knopftest: Kurve - path_corner " + std::to_string(n));
        return n >= 5 && n % 2 == 1 && erster;
    }));
    add(tu("Kurve: zwei Undo (Kette, info_null)", [] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Kurve: die Karte wie vorher", [kuVorher, kuPatches] {
        return ke::editor().karte.entitaeten().size() == *kuVorher && ke::editor().karte.welt()->kinder().size() == *kuPatches;
    }));

    // --- Punkt 8: Modelle verteilen -----------------------------------------------------------
    // Eine misc_model_static-Vorlage und der Boden-Brush des Raums gewaehlt: View > Scatter models
    // > 10 -> zehn Kopien auf dem Boden (Oberkante des Bodens), gewaehlt; Undo; Undo (Vorlage).
    auto vtVorher = std::make_shared<std::size_t>(0);
    auto vtBoden = std::make_shared<float>(0.0F);
    add(tu("Verteilen: Vorlage misc_model_static und der Boden des Raums gewaehlt", [vtVorher, vtBoden] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *vtVorher = ed.karte.entitaeten().size();
        std::shared_ptr<karte::Primitiv> boden;
        float tief = 1e30F;
        for (const auto& p : ed.karte.welt()->kinder()) {
            if (!p || p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Kasten b = static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext);
            if (b.origin[2] + b.extents[2] < tief) {
                tief = b.origin[2] + b.extents[2];
                boden = p;
            }
        }
        *vtBoden = tief;
        karte::Befehl b(ed.karte.undo, "entityCreate -class misc_model_static");
        auto en = ed.karte.neueEntitaet("misc_model_static", false);
        en->setzen("origin", "0 0 -40");
        en->setzen("model", "models/map_objects/imperial/crate_banded.md3");
        ed.karte.entitaetEinfuegen(en);
        ed.karte.auswahl.leeren();
        ed.karte.auswahl.waehlen(en);
        if (boden) { ed.karte.auswahl.waehlen(ed.karte.welt(), boden); }
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeVtMenue)) + "###verteilen"));
    add(pause(0.3));
    {
        char z[160];
        std::snprintf(z, sizeof(z), ke::trK(Str::KeVtAnzahl), 10);
        add(klick(std::string(z) + "###verteilen10"));
    }
    add(pause(0.4));
    add(pruefe("Verteilen: zehn neue misc_model_static auf dem Boden, gewaehlt, Konsole", [vtVorher, vtBoden] {
        const ke::Editor& ed = ke::editor();
        int n = 0;
        bool aufBoden = true;
        for (const auto& en : ed.karte.entitaeten()) {
            if (en->wert("classname") != "misc_model_static" || en->wert("origin") == "0 0 -40") { continue; }
            ++n;
            aufBoden = aufBoden && std::fabs(karte::entitaetUrsprung(*en)[2] - *vtBoden) < 0.5F;
        }
        diag::detail("Knopftest: Verteilen - " + std::to_string(n) + " Kopien, Boden " + std::to_string(*vtBoden));
        return n == 10 && aufBoden && ed.karte.auswahl.anzahl() == 10 && ed.karte.entitaeten().size() == *vtVorher + 11 &&
               konsoleText().find("Scatter models: 10 x misc_model_static") != std::string::npos;
    }));
    add(tu("Verteilen: zwei Undo", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Verteilen: die Karte wie vorher", [vtVorher] { return ke::editor().karte.entitaeten().size() == *vtVorher; }));

    // --- Punkt 9: Gelaende formen -------------------------------------------------------------
    // Ein Terrain-Patch auf dem Boden; View > Terrain sculpt: an, Heben, Radius 128 (Klicks);
    // ein Strich in der Mitte (wie M1 in der Kamera: gelaendeFormenAn je Bild) hebt die Mitte, der
    // Rand bleibt; ein Undo nimmt den Strich zurueck; aus; Undo (Patch).
    auto foPatch = std::make_shared<std::shared_ptr<karte::Patch>>();
    add(tu("Formen: ein ebener Terrain-Patch 9 x 9 auf dem Boden", [foPatch] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        auto p = std::make_shared<karte::Patch>();
        karte::Kasten b;
        b.origin = karte::Vektor3f(0, 0, -60);
        b.extents = karte::Vektor3f(192, 192, 0);
        p->ebeneBauen(b, 2, 9, 9);
        p->setzeShader("textures/system/caulk");
        karte::Befehl c(ed.karte.undo, "patchCreate");
        ed.karte.welt()->kindEinfuegen(p);
        *foPatch = p;
        ed.formen = ke::Editor::FormenZustand{};
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeFoMenue)) + "###formen"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeFoAn)) + "###formenan"));
    add(pause(0.3));
    add(tu("Formen: ein Strich in der Mitte (10 Bilder je 1/60 s)", [] {
        for (int i = 0; i < 10; ++i) { (void)ke::gelaendeFormenAn(karte::Vektor3f(0, 0, -60), 1.0F / 60.0F); }
        ke::gelaendeFormenEnde();
    }));
    add(pruefe("Formen: an, Mitte gehoben (16 = 10 x 96/60), Rand unveraendert", [foPatch] {
        const auto& p = **foPatch;
        const float mitte = p.punkt(4, 4).vertex[2] + 60.0F, rand = p.punkt(0, 0).vertex[2] + 60.0F;
        diag::detail("Knopftest: Formen - Mitte " + std::to_string(mitte) + ", Rand " + std::to_string(rand));
        return ke::editor().formen.an && std::fabs(mitte - 16.0F) < 0.01F && rand == 0.0F && !ke::editor().formen.strich;
    }));
    add(tu("Formen: Undo", [] { (void)ke::ausfuehren("Undo"); }));
    add(pause(0.3));
    add(pruefe("Formen: ein Undo nimmt den Strich zurueck", [foPatch] { return (**foPatch).punkt(4, 4).vertex[2] == -60.0F; }));
    add(tu("Formen: aus, Undo (Patch)", [foPatch] {
        ke::editor().formen.an = false;
        (void)ke::ausfuehren("Undo");
        foPatch->reset();
    }));
    add(pause(0.2));

    // --- Punkt 10: Blender hin und zurueck ----------------------------------------------------
    // Ein Brush (gewaehlt) als .glb fuer Blender, mit Geometrie als Brushes (Y oben) zurueck: der
    // neue Brush hat dieselben Grenzen; drei Undo (Einfuegen, Brush) - die Karte wie vorher.
    auto blVorher = std::make_shared<std::size_t>(0);
    auto blKasten = std::make_shared<karte::Kasten>();
    add(tu("Blender: ein Brush, als .glb exportiert, wieder gelesen und eingefuegt", [blVorher, blKasten, vbQuader] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *blVorher = ed.karte.welt()->kinder().size();
        {
            karte::Befehl b(ed.karte.undo, "brushCreate");
            auto p = vbQuader(64, -32);
            ed.karte.welt()->kindEinfuegen(p);
            ed.karte.auswahl.leeren();
            ed.karte.auswahl.waehlen(ed.karte.welt(), p);
            *blKasten = p->grenzen(ed.karte.kontext);
        }
        const std::string pfad = platform::settingsDirectory() + "\\blender_test.glb";
        if (!ke::blenderExport(pfad)) { meldeFehler("Blender: Export gescheitert"); }
        ed.geoImport.yOben = true;
        ed.geoImport.skala = 1.0F;
        ed.geoImport.optimierer = false;
        if (!ke::geoImportLaden(pfad)) { meldeFehler("Blender: .glb nicht lesbar"); }
        ke::geoImportEinfuegen();
        ed.geoImport.optimierer = true;
    }));
    add(pause(0.3));
    add(pruefe("Blender: der zurueckgeholte Brush liegt am selben Ort (gleiche Grenzen), Konsole", [blVorher, blKasten] {
        const ke::Editor& ed = ke::editor();
        const auto& kinder = ed.karte.welt()->kinder();
        if (kinder.size() != *blVorher + 2 || !kinder.back() || kinder.back()->art() != karte::Primitiv::Art::Pinsel) { return false; }
        const karte::Kasten b = static_cast<const karte::Pinsel&>(*kinder.back()).grenzen(ed.karte.kontext);
        float abw = 0;
        for (std::size_t i = 0; i < 3; ++i) { abw += std::fabs(b.origin[i] - blKasten->origin[i]) + std::fabs(b.extents[i] - blKasten->extents[i]); }
        diag::detail("Knopftest: Blender - Abweichung der Grenzen " + std::to_string(abw));
        return abw < 0.05F && konsoleText().find("Exported for Blender") != std::string::npos;
    }));
    add(tu("Blender: zwei Undo", [] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("Undo");
    }));
    add(pause(0.3));
    add(pruefe("Blender: die Karte wie vorher", [blVorher] { return ke::editor().karte.welt()->kinder().size() == *blVorher; }));

    // --- Punkt 11: Asset-Browser ---------------------------------------------------------------
    // Modelle und Klaenge aus dem Spiel, die Suche (Name findet, Unsinn findet nichts); ein Modell
    // mit der Maus aus dem Browser in die XY-Ansicht gezogen wird misc_model am Ort unter dem
    // Zeiger; Doppelklick-Weg (vor die Kamera) fuer einen Klang: target_speaker; ein Brush als
    // Prefab gespeichert erscheint unter Prefabs und wird eingefuegt; alles per Undo zurueck.
    struct AbZustand {
        std::string modell, klang, prefab;
        std::size_t modelle = 0, klaenge = 0, ents = 0, kinder = 0;
        bool gefunden = false, leer = false;
    };
    auto ab = std::make_shared<AbZustand>();
    add(tu("Assets: Dialoge zu", [] { karteDialogeZu(); }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeAbMenue)) + "###assetbrowser_an"));
    add(pause(0.5));
    add(pruefe("Assets: View > Map tools > Asset browser oeffnet das Fenster", [] { return ke::assetBrowserOffen(); }));
    add(tu("Assets: Modelle, Klaenge, Suche", [ab] {
        ke::konsoleLeeren();
        ke::assetSucheSetzen("", 0);
        const auto m = ke::assetListe(0);
        ab->modelle = m.size();
        for (const auto& a : m) {
            if (a.pfad.size() > 4 && a.pfad.compare(a.pfad.size() - 4, 4, ".md3") == 0) {
                ab->modell = a.pfad;
                break;
            }
        }
        const auto k = ke::assetListe(2);
        ab->klaenge = k.size();
        if (!k.empty()) { ab->klang = k.front().pfad; }
        ke::assetSucheSetzen("zzqqxxnichtda", 0);
        ab->leer = ke::assetListe(0).empty();
        const std::string name = std::filesystem::path(ab->modell).stem().string();
        ke::assetSucheSetzen(name, 0);
        for (const auto& a : ke::assetListe(0)) { ab->gefunden = ab->gefunden || a.pfad == ab->modell; }
        ke::assetSucheSetzen(ab->modell, 0);   // fuers Ziehen: genau dieses Modell in der ersten Zelle
        ab->ents = ke::editor().karte.entitaeten().size();
        ab->kinder = ke::editor().karte.welt()->kinder().size();
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: Assets - %zu Modelle, %zu Klaenge, Modell %s, Klang %s", ab->modelle, ab->klaenge, ab->modell.c_str(), ab->klang.c_str());
        diag::detail(z);
    }));
    add(pause(0.3));
    add(pruefe("Assets: Modelle und Klaenge da, die Suche findet das Modell, Unsinn findet nichts", [ab] {
        return ab->modelle > 0 && ab->klaenge > 0 && !ab->modell.empty() && ab->gefunden && ab->leer && ke::assetBrowserOffen();
    }));
    add(Schritt{"Assets: das Modell aus dem Browser in die XY-Ansicht ziehen (Maus)", [ab](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    const ImVec2 ziel = stufe2::weltXy(0, 0);
                    if (b == 0) {   // XY-Ansicht auf 0 0, den Browser weg vom Ziel, andere Fenster (Optimizer) zu
                        ke::optimiererSchliessen();
                        karte::XyAnsicht& an = ke::editor().xy[0].ansicht;
                        an.setzeUrsprung(karte::Vektor3f(0, 0, an.ursprung[2]));
                        ++ke::editor().sichtStand;
                        const ImVec2 z = stufe2::weltXy(0, 0);
                        ke::assetBrowserNach(z.x > io.DisplaySize.x * 0.5F ? ImVec2(20, 90) : ImVec2(io.DisplaySize.x - 600, 90));
                        return false;
                    }
                    if (b < 3) { return false; }
                    ImVec2 zelle;
                    if (b == 3) {
                        if (!ke::assetZelleMitte(ke::assetIndex(ab->modell), zelle)) { meldeFehler("Assets: keine Zelle fuer das Modell"); }
                        setzeMaus(zelle);
                        return false;
                    }
                    if (b == 4) {
                        io.AddMouseButtonEvent(0, true);
                        return false;
                    }
                    if (b >= 5 && b <= 16) {
                        (void)ke::assetZelleMitte(ke::assetIndex(ab->modell), zelle);
                        const ImVec2 a = b == 5 ? zelle : io.MousePos;
                        const float t = 1.0F / static_cast<float>(17 - b);
                        setzeMaus(ImVec2(a.x + (ziel.x - a.x) * t, a.y + (ziel.y - a.y) * t));
                        return false;
                    }
                    if (b == 17) {
                        io.AddMouseButtonEvent(0, false);
                        return false;
                    }
                    return b >= 20;
                }});
    add(pause(0.3));
    add(pruefe("Assets: ein misc_model mit dem Modell unter dem Zeiger (XY 0 0), gewaehlt", [ab] {
        const ke::Editor& ed = ke::editor();
        if (ed.karte.entitaeten().size() != ab->ents + 1) { return false; }
        const auto& en = ed.karte.entitaeten().back();
        const karte::Vektor3f o = karte::entitaetUrsprung(*en);
        diag::detail("Knopftest: Assets - abgelegt bei " + en->wert("origin"));
        return en->wert("classname") == "misc_model" && en->wert("model") == ab->modell && std::fabs(o[0]) <= 8.0F && std::fabs(o[1]) <= 8.0F &&
               konsoleText().find(" placed at ") != std::string::npos;
    }));
    add(tu("Assets: ein Klang (wie Doppelklick: vor die Kamera), ein Brush als Prefab st9prefab", [ab, vbQuader] {
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        ke::assetSucheSetzen("", 2);
        (void)ke::assetListe(2);
        if (!ke::assetAbgelegt(ke::assetIndex(ab->klang), ImVec2(-1, -1))) { meldeFehler("Assets: Klang nicht eingesetzt"); }
        {
            karte::Befehl b(ed.karte.undo, "brushCreate");
            auto p = vbQuader(512, 512);
            ed.karte.welt()->kindEinfuegen(p);
            ed.karte.auswahl.leeren();
            ed.karte.auswahl.waehlen(ed.karte.welt(), p);
        }
        if (!ke::prefabSpeichern("st9prefab")) { meldeFehler("Assets: Prefab nicht gespeichert"); }
        ke::assetSucheSetzen("st9prefab", 1);
        const auto pr = ke::assetListe(1);
        if (pr.size() == 1) { ab->prefab = pr.front().pfad; }
    }));
    add(pause(0.3));
    add(pruefe("Assets: target_speaker mit dem Klang, Prefab-Datei da und unter Prefabs gelistet", [ab] {
        const ke::Editor& ed = ke::editor();
        bool sp = false;
        for (const auto& en : ed.karte.entitaeten()) { sp = sp || (en->wert("classname") == "target_speaker" && en->wert("noise") == ab->klang); }
        std::error_code ec;
        return sp && !ab->prefab.empty() && std::filesystem::is_regular_file(std::filesystem::path(std::u8string(ab->prefab.begin(), ab->prefab.end())), ec) &&
               konsoleText().find("Prefab saved") != std::string::npos;
    }));
    add(tu("Assets: das Prefab einsetzen (wie Doppelklick)", [ab] {
        ke::konsoleLeeren();
        if (!ke::assetAbgelegt(ke::assetIndex(ab->prefab), ImVec2(-1, -1))) { meldeFehler("Assets: Prefab nicht eingesetzt"); }
    }));
    add(pause(0.3));
    add(pruefe("Assets: ein Brush mehr (das Prefab), Konsole", [ab] {
        return ke::editor().karte.welt()->kinder().size() == ab->kinder + 2 && konsoleText().find(" placed at ") != std::string::npos;
    }));
    add(tu("Assets: Undo bis zum Anfang, Browser zu", [ab] {
        ke::Editor& ed = ke::editor();
        for (int i = 0; i < 8 && (ed.karte.entitaeten().size() != ab->ents || ed.karte.welt()->kinder().size() != ab->kinder); ++i) { (void)ke::ausfuehren("Undo"); }
        ke::assetSucheSetzen("", 0);
        if (ke::assetBrowserOffen()) { ke::assetBrowserUmschalten(); }
    }));
    add(pause(0.3));
    add(pruefe("Assets: die Karte wie vorher", [ab] {
        return ke::editor().karte.entitaeten().size() == ab->ents && ke::editor().karte.welt()->kinder().size() == ab->kinder;
    }));

    // --- Punkt 9 (Rest): Texturen ueberblenden ------------------------------------------------
    // Eine func_group aus zwei Brushes (Textur test/boden), gewaehlt; View > Map tools > Texture
    // blending; zwei Schichten, "Set up" angeklickt: die Schluessel _indexmap/_layers/_shader, die
    // Shader-Datei und die Index-Karte (TGA, alles Schicht 0) im Import-Ordner, die Flaechen haben
    // den Grund-Shader; dann mit der Maus Schicht 1 in die Draufsicht gemalt: die Karte und die
    // Datei zeigen Schicht 1; Undo bis zum Anfang.
    struct UbZustand {
        std::size_t ents = 0;
        std::string shaderDatei;
    };
    auto ub = std::make_shared<UbZustand>();
    add(tu("Ueberblenden: func_group aus zwei Brushes, gewaehlt", [ub] {
        karteDialogeZu();
        ke::optimiererSchliessen();   // verdeckt sonst die Draufsicht
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        ub->ents = ed.karte.entitaeten().size();
        karte::Befehl b(ed.karte.undo, "entityCreate -class func_group");
        auto en = ed.karte.neueEntitaet("func_group", true);
        ed.karte.entitaetEinfuegen(en);
        ed.karte.auswahl.leeren();
        for (const float x : {-256.0F, 256.0F}) {
            auto pi = std::make_shared<karte::Pinsel>();
            karte::Kasten k;
            k.origin = karte::Vektor3f(x, 0, -512);
            k.extents = karte::Vektor3f(256, 512, 16);
            karte::TexturProjektion t;
            karte::texdefStandard(ed.karte.kontext.art(), t);
            pi->quaderBauen(k, "textures/test/boden", t, ed.karte.kontext);
            en->kindEinfuegen(pi);
            ed.karte.auswahl.waehlen(en, pi);
        }
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeWzMenue)) + "###werkzeuge9"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeUbMenue)) + "###ueberblenden_an"));
    add(pause(0.5));
    add(tu("Ueberblenden: zwei Schichten, Texturen", [] {
        ke::ueberblendenSchichten(2);
        ke::ueberblendenTextur(0, "textures/test/gras");
        ke::ueberblendenTextur(1, "test/fels");   // ohne "textures/": wird ergaenzt
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeUbEinrichten)) + "##ubein", "###ueberblenden"));
    add(pause(0.5));
    add(pruefe("Ueberblenden: Schluessel, Shader-Datei, Index-Karte (Schicht 0), Grund-Shader auf den Flaechen", [ub] {
        const ke::Editor& ed = ke::editor();
        if (ed.karte.entitaeten().size() != ub->ents + 1) { return false; }
        const auto& en = ed.karte.entitaeten().back();
        const std::string basis = en->wert("_shader");
        const std::string imp = platform::settingsDirectory() + "\\import\\";
        ub->shaderDatei = imp + "shaders\\jks_blend_" + basis.substr(basis.find('/') + 1) + ".shader";
        const std::string sh = dateiInhalt(ub->shaderDatei);
        const karte::IndexKarte& im = ke::ueberblendenKarte();
        bool flaechen = !en->kinder().empty();
        for (const auto& c : en->kinder()) {
            const auto& pi = static_cast<const karte::Pinsel&>(*c);
            for (std::size_t i = 0; i < pi.anzahl(); ++i) { flaechen = flaechen && pi[i].shader() == "textures/" + basis; }
        }
        std::error_code ec;
        const std::string datei = ke::ueberblendenDatei();   // eine Kopie: begin()/end() derselben Zeichenkette
        diag::detail("Knopftest: Ueberblenden - " + basis + ", " + en->wert("_indexmap") + ", Datei " + datei);
        return basis.rfind("jks_blend/", 0) == 0 && en->wert("_layers") == "2" && en->wert("_indexmap").rfind("maps/", 0) == 0 &&
               sh.find("textures/" + basis + "_0to1") != std::string::npos && sh.find("textures/test/fels.tga") != std::string::npos &&
               std::filesystem::is_regular_file(std::filesystem::path(std::u8string(datei.begin(), datei.end())), ec) &&
               im.breite == 128 && im.hoehe == 128 && std::all_of(im.index.begin(), im.index.end(), [](unsigned char v) { return v == 0; }) && flaechen &&
               konsoleText().find("Texture blending set up") != std::string::npos;
    }));
    add(Schritt{"Ueberblenden: Schicht 1 mit der Maus in die Draufsicht malen", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    ImVec2 mi, ma;
                    if (!ke::ueberblendenLeinwand(mi, ma)) {
                        meldeFehler("Ueberblenden: keine Draufsicht");
                        return true;
                    }
                    const ImVec2 m((mi.x + ma.x) * 0.5F, (mi.y + ma.y) * 0.5F);
                    if (b == 0) {
                        ke::ueberblendenPinsel(1, 256.0F);   // Radius 256: ein Kreis um die Mitte
                        setzeMaus(m);
                        return false;
                    }
                    if (b == 1) {
                        io.AddMouseButtonEvent(0, true);
                        return false;
                    }
                    if (b >= 2 && b <= 6) {
                        setzeMaus(ImVec2(m.x + static_cast<float>(b - 2), m.y));
                        return false;
                    }
                    if (b == 7) {
                        io.AddMouseButtonEvent(0, false);
                        return false;
                    }
                    return b >= 10;
                }});
    add(pause(0.3));
    add(pruefe("Ueberblenden: Mitte Schicht 1, Rand Schicht 0, Datei gleich (TGA), Konsole", [] {
        const karte::IndexKarte& im = ke::ueberblendenKarte();
        if (im.index.size() != 128U * 128U) { return false; }
        const std::string d = dateiInhalt(ke::ueberblendenDatei());
        karte::IndexKarte z;
        const bool gelesen = karte::blendTgaLesen(std::vector<unsigned char>(d.begin(), d.end()), 2, z);
        const auto n1 = std::count(im.index.begin(), im.index.end(), static_cast<unsigned char>(1));
        diag::detail("Knopftest: Ueberblenden - " + std::to_string(n1) + " Punkte Schicht 1");
        return im.index[64 * 128 + 64] == 1 && im.index[0] == 0 && n1 > 100 && gelesen && z.index == im.index &&
               konsoleText().find("Index map saved") != std::string::npos;
    }));
    add(tu("Ueberblenden: Undo bis zum Anfang, Fenster zu, Shader-Datei weg", [ub] {
        ke::Editor& ed = ke::editor();
        for (int i = 0; i < 4 && ed.karte.entitaeten().size() != ub->ents; ++i) { (void)ke::ausfuehren("Undo"); }
        if (ke::ueberblendenOffen()) { ke::ueberblendenUmschalten(); }
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(std::u8string(ub->shaderDatei.begin(), ub->shaderDatei.end())), ec);
    }));
    add(pause(0.3));
    add(pruefe("Ueberblenden: die Karte wie vorher", [ub] { return ke::editor().karte.entitaeten().size() == ub->ents; }));

    // --- Punkt 12: nach einem Absturz wiederherstellen ----------------------------------------
    // Die Sitzungsdatei nennt die offene Karte (nach einer Sekunde Editor); ein Autosave, neuer
    // als die Karte, mit einem info_null mehr wird angelegt; absturzPruefen (wie beim Start) fragt,
    // "Ja" laedt es unter dem Namen der Karte, als geaendert; danach die Karte neu laden.
    auto whName = std::make_shared<std::string>();
    add(stufe8::warteSek("Wiederherstellen: die Sitzungsdatei nennt die Karte", [] {
        return dateiInhalt(ke::sitzungsPfad()).rfind(ke::editor().name + "\n", 0) == 0;
    }, 10));
    add(tu("Wiederherstellen: ein neueres Autosave mit info_null st9wieder, absturzPruefen", [whName] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::Editor& ed = ke::editor();
        *whName = ed.name;
        std::string text = karte::karteSchreiben(ed.karte);
        text += "{\n\"classname\" \"info_null\"\n\"targetname\" \"st9wieder\"\n\"origin\" \"0 0 0\"\n}\n";
        const std::string auto1 = karte::autosavePfad(ed.name);
        std::ofstream(std::filesystem::path(std::u8string(auto1.begin(), auto1.end())), std::ios::binary) << text;
        std::error_code ec;
        const std::filesystem::path kp(std::u8string(ed.name.begin(), ed.name.end()));
        std::filesystem::last_write_time(kp, std::filesystem::file_time_type::clock::now() - std::chrono::minutes(5), ec);
        karte::sitzungSchreiben(ke::sitzungsPfad(), ed.name, 1);   // wie von einem frueheren, abgestuerzten Lauf
        if (!ke::absturzPruefen()) { meldeFehler("Wiederherstellen: keine Frage"); }
    }));
    add(pause(0.3));
    add(pruefe("Wiederherstellen: die Frage ist offen", [] { return ke::editor().meldung.offen && ke::editor().meldung.jaNein; }));
    add(tu("Wiederherstellen: Ja", [] {
        ke::Editor& ed = ke::editor();
        auto danach = ed.meldung.danach;
        ed.meldung.offen = false;
        if (danach) { danach(true); }
    }));
    add(pause(0.5));
    add(pruefe("Wiederherstellen: das Autosave ist geladen (info_null st9wieder), Name der Karte, geaendert", [whName] {
        const ke::Editor& ed = ke::editor();
        bool da = false;
        for (const auto& en : ed.karte.entitaeten()) { da = da || en->wert("targetname") == "st9wieder"; }
        return da && ed.name == *whName && ke::geaendert() && konsoleText().find("Restored from") != std::string::npos;
    }));
    add(tu("Wiederherstellen: die Karte neu laden, Autosave weg", [whName] {
        const std::string auto1 = karte::autosavePfad(*whName);
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(std::u8string(auto1.begin(), auto1.end())), ec);
        (void)ke::karteOeffnen(*whName);
    }));
    add(pause(0.5));
    add(pruefe("Wiederherstellen: wieder die gespeicherte Karte", [whName] {
        bool da = false;
        for (const auto& en : ke::editor().karte.entitaeten()) { da = da || en->wert("targetname") == "st9wieder"; }
        return !da && ke::editor().name == *whName && !ke::geaendert();
    }));
    add(tu("Geometrie: Ende - Import-Ordner weg (spaetere Befehlsvergleiche ohne -fs_pakpath)", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory() + "\\import"), ec);
        std::filesystem::remove(std::filesystem::path(platform::settingsDirectory() + "\\geo_test.obj"), ec);
    }));
    add(pause(0.3));
    return s;
}

}  // namespace stufe9
