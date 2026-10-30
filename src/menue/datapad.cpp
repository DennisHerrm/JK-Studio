// datapad.cpp - Menue-Editor M7 Punkt 14: Datapad-Ownerdraws des Einzelspielers (Herkunft und
// Lizenz im Kopf von include/bhed/menue/datapad.h).
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
#include "bhed/menue/datapad.h"

#include "bhed/menue/vorschau.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>

namespace bhed::menue {

namespace {

constexpr Farbe kWeiss{1, 1, 1, 1};
constexpr Farbe kTitel{0.549F, 0.854F, 1.0F, 1.0F};   // CT_TITLE
constexpr Farbe kBlau3{0.199F, 0.71F, 0.92F, 1};      // CT_BLUE3
constexpr Farbe kHellblau1{0.367F, 0.261F, 0.722F, 1};   // CT_LTBLUE1
constexpr Farbe kIconBlau{0.567F, 0.685F, 1.0F, 0.75F};  // CT_ICON_BLUE
constexpr const char* kMittel = "ergoec";            // cgs.media.qhFontMedium
constexpr const char* kArialnb = "arialnb";          // CG_MagicFontToReal(4)
constexpr int kMaxDpPowers = 16;                     // MAX_DPSHOWPOWERS
constexpr int WP_SABER = 1, WP_FLECHETTE = 8, WP_ROCKET_LAUNCHER = 9, WP_CONCUSSION = 13;

// cg_main.cpp showDataPadPowers (FP_* der Einzelspieler-Fassung: HEAL 0, LEVITATION 1, SPEED 2, PUSH 3,
// PULL 4, TELEPATHY 5, GRIP 6, LIGHTNING 7, SABERTHROW 8, SABER_DEFENSE 9, SABER_OFFENSE 10, RAGE 11,
// PROTECT 12, ABSORB 13, DRAIN 14, SEE 15)
constexpr int kShowDataPadPowers[kMaxDpPowers] = {13, 0, 12, 5, 1, 2, 3, 4, 8, 9, 10, 15, 14, 7, 11, 6};
// force_icon_files[FP_*]
constexpr const char* kForceIcons[16] = {"gfx/mp/f_icon_lt_heal",      "gfx/mp/f_icon_levitation",   "gfx/mp/f_icon_speed",
                                         "gfx/mp/f_icon_push",         "gfx/mp/f_icon_pull",         "gfx/mp/f_icon_lt_telepathy",
                                         "gfx/mp/f_icon_dk_grip",      "gfx/mp/f_icon_dk_l1",        "gfx/mp/f_icon_saber_throw",
                                         "gfx/mp/f_icon_saber_defend", "gfx/mp/f_icon_saber_attack", "gfx/mp/f_icon_dk_rage",
                                         "gfx/mp/f_icon_lt_protect",   "gfx/mp/f_icon_lt_absorb",    "gfx/mp/f_icon_dk_drain",
                                         "gfx/mp/f_icon_sight"};
// forcepowerDesc und Stufen (Index = showDataPadPowers-Index)
constexpr const char* kKraftName[kMaxDpPowers] = {"ABSORB", "HEAL",        "PROTECT",       "MIND_TRICK",    "JUMP",  "SPEED",     "PUSH", "PULL",
                                                  "SABER_THROW", "SABER_DEFENSE", "SABER_OFFENSE", "SENSE", "DRAIN", "LIGHTNING", "RAGE", "GRIP"};
// cg_weapons.cpp weaponDesc[13]
constexpr const char* kWaffenText[13] = {"SABER_DESC",         "NEW_BLASTER_PISTOL_DESC", "BLASTER_RIFLE_DESC", "DISRUPTOR_RIFLE_DESC", "BOWCASTER_DESC",
                                         "HEAVYREPEATER_DESC", "DEMP2_DESC",              "FLECHETTE_DESC",     "MERR_SONN_DESC",       "THERMAL_DETONATOR_DESC",
                                         "TRIP_MINE_DESC",     "DET_PACK_DESC",           "CONCUSSION_DESC"};

// --- Jedi Outcast (codeJK2/cgame) ---
// showDataPadPowers = FP_HEAL .. FP_SABER_OFFENSE (MAX_DPSHOWPOWERS 11), force_icon_files[FP_*]
constexpr int kJk2Kraefte = 11;
constexpr const char* kJk2ForceIcons[kJk2Kraefte] = {"gfx/hud/f_icon_heal",        "gfx/hud/f_icon_levitation",   "gfx/hud/f_icon_speed",
                                                     "gfx/hud/f_icon_push",        "gfx/hud/f_icon_pull",         "gfx/hud/f_icon_telepathy",
                                                     "gfx/hud/f_icon_grip",        "gfx/hud/f_icon_l1",           "gfx/hud/f_icon_saber_throw",
                                                     "gfx/hud/f_icon_saber_defend", "gfx/hud/f_icon_saber_attack"};
// forcepowerDesc / forcepowerLvlNDesc: FORCE_<name>_DESC, FORCE_<name>_LVL<n>_DESC
constexpr const char* kJk2KraftName[kJk2Kraefte] = {"HEAL", "JUMP", "SPEED", "PUSH", "PULL", "MIND_TRICK", "GRIP", "LIGHTNING", "SABER_THROW",
                                                    "SABER_DEFENSE", "SABER_OFFENSE"};
// cg_weapons.cpp weaponDesc[13] (WP_SABER .. WP_STUN_BATON)
constexpr const char* kJk2WaffenText[13] = {"SABER_DESC",         "BLASTER_PISTOL_DESC", "BLASTER_RIFLE_DESC", "DISRUPTOR_RIFLE_DESC", "BOWCASTER_DESC",
                                            "HEAVYREPEATER_DESC", "DEMP2_DESC",          "FLECHETTE_DESC",     "MERR_SONN_DESC",       "THERMAL_DETONATOR_DESC",
                                            "TRIP_MINE_DESC",     "DET_PACK_DESC",       "STUN_BATON_DESC"};

// --- Movie Duels (code/cgame, MAX_DPSHOWPOWERS 25) ---
// showDataPadPowers: hell, Kern, dunkel (FP_*: ... SEE 15, DESTRUCTION 16, STASIS 17, GRASP 18, REPULSE 19,
// LIGHTNING_STRIKE 20, FEAR 21, DEADLYSIGHT 22, PROJECTION 23, BLAST 24)
constexpr int kMdKraefte = 25;
constexpr int kMdShowDataPadPowers[kMdKraefte] = {13, 0, 12, 5, 20, 24, 17, 1, 2, 3, 4, 8, 9, 10, 15, 18, 19, 14, 7, 11, 6, 16, 21, 22, 23};
constexpr const char* kMdForceIcons[kMdKraefte] = {
    "gfx/mp/f_icon_lt_heal",       "gfx/mp/f_icon_levitation",   "gfx/mp/f_icon_speed",           "gfx/mp/f_icon_push",
    "gfx/mp/f_icon_pull",          "gfx/mp/f_icon_lt_telepathy", "gfx/mp/f_icon_dk_grip",         "gfx/mp/f_icon_dk_l1",
    "gfx/mp/f_icon_saber_throw",   "gfx/mp/f_icon_saber_defend", "gfx/mp/f_icon_saber_attack",    "gfx/mp/f_icon_dk_rage",
    "gfx/mp/f_icon_lt_protect",    "gfx/mp/f_icon_lt_absorb",    "gfx/mp/f_icon_dk_drain",        "gfx/mp/f_icon_sight",
    "gfx/mp/f_icon_dk_destruction", "gfx/mp/f_icon_lt_stasis",   "gfx/mp/f_icon_lt_grasp",        "gfx/mp/f_icon_repulse",
    "gfx/mp/f_icon_dk_lightning_strike", "gfx/mp/f_icon_dk_fear", "gfx/mp/f_icon_dk_deadlysight", "gfx/mp/f_icon_lt_projection",
    "gfx/mp/f_icon_lt_blast"};
// forcepowerDesc (Index = showDataPadPowers-Index) und der Name in forcepowerLvlNDesc (dort LIGHTNING_STRIKE)
constexpr const char* kMdKraftDesc[kMdKraefte] = {"ABSORB",      "HEAL",          "PROTECT",       "MIND_TRICK", "LIGHTNINGSTRIKE", "BLAST",     "STASIS",
                                                  "JUMP",        "SPEED",         "PUSH",          "PULL",       "SABER_THROW",     "SABER_DEFENSE",
                                                  "SABER_OFFENSE", "SENSE",       "GRASP",         "REPULSE",    "DRAIN",           "LIGHTNING", "RAGE",
                                                  "GRIP",        "DESTRUCTION",   "FEAR",          "DEADLYSIGHT", "PROJECTION"};
constexpr const char* kMdKraftStufe[kMdKraefte] = {"ABSORB",      "HEAL",          "PROTECT",       "MIND_TRICK", "LIGHTNING_STRIKE", "BLAST",     "STASIS",
                                                   "JUMP",        "SPEED",         "PUSH",          "PULL",       "SABER_THROW",      "SABER_DEFENSE",
                                                   "SABER_OFFENSE", "SENSE",       "GRASP",         "REPULSE",    "DRAIN",            "LIGHTNING", "RAGE",
                                                   "GRIP",        "DESTRUCTION",   "FEAR",          "DEADLYSIGHT", "PROJECTION"};

struct Maler {
    const DatapadKontext& k;
    std::vector<OdTeil>& out;

