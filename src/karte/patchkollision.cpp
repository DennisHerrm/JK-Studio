// patchkollision.cpp - Karten-Editor, Stufe 5 Punkt 2: Kollisionsebenen und
// -facetten eines Patches wie die Engine (siehe patchkollision.h).
//
// Portiert aus OpenJK code/qcommon/cm_patch.cpp (Zeilen 130-1265) und
// code/qcommon/cm_polylib.cpp (75-262), Rechenweg und Genauigkeit (float,
// Vergleiche gegen double-Konstanten) wie dort. Die festen Felder
// (cGrid_t::points[129][129], gridPlanes, planes[MAX_PATCH_PLANES],
// facets[MAX_FACETS], borderPlanes[4+6+16]) sind hier wachsende Vektoren:
// wo die Engine abbricht oder hinter ein Feld schreibt, zaehlt diese Fassung
// weiter und meldet die erreichte Groesse.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/karte/patchkollision.h"

#include <cmath>
#include <cstring>
#include <utility>

namespace bhed::karte {

namespace {

using V3 = std::array<float, 3>;

constexpr int kSubdivideDistance = 16;      // SUBDIVIDE_DISTANCE (cm_patch.h:99)
constexpr double kPlaneTriEpsilon = 0.1;    // PLANE_TRI_EPSILON (cm_patch.h:100)
constexpr double kWrapPointEpsilon = 0.1;   // WRAP_POINT_EPSILON (cm_patch.h:101)
constexpr double kPointEpsilon = 0.1;       // POINT_EPSILON (cm_patch.cpp:363)
constexpr double kNormalEpsilon = 0.00015;  // NORMAL_EPSILON (cm_patch.cpp:452)
constexpr double kDistEpsilon = 0.0235;     // DIST_EPSILON (cm_patch.cpp:453)
constexpr int kMaxMapBounds = 65535;        // MAX_MAP_BOUNDS (cm_polylib.h:44)
constexpr float kWorldSize = 64.0F * 1024.0F * 2.0F;   // WORLD_SIZE (q_shared.h:804)
constexpr int kMaxGridSize = 129;           // CM_MAX_GRID_SIZE (cm_patch.h:89)
constexpr int kSideFront = 0;               // SIDE_FRONT (cm_polylib.h:37)
constexpr int kSideBack = 1;
constexpr int kSideOn = 2;

inline float dot(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
inline void cross(const float* v1, const float* v2, float* out) {
    out[0] = v1[1] * v2[2] - v1[2] * v2[1];
    out[1] = v1[2] * v2[0] - v1[0] * v2[2];
    out[2] = v1[0] * v2[1] - v1[1] * v2[0];
}
// VectorNormalize (q_math.c:1158)
inline float normalize(float* v) {
    float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    length = std::sqrt(length);
    if (length != 0.0F) {
        const float ilength = 1 / length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }
    return length;
}

struct Ebene {
    float plane[4];
};

struct Facette {
    int surfacePlane = 0;
    int numBorders = 0;
    std::vector<int> borderPlanes;
    std::vector<int> borderInward;
    Facette() : borderPlanes(64, 0), borderInward(64, 0) {}
    void sicherstellen() {
        if (numBorders + 1 >= static_cast<int>(borderPlanes.size())) {
            borderPlanes.resize(borderPlanes.size() * 2, 0);
            borderInward.resize(borderInward.size() * 2, 0);
        }
    }
};

using Wicklung = std::vector<V3>;

struct Gitter {
    int width = 0;
    int height = 0;
    bool wrapWidth = false;
    bool wrapHeight = false;
    std::vector<std::vector<V3>> points;   // [width][height]
};

class Erzeuger {
public:
    PatchKollision ergebnis;

    // --- cm_polylib.cpp ---
    // BaseWindingForPlane (cm_polylib.cpp:101-165)
    static Wicklung basisWicklung(const float* normal, float dist) {
        float max = -kMaxMapBounds;
        int x = -1;
        for (int i = 0; i < 3; i++) {
            const float v = std::fabs(normal[i]);
            if (v > max) {
                x = i;
                max = v;
            }
        }
        if (x == -1) { return {}; }   // Com_Error "BaseWindingForPlane: no axis found"
        float vup[3] = {0, 0, 0};
        switch (x) {
        case 0:
        case 1: vup[2] = 1; break;
        case 2: vup[0] = 1; break;
        default: break;
        }
        float v = dot(vup, normal);
        for (int i = 0; i < 3; ++i) { vup[i] = vup[i] + normal[i] * -v; }   // VectorMA( vup, -v, normal, vup )
        // VectorNormalize2( vup, vup )
        {
            float length = vup[0] * vup[0] + vup[1] * vup[1] + vup[2] * vup[2];
            length = std::sqrt(length);
            if (length != 0.0F) {
                const float ilength = 1 / length;
                vup[0] *= ilength;
                vup[1] *= ilength;
                vup[2] *= ilength;
            }
        }
        float org[3] = {normal[0] * dist, normal[1] * dist, normal[2] * dist};
        float vright[3];
        cross(vup, normal, vright);
        for (int i = 0; i < 3; ++i) {
            vup[i] = vup[i] * kMaxMapBounds;
            vright[i] = vright[i] * kMaxMapBounds;
        }
        Wicklung w(4);
        for (int i = 0; i < 3; ++i) {
            w[0][i] = org[i] - vright[i];
            w[0][i] = w[0][i] + vup[i];
            w[1][i] = org[i] + vright[i];
            w[1][i] = w[1][i] + vup[i];
            w[2][i] = org[i] + vright[i];
            w[2][i] = w[2][i] - vup[i];
            w[3][i] = org[i] - vright[i];
            w[3][i] = w[3][i] - vup[i];
        }
        return w;
    }

    // ChopWindingInPlace (cm_polylib.cpp:188-262); false = ganz weggeschnitten
    bool schneiden(Wicklung& in, const float* normal, float dist, float epsilon) {
        const std::size_t n = in.size();
        std::vector<float> dists(n + 4, 0.0F);
        std::vector<int> sides(n + 4, 0);
        int counts[3] = {0, 0, 0};
        for (std::size_t i = 0; i < n; i++) {
            float d = dot(in[i].data(), normal);
            d -= dist;
            dists[i] = d;
            if (d > epsilon) { sides[i] = kSideFront; }
            else if (d < -epsilon) { sides[i] = kSideBack; }
            else { sides[i] = kSideOn; }
            counts[sides[i]]++;
        }
        sides[n] = sides[0];
        dists[n] = dists[0];
        if (counts[0] == 0) {
            in.clear();
            return false;
        }
        if (counts[1] == 0) { return true; }
        Wicklung f;
        f.reserve(n + 4);
        for (std::size_t i = 0; i < n; i++) {
            const V3& p1 = in[i];
            if (sides[i] == kSideOn) {
                f.push_back(p1);
                continue;
            }
            if (sides[i] == kSideFront) { f.push_back(p1); }
            if (sides[i + 1] == kSideOn || sides[i + 1] == sides[i]) { continue; }
            const V3& p2 = in[(i + 1) % n];
            const float d = dists[i] / (dists[i] - dists[i + 1]);
            V3 mid{};
            for (int j = 0; j < 3; j++) {
                if (normal[j] == 1) { mid[j] = dist; }
                else if (normal[j] == -1) { mid[j] = -dist; }
                else { mid[j] = p1[j] + d * (p2[j] - p1[j]); }
            }
            f.push_back(mid);
        }
        if (static_cast<int>(f.size()) > ergebnis.windungsPunkte) { ergebnis.windungsPunkte = static_cast<int>(f.size()); }
        in = std::move(f);
        return true;
    }

    // WindingBounds (cm_polylib.cpp:75-95)
    static void wicklungsGrenzen(const Wicklung& w, float* mins, float* maxs) {
        mins[0] = mins[1] = mins[2] = kWorldSize;
        maxs[0] = maxs[1] = maxs[2] = -kWorldSize;
        for (const V3& p : w) {
            for (int j = 0; j < 3; j++) {
                const float v = p[j];
                if (v < mins[j]) { mins[j] = v; }
                if (v > maxs[j]) { maxs[j] = v; }
            }
        }
    }

    // --- cm_patch.cpp: Unterteilung ---
    // CM_NeedsSubdivision (:183-205)
    static bool brauchtUnterteilung(const V3& a, const V3& b, const V3& c) {
        float cmid[3];
        float lmid[3];
        float delta[3];
        for (int i = 0; i < 3; i++) { lmid[i] = static_cast<float>(0.5 * (a[i] + c[i])); }
        for (int i = 0; i < 3; i++) { cmid[i] = static_cast<float>(0.5 * (0.5 * (a[i] + b[i]) + 0.5 * (b[i] + c[i]))); }
        for (int i = 0; i < 3; ++i) { delta[i] = cmid[i] - lmid[i]; }
        const float dist = dot(delta, delta);
        return dist >= (kSubdivideDistance * kSubdivideDistance);
    }
    // CM_Subdivide (:215-223)
    static void unterteilen(const V3& a, const V3& b, const V3& c, V3& out1, V3& out2, V3& out3) {
        for (int i = 0; i < 3; i++) {
            out1[i] = static_cast<float>(0.5 * (a[i] + b[i]));
            out3[i] = static_cast<float>(0.5 * (b[i] + c[i]));
            out2[i] = static_cast<float>(0.5 * (out1[i] + out3[i]));
        }
    }
    // CM_TransposeGrid (:231-275)
    static void transponieren(Gitter& g) {
        std::vector<std::vector<V3>> neu(static_cast<std::size_t>(g.height), std::vector<V3>(static_cast<std::size_t>(g.width)));
        for (int i = 0; i < g.width; ++i) {
            for (int j = 0; j < g.height; ++j) {
                neu[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = g.points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
            }
        }
        g.points = std::move(neu);
        std::swap(g.width, g.height);
        std::swap(g.wrapWidth, g.wrapHeight);
    }
    // CM_SetGridWrapWidth (:284-305)
    static void wrapSetzen(Gitter& g) {
        int i = 0;
        for (i = 0; i < g.height; i++) {
            int j = 0;
            for (j = 0; j < 3; j++) {
                const float d = g.points[0][static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] -
                                g.points[static_cast<std::size_t>(g.width - 1)][static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
                if (d < -kWrapPointEpsilon || d > kWrapPointEpsilon) { break; }
            }
            if (j != 3) { break; }
        }
        g.wrapWidth = i == g.height;
    }
    // CM_SubdivideGridColumns (:316-370)
    void spaltenUnterteilen(Gitter& g) {
        for (int i = 0; i < g.width - 2;) {
            int j = 0;
            for (j = 0; j < g.height; j++) {
                const auto ju = static_cast<std::size_t>(j);
                if (brauchtUnterteilung(g.points[static_cast<std::size_t>(i)][ju], g.points[static_cast<std::size_t>(i + 1)][ju],
                                        g.points[static_cast<std::size_t>(i + 2)][ju])) {
                    break;
                }
            }
            if (j == g.height) {
                // die Naeherungsspalte entfernen
                g.points.erase(g.points.begin() + (i + 1));
                g.width--;
                i++;
                continue;
            }
            // zwei Spalten einfuegen: i+1 wird ersetzt, aus i+2 wird i+4
            std::vector<V3> s1(static_cast<std::size_t>(g.height));
            std::vector<V3> s2(static_cast<std::size_t>(g.height));
            std::vector<V3> s3(static_cast<std::size_t>(g.height));
            for (j = 0; j < g.height; j++) {
                const auto ju = static_cast<std::size_t>(j);
                const V3 prev = g.points[static_cast<std::size_t>(i)][ju];
                const V3 mid = g.points[static_cast<std::size_t>(i + 1)][ju];
                const V3 next = g.points[static_cast<std::size_t>(i + 2)][ju];
                unterteilen(prev, mid, next, s1[ju], s2[ju], s3[ju]);
            }
            g.points[static_cast<std::size_t>(i + 1)] = std::move(s1);
            g.points.insert(g.points.begin() + (i + 2), std::move(s2));
            g.points.insert(g.points.begin() + (i + 3), std::move(s3));
            g.width += 2;
            if (g.width > ergebnis.groessteGitterSeite) { ergebnis.groessteGitterSeite = g.width; }
        }
    }
    // CM_ComparePoints (:369-387)
    static bool punkteGleich(const V3& a, const V3& b) {
        for (int k = 0; k < 3; ++k) {
            const float d = a[k] - b[k];
            if (d < -kPointEpsilon || d > kPointEpsilon) { return false; }
        }
        return true;
    }
    // CM_RemoveDegenerateColumns (:396-422)
    static void doppelteSpaltenEntfernen(Gitter& g) {
        for (int i = 0; i < g.width - 1; i++) {
            int j = 0;
            for (j = 0; j < g.height; j++) {
                if (!punkteGleich(g.points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)],
                                  g.points[static_cast<std::size_t>(i + 1)][static_cast<std::size_t>(j)])) {
                    break;
                }
            }
            if (j != g.height) { continue; }
            g.points.erase(g.points.begin() + (i + 1));
            g.width--;
            i--;
        }
    }

    // --- cm_patch.cpp: Ebenen ---
    std::vector<Ebene> planes;

    // CM_PlaneFromPoints (:156-168)
    static bool ebeneAusPunkten(float* plane, const float* a, const float* b, const float* c) {
        float d1[3];
        float d2[3];
        for (int i = 0; i < 3; ++i) {
            d1[i] = b[i] - a[i];
            d2[i] = c[i] - a[i];
        }
        cross(d2, d1, plane);
        if (normalize(plane) == 0) { return false; }
        plane[3] = dot(a, plane);
        return true;
    }
    // CM_PlaneEqual (:455-484)
    static bool ebeneGleich(const Ebene& p, const float* plane, bool& flipped) {
        if (std::fabs(p.plane[0] - plane[0]) < kNormalEpsilon && std::fabs(p.plane[1] - plane[1]) < kNormalEpsilon &&
            std::fabs(p.plane[2] - plane[2]) < kNormalEpsilon && std::fabs(p.plane[3] - plane[3]) < kDistEpsilon) {
            flipped = false;
            return true;
        }
        float inv[4] = {-plane[0], -plane[1], -plane[2], -plane[3]};
        if (std::fabs(p.plane[0] - inv[0]) < kNormalEpsilon && std::fabs(p.plane[1] - inv[1]) < kNormalEpsilon &&
            std::fabs(p.plane[2] - inv[2]) < kNormalEpsilon && std::fabs(p.plane[3] - inv[3]) < kDistEpsilon) {
            flipped = true;
            return true;
        }
        return false;
    }
    // CM_SnapVector (:486-504)
    static void einrasten(float* normal) {
        for (int i = 0; i < 3; i++) {
            if (std::fabs(normal[i] - 1) < kNormalEpsilon) {
                normal[0] = normal[1] = normal[2] = 0;
                normal[i] = 1;
                break;
            }
            if (std::fabs(normal[i] - -1) < kNormalEpsilon) {
                normal[0] = normal[1] = normal[2] = 0;
                normal[i] = -1;
                break;
            }
        }
    }
    // CM_FindPlane2 (:506-528)
    int ebeneFinden2(const float* plane, bool& flipped) {
        for (std::size_t i = 0; i < planes.size(); i++) {
            if (ebeneGleich(planes[i], plane, flipped)) { return static_cast<int>(i); }
        }
        Ebene e{};
        std::memcpy(e.plane, plane, sizeof(e.plane));
        planes.push_back(e);   // numPlanes == MAX_PATCH_PLANES: Com_Error (hier weitergezaehlt)
        flipped = false;
        return static_cast<int>(planes.size()) - 1;
    }
    // CM_FindPlane (:535-575)
    int ebeneFinden(const float* p1, const float* p2, const float* p3) {
        float plane[4];
        if (!ebeneAusPunkten(plane, p1, p2, p3)) { return -1; }
        for (std::size_t i = 0; i < planes.size(); i++) {
            const float* q = planes[i].plane;
            if (dot(plane, q) < 0) { continue; }
            float d = dot(p1, q) - q[3];
            if (d < -kPlaneTriEpsilon || d > kPlaneTriEpsilon) { continue; }
            d = dot(p2, q) - q[3];
            if (d < -kPlaneTriEpsilon || d > kPlaneTriEpsilon) { continue; }
            d = dot(p3, q) - q[3];
            if (d < -kPlaneTriEpsilon || d > kPlaneTriEpsilon) { continue; }
            return static_cast<int>(i);
        }
        Ebene e{};
        std::memcpy(e.plane, plane, sizeof(e.plane));
        planes.push_back(e);
        return static_cast<int>(planes.size()) - 1;
    }
    // CM_PointOnPlaneSide (:583-603)
    int punktSeite(const float* p, int planeNum) const {
        if (planeNum == -1) { return kSideOn; }
        const float* plane = planes[static_cast<std::size_t>(planeNum)].plane;
        const float d = dot(p, plane) - plane[3];
        if (d > kPlaneTriEpsilon) { return kSideFront; }
        if (d < -kPlaneTriEpsilon) { return kSideBack; }
        return kSideOn;
    }

    // gridPlanes[i][j][tri]
    std::vector<int> gridPlanes;
    int gpHoehe = 0;
    int& gp(int i, int j, int tri) { return gridPlanes[(static_cast<std::size_t>(i) * static_cast<std::size_t>(gpHoehe) + static_cast<std::size_t>(j)) * 2 + static_cast<std::size_t>(tri)]; }

    // CM_GridPlane (:606-622)
    int gitterEbene(int i, int j, int tri) {
        int p = gp(i, j, tri);
        if (p != -1) { return p; }
        p = gp(i, j, tri == 0 ? 1 : 0);
        if (p != -1) { return p; }
        return -1;   // "WARNING: CM_GridPlane unresolvable"
    }
    // CM_EdgePlaneNum (:629-703)
    int kantenEbene(const Gitter& g, int i, int j, int k) {
        const float* p1 = nullptr;
        const float* p2 = nullptr;
        float up[3];
        int p = -1;
        auto P = [&](int a, int b) { return g.points[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)].data(); };
        auto hoch = [&](const float* von, int ebene) {
            const float* n = planes[static_cast<std::size_t>(ebene)].plane;
            for (int x = 0; x < 3; ++x) { up[x] = von[x] + n[x] * 4; }
        };
        switch (k) {
        case 0:   // top border
            p1 = P(i, j);
            p2 = P(i + 1, j);
            p = gitterEbene(i, j, 0);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p1, p2, up);
        case 2:   // bottom border
            p1 = P(i, j + 1);
            p2 = P(i + 1, j + 1);
            p = gitterEbene(i, j, 1);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p2, p1, up);
        case 3:   // left border
            p1 = P(i, j);
            p2 = P(i, j + 1);
            p = gitterEbene(i, j, 1);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p2, p1, up);
        case 1:   // right border
            p1 = P(i + 1, j);
            p2 = P(i + 1, j + 1);
            p = gitterEbene(i, j, 0);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p1, p2, up);
        case 4:   // diagonal out of triangle 0
            p1 = P(i + 1, j + 1);
            p2 = P(i, j);
            p = gitterEbene(i, j, 0);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p1, p2, up);
        case 5:   // diagonal out of triangle 1
            p1 = P(i, j);
            p2 = P(i + 1, j + 1);
            p = gitterEbene(i, j, 1);
            if (p == -1) { return -1; }
            hoch(p1, p);
            return ebeneFinden(p1, p2, up);
        default: break;
        }
        return -1;
    }
    // CM_SetBorderInward (:710-777)
    void randInnen(Facette& f, const Gitter& g, int i, int j, int which) {
        const float* points[4] = {nullptr, nullptr, nullptr, nullptr};
        int numPoints = 0;
        auto P = [&](int a, int b) { return g.points[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)].data(); };
        switch (which) {
        case -1:
            points[0] = P(i, j);
            points[1] = P(i + 1, j);
            points[2] = P(i + 1, j + 1);
            points[3] = P(i, j + 1);
            numPoints = 4;
            break;
        case 0:
            points[0] = P(i, j);
            points[1] = P(i + 1, j);
            points[2] = P(i + 1, j + 1);
            numPoints = 3;
            break;
        case 1:
            points[0] = P(i + 1, j + 1);
            points[1] = P(i, j + 1);
            points[2] = P(i, j);
            numPoints = 3;
            break;
        default: return;
        }
        for (int k = 0; k < f.numBorders; k++) {
            int front = 0;
            int back = 0;
            for (int l = 0; l < numPoints; l++) {
                const int side = punktSeite(points[l], f.borderPlanes[static_cast<std::size_t>(k)]);
                if (side == kSideFront) { front++; }
                if (side == kSideBack) { back++; }
            }
            if (front != 0 && back == 0) { f.borderInward[static_cast<std::size_t>(k)] = 1; }
            else if (back != 0 && front == 0) { f.borderInward[static_cast<std::size_t>(k)] = 0; }
            else if (front == 0 && back == 0) { f.borderPlanes[static_cast<std::size_t>(k)] = -1; }   // flat side border
            else { f.borderInward[static_cast<std::size_t>(k)] = 0; }   // "mixed plane sides"
        }
    }
    // CM_ValidateFacet (:786-831)
    bool facettePruefen(const Facette& f) {
        if (f.surfacePlane == -1) { return false; }
        float plane[4];
        std::memcpy(plane, planes[static_cast<std::size_t>(f.surfacePlane)].plane, sizeof(plane));
        Wicklung w = basisWicklung(plane, plane[3]);
        bool da = !w.empty();
        for (int j = 0; j < f.numBorders && da; j++) {
            if (f.borderPlanes[static_cast<std::size_t>(j)] == -1) { return false; }
            std::memcpy(plane, planes[static_cast<std::size_t>(f.borderPlanes[static_cast<std::size_t>(j)])].plane, sizeof(plane));
            if (f.borderInward[static_cast<std::size_t>(j)] == 0) {
                plane[0] = 0 - plane[0];
                plane[1] = 0 - plane[1];
                plane[2] = 0 - plane[2];
                plane[3] = -plane[3];
            }
            da = schneiden(w, plane, plane[3], 0.1F);
        }
        if (!da) { return false; }
        float b0[3];
        float b1[3];
        wicklungsGrenzen(w, b0, b1);
        for (int j = 0; j < 3; j++) {
            if (b1[j] - b0[j] > kMaxMapBounds) { return false; }
            if (b0[j] >= kMaxMapBounds) { return false; }
            if (b1[j] <= -kMaxMapBounds) { return false; }
        }
        return true;
    }
    // CM_AddFacetBevels (:838-986)
    void facettenBevels(Facette& f) {
        float plane[4];
        std::memcpy(plane, planes[static_cast<std::size_t>(f.surfacePlane)].plane, sizeof(plane));
        Wicklung w = basisWicklung(plane, plane[3]);
        bool da = !w.empty();
        for (int j = 0; j < f.numBorders && da; j++) {
            if (f.borderPlanes[static_cast<std::size_t>(j)] == f.surfacePlane) { continue; }
            std::memcpy(plane, planes[static_cast<std::size_t>(f.borderPlanes[static_cast<std::size_t>(j)])].plane, sizeof(plane));
            if (f.borderInward[static_cast<std::size_t>(j)] == 0) {
                plane[0] = 0 - plane[0];
                plane[1] = 0 - plane[1];
                plane[2] = 0 - plane[2];
                plane[3] = -plane[3];
            }
            da = schneiden(w, plane, plane[3], 0.1F);
        }
        if (!da) { return; }
        float mins[3];
        float maxs[3];
        wicklungsGrenzen(w, mins, maxs);
        bool flipped = false;
        // die achsparallelen Ebenen
        for (int axis = 0; axis < 3; axis++) {
            for (int dir = -1; dir <= 1; dir += 2) {
                plane[0] = plane[1] = plane[2] = 0;
                plane[axis] = static_cast<float>(dir);
                plane[3] = dir == 1 ? maxs[axis] : -mins[axis];
                if (ebeneGleich(planes[static_cast<std::size_t>(f.surfacePlane)], plane, flipped)) { continue; }
                int i = 0;
                for (i = 0; i < f.numBorders; i++) {
                    if (ebeneGleich(planes[static_cast<std::size_t>(f.borderPlanes[static_cast<std::size_t>(i)])], plane, flipped)) { break; }
                }
                if (i == f.numBorders) {
                    f.sicherstellen();
                    f.borderPlanes[static_cast<std::size_t>(f.numBorders)] = ebeneFinden2(plane, flipped);
                    f.borderInward[static_cast<std::size_t>(f.numBorders)] = flipped ? 1 : 0;
                    f.numBorders++;
                }
            }
        }
        // die Kanten-Bevels (nur nicht achsparallele Kanten)
        const std::size_t n = w.size();
        for (std::size_t j = 0; j < n; j++) {
            const std::size_t k1 = (j + 1) % n;
            float vec[3] = {w[j][0] - w[k1][0], w[j][1] - w[k1][1], w[j][2] - w[k1][2]};
            if (normalize(vec) < 0.5) { continue; }
            einrasten(vec);
            int k = 0;
            for (k = 0; k < 3; k++) {
                if (vec[k] == -1 || vec[k] == 1) { break; }
            }
            if (k < 3) { continue; }
            for (int axis = 0; axis < 3; axis++) {
                for (int dir = -1; dir <= 1; dir += 2) {
                    float vec2[3] = {0, 0, 0};
                    vec2[axis] = static_cast<float>(dir);
                    cross(vec, vec2, plane);
                    if (normalize(plane) < 0.5) { continue; }
                    plane[3] = dot(w[j].data(), plane);
                    std::size_t l = 0;
                    for (l = 0; l < n; l++) {
                        const float d = dot(w[l].data(), plane) - plane[3];
                        if (d > 0.1) { break; }
                    }
                    if (l < n) { continue; }
                    if (ebeneGleich(planes[static_cast<std::size_t>(f.surfacePlane)], plane, flipped)) { continue; }
                    int i = 0;
                    for (i = 0; i < f.numBorders; i++) {
                        if (ebeneGleich(planes[static_cast<std::size_t>(f.borderPlanes[static_cast<std::size_t>(i)])], plane, flipped)) { break; }
                    }
                    if (i == f.numBorders) {
                        f.sicherstellen();
                        f.borderPlanes[static_cast<std::size_t>(f.numBorders)] = ebeneFinden2(plane, flipped);
                        f.borderInward[static_cast<std::size_t>(f.numBorders)] = flipped ? 1 : 0;
                        Wicklung w2 = w;
                        float newplane[4];
                        std::memcpy(newplane, planes[static_cast<std::size_t>(f.borderPlanes[static_cast<std::size_t>(f.numBorders)])].plane, sizeof(newplane));
                        if (f.borderInward[static_cast<std::size_t>(f.numBorders)] == 0) {
                            newplane[0] = -newplane[0];
                            newplane[1] = -newplane[1];
                            newplane[2] = -newplane[2];
                            newplane[3] = -newplane[3];
                        }
                        if (!schneiden(w2, newplane, newplane[3], 0.1F)) { continue; }   // "invalid bevel"
                        f.numBorders++;
                    }
                }
            }
        }
        // die Gegen-Ebene (#ifndef BSPC)
        f.sicherstellen();
        f.borderPlanes[static_cast<std::size_t>(f.numBorders)] = f.surfacePlane;
        f.borderInward[static_cast<std::size_t>(f.numBorders)] = 1;
        f.numBorders++;
    }

    // CM_PatchCollideFromGrid (:1015-1165)
    void ausGitter(const Gitter& g) {
        planes.clear();
        gpHoehe = g.height;
        gridPlanes.assign(static_cast<std::size_t>(g.width) * static_cast<std::size_t>(g.height) * 2, -1);
        int numFacets = 0;
        int angefragt = 0;   // groesstes numFacets an einer Pruefung (== MAX_FACETS -> Com_Error)
        auto P = [&](int a, int b) { return g.points[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)].data(); };
        for (int i = 0; i < g.width - 1; i++) {
            for (int j = 0; j < g.height - 1; j++) {
                gp(i, j, 0) = ebeneFinden(P(i, j), P(i + 1, j), P(i + 1, j + 1));
                gp(i, j, 1) = ebeneFinden(P(i + 1, j + 1), P(i, j + 1), P(i, j));
            }
        }
        enum { kTop, kRight, kBottom, kLeft };
        for (int i = 0; i < g.width - 1; i++) {
            for (int j = 0; j < g.height - 1; j++) {
                int borders[4];
                bool noAdjust[4];
                borders[kTop] = -1;
                if (j > 0) { borders[kTop] = gp(i, j - 1, 1); }
                else if (g.wrapHeight) { borders[kTop] = gp(i, g.height - 2, 1); }
                noAdjust[kTop] = borders[kTop] == gp(i, j, 0);
                if (borders[kTop] == -1 || noAdjust[kTop]) { borders[kTop] = kantenEbene(g, i, j, 0); }

                borders[kBottom] = -1;
                if (j < g.height - 2) { borders[kBottom] = gp(i, j + 1, 0); }
                else if (g.wrapHeight) { borders[kBottom] = gp(i, 0, 0); }
                noAdjust[kBottom] = borders[kBottom] == gp(i, j, 1);
                if (borders[kBottom] == -1 || noAdjust[kBottom]) { borders[kBottom] = kantenEbene(g, i, j, 2); }

                borders[kLeft] = -1;
                if (i > 0) { borders[kLeft] = gp(i - 1, j, 0); }
                else if (g.wrapWidth) { borders[kLeft] = gp(g.width - 2, j, 0); }
                noAdjust[kLeft] = borders[kLeft] == gp(i, j, 1);
                if (borders[kLeft] == -1 || noAdjust[kLeft]) { borders[kLeft] = kantenEbene(g, i, j, 3); }

                borders[kRight] = -1;
                if (i < g.width - 2) { borders[kRight] = gp(i + 1, j, 1); }
                else if (g.wrapWidth) { borders[kRight] = gp(0, j, 1); }
                noAdjust[kRight] = borders[kRight] == gp(i, j, 0);
                if (borders[kRight] == -1 || noAdjust[kRight]) { borders[kRight] = kantenEbene(g, i, j, 1); }

                angefragt = std::max(angefragt, numFacets);
                Facette f;
                if (gp(i, j, 0) == gp(i, j, 1)) {
                    if (gp(i, j, 0) == -1) { continue; }   // degenerate
                    f.surfacePlane = gp(i, j, 0);
                    f.numBorders = 4;
                    f.borderPlanes[0] = borders[kTop];
                    f.borderPlanes[1] = borders[kRight];
                    f.borderPlanes[2] = borders[kBottom];
                    f.borderPlanes[3] = borders[kLeft];
                    randInnen(f, g, i, j, -1);
                    if (facettePruefen(f)) {
                        facettenBevels(f);
                        ergebnis.meisteRaender = std::max(ergebnis.meisteRaender, f.numBorders);
                        numFacets++;
                    }
                } else {
                    f.surfacePlane = gp(i, j, 0);
                    f.numBorders = 3;
                    f.borderPlanes[0] = borders[kTop];
                    f.borderPlanes[1] = borders[kRight];
                    f.borderPlanes[2] = gp(i, j, 1);
                    if (f.borderPlanes[2] == -1) {
                        f.borderPlanes[2] = borders[kBottom];
                        if (f.borderPlanes[2] == -1) { f.borderPlanes[2] = kantenEbene(g, i, j, 4); }
                    }
                    randInnen(f, g, i, j, 0);
                    if (facettePruefen(f)) {
                        facettenBevels(f);
                        ergebnis.meisteRaender = std::max(ergebnis.meisteRaender, f.numBorders);
                        numFacets++;
                    }
                    angefragt = std::max(angefragt, numFacets);
                    Facette f2;
                    f2.surfacePlane = gp(i, j, 1);
                    f2.numBorders = 3;
                    f2.borderPlanes[0] = borders[kBottom];
                    f2.borderPlanes[1] = borders[kLeft];
                    f2.borderPlanes[2] = gp(i, j, 0);
                    if (f2.borderPlanes[2] == -1) {
                        f2.borderPlanes[2] = borders[kTop];
                        if (f2.borderPlanes[2] == -1) { f2.borderPlanes[2] = kantenEbene(g, i, j, 5); }
                    }
                    randInnen(f2, g, i, j, 1);
                    if (facettePruefen(f2)) {
                        facettenBevels(f2);
                        ergebnis.meisteRaender = std::max(ergebnis.meisteRaender, f2.numBorders);
                        numFacets++;
                    }
                }
            }
        }
        ergebnis.ebenen = static_cast<int>(planes.size());
        // die Grenze gilt fuer die Plaetze, die angefragt wurden (vor jeder
        // Facette: numFacets == MAX_FACETS -> Com_Error)
        ergebnis.facetten = std::max(numFacets, angefragt + 1);
        if (numFacets == 0 && angefragt == 0) { ergebnis.facetten = 0; }
    }
};

}  // namespace

