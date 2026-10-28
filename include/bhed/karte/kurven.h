// kurven.h - Karten-Editor, Stufe 9 Punkt 7: Kurven-Werkzeug (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Portiert aus NetRadiant-Custom 20260114 sind die Bausteine: Patch (patch.cpp setDims,
// controlPointsChanged), der Kreis eines Zylinder-Patches aus 9 Kontrollpunkten
// (Patch::ConstructPrefab, eCylinder: Ecken des Quadrats als Kontrollpunkte); die Kurve
// selbst ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Die Kontrollpunkte (z. B. gewaehlte Punkt-Entities in der Reihenfolge der Wahl) ergeben eine
// Catmull-Rom-Kurve, abgetastet etwa alle "schritt" Einheiten (ungerade Zahl Punkte, wie die
// Zeilen eines Patches). Daraus: ein Weg (flaches Band), ein Rohr (Zylinder-Patch), ein
// Gelaender (zwei duenne Rohre links und rechts in Haltehoehe), eine Bruecke (Deck mit
// Seiten, Unterseite), oder eine Kette path_corner (func_train, Kamerafahrt) bzw. waypoint
// (NPC-Weg) mit targetname/target.
#ifndef BHED_KARTE_KURVEN_H
#define BHED_KARTE_KURVEN_H

#include "bhed/karte/mathe.h"

#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {

class Patch;
class Karte;

struct KurvenOptionen {
    float schritt = 64.0F;
    float breite = 128.0F;       // Weg, Bruecke, Abstand der Gelaender
    float radius = 16.0F;        // Rohr
    float rohrHoehe = 16.0F;     // Mitte des Rohrs ueber der Kurve
    float gelaenderHoehe = 40.0F;
    float gelaenderRadius = 2.0F;
    float dicke = 16.0F;         // Bruecke
    std::string shader = "textures/system/caulk";
};

enum class KurvenArt { Weg, Rohr, Gelaender, Bruecke, PfadEcken, Wegpunkte };

[[nodiscard]] std::vector<Vektor3f> kurvePunkte(const std::vector<Vektor3f>& kontroll, float schritt);
[[nodiscard]] std::vector<std::shared_ptr<Patch>> kurvenPatches(const std::vector<Vektor3f>& kontroll, KurvenArt art, const KurvenOptionen& o);

// Einfuegen (ein Befehl "curveTool"): Patches in den worldspawn bzw. die Entity-Kette (Namen
// praefix1, praefix2 ...); alles gewaehlt. Gibt die Zahl der neuen Objekte zurueck.
std::size_t kurveEinfuegen(Karte& k, const std::vector<Vektor3f>& kontroll, KurvenArt art, const KurvenOptionen& o, const std::string& praefix);

}  // namespace bhed::karte

#endif