    std::string se(const std::string& key) const { return k.se ? k.se(key) : key; }
    int breite(const std::string& t, float sc, const char* f) const { return k.breite ? k.breite(t, sc, f) : 0; }
    int hoehe(float sc, const char* f) const { return k.hoehe ? k.hoehe(sc, f) : 0; }

    void bild(int x, int y, int w, int h, const std::string& shader, const Farbe& c = kWeiss) {
        OdTeil p;
        p.art = OdTeil::Art::Bild;
        p.x = static_cast<float>(x);
        p.y = static_cast<float>(y);
        p.w = static_cast<float>(w);
        p.h = static_cast<float>(h);
        p.inhalt = shader;
        p.farbe = c;
        out.push_back(std::move(p));
    }
    // cgi_R_Font_DrawString(x, y, text, color, font, -1, scale)
    void text(int x, int y, const std::string& t, const Farbe& c, const char* f, float sc = 1.0F) {
        OdTeil p;
        p.x = static_cast<float>(x);
        p.y = static_cast<float>(y);
        p.inhalt = t;
        p.farbe = c;
        p.scale = sc;
        p.schrift = f;
        out.push_back(std::move(p));
    }

    // CG_DisplayBoxedText: Zeilenvorschub 1,5 * Schrifthoehe (gfAdvanceHack 0) bzw. vorschub * Hoehe
    int kastenText(int x, int y, int w, int h, const std::string& t, const char* f, float sc, const Farbe& c, float vorschub = 0.0F) {
        const int fh = hoehe(sc, f);
        const int adv = static_cast<int>((vorschub == 0.0F ? 1.5F : vorschub) * static_cast<float>(fh));
        const std::function<int(const std::string&)> b = [&](const std::string& s) { return breite(s, sc, f); };
        int zeilen = 0;
        int yy = y;
        std::size_t pos = 0;
        while (pos < t.size() && t[pos] != 0 && yy + fh < y + h) {
            const UmbruchZeile z = naechsteZeile(t, pos, w, b, k.kodierung);
            text(x, yy, z.text, c, f, sc);
            yy += adv;
            ++zeilen;
        }
        return zeilen;
    }

