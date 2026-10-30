// hud.h - Menue-Editor M3 Punkt 5: der HUD-Editor. Die HUD-Menues (ui/hud.menu, von
// cg_hudFiles bzw. ui/jahud.txt geladen) zeichnet die Engine nicht allein ueber Menu_Paint:
// cgame liest die Lage und Farbe benannter Items (cgi_UI_GetMenuItemInfo) und zeichnet
// darueber Striche, Zahlen und Bilder mit den Werten des Spielers. Das hier ist diese
// Darstellung 1:1 mit Beispielwerten.
//
// Portiert aus OpenJK code/cgame/cg_draw.cpp (CG_DrawHUD, CG_DrawHealth, CG_DrawArmor,
// CG_DrawForcePower, CG_DrawSaberStyle, CG_DrawAmmo, CG_DrawCrosshair),
// code/cgame/cg_drawtools.cpp (CG_DrawNumField), code/cgame/cg_main.cpp (forceTics,
// ammoTics, armorTics, healthTics, otherHUDBits, sb_t_nums, crosshairShader),
// code/client/cl_cgame.cpp (CG_UI_GETITEMINFO) und shared/qcommon/q_color.c (colorTable).
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
#ifndef BHED_MENUE_HUD_H
#define BHED_MENUE_HUD_H

#include "bhed/menue/vorschau.h"

#include <string>
#include <vector>

namespace bhed::menue {

// Die Werte, die cgame aus playerState/cvars nimmt - im Editor Schieberegler.
struct HudWerte {
    int gesundheit = 100;      // ps.stats[STAT_HEALTH]
    int gesundheitMax = 100;   // ps.stats[STAT_MAX_HEALTH]
    int ruestung = 100;        // ps.stats[STAT_ARMOR]
    int macht = 100;           // ps.forcePower
    int machtMax = 100;        // ps.forcePowerMax
    bool machtBekannt = true;  // ps.forcePowersKnown != 0
    bool saebel = true;        // cent->currentState.weapon == WP_SABER (sonst Munition)
    int saebelStil = 2;        // cg.saberAnimLevelPending: 1 SS_FAST, 2 SS_MEDIUM, 3 SS_STRONG,
                               // 4 SS_DESANN, 5 SS_TAVION, 6 SS_DUAL, 7 SS_STAFF
    int munition = 100;        // ps.ammo[weaponData[weapon].ammoIndex]
    int munitionMax = 300;     // ammoData[ammoIndex].max (AMMO_BLASTER 300)
    int fadenkreuz = 1;        // cg_drawCrosshair (0 = aus)
    float fadenkreuzGroesse = 24;   // cg_crosshairSize
    int fadenkreuzX = 0, fadenkreuzY = 0;   // cg_crosshairX/Y
    int zeitMs = 0;            // cg.time (Blinken der letzten Ruestung)
};

// M7: cgi_UI_GetMenuItemInfo (CG_UI_GETITEMINFO): Lage als int, foreColor, background; gefunden = false:
// alles 0 (die statische Vorbelegung)
struct MenueItemInfo {
    int x = 0, y = 0, w = 0, h = 0;
    Farbe farbe{0, 0, 0, 0};
    std::string bild;
    bool gefunden = false;
};
[[nodiscard]] MenueItemInfo menueItemInfo(const Datei& d, const char* menue, const char* item);

// Ist das ein HUD-Menue (hat es lefthud oder righthud)?
[[nodiscard]] bool istHud(const Datei& d);
// Bildet die Vorschau nach, was cgame darueber zeichnet? Nur Jedi Academy SP (code/cgame) -
// das MP-cgame (codemp: ganzzahlige Schritte, Striche weiss) und Movie Duels (eigene
// HUD-Arten, Blockpunkte, Toenung) zeichnen anders; dann nur die Menues (Menu_Paint).
[[nodiscard]] bool hudNachgebildet(const Datei& d);

// cg.HUDArmorFlag / cg.HUDTickFlashTime: das Blinken der letzten Ruestung unter einem
// Viertel (die Engine schaltet es nach dem Zeichnen um, also ueber die Bilder hinweg).
struct HudZustand {
    bool armorFlag = true;
    int tickFlashTime = 0;
};

// CG_DrawHUD (SP, cg_hudFiles 0) in der Reihenfolge der Engine: lefthud (Menu_Paint mit
// forcePaint), dessen frame, Ruestung, Gesundheit; righthud, frame, Macht, Saebelstil bzw.
// Munition; danach CG_DrawCrosshair (ohne Ziel: weiss, Mitte + cg_crosshairX/Y).
// Zeichenbefehle der Menues ausser `aktuell` tragen item -1 (nicht waehlbar).
void zeichneHud(const Datei& d, int aktuell, const Umgebung& u, const HudWerte& w, HudZustand& z, std::vector<Prim>& out);

// M7: CG_DrawDataPadHUD (code/cgame/cg_draw.cpp, Jedi Academy): CG_DrawHealth, CG_DrawForcePower,
// CG_DrawAmmo mit den Strichen aus lefthud/righthud der HUD-Datei (ui/hud.menu), CG_DrawMessageLit
// (526 + 33, 286 + 41, gfx/hud/message_off) und gfx/menus/datapad ueber den ganzen Schirm.
void zeichneDatapadHud(const Datei& hud, const HudWerte& w, std::vector<Prim>& out);

// M7: CG_DrawDataPadHUD von Jedi Outcast (codeJK2/cgame/cg_draw.cpp: CG_DrawHUDLeftFrame1, CG_DrawArmor,
// CG_DrawHealth, CG_DrawHUDRightFrame1, CG_DrawForcePower, CG_DrawAmmo, CG_DrawMessageLit) und Movie Duels
// (dazu CG_DrawHUDLeftFrame2/RightFrame2, eigene Bilder gfx/hud/Datapad*, sieben Saebelstile) - ohne
// hud.menu: Ringe, Striche (forceTicPos/ammoTicPos, rechts gespiegelt) und Zahlen an festen Stellen,
// darueber gfx/hud/datapad2.
void zeichneDatapadHudJk2(Spiel spiel, const HudWerte& w, std::vector<Prim>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_HUD_H
