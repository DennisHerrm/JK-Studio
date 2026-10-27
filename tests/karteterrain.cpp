// karteterrain.cpp - Karten-Editor Stufe 8b Punkt 1: der Terrain Generator
// gegen NRCs eigenen Code.
//
// tests/karte/terrain_nrc enthaelt contrib/terrain_generator aus
// NetRadiant-Custom master (origin/master 8216133) unveraendert: noise.cpp,
// terrain_engine.cpp, brush_builder.cpp; der Erbauer schreibt ueber einen
// Test-Ersatz der Radiant-Schnittstellen (shim/nrc_shim.h) mit. Geprueft wird
// fuer viele Werte (Formen, Rauschen, Terrassen, Achsen, unregelmaessiges
// Raster, Hoehenstufen, Schritte, krumme Grenzen, Startwerte):
//   - Hoehen- und Tunnelkarten: dieselben Schluessel, x/y/z Bit fuer Bit gleich,
//     danach derselbe Stand der Zufallsfolge (gleich viele rand()-Aufrufe);
//   - die Pinsel: dieselben Gruppen, Pinsel, Flaechen in derselben Reihenfolge,
//     die drei Punkte Bit fuer Bit, Shader, texdef, contents; dieselbe Wahl;
//   - erzeugen() an einer Karte: Auswahl-Pinsel weg, func_group mit
//     _terraingen_size, detail, ein Rueckgaengig stellt die Karte Byte fuer Byte
//     her, Fehlerkaesten wie NRC.
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/terraingen.h"

#include "brush_builder.h"
#include "nrc_shim.h"
#include "noise.h"
#include "terrain_engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace tg = bhed::karte::terraingen;
using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        if (g_fehler < 40) { std::printf("FEHL  %s\n", was.c_str()); }
    }
}

bool bitgleich(double a, double b) { return std::memcmp(&a, &b, sizeof(double)) == 0; }
bool bitgleichF(float a, float b) { return std::memcmp(&a, &b, sizeof(float)) == 0; }

::BrushData nrcZiel(const tg::BrushData& b) {
    ::BrushData n;
    n.min_x = b.min_x;
    n.max_x = b.max_x;
    n.min_y = b.min_y;
    n.max_y = b.max_y;
    n.min_z = b.min_z;
    n.max_z = b.max_z;
    n.width_x = b.width_x;
    n.length_y = b.length_y;
    n.height_z = b.height_z;
    return n;
}

const tg::Zufall kRand = [] { return std::rand(); };

// Der Bau aus der Aufzeichnung des Test-Ersatzes
tg::Bau nrcBau() {
    tg::Bau b;
    const NrcAufzeichnung& a = nrcAufzeichnung();
    std::vector<const scene::Node*> gruppen(a.wurzel.kinder.begin(), a.wurzel.kinder.end());
    for (const scene::Node* g : gruppen) {
        std::vector<std::vector<tg::FaceData>> gp;
        for (const scene::Node* p : g->kinder) {
            std::vector<tg::FaceData> fl;
            for (std::size_t i = 0; i < p->faces.size(); ++i) {
                const _QERFaceData& q = p->faces[i];
                tg::FaceData f;
                for (int j = 0; j < 3; ++j) {
                    f.p0[j] = q.m_p0[j];
                    f.p1[j] = q.m_p1[j];
                    f.p2[j] = q.m_p2[j];
                }
                f.shift[0] = q.m_texdef.shift[0];
                f.shift[1] = q.m_texdef.shift[1];
                f.rotate = q.m_texdef.rotate;
                f.scale[0] = q.m_texdef.scale[0];
                f.scale[1] = q.m_texdef.scale[1];
                f.shader = p->shader[i];
                f.contents = q.contents;
                f.flags = q.flags;
                f.value = q.value;
                fl.push_back(f);
            }
            gp.push_back(fl);
        }
        b.gruppen.push_back(gp);
    }
    for (const scene::Node* s : a.gewaehlt) {
        for (std::size_t i = 0; i < gruppen.size(); ++i) {
            if (gruppen[i] == s) { b.gewaehlt.push_back(static_cast<int>(i)); }
        }
    }
    std::string f = a.fehler.str();
    std::size_t p = 0;
    while (p < f.size()) {
        const std::size_t e = f.find('\n', p);
        b.fehler.push_back(f.substr(p, e == std::string::npos ? std::string::npos : e - p + 1));
        p = e == std::string::npos ? f.size() : e + 1;
    }
    return b;
}

