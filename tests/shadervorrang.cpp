// shadervorrang.cpp - REST7 Punkt 1: welcher Eintrag eines mehrfach stehenden Shadernamens gilt, wie im Spiel.
//
// FS_ListFiles("shaders", ".shader") (fsListFiles, bhed/pk3.h) und ScanAndLoadShaderFiles/SetupShaderEntryPtrs
// (shaderTextWieSpiel, bhed/shaderscript.h): spaeter gelistete Dateien gewinnen, in einer Datei der erste Eintrag,
// COM_Compress (OpenJK/Movie Duels) bzw. der rohe Text (Jedi Outcast), Nullbyte und offene Klammern wie strcat und
// SkipBracedSection. Dazu die alte Regel der Vorschau (alle Dateien nach Namen sortiert, der letzte Eintrag gilt) als
// Gegenprobe: sie haette hier anders entschieden.
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"
#include "bhed/shaderscript.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace bhed;
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
std::string bild(const std::vector<std::string>& texte, int spiel, const char* name) {
    ShaderMap m;
    parseShaderTextWieSpiel(shaderTextWieSpiel(texte, spiel), m);
    const auto it = m.find(name);
    return it == m.end() ? std::string("(keiner)") : it->second.image;
}
std::string folge(const std::vector<std::string>& v) {
    std::string s;
    for (const std::string& x : v) { s += x + " "; }
    return s;
}
}  // namespace

