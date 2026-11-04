// bspmerge.cpp - rc609 B1: Brush-Entities in fertige Karten einfuegen, Stufe 1 (siehe bspmerge.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, third_party/q3map2/tools/quake3/q3map2):
//   convert_bsp.cpp MergeBSPMain   das Umrechnen der Indizes und *N (Abweichungen in bspmerge.h)
//   writebsp.cpp EmitShader        Wiederverwenden eines Shaders (Flags gleich, Name ohne Gross/klein)
//   writebsp.cpp EmitLeaf          das Blatt eines Untermodells (Kasten, leafbrushes, keine Flaechen)
//   bspfile_ibsp.cpp/_rbsp.cpp     die Strukturen (dmodel_t 40, dshader_t 72, dplane_t 16,
//                                  dleaf_t 48, dbrush_t 12, dbrushside_t 8 bzw. 12 Bytes)
// und aus OpenJK / Movie Duels 9 (code/ und codemp/qcommon/cm_load.cpp: CMod_LoadSubmodels,
// CMod_LoadBrushes, CMod_LoadBrushSides; cm_local.h; q_shared.h MAX_SUBMODELS; qfiles.h MAX_MAP_*;
// code/game/surfaceflags.h CONTENTS_SOLID).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/bspmerge.h"

#include "bhed/bsp.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

namespace {

// Lump-Nummern (bspfile_ibsp.cpp / bspfile_rbsp.cpp, fuer 0-16 gleich)
constexpr std::size_t kEnt = 0;
constexpr std::size_t kShader = 1;
constexpr std::size_t kEbenen = 2;
constexpr std::size_t kKnoten = 3;
constexpr std::size_t kBlaetter = 4;
constexpr std::size_t kBlattFlaechen = 5;
constexpr std::size_t kBlattPinsel = 6;
constexpr std::size_t kModelle = 7;
constexpr std::size_t kPinsel = 8;
constexpr std::size_t kSeiten = 9;
constexpr std::size_t kEcken = 10;
constexpr std::size_t kIndizes = 11;
constexpr std::size_t kNebel = 12;
constexpr std::size_t kFlaechen = 13;
constexpr std::size_t kLightmaps = 14;

constexpr std::uint32_t kContentsSolid = 0x00000001U;   // CONTENTS_SOLID (surfaceflags.h, JKA und JK2)
// Stufe 2 (surfaceflags.h): ein Clip hat PLAYERCLIP, MONSTERCLIP, BOTCLIP oder SHOTCLIP; in die Welt darf nichts, was
// sie fuer das Bild oder VIS aendert: SOLID, LAVA, WATER, FOG, TERRAIN, OPAQUE, SLIME.
constexpr std::uint32_t kContentsClips = 0x00000010U | 0x00000020U | 0x00000040U | 0x00000080U;
constexpr std::uint32_t kContentsWeltAendernd = 0x00000001U | 0x00000002U | 0x00000004U | 0x00000008U | 0x00001000U | 0x00008000U | 0x00020000U;

// qfiles.h (OJK code/ und codemp/, MD9 ebenso): Grenzen des Formats und der Werkzeuge. Die Engine
// prueft sie beim Laden nicht (cm_load.cpp liest jede Zahl) - darum nur Warnungen.
struct FormatGrenze {
    std::size_t lump;
    std::size_t hoechstens;
    const char* name;
};
constexpr std::array<FormatGrenze, 6> kFormatGrenzen{{
    {kShader, 0x400, "MAX_MAP_SHADERS"},
    {kEbenen, 0x20000, "MAX_MAP_PLANES"},
    {kBlaetter, 0x20000, "MAX_MAP_LEAFS"},
    {kBlattPinsel, 0x40000, "MAX_MAP_LEAFBRUSHES"},
    {kPinsel, 0x8000, "MAX_MAP_BRUSHES"},
    {kSeiten, 0x20000, "MAX_MAP_BRUSHSIDES"},
}};
constexpr std::size_t kMaxMapEntstring = 0x40000;   // MAX_MAP_ENTSTRING

std::int32_t i32(const std::string& b, std::size_t at) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(b[at + i])) << (8 * i); }
    return static_cast<std::int32_t>(v);
}
float f32(const std::string& b, std::size_t at) {
    const std::uint32_t v = static_cast<std::uint32_t>(i32(b, at));
    float f = 0.0F;
    std::memcpy(&f, &v, sizeof f);
    return f;
}
void dazu(std::string& b, std::int32_t v) {
    const auto u = static_cast<std::uint32_t>(v);
    for (std::size_t i = 0; i < 4; ++i) { b += static_cast<char>((u >> (8 * i)) & 0xFFU); }
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
// dshader_t.shader: char[MAX_QPATH], bis zur NUL
std::string shaderName(const std::string& lump, std::size_t i) {
    const std::string roh = lump.substr(i * 72, 64);
    return roh.substr(0, roh.find('\0'));
}

struct Teilpinsel {
    std::vector<Halbraum> h;
    float mins[3]{};
    float maxs[3]{};
    std::uint32_t inhalt = 0;
    int modell = 0;   // *k im Teil
};

// Die Halbraeume eines Brushes (Seiten -> Ebenen), verschoben um den Ursprung der Entity, und sein
// Kasten wie CM_BoundBrush (cm_load.cpp: die ersten sechs Seiten sind die achsparallelen).
bool pinselLesen(const BspDatei& d, std::size_t seitenGroesse, int nr, const double ursprung[3], Teilpinsel& p) {
    const std::string& br = d.lumps[kPinsel];
    const std::string& sd = d.lumps[kSeiten];
    const std::string& pl = d.lumps[kEbenen];
    const int erste = i32(br, static_cast<std::size_t>(nr) * 12);
    const int zahl = i32(br, static_cast<std::size_t>(nr) * 12 + 4);
    if (erste < 0 || zahl < 6 || static_cast<std::size_t>(erste) + static_cast<std::size_t>(zahl) > sd.size() / seitenGroesse) { return false; }
    p.h.clear();
    for (int s = 0; s < zahl; ++s) {
        const int e = i32(sd, static_cast<std::size_t>(erste + s) * seitenGroesse);
        if (e < 0 || static_cast<std::size_t>(e) >= pl.size() / 16) { return false; }
        Halbraum h;
        double nd = 0.0;
        for (int k = 0; k < 3; ++k) {
            h.n[k] = f32(pl, static_cast<std::size_t>(e) * 16 + static_cast<std::size_t>(k) * 4);
            nd += h.n[k] * ursprung[k];
        }
        h.d = f32(pl, static_cast<std::size_t>(e) * 16 + 12) + nd;
        p.h.push_back(h);
    }
    for (int k = 0; k < 3; ++k) {
        p.mins[k] = static_cast<float>(-p.h[static_cast<std::size_t>(2 * k)].d);
        p.maxs[k] = static_cast<float>(p.h[static_cast<std::size_t>(2 * k + 1)].d);
    }
    return true;
}

bool kaestenBeruehren(const Teilpinsel& a, const Teilpinsel& b) {
    for (int k = 0; k < 3; ++k) {
        if (a.mins[k] >= b.maxs[k] || b.mins[k] >= a.maxs[k]) { return false; }
    }
    return true;
}

}  // namespace

bool MergeErgebnis::hatFehler() const {
    return std::any_of(meldungen.begin(), meldungen.end(), [](const MergeMeldung& m) { return m.fehler; });
}