bool bauGleich(const tg::Bau& a, const tg::Bau& b, std::string& wo) {
    if (a.gruppen.size() != b.gruppen.size()) {
        wo = "Gruppen " + std::to_string(a.gruppen.size()) + " / " + std::to_string(b.gruppen.size());
        return false;
    }
    for (std::size_t g = 0; g < a.gruppen.size(); ++g) {
        if (a.gruppen[g].size() != b.gruppen[g].size()) {
            wo = "Gruppe " + std::to_string(g) + " Pinsel " + std::to_string(a.gruppen[g].size()) + " / " + std::to_string(b.gruppen[g].size());
            return false;
        }
        for (std::size_t p = 0; p < a.gruppen[g].size(); ++p) {
            const auto& fa = a.gruppen[g][p];
            const auto& fb = b.gruppen[g][p];
            if (fa.size() != fb.size()) {
                wo = "Pinsel " + std::to_string(p) + " Flaechen";
                return false;
            }
            for (std::size_t i = 0; i < fa.size(); ++i) {
                const tg::FaceData& x = fa[i];
                const tg::FaceData& y = fb[i];
                bool ok = x.shader == y.shader && x.contents == y.contents && x.flags == y.flags && x.value == y.value && bitgleichF(x.rotate, y.rotate);
                for (int j = 0; j < 3; ++j) {
                    ok = ok && bitgleich(x.p0[j], y.p0[j]) && bitgleich(x.p1[j], y.p1[j]) && bitgleich(x.p2[j], y.p2[j]);
                }
                for (int j = 0; j < 2; ++j) { ok = ok && bitgleichF(x.scale[j], y.scale[j]) && bitgleichF(x.shift[j], y.shift[j]); }
                if (!ok) {
                    char z[256];
                    std::snprintf(z, sizeof(z), "Gruppe %zu Pinsel %zu Flaeche %zu: (%.17g %.17g %.17g) / (%.17g %.17g %.17g)", g, p, i, x.p0[0], x.p0[1],
                                  x.p0[2], y.p0[0], y.p0[1], y.p0[2]);
                    wo = z;
                    return false;
                }
            }
        }
    }
    if (a.gewaehlt != b.gewaehlt) {
        wo = "Wahl";
        return false;
    }
    if (a.fehler != b.fehler) {
        wo = "Fehlermeldungen";
        return false;
    }
    return true;
}

struct Fall {
    double x0, x1, y0, y1, z0, z1;
    double sx, sy;
};

int g_karten = 0, g_pinselFlaechen = 0;

