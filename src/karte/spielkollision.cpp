// spielkollision.cpp - Karten-Editor, Stufe 8b Punkt 3 ("Im Editor spielen"):
// die Kollision direkt aus den Brushes und Patches der Karte (siehe
// spielkollision.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, tools/quake3/q3map2):
// games.cpp (game_sof2, game_ja), shaders.cpp (ApplySurfaceParm), map.cpp
// (SetBrushContents, AddBrushBevels, SnapNormal, SnapPlaneImproved,
// PlaneEqual mit Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX/-PLANE_FIX und
// HIGH_PRECISION_MATH_FIXES: normalEpsilon 0.00005, distanceEpsilon 0.005);
// CM_LoadMap (OpenJK code/qcommon/cm_load.cpp) fuer den Aufbau.
//
// Abweichung vom Bau (ehrlich): q3map2 legt jede Ebene ueber FindFloatPlane
// in eine Tabelle und nimmt eine schon vorhandene, wenn sie innerhalb der
// Epsilons gleich ist - die Ebene eines Brushes kann darum die eines frueheren
// sein (Unterschied unter 0,005 Einheiten bzw. 0,00005 in der Normalen). Hier
// wird jede Ebene fuer sich eingerastet. Die Wicklungen kommen aus dem Editor
// (double) statt aus CreateBrushWindings (float).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/spielkollision.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <utility>

