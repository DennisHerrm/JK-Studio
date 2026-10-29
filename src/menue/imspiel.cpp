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
    if (karte::alsStartwertTauglich(e.testOrdner) && e.faehigkeit.homepath) {
        p.weg = karte::SpielTestWeg::HomePfad;
        p.pk3 = mitSchraeg(e.testOrdner) + "/" + spielOrdner + "/" + kMenueTestPk3;
        pfadArg = "+set fs_homepath \"" + mitSchraeg(e.testOrdner) + "\" ";
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
