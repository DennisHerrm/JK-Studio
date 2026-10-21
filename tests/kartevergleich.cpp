// kartevergleich.cpp - Karten-Editor Stufe 1a, Punkt 3: Geometrie und
// Texturkoordinaten gegen das echte q3map2.
//
// tests/karte/vergleich_q3.map und vergleich_bp.map (tools/gen_karte_vergleich.py)
// wurden mit q3map2 aus NetRadiant-Custom 20260114 ohne -meta kompiliert
// (q3map2.exe -game ja -fs_basepath "<JA>/" <karte>) - die .bsp liegen daneben.
// Ohne -meta macht q3map2 aus jeder sichtbaren Pinselseite eine ebene
// Zeichenflaeche (DrawSurfaceForSide, surface.cpp:616-749) und legt die
// Kontrollpunkte eines Patches unveraendert ab.
//
// Verglichen wird je Flaeche (ausser dem Raum aus imperial/basic):
//   xyz: q3map2 rastet jeden Punkt auf 1/8 ein (g_brushSnap ist an,
//        surface.cpp:695-697: SNAP_INT_TO_FLOAT * floor(xyz * 8 + 0.5)).
//        Unser Punkt (double) mit derselben Rechnung muss GENAU den q3map2-
//        Punkt ergeben; der rohe Abstand ist hoechstens 1/16 plus die float-
//        Genauigkeit der q3map2-Wicklung (bei |x| <= 528: 2^-14 ~ 6.1e-5).
//   st:  q3map2 rechnet st am eingerasteten Punkt in float (texMat bzw.
//        vecs float, x und y float) und zieht je Flaeche floor(0.5*(min+max))
//        ab (CalcSurfaceTextureBias, surface.cpp:228-245). Wir rechnen st am
//        selben Punkt mit NRCs Projektion (float-Matrix, double-Punkt) und
//        ziehen denselben Bias ab. Fehlerschranke: drei float-Rundungen bei
//        |st| < 32 (3 * 2^-19 * 32 / 32 ~ 5.7e-6) plus die float-Rundung von
//        x, y (2^-14 * |Matrix| <= 6.1e-5 * 0.04 = 2.4e-6) -> 1e-5.
//   Patch: Kontrollpunkte xyz genau gleich, st bis auf den ganzzahligen Bias.
//
// Aufruf: kartevergleich <tests/karte>
#include "bhed/bspgeo.h"
#include "bhed/karte/kartenformat.h"

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
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Groessen der Editorbilder in JA (assets1.pk3, gemessen 21.10.2026):
// caution 256x64, edge 512x128, door_1new_onoff 256x512, grid 256x256,
// basic 512x512, floor 512x512.
class JaGroessen final : public TexturGroessen {
public:
    std::pair<std::size_t, std::size_t> groesse(const std::string& s) const override {
        static const std::map<std::string, std::pair<std::size_t, std::size_t>> t = {
            {"textures/imperial/caution", {256, 64}},       {"textures/imperial/edge", {512, 128}},
            {"textures/imperial/door_1new_onoff", {256, 512}}, {"textures/imperial/grid", {256, 256}},
            {"textures/imperial/basic", {512, 512}},        {"textures/imperial/floor", {512, 512}},
        };
        auto it = t.find(klein(s));
        return it == t.end() ? std::make_pair<std::size_t, std::size_t>(64, 64) : it->second;
    }
};

double einrasten(double v) { return (1.0 / 8.0) * std::floor(v * 8.0 + 0.5); }   // SNAP_* (surface.cpp:697)

struct Ergebnis {
    std::size_t flaechen = 0;
    std::size_t patches = 0;
    double xyzRoh = 0;       // groesster Abstand unser Punkt <-> q3map2-Punkt
    double xyzEingerastet = 0;
    double st = 0;
    double stWicklung = 0;   // st an unserem (nicht eingerasteten) Punkt, nur zur Info
};

