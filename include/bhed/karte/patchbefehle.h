// patchbefehle.h - Karten-Editor: die Patch-Befehle der Szene (Curve-Menue).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/patchmanip.cpp:49-305
// (Scene_PatchConstructPrefab, Patch_makeCaps, Scene_PatchDoCap_Selected,
// Patch_deform, Patch_thicken, Scene_Patch*_Selected), :374-560
// (PatchCreator_getBounds, Patch_* mit ihren Rueckgaengig-Namen) und
// radiant/surfacedialog.cpp:413-420 (SurfaceInspector_ResetTexture).
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
#ifndef BHED_KARTE_PATCHBEFEHLE_H
#define BHED_KARTE_PATCHBEFEHLE_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/protokoll.h"

#include <cstddef>
#include <string>

namespace bhed::karte {

class Karte;

// PatchCreator_getBounds (patchmanip.cpp:374-391): der Arbeitsbereich, leere
// Ausdehnungen mit dem Raster, ungueltig: 0 0 0 +- 64.
[[nodiscard]] Kasten patchGrenzen(const Kasten& arbeitsbereich, float raster);

// Scene_PatchConstructPrefab (patchmanip.cpp:49-82) ohne Befehl: der Patch
// kommt in die Entitaet des zuletzt gewaehlten Primitivs (sonst worldspawn),
// die Auswahl wird geloescht (Select_Delete), der Patch gewaehlt.
void patchVorlage(Karte& k, const Kasten& grenzen, const std::string& shader, PatchVorlage art, int achse, std::size_t breite = 3,
                  std::size_t hoehe = 3, bool neuVerteilen = false);
// Die Befehle mit Vorlage aus dem Menue (Patch_Cylinder usw.): Rueckgaengig
// "patchCreateCylinder", "patchCreateSquareCylinder", "patchCreateEndCap",
// "patchCreateBevel", "patchCreateSphere", "patchCreateCone"; und die Dialog-
// Varianten (DoNewPatchDlg) "patchCreatePlane", "patchCreateXactCylinder", ...
void patchVorlageBefehl(Karte& k, const Kasten& grenzen, const std::string& shader, PatchVorlage art, int achse, std::size_t breite = 3,
                        std::size_t hoehe = 3, bool neuVerteilen = false);
// Grenzen von DoNewPatchDlg je Vorlage (min/Vorgabe/max Zeilen, Spalten)
struct PatchDichte {
    int minZeilen;
    int minSpalten;
    int vorgabeZeilen;
    int vorgabeSpalten;
    int maxZeilen;    // 0: bis 31
    int maxSpalten;
};
[[nodiscard]] PatchDichte patchDichte(PatchVorlage art);

// Scene_PatchDoCap_Selected (patchmanip.cpp:143-149) mit Patch_makeCaps: zwei
// Kappen je sichtbarem gewaehltem Patch ("patchPutCaps"); Fehler wie NRC.
void patchKappen(Karte& k, const std::string& shader, PatchKappe art, Protokoll& p);
// Scene_PatchDeform (patchmanip.cpp:161-169): "patchDeform"; Zufall aus rand()
void patchVerformen(Karte& k, int staerke, int achse);
// Scene_PatchThicken (patchmanip.cpp:233-241): "patchThicken"; achse 3 = Normalen
void patchVerdicken(Karte& k, int dicke, bool waende, int achse);

// Einfache Befehle an jedem sichtbaren gewaehlten Patch mit dem NRC-Namen:
// "patchInsertFirstColumns" ... "patchDeleteLastRows", "patchInvert",
// "patchRedisperseRows/Columns", "patchSmoothRows/Columns", "patchTranspose",
// "patchFlipTextureU/V", "patchNaturalTexture", "patchCapTexture".
enum class PatchBefehl {
    SpaltenVornEinfuegen, SpaltenHintenEinfuegen, ZeilenVornEinfuegen, ZeilenHintenEinfuegen,
    SpaltenVornLoeschen, SpaltenHintenLoeschen, ZeilenVornLoeschen, ZeilenHintenLoeschen,
    Umkehren, ZeilenVerteilen, SpaltenVerteilen, ZeilenGlaetten, SpaltenGlaetten, Transponieren,
    TexturX, TexturY, TexturNatuerlich, TexturKappen
};
void patchBefehl(Karte& k, PatchBefehl b);

// SurfaceInspector_ResetTexture (surfacedialog.cpp:413-420): "textureReset/Cap"
void texturZuruecksetzenKappen(Karte& k, const AuswahlEinstellung& e);

}  // namespace bhed::karte

#endif
