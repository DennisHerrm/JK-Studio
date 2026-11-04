// losedateientest.cpp - rc609 W2b F17: lose Dateien in den Spielordnern wie das Spiel
//
// FS_AddGameDirectory (OJK code/qcommon/files.cpp:2730-2805, MD9 :2810-2885): je
// Ordner die .pk3 und die losen Dateien, spaetere Ordner zuerst; mit
// fs_dirbeforepak 0 (Vorgabe) die .pk3 eines Ordners vor seinen losen Dateien,
// mit 1 umgekehrt. Zwei nachgebaute Spielordner im Arbeitsordner.
//
// Aufruf: losedateientest <Arbeitsordner>
#include "bhed/pk3.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

using namespace bhed;
namespace fs = std::filesystem;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
void schreib(const fs::path& p, const std::string& t) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}
std::string lies(const std::vector<GamePath>& o, const std::string& name, bool dirVorPak, std::string* her = nullptr, bool* lose = nullptr) {
    std::string out;
    if (!liesWieSpiel(o, name, dirVorPak, out, her, lose)) { return "<nichts>"; }
    return out;
}
}  // namespace

int main(int argc, char** argv) {
    const fs::path w = fs::path(argc > 1 ? argv[1] : ".") / "losedateien";
    std::error_code ec;
    fs::remove_all(w, ec);
    const fs::path base = w / "GameData" / "base";
    const fs::path md = w / "GameData" / "MD";
    // base: assets0.pk3 (x.txt, y.txt), dazu lose y.txt und z.txt; MD: lose x.txt, zz.pk3 mit y.txt
    fs::create_directories(base, ec);
    fs::create_directories(md, ec);
    std::string fehler;
    const bool a = writePk3((base / "assets0.pk3").string(), {{"ext_data/x.txt", "base-pk3-x"}, {"ext_data/y.txt", "base-pk3-y"}}, &fehler);
    const bool b = writePk3((md / "zz_md.pk3").string(), {{"ext_data/y.txt", "md-pk3-y"}}, &fehler);
    schreib(base / "ext_data" / "y.txt", "base-lose-y");
    schreib(base / "ext_data" / "z.txt", "base-lose-z");
    schreib(md / "ext_data" / "x.txt", "md-lose-x");
    schreib(md / "ext_data" / "q.txt", "md-lose-q");
    pruefe(a && b, "zwei Spielordner angelegt " + fehler);
    std::vector<GamePath> o(2);
    (void)scanGamePath(base.string(), o[0]);
    (void)scanGamePath(md.string(), o[1]);

    std::printf("fs_dirbeforepak 0 (Vorgabe)\n");
    std::string her;
    bool lose = false;
    pruefe(lies(o, "ext_data/x.txt", false, &her, &lose) == "md-lose-x" && lose && her.find("MD") != std::string::npos,
           "lose Datei im spaeteren Ordner (MD) vor der .pk3 von base");
    pruefe(lies(o, "ext_data/y.txt", false, &her, &lose) == "md-pk3-y" && !lose, "die .pk3 von MD vor allem in base");
    pruefe(lies(o, "ext_data/z.txt", false) == "base-lose-z", "nur lose in base: gefunden");
    pruefe(lies(o, "ext_data/q.txt", false) == "md-lose-q", "nur lose in MD: gefunden");
    {
        // Ein Ordner mit .pk3 und loser Datei desselben Namens.
        std::vector<GamePath> nurBase(1, o[0]);
        pruefe(lies(nurBase, "ext_data/y.txt", false, &her, &lose) == "base-pk3-y" && !lose, "gleicher Ordner: .pk3 vor der losen Datei");
        std::printf("fs_dirbeforepak 1\n");
        pruefe(lies(nurBase, "ext_data/y.txt", true, &her, &lose) == "base-lose-y" && lose, "gleicher Ordner: lose Datei vor der .pk3");
    }
    pruefe(lies(o, "ext_data/y.txt", true) == "md-pk3-y", "spaeterer Ordner bleibt vorn (MD hat keine lose y.txt)");
    pruefe(lies(o, "ext_data/x.txt", true) == "md-lose-x", "lose x.txt in MD");
    std::printf("Namen\n");
    pruefe(lies(o, "/ext_data/z.txt", false) == "base-lose-z", "fuehrender Strich faellt weg (files.cpp:1276)");
    pruefe(lies(o, "ext_data/../ext_data/z.txt", false) == "<nichts>", "\"..\" findet nichts (files.cpp:1283)");
    pruefe(lies(o, "EXT_DATA/X.TXT", false) == "md-lose-x", "Gross/Klein egal (Windows-Dateisystem, FS_FilenameCompare)");

    fs::remove_all(w, ec);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "losedateien: alles gut" : "losedateien: FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
