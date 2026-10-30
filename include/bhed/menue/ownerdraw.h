// ownerdraw.h - Menue-Editor M7 Punkt 14: was die Engine fuer ein ownerdraw-Item zeichnet
// (DC->ownerDrawItem = UI_OwnerDraw), je Nummer und Fassung, als Liste von Zeichenbefehlen.
// Was im Spiel aus dem laufenden Spiel oder vom Server kommt, nimmt Beispielwerte
// (Spielbeispiel); das Ergebnis sagt dann "beispiel", die Oberflaeche beschriftet es.
//
// Portiert aus OpenJK: codemp/ui/ui_main.c (UI_OwnerDraw, UI_DrawSkinColor, UI_DrawForceSide,
// UI_DrawJediNonJedi, UI_TrueJediEnabled, UI_DrawGenericNum, UI_DrawForceMastery,
// UI_DrawGameType, UI_DrawNetGameType, UI_DrawJoinGameType, UI_DrawSkill, UI_DrawTeamMember,
// UI_DrawNetSource, GetNetSourceString, GetCRDelineatedString, UI_DrawNetFilter,
// UI_FilterDescription, UI_DrawMapCinematic, UI_DrawMapPreview, UI_DrawAllMapsSelection,
// UI_DrawBotName, UI_DrawBotSkill, UI_DrawRedBlue, UI_DrawSelectedPlayer,
// UI_DrawServerRefreshDate, UI_DrawServerMOTD, UI_DrawKeyBindStatus, UI_DrawGLInfo),
// codemp/ui/ui_force.c (UI_UpdateForcePowers,
// UpdateForceUsed, UI_DrawForceStars, uiForcePowerDarkLight, UI_InitForceShaders),
// codemp/game/bg_misc.c (forceMasteryLevels, forceMasteryPoints, bgForcePowerCost),
// codemp/rd-vanilla/tr_init.cpp (TruncateGLExtensionsString), code/ui/ui_main.cpp
// (UI_OwnerDraw, UI_DrawEffects, UI_DrawGLInfo, UI_DrawKeyBindStatus) und aus Movie Duels
// code/ui/ui_main.cpp (UI_DUELMAPS_SELECTION).
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
#ifndef BHED_MENUE_OWNERDRAW_H
#define BHED_MENUE_OWNERDRAW_H

#include "bhed/menue/menue.h"

#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::menue {

struct FeederDaten;

// Ein Zeichenbefehl: Text_Paint / R_Font_DrawString, UI_DrawHandlePic, UI_FillRect
struct OdTeil {
    enum class Art { Text, Bild, Fuellen };
    Art art = Art::Text;
    float x = 0, y = 0, w = 0, h = 0;
    std::string inhalt;    // Text (schon uebersetzt) bzw. Shader
    Farbe farbe{1, 1, 1, 1};
    float scale = 1;
    int stil = 0;          // ITEM_TEXTSTYLE_*
    int maxPixel = 0;      // limit (0 = ohne)
    int font = 0;          // item->font bzw. iMenuFont
    std::string schrift;   // cgame-Schrift beim Namen ("ergoec"); leer = font
    float s0 = 0, t0 = 0, s1 = 1, t1 = 1;
};

// DC->ownerDrawItem(x, y, w, h, text_x, text_y, ownerDraw, ownerDrawFlags, align, special,
// scale, color, shader, textStyle, font)
struct OdAufruf {
    float x = 0, y = 0, w = 0, h = 0, textX = 0, textY = 0;
    int id = 0;
    int flags = 0;
    int align = 0;
    float special = 0;
    float scale = 1;
    Farbe farbe{1, 1, 1, 1};
    std::string shader;   // item->window.background
    int stil = 0;
    int font = 0;
};

// uiInfo.uiDC.glconfig: vendor/renderer/version/extensions des Treibers (glGetString) und die
// Bits des Pixelformats. echt = vom Treiber dieses Rechners gelesen, sonst Beispiel.
struct GlDaten {
    std::string vendor, renderer, version, extensions;
    int colorBits = 24, depthBits = 24, stencilBits = 8;   // OpenJK GLimp_SetMode: r_colorbits 0 -> 24
    bool echt = false;
};
// Beispiel, solange der Treiber nicht gelesen werden kann (beschriftet)
[[nodiscard]] GlDaten glBeispiel();

// Was im Spiel vom Server bzw. aus der laufenden Partie kommt (CS_SERVERINFO, cg_*);
// im Hauptmenue ist CS_SERVERINFO leer - das sind die Vorgaben.
struct Spielbeispiel {
    bool forceBasedTeams = false;   // g_forceBasedTeams
    bool jediVmerc = false;         // g_jediVmerc (UI_TrueJediEnabled)
    int maxRank = 7;                // uiMaxRank (MAX_FORCE_RANK; im Spiel ui_rankChange)
    bool serverRefresh = false;     // uiInfo.serverStatus.refreshActive
    int serverAnzahl = 0;           // trap->LAN_GetServerCount
    std::string motd;               // uiInfo.serverStatus.motd (Jedi Outcast MP)
};

struct OdKontext {
    Dialekt dialekt = Dialekt::SP;
    Spiel spiel = Spiel::JA;
    const FeederDaten* daten = nullptr;
    std::function<std::string(const std::string&)> cvar;   // DC->getCVarString ("" = unbekannt)
    std::function<std::string(const std::string&)> se;     // SE_GetString(KEY) ohne "@"
    // Text_Width / RE_Font_StrLenPixels und Text_Height / RE_Font_HeightPixels
    std::function<int(const std::string& text, float scale, int font, const std::string& schrift)> breite;
    std::function<int(float scale, int font, const std::string& schrift)> hoehe;
    std::function<bool(const std::string& bild)> bildDa;   // R_RegisterShaderNoMip != 0
    int zeitMs = 0;                                        // DC->realTime
    const Spielbeispiel* beispiel = nullptr;
    const GlDaten* gl = nullptr;
};

struct OdErgebnis {
    bool bekannt = false;    // die Engine hat einen Fall dafuer (auch "zeichnet nichts")
    bool beispiel = false;   // zeigt Beispielwerte statt Werten aus dem laufenden Spiel
    std::vector<OdTeil> teile;
    // Menu_ShowItemByName(Menus_FindByName(menue), item, zeigen), das UI_DrawForceSide beim Zeichnen tut
    struct Zeigen {
        std::string menue, item;
        bool zeigen = false;
    };
    std::vector<Zeigen> zeigen;
};

[[nodiscard]] OdErgebnis ownerDraw(const OdAufruf& a, const OdKontext& k);

// MP-Kraftwerte wie UI_UpdateForcePowers (aus der cvar "forcepowers") + UpdateForceUsed
struct MpKraft {
    int seite = 1;   // uiForceSide
    int rang = 7;    // uiForceRank
    int verfuegbar = 0;   // uiForceAvailable
    int jediNonJedi = -1;
    std::array<int, 18> raenge{};   // uiForcePowersRank[NUM_FORCE_POWERS]
};
[[nodiscard]] MpKraft mpKraft(const std::string& forcepowers, bool freeSaber, const Spielbeispiel& b);

}  // namespace bhed::menue

#endif  // BHED_MENUE_OWNERDRAW_H
