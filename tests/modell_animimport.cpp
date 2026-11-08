// modell_animimport.cpp - Animations-Import aus FBX/glTF/BVH/COLLADA (bhed/modell/animimport.h)
//
// Testdaten tests/animimport/: eine eigene Szene aus Blender 4.5 (blender_testgang.py: Skelett mit HumanIK/Mixamo-Namen,
// Gang auf der Stelle mit Fuss-IK, Figur schaut nach -Y), exportiert von Blenders eigenen Exportern als FBX, glb, BVH und
// COLLADA, dazu die Weltorte der Knochenkoepfe je Bild aus Blender (testgang_ref.txt, Meter, +Z oben).
// Geprueft je Format: Knochenzahl, Bilder, fps, Lage aller Knochen in allen Bildern gegen Blender (cm), Blickrichtung,
// Mixamo-Vorlage; BVH dazu gegen die Rohdaten von assimp; BVH schreiben -> lesen gleich; Retargeting auf das JKA-Skelett
// (Hand-Tabelle) mit kleinem Fussrutschen.
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/animimport.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/retarget.h"
#include "bhed/modell/rig.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace bhed;
using namespace bhed::modell;
using V3 = std::array<float, 3>;

#include "rig_handtabelle.inc"

namespace {
int g_fehler = 0;
void expect(const std::string& what, bool ok) {
    std::printf("%s %s\n", ok ? "ok    " : "FEHLER", what.c_str());
    if (!ok) { ++g_fehler; }
}
std::string z(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.4f", v);
    return b;
}
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
// FIX2 P5/P9: FNV-1a 64 ueber alle Bytes der Bilder (roh) und der Grundstellungen
unsigned long long rohHash(const GlaAnimation& g) {
    unsigned long long h = 1469598103934665603ULL;
    const auto dazu = [&h](const void* p, std::size_t n) {
        const auto* b = static_cast<const unsigned char*>(p);
        for (std::size_t i = 0; i < n; ++i) { h = (h ^ b[i]) * 1099511628211ULL; }
    };
    if (!g.roh.empty()) { dazu(g.roh.data(), g.roh.size() * sizeof(BoneMatrix)); }
    for (const GlaBone& b : g.bones) { dazu(&b.basePose, sizeof(BoneMatrix)); }
    return h;
}

struct Ref {
    std::vector<std::string> namen;
    std::vector<std::vector<V3>> bilder;   // cm
};
Ref refLesen(const std::string& pfad) {
    Ref r;
    std::ifstream f(pfad);
    std::string zeile;
    std::getline(f, zeile);   // Kommentar
    std::getline(f, zeile);
    std::istringstream n(zeile);
    for (std::string s; n >> s;) { r.namen.push_back(s); }
    while (std::getline(f, zeile)) {
        std::istringstream s(zeile);
        std::vector<V3> b;
        V3 p{};
        while (s >> p[0] >> p[1] >> p[2]) { b.push_back({p[0] * 100.0F, p[1] * 100.0F, p[2] * 100.0F}); }
        if (b.size() == r.namen.size()) { r.bilder.push_back(b); }
    }
    return r;
}
V3 ort(const BoneMatrix& m) { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }
float abst(const V3& a, const V3& b) { return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2])); }

// groesste Abweichung aller Knochenorte aller Bilder gegen die Referenz (cm)
float gegenRef(const AnimImport& a, const Ref& r, std::string& schlimmster) {
    float m = 0.0F;
    std::vector<BoneMatrix> w;
    for (std::size_t f = 0; f < r.bilder.size() && static_cast<int>(f) < a.anim.numFrames; ++f) {
        a.anim.worldMatrices(static_cast<int>(f), w);
        for (std::size_t k = 0; k < r.namen.size(); ++k) {
            const int b = boneIndex(a.anim, r.namen[k]);
            if (b < 0) {
                schlimmster = r.namen[k] + " fehlt";
                return 1e9F;
            }
            const float d = abst(ort(w[uz(b)]), r.bilder[f][k]);
            if (d > m) {
                m = d;
                schlimmster = r.namen[k] + " Bild " + std::to_string(f);
            }
        }
    }
    return m;
}

// Welten zweier Importe vergleichen: Orte (cm) und Drehung gegenueber der eigenen Ruhe (Grad), Knochen ueber den Namen
std::string g_wo;   // Knochen und Bild der groessten Drehabweichung
void vergleich(const GlaAnimation& a, const GlaAnimation& b, int bilder, float& ortMax, float& gradMax) {
    ortMax = 0.0F;
    gradMax = 0.0F;
    std::vector<BoneMatrix> wa;
    std::vector<BoneMatrix> wb;
    for (int f = 0; f < bilder; ++f) {
        a.worldMatrices(f, wa);
        b.worldMatrices(f, wb);
        for (std::size_t i = 0; i < a.bones.size(); ++i) {
            const int j = boneIndex(b, a.bones[i].name);
            if (j < 0) {
                ortMax = 1e9F;
                return;
            }
            ortMax = std::max(ortMax, abst(ort(wa[i]), ort(wb[uz(j)])));
            // Q = X R0^T (Drehung gegenueber der Ruhe), Winkel zwischen Qa und Qb
            const BoneMatrix qa = wa[i] * a.bones[i].basePoseInv;
            const BoneMatrix qb = wb[uz(j)] * b.bones[uz(j)].basePoseInv;
            // Winkel von M = Qa^T Qb ueber atan2(Sinus aus dem schiefen Teil, Kosinus aus der Spur): acos der Spur allein verliert
            // nahe 0 Grad die Haelfte der Stellen (float-Matrizen: 0,035 Grad Rauschen)
            double m[3][3];
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    m[r][c] = 0.0;
                    for (int k = 0; k < 3; ++k) { m[r][c] += static_cast<double>(qa.m[k][r]) * qb.m[k][c]; }
                }
            }
            const double sn = std::sqrt((m[2][1] - m[1][2]) * (m[2][1] - m[1][2]) + (m[0][2] - m[2][0]) * (m[0][2] - m[2][0]) + (m[1][0] - m[0][1]) * (m[1][0] - m[0][1])) / 2.0;
            const double w = std::atan2(sn, (m[0][0] + m[1][1] + m[2][2] - 1.0) / 2.0) * 57.29577951308232;
            if (w > gradMax) {
                gradMax = static_cast<float>(w);
                g_wo = a.bones[i].name + " Bild " + std::to_string(f);
            }
        }
    }
}

