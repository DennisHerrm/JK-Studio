// bobtoolz_d.cpp - Karten-Editor, Stufe 4: die Hilfsklassen von bobToolz
// (siehe bobtoolz_d.h).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz: DPoint.cpp,
// DPlane.cpp, DWinding.cpp, DBrush.cpp, DPatch.cpp, DEntity.cpp, DMap.cpp,
// DShape.cpp, misc.cpp; libs/mathlib/mathlib.c.
//
//   BobToolz plugin for GtkRadiant
//   Copyright (C) 2001 Gordon Biggans
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2: This program is free software; you can redistribute
//   it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; version 2 of the License. This
//   program is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#include "bobtoolz_d.h"

#include "bhed/karte/rueckgaengig.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bhed::karte::bobtoolz::d {

// --- libs/mathlib/mathlib.c ------------------------------------------------------

vec_t VectorLength(const vec_t* v) {   // mathlib.c:95-100
    vec_t length = 0;
    for (int i = 0; i < 3; ++i) { length += v[i] * v[i]; }
    length = static_cast<vec_t>(std::sqrt(length));
    return length;
}

vec_t VectorNormalize(const vec_t* in, vec_t* out) {   // VectorAccurateNormalize (mathlib.c:150-168)
    const double x = in[0];
    const double y = in[1];
    const double z = in[2];
    const double length = std::sqrt((x * x) + (y * y) + (z * z));
    if (length == 0) {
        VectorClear(out);
        return 0;
    }
    out[0] = static_cast<vec_t>(x / length);
    out[1] = static_cast<vec_t>(y / length);
    out[2] = static_cast<vec_t>(z / length);
    return static_cast<vec_t>(length);
}

bool VectorCompare(const vec_t* v1, const vec_t* v2) {   // mathlib.c:102-108
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(v1[i] - v2[i]) > EQUAL_EPSILON) { return false; }
    }
    return true;
}

// --- misc.cpp ----------------------------------------------------------------------

vec_t Min(vec_t a, vec_t b) { return a < b ? a : b; }   // misc.cpp:320-325

void MakeNormal(const vec_t* va, const vec_t* vb, const vec_t* vc, vec_t* out) {   // misc.cpp:327-332
    vec3_t v1;
    vec3_t v2;
    VectorSubtract(va, vb, v1);
    VectorSubtract(vc, vb, v2);
    CrossProduct(v1, v2, out);
}

float Determinant3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3) {   // misc.cpp:300-304
    return a1 * (b2 * c3 - b3 * c2) - a2 * (b1 * c3 - b3 * c1) + a3 * (b1 * c2 - b2 * c1);
}

void MoveBlock(int dir, vec3_t min, vec3_t max, float dist) {   // misc.cpp:66-94
    switch (dir) {
    case MOVE_EAST:
        min[0] += dist;
        max[0] += dist;
        break;
    case MOVE_WEST:
        min[0] -= dist;
        max[0] -= dist;
        break;
    case MOVE_NORTH:
        min[1] += dist;
        max[1] += dist;
        break;
    case MOVE_SOUTH:
        min[1] -= dist;
        max[1] -= dist;
        break;
    default: break;
    }
}

void SetInitialStairPos(int dir, vec3_t min, vec3_t max, float width) {   // misc.cpp:96-120
    switch (dir) {
    case MOVE_EAST: max[0] = min[0] + width; break;
    case MOVE_WEST: min[0] = max[0] - width; break;
    case MOVE_NORTH: max[1] = min[1] + width; break;
    case MOVE_SOUTH: min[1] = max[1] - width; break;
    default: break;
    }
}

void FillDefaultTexture(FaceData* faceData, const double* va, const double* vb, const double* vc, const char* texture) {   // misc.cpp:280-298
    faceData->texdef.rotate = 0;
    faceData->texdef.scale[0] = 0.5F;
    faceData->texdef.scale[1] = 0.5F;
    faceData->texdef.shift[0] = 0;
    faceData->texdef.shift[1] = 0;
    faceData->contents = 0;
    faceData->flags = 0;
    faceData->value = 0;
    faceData->shader = *texture != 0 ? texture : "textures/common/caulk";
    for (int i = 0; i < 3; ++i) {
        faceData->p0[i] = va[i];
        faceData->p1[i] = vb[i];
        faceData->p2[i] = vc[i];
    }
}

void BrushAddFace(Pinsel& p, const FaceData& f, const Kontext& k) {   // Brush_addFace (brushmodule.cpp:237-240)
    TexturProjektion proj;   // TextureProjection( m_texdef, brushprimit_texdef_t(), 0, 0 )
    proj.texdef = f.texdef;
    proj.basisS = Vektor3f(0, 0, 0);
    proj.basisT = Vektor3f(0, 0, 0);
    (void)p.ebeneHinzufuegen(Vektor3d(f.p0[0], f.p0[1], f.p0[2]), Vektor3d(f.p1[0], f.p1[1], f.p1[2]), Vektor3d(f.p2[0], f.p2[1], f.p2[2]),
                             f.shader, proj, k);
}

// --- DPoint.cpp ---------------------------------------------------------------------

bool DPoint::operator==(const vec_t* other) const {
    vec3_t test;
    VectorSubtract(other, _pnt, test);
    return !(std::fabs(VectorLength(test)) > MAX_ROUND_ERROR);
}

// --- DPlane.cpp ---------------------------------------------------------------------

DPlane::DPlane(const vec_t* va, const vec_t* vb, const vec_t* vc, const FaceData* texData) {   // DPlane.cpp:30-53
    MakeNormal(va, vb, vc, normal);
    (void)VectorNormalize(normal, normal);   // "DPlane::DPlane: Bad Normal." bei 0 (globalErrorStream)
    _d = (normal[0] * va[0]) + (normal[1] * va[1]) + (normal[2] * va[2]);
    VectorCopy(va, points[0]);
    VectorCopy(vb, points[1]);
    VectorCopy(vc, points[2]);
    m_bChkOk = true;
    if (texData != nullptr) {
        texInfo = *texData;
    } else {
        double a[3];
        double b[3];
        double c[3];
        VectorCopy(points[0], a);
        VectorCopy(points[1], b);
        VectorCopy(points[2], c);
        FillDefaultTexture(&texInfo, a, b, c, "textures/common/caulk");
    }
    m_shader = texInfo.shader;
}

DPlane::DPlane(const vec_t* va, const vec_t* vb, const vec_t* vc, const char* textureName, bool bDetail) {   // DPlane.cpp:243-266
    vec3_t v1;
    vec3_t v2;
    VectorSubtract(va, vb, v1);
    VectorSubtract(vc, vb, v2);
    CrossProduct(v1, v2, normal);
    (void)VectorNormalize(normal, normal);
    _d = (normal[0] * va[0]) + (normal[1] * va[1]) + (normal[2] * va[2]);
    VectorCopy(va, points[0]);
    VectorCopy(vb, points[1]);
    VectorCopy(vc, points[2]);
    m_bChkOk = true;
    double a[3];
    double b[3];
    double c[3];
    VectorCopy(points[0], a);
    VectorCopy(points[1], b);
    VectorCopy(points[2], c);
    FillDefaultTexture(&texInfo, a, b, c, textureName);
    m_shader = texInfo.shader;
    if (bDetail) { texInfo.contents |= FACE_DETAIL; }
}

vec_t DPlane::DistanceToPoint(const vec_t* pnt) const {   // DPlane.cpp:55-60
    vec3_t tmp;
    VectorSubtract(pnt, points[0], tmp);
    return DotProduct(tmp, normal);
}

bool DPlane::PlaneIntersection(const DPlane* pl1, const DPlane* pl2, vec_t* out) const {   // DPlane.cpp:62-92
    const float a1 = normal[0];
    const float a2 = normal[1];
    const float a3 = normal[2];
    const float b1 = pl1->normal[0];
    const float b2 = pl1->normal[1];
    const float b3 = pl1->normal[2];
    const float c1 = pl2->normal[0];
    const float c2 = pl2->normal[1];
    const float c3 = pl2->normal[2];
    const float d = Determinant3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
    if (d == 0) { return false; }
    const float v1 = _d;
    const float v2 = pl1->_d;
    const float v3 = pl2->_d;
    const float d1 = Determinant3x3(v1, a2, a3, v2, b2, b3, v3, c2, c3);
    const float d2 = Determinant3x3(a1, v1, a3, b1, v2, b3, c1, v3, c3);
    const float d3 = Determinant3x3(a1, a2, v1, b1, b2, v2, c1, c2, v3);
    out[0] = d1 / d;
    out[1] = d2 / d;
    out[2] = d3 / d;
    return true;
}

