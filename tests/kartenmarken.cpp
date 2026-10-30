// kartenmarken.cpp - Liste reste1 Punkt 1 / reste2 Punkt 4: Marken (Decals der Effekte)
// im Karten-Editor.
//
// Die Welt fuer R_MarkFragments aus der Karte (markenWeltAusKarte: der Port von
// ProcessWorldModel, src/karte/sichtbareseiten.inc) gegen die aus der mit bhed_q3map2
// gebauten .bsp (spurWeltAus, wie die Vorschau sie laedt): dasselbe Quadrat wie
// CG_ImpactMark ergibt auf beiden dieselbe Flaeche und denselben Kasten. spielraum: auf
// dem Boden, an der Wand, auf den Stufen (unter der Stufe 16 kein Stueck Boden: das Blatt
// im Inneren der Stufe ist deckend und verweist auf keine Flaeche), auf der Rampe und auf
// dem Patch; keine Marke auf der Tuer (func_door ist ein eigenes Modell), keine auf
// clip/monsterclip (nodraw). markenraum (dicht, FillOutside): der Boden endet an der Wand
// (sein Teil draussen ist nicht in der .bsp), unter einem Detail-Pinsel bekommt der Boden
// ein Stueck (Detail macht kein Blatt deckend), das misc_model traegt Marken; dazu je Ebene
// dieselbe Flaeche der Seiten wie in der .bsp. Die Bruchstuecke selbst duerfen anders zerlegt
// sein (q3map2 dreieckt die Seiten anders als der Faecher des Editors).
//
//   kartenmarken <tests/karte/q3map2>
#include "bhed/bspgeo.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/karte/texturen.h"
#include "bhed/saberspuren.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
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
    double summe[3] = {0, 0, 0};   // aller Punkte: welche Flaechen genau getroffen wurden (Abschluss rc607)
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
                e.summe[k] += p[k];
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

// Die Modelle der Testkarte: jedes misc_model ist marken.ase
class TestModelle final : public ModellQuelle {
public:
    explicit TestModelle(const std::string& ase) {
        auto m = std::make_shared<Modell>();
        m_gut = aseLesen(ase, "models/test/marken.ase", *m);
        m_modell = m;
    }
    [[nodiscard]] bool gut() const { return m_gut && !m_modell->flaechen.empty(); }
    [[nodiscard]] ModellAnsicht ansicht(const Entitaet&) const override {
        ModellAnsicht a;
        a.modell = m_modell;
        for (const ModellFlaeche& f : m_modell->flaechen) { a.shader.push_back(f.shader); }
        return a;
    }

private:
    std::shared_ptr<const Modell> m_modell;
    bool m_gut = false;
};