GlaAnimation jkaZiel() {
    const RigHandSkelett* h = &kRigHandSkelette[0];
    GlaAnimation g;
    g.name = h->gla;
    g.skeletonName = "models/players/_humanoid/_humanoid";
    g.scale = 0.64F;
    for (int i = 0; i < h->zahl; ++i) {
        GlaBone b;
        b.name = h->knochen[i].name;
        b.parent = h->knochen[i].eltern;
        b.basePose = BoneMatrix::identity();
        b.basePoseInv = BoneMatrix::identity();
        const float p[3] = {h->knochen[i].x, h->knochen[i].y, h->knochen[i].z};
        for (int k = 0; k < 3; ++k) {
            b.basePose.m[k][3] = p[k];
            b.basePoseInv.m[k][3] = -p[k];
        }
        g.bones.push_back(b);
    }
    for (std::size_t i = 0; i < g.bones.size(); ++i) {
        if (g.bones[i].parent >= 0) { g.bones[uz(g.bones[i].parent)].kinder.push_back(static_cast<int>(i)); }
    }
    return g;
}
void schreibDatei(const std::string& pfad, const std::string& inhalt) {
    std::filesystem::create_directories(std::filesystem::path(pfad).parent_path());
    std::ofstream o(pfad, std::ios::binary);
    o << inhalt;
}

#ifdef _WIN32
// FIX4 P34: GlobalMemoryStatusEx in der Importtabelle dieses Programms umgelenkt - der freie und der gesamte Arbeitsspeicher
// vorgetaeuscht, ohne den Rechner zu fuellen. Eine Jobgrenze (JOB_OBJECT_LIMIT_PROCESS_MEMORY/JOB_MEMORY) aendert die Werte von
// GlobalMemoryStatusEx nicht (gemessen 08.11.: ullAvailPhys 43 GB unter einer 300-MB-Grenze).
std::uint64_t gFreiVorgabe = 0;     // ullAvailPhys, 0: echt
std::uint64_t gGesamtVorgabe = 0;   // ullTotalPhys, 0: echt
using GmsFn = BOOL(WINAPI*)(LPMEMORYSTATUSEX);
GmsFn gEchtGms = nullptr;
BOOL WINAPI vorgetaeuscht(LPMEMORYSTATUSEX m) {
    const BOOL r = gEchtGms(m);
    if (r != 0 && gFreiVorgabe != 0) { m->ullAvailPhys = gFreiVorgabe; }
    if (r != 0 && gGesamtVorgabe != 0) { m->ullTotalPhys = gGesamtVorgabe; }
    return r;
}
bool speicherUmlenken() {
    auto* basis = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(basis);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(basis + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (auto* d = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(basis + dir.VirtualAddress); d->Name != 0; ++d) {
        const auto* namen = reinterpret_cast<const IMAGE_THUNK_DATA*>(basis + d->OriginalFirstThunk);
        auto* adressen = reinterpret_cast<IMAGE_THUNK_DATA*>(basis + d->FirstThunk);
        for (; namen->u1.AddressOfData != 0; ++namen, ++adressen) {
            if (IMAGE_SNAP_BY_ORDINAL(namen->u1.Ordinal)) { continue; }
            const auto* n = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(basis + namen->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char*>(n->Name), "GlobalMemoryStatusEx") != 0) { continue; }
            DWORD alt = 0;
            if (VirtualProtect(&adressen->u1.Function, sizeof(void*), PAGE_READWRITE, &alt) == 0) { return false; }
            gEchtGms = reinterpret_cast<GmsFn>(adressen->u1.Function);
            adressen->u1.Function = reinterpret_cast<ULONG_PTR>(&vorgetaeuscht);
            VirtualProtect(&adressen->u1.Function, sizeof(void*), alt, &alt);
            return true;
        }
    }
    return false;
}
#endif

