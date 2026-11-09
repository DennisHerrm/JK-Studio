// shadervorrang.cpp - REST7 Punkt 1: welcher Eintrag eines mehrfach stehenden Shadernamens gilt, wie im Spiel.
//
// FS_ListFiles("shaders", ".shader") (fsListFiles, bhed/pk3.h) und ScanAndLoadShaderFiles/SetupShaderEntryPtrs
// (shaderTextWieSpiel, bhed/shaderscript.h): spaeter gelistete Dateien gewinnen, in einer Datei der erste Eintrag,
// COM_Compress (OpenJK/Movie Duels) bzw. der rohe Text (Jedi Outcast), Nullbyte und offene Klammern wie strcat und
// SkipBracedSection. Dazu die alte Regel der Vorschau (alle Dateien nach Namen sortiert, der letzte Eintrag gilt) als
// Gegenprobe: sie haette hier anders entschieden.
//
// REST8: die Regel von bhed_q3map2 (bhed/karte/q3map2shader.h: vfs.cpp, scriplib.cpp, LoadShaderInfo, ParseShaderFile) fuer das
// Echtzeit-Licht - lose Dateien vor jeder .pk3, die .pk3 von base vor denen des Mods, der erste Eintrag gilt; shaderlist.txt,
// ":q3map", der Rest einer Zeile, Matrizen ueber Zeilen, Bytes ab 0x80, ';'-Kommentar. Gegenprobe: das Inventar von
// NetRadiant-Custom (bis REST8 die Quelle des Echtzeit-Lichts) und die Regel des Spiels entscheiden hier anders.
#include "bhed/karte/q3map2shader.h"
#include "bhed/karte/texturen.h"
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

    // --- REST8: bhed_q3map2 ---------------------------------------------------------------------------------------------------
    {
        const fs::path w = fs::temp_directory_path(ec) / "bhed_shadervorrang_q3";
        fs::remove_all(w, ec);
        const fs::path qb = w / "GameData" / "base";
        const fs::path qm = w / "GameData" / "MD";
        fs::create_directories(qb);
        fs::create_directories(qm);
        const auto sh = [](const char* name, const char* b) { return std::string(name) + "\n{\n{\nmap " + b + "\n}\n}\n"; };
        pruefe(writePk3((qb / "assets0.pk3").string(), {{"shaders/a.shader", sh("textures/q/a", "a_base0")}}, &fehler), "q3 assets0.pk3");
        pruefe(writePk3((qb / "assets1.pk3").string(),
                        {{"shaders/b.shader", sh("textures/q/b", "b_base1") + sh("textures/q/b", "b_zweit") + sh("textures/q/e:q3map", "e_q3map") +
                                                  "textures/q/f\n{\n{\nmap f_bild\n} }\ntextures/q/g\n{\n}\n}\n" +
                                                  "textures/q/h\n{\nq3map_fogDir\n( 0 0 1\n)\n{\nmap h_bild\n}\n}\n" + "textures/q/i\xA0{\n{\nmap i_bild\n}\n}\n" +
                                                  "textures/q/j ; kommentar {\n{\n{\nmap j_bild\n}\n}\n"}},
                        &fehler),
               "q3 assets1.pk3");
        pruefe(writePk3((qm / "md_a.pk3").string(), {{"shaders/c.shader", sh("textures/q/b", "b_md_a") + sh("textures/q/c", "c_md_a")}}, &fehler), "q3 md_a.pk3");
        pruefe(writePk3((qm / "md_z.pk3").string(), {{"shaders/d.shader", sh("textures/q/c", "c_md_z")}}, &fehler), "q3 md_z.pk3");
        lose(qb / "shaders" / "a.shader", sh("textures/q/a", "a_lose"));
        const std::vector<std::string> verz = karte::q3map2Verzeichnisse((w / "GameData").generic_string(), "MD", "base", {});
        pruefe(verz.size() == 2 && verz[0] == (w / "GameData" / "MD").generic_string() + "/" && verz[1] == (w / "GameData" / "base").generic_string() + "/",
               "InitPaths: <basepath>/MD/, <basepath>/base/");
        const std::vector<std::string> einmal = karte::q3map2Verzeichnisse("C:/x", "BASE", "base", {"C:\\p", "", "c:/P/"});
        pruefe(einmal == std::vector<std::string>{"C:/x/BASE/", "C:/p/"}, "insert_unique ohne Gross/klein, -fs_pakpath einmal: " + folge(einmal));
        const karte::Q3map2Shader q = karte::q3map2ShaderLesen(verz);
        const auto q3bild = [&q](const char* name) {
            const auto it = q.eintraege.find(name);
            if (it == q.eintraege.end()) { return std::string("(keiner)"); }
            ShaderMap m;
            parseShaderScript(it->second.text, m);
            const auto s = m.find(name);
            return s == m.end() ? std::string("(nicht zerlegt)") : s->second.image;
        };
        pruefe(q.abbruch.empty() && !q.shaderliste, "q3map2: kein Abbruch, ohne shaderlist.txt: " + q.abbruch);
        pruefe(q.dateien == std::vector<std::string>{"a.shader", "b.shader", "d.shader", "c.shader"},
               "vfsListShaderFiles: lose zuerst, dann base (assets1 vor assets0), dann MD (md_z vor md_a): " + folge(q.dateien));
        pruefe(q3bild("textures/q/a") == "a_lose", "die lose a.shader gewinnt gegen die in assets0.pk3: " + q3bild("textures/q/a"));
        pruefe(q3bild("textures/q/b") == "b_base1", "b: base vor dem Mod, in der Datei der erste: " + q3bild("textures/q/b"));
        pruefe(q3bild("textures/q/c") == "c_md_z", "c: md_z.pk3 vor md_a.pk3: " + q3bild("textures/q/c"));
        pruefe(q3bild("textures/q/e") == "e_q3map", "\":q3map\" faellt weg");
        pruefe(q3bild("textures/q/f") == "f_bild" && q.eintraege.count("textures/q/g") == 0,
               "nach dem Ende einer Stufe zaehlt der Rest der Zeile nicht: \"} }\" schliesst f nicht, g gehoert zu f");
        pruefe(q3bild("textures/q/h") == "h_bild", "q3map_fogDir: \"(\" und \")\" der Matrix auf anderen Zeilen");
        pruefe(q3bild("textures/q/i") == "i_bild", "ein Byte ab 0x80 trennt wie Leerraum (char mit Vorzeichen)");
        pruefe(q3bild("textures/q/j") == "j_bild", "';' beginnt einen Kommentar bis zum Zeilenende");
        pruefe(q.verdeckt == 3, "verdeckt: b zweimal in b.shader und in c.shader, c in c.shader: " + std::to_string(q.verdeckt));
        const auto qa = q.eintraege.find("textures/q/a");
        pruefe(qa != q.eintraege.end() && qa->second.datei == "shaders/a.shader" && karte::pfadBlatt(qa->second.quelle) == "base/",
               "Quelle von a: der Ordner base/");

        // Die Regel des Spiels (Movie Duels, fs_dirbeforepak 0) auf denselben Daten: a aus assets0 (in einem Ordner die .pk3 vor
        // den losen Dateien), c aus md_a (spaeter gelistete Datei)
        std::vector<GamePath> spielOrdner(2);
        (void)scanGamePath(qb.string(), spielOrdner[0]);
        (void)scanGamePath(qm.string(), spielOrdner[1]);
        const ShaderTextWieSpiel st = shaderTextAusOrdnern(spielOrdner, kRegelwerkMovieDuels, false);
        ShaderMap sm;
        parseShaderTextWieSpiel(st, sm);
        pruefe(sm["textures/q/a"].image == "a_base0" && sm["textures/q/b"].image == "b_base1" && sm["textures/q/c"].image == "c_md_a",
               "Spiel: a_base0, b_base1, c_md_a (" + sm["textures/q/a"].image + " " + sm["textures/q/b"].image + " " + sm["textures/q/c"].image + ")");
        std::string spielA;
        std::string spielB;
        for (const std::string& e : st.eintraege) {
            if (e.rfind("textures/q/a\n", 0) == 0) { spielA = e; }
            if (e.rfind("textures/q/b\n", 0) == 0) { spielB = e; }
        }
        pruefe(!karte::shaderEintraegeGleich(qa->second.text, spielA) && karte::shaderEintraegeGleich(q.eintraege.at("textures/q/b").text, spielB),
               "shaderEintraegeGleich: a verschieden aufgeloest, b gleich");

        // Gegenprobe: das Inventar von NetRadiant-Custom (Mod-Ordner zuerst, je Ordner die losen Dateien, dann die .pk3 absteigend)
        // nimmt b aus c.shader des Mods - bis REST8 zeigte das Echtzeit-Licht so b_md_a
        karte::Dateisystem nrcFs;
        nrcFs.ordnerHinzufuegen(qm.generic_string(), "sp");
        nrcFs.ordnerHinzufuegen(qb.generic_string(), "sp");
        karte::Texturinventar inv;
        inv.laden(nrcFs);
        const karte::ShaderEintrag* nb = inv.shaderFinden("textures/q/b");
        pruefe(nb != nullptr && nb->datei == "shaders/c.shader", "Gegenprobe NRC-Inventar: b aus shaders/c.shader (nicht wie q3map2)");

        // shaderlist.txt: nur die genannten Dateien
        lose(qb / "shaders" / "shaderlist.txt", "b\nd.shader // Kommentar\nB\n");
        const karte::Q3map2Shader ql = karte::q3map2ShaderLesen(verz);
        pruefe(ql.shaderliste && ql.dateien == std::vector<std::string>{"b.shader", "d.shader"} && ql.eintraege.count("textures/q/a") == 0 &&
                   ql.eintraege.count("textures/q/c") == 1,
               "shaderlist.txt: nur b.shader und d.shader (eindeutig ohne Gross/klein): " + folge(ql.dateien));
        // Ohne "{" nach dem Namen bricht q3map2 ab (Error) - danach keine Eintraege mehr
        lose(qb / "shaders" / "shaderlist.txt", "a\nkaputt\nd\n");
        lose(qb / "shaders" / "kaputt.shader", "textures/q/k\nmap x\n");
        const karte::Q3map2Shader qk = karte::q3map2ShaderLesen(verz);
        pruefe(!qk.abbruch.empty() && qk.eintraege.count("textures/q/a") == 1 && qk.eintraege.count("textures/q/c") == 0,
               "fehlendes { : Abbruch, d.shader danach nicht gelesen: " + qk.abbruch);
        fs::remove_all(w, ec);
    }

    if (g_fehler == 0) { std::printf("shadervorrang: alles richtig\n"); }
    return g_fehler == 0 ? 0 : 1;
}