std::vector<ModellGrenze> modellGrenzen(MergeSpiel s) {
    switch (s) {
    case MergeSpiel::Jk2:
        // OpenJK baut JK2 aus code/ (JK2_MODE); fuer JK2 MP gibt es dort keine Quelle.
        return {{"JK2 SP", 511, "OpenJK code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"JK2 MP", 510, "assumed like JKA MP (no JK2 MP source in OpenJK)"}};
    case MergeSpiel::MovieDuels:
        return {{"Movie Duels SP", 511, "MD9 code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"Movie Duels MP", 510, "MD9 codemp/qcommon/cm_local.h CAPSULE_MODEL_HANDLE (MAX_SUBMODELS-2)"}};
    case MergeSpiel::Jka:
    default:
        return {{"JKA SP", 511, "OpenJK code/qcommon/cm_load.cpp CMod_LoadSubmodels, CM_ClipHandleToModel, cm_local.h BOX_MODEL_HANDLE"},
                {"JKA MP", 510, "OpenJK codemp/qcommon/cm_local.h CAPSULE_MODEL_HANDLE (MAX_SUBMODELS-2)"}};
    }
}

bool pinselUeberschneiden(const std::vector<Halbraum>& a, const std::vector<Halbraum>& b, double epsilon) {
    std::vector<Halbraum> h;
    h.reserve(a.size() + b.size());
    for (const auto* v : {&a, &b}) {
        for (Halbraum x : *v) {
            x.d -= epsilon;
            h.push_back(x);
        }
    }
    const auto kreuz = [](const double* p, const double* q, double* r) {
        r[0] = p[1] * q[2] - p[2] * q[1];
        r[1] = p[2] * q[0] - p[0] * q[2];
        r[2] = p[0] * q[1] - p[1] * q[0];
    };
    const std::size_t n = h.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            double jk[3];
            for (std::size_t k = j + 1; k < n; ++k) {
                kreuz(h[j].n, h[k].n, jk);
                const double det = h[i].n[0] * jk[0] + h[i].n[1] * jk[1] + h[i].n[2] * jk[2];
                if (std::fabs(det) < 1e-9) { continue; }
                double ki[3];
                double ij[3];
                kreuz(h[k].n, h[i].n, ki);
                kreuz(h[i].n, h[j].n, ij);
                double x[3];
                for (int c = 0; c < 3; ++c) { x[c] = (h[i].d * jk[c] + h[j].d * ki[c] + h[k].d * ij[c]) / det; }
                bool drin = true;
                for (std::size_t l = 0; drin && l < n; ++l) {
                    drin = h[l].n[0] * x[0] + h[l].n[1] * x[1] + h[l].n[2] * x[2] <= h[l].d + 1e-4;
                }
                if (drin) { return true; }
            }
        }
    }
    return false;
}

// --- Stufe 2: FilterBrushIntoTree_r mit SplitBrush (q3map2 brush.cpp) ---------------------------------
// In double wie Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES (q3map2.h:91): BaseWindingForPlaneAccu und
// ChopWindingInPlaceAccu (common/polylib.cpp), ClipWindingEpsilonStrict, BoundBrush, BrushVolume.
namespace {

using P3 = std::array<double, 3>;
using Wicklung = std::vector<P3>;
constexpr double kMaxWorldCoord = 65536.0;   // common/maxworld.h MAX_WORLD_COORD

struct SplitSeite {
    Halbraum e;
    Wicklung w;
};
using SplitPinsel = std::vector<SplitSeite>;

double abstand(const Halbraum& e, const P3& p) { return e.n[0] * p[0] + e.n[1] * p[1] + e.n[2] * p[2] - e.d; }
Halbraum gedreht(const Halbraum& e) { return {{-e.n[0], -e.n[1], -e.n[2]}, -e.d}; }

// BaseWindingForPlaneAccu( plane ) (polylib.cpp:178)
Wicklung grundWicklung(const Halbraum& e) {
    int x = -1;
    float max = 0.56F;
    for (int i = 0; i < 3; ++i) {
        const auto v = static_cast<float>(std::fabs(e.n[i]));
        if (v > max) {
            x = i;
            max = v;
        }
    }
    if (x < 0) { return {}; }
    P3 rechts{};
    if (x == 0 || x == 1) {
        rechts = {-e.n[1], e.n[0], 0.0};
    } else {
        rechts = {0.0, -e.n[2], e.n[1]};
    }
    for (double& c : rechts) { c *= kMaxWorldCoord * 4.0; }
    const P3 hoch{e.n[1] * rechts[2] - e.n[2] * rechts[1], e.n[2] * rechts[0] - e.n[0] * rechts[2], e.n[0] * rechts[1] - e.n[1] * rechts[0]};
    const P3 o{e.n[0] * e.d, e.n[1] * e.d, e.n[2] * e.d};
    Wicklung w(4);
    for (std::size_t i = 0; i < 3; ++i) {
        w[0][i] = o[i] - rechts[i] + hoch[i];
        w[1][i] = o[i] + rechts[i] + hoch[i];
        w[2][i] = o[i] + rechts[i] - hoch[i];
        w[3][i] = o[i] - rechts[i] - hoch[i];
    }
    return w;
}

P3 teilpunkt(const P3& p1, const P3& p2, double d1, double d2, const Halbraum& e) {
    const double t = d1 / (d1 - d2);
    P3 m{};
    for (std::size_t j = 0; j < 3; ++j) {
        if (e.n[j] == 1.0) {
            m[j] = e.d;
        } else if (e.n[j] == -1.0) {
            m[j] = -e.d;
        } else {
            m[j] = p1[j] + t * (p2[j] - p1[j]);
        }
    }
    return m;
}

// ChopWindingInPlaceAccu( w, plane, 0 ) (polylib.cpp:478): behaelt die Vorderseite
void abschneiden(Wicklung& w, const Halbraum& e) {
    constexpr double eps = 1.1920928955078125e-07 * 0.5;   // VEC_SMALLEST_EPSILON_AROUND_ONE * 0.5
    const std::size_t n = w.size();
    std::vector<double> d(n + 1);
    std::vector<int> s(n + 1);
    int vorn = 0;
    int hinten = 0;
    for (std::size_t i = 0; i < n; ++i) {
        d[i] = abstand(e, w[i]);
        s[i] = d[i] > eps ? 0 : (d[i] < -eps ? 1 : 2);
        vorn += s[i] == 0 ? 1 : 0;
        hinten += s[i] == 1 ? 1 : 0;
    }
    if (vorn == 0) {
        w.clear();
        return;
    }
    if (hinten == 0) { return; }
    s[n] = s[0];
    d[n] = d[0];
    Wicklung f;
    for (std::size_t i = 0; i < n; ++i) {
        if (s[i] == 2 || s[i] == 0) {
            f.push_back(w[i]);
            if (s[i] == 2) { continue; }
        }
        if (s[i + 1] == 2 || s[i + 1] == s[i]) { continue; }
        f.push_back(teilpunkt(w[i], w[(i + 1) % n], d[i], d[i + 1], e));
    }
    w.swap(f);
}

// FixWindingAccu (brush.cpp:217): Kanten unter DEGENERATE_EPSILON (0.1) zusammenlegen (SnapWeldVectorAccu)
void wicklungReparieren(Wicklung& w) {
    bool weiter = true;
    while (weiter && w.size() > 1) {
        weiter = false;
        for (std::size_t j = 0; j < w.size(); ++j) {
            const std::size_t i = j == 0 ? w.size() - 1 : j - 1;
            const double dx = w[i][0] - w[j][0];
            const double dy = w[i][1] - w[j][1];
            const double dz = w[i][2] - w[j][2];
            if (std::sqrt(dx * dx + dy * dy + dz * dz) < 0.1) {
                P3 m{};
                for (std::size_t c = 0; c < 3; ++c) {   // SnapWeldVectorAccu (brush.cpp:113), SNAP_EPSILON 0.01
                    const double ai = std::rint(w[i][c]);
                    const double bi = std::rint(w[j][c]);
                    const double ad = std::fabs(ai - w[i][c]);
                    const double bd = std::fabs(bi - w[j][c]);
                    if (ad < bd) {
                        m[c] = ad < 0.01 ? ai : w[i][c];
                    } else {
                        m[c] = bd < 0.01 ? bi : w[j][c];
                    }
                }
                w[i] = m;
                w.erase(w.begin() + static_cast<std::ptrdiff_t>(j));
                weiter = true;
                break;
            }
        }
    }
}

bool kastenGueltig(const SplitPinsel& b) {   // BoundBrush mit c_worldMinmax = +-MAX_WORLD_COORD (eigene Annahme)
    bool irgendwas = false;
    for (const SplitSeite& s : b) {
        for (const P3& p : s.w) {
            irgendwas = true;
            for (double c : p) {
                if (c <= -kMaxWorldCoord || c >= kMaxWorldCoord) { return false; }
            }
        }
    }
    return irgendwas;
}

// CreateBrushWindings (brush.cpp:318)
bool pinselWicklungen(const std::vector<Halbraum>& h, SplitPinsel& b) {
    b.clear();
    for (std::size_t i = 0; i < h.size(); ++i) {
        Wicklung w = grundWicklung(h[i]);
        for (std::size_t j = 0; j < h.size() && !w.empty(); ++j) {
            const bool gegen = h[j].n[0] == -h[i].n[0] && h[j].n[1] == -h[i].n[1] && h[j].n[2] == -h[i].n[2] && h[j].d == -h[i].d;
            if (i == j || gegen) { continue; }   // back side clipaway
            abschneiden(w, gedreht(h[j]));
        }
        wicklungReparieren(w);
        if (w.size() < 3) { w.clear(); }
        for (P3& p : w) {   // CopyWindingAccuToRegular: float
            for (double& c : p) { c = static_cast<double>(static_cast<float>(c)); }
        }
        b.push_back({h[i], std::move(w)});
    }
    return kastenGueltig(b);
}

double flaeche(const Wicklung& w) {   // WindingArea
    double a = 0.0;
    for (std::size_t i = 2; i < w.size(); ++i) {
        const P3 d1{w[i - 1][0] - w[0][0], w[i - 1][1] - w[0][1], w[i - 1][2] - w[0][2]};
        const P3 d2{w[i][0] - w[0][0], w[i][1] - w[0][1], w[i][2] - w[0][2]};
        const P3 k{d1[1] * d2[2] - d1[2] * d2[1], d1[2] * d2[0] - d1[0] * d2[2], d1[0] * d2[1] - d1[1] * d2[0]};
        a += 0.5 * std::sqrt(k[0] * k[0] + k[1] * k[1] + k[2] * k[2]);
    }
    return a;
}

double volumen(const SplitPinsel& b) {   // BrushVolume (brush.cpp:437)
    double v = 0.0;
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b[i].w.empty()) { continue; }
        const P3 ecke = b[i].w[0];
        for (++i; i < b.size(); ++i) {
            if (!b[i].w.empty()) { v += -abstand(b[i].e, ecke) * flaeche(b[i].w); }
        }
        break;
    }
    return v / 3.0;
}

// ClipWindingEpsilonStrict( w, plane, 0 ) (polylib.cpp:367)
std::pair<Wicklung, Wicklung> zerteilen(const Wicklung& w, const Halbraum& e) {
    const std::size_t n = w.size();
    std::vector<double> d(n + 1);
    std::vector<int> s(n + 1);
    int vorn = 0;
    int hinten = 0;
    for (std::size_t i = 0; i < n; ++i) {
        d[i] = static_cast<float>(abstand(e, w[i]));
        s[i] = d[i] > 0.0 ? 0 : (d[i] < 0.0 ? 1 : 2);
        vorn += s[i] == 0 ? 1 : 0;
        hinten += s[i] == 1 ? 1 : 0;
    }
    if (vorn == 0 && hinten == 0) { return {}; }
    if (vorn == 0) { return {{}, w}; }
    if (hinten == 0) { return {w, {}}; }
    s[n] = s[0];
    d[n] = d[0];
    Wicklung f;
    Wicklung b;
    for (std::size_t i = 0; i < n; ++i) {
        if (s[i] == 2) {
            f.push_back(w[i]);
            b.push_back(w[i]);
            continue;
        }
        (s[i] == 0 ? f : b).push_back(w[i]);
        if (s[i + 1] == 2 || s[i + 1] == s[i]) { continue; }
        P3 m = teilpunkt(w[i], w[(i + 1) % n], d[i], d[i + 1], e);
        for (double& c : m) { c = static_cast<double>(static_cast<float>(c)); }
        f.push_back(m);
        b.push_back(m);
    }
    return {f, b};
}

// SplitBrush (brush.cpp:682): {vorn, hinten}, leer = nichts auf der Seite
std::pair<SplitPinsel, SplitPinsel> pinselTeilen(const SplitPinsel& b, const Halbraum& e) {
    float dv = 0.0F;
    float dh = 0.0F;
    for (const SplitSeite& s : b) {
        for (const P3& p : s.w) {
            const auto d = static_cast<float>(abstand(e, p));
            if (d > 0.0F) { dv = std::max(dv, d); }
            if (d < 0.0F) { dh = std::min(dh, d); }
        }
    }
    if (dv < 0.1F) { return {{}, b}; }   // PLANESIDE_EPSILON: nur hinten
    if (dh > -0.1F) { return {b, {}}; }  // nur vorn
    Wicklung mitte = grundWicklung(e);
    for (const SplitSeite& s : b) {
        abschneiden(mitte, gedreht(s.e));
        if (mitte.empty()) { break; }
    }
    if (mitte.empty()) {   // BrushMostlyOnSide
        double max = 0.0;
        bool hintenMehr = false;
        for (const SplitSeite& s : b) {
            for (const P3& p : s.w) {
                const double d = abstand(e, p);
                if (d > max) {
                    max = d;
                    hintenMehr = false;
                }
                if (-d > max) {
                    max = -d;
                    hintenMehr = true;
                }
            }
        }
        return hintenMehr ? std::pair<SplitPinsel, SplitPinsel>{{}, b} : std::pair<SplitPinsel, SplitPinsel>{b, {}};
    }
    SplitPinsel t[2];
    for (const SplitSeite& s : b) {
        if (s.w.empty()) { continue; }
        auto [v, h] = zerteilen(s.w, e);
        if (!v.empty()) { t[0].push_back({s.e, std::move(v)}); }
        if (!h.empty()) { t[1].push_back({s.e, std::move(h)}); }
    }
    for (SplitPinsel& x : t) {
        if (x.size() < 3 || !kastenGueltig(x)) { x.clear(); }
    }
    if (t[0].empty() || t[1].empty()) {
        if (!t[0].empty()) { return {b, {}}; }
        if (!t[1].empty()) { return {{}, b}; }
        return {};
    }
    t[0].push_back({gedreht(e), mitte});   // planenum ^ 0 ^ 1: die Vorderhaelfte endet an der umgedrehten Ebene
    t[1].push_back({e, mitte});
    for (SplitPinsel& x : t) {
        if (volumen(x) <= 0.0) { x.clear(); }   // g_microVolume = 0 (q3map2.h, PR 316 P1)
    }
    return {std::move(t[0]), std::move(t[1])};
}

struct Baum {
    const std::string* knoten = nullptr;   // dnode_t 36: planeNum, children[2], mins[3], maxs[3]
    const std::string* ebenen = nullptr;
};

void insBaum(const Baum& t, SplitPinsel&& b, int knoten, std::vector<int>& blaetter) {
    if (b.empty()) { return; }
    if (knoten < 0) {
        blaetter.push_back(-1 - knoten);
        return;
    }
    const std::size_t at = static_cast<std::size_t>(knoten) * 36;
    if (at + 36 > t.knoten->size()) { return; }
    const int e = i32(*t.knoten, at);
    if (e < 0 || static_cast<std::size_t>(e) * 16 + 16 > t.ebenen->size()) { return; }
    Halbraum h;
    for (std::size_t k = 0; k < 3; ++k) { h.n[k] = f32(*t.ebenen, static_cast<std::size_t>(e) * 16 + k * 4); }
    h.d = f32(*t.ebenen, static_cast<std::size_t>(e) * 16 + 12);
    auto [vorn, hinten] = pinselTeilen(b, h);
    insBaum(t, std::move(vorn), i32(*t.knoten, at + 4), blaetter);
    insBaum(t, std::move(hinten), i32(*t.knoten, at + 8), blaetter);
}

std::vector<int> blaetterImBaum(const BspDatei& d, const std::vector<Halbraum>& h) {
    std::vector<int> blaetter;
    SplitPinsel b;
    if (d.lumps[kKnoten].empty() || !pinselWicklungen(h, b)) { return blaetter; }
    const Baum t{&d.lumps[kKnoten], &d.lumps[kEbenen]};
    insBaum(t, std::move(b), 0, blaetter);
    return blaetter;
}

}  // namespace