    bool md() const { return k.spiel == Spiel::MD; }
    bool jk2() const { return k.spiel == Spiel::JK2; }

    // Lange Zielzeile Zeichen fuer Zeichen messen (+2 "kerning"), am Ende zurueck bis zum Leerzeichen
    // (ObjectivePrint_Line: bis 10 Zeichen; MissionPrint_Line von Jedi Outcast: bis 0)
    void langeZeile(const std::string& t, int x, int startY, int zeile, int& missionYcnt, const Farbe& c, int mindestens) {
        constexpr int boxW = 500;
        std::size_t str = 0, begin = 0;
        int len = 0, charLen = 0;
        while (str < t.size()) {
            len += breite(std::string(1, t[str]), 1.0F, kMittel) + 2;
            ++charLen;
            if (len > boxW) {
                while (charLen > mindestens && t[str] != ' ') {
                    --str;
                    --charLen;
                }
                if (t[str] == ' ') { ++str; }
                // Q_strncpyz(holdText, strBegin, charLen): charLen - 1 Zeichen
                text(x, startY + zeile * missionYcnt, t.substr(begin, static_cast<std::size_t>(std::max(charLen - 1, 0))), c, kMittel);
                begin = str;
                len = 0;
                charLen = 1;
                ++missionYcnt;
            } else if (str + 1 >= t.size()) {
                ++charLen;
                text(x, startY + zeile * missionYcnt, t.substr(begin, static_cast<std::size_t>(charLen - 1)), c, kMittel);
                ++missionYcnt;
                break;
            }
            ++str;
        }
    }

    // Sicherheitscodes der Jedi-Outcast-Ziele (MissionPrint_Line, auch Movie Duels ObjectivePrint_Line), in
    // der Farbe des Textes davor (RE_Font_DrawString setzt sie)
    void sicherheitsCode(const std::string& name, int missionYcnt, std::array<bool, 4>& grafik, const Farbe& c) {
        constexpr int missionYpos = 79;
        const int y = missionYpos + zeilenHoehe() * missionYcnt;
        if (name == "DOOM_COMM_OBJ4") {
            bild(320 - 128 / 2, y + 8, 128, 32, "textures/system/securitycode", c);
            grafik[0] = true;
        } else if (name == "KEJIM_POST_OBJ3") {
            bild(320 - 32 / 2, y + 8, 32, 32, "textures/system/securitycode_red", c);
            grafik[1] = true;
        } else if (name == "KEJIM_POST_OBJ4") {
            bild(320 - 32 / 2, y + 8 + (grafik[1] ? 32 + 4 : 0), 32, 32, "textures/system/securitycode_green", c);
            grafik[2] = true;
        } else if (name == "KEJIM_POST_OBJ5") {
            bild(320 - 32 / 2, y + 8 + (grafik[1] ? 32 + 4 : 0) + (grafik[2] ? 32 + 4 : 0), 32, 32, "textures/system/securitycode_blue", c);
            grafik[3] = true;
        }
    }
    // iYPixelsPerLine (Jedi Outcast: asiatisch * 1,2)
    int zeilenHoehe() const {
        const int h = hoehe(1.0F, kMittel);
        return jk2() && asiatisch(k.kodierung) ? static_cast<int>(static_cast<float>(h) * 1.2F) : h;
    }

    // ObjectivePrint_Line (JA weiss, Movie Duels CT_BLUE3 und die Codes aus Jedi Outcast)
    void zielZeile(const DatapadWerte::Ziel& ziel, int& missionYcnt, std::array<bool, 4>& grafik, const Farbe& c) {
        constexpr int startX = 60, startY = 75, boxW = 500, boxH = 300;
        const int zeile = zeilenHoehe();
        std::string t = se("OBJECTIVES_" + ziel.name);
        if (ziel.name == "T2_RANCOR_OBJ5") {   // Gefangene zaehlen: va(text, currtotal, maxtotal) mit 0, 0
            for (int i = 0; i < 2; ++i) {
                const std::size_t p = t.find("%d");
                if (p != std::string::npos) { t.replace(p, 2, "0"); }
            }
        }
        const int pixelLen = breite(t, 1.0F, kMittel);
        if (asiatisch(k.kodierung)) {
            int y = startY + zeile * missionYcnt;
            for (const bool g : grafik) { y += g ? 240 + 4 : 0; }   // OBJ_GRAPHIC_SIZE + 4
            missionYcnt += kastenText(startX, y, boxW, boxH, t, kMittel, 1.0F, c, 1.0F);
        } else if (pixelLen < boxW) {
            text(startX, startY + zeile * missionYcnt, t, c, kMittel);
            ++missionYcnt;
        } else {
            langeZeile(t, startX, startY, zeile, missionYcnt, c, 10);
        }
        if (ziel.name == "T3_BOUNTY_OBJ1") {
            bild(355, 50, 240, 240, "textures/system/viewscreen1", c);
            grafik[md() ? 3 : 0] = true;
        } else if (md()) {
            sicherheitsCode(ziel.name, missionYcnt, grafik, c);
        }
    }

