// bobtoolz_d.h - Karten-Editor, Stufe 4: die Hilfsklassen von bobToolz
// (DPoint, DEPair, DPlane, DWinding, DBrush, DPatch, DEntity, DMap, DShape)
// und libs/mathlib in float wie bobToolz - intern fuer bobtoolz*.cpp.
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz: DPoint.h/.cpp,
// DEPair.h/.cpp, DPlane.h/.cpp, DWinding.h/.cpp, DBrush.h/.cpp, DPatch.h/.cpp,
// DEntity.h/.cpp, DMap.h/.cpp, DShape.h/.cpp, misc.h/.cpp; libs/mathlib.h,
// libs/mathlib/mathlib.c (vec_t = float, VectorNormalize = VectorAccurateNormalize).
// Die Verbindung zur Szene (Brush_forEachFace, Brush_addFace, createBrush,
// createEntity, Path_deleteTop ...) geht hier an bhed::karte::Karte.
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
#ifndef BHED_KARTE_BOBTOOLZ_D_H
#define BHED_KARTE_BOBTOOLZ_D_H

#include "bhed/karte/bobtoolz.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <array>
#include <cmath>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte::bobtoolz::d {

// --- libs/mathlib (float) -------------------------------------------------------
using vec_t = float;
using vec3_t = vec_t[3];
using vec3_accu_t = double[3];
constexpr double Q_PI = 3.14159265358979323846;
constexpr float EQUAL_EPSILON = 0.001F;   // mathlib.h:69
constexpr float MAX_ROUND_ERROR = 0.05F;  // misc.h:27 (#define 0.05: double, aber mit float verglichen)
constexpr int MAX_POINTS_ON_WINDING = 256;   // DWinding.h:63
constexpr int MAX_POLYGON_FACES = 128;       // DShape.h:33
constexpr int POINT_IN_BRUSH = 0;            // DBrush.h:43-45
constexpr int POINT_ON_BRUSH = 1;
constexpr int POINT_OUT_BRUSH = 2;
constexpr int SIDE_FRONT = 0;                // DWinding.h
constexpr int SIDE_BACK = 1;
constexpr int SIDE_ON = 2;
constexpr int FACE_DETAIL = 0x8000000;       // qerplugin.h: CONTENTS_DETAIL

inline vec_t DotProduct(const vec_t* a, const vec_t* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
inline void VectorSubtract(const vec_t* a, const vec_t* b, vec_t* c) {
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
}
inline void VectorAdd(const vec_t* a, const vec_t* b, vec_t* c) {
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
}
inline void VectorCopy(const vec_t* a, vec_t* b) {
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}
inline void VectorCopy(const double* a, vec_t* b) {
    b[0] = static_cast<float>(a[0]);
    b[1] = static_cast<float>(a[1]);
    b[2] = static_cast<float>(a[2]);
}
inline void VectorCopy(const vec_t* a, double* b) {
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}
inline void VectorScale(const vec_t* v, vec_t s, vec_t* o) {
    o[0] = v[0] * s;
    o[1] = v[1] * s;
    o[2] = v[2] * s;
}
inline void VectorClear(vec_t* v) { v[0] = v[1] = v[2] = 0; }
inline void VectorNegate(const vec_t* a, vec_t* b) {
    b[0] = -a[0];
    b[1] = -a[1];
    b[2] = -a[2];
}
inline void VectorSet(vec_t* v, vec_t x, vec_t y, vec_t z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
}
inline void VectorMid(const vec_t* a, const vec_t* b, vec_t* c) {   // mathlib.h VectorMid
    c[0] = (a[0] + b[0]) * 0.5F;
    c[1] = (a[1] + b[1]) * 0.5F;
    c[2] = (a[2] + b[2]) * 0.5F;
}
inline vec_t VectorMax(const vec_t* x) {   // mathlib.h:82 (groesste Komponente, ohne Betrag)
    return (x[0] > x[1]) ? ((x[0] > x[2]) ? x[0] : x[2]) : ((x[1] > x[2]) ? x[1] : x[2]);
}
inline void CrossProduct(const vec_t* a, const vec_t* b, vec_t* c) {
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}
inline void VectorMA(const vec_t* va, vec_t scale, const vec_t* vb, vec_t* vc) {
    vc[0] = va[0] + scale * vb[0];
    vc[1] = va[1] + scale * vb[1];
    vc[2] = va[2] + scale * vb[2];
}
vec_t VectorLength(const vec_t* v);                        // mathlib.c:95-100
vec_t VectorNormalize(const vec_t* in, vec_t* out);        // VectorAccurateNormalize (mathlib.c:150-168)
bool VectorCompare(const vec_t* a, const vec_t* b);        // mathlib.c:102-108
vec_t Min(vec_t a, vec_t b);                               // misc.cpp:320-325
void MakeNormal(const vec_t* va, const vec_t* vb, const vec_t* vc, vec_t* out);   // misc.cpp:327-332
float Determinant3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3);   // misc.cpp:300-304
void MoveBlock(int dir, vec3_t min, vec3_t max, float dist);              // misc.cpp:66-94
void SetInitialStairPos(int dir, vec3_t min, vec3_t max, float width);    // misc.cpp:96-120

