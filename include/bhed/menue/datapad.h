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
// Jedi Outcast aus OpenJK codeJK2/cgame (MissionInformation_Draw, MissionPrint_Line,
// CG_DrawDataPadIconBackground, CG_DrawDataPadWeaponSelect, CG_DrawDataPadInventorySelect,
// CG_DrawDataPadForceSelect, weaponDesc, forcepowerDesc, force_icon_files), Movie Duels aus seinem
// code/cgame (dieselben Funktionen in der Fassung von Movie Duels: 25 Kraefte, kleinere Symbole, SPMOD_).
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
    // forcePowerLevel[FP_*] (0 = unbekannt): Sprung, Tempo, Stoss, Ziehen, Saebelwurf/-abwehr/-angriff, Sicht;
    // Movie Duels hat 25 Kraefte (FP_DESTRUCTION .. FP_BLAST dahinter)
    std::array<int, 25> kraft{0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1};
    int kraftWahl = 4;   // cg.DataPadforcepowerSelect (Index in showDataPadPowers des Spiels; JA 4 = FP_LEVITATION)
    // sess.mission_objectives: Name aus objectiveTable (OBJECTIVES_<name>), erfuellt?
    struct Ziel {
        std::string name;
        bool erfuellt = false;
    };
    std::vector<Ziel> ziele{{"T1_SOUR_OBJ1", true}, {"T1_SOUR_OBJ2", false}};
};

// M7: Beispiel je Spiel - die Ziele der ersten Karte (JA t1_sour, JK2 kejim_post) und Sprung als gewaehlte Kraft
[[nodiscard]] DatapadWerte datapadBeispiel(Spiel s);
// showDataPadPowers des Spiels: FP_* je Index und der Name aus forcepowerDesc (ohne FORCE_/_DESC)
[[nodiscard]] std::vector<int> datapadKraefte(Spiel s);
[[nodiscard]] std::vector<std::string> datapadKraftNamen(Spiel s);

struct DatapadKontext {
    Spiel spiel = Spiel::JA;                              // M7: Jedi Outcast und Movie Duels haben ihr eigenes Datapad
    std::function<std::string(const std::string&)> se;   // SP_GetStringTextString / SE_GetString (ohne "@")
    std::function<int(const std::string& text, float scale, const std::string& schrift)> breite;   // RE_Font_StrLenPixels
    std::function<int(float scale, const std::string& schrift)> hoehe;                             // RE_Font_HeightPixels
    Kodierung kodierung = Kodierung::Latin1;
    std::vector<std::string> waffenIcon;   // weaponData[wp].weaponIcon aus ext_data/weapons.dat
    std::string schrift4;                  // Movie Duels: die Schrift hinter Griff 4 (CG_DisplayBoxedText(..., 4, ...))
    std::function<bool(const std::string&)> bildDa;   // Movie Duels: R_RegisterShader != 0 (if (force_icons[...])); ohne: da
    DatapadWerte werte;
};

// force_icon_files[fp] der SP-Fassung (cg_main.cpp), "" ausserhalb
[[nodiscard]] std::string kraftSymbol(int fp);

// ext_data/weapons.dat: weaponIcon je weapontype (Index = WP_* des Spiels), wie WPN_Parse sie liest
[[nodiscard]] std::vector<std::string> waffenIcons(const std::string& weaponsDat, Spiel s = Spiel::JA);
// Die Waffen 1-13 des Spiels (WP_SABER .. WP_CONCUSSION, Jedi Outcast .. WP_STUN_BATON)
[[nodiscard]] std::vector<std::string> datapadWaffenNamen(Spiel s);

// CG_DRAW_DATAPAD_OBJECTIVES / _WEAPONS / _INVENTORY / _FORCEPOWERS (ohne den HUD-Teil, der kommt
// aus hud.h zeichneDatapadHud bzw. zeichneDatapadHudJk2). art = UI_DATAPAD_* (203-206).
void zeichneDatapad(int art, const DatapadKontext& k, std::vector<OdTeil>& out);

}  // namespace bhed::menue

#endif  // BHED_MENUE_DATAPAD_H
