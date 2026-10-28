// gelaende.h - Karten-Editor, Stufe 9 Punkt 9: Gelaende formen (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Portiert aus NetRadiant-Custom 20260114 sind die Bausteine (Patch, controlPointsChanged, das
// Undo je Patch); das Formen selbst ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Ein Pinselstrich auf Terrain-Patches: jeder Kontrollpunkt im Radius (waagerecht gemessen)
// bewegt sich mit dem Gewicht (1 - d/r)^2 - heben, senken, glaetten (zum Mittel der Nachbarn)
// oder ebnen (zur Hoehe der Mitte). Die Patches werden vorher gesichert (ein Befehl je Strich).
#ifndef BHED_KARTE_GELAENDE_H
#define BHED_KARTE_GELAENDE_H

#include "bhed/karte/mathe.h"

#include <cstddef>

namespace bhed::karte {

class Patch;
class Karte;

enum class FormArt { Heben, Senken, Glaetten, Ebnen };

struct FormPinsel {
    float radius = 128.0F;
    float staerke = 8.0F;   // Einheiten je Anwendung bei Gewicht 1 (Heben/Senken), Anteil 0..1 sonst / 8
    FormArt art = FormArt::Heben;
};

// Wendet den Pinsel an der Stelle "mitte" auf einen Patch an; gibt die Zahl bewegter Punkte zurueck.
std::size_t formenAnwenden(Patch& p, const Vektor3f& mitte, const FormPinsel& f, float ebeneZ);

// Auf alle Patches der Karte, die der Kreis beruehrt (sichert sie vorher fuer das Undo).
std::size_t formenAufKarte(Karte& k, const Vektor3f& mitte, const FormPinsel& f, float ebeneZ);

}  // namespace bhed::karte

#endif
