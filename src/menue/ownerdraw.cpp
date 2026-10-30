// ownerdraw.cpp - Menue-Editor M7 Punkt 14: UI_OwnerDraw je Nummer (Herkunft und Lizenz im Kopf
// von include/bhed/menue/ownerdraw.h).
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
#include "bhed/menue/ownerdraw.h"

#include "bhed/menue/feeder.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace bhed::menue {

namespace {

constexpr int kNumForcePowers = 18;    // NUM_FORCE_POWERS
constexpr int kMaxForceRank = 7;       // MAX_FORCE_RANK
constexpr int kForceLevels = 4;        // NUM_FORCE_POWER_LEVELS
constexpr int FP_LEVITATION = 1, FP_SABER_OFFENSE = 15, FP_SABER_DEFENSE = 16, FP_SABERTHROW = 17;
constexpr float kPulseDivisor = 75.0F;   // PULSE_DIVISOR

// bg_misc.c
constexpr const char* kForceMasteryLevels[kMaxForceRank + 1] = {"MASTERY0", "MASTERY1", "MASTERY2", "MASTERY3",
                                                                "MASTERY4", "MASTERY5", "MASTERY6", "MASTERY7"};
constexpr int kForceMasteryPoints[kMaxForceRank + 1] = {0, 5, 10, 20, 30, 50, 75, 100};
constexpr int kForcePowerCost[kNumForcePowers][kForceLevels] = {
    {0, 2, 4, 6}, {0, 0, 2, 6}, {0, 2, 4, 6}, {0, 1, 3, 6}, {0, 1, 3, 6}, {0, 4, 6, 8}, {0, 1, 3, 6}, {0, 2, 5, 8}, {0, 4, 6, 8},
    {0, 2, 5, 8}, {0, 1, 3, 6}, {0, 1, 3, 6}, {0, 1, 3, 6}, {0, 2, 4, 6}, {0, 2, 5, 8}, {0, 1, 5, 8}, {0, 1, 5, 8}, {0, 4, 6, 8}};
// ui_force.c uiForcePowerDarkLight (1 hell, 2 dunkel)
constexpr int kForceDarkLight[kNumForcePowers] = {1, 0, 0, 0, 0, 1, 2, 2, 2, 1, 1, 1, 2, 2, 0, 0, 0, 0};
// codemp/ui/ui_main.c skillLevels
constexpr const char* kSkillLevels[5] = {"SKILL1", "SKILL2", "SKILL3", "SKILL4", "SKILL5"};

// codemp/ui/menudef.h
enum MpOd {
    UI_EFFECTS = 201, UI_GAMETYPE = 205, UI_SKILL = 207, UI_NETSOURCE = 220, UI_NETFILTER = 222, UI_ALLMAPS_SELECTION = 236,
    UI_BOTNAME = 239, UI_BOTSKILL = 240, UI_REDBLUE = 241, UI_SELECTEDPLAYER = 243, UI_MAPCINEMATIC = 244,
    UI_NETGAMETYPE = 245, UI_SERVERREFRESHDATE = 247, UI_SERVERMOTD = 248, UI_GLINFO = 249, UI_KEYBINDSTATUS = 250,
    UI_JOINGAMETYPE = 253, UI_STARTMAPCINEMATIC = 255, UI_FORCE_SIDE = 257, UI_FORCE_RANK = 258, UI_FORCE_RANK_SABERTHROW = 276,
    UI_FORCE_MASTERY_SET = 286, UI_SKIN_COLOR = 287, UI_FORCE_POINTS = 288, UI_JEDI_NONJEDI = 289,
    UI_CHAT_MAIN = 291, UI_CHAT_TACTICAL = 297
};
// code/ui/menudef.h (Einzelspieler)
enum SpOd { SP_EFFECTS = 201, SP_ALLMAPS_SELECTION = 236, SP_GLINFO = 249, SP_KEYBINDSTATUS = 250 };

int zahl(const OdKontext& k, const std::string& cv) { return k.cvar ? std::atoi(k.cvar(cv).c_str()) : 0; }
std::string se(const OdKontext& k, const std::string& key) { return k.se ? k.se(key) : key; }

struct Zeichner {
    const OdAufruf& a;
    const OdKontext& k;
    OdErgebnis& r;

