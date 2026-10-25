// optimierer.h - Karten-Editor, Stufe 5: der Optimierer. Er zaehlt, was eine
// Karte gegen die Grenzen der Engine verbraucht (Punkt 2), sucht Bauprobleme
// (Punkt 3) und behebt sie auf Knopfdruck (Punkt 4).
//
// NRC hat keinen Optimierer - das ist eine eigene Funktion.
// Portiert aus OpenJK (die Zaehlregeln) und aus q3map2: gezaehlt wird wie
// die Engine zaehlt, mit den Regeln aus deren Quelltext:
//   - OpenJK code/game/g_spawn.cpp G_SpawnGEntityFromSpawnVars / G_ParseSpawnVars
//     / G_AddSpawnVarToken (was nach dem Spawn bleibt, Schluessel je Entitaet),
//     g_utils.cpp G_FindConfigstringIndex (Modelle, Klaenge, Effekte),
//     g_fx.cpp SP_CreateRain/Snow/Wind/WindZone/PuffSystem und g_misc.cpp
//     SP_misc_weather_zone (Welteffekte), die SP_*-Funktionen, die sich beim
//     Spawn selbst freigeben (g_misc.cpp, g_mover.cpp, g_nav.cpp, ...);
//   - code/qcommon/cm_load.cpp CMod_LoadSubmodels / CMod_LoadPatches und
//     cm_patch.cpp (patchkollision.h);
//   - code/rd-vanilla/tr_bsp.cpp (Lightmaps, Lichtgitter, Flaechen, Nebel);
//   - q3map2 (NetRadiant-Custom 20260114): map.cpp (func_group, group_info),
//     bsp.cpp / writebsp.cpp (Modellnummern, SetLightStyles, UnparseEntities).
// Movie Duels und Jedi Outcast mit ihren Abweichungen (grenzen.h).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#ifndef BHED_KARTE_OPTIMIERER_H
#define BHED_KARTE_OPTIMIERER_H

#include "bhed/karte/grenzen.h"
#include "bhed/karte/mathe.h"

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed {
struct Gegenstandsliste;
}