    // CG_DrawDataPadObjectives (Movie Duels: Text CT_BLUE3, "keine" CT_LTBLUE1)
    void ziele() {
        constexpr int startX = 60, startY = 75, boxW = 500, boxH = 300;
        const int zeile = zeilenHoehe();
        const int titelX = startX - 22, titelY = startY - 23;
        const int g = 16;
        const int gx = startX - g - 8;
        const int gy = (zeile - g) / 2;
        const Farbe textFarbe = md() ? kBlau3 : kWeiss;
        text(titelX, titelY, se("SP_INGAME_OBJECTIVES"), kTitel, kMittel);
        int missionYcnt = 0;
        std::array<bool, 4> grafik{};
        for (const DatapadWerte::Ziel& z : k.werte.ziele) {
            const int totalY = startY + zeile * missionYcnt + zeile / 2;
            bild(gx, totalY - gy, g, g, "gfx/hud/objective_circle", kBlau3);
            if (z.erfuellt) { bild(gx, totalY - gy, g, g, "gfx/hud/message_on", kBlau3); }
            zielZeile(z, missionYcnt, grafik, textFarbe);
        }
        if (missionYcnt == 0) {
            const std::string t = se("SP_INGAME_OBJNONE");
            text(startX + boxW / 2 - breite(t, 1.0F, kMittel) / 2, startY + boxH / 4, t, md() ? kHellblau1 : kWeiss, kMittel);
        }
    }

    // MissionInformation_Draw / MissionPrint_Line (Jedi Outcast): Text und Kreise CT_BLUE3 ab x 108 bzw. 88
    void missionJk2() {
        constexpr int missionYpos = 79;
        const int zeile = zeilenHoehe();
        text(96, missionYpos - 23, se("INGAME_OBJECTIVES"), kWeiss, kMittel);
        int missionYcnt = 0;
        std::array<bool, 4> grafik{};
        for (const DatapadWerte::Ziel& z : k.werte.ziele) {
            int totalY = missionYpos + zeile * missionYcnt + zeile / 2;
            for (const bool gr : grafik) { totalY += gr ? 32 + 4 : 0; }
            bild(88, totalY, 16, 16, "gfx/hud/objective_circle", kBlau3);
            if (z.erfuellt) { bild(88, totalY, 16, 16, "gfx/hud/message_on", kBlau3); }
            const std::string t = se("OBJECTIVES_" + z.name);
            if (breite(t, 1.0F, kMittel) < 500) {
                int y = missionYpos + zeile * missionYcnt;
                for (int i = 0; i < 3; ++i) { y += grafik[static_cast<std::size_t>(i)] ? 32 + 4 : 0; }
                text(108, y, t, kBlau3, kMittel);
                ++missionYcnt;
            } else {   // CG_DrawProportionalString = qhFontMedium
                langeZeile(t, 108, missionYpos, zeile, missionYcnt, kBlau3, 0);
            }
            sicherheitsCode(z.name, missionYcnt, grafik, kBlau3);
        }
        if (missionYcnt == 0) { text(108, missionYpos, se("INGAME_OBJNONE"), kHellblau1, kMittel); }
    }