bool DPlane::IsRedundant(const std::list<DPoint*>& pointList) const {   // DPlane.cpp:94-110
    int cnt = 0;
    for (const DPoint* point : pointList) {
        if (std::fabs(DistanceToPoint(point->_pnt)) < MAX_ROUND_ERROR) { cnt++; }
        if (cnt == 3) { return false; }
    }
    return true;
}

bool DPlane::operator==(const DPlane& other) const {   // DPlane.cpp:112-125
    vec3_t chk;
    VectorSubtract(other.normal, normal, chk);
    if (std::fabs(VectorLength(chk)) > MAX_ROUND_ERROR) { return false; }
    if (std::fabs(other._d - _d) > MAX_ROUND_ERROR) { return false; }
    return true;
}

bool DPlane::operator!=(const DPlane& other) const {   // DPlane.cpp:127-135
    vec3_t chk;
    VectorAdd(other.normal, normal, chk);
    return !(std::fabs(VectorLength(chk)) > MAX_ROUND_ERROR);
}

DWinding DPlane::BaseWindingForPlane() const {   // DPlane.cpp:137-196
    vec_t max = -131072;
    int x = -1;
    for (int i = 0; i < 3; ++i) {
        const vec_t v = std::fabs(normal[i]);
        if (v > max) {
            x = i;
            max = v;
        }
    }
    if (x == -1) { return DWinding(); }   // "BaseWindingForPlane: no axis found"
    vec3_t vup = {0, 0, 0};
    switch (x) {
    case 0:
    case 1: vup[2] = 1; break;
    default: vup[0] = 1; break;
    }
    const vec_t v = DotProduct(vup, normal);
    VectorMA(vup, -v, normal, vup);
    (void)VectorNormalize(vup, vup);
    vec3_t org;
    VectorScale(normal, _d, org);
    vec3_t vright;
    CrossProduct(vup, normal, vright);
    VectorScale(vup, 131072, vup);
    VectorScale(vright, 131072, vright);
    DWinding w;
    w.AllocWinding(4);
    VectorSubtract(org, vright, w.p[0].data());
    VectorAdd(w.p[0].data(), vup, w.p[0].data());
    VectorAdd(org, vright, w.p[1].data());
    VectorAdd(w.p[1].data(), vup, w.p[1].data());
    VectorAdd(org, vright, w.p[2].data());
    VectorSubtract(w.p[2].data(), vup, w.p[2].data());
    VectorSubtract(org, vright, w.p[3].data());
    VectorSubtract(w.p[3].data(), vup, w.p[3].data());
    return w;
}

void DPlane::Rebuild() {   // DPlane.cpp:198-214
    vec3_t v1;
    vec3_t v2;
    VectorSubtract(points[0], points[1], v1);
    VectorSubtract(points[2], points[1], v2);
    CrossProduct(v1, v2, normal);
    (void)VectorNormalize(normal, normal);   // "DPlane::Rebuild: Bad Normal."
    _d = (normal[0] * points[0][0]) + (normal[1] * points[0][1]) + (normal[2] * points[0][2]);
    VectorCopy(points[0], texInfo.p0);
    VectorCopy(points[1], texInfo.p1);
    VectorCopy(points[2], texInfo.p2);
}

bool DPlane::AddToBrush(Pinsel& brush, const Kontext& k) {   // DPlane.cpp:216-235
    bool changed = false;
    if (!(m_bChkOk || m_shader == "textures/common/caulk")) {
        m_shader = "textures/common/caulk";
        changed = true;
    }
    FaceData faceData;   // nur Punkte, texdef und Shader (Inhalt/Flags nicht)
    VectorCopy(points[0], faceData.p0);
    VectorCopy(points[1], faceData.p1);
    VectorCopy(points[2], faceData.p2);
    faceData.texdef = texInfo.texdef;
    faceData.shader = m_shader;
    BrushAddFace(brush, faceData, k);
    return changed;
}

// --- DWinding.cpp -------------------------------------------------------------------

void DWinding::AllocWinding(int points) {   // DWinding.cpp:61-67
    numpoints = points;
    p.assign(static_cast<std::size_t>(std::max(points, 0)), std::array<vec_t, 3>{0, 0, 0});
}

vec_t DWinding::WindingArea() const {   // DWinding.cpp:69-85
    vec_t total = 0;
    for (int i = 2; i < numpoints; ++i) {
        vec3_t d1;
        vec3_t d2;
        vec3_t cross;
        VectorSubtract(p[static_cast<std::size_t>(i - 1)].data(), p[0].data(), d1);
        VectorSubtract(p[static_cast<std::size_t>(i)].data(), p[0].data(), d2);
        CrossProduct(d1, d2, cross);
        total += 0.5F * VectorLength(cross);
    }
    return total;
}

void DWinding::RemoveColinearPoints() {   // DWinding.cpp:87-114
    std::vector<std::array<vec_t, 3>> p2;
    for (int i = 0; i < numpoints; ++i) {
        const int j = (i + 1) % numpoints;
        const int k = (i + numpoints - 1) % numpoints;
        vec3_t v1;
        vec3_t v2;
        VectorSubtract(p[static_cast<std::size_t>(j)].data(), p[static_cast<std::size_t>(i)].data(), v1);
        VectorSubtract(p[static_cast<std::size_t>(i)].data(), p[static_cast<std::size_t>(k)].data(), v2);
        (void)VectorNormalize(v1, v1);
        (void)VectorNormalize(v2, v2);
        if (DotProduct(v1, v2) < 0.999) { p2.push_back(p[static_cast<std::size_t>(i)]); }
    }
    if (static_cast<int>(p2.size()) == numpoints) { return; }
    numpoints = static_cast<int>(p2.size());
    p = std::move(p2);
}

DPlane* DWinding::WindingPlane() const {   // DWinding.cpp:116-119
    return new DPlane(p[0].data(), p[1].data(), p[2].data(), static_cast<const FaceData*>(nullptr));
}

