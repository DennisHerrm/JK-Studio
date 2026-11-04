// texturliste.cpp - Karten-Editor (rc609 K): die Texturen der Karte (siehe texturliste.h).
//
// Portiert aus NetRadiant-Custom 20260114 (radiant/brushmanip.cpp:
// FaceSelectByShader, Scene_BrushFindReplaceShader; radiant/patchmanip.cpp:
// Scene_PatchSelectByShader, Scene_PatchFindReplaceShader; radiant/brush.h:
// Face::SetShader; libs/shaderlib.h: shader_less_t, texdef_name_valid) und aus
// q3map2 (map.cpp: QuakeTextureVecs, ParseRawBrush; surface.cpp).
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
#include "bhed/karte/texturliste.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturbrowser.h"

#include <map>
#include <memory>
#include <unordered_set>

namespace bhed::karte {

namespace {

// Was im Umfang "nur Auswahl" steht: Primitive, Entitaeten als Ganzes, einzelne Flaechen.
struct Gewaehlt {
    std::unordered_set<const Entitaet*> ganz;
    std::unordered_set<const Primitiv*> prim;
    std::unordered_set<const Flaeche*> flaechen;
    explicit Gewaehlt(const Karte& k) {
        for (const auto& el : k.auswahl.elemente()) {
            const auto e = el.entitaet.lock();
            if (!e) { continue; }
            if (const auto p = el.primitiv.lock()) {
                prim.insert(p.get());
            } else if (!el.primitiv.owner_before(std::weak_ptr<Primitiv>{}) && !std::weak_ptr<Primitiv>{}.owner_before(el.primitiv)) {
                ganz.insert(e.get());   // ohne Primitiv: die ganze Entitaet (ein geloeschtes Primitiv zaehlt nicht)
            }
        }
        for (const auto& fw : k.auswahl.flaechen()) {
            if (const auto f = fw.flaeche.lock()) { flaechen.insert(f.get()); }
        }
    }
};

// Jede Brush-Seite und jeder Patch im Umfang: seite(ent, pinsel, flaeche), patch(ent, patch).
template <typename S, typename P>
void imUmfang(const Karte& k, TexturUmfang u, S seite, P patch) {
    const std::unique_ptr<Gewaehlt> g = u == TexturUmfang::Auswahl ? std::make_unique<Gewaehlt>(k) : nullptr;
    for (const auto& ent : k.entitaeten()) {
        if (u == TexturUmfang::Sichtbar && !ent->sichtbar()) { continue; }
        const bool entGanz = g && g->ganz.count(ent.get()) != 0;
        for (const auto& p : ent->kinder()) {
            if (u == TexturUmfang::Sichtbar && !p->sichtbar()) { continue; }
            const bool primGanz = entGanz || (g && g->prim.count(p.get()) != 0);
            if (p->art() == Primitiv::Art::Pinsel) {
                const auto pinsel = std::static_pointer_cast<Pinsel>(p);
                for (const auto& f : pinsel->flaechen()) {
                    if (u == TexturUmfang::Sichtbar && f->gefiltert) { continue; }
                    if (g && !primGanz && g->flaechen.count(f.get()) == 0) { continue; }
                    seite(ent, pinsel, f);
                }
            } else if (!g || primGanz) {
                patch(ent, p);
            }
        }
    }
}

bool gleich(const std::string& a, const std::string& b) { return shaderNamenGleich(a, b); }

// Face::SetShader und, mit weltGroesse, die Projektion so, dass eine Wiederholung
// dieselbe Weltflaeche deckt (texturliste.h).
void seiteErsetzen(Flaeche& f, const std::string& neu, bool weltGroesse, const Kontext& k) {
    TexturProjektion p = f.projektion();
    const auto [wa, ha] = k.groesse(f.shader());
    f.setzeShader(neu, k);
    if (!weltGroesse) { return; }
    const auto [wn, hn] = k.groesse(neu);
    if (wa == wn && ha == hn) { return; }
    if (k.art() != TexdefArt::BrushPrimitives) {   // Quake und Valve 220: Welt je Wiederholung = Breite * scale
        const float sx = static_cast<float>(wa) / static_cast<float>(wn);
        const float sy = static_cast<float>(ha) / static_cast<float>(hn);
        p.texdef.scale[0] *= sx;
        p.texdef.scale[1] *= sy;
        p.texdef.shift[0] /= sx;   // Verschiebung in Bildpunkten: derselbe Bruchteil der Wiederholung
        p.texdef.shift[1] /= sy;
    }
    // Brush Primitives: die Werte vor setzeShader sind die der .map (normiert) - sie bleiben.
    f.projektionSetzen(p);
}

}  // namespace

std::vector<TexturZeile> texturListe(const Karte& k, TexturUmfang u) {
    std::map<std::string, TexturZeile, ShaderKleiner> m;
    std::map<std::string, std::unordered_set<const Pinsel*>, ShaderKleiner> pinselJe;   // je Shader: Brushes nur einmal
    imUmfang(
        k, u,
        [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Pinsel>& p, const std::shared_ptr<Flaeche>& f) {
            auto [it, neu] = m.try_emplace(f->shader());
            if (neu) { it->second.shader = f->shader(); }
            ++it->second.seiten;
            if (pinselJe[f->shader()].insert(p.get()).second) { ++it->second.pinsel; }
        },
        [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Primitiv>& p) {
            const std::string& s = static_cast<const Patch&>(*p).shader();
            auto [it, neu] = m.try_emplace(s);
            if (neu) { it->second.shader = s; }
            ++it->second.patches;
        });
    std::vector<TexturZeile> v;
    v.reserve(m.size());
    for (auto& [name, z] : m) { v.push_back(std::move(z)); }
    return v;
}

std::pair<int, int> texturListeWaehlen(Karte& k, const std::string& shader, TexturUmfang u, bool dazu) {
    std::vector<std::pair<std::shared_ptr<Pinsel>, std::shared_ptr<Flaeche>>> seiten;
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>> patches;
    imUmfang(
        k, u,
        [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Pinsel>& p, const std::shared_ptr<Flaeche>& f) {
            if (gleich(f->shader(), shader)) { seiten.emplace_back(p, f); }
        },
        [&](const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p) {
            if (gleich(static_cast<const Patch&>(*p).shader(), shader)) { patches.emplace_back(e, p); }
        });
    if (!dazu) { k.auswahl.leeren(); }
    for (const auto& [p, f] : seiten) { k.auswahl.flaecheWaehlen(p, f); }   // FaceSelectByShader
    for (const auto& [e, p] : patches) { k.auswahl.waehlen(e, p); }         // PatchSelectByShader
    return {static_cast<int>(seiten.size()), static_cast<int>(patches.size())};
}

ErsetzenZahl texturListeErsetzen(Karte& k, const std::vector<TexturErsatz>& paare, TexturUmfang u, bool weltGroesse) {
    std::map<std::string, std::string, ShaderKleiner> ziel;
    for (const TexturErsatz& p : paare) {
        if (texturNameGueltig(p.alt) && texturNameGueltig(p.neu) && !gleich(p.alt, p.neu)) { ziel[p.alt] = p.neu; }
    }
    std::vector<std::pair<std::shared_ptr<Flaeche>, const std::string*>> seiten;
    std::vector<std::pair<std::shared_ptr<Primitiv>, const std::string*>> patches;
    imUmfang(
        k, u,
        [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Pinsel>&, const std::shared_ptr<Flaeche>& f) {
            if (const auto it = ziel.find(f->shader()); it != ziel.end()) { seiten.emplace_back(f, &it->second); }
        },
        [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Primitiv>& p) {
            if (const auto it = ziel.find(static_cast<const Patch&>(*p).shader()); it != ziel.end()) { patches.emplace_back(p, &it->second); }
        });
    ErsetzenZahl n;
    if (seiten.empty() && patches.empty()) { return n; }
    k.undo.start();
    for (const auto& [f, neu] : seiten) { seiteErsetzen(*f, *neu, weltGroesse, k.kontext); }
    for (const auto& [p, neu] : patches) { static_cast<Patch&>(*p).setzeShader(*neu); }
    std::string name = "textureListReplace";
    for (const auto& [alt, neu] : ziel) { name += " " + alt + " -> " + neu; }
    k.undo.finish(name);
    n.seiten = static_cast<int>(seiten.size());
    n.patches = static_cast<int>(patches.size());
    return n;
}

std::string texturOrdnerName(const std::string& ordner) {
    std::string o = ordner;
    for (char& c : o) {
        if (c == '\\') { c = '/'; }
    }
    while (!o.empty() && o.front() == '/') { o.erase(0, 1); }
    if (o.empty()) { return o; }
    if (o.back() != '/') { o += '/'; }
    const std::string p = kTexturPraefix;
    if (o.size() < p.size() || !gleich(o.substr(0, p.size()), p)) { o = p + o; }
    return o;
}

std::vector<OrdnerZuordnung> texturOrdnerZuordnung(const std::vector<TexturZeile>& liste, const std::string& altOrdner,
                                                   const std::string& neuOrdner,
                                                   const std::function<bool(const std::string&)>& vorhanden) {
    std::vector<OrdnerZuordnung> v;
    const std::string alt = texturOrdnerName(altOrdner);
    const std::string neu = texturOrdnerName(neuOrdner);
    if (alt.empty() || neu.empty() || gleich(alt, neu)) { return v; }
    for (const TexturZeile& z : liste) {
        if (z.shader.size() <= alt.size() || !gleich(z.shader.substr(0, alt.size()), alt)) { continue; }
        OrdnerZuordnung o;
        o.alt = z.shader;
        o.neu = neu + z.shader.substr(alt.size());
        o.fehlt = vorhanden && !vorhanden(o.neu);
        v.push_back(std::move(o));
    }
    return v;
}

}  // namespace bhed::karte
