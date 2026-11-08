// modell/animimport.cpp - Animationen aus glTF/FBX/BVH/COLLADA lesen (assimp) und BVH schreiben. Siehe animimport.h.
#include "bhed/modell/animimport.h"

#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <new>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace bhed::modell {
namespace {

// --- kleine Rechnung in double ----------------------------------------------------------------------------------------
struct M3 {
    std::array<std::array<double, 3>, 3> r{};
};
struct Starr {   // Drehung (orthonormal) und Ort
    M3 d;
    std::array<double, 3> o{};
};
M3 einheit3() {
    M3 m;
    for (int i = 0; i < 3; ++i) { m.r[static_cast<std::size_t>(i)][static_cast<std::size_t>(i)] = 1.0; }
    return m;
}
M3 mal(const M3& a, const M3& b) {
    M3 m;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            m.r[i][j] = a.r[i][0] * b.r[0][j] + a.r[i][1] * b.r[1][j] + a.r[i][2] * b.r[2][j];
        }
    }
    return m;
}
M3 transp(const M3& a) {
    M3 m;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) { m.r[i][j] = a.r[j][i]; }
    }
    return m;
}
std::array<double, 3> malV(const M3& a, const std::array<double, 3>& v) {
    return {a.r[0][0] * v[0] + a.r[0][1] * v[1] + a.r[0][2] * v[2], a.r[1][0] * v[0] + a.r[1][1] * v[1] + a.r[1][2] * v[2],
            a.r[2][0] * v[0] + a.r[2][1] * v[1] + a.r[2][2] * v[2]};
}
Starr malS(const Starr& a, const Starr& b) {
    Starr s;
    s.d = mal(a.d, b.d);
    const std::array<double, 3> v = malV(a.d, b.o);
    for (std::size_t i = 0; i < 3; ++i) { s.o[i] = v[i] + a.o[i]; }
    return s;
}
Starr invS(const Starr& a) {
    Starr s;
    s.d = transp(a.d);
    const std::array<double, 3> v = malV(s.d, a.o);
    for (std::size_t i = 0; i < 3; ++i) { s.o[i] = -v[i]; }
    return s;
}
BoneMatrix alsBone(const Starr& s) {
    BoneMatrix m;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) { m.m[i][j] = static_cast<float>(s.d.r[i][j]); }
        m.m[i][3] = static_cast<float>(s.o[i]);
    }
    return m;
}
Starr ausBone(const BoneMatrix& m) {
    Starr s;
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) { s.d.r[i][j] = m.m[i][j]; }
        s.o[i] = m.m[i][3];
    }
    return s;
}
// Spalten normieren und orthogonalisieren (Gram-Schmidt, die erste Spalte bleibt in ihrer Richtung): entfernt die
// Skalierung einer Weltmatrix. Gibt die Laengen der Spalten zurueck (fuer den Hinweis auf ungleichmaessige Skalierung).
std::array<double, 3> orthonormal(M3& m) {
    std::array<std::array<double, 3>, 3> c{};
    std::array<double, 3> len{};
    for (std::size_t j = 0; j < 3; ++j) {
        for (std::size_t i = 0; i < 3; ++i) { c[j][i] = m.r[i][j]; }
    }
    auto dot = [](const std::array<double, 3>& a, const std::array<double, 3>& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    for (std::size_t j = 0; j < 3; ++j) {
        len[j] = std::sqrt(dot(c[j], c[j]));
        for (std::size_t k = 0; k < j; ++k) {
            const double p = dot(c[j], c[k]);
            for (std::size_t i = 0; i < 3; ++i) { c[j][i] -= p * c[k][i]; }
        }
        const double l = std::sqrt(dot(c[j], c[j]));
        for (std::size_t i = 0; i < 3; ++i) { c[j][i] = l > 1e-12 ? c[j][i] / l : (i == j ? 1.0 : 0.0); }
    }
    for (std::size_t j = 0; j < 3; ++j) {
        for (std::size_t i = 0; i < 3; ++i) { m.r[i][j] = c[j][i]; }
    }
    return len;
}

// --- assimp-Szene ------------------------------------------------------------------------------------------------------
struct Mat4 {   // aiMatrix4x4 in double (Zeilen; Spalte 3 = Ort)
    std::array<std::array<double, 4>, 4> r{};
};
Mat4 ausAi(const aiMatrix4x4& a) {
    Mat4 m;
    const ai_real* p = &a.a1;
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < 4; ++j) { m.r[i][j] = static_cast<double>(p[i * 4 + j]); }
    }
    return m;
}
Mat4 mal4(const Mat4& a, const Mat4& b) {
    Mat4 m;
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            m.r[i][j] = a.r[i][0] * b.r[0][j] + a.r[i][1] * b.r[1][j] + a.r[i][2] * b.r[2][j] + a.r[i][3] * b.r[3][j];
        }
    }
    return m;
}
Mat4 einheit4() {
    Mat4 m;
    for (std::size_t i = 0; i < 4; ++i) { m.r[i][i] = 1.0; }
    return m;
}

