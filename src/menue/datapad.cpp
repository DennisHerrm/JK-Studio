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

    // ObjectivePrint_Line
    void zielZeile(const DatapadWerte::Ziel& ziel, int& missionYcnt, bool& grafik) {
        constexpr int startX = 60, startY = 75, boxW = 500, boxH = 300;
        const int zeile = hoehe(1.0F, kMittel);
        std::string t = se("OBJECTIVES_" + ziel.name);
        if (ziel.name == "T2_RANCOR_OBJ5") {   // Gefangene zaehlen: va(text, currtotal, maxtotal) mit 0, 0
            for (int i = 0; i < 2; ++i) {
                const std::size_t p = t.find("%d");
                if (p != std::string::npos) { t.replace(p, 2, "0"); }
            }
        }
        const int pixelLen = breite(t, 1.0F, kMittel);
        if (asiatisch(k.kodierung)) {
            const int y = startY + zeile * missionYcnt + (grafik ? 240 + 4 : 0);
            missionYcnt += kastenText(startX, y, boxW, boxH, t, kMittel, 1.0F, kWeiss, 1.0F);
        } else if (pixelLen < boxW) {
            text(startX, startY + zeile * missionYcnt, t, kWeiss, kMittel);
            ++missionYcnt;
        } else {   // Zeichen fuer Zeichen messen (+2 "kerning"), am Ende zurueck bis zum Leerzeichen
            std::size_t str = 0, begin = 0;
            int len = 0, charLen = 0;
            while (str < t.size()) {
                len += breite(std::string(1, t[str]), 1.0F, kMittel) + 2;
                ++charLen;
                if (len > boxW) {
                    while (charLen > 10 && t[str] != ' ') {
                        --str;
                        --charLen;
                    }
                    if (t[str] == ' ') { ++str; }
                    // Q_strncpyz(holdText, strBegin, charLen): charLen - 1 Zeichen
                    text(startX, startY + zeile * missionYcnt, t.substr(begin, static_cast<std::size_t>(charLen - 1)), kWeiss, kMittel);
                    begin = str;
                    len = 0;
                    charLen = 1;
                    ++missionYcnt;
                } else if (str + 1 >= t.size()) {
                    ++charLen;
                    text(startX, startY + zeile * missionYcnt, t.substr(begin, static_cast<std::size_t>(charLen - 1)), kWeiss, kMittel);
                    ++missionYcnt;
                    break;
                }
                ++str;
            }
        }
        if (ziel.name == "T3_BOUNTY_OBJ1") {
            bild(355, 50, 240, 240, "textures/system/viewscreen1");
            grafik = true;
        }
    }

    // CG_DrawDataPadObjectives
    void ziele() {
        constexpr int startX = 60, startY = 75, boxW = 500, boxH = 300;
        const int zeile = hoehe(1.0F, kMittel);
        const int titelX = startX - 22, titelY = startY - 23;
        const int g = 16;
        const int gx = startX - g - 8;
        const int gy = (zeile - g) / 2;
        text(titelX, titelY, se("SP_INGAME_OBJECTIVES"), kTitel, kMittel);
        int missionYcnt = 0;
        bool grafik = false;
        for (const DatapadWerte::Ziel& z : k.werte.ziele) {
            const int totalY = startY + zeile * missionYcnt + zeile / 2;
            bild(gx, totalY - gy, g, g, "gfx/hud/objective_circle", kBlau3);
            if (z.erfuellt) { bild(gx, totalY - gy, g, g, "gfx/hud/message_on", kBlau3); }
            zielZeile(z, missionYcnt, grafik);
        }
        if (missionYcnt == 0) {
            const std::string t = se("SP_INGAME_OBJNONE");
            text(startX + boxW / 2 - breite(t, 1.0F, kMittel) / 2, startY + boxH / 4, t, kWeiss, kMittel);
        }
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
};

}  // namespace

std::string kraftSymbol(int fp) { return fp >= 0 && fp < 16 ? kForceIcons[fp] : ""; }

std::vector<std::string> waffenIcons(const std::string& dat) {
    // WPN_Parse: { weapontype WP_x ... weaponIcon <bild> ... }; Kommentare // und /* */
    static constexpr const char* kNamen[] = {"WP_NONE",    "WP_SABER",    "WP_BLASTER_PISTOL", "WP_BLASTER",  "WP_DISRUPTOR", "WP_BOWCASTER",
                                             "WP_REPEATER", "WP_DEMP2",   "WP_FLECHETTE",      "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE",
                                             "WP_DET_PACK", "WP_CONCUSSION"};
    std::vector<std::string> icons(std::size(kNamen));
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
            for (std::size_t n = 0; n < std::size(kNamen); ++n) {
                if (w[i + 1] == kNamen[n]) { wp = static_cast<int>(n); }
            }
        } else if (a == "weaponicon" && wp >= 0) {
            icons[static_cast<std::size_t>(wp)] = w[i + 1];
        }
    }
    return icons;
}

void zeichneDatapad(int art, const DatapadKontext& k, std::vector<OdTeil>& out) {
    Maler m{k, out};
    switch (art) {
        case kDpObjectives: m.ziele(); break;
        case kDpWeapons: m.waffen(); break;   // CG_DrawDataPadIconBackground ist auskommentiert
        case kDpForcePowers: m.kraefte(); break;
        case kDpInventory: {   // CG_DrawDataPadInventorySelect: Beispiel ohne Inventar
            // Breite mit qhFontSmall gemessen, gezeichnet aber mit qhFontMedium (CG_DrawProportionalString)
            const std::string t = m.se("SP_INGAME_EMPTY_INV");
            m.text((640 - m.breite(t, 1.0F, "ocr_a")) / 2, 300 + 22, t, Farbe{0.567F, 0.685F, 1.0F, 0.75F}, kMittel);   // CT_ICON_BLUE
            break;
        }
        default: break;
    }
}

}  // namespace bhed::menue