namespace bhed::karte {

class Karte;

// --- Momentaufnahme der Karte (fuer die Arbeit im Hintergrund) -------------
//
// Auf dem Hauptfaden aus der Karte gezogen (kartenAbbild), danach ohne Bezug
// zur Karte: der Optimierer laeuft in einem eigenen Faden. Die Indizes sind
// die der Karte zum Zeitpunkt der Aufnahme (Entitaet in Karte::entitaeten(),
// Primitiv in Entitaet::kinder()).
struct AbbildFlaeche {
    std::string shader;
    Ebene3d ebene;
    std::vector<Vektor3d> punkte;   // die Wicklung (Brush::buildWindings)
    int inhalt = 0;                 // m_contentFlags
    int flaechenFlags = 0;
};
struct AbbildPrimitiv {
    bool patch = false;
    bool detail = false;
    bool entartet = false;          // Pinsel::entartet (keine gueltige Wicklung)
    std::vector<AbbildFlaeche> flaechen;
    int breite = 0;                 // Patch
    int hoehe = 0;
    std::vector<std::array<float, 3>> kontrollpunkte;   // zeilenweise
    std::string shader;             // Patch
    Vektor3f mins{0, 0, 0};
    Vektor3f maxs{0, 0, 0};
};
struct AbbildEntitaet {
    std::vector<std::pair<std::string, std::string>> schluessel;
    std::string klasse;
    std::vector<AbbildPrimitiv> prims;
    [[nodiscard]] std::string wert(const std::string& schluessel) const;   // "" wenn nicht da (ohne Gross/klein)
    [[nodiscard]] bool hat(const std::string& schluessel) const;
};
struct KartenAbbild {
    std::vector<AbbildEntitaet> ents;
    std::size_t stand = 0;   // Rueckgaengig::stand() zum Zeitpunkt
};
[[nodiscard]] KartenAbbild kartenAbbild(const Karte& k);

// --- Zaehlen ---------------------------------------------------------------

struct ObjektRef {
    int entitaet = -1;     // Index in KartenAbbild::ents (bzw. im Entity-Lump der BSP)
    int primitiv = -1;     // Index in AbbildEntitaet::prims
    int bspFlaeche = -1;   // Index der Zeichenflaeche in der BSP
    std::string text;      // was es ist ("worldspawn, Pinsel 12", "textures/x", "maps/a.rof")
};
struct Beitrag {
    ObjektRef ref;
    long long wert = 0;
};
enum class ZaehlQuelle { Karte, Bsp };
struct Zaehlung {
    std::string id;           // Grenze (grenzen.h)
    ZaehlQuelle quelle = ZaehlQuelle::Karte;
    long long wert = 0;
    bool schaetzung = false;  // nicht genau so zu zaehlen wie die Engine
    bool obergrenze = false;  // die Schaetzung liegt ueber dem wahren Wert (nur "ueber", wenn auch "sicher" darueber liegt)
    long long sicher = -1;    // der Teil, der sicher eintritt (z. B. NPC-Spawner ohne targetname), -1 = keiner
    std::string hinweis;      // Erklaerung der Zaehlung (Englisch, fuer den Bericht)
    std::vector<Beitrag> beitraege;   // groesste zuerst, hoechstens kMaxBeitraege
};
inline constexpr std::size_t kMaxBeitraege = 12;

struct ZaehlOptionen {
    GrenzSpiel spiel = GrenzSpiel::JkaSp;
    // NPC_type -> playerModel (ext_data/npcs/*.npc); leer = unbekannt.
    std::function<std::string(const std::string&)> npcModell;
    // items.dat des Spiels (Gegenstaende spawnen ohne Eintrag in spawns[]).
    const Gegenstandsliste* gegenstaende = nullptr;
};

// Eine Entitaet nur mit Schluesseln (Entity-Lump der BSP oder Karte).
using EntSchluessel = std::vector<std::pair<std::string, std::string>>;

// Bleibt die Entitaet nach G_SpawnGEntityFromSpawnVars? (spawns[] + die
// SP_*-Funktionen, die sich selbst freigeben, je Spiel; "notsingle")
[[nodiscard]] bool bleibtNachSpawn(const EntSchluessel& e, GrenzSpiel spiel, const Gegenstandsliste* gegenstaende = nullptr);
// G_ParseSpawnVars / G_AddSpawnVarToken: Zahl der Paare und Zeichen samt Endnullen.
[[nodiscard]] std::pair<long long, long long> spawnVariablen(const EntSchluessel& e);

// Aus der Karte (ohne Bauen): alles, was die .map schon sagt.
[[nodiscard]] std::vector<Zaehlung> zaehleKarte(const KartenAbbild& a, const ZaehlOptionen& o);

// Aus der gebauten .bsp (RBSP): jeder Lump, Lichtgitter, Lightmaps, Flaechen,
// Patch-Kollision, Entity-Lump; prtDaten: die .prt daneben (leer: ohne Portale).
struct BspAnalyse {
    bool ok = false;
    std::string fehler;
    std::vector<Zaehlung> werte;
    // fuer die Pruefungen (Punkt 3)
    long long lichtgitterUeberlauf = 0;   // Zellen mit Index >= Zahl der Punkte
    bool lichtgitterFehlpassung = false;  // "light grid array mismatch" (tr_bsp.cpp)
    long long lichtgitterErwartet = 0;    // gridBounds-Produkt der Engine
    std::array<float, 3> gitterGroesse{64, 64, 128};
    long long cluster = 0;
    long long portale = 0;
    long long sichtBytes = 0;
};
[[nodiscard]] BspAnalyse analysiereBsp(const std::string& bspDaten, const ZaehlOptionen& o, const std::string& prtDaten = std::string());

// --- Bewerten --------------------------------------------------------------

enum class Ampel { Ok, Knapp, Ueber, Hinweis };
struct Bewertung {
    const Grenze* grenze = nullptr;
    Zaehlung zaehlung;
    double anteil = 0;       // wert / erlaubt
    Ampel ampel = Ampel::Ok;
};
// knappAb: ab diesem Anteil "knapp" (Einstellung, Vorgabe 0.9). Harte Grenzen
// werden "ueber", wenn wert > erlaubt (ausser obergrenze); weiche
// (Formatgrenzen) hoechstens "Hinweis".
[[nodiscard]] std::vector<Bewertung> bewerten(GrenzSpiel s, const std::vector<Zaehlung>& z, double knappAb = 0.9);
[[nodiscard]] const char* ampelName(Ampel a);   // "ok" / "near" / "over" / "note"

}  // namespace bhed::karte

#endif