// --- _QERFaceData (include/ibrush.h:97-110) und texdef_t ------------------------
struct FaceData {
    double p0[3] = {0, 0, 0};
    double p1[3] = {0, 0, 0};
    double p2[3] = {0, 0, 0};
    Texdef texdef;   // texdef_t (shift 0, rotate 0, scale 1)
    std::string shader;
    int contents = 0;
    int flags = 0;
    int value = 0;
};
// misc.cpp:280-298
void FillDefaultTexture(FaceData* faceData, const double* va, const double* vb, const double* vc, const char* texture);

// Brush_addFace (brushmodule.cpp:237-240): addPlane mit TextureProjection(
// m_texdef, brushprimit_texdef_t() ) - in Brush Primitives also die
// Vorgabe-Matrix, Inhalt/Flags kommen nicht mit.
void BrushAddFace(Pinsel& p, const FaceData& f, const Kontext& k);

// --- DPoint, DEPair ---------------------------------------------------------------
struct DPoint {
    vec3_t _pnt = {0, 0, 0};
    bool operator==(const vec_t* other) const;   // DPoint.cpp
};
struct DEPair {
    std::string key, value;
};

class DWinding;

// --- DPlane -----------------------------------------------------------------------
class DPlane {
public:
    DPlane() = default;
    DPlane(const vec_t* va, const vec_t* vb, const vec_t* vc, const FaceData* texData);
    DPlane(const vec_t* va, const vec_t* vb, const vec_t* vc, const char* textureName, bool bDetail);
    [[nodiscard]] vec_t DistanceToPoint(const vec_t* pnt) const;
    bool PlaneIntersection(const DPlane* pl1, const DPlane* pl2, vec_t* out) const;
    bool IsRedundant(const std::list<DPoint*>& pointList) const;
    bool operator==(const DPlane& other) const;
    bool operator!=(const DPlane& other) const;
    [[nodiscard]] DWinding BaseWindingForPlane() const;
    void Rebuild();
    bool AddToBrush(Pinsel& brush, const Kontext& k);

    vec3_t normal = {0, 0, 0};
    vec_t _d = 0;
    vec3_t points[3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    bool m_bChkOk = false;
    FaceData texInfo;
    std::string m_shader;
};

// --- DWinding ---------------------------------------------------------------------
class DWinding {
public:
    DWinding() = default;
    DWinding(const DWinding& o) { *this = o; }
    DWinding& operator=(const DWinding& o) {
        if (this != &o) {
            numpoints = o.numpoints;
            p = o.p;
        }
        return *this;
    }
    DWinding(DWinding&&) noexcept = default;
    DWinding& operator=(DWinding&&) noexcept = default;
    void AllocWinding(int points);
    vec_t WindingArea() const;
    void RemoveColinearPoints();
    DPlane* WindingPlane() const;
    void WindingCentroid(vec_t* centroid) const;
    bool ChopWindingInPlace(const DPlane* chopPlane, vec_t epsilon);
    int numpoints = 0;
    std::vector<std::array<vec_t, 3>> p;
};

// --- DBrush -----------------------------------------------------------------------
class DBrush {
public:
    DBrush() = default;
    ~DBrush();
    DBrush(const DBrush&) = delete;
    DBrush& operator=(const DBrush&) = delete;
    DPlane* AddFace(const vec_t* va, const vec_t* vb, const vec_t* vc, const FaceData* texData);
    DPlane* AddFace(const vec_t* va, const vec_t* vb, const vec_t* vc, const char* textureName, bool bDetail);
    int BuildPoints();
    void LoadFromBrush(const std::shared_ptr<Entitaet>& entity, const std::shared_ptr<Pinsel>& brush, bool textured);
    int PointPosition(const vec_t* pnt) const;
    void ClearPoints();
    void ClearFaces();
    void AddPoint(const vec_t* pnt);
    bool HasPoint(const vec_t* pnt) const;
    int RemoveRedundantPlanes();
    bool GetBounds(vec_t* min, vec_t* max);
    bool BBoxCollision(DBrush* chkBrush);
    DPlane* HasPlane(const DPlane* chkPlane) const;
    bool IsCutByPlane(const DPlane* cuttingPlane);
    // BuildInRadiant: neuer Pinsel in "entity" (nullptr: worldspawn)
    std::shared_ptr<Pinsel> BuildInRadiant(Karte& k, bool allowDestruction, int* changeCnt, const std::shared_ptr<Entitaet>& entity,
                                           Meldungen& m);
    void selectInRadiant(Karte& k) const;
    bool IntersectsWith(DBrush* chkBrush);
    bool IntersectsWith(const DPlane* p1, const DPlane* p2, vec_t* v) const;
    void BuildBounds();
    void ResetChecks(const std::vector<std::string>& exclusionList);
    bool HasTexture(const char* textureName) const;
    bool IsDetail() const;
    bool ResetTextures(const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                       bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation);
    bool operator==(const DBrush* other) const;
    DPlane* FindPlaneWithClosestNormal(const vec_t* normal) const;
    int FindPointsForPlane(DPlane* plane, DPoint** pnts, int maxpnts);

