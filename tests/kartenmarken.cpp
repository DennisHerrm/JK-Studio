// kartenmarken.cpp - Liste reste1 Punkt 1: Marken (Decals der Effekte) im Karten-Editor.
//
// Die Welt fuer R_MarkFragments aus den Brushes und Patches der Karte
// (markenWeltAusKarte) gegen die aus der mit bhed_q3map2 gebauten .bsp
// (spurWeltAus, wie die Vorschau sie laedt): dasselbe Quadrat wie CG_ImpactMark
// ergibt auf beiden dieselbe Flaeche und denselben Kasten - auf dem Boden, an der
// Wand, auf den Stufen, auf der Rampe und auf dem Patch. Keine Marke auf der Tuer
// (func_door ist ein eigenes Modell), keine auf clip/monsterclip (nodraw). Die
// Bruchstuecke selbst duerfen anders zerlegt sein (q3map2 dreieckt die Seiten
// anders als der Faecher des Editors).
// Abweichung (bewusst): der Editor schneidet die ganzen Seiten, q3map2 nur den
// sichtbaren Teil (visibleHull) - unter der Stufe 16 liegt darum ein Stueck Boden
// im Inneren der Stufe mehr (nicht zu sehen).
//
//   kartenmarken <tests/karte/q3map2>
#include "bhed/bspgeo.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/saberspuren.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) { ++g_fehler; }
    std::printf("%s  %s\n", ok ? "OK  " : "FEHL", was.c_str());
}

std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// Die surfaceparms aus einer Shaderdatei (Name -> Liste)
std::map<std::string, std::vector<std::string>> shaderParms(const std::string& text) {
    std::map<std::string, std::vector<std::string>> m;
    std::istringstream s(text);
    std::string tok;
    std::string name;
    int tiefe = 0;
    while (s >> tok) {
        if (tok == "{") {
            ++tiefe;
        } else if (tok == "}") {
            --tiefe;
        } else if (tiefe == 0) {
            name = tok;
            m[name];
        } else if (tiefe == 1 && tok == "surfaceparm") {
            std::string p;
            s >> p;
            m[name].push_back(p);
        }
    }
    return m;
}

struct Ergebnis {
    int bruchstuecke = 0;
    double flaeche = 0;
    float mins[3] = {1e30F, 1e30F, 1e30F};
    float maxs[3] = {-1e30F, -1e30F, -1e30F};
};

