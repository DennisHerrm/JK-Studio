// kartenpcwege.cpp - Stufe 9 Punkt 5: NPC-Wege (ctest karte_npcwege)
//
// Eine Karte aus Text: ein Boden, eine Wand mit einer Luecke an einer Seite, drei Wegpunkte
// (w1 -> w2 in der Luecke -> w3 -> w4). Das Netz aus den Entities der Karte hat vier Knoten und die
// Kanten w1-w2, w2-w3, w3-w4 (bei hoechstens drei baut das Spiel keins); ein Testlauf von w1 nach w3 kommt an und geht durch die Luecke (der
// vorhandene Port: NAV::GoTo und Pmove gegen die Kollision aus den Brushes). Ohne Wegpunkte
// laeuft die Figur gegen die Wand und kommt nicht an.
#include <cmath>
#include <cstdio>
#include <string>

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/npcwege.h"
#include "bhed/karte/spielkollision.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
std::string quader(int x0, int y0, int z0, int x1, int y1, int z1) {
    char z[1024];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, y0, z0, x0, y1, z0, x0, y0, z1, x1, y0, z0, x1, y0, z1, x1, y1, z0, x0, y0, z0, x0, y0, z1, x1, y0, z0, x0, y1, z0, x1, y1,
                  z0, x0, y1, z1, x0, y0, z0, x1, y0, z0, x0, y1, z0, x0, y0, z1, x0, y1, z1, x1, y0, z1);
    return z;
}
std::string wp(const char* name, const char* ziel, int x, int y) {
    char z[256];
    std::snprintf(z, sizeof(z), "{\n\"classname\" \"waypoint\"\n\"targetname\" \"%s\"\n%s\"origin\" \"%d %d 32\"\n}\n", name,
                  ziel[0] != '\0' ? (std::string("\"target\" \"") + ziel + "\"\n").c_str() : "", x, y);
    return z;
}
}  // namespace

int main() {
    using namespace bhed;
    using namespace bhed::karte;
    const std::string welt = "{\n\"classname\" \"worldspawn\"\n" + quader(-128, -256, -16, 1152, 256, 0) + quader(480, -256, 0, 544, 112, 256) + "}\n";
    const auto bau = [](const std::string& text, std::shared_ptr<NavWelt>& w) {
        auto k = std::make_shared<Karte>();
        (void)karteLesen(text, *k);
        auto koll = std::make_shared<KollisionsKarte>();
        (void)kollisionAusKarte(*k, [](const std::string&) { return std::vector<std::string>{}; }, true, *koll);
        w = npcWelt(*k, koll);
        return k;
    };
    std::shared_ptr<NavWelt> mit;
    const auto km = bau(welt + wp("w1", "w2", 64, 0) + wp("w2", "w3", 512, 184) + wp("w3", "w4", 960, 0) + wp("w4", "", 960, 160), mit);
    const auto ents = mapEntitaeten(*km);
    expect("Entities wie im Lump: worldspawn zuerst, 5 Stueck", ents.size() == 5 && ents[0].classname == "worldspawn");
    const NavNetz& n = *mit->netz;
    const std::size_t knoten = n.knoten.size() > 0 ? n.knoten.size() - 1 : 0;
    const int kanten = n.zahlKanten();
    std::printf("      %zu Knoten, %d Kanten\n", knoten, kanten);
    expect("Netz (mehr als 3 Knoten, sonst baut das Spiel keins): vier Knoten, mindestens w1-w2, w2-w3, w3-w4", knoten == 4 && kanten >= 3);
    const LaufPlan p = npcTestlauf(*mit, NavVec{64, 0, 32}, NavVec{960, 0, 32}, true);
    float maxY = -1e9F;
    for (const LaufZustand& z : p.zustaende) { maxY = std::max(maxY, z.ort[1]); }
    const NavVec ende = p.zustaende.empty() ? NavVec{} : p.zustaende.back().ort;
    std::printf("      Ankunft %.0f ms, Ende %.0f %.0f %.0f, max y %.0f\n", p.ankunftMs, ende[0], ende[1], ende[2], maxY);
    expect("Testlauf mit Wegpunkten kommt an", p.ankunftMs > 0 && !p.nie && std::hypot(ende[0] - 960, ende[1]) < 64);
    expect("... und geht durch die Luecke (y > 112)", maxY > 112);
    std::shared_ptr<NavWelt> ohne;
    (void)bau(welt, ohne);
    const LaufPlan q = npcTestlauf(*ohne, NavVec{64, 0, 32}, NavVec{960, 0, 32}, true, 20000.0);
    const NavVec e2 = q.zustaende.empty() ? NavVec{} : q.zustaende.back().ort;
    std::printf("      ohne Wegpunkte: Ankunft %.0f ms, Ende %.0f %.0f\n", q.ankunftMs, e2[0], e2[1]);
    expect("ohne Wegpunkte bleibt die Figur vor der Wand", q.ankunftMs < 0 && e2[0] < 480);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