void DWinding::WindingCentroid(vec_t* centroid) const {   // DWinding.cpp:153-169 (in double)
    double cent[3] = {0, 0, 0};
    double areasum = 0;
    for (int i = 1; i < numpoints - 1; ++i) {
        double a[3];
        double b[3];
        double c[3];
        for (int j = 0; j < 3; ++j) {
            a[j] = p[0][static_cast<std::size_t>(j)];
            b[j] = p[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
            c[j] = p[static_cast<std::size_t>(i + 1)][static_cast<std::size_t>(j)];
        }
        const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const double v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        const double x[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const double area = std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        for (int j = 0; j < 3; ++j) { cent[j] += (a[j] + b[j] + c[j]) * area; }
        areasum += area;
    }
    for (int j = 0; j < 3; ++j) {
        cent[j] /= areasum * 3;
        centroid[j] = static_cast<vec_t>(cent[j]);
    }
}

bool DWinding::ChopWindingInPlace(const DPlane* chopPlane, vec_t epsilon) {   // DWinding.cpp:284-381
    if (numpoints > MAX_POINTS_ON_WINDING) {
        *this = DWinding();   // "ChopWindingInPlace: MAX_POINTS_ON_WINDING"
        return false;
    }
    std::vector<vec_t> dists(static_cast<std::size_t>(numpoints + 4));
    std::vector<int> sides(static_cast<std::size_t>(numpoints + 4));
    int counts[3] = {0, 0, 0};
    for (int i = 0; i < numpoints; ++i) {
        const auto ui = static_cast<std::size_t>(i);
        dists[ui] = DotProduct(p[ui].data(), chopPlane->normal) - chopPlane->_d;
        if (dists[ui] > epsilon) {
            sides[ui] = SIDE_FRONT;
        } else if (dists[ui] < -epsilon) {
            sides[ui] = SIDE_BACK;
        } else {
            sides[ui] = SIDE_ON;
        }
        counts[sides[ui]]++;
    }
    sides[static_cast<std::size_t>(numpoints)] = sides[0];
    dists[static_cast<std::size_t>(numpoints)] = dists[0];
    if (counts[SIDE_FRONT] == 0) {
        *this = DWinding();
        return false;
    }
    if (counts[SIDE_BACK] == 0) { return true; }
    const int maxpts = numpoints + 4;
    DWinding f;
    f.AllocWinding(maxpts);
    f.numpoints = 0;
    for (int i = 0; i < numpoints; ++i) {
        const auto ui = static_cast<std::size_t>(i);
        const std::array<vec_t, 3> p1 = p[ui];
        if (sides[ui] == SIDE_ON) {
            f.p[static_cast<std::size_t>(f.numpoints++)] = p1;
            continue;
        }
        if (sides[ui] == SIDE_FRONT) { f.p[static_cast<std::size_t>(f.numpoints++)] = p1; }
        if (sides[ui + 1] == SIDE_ON || sides[ui + 1] == sides[ui]) { continue; }
        const std::array<vec_t, 3> p2 = p[static_cast<std::size_t>((i + 1) % numpoints)];
        const vec_t dot = dists[ui] / (dists[ui] - dists[ui + 1]);
        std::array<vec_t, 3> mid{};
        for (int j = 0; j < 3; ++j) {
            const auto uj = static_cast<std::size_t>(j);
            if (chopPlane->normal[j] == 1) {
                mid[uj] = chopPlane->_d;
            } else if (chopPlane->normal[j] == -1) {
                mid[uj] = -chopPlane->_d;
            } else {
                mid[uj] = p1[uj] + dot * (p2[uj] - p1[uj]);
            }
        }
        f.p[static_cast<std::size_t>(f.numpoints++)] = mid;
    }
    f.p.resize(static_cast<std::size_t>(f.numpoints));
    *this = std::move(f);
    return true;
}

// --- DBrush.cpp ---------------------------------------------------------------------

DBrush::~DBrush() {
    ClearFaces();
    ClearPoints();
}

DPlane* DBrush::AddFace(const vec_t* va, const vec_t* vb, const vec_t* vc, const FaceData* texData) {   // DBrush.cpp:61-71
    bBoundsBuilt = false;
    auto* newFace = new DPlane(va, vb, vc, texData);
    faceList.push_back(newFace);
    return newFace;
}

DPlane* DBrush::AddFace(const vec_t* va, const vec_t* vb, const vec_t* vc, const char* textureName, bool bDetail) {   // DBrush.cpp:796-801
    bBoundsBuilt = false;
    auto* newFace = new DPlane(va, vb, vc, textureName, bDetail);
    faceList.push_back(newFace);
    return newFace;
}

int DBrush::BuildPoints() {   // DBrush.cpp:73-112
    ClearPoints();
    if (faceList.size() <= 3) { return 0; }
    for (auto p1 = faceList.begin(); p1 != faceList.end(); ++p1) {
        auto p2 = p1;
        for (++p2; p2 != faceList.end(); ++p2) {
            auto p3 = p2;
            for (++p3; p3 != faceList.end(); ++p3) {
                vec3_t pnt;
                if ((*p1)->PlaneIntersection(*p2, *p3, pnt)) {
                    const int pos = PointPosition(pnt);
                    if (pos == POINT_ON_BRUSH && !HasPoint(pnt)) { AddPoint(pnt); }   // POINT_IN_BRUSH: "Point IN brush!!!"
                }
            }
        }
    }
    return static_cast<int>(pointList.size());
}

void DBrush::LoadFromBrush(const std::shared_ptr<Entitaet>& entity, const std::shared_ptr<Pinsel>& brush, bool textured) {   // DBrush.cpp:132-140
    ClearFaces();
    ClearPoints();
    // Brush_forEachFace mit BrushFaceData_fromFace (brushmodule.cpp:207-218)
    for (std::size_t i = 0; i < brush->anzahl(); ++i) {
        const Flaeche& f = (*brush)[i];
        FaceData fd;
        const EbenenPunkte& e = f.ebenenPunkte();
        for (int j = 0; j < 3; ++j) {
            fd.p0[j] = e[0][static_cast<std::size_t>(j)];
            fd.p1[j] = e[1][static_cast<std::size_t>(j)];
            fd.p2[j] = e[2][static_cast<std::size_t>(j)];
        }
        fd.shader = f.shader();
        fd.texdef = f.projektion().texdef;   // m_projection.m_texdef
        fd.contents = f.flags().inhaltFlags;
        fd.flags = f.flags().flaechenFlags;
        fd.value = f.flags().wert;
        vec3_t a;
        vec3_t b;
        vec3_t c;
        VectorCopy(fd.p0, a);
        VectorCopy(fd.p1, b);
        VectorCopy(fd.p2, c);
        (void)AddFace(a, b, c, textured ? &fd : nullptr);   // DBrush_addFaceTextured / DBrush_addFace
    }
    QER_entity = entity;
    QER_brush = brush;
}

int DBrush::PointPosition(const vec_t* pnt) const {   // DBrush.cpp:142-158
    int state = POINT_IN_BRUSH;
    for (const DPlane* plane : faceList) {
        const float dist = plane->DistanceToPoint(pnt);
        if (dist > MAX_ROUND_ERROR) { return POINT_OUT_BRUSH; }
        if (std::fabs(dist) < MAX_ROUND_ERROR) { state = POINT_ON_BRUSH; }
    }
    return state;
}

void DBrush::ClearPoints() {
    for (DPoint* point : pointList) { delete point; }
    pointList.clear();
}

void DBrush::ClearFaces() {
    bBoundsBuilt = false;
    for (DPlane* plane : faceList) { delete plane; }
    faceList.clear();
}

void DBrush::AddPoint(const vec_t* pnt) {
    auto* newPoint = new DPoint;
    VectorCopy(pnt, newPoint->_pnt);
    pointList.push_back(newPoint);
}

bool DBrush::HasPoint(const vec_t* pnt) const {
    return std::any_of(pointList.begin(), pointList.end(), [pnt](const DPoint* p) { return *p == pnt; });
}

int DBrush::RemoveRedundantPlanes() {   // DBrush.cpp:194-261
    int cnt = 0;
    auto p1 = faceList.begin();
    while (p1 != faceList.end()) {
        auto p2 = p1;
        for (++p2; p2 != faceList.end(); ++p2) {
            if (**p1 == **p2) {
                if ((*p1)->m_shader == "textures/common/caulk") {
                    delete *p1;
                    p1 = faceList.erase(p1);
                } else {
                    delete *p2;
                    p2 = faceList.erase(p2);
                }
                cnt++;
                break;
            }
        }
        if (p2 == faceList.end()) { p1++; }
    }
    auto chkPlane = faceList.begin();
    while (chkPlane != faceList.end()) {
        if (VectorLength((*chkPlane)->normal) == 0) {
            delete *chkPlane;
            chkPlane = faceList.erase(chkPlane);
            cnt++;
        } else {
            chkPlane++;
        }
    }
    if (pointList.empty()) { (void)BuildPoints(); }
    chkPlane = faceList.begin();
    while (chkPlane != faceList.end()) {
        if ((*chkPlane)->IsRedundant(pointList)) {
            delete *chkPlane;
            chkPlane = faceList.erase(chkPlane);
            cnt++;
        } else {
            chkPlane++;
        }
    }
    return cnt;
}

bool DBrush::GetBounds(vec_t* min, vec_t* max) {   // DBrush.cpp:263-273
    BuildBounds();
    if (!bBoundsBuilt) { return false; }
    VectorCopy(bbox_min, min);
    VectorCopy(bbox_max, max);
    return true;
}

bool DBrush::BBoxCollision(DBrush* chkBrush) {   // DBrush.cpp:275-308
    vec3_t min1;
    vec3_t min2;
    vec3_t max1;
    vec3_t max2;
    if (!GetBounds(min1, max1) || !chkBrush->GetBounds(min2, max2)) { return false; }
    if (min1[0] >= max2[0] || min1[1] >= max2[1] || min1[2] >= max2[2]) { return false; }
    if (max1[0] <= min2[0] || max1[1] <= min2[1] || max1[2] <= min2[2]) { return false; }
    return true;
}

DPlane* DBrush::HasPlane(const DPlane* chkPlane) const {   // DBrush.cpp:310-318
    for (DPlane* plane : faceList) {
        if (*plane == *chkPlane) { return plane; }
    }
    return nullptr;
}

bool DBrush::IsCutByPlane(const DPlane* cuttingPlane) {   // DBrush.cpp:320-367
    bool isInFront = false;
    if (pointList.empty() && BuildPoints() == 0) { return false; }
    auto chkPnt = pointList.begin();
    if (chkPnt == pointList.end()) { return false; }
    float dist = cuttingPlane->DistanceToPoint((*chkPnt)->_pnt);
    if (dist > MAX_ROUND_ERROR) {
        isInFront = false;
    } else if (dist < MAX_ROUND_ERROR) {
        isInFront = true;
    } else {
        return true;
    }
    for (++chkPnt; chkPnt != pointList.end(); ++chkPnt) {
        dist = cuttingPlane->DistanceToPoint((*chkPnt)->_pnt);
        if (dist > MAX_ROUND_ERROR) {
            if (isInFront) { return true; }
        } else if (dist < MAX_ROUND_ERROR) {
            if (!isInFront) { return true; }
        } else {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Pinsel> DBrush::BuildInRadiant(Karte& k, bool allowDestruction, int* changeCnt, const std::shared_ptr<Entitaet>& entity,
                                               Meldungen& m) {   // DBrush.cpp:369-410
    if (allowDestruction) {
        const bool kill = std::none_of(faceList.begin(), faceList.end(), [](const DPlane* p) { return p->m_bChkOk; });
        if (kill) { return nullptr; }
    }
    if (faceList.size() < 4) {
        m.push_back({2, "Possible Phantom Brush Found, will not rebuild\n"});
        return nullptr;
    }
    auto node = std::make_shared<Pinsel>();   // GlobalBrushCreator().createBrush()
    node->schicht = static_cast<int>(k.aktuelleSchicht);
    for (DPlane* plane : faceList) {
        if (plane->AddToBrush(*node, k.kontext) && changeCnt != nullptr) { (*changeCnt)++; }
    }
    const std::shared_ptr<Entitaet> ziel = entity ? entity : k.weltHolenOderAnlegen();   // GlobalRadiant().getMapWorldEntity()
    ziel->kindEinfuegen(node);
    QER_entity = entity;
    QER_brush = node;
    return node;
}

void DBrush::selectInRadiant(Karte& k) const {   // DBrush.cpp:412-417 (select_primitive)
    const auto e = QER_entity.lock();
    const auto b = QER_brush.lock();
    if (e && b) { k.auswahl.waehlen(e, b); }
}

bool DBrush::IntersectsWith(DBrush* chkBrush) {   // DBrush.cpp:437-482
    if (pointList.empty() && BuildPoints() == 0) { return false; }
    if (chkBrush->pointList.empty() && chkBrush->BuildPoints() == 0) { return false; }
    if (!BBoxCollision(chkBrush)) { return false; }
    for (const DPlane* plane : faceList) {
        bool allInFront = true;
        for (const DPoint* point : chkBrush->pointList) {
            if (plane->DistanceToPoint(point->_pnt) < -MAX_ROUND_ERROR) {
                allInFront = false;
                break;
            }
        }
        if (allInFront) { return false; }
    }
    for (const DPlane* plane : chkBrush->faceList) {
        bool allInFront = true;
        for (const DPoint* point : pointList) {
            if (plane->DistanceToPoint(point->_pnt) < -MAX_ROUND_ERROR) {
                allInFront = false;
                break;
            }
        }
        if (allInFront) { return false; }
    }
    return true;
}

bool DBrush::IntersectsWith(const DPlane* p1, const DPlane* p2, vec_t* v) const {   // DBrush.cpp:484-503
    const vec3_t vDown = {0, 0, -1};
    for (const DPlane* plane : faceList) {
        const vec_t d = DotProduct(plane->normal, vDown);
        if (d >= 0) { continue; }
        if (plane->PlaneIntersection(p1, p2, v) && PointPosition(v) != POINT_OUT_BRUSH) { return true; }
    }
    return false;
}

void DBrush::BuildBounds() {   // DBrush.cpp:505-546
    if (bBoundsBuilt) { return; }
    if (pointList.empty() && BuildPoints() == 0) { return; }
    auto first = pointList.begin();
    VectorCopy((*first)->_pnt, bbox_min);
    VectorCopy((*first)->_pnt, bbox_max);
    auto point = pointList.begin();
    for (++point; point != pointList.end(); ++point) {
        for (int i = 0; i < 3; ++i) {
            if ((*point)->_pnt[i] > bbox_max[i]) { bbox_max[i] = (*point)->_pnt[i]; }
            if ((*point)->_pnt[i] < bbox_min[i]) { bbox_min[i] = (*point)->_pnt[i]; }
        }
    }
    bBoundsBuilt = true;
}

void DBrush::ResetChecks(const std::vector<std::string>& exclusionList) {   // DBrush.cpp:614-621
    for (DPlane* plane : faceList) {
        plane->m_bChkOk = std::any_of(exclusionList.begin(), exclusionList.end(),
                                      [plane](const std::string& texture) { return plane->m_shader.find(texture) != std::string::npos; });
    }
}

bool DBrush::HasTexture(const char* textureName) const {   // DBrush.cpp:641-650
    return std::any_of(faceList.begin(), faceList.end(), [textureName](const DPlane* p) { return p->m_shader.find(textureName) != std::string::npos; });
}

bool DBrush::IsDetail() const {   // DBrush.cpp:652-661
    return std::any_of(faceList.begin(), faceList.end(), [](const DPlane* p) { return (p->texInfo.contents & FACE_DETAIL) != 0; });
}

bool DBrush::ResetTextures(const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                           bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation) {   // DBrush.cpp:718-781
    const auto setzen = [&](DPlane* plane) {
        if (bResetTextureName) { plane->m_shader = newTextureName; }
        if (bResetScale[0]) { plane->texInfo.texdef.scale[0] = fScale[0]; }
        if (bResetScale[1]) { plane->texInfo.texdef.scale[1] = fScale[1]; }
        if (bResetShift[0]) { plane->texInfo.texdef.shift[0] = fShift[0]; }
        if (bResetShift[1]) { plane->texInfo.texdef.shift[1] = fShift[1]; }
        if (bResetRotation) { plane->texInfo.texdef.rotate = static_cast<float>(rotation); }
    };
    if (textureName != nullptr) {
        bool changed = false;
        for (DPlane* plane : faceList) {
            if (plane->m_shader == textureName) {
                setzen(plane);
                changed = true;
            }
        }
        return changed;
    }
    for (DPlane* plane : faceList) { setzen(plane); }
    return true;
}

bool DBrush::operator==(const DBrush* other) const {   // DBrush.cpp:783-794
    for (DPlane* plane : faceList) {
        if (other->HasPlane(plane) == nullptr) { return false; }
    }
    for (DPlane* plane : other->faceList) {
        if (HasPlane(plane) == nullptr) { return false; }
    }
    return true;
}

DPlane* DBrush::FindPlaneWithClosestNormal(const vec_t* normal) const {   // DBrush.cpp:803-815
    vec_t bestDot = -2;
    DPlane* bestDotPlane = nullptr;
    for (DPlane* plane : faceList) {
        const vec_t dot = DotProduct(plane->normal, normal);
        if (dot > bestDot) {
            bestDot = dot;
            bestDotPlane = plane;
        }
    }
    return bestDotPlane;
}

int DBrush::FindPointsForPlane(DPlane* plane, DPoint** pnts, int maxpnts) {   // DBrush.cpp:817-837
    int numpnts = 0;
    if (maxpnts == 0) { return 0; }
    (void)BuildPoints();
    for (DPoint* point : pointList) {
        if (std::fabs(plane->DistanceToPoint(point->_pnt)) < MAX_ROUND_ERROR) {
            pnts[numpnts] = point;
            numpnts++;
            if (numpnts >= maxpnts) { return numpnts; }
        }
    }
    return numpnts;
}

// --- DPatch.cpp ---------------------------------------------------------------------

std::shared_ptr<Patch> DPatch::BuildInRadiant(Karte& k, const std::shared_ptr<Entitaet>& entity) {   // DPatch.cpp:46-70
    auto patch = std::make_shared<Patch>();   // GlobalPatchCreator().createPatch()
    patch->schicht = static_cast<int>(k.aktuelleSchicht);
    const std::shared_ptr<Entitaet> parent = entity ? entity : k.weltHolenOderAnlegen();
    patch->setzeShader(texture);                                                       // Patch_setShader
    patch->setzeGroesse(static_cast<std::size_t>(height), static_cast<std::size_t>(width));   // Patch_resize( patch, height, width )
    std::vector<PatchPunkt> pkt = patch->punkte();
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            PatchPunkt& p = pkt[static_cast<std::size_t>(x) * static_cast<std::size_t>(height) + static_cast<std::size_t>(y)];   // matrix( x, y )
            const drawVert_t& v = points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)];
            p.vertex = Vektor3f(v.xyz[0], v.xyz[1], v.xyz[2]);
            p.texcoord = Vektor2f(v.st[0], v.st[1]);
        }
    }
    parent->kindEinfuegen(patch);   // Node_getTraversable( parent )->insert( patch )
    patch->punkteSetzen(pkt);       // Patch_controlPointsChanged
    QER_entity = entity;
    QER_brush = patch;
    return patch;
}

void DPatch::LoadFromPatch(const std::shared_ptr<Entitaet>& entity, const std::shared_ptr<Patch>& patch) {   // DPatch.cpp:104-124
    QER_entity = entity;
    QER_brush = patch;
    width = static_cast<int>(patch->hoehe());    // matrix.x() = getHeight()
    height = static_cast<int>(patch->breite());  // matrix.y() = getWidth()
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            const PatchPunkt& p = patch->punkt(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
            drawVert_t& v = points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)];
            v.xyz[0] = p.vertex[0];
            v.xyz[1] = p.vertex[1];
            v.xyz[2] = p.vertex[2];
            v.st[0] = p.texcoord[0];
            v.st[1] = p.texcoord[1];
        }
    }
    SetTexture(patch->shader());
}

