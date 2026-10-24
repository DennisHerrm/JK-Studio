// filter.cpp - Karten-Editor: die Filter.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in filter.h).
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
#include "bhed/karte/filter.h"

#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texturen.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>

namespace bhed::karte {

namespace {

// shader_equal / shader_equal_n (string_equal_nocase(_n))
bool gleichOhneFall(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != 0; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return i == a.size() && b[i] == 0;
}
bool praefixOhneFall(const std::string& a, const char* b) {   // shader_equal_n( a, b, strlen( b ) )
    const std::size_t n = std::strlen(b);
    if (a.size() < n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

// face.getShader().shaderFlags()
unsigned int shaderFlaggen(const std::string& shader, const Kontext& k) {
    const auto* inv = dynamic_cast<const Texturinventar*>(k.texturen);
    if (inv == nullptr) { return 0; }
    const ShaderEintrag* s = inv->shaderFinden(shader);
    return s != nullptr ? s->flags : 0;
}

// FaceFilter
using FlaechenRegel = std::function<bool(const Flaeche&, const Kontext&)>;
FlaechenRegel shaderRegel(const char* s) {   // filter_face_shader
    return [s](const Flaeche& f, const Kontext&) { return gleichOhneFall(f.shader(), s); };
}
FlaechenRegel praefixRegel(const char* s) {   // filter_face_shader_prefix
    return [s](const Flaeche& f, const Kontext&) { return praefixOhneFall(f.shader(), s); };
}
FlaechenRegel flaggenRegel(unsigned int flaggen) {   // filter_face_flags
    return [flaggen](const Flaeche& f, const Kontext& k) { return (shaderFlaggen(f.shader(), k) & flaggen) != 0; };
}
FlaechenRegel detailRegel() {   // filter_face_contents( BRUSH_DETAIL_MASK )
    return [](const Flaeche& f, const Kontext&) { return f.istDetail(); };
}

// BrushFilterWrapper: Maske, Umkehrung, Regel ueber die Flaechen
struct PinselFilter {
    unsigned int maske;
    bool umkehren;
    bool alle;   // filter_brush_all_faces (sonst filter_brush_any_face)
    FlaechenRegel regel;
    [[nodiscard]] bool filtert(const Pinsel& p, const Kontext& k) const {
        bool ergebnis = false;
        if (alle) {
            ergebnis = !p.flaechen().empty();   // don't filter empty brushes
            for (const auto& f : p.flaechen()) {
                if (!regel(*f, k)) {
                    ergebnis = false;
                    break;
                }
            }
        } else {
            for (const auto& f : p.flaechen()) {
                if (regel(*f, k)) {
                    ergebnis = true;
                    break;
                }
            }
        }
        return umkehren != ergebnis;
    }
};

struct FlaechenFilter {
    unsigned int maske;
    FlaechenRegel regel;
};

// BrushFilters_construct (brushmanip.cpp:1154-1188) fuer brushtypes/entities "quake3"
const std::vector<PinselFilter>& pinselFilter() {
    static const std::vector<PinselFilter> f = {
        {EXCLUDE_CLIP, false, true, flaggenRegel(QER_CLIP)},
        {EXCLUDE_CLIP, false, true, shaderRegel("textures/clip")},
        {EXCLUDE_CLIP, false, true, shaderRegel("textures/common/weapclip")},
        {EXCLUDE_CLIP, false, true, shaderRegel("textures/common/fullclip")},
        {EXCLUDE_CLIP, false, true, shaderRegel("textures/common/clip")},
        {EXCLUDE_BOTCLIP, false, true, shaderRegel("textures/common/botclip")},
        {EXCLUDE_BOTCLIP, false, true, shaderRegel("textures/common/donotenter")},
        {EXCLUDE_CAULK, false, true, praefixRegel("textures/common/caulk")},
        {EXCLUDE_CAULK, false, true, praefixRegel("textures/system/caulk")},
        {EXCLUDE_LIQUIDS, false, false, flaggenRegel(QER_LIQUID)},
        {EXCLUDE_LIQUIDS, false, false, praefixRegel("textures/liquids/")},
        {EXCLUDE_HINTSSKIPS, false, false, shaderRegel("textures/common/hint")},
        {EXCLUDE_HINTSSKIPS, false, false, shaderRegel("textures/common/hintlocal")},
        {EXCLUDE_HINTSSKIPS, false, false, shaderRegel("textures/hint")},
        {EXCLUDE_HINTSSKIPS, false, false, shaderRegel("textures/system/hint")},
        {EXCLUDE_CLUSTERPORTALS, false, true, shaderRegel("textures/common/clusterportal")},
        {EXCLUDE_VISPORTALS, false, false, shaderRegel("textures/editor/visportal")},
        {EXCLUDE_AREAPORTALS, false, false, shaderRegel("textures/common/areaportal")},
        {EXCLUDE_TRANSLUCENT, false, false, flaggenRegel(QER_TRANS | QER_ALPHATEST)},
        {EXCLUDE_DETAILS, false, true, detailRegel()},
        {EXCLUDE_STRUCTURAL, true, true, detailRegel()},
        {EXCLUDE_LIGHTGRID, false, true, shaderRegel("textures/common/lightgrid")},
        {EXCLUDE_DECALS, false, false, praefixRegel("textures/decals/")},
        {EXCLUDE_SKY, false, false, flaggenRegel(QER_SKY)},
    };
    return f;
}

const std::vector<FlaechenFilter>& flaechenFilter() {   // add_face_filter
    static const std::vector<FlaechenFilter> f = {
        {EXCLUDE_CAULK, praefixRegel("textures/common/caulk")},
        {EXCLUDE_CAULK, praefixRegel("textures/system/caulk")},
    };
    return f;
}

// PatchFilters_construct (patchmanip.cpp:662-667)
struct PatchFilterEintrag {
    unsigned int maske;
    std::function<bool(const Patch&, const Kontext&)> regel;
};
const std::vector<PatchFilterEintrag>& patchFilter() {
    static const std::vector<PatchFilterEintrag> f = {
        {EXCLUDE_CURVES, [](const Patch&, const Kontext&) { return true; }},   // filter_patch_all
        {EXCLUDE_CLIP, [](const Patch& p, const Kontext& k) { return (shaderFlaggen(p.shader(), k) & QER_CLIP) != 0; }},
        {EXCLUDE_CLIP, [](const Patch& p, const Kontext&) { return gleichOhneFall(p.shader(), "textures/common/clip"); }},
        {EXCLUDE_CLIP, [](const Patch& p, const Kontext&) { return gleichOhneFall(p.shader(), "textures/common/weapclip"); }},
        {EXCLUDE_TRANSLUCENT, [](const Patch& p, const Kontext& k) { return (shaderFlaggen(p.shader(), k) & (QER_TRANS | QER_ALPHATEST)) != 0; }},
    };
    return f;
}

// Entity_InitFilters (entity.cpp:379-393): brushtypes "quake3" -> keine
// func_detail-Filter auf Entitaeten
bool istWelt(const std::string& c) {   // filter_entity_world
    return c == "worldspawn" || c == "func_group" || c.compare(0, 11, "func_detail") == 0;
}
struct EntitaetFilterEintrag {
    unsigned int maske;
    bool umkehren;
    std::function<bool(const Entitaet&)> regel;
};
const std::vector<EntitaetFilterEintrag>& entitaetFilter() {
    static const std::vector<EntitaetFilterEintrag> f = {
        {EXCLUDE_WORLD, false, [](const Entitaet& e) { return istWelt(e.klassenname()); }},
        {EXCLUDE_FUNC_GROUPS, false, [](const Entitaet& e) { return e.klassenname() == "func_group"; }},   // filter_entity_classname
        {EXCLUDE_ENT, true, [](const Entitaet& e) { return istWelt(e.klassenname()); }},
        {EXCLUDE_POINT_ENT, false,
         [](const Entitaet& e) {   // filter_entity_point
             const EntitaetsKlasse* kl = e.klasse();
             return !e.istBehaelter() && !(kl != nullptr && kl->miscmodelIs) && e.klassenname().compare(0, 5, "light") != 0;
         }},
        {EXCLUDE_TRIGGERS, false, [](const Entitaet& e) { return e.klassenname().compare(0, 8, "trigger_") == 0; }},   // filter_entity_classgroup
        {EXCLUDE_MODELS, false,
         [](const Entitaet& e) {   // filter_entity_misc_model
             const EntitaetsKlasse* kl = e.klasse();
             return kl != nullptr && kl->miscmodelIs;
         }},
        {EXCLUDE_MODELS, false,
         [](const Entitaet& e) {   // filter_entity_doom3model
             return e.klassenname() == "func_static" && e.wert("model") != e.wert("name");
         }},
        {EXCLUDE_LIGHTS, false, [](const Entitaet& e) { return e.klassenname() == "light"; }},
        {EXCLUDE_PATHS, false, [](const Entitaet& e) { return e.klassenname().compare(0, 5, "path_") == 0; }},
    };
    return f;
}

}  // namespace

const std::vector<FilterBefehl>& filterBefehle() {   // ConstructFilters (filters.cpp:209-243)
    static const std::vector<FilterBefehl> b = {
        {"FilterWorldBrushes", EXCLUDE_WORLD, "Alt+1"},
        {"FilterEntities", EXCLUDE_ENT, "Alt+2"},
        {"FilterAreaportals", EXCLUDE_AREAPORTALS, "Alt+3"},
        {"FilterTranslucent", EXCLUDE_TRANSLUCENT, "Alt+4"},
        {"FilterLiquids", EXCLUDE_LIQUIDS, "Alt+5"},
        {"FilterCaulk", EXCLUDE_CAULK, "Alt+6"},
        {"FilterClips", EXCLUDE_CLIP, "Alt+7"},
        {"FilterPaths", EXCLUDE_PATHS, "Alt+8"},
        {"FilterClusterportals", EXCLUDE_CLUSTERPORTALS, "Alt+9"},
        {"FilterLights", EXCLUDE_LIGHTS, "Alt+0"},
        {"FilterStructural", EXCLUDE_STRUCTURAL, "Ctrl+Shift+D"},
        {"FilterLightgrid", EXCLUDE_LIGHTGRID, ""},
        {"FilterPatches", EXCLUDE_CURVES, "Ctrl+P"},
        {"FilterDetails", EXCLUDE_DETAILS, "Ctrl+D"},
        {"FilterHintsSkips", EXCLUDE_HINTSSKIPS, "Ctrl+H"},
        {"FilterModels", EXCLUDE_MODELS, "Shift+M"},
        {"FilterTriggers", EXCLUDE_TRIGGERS, "Ctrl+Shift+T"},
        {"FilterBotClips", EXCLUDE_BOTCLIP, "Alt+M"},
        {"FilterDecals", EXCLUDE_DECALS, "Shift+D"},
        {"FilterFuncGroups", EXCLUDE_FUNC_GROUPS, ""},
        {"FilterPointEntities", EXCLUDE_POINT_ENT, ""},
        {"FilterSky", EXCLUDE_SKY, ""},
    };
    return b;
}

const FilterBefehl* filterBefehl(const std::string& name) {
    for (const FilterBefehl& b : filterBefehle()) {
        if (name == b.name) { return &b; }
    }
    return nullptr;
}

unsigned int filterUmkehren(unsigned int maske) {   // InvertFilters: toggle() je Eintrag
    for (const FilterBefehl& b : filterBefehle()) { maske ^= b.maske; }
    return maske;
}

bool flaecheGefiltert(const Flaeche& f, unsigned int maske, const Kontext& k) {   // face_filtered
    return std::any_of(flaechenFilter().begin(), flaechenFilter().end(),
                       [&](const FlaechenFilter& x) { return (maske & x.maske) != 0 && x.regel(f, k); });
}

bool pinselGefiltert(const Pinsel& p, unsigned int maske, const Kontext& k) {   // brush_filtered
    return std::any_of(pinselFilter().begin(), pinselFilter().end(), [&](const PinselFilter& x) { return (maske & x.maske) != 0 && x.filtert(p, k); });
}

bool patchGefiltert(const Patch& p, unsigned int maske, const Kontext& k) {   // patch_filtered
    return std::any_of(patchFilter().begin(), patchFilter().end(),
                       [&](const PatchFilterEintrag& x) { return (maske & x.maske) != 0 && x.regel(p, k); });
}

bool entitaetGefiltert(const Entitaet& e, unsigned int maske) {   // entity_filtered
    return std::any_of(entitaetFilter().begin(), entitaetFilter().end(),
                       [&](const EntitaetFilterEintrag& x) { return (maske & x.maske) != 0 && (x.umkehren != x.regel(e)); });
}

void filterAnwenden(Karte& k, unsigned int maske) {   // UpdateFilters
    for (const auto& ent : k.entitaeten()) {
        ent->gefiltert = entitaetGefiltert(*ent, maske);
        for (const auto& p : ent->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) {
                auto& pinsel = static_cast<Pinsel&>(*p);
                p->gefiltert = pinselGefiltert(pinsel, maske, k.kontext);
                for (const auto& f : pinsel.flaechen()) { f->gefiltert = flaecheGefiltert(*f, maske, k.kontext); }
            } else {
                p->gefiltert = patchGefiltert(static_cast<Patch&>(*p), maske, k.kontext);
            }
        }
    }
}

}  // namespace bhed::karte