    // Text_Paint(x, y, scale, color, text, limit, style, font)
    void text(float x, float y, const Farbe& c, const std::string& t, int limit = 0, int stil = -1) {
        OdTeil p;
        p.art = OdTeil::Art::Text;
        p.x = x;
        p.y = y;
        p.inhalt = t;
        p.farbe = c;
        p.scale = a.scale;
        p.stil = stil < 0 ? a.stil : stil;
        p.maxPixel = limit;
        p.font = a.font;
        r.teile.push_back(std::move(p));
    }
    // UI_DrawHandlePic
    void bild(float x, float y, float w, float h, const std::string& shader, const Farbe& c = Farbe{1, 1, 1, 1}) {
        OdTeil p;
        p.art = OdTeil::Art::Bild;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.inhalt = shader;
        p.farbe = c;
        r.teile.push_back(std::move(p));
    }
};

// UI_DrawGLInfo (codemp: zwei Spalten ab y + 45, Zeilen 11; code/ui: eine Spalte, 16, und die
// Zeilenzahl bis rect->h vorher gezaehlt)
void glInfo(Zeichner& z, float x, float y, float w, float h) {
    const GlDaten g = (z.k.gl != nullptr && z.k.gl->echt) ? *z.k.gl : glBeispiel();
    z.r.beispiel = !g.echt;
    // Q_strncpyz(buff, extensions_string, 4096); MP kuerzt vorher auf 128 Erweiterungen
    std::string ext = g.extensions;
    if (z.k.dialekt == Dialekt::MP) {   // TruncateGLExtensionsString(..., 128)
        std::size_t p = 0;
        int n = 0;
        std::size_t q = 0;
        while ((q = ext.find(' ', p)) != std::string::npos && n < 128) {
            p = q + 1;
            ++n;
        }
        if (q != std::string::npos) { ext.resize(p - 1); }
    }
    if (ext.size() > 4095) { ext.resize(4095); }
    std::vector<std::string> namen;
    {
        std::size_t p = 0;
        while (p < ext.size()) {
            while (p < ext.size() && ext[p] == ' ') { ++p; }
            const std::size_t e = ext.find(' ', p);
            if (p < ext.size()) { namen.push_back(ext.substr(p, e == std::string::npos ? std::string::npos : e - p)); }
            p = e == std::string::npos ? ext.size() : e;
        }
    }
    const Farbe c = z.a.farbe;
    if (z.k.dialekt == Dialekt::MP) {
        const int lw = static_cast<int>(w);
        z.text(x + 2, y, c, "GL_VENDOR: " + g.vendor, lw);
        z.text(x + 2, y + 15, c, "GL_VERSION: " + g.version + ": " + g.renderer, lw);
        z.text(x + 2, y + 30, c,
               "GL_PIXELFORMAT: color(" + std::to_string(g.colorBits) + "-bits) Z(" + std::to_string(g.depthBits) + "-bits) stencil(" +
                   std::to_string(g.stencilBits) + "-bits)",
               lw);
        int yy = static_cast<int>(y) + 45;
        const int halb = static_cast<int>(w) / 2;   // rect->w / 2 (float -> int beim Aufruf)
        for (std::size_t i = 0; i < namen.size() && static_cast<float>(yy) < y + h; ++i) {
            if (i % 2 == 0) {
                z.text(x + 2, static_cast<float>(yy), c, namen[i], halb);
            } else {
                z.text(x + w / 2, static_cast<float>(yy), c, namen[i], halb);
                yy += 11;
            }
        }
        return;
    }
    const int lw = static_cast<int>(w);
    int yy = static_cast<int>(y);
    z.text(x, static_cast<float>(yy), c, "GL_VENDOR: " + g.vendor, lw);
    yy += 15;
    z.text(x, static_cast<float>(yy), c, "GL_VERSION: " + g.version + ": " + g.renderer, lw);
    yy += 15;
    z.text(x, static_cast<float>(yy), c, "GL_PIXELFORMAT:", lw);
    yy += 15;
    z.text(x, static_cast<float>(yy),
           c, "Color(" + std::to_string(g.colorBits) + "-bits) Z(" + std::to_string(g.depthBits) + "-bits) stencil(" +
                  std::to_string(g.stencilBits) + "-bits)",
           lw);
    yy += 15;
    // testy = y - 16; while (testy <= rect->y + rect->h && *eptr && numLines < MAX_LINES) ... numLines--
    int testy = yy - 16;
    int numLines = 0;
    for (std::size_t i = 0; i < namen.size() && static_cast<float>(testy) <= y + h && numLines < 64; ++i) {
        ++numLines;
        testy += 16;
    }
    --numLines;
    for (int i = 0; i < numLines; ++i) {
        z.text(x, static_cast<float>(yy), c, namen[static_cast<std::size_t>(i)], lw);
        yy += 16;
    }
}

// UI_GetGameTypeName (codemp/ui/ui_main.c)
std::string spieltypName(int gt) {
    switch (gt) {
        case 0: return "MENUS_FREE_FOR_ALL";
        case 1: return "MENUS_HOLOCRON_FFA";
        case 3: return "MENUS_DUEL";
        case 4: return "MENUS_POWERDUEL";
        case 6: return "MENUS_TEAM_FFA";
        case 7: return "MENUS_SIEGE";
        case 8: return "MENUS_CAPTURE_THE_FLAG";
        case 9: return "MENUS_CAPTURE_THE_YSALIMARI";
        default: return "MENUS_SAGA";   // GT_JEDIMASTER, GT_SINGLE_PLAYER und sonst
    }
}

// GetCRDelineatedString: Zeile iIndex eines StringEd-Texts (NULL ausserhalb)
bool zeileAus(const std::string& liste, int index, std::string& out) {
    std::size_t p = 0;
    while (index-- > 0) {
        p = liste.find('\n', p);
        if (p == std::string::npos) { return false; }
        ++p;
    }
    out = liste.substr(p, std::min<std::size_t>(255, liste.size() - p));
    const std::size_t n = out.find('\n');
    if (n != std::string::npos) { out.resize(n); }
    return true;
}

// UI_TrueJediEnabled mit leerem bzw. Beispiel-CS_SERVERINFO
bool trueJedi(const Spielbeispiel& b) { return b.jediVmerc; }

void mp(Zeichner& z, OdErgebnis& r) {
    const OdAufruf& a = z.a;
    const OdKontext& k = z.k;
    const FeederDaten* fd = k.daten;
    const Spielbeispiel bsp = k.beispiel != nullptr ? *k.beispiel : Spielbeispiel{};
    const float x = a.x + a.textX;
    const float y = a.y + a.textY;
    const Farbe c = a.farbe;
    // uiInfo.gameTypes[i].gtEnum
    auto gtEnum = [&](const std::vector<std::pair<std::string, int>>& liste, const char* cv, bool joinGrenze) {
        int i = zahl(k, cv);
        const int n = static_cast<int>(liste.size());
        if (joinGrenze ? (i < 0 || i > n) : (i < 0 || i >= n)) { i = 0; }   // UI_DrawJoinGameType: "> num"
        return (i >= 0 && i < n) ? liste[static_cast<std::size_t>(i)].second : 0;
    };
    const MpKraft kraft = mpKraft(k.cvar ? k.cvar("forcepowers") : std::string(), zahl(k, "ui_freeSaber") != 0, bsp);
    const int id = a.id;
    r.bekannt = true;
    if (id == UI_SKIN_COLOR) {   // uiSkinColor = TEAM_FREE
        z.text(x, y, c, se(k, "MENUS_DEFAULT"));
    } else if (id == UI_FORCE_SIDE) {   // UI_DrawForceSide
        Farbe f = c;
        if (bsp.forceBasedTeams) {
            const int team = zahl(k, "ui_myteam");
            if (team == 1 || team == 2) { f = Farbe{0.2F, 0.2F, 0.2F, c.a}; }
        }
        const bool hell = kraft.seite == 1;
        for (const char* m : {"forcealloc", "ingame_playerforce"}) {
            if (hell) {
                r.zeigen.push_back({m, "lightpowers", true});
                r.zeigen.push_back({m, "darkpowers", false});
                r.zeigen.push_back({m, "darkpowers_team", false});
                r.zeigen.push_back({m, "lightpowers_team", true});
            } else {
                r.zeigen.push_back({m, "lightpowers", false});
                r.zeigen.push_back({m, "lightpowers_team", false});
                r.zeigen.push_back({m, "darkpowers", true});
                r.zeigen.push_back({m, "darkpowers_team", true});
            }
        }
        z.text(x, y, f, se(k, hell ? "MENUS_FORCEDESC_LIGHT" : "MENUS_FORCEDESC_DARK"));
        r.beispiel = true;
    } else if (id == UI_JEDI_NONJEDI) {   // UI_DrawJediNonJedi: ohne g_jediVmerc nichts
        if (trueJedi(bsp)) { z.text(x, y, c, se(k, kraft.jediNonJedi == 0 ? "MENUS_NO" : "MENUS_YES")); }
        r.beispiel = true;
    } else if (id == UI_FORCE_POINTS) {   // UI_DrawGenericNum(uiForceAvailable): druckt val
        z.text(x, y, c, std::to_string(kraft.verfuegbar));
        r.beispiel = true;
    } else if (id == UI_FORCE_MASTERY_SET || id == UI_FORCE_RANK) {   // UI_DrawForceMastery
        const int i = std::clamp(kraft.rang, 0, kMaxForceRank);
        z.text(x, y, c, se(k, std::string("MP_INGAME_") + kForceMasteryLevels[i]));
        r.beispiel = id == UI_FORCE_RANK;
    } else if (id > UI_FORCE_RANK && id <= UI_FORCE_RANK_SABERTHROW) {   // UI_DrawForceStars
        const int findex = id - UI_FORCE_RANK - 1;
        int val = kraft.raenge[static_cast<std::size_t>(findex)];
        if (val < 0 || val > kForceLevels - 1) { val = 0; }
        float xPos = x;
        for (int i = 1; i <= kForceLevels - 1; ++i) {
            const int stern = kForcePowerCost[findex][i];
            // uiForceStarShaders[0][*] = forcestar0, sonst forcecircle<n> / forcestar<n>
            const std::string bild = stern == 0 ? std::string("forcestar0") : (val >= i ? "forcestar" : "forcecircle") + std::to_string(stern);
            z.bild(xPos, y + 6, 16, 16, bild);
            xPos += 16 + 4;
        }
        r.beispiel = true;
    } else if (id == UI_GAMETYPE || id == UI_NETGAMETYPE) {
        const std::vector<std::pair<std::string, int>> leer;
        z.text(x, y, c, se(k, spieltypName(gtEnum(fd != nullptr ? fd->spieltypen : leer, id == UI_GAMETYPE ? "ui_gametype" : "ui_netGametype", false))));
    } else if (id == UI_JOINGAMETYPE) {
        const std::vector<std::pair<std::string, int>> leer;
        z.text(x, y, c, se(k, spieltypName(gtEnum(fd != nullptr ? fd->joinSpieltypen : leer, "ui_joinGametype", true))));
    } else if (id == UI_SKILL) {   // UI_DrawSkill: g_spSkill 1..5
        int i = zahl(k, "g_spSkill");
        if (i < 1 || i > 5) { i = 1; }
        z.text(x, y, c, se(k, "MP_INGAME_SKILL" + std::to_string(i)));
    } else if ((id >= 210 && id <= 219) || (id >= 280 && id <= 285)) {   // UI_DrawTeamMember
        const bool blau = (id >= 210 && id <= 214) || (id >= 280 && id <= 282);
        const int num = id <= 214 ? id - 209 : id <= 219 ? id - 214 : id <= 282 ? id - 274 : id - 277;
        int value = zahl(k, std::string(blau ? "ui_blueteam" : "ui_redteam") + std::to_string(num));
        const int maxcl = zahl(k, "sv_maxClients");
        const int numval = num * 2 - (blau ? 1 : 0);
        Farbe f = c;
        if (numval > maxcl) {
            f = Farbe{c.r * 0.5F, c.g * 0.5F, c.b * 0.5F, c.a};
            value = -1;
        }
        const std::vector<std::pair<std::string, int>> leer;
        if (gtEnum(fd != nullptr ? fd->spieltypen : leer, "ui_netGametype", false) == 7 && value > 1) { value = 1; }   // GT_SIEGE
        std::string t;
        if (value <= 1) {
            t = se(k, value == -1 ? "MENUS_CLOSED" : "MENUS_HUMAN");
        } else {
            value -= 2;
            const int nb = fd != nullptr ? static_cast<int>(fd->bots.size()) : 0;
            if (value >= nb) { value = 1; }
            t = (value >= 0 && value < nb) ? fd->bots[static_cast<std::size_t>(value)] : std::string("Kyle");
        }
        z.text(x, y, f, t);
    } else if (id == UI_MAPCINEMATIC || id == UI_STARTMAPCINEMATIC) {   // UI_DrawMapCinematic -> UI_DrawMapPreview
        const std::size_t anzahl = fd != nullptr ? fd->kartenDatei.size() : 0;
        int map = zahl(k, id == UI_STARTMAPCINEMATIC ? "ui_currentNetMap" : "ui_currentMap");
        if (map < 0 || static_cast<std::size_t>(map) > anzahl) { map = 0; }
        std::string bild = static_cast<std::size_t>(map) < anzahl ? "levelshots/" + fd->kartenDatei[static_cast<std::size_t>(map)] : std::string();
        if (bild.empty() || !k.bildDa || !k.bildDa(bild)) { bild = "menu/art/unknownmap_mp"; }
        z.bild(a.x + a.textX, a.y + a.textY, a.w, a.h, bild);
    } else if (id == UI_ALLMAPS_SELECTION) {   // UI_DrawAllMapsSelection(net = qtrue)
        const int map = zahl(k, "ui_currentNetMap");
        if (fd != nullptr && map >= 0 && static_cast<std::size_t>(map) < fd->karten.size()) {
            z.text(x, y, c, fd->karten[static_cast<std::size_t>(map)].first);
        }
    } else if (id == UI_NETSOURCE) {   // UI_DrawNetSource
        int src = zahl(k, "ui_netSource");
        if (src < 0 || src >= 7) { src = 0; }   // numNetSources = 7
        const int lan = src == 0 ? 0 : src <= 5 ? 1 : 2;   // UI_SourceForLAN
        std::string name;
        if (!zeileAus(se(k, "MP_INGAME_NET_SOURCES"), lan, name)) { name.clear(); }
        if (src >= 1 && src <= 5) { name += " " + std::to_string(src); }
        z.text(x, y, c, se(k, "MENUS_SOURCE") + " " + name);
    } else if (id == UI_NETFILTER) {   // UI_DrawNetFilter / UI_FilterDescription
        const int v = zahl(k, "ui_serverFilterType");
        const int n = fd != nullptr ? static_cast<int>(fd->mods.size()) : 0;
        std::string d = se(k, "MENUS_ALL");
        if (d.size() > 14) { d.resize(14); }   // char sAll[15]
        if (v > 0 && v <= n) { d = fd->mods[static_cast<std::size_t>(v - 1)].second; }
        z.text(x, y, c, se(k, "MENUS_GAME") + " " + d);
    } else if (id == UI_BOTNAME) {   // UI_DrawBotName: uiInfo.botIndex = 0
        z.text(x, y, c, fd != nullptr && !fd->bots.empty() ? fd->bots.front() : std::string());
    } else if (id == UI_BOTSKILL) {   // uiInfo.skillIndex = 0
        z.text(x, y, c, se(k, std::string("MP_INGAME_") + kSkillLevels[0]));
    } else if (id == UI_REDBLUE) {   // uiInfo.redBlue = 0
        z.text(x, y, c, se(k, "MP_INGAME_RED"));
    } else if (id == UI_SELECTEDPLAYER) {
        z.text(x, y, c, k.cvar ? k.cvar("cg_selectedPlayerName") : std::string());
    } else if (id == UI_SERVERREFRESHDATE) {
        if (bsp.serverRefresh) {
            const Farbe tief{0.8F * c.r, 0.8F * c.g, 0.8F * c.b, 0.8F * c.a};
            const float t = 0.5F + 0.5F * std::sin(static_cast<float>(k.zeitMs) / kPulseDivisor);
            const Farbe f{c.r + t * (tief.r - c.r), c.g + t * (tief.g - c.g), c.b + t * (tief.b - c.b), c.a + t * (tief.a - c.a)};
            std::string s = se(k, "MP_INGAME_GETTINGINFOFORSERVERS");
            const std::size_t p = s.find("%d");
            if (p != std::string::npos) { s.replace(p, 2, std::to_string(bsp.serverAnzahl)); }
            z.text(x, y, f, s);
            r.beispiel = true;
        } else {
            std::string buff = k.cvar ? k.cvar("ui_lastServerRefresh_" + std::to_string(zahl(k, "ui_netSource"))) : std::string();
            if (buff.size() > 63) { buff.resize(63); }
            z.text(x, y, c, se(k, "MP_INGAME_SERVER_REFRESHTIME") + ": " + buff);
        }
    } else if (id == UI_SERVERMOTD) {   // motdLen 0: nichts; mit Beispieltext der erste Schritt (motdPaintX = x + 1)
        if (!bsp.motd.empty()) { z.text(a.x + 1, a.y + a.h - 3, c, bsp.motd, static_cast<int>(a.w - 3), 0); }
        r.beispiel = true;
    } else if (id == UI_GLINFO) {
        glInfo(z, x, y, a.w, a.h);
    } else if (id == UI_KEYBINDSTATUS) {
        // Display_KeyBindPending: ohne wartende Taste nichts
    } else if (id == UI_EFFECTS) {
        // JKA MP: "case UI_EFFECTS: break;". Jedi Outcast MP (ui.qvm, Q3TA-Erbe): UI_DrawEffects mit
        // fxBasePic und fxPic[effectsColor], effectsColor = gamecodetoui[color1 - 1]
        if (k.spiel == Spiel::JK2) {
            static constexpr int kGameCodeToUi[7] = {4, 2, 3, 0, 5, 1, 6};
            static constexpr const char* kFx[7] = {"menu/art/fx_red", "menu/art/fx_orange", "menu/art/fx_yel", "menu/art/fx_grn",
                                                   "menu/art/fx_blue", "menu/art/fx_purple", "menu/art/fx_white"};
            const int c1 = std::clamp(zahl(k, "color1"), 1, 7);
            const int e = kGameCodeToUi[c1 - 1];
            z.bild(x, y - 14, 128, 8, "menu/art/fx_base");
            z.bild(x + static_cast<float>(e * 16) + 8, y - 16, 16, 12, kFx[e]);
        }
    } else if (id >= UI_CHAT_MAIN && id <= UI_CHAT_TACTICAL) {
        // UI_OwnerDraw hat keinen Fall: nichts (nur UI_OwnerDrawHandleKey)
    } else {
        r.bekannt = false;
    }
}

void sp(Zeichner& z, OdErgebnis& r) {
    const OdAufruf& a = z.a;
    const float x = a.x + a.textX;
    const float y = a.y + a.textY;
    r.bekannt = true;
    switch (a.id) {
        case SP_EFFECTS:   // UI_DrawEffects: beide Bilder mit Griff 0 = "<default>"
            z.bild(x, y - 14, 128, 8, "*default");
            z.bild(x + 8, y - 16, 16, 12, "*default");   // uiInfo.effectsColor = 0
            break;
        case SP_KEYBINDSTATUS:   // ohne wartende Taste nichts
            break;
        case SP_ALLMAPS_SELECTION: {
            // Spielstand-Vorschau: levelshots/<karte> des gewaehlten Spielstands; ohne Spielstand
            // uis.menuBackShader ("menu/art/unknownmap"), der Kartenname ist dann leer
            z.bild(a.x, a.y, a.w, a.h, "menu/art/unknownmap");
            break;
        }
        case SP_GLINFO: glInfo(z, x, y, a.w, a.h); break;
        default: r.bekannt = false; break;
    }
}

}  // namespace