bool DPatch::ResetTextures(const char* oldTextureName, const char* newTextureName) {   // DPatch.cpp:145-152
    if (oldTextureName == nullptr || texture == oldTextureName) {
        texture = newTextureName;
        return true;
    }
    return false;
}

namespace {

void Build1dArray(vec3_t* array, const std::array<std::array<drawVert_t, MAX_PATCH_HEIGHT>, MAX_PATCH_WIDTH>& points, int startX, int startY,
                  int number, bool horizontal, bool inverse) {   // DPatch.cpp:154-180
    int x = startX;
    int y = startY;
    const int step = inverse ? -1 : 1;
    for (int i = 0; i < number; ++i) {
        VectorCopy(points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)].xyz, array[i]);
        if (horizontal) {
            x += step;
        } else {
            y += step;
        }
    }
}

bool Compare1dArrays(vec3_t* a1, vec3_t* a2, int size) {   // DPatch.cpp:188-203
    for (int i = 0; i < size; ++i) {
        if (!VectorCompare(a1[i], a2[size - i - 1])) { return false; }
    }
    return true;
}

}  // namespace

patch_merge_t DPatch::IsMergable(const DPatch& other) const {   // DPatch.cpp:205-262
    vec3_t p1Array[4][MAX_PATCH_HEIGHT];
    vec3_t p2Array[4][MAX_PATCH_HEIGHT];
    Build1dArray(p1Array[0], points, 0, 0, width, true, false);
    Build1dArray(p1Array[1], points, width - 1, 0, height, false, false);
    Build1dArray(p1Array[2], points, width - 1, height - 1, width, true, true);
    Build1dArray(p1Array[3], points, 0, height - 1, height, false, true);
    Build1dArray(p2Array[0], other.points, 0, 0, other.width, true, false);
    Build1dArray(p2Array[1], other.points, other.width - 1, 0, other.height, false, false);
    Build1dArray(p2Array[2], other.points, other.width - 1, other.height - 1, other.width, true, true);
    Build1dArray(p2Array[3], other.points, 0, other.height - 1, other.height, false, true);
    const int p1ArraySizes[4] = {width, height, width, height};
    const int p2ArraySizes[4] = {other.width, other.height, other.width, other.height};
    patch_merge_t merge_info;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (p1ArraySizes[i] == p2ArraySizes[j] && Compare1dArrays(p1Array[i], p2Array[j], p1ArraySizes[i])) {
                merge_info.pos1 = i;
                merge_info.pos2 = j;
                merge_info.mergable = true;
                return merge_info;
            }
        }
    }
    merge_info.mergable = false;
    return merge_info;
}