std::vector<int> pinselBlaetter(const std::string& bsp, const std::vector<Halbraum>& halbraeume) {
    BspDatei d;
    if (!bspLesen(bsp, d) || d.lumps.size() <= kKnoten) { return {}; }
    return blaetterImBaum(d, halbraeume);
}

MergeErgebnis bspModelleAnhaengen(const std::string& original, const std::string& teil, MergeSpiel spiel, bool beleuchtet) {
    MergeErgebnis r;
    const auto melde = [&r](bool fehler, const char* pruefung, const std::string& text) { r.meldungen.push_back({fehler, pruefung, text}); };
    BspDatei a;
    BspDatei b;
    if (!bspLesen(original, a)) {
        melde(true, "format", "The compiled map is not an IBSP/RBSP file");
        return r;
    }
    if (!bspLesen(teil, b)) {
        melde(true, "format", "The part compile is not an IBSP/RBSP file");
        return r;
    }
    if (a.ident != b.ident || a.version != b.version) {
        melde(true, "format",
              "Different BSP formats: " + a.ident + " " + std::to_string(a.version) + " and " + b.ident + " " + std::to_string(b.version));
        return r;
    }
    const bool rbsp = a.ident == "RBSP";
    const std::size_t seitenGroesse = rbsp ? 12 : 8;            // dbrushside_t (RBSP: + drawSurfNum)
    const std::size_t flaechenGroesse = rbsp ? 148 : 104;       // dsurface_t
    const std::size_t eckenGroesse = rbsp ? 80 : 44;            // drawVert_t (RBSP: 4 Lightmap-Koordinaten und Farben)
    const std::size_t lightmapZahl = rbsp ? 4 : 1;              // MAXLIGHTMAPS je Flaeche
    const std::size_t lightmapAn = rbsp ? 36 : 28;              // dsurface_t.lightmapNum
    // "funny lump size" (cm_load.cpp CMod_Load*, tr_bsp.cpp R_Load*): jede Laenge ein Vielfaches der Struktur
    const std::array<std::pair<std::size_t, std::size_t>, 13> groessen{{{kShader, 72},
                                                                        {kEbenen, 16},
                                                                        {kKnoten, 36},
                                                                        {kBlaetter, 48},
                                                                        {kBlattFlaechen, 4},
                                                                        {kBlattPinsel, 4},
                                                                        {kModelle, 40},
                                                                        {kPinsel, 12},
                                                                        {kSeiten, seitenGroesse},
                                                                        {kEcken, eckenGroesse},
                                                                        {kIndizes, 4},
                                                                        {kNebel, 72},
                                                                        {kFlaechen, flaechenGroesse}}};
    for (const BspDatei* d : {&a, &b}) {
        for (const auto& [nr, gr] : groessen) {
            if (d->lumps[nr].size() % gr != 0) {
                melde(true, "format", std::string(d == &a ? "Compiled map" : "Part compile") + ": funny lump size in lump " + std::to_string(nr));
                return r;
            }
        }
    }
    const auto zahl = [](const BspDatei& d, std::size_t nr, std::size_t gr) { return d.lumps[nr].size() / gr; };
    const std::size_t modelleA = zahl(a, kModelle, 40);
    const std::size_t modelleT = zahl(b, kModelle, 40);
    if (modelleA < 1 || modelleT < 1) {
        melde(true, "format", "Map with no models");   // CMod_LoadSubmodels
        return r;
    }
    const std::string& mdT = b.lumps[kModelle];
    const std::string& mdA = a.lumps[kModelle];

    // --- Entities des Teils: welche Untermodelle neu sind, welches in die Welt geht (Marken der Teilkarte) ---
    std::vector<MapEntity> entT;
    parseEntities(bspEntitaetenText(b), entT);
    if (entT.empty() || entT[0].classname != "worldspawn") {
        melde(true, "format", "The part compile has no worldspawn");
        return r;
    }
    const auto modellVon = [modelleT](const MapEntity& e) {
        const std::string* m = e.find("model");
        if (m == nullptr || m->empty() || (*m)[0] != '*') { return 0; }
        const int k = std::atoi(m->c_str() + 1);
        return k > 0 && static_cast<std::size_t>(k) < modelleT ? k : 0;
    };
    enum Rolle { kKeine, kNeu, kWelt };
    std::vector<Rolle> rolle(modelleT, kKeine);
    std::vector<std::array<double, 3>> ursprung(modelleT, std::array<double, 3>{0.0, 0.0, 0.0});
    for (std::size_t i = 1; i < entT.size(); ++i) {
        const MapEntity& e = entT[i];
        const int k = modellVon(e);
        if (k == 0) { continue; }
        if (e.find(kTeilWelt) != nullptr) {
            rolle[static_cast<std::size_t>(k)] = kWelt;
        } else if (e.find(kTeilNeu) != nullptr) {
            rolle[static_cast<std::size_t>(k)] = kNeu;
        }
        const std::string* o = e.find("origin");   // q3map2 setzt ihn bei origin-Brushes
        std::array<double, 3>& u = ursprung[static_cast<std::size_t>(k)];
        if (o != nullptr && std::sscanf(o->c_str(), "%lf %lf %lf", &u[0], &u[1], &u[2]) != 3) { u = {0.0, 0.0, 0.0}; }   // vectorForKey
    }
    if (modelleT < 2) { melde(true, "format", "The part compile has no brush entities"); }
    std::size_t neueModelle = 0;
    std::size_t weltModell = 0;
    for (std::size_t m = 1; m < modelleT; ++m) {
        const int flaechen = i32(mdT, m * 40 + 28);
        if (rolle[m] == kKeine) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part belongs to no new entity");
        } else if (rolle[m] == kWelt) {
            if (weltModell != 0) { melde(true, "format", "The part has more than one model for the world"); }
            weltModell = m;
            if (flaechen != 0) {
                melde(true, "welt", "The new world brushes have " + std::to_string(flaechen) +
                                        " visible surfaces - only clip brushes (playerclip, monsterclip, botclip, shotclip) go into the world");
            }
        } else {
            ++neueModelle;
            if (flaechen != 0 && !beleuchtet) {
                melde(true, "sichtbar", "Model *" + std::to_string(m) + " of the part has " + std::to_string(flaechen) +
                                            " visible surfaces - visible brush entities are lit with the world (stage 3)");
            }
        }
    }
    // Clips in der Welt: der Inhalt jedes Brushes (dbrush_t.shaderNum -> contentFlags, EmitBrushes: contentShader)
    // muss ein Clip sein und darf die Welt fuer Bild und VIS nicht aendern (surfaceflags.h).
    if (weltModell != 0) {
        const int erster = i32(mdT, weltModell * 40 + 32);
        const int anzahl = i32(mdT, weltModell * 40 + 36);
        for (int i = 0; i < anzahl && erster >= 0; ++i) {
            const std::size_t at = static_cast<std::size_t>(erster + i) * 12;
            if (at + 12 > b.lumps[kPinsel].size()) { break; }
            const int sh = i32(b.lumps[kPinsel], at + 8);
            const std::uint32_t c = sh >= 0 && static_cast<std::size_t>(sh) < zahl(b, kShader, 72)
                                        ? static_cast<std::uint32_t>(i32(b.lumps[kShader], static_cast<std::size_t>(sh) * 72 + 68))
                                        : 0U;
            if ((c & kContentsClips) == 0U || (c & kContentsWeltAendernd) != 0U) {
                char z[256];
                std::snprintf(z, sizeof z, "New world brush %d has contents 0x%08x - only clip brushes go into the world, no solid, water, fog or opaque ones",
                              i + 1, c);
                melde(true, "welt", z);
            }
        }
    }
    const std::size_t gesamt = modelleA + neueModelle;
    for (const ModellGrenze& g : modellGrenzen(spiel)) {
        if (gesamt > static_cast<std::size_t>(g.hoechstens)) {
            melde(true, "grenze", std::string(g.name) + ": " + std::to_string(gesamt) + " models, at most " + std::to_string(g.hoechstens) + " (" +
                                      g.quelle + ")");
        }
    }
    if (r.hatFehler()) { return r; }

    // --- Shader (EmitShader: gleiche Flags und Name ohne Gross/klein -> wiederverwenden) ----------
    std::string shOut = a.lumps[kShader];
    std::vector<int> shaderNeu(zahl(b, kShader, 72), -1);
    bool kaputt = false;
    const auto shaderFuer = [&](int t) {
        if (t < 0 || static_cast<std::size_t>(t) >= shaderNeu.size()) {
            kaputt = true;
            return 0;
        }
        int& z = shaderNeu[static_cast<std::size_t>(t)];
        if (z >= 0) { return z; }
        const std::string& sb = b.lumps[kShader];
        const std::string name = klein(shaderName(sb, static_cast<std::size_t>(t)));
        const std::int32_t sf = i32(sb, static_cast<std::size_t>(t) * 72 + 64);
        const std::int32_t cf = i32(sb, static_cast<std::size_t>(t) * 72 + 68);
        for (std::size_t i = 0; i < shOut.size() / 72; ++i) {
            if (i32(shOut, i * 72 + 64) == sf && i32(shOut, i * 72 + 68) == cf && klein(shaderName(shOut, i)) == name) {
                z = static_cast<int>(i);
                return z;
            }
        }
        z = static_cast<int>(shOut.size() / 72);
        shOut += sb.substr(static_cast<std::size_t>(t) * 72, 72);
        return z;
    };
    // --- Ebenen: das Paar (n & ~1, n | 1); Byte-gleiches Paar an gerader Stelle wiederverwenden -----
    std::string plOut = a.lumps[kEbenen];
    std::map<std::string, int> paareA;
    for (std::size_t i = 0; i + 1 < plOut.size() / 16; i += 2) { paareA.emplace(plOut.substr(i * 16, 32), static_cast<int>(i)); }
    std::map<int, int> paarNeu;
    const auto ebeneFuer = [&](int t) {
        const std::size_t zahlT = b.lumps[kEbenen].size() / 16;
        if (t < 0 || static_cast<std::size_t>(t) >= zahlT) {
            kaputt = true;
            return 0;
        }
        const int paar = t & ~1;
        auto it = paarNeu.find(paar);
        if (it == paarNeu.end()) {
            const std::size_t laenge = static_cast<std::size_t>(paar) + 1 < zahlT ? 32 : 16;
            const std::string bytes = b.lumps[kEbenen].substr(static_cast<std::size_t>(paar) * 16, laenge);
            const auto vorhanden = laenge == 32 ? paareA.find(bytes) : paareA.end();
            int basis = 0;
            if (vorhanden != paareA.end()) {
                basis = vorhanden->second;
            } else {
                basis = static_cast<int>(plOut.size() / 16);
                plOut += bytes;
            }
            it = paarNeu.emplace(paar, basis).first;
        }
        return it->second + (t & 1);
    };

    std::string brOut = a.lumps[kPinsel];
    std::string sdOut = a.lumps[kSeiten];
    std::string mdOut = mdA;
    std::string lfOut = a.lumps[kBlaetter];
    std::string lbOut = a.lumps[kBlattPinsel];
    std::string lsOut = a.lumps[kBlattFlaechen];
    std::string vtOut = a.lumps[kEcken];
    std::string ixOut = a.lumps[kIndizes];
    std::string fgOut = a.lumps[kNebel];
    std::string sfOut = a.lumps[kFlaechen];
    std::string lmOut = a.lumps[kLightmaps];
    const std::string& brT = b.lumps[kPinsel];
    const std::string& sdT = b.lumps[kSeiten];
    const std::string& lfT = b.lumps[kBlaetter];
    const std::string& lbT = b.lumps[kBlattPinsel];
    const std::string& lsT = b.lumps[kBlattFlaechen];
    const std::string& sfT = b.lumps[kFlaechen];
    const auto setze = [](std::string& s, std::size_t at, std::int32_t v) {
        const auto u = static_cast<std::uint32_t>(v);
        for (std::size_t i = 0; i < 4; ++i) { s[at + i] = static_cast<char>((u >> (8 * i)) & 0xFFU); }
    };
    // Ein Brush des Teils mit seinen Seiten anhaengen (Seiten -> Ebenen, Shader; RBSP drawSurfNum ueber flaecheNeu wie
    // FixBrushSides, ohne Flaeche bleibt der Wert, den EmitBrushes hinterlaesst - das Spiel liest ihn nicht,
    // CMod_LoadBrushSides). Liefert den dbrush_t (12 Bytes).
    const auto pinselUebernehmen = [&](int nr, const std::map<int, int>& flaecheNeu, std::string& bOut) {
        const std::size_t at = static_cast<std::size_t>(nr) * 12;
        const int ersteSeite = i32(brT, at);
        const int seiten = i32(brT, at + 4);
        if (ersteSeite < 0 || seiten < 0 || static_cast<std::size_t>(ersteSeite) + static_cast<std::size_t>(seiten) > sdT.size() / seitenGroesse) {
            kaputt = true;
            return;
        }
        const auto seiteNeu = static_cast<std::int32_t>(sdOut.size() / seitenGroesse);
        for (int s = 0; s < seiten; ++s) {
            const std::size_t q = static_cast<std::size_t>(ersteSeite + s) * seitenGroesse;
            dazu(sdOut, ebeneFuer(i32(sdT, q)));
            dazu(sdOut, shaderFuer(i32(sdT, q + 4)));
            if (rbsp) {
                const auto f = flaecheNeu.find(i32(sdT, q + 8));
                dazu(sdOut, f != flaecheNeu.end() ? f->second : i32(sdT, q + 8));
            }
        }
        dazu(bOut, seiteNeu);
        dazu(bOut, seiten);
        dazu(bOut, shaderFuer(i32(brT, at + 8)));
    };
    const double null3[3] = {0.0, 0.0, 0.0};

    // --- Stufe 2: Clips in die Welt (MergeBSPMain -world, convert_bsp.cpp) --------------------------
    // Die Brushes kommen ans Ende der Welt-Brushes (Modell 0); alle spaeteren Brush-Nummern (leafbrushes, Nebel,
    // firstBrush der Untermodelle) ruecken nach. Jeder Clip kommt vorn in die leafbrushes jedes Welt-Blatts, in das
    // q3map2 ihn legen wuerde (FilterBrushIntoTree_r statt des Kastentests von -world); das Spiel findet Welt-Brushes
    // nur so (CM_TraceThroughLeaf / CM_PointContents ueber cm.leafbrushes, cm_trace.cpp, cm_test.cpp).
    if (weltModell != 0) {
        const int erster = i32(mdT, weltModell * 40 + 32);
        const int anzahl = i32(mdT, weltModell * 40 + 36);
        if (erster < 0 || anzahl < 0 || static_cast<std::size_t>(erster) + static_cast<std::size_t>(anzahl) > brT.size() / 12) {
            melde(true, "format", "World model of the part: brushes out of range");
            return r;
        }
        std::string neu;
        std::vector<std::vector<Halbraum>> halbraeume;
        for (int i = 0; i < anzahl; ++i) {
            pinselUebernehmen(erster + i, {}, neu);
            Teilpinsel p;
            halbraeume.push_back(pinselLesen(b, seitenGroesse, erster + i, null3, p) ? p.h : std::vector<Halbraum>{});
        }
        if (kaputt) {
            melde(true, "format", "World model of the part: plane, shader or side out of range");
            return r;
        }
        const int pos = i32(mdA, 32) + i32(mdA, 36);
        brOut.insert(static_cast<std::size_t>(pos) * 12, neu);
        for (std::size_t i = 0; i < lbOut.size() / 4; ++i) {
            if (i32(lbOut, i * 4) >= pos) { setze(lbOut, i * 4, i32(lbOut, i * 4) + anzahl); }
        }
        for (std::size_t i = 0; i < fgOut.size() / 72; ++i) {
            if (i32(fgOut, i * 72 + 64) >= pos) { setze(fgOut, i * 72 + 64, i32(fgOut, i * 72 + 64) + anzahl); }
        }
        for (std::size_t m = 1; m < modelleA; ++m) {
            if (i32(mdOut, m * 40 + 32) >= pos) { setze(mdOut, m * 40 + 32, i32(mdOut, m * 40 + 32) + anzahl); }
        }
        setze(mdOut, 36, i32(mdOut, 36) + anzahl);
        const std::size_t blaetterA = lfOut.size() / 48;
        for (int i = 0; i < anzahl; ++i) {
            const std::vector<int> blaetter = blaetterImBaum(a, halbraeume[static_cast<std::size_t>(i)]);
            if (blaetter.empty()) { melde(false, "welt", "New world clip " + std::to_string(i + 1) + " lies in no leaf of the BSP tree"); }
            for (const int l : blaetter) {
                if (l < 0 || static_cast<std::size_t>(l) >= blaetterA) { continue; }
                const std::size_t at = static_cast<std::size_t>(l) * 48;
                const int vorn = i32(lfOut, at + 40);
                for (std::size_t x = 0; x < blaetterA; ++x) {
                    if (x != static_cast<std::size_t>(l) && i32(lfOut, x * 48 + 40) >= vorn) { setze(lfOut, x * 48 + 40, i32(lfOut, x * 48 + 40) + 1); }
                }
                std::string idx;
                dazu(idx, pos + i);
                lbOut.insert(static_cast<std::size_t>(vorn) * 4, idx);
                setze(lfOut, at + 44, i32(lfOut, at + 44) + 1);
                ++r.weltBlaetter;
            }
        }
        r.weltPinsel = anzahl;
    }

    // --- Stufe 3: Lightmaps und Nebel der neuen Flaechen ---------------------------------------------
    // Seiten zu 128 x 128 x 3 Bytes (LIGHTMAP_SIZE, R_LoadLightmaps tr_bsp.cpp); angehaengt werden nur die Seiten, die
    // die neuen Flaechen benutzen (der Teil hat auch die neu beleuchtete Welt), neue Nummer = Seiten der Karte + n.
    constexpr std::size_t kSeite = 128 * 128 * 3;
    const std::size_t seitenA = a.lumps[kLightmaps].size() / kSeite;
    const std::size_t seitenT = b.lumps[kLightmaps].size() / kSeite;
    int hoechsteA = -1;   // MergeBSPMain: "deduce max lm index" (native externe Lightmaps lm_%04d.tga, R_FindLightmap)
    for (std::size_t s = 0; s < zahl(a, kFlaechen, flaechenGroesse); ++s) {
        for (std::size_t k = 0; k < lightmapZahl; ++k) { hoechsteA = std::max(hoechsteA, i32(a.lumps[kFlaechen], s * flaechenGroesse + lightmapAn + k * 4)); }
    }
    std::map<int, int> seiteNeu;
    std::vector<int> seitenReihe;
    bool lightmapFehlt = false;
    const auto lightmapFuer = [&](int t) {
        if (t < 0) { return t; }   // LIGHTMAP_BY_VERTEX, LIGHTMAP_NONE ...
        if (static_cast<std::size_t>(t) >= seitenT) {
            lightmapFehlt = true;
            return t;
        }
        auto it = seiteNeu.find(t);
        if (it == seiteNeu.end()) {
            it = seiteNeu.emplace(t, static_cast<int>(seitenA + seitenReihe.size())).first;
            seitenReihe.push_back(t);
        }
        return it->second;
    };
    std::map<int, int> nebelNeu;
    const auto nebelFuer = [&](int t) {
        if (t < 0) { return -1; }
        const auto it = nebelNeu.find(t);
        if (it != nebelNeu.end()) { return it->second; }
        int z = -1;
        if (static_cast<std::size_t>(t) < zahl(b, kNebel, 72)) {
            // derselbe Nebel der Karte: gleicher Shader (ohne Gross/klein) und gleicher Kasten des Nebel-Brushes
            const std::string name = klein(shaderName(b.lumps[kNebel], static_cast<std::size_t>(t)));
            Teilpinsel pt;
            const bool mitPinsel = pinselLesen(b, seitenGroesse, i32(b.lumps[kNebel], static_cast<std::size_t>(t) * 72 + 64), null3, pt);
            for (std::size_t i = 0; mitPinsel && z < 0 && i < zahl(a, kNebel, 72); ++i) {
                Teilpinsel pa;
                if (klein(shaderName(a.lumps[kNebel], i)) != name || !pinselLesen(a, seitenGroesse, i32(a.lumps[kNebel], i * 72 + 64), null3, pa)) { continue; }
                bool gleich = true;
                for (int k = 0; k < 3; ++k) { gleich = gleich && std::fabs(pa.mins[k] - pt.mins[k]) < 0.01F && std::fabs(pa.maxs[k] - pt.maxs[k]) < 0.01F; }
                if (gleich) { z = static_cast<int>(i); }
            }
            if (z < 0) { melde(false, "nebel", "Fog " + shaderName(b.lumps[kNebel], static_cast<std::size_t>(t)) + " of the part is not in the compiled map - the surfaces get no fog"); }
        }
        nebelNeu.emplace(t, z);
        return z;
    };

    // --- Untermodelle: Flaechen, Brushes, Seiten, Blaetter ----------------------------------------------
    std::vector<Teilpinsel> neuePinsel;
    std::vector<int> nummer(modelleT, 0);   // *k des Teils -> *N der Karte
    int naechste = static_cast<int>(modelleA);
    for (std::size_t m = 1; m < modelleT; ++m) {
        if (rolle[m] != kNeu) { continue; }
        nummer[m] = naechste++;
        const int ersterT = i32(mdT, m * 40 + 32);
        const int zahlT = i32(mdT, m * 40 + 36);
        const int ersteFlT = i32(mdT, m * 40 + 24);
        const int flT = i32(mdT, m * 40 + 28);
        if (ersterT < 0 || zahlT < 0 || static_cast<std::size_t>(ersterT) + static_cast<std::size_t>(zahlT) > brT.size() / 12 || ersteFlT < 0 || flT < 0 ||
            static_cast<std::size_t>(ersteFlT) + static_cast<std::size_t>(flT) > sfT.size() / flaechenGroesse) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part: brushes or surfaces out of range");
            return r;
        }
        // Flaechen (MergeBSPMain: shaderNum, fogNum, firstVert, firstIndex, lightmapNum umrechnen)
        const auto ersteFlNeu = static_cast<std::int32_t>(sfOut.size() / flaechenGroesse);
        std::map<int, int> flaecheNeu;
        for (int s = ersteFlT; s < ersteFlT + flT; ++s) {
            std::string f = sfT.substr(static_cast<std::size_t>(s) * flaechenGroesse, flaechenGroesse);
            const int ev = i32(f, 12);
            const int nv = i32(f, 16);
            const int ei = i32(f, 20);
            const int ni = i32(f, 24);
            if (ev < 0 || nv < 0 || ei < 0 || ni < 0 || static_cast<std::size_t>(ev) + static_cast<std::size_t>(nv) > b.lumps[kEcken].size() / eckenGroesse ||
                static_cast<std::size_t>(ei) + static_cast<std::size_t>(ni) > b.lumps[kIndizes].size() / 4) {
                melde(true, "format", "Model *" + std::to_string(m) + " of the part: vertexes or indexes out of range");
                return r;
            }
            setze(f, 0, shaderFuer(i32(f, 0)));
            setze(f, 4, nebelFuer(i32(f, 4)));
            setze(f, 12, static_cast<std::int32_t>(vtOut.size() / eckenGroesse));
            setze(f, 20, static_cast<std::int32_t>(ixOut.size() / 4));
            for (std::size_t k = 0; k < lightmapZahl; ++k) { setze(f, lightmapAn + k * 4, lightmapFuer(i32(f, lightmapAn + k * 4))); }
            vtOut += b.lumps[kEcken].substr(static_cast<std::size_t>(ev) * eckenGroesse, static_cast<std::size_t>(nv) * eckenGroesse);
            ixOut += b.lumps[kIndizes].substr(static_cast<std::size_t>(ei) * 4, static_cast<std::size_t>(ni) * 4);
            flaecheNeu.emplace(s, static_cast<int>(sfOut.size() / flaechenGroesse));
            sfOut += f;
        }
        r.neueFlaechen += flT;
        const auto ersterNeu = static_cast<std::int32_t>(brOut.size() / 12);
        for (int i = 0; i < zahlT; ++i) {
            pinselUebernehmen(ersterT + i, flaecheNeu, brOut);
            Teilpinsel p;
            p.modell = static_cast<int>(m);
            const int shT = i32(brT, static_cast<std::size_t>(ersterT + i) * 12 + 8);
            p.inhalt = shT >= 0 && static_cast<std::size_t>(shT) < shaderNeu.size()
                           ? static_cast<std::uint32_t>(i32(b.lumps[kShader], static_cast<std::size_t>(shT) * 72 + 68))
                           : 0U;
            if (pinselLesen(b, seitenGroesse, ersterT + i, ursprung[m].data(), p)) { neuePinsel.push_back(std::move(p)); }
        }
        if (kaputt) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part: plane, shader or side out of range");
            return r;
        }
        // dmodel_t: mins, maxs (wie gebaut), firstSurface (bspDrawSurfaces.size() beim Bau), numSurfaces, brushes
        mdOut += mdT.substr(m * 40, 24);
        dazu(mdOut, ersteFlNeu);
        dazu(mdOut, flT);
        dazu(mdOut, ersterNeu);
        dazu(mdOut, zahlT);
        // Das Blatt des Modells (EmitLeaf): im Teil das Blatt mit genau diesen leafbrushes
        std::multiset<int> soll;
        for (int i = 0; i < zahlT; ++i) { soll.insert(ersterT + i); }
        std::size_t blatt = lfT.size() / 48;
        for (std::size_t l = 0; l < lfT.size() / 48 && blatt == lfT.size() / 48; ++l) {
            const int erste = i32(lfT, l * 48 + 40);
            const int anzahl = i32(lfT, l * 48 + 44);
            if (anzahl != zahlT || erste < 0 || static_cast<std::size_t>(erste) + static_cast<std::size_t>(anzahl) > lbT.size() / 4) { continue; }
            std::multiset<int> ist;
            for (int i = 0; i < anzahl; ++i) { ist.insert(i32(lbT, static_cast<std::size_t>(erste + i) * 4)); }
            if (ist == soll) { blatt = l; }
        }
        if (blatt == lfT.size() / 48) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part has no leaf (q3map2 EmitLeaf)");
            return r;
        }
        lfOut += lfT.substr(blatt * 48, 32);   // cluster, area, mins, maxs wie gebaut
        // firstLeafSurface, numLeafSurfaces: die Flaechen des Modells (EmitLeaf: drawSurfReferences)
        const int ersteBf = i32(lfT, blatt * 48 + 32);
        const int bf = i32(lfT, blatt * 48 + 36);
        if (ersteBf < 0 || bf < 0 || static_cast<std::size_t>(ersteBf) + static_cast<std::size_t>(bf) > lsT.size() / 4) {
            melde(true, "format", "Model *" + std::to_string(m) + " of the part: leaf surfaces out of range");
            return r;
        }
        dazu(lfOut, static_cast<std::int32_t>(lsOut.size() / 4));
        dazu(lfOut, bf);
        for (int i = 0; i < bf; ++i) {
            const auto f = flaecheNeu.find(i32(lsT, static_cast<std::size_t>(ersteBf + i) * 4));
            if (f == flaecheNeu.end()) {
                melde(true, "format", "Model *" + std::to_string(m) + " of the part: leaf surface of another model");
                return r;
            }
            dazu(lsOut, f->second);
        }
        dazu(lfOut, static_cast<std::int32_t>(lbOut.size() / 4));
        dazu(lfOut, zahlT);
        const int erste = i32(lfT, blatt * 48 + 40);
        for (int i = 0; i < zahlT; ++i) { dazu(lbOut, ersterNeu + (i32(lbT, static_cast<std::size_t>(erste + i) * 4) - ersterT)); }
    }

    // --- Lightmaps anhaengen und pruefen -------------------------------------------------------------
    if (!seitenReihe.empty()) {
        if (a.lumps[kLightmaps].size() % kSeite != 0) {
            melde(true, "lightmap", "The lightmap lump of the compiled map is not a whole number of 128 x 128 pages");
        }
        if (hoechsteA >= static_cast<int>(seitenA)) {
            melde(true, "lightmap", "The compiled map uses external lightmaps (lightmap " + std::to_string(hoechsteA) + ", " + std::to_string(seitenA) +
                                        " in the BSP: maps/<name>/lm_####.tga, R_FindLightmap) - not supported");
        }
        for (const int t : seitenReihe) { lmOut += b.lumps[kLightmaps].substr(static_cast<std::size_t>(t) * kSeite, kSeite); }
    }
    if (lightmapFehlt) { melde(true, "lightmap", "The part uses lightmaps that are not in its BSP (external lightmaps) - not supported"); }
    r.neueLightmapSeiten = static_cast<int>(seitenReihe.size());
    // Grenzen des Spiels fuer das Bild (Fehler): tr.lightmaps[MAX_LIGHTMAPS] (tr_local.h, R_LoadLightmaps prueft die Zahl
    // nicht), MAX_MAP_DRAW_SURFS / _VERTS / _INDEXES (qfiles.h)
    struct BildGrenze {
        std::size_t vorher;
        std::size_t nachher;
        std::size_t hoechstens;
        const char* name;
        const char* quelle;
    };
    const std::array<BildGrenze, 4> bildGrenzen{{
        {seitenA, lmOut.size() / kSeite, 256, "MAX_LIGHTMAPS", "rd-vanilla/tr_local.h tr.lightmaps"},
        {zahl(a, kFlaechen, flaechenGroesse), sfOut.size() / flaechenGroesse, 0x20000, "MAX_MAP_DRAW_SURFS", "qfiles.h"},
        {zahl(a, kEcken, eckenGroesse), vtOut.size() / eckenGroesse, 0x80000, "MAX_MAP_DRAW_VERTS", "qfiles.h"},
        {zahl(a, kIndizes, 4), ixOut.size() / 4, 0x80000, "MAX_MAP_DRAW_INDEXES", "qfiles.h"},
    }};
    for (const BildGrenze& g : bildGrenzen) {
        if (g.nachher > g.vorher && g.nachher > g.hoechstens) {
            melde(true, "grenze", std::string(g.name) + " exceeded: " + std::to_string(g.nachher) + " > " + std::to_string(g.hoechstens) + " (" + g.quelle + ")");
        }
    }
    if (r.hatFehler()) { return r; }

    // --- Entity-Lump: die neuen Entities des Teils (Marke kTeilNeu, ohne sie) mit *N ------------------
    std::string entNeu;
    std::vector<MapEntity> entA;
    const std::string textA = bspEntitaetenText(a);
    parseEntities(textA, entA);
    std::set<std::string> namen;
    for (const auto* liste : {&entA, &entT}) {
        for (const MapEntity& e : *liste) {
            if (!e.targetname.empty()) { namen.insert(klein(e.targetname)); }   // G_Find: Q_stricmp
        }
    }
    for (std::size_t i = 1; i < entT.size(); ++i) {
        const MapEntity& e = entT[i];
        if (e.find(kTeilNeu) == nullptr || e.find(kTeilWelt) != nullptr) { continue; }
        const int k = modellVon(e);
        entNeu += "{\n";
        for (const auto& [s, v] : e.keys) {
            if (s == kTeilNeu) { continue; }
            const std::string wert = klein(s) == "model" && k > 0 ? "*" + std::to_string(nummer[static_cast<std::size_t>(k)]) : v;   // MergeBSPMain
            entNeu += "\"" + s + "\" \"" + wert + "\"\n";
            if ((klein(s) == "target" || klein(s) == "target2") && namen.count(klein(v)) == 0U) {
                melde(false, "ziel", "New " + e.classname + ": " + s + " \"" + v + "\" matches no targetname in the map");
            }
        }
        entNeu += "}\n";
        r.klassen.push_back(e.classname);
    }
    const std::string entOut = textA + entNeu + '\0';

    // --- Ueberschneidung mit soliden Welt-Brushes --------------------------------------------------
    {
        const int ersterW = i32(mdA, 32);
        const int zahlW = i32(mdA, 36);
        std::vector<Teilpinsel> welt;
        for (int i = 0; i < zahlW && ersterW >= 0; ++i) {
            const int nr = ersterW + i;
            if (static_cast<std::size_t>(nr) >= a.lumps[kPinsel].size() / 12) { break; }
            const int sh = i32(a.lumps[kPinsel], static_cast<std::size_t>(nr) * 12 + 8);
            if (sh < 0 || static_cast<std::size_t>(sh) >= a.lumps[kShader].size() / 72) { continue; }
            if ((static_cast<std::uint32_t>(i32(a.lumps[kShader], static_cast<std::size_t>(sh) * 72 + 68)) & kContentsSolid) == 0U) { continue; }
            Teilpinsel w;
            if (pinselLesen(a, seitenGroesse, nr, null3, w)) { welt.push_back(std::move(w)); }
        }
        std::map<int, int> treffer;   // Modell -> Brushes mit Ueberschneidung
        for (const Teilpinsel& p : neuePinsel) {
            if ((p.inhalt & kContentsSolid) == 0U) { continue; }   // Trigger duerfen in die Wand reichen
            for (const Teilpinsel& w : welt) {
                if (kaestenBeruehren(p, w) && pinselUeberschneiden(p.h, w.h)) {
                    ++treffer[p.modell];
                    break;
                }
            }
        }
        for (const auto& [m, n] : treffer) {
            melde(false, "ueberschneidung", "New *" + std::to_string(nummer[static_cast<std::size_t>(m)]) + ": " + std::to_string(n) +
                                                " solid brush(es) overlap solid world brushes");
        }
    }

    // --- Grenzen des Formats (Warnungen) ------------------------------------------------------------
    const std::map<std::size_t, const std::string*> neu{{kShader, &shOut}, {kEbenen, &plOut}, {kBlaetter, &lfOut},
                                                         {kBlattPinsel, &lbOut}, {kPinsel, &brOut}, {kSeiten, &sdOut}};
    const std::map<std::size_t, std::size_t> strukt{{kShader, 72}, {kEbenen, 16}, {kBlaetter, 48}, {kBlattPinsel, 4}, {kPinsel, 12}, {kSeiten, seitenGroesse}};
    for (const FormatGrenze& g : kFormatGrenzen) {
        const std::size_t n = neu.at(g.lump)->size() / strukt.at(g.lump);
        if (n > g.hoechstens) {
            melde(false, "grenze", std::string(g.name) + " exceeded: " + std::to_string(n) + " > " + std::to_string(g.hoechstens) + " (qfiles.h)");
        }
    }
    if (entOut.size() > kMaxMapEntstring) {
        melde(false, "grenze", "MAX_MAP_ENTSTRING exceeded: " + std::to_string(entOut.size()) + " > " + std::to_string(kMaxMapEntstring) + " (qfiles.h)");
    }

    // Nur die geaenderten Lumps neu; Baum, VIS, Lichtgitter und die Welt-Flaechen bleiben Byte fuer Byte.
    std::map<std::size_t, std::string> lumps{{kEnt, entOut},     {kShader, shOut}, {kEbenen, plOut}, {kBlaetter, lfOut},
                                             {kBlattPinsel, lbOut}, {kModelle, mdOut}, {kPinsel, brOut}, {kSeiten, sdOut}};
    for (const auto& [nr, inhalt] : std::initializer_list<std::pair<std::size_t, const std::string*>>{
             {kBlattFlaechen, &lsOut}, {kEcken, &vtOut}, {kIndizes, &ixOut}, {kNebel, &fgOut}, {kFlaechen, &sfOut}, {kLightmaps, &lmOut}}) {
        if (*inhalt != a.lumps[nr]) { lumps.emplace(nr, *inhalt); }
    }
    r.bsp = bspMitLumps(original, lumps);
    r.ersteNummer = static_cast<int>(modelleA);
    r.neueModelle = static_cast<int>(neueModelle);
    return r;
}

