// kartetexturen.cpp - Karten-Editor Stufe 1a, Punkt 2g: Dateisystem und
// Textur-Inventar (bhed/karte/texturen.h) wie NRC (vfs.cpp, shaders.cpp,
// texwindow.cpp, image.cpp).
//
// 1. Ein kleiner Spielordner wird im Temp-Verzeichnis gebaut (zwei .pk3 und
//    lose Dateien): Archivreihenfolge (Ordner, dann .pk3 absteigend, mp_ im
//    Modus sp uebergangen), shaderlist.txt, qer_editorimage/surfaceparm/
//    qer_trans, Verzeichnisse, Inhalt eines Verzeichnisses (ignorierte
//    Endungen wie _glow), Texturgroesse aus dem Editorbild (spaeteres Paket
//    gewinnt), 64x64 ohne Bild, Rueckfall ohne shaderlist.txt.
// 2. Optional die echten Spieldaten (nur lesen): JA, JK2, Movie Duels -
//    Zahlen fuer die Notizen.
//
// Aufruf: kartetexturen [<JA base>] [<JK2 base>] [<MD base> <MD mod>]
#include "bhed/image.h"
#include "bhed/karte/texturen.h"
#include "bhed/pk3.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string tga(int w, int h) {
    std::vector<unsigned char> rgba(static_cast<std::size_t>(w * h * 4), 128);
    const auto b = image::encodeTga(rgba.data(), w, h);
    return std::string(b.begin(), b.end());
}

void schreibe(const std::filesystem::path& p, const std::string& inhalt) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    f << inhalt;
}

void zahlen(const char* titel, const std::vector<std::string>& ordner) {
    const auto t0 = std::chrono::steady_clock::now();
    Dateisystem fs;
    for (const auto& o : ordner) { fs.ordnerHinzufuegen(o, "sp"); }
    Texturinventar inv;
    inv.laden(fs);
    const auto verz = inv.verzeichnisse();
    std::size_t texturen = 0;
    for (const auto& d : verz) { texturen += inv.verzeichnis(d).size(); }
    const auto t1 = std::chrono::steady_clock::now();
    std::size_t fehler = 0;
    for (const auto& m : inv.meldungen()) { fehler += m.rfind("Error parsing", 0) == 0 || m.rfind("Unable", 0) == 0; }
    std::printf("%s: %zu Archive, %zu Shaderdateien, %zu Shader, %zu Verzeichnisse, %zu Texturen in den Verzeichnissen, "
                "%zu Fehlermeldungen, %.0f ms\n",
                titel, fs.anzahl(), inv.shaderDateien().size(), inv.shader().size(), verz.size(), texturen, fehler,
                std::chrono::duration<double, std::milli>(t1 - t0).count());
    for (const auto& m : inv.meldungen()) {
        if (m.rfind("Error parsing", 0) == 0 || m.rfind("Unable", 0) == 0) { std::printf("  %s\n", m.c_str()); }
    }
    pruefe(!inv.shader().empty() && !verz.empty(), std::string(titel) + ": Shader und Verzeichnisse gefunden");
    const auto caulk = inv.groesse("textures/system/caulk");
    std::printf("  textures/system/caulk: %zux%zu\n", caulk.first, caulk.second);
}

}  // namespace

