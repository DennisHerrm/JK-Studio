// bspoeffnen.h - Karten-Editor, Stufe 4: fertige Karten oeffnen (.bsp aus
// einem Ordner oder einer .pk3) und das Originallicht behalten.
//
// Portiert aus NetRadiant-Custom 20260114:
//   radiant/map.cpp:1539-1622   Map_ImportFile mit tryDecompile (der Aufruf
//                               von q3map2 -convert, "<name>_converted.map")
//   plugins/mapq3/plugin.cpp:239 "quake3 compiled maps" *.bsp (nur Import)
// und aus q3map2 (NRC 20260114, tools/quake3/q3map2):
//   bsp.cpp:570-605, 614-650, 870-897  BSPMain mit -onlyents, OnlyEnts
//   writebsp.cpp UnparseEntities        misc_model, _decal, _skybox kommen
//                                       nicht in die .bsp (eingebacken)
//   bspfile_ibsp.cpp / bspfile_rbsp.cpp  Kopf und Lumps (IBSP 17, RBSP 18)
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
#ifndef BHED_KARTE_BSPOEFFNEN_H
#define BHED_KARTE_BSPOEFFNEN_H

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

class Dateisystem;
class Karte;

// Eine fertige Karte im Spiel: maps/**.bsp in einem Archiv (.pk3) oder Ordner.
struct BspEintrag {
    std::string pfad;     // "maps/mp/duel1.bsp" (Schreibweise wie im Archiv)
    std::string archiv;   // Pfad der .pk3 bzw. Ordner mit '/' am Ende
};

// Alle maps/**/*.bsp des Dateisystems, wie Spiel und Editor sie sehen: je Name
// (ohne Gross/klein) das erste Archiv in der Suchreihenfolge (QE_InitVFS:
// fs_game vor basegame, je Ordner die spaeteren .pk3 zuerst), nach Pfad
// sortiert.
[[nodiscard]] std::vector<BspEintrag> bspListe(const Dateisystem& fs);

// Der Aufruf von q3map2 (Map_ImportFile tryDecompile, map.cpp:1583-1594):
//   <programm> -v -game <typ> -fs_basepath "<Engine>" -fs_homepath "<Benutzer>"
//   -fs_game <fs_game> -convert -format map|map_bp "<bsp>"
// typ = q3map2_type der .game (ja / jk2), format nach dem Pinselformat.
struct Q3map2Aufruf {
    std::string programm;      // Pfad von bhed_q3map2.exe (wird in "" gesetzt)
    std::string spielTyp;      // "ja" / "jk2"
    std::string enginePfad;    // EnginePath_get
    std::string benutzerPfad;  // g_qeglobals.m_userEnginePath (JA/JK2: leer)
    std::string fsGame;        // gamename_get ("base", "MD")
};
[[nodiscard]] std::string convertBefehl(const Q3map2Aufruf& a, bool brushPrimitives, const std::string& bsp);
// str( PathExtensionless( filename ), "_converted.map" )
[[nodiscard]] std::string convertZiel(const std::string& bsp);
// q3map2 -bsp -onlyents (BSPMain, bsp.cpp:647-650, 894-897): liest die
// Entitaeten aus "<stamm>.map" und schreibt sie in "<stamm>.bsp". Mit
// lichterBehalten zusaetzlich -keeplights (bsp.cpp:658): SetLightStyles
// (writebsp.cpp:222-260) entfernt sonst jedes light ohne targetname - Ravens
// Jedi-Outcast-Karten haben ihre Lichter aber im Entity-Lump.
[[nodiscard]] std::string onlyentsBefehl(const Q3map2Aufruf& a, const std::string& map, bool lichterBehalten = false);

// Entitaeten, deren Inhalt q3map2 in die Geometrie einbackt und aus dem
// Entity-Lump entfernt (UnparseEntities: misc_model, _decal, _skybox).
[[nodiscard]] bool eingebackeneEntitaet(const std::string& klassenname);

// Fingerabdruck der Geometrie: jede Entitaet mit Pinseln oder Patches (mit
// allen Ebenenpunkten, Shadern, Texturprojektionen, Flags und Kontrollpunkten,
// so wie sie in der .map stehen, ohne die Schluessel) und jede eingebackene
// Entitaet (mit ihren Schluesseln). Gleich nach einer Aenderung = es wurden
// nur Entities geaendert (Schluessel, Punkt-Entities verschoben, neue
// Punkt-Entities): dann reicht -onlyents, Lightmaps und Lichtgitter bleiben.
[[nodiscard]] std::uint64_t geometrieFingerabdruck(const Karte& k);

// Kopf und Lumps einer IBSP- (Quake 3, 17 Lumps) oder RBSP-Datei (Raven:
// Jedi Academy, Jedi Outcast, 18 Lumps). Der Entity-Lump ist Lump 0.
struct BspDatei {
    std::string ident;     // "IBSP" / "RBSP"
    int version = 0;
    std::vector<std::string> lumps;
};
[[nodiscard]] bool bspLesen(const std::string& daten, BspDatei& out);
// Der Text des Entity-Lumps (ohne die abschliessende NUL).
[[nodiscard]] std::string bspEntitaetenText(const BspDatei& b);
// Hat der Entity-Lump ein light (classname "light...")? Dann -keeplights.
[[nodiscard]] bool bspHatLichter(const std::string& entitaetenText);
// Stufe 6 (JKHub, Lichtstile nach Entity-Aenderung): die schaltbaren Lichter in der Reihenfolge,
// in der q3map2 SetLightStyles (writebsp.cpp) ihnen die Stile 32, 33, ... gibt - je
// (targetname, urspruenglicher Stil) beim ersten Auftreten. ausBsp: der Entity-Lump einer
// gebauten .bsp (Stil steht dort in "switch_style", "style" ist 32+j); sonst eine .map
// (UnSetLightStyles hat "style" zurueckgesetzt). Aendert sich diese Liste, passen die in die
// Lightmaps gebackenen Stile nicht mehr zu den Lichtern - -onlyents darf dann nicht laufen.
[[nodiscard]] std::vector<std::pair<std::string, int>> schaltlichter(const std::string& entitaetenText, bool ausBsp);
// Die Datei "original" mit dem Entity-Lump aus "neu" (beide IBSP/RBSP
// derselben Art): alle anderen Lumps Byte fuer Byte aus dem Original, in
// dessen Reihenfolge, je auf 4 Bytes aufgefuellt (AddLump). Abweichung von
// q3map2 -onlyents: dessen WriteRBSPFile packt das Lichtgitter neu
// (AddLightGridLumps: Naeherungs-Zwillinge, LG_EPSILON 4) - bei Ravens
// Jedi-Outcast-Karten aendert das lightgrid und lightarray. "" bei Fehler.
[[nodiscard]] std::string bspEntitaetenEinsetzen(const std::string& original, const std::string& neu);

}  // namespace bhed::karte

#endif
