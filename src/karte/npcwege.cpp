// npcwege.cpp - Karten-Editor, Stufe 9 Punkt 5: NPC-Wege (siehe npcwege.h).
// Portiert aus OpenJK / Movie Duels / Jedi Outcast ueber baueNavNetz und planeLauf.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/npcwege.h"

#include "bhed/karte/karte.h"

#include <atomic>
#include <cstdlib>

namespace bhed::karte {

std::vector<MapEntity> mapEntitaeten(const Karte& k) {
    std::vector<MapEntity> v;
    for (const auto& e : k.entitaeten()) {
        MapEntity m;
        m.classname = e->wert("classname");
        m.targetname = e->wert("targetname");
        m.origin = e->wert("origin");
        m.keys = e->schluessel();
        if (m.classname == "worldspawn") {
            v.insert(v.begin(), std::move(m));
        } else {
            v.push_back(std::move(m));
        }
    }
    return v;
}

std::shared_ptr<NavWelt> npcWelt(const Karte& k, std::shared_ptr<const KollisionsKarte> koll) {
    auto welt = std::make_shared<NavWelt>();
    const NavSpurFn spur = [koll](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t maske) {
        const KollisionsSpur s = kollisionsSpur(*koll, a.data(), mi.data(), ma.data(), b.data(), maske, 0);
        NavSpur r;
        r.allsolid = s.allsolid;
        r.startsolid = s.startsolid;
        r.fraction = s.fraction;
        r.endpos = s.endpos;
        r.normal = s.normale;
        r.contents = s.inhalt;
        r.entity = s.fraction < 1.0F ? kNavWeltEntity : kNavKeineEntity;
        return r;
    };
    welt->spur = spur;
    welt->boden = [spur](float x, float y, float zOben, float zUnten, float& bodenZ) {
        const NavSpur s = spur(NavVec{x, y, zOben}, NavVec{0, 0, 0}, NavVec{0, 0, 0}, NavVec{x, y, zUnten}, kNavKeineEntity, kNavMaskPlayerSolid);
        if (s.fraction >= 1.0F || s.startsolid) { return false; }
        bodenZ = s.endpos[2];
        return true;
    };
    NavBauHilfe bau;
    bau.spur = spur;
    const std::vector<MapEntity> ents = mapEntitaeten(k);
    welt->netz = baueNavNetz(ents, bau);
    static std::atomic<std::uint64_t> zaehler{0x5e9000};
    welt->kennung = ++zaehler;   // jede Welt neu: die Plaene (Zwischenspeicher) nicht teilen
    return welt;
}

LaufPlan npcTestlauf(const NavWelt& welt, const NavVec& a, const NavVec& b, bool rennen, double grenzeMs) {
    LaufWerte w;
    w.zielGesetzt = true;
    w.ziel = b;
    w.maxTempo = rennen ? 300.0F : 90.0F;   // NPC_stats.cpp: runSpeed 300, walkSpeed 90 (Vorgaben)
    w.gehTempo = 90.0F;
    w.gangSchalter = rennen ? 2 : 1;
    w.fallen = true;
    w.schwerkraft = 800.0F;
    w.weltSchwerkraft = 800.0F;
    w.verhaltenTaktMs = navMovieDuelsRegeln() ? 50 : 100;
    LaufZustand z;
    z.ort = a;
    return planeLauf(welt, z, w, 0.0, grenzeMs);
}

}  // namespace bhed::karte
