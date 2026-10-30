// menueownerdraw.cpp - Menue-Editor M7 Punkt 14: UI_OwnerDraw je Nummer wie die Engine
// (codemp/ui/ui_main.c, ui_force.c, code/ui/ui_main.cpp; Movie Duels ui_main.cpp, ui_pazaak.cpp).
#include "bhed/menue/feeder.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/ownerdraw.h"
#include "bhed/menue/vorschau.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(float a, float b) { return std::fabs(a - b) < 0.01F; }

struct Welt {
    std::map<std::string, std::string> cvars;
    std::map<std::string, std::string> texte{{"MP_INGAME_NET_SOURCES", "Local\nInternet\nFavorites"}};
    FeederDaten fd;
    OdKontext k;
    Welt(Dialekt d, Spiel s = Spiel::JA) {
        fd.gebaut = true;
        fd.dialekt = d;
        k.dialekt = d;
        k.spiel = s;
        k.daten = &fd;
        k.cvar = [this](const std::string& n) {
            const auto it = cvars.find(n);
            return it == cvars.end() ? std::string() : it->second;
        };
        k.se = [this](const std::string& key) {
            const auto it = texte.find(key);
            return it == texte.end() ? "@" + key : it->second;
        };
    }
    OdErgebnis od(int id, float x = 100, float y = 50, float w = 200, float h = 100) const {
        OdAufruf a;
        a.id = id;
        a.x = x;
        a.y = y;
        a.w = w;
        a.h = h;
        return ownerDraw(a, k);
    }
    std::string text(int id) const {
        const OdErgebnis r = od(id);
        return r.teile.empty() ? std::string() : r.teile.front().inhalt;
    }
};
}  // namespace