// --- Karten-Editor ------------------------------------------------------------------------------

namespace {
// FNV-1a ueber die Zeilen einer Entity in der .map (ohne Kommentare wie "// entity N"); mitSchluesseln:
// auch die Zeilen "key" "value" (sonst nur die Primitive, wie geometrieFingerabdruck).
std::uint64_t abdruck(const Karte& k, const Entitaet& e, bool mitSchluesseln) {
    std::uint64_t h = 1469598103934665603ULL;
    const std::string text = karteSchreiben(k, [&e](const Entitaet& x) { return &x == &e; }, [](const Entitaet&, const Primitiv&) { return true; });
    std::size_t a = 0;
    while (a < text.size()) {
        std::size_t b = text.find('\n', a);
        if (b == std::string::npos) { b = text.size(); }
        if (b > a && (mitSchluesseln || text[a] != '"') && text.compare(a, 2, "//") != 0) {
            for (std::size_t i = a; i < b; ++i) {
                h ^= static_cast<unsigned char>(text[i]);
                h *= 1099511628211ULL;
            }
            h ^= 0xFFU;
            h *= 1099511628211ULL;
        }
        a = b + 1;
    }
    return h;
}
std::uint64_t textAbdruck(const std::string& t) {   // FNV-1a
    std::uint64_t h = 1469598103934665603ULL;
    for (const char c : t) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}
bool zaehlt(const Karte& k, const Entitaet& e) {
    return !e.kinder().empty() && &e != k.welt().get() && !eingebackeneEntitaet(e.klassenname());
}
bool punkt(const Karte& k, const Entitaet& e) {
    return e.kinder().empty() && &e != k.welt().get() && !eingebackeneEntitaet(e.klassenname());
}
}  // namespace