// FIX2 P5: ein glTF mit einer Kette aus `knochen` Knoten, alle mit derselben Ortsspur (Schluessel bei den Zeiten `zeiten`,
// Sekunden), Puffer als data:-URI (glTF 2.0 5.10 "uri")
std::string base64(const std::vector<std::uint8_t>& b) {
    static const char* const kZ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string s;
    for (std::size_t i = 0; i < b.size(); i += 3) {
        const std::uint32_t v = (static_cast<std::uint32_t>(b[i]) << 16) | (i + 1 < b.size() ? static_cast<std::uint32_t>(b[i + 1]) << 8 : 0U) |
                                (i + 2 < b.size() ? static_cast<std::uint32_t>(b[i + 2]) : 0U);
        s += kZ[(v >> 18) & 63U];
        s += kZ[(v >> 12) & 63U];
        s += i + 1 < b.size() ? kZ[(v >> 6) & 63U] : '=';
        s += i + 2 < b.size() ? kZ[v & 63U] : '=';
    }
    return s;
}
std::string gltfKette(int knochen, const std::vector<float>& zeiten) {
    std::vector<std::uint8_t> puffer;
    auto f32 = [&](float v) {
        std::uint8_t b[4];
        std::memcpy(b, &v, 4);
        puffer.insert(puffer.end(), b, b + 4);
    };
    for (const float t : zeiten) { f32(t); }
    for (std::size_t i = 0; i < zeiten.size(); ++i) {
        f32(static_cast<float>(i));
        f32(0.0F);
        f32(1.0F);
    }
    const std::size_t nz = zeiten.size();
    std::string knoten;
    std::string kanaele;
    for (int i = 0; i < knochen; ++i) {
        knoten += std::string(i > 0 ? "," : "") + "{\"name\":\"k" + std::to_string(i) + "\",\"translation\":[0,0,1]" +
                  (i + 1 < knochen ? ",\"children\":[" + std::to_string(i + 1) + "]" : std::string()) + "}";
        kanaele += std::string(i > 0 ? "," : "") + "{\"sampler\":0,\"target\":{\"node\":" + std::to_string(i) + ",\"path\":\"translation\"}}";
    }
    char mm[64];
    std::snprintf(mm, sizeof mm, "\"min\":[%.9g],\"max\":[%.9g]", static_cast<double>(zeiten.front()), static_cast<double>(zeiten.back()));
    return "{\"asset\":{\"version\":\"2.0\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[" + knoten + "],\"buffers\":[{\"byteLength\":" +
           std::to_string(puffer.size()) + ",\"uri\":\"data:application/octet-stream;base64," + base64(puffer) +
           "\"}],\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" + std::to_string(nz * 4) + "},{\"buffer\":0,\"byteOffset\":" + std::to_string(nz * 4) +
           ",\"byteLength\":" + std::to_string(nz * 12) + "}],\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":" + std::to_string(nz) +
           ",\"type\":\"SCALAR\"," + mm + "},{\"bufferView\":1,\"componentType\":5126,\"count\":" + std::to_string(nz) +
           ",\"type\":\"VEC3\"}],\"animations\":[{\"name\":\"lang\",\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}],\"channels\":[" + kanaele +
           "]}]}";
}

