// region.h - Karten-Editor: die Region (Menue "Region") und die Auswahl
// nach Kaesten (Select Inside / Select Touching, Expand Selection To
// Primitives).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/map.cpp:1189-1201
// (RegionExcluder, Map_Traverse_Region), 1286-1533 (g_region_active,
// g_region_mins/maxs, Region_defaultMinMax, ScopeRegionBrushes,
// Map_SaveRegion, exclude_node, ExcludeAllWalker, ExcludeSelectedWalker,
// ExcludeRegionedWalker, Map_RegionOff, Map_ApplyRegion,
// Map_RegionSelectedBrushes, Map_RegionXY, Map_RegionBounds, Map_RegionBrush),
// radiant/brushmanip.cpp:463-498 (g_regionBoxShader, ConstructRegionBrushes),
// radiant/select.cpp:64-210 (CollectSelectedBrushesBounds, SelectByBounds,
// SelectionPolicy_Touching, SelectionPolicy_Inside), 380-418
// (ExpandSelectionToPrimitivesWalker), 897-903 (Select_Inside,
// Select_Touching), libs/math/aabb.h:148-157 (aabb_intersects_aabb).
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
#ifndef BHED_KARTE_REGION_H
#define BHED_KARTE_REGION_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/mathe.h"

#include <array>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Pinsel;
struct Kontext;

// aabb_intersects_aabb (aabb.h:148-157)
[[nodiscard]] bool kastenSchneidet(const Kasten& a, const Kasten& b);

// Instance::worldAABB: Pinsel und Patches ihre Grenzen, Punktentitaeten ihr
// Kasten am Ursprung (GenericEntity/Light: nur verschoben), Gruppen und
// worldspawn die Vereinigung der Kinder (AABBExtendByChildren).
[[nodiscard]] Kasten instanzGrenzen(const Entitaet& e, const Kontext& k);
[[nodiscard]] Kasten instanzGrenzen(const Primitiv& p, const Kontext& k);

// --- select.cpp ------------------------------------------------------------------------

// Select_Inside: im Primitiv-Modus die Kaesten der gewaehlten Pinsel nehmen,
// die Pinsel loeschen (Rueckgaengig "deleteSelected") und alles waehlen, was
// ganz in einem Kasten liegt (SelectionPolicy_Inside).
void auswahlInnen(Karte& k, const AuswahlEinstellung& e);
// Select_Touching: dasselbe ohne Loeschen, mit SelectionPolicy_Touching.
void auswahlBeruehrend(Karte& k, const AuswahlEinstellung& e);
// Scene_ExpandSelectionToPrimitives (Ctrl+E)
void auswahlAufPrimitiveErweitern(Karte& k);

// --- map.cpp: Region ---------------------------------------------------------------------

struct Region {
    bool aktiv = false;                          // g_region_active
    Vektor3f min{-65536.0F, -65536.0F, -65536.0F};   // g_region_mins
    Vektor3f max{65536.0F, 65536.0F, 65536.0F};      // g_region_maxs
};

// Region_defaultMinMax: ohne aktive Region +-GetMaxGridCoord()
void regionStandard(Region& r, float maxRaster);
// Map_RegionOff (Scene_Exclude_All( false ))
void regionAus(Karte& k, Region& r, float maxRaster);
// Map_ApplyRegion (Scene_Exclude_Region( false ), worldspawn bleibt sichtbar)
void regionAnwenden(Karte& k, Region& r);
// Map_RegionSelectedBrushes (Ctrl+Shift+R): nichts gewaehlt -> Map_RegionOff
void regionAuswahl(Karte& k, Region& r, float maxRaster);
// Map_RegionXY: mit dem vorigen Bereich geschnitten
void regionXY(Karte& k, Region& r, const Vektor3f& min, const Vektor3f& max);
// Map_RegionBounds
void regionGrenzen(Karte& k, Region& r, const Kasten& b, float maxRaster);
// Map_RegionBrush: der zuletzt gewaehlte bestimmt die Region und wird
// geloescht ("deleteSelected"); false = "Nothing is selected!"
bool regionPinsel(Karte& k, Region& r, float maxRaster, const AuswahlEinstellung& e);

// g_regionBoxShader: leer -> texdef_name_default ("textures/"), ungueltig ->
// Warnung und ebenso "textures/".
[[nodiscard]] std::string regionKastenShader(const std::string& einstellung, std::string* warnung);
// ConstructRegionBrushes: sechs Waende um min..max (32 dick)
void regionPinselBauen(const std::array<Pinsel*, 6>& pinsel, const Vektor3f& min, const Vektor3f& max, const std::string& shader,
                       const Kontext& k);
// Map_SaveRegion: ScopeRegionBrushes (sechs Waende in worldspawn, ein
// info_player_start an der Kamera) und nur, was nicht ausgeschlossen ist
// (Map_Traverse_Region). kameraGier in Grad (CAMERA_YAW).
[[nodiscard]] std::string regionText(Karte& k, const Region& r, const Vektor3f& kameraUrsprung, float kameraGier,
                                     const std::string& kastenShader, std::vector<std::string>* meldungen = nullptr);
// Stufe 6 (NRC#132, ABSICHT in NRC): leckt der Region-Bau? Der Startpunkt der Region-Karte
// (die Kamera, in den Kasten gezogen wie in regionText) liegt in einem festen
// strukturellen Pinsel, und keine andere Entity der Region steht im Freien - dann findet
// q3map2 (FloodEntities) keine Luft und meldet ein Leck. Fest = nicht detail, deckender
// Shader, kein Werkzeug (shaderArtNachName des Optimierers).
[[nodiscard]] bool regionStartImFesten(const Karte& k, const Region& r, const Vektor3f& kameraUrsprung);

}  // namespace bhed::karte

#endif
