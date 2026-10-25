// bobtoolz.cpp - Karten-Editor, Stufe 4: die Befehle von bobToolz (siehe
// bobtoolz.h).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz:
// bobToolz-GTK.cpp (PLUGIN_COMMANDS, PLUGIN_ABOUT), funchandlers-GTK.cpp
// (Do*), shapes.cpp (Build_*, BuildDoorsX2, BuildApertureDoors,
// BuildCornerStairs, MakeBevel), DTreePlanter.cpp (mouseDown, FindDropPoint,
// DropEntsToGround, MakeChain), DTrainDrawer.cpp (BuildPaths,
// CalculateSpline_r), DBobView.cpp (CalculateTrajectory, UpdatePath,
// DBobView_setEntity), lists.cpp, misc.cpp (GetEntityCentre).
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
#include <limits>
#include <sstream>

namespace bhed::karte::bobtoolz {

using namespace d;

// bobToolz-GTK.cpp:44-76
const char* const kPluginBefehle = "About...,-,Stair Builder...,Aperture Door...,Door Builder...,Find Duplicates,Intersect...,Make Chain...,"
                                   "Path Plotter...,-,Reset Textures...,PitOMatic,-,Vis Viewer,Brush Cleanup,Polygon Builder,Caulk Selection,-,"
                                   "Explode Entity,Tree Planter,Drop Entity,Plot Splines,-,Merge Patches,Split patch,Split patch columns,"
                                   "Split patch rows,Flip terrain";
// bobToolz-GTK.cpp:81-85 (PLUGIN_ABOUT, ohne die HTML-Verweise)
const char* const kAbout = "bobToolz for SDRadiant\nby digibob (digibob@splashdamage.com)\nhttp://www.splashdamage.com\n\n"
                           "Additional Contributors:\nMarsMattel, RR2DO2";

namespace {

void aus(Meldungen& m, const std::string& t) { m.push_back({0, t}); }
void warn(Meldungen& m, const std::string& t) { m.push_back({1, t}); }
void fehl(Meldungen& m, const std::string& t) { m.push_back({2, t}); }

int zufall(const Umgebung& u) { return u.zufall ? u.zufall() : 0; }
void zufallStart(const Umgebung& u) {
    if (u.zufallStart) { u.zufallStart(static_cast<unsigned>(u.zeit ? u.zeit() : 0)); }
}

// instance.worldAABB() als vMin/vMax
void grenzen(const Karte& k, const Primitiv& p, vec3_t vMin, vec3_t vMax) { grenzenVon(p, k.kontext, vMin, vMax); }

// Ein neuer Pinsel mit Flaechen (createBrush + AddFaceWithTexture), am Ende
// in worldspawn oder die Entitaet (Node_getTraversable( ... )->insert).
struct NeuerPinsel {
    std::shared_ptr<Pinsel> p = std::make_shared<Pinsel>();
};

// AddFaceWithTexture (shapes.cpp:77-87): FillDefaultTexture, detail nur in
// contents (Brush_addFace nimmt es nicht mit)
void AddFaceWithTexture(Karte& k, NeuerPinsel& brush, const double* va, const double* vb, const double* vc, const char* texture, bool detail) {
    FaceData faceData;
    FillDefaultTexture(&faceData, va, vb, vc, texture);
    if (detail) { faceData.contents |= FACE_DETAIL; }
    BrushAddFace(*brush.p, faceData, k.kontext);
}
void AddFaceWithTexture(Karte& k, NeuerPinsel& brush, const vec_t* va, const vec_t* vb, const vec_t* vc, const char* texture, bool detail) {
    const double a[3] = {va[0], va[1], va[2]};
    const double b[3] = {vb[0], vb[1], vb[2]};
    const double c[3] = {vc[0], vc[1], vc[2]};
    AddFaceWithTexture(k, brush, a, b, c, texture, detail);
}

void einfuegen(Karte& k, NeuerPinsel& brush, const std::shared_ptr<Entitaet>& ziel = nullptr) {
    brush.p->schicht = static_cast<int>(k.aktuelleSchicht);
    (ziel ? ziel : k.weltHolenOderAnlegen())->kindEinfuegen(brush.p);
}

// AddFaceWithTextureScaled (shapes.cpp:89-139)
void AddFaceWithTextureScaled(Karte& k, NeuerPinsel& brush, vec3_t va, vec3_t vb, vec3_t vc, const char* texture, bool bVertScale, bool bHorScale,
                              float minX, float minY, float maxX, float maxY, const Umgebung& u, Meldungen& m) {
    const std::pair<int, int> g = u.texturGroesse ? u.texturGroesse(texture) : std::pair<int, int>{0, 0};
    if (g.first > 0 && g.second > 0) {   // pqtTexInfo
        float scale[2] = {0.5F, 0.5F};
        float shift[2] = {0, 0};
        if (bHorScale) {
            const float width = maxX - minX;
            scale[0] = width / static_cast<float>(g.first);
            shift[0] = -static_cast<float>(static_cast<int>(maxX) % static_cast<int>(width)) / scale[0];
        }
        if (bVertScale) {
            const float height = maxY - minY;
            scale[1] = height / static_cast<float>(g.second);
            shift[1] = static_cast<float>(static_cast<int>(minY) % static_cast<int>(height)) / scale[1];
        }
        FaceData addFace;
        const double a[3] = {va[0], va[1], va[2]};
        const double b[3] = {vb[0], vb[1], vb[2]};
        const double c[3] = {vc[0], vc[1], vc[2]};
        FillDefaultTexture(&addFace, a, b, c, texture);
        addFace.texdef.scale[0] = scale[0];
        addFace.texdef.scale[1] = scale[1];
        addFace.texdef.shift[0] = shift[0];
        addFace.texdef.shift[1] = shift[1];
        BrushAddFace(*brush.p, addFace, k.kontext);
    } else {
        AddFaceWithTexture(k, brush, va, vb, vc, texture, false);
        fehl(m, std::string("BobToolz::Invalid Texture Name-> ") + texture);
    }
}

// --- shapes.cpp ---------------------------------------------------------------------

void Build_Wedge(Karte& k, int dir, vec3_t min, vec3_t max, bool bUp) {   // shapes.cpp:144-241
    NeuerPinsel newBrush;
    vec3_t v1, v2, v3, v5, v6, v7, v8;
    VectorCopy(min, v1);
    VectorCopy(min, v2);
    VectorCopy(min, v3);
    VectorCopy(max, v5);
    VectorCopy(max, v6);
    VectorCopy(max, v7);
    VectorCopy(max, v8);
    v2[0] = max[0];
    v3[1] = max[1];
    v6[0] = min[0];
    v7[1] = min[1];
    v8[2] = min[2];
    const char* c = "textures/common/caulk";
    if (bUp) {
        if (dir != MOVE_EAST) { AddFaceWithTexture(k, newBrush, v1, v3, v6, c, false); }
        if (dir != MOVE_WEST) { AddFaceWithTexture(k, newBrush, v7, v5, v8, c, false); }
        if (dir != MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v1, v7, v2, c, false); }
        if (dir != MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v3, v8, v6, c, false); }
        AddFaceWithTexture(k, newBrush, v1, v2, v3, c, false);
        if (dir == MOVE_EAST) { AddFaceWithTexture(k, newBrush, v1, v3, v5, c, false); }
        if (dir == MOVE_WEST) { AddFaceWithTexture(k, newBrush, v2, v6, v8, c, false); }
        if (dir == MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v1, v6, v5, c, false); }
        if (dir == MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v7, v3, v8, c, false); }
    } else {
        if (dir != MOVE_WEST) { AddFaceWithTexture(k, newBrush, v7, v5, v8, c, false); }
        if (dir != MOVE_EAST) { AddFaceWithTexture(k, newBrush, v1, v3, v6, c, false); }
        if (dir != MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v3, v8, v6, c, false); }
        if (dir != MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v1, v7, v2, c, false); }
        AddFaceWithTexture(k, newBrush, v6, v5, v7, c, false);
        if (dir == MOVE_WEST) { AddFaceWithTexture(k, newBrush, v1, v5, v3, c, false); }
        if (dir == MOVE_EAST) { AddFaceWithTexture(k, newBrush, v2, v8, v6, c, false); }
        if (dir == MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v1, v5, v6, c, false); }
        if (dir == MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v7, v8, v3, c, false); }
    }
    einfuegen(k, newBrush);
}

void Build_StairStep_Wedge(Karte& k, int dir, vec3_t min, vec3_t max, const char* mainTexture, const char* riserTexture, bool detail) {   // shapes.cpp:243-328
    NeuerPinsel newBrush;
    vec3_t v1, v2, v3, v5, v6, v7, v8;
    VectorCopy(min, v1);
    VectorCopy(min, v2);
    VectorCopy(min, v3);
    VectorCopy(max, v5);
    VectorCopy(max, v6);
    VectorCopy(max, v7);
    VectorCopy(max, v8);
    v2[0] = max[0];
    v3[1] = max[1];
    v6[0] = min[0];
    v7[1] = min[1];
    v8[2] = min[2];
    const char* c = "textures/common/caulk";
    AddFaceWithTexture(k, newBrush, v6, v5, v7, mainTexture, detail);
    if (dir != MOVE_EAST) { AddFaceWithTexture(k, newBrush, v5, v2, v7, dir == MOVE_WEST ? riserTexture : c, detail); }
    if (dir != MOVE_WEST) { AddFaceWithTexture(k, newBrush, v1, v3, v6, dir == MOVE_EAST ? riserTexture : c, detail); }
    if (dir != MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v3, v5, v6, dir == MOVE_SOUTH ? riserTexture : c, detail); }
    if (dir != MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v1, v7, v2, dir == MOVE_NORTH ? riserTexture : c, detail); }
    if (dir == MOVE_EAST) { AddFaceWithTexture(k, newBrush, v1, v5, v3, c, detail); }
    if (dir == MOVE_WEST) { AddFaceWithTexture(k, newBrush, v2, v8, v6, c, detail); }
    if (dir == MOVE_NORTH) { AddFaceWithTexture(k, newBrush, v1, v5, v6, c, detail); }
    if (dir == MOVE_SOUTH) { AddFaceWithTexture(k, newBrush, v7, v8, v3, c, detail); }
    einfuegen(k, newBrush);
}

