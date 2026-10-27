// lichtschaetzung.h - Karten-Editor Stufe 8a Punkt 4: Lichter einer fertigen .bsp
// schaetzen (eigene Funktion, nicht in NetRadiant-Custom; experimentell).
//
// q3map2 entfernt jedes Licht ohne targetname (writebsp.cpp SetLightStyles). Was
// bleibt, ist ihre Wirkung: das Lichtgitter (je Zelle Umgebungslicht, gerichtetes
// Licht und die Richtung ZUR Hauptlichtquelle, light.cpp TraceGrid) und die
// Lightmaps. Fuer ein Punktlicht gilt im Gitter gerichtet = Farbe * light * 7500 /
// Abstand^2 (LightContributionToPoint: photons = light * pointScale, add = photons /
// dist^2, Abstand mindestens 16), solange es die Zelle beherrscht.
//
// Schaetzung: jede Zelle mit gerichtetem Licht schickt einen Strahl in ihre
// Lichtrichtung; wo sich viele Strahlen treffen (Stimmen in einem 32er-Raster),
// steht vermutlich ein Licht; der Ort ist der Punkt mit dem kleinsten Abstand zu den
// beteiligten Strahlen (kleinste Quadrate), die Staerke der Median aus gerichtet *
// Abstand^2 / 7500 der ungesaettigten Zellen, die Farbe das Mittel ihrer Richtungs-
// farben. Ein info_null mit targetname, auf den keine Entitaet mehr zeigt, war
// meist das Ziel eines Spotlichts: ein geschaetztes Licht darueber bekommt "target".
//
// Jedes geschaetzte Licht traegt "_bhed_geschaetzt" "1" - q3map2 und das Spiel
// lesen den Schluessel nicht (q3map2 nur bekannte Schluessel, das Spiel gibt
// Lichter ohne targetname beim Spawnen frei).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: light.cpp TraceGrid - die
// Kodierung der Gitterpunkte, LightContributionToPoint - die Staerke eines Punktlichts
// 7500 / d^2).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_LICHTSCHAETZUNG_H
#define BHED_KARTE_LICHTSCHAETZUNG_H

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

struct GeschaetztesLicht {
    double origin[3]{0, 0, 0};
    double licht = 300;             // Schluessel "light"
    double farbe[3]{1, 1, 1};       // "_color", groesster Anteil 1
    std::string ziel;               // "target" (Spotlicht auf ein info_null)
    double stimmen = 0;             // Gewicht der Strahlen, die es stuetzen
    int zellen = 0;                 // Gitterzellen der Staerke-Schaetzung
};
struct LichtSchaetzOptionen {
    double raster = 32.0;           // Stimmenraster (Einheiten)
    double reichweite = 1536.0;     // so weit laeuft ein Strahl
    double mindestStimmen = 0.02;   // Anteil am staerksten Licht, darunter verworfen
    std::function<bool(double)> fortschritt;
};
[[nodiscard]] std::vector<GeschaetztesLicht> lichterSchaetzen(const std::string& bspDaten, const LichtSchaetzOptionen& o);
// Die Schluessel einer light-Entitaet (classname, origin, light, _color, target, _bhed_geschaetzt).
[[nodiscard]] std::vector<std::pair<std::string, std::string>> lichtSchluessel(const GeschaetztesLicht& l);

}  // namespace bhed::karte

#endif