std::uint64_t pinselAbdruck(const Karte& k, const Entitaet& e) { return abdruck(k, e, false); }
std::uint64_t entitaetAbdruck(const Karte& k, const Entitaet& e) { return abdruck(k, e, true); }

std::vector<std::uint64_t> punktAbdruecke(const Karte& k) {
    std::vector<std::uint64_t> v;
    for (const auto& e : k.entitaeten()) {
        if (punkt(k, *e)) { v.push_back(entitaetAbdruck(k, *e)); }
    }
    return v;
}

std::vector<const Entitaet*> neuePunktEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher, int* geaendert) {
    std::multiset<std::uint64_t> frei(vorher.begin(), vorher.end());
    std::vector<const Entitaet*> neu;
    for (const auto& e : k.entitaeten()) {
        if (!punkt(k, *e)) { continue; }
        const auto it = frei.find(entitaetAbdruck(k, *e));
        if (it != frei.end()) {
            frei.erase(it);
        } else {
            neu.push_back(e.get());
        }
    }
    if (geaendert != nullptr) { *geaendert = static_cast<int>(frei.size()); }
    return neu;
}

std::vector<std::uint64_t> pinselAbdruecke(const Karte& k) {
    std::vector<std::uint64_t> v;
    for (const auto& e : k.entitaeten()) {
        if (zaehlt(k, *e)) { v.push_back(pinselAbdruck(k, *e)); }
    }
    return v;
}

