// feeder.h - Menue-Editor M2 Punkt 1: die Listen der Listboxen (Feeder) aus
// den Spieldaten, wie das Spiel sie fuellt.
//
// Portiert aus OpenJK: code/ui/ui_main.cpp (UI_FeederCount, UI_FeederItemText,
// UI_FeederItemImage, UI_BuildPlayerModel_List, IsImageFile,
// UI_ParseColorData, datapadMoveTitleData/datapadMoveData, UI_LoadMods,
// UI_LoadLanguages) und codemp/ui/ui_main.c (UI_FeederCount,
// UI_FeederItemText, UI_LoadArenas/UI_ParseInfos fuer FEEDER_MAPS/ALLMAPS,
// UI_BuildQ3Model_List fuer FEEDER_Q3HEADS, UI_LoadDemos, UI_LoadMovies,
// UI_LoadForceConfig_List) sowie code/qcommon/files.cpp (FS_GetFileList ->
// FS_ListFilteredFiles: Reihenfolge der Suchpfade, Tiefe, keine Doppelten).
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
// Was ein laufendes Spiel braucht (Server, Spieler, Punktestand, Siege-
// Klassen) bleibt leer - so zeigt das Spiel diese Listen im Hauptmenue auch.
#ifndef BHED_MENUE_FEEDER_H
#define BHED_MENUE_FEEDER_H

#include "bhed/menue/menue.h"
#include "bhed/menue/spieldaten.h"
#include "bhed/menue/vorschau.h"

#include <string>
#include <vector>

namespace bhed::menue {

// Die Feeder-Nummern je Dialekt (menudef.h)
namespace feeder_sp {
inline constexpr int SAVEGAMES = 0x00, MODS = 0x09, PLAYER_SPECIES = 0x10, PLAYER_SKIN_HEAD = 0x11, PLAYER_SKIN_TORSO = 0x12,
                     PLAYER_SKIN_LEGS = 0x13, COLORCHOICES = 0x14, MOVES = 0x15, MOVES_TITLES = 0x16, LANGUAGES = 0x17;
}
namespace feeder_mp {
inline constexpr int MAPS = 0x01, SERVERS = 0x02, ALLMAPS = 0x04, MODS = 0x09, DEMOS = 0x0a, Q3HEADS = 0x0c, CINEMATICS = 0x0f,
                     FORCECFG = 0x10, PLAYER_SPECIES = 0x13, PLAYER_SKIN_HEAD = 0x14, PLAYER_SKIN_TORSO = 0x15, PLAYER_SKIN_LEGS = 0x16,
                     COLORCHOICES = 0x17, LANGUAGES = 0x28, MOVES = 0x29, MOVES_TITLES = 0x2a;
}

// Ein Spezies-Ordner aus models/players (UI_BuildPlayerModel_List)
struct Spezies {
    std::string name;
    std::vector<std::string> kopf, rumpf, beine;   // head_*, torso_*, lower_* (mit Symbol)
    std::vector<std::string> farbShader;           // PlayerChoice.txt
    std::vector<std::string> farbAktion;
};

struct FeederDaten {
    const Spieldaten* daten = nullptr;
    Dialekt dialekt = Dialekt::SP;
    Spiel spiel = Spiel::JA;
    // Ordner der Spieldaten auf der Platte (fuer saves/, Mods, demos/)
    std::vector<std::string> ordner;
    std::string basisOrdner;   // fs_basepath (Eltern von base)

    // Einmal aufbauen (UI_Init/UI_Load der Engine)
    void baue();
    // Eintraege einer Liste; spezies = uiInfo.playerSpeciesIndex, titel = movesTitleIndex
    [[nodiscard]] std::vector<FeederEintrag> eintraege(float feeder, int spezies = 0, int titel = 0) const;

    std::vector<Spezies> spezies;
    std::vector<std::string> sprachen;
    std::vector<std::pair<std::string, std::string>> mods;   // Ordner, Beschreibung
    std::vector<std::pair<std::string, std::string>> karten; // MP: Karte, Langname (arenas)
    std::vector<std::string> koepfe;                        // MP: kopf/skin (icon)
    std::vector<std::string> demos, filme, kraefte;
    std::vector<std::pair<std::string, std::string>> spielstaende;   // Name, Datum
    bool gebaut = false;
};

// FS_GetFileList(pfad, endung) wie die Engine; endung "/" = Ordner.
[[nodiscard]] std::vector<std::string> dateiListe(const Spieldaten& d, const std::string& pfad, const std::string& endung);

}  // namespace bhed::menue

#endif  // BHED_MENUE_FEEDER_H