PatchKollision patchKollision(int breite, int hoehe, const std::vector<std::array<float, 3>>& punkte) {
    Erzeuger e;
    // CM_GeneratePatchCollide (cm_patch.cpp:1203-1265)
    if (breite <= 2 || hoehe <= 2 || punkte.size() < static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe)) { return e.ergebnis; }
    if ((breite & 1) == 0 || (hoehe & 1) == 0) { return e.ergebnis; }   // "even sizes are invalid for quadratic meshes"
    if (breite > kMaxGridSize || hoehe > kMaxGridSize) { return e.ergebnis; }
    e.ergebnis.gueltig = true;
    Gitter g;
    g.width = breite;
    g.height = hoehe;
    g.points.assign(static_cast<std::size_t>(breite), std::vector<V3>(static_cast<std::size_t>(hoehe)));
    for (int i = 0; i < breite; i++) {
        for (int j = 0; j < hoehe; j++) {
            g.points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = punkte[static_cast<std::size_t>(j) * static_cast<std::size_t>(breite) + static_cast<std::size_t>(i)];
        }
    }
    e.ergebnis.groessteGitterSeite = std::max(breite, hoehe);
    Erzeuger::wrapSetzen(g);
    e.spaltenUnterteilen(g);
    Erzeuger::doppelteSpaltenEntfernen(g);
    Erzeuger::transponieren(g);
    Erzeuger::wrapSetzen(g);
    e.spaltenUnterteilen(g);
    Erzeuger::doppelteSpaltenEntfernen(g);
    e.ergebnis.gitterBreite = g.width;
    e.ergebnis.gitterHoehe = g.height;
    e.ausGitter(g);
    return e.ergebnis;
}

}  // namespace bhed::karte