// Schluessel abtasten wie assimp es meint: Ort/Skalierung linear, Drehung sphaerisch (aiQuaternion::Interpolate), vor dem
// ersten und nach dem letzten Schluessel der Randwert.
// FIX2 P9: der Schluessel vor t ist der erste i mit k[i + 1].mTime > t, von vorn gesucht. Die Bilder einer Animation laufen in
// steigender Zeit: bei aufsteigenden Schluesselzeiten beginnt die Suche dort, wo sie im letzten Bild stand (`ab`, je Spur) -
// dasselbe i, aber linear statt quadratisch in der Laenge der Aufnahme. Ohne Ordnung (ab == nullptr) von vorn wie bisher.
template <typename K>
std::size_t schluesselVor(const K* k, unsigned n, double t, std::size_t* ab) {
    std::size_t i = ab != nullptr ? *ab : 0;
    while (i + 1 < n && k[i + 1].mTime <= t) { ++i; }
    if (ab != nullptr) { *ab = i; }
    return i;
}
template <typename K>
bool aufsteigend(const K* k, unsigned n) {
    for (unsigned i = 1; i < n; ++i) {
        if (k[i].mTime < k[i - 1].mTime) { return false; }
    }
    return true;
}
aiVector3D tasteV(const aiVectorKey* k, unsigned n, double t, const aiVector3D& vorgabe, std::size_t* ab) {
    if (n == 0) { return vorgabe; }
    if (n == 1 || t <= k[0].mTime) { return k[0].mValue; }
    if (t >= k[n - 1].mTime) { return k[n - 1].mValue; }
    const std::size_t i = schluesselVor(k, n, t, ab);
    const double d = k[i + 1].mTime - k[i].mTime;
    const auto f = static_cast<ai_real>(d > 0.0 ? (t - k[i].mTime) / d : 0.0);
    return k[i].mValue + (k[i + 1].mValue - k[i].mValue) * f;
}
aiQuaternion tasteQ(const aiQuatKey* k, unsigned n, double t, const aiQuaternion& vorgabe, std::size_t* ab) {
    if (n == 0) { return vorgabe; }
    if (n == 1 || t <= k[0].mTime) { return k[0].mValue; }
    if (t >= k[n - 1].mTime) { return k[n - 1].mValue; }
    const std::size_t i = schluesselVor(k, n, t, ab);
    const double d = k[i + 1].mTime - k[i].mTime;
    const auto f = static_cast<ai_real>(d > 0.0 ? (t - k[i].mTime) / d : 0.0);
    aiQuaternion q;
    aiQuaternion::Interpolate(q, k[i].mValue, k[i + 1].mValue, f);
    return q.Normalize();
}

// FIX4 P34: der Arbeitsspeicher des Rechners in Bytes (0: unbekannt) - fest je Rechner, nicht der gerade freie (FIX2 P5 nahm
// ullAvailPhys: dieselbe Datei mit offenem Browser abgelehnt, nach dem Schliessen gelesen)
std::uint64_t speicherGesamt() {
#ifdef _WIN32
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m) != 0) { return m.ullTotalPhys; }
#endif
    return 0;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string endung(const std::string& p) {
    const std::size_t d = p.find_last_of('.');
    const std::size_t s = p.find_last_of("/\\");
    if (d == std::string::npos || (s != std::string::npos && d < s)) { return {}; }
    return klein(p.substr(d + 1));
}

// Seite aus dem Namen: 1 links, -1 rechts, 0 keine (Left*/Right*, *_l/*_r, *.L/*.R, l_/r_ am Anfang)
int seiteAusName(const std::string& n0) {
    const std::string n = klein(n0.substr(n0.find_last_of(':') == std::string::npos ? 0 : n0.find_last_of(':') + 1));
    if (n.find("left") != std::string::npos) { return 1; }
    if (n.find("right") != std::string::npos) { return -1; }
    const std::size_t l = n.size();
    if (l > 2 && (n[l - 2] == '_' || n[l - 2] == '.')) {
        if (n[l - 1] == 'l') { return 1; }
        if (n[l - 1] == 'r') { return -1; }
    }
    if (l > 2 && n[1] == '_') {
        if (n[0] == 'l') { return 1; }
        if (n[0] == 'r') { return -1; }
    }
    return 0;
}

std::array<double, 3> achse(int i, int vz) {
    std::array<double, 3> a{};
    a[static_cast<std::size_t>(std::clamp(i, 0, 2))] = vz < 0 ? -1.0 : 1.0;
    return a;
}
std::string achsText(const std::array<double, 3>& a) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs(a[i]) > 0.5) { return std::string(a[i] > 0.0 ? "+" : "-") + static_cast<char>('X' + i); }
    }
    return "?";
}
std::array<double, 3> kreuz(const std::array<double, 3>& a, const std::array<double, 3>& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

int metaInt(const aiScene* sc, const char* k, int vorgabe) {
    if (sc->mMetaData == nullptr) { return vorgabe; }
    std::int32_t v = vorgabe;
    if (sc->mMetaData->Get<std::int32_t>(k, v)) { return v; }
    return vorgabe;
}
double metaDouble(const aiScene* sc, const char* k, double vorgabe) {
    if (sc->mMetaData == nullptr) { return vorgabe; }
    double d = vorgabe;
    if (sc->mMetaData->Get<double>(k, d)) { return d; }
    float f = static_cast<float>(vorgabe);
    if (sc->mMetaData->Get<float>(k, f)) { return f; }
    return vorgabe;
}

struct Knoten {
    const aiNode* n = nullptr;
    int eltern = -1;          // Index in der Knotenliste
    int knochen = -1;         // Index im Skelett (-1: kein Knochen)
};

void sammeln(const aiNode* n, int eltern, std::vector<Knoten>& k) {
    const int ich = static_cast<int>(k.size());
    k.push_back(Knoten{n, eltern, -1});
    for (unsigned i = 0; i < n->mNumChildren; ++i) { sammeln(n->mChildren[i], ich, k); }
}

}  // namespace

bool istAnimImportEndung(const std::string& pfad) {
    const std::string e = endung(pfad);
    return e == "fbx" || e == "glb" || e == "gltf" || e == "bvh" || e == "dae";
}

