// csg.h - Karten-Editor: CSG (Subtract, Merge, Wrap Merge, Intersect, Make
// Room, Hollow, Shrink/Expand) und das Teilen durch eine Ebene (Clipper).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/csg.cpp (HollowSettings,
// CaulkFace, FaceMakeBrush, brush_extrudeDiag, FaceOffset,
// BrushHollowSelectedWalker, BrushGatherSelected, BrushDeleteSelected,
// Brush_testPlane, Brush_classifyPlane, Brush_subtract,
// SubtractBrushesFromUnselected, CSG_Subtract, BrushSplitByPlaneSelected,
// Scene_BrushSplitByPlane, BrushInstanceSetClipPlane, Brush_merge,
// ultimate_group_path, CSG_Merge, MergeVertices, Scene_gatherSelectedComponents,
// MergePlane(s), CSG_build_hull, CSG_WrapMerge, CSG_Intersect, BrushFaceOffset,
// CSG_MakeRoom, CSGdlg_getSettings, CSG_Hollow, CSGdlg_BrushShrink/-Expand)
// und radiant/clippertool.h (ClipperPoints).
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
#ifndef BHED_KARTE_CSG_H
#define BHED_KARTE_CSG_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/protokoll.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;

// ClipperPoints (clippertool.h:26-50)
struct ClipperPunkte {
    Vektor3d punkte[3] = {Vektor3d(0, 0, 0), Vektor3d(0, 0, 0), Vektor3d(0, 0, 0)};
    std::size_t anzahl = 0;
};

// Was die CSG-Befehle aus der Oberflaeche brauchen (TextureBrowser_GetSelectedShader,
// GetCaulkShader, GetGridSize, Mode() == ePrimitive).
struct CsgUmgebung {
    std::string browserShader = "textures/";
    std::string caulkShader = "textures/common/caulk";
    float raster = 8;
    bool primitivModus = true;
};

// EHollowType (csg.cpp:64-70)
enum class HohlArt { Diagonal = 0, Huelle = 1, Extrudieren = 2, Ziehen = 3 };

// HollowSettings (csg.cpp:71-121) mit den Werten des CSG-Tool-Dialogs
// (CSGdlg_getSettings, csg.cpp:1416-1436)
struct HohlEinstellung {
    HohlArt art = HohlArt::Ziehen;
    float versatz = 16;
    bool nachAchse = false;              // m_excludeByAxis
    Vektor3f achse{0, 0, 1};             // m_exclusionAxis
    bool gewaehlteAusschliessen = true;  // m_excludeSelectedFaces
    bool caulk = true;
    bool innenEntfernen = true;          // m_removeInner
};

// CSG_Subtract (csg.cpp:710-733): Rueckgaengig "brushSubtract"
void csgSubtrahieren(Karte& k, Protokoll& p);
// CSG_Merge (csg.cpp:937-981): "brushMerge"
void csgVereinigen(Karte& k, Protokoll& p);
// CSG_WrapMerge (csg.cpp:1115-1214) mit den Clipper-Punkten (ohne Befehl);
// csgHuellenVereinigen mit Rueckgaengig "brushWrapMerge".
void csgHuelleBauen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkte, Protokoll& p);
void csgHuellenVereinigen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkte, Protokoll& p);
// CSG_Intersect (csg.cpp:1216-1259): "brushIntersect"
void csgSchneiden(Karte& k, Protokoll& p);
// CSG_MakeRoom (csg.cpp:1401-1414): "makeRoom"
void csgRaumBauen(Karte& k, const CsgUmgebung& u);
// CSG_Hollow (csg.cpp:1438-1447) mit dem Rueckgaengig-Namen
void csgAushoehlen(Karte& k, const CsgUmgebung& u, const HohlEinstellung& e, const std::string& rueckgaengig);
// CSGdlg_BrushShrink / CSGdlg_BrushExpand (csg.cpp:1451-1467): "Shrink brush" / "Expand brush"
void csgPinselVersetzen(Karte& k, const HohlEinstellung& e, bool schrumpfen);

// Scene_BrushSplitByPlane (csg.cpp:795-806): Clip (split = false) oder Split;
// schneidet die Ebene nichts, macht es CSG_WrapMerge mit den Punkten.
void pinselTeilen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkte, bool umgedreht, bool caulk, bool teilen, Protokoll& p);
// Scene_BrushSetClipPlane: die Ebene, die BrushInstance::setClipPlane bekommt.
[[nodiscard]] Ebene3d clipperEbene(const ClipperPunkte& punkte, bool umgedreht);
// BrushClipPlane::setPlane (brush.h:3058-3066): die Wicklung der Ebene in einem Pinsel.
void clipperWicklung(const Pinsel& pinsel, const Kontext& k, const Ebene3d& ebene, Wicklung& w);

// Brush_classifyPlane (csg.cpp:533-543)
[[nodiscard]] PinselTeilung pinselEinordnen(const Pinsel& p, const Kontext& k, const Ebene3d& e);
// Brush_merge (csg.cpp:840-917)
[[nodiscard]] bool pinselVereinigen(Pinsel& ziel, const std::vector<std::shared_ptr<Pinsel>>& ein, bool nurForm, const Kontext& k);

}  // namespace bhed::karte

#endif
