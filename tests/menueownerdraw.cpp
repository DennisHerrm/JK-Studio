// menueownerdraw.cpp - Menue-Editor M7 Punkt 14: UI_OwnerDraw je Nummer wie die Engine
// (codemp/ui/ui_main.c, ui_force.c, code/ui/ui_main.cpp; Movie Duels ui_main.cpp, ui_pazaak.cpp).
#include "bhed/menue/datapad.h"
#include "bhed/menue/feeder.h"
#include "bhed/menue/hud.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/ownerdraw.h"
#include "bhed/menue/pazaak.h"
#include "bhed/menue/vorschau.h"

#include <cmath>
#include <cstddef>
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
        // rc608: im laufenden jk2mp nachgesehen: das Saebelbild ueber das ganze Feld, color1 ist die Nummer (4 blau)
        pruefe(e.teile.size() == 1 && e.teile[0].inhalt == "menu/art/saber_blue", "Jedi Outcast MP: UI_EFFECTS Saebelbild nach color1");
        j.cvars["color1"] = "7";
        pruefe(j.od(201).teile.empty(), "Jedi Outcast MP: color1 ausserhalb der sechs Saebelfarben zeichnet kein Saebelbild");
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
    // --- Datapad (code/cgame CG_DrawDataPad*) ---------------------------------------------
    {
        const std::vector<std::string> icons = waffenIcons("// x\n{\nweapontype WP_SABER\nweaponIcon gfx/hud/w_icon_lightsaber\n}\n"
                                                           "{ weapontype WP_BLASTER_PISTOL weaponIcon gfx/hud/w_icon_blaster_pistol }\n"
                                                           "{ weapontype WP_BLASTER weaponIcon gfx/hud/w_icon_blaster }\n"
                                                           "/* { weapontype WP_THERMAL weaponIcon falsch } */ { weapontype WP_THERMAL weaponIcon gfx/hud/w_icon_thermal }");
        pruefe(icons.size() == 14 && icons[1] == "gfx/hud/w_icon_lightsaber" && icons[10] == "gfx/hud/w_icon_thermal", "weapons.dat: weaponIcon je weapontype");
        DatapadKontext k;
        k.se = [](const std::string& key) { return "@" + key; };
        k.breite = [](const std::string& t, float, const std::string&) { return static_cast<int>(t.size()) * 10; };
        k.hoehe = [](float, const std::string&) { return 20; };
        k.waffenIcon = icons;
        std::vector<OdTeil> w;
        zeichneDatapad(kDpWeapons, k, w);
        // 4 Waffen, Saebel gewaehlt: links 1 (Thermal, rueckwaerts ueber Erschuetterung), Mitte, rechts 2
        pruefe(w.size() >= 4 && w[0].inhalt == "gfx/hud/w_icon_thermal" && nah(w[0].x, 176) && w[1].inhalt == "gfx/hud/w_icon_lightsaber" &&
                   nah(w[1].x, 280) && nah(w[1].y, 330) && nah(w[1].w, 80) && w[2].inhalt == "gfx/hud/w_icon_blaster_pistol" && nah(w[2].x, 424) &&
                   nah(w[3].x, 496),
               "CG_DrawDataPadWeaponSelect: Lage der Symbole");
        pruefe(w.size() >= 5 && w[4].inhalt == "@SP_INGAME_SABER_DESC" && w[4].schrift == "arialnb" && nah(w[4].x, 40) && nah(w[4].y, 60),
               "Waffentext in CG_DisplayBoxedText(40, 60, ..., arialnb)");
        std::vector<OdTeil> f;
        zeichneDatapad(kDpForcePowers, k, f);
        // 8 Kraefte, Sprung gewaehlt (Index 4): links 3, Mitte 70x70, rechts 3
        pruefe(f.size() >= 8 && f[3].inhalt == "gfx/mp/f_icon_levitation" && nah(f[3].x, 285) && nah(f[3].y, 325) && nah(f[3].w, 70) &&
                   f[0].inhalt == "gfx/mp/f_icon_sight" && nah(f[0].x, 181) && nah(f[2].x, 37) && f[4].inhalt == "gfx/mp/f_icon_speed" && nah(f[6].x, 563),
               "CG_DrawDataPadForceSelect: Mitte und Nachbarn");
        std::string ft;
        for (std::size_t i = 7; i < f.size(); ++i) { ft += f[i].inhalt; }
        pruefe(ft == "@SP_INGAME_FORCE_JUMP_DESC@SP_INGAME_FORCE_JUMP_LVL1_DESC" && f.size() == 9 && nah(f[8].y, 90), "Krafttext + Stufe 1, umbrochen (1,5 * Hoehe)");
        std::vector<OdTeil> z;
        zeichneDatapad(kDpObjectives, k, z);
        // Titel, Kreis, Punkt (erfuellt), Text, Kreis, Text
        pruefe(z.size() == 6 && z[0].inhalt == "@SP_INGAME_OBJECTIVES" && nah(z[0].x, 38) && nah(z[0].y, 52) && z[1].inhalt == "gfx/hud/objective_circle" &&
                   nah(z[1].x, 36) && nah(z[1].y, 83) && z[2].inhalt == "gfx/hud/message_on" && z[3].inhalt == "@OBJECTIVES_T1_SOUR_OBJ1" && nah(z[3].y, 75) &&
                   nah(z[5].y, 95),
               "CG_DrawDataPadObjectives: Titel, Kreise, Zeilen");
        k.werte.ziele.clear();
        z.clear();
        zeichneDatapad(kDpObjectives, k, z);
        pruefe(z.size() == 2 && z[1].inhalt == "@SP_INGAME_OBJNONE", "keine Ziele: SP_INGAME_OBJNONE");

        // M7: Jedi Outcast (codeJK2/cgame) - eigene Waffennamen, Hintergrund mit Zinken, 40/80 und Deckung 0,6
        const std::vector<std::string> jk2Icons = waffenIcons("{ weapontype WP_SABER weaponIcon gfx/hud/w_icon_lightsaber }"
                                                              "{ weapontype WP_BRYAR_PISTOL weaponIcon gfx/hud/w_icon_blaster_pistol }"
                                                              "{ weapontype WP_BLASTER weaponIcon gfx/hud/w_icon_blaster }"
                                                              "{ weapontype WP_THERMAL weaponIcon gfx/hud/w_icon_thermal }",
                                                              Spiel::JK2);
        pruefe(jk2Icons.size() == 14 && jk2Icons[2] == "gfx/hud/w_icon_blaster_pistol" && datapadWaffenNamen(Spiel::JK2)[12] == "WP_STUN_BATON",
               "Jedi Outcast: WP_BRYAR_PISTOL und WP_STUN_BATON");
        auto text = [](const std::vector<OdTeil>& v, std::size_t ab) {   // umbrochene Zeilen zusammen
            std::string t;
            for (std::size_t i = ab; i < v.size(); ++i) { t += v[i].inhalt; }
            return t;
        };
        DatapadKontext j = k;
        j.spiel = Spiel::JK2;
        j.waffenIcon = jk2Icons;
        j.werte = datapadBeispiel(Spiel::JK2);
        std::vector<OdTeil> jw;
        zeichneDatapad(kDpWeapons, j, jw);
        pruefe(jw.size() == 9 && jw[0].inhalt == "gfx/hud/background" && nah(jw[0].x, 110) && nah(jw[0].y, 325) && nah(jw[0].h, -60) && nah(jw[1].y, 323) &&
                   jw[2].inhalt == "gfx/hud/prong_on_w" && nah(jw[2].x, 105) && nah(jw[3].x, 536) && nah(jw[3].w, -40),
               "CG_DrawDataPadIconBackground: Hintergrund oben gespiegelt, Zinken");
        pruefe(jw.size() == 9 && jw[4].inhalt == "gfx/hud/w_icon_thermal" && nah(jw[4].x, 232) && nah(jw[4].y, 310) && nah(jw[4].farbe.a, 0.6F) &&
                   jw[5].inhalt == "gfx/hud/w_icon_lightsaber" && nah(jw[5].x, 280) && nah(jw[5].y, 290) && nah(jw[5].farbe.a, 1) && nah(jw[6].x, 368) &&
                   nah(jw[7].x, 416) && jw[8].inhalt == "@INGAME_SABER_DESC" && jw[8].schrift == "ocr_a" && nah(jw[8].scale, 0.7F) && nah(jw[8].x, 70),
               "Jedi Outcast CG_DrawDataPadWeaponSelect: Seiten 0,6, Mitte, Text qhFontSmall 0,7");
        std::vector<OdTeil> jf;
        zeichneDatapad(kDpForcePowers, j, jf);
        pruefe(jf.size() >= 12 && jf[4].inhalt == "gfx/hud/f_icon_saber_attack" && nah(jf[4].x, 252) && nah(jf[6].x, 176) && jf[7].inhalt == "gfx/hud/f_icon_levitation" &&
                   nah(jf[7].x, 290) && nah(jf[7].y, 295) && nah(jf[7].w, 60) && jf[8].inhalt == "gfx/hud/f_icon_speed" && nah(jf[8].x, 358) && nah(jf[10].x, 434) &&
                   text(jf, 11) == "@INGAME_FORCE_JUMP_DESC@INGAME_FORCE_JUMP_LVL1_DESC" && nah(jf[11].farbe.a, 0.75F) && jf[11].schrift == "ocr_a",
               "Jedi Outcast CG_DrawDataPadForceSelect: 30/60, Pad 8, Text CT_ICON_BLUE");
        std::vector<OdTeil> jz;
        zeichneDatapad(kDpObjectives, j, jz);
        pruefe(jz.size() == 6 && jz[0].inhalt == "@INGAME_OBJECTIVES" && nah(jz[0].x, 96) && nah(jz[0].y, 56) && nah(jz[1].x, 88) && nah(jz[1].y, 89) &&
                   jz[3].inhalt == "@OBJECTIVES_KEJIM_POST_OBJ1" && nah(jz[3].x, 108) && nah(jz[3].y, 79) && nah(jz[3].farbe.b, 0.92F) && nah(jz[4].y, 109) &&
                   nah(jz[5].y, 99),
               "MissionInformation_Draw: Titel 96, Kreise 88, Text 108 in CT_BLUE3");
        std::vector<OdTeil> ji;
        zeichneDatapad(kDpInventory, j, ji);
        pruefe(ji.size() == 5 && ji[0].inhalt == "gfx/hud/background_i" && ji[4].inhalt == "@INGAME_EMPTY_INV", "Jedi Outcast: Inventar leer (INGAME_EMPTY_INV)");

        // M7: Movie Duels - 25 Kraefte, eine je Seite, 22/45, Text mit Griff 4, SPMOD_ wenn SP_ fehlt
        DatapadKontext m = k;
        m.spiel = Spiel::MD;
        m.schrift4 = "griff4";
        m.werte = datapadBeispiel(Spiel::MD);
        pruefe(datapadKraefte(Spiel::MD).size() == 25 && datapadKraefte(Spiel::MD)[7] == 1 && datapadKraftNamen(Spiel::MD)[4] == "LIGHTNINGSTRIKE",
               "Movie Duels showDataPadPowers");
        std::vector<OdTeil> mf;
        zeichneDatapad(kDpForcePowers, m, mf);
        pruefe(mf.size() >= 8 && mf[4].inhalt == "gfx/mp/f_icon_sight" && nah(mf[4].x, 212) && nah(mf[4].y, 310) && nah(mf[4].w, 22) &&
                   mf[5].inhalt == "gfx/mp/f_icon_levitation" && nah(mf[5].x, 298) && nah(mf[5].y, 299) && nah(mf[5].w, 45) && mf[6].inhalt == "gfx/mp/f_icon_speed" &&
                   nah(mf[6].x, 406) && text(mf, 7) == "@SP_INGAME_FORCE_JUMP_DESC@SP_INGAME_FORCE_JUMP_LVL1_DESC" && mf[7].schrift == "griff4" &&
                   nah(mf[7].x, 40),
               "Movie Duels CG_DrawDataPadForceSelect: Lage, Text mit Griff 4");
        m.waffenIcon = icons;
        std::vector<OdTeil> mw;
        zeichneDatapad(kDpWeapons, m, mw);
        pruefe(mw.size() == 8 && mw[4].inhalt == "gfx/hud/w_icon_thermal" && nah(mw[4].x, 212) && nah(mw[4].y, 300) && nah(mw[5].x, 298) && nah(mw[5].y, 299) &&
                   mw[6].inhalt == "gfx/hud/w_icon_blaster_pistol" && nah(mw[6].x, 406) && mw[7].inhalt == "@SP_INGAME_SABER_DESC",
               "Movie Duels CG_DrawDataPadWeaponSelect: eine Waffe je Seite");
        m.se = [](const std::string& key) { return key.rfind("SP_INGAME_", 0) == 0 ? std::string() : "@" + key; };
        m.bildDa = [](const std::string& s) { return s != "gfx/mp/f_icon_sight"; };
        mf.clear();
        zeichneDatapad(kDpForcePowers, m, mf);
        pruefe(mf.size() >= 7 && mf[4].inhalt == "gfx/mp/f_icon_levitation" && text(mf, 6) == "@SPMOD_INGAME_FORCE_JUMP_DESC@SPMOD_INGAME_FORCE_JUMP_LVL1_DESC",
               "Movie Duels: fehlendes Symbol nicht gezeichnet, Text aus SPMOD_INGAME_");
    }
    // --- M7: Datapad-HUD von Jedi Outcast und Movie Duels (CG_DrawDataPadHUD) ---------------------
    {
        HudWerte hw;
        hw.gesundheit = 45;
        std::vector<Prim> h;
        zeichneDatapadHudJk2(Spiel::JK2, hw, h);
        auto finde = [&](const std::string& s, float x, float y) {
            for (const Prim& p : h) {
                if (p.shader == s && nah(p.x, x) && nah(p.y, y)) { return &p; }
            }
            return static_cast<const Prim*>(nullptr);
        };
        const Prim* tic = finde("gfx/hud/force_tick1", 526 + 69, 286 + 41);
        pruefe(!h.empty() && h[0].shader == "gfx/hud/hudleft_innerframe" && finde("gfx/2d/numbers/t_zero", 34 + 16, 286 + 40) != nullptr &&
                   tic != nullptr && nah(tic->w, -20) && nah(tic->farbe.r, 0.75F) && finde("gfx/hud/saber_stylesMed", 526, 286) != nullptr &&
                   h.back().shader == "gfx/hud/datapad2",
               "Jedi Outcast CG_DrawDataPadHUD: Rahmen, Gesundheit mit fuehrender Null, Macht-Striche gespiegelt, Saebelstil, datapad2");
        const Prim* gesund = finde("gfx/hud/health", 34, 286);
        pruefe(gesund != nullptr && nah(gesund->farbe.r, 0.835F * 0.45F), "CG_DrawHealth: CT_HUD_RED * Anteil");
        h.clear();
        zeichneDatapadHudJk2(Spiel::MD, hw, h);
        pruefe(finde("gfx/hud/hudleftframe", 34, 286) != nullptr && finde("gfx/hud/DataPadforce_tick1", 526 + 69, 286 + 41) != nullptr &&
                   finde("gfx/hud/DataPadsaber_stylesmed", 526, 286) != nullptr,
               "Movie Duels: zweiter Rahmen, eigene Datapad-Bilder");
    }
    // --- Feeder: Movie Duels (Tabellen aus ui_main.cpp), MP-Saebel, Darstellung ja/nein -------
    {
        FeederDaten fd;
        fd.gebaut = true;
        fd.dialekt = Dialekt::SP;
        fd.spiel = Spiel::MD;
        const std::vector<FeederEintrag> karten = fd.eintraege(87);
        pruefe(!karten.empty() && karten[0].spalten[0] == "@MD_MENU_MAIN_DM_EPISODE_1" && karten[1].spalten[0] == "@MD_MENU_MAIN_DM_N_TFF",
               "MD FEEDER_DUEL_MAPS: duelMaps[].duelMapName");
        const std::vector<FeederEintrag> figuren = fd.eintraege(85);
        pruefe(!figuren.empty() && figuren[0].bild == "gfx/menus/charactermenuicons/md_atris", "MD FEEDER_MD_MODELS: Symbol aus npc, wenn icon leer");
        pruefe(fd.eintraege(84).size() == 8 && fd.eintraege(88).size() > 1 && fd.eintraege(88)[0].spalten[0] == "@MENUS_SINGLE_HILT1",
               "MD Epochen und Griffe");
        pruefe(fd.art(87) == 1 && fd.art(0x60) == 0 && fd.art(feeder_sp::SAVEGAMES) == 1, "Darstellung: MD-Liste ja, unbekannte nein");
        fd.dialekt = Dialekt::MP;
        fd.spiel = Spiel::JA;
        pruefe(fd.art(feeder_mp::PLAYER_LIST) == 2 && fd.eintraege(feeder_mp::PLAYER_LIST).size() == 3 && fd.art(0x0d) == 1 && fd.art(0x99) == 0,
               "MP: Spielerliste als Beispiel, Serverstatus leer wie im Hauptmenue, unbekannt = Platzhalter");
        std::vector<std::string> ein, stab;
        // Name, "{" in der naechsten Zeile; notInMP 1 faellt weg; twoHanded -> Stab; "{" in derselben Zeile wird nicht erkannt
        saberListen("single_1\n{\nname \"@MENUS_SINGLE\"\n}\nkylesaber\n{\nname Kyle\nnotInMP 1\n}\n"
                    "dual_1\n{\nname \"Stab 1\"\ntwoHanded 1\n}\nfalsch {\nname X\n}\n",
                    ein, stab);
        pruefe(ein.size() == 1 && ein[0] == "@MENUS_SINGLE" && stab.size() == 1 && stab[0] == "Stab 1", "WP_SaberGetHiltInfo: Listen wie das Spiel");
        fd.saberEinhand = ein;
        fd.se = [](const std::string& k) { return "<" + k + ">"; };
        pruefe(fd.eintraege(feeder_mp::SABER_SINGLE_INFO).size() == 1 && fd.eintraege(feeder_mp::SABER_SINGLE_INFO)[0].spalten[0] == "<MENUS_SINGLE>",
               "FEEDER_SABER_SINGLE_INFO: UI_SaberProperNameForSaber uebersetzt @");
    }
    // --- Movie Duels: Pazaak (ui_pazaak.cpp) und UI_DUELMAPS_SELECTION ---------------------
    {
        PazaakKontext pk;
        pk.breite = [](const std::string& t, float) { return static_cast<int>(t.size()) * 10; };
        std::vector<OdTeil> f;
        // Feld 1: Deckkarte 5 (green_m), Text in der Kartenmitte: x + w/4 + (w/4 - Breite*0,5/2), y + h*0,25
        pruefe(pazaakOwnerDraw(2001, 100, 50, 40, 60, "", pk, f) && f.size() == 2 && f[0].inhalt == "gfx/minigames/pazaak/cards/green_m.png" &&
                   nah(f[0].farbe.a, 0.8F) && f[1].inhalt == "5" && nah(f[1].x, 117) && nah(f[1].y, 65) && f[1].schrift == "ergoec" && nah(f[1].scale, 0.5F),
               "Pazaak Feldkarte");
        f.clear();
        (void)pazaakOwnerDraw(2105, 0, 0, 40, 60, "", pk, f);
        pruefe(f.size() == 2 && f[0].inhalt == "gfx/minigames/pazaak/cards/back_m.png" && f[1].inhalt.empty(), "Pazaak Gegnerhand: Rueckseite ohne Wert");
        f.clear();
        (void)pazaakOwnerDraw(2301, 300, 20, 0, 0, "", pk, f);
        pruefe(f.size() == 1 && f[0].inhalt == "( ^516 ^7)" && nah(f[0].x, 300 - 50), "Pazaak Punkte rechtsbuendig, Farbcodes");
        f.clear();
        (void)pazaakOwnerDraw(2202, 400, 20, 0, 0, "", pk, f);
        pruefe(f.size() == 2 && f[1].inhalt == "Gambler" && nah(f[0].scale, 0.51F) && nah(f[1].x, 400 - 35), "Pazaak Gegnername mit Schatten");
        f.clear();
        (void)pazaakOwnerDraw(2601, 0, 0, 40, 60, "", pk, f);
        pruefe(f.size() == 3 && f[2].inhalt == "1" && f[2].stil == 3, "Pazaak Kartenwahl: Anzahl - im Deck, mit Schatten");
        pruefe(!pazaakOwnerDraw(1999, 0, 0, 1, 1, "", pk, f), "keine Pazaak-Nummer");
        Welt w(Dialekt::SP, Spiel::MD);
        w.k.bildDa = [](const std::string& b) { return b == "levelshots/bg_tradefed"; };
        const OdErgebnis d = w.od(257);
        pruefe(d.bekannt && d.teile.size() == 1 && d.teile[0].inhalt == "levelshots/bg_tradefed", "MD UI_DUELMAPS_SELECTION: duelMaps[1].duelMapImage");
        Welt ja(Dialekt::SP);
        pruefe(!ja.od(257).bekannt, "257 nur in Movie Duels");
    }
    // --- Movie Duels 9: der Laden (ui_pazaak.cpp UI_Shop_OwnerDraw, Sollwerte von Hand nach der Quelle) -----
    {
        const std::string datei =
            "// Kommentar\nitem // 1\n{\n\tname \"Blaster Pistol\"\n\tcat pistols\n\ticon \"gfx/hud/w_icon_blaster_pistol\"\n\tprice 200\n}\n"
            "item // 2\n{\n\tname \"Bryar Pistol\"\n\tcat pistols\n\tprice 1200\n\tonly sp\n}\n"
            "item /* 3 */\n{\n\tname \"Thermal\"\n\tcat explosives\n\tprice 90\n}\n"
            "item // 4\n{\n\tcat unbekannt\n}\n";
        PazaakKontext pk;
        pk.breiteMenue = [](const std::string& t, float sc) { return static_cast<int>(static_cast<float>(t.size()) * 10.0F * sc); };
        pk.bildDa = [](const std::string& b) { return b == "gfx/hud/w_icon_blaster_pistol" || b == "gfx/hud/i_icon_bacta"; };
        pk.shopDatei = &datei;
        std::vector<OdTeil> f;
        // Zeile 0 (Pistols): Fuellen, Bild (x+4, y+2, (h-4)*2, h-4), Name bei x+12+(h-4)*2, y+h/2-8, Preis rechts gold
        pruefe(pazaakOwnerDraw(3200, 100, 50, 400, 24, "", pk, f) && f.size() == 4 && f[0].art == OdTeil::Art::Fuellen &&
                   nah(f[0].farbe.a, 0.85F) && f[1].art == OdTeil::Art::Bild && nah(f[1].x, 104) && nah(f[1].y, 52) && nah(f[1].w, 40) &&
                   nah(f[1].h, 20) && f[2].inhalt == "Blaster Pistol" && nah(f[2].x, 152) && nah(f[2].y, 54) && nah(f[2].scale, 0.55F) &&
                   f[2].font == 0 && f[3].inhalt == "200 cr" && nah(f[3].x, 100 + 400 - 8 - 33) && nah(f[3].farbe.g, 0.8F),
               "Laden: Zeile mit Bild, Name und Preis (Shop_DrawRow)");
        f.clear();
        // SP: Bryar (1200 cr) ist teurer als die 1000 Credits - rot, ohne Bild (kein Shader)
        (void)pazaakOwnerDraw(3201, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 3 && f[1].inhalt == "Bryar Pistol" && f[2].inhalt == "1200 cr" && nah(f[2].farbe.r, 1.0F) && nah(f[2].farbe.g, 0.4F),
               "Laden: zu teuer rot, ohne Bild");
        f.clear();
        pk.zustand.shop.besitz[0] = 1U << 1;
        pk.zustand.shop.inHand[0] = 1U << 1;
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 4 && f[3].inhalt == "In hand" && nah(f[3].farbe.g, 1.0F), "Laden: Bit 1 = Eintrag 1 in der Hand");
        f.clear();
        pk.zustand.shop.inHand[0] = 0;
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 4 && f[3].inhalt == "Owned", "Laden: Eintrag 1 im Besitz");
        f.clear();
        pk.zustand.shop.besitz[0] = 0;
        // MP: "only sp" faellt weg, Zeile 1 leer; Schrift MenuFontToHandle(1)
        pk.mp = true;
        (void)pazaakOwnerDraw(3201, 0, 0, 400, 24, "", pk, f);
        pruefe(f.empty(), "Laden MP: only sp nicht verkauft");
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 4 && f[2].font == 1, "Laden MP: Schrift 1");
        f.clear();
        // Explosives: Nummer 3 behaelt ihren Platz, Packung zu drei
        pk.mp = false;
        pk.zustand.shop.kategorie = 3;
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 3 && f[1].inhalt == "Thermal" && f[2].inhalt == "90 cr (x3)", "Laden: Explosives mit (x3)");
        f.clear();
        // Supplies: Munition 150, Bacta 200 (SP) bzw. Medpac (MP), Preise aus dem Zustand; Bild des Bacta-Kanisters
        pk.zustand.shop.kategorie = 5;
        (void)pazaakOwnerDraw(3201, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 4 && f[2].inhalt == "Bacta canister" && f[3].inhalt == "200 cr" && f[1].inhalt == "gfx/hud/i_icon_bacta", "Laden: Bacta 200");
        f.clear();
        pk.mp = true;
        (void)pazaakOwnerDraw(3201, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 3 && f[1].inhalt == "Medpac", "Laden MP: Medpac");
        f.clear();
        pk.mp = false;
        // unbekannte Kategorie = Rifles (Shop_CatByName), Name "?"
        pk.zustand.shop.kategorie = 1;
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.size() == 3 && f[1].inhalt == "?" && f[2].inhalt == "300 cr", "Laden: Vorgaben Rifles, ?, 300");
        f.clear();
        // Kopf: Credits rechtsbuendig (0,7), Seite mittig; Reiter gewaehlt gold auf tabOn
        pk.zustand.shop.kategorie = 0;
        (void)pazaakOwnerDraw(3001, 300, 30, 244, 30, "", pk, f);
        pruefe(f.size() == 1 && f[0].inhalt == "Credits: 1000" && nah(f[0].x, static_cast<float>(static_cast<int>(544.0F - 13 * 7.0F))) &&
                   nah(f[0].scale, 0.7F),
               "Laden: Credits rechts");
        f.clear();
        (void)pazaakOwnerDraw(3003, 100, 400, 200, 20, "", pk, f);
        pruefe(f.size() == 1 && f[0].inhalt == "Page 1 / 1", "Laden: Seite 1 / 1");
        f.clear();
        (void)pazaakOwnerDraw(3100, 56, 66, 86, 26, "", pk, f);
        pruefe(f.size() == 2 && f[0].art == OdTeil::Art::Fuellen && nah(f[0].farbe.b, 0.65F) && f[1].inhalt == "Pistols" && nah(f[1].farbe.g, 0.8F) &&
                   nah(f[1].y, 71),
               "Laden: gewaehlter Reiter");
        f.clear();
        pruefe(pazaakOwnerDraw(3999, 0, 0, 1, 1, "", pk, f) && f.empty() && !pazaakOwnerDraw(4000, 0, 0, 1, 1, "", pk, f), "Laden: 3000 bis 3999");
        pk.shopDatei = nullptr;
        (void)pazaakOwnerDraw(3200, 0, 0, 400, 24, "", pk, f);
        pruefe(f.empty(), "Laden ohne md_shop.dat: nur die Vorraete");
    }
    // M7: Renderer-Cvars der Setup-Menues (R_Register vor der Oberflaeche; SP und MP verschieden)
    pruefe(spielCvarVorgabe(Dialekt::SP, Spiel::JA, "r_texturemode") == "GL_LINEAR_MIPMAP_LINEAR" &&
               spielCvarVorgabe(Dialekt::MP, Spiel::JA, "r_textureMode") == "GL_LINEAR_MIPMAP_NEAREST" &&
               spielCvarVorgabe(Dialekt::SP, Spiel::JK2, "r_picmip") == "1" && !spielCvarVorgabe(Dialekt::SP, Spiel::MD, "r_detailtextures").empty(),
           "Engine-Cvars (R_Register) als Vorgabe");
    std::printf("menueownerdraw: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