int main() {
    // --- FS_ListFiles --------------------------------------------------------------------------------------------------
    std::error_code ec;
    const fs::path t = fs::temp_directory_path(ec) / "bhed_shadervorrang";
    fs::remove_all(t, ec);
    const fs::path base = t / "GameData" / "base";
    const fs::path md = t / "GameData" / "MD";
    fs::create_directories(base);
    fs::create_directories(md);
    std::string fehler;
    // Zip-Folge b, a, sub/c, sub/tief/d (drei Ebenen: nicht gelistet), scripts/x (anderer Ordner), shaders.shader (kein Ordner)
    pruefe(writePk3((base / "assets0.pk3").string(),
                    {{"shaders/b.shader", "b0"}, {"shaders/A.shader", "a0"}, {"shaders/sub/c.shader", "c0"}, {"shaders/sub/tief/d.shader", "d0"},
                     {"scripts/x.shader", "x0"}, {"shaders.shader", "s0"}, {"shaders/e.txt", "e0"}},
                    &fehler),
           "assets0.pk3");
    pruefe(writePk3((base / "assets1.pk3").string(), {{"shaders/a.shader", "a1"}}, &fehler), "assets1.pk3");
    pruefe(writePk3((md / "MD_Assets.pk3").string(), {{"shaders/m.shader", "m"}}, &fehler), "MD_Assets.pk3");
    lose(base / "shaders" / "a.shader", "a-lose");   // gleicher Name: schon aus assets1 gelistet
    lose(base / "shaders" / "l.shader", "l-lose");
    lose(base / "shaders" / "unter" / "u.shader", "u-lose");   // Sys_ListFiles ohne Unterordner
    std::vector<GamePath> ordner(2);
    (void)scanGamePath(base.string(), ordner[0]);
    (void)scanGamePath(md.string(), ordner[1]);
    const std::vector<std::string> l0 = fsListFiles(ordner, "shaders", ".shader", false);
    pruefe(l0 == std::vector<std::string>{"m.shader", "a.shader", "b.shader", "sub/c.shader", "l.shader"}, "FS_ListFiles fs_dirbeforepak 0: " + folge(l0));
    const std::vector<std::string> l1 = fsListFiles(ordner, "shaders", ".shader", true);
    pruefe(l1 == std::vector<std::string>{"m.shader", "a.shader", "l.shader", "b.shader", "sub/c.shader"}, "FS_ListFiles fs_dirbeforepak 1: " + folge(l1));
    std::string inhalt;
    pruefe(liesWieSpiel(ordner, "shaders/" + l0[1], false, inhalt) && inhalt == "a1", "FS_ReadFile der gelisteten a.shader: assets1");
    fs::remove_all(t, ec);

    // --- ScanAndLoadShaderFiles + SetupShaderEntryPtrs -----------------------------------------------------------------
    const std::string def = "textures/t/a\n{\n{\nmap a_frueh\n}\n}\n";      // in der frueher gelisteten Datei (hoeherer Rang)
    const std::string spaet = "textures/t/a\n{\n{\nmap a_spaet\n}\n}\n";   // in der spaeter gelisteten Datei
    pruefe(bild({def, spaet}, kRegelwerkMovieDuels, "textures/t/a") == "a_spaet", "die spaeter gelistete Datei gewinnt");
    pruefe(bild({def, spaet}, kRegelwerkOpenJK, "textures/t/a") == "a_spaet", "OpenJK ebenso");
    pruefe(bild({def, spaet}, kRegelwerkJediOutcast, "textures/t/a") == "a_spaet", "Jedi Outcast ebenso");
    const std::string zweimal = "textures/t/b\n{\n{\nmap b_erst\n}\n}\ntextures/t/b\n{\n{\nmap b_zweit\n}\n}\n";
    pruefe(bild({zweimal}, kRegelwerkMovieDuels, "textures/t/b") == "b_erst", "in einer Datei gilt der erste Eintrag");
    const ShaderTextWieSpiel z = shaderTextWieSpiel({def, spaet, zweimal}, kRegelwerkMovieDuels);
    pruefe(z.namen == 2 && z.verdeckt == 2, "Namen 2, verdeckt 2: " + std::to_string(z.namen) + " " + std::to_string(z.verdeckt));
    // Name und Klammer auf einer Zeile: SkipRestOfLine schluckt die Klammer, ParseShader liest sie trotzdem
    pruefe(bild({"textures/t/c {\n{\nmap c_bild\n}\n}\n"}, kRegelwerkMovieDuels, "textures/t/c") == "c_bild", "Name und { auf einer Zeile");
    // Eine Klammer mit einem Befehl auf derselben Zeile (MD: textures/tatooine_outpost/..., "{ map ...") liest COM_ParseExt als
    // eigenes Wort - der Zeilenleser bekommt sie auf eine eigene Zeile, und der Shader danach geht nicht verloren
    const std::string klammer = "textures/t/j\n{\n{ map j_bild\n}\n}\ntextures/t/k\n{\n{\nmap k_bild\n}\n}\n";
    pruefe(bild({klammer}, kRegelwerkMovieDuels, "textures/t/j") == "j_bild" && bild({klammer}, kRegelwerkMovieDuels, "textures/t/k") == "k_bild",
           "{ und Befehl auf einer Zeile, der naechste Shader bleibt");
    // Kommentare zaehlen nicht als Eintrag
    pruefe(bild({"/* textures/t/d\n{\n{\nmap d_kommentar\n}\n} */\n// textures/t/d\n"}, kRegelwerkMovieDuels, "textures/t/d") == "(keiner)",
           "auskommentierter Eintrag gilt nicht");
    // strcat: nach einem Nullbyte ist die Datei zu Ende
    pruefe(bild({std::string("textures/t/e\n{\n{\nmap e_vorn\n}\n}\n") + '\0' + "textures/t/f\n{\n{\nmap f_hinter\n}\n}\n"}, kRegelwerkMovieDuels,
                "textures/t/f") == "(keiner)",
           "nach einem Nullbyte liest das Spiel nichts mehr");
    // Eine offene Klammer am Ende einer Datei verschluckt die im Text folgenden (frueher gelisteten) Dateien
    const std::string offen = "textures/t/g\n{\n{\nmap g_bild\n}\n}\n{\n";
    pruefe(bild({def, offen}, kRegelwerkMovieDuels, "textures/t/a") == "(keiner)", "offene Klammer verschluckt die naechste Datei");
    // Ohne "{" nach dem Namen ist der Shader im Spiel fehlerhaft (defaultShader) - und verdeckt doch die spaeteren
    pruefe(bild({def, "textures/t/a\nmap kaputt\n"}, kRegelwerkMovieDuels, "textures/t/a") == "(keiner)", "Name ohne { verdeckt trotzdem");
    // Jedi Outcast liest den Text ohne COM_Compress: ein mehrzeiliger Kommentar hinter einem Namen wird ab der zweiten Zeile
    // zu Namen (SkipRestOfLine haelt am ersten Zeilenwechsel) - textures/t/h ist dann ein kaputter Eintrag und verdeckt den
    // in der frueher gelisteten Datei. OpenJK entfernt den Kommentar vorher.
    const std::string jk2falle = "textures/t/i /*\ntextures/t/h\n*/\n{\n{\nmap i_bild\n}\n}\n";
    const std::string h = "textures/t/h\n{\n{\nmap h_bild\n}\n}\n";
    pruefe(bild({h, jk2falle}, kRegelwerkOpenJK, "textures/t/h") == "h_bild", "OpenJK: COM_Compress entfernt den Kommentar");
    pruefe(bild({h, jk2falle}, kRegelwerkJediOutcast, "textures/t/h") == "(keiner)", "Jedi Outcast: roher Text, der Name im Kommentar verdeckt");
    pruefe(bild({h, jk2falle}, kRegelwerkJediOutcast, "textures/t/i") == "i_bild", "Jedi Outcast: der Eintrag mit dem Kommentar gilt");
    pruefe(maxShaderDateien(kRegelwerkMovieDuels) == 8192 && maxShaderDateien(kRegelwerkOpenJK) == 4096 && maxShaderDateien(kRegelwerkJediOutcast) == 1024,
           "MAX_SHADER_FILES je Spiel");

    // Gegenprobe: die alte Regel der Vorschau (jede Datei fuer sich nach Dateinamen, der letzte Eintrag gilt) - steht die
    // frueher gelistete Datei im Alphabet hinten, gewinnt sie dort, und in einer Datei der zweite Eintrag
    ShaderMap alt;
    parseShaderScript(spaet, alt);
    parseShaderScript(def, alt);
    parseShaderScript(zweimal, alt);
    pruefe(alt["textures/t/a"].image == "a_frueh" && alt["textures/t/b"].image == "b_zweit", "alte Regel: der letzte Eintrag");

    if (g_fehler == 0) { std::printf("shadervorrang: alles richtig\n"); }
    return g_fehler == 0 ? 0 : 1;
}
