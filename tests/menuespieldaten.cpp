// menuespieldaten.cpp - rc608 Punkt 15: Dateien finden wie das Spiel (FS_Startup, FS_AddGameDirectory, FS_ListFiles).
//
// base wird immer mitgelesen, die pk3 eines Ordners gehen vor seinen losen Dateien, spaetere Ordner vor frueheren,
// lose Dateien zaehlen auch beim Suchen und Auflisten, Gross/Klein egal.
#include "bhed/menue/spieldaten.h"
#include "bhed/pk3.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace bhed;
using namespace bhed::menue;
namespace fs = std::filesystem;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
void lose(const fs::path& p, const std::string& inhalt) {
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << inhalt;
}
std::string gelesen(const Spieldaten& d, const std::string& pfad) {
    std::string s;
    return d.lies(pfad, s) ? s : std::string("(nichts)");
}
}  // namespace

int main() {
    std::error_code ec;
    const fs::path t = fs::temp_directory_path(ec) / "bhed_menuespieldaten";
    fs::remove_all(t, ec);
    const fs::path base = t / "GameData" / "base";
    const fs::path md = t / "GameData" / "MD";
    fs::create_directories(base);
    fs::create_directories(md);
    std::string fehler;
    pruefe(writePk3((base / "assets1.pk3").string(), {{"gfx/mp/new_f_icon_push.tga", "base"}, {"ui/beide.menu", "base"}, {"nur/pk3.txt", "pk3-base"}, {"shaders/ui.shader", "a"}}, &fehler), "pk3 base");
    pruefe(writePk3((md / "MD_Assets.pk3").string(), {{"ui/beide.menu", "md"}, {"shaders/md.shader", "b"}}, &fehler), "pk3 MD");
    lose(base / "nur" / "pk3.txt", "lose-base");       // pk3 geht vor lose im selben Ordner
    lose(md / "nur" / "pk3.txt", "lose-md");           // spaeterer Ordner geht vor, auch lose
    lose(md / "shaders" / "lose.shader", "c");
    lose(md / "gfx" / "lose.tga", "d");

    // Nur der Mod-Ordner eingerichtet: base kommt davor (niedrigster Rang), wie FS_Startup
    const std::vector<std::string> nurMd{md.string()};
    const std::vector<std::string> mit = spielordnerMitBase(nurMd);
    pruefe(mit.size() == 2 && fs::path(mit[0]).filename() == "base" && mit[1] == md.string(), "nur MD: base kommt davor");
    const std::vector<std::string> beide{base.string(), md.string()};
    pruefe(spielordnerMitBase(beide) == beide, "base und MD: nichts dazu");
    const std::vector<std::string> umgekehrt{md.string(), base.string()};
    pruefe(spielordnerMitBase(umgekehrt) == umgekehrt, "eigene Reihenfolge bleibt");
    const std::vector<std::string> nurBase{base.string()};
    pruefe(spielordnerMitBase(nurBase) == nurBase, "nur base: nichts dazu");
    const std::vector<std::string> gross{(t / "GameData" / "BASE").string(), md.string()};
    pruefe(spielordnerMitBase(gross) == gross, "Gross/Klein beim Vergleich egal");
    const std::vector<std::string> ohneNachbar{(t / "woanders" / "MD").string()};
    pruefe(spielordnerMitBase(ohneNachbar) == ohneNachbar, "kein base daneben: nichts dazu");

    Spieldaten d;
    for (const std::string& o : mit) { (void)d.ordnerDazu(o); }
    pruefe(gelesen(d, "GFX/MP/NEW_f_icon_push.TGA") == "base", "Gross/Klein: Machtsymbol aus base");
    pruefe(gelesen(d, "ui/beide.menu") == "md", "spaeterer Ordner gewinnt");
    pruefe(d.gibt("gfx/lose.tga"), "gibt findet lose Dateien");
    pruefe(!d.gibt("gfx/nichts.tga"), "gibt: was es nicht gibt");

    // Lose im selben Ordner: die pk3 geht vor (fs_dirbeforepak 0), der hoehere Ordner trotzdem zuerst
    Spieldaten nurBaseDaten;
    (void)nurBaseDaten.ordnerDazu(base.string());
    pruefe(gelesen(nurBaseDaten, "nur/pk3.txt") == "pk3-base", "pk3 vor loser Datei desselben Ordners");
    pruefe(gelesen(d, "nur/pk3.txt") == "lose-md", "loser Datei im hoeheren Ordner vor pk3 im niedrigeren");

    // Auflisten: auch lose, ohne Doppelte; listeWieSpiel: hoechster Rang zuerst
    const std::vector<std::string> sh = d.listeWieSpiel("shaders/", {".shader"});
    pruefe(sh.size() == 3 && sh[0] == "shaders/md.shader" && sh[1] == "shaders/lose.shader" && sh[2] == "shaders/ui.shader",
           "Suchreihenfolge der Shader: " + (sh.empty() ? std::string() : sh[0]));
    const std::vector<std::string> so = d.liste("shaders/", {".shader"});
    pruefe(so.size() == 3 && so[0] == "shaders/lose.shader" && so[2] == "shaders/ui.shader", "liste bleibt sortiert");

    fs::remove_all(t, ec);
    if (g_fehler == 0) { std::printf("menuespieldaten: alles richtig\n"); }
    return g_fehler == 0 ? 0 : 1;
}