// Flaeche der ebenen Flaechen je Ebene (Normale und Abstand auf 1/100 gerundet)
std::map<std::string, double> flaecheJeEbene(const SpurWelt& w) {
    std::map<std::string, double> m;
    for (const SpurFlaeche& f : w.flaechen) {
        if (f.art != SpurFlaeche::Art::Flaeche) { continue; }
        double a = 0;
        for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
            const auto& p0 = f.punkte[static_cast<std::size_t>(f.indizes[i])];
            const auto& p1 = f.punkte[static_cast<std::size_t>(f.indizes[i + 1])];
            const auto& p2 = f.punkte[static_cast<std::size_t>(f.indizes[i + 2])];
            const double u[3] = {static_cast<double>(p1[0]) - p0[0], static_cast<double>(p1[1]) - p0[1], static_cast<double>(p1[2]) - p0[2]};
            const double v[3] = {static_cast<double>(p2[0]) - p0[0], static_cast<double>(p2[1]) - p0[1], static_cast<double>(p2[2]) - p0[2]};
            const double c[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
            a += 0.5 * std::sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
        }
        char k[96];
        std::snprintf(k, sizeof(k), "(%.2f %.2f %.2f) %.2f", static_cast<double>(f.ebene.normale[0]) + 0.0, static_cast<double>(f.ebene.normale[1]) + 0.0,
                      static_cast<double>(f.ebene.normale[2]) + 0.0, static_cast<double>(f.ebene.abstand) + 0.0);
        m[k] += a;
    }
    return m;
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
    const Q3ShaderInfoFn info = [&parms](const std::string& sh) {
        const auto it = parms.find(sh.rfind("textures/", 0) == 0 ? sh : "textures/" + sh);
        Q3ShaderInfo i;
        if (it != parms.end()) { i.surfaceparms = it->second; }
        return i;
    };
    const SpurWelt ed = markenWeltAusKarte(karte, info, true);
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
        pruefe(gleich(a, b) && (a.bruchstuecke > 0) == f.treffer, std::string("Marke ") + f.was + ": Editor wie .bsp");
    }
    // reste2 Punkt 4: der dichte Raum
    {
        Karte raum;
        (void)karteLesen(lies(ordner + "/markenraum.map"), raum);
        raum.anmelden();
        const TestModelle modelle(lies(ordner + "/marken.ase"));
        pruefe(modelle.gut(), "marken.ase gelesen");
        raum.kontext.modelle = &modelle;
        MarkenWeltBericht bericht;
        const SpurWelt er = markenWeltAusKarte(raum, info, true, &bericht);
        BspGeometry rgeo;
        pruefe(readBspGeometry(lies(ordner + "/markenraum.bsp"), rgeo), "markenraum.bsp geladen");
        const SpurWelt rb = spurWeltAus(rgeo);
        std::printf("markenraum: %d Pinsel, %d Seiten, %d verdeckt, %d Modellflaechen, Leck %d; Editor %zu Flaechen, .bsp %zu\n", bericht.pinsel,
                    bericht.seiten, bericht.verdeckt, bericht.modelle, bericht.leck ? 1 : 0, er.flaechen.size(), rb.flaechen.size());
        pruefe(!bericht.leck && !bericht.leer && bericht.modelle == 1, "markenraum: dicht (FillOutside), das misc_model in der Welt");
        const std::vector<Fall> raumFaelle = {
            {"Boden an der Ostwand (der Teil unter der Wand und draussen ist nicht in der .bsp)", {500, 0, 0}, {0, 0, 1}, 24, true},
            {"Boden mitten im Raum", {-100, -300, 0}, {0, 0, 1}, 32, true},
            {"Stufe 16 oben (kein Boden darunter)", {160, 0, 16}, {0, 0, 1}, 12, true},
            {"Detail-Pinsel 8 oben (der Boden darunter bekommt ein Stueck)", {-224, 0, 8}, {0, 0, 1}, 12, true},
            {"misc_model oben", {0, 256, 32}, {0, 0, 1}, 8, true},
            {"misc_model und Boden daneben", {0, 284, 0}, {0, 0, 1}, 20, true},
            {"Decke von unten", {0, 0, 256}, {0, 0, -1}, 32, true},
        };
        for (const Fall& f : raumFaelle) {
            const Ergebnis a = marke(er, f.o, f.d, f.r);
            const Ergebnis b = marke(rb, f.o, f.d, f.r);
            std::printf("  %s\n    Editor: %s\n    .bsp:   %s\n", f.was, text(a).c_str(), text(b).c_str());
            pruefe(gleich(a, b) && (a.bruchstuecke > 0) == f.treffer, std::string("Marke ") + f.was + ": Editor wie .bsp");
        }
        // je Ebene dieselbe Flaeche der ebenen Flaechen (visibleHull, CullSides, nur Verwiesenes)
        const auto fe = flaecheJeEbene(er);
        const auto fb = flaecheJeEbene(rb);
        bool alle = fe.size() == fb.size();
        for (const auto& [k, v] : fb) {
            const auto it = fe.find(k);
            const double e = it != fe.end() ? it->second : 0.0;
            const bool g = std::fabs(e - v) <= 0.005 * std::max(1.0, v);
            if (!g) { std::printf("    Ebene %s: Editor %.2f, .bsp %.2f\n", k.c_str(), e, v); }
            alle = alle && g;
        }
        std::printf("  %zu Ebenen mit Flaechen (Editor %zu)\n", fb.size(), fe.size());
        pruefe(alle, "markenraum: je Ebene dieselbe Flaeche wie in der .bsp");
    }
    // Abschluss rc607: die Verdickung von "deformVertexes move" und mehr als 64 Flaechen im Kasten
    // (tools/karte/gen_markenreihe.py, ohne -meta gebaut). Die Shader liest der Editor selbst (Inventar).
    {
        const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "jkstudio_kartenmarken";
        std::error_code ec;
        std::filesystem::remove_all(tmp, ec);
        std::filesystem::create_directories(tmp / "base" / "shaders", ec);
        {
            std::ofstream o(tmp / "base" / "shaders" / "markenreihe.shader", std::ios::binary);
            o << lies(ordner + "/markenreihe.shader");
        }
        Dateisystem fs;
        fs.ordnerHinzufuegen((tmp / "base").string(), "sp");
        Texturinventar inv;
        inv.laden(fs);
        const ShaderEintrag* bewegt = inv.shaderFinden("textures/test/bewegt");
        const ShaderEintrag* flach = inv.shaderFinden("textures/test/flach");
        pruefe(bewegt != nullptr && bewegt->bewegt && bewegt->bewegtMin == std::array<float, 3>{0, 0, 0} &&
                   bewegt->bewegtMax == std::array<float, 3>{4, 4, 32},
               "deformVertexes move 4 4 32 sin 0 1: mins 0 0 0, maxs 4 4 32 (shaderInfo minmax)");
        pruefe(flach != nullptr && !flach->bewegt, "deformVertexes move 0 0 32: MinMax::valid() nicht erfuellt (0 < 0) - keine Verdickung");
        const Q3ShaderInfoFn infoReihe = [&inv](const std::string& sh) {
            return q3ShaderInfoAus(inv.shaderFinden(sh.rfind("textures/", 0) == 0 ? sh : "textures/" + sh));
        };
        Karte reihe;
        (void)karteLesen(lies(ordner + "/markenreihe.map"), reihe);
        reihe.anmelden();
        const SpurWelt er = markenWeltAusKarte(reihe, infoReihe, true);
        BspGeometry rgeo;
        pruefe(readBspGeometry(lies(ordner + "/markenreihe.bsp"), rgeo), "markenreihe.bsp geladen");
        const SpurWelt rb = spurWeltAus(rgeo);
        std::printf("markenreihe: Editor %zu Flaechen, .bsp %zu\n", er.flaechen.size(), rb.flaechen.size());
        const std::vector<Fall> reiheFaelle = {
            {"Stufe oben auf dem bewegten Boden (der verdickte Boden steht auch ueber der Stufe)", {-160, 0, 16}, {0, 0, 1}, 12, true},
            {"bewegter Boden frei", {-400, 100, 0}, {0, 0, 1}, 16, true},
            {"Saeulen, grosse Marke (100 Deckflaechen im Kasten, die ersten 64 zaehlen)", {172, 0, 32}, {0, 0, 1}, 96, true},
            {"Saeulen, Ecke", {100, -76, 32}, {0, 0, 1}, 40, true},
            {"Boden zwischen den Saeulen", {108, -68, 0}, {0, 0, 1}, 6, true},
        };
        for (const Fall& f : reiheFaelle) {
            const Ergebnis a = marke(er, f.o, f.d, f.r);
            const Ergebnis b = marke(rb, f.o, f.d, f.r);
            std::printf("  %s\n    Editor: %s\n    .bsp:   %s\n", f.was, text(a).c_str(), text(b).c_str());
            // ohne -meta sind die Flaechen dieselben: auch gleich viele Bruchstuecke (die ersten 64 Flaechen im Kasten)
            const bool punkte = std::fabs(a.summe[0] - b.summe[0]) < 0.5 && std::fabs(a.summe[1] - b.summe[1]) < 0.5 &&
                                std::fabs(a.summe[2] - b.summe[2]) < 0.5;
            std::printf("    Punktsumme Editor (%.1f %.1f %.1f), .bsp (%.1f %.1f %.1f)\n", a.summe[0], a.summe[1], a.summe[2], b.summe[0], b.summe[1],
                        b.summe[2]);
            pruefe(gleich(a, b) && a.bruchstuecke == b.bruchstuecke && punkte && (a.bruchstuecke > 0) == f.treffer,
                   std::string("Marke ") + f.was + ": Editor wie .bsp, dieselben Bruchstuecke");
        }
        std::filesystem::remove_all(tmp, ec);
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