int main() {
    // --- MP-Kraefte: UI_UpdateForcePowers + UpdateForceUsed ---------------------------------
    {
        const Spielbeispiel b;
        // Vorgabe DEFAULT_FORCEPOWERS "5-1-000000000000000000": Rang wird uiMaxRank (7), Sprung 1 frei
        const MpKraft k = mpKraft("5-1-000000000000000000", false, b);
        pruefe(k.seite == 1 && k.rang == 7 && k.verfuegbar == 100 && k.raenge[1] == 1 && k.raenge[15] == 0,
               "forcepowers Vorgabe: hell, Rang 7, 100 Punkte frei, Sprung 1");
        // Heilen 3 (2+4+6), Sprung 3 (1. Stufe frei, 2+6), Tempo 3 (2+4+6) = 32
        const MpKraft d = mpKraft("7-2-333000000000000000", false, b);
        pruefe(d.seite == 2 && d.verfuegbar == 68 && d.raenge[0] == 3, "dunkel, 32 Punkte verbraucht");
        // leer -> validitycheck: Seite 1, Sprung 1; ui_freeSaber: Saebel 1/1 kostenlos
        const MpKraft f = mpKraft("", true, b);
        pruefe(f.seite == 1 && f.raenge[15] == 1 && f.raenge[16] == 1 && f.verfuegbar == 100, "leer + ui_freeSaber");
        // Saebelangriff 0 ohne freeSaber: Abwehr und Wurf auf 0
        const MpKraft s = mpKraft("7-1-000000000000000033", false, b);
        pruefe(s.raenge[16] == 0 && s.raenge[17] == 0, "ohne Saebelangriff keine Abwehr/kein Wurf");
    }
    {
        Welt w(Dialekt::MP);
        w.cvars["forcepowers"] = "7-2-300000000000000000";
        // UI_FORCE_RANK_HEAL (259): Heilen 3 -> drei Sterne forcestar2/4/6, 16x16, Abstand 20, y + 6
        const OdErgebnis r = w.od(259);
        pruefe(r.bekannt && r.beispiel && r.teile.size() == 3 && r.teile[0].inhalt == "forcestar2" && r.teile[1].inhalt == "forcestar4" &&
                   r.teile[2].inhalt == "forcestar6" && nah(r.teile[1].x, 120) && nah(r.teile[0].y, 56) && nah(r.teile[0].w, 16),
               "UI_DrawForceStars: Heilen 3");
        // UI_FORCE_RANK_LEVITATION (260): Kosten 0/2/6, Rang 1 -> forcestar0, forcecircle2, forcecircle6
        const OdErgebnis l = w.od(260);
        pruefe(l.teile.size() == 3 && l.teile[0].inhalt == "forcestar0" && l.teile[1].inhalt == "forcecircle2" && l.teile[2].inhalt == "forcecircle6",
               "UI_DrawForceStars: Sprung 1 (erste Stufe kostet 0 -> forcestar0)");
        pruefe(w.text(288) == "88" && w.text(258) == "@MP_INGAME_MASTERY7" && w.text(286) == "@MP_INGAME_MASTERY7",
               "UI_FORCE_POINTS 100-12, UI_FORCE_RANK/MASTERY_SET");
        const OdErgebnis seite = w.od(257);
        bool dunkel = false, hellAus = false;
        for (const auto& z : seite.zeigen) {
            dunkel = dunkel || (z.menue == "ingame_playerforce" && z.item == "darkpowers" && z.zeigen);
            hellAus = hellAus || (z.menue == "ingame_playerforce" && z.item == "lightpowers" && !z.zeigen);
        }
        pruefe(w.text(257) == "@MENUS_FORCEDESC_DARK" && dunkel && hellAus, "UI_FORCE_SIDE: dunkel, zeigt darkpowers, verbirgt lightpowers");
        pruefe(w.od(289).bekannt && w.od(289).teile.empty(), "UI_JEDI_NONJEDI ohne g_jediVmerc: nichts");
        Spielbeispiel jv;
        jv.jediVmerc = true;
        w.k.beispiel = &jv;
        pruefe(w.text(289) == "@MENUS_YES", "UI_JEDI_NONJEDI mit g_jediVmerc: Jedi");
        w.k.beispiel = nullptr;
    }
    // Menu_ShowItemByName aus UI_DrawForceSide wirkt in der Vorschau auf das ganze Menue
    {
        LeseOptionen o;
        o.dialekt = Dialekt::MP;
        const Datei d = lies("ui/jamp/ingame_playerforce.menu",
                             "{ menuDef { name ingame_playerforce rect 0 0 640 480 visible 1\n"
                             " itemDef { name seite type 8 ownerdraw 257 rect 10 10 100 20 visible 1 }\n"
                             " itemDef { name a group lightpowers style 1 backcolor 1 1 1 1 rect 10 40 50 20 visible 1 decoration }\n"
                             " itemDef { name b group darkpowers style 1 backcolor 1 1 1 1 rect 10 70 50 20 visible 1 decoration }\n"
                             "} }\n",
                             o);
        Welt w(Dialekt::MP);
        Umgebung u;
        u.dialekt = Dialekt::MP;
        u.ownerDraw = [&](const OdAufruf& a) { return ownerDraw(a, w.k); };
        std::vector<Prim> p;
        zeichneMenu(d, 0, u, p);
        bool hell = false, dunkel = false;
        for (const Prim& x : p) {
            hell = hell || x.item == 1;
            dunkel = dunkel || x.item == 2;
        }
        pruefe(hell && !dunkel, "UI_FORCE_SIDE hell: darkpowers verborgen, lightpowers sichtbar");
    }
    // --- MP: Server, Spielarten, Bots, Chat -------------------------------------------------
    {
        Welt w(Dialekt::MP);
        w.fd.joinSpieltypen = {{"All", -1}, {"Free For All", 0}, {"Duel", 3}};
        w.fd.bots = {"Alora", "Bastila"};
        w.fd.karten = {{"Lange Karte", "1"}};
        w.fd.mods = {{"md", "Movie Duels"}};
        w.cvars["ui_joinGametype"] = "2";
        pruefe(w.text(253) == "@MENUS_DUEL", "UI_JOINGAMETYPE: joinGameTypes[2]");
        w.cvars["ui_joinGametype"] = "0";
        pruefe(w.text(253) == "@MENUS_SAGA", "UI_JOINGAMETYPE: All (-1) -> UI_GetGameTypeName default");
        w.cvars["ui_netSource"] = "2";
        pruefe(w.text(220) == "@MENUS_SOURCE Internet 2", "UI_NETSOURCE: NET_SOURCES-Zeile 1 + Nummer");
        w.cvars["ui_netSource"] = "6";
        pruefe(w.text(220) == "@MENUS_SOURCE Favorites", "UI_NETSOURCE: Favoriten");
        pruefe(w.text(222) == "@MENUS_GAME @MENUS_ALL", "UI_NETFILTER: Alle");
        w.cvars["ui_serverFilterType"] = "1";
        pruefe(w.text(222) == "@MENUS_GAME Movie Duels", "UI_NETFILTER: modList[0].modDescr");
        pruefe(w.text(239) == "Alora" && w.text(240) == "@MP_INGAME_SKILL1" && w.text(241) == "@MP_INGAME_RED",
               "UI_BOTNAME/BOTSKILL/REDBLUE wie uiInfo nach dem Start (0)");
        pruefe(w.text(236) == "Lange Karte", "UI_ALLMAPS_SELECTION: mapList[ui_currentNetMap].mapName");
        w.cvars["ui_lastServerRefresh_6"] = "10-30 12:00";
        pruefe(w.text(247) == "@MP_INGAME_SERVER_REFRESHTIME: 10-30 12:00", "UI_SERVERREFRESHDATE");
        for (int id = 291; id <= 297; ++id) { pruefe(w.od(id).bekannt && w.od(id).teile.empty(), "UI_CHAT_* zeichnet nichts: " + std::to_string(id)); }
        pruefe(w.od(201).bekannt && w.od(201).teile.empty() && w.od(250).teile.empty(), "JKA MP: UI_EFFECTS und UI_KEYBINDSTATUS nichts");
        Welt j(Dialekt::MP, Spiel::JK2);
        j.cvars["color1"] = "4";
        const OdErgebnis e = j.od(201);
        pruefe(e.teile.size() == 2 && e.teile[0].inhalt == "menu/art/fx_base" && e.teile[1].inhalt == "menu/art/fx_red" && nah(e.teile[1].x, 108),
               "Jedi Outcast MP: UI_EFFECTS fx_base + fxPic[gamecodetoui[color1-1]]");
    }
    // --- UI_GLINFO --------------------------------------------------------------------------
    {
        GlDaten g;
        g.echt = true;
        g.vendor = "V";
        g.renderer = "R";
        g.version = "4.6";
        g.extensions = "A B C D E";
        Welt w(Dialekt::MP);
        w.k.gl = &g;
        const OdErgebnis r = w.od(249, 10, 20, 300, 60);
        // drei Kopfzeilen, dann Erweiterungen in zwei Spalten ab y + 45, 11 Zeilen hoch, bis rect.h
        pruefe(!r.beispiel && r.teile.size() == 7 && r.teile[0].inhalt == "GL_VENDOR: V" && r.teile[1].inhalt == "GL_VERSION: 4.6: R" &&
                   nah(r.teile[3].y, 65) && nah(r.teile[4].x, 160) && r.teile[4].maxPixel == 150,
               "MP UI_DrawGLInfo: Spalten, Grenze rect.h");
        Welt s(Dialekt::SP);
        s.k.gl = &g;
        const OdErgebnis q = s.od(249, 10, 20, 300, 100);
        // SP: 4 Kopfzeilen (y, +15 ...), Zeilen gezaehlt bis testy > y + h, dann numLines--
        pruefe(q.teile.size() == 4 + 3 && q.teile[3].inhalt == "Color(24-bits) Z(24-bits) stencil(8-bits)" && nah(q.teile[4].y, 80) &&
                   nah(q.teile[6].y, 112),
               "SP UI_DrawGLInfo: eine Spalte, letzte gezaehlte Zeile faellt weg");
        s.k.gl = nullptr;
        pruefe(s.od(249).beispiel, "ohne Treiberdaten: Beispiel");
    }
    std::printf("menueownerdraw: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
