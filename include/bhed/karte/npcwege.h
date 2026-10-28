// npcwege.h - Karten-Editor, Stufe 9 Punkt 5: NPC-Wege (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Portiert aus OpenJK / Movie Duels / Jedi Outcast ueber die vorhandenen 1:1-Ports: das Wegnetz
// baut baueNavNetz (SP_waypoint ..., NAV::LoadFromEntitiesAndSaveToFile, g_navigator.cpp), den
// Lauf rechnet planeLauf (NPC_MoveToGoal, NAV::GoTo, STEER, Pmove; NPC.cpp, g_navigator.cpp,
// bg_pmove.cpp). Hier nur die Bruecke von den Entities der offenen Karte und ihrer Kollision
// (kollisionAusKarte) dorthin.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#ifndef BHED_KARTE_NPCWEGE_H
#define BHED_KARTE_NPCWEGE_H

#include "bhed/bsp.h"
#include "bhed/kollision.h"
#include "bhed/navlauf.h"

#include <memory>
#include <vector>

namespace bhed::karte {

class Karte;

// Die Entities wie im Entity-Lump einer .bsp (worldspawn zuerst, alle Schluessel).
[[nodiscard]] std::vector<MapEntity> mapEntitaeten(const Karte& k);

// Die Welt fuer die Wegfindung: Netz aus den Wegpunkten, Spuren und Boden aus der Kollision.
[[nodiscard]] std::shared_ptr<NavWelt> npcWelt(const Karte& k, std::shared_ptr<const KollisionsKarte> koll);

// Ein Testlauf: eine Figur (Kasten 16 x 40, Stufe 18, Schwerkraft 800) laeuft von a zum Ziel b
// (SET_NAVGOAL), rennend (runSpeed 300) oder gehend (walkSpeed 90), bis sie ankommt, stehen
// bleibt oder grenzeMs vergeht.
[[nodiscard]] LaufPlan npcTestlauf(const NavWelt& welt, const NavVec& a, const NavVec& b, bool rennen, double grenzeMs = 60000.0);

}  // namespace bhed::karte

#endif