void standard(unsigned seed, const Fall& f, int form, int rauschen, double varianz, double frequenz, double terrasse, int achse, bool jitter,
              double rasterschritt, const std::string& name) {
    tg::BrushData ziel;
    ziel.min_x = f.x0;
    ziel.max_x = f.x1;
    ziel.min_y = f.y0;
    ziel.max_y = f.y1;
    ziel.min_z = f.z0;
    ziel.max_z = f.z1;
    ziel.width_x = f.x1 - f.x0;
    ziel.length_y = f.y1 - f.y0;
    ziel.height_z = f.z1 - f.z0;
    tg::BrushData unser = ziel;
    tg::adjust_bounds_to_fit_grid(unser, f.sx, f.sy);
    ::BrushData nrc = nrcZiel(ziel);
    ::adjust_bounds_to_fit_grid(nrc, f.sx, f.sy);
    pruefe(bitgleich(unser.min_x, nrc.min_x) && bitgleich(unser.max_x, nrc.max_x) && bitgleich(unser.min_y, nrc.min_y) &&
               bitgleich(unser.max_y, nrc.max_y) && bitgleich(unser.width_x, nrc.width_x) && bitgleich(unser.length_y, nrc.length_y),
           name + ": adjust_bounds_to_fit_grid");

    std::srand(seed);
    const ::TerrainMap mn = ::generate_height_map(nrc, f.sx, f.sy, static_cast<::ShapeType>(form), 300.0, varianz, frequenz,
                                                  static_cast<::NoiseType>(rauschen), terrasse, static_cast<::Axis>(achse), jitter, rasterschritt);
    const int nachNrc = std::rand();
    std::srand(seed);
    const tg::TerrainMap mu = tg::generate_height_map(unser, f.sx, f.sy, static_cast<tg::ShapeType>(form), 300.0, varianz, frequenz,
                                                      static_cast<tg::NoiseType>(rauschen), terrasse, static_cast<tg::Axis>(achse), jitter, rasterschritt,
                                                      kRand);
    const int nachUns = std::rand();
    bool gleich = mn.size() == mu.size();
    if (gleich) {
        auto a = mn.begin();
        auto b = mu.begin();
        for (; a != mn.end(); ++a, ++b) {
            if (!bitgleich(a->first.first, b->first.first) || !bitgleich(a->first.second, b->first.second) || !bitgleich(a->second.x, b->second.x) ||
                !bitgleich(a->second.y, b->second.y) || !bitgleich(a->second.z, b->second.z)) {
                gleich = false;
                break;
            }
        }
    }
    pruefe(gleich, name + ": Hoehenkarte Bit fuer Bit (" + std::to_string(mn.size()) + " Punkte)");
    pruefe(nachNrc == nachUns, name + ": danach derselbe Stand der Zufallsfolge");
    ++g_karten;

    // Pinsel
    const bool split = varianz > 0 || form != 0 || jitter;
    nrcAufzeichnung().leeren();
    ::build_terrain_brushes(nrc, f.sx, f.sy, mn, "textures/test/boden", split);
    const tg::Bau bn = nrcBau();
    tg::Bau bu;
    tg::build_terrain_brushes(unser, f.sx, f.sy, mu, "textures/test/boden", split, bu);
    std::string wo;
    pruefe(bauGleich(bn, bu, wo), name + ": Pinsel wie NRCs Erbauer " + wo);
    for (const auto& g : bu.gruppen) {
        for (const auto& p : g) { g_pinselFlaechen += static_cast<int>(p.size()); }
    }
}

void tunnel(unsigned seed, const Fall& f, bool schraeg, int rauschen, double varianz, double frequenz, double terrasse, int achse, double rasterschritt,
            double hoehe, double tunnelHoehe, const std::string& name) {
    tg::BrushData ziel;
    ziel.min_x = f.x0;
    ziel.max_x = f.x1;
    ziel.min_y = f.y0;
    ziel.max_y = f.y1;
    ziel.min_z = f.z0;
    ziel.max_z = f.z1;
    ziel.width_x = f.x1 - f.x0;
    ziel.length_y = f.y1 - f.y0;
    ziel.height_z = f.z1 - f.z0;
    tg::adjust_bounds_to_fit_grid(ziel, f.sx, f.sy);
    const ::BrushData nrc = nrcZiel(ziel);
    const double cave = schraeg ? tunnelHoehe : hoehe;
    const double slope = schraeg ? hoehe : 0;
    const double terr = schraeg ? terrasse : 0.0;
    std::srand(seed);
    const ::TunnelMaps mn = ::generate_tunnel_height_maps(nrc, f.sx, f.sy, cave, slope, varianz, frequenz, static_cast<::NoiseType>(rauschen), terr,
                                                          static_cast<::Axis>(achse), rasterschritt);
    const int nachNrc = std::rand();
    std::srand(seed);
    const tg::TunnelMaps mu = tg::generate_tunnel_height_maps(ziel, f.sx, f.sy, cave, slope, varianz, frequenz, static_cast<tg::NoiseType>(rauschen),
                                                              terr, static_cast<tg::Axis>(achse), rasterschritt, kRand);
    const int nachUns = std::rand();
    const auto mapGleich = [](const auto& a, const auto& b) {
        if (a.size() != b.size()) { return false; }
        auto x = a.begin();
        auto y = b.begin();
        for (; x != a.end(); ++x, ++y) {
            if (!bitgleich(x->first.first, y->first.first) || !bitgleich(x->first.second, y->first.second) || !bitgleich(x->second, y->second)) {
                return false;
            }
        }
        return true;
    };
    pruefe(mapGleich(mn.floor_map, mu.floor_map) && mapGleich(mn.ceiling_map, mu.ceiling_map) && mapGleich(mn.left_wall_map, mu.left_wall_map) &&
               mapGleich(mn.right_wall_map, mu.right_wall_map) && bitgleich(mn.step_z, mu.step_z),
           name + ": Tunnelkarten Bit fuer Bit");
    pruefe(nachNrc == nachUns, name + ": danach derselbe Stand der Zufallsfolge");
    ++g_karten;
    nrcAufzeichnung().leeren();
    ::build_tunnel_brushes(nrc, f.sx, f.sy, mn, "textures/test/fels", cave, slope, static_cast<::Axis>(achse));
    const tg::Bau bn = nrcBau();
    tg::Bau bu;
    tg::build_tunnel_brushes(ziel, f.sx, f.sy, mu, "textures/test/fels", cave, slope, static_cast<tg::Axis>(achse), bu);
    std::string wo;
    pruefe(bauGleich(bn, bu, wo), name + ": Tunnel-Pinsel wie NRCs Erbauer " + wo);
    for (const auto& g : bu.gruppen) {
        for (const auto& p : g) { g_pinselFlaechen += static_cast<int>(p.size()); }
    }
}