void Build_StairStep(Karte& k, vec3_t min, vec3_t max, const char* mainTexture, const char* riserTexture, int direction) {   // shapes.cpp:385-433
    NeuerPinsel newBrush;
    vec3_t v1, v2, v3, v5, v6, v7;
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
    const char* c = "textures/common/caulk";
    AddFaceWithTexture(k, newBrush, v6, v5, v7, mainTexture, false);
    AddFaceWithTexture(k, newBrush, v1, v3, v6, direction == MOVE_EAST ? riserTexture : c, false);
    AddFaceWithTexture(k, newBrush, v1, v7, v2, direction == MOVE_NORTH ? riserTexture : c, false);
    AddFaceWithTexture(k, newBrush, v3, v5, v6, direction == MOVE_SOUTH ? riserTexture : c, false);
    AddFaceWithTexture(k, newBrush, v7, v5, v2, direction == MOVE_WEST ? riserTexture : c, false);
    AddFaceWithTexture(k, newBrush, v1, v2, v3, c, false);
    einfuegen(k, newBrush);
}

// MakeBevel (shapes.cpp:920-958)
void MakeBevel(Karte& k, vec3_t vMin, vec3_t vMax) {
    auto patch = std::make_shared<Patch>();
    patch->schicht = static_cast<int>(k.aktuelleSchicht);
    patch->setzeGroesse(3, 3);                         // Patch_resize( patch, 3, 3 )
    patch->setzeShader("textures/common/caulk");
    std::vector<PatchPunkt> pkt = patch->punkte();
    vec3_t x_3;
    vec3_t y_3;
    vec3_t z_3;
    x_3[0] = vMin[0];
    x_3[1] = vMin[0];
    x_3[2] = vMax[0];
    y_3[0] = vMin[1];
    y_3[1] = vMax[1];
    y_3[2] = vMax[1];
    z_3[0] = vMin[2];
    z_3[1] = (vMax[2] + vMin[2]) / 2;
    z_3[2] = vMax[2];
    const auto at = [&pkt](int i, int j) -> PatchPunkt& { return pkt[static_cast<std::size_t>(i * 3 + j)]; };   // matrix( i, j ), m_y = 3
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { at(i, j).vertex = Vektor3f(x_3[i], y_3[i], z_3[j]); }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 1; ++j) { std::swap(at(i, 2 - j).vertex, at(i, j).vertex); }
    }
    k.weltHolenOderAnlegen()->kindEinfuegen(patch);
    patch->punkteSetzen(pkt);   // Patch_controlPointsChanged
}

void BuildCornerStairs(Karte& k, vec3_t vMin, vec3_t vMax, int nSteps, const char* mainTexture, const char* riserTex) {   // shapes.cpp:960-1038
    std::vector<std::array<vec_t, 3>> topPoints(static_cast<std::size_t>(nSteps + 1));
    std::vector<std::array<vec_t, 3>> botPoints(static_cast<std::size_t>(nSteps + 1));
    vec3_t centre;
    VectorCopy(vMin, centre);
    centre[0] = vMax[0];
    const int height = static_cast<int>(vMax[2] - vMin[2]) / nSteps;
    vec3_t vTop;
    vec3_t vBot;
    VectorCopy(vMax, vTop);
    VectorCopy(vMin, vBot);
    vTop[2] = vMin[2] + static_cast<float>(height);
    for (int i = 0; i <= nSteps; ++i) {
        vec_t* tp = topPoints[static_cast<std::size_t>(i)].data();
        vec_t* bp = botPoints[static_cast<std::size_t>(i)].data();
        VectorCopy(centre, tp);
        VectorCopy(centre, bp);
        tp[2] = vMax[2];
        bp[2] = vMin[2];
        tp[0] -= 10 * std::sin(static_cast<float>(Q_PI * i / (2 * nSteps)));
        tp[1] += 10 * std::cos(static_cast<float>(Q_PI * i / (2 * nSteps)));
        bp[0] = tp[0];
        bp[1] = tp[1];
    }
    vec3_t tp[3];
    for (int j = 0; j < 3; ++j) { VectorCopy(topPoints[static_cast<std::size_t>(j)].data(), tp[j]); }
    for (int i = 0; i < nSteps; ++i) {
        NeuerPinsel brush;
        vec3_t v1, v2, v3, v5, v6, v7;
        VectorCopy(vBot, v1);
        VectorCopy(vBot, v2);
        VectorCopy(vBot, v3);
        VectorCopy(vTop, v5);
        VectorCopy(vTop, v6);
        VectorCopy(vTop, v7);
        v2[0] = vTop[0];
        v3[1] = vTop[1];
        v6[0] = vBot[0];
        v7[1] = vBot[1];
        AddFaceWithTexture(k, brush, v1, v2, v3, "textures/common/caulk", false);
        AddFaceWithTexture(k, brush, v1, v3, v6, "textures/common/caulk", false);
        AddFaceWithTexture(k, brush, v5, v6, v3, "textures/common/caulk", false);
        for (int j = 0; j < 3; ++j) { tp[j][2] = vTop[2]; }
        AddFaceWithTexture(k, brush, tp[2], tp[1], tp[0], mainTexture, false);
        AddFaceWithTexture(k, brush, centre, botPoints[static_cast<std::size_t>(i + 1)].data(), topPoints[static_cast<std::size_t>(i + 1)].data(),
                           "textures/common/caulk", false);
        AddFaceWithTexture(k, brush, centre, topPoints[static_cast<std::size_t>(i)].data(), botPoints[static_cast<std::size_t>(i)].data(), riserTex, false);
        einfuegen(k, brush);
        vTop[2] += static_cast<float>(height);
        vBot[2] += static_cast<float>(height);
    }
    vMin[2] += static_cast<float>(height);
    vMax[2] += static_cast<float>(height);
    MakeBevel(k, vMin, vMax);
}

void BuildDoorsX2(Karte& k, vec3_t min, vec3_t max, bool bSclMainHor, bool bSclMainVert, bool bSclTrimHor, bool bSclTrimVert, const char* mainTexture,
                  const char* trimTexture, int direction, const Umgebung& u, Meldungen& m) {   // shapes.cpp:435-602
    const int xy = direction == 0 ? 0 : 1;
    vec3_t v1, v2, v3, v5, v6, v7, ve_1, ve_2, ve_3;
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
    const float width = (max[xy] - min[xy]) / 2;
    if (direction == 0) {
        VectorCopy(v1, ve_1);
        VectorCopy(v3, ve_2);
        VectorCopy(v6, ve_3);
    } else {
        VectorCopy(v7, ve_1);
        VectorCopy(v1, ve_2);
        VectorCopy(v2, ve_3);
    }
    ve_1[xy] += width;
    ve_2[xy] += width;
    ve_3[xy] += width;
    NeuerPinsel newBrush1;
    NeuerPinsel newBrush2;
    const char* c = "textures/common/caulk";
    AddFaceWithTexture(k, newBrush1, v1, v2, v3, c, false);
    AddFaceWithTexture(k, newBrush1, v5, v7, v6, c, false);
    AddFaceWithTexture(k, newBrush2, v1, v2, v3, c, false);
    AddFaceWithTexture(k, newBrush2, v5, v7, v6, c, false);
    if (direction == 0) {
        AddFaceWithTexture(k, newBrush1, v1, v3, v6, c, false);
        AddFaceWithTexture(k, newBrush2, v5, v2, v7, c, false);
    } else {
        AddFaceWithTexture(k, newBrush1, v1, v7, v2, c, false);
        AddFaceWithTexture(k, newBrush2, v5, v6, v3, c, false);
    }
    if (direction == 0) {
        AddFaceWithTextureScaled(k, newBrush1, v1, v7, v2, mainTexture, bSclMainVert, bSclMainHor, min[0], min[2], max[0], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush1, v5, v6, v3, mainTexture, bSclMainVert, bSclMainHor, max[0], min[2], min[0], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, v1, v7, v2, mainTexture, bSclMainVert, bSclMainHor, min[0], min[2], max[0], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, v5, v6, v3, mainTexture, bSclMainVert, bSclMainHor, max[0], min[2], min[0], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush1, ve_3, ve_2, ve_1, trimTexture, bSclTrimVert, bSclTrimHor, min[1], min[2], max[1], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, ve_1, ve_2, ve_3, trimTexture, bSclTrimVert, bSclTrimHor, max[1], min[2], min[1], max[2], u, m);
    } else {
        AddFaceWithTextureScaled(k, newBrush1, v1, v3, v6, mainTexture, bSclMainVert, bSclMainHor, min[1], min[2], max[1], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush1, v5, v2, v7, mainTexture, bSclMainVert, bSclMainHor, max[1], min[2], min[1], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, v1, v3, v6, mainTexture, bSclMainVert, bSclMainHor, min[1], min[2], max[1], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, v5, v2, v7, mainTexture, bSclMainVert, bSclMainHor, max[1], min[2], min[1], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush1, ve_1, ve_2, ve_3, trimTexture, bSclTrimVert, bSclTrimHor, min[0], min[2], max[0], max[2], u, m);
        AddFaceWithTextureScaled(k, newBrush2, ve_3, ve_2, ve_1, trimTexture, bSclTrimVert, bSclTrimHor, max[0], min[2], min[0], max[2], u, m);
    }
    auto pEDoor1 = k.neueEntitaet("func_door", true);   // findOrInsert( "func_door", true ), createEntity
    auto pEDoor2 = k.neueEntitaet("func_door", true);
    if (direction == 0) {
        pEDoor1->setzen("angle", "180");
        pEDoor2->setzen("angle", "360");
    } else {
        pEDoor1->setzen("angle", "270");
        pEDoor2->setzen("angle", "90");
    }
    zufallStart(u);   // srand( (unsigned)time( nullptr ) )
    char teamname[256];
    std::snprintf(teamname, sizeof(teamname), "t%i", zufall(u));
    pEDoor1->setzen("team", teamname);
    pEDoor2->setzen("team", teamname);
    newBrush1.p->schicht = static_cast<int>(k.aktuelleSchicht);
    newBrush2.p->schicht = static_cast<int>(k.aktuelleSchicht);
    pEDoor1->kindEinfuegen(newBrush1.p);
    pEDoor2->kindEinfuegen(newBrush2.p);
    k.entitaetEinfuegen(pEDoor1);
    k.entitaetEinfuegen(pEDoor2);
}