std::vector<const Entitaet*> neuePinselEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher) {
    std::multiset<std::uint64_t> frei(vorher.begin(), vorher.end());
    std::vector<const Entitaet*> neu;
    for (const auto& e : k.entitaeten()) {
        if (!zaehlt(k, *e)) { continue; }
        const auto it = frei.find(pinselAbdruck(k, *e));
        if (it != frei.end()) {
            frei.erase(it);
        } else {
            neu.push_back(e.get());
        }
    }
    return neu;
}

std::vector<std::uint64_t> weltAbdruecke(const Karte& k) {
    std::vector<std::uint64_t> v;
    const auto welt = k.welt();
    if (!welt) { return v; }
    for (const auto& p : welt->kinder()) { v.push_back(textAbdruck(primitivText(*p, k.kontext))); }
    return v;
}

std::vector<const Primitiv*> neueWeltPrimitive(const Karte& k, const std::vector<std::uint64_t>& vorher, int* fehlend) {
    std::multiset<std::uint64_t> frei(vorher.begin(), vorher.end());
    std::vector<const Primitiv*> neu;
    if (const auto welt = k.welt()) {
        for (const auto& p : welt->kinder()) {
            const auto it = frei.find(textAbdruck(primitivText(*p, k.kontext)));
            if (it != frei.end()) {
                frei.erase(it);
            } else {
                neu.push_back(p.get());
            }
        }
    }
    if (fehlend != nullptr) { *fehlend = static_cast<int>(frei.size()); }
    return neu;
}

