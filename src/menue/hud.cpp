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

// HUDMenuItem_t nach cgi_UI_GetMenuItemInfo (CG_UI_GETITEMINFO): Lage als int, foreColor,
// background; nicht gefunden = alles 0 (die statische Vorbelegung) - zeichnet nichts.
struct HudTeil {
    int x = 0, y = 0, w = 0, h = 0;
    Farbe farbe{0, 0, 0, 0};
    std::string bild;
};
HudTeil teil(const Datei& d, const char* menue, const char* item) {
    HudTeil t;
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
        break;
    }
    return t;
}

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

    // CG_DrawNumField(x, y, 3, value, charWidth, charHeight, NUM_FONT_SMALL, qfalse)
    void zahlenFeld(int x, int y, int width, int value, int charWidth, int charHeight, const Farbe& f) {
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
        x += 2 + xWidth * (width - l);
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

}  // namespace bhed::menue