// misc.cpp GetEntityCentre: die Mitte der Grenzen der ersten Entitaet mit
// target bzw. targetname == entityKey (FindEntityFromTarget/-name)
bool GetEntityCentre(const Karte& k, const char* entityKey, bool keyIsTarget, vec_t* centre) {
    for (const auto& e : k.entitaeten()) {
        if (e->wert(keyIsTarget ? "target" : "targetname") != entityKey) { continue; }
        // instance.worldAABB(): Punkt-Entitaet: ihr Kasten um origin; Gruppe: die Kinder
        Kasten b;
        bool da = false;
        Vektor3f mn(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vektor3f mx(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
        for (const auto& kind : e->kinder()) {
            vec3_t a;
            vec3_t z;
            grenzenVon(*kind, k.kontext, a, z);
            for (std::size_t i = 0; i < 3; ++i) {
                mn[i] = std::min(mn[i], a[i]);
                mx[i] = std::max(mx[i], z[i]);
            }
            da = true;
        }
        if (!da) {
            vec3_t o = {0, 0, 0};
            (void)std::sscanf(e->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            for (std::size_t i = 0; i < 3; ++i) { mn[i] = mx[i] = o[i]; }
        }
        for (std::size_t i = 0; i < 3; ++i) { centre[i] = (mn[i] + mx[i]) * 0.5F; }
        (void)b;
        return true;
    }
    return false;
}

}  // namespace

std::vector<std::string> listeLesen(const std::string& text) {   // lists.cpp:24-46 (fscanf( "%s\n" ))
    std::vector<std::string> v;
    std::istringstream s(text);
    std::string wort;
    while (s >> wort) { v.push_back(wort.substr(0, 255)); }
    return v;
}

// --- funchandlers-GTK.cpp ---------------------------------------------------------------

Meldungen intersect(Karte& k, const IntersectRS& rs, const Umgebung& u) {   // DoIntersect (funchandlers-GTK.cpp:78-123)
    Meldungen m;
    if (!rs.ganzeKarte && k.auswahl.anzahl() < 2) {
        fehl(m, "bobToolz Intersect: Invalid number of brushes selected, choose at least 2.\n");
        return m;
    }
    DEntity world;
    if (!rs.ganzeKarte) {
        world.LoadSelectedBrushes(k);
    } else {
        LoadOptions o;
        o.loadDetail = rs.detail;
        (void)world.LoadFromEntity(k, k.weltHolenOderAnlegen(), o);   // GlobalRadiant().getMapWorldEntity()
    }
    world.RemoveNonCheckBrushes(u.ausschluss);
    const std::vector<bool> pbSelectList = rs.nurDuplikate ? world.BuildDuplicateList() : world.BuildIntersectList();
    world.SelectBrushes(k, pbSelectList);
    m.push_back({0, "bobToolz Intersect: " + std::to_string(k.auswahl.anzahl()) + " intersecting brushes found.\n"});
    return m;
}

Meldungen findDuplicates(Karte& k) {   // DoFindDuplicates (funchandlers-GTK.cpp:125-149)
    Meldungen m;
    DMap map;
    LoadOptions o;
    o.loadVisibleOnly = true;
    map.LoadAll(k, o);
    std::vector<const DBrush*> brushes;
    for (const DEntity* e : map.entityList) {
        for (const DBrush* b : e->brushList) { brushes.push_back(b); }
    }
    k.auswahl.leeren();
    for (auto b = brushes.begin(); b != brushes.end(); ++b) {
        if (*b == nullptr) { continue; }
        for (auto b2 = std::next(b); b2 != brushes.end(); ++b2) {
            if (*b2 != nullptr && (*b)->operator==(*b2)) {
                (*b2)->selectInRadiant(k);
                *b2 = nullptr;
            }
        }
    }
    m.push_back({0, "bobToolz Find Duplicates: " + std::to_string(std::count(brushes.cbegin(), brushes.cend(), nullptr)) + " duplicate brushes found.\n"});
    return m;
}

Meldungen polygonPruefen(const Karte& k) {   // DoPolygons vor DoPolygonBox
    Meldungen m;
    if (k.auswahl.anzahl() != 1) {
        fehl(m, "bobToolz Polygons: Invalid number of brushes selected, choose 1 only.\n");
        return m;
    }
    const auto& a = k.auswahl.elemente().back();
    const auto p = a.primitiv.lock();
    if (!p || p->art() != Primitiv::Art::Pinsel) { fehl(m, "bobToolz Polygons: No brush selected, select ONLY one brush.\n"); }
    return m;
}

Meldungen polygonBuilder(Karte& k, const PolygonRS& rs, const Umgebung& u) {   // DoPolygons (funchandlers-GTK.cpp:155-199)
    Befehl undo(k.undo, "bobToolz.polygons");
    Meldungen m = polygonPruefen(k);
    if (!m.empty()) { return m; }
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();
    const auto prim = a.primitiv.lock();
    DShape poly(u.aktuelleTextur);
    vec3_t vMin;
    vec3_t vMax;
    grenzen(k, *prim, vMin, vMax);
    primitivLoeschen(k, ent, prim);   // Path_deleteTop( instance.path() )
    if (rs.invers) {
        poly.BuildInversePrism(vMin, vMax, rs.seiten, rs.obenAusrichten);
    } else if (rs.rand) {
        poly.BuildBorderedPrism(vMin, vMax, rs.seiten, rs.randBreite, rs.obenAusrichten);
    } else {
        poly.BuildRegularPrism(vMin, vMax, rs.seiten, rs.obenAusrichten);
    }
    poly.Commit(k, m);
    return m;
}

Meldungen brushCleanup(Karte& k) {   // DoFixBrushes (funchandlers-GTK.cpp:201-209): zaehlt nur, baut nichts neu
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.fixBrushes");
    DMap world;
    world.LoadAll(k);
    const int count = world.FixBrushes();
    m.push_back({0, "bobToolz FixBrushes: " + std::to_string(count) + " invalid/duplicate planes removed.\n"});
    return m;
}

Meldungen resetTextures(Karte& k, const ResetTextureRS& rs, bool alle) {   // DoResetTextures (funchandlers-GTK.cpp:211-246)
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.resetTextures");
    const char* texName = nullptr;   // if( 1 /*GetSelectedFaceCount() != 1*/ ) texName = nullptr
    if (rs.nameErsetzen) { texName = rs.alterName.c_str(); }
    const float fScale[2] = {rs.skala[0], rs.skala[1]};
    const float fShift[2] = {rs.verschiebung[0], rs.verschiebung[1]};
    if (!alle) {   // eIDOK: Use Selected Brushes
        DEntity world;
        world.LoadSelectedBrushes(k);
        (void)world.ResetTextures(k, texName, fScale, fShift, rs.drehung, rs.neuerName.c_str(), rs.nameErsetzen, rs.skalaSetzen, rs.verschiebungSetzen,
                                  rs.drehungSetzen, true, m);
    } else {       // eIDYES: Use All Brushes
        DMap world;
        LoadOptions o;
        o.loadPatches = true;
        world.LoadAll(k, o);
        world.ResetTextures(k, texName, fScale, fShift, rs.drehung, rs.neuerName.c_str(), rs.nameErsetzen, rs.skalaSetzen, rs.verschiebungSetzen,
                            rs.drehungSetzen, m);
    }
    return m;
}

Meldungen stairPruefen(const Karte& k) {
    Meldungen m;
    if (k.auswahl.anzahl() != 1) { fehl(m, "bobToolz BuildStairs: Invalid number of brushes selected, choose 1 only.\n"); }
    return m;
}

Meldungen stairBuilder(Karte& k, const BuildStairsRS& rs) {   // DoBuildStairs (funchandlers-GTK.cpp:248-343)
    Befehl undo(k.undo, "bobToolz.buildStairs");
    Meldungen m = stairPruefen(k);
    if (!m.empty()) { return m; }
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();
    const auto prim = a.primitiv.lock();
    if (!prim) { return m; }
    vec3_t vMin;
    vec3_t vMax;
    grenzen(k, *prim, vMin, vMax);
    vec3_t size;
    VectorSubtract(vMax, vMin, size);
    if ((static_cast<int>(size[2]) % rs.stufenHoehe) != 0) {
        fehl(m, "bobToolz BuildStairs: Invalid stair height. Height of block must be divisable by stair height.\n");
        return m;
    }
    primitivLoeschen(k, ent, prim);
    const int numSteps = static_cast<int>(size[2]) / rs.stufenHoehe;
    if (rs.stil == STYLE_CORNER) {
        BuildCornerStairs(k, vMin, vMax, numSteps, rs.hauptTextur.c_str(), rs.stirnTextur.c_str());
        return m;
    }
    const auto stairHeight = static_cast<float>(rs.stufenHoehe);
    float stairWidth = 0;
    if (rs.richtung == MOVE_EAST || rs.richtung == MOVE_WEST) {
        stairWidth = size[0] / static_cast<float>(numSteps);
    } else {
        stairWidth = size[1] / static_cast<float>(numSteps);
    }
    if (rs.stil == STYLE_BOB) { Build_Wedge(k, rs.richtung, vMin, vMax, true); }
    vMax[2] = vMin[2] + stairHeight;
    SetInitialStairPos(rs.richtung, vMin, vMax, stairWidth);
    for (int i = 0; i < numSteps; ++i) {
        if (rs.stil == STYLE_BOB) {
            Build_StairStep_Wedge(k, rs.richtung, vMin, vMax, rs.hauptTextur.c_str(), rs.stirnTextur.c_str(), rs.detail);
        } else if (rs.stil == STYLE_ORIGINAL) {
            Build_StairStep(k, vMin, vMax, rs.hauptTextur.c_str(), rs.stirnTextur.c_str(), rs.richtung);
        }
        MoveBlock(rs.richtung, vMin, vMax, stairWidth);
        vMax[2] += stairHeight;
        if (rs.stil == STYLE_BOB) { vMin[2] += stairHeight; }   // wedge bottom must be raised
    }
    return m;
}

Meldungen doorPruefen(const Karte& k) {
    Meldungen m;
    if (k.auswahl.anzahl() != 1) { fehl(m, "bobToolz BuildDoors: Invalid number of brushes selected, choose 1 only.\n"); }
    return m;
}

Meldungen doorBuilder(Karte& k, const DoorRS& rs, const Umgebung& u) {   // DoBuildDoors (funchandlers-GTK.cpp:345-378)
    Meldungen m = doorPruefen(k);
    if (!m.empty()) { return m; }
    Befehl undo(k.undo, "bobToolz.buildDoors");
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();
    const auto prim = a.primitiv.lock();
    if (!prim) { return m; }
    vec3_t vMin;
    vec3_t vMax;
    grenzen(k, *prim, vMin, vMax);
    primitivLoeschen(k, ent, prim);
    BuildDoorsX2(k, vMin, vMax, rs.hauptH, rs.hauptV, rs.randH, rs.randV, ("textures/" + rs.hauptTextur).c_str(), ("textures/" + rs.randTextur).c_str(),
                 rs.ausrichtung, u, m);
    return m;
}

Meldungen pitOMatic(Karte& k, const Umgebung& u) {   // DoPitBuilder (funchandlers-GTK.cpp:411-441)
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.pitBuilder");
    if (k.auswahl.anzahl() != 1) {
        fehl(m, "bobToolz PitBuilder: Invalid number of brushes selected, choose 1 only.\n");
        return m;
    }
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();
    const auto prim = a.primitiv.lock();
    if (!prim || prim->art() != Primitiv::Art::Pinsel) {
        fehl(m, "bobToolz PitBuilder: No brush selected, select ONLY 1 brush.\n");
        return m;
    }
    vec3_t vMin;
    vec3_t vMax;
    grenzen(k, *prim, vMin, vMax);
    DShape pit(u.aktuelleTextur);
    if (pit.BuildPit(vMin, vMax, u)) {
        pit.Commit(k, m);
        primitivLoeschen(k, ent, prim);
    } else {
        fehl(m, "bobToolz PitBuilder: Failed to make Pit, try making the brush bigger.\n");
    }
    return m;
}

Meldungen caulkSelection(Karte& k) {   // DoCaulkSelection (funchandlers-GTK.cpp:651-664)
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.caulkSelection");
    DEntity world;
    const float fScale[2] = {0.5F, 0.5F};
    const float fShift[2] = {0.0F, 0.0F};
    const bool bResetScale[2] = {false, false};
    const bool bResetShift[2] = {false, false};
    world.LoadSelectedBrushes(k);
    world.LoadSelectedPatches(k);
    (void)world.ResetTextures(k, nullptr, fScale, fShift, 0, "textures/common/caulk", true, bResetScale, bResetShift, false, true, m);
    return m;
}

Meldungen explodeEntity(Karte& k) {   // DoExplodeEntity (funchandlers-GTK.cpp:896-936) mit ExplodeWalker
    Meldungen m;
    if (k.auswahl.anzahl() == 0) {
        fehl(m, "nothing selected\n");
        return m;
    }
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();   // Node_getEntity( path.top() ) bzw. eine Stufe hoeher
    if (!ent) {
        fehl(m, "no entity selected\n");
        return m;
    }
    if (!ent->istBehaelter()) {
        fehl(m, "!entity->isContainer()\n");
        return m;
    }
    std::string c = ent->klassenname();
    for (char& ch : c) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
    if (c == "worldspawn") {
        fehl(m, "can't explode worldspawn\n");
        return m;
    }
    Befehl undo(k.undo, "bobToolz.explodeEntity");
    k.auswahl.leeren();
    const auto kinder = ent->kinder();
    for (const auto& node : kinder) {   // post(): clone der Entitaet, das Primitiv dort hinein
        auto klon = k.neueEntitaet(ent->klassenname(), true);
        for (const auto& [key, value] : ent->schluessel()) {
            if (key != "classname") { klon->setzen(key, value); }
        }
        k.entitaetEinfuegen(klon);
        (void)ent->kindEntfernen(node.get());
        klon->kindEinfuegen(node);
    }
    (void)k.entitaetEntfernen(ent.get());   // traversableroot->erase( *entitynode )
    return m;
}

Meldungen makeChain(Karte& k, const MakeChainRS& rs) {   // DoMakeChain (funchandlers-GTK.cpp:688-698) + MakeChain (DTreePlanter.cpp:196-229)
    Meldungen m;
    if (rs.glieder > 1001) {
        fehl(m, "bobToolz MakeChain: " + std::to_string(rs.glieder) + " to many Elemets, limited to 1000.\n");
        return m;
    }
    Befehl undo(k.undo, "bobToolz.makeChain");
    char buffer[256];
    for (int i = 0; i < rs.glieder; ++i) {
        DEntity e("info_train_spline_main");
        std::snprintf(buffer, sizeof(buffer), "%s_pt%i", rs.name.c_str(), i);
        e.AddEPair("targetname", buffer);
        std::snprintf(buffer, sizeof(buffer), "0 %i 0", i * 64);
        e.AddEPair("origin", buffer);
        if (i != rs.glieder - 1) {
            std::snprintf(buffer, sizeof(buffer), "%s_pt%i", rs.name.c_str(), i + 1);
            e.AddEPair("target", buffer);
            std::snprintf(buffer, sizeof(buffer), "%s_ctl%i", rs.name.c_str(), i);
            e.AddEPair("control", buffer);
        }
        e.BuildInRadiant(k, false, m);
    }
    for (int i = 0; i < rs.glieder - 1; ++i) {
        DEntity e("info_train_spline_control");
        std::snprintf(buffer, sizeof(buffer), "%s_ctl%i", rs.name.c_str(), i);
        e.AddEPair("targetname", buffer);
        std::snprintf(buffer, sizeof(buffer), "0 %i 0", (i * 64) + 32);
        e.AddEPair("origin", buffer);
        e.BuildInRadiant(k, false, m);
    }
    return m;
}

Meldungen mergePatches(Karte& k) {   // DoMergePatches (funchandlers-GTK.cpp:443-512)
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.mergePatches");
    struct Daten {
        DPatch patch;
        std::shared_ptr<Entitaet> ent;
        std::shared_ptr<Patch> inst;
        bool merged = false;
    };
    std::list<Daten> mrg;
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Patch>>> instances_erase;
    for (const Gewaehlt& g : auswahlListe(k)) {
        if (g.primitiv && g.primitiv->art() == Primitiv::Art::Patch) {
            Daten& d = mrg.emplace_back();
            d.ent = g.entitaet;
            d.inst = std::static_pointer_cast<Patch>(g.primitiv);
            d.patch.LoadFromPatch(d.ent, d.inst);
        }
    }
    if (mrg.size() < 2) {
        fehl(m, "bobToolz MergePatches: Invalid number of patches selected, choose 2 or more.\n");
        return m;
    }
    const auto try_merge_one = [&]() {
        for (auto one = mrg.begin(); one != mrg.end(); ++one) {
            for (auto two = std::next(one); two != mrg.end(); ++two) {
                const patch_merge_t merge_info = one->patch.IsMergable(two->patch);
                if (merge_info.mergable) {
                    DPatch* newPatch = one->patch.MergePatches(merge_info, one->patch, two->patch);
                    if (newPatch != nullptr) {
                        one->merged = true;
                        one->patch = *newPatch;
                        delete newPatch;
                        instances_erase.emplace_back(two->ent, two->inst);
                        mrg.erase(two);
                        return true;
                    }
                }
            }
        }
        return false;
    };
    while (try_merge_one()) {}
    if (instances_erase.empty()) {
        fehl(m, "bobToolz.mergePatch: The selected patches are not mergable.\n");
        return m;
    }
    for (auto&& d : mrg) {
        if (d.merged) {
            (void)d.patch.BuildInRadiant(k, d.ent);
            primitivLoeschen(k, d.ent, d.inst);
        }
    }
    for (auto& [parent, inst] : instances_erase) {
        primitivLoeschen(k, parent, inst);
        // don't leave empty entities
        if (parent != k.welt() && parent->kinder().empty()) {
            k.auswahl.abwaehlen(parent);
            (void)k.entitaetEntfernen(parent.get());
        }
    }
    return m;
}

Meldungen splitPatch(Karte& k, int art) {   // DoSplitPatch / DoSplitPatchCols / DoSplitPatchRows (funchandlers-GTK.cpp:514-583)
    static const char* const kUndo[3] = {"bobToolz.splitPatch", "bobToolz.splitPatchCols", "bobToolz.splitPatchRows"};
    static const char* const kName[3] = {"SplitPatch", "SplitPatchCols", "SplitPatchRows"};
    static const char* const kAnzahl[3] = {"choose only 1 patch", "choose 1 only", "choose 1 only"};
    Meldungen m;
    Befehl undo(k.undo, kUndo[art]);
    if (k.auswahl.anzahl() != 1) {
        fehl(m, std::string("bobToolz ") + kName[art] + ": Invalid number of patches selected, " + kAnzahl[art] + ".\n");
        return m;
    }
    const auto& a = k.auswahl.elemente().back();
    const auto ent = a.entitaet.lock();
    const auto prim = a.primitiv.lock();
    if (!prim || prim->art() != Primitiv::Art::Patch) {
        fehl(m, std::string("bobToolz ") + kName[art] + ": No patch selected, select ONLY 1 patch.\n");
        return m;
    }
    DPatch patch;
    patch.LoadFromPatch(ent, std::static_pointer_cast<Patch>(prim));
    std::list<DPatch> teile = art == 0 ? patch.Split(m) : (art == 1 ? patch.SplitCols(m) : patch.SplitRows(m));
    for (auto&& p : teile) { (void)p.BuildInRadiant(k, ent); }
    primitivLoeschen(k, ent, prim);
    return m;
}

Meldungen flipTerrain(Karte& k) {   // DoFlipTerrain (funchandlers-GTK.cpp:702-862)
    Meldungen m;
    Befehl undo(k.undo, "bobToolz.flipTerrain");
    const vec3_t vUp = {0, 0, 1};
    if (k.auswahl.anzahl() != 2) {
        fehl(m, "bobToolz FlipTerrain: Invalid number of objects selected, choose 2 only.\n");
        return m;
    }
    const auto& el = k.auswahl.elemente();
    std::shared_ptr<Entitaet> ents[2] = {el[el.size() - 1].entitaet.lock(), el[el.size() - 2].entitaet.lock()};   // ultimate, penultimate
    std::shared_ptr<Primitiv> brushes[2] = {el[el.size() - 1].primitiv.lock(), el[el.size() - 2].primitiv.lock()};
    for (int i = 0; i < 2; ++i) {
        if (!brushes[i] || brushes[i]->art() != Primitiv::Art::Pinsel) {
            fehl(m, "bobToolz FlipTerrain: No brushes selected, select ONLY 2 brushes.\n");
            return m;
        }
    }
    DBrush Brushes[2];
    DPlane* Planes[2] = {nullptr, nullptr};
    DPoint* Points[2][3] = {{nullptr, nullptr, nullptr}, {nullptr, nullptr, nullptr}};
    for (int i = 0; i < 2; ++i) {
        Brushes[i].LoadFromBrush(ents[i], std::static_pointer_cast<Pinsel>(brushes[i]), false);
        if ((Planes[i] = Brushes[i].FindPlaneWithClosestNormal(vUp)) == nullptr || Brushes[i].FindPointsForPlane(Planes[i], Points[i], 3) != 3) {
            fehl(m, "bobToolz FlipTerrain: ERROR (FindPlaneWithClosestNormal/FindPointsForPlane).\n");
            return m;
        }
    }
    vec3_t mins1, mins2, maxs1, maxs2;
    (void)Brushes[0].GetBounds(mins1, maxs1);
    (void)Brushes[1].GetBounds(mins2, maxs2);
    int dontmatch[2] = {-1, -1};
    bool found = false;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3 && !found; ++j) {
            if (VectorCompare(Points[0][i]->_pnt, Points[1][j]->_pnt)) {
                found = true;
                break;
            }
        }
        if (!found) {
            dontmatch[0] = i;
            break;
        }
        found = false;
    }
    if (dontmatch[0] == -1) {
        fehl(m, "bobToolz FlipTerrain: ERROR (dontmatch[0]).\n");
        return m;
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3 && !found; ++j) {
            if (VectorCompare(Points[1][i]->_pnt, Points[0][j]->_pnt)) {
                found = true;
                break;
            }
        }
        if (!found) {
            dontmatch[1] = i;
            break;
        }
        found = false;
    }
    if (dontmatch[1] == -1) {
        fehl(m, "bobToolz FlipTerrain: ERROR (dontmatch[1]).\n");
        return m;
    }
    vec3_t plnpnts1[3];
    vec3_t plnpnts2[3];
    vec3_t plnpntsshr[3];
    VectorCopy(Points[0][dontmatch[0]]->_pnt, plnpnts1[0]);
    for (int i = 0; i < 3; ++i) {
        if (dontmatch[0] != i) {
            VectorCopy(Points[0][i]->_pnt, plnpnts1[1]);
            break;
        }
    }
    VectorCopy(Points[1][dontmatch[1]]->_pnt, plnpnts1[2]);
    VectorCopy(Points[1][dontmatch[1]]->_pnt, plnpnts2[0]);
    for (int i = 0; i < 3; ++i) {
        if (dontmatch[1] != i && !VectorCompare(Points[1][i]->_pnt, plnpnts1[1])) {
            VectorCopy(Points[1][i]->_pnt, plnpnts2[1]);
            break;
        }
    }
    VectorCopy(Points[0][dontmatch[0]]->_pnt, plnpnts2[2]);
    VectorCopy(Points[0][dontmatch[0]]->_pnt, plnpntsshr[0]);
    VectorCopy(Points[1][dontmatch[1]]->_pnt, plnpntsshr[1]);
    if (Points[1][dontmatch[1]]->_pnt[2] < Points[0][dontmatch[0]]->_pnt[2]) {
        VectorCopy(Points[1][dontmatch[1]]->_pnt, plnpntsshr[2]);
    } else {
        VectorCopy(Points[0][dontmatch[0]]->_pnt, plnpntsshr[2]);
    }
    plnpntsshr[2][2] -= 16;
    for (int i = 0; i < 3; ++i) {
        if (mins2[i] < mins1[i]) { mins1[i] = mins2[i]; }
        if (maxs2[i] > maxs1[i]) { maxs1[i] = maxs2[i]; }
    }
    DBrush* newBrushes[2];
    newBrushes[0] = DShape::GetBoundingCube_Ext(mins1, maxs1, "textures/common/caulk", bFacesAll, true);
    newBrushes[1] = DShape::GetBoundingCube_Ext(mins1, maxs1, "textures/common/caulk", bFacesAll, true);
    vec3_t normal;
    MakeNormal(plnpnts1[0], plnpnts1[1], plnpnts1[2], normal);
    if (normal[2] >= 0) {
        (void)newBrushes[0]->AddFace(plnpnts1[0], plnpnts1[1], plnpnts1[2], "textures/common/terrain", true);
    } else {
        (void)newBrushes[0]->AddFace(plnpnts1[2], plnpnts1[1], plnpnts1[0], "textures/common/terrain", true);
    }
    MakeNormal(plnpnts2[0], plnpnts2[1], plnpnts2[2], normal);
    if (normal[2] >= 0) {
        (void)newBrushes[1]->AddFace(plnpnts2[0], plnpnts2[1], plnpnts2[2], "textures/common/terrain", true);
    } else {
        (void)newBrushes[1]->AddFace(plnpnts2[2], plnpnts2[1], plnpnts2[0], "textures/common/terrain", true);
    }
    vec3_t vec;
    MakeNormal(plnpntsshr[0], plnpntsshr[1], plnpntsshr[2], normal);
    VectorSubtract(plnpnts1[2], plnpnts1[1], vec);
    if (DotProduct(vec, normal) >= 0) {
        (void)newBrushes[0]->AddFace(plnpntsshr[0], plnpntsshr[1], plnpntsshr[2], "textures/common/caulk", true);
    } else {
        (void)newBrushes[0]->AddFace(plnpntsshr[2], plnpntsshr[1], plnpntsshr[0], "textures/common/caulk", true);
    }
    VectorSubtract(plnpnts2[2], plnpnts2[1], vec);
    if (DotProduct(vec, normal) >= 0) {
        (void)newBrushes[1]->AddFace(plnpntsshr[0], plnpntsshr[1], plnpntsshr[2], "textures/common/caulk", true);
    } else {
        (void)newBrushes[1]->AddFace(plnpntsshr[2], plnpntsshr[1], plnpntsshr[0], "textures/common/caulk", true);
    }
    for (int i = 0; i < 2; ++i) {
        (void)newBrushes[i]->RemoveRedundantPlanes();
        (void)newBrushes[i]->BuildInRadiant(k, false, nullptr, ents[i], m);
        primitivLoeschen(k, ents[i], brushes[i]);
        delete newBrushes[i];
    }
    return m;
}