void rauschenGleich() {
    for (double x = -300.0; x < 300.0; x += 13.37) {
        for (double y = -200.0; y < 200.0; y += 7.77) {
            pruefe(bitgleich(Perlin::noise(x * 0.11, y * 0.07), tg::perlinNoise(x * 0.11, y * 0.07)), "Perlin");
            pruefe(bitgleich(Simplex::noise(x * 0.11, y * 0.07), tg::simplexNoise(x * 0.11, y * 0.07)), "Simplex");
        }
    }
}

// --- erzeugen() an einer Karte -------------------------------------------------------
std::string welt(const std::string& pinsel) { return "{\n\"classname\" \"worldspawn\"\n" + pinsel + "}\n"; }
std::string quader(int x0, int y0, int z0, int x1, int y1, int z1) {
    char z[1024];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) test/wand 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, y0, z0, x0, y1, z0, x0, y0, z1,   // -x
                  x1, y0, z0, x1, y0, z1, x1, y1, z0,   // +x
                  x0, y0, z0, x0, y0, z1, x1, y0, z0,   // -y
                  x0, y1, z0, x1, y1, z0, x0, y1, z1,   // +y
                  x0, y0, z0, x1, y0, z0, x0, y1, z0,   // -z
                  x0, y0, z1, x0, y1, z1, x1, y0, z1);  // +z
    return z;
}

