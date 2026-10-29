// karteueberblenden.cpp - Stufe 9 Punkt 9: Texturen ueberblenden (ctest karte_ueberblenden)
//
// Bildpunkt wie q3map2 GetShaderIndexForPoint (oben = groesstes Y), Malen im Kreis, TGA hin und
// zurueck so, wie LoadEntityIndexMap liest ((Rot * Schichten) / 256), Grenzen einer func_group
// (Pinsel und Patch-Kontrollpunkte, "min"/"max" gehen vor), Namen der Shader wie GetIndexedShader.
// Fuer tools/karte/q3map2_ueberblenden.py:
//   karteueberblenden --shader <basis> <textur>...   Shader-Text auf stdout
//   karteueberblenden --tga <datei>                  4 x 4, linke Haelfte Schicht 0, rechte 1
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/ueberblenden.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
}  // namespace

int main(int argc, char** argv) {
    using namespace bhed::karte;
    if (argc >= 3 && std::strcmp(argv[1], "--shader") == 0) {
        std::vector<std::string> t;
        for (int i = 3; i < argc; ++i) { t.emplace_back(argv[i]); }
        std::fputs(blendShaderText(argv[2], t).c_str(), stdout);
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--tga") == 0) {
        IndexKarte im;
        im.breite = im.hoehe = 4;
        im.schichten = 2;
        im.index = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1};
        const std::vector<unsigned char> d = blendTga(im);
        std::ofstream(argv[2], std::ios::binary).write(reinterpret_cast<const char*>(d.data()), static_cast<std::streamsize>(d.size()));
        return 0;
    }

    std::printf("--- Bildpunkt wie GetShaderIndexForPoint ---\n");
    const float mins[3] = {-128, -64, 0};
    const float maxs[3] = {128, 64, 16};
    int px = 0, py = 0;
    blendBildpunkt(mins, maxs, 8, 4, -128, 64, px, py);
    expect("links oben: (-128, 64) -> (0, 0)", px == 0 && py == 0);
    blendBildpunkt(mins, maxs, 8, 4, 128, -64, px, py);
    expect("rechts unten: (128, -64) -> (7, 3) (geklemmt)", px == 7 && py == 3);
    blendBildpunkt(mins, maxs, 8, 4, 0, 0, px, py);
    expect("Mitte: (0, 0) -> (4, 2)", px == 4 && py == 2);

    std::printf("--- Malen ---\n");
    IndexKarte im;
    im.breite = 8;
    im.hoehe = 4;
    im.schichten = 3;
    im.index.assign(32, 0);
    const int n = blendMalen(im, mins, maxs, 112, 48, 20, 2);   // Mitte des Punkts (7, 0): 112, 48
    expect("ein Punkt rechts oben (Radius 20) wird Schicht 2", n == 1 && im.index[7] == 2);
    expect("Schicht ausserhalb (3 bei 3 Schichten) malt nichts", blendMalen(im, mins, maxs, 0, 0, 500, 3) == 0);

    std::printf("--- TGA wie LoadEntityIndexMap ---\n");
    im.index = {0, 1, 2, 0, 1, 2, 0, 1, 2, 2, 1, 0, 0, 0, 1, 1, 2, 2, 0, 1, 0, 2, 1, 2, 2, 2, 2, 1, 1, 1, 0, 0};
    const std::vector<unsigned char> d = blendTga(im);
    bool rotGut = d.size() == 18 + 32 * 4 && d[2] == 2 && d[16] == 32 && (d[17] & 0x20) != 0;
    for (std::size_t i = 0; i < im.index.size() && rotGut; ++i) {
        const unsigned rot = d[18 + i * 4 + 2];
        rotGut = (rot * 3U) / 256U == im.index[i];   // ( pixels32[ i ] & 0xFF ) * numLayers / 256
    }
    expect("32 Bit, oben links, Rot * 3 / 256 = Schicht", rotGut);
    IndexKarte z;
    expect("zurueckgelesen gleich", blendTgaLesen(d, 3, z) && z.breite == 8 && z.hoehe == 4 && z.index == im.index);

    std::printf("--- Grenzen einer func_group ---\n");
    Karte k;
    // der Quader aus karteoptsicher.cpp: x0..x1, y und z 0..64
    char q[2048];
    const int x0 = -64, x1 = 64;
    std::snprintf(q, sizeof(q),
                  "{\n( %d 0 0 ) ( %d 64 0 ) ( %d 0 64 ) a 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 0 ) ( %d 0 64 ) ( %d 64 0 ) a 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 0 ) ( %d 0 64 ) ( %d 0 0 ) a 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 64 0 ) ( %d 64 0 ) ( %d 64 64 ) a 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 0 ) ( %d 0 0 ) ( %d 64 0 ) a 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d 0 64 ) ( %d 64 64 ) ( %d 0 64 ) a 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, x0, x0, x1, x1, x1, x0, x0, x1, x0, x1, x0, x0, x1, x0, x0, x0, x1);
    const std::string text = std::string("{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"func_group\"\n") + q + "}\n";
    const bool gelesen = karteLesen(text, k).vollstaendig;
    float mn[3] = {0, 0, 0}, mx[3] = {0, 0, 0};
    bool grenzen = false;
    if (gelesen && k.entitaeten().size() == 2) { grenzen = blendGrenzen(*k.entitaeten()[1], k.kontext, mn, mx); }
    expect("Pinsel -64..64 x 0..64 x 0..64", grenzen && mn[0] == -64 && mx[0] == 64 && mn[1] == 0 && mx[1] == 64 && mn[2] == 0 && mx[2] == 64);
    if (grenzen) {
        k.entitaeten()[1]->setzen("min", "-256 -256 -16");
        k.entitaeten()[1]->setzen("max", "256 256 64");
        grenzen = blendGrenzen(*k.entitaeten()[1], k.kontext, mn, mx);
    }
    expect("\"min\"/\"max\" gehen vor", grenzen && mn[0] == -256 && mx[1] == 256 && mx[2] == 64);

    std::printf("--- Shader wie GetIndexedShader ---\n");
    const std::string s = blendShaderText("jks_blend/test", {"textures/a/eins", "textures/a/zwei", "textures/a/drei"});
    const auto hat = [&s](const char* n) { return s.find(std::string(n) + "\n{") != std::string::npos; };
    expect("Grund-Shader mit q3map_terrain", hat("textures/jks_blend/test") && s.find("q3map_terrain") != std::string::npos);
    expect("je Schicht _0 _1 _2", hat("textures/jks_blend/test_0") && hat("textures/jks_blend/test_1") && hat("textures/jks_blend/test_2"));
    expect("je Paar _0to1 _0to2 _1to2 (zweite Schicht ueber alphaGen vertex)",
           hat("textures/jks_blend/test_0to1") && hat("textures/jks_blend/test_0to2") && hat("textures/jks_blend/test_1to2") && s.find("alphaGen vertex") != std::string::npos);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
