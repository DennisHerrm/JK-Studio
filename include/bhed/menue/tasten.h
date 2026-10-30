// tasten.h - Menue-Editor M7: Tastenbelegung wie im Spiel fuer ITEM_TYPE_BIND (Item_Bind_Paint zeigt
// die Tasten eines Befehls statt "???"). Die Bindungen kommen aus der Vorgabe-Config des Spiels
// (default.cfg, mpdefault.cfg, MovieDuels-SP-default.cfg ...), wie Com_Init sie ausfuehrt.
//
// Portiert aus OpenJK: code/client/cl_keys.cpp (keynames, Key_StringToKeynum, Key_KeynumToString,
// Key_KeyToAscii, Key_KeyToHex, Key_SetBinding, Key_Bind_f, Key_Unbindall_f), code/client/cl_ui.cpp
// (Key_KeynumToStringBuf mit KEYNAMES_KEYNAME_*), code/qcommon/cmd.cpp (Cbuf/Cmd_TokenizeString:
// Befehle durch ";" und Zeilen, Anfuehrungszeichen, //-Kommentare), code/ui/ui_shared.cpp und
// codemp/ui/ui_shared.c (g_bindCommands, Controls_GetKeyAssignment, BindingFromName).
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
#ifndef BHED_MENUE_TASTEN_H
#define BHED_MENUE_TASTEN_H

#include "bhed/menue/menue.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

// kg.keys[].binding
struct Tastenbelegung {
    std::vector<std::string> bindung;   // je Tastennummer (MAX_KEYS)
    Tastenbelegung();
};

// Die Config des Spiels ausfuehren: bind, unbind, unbindall, exec (ueber lader); der Rest zaehlt hier nicht
void configAusfuehren(const std::string& text, Tastenbelegung& b, const std::function<bool(const std::string&, std::string&)>& lader, int tiefe = 0);
// Welche Datei Com_Init ausfuehrt (code/qcommon/common.cpp; Movie Duels eigene Namen)
[[nodiscard]] std::string vorgabeConfig(Dialekt d, Spiel s);

// Key_StringToKeynum / Key_KeynumToString
[[nodiscard]] int tastenNummer(const std::string& name);
[[nodiscard]] std::string tastenName(int nummer);

// BindingFromName(befehl): "???" wenn der Befehl nicht in g_bindCommands steht oder keine Taste hat,
// sonst "<Taste1>" bzw. "<Taste1> <oder> <Taste2>". se = SE_GetString (KEYNAMES_KEYNAME_*, MENUS_KEYBIND_OR).
[[nodiscard]] std::string bindungsText(const Tastenbelegung& b, const std::string& befehl, Dialekt d, Spiel s,
                                       const std::function<std::string(const std::string&)>& se);

}  // namespace bhed::menue

#endif  // BHED_MENUE_TASTEN_H
