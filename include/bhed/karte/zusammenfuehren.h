// zusammenfuehren.h - Karten-Editor Stufe 8a Punkt 5c: "Zur fertigen Karte hinzufuegen"
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Eine fertige .bsp bleibt, wie sie ist; nur das Neue (eine Ebene der Karte) wird
// gebaut und mit q3map2 -mergebsp -world -fixnames eingefuegt (convert_bsp.cpp
// MergeBSPMain: die Welt-Brushes und -Flaechen des Teils kommen in die Welt der
// Hauptkarte "as if they were detail" - kein neuer BSP-Baum, keine neue Sichtbarkeit;
// sie werden in jedes Blatt eingetragen, dessen Kasten sie beruehren; Lightmaps
// werden angehaengt, das Lichtgitter bleibt das der Hauptkarte; -fixnames macht
// target/targetname des Teils eindeutig).
//
// Pruefungen vor dem Zusammenfuehren (JKHub-Thread 13136 und die Grenzen):
//   - Inside- und Outside-Wetterbrushes gemischt: die Karte laedt nicht;
//   - gibt es misc_weather_zone, gelten Inside/Outside nur innerhalb einer Zone;
//   - gleiche targetname in beiden Teilen (-fixnames benennt um - Verweise
//     zwischen Haupt und Teil gehen damit verloren);
//   - danach: die Grenzen der zusammengefuehrten .bsp (Optimierer) und die
//     Kollision der eingefuegten Brushes im OpenJK-CM-Port (CM_BoxTrace).
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_ZUSAMMENFUEHREN_H
#define BHED_KARTE_ZUSAMMENFUEHREN_H

#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

struct Q3map2Aufruf;

// q3map2 -mergebsp -world -fixnames <haupt> <teil> (mit -game ja/jk2 aus dem Aufruf;
// ohne -game meldet q3map2 "not IBSP"). Das Ergebnis heisst <haupt ohne .bsp>_merged.bsp.
[[nodiscard]] std::string mergeBefehl(const Q3map2Aufruf& a, const std::string& hauptBsp, const std::string& teilBsp);
[[nodiscard]] std::string mergeZiel(const std::string& hauptBsp);

struct MergeBefund {
    enum class Schwere { Hinweis, Warnung, Fehler };
    Schwere schwere = Schwere::Hinweis;
    std::string pruefung;    // "wetter", "zone", "namen", "grenze", "kollision"
    std::string text;        // englisch, mit Zahlen
    long long anzahl = 0;
};

// Vor dem Zusammenfuehren: Wetter (inside/outside), Zonen, Namen.
[[nodiscard]] std::vector<MergeBefund> mergeVorPruefen(const std::string& hauptBsp, const std::string& teilBsp);

// Nach dem Zusammenfuehren: die eingefuegten Welt-Brushes (die letzten teilBrushes der
// Welt) muessen den Spieler stoppen - je Brush eine Spur des Spielerkastens
// (-15 -15 -24 .. 15 15 32, MASK_PLAYERSOLID) von aussen auf seine Mitte; dazu laedt
// die .bsp im CM-Port. ok/gesamt = Brushes, die stoppen / pruefbar waren.
struct MergeKollision {
    bool geladen = false;
    int gesamt = 0;          // feste Brushes des Teils, die sich pruefen liessen
    int stoppen = 0;
    int nichtPruefbar = 0;   // kein freier Startpunkt um den Brush
};
[[nodiscard]] MergeKollision mergeKollisionPruefen(const std::string& mergedBsp, int teilWeltBrushes);

// Licht fuer den Teil aus der Hauptkarte (angenaehert - q3map2 kann den Teil nicht
// gegen die Geometrie der Hauptkarte beleuchten): das Lichtgitter der Hauptkarte im
// Kasten des Teils gemittelt als worldspawn "_ambient"/"_color", dazu die Lichter, die in
// der Hauptkarte stehen geblieben sind (targetname, -keeplights), im Kasten + Rand.
struct MergeLicht {
    double ambient = 0;
    double farbe[3]{1, 1, 1};
    int gitterPunkte = 0;
    std::vector<std::vector<std::pair<std::string, std::string>>> lichter;
};
[[nodiscard]] MergeLicht mergeLichtAusHauptkarte(const std::string& hauptBsp, const double mins[3], const double maxs[3], double rand);

// Zahl der Welt-Brushes einer .bsp (Modell 0)
[[nodiscard]] int weltBrushes(const std::string& bspDaten);

}  // namespace bhed::karte

#endif
