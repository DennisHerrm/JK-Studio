// spielkollision.h - Karten-Editor, Stufe 8b Punkt 3 ("Im Editor spielen"):
// die Kollision direkt aus den Brushes und Patches der Karte, ohne Kompilieren.
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114, tools/quake3/q3map2):
//   games.cpp (game_sof2 / game_jk2 / game_ja: die surfaceparm-Tabelle und
//   "default"), shaders.cpp (ApplySurfaceParm, AllocShaderInfo), map.cpp
//   (SetBrushContents, AddBrushBevels, SnapNormal/SnapPlaneImproved,
//   PlaneEqual), writebsp.cpp (EmitBrushes: der Inhalt des Brushes ist der
//   seines contentShader), surface.cpp (Patches werden auch nodraw
//   ausgegeben);
// und der Aufbau der Kollisionskarte wie CM_LoadMap (OpenJK
// code/qcommon/cm_load.cpp: CMod_LoadBrushes/CM_BoundBrush mit den Seiten
// 0..5 als Kasten, CMod_LoadPatches mit CM_GeneratePatchCollide) fuer den
// vorhandenen CM-Port (src/kollision.cpp).
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
#ifndef BHED_KARTE_SPIELKOLLISION_H
#define BHED_KARTE_SPIELKOLLISION_H

#include "bhed/kollision.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;

// q3map2: contentFlags, surfaceFlags, compileFlags eines Shaders
struct Q3ShaderFlags {
    std::uint32_t inhalt = 0;
    std::uint32_t flaeche = 0;
    std::uint32_t compile = 0;
};
// AllocShaderInfo ("default") und ApplySurfaceParm je surfaceparm, wie
// game_sof2 (JK2) bzw. game_ja (dazu "inside", "forcesight").
[[nodiscard]] Q3ShaderFlags q3map2ShaderFlags(const std::vector<std::string>& surfaceparms, bool ja);

// SetBrushContents (map.cpp:455-503): welche Seite den Inhalt des Brushes
// bestimmt (contentShader), die vereinigten compileFlags, und ob es ein
// Ursprungs-Brush ist (C_ORIGIN: q3map2 entfernt ihn).
struct PinselInhalt {
    int seite = 0;                 // Index der Seite des contentShader
    Q3ShaderFlags flags;           // dessen Flags = der Inhalt des Brushes in der .bsp
    std::uint32_t compile = 0;     // compileFlags aller Seiten
    bool ursprung = false;
};
[[nodiscard]] PinselInhalt pinselInhalt(const std::vector<Q3ShaderFlags>& seiten);

// Liefert die surfaceparms eines Shaders (leer: nicht in den Skripten = "default").
using SurfaceparmFn = std::function<std::vector<std::string>(const std::string& shader)>;

struct SpielKollisionsBericht {
    int pinsel = 0;         // Brushes in der Kollisionskarte
    int seiten = 0;         // Seiten samt Bevels
    int bevels = 0;         // AddBrushBevels (Kasten- und Kantenbevels)
    int patches = 0;
    int ausgelassen = 0;    // trigger_*, Ursprungs-Brushes (C_ORIGIN), ungueltige
};

// Die Kollisionskarte aus allen Brushes und Patches von worldspawn,
// func_group und den Brush-Entities (ausser trigger_*: InitTrigger setzt
// CONTENTS_TRIGGER). Ein Baum mit einem Blatt: das Ergebnis jeder Spur ist das
// des Spiels (der BSP-Baum beschleunigt nur), nur bei genau gleich weiten
// Treffern kann die gemeldete Ebene eine andere sein.
SpielKollisionsBericht kollisionAusKarte(const Karte& k, const SurfaceparmFn& parms, bool ja, KollisionsKarte& aus);

}  // namespace bhed::karte

#endif
