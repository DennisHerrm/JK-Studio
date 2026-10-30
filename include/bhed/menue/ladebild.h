// ladebild.h - Menue-Editor M7 Punkt 11: Lade- und Verbindungsmenues mit Beispielwerten (wie das HUD).
// Der Ladebildschirm des Einzelspielers ist das Menue "loadscreen", das cgame beim Laden einer Karte
// zeichnet (CG_DrawInformation) - mit Levelshot, Waffen- und Kraftsymbolen aus dem Spielstand, dem
// Ladebalken und dem Kartennamen. Im Mehrspieler zeichnet die Oberflaeche beim Verbinden das Menue
// "Connect" und darueber Karte, Server und Zustand (UI_DrawConnectScreen).
//
// Portiert aus OpenJK code/cgame/cg_info.cpp (CG_DrawInformation, CG_DrawLoadingScreen, CG_LoadBar,
// CG_DrawLoadWeapons, CG_DrawLoadWeaponsPrintRow, CG_DrawLoadForcePowers, CG_DrawLoadForcePrintRow,
// CG_ForcePower_Valid, CG_GetLoadScreenInfo), code/cgame/cg_main.cpp (showPowers, levelLoad,
// loadTick, loadTickCap) und codemp/ui/ui_main.c (UI_DrawConnectScreen, Text_PaintCenter).
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
#ifndef BHED_MENUE_LADEBILD_H
#define BHED_MENUE_LADEBILD_H

#include "bhed/menue/datapad.h"
#include "bhed/menue/vorschau.h"

#include <string>
#include <vector>

namespace bhed::menue {

// Was cgame beim Laden aus dem Spielstand und vom Server nimmt
struct LadeWerte {
    std::string karte = "t1_sour";   // Info_ValueForKey(CS_SERVERINFO, "mapname")
    std::string nachricht;           // CS_MESSAGE (langer Kartenname, "@" = StringEd)
    int stufe = 5;                   // cg.loadLCARSStage (0..9 Striche)
    bool ersteKarte = false;         // yavin1 ohne geladenen Spielstand: schwarz + SP_INGAME_ALONGTIME
};

// Was UI_DrawConnectScreen vom Client nimmt (uiClientState_t, CS_SERVERINFO)
struct VerbindungsWerte {
    std::string server = "192.168.0.10";   // cstate.servername ("localhost" = eigenes Spiel)
    std::string karte = "mp/ffa1";         // mapname
    int zustand = 2;                       // 1 CA_CONNECTING, 2 CA_CHALLENGING, 3 CA_CONNECTED
    int pakete = 1;                        // cstate.connectPacketCount
    std::string meldung;                   // cstate.messageString
    std::string motd;                      // Info_ValueForKey(updateInfoString, "motd")
};

[[nodiscard]] bool istLadebild(const Datei& d);     // SP mit Menue "loadscreen"
[[nodiscard]] bool istVerbindung(const Datei& d);   // MP mit Menue "Connect"

struct LadeKontext {
    std::function<std::string(const std::string&)> se;   // SE_GetString ohne "@"
    std::function<bool(const std::string&)> bildDa;      // R_RegisterShaderNoMip != 0
    std::vector<std::string> waffenIcon;                  // weaponData[].weaponIcon (weapons.dat)
    DatapadWerte spieler;                                 // Waffen und Kraefte aus playersave/playerfplvl
};

// CG_DrawInformation: das Menue "loadscreen" mit forcePaint und was cgame darueber legt; aktuell = das
// bearbeitete Menue (die uebrigen Zeichenbefehle tragen item -1). Setzt ui_missionbriefing in u.cvar.
void zeichneLadebild(const Datei& d, int aktuell, const Umgebung& u, const LadeWerte& w, const LadeKontext& k, std::vector<Prim>& out);
// UI_DrawConnectScreen(qfalse)
void zeichneVerbindung(const Datei& d, int aktuell, const Umgebung& u, const VerbindungsWerte& w,
                       const std::function<std::string(const std::string&)>& se, std::vector<Prim>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_LADEBILD_H
