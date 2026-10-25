// kleineplugins.h - Karten-Editor, Stufe 4 Punkt 3: die kleinen Plugins von
// NetRadiant-Custom ohne Oberflaeche - ShaderPlug, SunPlug (ET-MapCoordinator)
// und UFO:AI.
//
// Portiert aus NetRadiant-Custom 20260114:
//   contrib/shaderplug/shaderplug.cpp (CreateTagFile, LoadTextureFile,
//   LoadShaderList),
//   contrib/sunplug/sunplug.cpp (GetOptimalCoordinates, MapCoordinator),
//   contrib/ufoaiplug/ufoai_level.cpp (assign_default_values_to_worldspawn,
//   check_map_values, get_team_count, check_entity_flags) und
//   ufoai_filters.cpp (filter_level, filter_stepon, filter_nodraw,
//   filter_actorclip, filter_weaponclip).
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
#ifndef BHED_KARTE_KLEINEPLUGINS_H
#define BHED_KARTE_KLEINEPLUGINS_H

#include "bhed/karte/mathe.h"

#include <array>
#include <set>
#include <string>
#include <vector>

namespace bhed::karte {
class Karte;
class Entitaet;
class TexturTags;
}

namespace bhed::karte::plugins {

// --- ShaderPlug ----------------------------------------------------------------
struct ShaderplugListe {
    std::set<std::string> shader;     // std::set<CopiedString> shaders
    std::set<std::string> texturen;   // std::set<CopiedString> textures
};
// LoadShaderList (nur "textures/...") und LoadTextureFile ("textures/" + Datei
// ohne die letzten 4 Zeichen, wenn es keinen Shader dieses Namens gibt).
// texturDateien: relativ zu "textures/", in der Reihenfolge jpg, tga, png.
[[nodiscard]] ShaderplugListe shaderplugSammeln(const std::vector<std::string>& shaderNamen, const std::vector<std::string>& texturDateien);
// CreateXmlDocument + AddShaderNode: erst die Texturen, dann die Shader, je rueckwaerts
void shaderplugTags(const ShaderplugListe& l, TexturTags& tags);
extern const char* const kShaderplugAbout;

// --- SunPlug -----------------------------------------------------------------------
struct Kartenkoordinaten {
    int minX = 0, minY = 0, maxX = 0, maxY = 0;   // calc_min, calc_max (BasicVector2<int>)
};
// GetOptimalCoordinates: ein Quadrat um die Mitte, halbe Seite wenigstens 175
[[nodiscard]] Kartenkoordinaten optimaleKoordinaten(const Kasten& grenzen);
// "mapcoordsmins" / "mapcoordsmaxs" (StringTokeniser + atoi): {minX, minY, maxX, maxY}
[[nodiscard]] std::array<int, 4> kartenkoordinatenLesen(const Entitaet& welt);
extern const char* const kSunplugAbout;

// --- UFO:AI ------------------------------------------------------------------------
constexpr int kSurfNodraw = 0x80;            // SURF_NODRAW
constexpr int kContentsLevel1 = 0x0100;      // CONTENTS_LEVEL1 .. LEVEL8 = 0x0100 << (n - 1)
constexpr int kContentsActorclip = 0x10000;
constexpr int kContentsWeaponclip = 0x2000000;
constexpr int kContentsStepon = 0x40000000;
struct UfoaiZustand {
    bool actorclip = false, stepon = false, nodraw = false, weaponclip = false;   // *_active
    int level = 0;                                                               // level_active
};
// assign_default_values_to_worldspawn: die Nachricht ("" = keine), Konsole in zeilen
[[nodiscard]] std::string ufoaiWeltWerte(Karte& k, bool ueberschreiben, std::vector<std::string>& zeilen);
// check_map_values
[[nodiscard]] std::string ufoaiPruefen(Karte& k, std::vector<std::string>& zeilen);
// filter_level( CONTENTS_LEVELn )
void ufoaiLevel(Karte& k, UfoaiZustand& z, int flag, std::vector<std::string>& zeilen);
// filter_stepon (0), filter_actorclip (1), filter_weaponclip (2), filter_nodraw (3)
void ufoaiFilter(Karte& k, UfoaiZustand& z, int art, std::vector<std::string>& zeilen);
// Der Text von "About" (Build-Datum, Programmversion)
[[nodiscard]] std::string ufoaiAbout(const std::string& programmVersion);

}  // namespace bhed::karte::plugins

#endif
