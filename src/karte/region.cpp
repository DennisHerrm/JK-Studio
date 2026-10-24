// region.cpp - Karten-Editor: die Region und die Auswahl nach Kaesten.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in region.h).
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
#include "bhed/karte/region.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <unordered_set>

namespace bhed::karte {

namespace {

// Ein leerer Verweis (nie gesetzt) - ein geloeschtes Primitiv ist abgelaufen, aber gesetzt.
bool leer(const std::weak_ptr<Primitiv>& w) {
    return !w.owner_before(std::weak_ptr<Primitiv>{}) && !std::weak_ptr<Primitiv>{}.owner_before(w);
}

// Die gewaehlten Instanzen (Instance_isSelected / childSelected / parentSelected).
struct Gewaehlte {
    std::unordered_set<const Entitaet*> ganz;
    std::unordered_set<const Primitiv*> prim;
    explicit Gewaehlte(const Karte& k) {
        for (const auto& el : k.auswahl.elemente()) {
            const auto e = el.entitaet.lock();
            const auto p = el.primitiv.lock();
            if (!e) { continue; }
            if (p) {
                prim.insert(p.get());
            } else if (leer(el.primitiv)) {
                ganz.insert(e.get());   // ein geloeschtes Primitiv ist nicht die ganze Entitaet
            }
        }
    }
    [[nodiscard]] bool kindGewaehlt(const Entitaet& e) const {
        return std::any_of(e.kinder().begin(), e.kinder().end(), [&](const auto& p) { return prim.count(p.get()) != 0; });
    }
};

// Die Instanz, die SelectionSystem::ultimateSelected liefert.
struct Instanz {
    std::shared_ptr<Entitaet> ent;
    std::shared_ptr<Primitiv> prim;   // leer: die Entitaet selbst
};

bool letzteGewaehlte(const Karte& k, Instanz& i) {
    const auto& el = k.auswahl.elemente();
    for (auto it = el.rbegin(); it != el.rend(); ++it) {
        i.ent = it->entitaet.lock();
        i.prim = it->primitiv.lock();
        if (i.ent && (i.prim || leer(it->primitiv))) { return true; }
    }
    return false;
}

Kasten grenzenVon(const Instanz& i, const Kontext& k) { return i.prim ? instanzGrenzen(*i.prim, k) : instanzGrenzen(*i.ent, k); }

// SelectByBounds<TSelectionPolicy>::DoSelection
void nachKaestenWaehlen(Karte& k, const AuswahlEinstellung& e, bool loeschen, const std::function<bool(const Kasten&, const Kasten&)>& regel) {
    if (e.modus != AuswahlModus::Primitiv) { return; }
    // CollectSelectedBrushesBounds: nur Pinsel, in Auswahlreihenfolge
    std::vector<Kasten> kaesten;
    for (const auto& el : k.auswahl.elemente()) {
        const auto ent = el.entitaet.lock();
        const auto p = el.primitiv.lock();
        if (ent && p && p->art() == Primitiv::Art::Pinsel) { kaesten.push_back(instanzGrenzen(*p, k.kontext)); }
    }
    if (kaesten.empty()) { return; }   // nothing usable in selection
    if (loeschen) {   // see deleteSelection
        k.undo.start();
        auswahlEntfernen(k);   // Select_Delete
        k.undo.finish("deleteSelected");
    }
    const auto passt = [&](const Kasten& b) {
        return std::any_of(kaesten.begin(), kaesten.end(), [&](const Kasten& a) { return regel(a, b); });
    };
    for (const auto& ent : k.entitaeten()) {
        if (!ent->sichtbar()) { continue; }   // nicht sichtbar: auch die Kinder nicht
        if (ent->klassenname() != "worldspawn" && !ent->istBehaelter()) {   // ignore worldspawn, !node_is_group
            if (passt(instanzGrenzen(*ent, k.kontext))) { k.auswahl.waehlen(ent); }
        }
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar()) { continue; }
            if (passt(instanzGrenzen(*p, k.kontext))) { k.auswahl.waehlen(ent, p); }
        }
    }
}

// exclude_node
template<typename T>
void ausschliessen(T& knoten, bool aus) { knoten.ausgeschlossen = aus; }

}  // namespace

bool kastenSchneidet(const Kasten& a, const Kasten& b) {   // aabb_intersects_aabb
    for (std::size_t i = 0; i < 3; ++i) {
        if (!(std::fabs(b.origin[i] - a.origin[i]) < (a.extents[i] + b.extents[i]))) { return false; }
    }
    return true;
}

Kasten instanzGrenzen(const Primitiv& p, const Kontext& k) {
    if (p.art() == Primitiv::Art::Pinsel) { return static_cast<const Pinsel&>(p).grenzen(k); }
    return static_cast<const Patch&>(p).grenzen();
}