DPatch* DPatch::MergePatches(patch_merge_t merge_info, DPatch& p1, DPatch& p2) {   // DPatch.cpp:264-306
    while (merge_info.pos1 != 2) {
        p1.Transpose();
        merge_info.pos1--;
        if (merge_info.pos1 < 0) { merge_info.pos1 += 4; }
    }
    while (merge_info.pos2 != 0) {
        p2.Transpose();
        merge_info.pos2--;
        if (merge_info.pos2 < 0) { merge_info.pos2 += 3; }   // so bei NRC (+= 3)
    }
    const int newHeight = p1.height + p2.height - 1;
    if (newHeight > MAX_PATCH_HEIGHT) { return nullptr; }
    auto* newPatch = new DPatch();
    newPatch->height = newHeight;
    newPatch->width = p1.width;
    newPatch->SetTexture(p1.texture);
    for (int y = 0; y < p1.height; ++y) {
        for (int x = 0; x < p1.width; ++x) {
            newPatch->points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)] = p1.points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)];
        }
    }
    for (int y = 1; y < p2.height; ++y) {
        for (int x = 0; x < p2.width; ++x) {
            newPatch->points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y + p1.height - 1)] =
                p2.points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)];
        }
    }
    return newPatch;
}

void DPatch::Invert() {   // DPatch.cpp:308-318
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height / 2; ++j) {
            std::swap(points[static_cast<std::size_t>(i)][static_cast<std::size_t>(height - 1 - j)], points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
        }
    }
}

void DPatch::Transpose() {   // DPatch.cpp:365-403
    if (width > height) {
        for (int i = 0; i < height; ++i) {
            for (int j = i + 1; j < width; ++j) {
                if (j < height) {
                    std::swap(points[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)], points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
                } else {
                    points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = points[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)];
                }
            }
        }
    } else {
        for (int i = 0; i < width; ++i) {
            for (int j = i + 1; j < height; ++j) {
                if (j < width) {
                    std::swap(points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)], points[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)]);
                } else {
                    points[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = points[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
                }
            }
        }
    }
    std::swap(width, height);
    Invert();
}

std::list<DPatch> DPatch::SplitCols() {
    Meldungen m;
    return SplitCols(m);
}
std::list<DPatch> DPatch::SplitRows() {
    Meldungen m;
    return SplitRows(m);
}

std::list<DPatch> DPatch::SplitCols(Meldungen& m) {   // DPatch.cpp:405-436
    std::list<DPatch> patchList;
    if (height >= 5) {
        for (int i = 0; i < (height - 1) / 2; ++i) {
            DPatch p;
            p.width = width;
            p.height = MIN_PATCH_HEIGHT;
            p.SetTexture(texture);
            for (int x = 0; x < p.width; ++x) {
                for (int y = 0; y < MIN_PATCH_HEIGHT; ++y) {
                    p.points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)] = points[static_cast<std::size_t>(x)][static_cast<std::size_t>((i * 2) + y)];
                }
            }
            patchList.push_back(p);
        }
    } else {
        m.push_back({1, "bobToolz SplitPatchCols: Patch has not enough columns for splitting.\n"});
        patchList.push_back(*this);
    }
    return patchList;
}

std::list<DPatch> DPatch::SplitRows(Meldungen& m) {   // DPatch.cpp:438-470
    std::list<DPatch> patchList;
    if (width >= 5) {
        for (int i = 0; i < (width - 1) / 2; ++i) {
            DPatch p;
            p.width = MIN_PATCH_WIDTH;
            p.height = height;
            p.SetTexture(texture);
            for (int x = 0; x < MIN_PATCH_WIDTH; ++x) {
                for (int y = 0; y < p.height; ++y) {
                    p.points[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)] = points[static_cast<std::size_t>((i * 2) + x)][static_cast<std::size_t>(y)];
                }
            }
            patchList.push_back(p);
        }
    } else {
        m.push_back({1, "bobToolz SplitPatchRows: Patch has not enough rows for splitting.\n"});
        patchList.push_back(*this);
    }
    return patchList;
}

std::list<DPatch> DPatch::Split(Meldungen& m) {   // DPatch.cpp:472-491
    std::list<DPatch> patchList;
    if (height >= 5) {
        std::list<DPatch> patchColList = SplitCols(m);
        if (width >= 5) {
            for (auto&& patchesCol : patchColList) { patchList.splice(patchList.cend(), patchesCol.SplitRows(m)); }
        } else {
            patchList.swap(patchColList);
        }
    } else if (width >= 5) {
        patchList = SplitRows(m);
    } else {
        m.push_back({1, "bobToolz SplitPatch: Patch has not enough rows and columns for splitting.\n"});
        patchList.push_back(*this);
    }
    return patchList;
}

// --- DEntity.cpp --------------------------------------------------------------------

DEntity::DEntity(const char* classname, int ID) : m_nID(ID), m_Classname(classname) {}

DEntity::~DEntity() {
    ClearPatches();
    ClearBrushes();
    ClearEPairs();
}

void DEntity::ClearBrushes() {
    for (DBrush* brush : brushList) { delete brush; }
    brushList.clear();
}

void DEntity::ClearPatches() {
    for (DPatch* patch : patchList) { delete patch; }
    patchList.clear();
}

DPatch* DEntity::NewPatch() { return patchList.emplace_back(new DPatch); }
DBrush* DEntity::NewBrush() { return brushList.emplace_back(new DBrush); }