// Das Quadrat von CG_ImpactMark (Radius r, Drehung 0) um o, Richtung d (Laenge 1),
// projiziert um -20 entlang d (cg_marks.cpp:133-205 OJ)
Ergebnis marke(const SpurWelt& w, const float o[3], const float d[3], float r) {
    float a1[3];
    float a2[3];
    // irgendeine Senkrechte zu d (PerpendicularVector waehlt eine andere - das Ergebnis haengt nicht daran)
    const float hilf[3] = {std::fabs(d[2]) < 0.9F ? 0.0F : 1.0F, 0.0F, std::fabs(d[2]) < 0.9F ? 1.0F : 0.0F};
    a1[0] = d[1] * hilf[2] - d[2] * hilf[1];
    a1[1] = d[2] * hilf[0] - d[0] * hilf[2];
    a1[2] = d[0] * hilf[1] - d[1] * hilf[0];
    const float l = std::sqrt(a1[0] * a1[0] + a1[1] * a1[1] + a1[2] * a1[2]);
    for (float& v : a1) { v /= l; }
    // wie im Spiel axis[1] = axis[0] x axis[2], also axis[2] = axis[1] x axis[0] (sonst zeigen die
    // Seitenebenen von R_MarkFragments nach aussen)
    a2[0] = a1[1] * d[2] - a1[2] * d[1];
    a2[1] = a1[2] * d[0] - a1[0] * d[2];
    a2[2] = a1[0] * d[1] - a1[1] * d[0];
    float pts[4][3];
    for (int k = 0; k < 3; ++k) {
        pts[0][k] = o[k] - r * a1[k] - r * a2[k];
        pts[1][k] = o[k] + r * a1[k] - r * a2[k];
        pts[2][k] = o[k] + r * a1[k] + r * a2[k];
        pts[3][k] = o[k] - r * a1[k] + r * a2[k];
    }
    const float proj[3] = {d[0] * -20.0F, d[1] * -20.0F, d[2] * -20.0F};
    static float puffer[384][3];
    SpurBruchstueck br[128];
    Ergebnis e;
    e.bruchstuecke = spurBruchstuecke(w, 4, pts, proj, 384, puffer, 128, br);
    for (int i = 0; i < e.bruchstuecke; ++i) {
        const float* p0 = puffer[br[i].ersterPunkt];
        double kx = 0;
        double ky = 0;
        double kz = 0;
        for (int j = 0; j < br[i].zahl; ++j) {
            const float* p = puffer[br[i].ersterPunkt + j];
            for (int k = 0; k < 3; ++k) {
                e.mins[k] = std::min(e.mins[k], p[k]);
                e.maxs[k] = std::max(e.maxs[k], p[k]);
            }
            if (j >= 1 && j + 1 < br[i].zahl) {
                const float* q = puffer[br[i].ersterPunkt + j + 1];
                const double u[3] = {static_cast<double>(p[0]) - p0[0], static_cast<double>(p[1]) - p0[1], static_cast<double>(p[2]) - p0[2]};
                const double v[3] = {static_cast<double>(q[0]) - p0[0], static_cast<double>(q[1]) - p0[1], static_cast<double>(q[2]) - p0[2]};
                kx += u[1] * v[2] - u[2] * v[1];
                ky += u[2] * v[0] - u[0] * v[2];
                kz += u[0] * v[1] - u[1] * v[0];
            }
        }
        e.flaeche += 0.5 * std::sqrt(kx * kx + ky * ky + kz * kz);
    }
    return e;
}

std::string text(const Ergebnis& e) {
    char b[160];
    std::snprintf(b, sizeof(b), "%d Bruchstuecke, Flaeche %.2f, Kasten (%.2f %.2f %.2f)-(%.2f %.2f %.2f)", e.bruchstuecke, e.flaeche,
                  static_cast<double>(e.mins[0]), static_cast<double>(e.mins[1]), static_cast<double>(e.mins[2]),
                  static_cast<double>(e.maxs[0]), static_cast<double>(e.maxs[1]), static_cast<double>(e.maxs[2]));
    return b;
}