    std::list<DPlane*> faceList;
    std::list<DPoint*> pointList;
    bool bBoundsBuilt = false;
    vec3_t bbox_min = {0, 0, 0};
    vec3_t bbox_max = {0, 0, 0};
    std::weak_ptr<Entitaet> QER_entity;   // brush.path().parent()
    std::weak_ptr<Pinsel> QER_brush;      // brush.path().top()
};

// --- DPatch -----------------------------------------------------------------------
struct drawVert_t {
    float xyz[3] = {0, 0, 0};
    float st[2] = {0, 0};
    float normal[3] = {0, 0, 0};
    float lightmap[2] = {0, 0};
};
struct patch_merge_t {
    bool mergable = false;
    int pos1 = 0;
    int pos2 = 0;
};
constexpr int MAX_PATCH_WIDTH = 32;   // DPatch.h
constexpr int MAX_PATCH_HEIGHT = 32;
constexpr int MIN_PATCH_WIDTH = 3;
constexpr int MIN_PATCH_HEIGHT = 3;

class DPatch {
public:
    std::list<DPatch> SplitRows();
    std::list<DPatch> SplitCols();
    std::list<DPatch> Split(Meldungen& m);
    std::list<DPatch> SplitRows(Meldungen& m);
    std::list<DPatch> SplitCols(Meldungen& m);
    void Transpose();
    void Invert();
    DPatch* MergePatches(patch_merge_t merge_info, DPatch& p1, DPatch& p2);
    patch_merge_t IsMergable(const DPatch& other) const;
    bool ResetTextures(const char* oldTextureName, const char* newTextureName);
    // width = Zeilen des Patches, height = Spalten (Patch_getControlPoints liefert
    // PatchControlMatrix( getHeight(), getWidth() ) - points[x][y] = Zeile x, Spalte y)
    void LoadFromPatch(const std::shared_ptr<Entitaet>& entity, const std::shared_ptr<Patch>& patch);
    std::shared_ptr<Patch> BuildInRadiant(Karte& k, const std::shared_ptr<Entitaet>& entity);
    void SetTexture(const std::string& textureName) { texture = textureName; }

