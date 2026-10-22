// entitaetbefehle.h - Karten-Editor: Entitaeten erzeugen, umwandeln,
// verbinden, ihre Schluessel setzen und Entitaeten auswaehlen - was der Entity
// Inspector, das Entitaetenmenue und das Entity-Menue tun.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/entity.cpp
// (EntitySetKeyValueSelected, EntitySetClassnameSelected,
// Scene_EntitySetClassname_Selected, Entity_ungroup, Entity_moveSelectedPrimitives,
// Entity_connectSelected, Entity_killconnectSelected, Entity_createFromSelection,
// Entity_ungroupSelectedPrimitives, NormalizeColor, Entity_normalizeColor,
// Entity_setColour), radiant/entityinspector.cpp (SelectedEntity_getValueForKey,
// Scene_EntitySetKeyValue_Selected_Undoable, Entity_GetKeyValues(_Selected),
// EntityInspector_applySpawnflags, EntityInspector_clearKeyValue,
// EntityInspector_clearAllKeyValues), radiant/select.cpp
// (Select_EntitiesByKeyValue, Select_ConnectedEntities,
// EntityFindByPropertyValueWalker, Scene_EntityGetPropertyValues_nonEmpty,
// ExpandSelectionToEntitiesWalker), radiant/map.cpp
// (ParentSelectedBrushesToEntityWalker, Scene_countSelectedBrushes),
// libs/scenelib.h (parentBrushes), radiant/brushmanip.cpp
// (Brush_ConstructPlacehoderCuboid), radiant/filterbar.cpp (GetCommonShader),
// plugins/entity/entity.cpp (Quake3EntityCreator::connectEntities,
// ConnectEntities) und radiant/xywindow.cpp (EntityClassMenuInserter).
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
#ifndef BHED_KARTE_ENTITAETBEFEHLE_H
#define BHED_KARTE_ENTITAETBEFEHLE_H

#include "bhed/karte/eklasse.h"
#include "bhed/karte/mathe.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class Primitiv;

// g_selectedKeyValues / g_selectedDefaultKeyValues (Entity_GetKeyValues_Selected):
// die Schluessel aller gewaehlten Entitaeten (oder der Entitaet eines
// gewaehlten Primitivs), der erste Wert gewinnt; dazu die Vorgaben der Klassen.
struct GewaehlteSchluessel {
    std::map<std::string, std::string> werte;
    std::map<std::string, std::string> vorgaben;
    // SelectedEntity_getValueForKey: Wert, sonst Vorgabe, sonst ""
    [[nodiscard]] std::string wert(const std::string& schluessel) const;
};
[[nodiscard]] GewaehlteSchluessel gewaehlteSchluessel(const Karte& k);

// Scene_EntitySetKeyValue_Selected: jede Entitaet, die gewaehlt ist oder ein
// gewaehltes Primitiv hat (leerer Wert loescht den Schluessel).
void schluesselSetzenAuswahl(Karte& k, const std::string& schluessel, const std::string& wert);
// Scene_EntitySetKeyValue_Selected_Undoable: Rueckgaengig
// "entitySetKeyValue -key \"k\" -value \"v\"".
void schluesselSetzenRueckgaengig(Karte& k, const std::string& schluessel, const std::string& wert);
// EntityInspector_clearKeyValue: nicht "classname"; "entityDeleteKey -key k"
void schluesselLoeschenAuswahl(Karte& k, const std::string& schluessel);
// EntityInspector_clearAllKeyValues: alle ausser classname und origin; "entityClear"
void alleSchluesselLoeschen(Karte& k, const GewaehlteSchluessel& g);
// EntityInspector_applySpawnflags: "entitySetSpawnflags -flags N"
void spawnflagsSetzen(Karte& k, int flags);

