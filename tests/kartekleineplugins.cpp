// kartekleineplugins.cpp - Karten-Editor Stufe 4 Punkt 3: ShaderPlug, SunPlug
// und UFO:AI ohne Oberflaeche (bhed/karte/kleineplugins.h).
//
// Geprueft gegen NRC 20260114:
//   - shaderplug.cpp: nur "textures/..." als Shader, Bilder ohne Shader als
//     Texturen ("textures/" + Datei ohne Endung), der Tag-Baum erst mit den
//     Texturen, dann den Shadern, je rueckwaerts;
//   - sunplug.cpp: GetOptimalCoordinates (Quadrat, halbe Seite >= 175, int
//     abgeschnitten), mapcoordsmins/-maxs lesen, GetMapBounds;
//   - ufoai_level.cpp: Worldspawn (maxlevel 5, maxteams, die Nachrichten
//     ohne Trenner aneinander), Perform check (die Zeilen, maxlevel > 8);
//   - ufoai_filters.cpp: Level-Filter (Inhalt der letzten Flaeche, die
//     Entitaeten nach spawnflags & level), StepOn / NoDraw an und aus.
//
// Aufruf: kartekleineplugins
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/kleineplugins.h"
#include "bhed/karte/texturtags.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed::karte;
namespace kp = bhed::karte::plugins;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string kasten(int x0, int y0, int z0, int x1, int y1, int z1, const char* tex, int inhalt, int flaeche) {
    char t[96];
    std::snprintf(t, sizeof(t), "%s 0 0 0 0.5 0.5 %d %d 0", tex, inhalt, flaeche);
    char z[1600];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s\n}\n",
                  x1, y1, z1, x1, y0, z1, x0, y1, z1, t, x1, y1, z1, x0, y1, z1, x1, y1, z0, t, x1, y1, z1, x1, y1, z0, x1, y0, z1, t, x0, y0, z0, x1, y0, z0,
                  x0, y1, z0, t, x0, y0, z0, x0, y0, z1, x1, y0, z0, t, x0, y0, z0, x0, y1, z0, x0, y0, z1, t);
    return z;
}

void laden(Karte& k, const std::string& text) {
    k.leeren();
    (void)karteLesen(text, k);
    k.anmelden();
}

}  // namespace

