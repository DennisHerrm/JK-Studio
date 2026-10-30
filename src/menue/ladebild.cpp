// ladebild.cpp - Menue-Editor M7 Punkt 11: Lade- und Verbindungsmenues mit Beispielwerten
// (Portiert aus OpenJK, Einzelheiten und Lizenz im Kopf von include/bhed/menue/ladebild.h).
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
#include "bhed/menue/ladebild.h"

#include "bhed/menue/hud.h"

#include <algorithm>
#include <cctype>

namespace bhed::menue {

namespace {

constexpr Farbe kWeiss{1, 1, 1, 1};
constexpr int kProReihe = 8;     // MAXLOADICONSPERROW
constexpr int kMaxWaffen = 16;   // MAXLOADWEAPONS
constexpr int kKraftGroesse = 40, kKraftAbstand = 12;   // MAXLOAD_FORCEICONSIZE / _FORCEICONPAD
// cg_main.cpp showPowers (FP der SP-Fassung)
constexpr int kShowPowers[12] = {13, 0, 12, 5, 2, 3, 4, 15, 14, 7, 11, 6};

int menueNr(const Datei& d, const char* name) {   // Menus_FindByName
    for (std::size_t i = 0; i < d.menus.size(); ++i) {
        const std::string& n = d.menus[i].window.name;
        if (n.size() != std::char_traits<char>::length(name)) { continue; }
        bool gleich = true;
        for (std::size_t j = 0; j < n.size() && gleich; ++j) {
            gleich = std::tolower(static_cast<unsigned char>(n[j])) == std::tolower(static_cast<unsigned char>(name[j]));
        }
        if (gleich) { return static_cast<int>(i); }
    }
    return -1;
}

// cgi_UI_Menu_Paint(menu, qtrue); Zeichenbefehle anderer Menues als aktuell tragen item -1
void menuePaint(const Datei& d, int m, int aktuell, const Umgebung& u, std::vector<Prim>& out) {
    if (m < 0) { return; }
    const std::size_t ab = out.size();
    Umgebung uu = u;
    uu.erzwingen = true;
    zeichneMenu(d, m, uu, out);
    if (m != aktuell) {
        for (std::size_t i = ab; i < out.size(); ++i) { out[i].item = -1; }
    }
}

OdTeil bild(int x, int y, int w, int h, const std::string& s, const Farbe& c) {
    OdTeil t;
    t.art = OdTeil::Art::Bild;
    t.x = static_cast<float>(x);
    t.y = static_cast<float>(y);
    t.w = static_cast<float>(w);
    t.h = static_cast<float>(h);
    t.inhalt = s;
    t.farbe = c;
    return t;
}
OdTeil text(int x, int y, const std::string& s, const Farbe& c, const std::string& schrift, int stil = 0, int font = 0) {
    OdTeil t;
    t.x = static_cast<float>(x);
    t.y = static_cast<float>(y);
    t.inhalt = s;
    t.farbe = c;
    t.schrift = schrift;
    t.stil = stil;
    t.font = font;
    return t;
}

int breite(const Umgebung& u, const std::string& t, float sc, const std::string& name, int font) {
    const Schrift* f = !name.empty() ? (u.schriftNamens ? u.schriftNamens(name) : nullptr) : (u.schrift ? u.schrift(font) : nullptr);
    return f != nullptr ? schriftBreite(*f, t, sc, u.kodierung, u.asienGlyphen, u.jk2) : 0;
}

}  // namespace

bool istLadebild(const Datei& d) { return d.dialekt == Dialekt::SP && menueNr(d, "loadscreen") >= 0; }
bool istVerbindung(const Datei& d) { return d.dialekt == Dialekt::MP && menueNr(d, "Connect") >= 0; }

void zeichneLadebild(const Datei& d, int aktuell, const Umgebung& u0, const LadeWerte& w, const LadeKontext& k, std::vector<Prim>& out) {
    // CG_DrawLoadingScreen setzt ui_missionbriefing, bevor das Menue gezeichnet wird
    const std::string briefing = k.se && !k.se("BRIEFINGS_" + w.karte).empty() ? "@BRIEFINGS_" + w.karte : std::string("@BRIEFINGS_NONE");
    Umgebung u = u0;
    u.cvar = [alt = u0.cvar, briefing](const std::string& n) {
        std::string k2 = n;
        for (char& c : k2) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return k2 == "ui_missionbriefing" ? briefing : (alt ? alt(n) : std::string());
    };
    std::vector<OdTeil> t;
    std::string levelshot = "levelshots/" + w.karte;
    if (!k.bildDa || !k.bildDa(levelshot)) { levelshot = "menu/art/unknownmap"; }
    if (w.ersteKarte) {   // yavin1 ohne geladenen Spielstand
        OdTeil schwarz;   // CG_DrawPic(0, 0, 640, 480, whiteShader) mit CT_BLACK
        schwarz.art = OdTeil::Art::Fuellen;
        schwarz.w = 640;
        schwarz.h = 480;
        schwarz.farbe = Farbe{0, 0, 0, 1};
        t.push_back(schwarz);
        const std::string s = k.se ? k.se("SP_INGAME_ALONGTIME") : std::string();
        t.push_back(text(320 - breite(u, s, 1.0F, "ergoec", 0) / 2, 140, s, Farbe{0.567F, 0.685F, 1.0F, 0.75F}, "ergoec"));
        zeichneTeile(t, u, out);
        t.clear();
    } else {
        const MenueItemInfo hg = menueItemInfo(d, "loadScreen", "background");
        if (hg.gefunden) { t.push_back(bild(hg.x, hg.y, hg.w, hg.h, hg.bild, hg.farbe)); }
        const MenueItemInfo kb = menueItemInfo(d, "loadScreen", "mappic");
        if (kb.gefunden) { t.push_back(bild(kb.x, kb.y, kb.w, kb.h, levelshot, kb.farbe)); }
        // CG_DrawLoadWeapons (playersave: STAT_WEAPONS)
        const unsigned waffen = k.spieler.waffen;
        int anzahl = 0;
        for (int i = 1; i < kMaxWaffen; ++i) { anzahl += (waffen & (1U << i)) != 0 ? 1 : 0; }
        auto waffenReihe = [&](const char* item, int reihe, int start) {   // CG_DrawLoadWeaponsPrintRow
            const MenueItemInfo r = menueItemInfo(d, "loadScreen", item);
            if (!r.gefunden) { return 0; }
            constexpr int groesse = 60, pad = 12;
            int x = r.x + (r.w - (groesse * reihe + pad * (reihe - 1))) / 2;
            int ende = 0, gezeigt = 0;
            for (int i = start; i < kMaxWaffen; ++i) {
                if ((waffen & (1U << i)) == 0) { continue; }
                const std::string ic = static_cast<std::size_t>(i) < k.waffenIcon.size() ? k.waffenIcon[static_cast<std::size_t>(i)] : std::string();
                if (ic.empty()) { continue; }
                ende = i;
                t.push_back(bild(x, r.y, groesse, groesse, ic, r.farbe));
                if (++gezeigt == kProReihe) { break; }
                x += groesse + pad;
            }
            return ende;
        };
        if (anzahl > 0) {
            if (anzahl <= kProReihe) {
                waffenReihe("weaponicons_singlerow", anzahl, 0);
            } else {
                const int e = waffenReihe("weaponicons_row1", kProReihe, 0);
                waffenReihe("weaponicons_row2", anzahl - kProReihe, e + 1);
            }
        }
        // CG_DrawLoadForcePowers (forcePowersKnown, playerfplvl)
        auto gueltig = [&](int i) { return k.spieler.kraft[static_cast<std::size_t>(kShowPowers[i])] > 0; };
        int kraefte = 0;
        for (int i = 0; i < 12; ++i) { kraefte += gueltig(i) ? 1 : 0; }
        auto kraftReihe = [&](const char* item, int reihe, int start) {   // CG_DrawLoadForcePrintRow
            const MenueItemInfo r = menueItemInfo(d, "loadScreen", item);
            if (!r.gefunden) { return 0; }
            int x = r.x + (r.w - (kKraftGroesse * reihe + kKraftAbstand * (reihe - 1))) / 2;
            int ende = 0, gezeigt = 0;
            for (int i = start; i < 12; ++i) {
                if (!gueltig(i)) { continue; }
                ende = i;
                t.push_back(bild(x, r.y, kKraftGroesse, kKraftGroesse, kraftSymbol(kShowPowers[i]), r.farbe));
                if (++gezeigt == kProReihe) { break; }
                x += kKraftGroesse + kKraftAbstand;
            }
            return ende;
        };
        if (kraefte > 0) {
            if (kraefte <= kProReihe) {
                kraftReihe("forceicons_singlerow", kraefte, 0);
            } else {
                const int e = kraftReihe("forceicons_row1", kProReihe, 0);
                kraftReihe("forceicons_row2", kraefte - kProReihe, e + 1);
            }
        }
        zeichneTeile(t, u, out);
        t.clear();
        menuePaint(d, menueNr(d, "loadscreen"), aktuell, u, out);
    }
    // CG_LoadBar
    constexpr int ticks = 9, tw = 40, th = 8, padx = 20, pady = 12, cap = 8;
    constexpr int bw = ticks * tw + padx * 2 + cap * 2, bl = (640 - bw) / 2, bh = th + pady * 2, bt = 475 - bh;
    constexpr int capl = bl + padx, tl = capl + cap, tt = bt + pady;
    const int stufe = std::clamp(w.stufe, 0, ticks);
    t.push_back(bild(bl, bt, bw, bh, "gfx/hud/mp_levelload", kWeiss));
    t.push_back(bild(tl, tt, -cap, th, "gfx/hud/load_tick_cap", kWeiss));
    t.push_back(bild(tl, tt, tw * stufe, th, "gfx/hud/load_tick", kWeiss));
    t.push_back(bild(tl + tw * stufe, tt, cap, th, "gfx/hud/load_tick_cap", kWeiss));
    // CS_MESSAGE in Anfuehrungszeichen bei (15, 20)
    if (!w.nachricht.empty()) {
        const std::string n = w.nachricht[0] == '@' && k.se ? k.se(w.nachricht.substr(1)) : w.nachricht;
        t.push_back(text(15, 20, "\"" + n + "\"", kWeiss, "ergoec"));
    }
    zeichneTeile(t, u, out);
}

void zeichneVerbindung(const Datei& d, int aktuell, const Umgebung& u, const VerbindungsWerte& w,
                       const std::function<std::string(const std::string&)>& se, std::vector<Prim>& out) {
    menuePaint(d, menueNr(d, "Connect"), aktuell, u, out);
    constexpr int kMittel = 2;   // FONT_MEDIUM
    constexpr int kSchatten = 6;   // ITEM_TEXTSTYLE_SHADOWEDMORE (Text_PaintCenter nimmt immer diesen)
    const float mitte = 320, y0 = 130;
    std::vector<OdTeil> t;
    auto mittig = [&](float y, const std::string& s) {   // Text_PaintCenter
        const int len = breite(u, s, 1.0F, {}, kMittel);
        t.push_back(text(static_cast<int>(mitte - static_cast<float>(len / 2)), static_cast<int>(y), s, kWeiss, {}, kSchatten, kMittel));
    };
    auto ersetze = [](std::string s, const std::string& wert) {   // va(text, %s / %i)
        for (const char* f : {"%s", "%i", "%d"}) {
            const std::size_t p = s.find(f);
            if (p != std::string::npos) {
                s.replace(p, 2, wert);
                break;
            }
        }
        return s;
    };
    auto tx = [&](const char* key) { return se ? se(key) : std::string(key); };
    mittig(y0, ersetze(tx("MENUS_LOADING_MAPNAME"), w.karte));
    const bool lokal = w.server == "localhost";
    if (lokal) {
        mittig(y0 + 48, tx("MENUS_STARTING_UP"));
    } else {
        std::string s = ersetze(tx("MENUS_CONNECTING_TO"), w.server);
        if (s.size() > 255) { s.resize(255); }
        mittig(y0 + 48, s);
    }
    mittig(425, w.motd);
    if (w.zustand < 3) { mittig(y0 + 176, w.meldung); }   // connState < CA_CONNECTED
    std::string s;
    if (w.zustand == 1) { s = ersetze(tx("MENUS_AWAITING_CONNECTION"), std::to_string(w.pakete)); }
    else if (w.zustand == 2) { s = ersetze(tx("MENUS_AWAITING_CHALLENGE"), std::to_string(w.pakete)); }
    else { s = tx("MENUS_AWAITING_GAMESTATE"); }   // ohne cl_downloadName
    if (!lokal) { mittig(y0 + 80, s); }
    zeichneTeile(t, u, out);
}

}  // namespace bhed::menue