bool gleich(const Ergebnis& a, const Ergebnis& b) {
    if ((a.bruchstuecke == 0) != (b.bruchstuecke == 0)) { return false; }
    if (a.bruchstuecke == 0) { return true; }
    if (std::fabs(a.flaeche - b.flaeche) > 0.01 * std::max(1.0, b.flaeche)) { return false; }
    for (int k = 0; k < 3; ++k) {
        if (std::fabs(a.mins[k] - b.mins[k]) > 0.05F || std::fabs(a.maxs[k] - b.maxs[k]) > 0.05F) { return false; }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: kartenmarken <tests/karte/q3map2>\n");
        return 2;
    }
    const std::string ordner = argv[1];
    Karte karte;
    (void)karteLesen(lies(ordner + "/spielraum.map"), karte);
    karte.anmelden();
    const auto parms = shaderParms(lies(ordner + "/spielraum.shader"));
    const SpurWelt ed = markenWeltAusKarte(
        karte,
        [&parms](const std::string& sh) {
            const auto it = parms.find(sh.rfind("textures/", 0) == 0 ? sh : "textures/" + sh);
            return it != parms.end() ? it->second : std::vector<std::string>{};
        },
        true);
    BspGeometry geo;
    pruefe(readBspGeometry(lies(ordner + "/spielraum.bsp"), geo), "spielraum.bsp geladen");
    const SpurWelt bsp = spurWeltAus(geo);
    std::printf("Editor: %zu Flaechen, %zu Knoten, %zu Blaetter; .bsp: %zu Flaechen\n", ed.flaechen.size(), ed.knoten.size(),
                ed.blaetter.size(), bsp.flaechen.size());
    pruefe(!ed.leer() && !bsp.leer(), "beide Welten haben Flaechen und einen Baum");

    struct Fall {
        const char* was;
        float o[3];
        float d[3];
        float r;
        bool treffer;
    };
    const float rampe = 1.0F / std::sqrt(74.0F * 74.0F + 128.0F * 128.0F);
    const std::vector<Fall> faelle = {
        {"Boden (300 300)", {300, 300, 0}, {0, 0, 1}, 16, true},
        {"Boden (-400 0), gross", {-400, 0, 0}, {0, 0, 1}, 48, true},
        {"Boden (0 -200)", {0, -200, 0}, {0, 0, 1}, 8, true},
        {"Boden (-600 600), schraeg getroffen", {-600, 600, 0}, {0.3F, 0, 0.9539392F}, 16, true},
        {"Wand x=512 (y 200, z 128)", {512, 200, 128}, {-1, 0, 0}, 16, true},
        {"Wand x=512 (y -700, z 40)", {512, -700, 40}, {-1, 0, 0}, 24, true},
        {"Stufe 16 oben", {160, 0, 16}, {0, 0, 1}, 12, true},
        {"Stufe 24 oben", {-160, 0, 24}, {0, 0, 1}, 12, true},
        {"Rampe (x -64)", {-64, 320, 37}, {-74 * rampe, 0, 128 * rampe}, 16, true},
        {"Patch (Scheitel)", {-224, -256, 32}, {0, 0, 1}, 12, true},
        {"Patch (Flanke, gross)", {-224, -300, 20}, {0, -0.6F, 0.8F}, 20, true},
        {"nonsolid (trans, gezeichnet) oben", {0, 608, 128}, {0, 0, 1}, 16, true},
        {"Tuer (func_door) oben: eigenes Modell, keine Marke", {288, 608, 128}, {0, 0, 1}, 16, false},
        {"clip oben: nodraw, keine Marke", {0, -416, 128}, {0, 0, 1}, 16, false},
        {"monsterclip oben: nodraw, keine Marke", {0, -608, 128}, {0, 0, 1}, 16, false},
    };
    for (const Fall& f : faelle) {
        const Ergebnis a = marke(ed, f.o, f.d, f.r);
        const Ergebnis b = marke(bsp, f.o, f.d, f.r);
        std::printf("  %s\n    Editor: %s\n    .bsp:   %s\n", f.was, text(a).c_str(), text(b).c_str());
        if (f.o[0] == 160.0F) {
            // Die Stufe (16 hoch) steht auf dem Boden: der Boden unter ihr liegt im Bereich der Marke
            // (20 hinter der Flaeche). q3map2 schreibt ihn nicht (visibleHull), der Editor hat die ganze
            // Seite - das Stueck 24 x 24 bei z 0 liegt im Inneren der Stufe und ist nicht zu sehen.
            Ergebnis ohne = a;
            ohne.flaeche -= 24.0 * 24.0;
            ohne.mins[2] = b.mins[2];
            ohne.bruchstuecke = b.bruchstuecke;
            pruefe(gleich(ohne, b) && a.mins[2] == 0.0F, std::string("Marke ") + f.was + ": Editor wie .bsp, dazu der Boden im Inneren der Stufe");
            continue;
        }
        pruefe(gleich(a, b) && (a.bruchstuecke > 0) == f.treffer, std::string("Marke ") + f.was + ": Editor wie .bsp");
    }
    // Leere Karte: keine Welt, keine Marke (und kein Absturz)
    {
        Karte leer;
        const SpurWelt w = markenWeltAusKarte(leer, nullptr, true);
        const float o[3] = {0, 0, 0};
        const float d[3] = {0, 0, 1};
        pruefe(w.leer() && marke(w, o, d, 16).bruchstuecke == 0, "leere Karte: leere Welt, keine Marke");
    }
    std::printf("%d Pruefungen, %d Fehler\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
