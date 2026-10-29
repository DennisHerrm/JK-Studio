// skript.h - Menue-Editor M1 Punkt 6: die Menues bedienen wie im Spiel -
// Skripte (open/close/show/hide/setcvar/setfocus/transition ...), Fokus,
// Klicks auf Knoepfe, Ja/Nein, Mehrfachwahl, Schieber; alle Cvars nur in der
// Vorschau, nie in der Spielkonfiguration.
//
// Portiert aus OpenJK: codemp/ui/ui_shared.c (Item_RunScript, commandList
// mit Script_Show/Hide/Open/Close/SetFocus/SetCvar/SetItemColor/
// SetItemText/SetBackground/SetItemRect/Transition, Menu_ItemsMatchingGroup,
// Menu_ShowItemByName, Menus_ActivateByName, Menus_CloseByName,
// Menu_RunCloseScript, Item_SetFocus, Item_Action, Item_YesNo_HandleKey,
// Item_Multi_HandleKey, Item_Slider_HandleKey, Item_EnableShowViaCvar).
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
#ifndef BHED_MENUE_SKRIPT_H
#define BHED_MENUE_SKRIPT_H

#include "bhed/menue/menue.h"
#include "bhed/menue/vorschau.h"

#include <map>
#include <string>
#include <vector>

namespace bhed::menue {

// Ein Cvar, den das Menue liest oder setzt, mit den Werten, die vorkommen.
struct CvarFund {
    std::string name;
    std::vector<std::string> werte;    // aus showCvar/hideCvar, cvarStrList, setcvar ...
    std::vector<std::string> wofuer;   // "cvarTest (item x)", "setcvar (onOpen menu y)" ...
    bool sichtbarkeit = false;         // steuert Sichtbarkeit/Zustand (cvarTest)
};
[[nodiscard]] std::vector<CvarFund> cvarsImMenue(const Datei& d);

// Eine Seite fuer die Reiter ueber der Arbeitsflaeche: ein menuDef oder eine
// ui_tab-artige Unterseite (Items mit cvarTest X und showCvar { wert }).
struct Seite {
    int menu = 0;
    std::string cvar;   // leer = das ganze Menue
    std::string wert;
    std::string titel;
};
[[nodiscard]] std::vector<Seite> seitenImMenue(const Datei& d);

struct Laufzeit {
    const Datei* d = nullptr;
    std::vector<int> menuFlags;                     // WINDOW_VISIBLE / WINDOW_HASFOCUS je Menue
    std::vector<std::vector<ItemZustand>> items;    // je Menue, je Item
    std::vector<std::vector<Rechteck>> rects;       // window.rect je Item (setitemrect/transition)
    std::vector<std::vector<std::string>> texte;    // setitemtext ("" = wie gelesen)
    std::vector<std::vector<Farbe>> vorder;         // setitemcolor forecolor
    std::vector<std::vector<bool>> vorderGesetzt;
    std::vector<std::string> hintergrund;           // setbackground je Menue
    std::map<std::string, std::string> cvars;       // nur die Vorschau (Schluessel klein)
    std::vector<int> stapel;                        // menuStack
    std::vector<std::string> protokoll;             // was ohne Spiel nur gemeldet wird
    int maxProtokoll = 200;
    int version = 0;                                // zaehlt jede Aenderung
    Umgebung basis;                                 // Schriften/Texte (textRechteck beim Klick)

    void start(const Datei& datei);
    [[nodiscard]] int finde(const std::string& name) const;   // Menus_FindByName
    void oeffne(const std::string& name);                     // Menus_OpenByName
    void schliesse(const std::string& name);                  // Menus_CloseByName ("all" = alle)
    void fuehreAus(int menu, int item, const std::string& skript);   // Item_RunScript
    [[nodiscard]] std::string cvar(const std::string& n) const;
    void setzeCvar(const std::string& n, const std::string& w);
    // Maus ueber (x, y) im virtuellen Bildschirm: Fokus und mouseEnter/Exit.
    void maus(int menu, float x, float y);
    // Klick (Taste 1) auf (x, y): Item_HandleKey / Item_Action.
    void klick(int menu, float x, float y);
    void escape(int menu);   // onESC
    // Umgebung fuer zeichneMenu mit den Laufzeit-Flags dieses Menues.
    void fuelle(int menu, Umgebung& u) const;
    // Schreibt Rechtecke, Texte, Farben und Hintergruende in eine Kopie der Datei.
    void anwenden(Datei& kopie) const;
    // Das Item unter (x, y), das den Fokus nehmen kann (-1 = keins).
    [[nodiscard]] int itemBei(int menu, float x, float y, const Umgebung& u) const;
};

}  // namespace bhed::menue

#endif  // BHED_MENUE_SKRIPT_H