GlDaten glBeispiel() {
    GlDaten g;
    g.vendor = "(example) NVIDIA Corporation";
    g.renderer = "NVIDIA GeForce RTX/PCIe/SSE2";
    g.version = "4.6.0";
    g.extensions = "GL_ARB_multitexture GL_ARB_texture_compression GL_EXT_texture_compression_s3tc GL_EXT_texture_env_add "
                   "GL_EXT_texture_filter_anisotropic GL_ARB_vertex_buffer_object GL_EXT_compiled_vertex_array GL_ARB_texture_env_combine";
    return g;
}

MpKraft mpKraft(const std::string& forcePowers, bool freeSaber, const Spielbeispiel& b) {
    MpKraft k;
    // UI_UpdateForcePowers
    k.raenge = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0};   // uiForcePowersRank (Vorbelegung)
    int seite = 0;
    int rang = kMaxForceRank;
    std::size_t i = 0;
    bool ok = !forcePowers.empty();
    auto feld = [&](int& out) {
        std::string buf;
        while (i < forcePowers.size() && forcePowers[i] != '-' && buf.size() < 255) { buf += forcePowers[i++]; }
        if (buf.size() >= 255 || i >= forcePowers.size() || forcePowers[i] != '-') { return false; }
        out = std::atoi(buf.c_str());
        ++i;
        return true;
    };
    while (ok && i < forcePowers.size()) {
        if (!feld(rang)) { seite = 0; break; }
        if (rang > b.maxRank) { rang = b.maxRank; }
        if (!feld(seite)) { seite = 0; break; }
        int f = 0;
        while (i < forcePowers.size() && f < kNumForcePowers) {
            const char ch[2] = {forcePowers[i], 0};
            k.raenge[static_cast<std::size_t>(f)] = std::atoi(ch);
            if (f == FP_LEVITATION && k.raenge[static_cast<std::size_t>(f)] < 1) { k.raenge[static_cast<std::size_t>(f)] = 1; }
            if ((f == FP_SABER_OFFENSE || f == FP_SABER_DEFENSE) && k.raenge[static_cast<std::size_t>(f)] < 1 && freeSaber) {
                k.raenge[static_cast<std::size_t>(f)] = 1;
            }
            ++f;
            ++i;
        }
        if (f < kNumForcePowers) { seite = 0; break; }
        ++i;
    }
    if (seite == 0) {   // validitycheck
        seite = 1;
        rang = 1;
        for (int p = 0; p < kNumForcePowers; ++p) {
            k.raenge[static_cast<std::size_t>(p)] =
                (p == FP_LEVITATION || ((p == FP_SABER_OFFENSE || p == FP_SABER_DEFENSE) && freeSaber)) ? 1 : 0;
        }
    }
    k.seite = seite;
    // UpdateForceUsed
    rang = b.maxRank;
    k.rang = rang;
    int verfuegbar = kForceMasteryPoints[std::clamp(rang, 0, kMaxForceRank)];
    if (k.raenge[FP_LEVITATION] < 1) { k.raenge[FP_LEVITATION] = 1; }
    if (b.jediVmerc && k.jediNonJedi == -1) {   // UI_TrueJediEnabled
        k.jediNonJedi = 0;
        bool leeren = false;
        for (int p = 0; p < kNumForcePowers; ++p) {
            const int v = k.raenge[static_cast<std::size_t>(p)];
            if (p == FP_LEVITATION || p == FP_SABER_OFFENSE) {
                if (v > 1) { k.jediNonJedi = 1; break; }
                if (v > 0) { leeren = true; }
            } else if (v > 0) {
                k.jediNonJedi = 1;
                break;
            }
        }
        if (k.jediNonJedi == 1) {
            if (k.raenge[FP_SABER_OFFENSE] < 1) { k.raenge[FP_SABER_OFFENSE] = 1; }
        } else if (leeren) {
            k.raenge.fill(0);
        }
    }
    int kosten[kNumForcePowers][kForceLevels];
    std::copy(&kForcePowerCost[0][0], &kForcePowerCost[0][0] + kNumForcePowers * kForceLevels, &kosten[0][0]);
    kosten[FP_SABER_OFFENSE][1] = freeSaber ? 0 : 1;
    kosten[FP_SABER_DEFENSE][1] = freeSaber ? 0 : 1;
    if (freeSaber) {
        if (k.raenge[FP_SABER_OFFENSE] < 1) { k.raenge[FP_SABER_OFFENSE] = 1; }
        if (k.raenge[FP_SABER_DEFENSE] < 1) { k.raenge[FP_SABER_DEFENSE] = 1; }
    } else if (k.raenge[FP_SABER_OFFENSE] < 1) {
        k.raenge[FP_SABER_DEFENSE] = 0;
        k.raenge[FP_SABERTHROW] = 0;
    }
    for (int p = 0; p < kNumForcePowers; ++p) {
        int& v = k.raenge[static_cast<std::size_t>(p)];
        if (v < 0) { v = 0; }
        else if (v >= kForceLevels) { v = kForceLevels - 1; }
        for (int r = 1; r <= v; ++r) {
            const bool frei = (p == FP_LEVITATION && r == 1) || ((p == FP_SABER_OFFENSE || p == FP_SABER_DEFENSE) && r == 1 && freeSaber);
            if (frei) { continue; }
            if (kosten[p][r] > verfuegbar) {
                v = r - 1;
                break;
            }
            verfuegbar -= kosten[p][r];
        }
    }
    k.verfuegbar = verfuegbar;
    return k;
}

OdErgebnis ownerDraw(const OdAufruf& a, const OdKontext& k) {
    OdErgebnis r;
    Zeichner z{a, k, r};
    if (k.dialekt == Dialekt::MP) {
        mp(z, r);
    } else {
        sp(z, r);
    }
    return r;
}

}  // namespace bhed::menue
