// auswahlbefehle.h - Karten-Editor: die Auswahl-Befehle aus select.cpp.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/select.cpp
// (Selection_Deselect, InvertSelectionWalker/Select_Invert, Select_AllOfType
// mit EntityGetSelectedPropertyValuesWalker, EntityFindByPropertyValueWalker
// und Scene_BrushPatchSelectByShader, HideSelectedWalker/Select_Hide/
// HideSelected, Select_ShowAllHidden, DeleteSelected/deleteSelection,
// CloneSelected/Scene_Clone_Selected/Selection_Clone(_MakeUnique),
// Copy/Paste/PasteToCamera/MoveToCamera, NudgeSelection mit AxisBase,
// Selection_NudgeZ, Selection_SnapToGrid mit SnappableSnapToGridSelected),
// radiant/map.cpp (Map_ExportSelected mit IncludeSelectedWalker,
// Map_ImportSelected mit MapMergeEntities/MapMergeAll, Map_mergeLayers,
// Map_gatherNamespaced/Map_mergeClonedNames, BasicNamespace::mergeNames,
// Node_Clone/Node_Clone_Selected), libs/uniquenames.h (Postfix, name_read,
// name_write, PostFixes, UniqueNames), plugins/entity/namekeys.h
// (keyIsNameQuake3), plugins/entity (snapto der Punktentitaeten),
// radiant/brushmanip.cpp (Brush_hasShader, Scene_BrushSelectByShader(_Component)),
// radiant/patchmanip.cpp (Scene_PatchSelectByShader).
//
// Die Zwischenablage ist programmintern (NRC: QClipboard mit dem Format
// "RadiantClippings"): kopieren() liefert den Text, einfuegen() nimmt ihn.
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
#ifndef BHED_KARTE_AUSWAHLBEFEHLE_H
#define BHED_KARTE_AUSWAHLBEFEHLE_H

#include "bhed/karte/auswahlsystem.h"

#include <map>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class AuswahlBearbeitung;
enum class Sicht : int;

// --- libs/uniquenames.h ------------------------------------------------------------

struct NamensZahl {   // Postfix: -1 = keine Zahl, Mindestbreite bei fuehrender 0
    int wert = -1;
    int mindestBreite = 0;
    explicit NamensZahl(const std::string& text = std::string());
    bool operator<(const NamensZahl& o) const { return wert < o.wert; }
    bool operator==(const NamensZahl& o) const { return wert == o.wert; }
    bool operator!=(const NamensZahl& o) const { return wert != o.wert; }
};
struct Name {   // name_t
    std::string text;
    NamensZahl zahl;
};
[[nodiscard]] Name nameLesen(const std::string& name);   // name_read
[[nodiscard]] std::string nameSchreiben(const Name& n);   // name_write
class EindeutigeNamen {   // UniqueNames
public:
    [[nodiscard]] Name eindeutig(const Name& n) const;   // make_unique
    void einfuegen(const Name& n);                        // insert
    void entfernen(const Name& n);                        // erase
    [[nodiscard]] bool leer() const { return m_namen.empty(); }

private:
    std::map<std::string, std::map<NamensZahl, unsigned int>> m_namen;
};
// keyIsNameQuake3
[[nodiscard]] bool schluesselIstName(const std::string& schluessel);

// Map_mergeClonedNames( true ): die Namen (target, targetname, killtarget,
// targetN) der neuen Entitaeten gegen die der uebrigen Karte eindeutig machen.
void namenZusammenfuehren(Karte& k, const std::vector<Entitaet*>& neue);

// --- select.cpp ------------------------------------------------------------------------

// Selection_Deselect (C, Escape)
void auswahlAufheben(Karte& k, AuswahlEinstellung& e);
// Select_Invert (I)
void auswahlUmkehren(Karte& k, const AuswahlEinstellung& e);
// Select_AllOfType (Shift+A): browserShader = TextureBrowser_GetSelectedShader
void alleVomTyp(Karte& k, const AuswahlEinstellung& e, const std::string& browserShader);
// HideSelected (H): versteckt, gibt g_nodes_be_hidden zurueck ("versteckt" vorher)
bool versteckeGewaehlte(Karte& k, bool versteckt);
// Select_ShowAllHidden (Shift+H)
void zeigeVersteckte(Karte& k);
// deleteSelection (Z, Backspace, Delete) mit Rueckgaengig "deleteSelected"
void loescheAuswahl(Karte& k, const AuswahlEinstellung& e, bool eckenSperre = false);
// Selection_Clone (Space) / Selection_Clone_MakeUnique (Shift+Space);
// befehl: eigenes Rueckgaengig "cloneSelected(MakeUnique)" (sonst Scene_Clone_Selected).
void klonen(Karte& k, const AuswahlEinstellung& e, bool eindeutig, bool befehl = true);
// Map_ExportSelected: die Auswahl als .map-Text
[[nodiscard]] std::string kopieren(const Karte& k);
// Paste: Rueckgaengig "paste", vorher alles abwaehlen, das Eingefuegte gewaehlt;
// befehl = false: nur Selection_Paste (ohne Rueckgaengig und ohne Abwaehlen).
void einfuegen(Karte& k, const std::string& text, bool befehl = true);
// NudgeSelection: Richtung 0 links, 1 hoch, 2 rechts, 3 runter (ENudgeDirection)
[[nodiscard]] Vektor3f schubsRichtung(int richtung, float menge, Sicht s);
// Selection_SnapToGrid (Ctrl+G) mit Rueckgaengig "snapSelected -grid N"
void amRasterAusrichten(Karte& k, const AuswahlEinstellung& e, float raster, bool eckenSperre = false);

// --- Shader (select.cpp, brushmanip.cpp, patchmanip.cpp) -----------------------------

// Select_SetShader_Undo: bei einer Auswahl (Objekte oder Flaechen) Rueckgaengig
// "textureNameSetSelected" und Select_SetShader; true, wenn etwas gesetzt wurde.
bool shaderSetzen(Karte& k, const AuswahlEinstellung& e, const std::string& shader);
// Scene_BrushPatchSelectByShader: sichtbare Pinsel mit einer Flaeche dieses
// Shaders und sichtbare Patches mit diesem Shader dazuwaehlen.
void waehleNachShader(Karte& k, const std::string& shader);
// Select_FacesAndPatchesByShader: Flaechen aller Pinsel (Scene_forEachBrush)
// und sichtbare Patches mit diesem Shader dazuwaehlen.
void waehleFlaechenNachShader(Karte& k, const std::string& shader);
// FindReplaceTextures (select.cpp:679-707): Rueckgaengig "textureFindReplace
// -find <f> -replace <r>"; ersetzen = "textures/" sucht nur (waehlt Flaechen).
// false (mit Meldung), wenn ein Name ungueltig ist (texdef_name_valid).
bool texturenErsetzen(Karte& k, const AuswahlEinstellung& e, const std::string& suchen, const std::string& ersetzen,
                      bool nurAusgewaehlt, std::string* fehler = nullptr);

}  // namespace bhed::karte

#endif
