// datapad.h - Menue-Editor M7 Punkt 14: die Datapad-Ownerdraws des Einzelspielers (UI_DATAPAD_*
// 203-206). Die Engine reicht sie an cgame weiter (ui.Draw_DataPad -> CG_DRAW_DATAPAD_*); dort
// zeichnet sie Ziele, Waffen, Inventar und Kraefte des Spielers. Ohne laufendes Spiel: Beispielwerte.
//
// Portiert aus OpenJK code/cgame: cg_main.cpp (vmMain CG_DRAW_DATAPAD_*, showDataPadPowers,
// ForcePowerDataPad_Valid, CG_DrawDataPadForceSelect, forcepowerDesc/Lvl1-3Desc, force_icon_files,
// CG_MagicFontToReal, CG_DrawDataPadInventorySelect), cg_info.cpp (CG_DrawDataPadObjectives,
// ObjectivePrint_Line), cg_weapons.cpp (CG_DrawDataPadWeaponSelect, weaponDesc, CG_RegisterWeapon:
// weaponIcon / "_na"), cg_text.cpp (CG_DisplayBoxedText), cg_drawtools.cpp
// (CG_DrawProportionalString) und shared/qcommon/q_color.c (colorTable).
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
#ifndef BHED_MENUE_DATAPAD_H
#define BHED_MENUE_DATAPAD_H

#include "bhed/menue/kodierung.h"
#include "bhed/menue/ownerdraw.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr int kDpObjectives = 203, kDpWeapons = 204, kDpInventory = 205, kDpForcePowers = 206;   // UI_DATAPAD_*

// Was cgame aus playerState/Session nimmt - im Editor Beispielwerte
struct DatapadWerte {
    unsigned waffen = (1U << 1) | (1U << 2) | (1U << 3) | (1U << 10);   // ps.stats[STAT_WEAPONS]: Saebel, Pistole, Gewehr, Thermal
    int waffe = 1;                                                       // cg.DataPadWeaponSelect (WP_SABER)
    // forcePowerLevel[FP_*] der SP-Fassung (0 = unbekannt): Sprung, Tempo, Stoss, Ziehen, Saebelwurf/-abwehr/-angriff, Sicht
    std::array<int, 16> kraft{0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1};
    int kraftWahl = 4;                                                   // cg.DataPadforcepowerSelect (showDataPadPowers-Index, FP_LEVITATION)
    // sess.mission_objectives: Name aus objectiveTable (OBJECTIVES_<name>), erfuellt?
    struct Ziel {
        std::string name;
        bool erfuellt = false;
    };
    std::vector<Ziel> ziele{{"T1_SOUR_OBJ1", true}, {"T1_SOUR_OBJ2", false}};
};

struct DatapadKontext {
    std::function<std::string(const std::string&)> se;   // SP_GetStringTextString / SE_GetString (ohne "@")
    std::function<int(const std::string& text, float scale, const std::string& schrift)> breite;   // RE_Font_StrLenPixels
    std::function<int(float scale, const std::string& schrift)> hoehe;                             // RE_Font_HeightPixels
    Kodierung kodierung = Kodierung::Latin1;
    std::vector<std::string> waffenIcon;   // weaponData[wp].weaponIcon aus ext_data/weapons.dat
    DatapadWerte werte;
};

// ext_data/weapons.dat: weaponIcon je weapontype (Index = WP_*), wie WPN_Parse sie liest
[[nodiscard]] std::vector<std::string> waffenIcons(const std::string& weaponsDat);

// CG_DRAW_DATAPAD_OBJECTIVES / _WEAPONS / _INVENTORY / _FORCEPOWERS (ohne den HUD-Teil, der kommt
// aus hud.h zeichneDatapadHud). art = UI_DATAPAD_* (203-206).
void zeichneDatapad(int art, const DatapadKontext& k, std::vector<OdTeil>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_DATAPAD_H