namespace bhed::karte {

namespace {

// --- games.cpp: game_sof2 (JK2, JA) -------------------------------------------------
constexpr std::uint32_t S_CONT_SOLID = 0x00000001;
constexpr std::uint32_t S_CONT_LAVA = 0x00000002;
constexpr std::uint32_t S_CONT_WATER = 0x00000004;
constexpr std::uint32_t S_CONT_FOG = 0x00000008;
constexpr std::uint32_t S_CONT_PLAYERCLIP = 0x00000010;
constexpr std::uint32_t S_CONT_MONSTERCLIP = 0x00000020;
constexpr std::uint32_t S_CONT_BOTCLIP = 0x00000040;
constexpr std::uint32_t S_CONT_SHOTCLIP = 0x00000080;
constexpr std::uint32_t S_CONT_TRIGGER = 0x00000400;
constexpr std::uint32_t S_CONT_NODROP = 0x00000800;
constexpr std::uint32_t S_CONT_TERRAIN = 0x00001000;
constexpr std::uint32_t S_CONT_LADDER = 0x00002000;
constexpr std::uint32_t S_CONT_ABSEIL = 0x00004000;
constexpr std::uint32_t S_CONT_OPAQUE = 0x00008000;
constexpr std::uint32_t S_CONT_OUTSIDE = 0x00010000;
constexpr std::uint32_t S_CONT_SLIME = 0x00020000;
constexpr std::uint32_t S_CONT_DETAIL = 0x08000000;
constexpr std::uint32_t S_CONT_TRANSLUCENT = 0x80000000;
constexpr std::uint32_t S_SURF_SKY = 0x00002000;
constexpr std::uint32_t S_SURF_SLICK = 0x00004000;
constexpr std::uint32_t S_SURF_METALSTEPS = 0x00008000;
constexpr std::uint32_t S_SURF_FORCEFIELD = 0x00010000;
constexpr std::uint32_t S_SURF_NODAMAGE = 0x00040000;
constexpr std::uint32_t S_SURF_NOIMPACT = 0x00080000;
constexpr std::uint32_t S_SURF_NOMARKS = 0x00100000;
constexpr std::uint32_t S_SURF_NODRAW = 0x00200000;
constexpr std::uint32_t S_SURF_NOSTEPS = 0x00400000;
constexpr std::uint32_t S_SURF_NODLIGHT = 0x00800000;
constexpr std::uint32_t S_SURF_NOMISCENTS = 0x01000000;
constexpr std::uint32_t S_SURF_BEVELSMASK = S_SURF_SLICK | S_SURF_METALSTEPS | S_SURF_NODAMAGE | S_SURF_NOSTEPS | S_SURF_NOMISCENTS;
constexpr std::uint32_t JA_CONT_INSIDE = 0x10000000;
constexpr std::uint32_t JA_SURF_FORCESIGHT = 0x02000000;
// games.h: compiler flags
constexpr std::uint32_t C_SOLID = 0x00000001;
constexpr std::uint32_t C_TRANSLUCENT = 0x00000002;
constexpr std::uint32_t C_STRUCTURAL = 0x00000004;
constexpr std::uint32_t C_HINT = 0x00000008;
constexpr std::uint32_t C_NODRAW = 0x00000010;
constexpr std::uint32_t C_LIGHTGRID = 0x00000020;
constexpr std::uint32_t C_ALPHASHADOW = 0x00000040;
constexpr std::uint32_t C_LIGHTFILTER = 0x00000080;
constexpr std::uint32_t C_VERTEXLIT = 0x00000100;
constexpr std::uint32_t C_LIQUID = 0x00000200;
constexpr std::uint32_t C_FOG = 0x00000400;
constexpr std::uint32_t C_SKY = 0x00000800;
constexpr std::uint32_t C_ORIGIN = 0x00001000;
constexpr std::uint32_t C_AREAPORTAL = 0x00002000;
constexpr std::uint32_t C_ANTIPORTAL = 0x00004000;
constexpr std::uint32_t C_SKIP = 0x00008000;
constexpr std::uint32_t C_NOMARKS = 0x00010000;
constexpr std::uint32_t C_DETAIL = 0x08000000;
constexpr std::uint32_t kAlle = 0xFFFFFFFFU;   // -1

struct SurfaceParm {   // surfaceParm_t
    const char* name;
    std::uint32_t contentFlags, contentFlagsClear;
    std::uint32_t surfaceFlags, surfaceFlagsClear;
    std::uint32_t compileFlags, compileFlagsClear;
};

const SurfaceParm kSof2[] = {
    {"default", S_CONT_SOLID | S_CONT_OPAQUE, kAlle, 0, kAlle, C_SOLID, kAlle},
    {"lightgrid", 0, 0, 0, 0, C_LIGHTGRID, 0},
    {"antiportal", 0, 0, 0, 0, C_ANTIPORTAL, 0},
    {"skip", 0, 0, 0, 0, C_SKIP, 0},
    {"origin", 0, S_CONT_SOLID, 0, 0, C_ORIGIN | C_TRANSLUCENT, C_SOLID},
    {"areaportal", S_CONT_TRANSLUCENT, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_AREAPORTAL | C_TRANSLUCENT, C_SOLID},
    {"trans", S_CONT_TRANSLUCENT, 0, 0, 0, C_TRANSLUCENT, 0},
    {"detail", S_CONT_DETAIL, 0, 0, 0, C_DETAIL, 0},
    {"structural", 0, 0, 0, 0, C_STRUCTURAL, 0},
    {"hint", 0, 0, 0, 0, C_HINT, 0},
    {"nodraw", 0, 0, S_SURF_NODRAW, 0, C_NODRAW, 0},
    {"alphashadow", 0, 0, 0, 0, C_ALPHASHADOW | C_TRANSLUCENT, 0},
    {"lightfilter", 0, 0, 0, 0, C_LIGHTFILTER | C_TRANSLUCENT, 0},
    {"nolightmap", 0, 0, 0, 0, C_VERTEXLIT, 0},
    {"pointlight", 0, 0, 0, 0, C_VERTEXLIT, 0},
    {"nonsolid", 0, S_CONT_SOLID, 0, 0, 0, C_SOLID},
    {"nonopaque", 0, S_CONT_OPAQUE, 0, 0, C_TRANSLUCENT, 0},
    {"trigger", S_CONT_TRIGGER, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_TRANSLUCENT, C_SOLID},
    {"water", S_CONT_WATER, S_CONT_SOLID, 0, 0, C_LIQUID | C_TRANSLUCENT, C_SOLID},
    {"slime", S_CONT_SLIME, S_CONT_SOLID, 0, 0, C_LIQUID | C_TRANSLUCENT, C_SOLID},
    {"lava", S_CONT_LAVA, S_CONT_SOLID, 0, 0, C_LIQUID | C_TRANSLUCENT, C_SOLID},
    {"shotclip", S_CONT_SHOTCLIP, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"playerclip", S_CONT_PLAYERCLIP, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"monsterclip", S_CONT_MONSTERCLIP, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"nodrop", S_CONT_NODROP, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"terrain", S_CONT_TERRAIN, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"ladder", S_CONT_LADDER, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"abseil", S_CONT_ABSEIL, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"outside", S_CONT_OUTSIDE, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"botclip", S_CONT_BOTCLIP, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"fog", S_CONT_FOG, S_CONT_SOLID | S_CONT_OPAQUE, 0, 0, C_FOG | C_DETAIL | C_TRANSLUCENT, C_SOLID},
    {"sky", 0, 0, S_SURF_SKY, 0, C_SKY, 0},
    {"slick", 0, 0, S_SURF_SLICK, 0, 0, 0},
    {"noimpact", 0, 0, S_SURF_NOIMPACT, 0, 0, 0},
    {"nomarks", 0, 0, S_SURF_NOMARKS, 0, C_NOMARKS, 0},
    {"nodamage", 0, 0, S_SURF_NODAMAGE, 0, 0, 0},
    {"metalsteps", 0, 0, S_SURF_METALSTEPS, 0, 0, 0},
    {"nosteps", 0, 0, S_SURF_NOSTEPS, 0, 0, 0},
    {"nodlight", 0, 0, S_SURF_NODLIGHT, 0, 0, 0},
    {"nomiscents", 0, 0, S_SURF_NOMISCENTS, 0, 0, 0},
    {"forcefield", 0, 0, S_SURF_FORCEFIELD, 0, 0, 0},
};
// game_ja: surfaceParms.insert( end, { inside, forcesight } )
const SurfaceParm kJa[] = {
    {"inside", JA_CONT_INSIDE, 0, 0, 0, 0, 0},
    {"forcesight", 0, 0, JA_SURF_FORCESIGHT, 0, 0, 0},
};

// ApplySurfaceParm (shaders.cpp:230-252)
bool surfaceParmAnwenden(const std::string& name, bool ja, Q3ShaderFlags& f) {
    const auto anwenden = [&f](const SurfaceParm& sp) {
        f.inhalt &= ~sp.contentFlagsClear;
        f.inhalt |= sp.contentFlags;
        f.flaeche &= ~sp.surfaceFlagsClear;
        f.flaeche |= sp.surfaceFlags;
        f.compile &= ~sp.compileFlagsClear;
        f.compile |= sp.compileFlags;
    };
    for (const SurfaceParm& sp : kSof2) {
        if (name == sp.name) {   // striEqual: die Namen kommen schon klein
            anwenden(sp);
            return true;
        }
    }
    if (ja) {
        for (const SurfaceParm& sp : kJa) {
            if (name == sp.name) {
                anwenden(sp);
                return true;
            }
        }
    }
    return false;
}

// --- map.cpp: Ebenen ----------------------------------------------------------------
constexpr double kNormalEpsilon = 0.00005;   // Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX
constexpr double kDistanceEpsilon = 0.005;   // Q3MAP2_EXPERIMENTAL_HIGH_PRECISION_MATH_FIXES

struct Ebene {   // Plane3f
    float n[3] = {0, 0, 0};
    float d = 0;
};
using V3f = std::array<float, 3>;

// VectorNormalize (qmath.h, HIGH_PRECISION: in double)
double normieren(float v[3]) {
    const double l = std::sqrt(static_cast<double>(v[0]) * v[0] + static_cast<double>(v[1]) * v[1] + static_cast<double>(v[2]) * v[2]);
    if (l == 0.0) { return 0.0; }
    for (int i = 0; i < 3; ++i) { v[i] = static_cast<float>(v[i] / l); }
    return l;
}

// SnapNormal (map.cpp, Q3MAP2_EXPERIMENTAL_SNAP_NORMAL_FIX)
bool normaleEinrasten(float n[3]) {
    bool adjusted = false;
    if ((n[0] != 0 || n[1] != 0) && std::fabs(n[0]) < 0.00025F && std::fabs(n[1]) < 0.00025F) {
        n[0] = n[1] = 0;
        adjusted = true;
    } else if ((n[0] != 0 || n[2] != 0) && std::fabs(n[0]) < 0.00025F && std::fabs(n[2]) < 0.00025F) {
        n[0] = n[2] = 0;
        adjusted = true;
    } else if ((n[2] != 0 || n[1] != 0) && std::fabs(n[2]) < 0.00025F && std::fabs(n[1]) < 0.00025F) {
        n[2] = n[1] = 0;
        adjusted = true;
    }
    for (int i = 0; i < 3; ++i) {
        if (n[i] != 0 && -kNormalEpsilon < n[i] && n[i] < kNormalEpsilon) {
            n[i] = 0;
            adjusted = true;
        }
    }
    if (adjusted) { (void)normieren(n); }
    return adjusted;
}

bool aufAchse(const float n[3]) {   // VectorIsOnAxis
    int null = 0;
    for (int i = 0; i < 3; ++i) { null += n[i] == 0 ? 1 : 0; }
    return null > 1;
}

// SnapPlaneImproved (map.cpp) mit den Punkten, aus denen die Ebene stammt
void ebeneEinrasten(Ebene& e, const Vektor3d* punkte, std::size_t zahl) {
    if (normaleEinrasten(e.n) && zahl > 0) {
        double c[3] = {0, 0, 0};
        for (std::size_t i = 0; i < zahl; ++i) {
            c[0] += punkte[i][0];
            c[1] += punkte[i][1];
            c[2] += punkte[i][2];
        }
        for (double& v : c) { v /= static_cast<double>(zahl); }
        e.d = static_cast<float>(e.n[0] * c[0] + e.n[1] * c[1] + e.n[2] * c[2]);   // vector3_dot( normal, center )
    }
    if (aufAchse(e.n)) {
        const float rund = std::rint(e.d);
        if (-kDistanceEpsilon < e.d - rund && e.d - rund < kDistanceEpsilon) { e.d = rund; }
    }
}

// PlaneEqual (map.cpp:60-79)
bool ebenenGleich(const Ebene& p, const Ebene& e) {
    const auto nah = [](float a, float b, double eps) { return a == b || std::fabs(a - b) < eps; };
    return nah(p.d, e.d, kDistanceEpsilon) && nah(p.n[0], e.n[0], kNormalEpsilon) && nah(p.n[1], e.n[1], kNormalEpsilon) &&
           nah(p.n[2], e.n[2], kNormalEpsilon);
}

struct Seite {   // side_t
    Ebene ebene;
    std::vector<V3f> wicklung;
    int shader = 0;
    Q3ShaderFlags flags;
    bool bevel = false;
};

// AddBrushBevels (map.cpp:558-743)
int bevelsDazu(std::vector<Seite>& sides, const float mins[3], const float maxs[3]) {
    int dazu = 0;
    std::size_t order = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        for (int dir = -1; dir <= 1; dir += 2, ++order) {
            std::size_t i = 0;
            for (; i < sides.size(); ++i) {
                if ((dir > 0 && sides[i].ebene.n[axis] == 1) || (dir < 0 && sides[i].ebene.n[axis] == -1)) { break; }
            }
            if (i == sides.size()) {
                Seite s;
                Ebene plane;
                plane.n[axis] = static_cast<float>(dir);
                plane.d = dir == 1 ? maxs[axis] : -mins[axis];   // bevelSnap 0
                ebeneEinrasten(plane, nullptr, 0);   // FindFloatPlane( plane )
                s.ebene = plane;
                s.shader = sides[0].shader;
                s.flags.inhalt = sides[0].flags.inhalt;
                for (const Seite& side : sides) {   // "handle bevel surfaceflags"
                    for (const V3f& point : side.wicklung) {
                        if (std::fabs(plane.d - point[axis]) < .1F) {
                            s.flags.flaeche |= (side.flags.flaeche & S_SURF_BEVELSMASK);
                            break;
                        }
                    }
                }
                s.bevel = true;
                sides.push_back(std::move(s));
                ++dazu;
            }
            if (i != order) { std::swap(sides[order], sides[i]); }
        }
    }
    if (sides.size() == 6) { return dazu; }   // "pure axial"
    for (std::size_t i = 6; i < sides.size(); ++i) {
        for (std::size_t j = 0; j < sides[i].wicklung.size(); ++j) {
            const V3f a = sides[i].wicklung[j];
            const V3f b = sides[i].wicklung[(j + 1) % sides[i].wicklung.size()];   // winding_next_point
            float vec[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
            if (normieren(vec) < 0.5) { continue; }
            (void)normaleEinrasten(vec);
            if (vec[0] == -1 || vec[0] == 1 || (vec[0] == 0 && vec[1] == 0) || vec[1] == -1 || vec[1] == 1 || (vec[1] == 0 && vec[2] == 0) ||
                vec[2] == -1 || vec[2] == 1 || (vec[2] == 0 && vec[0] == 0)) {
                continue;   // "axial, only test non-axial edges"
            }
            for (int axis = 0; axis < 3; ++axis) {
                for (int dir = -1; dir <= 1; dir += 2) {
                    float vec2[3] = {0, 0, 0};
                    vec2[axis] = static_cast<float>(dir);
                    Ebene plane;
                    plane.n[0] = vec[1] * vec2[2] - vec[2] * vec2[1];   // vector3_cross( vec, vec2 )
                    plane.n[1] = vec[2] * vec2[0] - vec[0] * vec2[2];
                    plane.n[2] = vec[0] * vec2[1] - vec[1] * vec2[0];
                    if (normieren(plane.n) < 0.5) { continue; }
                    const V3f& wj = sides[i].wicklung[j];
                    plane.d = wj[0] * plane.n[0] + wj[1] * plane.n[1] + wj[2] * plane.n[2];
                    std::size_t k = 0;
                    for (; k < sides.size(); ++k) {
                        if (ebenenGleich(sides[k].ebene, plane)) {
                            if (sides[k].bevel) { sides[k].flags.flaeche |= (sides[i].flags.flaeche & S_SURF_BEVELSMASK); }
                            break;
                        }
                        const std::vector<V3f>& w2 = sides[k].wicklung;
                        if (w2.empty()) { continue; }
                        float minBack = 0;
                        bool vorn = false;
                        for (const V3f& p : w2) {
                            const float d = plane.n[0] * p[0] + plane.n[1] * p[1] + plane.n[2] * p[2] - plane.d;   // plane3_distance_to_point
                            if (d > 0.1F) {
                                vorn = true;
                                break;
                            }
                            if (d < minBack) { minBack = d; }
                        }
                        if (vorn) { break; }
                        if (minBack > -0.1F) { break; }   // "the winding is on the bevel plane"
                    }
                    if (k != sides.size()) { continue; }   // "wasn't part of the outer hull"
                    Seite s2;
                    const Vektor3d punkt(wj[0], wj[1], wj[2]);
                    ebeneEinrasten(plane, &punkt, 1);   // FindFloatPlane( plane, Span( &w[j], 1 ) )
                    s2.ebene = plane;
                    s2.shader = sides[0].shader;
                    s2.flags.inhalt = sides[0].flags.inhalt;
                    s2.flags.flaeche = sides[i].flags.flaeche & S_SURF_BEVELSMASK;
                    s2.bevel = true;
                    sides.push_back(std::move(s2));
                    ++dazu;
                }
            }
        }
    }
    return dazu;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

Q3ShaderFlags q3map2ShaderFlags(const std::vector<std::string>& surfaceparms, bool ja) {
    Q3ShaderFlags f;
    (void)surfaceParmAnwenden("default", ja, f);   // AllocShaderInfo
    for (const std::string& p : surfaceparms) { (void)surfaceParmAnwenden(klein(p), ja, f); }
    return f;
}

SpielKollisionsBericht kollisionAusKarte(const Karte& k, const SurfaceparmFn& parms, bool ja, KollisionsKarte& aus) {
    aus = KollisionsKarte{};
    SpielKollisionsBericht bericht;
    // Shader-Tabelle (dshader_t: contentFlags, surfaceFlags)
    std::map<std::string, int> shaderNr;
    std::vector<Q3ShaderFlags> shaderFlags;
    const auto shader = [&](const std::string& name) {
        const std::string schluessel = klein(name);
        const auto it = shaderNr.find(schluessel);
        if (it != shaderNr.end()) { return it->second; }
        const int nr = static_cast<int>(shaderFlags.size());
        shaderNr.emplace(schluessel, nr);
        shaderFlags.push_back(q3map2ShaderFlags(parms ? parms(name) : std::vector<std::string>{}, ja));
        aus.shaderInhalt.push_back(shaderFlags.back().inhalt);
        aus.shaderFlaeche.push_back(shaderFlags.back().flaeche);
        return nr;
    };
    const auto ebeneDazu = [&aus](const Ebene& e) {
        KollisionsKarte::Ebene p;
        int bits = 0;
        for (int j = 0; j < 3; ++j) {
            p.normale[j] = e.n[j];
            if (e.n[j] < 0) { bits |= 1 << j; }
        }
        p.abstand = e.d;
        p.typ = e.n[0] == 1.0F ? 0 : (e.n[1] == 1.0F ? 1 : (e.n[2] == 1.0F ? 2 : 3));   // PlaneTypeForNormal
        p.vorzeichen = bits;
        aus.ebenen.push_back(p);
        return static_cast<int>(aus.ebenen.size()) - 1;
    };
    // Ebene 0: die Teilungsebene des einen Knotens (x = 131072: alles dahinter, Kind 1)
    {
        Ebene teil;
        teil.n[0] = 1;
        teil.d = 131072.0F;
        (void)ebeneDazu(teil);
    }
    float weltMin[3] = {99999.0F, 99999.0F, 99999.0F};
    float weltMax[3] = {-99999.0F, -99999.0F, -99999.0F};
    for (const auto& ent : k.entitaeten()) {
        const std::string& klasse = ent->klassenname();
        if (klasse.rfind("trigger_", 0) == 0) {   // InitTrigger: CONTENTS_TRIGGER
            bericht.ausgelassen += static_cast<int>(ent->kinder().size());
            continue;
        }
        for (const auto& prim : ent->kinder()) {
            if (prim->art() == Primitiv::Art::Pinsel) {
                const auto& p = static_cast<const Pinsel&>(*prim);
                p.bRep(k.kontext);   // die Wicklungen (evaluateBRep)
                std::vector<Seite> sides;
                float mins[3] = {99999.0F, 99999.0F, 99999.0F};   // ClearBounds / BoundBrush
                float maxs[3] = {-99999.0F, -99999.0F, -99999.0F};
                for (std::size_t i = 0; i < p.anzahl(); ++i) {
                    const Flaeche& f = p[i];
                    if (!f.traegtBei()) { continue; }
                    Seite s;
                    const Ebene3d& e = f.ebene();
                    s.ebene.n[0] = static_cast<float>(e.a);
                    s.ebene.n[1] = static_cast<float>(e.b);
                    s.ebene.n[2] = static_cast<float>(e.c);
                    s.ebene.d = static_cast<float>(e.d);
                    ebeneEinrasten(s.ebene, f.ebenenPunkte().data(), 3);   // MapPlaneFromPoints -> FindFloatPlane( plane, 3, points )
                    s.shader = shader(f.shader());
                    s.flags = shaderFlags[static_cast<std::size_t>(s.shader)];
                    const Wicklung& w = f.wicklung();
                    for (std::size_t j = 0; j < w.numpoints; ++j) {
                        const Vektor3d& v = w.points[j].vertex;
                        const V3f q{static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])};
                        s.wicklung.push_back(q);
                        for (int a = 0; a < 3; ++a) {
                            mins[a] = std::min(mins[a], q[static_cast<std::size_t>(a)]);
                            maxs[a] = std::max(maxs[a], q[static_cast<std::size_t>(a)]);
                        }
                    }
                    sides.push_back(std::move(s));
                }
                if (sides.size() < 4) {
                    ++bericht.ausgelassen;
                    continue;
                }
                // SetBrushContents (map.cpp:455-503)
                Q3ShaderFlags contentShader = sides[0].flags;
                int contentShaderNr = sides[0].shader;
                std::uint32_t compileFlags = sides[0].flags.compile;
                for (std::size_t i = 1; i < sides.size(); ++i) {
                    const Q3ShaderFlags& sf = sides[i].flags;
                    compileFlags |= sf.compile;
                    bool nehmen = false;
                    if ((contentShader.compile & C_LIQUID) != 0U) {
                        continue;
                    } else if ((sf.compile & C_LIQUID) != 0U) {
                        nehmen = true;
                    } else if ((contentShader.compile & C_FOG) != 0U) {
                        continue;
                    } else if ((sf.compile & C_FOG) != 0U) {
                        nehmen = true;
                    } else if ((contentShader.inhalt & S_CONT_PLAYERCLIP) != 0U) {
                        continue;
                    } else if ((sf.inhalt & S_CONT_PLAYERCLIP) != 0U) {
                        nehmen = true;
                    } else if ((contentShader.compile & C_SOLID) == 0U) {
                        continue;
                    } else if ((sf.compile & C_SOLID) == 0U) {
                        nehmen = true;
                    }
                    if (nehmen) {
                        contentShader = sf;
                        contentShaderNr = sides[i].shader;
                    }
                }
                if ((compileFlags & C_ORIGIN) != 0U) {   // Ursprungs-Brush: setzt den Ursprung der Entitaet, wird entfernt
                    ++bericht.ausgelassen;
                    continue;
                }
                bericht.bevels += bevelsDazu(sides, mins, maxs);
                KollisionsKarte::Pinsel b;
                b.ersteSeite = static_cast<int>(aus.seiten.size());
                b.seiten = static_cast<int>(sides.size());
                b.shader = contentShaderNr;
                b.inhalt = contentShader.inhalt;   // EmitBrushes: EmitShader( contentShader )
                // cm_load.cpp:272-278 (JK2_MODE): Brushes mit SURF_SLICK verlieren CONTENTS_SOLID
                if (!ja && (contentShader.flaeche & S_SURF_SLICK) != 0U) { b.inhalt &= ~S_CONT_SOLID; }
                for (const Seite& s : sides) {
                    KollisionsKarte::Seite cs;
                    cs.ebene = ebeneDazu(s.ebene);
                    cs.shader = s.shader;
                    aus.seiten.push_back(cs);
                }
                // CM_BoundBrush: die Seiten 0..5 sind der Kasten
                b.mins[0] = -sides[0].ebene.d;
                b.maxs[0] = sides[1].ebene.d;
                b.mins[1] = -sides[2].ebene.d;
                b.maxs[1] = sides[3].ebene.d;
                b.mins[2] = -sides[4].ebene.d;
                b.maxs[2] = sides[5].ebene.d;
                for (int a = 0; a < 3; ++a) {
                    weltMin[a] = std::min(weltMin[a], b.mins[a]);
                    weltMax[a] = std::max(weltMax[a], b.maxs[a]);
                }
                aus.pinsel.push_back(b);
                bericht.seiten += static_cast<int>(sides.size());
            } else if (prim->art() == Primitiv::Art::Patch) {
                const auto& pa = static_cast<const Patch&>(*prim);
                std::vector<std::array<float, 3>> pts;
                for (const PatchPunkt& pp : pa.punkte()) { pts.push_back({pp.vertex[0], pp.vertex[1], pp.vertex[2]}); }
                KollisionsKarte::Patch kp;
                const int nr = shader(pa.shader());
                kp.inhalt = shaderFlags[static_cast<std::size_t>(nr)].inhalt;
                kp.flaeche = shaderFlags[static_cast<std::size_t>(nr)].flaeche;
                if (!kollisionPatchErzeugen(static_cast<int>(pa.breite()), static_cast<int>(pa.hoehe()), pts, kp)) {
                    ++bericht.ausgelassen;
                    continue;
                }
                for (int a = 0; a < 3; ++a) {
                    weltMin[a] = std::min(weltMin[a], kp.mins[a]);
                    weltMax[a] = std::max(weltMax[a], kp.maxs[a]);
                }
                aus.flaechePatch.push_back(static_cast<int>(aus.patches.size()));
                aus.patches.push_back(std::move(kp));
            }
        }
    }
    // Ein Knoten, beide Kinder das eine Blatt; im Blatt alle Brushes und Patches.
    KollisionsKarte::Knoten n;
    n.ebene = 0;
    n.kinder[0] = -1;
    n.kinder[1] = -1;
    aus.knoten.push_back(n);
    KollisionsKarte::Blatt bl;
    bl.cluster = 0;
    bl.erstePinsel = 0;
    bl.pinsel = static_cast<int>(aus.pinsel.size());
    bl.ersteFlaeche = 0;
    bl.flaechen = static_cast<int>(aus.patches.size());
    aus.blaetter.push_back(bl);
    for (int i = 0; i < static_cast<int>(aus.pinsel.size()); ++i) { aus.blattPinsel.push_back(i); }
    for (int i = 0; i < static_cast<int>(aus.patches.size()); ++i) { aus.blattFlaechen.push_back(i); }
    KollisionsKarte::Modell welt;
    for (int a = 0; a < 3; ++a) {
        welt.mins[a] = weltMin[a];
        welt.maxs[a] = weltMax[a];
    }
    welt.erster = 0;
    welt.zahl = static_cast<int>(aus.pinsel.size());
    welt.flaechenZahl = static_cast<int>(aus.patches.size());
    aus.modelle.push_back(welt);
    bericht.pinsel = static_cast<int>(aus.pinsel.size());
    bericht.patches = static_cast<int>(aus.patches.size());
    return bericht;
}

}  // namespace bhed::karte