bool animImportieren(const std::string& pfad, AnimImport& aus, std::string* fehler, int fpsVorgabe) {
    aus = AnimImport{};
    auto falschText = [&](const std::string& f) {
        if (fehler != nullptr) { *fehler = f; }
        return false;
    };
    auto falsch = [&](const ModellFehler& f) {
        aus.fehlerArt = f;
        return falschText(fehlerText(f));
    };
    Assimp::Importer imp;
    // die Lage nicht von assimp umrechnen lassen (correctRootTransform nur mit Netzen; Achsen unten aus den Metadaten),
    // keine Pivot-Hilfsknoten ($AssimpFbx$), die die Knochen zerteilen
    imp.SetPropertyBool(AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION, true);
    imp.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    imp.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);
    // COLLADA: die Knotennamen aus name="..." (sonst die id, bei Blender "Armature_mixamorig_Hips" - die Vorlagen kennen die
    // Namen der Knochen, nicht die ids; AI_CONFIG_IMPORT_COLLADA_USE_COLLADA_NAMES, config.h)
    imp.SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_USE_COLLADA_NAMES, true);
    const aiScene* sc = imp.ReadFile(pfad, aiProcess_ValidateDataStructure);
    if (sc == nullptr || sc->mRootNode == nullptr) { return falschText(std::string("assimp: ") + imp.GetErrorString()); }
    if (sc->mNumAnimations == 0) { return falsch(ModellFehler{FehlerArt::KeineAnimation}); }
    const std::string e = endung(pfad);

    // Achsen und Einheit der Datei
    std::array<double, 3> oben = achse(1, 1);
    std::array<double, 3> vorn = achse(2, 1);
    std::array<double, 3> links = achse(0, 1);
    double cm = 1.0;
    bool linksAusNamen = false;
    if (e == "fbx") {
        aus.format = "FBX";
        // FBX SDK FbxAxisSystem: CoordAxis ist die dritte Achse (bei Maya Y-oben +X, die linke Seite einer Figur, die zur
        // Vorderansicht +Z schaut), FrontAxis die Vorderseite; UnitScaleFactor: Zentimeter je Einheit (FbxSystemUnit)
        oben = achse(metaInt(sc, "UpAxis", 1), metaInt(sc, "UpAxisSign", 1));
        vorn = achse(metaInt(sc, "FrontAxis", 2), metaInt(sc, "FrontAxisSign", 1));
        links = achse(metaInt(sc, "CoordAxis", 0), metaInt(sc, "CoordAxisSign", 1));
        cm = metaDouble(sc, "UnitScaleFactor", 1.0);
        if (!(cm > 0.0)) { cm = 1.0; }
    } else if (e == "glb" || e == "gltf") {
        aus.format = "glTF";
        cm = 100.0;   // glTF 2.0 3.4: Meter
    } else if (e == "dae") {
        aus.format = "COLLADA";
        cm = 100.0;   // assimp rechnet <unit meter> in die Wurzel, danach Meter
    } else if (e == "bvh") {
        aus.format = "BVH";
        linksAusNamen = true;
    } else {
        aus.format = e;
    }

    std::vector<Knoten> kn;
    sammeln(sc->mRootNode, -1, kn);
    std::map<std::string, int> nachName;
    for (std::size_t i = 0; i < kn.size(); ++i) { nachName.emplace(kn[i].n->mName.C_Str(), static_cast<int>(i)); }

    // Skelett: Knoten mit Spur in einer Animation oder als Knochen eines Netzes, dazu die Knoten zwischen zweien davon
    std::vector<std::uint8_t> drin(kn.size(), 0);
    for (unsigned a = 0; a < sc->mNumAnimations; ++a) {
        const aiAnimation* an = sc->mAnimations[a];
        for (unsigned c = 0; c < an->mNumChannels; ++c) {
            const auto it = nachName.find(an->mChannels[c]->mNodeName.C_Str());
            if (it != nachName.end()) { drin[static_cast<std::size_t>(it->second)] = 1; }
        }
    }
    for (unsigned m = 0; m < sc->mNumMeshes; ++m) {
        for (unsigned b = 0; b < sc->mMeshes[m]->mNumBones; ++b) {
            const auto it = nachName.find(sc->mMeshes[m]->mBones[b]->mName.C_Str());
            if (it != nachName.end()) { drin[static_cast<std::size_t>(it->second)] = 1; }
        }
    }
    // BVH: "End Site" ist kein Gelenk, nur das Ende des letzten Knochens (BVHLoader.cpp ReadEndSite: Knoten "EndSite_<eltern>",
    // ohne Kanaele; nach den Netz-Knochen, auch dort stehen sie)
    if (e == "bvh") {
        for (std::size_t i = 0; i < kn.size(); ++i) {
            if (std::string(kn[i].n->mName.C_Str()).rfind("EndSite_", 0) == 0 && kn[i].n->mNumChildren == 0) { drin[i] = 0; }
        }
    }
    std::vector<std::uint8_t> vorfahr(kn.size(), 0);   // ein Vorfahr ist drin
    for (std::size_t i = 0; i < kn.size(); ++i) {
        const int p = kn[i].eltern;
        if (p >= 0) { vorfahr[i] = static_cast<std::uint8_t>(vorfahr[static_cast<std::size_t>(p)] != 0 || drin[static_cast<std::size_t>(p)] != 0); }
    }
    std::vector<std::uint8_t> nachfahr(kn.size(), 0);   // ein Nachfahr ist drin (Kinder stehen hinter den Eltern)
    for (std::size_t i = kn.size(); i-- > 0;) {
        const int p = kn[i].eltern;
        if (p >= 0 && (drin[i] != 0 || nachfahr[i] != 0)) { nachfahr[static_cast<std::size_t>(p)] = 1; }
    }
    GlaAnimation& g = aus.anim;
    for (std::size_t i = 0; i < kn.size(); ++i) {
        if (drin[i] != 0 || (vorfahr[i] != 0 && nachfahr[i] != 0)) {
            kn[i].knochen = static_cast<int>(g.bones.size());
            GlaBone b;
            b.name = kn[i].n->mName.C_Str();
            int p = kn[i].eltern;
            while (p >= 0 && kn[static_cast<std::size_t>(p)].knochen < 0) { p = kn[static_cast<std::size_t>(p)].eltern; }
            b.parent = p >= 0 ? kn[static_cast<std::size_t>(p)].knochen : -1;
            g.bones.push_back(std::move(b));
        }
    }
    if (g.bones.empty()) { return falsch(ModellFehler{FehlerArt::KeinSkelett}); }
    for (std::size_t b = 0; b < g.bones.size(); ++b) {
        const int p = g.bones[b].parent;
        if (p >= 0) { g.bones[static_cast<std::size_t>(p)].kinder.push_back(static_cast<int>(b)); }
    }

    // Welt aller Knoten fuer eine Belegung der lokalen Matrizen
    auto welten = [&](const std::vector<Mat4>& lokal, std::vector<Mat4>& w) {
        w.resize(kn.size());
        for (std::size_t i = 0; i < kn.size(); ++i) {
            w[i] = kn[i].eltern < 0 ? lokal[i] : mal4(w[static_cast<std::size_t>(kn[i].eltern)], lokal[i]);
        }
    };
    std::vector<Mat4> ruhe(kn.size());
    for (std::size_t i = 0; i < kn.size(); ++i) { ruhe[i] = ausAi(kn[i].n->mTransformation); }
    std::vector<Mat4> wRuhe;
    welten(ruhe, wRuhe);

    if (linksAusNamen) {
        // BVH: oben ist die Achse, entlang der der Rumpf steht (Wurzel -> der fernste Knochen ohne Seite, Kopf), in der
        // Ruhepose; Blender schreibt +Z, die meisten Aufnahmen +Y
        int wurzel = -1;
        int kopf = -1;
        double weit = 0.0;
        for (std::size_t i = 0; i < kn.size(); ++i) {
            if (kn[i].knochen < 0) { continue; }
            if (wurzel < 0) {
                wurzel = static_cast<int>(i);
                continue;
            }
            if (seiteAusName(kn[i].n->mName.C_Str()) != 0) { continue; }
            double d = 0.0;
            for (std::size_t k = 0; k < 3; ++k) {
                const double v = wRuhe[i].r[k][3] - wRuhe[static_cast<std::size_t>(wurzel)].r[k][3];
                d += v * v;
            }
            if (d > weit) {
                weit = d;
                kopf = static_cast<int>(i);
            }
        }
        if (kopf >= 0) {
            std::size_t best = 1;
            double bw = 0.0;
            for (std::size_t k = 0; k < 3; ++k) {
                const double v = wRuhe[static_cast<std::size_t>(kopf)].r[k][3] - wRuhe[static_cast<std::size_t>(wurzel)].r[k][3];
                if (std::fabs(v) > std::fabs(bw)) {
                    bw = v;
                    best = k;
                }
            }
            oben = achse(static_cast<int>(best), bw < 0.0 ? -1 : 1);
            links = achse(static_cast<int>((best + 2) % 3), 1);
            vorn = kreuz(links, oben);
        }
        // links aus den Seiten der Knochennamen in der Ruhepose (Mittel links - rechts, senkrecht zu oben)
        std::array<double, 3> s{};
        int zahl = 0;
        for (std::size_t i = 0; i < kn.size(); ++i) {
            if (kn[i].knochen < 0) { continue; }
            const int sd = seiteAusName(kn[i].n->mName.C_Str());
            if (sd == 0) { continue; }
            for (std::size_t k = 0; k < 3; ++k) { s[k] += sd * wRuhe[i].r[k][3]; }
            ++zahl;
        }
        const double o = s[0] * oben[0] + s[1] * oben[1] + s[2] * oben[2];
        for (std::size_t k = 0; k < 3; ++k) { s[k] -= o * oben[k]; }
        const double l = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
        if (zahl > 0 && l > 1e-9) {
            for (std::size_t k = 0; k < 3; ++k) { links[k] = s[k] / l; }
            vorn = kreuz(links, oben);   // rechtshaendig: links x oben = vorn (JKA: +X x +Z = -Y, glTF: +X x +Y = +Z)
        } else {
            aus.hinweise.push_back(ModellFehler{FehlerArt::BvhOhneSeiten});
        }
    }
    // Umrechnung Datei -> JK Studio (JKA): links -> +X, vorn -> -Y, oben -> +Z, Laengen in cm
    M3 c;
    for (std::size_t j = 0; j < 3; ++j) {
        c.r[0][j] = links[j];
        c.r[1][j] = -vorn[j];
        c.r[2][j] = oben[j];
    }
    const double det = c.r[0][0] * (c.r[1][1] * c.r[2][2] - c.r[1][2] * c.r[2][1]) - c.r[0][1] * (c.r[1][0] * c.r[2][2] - c.r[1][2] * c.r[2][0]) +
                       c.r[0][2] * (c.r[1][0] * c.r[2][1] - c.r[1][1] * c.r[2][0]);
    if (det < 0.5) {
        ModellFehler f{FehlerArt::AchsenUnbestimmt, achsText(oben)};
        f.name2 = achsText(vorn);
        return falsch(f);
    }
    aus.oben = achsText(oben);
    aus.vorn = achsText(vorn);
    aus.vornAusSkelett = linksAusNamen;
    aus.zentimeter = static_cast<float>(cm);

    bool ungleich = false;
    auto knochenWelt = [&](const std::vector<Mat4>& w, std::vector<Starr>& x) {
        x.resize(g.bones.size());
        for (std::size_t i = 0; i < kn.size(); ++i) {
            const int b = kn[i].knochen;
            if (b < 0) { continue; }
            M3 d;
            for (std::size_t r = 0; r < 3; ++r) {
                for (std::size_t s = 0; s < 3; ++s) { d.r[r][s] = w[i].r[r][s]; }
            }
            d = mal(c, d);
            const std::array<double, 3> len = orthonormal(d);
            if (std::fabs(len[0] - len[1]) > 1e-3 * len[0] || std::fabs(len[0] - len[2]) > 1e-3 * len[0]) { ungleich = true; }
            Starr s;
            s.d = d;
            s.o = malV(c, {w[i].r[0][3], w[i].r[1][3], w[i].r[2][3]});
            for (double& v : s.o) { v *= cm; }
            x[static_cast<std::size_t>(b)] = s;
        }
    };
    std::vector<Starr> basis;
    knochenWelt(wRuhe, basis);
    std::vector<std::vector<Starr>> alleX;   // je Bild die Welt der Knochen (die lokalen Stellungen erst nach der Grundstellung)

    // Bilder je Animation: erst Bildrate, Anfang und Zahl aller Animationen, dann begrenzen (FIX2 P5), dann abtasten
    aus.ticksJeSekunde = sc->mAnimations[0]->mTicksPerSecond;
    struct Abtastung {
        double tps = 25.0;
        double fps = 25.0;
        double anfang = 0.0;
        double n = 1.0;
    };
    std::vector<Abtastung> abt(sc->mNumAnimations);
    double alleBilder = 0.0;
    for (unsigned a = 0; a < sc->mNumAnimations; ++a) {
        const aiAnimation* an = sc->mAnimations[a];
        const double tps = an->mTicksPerSecond > 0.0 ? an->mTicksPerSecond : 25.0;   // assimp: 0 = nicht angegeben
        // Bildrate: FBX/BVH Schluessel auf ganzen Ticks, Ticks = Bilder; glTF/COLLADA: kleinster Abstand der Schluessel
        double fps = tps;
        if (aus.format == "glTF" || aus.format == "COLLADA" || tps > 240.0) {
            double abstand = 0.0;
            for (unsigned ch = 0; ch < an->mNumChannels; ++ch) {
                const aiNodeAnim* na = an->mChannels[ch];
                for (unsigned k = 1; k < na->mNumRotationKeys; ++k) {
                    const double d = na->mRotationKeys[k].mTime - na->mRotationKeys[k - 1].mTime;
                    if (d > 1e-6 && (abstand == 0.0 || d < abstand)) { abstand = d; }
                }
                for (unsigned k = 1; k < na->mNumPositionKeys; ++k) {
                    const double d = na->mPositionKeys[k].mTime - na->mPositionKeys[k - 1].mTime;
                    if (d > 1e-6 && (abstand == 0.0 || d < abstand)) { abstand = d; }
                }
            }
            fps = abstand > 0.0 ? std::round(tps / abstand) : 30.0;
        }
        if (fpsVorgabe > 0) { fps = fpsVorgabe; }
        fps = std::clamp(fps, 1.0, 1000.0);
        // Die Animation beginnt mit ihrem ersten Schluessel: glTF und COLLADA tragen die Zeit der Szene (Blender schreibt ab
        // frame_start, Bild 1 = 1/30 s), FBX zieht den Anfang des Takes schon ab (FBXConverter.cpp "adjust relative timing"),
        // BVH beginnt bei 0. Ende = letzter Schluessel (mDuration: "simply the maximum key", FBXConverter.cpp).
        double anfang = -1.0;
        double ende = 0.0;
        for (unsigned ch = 0; ch < an->mNumChannels; ++ch) {
            const aiNodeAnim* na = an->mChannels[ch];
            const auto rand = [&](double t) {
                anfang = anfang < 0.0 ? t : std::min(anfang, t);
                ende = std::max(ende, t);
            };
            if (na->mNumPositionKeys > 0) {
                rand(na->mPositionKeys[0].mTime);
                rand(na->mPositionKeys[na->mNumPositionKeys - 1].mTime);
            }
            if (na->mNumRotationKeys > 0) {
                rand(na->mRotationKeys[0].mTime);
                rand(na->mRotationKeys[na->mNumRotationKeys - 1].mTime);
            }
        }
        anfang = std::max(anfang, 0.0);
        ende = std::max(ende, anfang);
        const double sekunden = (ende - anfang) / tps;
        // + 1e-3: die Zeiten sind float (glTF in Sekunden, assimp x 1000), 1000 ms kommen als 999,99997 an; in double, damit eine
        // ferne Zeit nicht ueber int laeuft (FIX2 P5: vorher undefiniert, unter MSVC n = 1)
        abt[a] = Abtastung{tps, fps, anfang, std::max(1.0, std::floor(sekunden * fps + 1e-3) + 1.0)};
        alleBilder += abt[a].n;
    }
    // FIX4 P34: zwei feste Grenzen, keine vom Augenblick (vorher der gerade freie Speicher, ullAvailPhys):
    // (1) das Zielformat GLA: je Bild und Knochen ein Verweis von 3 Byte ((iFrameNum * numBones * 3) + (iBoneNum * 3),
    //     code/rd-common/mdx_format.h:426-439), Bildzahl und alle Offsets der Datei sind int (mdxaHeader_t numFrames, ofsFrames,
    //     ofsCompBonePool, ofsEnd, :389-395) - Bilder x Knochen x 3 muss in int passen (damit auch die Bildzahl selbst);
    // (2) der Arbeitsspeicher des Rechners (ullTotalPhys): je Bild und Knochen eine Welt (Starr) und eine lokale Stellung (roh),
    //     dazu je Bild ein Vektor - mehr als eingebaut ist, liefe nur ueber die Auslagerungsdatei. Das Commit-Limit
    //     (ullTotalPageFile) waechst mit der Auslagerungsdatei und waere wieder vom Augenblick abhaengig.
    // bad_alloc beim Abtasten (der Prozess bekommt den Speicher nicht) gibt weiter die Meldung (2).
    // FIX5 P37: (1) fuer die ganze Datei bis zum Vorrat (Kopf, Verzeichnis, Hierarchie, Bilder; glaDateiGroesse) - vorher nur der
    // Bildteil, dahinter lief ofsEnd ueber. Den Vorrat kennt erst das Backen; schreibGla prueft die ganze Datei noch einmal.
    std::uint64_t kinder = 0;
    for (const GlaBone& k : g.bones) { kinder += k.kinder.size(); }
    const double knochen = static_cast<double>(std::max<std::size_t>(g.bones.size(), 1U));
    const auto vorBildern = static_cast<double>(glaDateiGroesse(g.bones.size(), kinder, 0U, 0U));
    const double hoechstens = std::floor((static_cast<double>(kGlaHoechstens) - 3.0 - vorBildern) / (knochen * 3.0));
    if (!(alleBilder <= hoechstens)) {
        return falsch(ModellFehler{FehlerArt::AnimZuLangGla, {}, std::llround(std::min(alleBilder, 9.0e18)), std::llround(knochen), {},
                                   std::llround(hoechstens)});
    }
    const double jeBild = static_cast<double>(g.bones.size()) * static_cast<double>(sizeof(Starr) + sizeof(BoneMatrix)) +
                          static_cast<double>(sizeof(std::vector<Starr>));
    const double noetig = alleBilder * jeBild;
    const auto gesamt = static_cast<double>(speicherGesamt());
    const auto zuLang = [&] {
        return falsch(ModellFehler{FehlerArt::AnimZuLang, {}, std::llround(alleBilder), std::llround(noetig / 1048576.0), {}, std::llround(gesamt / 1048576.0)});
    };
    if (gesamt > 0.0 && noetig > gesamt) { return zuLang(); }
    std::vector<Mat4> lokal(kn.size());
    std::vector<Mat4> w;
    std::vector<Starr> x;
    // FIX2 P9: die Ruhe jedes Knotens zerlegt (Vorgabe fehlender Spuren) - einmal, nicht je Bild
    struct Zerlegt {
        aiVector3D s;
        aiQuaternion r;
        aiVector3D p;
    };
    std::vector<Zerlegt> ruheZerlegt(kn.size());
    for (std::size_t i = 0; i < kn.size(); ++i) { kn[i].n->mTransformation.Decompose(ruheZerlegt[i].s, ruheZerlegt[i].r, ruheZerlegt[i].p); }
    g.roh.clear();
    int bilder = 0;
    try {
        for (unsigned a = 0; a < sc->mNumAnimations; ++a) {
            const aiAnimation* an = sc->mAnimations[a];
            const double tps = abt[a].tps;
            const double fps = abt[a].fps;
            const double anfang = abt[a].anfang;
            const int n = static_cast<int>(abt[a].n);
            std::vector<const aiNodeAnim*> spur(kn.size(), nullptr);
            for (unsigned ch = 0; ch < an->mNumChannels; ++ch) {
                const auto it = nachName.find(an->mChannels[ch]->mNodeName.C_Str());
                if (it != nachName.end()) { spur[static_cast<std::size_t>(it->second)] = an->mChannels[ch]; }
            }
            // FIX2 P9: je Spur und Art der laufende Schluessel (nur bei aufsteigenden Zeiten, sonst nullptr: von vorn)
            struct Lauf {
                std::size_t p = 0;
                std::size_t r = 0;
                std::size_t s = 0;
                bool pAuf = false;
                bool rAuf = false;
                bool sAuf = false;
            };
            std::vector<Lauf> lauf(kn.size());
            for (std::size_t i = 0; i < kn.size(); ++i) {
                if (const aiNodeAnim* na = spur[i]) {
                    lauf[i].pAuf = aufsteigend(na->mPositionKeys, na->mNumPositionKeys);
                    lauf[i].rAuf = aufsteigend(na->mRotationKeys, na->mNumRotationKeys);
                    lauf[i].sAuf = aufsteigend(na->mScalingKeys, na->mNumScalingKeys);
                }
            }
            for (int f = 0; f < n; ++f) {
                const double t = anfang + static_cast<double>(f) / fps * tps;
                for (std::size_t i = 0; i < kn.size(); ++i) {
                    const aiNodeAnim* na = spur[i];
                    if (na == nullptr) {
                        lokal[i] = ruhe[i];
                        continue;
                    }
                    const Zerlegt& z0 = ruheZerlegt[i];
                    Lauf& l = lauf[i];
                    const aiVector3D p = tasteV(na->mPositionKeys, na->mNumPositionKeys, t, z0.p, l.pAuf ? &l.p : nullptr);
                    const aiQuaternion r = tasteQ(na->mRotationKeys, na->mNumRotationKeys, t, z0.r, l.rAuf ? &l.r : nullptr);
                    const aiVector3D s = tasteV(na->mScalingKeys, na->mNumScalingKeys, t, z0.s, l.sAuf ? &l.s : nullptr);
                    const aiMatrix4x4 m(s, r, p);
                    lokal[i] = ausAi(m);
                }
                welten(lokal, w);
                knochenWelt(w, x);
                alleX.push_back(x);
            }
            AnimEntry en;
            en.name = an->mName.length > 0 ? an->mName.C_Str() : ("Take " + std::to_string(a + 1));
            // "Armature|Walk" (Blender) -> "Walk"
            const std::size_t bar = en.name.find_last_of('|');
            if (bar != std::string::npos && bar + 1 < en.name.size()) { en.name = en.name.substr(bar + 1); }
            en.firstFrame = bilder;
            en.numFrames = n;
            en.loopFrame = -1;
            en.fps = static_cast<int>(std::lround(fps));
            aus.sequenzen.push_back(en);
            bilder += n;
        }
        // BVH: die Grundstellung (OFFSET-Hierarchie) hat keine Lage im Raum - ROOT OFFSET ist oft 0, die Wurzel steht in den
        // Bildern absolut (100STYLE: Becken in der Grundstellung auf dem Boden, in den Bildern 90 cm hoch). Das Retargeting setzt das
        // Becken des Ziels auf z(P) + k (q(f) - q(P)) (retarget.h) - die Figur schwebte um k mal diesen Abstand (100STYLE auf
        // _humanoid: 32 Einheiten, ctest "Boden"). Darum steht die Grundstellung hier, wo die Wurzel im ersten Bild steht (waagrecht),
        // und ihr tiefster Knochen auf dem tiefsten Stand der Bilder (senkrecht); die Bilder selbst bleiben, wie sie sind.
        if (e == "bvh" && !alleX.empty() && !g.bones.empty()) {
            double tiefRuhe = 1e300;
            double tiefBild = 1e300;
            for (const Starr& b : basis) { tiefRuhe = std::min(tiefRuhe, b.o[2]); }
            for (const std::vector<Starr>& xs : alleX) {
                for (const Starr& b : xs) { tiefBild = std::min(tiefBild, b.o[2]); }
            }
            const std::array<double, 3> d{alleX[0][0].o[0] - basis[0].o[0], alleX[0][0].o[1] - basis[0].o[1], tiefBild - tiefRuhe};
            for (Starr& b : basis) {
                for (std::size_t i = 0; i < 3; ++i) { b.o[i] += d[i]; }
            }
            if (std::fabs(d[2]) > 1.0) { aus.hinweise.push_back(ModellFehler{FehlerArt::BvhBoden, {}, std::llround(d[2] * 10.0)}); }
        }
        for (std::size_t b = 0; b < g.bones.size(); ++b) {
            g.bones[b].basePose = alsBone(basis[b]);
            g.bones[b].basePoseInv = alsBone(invS(basis[b]));
        }
        // lokale Stellung wie die .gla: X(b) = X(p) B(p)^-1 A(b) B(b) -> A(b) = B(p) X(p)^-1 X(b) B(b)^-1 (gla.h)
        g.roh.reserve(alleX.size() * g.bones.size());
        for (const std::vector<Starr>& xs : alleX) {
            for (std::size_t b = 0; b < g.bones.size(); ++b) {
                const int p = g.bones[b].parent;
                Starr lk = malS(xs[b], invS(basis[b]));
                if (p >= 0) { lk = malS(malS(basis[static_cast<std::size_t>(p)], invS(xs[static_cast<std::size_t>(p)])), lk); }
                g.roh.push_back(alsBone(lk));
            }
        }
    } catch (const std::bad_alloc&) {   // FIX2 P5 / FIX4 P34: der Prozess bekommt den Speicher nicht (Auslagerungsdatei voll)
        g.roh.clear();
        g.roh.shrink_to_fit();
        return zuLang();
    }
    if (ungleich) { aus.hinweise.push_back(ModellFehler{FehlerArt::UngleicheSkalierung}); }
    g.numFrames = bilder;
    g.scale = 1.0F;
    const std::size_t sl = pfad.find_last_of("/\\");
    g.name = sl == std::string::npos ? pfad : pfad.substr(sl + 1);
    const std::size_t pk = g.name.find_last_of('.');
    g.skeletonName = pk == std::string::npos ? g.name : g.name.substr(0, pk);
    return true;
}

