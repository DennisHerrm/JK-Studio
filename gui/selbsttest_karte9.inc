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