Kasten instanzGrenzen(const Entitaet& e, const Kontext& k) {
    Kasten b;   // aabb_for_oriented_aabb_safe( localAABB, localToWorld )
    if (!e.istBehaelter()) {
        const Kasten l = entitaetKastenLokal(e);
        b = Kasten(l.origin + entitaetUrsprung(e), l.extents);
    }
    for (const auto& p : e.kinder()) { kastenErweiternSicher(b, instanzGrenzen(*p, k)); }   // AABBExtendByChildren
    return b;
}

// --- select.cpp ------------------------------------------------------------------------

void auswahlInnen(Karte& k, const AuswahlEinstellung& e) {   // Select_Inside
    nachKaestenWaehlen(k, e, true, [](const Kasten& box, const Kasten& other) {   // SelectionPolicy_Inside
        for (std::size_t i = 0; i < 3; ++i) {
            if (std::fabs(box.origin[i] - other.origin[i]) > (box.extents[i] - other.extents[i])) { return false; }
        }
        return true;
    });
}

void auswahlBeruehrend(Karte& k, const AuswahlEinstellung& e) {   // Select_Touching
    nachKaestenWaehlen(k, e, false, [](const Kasten& box, const Kasten& other) {   // SelectionPolicy_Touching
        for (std::size_t i = 0; i < 3; ++i) {
            if (std::fabs(box.origin[i] - other.origin[i]) > (box.extents[i] + other.extents[i])) { return false; }
        }
        return true;
    });
}

void auswahlAufPrimitiveErweitern(Karte& k) {   // ExpandSelectionToPrimitivesWalker
    const Gewaehlte g(k);
    for (const auto& ent : k.entitaeten()) {
        if (!ent->sichtbar()) { continue; }
        const bool behaelter = ent->istBehaelter();
        if (!g.kindGewaehlt(*ent) && g.ganz.count(ent.get()) == 0) { continue; }
        if (behaelter) {   // Instance_setSelected( instance, !isContainer )
            k.auswahl.abwaehlen(ent);
        } else {
            k.auswahl.waehlen(ent);
            continue;
        }
        for (const auto& p : ent->kinder()) {   // primitive depth
            if (p->sichtbar()) { k.auswahl.waehlen(ent, p); }
        }
    }
}

// --- map.cpp: Region ---------------------------------------------------------------------

void regionStandard(Region& r, float maxRaster) {   // Region_defaultMinMax
    if (!r.aktiv) {   // don't invalidate region bounds, while in region mode
        r.max = Vektor3f(maxRaster, maxRaster, maxRaster);
        r.min = -r.max;
    }
}

void regionAus(Karte& k, Region& r, float maxRaster) {   // Map_RegionOff
    r.aktiv = false;
    regionStandard(r, maxRaster);
    for (const auto& ent : k.entitaeten()) {   // Scene_Exclude_All( false )
        ausschliessen(*ent, false);
        for (const auto& p : ent->kinder()) { ausschliessen(*p, false); }
    }
}

void regionAnwenden(Karte& k, Region& r) {   // Map_ApplyRegion
    if (komponentenAnzahl(k) != 0) { alleKomponentenAbwaehlen(k); }
    r.aktiv = true;
    const Kasten region = kastenAusMinMax(r.min, r.max);
    for (const auto& ent : k.entitaeten()) {   // ExcludeRegionedWalker( false )
        const bool aus = !kastenSchneidet(instanzGrenzen(*ent, k.kontext), region);
        ausschliessen(*ent, aus);
        if (aus) { k.auswahl.abwaehlen(ent); }
        for (const auto& p : ent->kinder()) {
            const bool paus = !kastenSchneidet(instanzGrenzen(*p, k.kontext), region);
            ausschliessen(*p, paus);
            if (paus) { k.auswahl.abwaehlen(ent, p); }
        }
    }
    // not hiding worldspawn node so that newly created brushes are visible
    if (const auto w = k.welt()) { ausschliessen(*w, false); }
}

void regionAuswahl(Karte& k, Region& r, float maxRaster) {   // Map_RegionSelectedBrushes
    if (k.auswahl.anzahl() == 0) {
        regionAus(k, r, maxRaster);
        return;
    }
    if (komponentenAnzahl(k) != 0) { alleKomponentenAbwaehlen(k); }
    r.aktiv = true;
    const Kasten b = auswahlGrenzenPivot(k);   // Select_GetBounds
    r.max = b.origin + b.extents;
    r.min = b.origin - b.extents;
    const Gewaehlte g(k);
    for (const auto& ent : k.entitaeten()) {   // ExcludeSelectedWalker( false )
        const bool entGewaehlt = g.ganz.count(ent.get()) != 0;
        ausschliessen(*ent, !(entGewaehlt || g.kindGewaehlt(*ent)));
        for (const auto& p : ent->kinder()) { ausschliessen(*p, !(g.prim.count(p.get()) != 0 || entGewaehlt)); }
    }
    if (const auto w = k.welt()) { ausschliessen(*w, false); }
    k.auswahl.leeren();   // setSelectedAll( false )
}

void regionXY(Karte& k, Region& r, const Vektor3f& min, const Vektor3f& max) {   // Map_RegionXY
    for (std::size_t i = 0; i < 3; ++i) {
        r.min[i] = std::max(r.min[i], min[i]);
        r.max[i] = std::min(r.max[i], max[i]);
    }
    regionAnwenden(k, r);
}