void karteTest() {
    Karte k;
    (void)karteLesen(welt(quader(0, 0, 0, 512, 256, 64)), k);
    k.anmelden();
    const std::string vorher = karteSchreiben(k);
    tg::Werte w;
    w.textur = "textures/test/boden";
    // ohne Auswahl: Kasten "No Selection"
    tg::Ergebnis e = tg::erzeugen(k, w, kRand);
    pruefe(!e.ok && e.fehler.titel == "Terrain Generator - No Selection", "ohne Auswahl: Kasten No Selection");
    const auto weltE = k.welt();
    k.auswahl.waehlen(weltE, weltE->kinder().front());
    w.textur = "   ";
    e = tg::erzeugen(k, w, kRand);
    pruefe(!e.ok && e.fehler.titel == "Terrain Generator - No Texture", "leere Textur: Kasten No Texture");
    w.textur = "textures/test/boden";
    w.form = static_cast<int>(tg::ShapeType::Hill);
    w.formHoehe = 128;
    std::srand(7);
    e = tg::erzeugen(k, w, kRand);
    pruefe(e.ok && e.gruppen == 1 && e.pinsel == 2 * 8 * 4, "Hill auf 512 x 256 (Schritt 64): 1 func_group, 64 Pinsel (" + std::to_string(e.pinsel) + ")");
    std::shared_ptr<Entitaet> gruppe;
    for (const auto& ent : k.entitaeten()) {
        if (ent->klassenname() == "func_group") { gruppe = ent; }
    }
    pruefe(gruppe && gruppe->wert("_terraingen_size") == "512 256 64", "func_group traegt _terraingen_size \"512 256 64\"");
    pruefe(k.welt()->kinder().empty(), "der Auswahl-Pinsel ist geloescht");
    pruefe(k.auswahl.anzahl() == 64, "die erzeugten Pinsel sind gewaehlt");
    bool detail = gruppe != nullptr;
    if (gruppe) {
        for (const auto& p : gruppe->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) { detail = detail && static_cast<const Pinsel&>(*p).istDetail(); }
        }
    }
    pruefe(detail, "alle erzeugten Pinsel sind detail (contents BRUSH_DETAIL_MASK)");
    pruefe(k.undo.undoName() == "terrainGenerator.generate", "Undo-Schritt terrainGenerator.generate");
    const std::string nachher = karteSchreiben(k);
    (void)k.undo.undo();
    pruefe(karteSchreiben(k) == vorher, "ein Rueckgaengig stellt die Karte Byte fuer Byte her");
    (void)k.undo.redo();
    pruefe(karteSchreiben(k) == nachher, "Wiederholen gibt das Terrain Byte fuer Byte zurueck");
    // Neu erzeugen mit der gespeicherten Groesse (waehlt die Pinsel des Terrains)
    std::srand(7);
    const std::string erst = karteSchreiben(k);
    e = tg::erzeugen(k, w, kRand);
    std::shared_ptr<Entitaet> g2;
    for (const auto& ent : k.entitaeten()) {
        if (ent->klassenname() == "func_group") { g2 = ent; }
    }
    pruefe(e.ok && g2 && g2->wert("_terraingen_size") == "512 256 64" && karteSchreiben(k) == erst,
           "neu erzeugen mit gleichem Startwert: gleiche Groesse aus _terraingen_size, gleiche Karte");
    // Slope aus der Auswahl: shape_height = -(Hoehe - 64)
    double h = 0;
    w.form = static_cast<int>(tg::ShapeType::Slope);
    pruefe(tg::schraegeAusAuswahl(k, w, h) && h == 64.0, "Slope aus der Auswahl: gespeicherte Hoehe 64");
}

}  // namespace

int main() {
    rauschenGleich();
    const Fall faelle[] = {
        {-512, 512, -512, 512, 0, 64, 64, 64},      {-100.5, 300.25, -37, 201, 16, 80, 64, 64}, {0, 1000, 0, 700, -64, 0, 32, 48},
        {64, 192, 64, 128, 0, 200, 16, 16},         {-256, 256, -128, 128, 0, 64, 128, 128},
    };
    int nr = 0;
    for (const unsigned seed : {1U, 42U, 31337U}) {
        for (const Fall& f : faelle) {
            for (int form = 0; form <= 6; ++form) {
                for (int rauschen = 0; rauschen <= 2; ++rauschen) {
                    const bool jitter = ((nr + form) % 3) == 0;
                    const double terr = (form % 2 == 1) ? 48.0 : 0.0;
                    const double var = (nr % 4 == 0) ? 0.0 : (nr % 4 == 1 ? 32.0 : 250.0);
                    const double rs = (nr % 5 == 0) ? 8.0 : 1.0;
                    const int achse = nr % 2;
                    standard(seed, f, form, rauschen, var, nr % 3 == 0 ? 0.05 : 0.005, terr, achse, jitter, rs,
                             "Fall " + std::to_string(nr) + " (Startwert " + std::to_string(seed) + ", Form " + std::to_string(form) + ", Rauschen " +
                                 std::to_string(rauschen) + ")");
                    ++nr;
                }
            }
            for (const bool schraeg : {false, true}) {
                for (int achse = 0; achse <= 1; ++achse) {
                    for (int rauschen = 0; rauschen <= 2; ++rauschen) {
                        tunnel(seed, f, schraeg, rauschen, nr % 2 == 0 ? 24.0 : 0.0, 0.01, schraeg ? 32.0 : 0.0, achse, nr % 3 == 0 ? 4.0 : 1.0,
                               schraeg ? (nr % 2 == 0 ? -136.0 : 200.0) : 192.0, 160.0,
                               "Tunnel " + std::to_string(nr) + (schraeg ? " (schraeg)" : "") + " Achse " + std::to_string(achse));
                        ++nr;
                    }
                }
            }
        }
    }
    karteTest();
    std::printf("karteterrain: %d Karten, %d Flaechen verglichen, %d Pruefungen, %d Fehlschlaege\n", g_karten, g_pinselFlaechen, g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