// --- Path Plotter (DBobView.cpp) ----------------------------------------------------------

namespace {

constexpr float LOCAL_GRAVITY = -800.0F;   // DBobView.cpp:118

bool CalculateTrajectory(vec3_t start, vec3_t apex, float multiplier, int points, float varGravity, Bahn& bahn) {   // DBobView.cpp:120-150
    if (apex[2] <= start[2]) {
        bahn.punkte.clear();
        return false;
    }
    vec3_t dist;
    vec3_t speed;
    VectorSubtract(apex, start, dist);
    const auto speed_z = static_cast<vec_t>(std::sqrt(-2 * LOCAL_GRAVITY * dist[2]));
    const float flight_time = -speed_z / LOCAL_GRAVITY;
    VectorScale(dist, 1 / flight_time, speed);
    speed[2] = speed_z;
    bahn.punkte.assign(static_cast<std::size_t>(points), Vektor3f(0, 0, 0));
    const float interval = multiplier * flight_time / static_cast<float>(points);
    for (int i = 0; i < points; ++i) {
        const float ltime = interval * static_cast<float>(i);
        vec3_t p;
        VectorScale(speed, ltime, p);
        VectorAdd(p, start, p);
        p[2] = start[2] + (speed_z * ltime) + (varGravity * 0.5F * ltime * ltime);
        bahn.punkte[static_cast<std::size_t>(i)] = Vektor3f(p[0], p[1], p[2]);
    }
    return true;
}

}  // namespace