// Punkt 4: eine eigene Figur auf eigenem Skelett fuer Movie Duels SP - das importierte Skelett (Mixamo-Namen), auf die Groesse
// von _humanoid gebracht (k = Beinlaenge _humanoid / Quelle, dieselbe Regel wie das Retargeting), als GLA mit dem Gang (rtBacken,
// compressBone), eine GLM mit eigener Haut (je Knochen ein Kasten zum Eltern, ganz an den Eltern gebunden), animation.cfg,
// .skin, Shader, Textur und NPC-Datei. Das Spiel sucht den Wurzelknochen "model_root" (SP g_client.cpp G_SetG2PlayerModel:
// rootBone = G2API_GetBoneIndex(..., "model_root"); ohne ihn setzt SET_ANIM keine Animation) - die Wurzel heisst so.
bool eigeneFigur(const AnimImport& imp, float k, const std::string& cfgVorlage, const std::string& aus, std::string& bericht) {
    GlaAnimation g = imp.anim;
    if (g.bones.empty() || g.bones[0].parent >= 0) { return false; }
    g.bones[0].name = "model_root";
    auto skaliere = [&](BoneMatrix& m) {
        for (int i = 0; i < 3; ++i) { m.m[i][3] *= k; }
    };
    for (GlaBone& b : g.bones) {
        skaliere(b.basePose);
        // starre Umkehrung: R^T, -R^T t
        BoneMatrix inv = BoneMatrix::identity();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) { inv.m[r][c] = b.basePose.m[c][r]; }
        }
        for (int r = 0; r < 3; ++r) { inv.m[r][3] = -(inv.m[r][0] * b.basePose.m[0][3] + inv.m[r][1] * b.basePose.m[1][3] + inv.m[r][2] * b.basePose.m[2][3]); }
        b.basePoseInv = inv;
    }
    for (BoneMatrix& m : g.roh) { skaliere(m); }
    const std::string ordner = "models/players/aitest";
    std::vector<std::vector<BoneMatrix>> welt(static_cast<std::size_t>(g.numFrames));
    for (int f = 0; f < g.numFrames; ++f) { g.worldMatrices(f, welt[uz(f)]); }
    GlaAnimation gla;
    std::string fehler;
    if (!rtBacken(g, welt, ordner + "/aitest", gla, &fehler)) {
        bericht = "rtBacken: " + fehler;
        return false;
    }
    std::string glaBytes;
    if (!schreibGla(gla, glaBytes, &fehler)) {
        bericht = "schreibGla: " + fehler;
        return false;
    }
    // GLM: ein Kasten je Knochen mit Eltern (Breite 12 % der Laenge, mindestens 0,8), 24 Ecken mit Flaechennormalen
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.name = ordner + "/model.glm";
    in.glaPfad = ordner + "/aitest";
    in.knochenZahl = static_cast<int>(g.bones.size());
    in.stufenZahl = 1;
    Teil t;
    t.name = "body";
    t.shader = ordner + "/body";
    std::vector<Ecke> ecken;
    std::vector<std::uint32_t> ind;
    for (std::size_t b = 0; b < g.bones.size(); ++b) {
        const int p = g.bones[b].parent;
        if (p < 0 || g.bones[uz(p)].parent < 0) { continue; }   // kein Kasten von model_root aus
        const V3 a0 = ort(g.bones[uz(p)].basePose);
        const V3 e0 = ort(g.bones[b].basePose);
        V3 d{e0[0] - a0[0], e0[1] - a0[1], e0[2] - a0[2]};
        const float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (len < 1e-3F) { continue; }
        for (float& x : d) { x /= len; }
        V3 hilf = std::fabs(d[2]) < 0.9F ? V3{0.0F, 0.0F, 1.0F} : V3{1.0F, 0.0F, 0.0F};
        V3 u{d[1] * hilf[2] - d[2] * hilf[1], d[2] * hilf[0] - d[0] * hilf[2], d[0] * hilf[1] - d[1] * hilf[0]};
        const float lu = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        for (float& x : u) { x /= lu; }
        const V3 v{d[1] * u[2] - d[2] * u[1], d[2] * u[0] - d[0] * u[2], d[0] * u[1] - d[1] * u[0]};
        const float w = std::max(0.8F, 0.12F * len);
        auto ecke = [&](float sd, float su, float sv) {
            const V3& o = sd > 0.0F ? e0 : a0;
            return V3{o[0] + u[0] * su * w + v[0] * sv * w, o[1] + u[1] * su * w + v[1] * sv * w, o[2] + u[2] * su * w + v[2] * sv * w};
        };
        // je Seite vier Ecken (Normale nach aussen) - Reihenfolge egal, der Shader zeichnet beide Seiten (cull none)
        const float seiten[6][4][3] = {{{-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}, {-1, -1, 1}}, {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},
                                       {{-1, -1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}}, {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}},
                                       {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}}, {{-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}, {1, -1, 1}}};
        const V3 normalen[6] = {{-d[0], -d[1], -d[2]}, {d[0], d[1], d[2]}, {-u[0], -u[1], -u[2]}, {u[0], u[1], u[2]}, {-v[0], -v[1], -v[2]}, {v[0], v[1], v[2]}};
        for (int f = 0; f < 6; ++f) {
            const auto basis = static_cast<std::uint32_t>(ecken.size());
            for (int c = 0; c < 4; ++c) {
                Ecke e;
                const V3 q = ecke(seiten[f][c][0], seiten[f][c][1], seiten[f][c][2]);
                for (int i = 0; i < 3; ++i) {
                    e.ort[i] = q[uz(i)];
                    e.normale[i] = normalen[f][uz(i)];
                }
                e.uv[0] = c == 1 || c == 2 ? 1.0F : 0.0F;
                e.uv[1] = c >= 2 ? 1.0F : 0.0F;
                e.knochen[0] = static_cast<std::uint8_t>(p);
                e.gewicht[0] = 1.0F;
                ecken.push_back(e);
            }
            for (const std::uint32_t i : {0U, 1U, 2U, 0U, 2U, 3U}) { ind.push_back(basis + i); }
        }
    }
    Netz n;
    n.aufbauen(ecken, ind);
    t.stufen.push_back(std::move(n));
    in.teile.push_back(std::move(t));
    std::string glmBytes;
    GlmSchreibBericht gb;
    if (!glmSchreiben(in, glmBytes, &gb)) {
        bericht = "glmSchreiben: " + gb.fehler;
        return false;
    }
    // animation.cfg: jede Zeile des Spiels zeigt auf den Gang (Bild 0, alle Bilder, Schleife ab 0, fps der Datei) - welche
    // Animation das Spiel der Figur auch gibt, sie spielt den eigenen Gang
    std::string cfg = "// JK Studio: eigenes Skelett (Animations-Import), jede Animation = der Gang\n";
    std::ifstream v(cfgVorlage);
    int zeilen = 0;
    const int fps = imp.sequenzen.empty() ? 30 : imp.sequenzen.front().fps;
    for (std::string z; std::getline(v, z);) {
        std::istringstream zs(z);
        std::string name;
        zs >> name;
        if (name.rfind("BOTH_", 0) != 0 && name.rfind("TORSO_", 0) != 0 && name.rfind("LEGS_", 0) != 0) { continue; }
        cfg += name + "\t0\t" + std::to_string(gla.numFrames) + "\t0\t" + std::to_string(fps) + "\n";
        ++zeilen;
    }
    // Textur 8x8, 32 bit, unkomprimiert (TGA Typ 2), eigene Farbe
    std::string tga(18, '\0');
    tga[2] = 2;
    tga[12] = 8;
    tga[14] = 8;
    tga[16] = 32;
    tga[17] = 8;
    for (int i = 0; i < 64; ++i) { tga += std::string{'\x20', '\x80', '\xe0', '\xff'}; }   // BGRA: orange
    schreibDatei(aus + "/" + ordner + "/model.glm", glmBytes);
    schreibDatei(aus + "/" + ordner + "/aitest.gla", glaBytes);
    schreibDatei(aus + "/" + ordner + "/animation.cfg", cfg);
    schreibDatei(aus + "/" + ordner + "/model_default.skin", "body," + ordner + "/body\n");
    schreibDatei(aus + "/" + ordner + "/body.tga", tga);
    schreibDatei(aus + "/shaders/aitest.shader", ordner + "/body\n{\n\tcull none\n\t{\n\t\tmap " + ordner + "/body.tga\n\t\trgbGen identity\n\t}\n}\n");
    schreibDatei(aus + "/ext_data/npcs/aitest.npc", "aitest\n{\n\tplayerModel\taitest\n\tweapon\tWP_NONE\n\thealth\t100\n\tplayerTeam\tTEAM_NEUTRAL\n"
                                                   "\tenemyTeam\tTEAM_NEUTRAL\n\twalkSpeed\t0\n\trunSpeed\t0\n}\n");
    // gelesen wie die Spiele: GLM und GLA
    GlmModel m;
    GlaAnimation zurueck;
    const bool glmOk = readGlm(glmBytes, m, &fehler);
    const bool glaOk = readGla(glaBytes, zurueck, &fehler);
    // gepackt gegen ungepackt: Orte aller Knochen in allen Bildern
    float abw = 0.0F;
    std::vector<BoneMatrix> wz;
    for (int f = 0; glaOk && f < zurueck.numFrames && f < g.numFrames; ++f) {
        zurueck.worldMatrices(f, wz);
        for (std::size_t b = 0; b < wz.size(); ++b) { abw = std::max(abw, abst(ort(wz[b]), ort(welt[uz(f)][b]))); }
    }
    bericht = "GLM " + std::to_string(glmBytes.size()) + " Bytes (" + std::to_string(m.surfaces.size()) + " Flaeche, " +
              std::to_string(m.surfaces.empty() ? 0 : m.surfaces[0].verts.size()) + " Ecken, " + std::to_string(m.numBones) + " Knochen, gla " + m.animFile +
              "), GLA " + std::to_string(glaBytes.size()) + " Bytes (" + std::to_string(zurueck.bones.size()) + " Knochen, " + std::to_string(zurueck.numFrames) +
              " Bilder, gepackt gegen gerechnet " + z(abw) + "), animation.cfg " + std::to_string(zeilen) + " Zeilen";
    return glmOk && glaOk && m.numBones == static_cast<int>(g.bones.size()) && m.animFile == ordner + "/aitest" && zurueck.numFrames == g.numFrames &&
           zurueck.bones[0].name == "model_root" && abw < 0.1F && zeilen > 100;
}
}  // namespace