// --- BVH schreiben ---------------------------------------------------------------------------------------------------
namespace {
// R = Rz(z) Rx(x) Ry(y) (Kanalfolge "Zrotation Xrotation Yrotation", BVHLoader.cpp multipliziert in Kanalfolge)
void zxy(const M3& r, double& z, double& x, double& y) {
    // x ueber atan2 statt asin: nahe +-90 Grad verliert asin die Haelfte der Stellen (Rundlauf-Test: 0,0365 Grad am Fuss)
    const double sx = std::clamp(r.r[2][1], -1.0, 1.0);
    const double cx = std::hypot(r.r[2][0], r.r[2][2]);
    x = std::atan2(sx, cx);
    if (cx > 1e-9) {
        y = std::atan2(-r.r[2][0], r.r[2][2]);
        z = std::atan2(-r.r[0][1], r.r[1][1]);
    } else {
        z = 0.0;
        y = std::atan2(r.r[0][2], r.r[0][0]);
    }
}
}  // namespace

bool schreibBvh(const GlaAnimation& g, int erstes, int anzahl, int fps, std::string& aus, std::string* fehler) {
    auto falsch = [&](const std::string& f) {
        if (fehler != nullptr) { *fehler = f; }
        return false;
    };
    if (g.bones.empty() || anzahl <= 0 || erstes < 0 || erstes + anzahl > g.bildZahl() || fps <= 0) { return falsch("nichts zu schreiben"); }
    const std::size_t nb = g.bones.size();
    int wurzeln = 0;
    for (const GlaBone& b : g.bones) { wurzeln += b.parent < 0 ? 1 : 0; }
    if (wurzeln != 1) { return falsch("BVH braucht genau eine Wurzel (" + std::to_string(wurzeln) + ")"); }
    // JK Studio -> Datei: +X links -> +X, -Y vorn -> +Z, +Z oben -> +Y (Umkehrung des Lesens)
    M3 c;
    c.r[0][0] = 1.0;
    c.r[1][2] = 1.0;
    c.r[2][1] = -1.0;
    // Gelenk-Rahmen der BVH: in der Ruhe achsparallel (Q = X R0^-1), Orte aus der Welt
    std::vector<Starr> b0(nb);
    for (std::size_t b = 0; b < nb; ++b) { b0[b] = ausBone(g.bones[b].basePose); }
    auto gelenk = [&](const std::vector<BoneMatrix>& welt, std::vector<Starr>& q) {
        q.resize(nb);
        for (std::size_t b = 0; b < nb; ++b) {
            const Starr x = ausBone(welt[b]);
            q[b].d = mal(mal(c, mal(x.d, transp(b0[b].d))), transp(c));
            q[b].o = malV(c, x.o);
        }
    };
    // Gelenke ohne eigene Bewegung der Lage (Abstand zum Eltern bleibt die Ruhe, bis 1e-3 cm = 0,01 mm, unter der Rundung des Spiels von 1/64) nur mit 3 Drehkanaelen;
    // assimp nimmt dort die OFFSET-Lage (BVHLoader.cpp: "if no translation part is given")
    std::vector<std::uint8_t> sechs(nb, 0);
    {
        std::vector<BoneMatrix> welt;
        std::vector<Starr> q;
        std::vector<Starr> q0;
        std::vector<BoneMatrix> w0(nb);
        for (std::size_t b = 0; b < nb; ++b) { w0[b] = g.bones[b].basePose; }
        gelenk(w0, q0);
        for (int f = erstes; f < erstes + anzahl; ++f) {
            g.worldMatrices(f, welt);
            gelenk(welt, q);
            for (std::size_t b = 0; b < nb; ++b) {
                const int p = g.bones[b].parent;
                if (p < 0) {
                    sechs[b] = 1;
                    continue;
                }
                const Starr& qp = q[static_cast<std::size_t>(p)];
                std::array<double, 3> d{};
                std::array<double, 3> d0{};
                for (std::size_t i = 0; i < 3; ++i) {
                    d[i] = q[b].o[i] - qp.o[i];
                    d0[i] = q0[b].o[i] - q0[static_cast<std::size_t>(p)].o[i];
                }
                const std::array<double, 3> l = malV(transp(qp.d), d);
                for (std::size_t i = 0; i < 3; ++i) {
                    if (std::fabs(l[i] - d0[i]) > 1e-3) { sechs[b] = 1; }
                }
            }
        }
    }
    char z[160];
    std::string s = "HIERARCHY\n";
    std::vector<int> ordnung;
    std::vector<Starr> qr;
    std::vector<BoneMatrix> wr(nb);
    for (std::size_t b = 0; b < nb; ++b) { wr[b] = g.bones[b].basePose; }
    gelenk(wr, qr);
    const std::function<void(int, int)> knoten = [&](int b, int tiefe) {
        const std::string ein(static_cast<std::size_t>(tiefe) * 2U, ' ');
        const GlaBone& k = g.bones[static_cast<std::size_t>(b)];
        std::array<double, 3> off = qr[static_cast<std::size_t>(b)].o;
        if (k.parent >= 0) {
            for (std::size_t i = 0; i < 3; ++i) { off[i] -= qr[static_cast<std::size_t>(k.parent)].o[i]; }
        }
        std::string name = k.name;
        for (char& ch : name) {
            if (std::isspace(static_cast<unsigned char>(ch)) != 0) { ch = '_'; }
        }
        s += ein + (k.parent < 0 ? "ROOT " : "JOINT ") + name + "\n" + ein + "{\n";
        std::snprintf(z, sizeof z, "%s  OFFSET %.4f %.4f %.4f\n", ein.c_str(), off[0], off[1], off[2]);
        s += z;
        s += ein + (sechs[static_cast<std::size_t>(b)] != 0 ? "  CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
                                                           : "  CHANNELS 3 Zrotation Xrotation Yrotation\n");
        ordnung.push_back(b);
        if (k.kinder.empty()) {
            s += ein + "  End Site\n" + ein + "  {\n" + ein + "    OFFSET 0.000000 0.000000 0.000000\n" + ein + "  }\n";
        }
        for (const int kd : k.kinder) { knoten(kd, tiefe + 1); }
        s += ein + "}\n";
    };
    for (std::size_t b = 0; b < nb; ++b) {
        if (g.bones[b].parent < 0) { knoten(static_cast<int>(b), 0); }
    }
    if (ordnung.size() != nb) { return falsch("Kinderliste unvollstaendig"); }
    std::snprintf(z, sizeof z, "MOTION\nFrames: %d\nFrame Time: %.9f\n", anzahl, 1.0 / fps);
    s += z;
    std::vector<BoneMatrix> welt;
    std::vector<Starr> q;
    constexpr double kGrad = 180.0 / 3.14159265358979323846;
    for (int f = erstes; f < erstes + anzahl; ++f) {
        g.worldMatrices(f, welt);
        gelenk(welt, q);
        std::string zeile;
        for (const int b : ordnung) {
            const auto ub = static_cast<std::size_t>(b);
            const int p = g.bones[ub].parent;
            M3 lok = q[ub].d;
            std::array<double, 3> ort = q[ub].o;
            if (p >= 0) {
                const Starr& qp = q[static_cast<std::size_t>(p)];
                lok = mal(transp(qp.d), q[ub].d);
                std::array<double, 3> d{};
                for (std::size_t i = 0; i < 3; ++i) { d[i] = ort[i] - qp.o[i]; }
                ort = malV(transp(qp.d), d);
            }
            double rz = 0.0;
            double rx = 0.0;
            double ry = 0.0;
            zxy(lok, rz, rx, ry);
            if (sechs[ub] != 0) {
                std::snprintf(z, sizeof z, "%.4f %.4f %.4f %.4f %.4f %.4f ", ort[0], ort[1], ort[2], rz * kGrad, rx * kGrad, ry * kGrad);
            } else {
                std::snprintf(z, sizeof z, "%.4f %.4f %.4f ", rz * kGrad, rx * kGrad, ry * kGrad);
            }
            zeile += z;
        }
        zeile.back() = '\n';
        s += zeile;
    }
    aus = std::move(s);
    return true;
}

}  // namespace bhed::modell