void DEntity::LoadSelectedBrushes(const Karte& k) {   // DEntity.cpp:166-171 (Scene_forEachSelectedBrush)
    ClearBrushes();
    ClearEPairs();
    for (const Gewaehlt& g : auswahlListe(k)) {
        if (g.primitiv && g.primitiv->art() == Primitiv::Art::Pinsel) {
            NewBrush()->LoadFromBrush(g.entitaet, std::static_pointer_cast<Pinsel>(g.primitiv), true);
        }
    }
}

void DEntity::LoadSelectedPatches(const Karte& k) {   // DEntity.cpp:200-205
    ClearPatches();
    ClearEPairs();
    for (const Gewaehlt& g : auswahlListe(k)) {
        if (g.primitiv && g.primitiv->art() == Primitiv::Art::Patch) {
            NewPatch()->LoadFromPatch(g.entitaet, std::static_pointer_cast<Patch>(g.primitiv));
        }
    }
}

std::vector<bool> DEntity::BuildIntersectList() {   // DEntity.cpp:207-226
    std::vector<bool> pbIntList(brushList.size(), false);
    for (std::size_t i = 0; i < brushList.size(); ++i) {
        for (std::size_t j = i + 1; j < brushList.size(); ++j) {
            if (brushList[i]->IntersectsWith(brushList[j])) {
                pbIntList[i] = true;
                pbIntList[j] = true;
            }
        }
    }
    return pbIntList;
}

std::vector<bool> DEntity::BuildDuplicateList() {   // DEntity.cpp:228-247
    std::vector<bool> pbDupList(brushList.size(), false);
    for (std::size_t i = 0; i < brushList.size(); ++i) {
        for (std::size_t j = i + 1; j < brushList.size(); ++j) {
            if (brushList[i]->operator==(brushList[j])) {
                pbDupList[i] = true;
                pbDupList[j] = true;
            }
        }
    }
    return pbDupList;
}

void DEntity::SelectBrushes(Karte& k, const std::vector<bool>& selectList) {   // DEntity.cpp:249-262
    if (selectList.empty()) { return; }
    k.auswahl.leeren();   // GlobalSelectionSystem().setSelectedAll( false )
    for (std::size_t i = 0; i < brushList.size(); ++i) {
        if (selectList[i]) { brushList[i]->selectInRadiant(k); }
    }
}

bool DEntity::LoadFromEntity(const Karte& k, const std::shared_ptr<Entitaet>& ent, LoadOptions options) {   // DEntity.cpp:271-323
    (void)k;
    ClearPatches();
    ClearBrushes();
    ClearEPairs();
    QER_Entity = ent;
    LoadEPairList(*ent);
    if (!ent->istBehaelter()) { return false; }   // node_is_group
    for (const auto& node : ent->kinder()) {
        if (options.loadVisibleOnly && !node->sichtbar()) { continue; }
        if (options.loadSelectedOnly && k.auswahl.gewaehlt(ent.get(), node.get())) { continue; }   // so bei NRC: !( loadSelectedOnly && isSelected )
        if (node->art() == Primitiv::Art::Patch) {
            if (options.loadPatches) { NewPatch()->LoadFromPatch(ent, std::static_pointer_cast<Patch>(node)); }
        } else {
            NewBrush()->LoadFromBrush(ent, std::static_pointer_cast<Pinsel>(node), true);
            if (!options.loadDetail && brushList.back()->IsDetail()) {
                delete brushList.back();
                brushList.pop_back();
            }
        }
    }
    return true;
}

void DEntity::RemoveNonCheckBrushes(const std::vector<std::string>& exclusionList) {   // DEntity.cpp:325-335
    std::erase_if(brushList, [&](DBrush* brush) {
        if (std::any_of(exclusionList.begin(), exclusionList.end(), [brush](const std::string& tex) { return brush->HasTexture(tex.c_str()); })) {
            delete brush;
            return true;
        }
        return false;
    });
}

void DEntity::ResetChecks(const std::vector<std::string>& exclusionList) {
    for (DBrush* brush : brushList) { brush->ResetChecks(exclusionList); }
}

int DEntity::FixBrushes() {   // DEntity.cpp:343-351
    int count = 0;
    for (DBrush* brush : brushList) { count += brush->RemoveRedundantPlanes(); }
    return count;
}

void DEntity::BuildInRadiant(Karte& k, bool allowDestruction, Meldungen& m) {   // DEntity.cpp:353-380
    const bool makeEntity = m_Classname != "worldspawn";
    if (makeEntity) {
        // createEntity( findOrInsert( classname, has brushes ) ), dann setKeyValue fuer jedes Paar
        auto node = k.neueEntitaet(m_Classname, !brushList.empty() || !patchList.empty());
        for (const DEPair& epair : epairList) { node->setzen(epair.key, epair.value); }
        k.entitaetEinfuegen(node);
        for (DBrush* brush : brushList) { (void)brush->BuildInRadiant(k, allowDestruction, nullptr, node, m); }
        for (DPatch* patch : patchList) { (void)patch->BuildInRadiant(k, node); }
        QER_Entity = node;
    } else {
        for (DBrush* brush : brushList) { (void)brush->BuildInRadiant(k, allowDestruction, nullptr, nullptr, m); }
        for (DPatch* patch : patchList) { (void)patch->BuildInRadiant(k, nullptr); }
    }
}

void DEntity::AddEPair(const char* key, const char* value) {   // DEntity.cpp:401-409
    if (DEPair* pair = FindEPairByKey(key)) {
        *pair = DEPair{key, value};
    } else {
        epairList.push_back(DEPair{key, value});
    }
}

void DEntity::LoadEPairList(const Entitaet& e) {   // DEntity.cpp:411-432
    for (const auto& [key, value] : e.schluessel()) {
        if (key == "classname") {
            SetClassname(value.c_str());
        } else {
            AddEPair(key.c_str(), value.c_str());
        }
    }
}

bool DEntity::ResetTextures(Karte& k, const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                            bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation, bool rebuild,
                            Meldungen& m) {   // DEntity.cpp:434-470
    bool reset = false;
    for (DBrush* brush : brushList) {
        const bool tmp =
            brush->ResetTextures(textureName, fScale, fShift, rotation, newTextureName, bResetTextureName, bResetScale, bResetShift, bResetRotation);
        if (tmp) {
            reset = true;
            if (rebuild) {
                const auto e = brush->QER_entity.lock();
                const auto b = brush->QER_brush.lock();
                if (e && b) { primitivLoeschen(k, e, b); }   // Node_getTraversable( *QER_entity )->erase( *QER_brush )
                (void)brush->BuildInRadiant(k, false, nullptr, e, m);
            }
        }
    }
    if (bResetTextureName) {
        for (DPatch* patch : patchList) {
            if (patch->ResetTextures(textureName, newTextureName)) {
                reset = true;
                if (rebuild) {
                    const auto e = patch->QER_entity.lock();
                    const auto b = patch->QER_brush.lock();
                    if (e && b) { primitivLoeschen(k, e, b); }
                    (void)patch->BuildInRadiant(k, e);
                }
            }
        }
    }
    return reset;
}

DEPair* DEntity::FindEPairByKey(const char* keyname) {   // DEntity.cpp:472-480
    for (DEPair& ep : epairList) {
        if (ep.key == keyname) { return &ep; }
    }
    return nullptr;
}

void DEntity::RemoveFromRadiant(Karte& k) {   // DEntity.cpp:482-486
    if (QER_Entity) {
        k.auswahl.abwaehlen(QER_Entity);
        for (const auto& kind : QER_Entity->kinder()) { k.auswahl.abwaehlen(QER_Entity, kind); }
        (void)k.entitaetEntfernen(QER_Entity.get());
    }
    QER_Entity = nullptr;
}

void DEntity::SpawnString(const char* key, const char* defaultstring, const char** out) {   // DEntity.cpp:488-496
    DEPair* pEP = FindEPairByKey(key);
    *out = pEP != nullptr ? pEP->value.c_str() : defaultstring;
}

void DEntity::SpawnVector(const char* key, const char* defaultstring, vec_t* out) {   // DEntity.cpp:518-526
    DEPair* pEP = FindEPairByKey(key);
    (void)std::sscanf(pEP != nullptr ? pEP->value.c_str() : defaultstring, "%f %f %f", &out[0], &out[1], &out[2]);
}

// --- DMap.cpp -----------------------------------------------------------------------

DMap::DMap() { (void)AddEntity("worldspawn", 0); }   // DMap.cpp:30-33
DMap::~DMap() { ClearEntities(); }

DEntity* DMap::AddEntity(const char* classname, int ID) {   // DMap.cpp:39-51
    DEntity* newEntity = ID == -1 ? new DEntity(classname, m_nNextEntity++) : new DEntity(classname, ID);
    entityList.push_back(newEntity);
    return newEntity;
}