int main(int argc, char** argv) {
    const std::string d = argc > 1 ? argv[1] : "tests/animimport";
    const Ref ref = refLesen(d + "/testgang_ref.txt");
    expect("Referenz: " + std::to_string(ref.namen.size()) + " Knochen, " + std::to_string(ref.bilder.size()) + " Bilder",
           ref.namen.size() == 22 && ref.bilder.size() == 31);

    expect("Endungen", istAnimImportEndung("a/B.FBX") && istAnimImportEndung("x.glb") && istAnimImportEndung("x.bvh") && !istAnimImportEndung("x.gla"));
    struct Fall {
        const char* datei;
        const char* format;
        float grenze;   // cm: Abtastung/Rundung des Exporters (FBX/glb float, BVH 6 Stellen, COLLADA Matrizen als Text)
    };
    const Fall faelle[] = {{"testgang.fbx", "FBX", 0.01F}, {"testgang.glb", "glTF", 0.01F}, {"testgang.bvh", "BVH", 0.01F}, {"testgang.dae", "COLLADA", 0.01F}};
    AnimImport glb;
    for (const Fall& fa : faelle) {
        AnimImport a;
        std::string fehler;
        const bool ok = animImportieren(d + "/" + fa.datei, a, &fehler);
        expect(std::string(fa.datei) + " gelesen " + fehler, ok);
        if (!ok) { continue; }
        const std::string pre = std::string(fa.format) + ": ";
        std::printf("roh %s: %d Bilder, FNV %016llx\n", fa.datei, a.anim.numFrames, rohHash(a.anim));   // FIX2 P5/P9: bytegleich vorher/nachher
        expect(pre + "Format " + a.format + ", oben " + a.oben + ", vorn " + a.vorn + ", cm je Einheit " + z(a.zentimeter), a.format == fa.format);
        // 22 Knochen, dazu das Objekt "Armature" als Wurzel, wenn der Exporter es als Knoten schreibt (FBX, COLLADA)
        const std::size_t soll = 22U + (boneIndex(a.anim, "Armature") >= 0 ? 1U : 0U);
        std::string namen;
        for (const GlaBone& b : a.anim.bones) { namen += " " + b.name; }
        expect(pre + std::to_string(a.anim.bones.size()) + " Knochen (" + std::to_string(soll) + ")" + (a.anim.bones.size() == soll ? std::string() : namen),
               a.anim.bones.size() == soll);
        const int fps = a.sequenzen.empty() ? 0 : a.sequenzen.front().fps;
        expect(pre + std::to_string(a.sequenzen.size()) + " Sequenz " + (a.sequenzen.empty() ? std::string() : a.sequenzen.front().name) + ", " +
                   std::to_string(a.anim.numFrames) + " Bilder (31), " + std::to_string(fps) + " fps (30)",
               a.sequenzen.size() == 1 && a.anim.numFrames == 31 && fps == 30);
        std::string wo;
        const float m = gegenRef(a, ref, wo);
        expect(pre + "Lage aller Knochen in allen Bildern gegen Blender: " + z(m) + " cm (" + wo + ", Grenze " + z(fa.grenze) + ")", m < fa.grenze);
        // Blickrichtung: die Zehen liegen vor dem Fuss (JKA vorn = -Y)
        std::vector<BoneMatrix> w;
        a.anim.worldMatrices(0, w);
        const int fu = boneIndex(a.anim, "mixamorig:LeftFoot");
        const int ze = boneIndex(a.anim, "mixamorig:LeftToeBase");
        const int li = boneIndex(a.anim, "mixamorig:LeftUpLeg");
        expect(pre + "vorn -Y, links +X, oben +Z",
               fu >= 0 && ze >= 0 && li >= 0 && w[uz(ze)].m[1][3] < w[uz(fu)].m[1][3] - 5.0F && w[uz(li)].m[0][3] > 5.0F && w[uz(li)].m[2][3] > 50.0F);
        const int v = rtVorlageFuer(a.anim, fa.datei);
        expect(pre + "Vorlage " + (v >= 0 ? rtVorlagen()[uz(v)].name : std::string("keine")), v >= 0 && rtVorlagen()[uz(v)].name.find("Mixamo") != std::string::npos);
        if (std::string(fa.format) == "glTF") { glb = a; }
        if (std::string(fa.format) == "BVH") {
            // gegen die Rohdaten von assimp: Wurzel (6 Kanaele) im Bild 10 = Ortschluessel 10 der Datei (cm, Blender +Z oben)
            Assimp::Importer imp;
            const aiScene* sc = imp.ReadFile(d + "/" + fa.datei, 0);
            const aiNodeAnim* na = sc != nullptr && sc->mNumAnimations > 0 ? sc->mAnimations[0]->mChannels[0] : nullptr;
            a.anim.worldMatrices(10, w);
            const int h = boneIndex(a.anim, "mixamorig:Hips");
            const bool da = na != nullptr && na->mNumPositionKeys > 10 && h >= 0;
            const aiVector3D p = da ? na->mPositionKeys[10].mValue : aiVector3D();
            const float dd = da ? abst(ort(w[uz(h)]), {p.x, p.y, p.z}) : 1e9F;
            expect(pre + "Wurzel Bild 10 gegen assimp-Schluessel " + z(dd) + " cm", dd < 1e-3F);
        }
    }

    // FIX2 P5: Bildzahl aus dem letzten Schluessel begrenzt. Ein verirrter Schluessel (Zeit in ms statt s) gibt eine Meldung statt
    // bad_alloc; eine Zeit ueber int (GlaAnimation::numFrames) gibt keinen Ueberlauf (vorher n = 1 und "gelesen"). Die kurze
    // Kette (3 Schluessel, 2 s) bleibt lesbar: 61 Bilder bei 30 fps.
    {
        struct Lang {
            const char* was;
            float ende;   // s
            bool gut;
        };
        const Lang lang[] = {{"2 s", 2.0F, true}, {"2e7 s (6e8 Bilder x 8 Knochen)", 2.0e7F, false}, {"1e9 s (3e10 Bilder, ueber int)", 1.0e9F, false}};
        for (const Lang& l : lang) {
            const std::string pfad = "animimport_lang.gltf";
            {
                std::ofstream o(pfad, std::ios::binary);
                o << gltfKette(8, {0.0F, 1.0F / 30.0F, l.ende});
            }
            AnimImport a;
            std::string fehler;
            const bool ok = animImportieren(pfad, a, &fehler);
            std::remove(pfad.c_str());
            if (l.gut) {
                expect(std::string("P5 ") + l.was + ": gelesen, " + std::to_string(a.anim.numFrames) + " Bilder (61), " + std::to_string(a.anim.bones.size()) +
                           " Knochen (8) " + fehler,
                       ok && a.anim.numFrames == 61 && a.anim.bones.size() == 8U);
            } else {
                // FIX4 P34: Bilder x 8 Knochen x 3 Byte passen nicht in die int-Offsets einer GLA - abgelehnt vor jedem Speichertest
                expect(std::string("P5 ") + l.was + ": abgelehnt mit Meldung \"" + fehler + "\" (" + std::to_string(a.anim.numFrames) + " Bilder)",
                       !ok && !fehler.empty() && a.fehlerArt.art == FehlerArt::AnimZuLangGla && a.fehlerArt.b == 8);
            }
        }
    }

#ifdef _WIN32
    // FIX4 P34: gleiche Datei, gleiches Ergebnis - unabhaengig davon, wie viel Speicher gerade frei ist (vorher lehnte der Import
    // ab, sobald die Bilder mehr brauchten als ullAvailPhys: mit offenem Browser abgelehnt, nach dem Schliessen gelesen)
    {
        expect("P34 GlobalMemoryStatusEx in der Importtabelle umgelenkt", speicherUmlenken());
        const auto lies = [](float ende, AnimImport& a, std::string& fehler) {
            const std::string pfad = "animimport_p34.gltf";
            {
                std::ofstream o(pfad, std::ios::binary);
                o << gltfKette(8, {0.0F, 1.0F / 30.0F, ende});
            }
            const bool ok = animImportieren(pfad, a, &fehler);
            std::remove(pfad.c_str());
            return ok;
        };
        struct Lage {
            const char* was;
            std::uint64_t frei;
        };
        const Lage lagen[] = {{"freier Speicher echt", 0ULL}, {"16 MB frei", 16ULL << 20U}, {"1 MB frei", 1ULL << 20U}};
        unsigned long long h0 = 0;
        for (const Lage& l : lagen) {
            gFreiVorgabe = l.frei;
            AnimImport a;
            std::string fehler;
            const bool ok = lies(1000.0F, a, fehler);
            const unsigned long long h = ok ? rohHash(a.anim) : 0ULL;
            if (l.frei == 0ULL) { h0 = h; }
            char z[64];
            std::snprintf(z, sizeof z, "FNV %016llx", h);
            expect(std::string("P34 1000 s (30001 Bilder x 8 Knochen), ") + l.was + ": gelesen, " + std::to_string(a.anim.numFrames) + " Bilder, " + z +
                       " wie mit echtem Speicher " + fehler,
                   ok && a.anim.numFrames == 30001 && h == h0 && h != 0ULL);
        }
        // Weniger Arbeitsspeicher im Rechner, als die Bilder brauchen: abgelehnt, wie viel auch frei ist
        gGesamtVorgabe = 16ULL << 20U;
        for (const std::uint64_t frei : {0ULL, 8ULL << 20U}) {
            gFreiVorgabe = frei;
            AnimImport a;
            std::string fehler;
            const bool ok = lies(1000.0F, a, fehler);
            expect(std::string("P34 1000 s auf einem Rechner mit 16 MB Arbeitsspeicher (frei ") + (frei == 0ULL ? "echt" : "8 MB") + "): abgelehnt \"" +
                       fehler + "\", c = " + std::to_string(a.fehlerArt.c) + " MB",
                   !ok && a.fehlerArt.art == FehlerArt::AnimZuLang && a.fehlerArt.c == 16);
        }
        // GLA: Bilder x Knochen x 3 Byte muessen in die int-Offsets passen (mdx_format.h) - bei 8 Knochen hoechstens 89478485 Bilder.
        // Knapp darunter lehnt nicht das Format ab, sondern (vorgetaeuscht 16 MB) der Arbeitsspeicher - ohne die 70 GB anzulegen.
        struct Grenze {
            float ende;   // s; glTF-Zeiten float, assimp x 1000 in float (2982616000 -> 2982616064 ms)
            bool gla;
        };
        for (const Grenze& gr : {Grenze{2982616.0F, false}, Grenze{2982616.5F, true}}) {
            gGesamtVorgabe = 16ULL << 20U;
            AnimImport a;
            std::string fehler;
            const bool ok = lies(gr.ende, a, fehler);
            char z[200];
            std::snprintf(z, sizeof z, "P34 %.1f s: %s, %lld Bilder x 8 x 3 = %lld Byte ", static_cast<double>(gr.ende),
                          gr.gla ? "ueber der GLA-Grenze" : "knapp unter der GLA-Grenze", a.fehlerArt.a, a.fehlerArt.a * 24LL);
            expect(std::string(z) + "\"" + fehler + "\"",
                   !ok && (gr.gla ? (a.fehlerArt.art == FehlerArt::AnimZuLangGla && a.fehlerArt.c == 89478485 && a.fehlerArt.a * 24LL > 2147483647LL)
                                  : (a.fehlerArt.art == FehlerArt::AnimZuLang && a.fehlerArt.a * 24LL <= 2147483647LL)));
        }
        gGesamtVorgabe = 0ULL;
        gFreiVorgabe = 0ULL;
    }
#endif

    // Unkomprimiert: ein Weg der Wurzel ueber 512 Einheiten bleibt (die 14 Bytes des Spiels kappen bei +-512)
    if (!glb.anim.bones.empty()) {
        AnimImport weit = glb;
        for (std::size_t f = 0; f < static_cast<std::size_t>(weit.anim.numFrames); ++f) { weit.anim.roh[f * weit.anim.bones.size()].m[1][3] -= 40.0F * static_cast<float>(f); }
        std::vector<BoneMatrix> w;
        const int letztes = glb.anim.numFrames - 1;
        weit.anim.worldMatrices(letztes, w);
        std::vector<BoneMatrix> w0;
        glb.anim.worldMatrices(letztes, w0);
        const float soll = 40.0F * static_cast<float>(letztes);
        expect("Wurzel " + z(soll) + " cm weit: " + z(w0[0].m[1][3] - w[0].m[1][3]), soll > 1000.0F && std::fabs(w0[0].m[1][3] - w[0].m[1][3] - soll) < 0.01F);
        std::string ga;
        std::string fehler;
        expect("schreibGla lehnt unkomprimierte Bilder ab", !schreibGla(glb.anim, ga, &fehler) && !fehler.empty());

        // BVH schreiben -> lesen: gleiche Orte und Drehungen
        std::string bvh;
        expect("BVH geschrieben " + fehler, schreibBvh(glb.anim, 0, glb.anim.numFrames, 30, bvh, &fehler));
        const std::string tmp = "animimport_rund.bvh";
        {
            std::ofstream o(tmp, std::ios::binary);
            o << bvh;
        }
        AnimImport rund;
        expect("BVH gelesen " + fehler, animImportieren(tmp, rund, &fehler));
        float om = 0.0F;
        float gm = 0.0F;
        vergleich(glb.anim, rund.anim, glb.anim.numFrames, om, gm);
        expect("Rundlauf glb -> BVH -> lesen: Ort " + z(om) + " cm, Drehung " + z(gm) + " Grad (" + g_wo + ")", om < 1e-3F && gm < 0.01F && rund.sequenzen.front().fps == 30);
        std::remove(tmp.c_str());

        // Retargeting auf das JKA-Skelett (Mixamo-Vorlage -> _humanoid) mit Becken, FK, IK, Kontakten
        const GlaAnimation jka = jkaZiel();
        const RtCharakter cq = rtCharakter(glb.anim, "testgang.glb");
        const RtCharakter cz = rtCharakter(jka, jka.name);
        const RtZuordnung zu = rtZuordnen(cq, cz);
        RtEinstellung e;
        std::vector<std::vector<BoneMatrix>> welt;
        RtBericht br;
        const bool ok = rtUebertragen(glb.anim, cq, jka, cz, zu, e, 0, glb.anim.numFrames, welt, &br);
        expect("Retarget Mixamo -> _humanoid: " + std::to_string(br.kettenZugeordnet) + " Ketten, " + std::to_string(br.knochenMitQuelle) + " Knochen mit Quelle, k " + z(br.k) + " " + br.fehler,
               ok && br.kettenZugeordnet >= 9);
        expect("Fussrutschen " + z(br.rutschenMax) + " Einheiten (Quelle " + z(br.rutschenQuelle) + ", Grenze 0,5 wie die Kontaktregel)", ok && br.rutschenMax < 0.5F);

        // Punkt 4: eigene Figur auf eigenem Skelett (aus dem FBX: mit dem Objekt Armature als Wurzel = model_root)
        AnimImport fbx;
        if (animImportieren(d + "/testgang.fbx", fbx, &fehler)) {
            const std::string ziel = argc > 3 ? argv[3] : "animimport_figur";
            std::string bericht;
            const bool fig = eigeneFigur(fbx, br.k, d + "/../../data/fixtures/animation.cfg", ziel, bericht);
            expect("eigene Figur (k " + z(br.k) + "): " + bericht, fig);
            if (argc <= 3) { std::filesystem::remove_all(ziel); }
        }
    }
    // Preset-Bibliothek data/presets: jeder Eintrag lesbar, Quelle und Lizenz genannt, Schleifen nahtlos, auf _humanoid uebertragen
    // mit kleinem Fussrutschen
    if (argc > 2) {
        const std::string b = argv[2];
        std::ifstream f(b + "/presets.txt");
        int eintraege = 0;
        std::size_t bytes = 0;
        const GlaAnimation jka = jkaZiel();
        const RtCharakter cz = rtCharakter(jka, jka.name);
        for (std::string zeile; std::getline(f, zeile);) {
            if (zeile.empty() || zeile[0] == '#') { continue; }
            std::vector<std::string> t;
            std::size_t a0 = 0;
            for (std::size_t i = 0; i <= zeile.size(); ++i) {
                if (i == zeile.size() || zeile[i] == '|') {
                    t.push_back(zeile.substr(a0, i - a0));
                    a0 = i + 1;
                }
            }
            if (t.size() != 7) {
                expect("Bibliothek: Zeile mit 7 Feldern: " + zeile, false);
                continue;
            }
            ++eintraege;
            AnimImport a;
            std::string fehler;
            const std::string pfad = b + "/" + t[2];
            {
                std::ifstream g(pfad, std::ios::binary | std::ios::ate);
                bytes += static_cast<std::size_t>(g.tellg());
            }
            if (!animImportieren(pfad, a, &fehler)) {
                expect("Bibliothek " + t[1] + ": gelesen " + fehler, false);
                continue;
            }
            // Naht: Sprung vom letzten zum ersten Bild gegen den groessten Schritt im Zyklus (Orte aller Knochen, cm); Grenze 1,25 x
            // (der Schritt b-1 -> b = a gehoert nicht zu den inneren Schritten, er darf etwas groesser sein als sie)
            std::vector<std::vector<BoneMatrix>> w(static_cast<std::size_t>(a.anim.numFrames));
            for (int k = 0; k < a.anim.numFrames; ++k) { a.anim.worldMatrices(k, w[uz(k)]); }
            auto schritt = [&](std::size_t x, std::size_t y) {
                float m = 0.0F;
                for (std::size_t k = 0; k < w[x].size(); ++k) { m = std::max(m, abst(ort(w[x][k]), ort(w[y][k]))); }
                return m;
            };
            float innen = 0.0F;
            for (std::size_t k = 1; k < w.size(); ++k) { innen = std::max(innen, schritt(k - 1, k)); }
            const float naht = w.size() > 2 ? schritt(w.size() - 1, 0) : 0.0F;
            const int v = rtVorlageFuer(a.anim, t[2]);
            const RtCharakter cq = rtCharakter(a.anim, t[2]);
            std::vector<std::vector<BoneMatrix>> welt;
            RtBericht br;
            const bool ok = rtUebertragen(a.anim, cq, jka, cz, rtZuordnen(cq, cz), RtEinstellung{}, 0, a.anim.numFrames, welt, &br);
            const bool schleife = t[6] == "1";
            // Boden: der tiefste Knoechel des Ziels ueber alle Bilder gegen den der Bindepose (die Figur steht weiter auf dem Boden,
            // sie schwebt nicht und sinkt nicht ein)
            // (die Knoechel ltalus/rtalus; model_root bleibt immer im Ursprung)
            const int kl = boneIndex(jka, "ltalus");
            const int kr = boneIndex(jka, "rtalus");
            float tiefZiel = 1e9F;
            for (const auto& wb : welt) {
                for (const int kk : {kl, kr}) { tiefZiel = std::min(tiefZiel, wb[uz(kk)].m[2][3]); }
            }
            const float tiefBinde = std::min(jka.bones[uz(kl)].basePose.m[2][3], jka.bones[uz(kr)].basePose.m[2][3]);
            const float boden = tiefZiel - tiefBinde;
            expect("Bibliothek " + t[0] + "/" + t[1] + ": " + std::to_string(a.anim.bones.size()) + " Knochen, " + std::to_string(a.anim.numFrames) + " Bilder " +
                       std::to_string(a.sequenzen.front().fps) + " fps, Vorlage " + (v >= 0 ? rtVorlagen()[uz(v)].name : std::string("vorgeschlagen")) + ", " +
                       std::to_string(br.kettenZugeordnet) + " Ketten, k " + z(br.k) + ", Rutschen " + z(br.rutschenMax) + " (Quelle " + z(br.rutschenQuelle) +
                       "), Boden " + z(boden) + ", " + (schleife ? "Naht " + z(naht) + " / Schritt " + z(innen) + " cm" : std::string("einmal")) + "; " + t[3] + ", " + t[4],
                   ok && br.kettenZugeordnet >= 9 && !t[3].empty() && !t[4].empty() && (!schleife || naht <= 1.25F * innen + 1e-3F) &&
                       (t[0] == "other" || std::fabs(boden) < 3.0F));
        }
        expect("Bibliothek: " + std::to_string(eintraege) + " Eintraege, " + std::to_string(bytes) + " Bytes", eintraege >= 4);
    }
    // FIX2 P9: eine lange Aufnahme (BHED_AI_LANG=<datei>): Zeit des Imports und Pruefsumme der Bilder (vorher/nachher bytegleich)
    if (const char* lang = std::getenv("BHED_AI_LANG"); lang != nullptr && *lang != 0) {
        AnimImport a;
        std::string fehler;
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = animImportieren(lang, a, &fehler);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::printf("lang %s: %s, %d Bilder, %zu Knochen, %.1f ms, FNV %016llx\n", lang, ok ? "gelesen" : fehler.c_str(), a.anim.numFrames,
                    a.anim.bones.size(), ms, rohHash(a.anim));
    }
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "alles ok" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
