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
    return s;
}

}  // namespace stufe9