void DMap::ClearEntities() {
    m_nNextEntity = 1;
    for (DEntity* entity : entityList) { delete entity; }
    entityList.clear();
}

DEntity* DMap::GetEntityForID(int ID) {   // DMap.cpp:62-77
    for (DEntity* entity : entityList) {
        if (entity->m_nID == ID) { return entity; }
    }
    return AddEntity("worldspawn", ID);
}

void DMap::BuildInRadiant(Karte& k, bool bAllowDestruction, Meldungen& m) {
    for (DEntity* entity : entityList) { entity->BuildInRadiant(k, bAllowDestruction, m); }
}

void DMap::LoadAll(Karte& k, LoadOptions options) {   // DMap.cpp:91-114
    ClearEntities();
    k.auswahl.leeren();   // GlobalSelectionSystem().setSelectedAll( false )
    for (const auto& node : k.entitaeten()) {
        if (!(options.loadVisibleOnly && !node->sichtbar())) { (void)AddEntity("", 0)->LoadFromEntity(k, node, options); }
    }
}

int DMap::FixBrushes() {
    int count = 0;
    for (DEntity* entity : entityList) { count += entity->FixBrushes(); }
    return count;
}

void DMap::ResetTextures(Karte& k, const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                         bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation, Meldungen& m) {   // DMap.cpp:125-148
    for (DEntity* entity : entityList) {
        std::string c = entity->m_Classname;
        for (char& ch : c) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
        if (c == "worldspawn") {
            (void)entity->ResetTextures(k, textureName, fScale, fShift, rotation, newTextureName, bResetTextureName, bResetScale, bResetShift,
                                        bResetRotation, true, m);
        } else if (entity->ResetTextures(k, textureName, fScale, fShift, rotation, newTextureName, bResetTextureName, bResetScale, bResetShift,
                                         bResetRotation, false, m)) {
            RebuildEntity(k, entity, m);
        }
    }
}

void DMap::RebuildEntity(Karte& k, DEntity* ent, Meldungen& m) {   // DMap.cpp:150-153
    ent->RemoveFromRadiant(k);
    ent->BuildInRadiant(k, false, m);
}

// --- DShape.cpp ---------------------------------------------------------------------

const bool bFacesAll[6] = {true, true, true, true, true, true};