    // CG_DrawDataPadWeaponSelect
    void waffen() {
        const unsigned bits = k.werte.waffen;
        int anzahl = 0;
        for (int i = 1; i < 16; ++i) {
            if ((bits & (1U << i)) != 0) { ++anzahl; }
        }
        if (anzahl == 0) { return; }
        constexpr int sideMax = 3;
        const int hold = anzahl - 1;
        const int links = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold / 2;
        const int rechts = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold - links;
        int sel = k.werte.waffe;
        if (sel < WP_SABER) { sel = WP_SABER; }
        else if (sel > 13) { sel = 13; }
        auto icon = [&](int wp) { return wp >= 0 && static_cast<std::size_t>(wp) < k.waffenIcon.size() ? k.waffenIcon[static_cast<std::size_t>(wp)] : std::string(); };
        constexpr int klein = 40, gross = 80, grossPad = 64, pad = 32, mitteX = 320, bildY = 340;
        bool drewConc = false;
        int i = sel == WP_CONCUSSION ? WP_FLECHETTE : sel - 1;
        if (i < 1) { i = 13; }
        int x = mitteX - (gross / 2 + grossPad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i == WP_CONCUSSION) { --i; }
            else if (i == WP_FLECHETTE && !drewConc && sel != WP_CONCUSSION) { i = WP_CONCUSSION; }
            if (i < 1) { i = 13; }
            if ((bits & (1U << i)) == 0) {
                if (i == WP_CONCUSSION) {
                    drewConc = true;
                    i = WP_ROCKET_LAUNCHER;
                }
                continue;
            }
            ++n;
            if (!icon(i).empty()) {   // CG_WeaponCheck: Beispiel - Munition da
                bild(x, bildY, klein, klein, icon(i));
                x -= klein + pad;
            }
            if (i == WP_CONCUSSION) {
                drewConc = true;
                i = WP_ROCKET_LAUNCHER;
            }
        }
        if (!icon(sel).empty()) { bild(mitteX - gross / 2, bildY - (gross - klein) / 2 + 10, gross, gross, icon(sel)); }
        i = sel == WP_CONCUSSION ? WP_ROCKET_LAUNCHER : sel + 1;
        if (i > 13) { i = 1; }
        x = mitteX + gross / 2 + grossPad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i == WP_CONCUSSION) { ++i; }
            else if (i == WP_ROCKET_LAUNCHER && !drewConc && sel != WP_CONCUSSION) { i = WP_CONCUSSION; }
            if (i > 13) { i = 1; }
            if ((bits & (1U << i)) == 0) {
                if (i == WP_CONCUSSION) {
                    drewConc = true;
                    i = WP_FLECHETTE;
                }
                continue;
            }
            ++n;
            if (!icon(i).empty()) {
                bild(x, bildY, klein, klein, icon(i));
                x += klein + pad;
            }
            if (i == WP_CONCUSSION) {
                drewConc = true;
                i = WP_FLECHETTE;
            }
        }
        const std::string t = se(std::string("SP_INGAME_") + kWaffenText[sel - 1]);
        if (!t.empty()) { kastenText(40, 60, 560, 300, t, kArialnb, 1.0F, kWeiss); }
    }

    // ForcePowerDataPad_Valid
    bool gueltig(int i) const { return k.werte.kraft[static_cast<std::size_t>(kShowDataPadPowers[i])] > 0; }

    // CG_DrawDataPadForceSelect (ohne cg_updatedDataPadForcePower: kein Wirbel)
    void kraefte() {
        int anzahl = 0;
        for (int i = 0; i < kMaxDpPowers; ++i) {
            if (gueltig(i)) { ++anzahl; }
        }
        if (anzahl < 1) { return; }
        constexpr int sideMax = 3;
        const int hold = anzahl - 1;
        const int links = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold / 2;
        const int rechts = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold - links;
        constexpr int klein = 40, gross = 70, grossPad = 64, pad = 32, mitteX = 320, bildY = 340;
        const int sel = std::clamp(k.werte.kraftWahl, 0, kMaxDpPowers - 1);
        auto icon = [](int i) { return kForceIcons[kShowDataPadPowers[i]]; };
        int i = sel - 1;
        if (i < 0) { i = kMaxDpPowers - 1; }
        int x = mitteX - (gross / 2 + grossPad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i < 0) { i = kMaxDpPowers - 1; }
            if (!gueltig(i)) { continue; }
            ++n;
            {
                bild(x, bildY, klein, klein, icon(i));
                x -= klein + pad;
            }
        }
        { bild(mitteX - gross / 2, bildY - (gross - klein) / 2, gross, gross, icon(sel)); }
        i = sel + 1;
        if (i >= kMaxDpPowers) { i = 0; }
        x = mitteX + gross / 2 + grossPad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i >= kMaxDpPowers) { i = 0; }
            if (!gueltig(i)) { continue; }
            ++n;
            {
                bild(x, bildY, klein, klein, icon(i));
                x += klein + pad;
            }
        }
        const std::string name = kKraftName[sel];
        const std::string t = se("SP_INGAME_FORCE_" + name + "_DESC");
        const int stufe = k.werte.kraft[static_cast<std::size_t>(kShowDataPadPowers[sel])];
        const std::string t2 = se("SP_INGAME_FORCE_" + name + "_LVL" + std::to_string(stufe == 1 ? 1 : stufe == 2 ? 2 : 3) + "_DESC");
        if (!t.empty()) { kastenText(40, 60, 560, 300, t + t2, kArialnb, 1.0F, kWeiss); }
    }

    // --- Jedi Outcast und Movie Duels ---------------------------------------------------------------

    // R_RegisterShader != 0: Movie Duels zeichnet ein Symbol nur, wenn es da ist
    bool da(const std::string& s) const { return !s.empty() && (!k.bildDa || k.bildDa(s)); }
    // SP_INGAME_<name>, sonst SPMOD_INGAME_<name> (Movie Duels)
    std::string seMd(const std::string& name) const {
        const std::string t = se("SP_INGAME_" + name);
        return t.empty() ? se("SPMOD_INGAME_" + name) : t;
    }
    const char* schrift4() const { return k.schrift4.c_str(); }

    // CG_DrawDataPadIconBackground: Hintergrund oben gespiegelt (Hoehe -60), Zinken rechts gespiegelt
    void iconHintergrund(int art) {
        const char* bg = art == kDpInventory ? "gfx/hud/background_i" : art == kDpWeapons ? "gfx/hud/background" : "gfx/hud/background_f";
        const char* zinke = art == kDpInventory ? "gfx/hud/prong_on_i" : art == kDpWeapons ? "gfx/hud/prong_on_w" : "gfx/hud/prong_on_f";
        constexpr int x2 = 0, y2 = 295, hoehe = 60, xAdd = 8;
        bild(x2 + 110, y2 + 30, 410, -hoehe, bg);
        bild(x2 + 110, y2 + 30 - 2, 410, hoehe, bg);
        bild(x2 + 97 + xAdd, y2 - 10, 40, 80, zinke);
        bild(x2 + 544 - xAdd, y2 - 10, -40, 80, zinke);
    }

    // Wie viele Symbole links und rechts der Mitte (sideMax)
    static void seiten(int anzahl, int sideMax, int& links, int& rechts) {
        const int hold = anzahl - 1;
        links = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold / 2;
        rechts = hold == 0 ? 0 : anzahl > 2 * sideMax ? sideMax : hold - links;
    }
    std::string waffenIcon(int wp) const {
        return wp >= 0 && static_cast<std::size_t>(wp) < k.waffenIcon.size() ? k.waffenIcon[static_cast<std::size_t>(wp)] : std::string();
    }

    // CG_DrawDataPadWeaponSelect (Jedi Outcast): Seiten mit Deckung 0,6, Mitte weiss, Text qhFontSmall 0,7
    void waffenJk2() {
        const unsigned bits = k.werte.waffen;
        int anzahl = 0;
        for (int i = 1; i < 16; ++i) { anzahl += (bits & (1U << i)) != 0 ? 1 : 0; }
        if (anzahl == 0) { return; }
        int links = 0, rechts = 0;
        seiten(anzahl, 3, links, rechts);
        const int sel = std::clamp(k.werte.waffe, 1, 13);   // FIRST_WEAPON .. MAX_PLAYER_WEAPONS (WP_STUN_BATON)
        constexpr int klein = 40, gross = 80, pad = 8, x = 320, y = 300;
        const Farbe seite{1, 1, 1, 0.6F};
        int i = sel - 1;
        if (i < 1) { i = 13; }
        int holdX = x - (gross / 2 + pad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i < 1) { i = 13; }
            if ((bits & (1U << i)) == 0) { continue; }
            ++n;
            if (!waffenIcon(i).empty()) {   // CG_WeaponCheck: Beispiel - Munition da
                bild(holdX, y + 10, klein, klein, waffenIcon(i), seite);
                holdX -= klein + pad;
            }
        }
        if (!waffenIcon(sel).empty()) { bild(x - gross / 2, (y - (gross - klein) / 2) + 10, gross, gross, waffenIcon(sel)); }
        i = sel + 1;
        if (i > 13) { i = 1; }
        holdX = x + gross / 2 + pad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i > 13) { i = 1; }
            if ((bits & (1U << i)) == 0) { continue; }
            ++n;
            if (!waffenIcon(i).empty()) {
                bild(holdX, y + 10, klein, klein, waffenIcon(i), seite);
                holdX += klein + pad;
            }
        }
        const std::string t = se(std::string("INGAME_") + kJk2WaffenText[sel - 1]);
        if (!t.empty()) { kastenText(70, 50, 500, 300, t, "ocr_a", 0.7F, Farbe{0.875F, 0.718F, 0.121F, 1}); }
    }

    // CG_DrawDataPadWeaponSelect (Movie Duels): eine Waffe je Seite, 22/45 gross, Text mit Griff 4
    void waffenMd() {
        const unsigned bits = k.werte.waffen;
        int anzahl = 0;
        for (int i = 1; i < 32; ++i) { anzahl += (bits & (1U << i)) != 0 ? 1 : 0; }
        if (anzahl == 0) { return; }
        int links = 0, rechts = 0;
        seiten(anzahl, 1, links, rechts);
        constexpr int kNum = 32;   // ueber alle Waffen: nur die vorhandenen zaehlen (WP_NUM_WEAPONS)
        const int sel = std::clamp(k.werte.waffe, 1, kNum - 1);
        auto hat = [&](int wp) { return wp > 0 && wp < kNum && (bits & (1U << wp)) != 0; };
        auto zeichne = [&](int wp, int xx, int yy, int g) {
            if (waffenIcon(wp).empty()) { return false; }   // weapon_Icon_file[0]
            bild(xx, yy, g, g, waffenIcon(wp));
            return true;
        };
        constexpr int klein = 22, gross = 45, grossPad = 64, pad = 32, mitteX = 320, bildY = 300;
        bool drewConc = false;
        int i = sel == WP_CONCUSSION ? WP_FLECHETTE : sel - 1;
        if (i < 1) { i = kNum - 1; }
        int holdX = mitteX - (gross / 2 + grossPad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i == WP_CONCUSSION) { --i; }
            else if (i == WP_FLECHETTE && !drewConc && sel != WP_CONCUSSION) { i = WP_CONCUSSION; }
            if (i < 1) { i = kNum - 1; }
            if (!hat(i)) {
                if (i == WP_CONCUSSION) {
                    drewConc = true;
                    i = WP_ROCKET_LAUNCHER;
                }
                continue;
            }
            ++n;
            if (zeichne(i, holdX, bildY, klein)) { holdX -= klein + pad; }
            if (i == WP_CONCUSSION) {
                drewConc = true;
                i = WP_ROCKET_LAUNCHER;
            }
        }
        zeichne(sel, mitteX - gross / 2, bildY - (gross - klein) / 2 + 10, gross);
        i = sel == WP_CONCUSSION ? WP_ROCKET_LAUNCHER : sel + 1;
        if (i >= kNum) { i = 1; }
        holdX = mitteX + gross / 2 + grossPad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i == WP_CONCUSSION) { ++i; }
            else if (i == WP_ROCKET_LAUNCHER && !drewConc && sel != WP_CONCUSSION) { i = WP_CONCUSSION; }
            if (i >= kNum) { i = 1; }
            if (!hat(i)) {
                if (i == WP_CONCUSSION) {
                    drewConc = true;
                    i = WP_FLECHETTE;
                }
                continue;
            }
            ++n;
            if (zeichne(i, holdX, bildY, klein)) { holdX += klein + pad; }
            if (i == WP_CONCUSSION) {
                drewConc = true;
                i = WP_FLECHETTE;
            }
        }
        if (sel <= 13) {   // weaponDesc: die ersten 13 wie Jedi Academy
            const std::string t = seMd(kWaffenText[sel - 1]);
            if (!t.empty()) { kastenText(40, 60, 560, 300, t, schrift4(), 1.0F, kWeiss); }
        }
    }

    // CG_DrawDataPadInventorySelect ohne Inventar: Breite mit qhFontSmall, gezeichnet mit qhFontMedium
    void leeresInventar(const std::string& schluessel) {
        const std::string t = se(schluessel);
        text((640 - breite(t, 1.0F, "ocr_a")) / 2, 300 + 22, t, kIconBlau, kMittel);
    }

    // CG_DrawDataPadForceSelect (Jedi Outcast: 30/60 gross, alle weiss; die Stufe kommt - wie im Spiel -
    // aus forcePowerLevel[Index] statt [showDataPadPowers[Index]], bei Jedi Outcast dasselbe)
    void kraefteJk2() {
        auto gueltigJk2 = [&](int i) { return k.werte.kraft[static_cast<std::size_t>(i)] > 0; };
        int anzahl = 0;
        for (int i = 0; i < kJk2Kraefte; ++i) { anzahl += gueltigJk2(i) ? 1 : 0; }
        if (anzahl == 0) { return; }
        int links = 0, rechts = 0;
        seiten(anzahl, 3, links, rechts);
        constexpr int klein = 30, gross = 60, pad = 8, x = 320, y = 310;
        const int sel = std::clamp(k.werte.kraftWahl, 0, kJk2Kraefte - 1);
        int i = sel - 1;
        if (i < 0) { i = kJk2Kraefte - 1; }
        int holdX = x - (gross / 2 + pad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i < 0) { i = kJk2Kraefte - 1; }
            if (!gueltigJk2(i)) { continue; }
            ++n;
            bild(holdX, y, klein, klein, kJk2ForceIcons[i]);
            holdX -= klein + pad;
        }
        bild(x - gross / 2, y - (gross - klein) / 2, gross, gross, kJk2ForceIcons[sel]);
        i = sel + 1;
        if (i >= kJk2Kraefte) { i = 0; }
        holdX = x + gross / 2 + pad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i >= kJk2Kraefte) { i = 0; }
            if (!gueltigJk2(i)) { continue; }
            ++n;
            bild(holdX, y, klein, klein, kJk2ForceIcons[i]);
            holdX += klein + pad;
        }
        const std::string name = kJk2KraftName[sel];
        const std::string t = se("INGAME_FORCE_" + name + "_DESC");
        const int stufe = k.werte.kraft[static_cast<std::size_t>(sel)];
        const std::string t2 = se("INGAME_FORCE_" + name + "_LVL" + std::to_string(stufe == 1 ? 1 : stufe == 2 ? 2 : 3) + "_DESC");
        if (!t.empty()) { kastenText(70, 50, 500, 300, t + t2, "ocr_a", 0.7F, kIconBlau); }
    }

    // CG_DrawDataPadForceSelect (Movie Duels): eine Kraft je Seite, 22/45 gross, Symbol nur wenn da,
    // Text SP_INGAME_/SPMOD_INGAME_ mit Griff 4 in CT_ICON_BLUE
    void kraefteMd() {
        auto fp = [](int i) { return kMdShowDataPadPowers[i]; };
        auto gueltigMd = [&](int i) { return k.werte.kraft[static_cast<std::size_t>(fp(i))] > 0; };
        int anzahl = 0;
        for (int i = 0; i < kMdKraefte; ++i) { anzahl += gueltigMd(i) ? 1 : 0; }
        if (anzahl < 1) { return; }
        int links = 0, rechts = 0;
        seiten(anzahl, 1, links, rechts);
        constexpr int klein = 22, gross = 45, grossPad = 64, pad = 32, mitteX = 320, bildY = 310;
        const int sel = std::clamp(k.werte.kraftWahl, 0, kMdKraefte - 1);
        int i = sel - 1;
        if (i < 0) { i = kMdKraefte - 1; }
        int holdX = mitteX - (gross / 2 + grossPad + klein);
        for (int n = 1; n < links + 1; --i) {
            if (i < 0) { i = kMdKraefte - 1; }
            if (!gueltigMd(i)) { continue; }
            ++n;
            if (da(kMdForceIcons[fp(i)])) {
                bild(holdX, bildY, klein, klein, kMdForceIcons[fp(i)]);
                holdX -= klein + pad;
            }
        }
        if (da(kMdForceIcons[fp(sel)])) { bild(mitteX - gross / 2, bildY - (gross - klein) / 2, gross, gross, kMdForceIcons[fp(sel)]); }
        i = sel + 1;
        if (i >= kMdKraefte) { i = 0; }
        holdX = mitteX + gross / 2 + grossPad;
        for (int n = 1; n < rechts + 1; ++i) {
            if (i >= kMdKraefte) { i = 0; }
            if (!gueltigMd(i)) { continue; }
            ++n;
            if (da(kMdForceIcons[fp(i)])) {
                bild(holdX, bildY, klein, klein, kMdForceIcons[fp(i)]);
                holdX += klein + pad;
            }
        }
        const std::string t = seMd(std::string("FORCE_") + kMdKraftDesc[sel] + "_DESC");
        const int stufe = k.werte.kraft[static_cast<std::size_t>(fp(sel))];
        const std::string t2 = seMd(std::string("FORCE_") + kMdKraftStufe[sel] + "_LVL" + std::to_string(stufe == 1 ? 1 : stufe == 2 ? 2 : 3) + "_DESC");
        if (!t.empty()) { kastenText(40, 60, 560, 300, t + t2, schrift4(), 1.0F, kIconBlau); }
    }
};

}  // namespace