namespace {
std::string schluesselText(const Entitaet& e) {
    std::string s;
    for (const auto& [k, v] : e.schluessel()) { s += "\"" + k + "\" \"" + v + "\"\n"; }
    return s;
}
// Der Platzhalter der Teil-Welt: ein Brush mit textures/system/caulk statt seiner Shader - ohne strukturellen, soliden
// Brush hat die Welt keinen Baum (q3map2 bricht mit "Backwards tree volume" ab). Der Shader steht in jeder Seitenzeile als
// erstes Wort nach der letzten ")" (Quake und Valve 220: nach dem dritten Punkt, brushDef: nach der Texturmatrix).
std::string mitCaulk(const std::string& pinsel) {
    std::string aus;
    std::size_t a = 0;
    while (a < pinsel.size()) {
        std::size_t z = pinsel.find('\n', a);
        if (z == std::string::npos) { z = pinsel.size(); }
        std::string zeile = pinsel.substr(a, z - a);
        const std::size_t klammer = zeile.rfind(')');
        if (!zeile.empty() && zeile[0] == '(' && klammer != std::string::npos) {
            const std::size_t w0 = zeile.find_first_not_of(' ', klammer + 1);
            const std::size_t w1 = w0 == std::string::npos ? w0 : zeile.find(' ', w0);
            if (w0 != std::string::npos) { zeile.replace(w0, (w1 == std::string::npos ? zeile.size() : w1) - w0, "system/caulk"); }
        }
        aus += zeile + "\n";
        a = z + 1;
    }
    return aus;
}
}  // namespace