bool bahnAktualisieren(const Karte& k, Bahn& bahn) {   // DBobView::UpdatePath (DBobView.cpp:171-181)
    vec3_t start;
    vec3_t apex;
    if (GetEntityCentre(k, bahn.zielName.c_str(), true, start) && GetEntityCentre(k, bahn.zielName.c_str(), false, apex)) {
        (void)CalculateTrajectory(start, apex, bahn.rs.faktor, bahn.rs.punkte, bahn.rs.schwerkraft, bahn);
        return true;
    }
    return false;
}

std::array<Vektor3f, 4> bahnKastenVersaetze() {   // g_transform_box1..4 (DBobView.cpp:94-97)
    return {Vektor3f(16.0F, 16.0F, 28.0F), Vektor3f(-16.0F, 16.0F, 28.0F), Vektor3f(16.0F, -16.0F, -28.0F), Vektor3f(-16.0F, -16.0F, -28.0F)};
}

Meldungen pathPlotter(const Karte& k, const PathPlotterRS& rs, Bahn& bahn) {   // DoPathPlotter + DBobView_setEntity (funchandlers-GTK.cpp:384-409, DBobView.cpp:183-231)
    Meldungen m;
    if (k.auswahl.anzahl() != 1) {
        fehl(m, "bobToolz PathPlotter: Invalid number of entities selected, choose 1 trigger_push entity only.\n");
        return m;
    }
    const auto ent = k.auswahl.elemente().back().entitaet.lock();
    if (!ent) {
        fehl(m, "bobToolz PathPlotter: No trigger_push entity selected, select 1 only (Use list to select it).\n");
        return m;
    }
    DEntity trigger;
    trigger.LoadEPairList(*ent);
    if (trigger.m_Classname != "trigger_push") {
        fehl(m, "bobToolz PathPlotter: You must select a 'trigger_push' entity..\n");
        return m;
    }
    DEPair* trigger_ep = trigger.FindEPairByKey("target");
    if (trigger_ep == nullptr) {
        fehl(m, "bobToolz PathPlotter: Entity must have a target.\n");
        return m;
    }
    bool ziel = false;
    for (const auto& e : k.entitaeten()) { ziel = ziel || e->wert("targetname") == trigger_ep->value; }   // FindEntityFromTargetname
    if (!ziel) {
        fehl(m, "bobToolz PathPlotter: trigger_push target could not be found..\n");
        return m;
    }
    bahn = Bahn{};   // g_PathView.reset(); new DBobView
    bahn.aktualisieren = !rs.keineAktualisierung;
    // DBobView::Begin
    bahn.zielName = trigger_ep->value;
    bahn.rs = rs;
    bahn.hilfslinien = rs.hilfslinien;
    if (!bahnAktualisieren(k, bahn)) {
        fehl(m, "Initialization Failure in DBobView::Begin\n");
        bahn = Bahn{};
    } else {
        bahn.aktiv = !bahn.punkte.empty();
    }
    aus(m, "Initialization of Path Plotter succeeded.\n");   // so bei NRC auch nach dem Fehler
    return m;
}

// --- Plot Splines (DTrainDrawer.cpp) ------------------------------------------------------

namespace {

void CalculateSpline_r(std::vector<std::array<vec_t, 3>> v, std::array<vec_t, 3>& out, float tension) {   // DTrainDrawer.cpp:56-75
    const auto count = static_cast<int>(v.size());
    if (count < 2) { return; }
    vec3_t dist;
    if (count == 2) {
        VectorSubtract(v[1].data(), v[0].data(), dist);
        VectorMA(v[0].data(), tension, dist, out.data());
        return;
    }
    std::vector<std::array<vec_t, 3>> v2(static_cast<std::size_t>(count - 1));
    for (int i = 0; i < count - 1; ++i) {
        VectorSubtract(v[static_cast<std::size_t>(i + 1)].data(), v[static_cast<std::size_t>(i)].data(), dist);
        VectorMA(v[static_cast<std::size_t>(i)].data(), tension, dist, v2[static_cast<std::size_t>(i)].data());
    }
    CalculateSpline_r(std::move(v2), out, tension);
}

struct ControlPoint {   // controlPoint_t
    std::string strName;
    vec3_t vOrigin = {0, 0, 0};
};
struct SplinePoint {    // splinePoint_t
    ControlPoint point;
    std::string strTarget;
    ControlPoint* pTarget = nullptr;
    std::list<ControlPoint> m_pointList;
    std::list<std::array<vec_t, 3>> m_vertexList;
};

}  // namespace