void regionGrenzen(Karte& k, Region& r, const Kasten& b, float maxRaster) {   // Map_RegionBounds
    regionAus(k, r, maxRaster);
    r.min = b.origin - b.extents;
    r.max = b.origin + b.extents;
    regionAnwenden(k, r);
}

bool regionPinsel(Karte& k, Region& r, float maxRaster, const AuswahlEinstellung& e) {   // Map_RegionBrush
    Instanz i;
    if (k.auswahl.anzahl() == 0 || !letzteGewaehlte(k, i)) { return false; }   // "Nothing is selected!"
    regionGrenzen(k, r, grenzenVon(i, k.kontext), maxRaster);
    if (k.auswahl.anzahl() != 1) {
        k.auswahl.leeren();
        k.auswahl.waehlen(i.ent, i.prim);
    }
    loescheAuswahl(k, e);   // deleteSelection
    return true;
}

std::string regionKastenShader(const std::string& einstellung, std::string* warnung) {   // ConstructRegionBrushes
    if (einstellung.empty()) { return "textures/"; }   // texdef_name_default
    if (texturNameGueltig(einstellung)) { return einstellung; }
    if (warnung != nullptr) { *warnung = "g_regionBoxShader \"" + einstellung + "\" !texdef_name_valid()"; }
    return "textures/";
}

void regionPinselBauen(const std::array<Pinsel*, 6>& pinsel, const Vektor3f& min, const Vektor3f& max, const std::string& shader,
                       const Kontext& k) {   // ConstructRegionBrushes
    const TexturProjektion projektion{};   // TextureProjection()
    {
        const Vektor3f mins = min - Vektor3f(32, 32, 32);   // set mins
        for (std::size_t i = 0; i < 3; ++i) {   // vary maxs
            Vektor3f maxs = max + Vektor3f(32, 32, 32);
            maxs[i] = min[i];
            pinsel[i]->quaderBauen(kastenAusMinMax(mins, maxs), shader, projektion, k);
        }
    }
    {
        const Vektor3f maxs = max + Vektor3f(32, 32, 32);   // set maxs
        for (std::size_t i = 0; i < 3; ++i) {   // vary mins
            Vektor3f mins = min - Vektor3f(32, 32, 32);
            mins[i] = max[i];
            pinsel[i + 3]->quaderBauen(kastenAusMinMax(mins, maxs), shader, projektion, k);
        }
    }
}

std::string regionText(Karte& k, const Region& r, const Vektor3f& kameraUrsprung, float kameraGier, const std::string& kastenShader,
                       std::vector<std::string>* meldungen) {   // Map_SaveRegion
    // ScopeRegionBrushes: die Waende kommen ans Ende von worldspawn, der
    // Startpunkt ans Ende der Karte - ohne Rueckgaengig, danach wieder weg.
    const auto welt = k.weltHolenOderAnlegen();   // Map_FindOrInsertWorldspawn
    std::array<std::shared_ptr<Pinsel>, 6> waende;
    std::array<Pinsel*, 6> roh{};
    for (std::size_t i = 0; i < 6; ++i) {
        waende[i] = std::make_shared<Pinsel>();
        waende[i]->schicht = static_cast<int>(k.aktuelleSchicht);
        roh[i] = waende[i].get();
        welt->kindEinfuegen(waende[i]);
    }
    const auto start = k.neueEntitaet("info_player_start", false);
    // adjust temp box: space may be too small, also help with lights and flat primitives
    const Vektor3f mn = r.min - Vektor3f(256, 256, 8);
    const Vektor3f mx = r.max + Vektor3f(256, 256, 512);
    Vektor3f spawn = kameraUrsprung;
    for (std::size_t i = 0; i < 3; ++i) {   // pull spawn point to the box, if needed
        spawn[i] = std::max(spawn[i], mn[i] + 64);
        spawn[i] = std::min(spawn[i], mx[i] - 64);
    }
    regionPinselBauen(roh, mn, mx, kastenShader, k.kontext);
    {   // ConstructRegionStartpoint
        char z[1024];
        std::snprintf(z, sizeof(z), "%d %d %d", static_cast<int>(spawn[0]), static_cast<int>(spawn[1]), static_cast<int>(spawn[2]));
        start->setzen("origin", z);
        std::snprintf(z, sizeof(z), "%d", static_cast<int>(kameraGier));
        start->setzen("angle", z);
    }
    k.entitaetEinfuegen(start);
    // Map_Traverse_Region: ExcludeWalker mit RegionExcluder (nur eExcluded)
    std::string text = karteSchreiben(
        k, [](const Entitaet& e) { return !e.ausgeschlossen; }, [](const Entitaet&, const Primitiv& p) { return !p.ausgeschlossen; }, meldungen);
    for (const auto& w : waende) { welt->kindEntfernen(w.get()); }   // ~ScopeRegionBrushes
    k.entitaetEntfernen(start.get());
    return text;
}

}  // namespace bhed::karte