std::string teilKarte(const Karte& k, const std::vector<const Entitaet*>& neu, const std::vector<const Primitiv*>& neueWelt, bool mitWelt) {
    const auto weltPtr = k.welt();
    const Entitaet* welt = weltPtr.get();
    std::string text = "// entity 0\n{\n" + (welt != nullptr ? schluesselText(*welt) : std::string("\"classname\" \"worldspawn\"\n"));
    if (mitWelt && welt != nullptr) {
        for (const auto& p : welt->kinder()) {
            if (std::find(neueWelt.begin(), neueWelt.end(), p.get()) == neueWelt.end()) { text += primitivText(*p, k.kontext); }
        }
    } else {
        // Eine Kopie, weil q3map2 das Pinselformat mit dem ersten Brush der Datei festlegt (ParseMapEntity, map.cpp:
        // g_brushType) - Quake, Valve 220 und brushDef duerfen nicht gemischt werden.
        const Primitiv* erster = nullptr;
        for (const Entitaet* e : neu) {
            for (const auto& p : e->kinder()) {
                if (erster == nullptr && p->art() == Primitiv::Art::Pinsel) { erster = p.get(); }
            }
        }
        for (const Primitiv* p : neueWelt) {
            if (erster == nullptr && p->art() == Primitiv::Art::Pinsel) { erster = p; }
        }
        if (erster != nullptr) { text += mitCaulk(primitivText(*erster, k.kontext)); }
    }
    text += "}\n";
    int nr = 1;
    for (const Entitaet* e : neu) {
        text += "// entity " + std::to_string(nr++) + "\n{\n" + schluesselText(*e) + "\"" + kTeilNeu + "\" \"1\"\n";
        for (const auto& p : e->kinder()) { text += primitivText(*p, k.kontext); }
        text += "}\n";
    }
    if (!neueWelt.empty()) {
        // Die neuen Welt-Brushes als eigenes Untermodell (q3map2 baut es wie jede Brush-Entity, ohne Baum und Flood-Fill);
        // bspModelleAnhaengen legt seine Brushes in die Welt.
        text += "// entity " + std::to_string(nr++) + "\n{\n\"classname\" \"func_static\"\n\"" + kTeilWelt + "\" \"1\"\n";
        for (const Primitiv* p : neueWelt) { text += primitivText(*p, k.kontext); }
        text += "}\n";
    }
    if (mitWelt) {
        // Schattenwerfer und Licht wie beim Bau der Karte: die eingebackenen Entities (misc_model ..., UnparseEntities),
        // die Lichter (q3map2 -light liest sie aus der .map, light.cpp LightMain: LoadMapFile) und die Ziele der Spots
        // (CreateEntityLights: "target"). Sie tragen keine Marke und kommen nicht ins Entity-Lump der Karte.
        std::set<std::string> ziele;
        for (const auto& e : k.entitaeten()) {
            if (klein(e->klassenname()).rfind("light", 0) == 0 && !e->wert("target").empty()) { ziele.insert(e->wert("target")); }
        }
        for (const auto& e : k.entitaeten()) {
            if (e.get() == welt || std::find(neu.begin(), neu.end(), e.get()) != neu.end()) { continue; }
            const bool licht = e->kinder().empty() && klein(e->klassenname()).rfind("light", 0) == 0;
            const bool ziel = e->kinder().empty() && !e->wert("targetname").empty() && ziele.count(e->wert("targetname")) != 0U;
            if (!eingebackeneEntitaet(e->klassenname()) && !licht && !ziel) { continue; }
            text += "// entity " + std::to_string(nr++) + "\n{\n" + entitaetText(*e, k.kontext) + "}\n";
        }
    }
    return text;
}

}  // namespace bhed::karte