Meldungen splinesBauen(const Karte& k, Splines& s) {   // DTrainDrawer::BuildPaths mit EntityBuildPaths (DTrainDrawer.cpp:157-262)
    Meldungen m;
    s = Splines{};
    std::list<ControlPoint> pointList;
    std::list<SplinePoint> splineList;
    const auto name64 = [](const char* n) { return std::string(n).substr(0, 63); };   // strncpy( strName, ..., 63 )
    for (const auto& ent : k.entitaeten()) {   // Scene_forEachEntity
        DEntity e;
        e.LoadEPairList(*ent);
        const char* targetname = nullptr;
        const char* target = nullptr;
        const char* control = nullptr;
        vec3_t vOrigin;
        e.SpawnString("targetname", nullptr, &targetname);
        e.SpawnVector("origin", "0 0 0", vOrigin);
        if (e.m_Classname == "info_train_spline_main") {
            if (targetname == nullptr) {
                warn(m, "info_train_spline_main with no targetname");
                continue;
            }
            e.SpawnString("target", nullptr, &target);
            if (target == nullptr) {
                ControlPoint cp;   // AddControlPoint
                cp.strName = name64(targetname);
                VectorCopy(vOrigin, cp.vOrigin);
                pointList.push_back(cp);
            } else {
                SplinePoint& sp = splineList.emplace_back();   // AddSplinePoint
                sp.point.strName = name64(targetname);
                sp.strTarget = name64(target);
                VectorCopy(vOrigin, sp.point.vOrigin);
                e.SpawnString("control", nullptr, &control);
                if (control != nullptr) {
                    ControlPoint c;
                    c.strName = name64(control);
                    sp.m_pointList.push_front(c);   // AddSplineControl: push_front
                    for (int j = 2;; ++j) {
                        char buffer[32];
                        std::snprintf(buffer, sizeof(buffer), "control%i", j);
                        e.SpawnString(buffer, nullptr, &control);
                        if (control == nullptr) { break; }
                        ControlPoint c2;
                        c2.strName = name64(control);
                        sp.m_pointList.push_front(c2);
                    }
                }
            }
        } else if (e.m_Classname == "info_train_spline_control") {
            if (targetname == nullptr) {
                warn(m, "info_train_spline_control with no targetname");
                continue;
            }
            ControlPoint cp;
            cp.strName = name64(targetname);
            VectorCopy(vOrigin, cp.vOrigin);
            pointList.push_back(cp);
        }
    }
    const auto FindControlPoint = [&](const std::string& name) -> ControlPoint* {   // DTrainDrawer.cpp:310-329
        for (ControlPoint& cp : pointList) {
            if (cp.strName == name) { return &cp; }
        }
        for (SplinePoint& sp : splineList) {
            if (sp.point.strName == name) { return &sp.point; }
        }
        return nullptr;
    };
    for (SplinePoint& sp : splineList) {
        ControlPoint* pTarget = FindControlPoint(sp.strTarget);
        if (pTarget == nullptr) {
            warn(m, "couldn't find target " + sp.strTarget);
            return m;
        }
        sp.pTarget = pTarget;
        for (ControlPoint& cp : sp.m_pointList) {
            ControlPoint* pControl = FindControlPoint(cp.strName);
            if (pControl == nullptr) {
                warn(m, "couldn't find control " + cp.strName);
                return m;
            }
            VectorCopy(pControl->vOrigin, cp.vOrigin);
        }
    }
    s.anzeigen = true;   // m_bDisplay
    for (SplinePoint& sp : splineList) {
        if (sp.pTarget == nullptr) { continue; }
        std::vector<std::array<vec_t, 3>> v;
        v.push_back({sp.point.vOrigin[0], sp.point.vOrigin[1], sp.point.vOrigin[2]});
        for (auto it = sp.m_pointList.rbegin(); it != sp.m_pointList.rend(); ++it) { v.push_back({it->vOrigin[0], it->vOrigin[1], it->vOrigin[2]}); }
        v.push_back({sp.pTarget->vOrigin[0], sp.pTarget->vOrigin[1], sp.pTarget->vOrigin[2]});
        for (float tension = 0; tension <= 1; tension += 0.01F) {
            std::array<vec_t, 3> out{};
            CalculateSpline_r(v, out, tension);
            sp.m_vertexList.push_front(out);
        }
        sp.m_vertexList.push_front({sp.pTarget->vOrigin[0], sp.pTarget->vOrigin[1], sp.pTarget->vOrigin[2]});
        std::vector<Vektor3f> linie;
        for (const auto& p : sp.m_vertexList) { linie.emplace_back(p[0], p[1], p[2]); }
        s.linien.push_back(std::move(linie));
    }
    return m;
}

// --- Tree Planter (DTreePlanter.h/.cpp, ScriptParser.cpp) -----------------------------------