// Scene_EntitySetClassname_Selected: Rueckgaengig "entitySetClass -class X"
// bzw. "ungroupSelectedEntities" (worldspawn); nichts ohne Auswahl.
void klasseSetzenAuswahl(Karte& k, const std::string& klassenname);
// Scene_parentSelectedBrushesToEntity: gewaehlte Primitive ans Ende von
// ziel haengen, leer gewordene Entitaeten (nicht worldspawn) loeschen.
// Mit nurAus nur aus dieser Entitaet (Scene_parentSubgraphSelectedBrushesToEntity).
// Liefert die umgehaengten Primitive (sie sind danach nicht gewaehlt).
// Reihenfolge: die der Karte - NRC laeuft durch die nach Zeigern geordnete
// InstanceMap, deren Reihenfolge sich nicht nachbilden laesst.
std::vector<std::shared_ptr<Primitiv>> gewaehltePrimitiveUmhaengen(Karte& k, const std::shared_ptr<Entitaet>& ziel,
                                                                   const Entitaet* nurAus = nullptr);
// Scene_countSelectedBrushes (gewaehlte Primitive)
[[nodiscard]] std::size_t gewaehltePrimitive(const Karte& k);
// GetCommonShader( name ): ja.game / jk2.game -> "textures/system/<name>"
[[nodiscard]] std::string gemeinsamerShader(const std::string& name);

// Entity_createFromSelection. Das Ergebnis sagt, ob danach nach der
// Lichtstaerke (DoLightIntensityDlg) bzw. dem Modell (misc_model_dialog)
// gefragt wird; der Rueckgaengig-Schritt "entityCreate -class X" bleibt dann
// offen (offen = true), bis der Aufrufer ihn mit befehlAbschliessen beendet.
struct ErzeugtErgebnis {
    std::shared_ptr<Entitaet> entitaet;   // nullptr: nichts erzeugt
    bool lichtFragen = false;
    bool modellFragen = false;
    bool offen = false;                   // undo.start() ohne finish
    std::string befehl;                   // Name fuer finish
    std::string fehler;                   // globalErrorStream
};
ErzeugtErgebnis entitaetErzeugen(Karte& k, const std::string& name, const Vektor3f& ursprung);
void befehlAbschliessen(Karte& k, ErzeugtErgebnis& e);
// Entity_ungroupSelectedPrimitives
ErzeugtErgebnis primitiveZurWelt(Karte& k);

// Entity_moveSelectedPrimitives( toLast ): Rueckgaengig
// "movePrimitivesToEntity \"klasse\""; false mit Meldung bei weniger als zwei.
bool primitiveZurEntitaet(Karte& k, bool zurLetzten, std::string* fehler = nullptr);

// Entity_connectSelected / Entity_killconnectSelected (index 0 / 1):
// genau zwei gewaehlt, Rueckgaengig "entityConnectSelected".
bool entitaetenVerbinden(Karte& k, int index, std::string* fehler = nullptr);

// Die Entitaet des zuletzt Gewaehlten (ultimateSelected, sonst ihre Eltern-
// Entitaet bei einem Primitiv).
[[nodiscard]] std::shared_ptr<Entitaet> letzteGewaehlteEntitaet(const Karte& k);
// NormalizeColor: so skaliert, dass die groesste Komponente 1 ist (0 -> weiss)
[[nodiscard]] Vektor3f farbeNormiert(Vektor3f c);
// Entity_setColour nach dem Farbdialog: "entitySetColour r g b"
void farbeSetzenAuswahl(Karte& k, const Vektor3f& farbe);
// Entity_normalizeColor: "entityNormalizeColour r g b"; false ohne _color
bool farbeNormierenAuswahl(Karte& k, Vektor3f* neueFarbe = nullptr);

// Select_EntitiesByKeyValue (nullptr = nicht beachtet)
void waehleNachSchluesselWert(Karte& k, const char* schluessel, const char* wert);
// Select_ConnectedEntities (ohne FocusAllViews); false: nichts gefunden
bool waehleVerbundene(Karte& k, bool zielende, bool ziele, std::string* fehler = nullptr);
// Scene_ExpandSelectionToEntities
void auswahlAufEntitaetenErweitern(Karte& k);

// EntityClassMenuInserter: Klassen mit gleichem Anfang bis '_' in ein
// Untermenue, wenn die naechste denselben Anfang hat.
struct EntitaetMenuePunkt {
    std::string name;                     // Klasse bzw. Untermenue
    std::vector<std::string> kinder;      // nicht leer: Untermenue
};
[[nodiscard]] std::vector<EntitaetMenuePunkt> entitaetMenue(const std::vector<std::string>& klassen);

}  // namespace bhed::karte

#endif
