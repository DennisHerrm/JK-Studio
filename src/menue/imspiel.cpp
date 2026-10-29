// imspiel.cpp - Menue-Editor M2 Punkt 8 (siehe imspiel.h).
// Eigener Code.
#include "bhed/menue/imspiel.h"

#include <algorithm>

namespace bhed::menue {

namespace {
std::string mitSchraeg(std::string s) {
    std::replace(s.begin(), s.end(), '\\', '/');
    while (!s.empty() && s.back() == '/') { s.pop_back(); }
    return s;
}
}  // namespace

MenueTestPlan menueTestPlanen(const MenueTestEingabe& e) {
    MenueTestPlan p;
    if (e.exe.empty()) {
        p.grund = "no game program found next to the game folders";
        return p;
    }
    const bool mod = !e.fsGame.empty() && e.fsGame != e.basisSpiel;
    const std::string spielOrdner = mod ? e.fsGame : e.basisSpiel;
    std::string pfadArg;
    std::string fsGame = spielOrdner;
    // Jedi Outcast 1.04 (jk2sp.exe, jk2mp.exe) stuerzt mit einem eigenen Pfad ab 105 Zeichen ab
    // (M5b gemessen: 100 laeuft, 105 haengt, 110 Stapelueberlauf) - dann lieber der Mod-Ordner
    const bool altJk2 = e.exe == "jk2sp.exe" || e.exe == "jk2mp.exe";
    const bool tauglich = karte::alsStartwertTauglich(e.testOrdner) && !(altJk2 && mitSchraeg(e.testOrdner).size() > 100);
    if (tauglich && e.faehigkeit.homepath) {
        p.weg = karte::SpielTestWeg::HomePfad;
        p.pk3 = mitSchraeg(e.testOrdner) + "/" + spielOrdner + "/" + kMenueTestPk3;
        pfadArg = "+set fs_homepath \"" + mitSchraeg(e.testOrdner) + "\" ";
    } else if (tauglich && e.faehigkeit.cdpath) {
        // M5b: ohne fs_homepath (jasp, jk2sp) der eigene Ordner als fs_basepath, die Spieldaten ueber
        // fs_cdpath (niedrigster Rang): das Testpaket steht ueber den Menues des Spiels, geschrieben
        // (Config, Log) wird nur im eigenen Ordner
        p.weg = karte::SpielTestWeg::CdPfad;
        p.pk3 = mitSchraeg(e.testOrdner) + "/" + spielOrdner + "/" + kMenueTestPk3;
        pfadArg = "+set fs_basepath \"" + mitSchraeg(e.testOrdner) + "\" +set fs_cdpath \"" + mitSchraeg(e.enginePfad) + "\" ";
    } else {
        p.weg = karte::SpielTestWeg::ModOrdner;
        if (!e.modOrdnerErlaubt) {
            p.bestaetigungNoetig = true;
            return p;
        }
        const std::string ordner = mod ? e.fsGame : std::string(karte::kSpielTestMod);
        p.pk3 = mitSchraeg(e.enginePfad) + "/" + ordner + "/" + kMenueTestPk3;
        fsGame = ordner;
    }
    std::string args;
    // Die Movie-Duels-Programme nehmen ohne fs_game ihre Mod MD; deren Menues (basepath/MD)
    // stuenden dann ueber dem Testpaket in <test>/base - darum dort fs_game ausdruecklich
    const bool eigeneVorgabeMod = e.exe.find("MovieDuels") != std::string::npos;
    if (fsGame != e.basisSpiel || eigeneVorgabeMod) { args += "+set fs_game " + fsGame + " "; }
    args += "+echo jkstudio_menu_test";   // ein Befehl: kein Vorspann
    p.befehl = "\"" + e.enginePfad + e.exe + "\" " + pfadArg + args;
    return p;
}

}  // namespace bhed::menue