std::string kraftSymbol(int fp) { return fp >= 0 && fp < 16 ? kForceIcons[fp] : ""; }

std::vector<std::string> datapadWaffenNamen(Spiel s) {
    std::vector<std::string> n = {"WP_SABER", "WP_BLASTER_PISTOL", "WP_BLASTER",         "WP_DISRUPTOR", "WP_BOWCASTER", "WP_REPEATER",  "WP_DEMP2",
                                  "WP_FLECHETTE", "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE", "WP_DET_PACK",  "WP_CONCUSSION"};
    if (s == Spiel::JK2) {   // codeJK2/game/weapons.h
        n[1] = "WP_BRYAR_PISTOL";
        n[12] = "WP_STUN_BATON";
    }
    return n;
}

DatapadWerte datapadBeispiel(Spiel s) {
    DatapadWerte w;
    if (s == Spiel::JK2) {
        w.kraftWahl = 1;   // showDataPadPowers[1] = FP_LEVITATION
        w.ziele = {{"KEJIM_POST_OBJ1", true}, {"KEJIM_POST_OBJ2", false}};
    } else if (s == Spiel::MD) {
        w.kraftWahl = 7;   // FP_LEVITATION
    }
    return w;
}

std::vector<int> datapadKraefte(Spiel s) {
    if (s == Spiel::JK2) { return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; }
    if (s == Spiel::MD) { return {std::begin(kMdShowDataPadPowers), std::end(kMdShowDataPadPowers)}; }
    return {std::begin(kShowDataPadPowers), std::end(kShowDataPadPowers)};
}

