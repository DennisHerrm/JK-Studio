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

#include <functional>
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
                     COLORCHOICES = 0x17, LANGUAGES = 0x28, MOVES = 0x29, MOVES_TITLES = 0x2a, PLAYER_LIST = 0x07,
                     SABER_SINGLE_INFO = 0x2b, SABER_STAFF_INFO = 0x2c;
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
    // M5: uiInfo.playerSpeciesIndex aus ui_char_model wie UI_GetCharacterCvars ("getcharcvars"):
    // SP (code/ui ui_main.cpp) nimmt den letzten passenden Ordner, MP (codemp UI_UpdateCharacterSkin
    // u. a.) den ersten; ohne Treffer bleibt bzw. wird es 0.
    [[nodiscard]] int speziesIndex(const std::string& modell) const;
    // M7: hat die Liste eine Darstellung? 0 = nein (Platzhalter), 1 = wie im Spiel (auch leer, z. B. die
    // Serverliste vor der Abfrage), 2 = Beispielwerte (nur im laufenden Spiel gefuellt)
    [[nodiscard]] int art(float feeder) const;
    void mdEintraege(int f, std::vector<FeederEintrag>& r) const;
    std::vector<Spezies> spezies;
    std::vector<std::string> sprachen;
    std::vector<std::pair<std::string, std::string>> mods;   // Ordner, Beschreibung
    std::vector<std::pair<std::string, std::string>> karten; // MP: Langname, "1" = FFA-tauglich (arenas)
    std::vector<std::string> kartenDatei;                   // MP: mapLoadName je Karte (gleiche Reihenfolge)
    std::vector<std::pair<std::string, int>> spieltypen;    // MP: ui/jamp/gameinfo.txt gametypes (Name, gtEnum)
    std::vector<std::pair<std::string, int>> joinSpieltypen;   // M7: joingametypes (UI_JOINGAMETYPE)
    std::vector<std::string> bots;                          // MP: Botnamen (UI_LoadBots)
    std::vector<std::string> koepfe;                        // MP: kopf/skin (icon)
    std::vector<std::string> demos, filme, kraefte;
    std::function<std::string(const std::string&)> se;   // M7: SE_GetStringTextString (ohne "@"); die Oberflaeche setzt es
    std::vector<std::string> saberEinhand, saberStab;   // M7: WP_SaberGetHiltInfo (MP), Name wie UI_SaberProperNameForSaber
    std::vector<std::pair<std::string, std::string>> spielstaende;   // Name, Datum
    std::string mdShop;      // Movie Duels 9: ext_data/md_shop.dat (Shop_LoadItems liest sie einmal)
    bool mdShopDa = false;
    bool gebaut = false;
};

// FS_GetFileList(pfad, endung) wie die Engine; endung "/" = Ordner.
[[nodiscard]] std::vector<std::string> dateiListe(const Spieldaten& d, const std::string& pfad, const std::string& endung);

// M7: WP_SaberGetHiltInfo ueber saberParms (alle .sab hintereinander): jeder Name mit "{" auf einer
// folgenden Zeile, ohne notInMP; twoHanded -> Stab. Eintrag = "name" des Saebels (UI_SaberProperNameForSaber).
void saberListen(const std::string& parms, std::vector<std::string>& einhand, std::vector<std::string>& stab);
// M7: Movie Duels duelMaps[index].duelMapImage (UI_DUELMAPS_SELECTION); "" ausserhalb
[[nodiscard]] std::string mdDuellKarteBild(int index);

}  // namespace bhed::menue

#endif  // BHED_MENUE_FEEDER_H
