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

#include <functional>
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
// M5: Woerter, die UI_RunMenuScript fuer ein uiScript liest (nach dem Namen), je Dialekt und Spiel
[[nodiscard]] int uiSkriptWortzahl(const std::string& name, Dialekt dl, Spiel sp);

// Eine Seite fuer die Reiter ueber der Arbeitsflaeche: ein menuDef oder eine
// ui_tab-artige Unterseite (Items mit cvarTest X und showCvar { wert }).
struct Seite {
    int menu = 0;
    std::string cvar;   // leer = das ganze Menue
    std::string wert;
    std::string titel;
};
[[nodiscard]] std::vector<Seite> seitenImMenue(const Datei& d);

// M2: Bewegung eines Items wie windowDef_t (rectClient, rectEffects,
// rectEffects2, offsetTime, nextTime) und die Deckkraft foreColor[3], die
// Fade() aendert.
struct ItemAnimation {
    Rechteck client, effects, effects2;   // menue-bezogen wie im Spiel
    int offsetTime = 0, nextTime = 0;
    float alpha = 1.0F;
    bool alphaGesetzt = false;
};

struct Laufzeit {
    const Datei* d = nullptr;
    std::vector<int> menuFlags;                     // WINDOW_VISIBLE / WINDOW_HASFOCUS je Menue
    std::vector<std::vector<ItemZustand>> items;    // je Menue, je Item
    std::vector<std::vector<Rechteck>> rects;       // window.rect je Item (setitemrect/transition)
    std::vector<std::vector<std::string>> texte;    // setitemtext ("" = wie gelesen)
    std::vector<std::vector<Farbe>> vorder;         // setitemcolor forecolor
    std::vector<std::vector<bool>> vorderGesetzt;
    std::vector<std::vector<Farbe>> hinten, rand;   // M4: setitemcolor backcolor / bordercolor, Fade von backColor[3]
    std::vector<std::vector<bool>> hintenGesetzt, randGesetzt;
    std::vector<std::string> hintergrund;           // setbackground je Menue
    std::map<std::string, std::string> cvars;       // nur die Vorschau (Schluessel klein)
    std::vector<int> stapel;                        // menuStack
    std::vector<std::string> protokoll;             // was ohne Spiel nur gemeldet wird
    // M2 Punkt 3: "open X" fuer ein menuDef, das nicht in dieser Datei steht -
    // die Oberflaeche sucht es in den anderen Dateien des Menuesatzes (menus.txt)
    std::string fremdesMenue;
    // M2: was die UI-Skripte an Modellen umstellen (UI_UpdateCharacter, UI_UpdateCharacterSkin,
    // UI_UpdateSaberHilt) - je Menue, je Item; "" = wie gelesen. anwenden() schreibt sie in die Kopie.
    std::vector<std::vector<std::string>> modellPfad, modellHaut, modellAnim;
    // M3 Punkt 2: DC->realTime (jetztMs), zu der UI_UpdateCharacter die Animation setzte
    // (ItemParse_asset_model_go -> g2hilev_SetAnim); -1 = beim Laden der Menues gesetzt
    std::vector<std::vector<int>> animGesetzt;
    // UI_SaberModelForSaber / UI_SaberSkinForSaber (sabers.cfg/.sab): false = kein Modell
    std::function<bool(const std::string& saber, std::string& modell, std::string& haut)> saberModell;
    // M2 Punkt 5: Klaenge wie die Engine sie ausloest - die Oberflaeche spielt sie ab
    // (im Selbsttest stumm) und leert die Liste. "musik:<datei>" = startBackgroundTrack.
    std::vector<std::string> klaenge;
    std::string fokusKlangStd;    // assetGlobalDef itemFocusSound (DC->Assets.itemFocusSound; menuEnter/ExitSound spielt die Engine nicht)
    int maxProtokoll = 200;
    int version = 0;                                // zaehlt jede Aenderung
    // M2: Animationen (transition, orbit, fadein/fadeout) im Takt der Engine
    std::vector<std::vector<ItemAnimation>> anim;   // je Menue, je Item
    int spielFps = 125;                             // com_maxfps: ein Item_Paint je Bild
    int jetztMs = 0;                                // DC->realTime des letzten Takts
    int letzterTakt = -1;
    // Menu_Init: fadeClamp/fadeAmount/fadeCycle aus assetGlobalDef, wenn das Menue sie nicht setzt
    float fadeClampStd = 1.0F, fadeAmountStd = 0.0F;
    int fadeCycleStd = 0;
    // Die Zeit bis jetztMs in Bildern zu 1000 / spielFps ms abarbeiten (Item_Paint:
    // WINDOW_ORBITING, WINDOW_INTRANSITION, Item_TextColor: Fade). Gibt die Zahl der Bilder zurueck.
    int takt(int jetzt);
    [[nodiscard]] bool animiert() const;            // laeuft noch etwas?
    void positionAus(int menu, std::size_t item);   // Item_UpdatePosition
    // UI_RunMenuScript fuer die Figuren- und Saeber-Skripte; false = nicht ausgefuehrt
    bool uiSkript(int menu, const std::string& name);
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
    // M4: Mausrad (schritte > 0 = rauf) ueber einer Textscroll mit Fokus (nur Movie Duels)
    void rad(int menu, float x, float y, int schritte);
    void escape(int menu);   // onESC
    // Umgebung fuer zeichneMenu mit den Laufzeit-Flags dieses Menues.
    void fuelle(int menu, Umgebung& u) const;
    // Schreibt Rechtecke, Texte, Farben und Hintergruende in eine Kopie der Datei.
    void anwenden(Datei& kopie) const;
    // Das Item unter (x, y), das den Fokus nehmen kann (-1 = keins).
    [[nodiscard]] int itemBei(int menu, float x, float y, const Umgebung& u) const;
    // M6 Punkt 2: die Items in der Reihenfolge, in der Tab/Pfeil runter sie fokussiert -
    // Menu_SetNextCursorItem ab cursorItem -1 (Menu_Init) mit den Pruefungen von Item_SetFocus
    // (keine Dekoration, sichtbar, SP nicht WINDOW_INACTIVE, enableCvar/showCvar), bis die Folge
    // sich wiederholt. Ab -1 beginnt die Engine bei Index 1; Index 0 kommt erst nach dem Umlauf.
    [[nodiscard]] std::vector<int> tabReihenfolge(int menu) const;
};

}  // namespace bhed::menue

#endif  // BHED_MENUE_SKRIPT_H