int main() {
    // --- ShaderPlug ---
    const kp::ShaderplugListe l = kp::shaderplugSammeln({"textures/a/b", "models/x/y", "textures/c/d"}, {"a/b.jpg", "e/f.tga", "g/h.png"});
    pruefe(l.shader == std::set<std::string>{"textures/a/b", "textures/c/d"} && l.texturen == std::set<std::string>{"textures/e/f", "textures/g/h"},
           "ShaderPlug: Shader nur textures/..., Bilder ohne gleichnamigen Shader als Texturen");
    TexturTags tags;
    kp::shaderplugTags(l, tags);
    const std::string x = tags.text();
    const std::size_t gh = x.find("textures/g/h");
    const std::size_t ef = x.find("textures/e/f");
    const std::size_t cd = x.find("textures/c/d");
    const std::size_t ab = x.find("textures/a/b");
    // AddShaderNode fuegt vorn ein (hinter den ersten Text): rueckwaerts eingefuegt = vorwaerts in der Datei
    pruefe(ab != std::string::npos && ab < cd && cd < ef && ef < gh, "ShaderPlug: rueckwaerts eingefuegt - in der Datei Shader, dann Texturen, je sortiert");
    pruefe(tags.hatShader("textures/a/b") && tags.hatShader("textures/g/h") && !tags.hatShader("models/x/y"), "ShaderPlug: alle im Tag-Baum");
    // --- SunPlug ---
    kp::Kartenkoordinaten c = kp::optimaleKoordinaten(Kasten(Vektor3f(100, 50, 0), Vektor3f(300, 100, 10)));
    pruefe(c.minX == -200 && c.minY == -250 && c.maxX == 400 && c.maxY == 350, "SunPlug: Quadrat mit der groesseren halben Seite (300)");
    c = kp::optimaleKoordinaten(Kasten(Vektor3f(10.7F, -10.7F, 0), Vektor3f(20, 20, 20)));
    pruefe(c.minX == -164 && c.minY == -185 && c.maxX == 185 && c.maxY == 164, "SunPlug: wenigstens 175, int abgeschnitten (gegen 0)");
    Karte k;
    laden(k, "{\n\"classname\" \"worldspawn\"\n\"mapcoordsmins\" \"-1024 2048\"\n\"mapcoordsmaxs\" \"1024 -2048\"\n" + kasten(0, 0, 0, 64, 64, 64, "test/a", 0, 0) +
                 "}\n{\n\"classname\" \"info_null\"\n\"origin\" \"200 0 0\"\n}\n");
    const auto v = kp::kartenkoordinatenLesen(*k.welt());
    pruefe(v[0] == -1024 && v[1] == 2048 && v[2] == 1024 && v[3] == -2048, "SunPlug: mapcoordsmins / mapcoordsmaxs gelesen");
    const Kasten g = kartenGrenzen(k);
    pruefe(std::fabs((g.origin[0] - g.extents[0]) - 0.0F) < 0.01F && std::fabs((g.origin[0] + g.extents[0]) - 208.0F) < 0.01F,
           "GetMapBounds: Pinsel und Punktentitaet (+-8)");
    // --- UFO:AI: Worldspawn, Perform check ---
    laden(k, "{\n\"classname\" \"worldspawn\"\n" + kasten(0, 0, 0, 64, 64, 64, "test/a", 0, 0) +
                 "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 0 0\"\n\"team\" \"1\"\n}\n"
                 "{\n\"classname\" \"info_player_start\"\n\"origin\" \"32 0 0\"\n\"team\" \"3\"\n}\n"
                 "{\n\"classname\" \"light\"\n\"origin\" \"0 0 128\"\n}\n");
    std::vector<std::string> zeilen;
    std::string m = kp::ufoaiWeltWerte(k, false, zeilen);
    pruefe(m == "Set maxlevel to: 5Set maxteams to: 3You should at least place 16 info_player_start" && k.welt()->wert("maxlevel") == "5" &&
               k.welt()->wert("maxteams") == "3",
           "Worldspawn: maxlevel 5, maxteams 3 (hoechstes team), die Nachrichten aneinander");
    pruefe(!zeilen.empty() && zeilen.back() == "UFO:AI: classname: info_player_start: #2", "Worldspawn: get_team_count in der Konsole");
    m = kp::ufoaiWeltWerte(k, false, zeilen);
    pruefe(m.empty(), "Worldspawn ohne reset: beides schon da - keine Nachricht");
    m = kp::ufoaiWeltWerte(k, true, zeilen);
    pruefe(m.rfind("Set maxlevel to: 5", 0) == 0, "Worldspawn reset: wieder gesetzt");
    k.welt()->setzen("maxlevel", "9");
    m = kp::ufoaiPruefen(k, zeilen);
    pruefe(m.find("No singleplayer start positions (info_human_start)\n") != std::string::npos &&
               m.find("No civilian start positions (info_civilian_start)\n") != std::string::npos &&
               m.find("Worldspawn: Highest maxlevel is 8\n") != std::string::npos && m.find("Found 1 lights with no color value\n") != std::string::npos &&
               m.find("No multiplayer") == std::string::npos && k.welt()->wert("maxlevel") == "8",
           "Perform check: fehlende Startpunkte, maxlevel auf 8, Licht ohne _color");
    // --- UFO:AI: Filter ---
    laden(k, "{\n\"classname\" \"worldspawn\"\n" + kasten(0, 0, 0, 16, 16, 16, "test/a", 0x300, 0) + kasten(32, 0, 0, 48, 16, 16, "test/a", 0x400, 0) +
                 kasten(64, 0, 0, 80, 16, 16, "test/a", 0, 0) + kasten(96, 0, 0, 112, 16, 16, "test/a", 0x40000000, 0) +
                 kasten(128, 0, 0, 144, 16, 16, "test/a", 0, 0x80) + "}\n" +
                 "{\n\"classname\" \"func_door\"\n\"spawnflags\" \"2\"\n" + kasten(0, 64, 0, 16, 80, 16, "test/a", 0, 0) + "}\n" +
                 "{\n\"classname\" \"func_door\"\n\"spawnflags\" \"1\"\n" + kasten(32, 64, 0, 48, 80, 16, "test/a", 0, 0) + "}\n");
    const auto& p = k.welt()->kinder();
    kp::UfoaiZustand z;
    zeilen.clear();
    kp::ufoaiLevel(k, z, kp::kContentsLevel1, zeilen);
    std::vector<std::shared_ptr<Entitaet>> tueren;
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() == "func_door") { tueren.push_back(e); }
    }
    pruefe(z.level == 1 && !p[0]->versteckt && p[1]->versteckt && !p[2]->versteckt && tueren[0]->versteckt && !tueren[1]->versteckt,
           "Level 1: Pinsel mit Inhalt ohne Level 1 versteckt (ohne Flags nicht), func_door ohne spawnflags & 1 versteckt");
    pruefe(!zeilen.empty() && zeilen[0] == "UFO:AI: level_active: 1, flag: 256.", "Level 1: Konsole level_active");
    kp::ufoaiLevel(k, z, kp::kContentsLevel1, zeilen);
    pruefe(z.level == 0 && !p[1]->versteckt && !tueren[0]->versteckt, "Level 1 noch einmal: Filter aus, alles wieder sichtbar");
    kp::ufoaiLevel(k, z, kp::kContentsLevel1 << 2, zeilen);
    pruefe(z.level == 4 && p[0]->versteckt && !p[1]->versteckt, "Level 3 (flag 0x400): der Pinsel ohne Level 3 versteckt");
    kp::ufoaiLevel(k, z, kp::kContentsLevel1, zeilen);
    pruefe(z.level == 1 && !p[0]->versteckt && p[1]->versteckt, "Wechsel auf Level 1: der vorige Filter aufgehoben");
    zeilen.clear();
    kp::ufoaiFilter(k, z, 0, zeilen);
    pruefe(z.stepon && p[3]->versteckt && zeilen.size() == 1 && zeilen[0] == "UFO:AI: Hiding 1 stepon brushes.", "StepOn: der StepOn-Pinsel versteckt");
    kp::ufoaiFilter(k, z, 0, zeilen);
    pruefe(!z.stepon && !p[3]->versteckt, "StepOn noch einmal: wieder sichtbar");
    kp::ufoaiFilter(k, z, 3, zeilen);
    pruefe(z.nodraw && p[4]->versteckt && !p[2]->versteckt, "NoDraw: Flaechen-Flag 0x80 versteckt");
    pruefe(kp::ufoaiAbout("behaved rc").find("Plugin version: 0.4\nAuthor: Martin Gerhardy (tlh2000/mattn)\n") != std::string::npos, "UFO:AI About");
    std::printf("kartekleineplugins: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
