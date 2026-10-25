// grenzen.h - Karten-Editor, Stufe 5 Punkt 1: die Grenzen von Jedi Academy
// (OpenJK SP, dazu die Werte von MP), Movie Duels und Jedi Outcast als Daten -
// jede mit ihrem Wert, der Stelle im Quelltext der Engine (Datei:Zeile), der
// Art (BSP-Lump, Laufzeit, Renderer, Kollision, Spawn, Compiler) und der
// strengeren Vanilla-Variante (Spieler ohne OpenJK).
//
// Portiert aus OpenJK (Werte und Pruefstellen; Quelltext unter SP\ojk\oj:
// code/ = Jedi Academy SP, code/ mit JK2_MODE + codeJK2/ = Jedi Outcast SP,
// codemp/ = Jedi Academy MP), aus Movie Duels (SP\mdgit\code, eigene Engine
// auf OpenJK-Grundlage) und aus q3map2 (NetRadiant-Custom 20260114,
// third_party/q3map2: was der Compiler selbst begrenzt).
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
//
// Was "erlaubt" heisst, steht je Grenze in "rechnung": die Engine prueft mal
// vor dem Anlegen (numPlanes == MAX_PATCH_PLANES -> Fehler: bis MAX erlaubt),
// mal ab Index 1 (G_FindConfigstringIndex: MAX_MODELS - 1 Modelle), mal mit
// abgezogenen festen Plaetzen (G_Spawn: MAX_GENTITIES - Welt - Leer - Spieler).
// Geprueft wird immer gegen die strengere Vanilla-Grenze.
//
// "hart" sind Grenzen, an denen die Engine abbricht (Com_Error / G_Error) oder
// hinter ein Feld schreibt. Die MAX_MAP_*-Werte aus qfiles.h prueft die Engine
// nicht (cm_load.cpp und tr_bsp.cpp legen jeden Lump in seiner Groesse an);
// Ravens eigene Karten liegen darueber (t2_dpred: 2304 Entities und 290934
// Bytes Entity-Text, yavin1b: 265730 Ebenen). Sie stehen als Formatgrenzen
// ("weich") in der Tabelle und werden nie "ueber" gemeldet.
#ifndef BHED_KARTE_GRENZEN_H
#define BHED_KARTE_GRENZEN_H

#include "bhed/i18n.h"

#include <string>
#include <string_view>
#include <vector>

namespace bhed::karte {

// Die Regelwerke des Programms (bhed/regelwerk.h) und dazu JKA MP.
enum class GrenzSpiel { JkaSp, JkaMp, MovieDuels, Jk2 };
inline constexpr int kGrenzSpiele = 4;

enum class GrenzArt { BspLump, Laufzeit, Renderer, Kollision, Spawn, Compiler };
inline constexpr int kGrenzArten = 6;

// Eine Stelle im Quelltext: ein #define bzw. constexpr (name) mit seinem Wert,
// oder (name leer) eine Zahl, die in der Zeile steht (q3map2 games.cpp).
struct GrenzStelle {
    std::string baum;         // "OpenJK", "MovieDuels", "q3map2"
    std::string datei;        // relativ zur Wurzel des Baums ("code/qcommon/qfiles.h")
    int zeile = 0;
    std::string name;         // "MAX_MAP_BRUSHES"
    long long wert = 0;
    bool jk2 = false;         // gilt im Zweig JK2_MODE (#ifdef JK2_MODE)
};

struct Grenze {
    std::string id;           // "kollision.patchebenen" (gleich in allen Spielen)
    GrenzArt art = GrenzArt::BspLump;
    i18n::Str titel = i18n::Str::Count;
    std::string name;         // Name im Quelltext ("MAX_PATCH_PLANES")
    long long wert = 0;       // der Wert in der Engine des Regelwerks
    long long vanilla = 0;    // die strengere Vanilla-Grenze (= wert, wenn gleich)
    long long erlaubt = 0;    // der groesste erlaubte Zaehlwert (aus der strengeren)
    bool hart = true;         // die Engine bricht ab / schreibt hinter ein Feld
    bool jeObjekt = false;    // gilt je Entitaet, je Patch, je Flaeche (nicht je Karte)
    bool zaehlbar = true;     // aus Karte oder BSP zu zaehlen (sonst nur Tabelle)
    bool offen = false;       // keine eigene Quelle fuer dieses Spiel (JK2)
    std::vector<GrenzStelle> stellen;   // Definition(en), die ctest gegen den Quelltext prueft
    std::string pruefstelle;  // wo die Engine prueft ("code/qcommon/cm_load.cpp:143 Com_Error")
    std::string rechnung;     // wie "erlaubt" aus dem Wert folgt
    std::string vanillaQuelle;   // woher die Vanilla-Grenze kommt (wenn anders)
};

[[nodiscard]] const char* grenzSpielName(GrenzSpiel s);      // "Jedi Academy (OpenJK SP)" ...
[[nodiscard]] const char* grenzSpielSchluessel(GrenzSpiel s); // "jkasp" ...
[[nodiscard]] bool grenzSpielAusSchluessel(std::string_view s, GrenzSpiel& out);
[[nodiscard]] const char* grenzArtName(GrenzArt a);           // "BSP lump" ... (Englisch, fuer den Bericht)
[[nodiscard]] i18n::Str grenzArtTitel(GrenzArt a);

// Die Tabelle eines Spiels (fest, in der Reihenfolge der Anzeige).
[[nodiscard]] const std::vector<Grenze>& grenzen(GrenzSpiel s);
[[nodiscard]] const Grenze* grenzeSuchen(GrenzSpiel s, std::string_view id);

// Die Spielwahl zum Regelwerk des Programms (kRegelwerkOpenJK/MovieDuels/JediOutcast).
[[nodiscard]] GrenzSpiel grenzSpielFuerRegelwerk(int regelwerkSpiel);

}  // namespace bhed::karte

#endif