void DShape::BuildRegularPrism(vec3_t min, vec3_t max, int nSides, bool bAlignTop) {   // DShape.cpp:43-90
    vec3_t vc[MAX_POLYGON_FACES + 2];
    vec3_t vd[MAX_POLYGON_FACES + 2];
    vec3_t radius;
    vec3_t origin;
    VectorSubtract(max, min, radius);
    VectorScale(radius, 0.5F, radius);
    VectorAdd(max, min, origin);
    VectorScale(origin, 0.5F, origin);
    float phase = 0;
    if (bAlignTop) {
        phase = -static_cast<float>(Q_PI / nSides);
        VectorScale(radius, static_cast<vec_t>(1.0 / std::cos(phase)), radius);
    }
    for (int i = 0; i < nSides; ++i) {
        VectorCopy(origin, vc[i]);
        VectorCopy(origin, vd[i]);
        vc[i][2] = min[2];
        vd[i][2] = max[2];
        vc[i][0] += radius[0] * std::sin(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vc[i][1] += radius[1] * std::cos(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vd[i][0] = vc[i][0];
        vd[i][1] = vc[i][1];
    }
    VectorCopy(vc[0], vc[nSides]);
    VectorCopy(vd[0], vd[nSides]);
    VectorCopy(vc[1], vc[nSides + 1]);
    VectorCopy(vd[1], vd[nSides + 1]);
    DBrush* pB = m_Container.GetWorldSpawn()->NewBrush();
    for (int i = 1; i <= nSides; ++i) { (void)pB->AddFace(vc[i - 1], vc[i], vd[i], m_textur.c_str(), false); }
    (void)pB->AddFace(vc[2], vc[1], vc[0], "textures/common/caulk", false);
    (void)pB->AddFace(vd[0], vd[1], vd[2], "textures/common/caulk", false);
}

void DShape::Commit(Karte& k, Meldungen& m) {   // DShape.cpp:92-95
    (void)m_Container.GetWorldSpawn()->FixBrushes();
    m_Container.BuildInRadiant(k, true, m);
}

void DShape::BuildInversePrism(vec3_t min, vec3_t max, int nSides, bool bAlignTop) {   // DShape.cpp:97-153
    vec3_t va[MAX_POLYGON_FACES + 1];
    vec3_t vb[MAX_POLYGON_FACES + 1];
    vec3_t radius;
    vec3_t origin;
    VectorSubtract(max, min, radius);
    VectorScale(radius, 0.5F, radius);
    VectorAdd(max, min, origin);
    VectorScale(origin, 0.5F, origin);
    float phase = 0;
    if (bAlignTop) {
        phase = -static_cast<float>(Q_PI / nSides);
        VectorScale(radius, static_cast<vec_t>(1.0 / std::cos(phase)), radius);
    }
    for (int i = 0; i < nSides; ++i) {
        VectorCopy(origin, va[i]);
        VectorCopy(origin, vb[i]);
        va[i][2] = min[2];
        vb[i][2] = max[2];
        va[i][0] += radius[0] * std::sin(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        va[i][1] += radius[1] * std::cos(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vb[i][0] = va[i][0];
        vb[i][1] = va[i][1];
    }
    VectorCopy(va[0], va[nSides]);
    VectorCopy(vb[0], vb[nSides]);
    for (int i = 1; i <= nSides; ++i) {
        DBrush* pB = GetBoundingCube(min, max, "textures/common/caulk");
        vec3_t top;
        vec3_t bottom;
        VectorCopy(va[i - 1], top);
        VectorCopy(va[i], bottom);
        if (va[i - 1][1] > va[i][1]) {
            top[0] += 5;
            bottom[0] += 5;
        } else {   // flip direction of plane on crossover
            top[0] -= 5;
            bottom[0] -= 5;
        }
        if (top[1] != bottom[1]) {   // internal line is flat already if true
            (void)pB->AddFace(va[i - 1], top, vb[i - 1], "textures/common/caulk", false);
            (void)pB->AddFace(va[i], vb[i], bottom, "textures/common/caulk", false);
        }
        (void)pB->AddFace(va[i - 1], vb[i - 1], vb[i], m_textur.c_str(), false);
    }
}

void DShape::BuildBorderedPrism(vec3_t min, vec3_t max, int nSides, int nBorder, bool bAlignTop) {   // DShape.cpp:155-218
    vec3_t va[MAX_POLYGON_FACES + 2];
    vec3_t vb[MAX_POLYGON_FACES + 2];
    vec3_t vc[MAX_POLYGON_FACES + 2];
    vec3_t vd[MAX_POLYGON_FACES + 2];
    vec3_t radius;
    vec3_t origin;
    VectorSubtract(max, min, radius);
    VectorScale(radius, 0.5F, radius);
    VectorAdd(max, min, origin);
    VectorScale(origin, 0.5F, origin);
    if (static_cast<float>(nBorder) >= Min(radius[0], radius[1])) { return; }
    float phase = 0;
    if (bAlignTop) {
        phase = -static_cast<float>(Q_PI / nSides);
        VectorScale(radius, static_cast<vec_t>(1.0 / std::cos(phase)), radius);
    }
    for (int i = 0; i < nSides; ++i) {
        VectorCopy(origin, va[i]);
        VectorCopy(origin, vb[i]);
        VectorCopy(origin, vc[i]);
        VectorCopy(origin, vd[i]);
        va[i][2] = min[2];
        vb[i][2] = max[2];
        va[i][0] += (radius[0] - static_cast<float>(nBorder)) * std::sin(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        va[i][1] += (radius[1] - static_cast<float>(nBorder)) * std::cos(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vb[i][0] = va[i][0];
        vb[i][1] = va[i][1];
        vc[i][2] = min[2];
        vd[i][2] = max[2];
        vc[i][0] += radius[0] * std::sin(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vc[i][1] += radius[1] * std::cos(static_cast<float>((2 * Q_PI * i / nSides) + phase));
        vd[i][0] = vc[i][0];
        vd[i][1] = vc[i][1];
    }
    VectorCopy(va[0], va[nSides]);
    VectorCopy(vb[0], vb[nSides]);
    VectorCopy(va[1], va[nSides + 1]);
    VectorCopy(vb[1], vb[nSides + 1]);
    VectorCopy(vc[0], vc[nSides]);
    VectorCopy(vd[0], vd[nSides]);
    VectorCopy(vc[1], vc[nSides + 1]);
    VectorCopy(vd[1], vd[nSides + 1]);
    for (int i = 1; i <= nSides; ++i) {
        DBrush* pB = GetBoundingCube(min, max, "textures/common/caulk");
        (void)pB->AddFace(origin, vc[i - 1], vd[i - 1], "textures/common/caulk", false);
        (void)pB->AddFace(origin, vd[i], vc[i], "textures/common/caulk", false);
        (void)pB->AddFace(vc[i - 1], vc[i], vd[i], m_textur.c_str(), false);
        (void)pB->AddFace(vb[i], va[i], va[i - 1], m_textur.c_str(), false);
    }
}

DBrush* DShape::GetBoundingCube_Ext(vec3_t min, vec3_t max, const char* textureName, const bool* bUseFaces, bool detail) {   // DShape.cpp:220-262
    auto* pB = new DBrush;
    vec3_t v1;
    vec3_t v2;
    vec3_t v3;
    vec3_t v5;
    vec3_t v6;
    vec3_t v7;
    VectorCopy(min, v1);
    VectorCopy(min, v2);
    VectorCopy(min, v3);
    VectorCopy(max, v5);
    VectorCopy(max, v6);
    VectorCopy(max, v7);
    v2[0] = max[0];
    v3[1] = max[1];
    v6[0] = min[0];
    v7[1] = min[1];
    if (bUseFaces[0]) { (void)pB->AddFace(v1, v2, v3, textureName, detail); }
    if (bUseFaces[1]) { (void)pB->AddFace(v1, v3, v6, textureName, detail); }
    if (bUseFaces[2]) { (void)pB->AddFace(v1, v7, v2, textureName, detail); }
    if (bUseFaces[3]) { (void)pB->AddFace(v5, v6, v3, textureName, detail); }
    if (bUseFaces[4]) { (void)pB->AddFace(v5, v2, v7, textureName, detail); }
    if (bUseFaces[5]) { (void)pB->AddFace(v5, v7, v6, textureName, detail); }
    return pB;
}

DBrush* DShape::GetBoundingCube(vec3_t min, vec3_t max, const char* textureName, DEntity* ent, const bool* bUseFaces) {   // DShape.cpp:264-306
    if (bUseFaces == nullptr) { bUseFaces = bFacesAll; }
    DBrush* pB = ent == nullptr ? m_Container.GetWorldSpawn()->NewBrush() : ent->NewBrush();
    vec3_t v1;
    vec3_t v2;
    vec3_t v3;
    vec3_t v5;
    vec3_t v6;
    vec3_t v7;
    VectorCopy(min, v1);
    VectorCopy(min, v2);
    VectorCopy(min, v3);
    VectorCopy(max, v5);
    VectorCopy(max, v6);
    VectorCopy(max, v7);
    v2[0] = max[0];
    v3[1] = max[1];
    v6[0] = min[0];
    v7[1] = min[1];
    if (bUseFaces[0]) { (void)pB->AddFace(v1, v2, v3, textureName, false); }
    if (bUseFaces[1]) { (void)pB->AddFace(v1, v3, v6, textureName, false); }
    if (bUseFaces[2]) { (void)pB->AddFace(v1, v7, v2, textureName, false); }
    if (bUseFaces[3]) { (void)pB->AddFace(v5, v6, v3, textureName, false); }
    if (bUseFaces[4]) { (void)pB->AddFace(v5, v2, v7, textureName, false); }
    if (bUseFaces[5]) { (void)pB->AddFace(v5, v7, v6, textureName, false); }
    return pB;
}

bool DShape::BuildPit(vec3_t min, vec3_t max, const Umgebung& u) {   // DShape.cpp:308-388
    if ((max[2] - min[2]) < 196) { return false; }
    if (u.zufallStart) { u.zufallStart(static_cast<unsigned>(u.zeit ? u.zeit() : 0)); }   // srand( time( nullptr ) )
    vec3_t centre;
    VectorAdd(min, max, centre);
    VectorScale(centre, 0.5F, centre);
    char buffer[256];
    const int team = ((u.zufall ? u.zufall() : 0) % 10000) + 5000;
    std::snprintf(buffer, sizeof(buffer), "t%i_1", team);
    vec3_t triggerVoiceBtm;
    VectorCopy(min, triggerVoiceBtm);
    triggerVoiceBtm[2] = max[2] - 16;
    DEntity* triggerVoice = m_Container.AddEntity("trigger_multiple");
    (void)GetBoundingCube(triggerVoiceBtm, max, "textures/common/trigger", triggerVoice);
    triggerVoice->AddEPair("target", buffer);
    vec3_t voiceOrigin;
    VectorCopy(centre, voiceOrigin);
    voiceOrigin[2] = max[2] + 16;
    DEntity* targetVoice = m_Container.AddEntity("target_speaker");
    targetVoice->AddEPair("targetname", buffer);
    std::snprintf(buffer, sizeof(buffer), "%f %f %f", voiceOrigin[0], voiceOrigin[1], voiceOrigin[2]);
    targetVoice->AddEPair("origin", buffer);
    targetVoice->AddEPair("spawnflags", "8");
    targetVoice->AddEPair("noise", "*falling1.wav");
    std::snprintf(buffer, sizeof(buffer), "t%i_2", team);
    vec3_t triggerPwrRmvTop;
    vec3_t triggerPwrRmvBtm;
    VectorCopy(min, triggerPwrRmvBtm);
    VectorCopy(max, triggerPwrRmvTop);
    triggerPwrRmvTop[2] = triggerVoiceBtm[2] - 64;
    triggerPwrRmvBtm[2] = triggerPwrRmvTop[2] - 16;
    DEntity* triggerPwrRmv = m_Container.AddEntity("trigger_multiple");
    (void)GetBoundingCube(triggerPwrRmvBtm, triggerPwrRmvTop, "textures/common/trigger", triggerPwrRmv);
    triggerPwrRmv->AddEPair("target", buffer);
    vec3_t pwrRmvOrigin;
    VectorCopy(centre, pwrRmvOrigin);
    pwrRmvOrigin[2] = triggerPwrRmvTop[2] + 16;
    DEntity* targetPwrRmv = m_Container.AddEntity("target_remove_powerups");
    targetPwrRmv->AddEPair("targetname", buffer);
    std::snprintf(buffer, sizeof(buffer), "%f %f %f", pwrRmvOrigin[0], pwrRmvOrigin[1], pwrRmvOrigin[2]);
    targetPwrRmv->AddEPair("origin", buffer);
    vec3_t triggerDmgTop;
    vec3_t triggerDmgBtm;
    VectorCopy(min, triggerDmgBtm);
    VectorCopy(max, triggerDmgTop);
    triggerDmgBtm[2] = min[2] + 64;
    triggerDmgTop[2] = triggerDmgBtm[2] + 16;
    DEntity* triggerDmg = m_Container.AddEntity("trigger_hurt");
    (void)GetBoundingCube(triggerDmgBtm, triggerDmgTop, "textures/common/trigger", triggerDmg);
    triggerDmg->AddEPair("dmg", "9999");
    triggerDmg->AddEPair("spawnflags", "12");
    vec3_t nodropTop;
    VectorCopy(max, nodropTop);
    nodropTop[2] = min[2] + 64;
    (void)GetBoundingCube(min, nodropTop, "textures/common/nodrop");
    return true;
}

// --- Szene ------------------------------------------------------------------------

std::vector<Gewaehlt> auswahlListe(const Karte& k) {
    std::vector<Gewaehlt> v;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        Gewaehlt g{a.entitaet.lock(), a.primitiv.lock()};
        if (g.entitaet) { v.push_back(std::move(g)); }
    }
    return v;
}

void primitivLoeschen(Karte& k, const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p) {
    k.auswahl.abwaehlen(e, p);
    (void)e->kindEntfernen(p.get());
}

void grenzenVon(const Primitiv& p, const Kontext& k, vec_t* min, vec_t* max) {   // instance.worldAABB() (origin -/+ extents)
    Kasten b;
    if (p.art() == Primitiv::Art::Pinsel) {
        b = static_cast<const Pinsel&>(p).grenzen(k);
    } else {
        b = static_cast<const Patch&>(p).grenzen();
    }
    for (int i = 0; i < 3; ++i) {
        min[i] = b.origin[static_cast<std::size_t>(i)] - b.extents[static_cast<std::size_t>(i)];
        max[i] = b.origin[static_cast<std::size_t>(i)] + b.extents[static_cast<std::size_t>(i)];
    }
}

}  // namespace bhed::karte::bobtoolz::d
