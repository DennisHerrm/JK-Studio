// kartespieltest.cpp - Stufe 9 Punkt 1: "Im Spiel testen" (ctest karte_spieltest)
//
// Prueft den Plan (welcher Suchpfad, wohin das Archiv, die Befehlszeile) fuer jede
// Engine-Art ohne ein Spiel zu starten. Mit einem Argument (ein Steam-Ordner
// "common") liest der Test zusaetzlich die echten .exe (nur lesen) und prueft, was
// sie koennen.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "bhed/karte/spieltest.h"

namespace {
int fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}
std::string binaer(const char* a, const char* b = "") {   // Cvar-Namen wie in einer .exe
    std::string s = "MZ\x90";
    s += std::string(a) + std::string(1, '\0') + "\x01\x02" + std::string(b) + std::string(1, '\0');
    return s;
}
}  // namespace

int main(int argc, char** argv) {
    using namespace bhed::karte;
    std::printf("--- Faehigkeit aus der .exe ---\n");
    const EngineFaehigkeit nurCd = engineFaehigkeit(binaer("fs_cdpath"));
    expect("nur fs_cdpath (jasp/jk2sp)", nurCd.gelesen && nurCd.cdpath && !nurCd.homepath);
    const EngineFaehigkeit beide = engineFaehigkeit(binaer("fs_cdpath", "fs_homepath"));
    expect("fs_cdpath und fs_homepath (jamp/OpenJK)", beide.cdpath && beide.homepath);
    expect("Teilwort zaehlt nicht (xfs_cdpath, fs_cdpathx)",
           !engineFaehigkeit(binaer("xfs_cdpath", "fs_cdpathx")).cdpath);
    expect("leer: nicht gelesen", !engineFaehigkeit("").gelesen);

    SpielTestEingabe e;
    e.enginePfad = "C:/Spiele/JKA/GameData/";
    e.exe = "jasp.exe";
    e.basisSpiel = "base";
    e.fsGame = "base";
    e.testOrdner = "C:\\Users\\a\\AppData\\Roaming\\JKStudio\\spieltest";
    e.kartenName = "meine_karte";
    e.faehigkeit = nurCd;

    std::printf("--- Wege ---\n");
    SpielTestPlan p = spielTestPlanen(e);
    expect("JA SP ohne Konflikt: fs_cdpath", p.weg == SpielTestWeg::CdPfad && !p.bestaetigungNoetig);
    expect("Archiv im eigenen Ordner unter base", p.pk3 == "C:/Users/a/AppData/Roaming/JKStudio/spieltest/base/zzz_jkstudio_test.pk3");
    expect("Befehl JA SP",
           p.befehl == "\"C:/Spiele/JKA/GameData/jasp.exe\" +set fs_cdpath \"C:/Users/a/AppData/Roaming/JKStudio/spieltest\" "
                       "+set sv_pure 0 +devmap meine_karte");
    std::printf("      %s\n", p.befehl.c_str());

    e.namensKonflikt = true;
    p = spielTestPlanen(e);
    expect("JA SP mit Konflikt (kein fs_homepath): Mod-Ordner erst nach Frage", p.weg == SpielTestWeg::ModOrdner && p.bestaetigungNoetig && p.befehl.empty());
    e.modOrdnerErlaubt = true;
    p = spielTestPlanen(e);
    expect("bestaetigt: eigener Mod-Ordner jkstudio_test", p.pk3 == "C:/Spiele/JKA/GameData/jkstudio_test/zzz_jkstudio_test.pk3");
    expect("bestaetigt: +set fs_game jkstudio_test",
           p.befehl == "\"C:/Spiele/JKA/GameData/jasp.exe\" +set sv_pure 0 +set fs_game jkstudio_test +devmap meine_karte");
    e.modOrdnerErlaubt = false;

    e.exe = "MovieDuels-SP.x86_64.exe";
    e.fsGame = "MD";
    e.faehigkeit = beide;
    p = spielTestPlanen(e);
    expect("Movie Duels mit Konflikt: fs_homepath, Archiv unter MD", p.weg == SpielTestWeg::HomePfad &&
                                                                         p.pk3 == "C:/Users/a/AppData/Roaming/JKStudio/spieltest/MD/zzz_jkstudio_test.pk3");
    expect("Befehl Movie Duels",
           p.befehl == "\"C:/Spiele/JKA/GameData/MovieDuels-SP.x86_64.exe\" +set fs_homepath \"C:/Users/a/AppData/Roaming/JKStudio/spieltest\" "
                       "+set sv_pure 0 +set fs_game MD +devmap meine_karte");
    e.namensKonflikt = false;
    p = spielTestPlanen(e);
    expect("Movie Duels ohne Konflikt: fs_cdpath (Einstellungen des Spielers bleiben)", p.weg == SpielTestWeg::CdPfad);
    e.modOrdnerErlaubt = true;
    e.faehigkeit = EngineFaehigkeit{};
    p = spielTestPlanen(e);
    expect("Mod aktiv, Engine ohne Pfad-Cvar: in den Ordner der Mod", p.weg == SpielTestWeg::ModOrdner &&
                                                                          p.pk3 == "C:/Spiele/JKA/GameData/MD/zzz_jkstudio_test.pk3" &&
                                                                          p.befehl.find("+set fs_game MD ") != std::string::npos);
    e.modOrdnerErlaubt = false;

    std::printf("--- Ungueltiges ---\n");
    e.faehigkeit = beide;
    e.testOrdner = "C:/Users/a+b/test";
    p = spielTestPlanen(e);
    expect("'+' im eigenen Ordner: nicht als Parameter, Mod-Ordner mit Frage", p.weg == SpielTestWeg::ModOrdner && p.bestaetigungNoetig);
    e.testOrdner = "C:/t";
    e.kartenName = "mit leer";
    p = spielTestPlanen(e);
    expect("Leerzeichen im Kartennamen: kein Start", p.weg == SpielTestWeg::Keiner && !p.grund.empty() && p.befehl.empty());
    e.kartenName = "k";
    e.exe.clear();
    expect("ohne Programm: kein Start", spielTestPlanen(e).weg == SpielTestWeg::Keiner);
    expect("Liste kennt jasp, jamp, OpenJK, Movie Duels, jk2", testEngines().size() >= 10);

    if (argc > 1) {   // die echten Programme lesen (nie starten)
        std::printf("--- echte Programme in %s ---\n", argv[1]);
        const auto lesen = [&](const std::string& rel) {
            std::ifstream f(std::string(argv[1]) + "/" + rel, std::ios::binary);
            std::ostringstream o;
            o << f.rdbuf();
            return engineFaehigkeit(o.str());
        };
        const struct {
            const char* rel;
            bool cd, home;
        } echt[] = {{"Jedi Academy/GameData Movie Duels/jasp.exe", true, false},
                    {"Jedi Academy/GameData Movie Duels/jamp.exe", true, true},
                    {"Jedi Academy/GameData Movie Duels/MovieDuels-SP.x86_64.exe", true, true},
                    {"Jedi Outcast/GameData/jk2sp.exe", true, false},
                    {"Jedi Outcast/GameData/jk2mp.exe", true, true}};
        for (const auto& x : echt) {
            const EngineFaehigkeit f = lesen(x.rel);
            if (!f.gelesen) {
                std::printf("  --    %s fehlt\n", x.rel);
                continue;
            }
            expect(x.rel, f.cdpath == x.cd && f.homepath == x.home);
        }
    }
    std::printf("%s (%d Fehler)\n", fehler == 0 ? "OK" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
