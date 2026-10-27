// logik.h - Karten-Editor, Stufe 8b Punkt 4: die Logik-Ansicht (eigene
// Funktion, nicht in NetRadiant-Custom) - der Graph der Entity-Verknuepfungen.
//
// Welche Schluessel verknuepfen, steht in der Spiellogik: OpenJK
// code/game/g_spawn.cpp (fields[]: target, target2..target4, targetJump,
// closetarget, opentarget, paintarget, backwardstarget, NPC_target,
// NPC_target2, NPC_target4 - G_Find( targetname ) bzw. NPC_targetname des
// gespawnten NPC; script_targetname fuer ICARUS) und in den
// Entity-Definitionen des Spiels (jeder "*target*"-Schluessel einer Klasse,
// z. B. target5, target6, goaltarget, throwtarget; killtarget/deathtarget aus
// Quake-artigen Definitionen). Die Skript-Schluessel sind die behaviorSet-
// Felder (g_spawn.cpp: spawnscript ... mindtrickscript).
//
// Uebernommene Teile (die Schluessellisten) aus OpenJK:
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License version 2 as published
//   by the Free Software Foundation (LICENSE). Portiert aus OpenJK.
#ifndef BHED_KARTE_LOGIK_H
#define BHED_KARTE_LOGIK_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class EntitaetsKlassen;

struct LogikKnoten {
    std::weak_ptr<Entitaet> entitaet;   // leer: ein fehlendes Ziel ("? name")
    std::string klasse;
    std::string name;                   // targetname (bzw. NPC_targetname, script_targetname)
    std::vector<std::pair<std::string, std::string>> namen;    // alle Namensschluessel mit Wert
    std::vector<std::pair<std::string, std::string>> skripte;  // Skript-Schluessel mit Wert
    int ebene = 0;                      // Spalte (laengster Weg von einer Quelle)
    int zeile = 0;                      // Platz in der Spalte
    int gruppe = 0;                     // zusammenhaengender Teil
    float x = 0, y = 0;                 // Lage im Graphen (Punkte)
    float breite = 0, hoehe = 0;
};
struct LogikKante {
    int von = 0, zu = 0;
    std::string schluessel;             // target, target2, NPC_target ...
    std::string wert;
};
struct LogikGraph {
    std::vector<LogikKnoten> knoten;
    std::vector<LogikKante> kanten;
    int fehlend = 0;                    // Werte ohne Ziel
    std::vector<std::string> verknuepfungen;   // die benutzten Verknuepfungsschluessel
};

// Die Verknuepfungsschluessel (OpenJK-Felder und "*target*" aus den Definitionen)
[[nodiscard]] std::vector<std::string> logikVerknuepfungen(const EntitaetsKlassen* klassen);
// Die Skript-Schluessel (behaviorSet, OpenJK g_spawn.cpp:312-327)
[[nodiscard]] const std::vector<std::string>& logikSkriptSchluessel();
// Die Namensschluessel: targetname, NPC_targetname, script_targetname
[[nodiscard]] const std::vector<std::string>& logikNamensSchluessel();

// Der Graph mit Lage: Spalten nach dem laengsten Weg (Kreise an der ersten
// Rueckkante gebrochen), Teile untereinander, alles fest sortiert.
[[nodiscard]] LogikGraph logikGraph(const Karte& k, const EntitaetsKlassen* klassen);

}  // namespace bhed::karte

#endif