std::vector<std::string> datapadKraftNamen(Spiel s) {
    if (s == Spiel::JK2) { return {std::begin(kJk2KraftName), std::end(kJk2KraftName)}; }
    if (s == Spiel::MD) { return {std::begin(kMdKraftDesc), std::end(kMdKraftDesc)}; }
    return {std::begin(kKraftName), std::end(kKraftName)};
}

std::vector<std::string> waffenIcons(const std::string& dat, Spiel s) {
    // WPN_Parse: { weapontype WP_x ... weaponIcon <bild> ... }; Kommentare // und /* */
    std::vector<std::string> namen = datapadWaffenNamen(s);
    namen.insert(namen.begin(), "WP_NONE");
    std::vector<std::string> icons(namen.size());
    std::vector<std::string> w;
    for (std::size_t p = 0; p < dat.size();) {
        if (dat.compare(p, 2, "//") == 0) {
            p = dat.find('\n', p);
            if (p == std::string::npos) { break; }
            continue;
        }
        if (dat.compare(p, 2, "/*") == 0) {
            p = dat.find("*/", p);
            if (p == std::string::npos) { break; }
            p += 2;
            continue;
        }
        if (static_cast<unsigned char>(dat[p]) <= ' ') {
            ++p;
            continue;
        }
        std::size_t e = p;
        while (e < dat.size() && static_cast<unsigned char>(dat[e]) > ' ') { ++e; }
        w.push_back(dat.substr(p, e - p));
        p = e;
    }
    int wp = -1;
    for (std::size_t i = 0; i + 1 < w.size(); ++i) {
        std::string a = w[i];
        for (char& c : a) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (a == "weapontype") {
            wp = -1;
            for (std::size_t n = 0; n < namen.size(); ++n) {
                if (w[i + 1] == namen[n]) { wp = static_cast<int>(n); }
            }
        } else if (a == "weaponicon" && wp >= 0) {
            icons[static_cast<std::size_t>(wp)] = w[i + 1];
        }
    }
    return icons;
}