int main(int argc, char** argv) {
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "behaved_kartetexturen";
    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
    const std::filesystem::path base = tmp / "base";

    // --- Spielordner bauen ------------------------------------------------
    {
        std::vector<Pk3Datei> a0 = {
            {"textures/stone/a.tga", tga(32, 16)},
            {"textures/stone/b_glow.tga", tga(8, 8)},
            {"textures/metal/plate.tga", tga(128, 32)},
            {"shaders/stone.shader",
             "// Kommentar\ntextures/stone/fancy\n{\n\tqer_editorimage textures/stone/a.tga\n\tsurfaceparm nonsolid\n"
             "\tqer_trans 0.4\n\t{\n\t\tmap textures/stone/a.tga\n\t}\n}\n"
             "textures/stone/nebel\n{\n\tsurfaceparm fog\n}\n"
             "textures/stone/fancy\n{\n\tsurfaceparm water\n}\n"
             "textures/wood/nur_shader\n{\n}\n"},
        };
        std::vector<Pk3Datei> a1 = {{"textures/stone/a.tga", tga(64, 8)}};
        std::vector<Pk3Datei> mp = {{"textures/mp/x.tga", tga(4, 4)}};
        std::filesystem::create_directories(base);
        pruefe(writePk3((base / "assets0.pk3").string(), a0) && writePk3((base / "assets1.pk3").string(), a1)
                   && writePk3((base / "mp_extra.pk3").string(), mp),
               "Test-.pk3 geschrieben");
        schreibe(base / "textures" / "wood" / "w.tga", tga(16, 64));
        schreibe(base / "shaders" / "shaderlist.txt", "stone\nmissing\nSTONE\n");
    }

    Dateisystem fs;
    fs.ordnerHinzufuegen(base.string(), "sp");
    const auto archive = fs.archive(false);
    pruefe(archive.size() == 3 && archive[0].back() == '/' && archive[1].find("assets1.pk3") != std::string::npos
               && archive[2].find("assets0.pk3") != std::string::npos,
           "InitDirectory: Ordner, dann assets1, assets0 (absteigend); mp_extra im Modus sp uebergangen");

    Texturinventar inv;
    inv.laden(fs);
    const auto& dateien = inv.shaderDateien();
    pruefe(dateien.size() == 2 && dateien[0] == "stone.shader" && dateien[1] == "missing.shader",
           "shaderlist.txt: .shader angehaengt, Dublette ohne Gross/klein entfernt");
    bool dublette = false;
    bool fehlt = false;
    for (const auto& m : inv.meldungen()) {
        dublette = dublette || m == "duplicate entry \"stone.shader\" in shaderlist.txt";
        fehlt = fehlt || m == "Unable to read shaderfile shaders/missing.shader";
    }
    pruefe(dublette && fehlt, "Meldungen 'duplicate entry' und 'Unable to read shaderfile' wie NRC");
    const ShaderEintrag* fancy = inv.shaderFinden("TEXTURES/stone/FANCY");
    pruefe(fancy && fancy->texturName == "textures/stone/a" && (fancy->flags & QER_NONSOLID) && (fancy->flags & QER_TRANS)
               && fancy->trans == 0.4f && !(fancy->flags & QER_LIQUID),
           "Shader: qer_editorimage ohne Endung, nonsolid, qer_trans; zweite Definition verworfen");
    const ShaderEintrag* nebel = inv.shaderFinden("textures/stone/nebel");
    pruefe(nebel && nebel->trans == 0.35f && (nebel->flags & QER_FOG), "surfaceparm fog: trans 0.35, QER_FOG");
    const auto verz = inv.verzeichnisse();
    pruefe(verz.size() == 3 && verz[0] == "metal" && verz[1] == "stone" && verz[2] == "wood",
           "Verzeichnisse: aus textures/ der Archive und den Shadernamen, sortiert");
    const auto stone = inv.verzeichnis("stone");
    pruefe(stone.size() == 3 && stone[0].name == "textures/stone/a" && stone[0].istStandard() && stone[1].name == "textures/stone/fancy"
               && !stone[1].istStandard() && stone[2].name == "textures/stone/nebel",
           "Verzeichnis stone: Bild a (ohne Shader), Shader fancy und nebel; b_glow ignoriert");
    const auto wood = inv.verzeichnis("wood/");
    pruefe(wood.size() == 2 && wood[0].name == "textures/wood/nur_shader" && wood[1].name == "textures/wood/w",
           "Verzeichnis wood: Shader ohne Bild und loses Bild");
    pruefe(inv.groesse("textures/stone/fancy") == std::make_pair<std::size_t, std::size_t>(64, 8),
           "Groesse fancy = Editorbild a aus assets1 (64x8, spaeteres Paket gewinnt)");
    pruefe(inv.groesse("textures/metal/plate") == std::make_pair<std::size_t, std::size_t>(128, 32), "Groesse eines Bilds ohne Shader");
    pruefe(inv.groesse("textures/wood/nur_shader") == std::make_pair<std::size_t, std::size_t>(64, 64)
               && inv.groesse("textures/gibt/es_nicht") == std::make_pair<std::size_t, std::size_t>(64, 64),
           "ohne Bild 64x64 (notex/shadernotex)");
    pruefe(texturNameIgnorieren("textures/x/Y_GLOW") && texturNameIgnorieren("a.specular") && !texturNameIgnorieren("textures/x/glowing"),
           "texture_name_ignore");

    // --- ohne shaderlist.txt: alle Shader ---------------------------------
    {
        std::filesystem::remove(base / "shaders" / "shaderlist.txt", ec);
        Dateisystem fs2;
        fs2.ordnerHinzufuegen(base.string(), "sp");
        Texturinventar inv2;
        inv2.laden(fs2);
        bool alle = false;
        for (const auto& m : inv2.meldungen()) { alle = alle || m == "No shaderlist.txt found: loading all shaders"; }
        pruefe(alle && inv2.shaderDateien().size() == 1 && inv2.shaderDateien()[0] == "stone.shader",
               "ohne shaderlist.txt: 'loading all shaders', shaders/*.shader");
    }
    std::filesystem::remove_all(tmp, ec);

    // --- echte Spieldaten (nur lesen) --------------------------------------
    if (argc >= 2 && std::filesystem::is_directory(argv[1])) { zahlen("Jedi Academy", {argv[1]}); }
    if (argc >= 3 && std::filesystem::is_directory(argv[2])) { zahlen("Jedi Outcast", {argv[2]}); }
    if (argc >= 5 && std::filesystem::is_directory(argv[3]) && std::filesystem::is_directory(argv[4])) {
        zahlen("Movie Duels", {argv[4], argv[3]});   // QE_InitVFS: Mod vor Basis
    }

    std::printf("kartetexturen: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
