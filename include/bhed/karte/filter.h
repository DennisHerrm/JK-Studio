// filter.h - Karten-Editor: die Filter (Menue Filter, Filterleiste).
//
// Portiert aus NetRadiant-Custom 20260114: include/ifilter.h (EXCLUDE_*),
// radiant/filters.cpp (g_filters_globals.exclude, ToggleFilterFlag,
// add_filter_command, InvertFilters, ResetFilters, Filters_constructMenu,
// ConstructFilters mit den Tasten), radiant/brush.cpp:310-383
// (FaceFilterWrapper, face_filtered, BrushFilterWrapper, brush_filtered),
// radiant/brushmanip.cpp:983-1188 (filter_face_shader, _shader_prefix,
// _flags, _contents, filter_brush_any_face, filter_brush_all_faces und
// BrushFilters_construct), radiant/patch.cpp:3283-3313 und
// radiant/patchmanip.cpp:623-667 (PatchFilters_construct),
// plugins/entity/entity.cpp:285-393 und plugins/entity/filters.cpp
// (Entity_InitFilters, entity_filtered), radiant/brush.h:1022-1027,
// 1759-1769, radiant/patch.h:585-593, plugins/entity/filters.h:57-65
// (updateFiltered: Node::eFiltered bzw. Face::m_filtered).
//
// Spieldaten wie ja.game/jk2.game: type "ja"/"jk2" (nicht doom3),
// brushtypes und entities "quake3".
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
#ifndef BHED_KARTE_FILTER_H
#define BHED_KARTE_FILTER_H

#include <cstddef>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class Flaeche;
class Pinsel;
class Patch;
struct Kontext;

// include/ifilter.h
enum FilterMaske : unsigned int {
    EXCLUDE_WORLD = 0x00000001,
    EXCLUDE_ENT = 0x00000002,
    EXCLUDE_CURVES = 0x00000004,
    EXCLUDE_TRANSLUCENT = 0x00000008,
    EXCLUDE_LIQUIDS = 0x00000010,
    EXCLUDE_CAULK = 0x00000020,
    EXCLUDE_CLIP = 0x00000040,
    EXCLUDE_PATHS = 0x00000080,
    EXCLUDE_LIGHTS = 0x00000100,
    EXCLUDE_DETAILS = 0x00000200,
    EXCLUDE_HINTSSKIPS = 0x00000400,
    EXCLUDE_MODELS = 0x00000800,
    EXCLUDE_AREAPORTALS = 0x00001000,
    EXCLUDE_TRIGGERS = 0x00002000,
    EXCLUDE_CLUSTERPORTALS = 0x00004000,
    EXCLUDE_TERRAIN = 0x00008000,
    EXCLUDE_LIGHTGRID = 0x00010000,
    EXCLUDE_STRUCTURAL = 0x00020000,
    EXCLUDE_BOTCLIP = 0x00040000,
    EXCLUDE_VISPORTALS = 0x00080000,
    EXCLUDE_DECALS = 0x00100000,
    EXCLUDE_FUNC_GROUPS = 0x00200000,
    EXCLUDE_POINT_ENT = 0x00400000,
    EXCLUDE_SKY = 0x00800000,
};

// add_filter_command (ConstructFilters): Befehl, Maske, Taste - in der
// Reihenfolge von NRC fuer ein Quake-3-Spiel (ja, jk2).
struct FilterBefehl {
    const char* name;
    unsigned int maske;
    const char* taste;
};
[[nodiscard]] const std::vector<FilterBefehl>& filterBefehle();
[[nodiscard]] const FilterBefehl* filterBefehl(const std::string& name);

// InvertFilters: jeder ToggleFilterFlag einmal umgeschaltet
[[nodiscard]] unsigned int filterUmkehren(unsigned int maske);

// Die Filter selbst (mit der aktiven Maske g_filters_globals.exclude)
[[nodiscard]] bool flaecheGefiltert(const Flaeche& f, unsigned int maske, const Kontext& k);   // face_filtered
[[nodiscard]] bool pinselGefiltert(const Pinsel& p, unsigned int maske, const Kontext& k);    // brush_filtered
[[nodiscard]] bool patchGefiltert(const Patch& p, unsigned int maske, const Kontext& k);      // patch_filtered
[[nodiscard]] bool entitaetGefiltert(const Entitaet& e, unsigned int maske);                  // entity_filtered

// UpdateFilters: jedes Filterable (Entitaeten, Pinsel, Flaechen, Patches)
// setzt seinen Zustand neu (Node::eFiltered, Face::m_filtered).
void filterAnwenden(Karte& k, unsigned int maske);

}  // namespace bhed::karte

#endif
