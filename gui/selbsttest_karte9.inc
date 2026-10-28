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