void zeichneDatapad(int art, const DatapadKontext& k, std::vector<OdTeil>& out) {
    Maler m{k, out};
    if (k.spiel != Spiel::JA) {   // vmMain CG_DRAW_DATAPAD_* von Jedi Outcast und Movie Duels
        const bool md = k.spiel == Spiel::MD;
        switch (art) {
            case kDpObjectives:
                if (md) { m.ziele(); }
                else { m.missionJk2(); }
                break;
            case kDpWeapons:
                m.iconHintergrund(art);
                if (md) { m.waffenMd(); }   // cg_com_kotor 0, charKOTORWeapons 0
                else { m.waffenJk2(); }
                break;
            case kDpInventory:
                m.iconHintergrund(art);
                m.leeresInventar(md ? "SP_INGAME_EMPTY_INV" : "INGAME_EMPTY_INV");
                break;
            case kDpForcePowers:
                m.iconHintergrund(art);
                if (md) { m.kraefteMd(); }
                else { m.kraefteJk2(); }
                break;
            default: break;
        }
        return;
    }
    switch (art) {
        case kDpObjectives: m.ziele(); break;
        case kDpWeapons: m.waffen(); break;   // CG_DrawDataPadIconBackground ist auskommentiert
        case kDpForcePowers: m.kraefte(); break;
        case kDpInventory: m.leeresInventar("SP_INGAME_EMPTY_INV"); break;   // CG_DrawDataPadInventorySelect: Beispiel ohne Inventar
        default: break;
    }
}

}  // namespace bhed::menue
