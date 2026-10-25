// patchkollision.h - Karten-Editor, Stufe 5 Punkt 2: wie viele
// Kollisionsebenen und -facetten erzeugt die Engine fuer einen Patch?
//
// Portiert aus OpenJK code/qcommon/cm_patch.cpp (CM_GeneratePatchCollide mit
// CM_SetGridWrapWidth, CM_SubdivideGridColumns, CM_RemoveDegenerateColumns,
// CM_TransposeGrid, CM_PatchCollideFromGrid, CM_FindPlane, CM_FindPlane2,
// CM_PlaneEqual, CM_SnapVector, CM_EdgePlaneNum, CM_SetBorderInward,
// CM_ValidateFacet, CM_AddFacetBevels) und code/qcommon/cm_polylib.cpp
// (BaseWindingForPlane, ChopWindingInPlace, WindingBounds), Zeile fuer Zeile
// in float wie die Engine. Statt Com_Error bei MAX_PATCH_PLANES / MAX_FACETS
// wird weitergezaehlt (Felder ohne feste Groesse) - das Ergebnis ist die Zahl,
// die die Engine erreichen WUERDE; ueber der Grenze bricht sie ab.
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
#ifndef BHED_KARTE_PATCHKOLLISION_H
#define BHED_KARTE_PATCHKOLLISION_H

#include <array>
#include <vector>

namespace bhed::karte {

struct PatchKollision {
    bool gueltig = false;     // CM_GeneratePatchCollide: bad parameters / even sizes / > CM_MAX_GRID_SIZE
    int ebenen = 0;           // numPlanes (MAX_PATCH_PLANES)
    int facetten = 0;         // numFacets (MAX_FACETS)
    int gitterBreite = 0;     // Gitter nach dem Unterteilen (cGrid_t, CM_MAX_GRID_SIZE 129)
    int gitterHoehe = 0;
    int groessteGitterSeite = 0;   // groesste Breite waehrend des Unterteilens (grid->points[129][129])
    int meisteRaender = 0;    // numBorders einer Facette (Feld 4 + 6 + 16; mehr: "too many bevels")
    int windungsPunkte = 0;   // groesste Wicklung (MAX_POINTS_ON_WINDING 64)
};

// punkte: Breite * Hoehe Kontrollpunkte, zeilenweise (points[j * width + i]),
// wie CMod_LoadPatches sie aus den Zeichenpunkten der BSP liest.
[[nodiscard]] PatchKollision patchKollision(int breite, int hoehe, const std::vector<std::array<float, 3>>& punkte);

}  // namespace bhed::karte

#endif