    std::weak_ptr<Entitaet> QER_entity;
    std::weak_ptr<Patch> QER_brush;
    std::string texture;
    int width = MIN_PATCH_WIDTH, height = MIN_PATCH_HEIGHT;
    std::array<std::array<drawVert_t, MAX_PATCH_HEIGHT>, MAX_PATCH_WIDTH> points{};
};

// --- DEntity ----------------------------------------------------------------------
struct LoadOptions {   // DEntity.h
    bool loadDetail = true;
    bool loadPatches = false;
    bool loadVisibleOnly = false;
    bool loadSelectedOnly = false;
};

class DEntity {
public:
    explicit DEntity(const char* classname = "worldspawn", int ID = -1);
    ~DEntity();
    DEntity(const DEntity&) = delete;
    DEntity& operator=(const DEntity&) = delete;
    void ClearBrushes();
    void ClearPatches();
    DPatch* NewPatch();
    DBrush* NewBrush();
    void LoadSelectedBrushes(const Karte& k);
    void LoadSelectedPatches(const Karte& k);
    std::vector<bool> BuildIntersectList();
    std::vector<bool> BuildDuplicateList();
    void SelectBrushes(Karte& k, const std::vector<bool>& selectList);
    bool LoadFromEntity(const Karte& k, const std::shared_ptr<Entitaet>& ent, LoadOptions options = {});
    void RemoveNonCheckBrushes(const std::vector<std::string>& exclusionList);
    void ResetChecks(const std::vector<std::string>& exclusionList);
    int FixBrushes();
    void BuildInRadiant(Karte& k, bool allowDestruction, Meldungen& m);
    void SetClassname(const char* classname) { m_Classname = classname; }
    void ClearEPairs() { epairList.clear(); }
    void AddEPair(const char* key, const char* value);
    void LoadEPairList(const Entitaet& e);
    bool ResetTextures(Karte& k, const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                       bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation, bool rebuild,
                       Meldungen& m);
    DEPair* FindEPairByKey(const char* keyname);
    void RemoveFromRadiant(Karte& k);
    void SpawnString(const char* key, const char* defaultstring, const char** out);
    void SpawnVector(const char* key, const char* defaultstring, vec_t* out);

    std::vector<DBrush*> brushList;
    std::list<DPatch*> patchList;
    std::list<DEPair> epairList;
    std::shared_ptr<Entitaet> QER_Entity;
    int m_nID = -1;
    std::string m_Classname;
};

// --- DMap -------------------------------------------------------------------------
class DMap {
public:
    DMap();
    ~DMap();
    DMap(const DMap&) = delete;
    DMap& operator=(const DMap&) = delete;
    DEntity* AddEntity(const char* classname = "worldspawn", int ID = -1);
    void ClearEntities();
    DEntity* GetEntityForID(int ID);
    DEntity* GetWorldSpawn() { return GetEntityForID(0); }
    void BuildInRadiant(Karte& k, bool bAllowDestruction, Meldungen& m);
    void LoadAll(Karte& k, LoadOptions options = {});
    int FixBrushes();
    void ResetTextures(Karte& k, const char* textureName, const float fScale[2], const float fShift[2], int rotation, const char* newTextureName,
                       bool bResetTextureName, const bool bResetScale[2], const bool bResetShift[2], bool bResetRotation, Meldungen& m);
    void RebuildEntity(Karte& k, DEntity* ent, Meldungen& m);

    std::list<DEntity*> entityList;
    int m_nNextEntity = 1;
};

// --- DShape -----------------------------------------------------------------------
class DShape {
public:
    explicit DShape(std::string aktuelleTextur) : m_textur(std::move(aktuelleTextur)) {}
    void BuildRegularPrism(vec3_t min, vec3_t max, int nSides, bool bAlignTop);
    void BuildInversePrism(vec3_t min, vec3_t max, int nSides, bool bAlignTop);
    void BuildBorderedPrism(vec3_t min, vec3_t max, int nSides, int nBorder, bool bAlignTop);
    bool BuildPit(vec3_t min, vec3_t max, const Umgebung& u);
    void Commit(Karte& k, Meldungen& m);
    static DBrush* GetBoundingCube_Ext(vec3_t min, vec3_t max, const char* textureName, const bool* bUseFaces, bool detail);
    DBrush* GetBoundingCube(vec3_t min, vec3_t max, const char* textureName, DEntity* ent = nullptr, const bool* bUseFaces = nullptr);

    DMap m_Container;
    std::string m_textur;   // GetCurrentTexture()
};
extern const bool bFacesAll[6];

// Szene: die Auswahl als (Entitaet, Primitiv) wie GlobalSelectionSystem().foreachSelected
struct Gewaehlt {
    std::shared_ptr<Entitaet> entitaet;
    std::shared_ptr<Primitiv> primitiv;   // nullptr: die Entitaet selbst
};
[[nodiscard]] std::vector<Gewaehlt> auswahlListe(const Karte& k);
// Path_deleteTop: das Primitiv aus seiner Entitaet entfernen (mit Undo, abgewaehlt)
void primitivLoeschen(Karte& k, const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p);
// instance.worldAABB() eines Primitivs
void grenzenVon(const Primitiv& p, const Kontext& k, vec_t* min, vec_t* max);

}  // namespace bhed::karte::bobtoolz::d

#endif