Ergebnis vergleichen(const std::filesystem::path& mapPfad, const std::filesystem::path& bspPfad) {
    Ergebnis e;
    JaGroessen groessen;
    Karte karte;
    karte.kontext.texturen = &groessen;
    const LeseErgebnis le = karteLesen(lies(mapPfad), karte);
    pruefe(le.vollstaendig, mapPfad.filename().string() + " gelesen");
    BspGeometry geo;
    pruefe(readBspGeometry(lies(bspPfad), geo), bspPfad.filename().string() + " gelesen");
    const Kontext& k = karte.kontext;

    std::vector<const Flaeche*> flaechen;
    std::vector<const Patch*> patches;
    for (const auto& en : karte.entitaeten()) {
        for (const auto& p : en->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) {
                const auto& b = static_cast<const Pinsel&>(*p);
                b.bRep(k);
                for (const auto& f : b.flaechen()) {
                    if (f->traegtBei() && klein(f->shader()) != "textures/imperial/basic") { flaechen.push_back(f.get()); }
                }
            } else {
                patches.push_back(static_cast<const Patch*>(p.get()));
            }
        }
    }

    std::size_t gefunden = 0;
    for (const auto& s : geo.surfaces) {
        const std::string shader = klein(geo.shaders[static_cast<std::size_t>(s.shader)].name);
        if (shader == "textures/imperial/basic") { continue; }
        const BspVertex* v = geo.verts.data() + s.firstVert;
        if (s.type == BspSurface::Type::Patch) {
            const Patch* treffer = nullptr;
            for (const Patch* p : patches) {
                if (klein(p->shader()) == shader && static_cast<int>(p->breite()) == s.patchWidth
                    && static_cast<int>(p->hoehe()) == s.patchHeight) {
                    treffer = p;
                }
            }
            pruefe(treffer != nullptr, "Patch " + shader + " gefunden");
            if (!treffer) { continue; }
            ++e.patches;
            bool gleich = true;
            for (int i = 0; i < s.numVerts; ++i) {
                const PatchPunkt& pp = treffer->punkte()[static_cast<std::size_t>(i)];
                for (int c = 0; c < 3; ++c) { gleich = gleich && pp.vertex[static_cast<std::size_t>(c)] == v[i].xyz[c]; }
            }
            pruefe(gleich, "Patch: Kontrollpunkte xyz genau gleich");
            // st: gleicher Bias wie CalcSurfaceTextureBias
            float mn[2] = {999999, 999999}, mx[2] = {-999999, -999999};
            for (const auto& pp : treffer->punkte()) {
                for (int c = 0; c < 2; ++c) {
                    mn[c] = std::min(mn[c], pp.texcoord[static_cast<std::size_t>(c)]);
                    mx[c] = std::max(mx[c], pp.texcoord[static_cast<std::size_t>(c)]);
                }
            }
            for (int i = 0; i < s.numVerts; ++i) {
                const PatchPunkt& pp = treffer->punkte()[static_cast<std::size_t>(i)];
                for (int c = 0; c < 2; ++c) {
                    const float bias = std::floor(0.5f * (mn[c] + mx[c]));
                    e.st = std::max(e.st, std::fabs(static_cast<double>(pp.texcoord[static_cast<std::size_t>(c)] - bias) - v[i].st[c]));
                }
            }
            continue;
        }
        if (s.type != BspSurface::Type::Planar) { continue; }
        // passende Flaeche: gleicher Shader, gleiche Normale, alle Punkte eingerastet gleich
        const Flaeche* treffer = nullptr;
        for (const Flaeche* f : flaechen) {
            if (klein(f->shader()) != shader || f->wicklung().numpoints != static_cast<std::size_t>(s.numVerts)) { continue; }
            const Vektor3d n = f->ebene().normal();
            const double d = n.x() * v[0].normal[0] + n.y() * v[0].normal[1] + n.z() * v[0].normal[2];
            if (d < 0.9999) { continue; }
            bool alle = true;
            for (int i = 0; i < s.numVerts && alle; ++i) {
                bool da = false;
                for (const auto& w : f->wicklung()) {
                    da = da || (std::fabs(w.vertex.x() - v[i].xyz[0]) < 0.07 && std::fabs(w.vertex.y() - v[i].xyz[1]) < 0.07
                                && std::fabs(w.vertex.z() - v[i].xyz[2]) < 0.07);
                }
                alle = da;
            }
            if (alle) { treffer = f; }
        }
        pruefe(treffer != nullptr, "Flaeche " + shader + " mit " + std::to_string(s.numVerts) + " Punkten gefunden");
        if (!treffer) { continue; }
        ++gefunden;
        ++e.flaechen;
        // xyz
        for (int i = 0; i < s.numVerts; ++i) {
            double bestRoh = 1e9, bestRast = 1e9;
            for (const auto& w : treffer->wicklung()) {
                double roh = 0, rast = 0;
                for (int c = 0; c < 3; ++c) {
                    roh = std::max(roh, std::fabs(w.vertex[static_cast<std::size_t>(c)] - v[i].xyz[c]));
                    rast = std::max(rast, std::fabs(einrasten(w.vertex[static_cast<std::size_t>(c)]) - v[i].xyz[c]));
                }
                if (roh < bestRoh) {
                    bestRoh = roh;
                    bestRast = rast;
                }
            }
            e.xyzRoh = std::max(e.xyzRoh, bestRoh);
            e.xyzEingerastet = std::max(e.xyzEingerastet, bestRast);
        }
        // st am q3map2-Punkt mit unserer Projektion
        Matrix4 m;
        const auto [bw, bh] = k.groesse(treffer->shader());
        lokalZuTextur(k.art(), treffer->projektion(), bw, bh, treffer->ebene().normal(), m);
        std::vector<Vektor2f> st(static_cast<std::size_t>(s.numVerts));
        float mn[2] = {999999, 999999}, mx[2] = {-999999, -999999};
        for (int i = 0; i < s.numVerts; ++i) {
            const Vektor3d p(v[i].xyz[0], v[i].xyz[1], v[i].xyz[2]);
            st[static_cast<std::size_t>(i)] = transformierterPunkt(m, p).vec2();
            for (int c = 0; c < 2; ++c) {
                mn[c] = std::min(mn[c], st[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)]);
                mx[c] = std::max(mx[c], st[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)]);
            }
        }
        for (int i = 0; i < s.numVerts; ++i) {
            for (int c = 0; c < 2; ++c) {
                const float bias = std::floor(0.5f * (mn[c] + mx[c]));
                e.st = std::max(e.st, std::fabs(static_cast<double>(st[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)] - bias) - v[i].st[c]));
            }
        }
        // zur Info: st an unserem Wicklungspunkt (Texdef_EmitTextureCoordinates)
        for (int i = 0; i < s.numVerts; ++i) {
            for (const auto& w : treffer->wicklung()) {
                if (std::fabs(einrasten(w.vertex.x()) - v[i].xyz[0]) < 1e-6 && std::fabs(einrasten(w.vertex.y()) - v[i].xyz[1]) < 1e-6
                    && std::fabs(einrasten(w.vertex.z()) - v[i].xyz[2]) < 1e-6) {
                    for (int c = 0; c < 2; ++c) {
                        const float bias = std::floor(0.5f * (mn[c] + mx[c]));
                        e.stWicklung = std::max(e.stWicklung,
                                                std::fabs(static_cast<double>(w.texcoord[static_cast<std::size_t>(c)] - bias) - v[i].st[c]));
                    }
                }
            }
        }
    }
    pruefe(gefunden == flaechen.size(), mapPfad.filename().string() + ": jede unserer Flaechen hat ihre q3map2-Flaeche (" +
                                            std::to_string(gefunden) + " von " + std::to_string(flaechen.size()) + ")");
    return e;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: kartevergleich <tests/karte>\n");
        return 2;
    }
    const std::filesystem::path d = argv[1];
    for (const char* name : {"vergleich_q3", "vergleich_bp"}) {
        const Ergebnis e = vergleichen(d / (std::string(name) + ".map"), d / (std::string(name) + ".bsp"));
        std::printf("%s: %zu Flaechen, %zu Patch; xyz roh max %.6g, eingerastet max %.6g; st max %.3g (am eigenen "
                    "Wicklungspunkt %.3g)\n",
                    name, e.flaechen, e.patches, e.xyzRoh, e.xyzEingerastet, e.st, e.stWicklung);
        pruefe(e.flaechen >= 10 && e.patches == 1, std::string(name) + ": Flaechen und Patch verglichen");
        pruefe(e.xyzEingerastet == 0, std::string(name) + ": xyz nach q3map2-Einrasten (1/8) genau gleich");
        pruefe(e.xyzRoh <= 1.0 / 16 + 6.1e-5, std::string(name) + ": xyz roh hoechstens 1/16 + float-Genauigkeit");
        pruefe(e.st <= 1e-5, std::string(name) + ": st bis 1e-5 (float-Rechnung von q3map2)");
    }
    std::printf("kartevergleich: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