namespace {

struct PlanterWelt {   // DEntity m_world
    DEntity welt;
};

// CScriptParser::GetToken( true ): Leerraum ueberspringen, "..." als ein Wort,
// // und /* */ als Kommentar, { } ( ) als eigene Zeichen (ScriptParser.cpp)
class Skript {
public:
    explicit Skript(std::string text) : m_text(std::move(text)) {}
    std::string token() {
        for (;;) {
            while (m_pos < m_text.size() && static_cast<unsigned char>(m_text[m_pos]) <= ' ') { ++m_pos; }
            if (m_pos + 1 < m_text.size() && m_text[m_pos] == '/' && m_text[m_pos + 1] == '/') {
                while (m_pos < m_text.size() && m_text[m_pos] != '\n') { ++m_pos; }
                continue;
            }
            if (m_pos + 1 < m_text.size() && m_text[m_pos] == '/' && m_text[m_pos + 1] == '*') {
                const std::size_t e = m_text.find("*/", m_pos + 2);
                m_pos = e == std::string::npos ? m_text.size() : e + 2;
                continue;
            }
            break;
        }
        if (m_pos >= m_text.size()) { return {}; }
        if (m_text[m_pos] == '"') {
            const std::size_t e = m_text.find('"', m_pos + 1);
            std::string t = m_text.substr(m_pos + 1, (e == std::string::npos ? m_text.size() : e) - m_pos - 1);
            m_pos = e == std::string::npos ? m_text.size() : e + 1;
            return t;
        }
        const char c = m_text[m_pos];
        if (c == '{' || c == '}' || c == '(' || c == ')') {
            ++m_pos;
            return std::string(1, c);
        }
        const std::size_t a = m_pos;
        while (m_pos < m_text.size() && static_cast<unsigned char>(m_text[m_pos]) > ' ' && m_text[m_pos] != '{' && m_text[m_pos] != '}' &&
               m_text[m_pos] != '(' && m_text[m_pos] != ')') {
            ++m_pos;
        }
        return m_text.substr(a, m_pos - a);
    }

private:
    std::string m_text;
    std::size_t m_pos = 0;
};

bool gleichOhneFall(const std::string& a, const char* b) {
    if (a.size() != std::strlen(b)) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

void ReadConfig(Skript& s, TreePlanter& tp) {   // DTreePlanter.h ReadConfig
    std::string pToken = s.token();   // GT
    if (pToken.empty()) { return; }   // CT
    do {
        pToken = s.token();
        if (pToken.empty() || pToken[0] == '}') { break; }
        if (gleichOhneFall(pToken, "model")) {
            if (tp.modelle.size() >= 256) { return; }   // MAX_TP_MODELS
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.modelle.push_back(pToken.substr(0, 63));
        } else if (gleichOhneFall(pToken, "link")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.verknuepfung = pToken.substr(0, 63);
            tp.autoVerknuepfen = true;
        } else if (gleichOhneFall(pToken, "entity")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.klasse = pToken.substr(0, 63);
        } else if (gleichOhneFall(pToken, "offset")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.versatz = std::atoi(pToken.c_str());
        } else if (gleichOhneFall(pToken, "pitch")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.pitchMin = std::atoi(pToken.c_str());
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.pitchMax = std::atoi(pToken.c_str());
            tp.winkelSetzen = true;
        } else if (gleichOhneFall(pToken, "yaw")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.yawMin = std::atoi(pToken.c_str());
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.yawMax = std::atoi(pToken.c_str());
            tp.winkelSetzen = true;
        } else if (gleichOhneFall(pToken, "scale")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.skalaMin = static_cast<float>(std::atof(pToken.c_str()));
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.skalaMax = static_cast<float>(std::atof(pToken.c_str()));
            tp.skalaNutzen = true;
        } else if (gleichOhneFall(pToken, "numlinks")) {
            pToken = s.token();
            if (pToken.empty()) { return; }
            tp.verknuepfungen = std::atoi(pToken.c_str());
        }
    } while (true);
}

DEntity& planterWelt(const TreePlanter& tp) { return static_cast<PlanterWelt*>(tp.intern.front().get())->welt; }

bool FindDropPoint(const TreePlanter& tp, vec_t* in, vec_t* out) {   // DTreePlanter.cpp:103-146
    DPlane p1;
    DPlane p2;
    vec3_t vUp = {0, 0, 1};
    vec3_t vForward = {0, 1, 0};
    vec3_t vLeft = {1, 0, 0};
    in[2] = 65535;
    VectorCopy(in, p1.points[0]);
    VectorCopy(in, p1.points[1]);
    VectorCopy(in, p1.points[2]);
    VectorMA(p1.points[1], 20, vUp, p1.points[1]);
    VectorMA(p1.points[1], 20, vLeft, p1.points[2]);
    VectorCopy(in, p2.points[0]);
    VectorCopy(in, p2.points[1]);
    VectorCopy(in, p2.points[2]);
    VectorMA(p1.points[1], 20, vUp, p2.points[1]);
    VectorMA(p1.points[1], 20, vForward, p2.points[2]);
    p1.Rebuild();
    p2.Rebuild();
    bool found = false;
    vec3_t temp;
    vec_t dist = 0;
    for (DBrush* brush : planterWelt(tp).brushList) {
        if (brush->IntersectsWith(&p1, &p2, temp)) {
            vec3_t diff;
            VectorSubtract(in, temp, diff);
            const vec_t tempdist = VectorLength(diff);
            if (!found || tempdist < dist) {
                dist = tempdist;
                VectorCopy(temp, out);
                found = true;
            }
        }
    }
    return found;
}

}  // namespace

Meldungen treePlanterStarten(const Karte& k, const std::string& konfiguration, TreePlanter& tp) {   // DTreePlanter::DTreePlanter
    Meldungen m;
    tp = TreePlanter{};
    auto w = std::make_shared<PlanterWelt>();
    w->welt.LoadSelectedBrushes(k);
    if (w->welt.brushList.empty()) { fehl(m, "bobToolz::TreePlanter requires selected brushes to plant on!\n"); }
    tp.intern.push_back(w);
    if (!konfiguration.empty()) {
        Skript s(konfiguration);
        ReadConfig(s, tp);
    }
    if (tp.klasse.empty()) { fehl(m, "bobToolz::TreePlanter parsed no entity name from \"bt/tp_ent.txt\"\n"); }
    tp.aktiv = true;
    return m;
}

bool treePlanterKlick(Karte& k, TreePlanter& tp, const Vektor3f& weltPunkt, const Umgebung& u, Meldungen& m) {   // DTreePlanter::mouseDown (DTreePlanter.cpp:31-101)
    if (!tp.aktiv || tp.intern.empty()) { return false; }
    vec3_t pt;
    vec3_t vhit;
    for (int i = 0; i < 3; ++i) {   // vector3_snapped( windowToWorld( position ), GetGridSize )
        pt[i] = static_cast<vec_t>(u.raster * std::floor(weltPunkt[static_cast<std::size_t>(i)] / u.raster + 0.5));
    }
    bool gepflanzt = false;
    if (FindDropPoint(tp, pt, vhit) && !tp.klasse.empty()) {
        Befehl undo(k.undo, "bobToolz.treePlanter");
        vhit[2] += static_cast<float>(tp.versatz);
        char buffer[128];
        DEntity e(tp.klasse.c_str());
        std::snprintf(buffer, sizeof(buffer), "%i %i %i", static_cast<int>(vhit[0]), static_cast<int>(vhit[1]), static_cast<int>(vhit[2]));
        e.AddEPair("origin", buffer);
        if (tp.autoVerknuepfen) {
            std::shared_ptr<Entitaet> pLastEntity;
            int entpos = 0;
            for (int i = 0; i < 256; ++i) {
                std::snprintf(buffer, sizeof(buffer), tp.verknuepfung.c_str(), i);
                for (const auto& x : k.entitaeten()) {
                    if (x->wert("targetname") == buffer) {
                        entpos = i;
                        pLastEntity = x;
                        break;
                    }
                }
            }
            std::snprintf(buffer, sizeof(buffer), tp.verknuepfung.c_str(), pLastEntity ? entpos + 1 : 0);
            e.AddEPair("targetname", buffer);
            if (pLastEntity) {
                DEntity e2;
                LoadOptions o;
                o.loadPatches = true;
                (void)e2.LoadFromEntity(k, pLastEntity, o);
                e2.AddEPair("target", buffer);
                e2.RemoveFromRadiant(k);
                e2.BuildInRadiant(k, false, m);
            }
        }
        if (tp.winkelSetzen) {
            const int angleYaw = (zufall(u) % (tp.yawMax - tp.yawMin + 1)) + tp.yawMin;
            const int anglePitch = (zufall(u) % (tp.pitchMax - tp.pitchMin + 1)) + tp.pitchMin;
            std::snprintf(buffer, sizeof(buffer), "%i %i 0", anglePitch, angleYaw);
            e.AddEPair("angles", buffer);
        }
        if (!tp.modelle.empty()) {
            const int treetype = zufall(u) % static_cast<int>(tp.modelle.size());
            e.AddEPair("model", tp.modelle[static_cast<std::size_t>(treetype)].c_str());
        }
        if (tp.skalaNutzen) {
            const float scale = ((static_cast<float>(zufall(u) % 1000) * 0.001F) * (tp.skalaMax - tp.skalaMin)) + tp.skalaMin;
            std::snprintf(buffer, sizeof(buffer), "%f", scale);
            e.AddEPair("modelscale", buffer);
        }
        e.BuildInRadiant(k, false, m);
        gepflanzt = true;
    }
    return gepflanzt;   // if( m_autoLink ) DoTrainPathPlot() macht der Aufrufer
}

Meldungen dropEntities(Karte& k, const TreePlanter* tp) {   // DoDropEnts (funchandlers-GTK.cpp:675-686) + DropEntsToGround
    Meldungen m;
    if (tp == nullptr || !tp->aktiv) {
        fehl(m, "bobToolz::DropEntity error: bobToolz::TreePlanter must be active\n");
        return m;
    }
    Befehl undo(k.undo, "bobToolz.dropEntities");
    for (const auto& ent : k.entitaeten()) {   // Scene_forEachEntity( TreePlanterDropEntityIfSelected )
        if (!k.auswahl.gewaehlt(ent.get())) { continue; }   // instance.isSelected()
        const std::string origin = ent->wert("origin");
        if (origin.empty()) { continue; }
        vec3_t vec;
        vec3_t out;
        if (std::sscanf(origin.c_str(), "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) { continue; }
        if (FindDropPoint(*tp, vec, out)) {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "%g %g %g", out[0], out[1], out[2]);
            ent->setzen("origin", buffer);
        }
    }
    return m;
}

// --- Aperture Door (shapes.cpp:612-918) ----------------------------------------------------

Meldungen aperturePruefen(const Karte& k) {   // DoBuildApertureDoors (funchandlers-GTK.cpp:371-382)
    Meldungen m;
    bool ok = k.auswahl.anzahl() > 0;
    if (ok) {
        const auto p = k.auswahl.elemente().back().primitiv.lock();
        ok = p && p->art() == Primitiv::Art::Pinsel;
    }
    if (!ok) { fehl(m, "bobToolz BuildDoors: Invalid number of brushes selected, choose 1 only.\n"); }
    return m;
}

namespace {

using V3 = Vektor3d;   // DoubleVector3

V3 v3(const vec_t* a) { return V3(a[0], a[1], a[2]); }
V3 operator*(const V3& a, double s) { return V3(a[0] * s, a[1] * s, a[2] * s); }
double dot3(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 cross3(const V3& a, const V3& b) { return V3(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]); }
V3 norm3(const V3& a) {
    const double l = std::sqrt(dot3(a, a));
    return l == 0 ? a : V3(a[0] / l, a[1] / l, a[2] / l);
}
// plane3_for_points + plane3_distance_to_point (libs/math/plane.h)
double ebeneAbstand(const V3& p0, const V3& p1, const V3& p2, const V3& punkt) {
    const V3 n = norm3(cross3(p1 - p0, p2 - p0));
    return dot3(n, punkt) - dot3(n, p0);
}
// matrix4_rotate_by_axisangle (quaternion) auf eine Richtung bzw. einen Punkt um "um"
V3 drehen(const V3& v, const V3& achse, double winkel) {   // Rodrigues = quaternion_for_axisangle + matrix4_transformed_direction
    const double c = std::cos(winkel);
    const double s = std::sin(winkel);
    return v * c + cross3(achse, v) * s + achse * (dot3(achse, v) * (1 - c));
}

}  // namespace

Meldungen apertureDoor(Karte& k, const ApertureDoorRS& rs, const Umgebung& u) {
    Meldungen m = aperturePruefen(k);
    if (!m.empty()) { return m; }
    const auto& a = k.auswahl.elemente().back();
    const auto brushEnt = a.entitaet.lock();
    const auto brushInst = std::static_pointer_cast<Pinsel>(a.primitiv.lock());
    const auto float2string = [](double value) {
        char z[64];
        std::snprintf(z, sizeof(z), "%g", static_cast<float>(value));
        return std::string(z);
    };
    DBrush dbrush;
    dbrush.LoadFromBrush(brushEnt, brushInst, false);
    const DPlane* bestplane = nullptr;
    DWinding bestwinding;
    {
        vec_t bestarea = 0;
        for (const DPlane* plane : dbrush.faceList) {
            DWinding w = plane->BaseWindingForPlane();
            for (const DPlane* cplane : dbrush.faceList) {
                DPlane negcplane(*cplane);   // negate plane to keep proper side
                VectorNegate(negcplane.normal, negcplane.normal);
                negcplane._d = -negcplane._d;
                if (plane != cplane) { (void)w.ChopWindingInPlace(&negcplane, .1F); }
                if (w.numpoints < 3) { break; }
            }
            const vec_t area = w.WindingArea();
            // so bei NRC; ohne bestplane waere VectorMax( bestplane->normal ) ein Nullzeiger - dann zaehlt nur die Flaeche
            if (area > bestarea + 1 || (area > bestarea - 1 && bestplane != nullptr && VectorMax(plane->normal) > VectorMax(bestplane->normal))) {
                bestarea = area;
                bestplane = plane;
                bestwinding = std::move(w);
            }
        }
    }
    if (bestplane == nullptr) { return m; }
    Befehl undo(k.undo, "bobToolz.buildApertureDoor");
    vec3_t center;
    bestwinding.WindingCentroid(center);
    double thickness = 32;
    {
        const DPlane* negplane = nullptr;
        vec_t bestdot = 1;
        for (const DPlane* plane : dbrush.faceList) {
            const vec_t dt = DotProduct(bestplane->normal, plane->normal);
            if (dt < bestdot) {
                bestdot = dt;
                negplane = plane;
            }
        }
        if (negplane != nullptr) { thickness = -negplane->DistanceToPoint(center); }
    }
    vec_t radius = std::numeric_limits<vec_t>::max();
    for (int i = bestwinding.numpoints - 1, j = 0; j < bestwinding.numpoints; i = j++) {
        vec3_t r;
        VectorMid(bestwinding.p[static_cast<std::size_t>(i)].data(), bestwinding.p[static_cast<std::size_t>(j)].data(), r);
        VectorSubtract(r, center, r);
        radius = std::min(radius, VectorLength(r));
    }
    const V3 normal = v3(bestplane->normal);
    // rot = normalised( plane3_project_point( Plane3( normal, 0 ), axes[min_abs], axes[max_abs] ) ) * radius + center
    std::size_t minI = 0;
    std::size_t maxI = 0;
    for (std::size_t i = 1; i < 3; ++i) {
        if (std::fabs(normal[i]) < std::fabs(normal[minI])) { minI = i; }
        if (std::fabs(normal[i]) > std::fabs(normal[maxI])) { maxI = i; }
    }
    V3 achseMin(0, 0, 0);
    V3 achseMax(0, 0, 0);
    achseMin[minI] = 1;
    achseMax[maxI] = 1;
    // plane3_project_point( plane, point, direction ): point - direction * ( distance_to_point / dot( normal, direction ) )
    const V3 proj = achseMin - achseMax * (dot3(normal, achseMin) / dot3(normal, achseMax));
    const V3 cent = v3(center);
    const V3 rot = norm3(proj) * radius + cent;
    zufallStart(u);
    char teamname[256];
    std::snprintf(teamname, sizeof(teamname), "t%i", zufall(u));
    std::vector<V3> winding;
    if (rs.ausWicklung) {
        for (int i = 0; i < bestwinding.numpoints; ++i) { winding.push_back(v3(bestwinding.p[static_cast<std::size_t>(i)].data())); }
    } else {
        for (int i = 0; i < rs.segmente; ++i) {   // matrix4_pivoted_rotate_by_axisangle( mat, normal, -2pi / segments * ( i + 0.5? ), cent )
            const double w = -(2 * Q_PI) / rs.segmente * (i + (rs.startVersetzt ? 0.5 : 0));
            winding.push_back(drehen(rot - cent, normal, w) + cent);
        }
    }
    const double edgespeed0 = rs.tempoNutzen ? rs.tempo : (rs.distanzSetzen ? rs.distanz : radius) / rs.zeit;
    double edgespeed = edgespeed0;
    const auto add = [&](NeuerPinsel& b, const V3& p0, const V3& p1, const V3& p2, const std::string& tex) {
        const double x[3] = {p0[0], p0[1], p0[2]};
        const double y[3] = {p1[0], p1[1], p1[2]};
        const double z[3] = {p2[0], p2[1], p2[2]};
        AddFaceWithTexture(k, b, x, y, z, tex.c_str(), false);
    };
    for (std::size_t i = 0; i < winding.size(); ++i) {
        const V3 rot1 = winding[i];
        const V3 rot2 = winding[(i + 1) % winding.size()];
        const V3 cent_ = cent - normal * thickness;
        const V3 rot1_ = rot1 - normal * thickness;
        const V3 rot2_ = rot2 - normal * thickness;
        auto door = k.neueEntitaet("func_door", true);
        NeuerPinsel brush;
        // AABB bounds (aabb_extend_by_point_safe); gueltig = mindestens ein Punkt
        V3 bmin(1e30, 1e30, 1e30);
        V3 bmax(-1e30, -1e30, -1e30);
        bool boundsGueltig = false;
        const auto erweitern = [&](const V3& p) {
            for (std::size_t c = 0; c < 3; ++c) {
                bmin[c] = std::min(bmin[c], p[c]);
                bmax[c] = std::max(bmax[c], p[c]);
            }
            boundsGueltig = true;
        };
        std::vector<NeuerPinsel> innen;
        if (!rs.schraegeSegmente) {
            add(brush, cent, rot1, rot2, rs.hauptTextur);
            add(brush, cent_, rot2_, rot1_, rs.hauptTextur);
            add(brush, cent, cent_, rot1_, rs.randTextur);
            add(brush, cent_, cent, rot2, rs.randTextur);
            add(brush, rot2, rot1, rot1_, rs.randTextur);
            for (const V3& p : {cent, cent_, rot1, rot1_, rot2, rot2_}) { erweitern(p); }
            if (rs.innen == ApertureDoorRS::Innen::segmentiert) {
                const double depth1 = std::clamp(rs.innenTiefe1, 0.0, thickness / 2 - 1);
                const double depth2 = std::clamp(rs.innenTiefe2, 0.0, thickness / 2 - 1);
                std::size_t jend = 1;
                for (; jend < winding.size(); ++jend) {
                    const V3 r1 = winding[(i + jend + 0) % winding.size()];
                    if (ebeneAbstand(cent, cent_, rot1_, r1) > -1) { break; }
                }
                for (std::size_t j = 1; j < jend; ++j) {
                    const V3 extradepth =
                        normal * (depth1 + (depth2 - depth1) / static_cast<double>(std::max(std::size_t(1), jend - 2)) * static_cast<double>(j - 1));
                    const V3 c = cent - extradepth;
                    const V3 r1 = winding[(i + j + 0) % winding.size()] - extradepth;
                    const V3 r2 = winding[(i + j + 1) % winding.size()] - extradepth;
                    const V3 c_ = cent_ + extradepth;
                    const V3 r1_ = r1 - normal * thickness + extradepth * 2;
                    const V3 r2_ = r2 - normal * thickness + extradepth * 2;
                    for (const V3& p : {c, c_, r1, r1_, r2, r2_}) { erweitern(p); }
                    NeuerPinsel b;
                    add(b, c, r1, r2, rs.innenHauptTextur);
                    add(b, c_, r2_, r1_, rs.innenHauptTextur);
                    add(b, c, c_, r1_, rs.innenRandTextur);
                    add(b, c_, c, r2, rs.innenRandTextur);
                    add(b, r2, r1, r1_, rs.innenRandTextur);
                    innen.push_back(b);
                }
            } else if (rs.innen == ApertureDoorRS::Innen::schraeg) {
                NeuerPinsel b;
                add(b, cent, cent_, rot2, rs.innenRandTextur);
                add(b, cent, cent_, rot1_, rs.innenRandTextur);
                double mindot = 2;
                V3 r(0, 0, 0);
                for (std::size_t j = 1; j < winding.size(); ++j) {
                    const V3 r1 = winding[(i + j + 0) % winding.size()];
                    const V3 r2 = winding[(i + j + 1) % winding.size()];
                    const V3 r2_ = r2 + normal * 16;
                    if (ebeneAbstand(cent, cent_, rot1_, r1) > -1) { break; }
                    add(b, r1, r2, r2_, rs.innenRandTextur);
                    const double dt = std::fabs(dot3(norm3(rot2 - cent), norm3(r2 - cent)));
                    if (dt < mindot) {
                        mindot = dt;
                        r = r2;
                    }
                }
                const double depth1 = std::clamp(rs.innenTiefe1, 0.0, thickness / 2 - 1);
                const double depth2 = std::clamp(rs.innenTiefe2, 0.0, thickness / 2 - 1);
                add(b, cent - normal * depth1, rot2 - normal * depth1, r - normal * depth2, rs.innenHauptTextur);
                add(b, rot2_ + normal * depth1, cent_ + normal * depth1, r - normal * (thickness - depth2), rs.innenHauptTextur);
                innen.push_back(b);
                boundsGueltig = false;   // invalidate to find out via radiant means
            }
        } else {   // bounds via radiant means
            const V3 r1 = rot1 + normal * rs.schraegTiefe1;
            const V3 r2 = rot1 + cross3(normal, cent - rot1) + normal * rs.schraegTiefe2;
            const V3 r4 = cent + (cent - rot1);
            const V3 r3 = r4 + cross3(normal, cent - rot1);
            const V3 r1_ = rot1_ - normal * rs.schraegTiefe1;
            const V3 r2_ = rot1_ + cross3(normal, cent - rot1) - normal * rs.schraegTiefe2;
            const V3 r4_ = cent_ + (cent - rot1);
            const V3 r3_ = r4_ + cross3(normal, cent - rot1);
            add(brush, r1, r2, r3, rs.hauptTextur);
            add(brush, r2_, r1_, r3_, rs.hauptTextur);
            add(brush, r1, r4, r4_, rs.randTextur);
            if (!rs.schraegRund) {
                add(brush, r2, r1, r1_, rs.randTextur);
                add(brush, r3, r2, r2_, rs.randTextur);
                add(brush, r4, r3, r3_, rs.randTextur);
            } else {   // optional round outline
                for (std::size_t j = 0; j < winding.size(); ++j) {
                    const V3 w1 = winding[(i + j + 0) % winding.size()];
                    const V3 w2 = winding[(i + j + 1) % winding.size()];
                    const V3 w2_ = w2 + normal * 16;
                    if (j != 0 && ebeneAbstand(cent, cent_, rot1_, w1) > -1) { break; }
                    add(brush, w1, w2, w2_, rs.randTextur);
                }
            }
        }
        if (rs.stille == ApertureDoorRS::Stille::pinsel) {
            const V3 sa = cent + rs.stilleVersatz;
            const V3 sb = sa + V3(0, 64, 0);
            const V3 sc = sa + V3(64, 64, 0);
            const V3 sd = sa + V3(64, 0, 0);
            const V3 sa_ = sa - V3(0, 0, 64);
            const V3 sb_ = sb - V3(0, 0, 64);
            const V3 sc_ = sc - V3(0, 0, 64);
            const V3 sd_ = sd - V3(0, 0, 64);
            NeuerPinsel b;
            add(b, sa, sb, sc, "textures/common/caulk");
            add(b, sc_, sb_, sa_, "textures/common/caulk");
            add(b, sb, sa, sa_, "textures/common/caulk");
            add(b, sc, sb, sb_, "textures/common/caulk");
            add(b, sd, sc, sc_, "textures/common/caulk");
            add(b, sa, sd, sd_, "textures/common/caulk");
            innen.push_back(b);
            if (boundsGueltig) {
                for (const V3& p : {sa, sa_, sb, sb_, sc, sc_, sd, sd_}) { erweitern(p); }
            }
        }
        V3 dir;
        {
            dir = norm3(drehen(rot2 - rot1, normal, rs.oeffnungsWinkel * Q_PI / 180.0));
            V3 angles(-std::asin(dir[2]) * 180.0 / Q_PI, std::atan2(dir[1], dir[0]) * 180.0 / Q_PI, 0);
            for (std::size_t c = 0; c < 3; ++c) {   // vector3_snap_to_zero( angles, 0.0001 )
                if (std::fabs(angles[c]) < 0.0001) { angles[c] = 0; }
            }
            char value[64];
            std::snprintf(value, sizeof(value), "%g %g %g", angles[0], angles[1], angles[2]);
            door->setzen("angles", value);
        }
        door->setzen("team", teamname);
        if (rs.gesundheit) { door->setzen("health", "1"); }
        door->setzen("wait", float2string(rs.warten));
        for (NeuerPinsel& b : innen) {   // die inneren Pinsel kamen zuerst in die Tuer
            b.p->schicht = static_cast<int>(k.aktuelleSchicht);
            door->kindEinfuegen(b.p);
        }
        brush.p->schicht = static_cast<int>(k.aktuelleSchicht);
        door->kindEinfuegen(brush.p);
        k.entitaetEinfuegen(door);
        if (!boundsGueltig) {   // GlobalSceneGraph().find( path )->worldAABB()
            for (const auto& kind : door->kinder()) {
                vec3_t mn;
                vec3_t mx;
                grenzenVon(*kind, k.kontext, mn, mx);
                erweitern(v3(mn));
                erweitern(v3(mx));
            }
        }
        {
            const double speed = edgespeed / dot3(norm3(rot2 - rot1), cross3(norm3(rot1 - cent), normal));
            edgespeed = speed * dot3(norm3(rot2 - rot1), cross3(norm3(rot2 - cent), normal));
            door->setzen("speed", float2string(speed));
            const V3 abs_dir(std::fabs(dir[0]), std::fabs(dir[1]), std::fabs(dir[2]));
            const V3 extents = (bmax - bmin) * 0.5;
            const double fullDistance = dot3(abs_dir, V3(2, 2, 2) + extents * 2);   // +2 to bounds in engine
            const auto lip = static_cast<float>(fullDistance - (rs.distanzSetzen ? rs.distanz : radius) * (edgespeed0 != 0 ? speed / edgespeed0 : 0));
            door->setzen("lip", float2string(lip));
        }
    }
    return m;
}

}  // namespace bhed::karte::bobtoolz
