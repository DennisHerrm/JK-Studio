// hud.cpp - Menue-Editor M3 Punkt 5: was cgame ueber die HUD-Menues zeichnet.
// Portiert aus OpenJK code/cgame/cg_draw.cpp, cg_drawtools.cpp und cg_main.cpp
// (Einzelheiten in include/bhed/menue/hud.h).
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
#include "bhed/menue/hud.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bhed::menue {

namespace {

constexpr int kHudTics = 4;   // MAX_HUD_TICS (cg_media.h)
// shared/qcommon/q_color.c colorTable
constexpr Farbe kWeiss{1, 1, 1, 1}, kRot{1, 0, 0, 1}, kGelb{1, 1, 0, 1};
// cg_main.cpp sb_t_nums (NUM_FONT_SMALL), STAT_MINUS = 10
const char* const kZahlen[11] = {"gfx/2d/numbers/t_zero", "gfx/2d/numbers/t_one",   "gfx/2d/numbers/t_two",  "gfx/2d/numbers/t_three",
                                 "gfx/2d/numbers/t_four", "gfx/2d/numbers/t_five",  "gfx/2d/numbers/t_six",  "gfx/2d/numbers/t_seven",
                                 "gfx/2d/numbers/t_eight", "gfx/2d/numbers/t_nine", "gfx/2d/numbers/t_minus"};

bool gleich(const std::string& a, const char* b) {   // Q_stricmp
    const std::size_t n = std::strlen(b);
    if (a.size() != n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

int menueNamens(const Datei& d, const char* name) {   // Menus_FindByName
    for (std::size_t i = 0; i < d.menus.size(); ++i) {
        if (gleich(d.menus[i].window.name, name)) { return static_cast<int>(i); }
    }
    return -1;
}

// HUDMenuItem_t nach cgi_UI_GetMenuItemInfo - nicht gefunden zeichnet nichts
using HudTeil = MenueItemInfo;
HudTeil teil(const Datei& d, const char* menue, const char* item) { return menueItemInfo(d, menue, item); }

class Hud {
public:
    Hud(const Datei& d, const HudWerte& w, HudZustand& z, std::vector<Prim>& out) : d_(d), w_(w), z_(z), out_(out) {}

    // CG_DrawPic mit der zuletzt gesetzten Farbe (cgi_R_SetColor)
    void bild(float x, float y, float b, float h, const std::string& shader, const Farbe& f) {
        if (b <= 0 || h <= 0 || shader.empty()) { return; }
        Prim p;
        p.art = PrimArt::Bild;
        p.x = x;
        p.y = y;
        p.w = b;
        p.h = h;
        p.farbe = f;
        p.shader = shader;
        out_.push_back(p);
    }
    void teilBild(const HudTeil& t, const Farbe& f) {
        bild(static_cast<float>(t.x), static_cast<float>(t.y), static_cast<float>(t.w), static_cast<float>(t.h), t.bild, f);
    }

    // CG_DrawNumField(x, y, width, value, charWidth, charHeight, NUM_FONT_SMALL, zeroFill)
    void zahlenFeld(int x, int y, int width, int value, int charWidth, int charHeight, const Farbe& f, bool zeroFill = false) {
        if (width < 1) { return; }
        if (width > 5) { width = 5; }
        switch (width) {
            case 1: value = value > 9 ? 9 : (value < 0 ? 0 : value); break;
            case 2: value = value > 99 ? 99 : (value < -9 ? -9 : value); break;
            case 3: value = value > 999 ? 999 : (value < -99 ? -99 : value); break;
            case 4: value = value > 9999 ? 9999 : (value < -999 ? -999 : value); break;
            default: break;
        }
        char num[16];
        std::snprintf(num, sizeof(num), "%i", value);
        int l = static_cast<int>(std::strlen(num));
        if (l > width) { l = width; }
        const int xWidth = charWidth;   // NUM_FONT_SMALL
        if (zeroFill) {
            for (int i = 0; i < width - l; ++i) {
                bild(static_cast<float>(x), static_cast<float>(y), static_cast<float>(charWidth), static_cast<float>(charHeight), kZahlen[0], f);
                x += 2 + xWidth;
            }
        } else {
            x += 2 + xWidth * (width - l);
        }
        for (const char* p = num; *p != 0 && l > 0; ++p, --l) {
            const int frame = *p == '-' ? 10 : *p - '0';
            bild(static_cast<float>(x), static_cast<float>(y), static_cast<float>(charWidth), static_cast<float>(charHeight), kZahlen[frame], f);
            ++x;   // For a one line gap
            x += xWidth;
        }
    }

    // Striche wie CG_DrawHealth/CG_DrawAmmo: von oben (Index 3) nach unten; volle weiss,
    // der angebrochene in der Farbe seines Items mit Alpha * Anteil
    void striche(const char* menue, const char* praefix, float inc, float wert, bool ruestung) {
        Farbe f = kWeiss;
        for (int i = kHudTics - 1; i >= 0; --i) {
            if (wert <= 0) { break; }
            char name[32];
            std::snprintf(name, sizeof(name), "%s%d", praefix, i + 1);
            const HudTeil t = teil(d_, menue, name);
            if (wert < inc) {
                f = t.farbe;
                f.a *= wert / inc;
            }
            // CG_DrawArmor: der angebrochene oberste Strich nur, wenn cg.HUDArmorFlag
            if (!ruestung || !(i == kHudTics - 1 && wert < inc) || z_.armorFlag) { teilBild(t, f); }
            wert -= inc;
        }
    }

    void gesundheit() {   // CG_DrawHealth
        striche("lefthud", "health_tic", static_cast<float>(w_.gesundheitMax) / kHudTics, static_cast<float>(w_.gesundheit), false);
        const HudTeil t = teil(d_, "lefthud", "healthamount");
        zahlenFeld(t.x, t.y, 3, w_.gesundheit, t.w, t.h, t.farbe);
    }

    void ruestung() {   // CG_DrawArmor
        striche("lefthud", "armor_tic", static_cast<float>(w_.gesundheitMax) / kHudTics, static_cast<float>(w_.ruestung), true);
        const HudTeil t = teil(d_, "lefthud", "armoramount");
        zahlenFeld(t.x, t.y, 3, w_.ruestung, t.w, t.h, t.farbe);
        if (w_.ruestung != 0) {
            const float viertel = static_cast<float>(w_.gesundheitMax) / 4.0F;
            if (static_cast<float>(w_.ruestung) < viertel) {
                if (z_.tickFlashTime < w_.zeitMs) {
                    z_.tickFlashTime = w_.zeitMs + 400;
                    z_.armorFlag = !z_.armorFlag;
                }
            } else {
                z_.armorFlag = true;
            }
        } else {
            z_.armorFlag = false;
        }
    }

    void macht() {   // CG_DrawForcePower (ohne das Aufblinken bei zu wenig Macht)
        if (!w_.machtBekannt) { return; }
        const float max = static_cast<float>(w_.machtMax);
        const float inc = max / kHudTics;
        float wert = static_cast<float>(w_.macht);
        float extra = 0;
        if (wert > max) {   // supercharged with force
            extra = wert - max;
            wert = max;
        }
        for (int i = kHudTics - 1; i >= 0; --i) {
            Farbe f = kWeiss;
            if (extra != 0) {
                const float anteil = 0.75F + (std::sin(static_cast<float>(w_.zeitMs) * 0.005F) * ((extra / max) * 0.25F));
                f.r *= anteil;
                f.g *= anteil;
                f.b *= anteil;
            } else if (wert <= 0) {
                break;
            } else if (wert < inc) {
                f.a = wert / inc;
            }
            char name[16];
            std::snprintf(name, sizeof(name), "force_tic%d", i + 1);
            teilBild(teil(d_, "righthud", name), f);
            wert -= inc;
        }
        const HudTeil t = teil(d_, "righthud", "forceamount");
        zahlenFeld(t.x, t.y, 3, w_.macht, t.w, t.h, t.farbe);
    }

    void saebelStil() {   // CG_DrawSaberStyle
        const int s = w_.saebelStil;
        const char* name = (s == 1 || s == 5) ? "saberstyle_fast" : (s == 2 || s == 6 || s == 7) ? "saberstyle_medium" : "saberstyle_strong";
        const HudTeil t = teil(d_, "righthud", name);
        teilBild(t, t.farbe);
    }

    void munition() {   // CG_DrawAmmo (nicht feuernd, ohne Aufnahme-Gelb)
        const float wert0 = static_cast<float>(w_.munition);
        if (wert0 < 0) { return; }
        const HudTeil t = teil(d_, "righthud", "ammoamount");
        const Farbe f = wert0 > 0 ? t.farbe : kRot;
        zahlenFeld(t.x, t.y, 3, w_.munition, t.w, t.h, f);
        striche("righthud", "ammo_tic", static_cast<float>(w_.munitionMax) / kHudTics, wert0, false);
    }

    void fadenkreuz() {   // CG_DrawCrosshair(NULL): kein Ziel - weiss
        if (w_.fadenkreuz == 0) { return; }
        const float b = w_.fadenkreuzGroesse;
        char name[32];
        std::snprintf(name, sizeof(name), "gfx/2d/crosshair%c", 'a' + (w_.fadenkreuz % 9));   // NUM_CROSSHAIRS
        bild(static_cast<float>(w_.fadenkreuzX) + 0.5F * (640.0F - b), static_cast<float>(w_.fadenkreuzY) + 0.5F * (480.0F - b), b, b, name, kWeiss);
    }

private:
    const Datei& d_;
    const HudWerte& w_;
    HudZustand& z_;
    std::vector<Prim>& out_;
};

}  // namespace

MenueItemInfo menueItemInfo(const Datei& d, const char* menue, const char* item) {
    MenueItemInfo t;
    const int m = menueNamens(d, menue);
    if (m < 0) { return t; }
    for (const Item& it : d.menus[static_cast<std::size_t>(m)].items) {
        if (!gleich(it.window.name, item)) { continue; }   // Menu_FindItemByName: das erste
        t.x = static_cast<int>(it.window.rect.x);
        t.y = static_cast<int>(it.window.rect.y);
        t.w = static_cast<int>(it.window.rect.w);
        t.h = static_cast<int>(it.window.rect.h);
        t.farbe = it.window.foreColor;
        t.bild = it.window.background;
        t.gefunden = true;
        break;
    }
    return t;
}

bool istHud(const Datei& d) { return menueNamens(d, "lefthud") >= 0 || menueNamens(d, "righthud") >= 0; }

bool hudNachgebildet(const Datei& d) { return d.dialekt == Dialekt::SP && d.spiel == Spiel::JA; }

void zeichneHud(const Datei& d, int aktuell, const Umgebung& u, const HudWerte& w, HudZustand& z, std::vector<Prim>& out) {
    Hud h(d, w, z, out);
    auto menue = [&](const char* name) {   // cgi_UI_Menu_Paint(cgi_UI_GetMenuByName(name), qtrue)
        const int m = menueNamens(d, name);
        if (m < 0) { return false; }
        const std::size_t ab = out.size();
        Umgebung uu = u;
        uu.erzwingen = true;
        zeichneMenu(d, m, uu, out);
        if (m != aktuell) {
            for (std::size_t i = ab; i < out.size(); ++i) { out[i].item = -1; }
        }
        return true;
    };
    if (!hudNachgebildet(d)) {
        (void)menue("lefthud");
        (void)menue("righthud");
        return;
    }
    if (menue("lefthud")) {
        h.teilBild(teil(d, "lefthud", "scanline"), teil(d, "lefthud", "scanline").farbe);
        h.teilBild(teil(d, "lefthud", "frame"), teil(d, "lefthud", "frame").farbe);
        h.ruestung();
        h.gesundheit();
    }
    if (menue("righthud")) {
        h.teilBild(teil(d, "righthud", "scanline"), teil(d, "righthud", "scanline").farbe);
        h.teilBild(teil(d, "righthud", "frame"), teil(d, "righthud", "frame").farbe);
        h.macht();
        if (w.saebel) { h.saebelStil(); }
        else { h.munition(); }
    }
    h.fadenkreuz();
}

void zeichneDatapadHud(const Datei& hud, const HudWerte& w, std::vector<Prim>& out) {
    HudZustand z;
    Hud h(hud, w, z, out);
    h.gesundheit();
    h.macht();
    h.munition();   // CG_DrawAmmo auch mit dem Saebel (ammo[AMMO_FORCE])
    h.bild(526 + 33, 286 + 41, 16, 16, "gfx/hud/message_off", kWeiss);   // ohne missionInfoFlashTime
    h.bild(0, 0, 640, 480, "gfx/menus/datapad", kWeiss);
}

void zeichneDatapadHudJk2(Spiel spiel, const HudWerte& w, std::vector<Prim>& out) {
    static const Datei leer{};
    HudZustand z;
    Hud h(leer, w, z, out);
    const bool md = spiel == Spiel::MD;
    const std::string dp = md ? "gfx/hud/DataPad" : "gfx/hud/";   // Movie Duels: eigene Datapad-Bilder
    // CG_DrawPic auch mit negativer Breite (RE_StretchPic spiegelt)
    auto bild = [&](int x, int y, int b, int hh, const std::string& shader, const Farbe& f) {
        Prim p;
        p.art = PrimArt::Bild;
        p.x = static_cast<float>(x);
        p.y = static_cast<float>(y);
        p.w = static_cast<float>(b);
        p.h = static_cast<float>(hh);
        p.farbe = f;
        p.shader = shader;
        out.push_back(p);
    };
    auto mal = [](Farbe f, float anteil) {   // calcColor[0..2] *= percent
        f.r *= anteil;
        f.g *= anteil;
        f.b *= anteil;
        return f;
    };
    constexpr Farbe kHudGruen{0.0F, 0.613F, 0.097F, 1}, kHudRot{0.835F, 0.015F, 0.015F, 1}, kHudOrange{1.0F, 0.658F, 0.062F, 1};
    constexpr Farbe kHellgrau{0.75F, 0.75F, 0.75F, 1}, kSchwarz{0, 0, 0, 1};
    // forceTicPos / ammoTicPos (cg_main.cpp): x, y, Breite, Hoehe, Bild; rechts mit negativer Breite
    struct Tic {
        int x, y, b, h;
        const char* bild;
    };
    static constexpr Tic kMacht[14] = {{11, 41, 20, 10, "force_tick1"},  {12, 45, 20, 10, "force_tick2"},  {14, 49, 20, 10, "force_tick3"},
                                       {17, 52, 20, 10, "force_tick4"},  {22, 55, 10, 10, "force_tick5"},  {28, 57, 10, 20, "force_tick6"},
                                       {34, 59, 10, 10, "force_tick7"},  {46, 59, -10, 10, "force_tick7"}, {52, 57, -10, 20, "force_tick6"},
                                       {58, 55, -10, 10, "force_tick5"}, {63, 52, -20, 10, "force_tick4"}, {66, 49, -20, 10, "force_tick3"},
                                       {68, 45, -20, 10, "force_tick2"}, {69, 41, -20, 10, "force_tick1"}};
    static constexpr Tic kMunition[14] = {{12, 34, 10, 10, "ammo_tick7-l"},  {13, 28, 10, 10, "ammo_tick6-l"},  {15, 23, 10, 10, "ammo_tick5-l"},
                                          {19, 19, 10, 10, "ammo_tick4-l"},  {23, 15, 10, 10, "ammo_tick3-l"},  {29, 12, 10, 10, "ammo_tick2-l"},
                                          {34, 11, 10, 10, "ammo_tick1-l"},  {47, 11, -10, 10, "ammo_tick1-r"}, {52, 12, -10, 10, "ammo_tick2-r"},
                                          {58, 15, -10, 10, "ammo_tick3-r"}, {62, 19, -10, 10, "ammo_tick4-r"}, {66, 23, -10, 10, "ammo_tick5-r"},
                                          {68, 28, -10, 10, "ammo_tick6-r"}, {69, 34, -10, 10, "ammo_tick7-r"}};
    constexpr int kTics = 14;   // MAX_TICS / MAX_DATAPADTICS
    int x = 34;
    const int y = 286;
    bild(x, y, 80, 80, "gfx/hud/hudleft_innerframe", kWeiss);   // CG_DrawHUDLeftFrame1
    if (md) { bild(x, y, 80, 80, "gfx/hud/hudleftframe", kWeiss); }   // CG_DrawHUDLeftFrame2
    {   // CG_DrawArmor (Movie Duels CG_DrawDataPadArmor)
        const float halb = static_cast<float>(w.gesundheitMax) / 2.0F;
        float anteil = (static_cast<float>(w.ruestung) - halb) / halb;
        if (anteil < 0) { anteil = 0; }
        bild(x, y, 80, 80, dp + "armor1", mal(kHudGruen, anteil));
        anteil = anteil > 0 ? 1.0F : static_cast<float>(w.ruestung) / halb;
        bild(x, y, 80, 80, dp + "armor2", mal(kHudGruen, anteil));
        h.zahlenFeld(x + 16 + 14, y + 40 + 14, 3, w.ruestung, 6, 12, kHudGruen);
    }
    {   // CG_DrawHealth (Movie Duels CG_DrawDatapadHealth); cg.HUDHealthFlag bleibt im Datapad aus
        const float anteil = static_cast<float>(w.gesundheit) / static_cast<float>(w.gesundheitMax);
        bild(x, y, 80, 80, dp + "health", mal(kHudRot, anteil));
        h.zahlenFeld(x + 16, y + 40, 3, w.gesundheit, 6, 12, kHudRot, true);
    }
    x = 526;
    bild(x, y, 80, 80, "gfx/hud/hudright_innerframe", kWeiss);   // CG_DrawHUDRightFrame1
    if (md) { bild(x, y, 80, 80, "gfx/hud/hudrightframe", kWeiss); }   // CG_DrawHUDRightFrame2
    if (w.machtBekannt) {   // CG_DrawForcePower (Movie Duels CG_DrawDatapadForcePower)
        const float max = static_cast<float>(w.machtMax);
        const float inc = max / kTics;
        float wert = static_cast<float>(w.macht);
        float extra = 0;
        if (wert > max) {   // supercharged
            extra = wert - max;
            wert = max;
        }
        for (int i = kTics - 1; i >= 0; --i) {
            Farbe f = kHellgrau;
            if (extra != 0) {
                f = mal(kWeiss, 0.75F + (std::sin(static_cast<float>(w.zeitMs) * 0.005F) * ((extra / max) * 0.25F)));
            } else if (wert <= 0) {
                f = kSchwarz;
            } else if (wert < inc) {
                f = mal(kHellgrau, wert / inc);
            }
            const Tic& t = kMacht[i];
            bild(x + t.x, y + t.y, t.b, t.h, dp + t.bild, f);
            wert -= inc;
        }
    }
    if (w.saebel) {   // CG_DrawAmmo mit dem Saebel: der Stil statt der Munition
        static constexpr const char* kJk2Stile[8] = {"", "saber_stylesFast", "saber_stylesMed", "saber_stylesStrong", "saber_stylesStrong",
                                                     "saber_stylesFast", "", ""};
        static constexpr const char* kMdStile[8] = {"", "saber_stylesfast", "saber_stylesmed", "saber_stylesstrong", "saber_stylesDesann",
                                                    "saber_styletavion", "saber_stylesDuels", "saber_stylesStaff"};
        const int s = w.saebelStil >= 0 && w.saebelStil < 8 ? w.saebelStil : 0;
        const char* stil = md ? kMdStile[s] : kJk2Stile[s];
        if (*stil != 0) { bild(x, y, 80, 40, dp + stil, kWeiss); }
    } else if (w.munition >= 0) {   // CG_DrawAmmo (nicht feuernd, ohne Aufnahme-Gelb)
        h.zahlenFeld(x + 29, y + 26, 3, w.munition, 6, 12, w.munition > 0 ? kHudOrange : kRot);
        const float inc = static_cast<float>(w.munitionMax) / kTics;
        float wert = static_cast<float>(w.munition);
        for (int i = kTics - 1; i >= 0; --i) {
            const Farbe f = wert <= 0 ? kSchwarz : wert < inc ? mal(kWeiss, wert / inc) : kWeiss;
            const Tic& t = kMunition[i];
            bild(x + t.x, y + t.y, t.b, t.h, dp + t.bild, f);
            wert -= inc;
        }
    }
    bild(x + 33, y + 41, 16, 16, "gfx/hud/message_off", kWeiss);   // CG_DrawMessageLit ohne missionInfoFlashTime
    bild(0, 0, 640, 480, "gfx/hud/datapad2", kWeiss);              // cgs.media.dataPadFrame
}

}  // namespace bhed::menue
