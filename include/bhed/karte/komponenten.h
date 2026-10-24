// komponenten.h - Karten-Editor: die Komponenten der Auswahl (Ecken, Kanten,
// Flaechen der Pinsel, Kontrollpunkte der Patches).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/brush.h (BrushInstance:
// isSelectedComponents, setSelectedComponents, invertComponentSelection,
// getSelectedComponentsBounds, gatherSelectedComponents, insert_vertices,
// remove_vertices, snapComponents; FaceInstance::iterate_selected,
// gatherSelectedComponents, snapComponents, transformComponents), radiant/
// patch.h (PatchInstance: dasselbe fuer die Kontrollpunkte), radiant/
// selection.cpp (RadiantSelectionSystem::setSelectedAllComponents,
// countSelectedComponents, Scene_SelectAll_Component, bounds_selected_component),
// radiant/select.cpp (InvertSelectionWalker eComponent,
// Scene_SnapToGrid_Component_Selected) und radiant/csg.cpp (CSG_DeleteComponents).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
//
// Wie NRC liegt die Wahl von Ecken und Kanten in den Flaechen der Pinsel
// (FaceInstance::m_vertexSelection / m_edgeSelection), die der Kontrollpunkte
// im Patch (PatchControlInstance) und die der ganzen Flaechen in
// Karte::auswahl (g_SelectedFaceInstances).
#ifndef BHED_KARTE_KOMPONENTEN_H
#define BHED_KARTE_KOMPONENTEN_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/pinsel.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace bhed::karte {

class Karte;
class Patch;

// Ein gewaehlter Pinsel oder Patch (Instance_isSelected) mit seiner Entitaet.
struct GewaehltesPrimitiv {
    std::shared_ptr<Entitaet> entitaet;
    std::shared_ptr<Primitiv> primitiv;
};
// Scene_forEachSelectedBrush / -Patch: die gewaehlten Primitive in
// Auswahlreihenfolge (RadiantSelectionSystem::m_selection), nur sichtbare.
[[nodiscard]] std::vector<GewaehltesPrimitiv> gewaehltePrimitiveListe(const Karte& k);

// BrushInstance::isSelectedComponents (brush.h:3666-3668)
[[nodiscard]] bool pinselKomponentenGewaehlt(const Karte& k, const Pinsel& p);
// countSelectedComponents (selection.cpp:1178-1180): die Zahl der gewaehlten
// Komponenten-Waehlbaren (FaceInstance::m_selectable, m_selectableVertices,
// m_selectableEdges, PatchControlInstance::m_selectable).
[[nodiscard]] std::size_t komponentenAnzahl(const Karte& k);
// Scene_SelectAll_Component( false, mode ) (selection.cpp:966-985)
void komponentenAbwaehlen(Karte& k, KomponentenModus modus);
// setSelectedAllComponents( false ) (selection.cpp:1225-1231) ohne den Manipulator
void alleKomponentenAbwaehlen(Karte& k);
// InvertSelectionWalker eComponent (select.cpp:296-309): Komponenten der
// gewaehlten Pinsel (und im Ecken-Modus der Patches) umkehren.
void komponentenUmkehren(Karte& k, KomponentenModus modus);
// FaceInstance::iterate_selected / PatchInstance::getSelectedComponentsBounds
// ueber alle gewaehlten Primitive (bounds_selected_component, selection.cpp:2129-2146).
[[nodiscard]] Kasten komponentenGrenzen(const Karte& k);
// BrushInstance::gatherSelectedComponents (brush.h:3922-3928) mit
// FaceInstance::gatherSelectedComponents (brush.h:2764-2783)
void komponentenSammeln(const Karte& k, const Pinsel& p, const std::function<void(const Vektor3d&)>& f);
// Scene_SnapToGrid_Component_Selected (select.cpp:1536-1557):
// BrushInstance::snapComponents (brush.h:3976-3986), PatchInstance::snapComponents.
void komponentenEinrasten(Karte& k, float raster, bool eckenSperre);
// CSG_DeleteComponents (csg.cpp:1341-1343) mit BrushInstance::remove_vertices
// (brush.h:3949-3975).
void eckenEntfernen(Karte& k, bool eckenSperre);
// BrushInstance::insert_vertices (brush.h:3930-3947) samt dem sofortigen
// Huellenbau, den NRC beim naechsten evaluateBRep macht (vertexModeTransform
// mit der Einheitsmatrix). Der Ecken-Modus bleibt an, bis das Ziehen endet.
void eckenEinfuegen(Pinsel& p, const std::vector<Pinsel::EckenModusPunkt>& neue, const Kontext& k, bool eckenSperre);

}  // namespace bhed::karte

#endif
