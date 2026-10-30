// menueprofil.cpp - Menue-Editor M5b Punkt 4: Mod-Profile. Profildatei (Rundlauf, unverstandene
// Zeilen bleiben), eingebaute Profile mit den Menuelisten der Spiele, Parser mit Profil-Schluesseln
// (ohne Meldung, Text byte-gleich; ohne Profil meldet er wie die Engine), uiScript-Wortzahl und
// Cvar-Vorgaben in der Laufzeit, beschriftete Platzhalter fuer Ownerdraws und Feeder.
#include "bhed/menue/profil.h"

#include "bhed/menue/skript.h"
#include "bhed/menue/vorschau.h"

#include <algorithm>
#include <cstdio>
#include <memory>
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
}  // namespace

int main() {
    // Profildatei
    const std::string text =
        "// Beispiel\n"
        "jksprofile 1\n"
        "name \"Meine Mod\"\n"
        "game ja\n"
        "menulist sp ui/meinemod.txt\n"
        "menulist mp ui/meinemod_mp.txt\n"
        "itemkey glScale float\n"
        "itemkey glOnHold script\n"
        "menukey glMusic string\n"
        "ownerdraw 300 GL_MAP_PREVIEW\n"
        "feeder 0x28 FEEDER_GL_CLASSES\n"
        "uiscript glSetClass 2\n"
        "cvar ui_gl_class \"3\"\n"
        "zukunft etwas neues\n"
        "itemkey kaputt keineart\n";
    ModProfil p;
    int zeile = 0;
    pruefe(liesProfil(text, p, zeile), "Profil gelesen");
    pruefe(p.name == "Meine Mod" && p.spiel == Spiel::JA && p.menuelistenSp.size() == 1 && p.menuelistenMp[0] == "ui/meinemod_mp.txt" &&
               p.itemSchluessel.size() == 2 && p.itemSchluessel["glScale"] == "float" && p.menuSchluessel["glMusic"] == "string" &&
               p.ownerdraws[300] == "GL_MAP_PREVIEW" && p.feeder[40] == "FEEDER_GL_CLASSES" && p.uiSkripte["glSetClass"] == 2 &&
               p.cvars["ui_gl_class"] == "3",
           "alle Angaben (feeder auch hexadezimal)");
    pruefe(p.unbekannt.size() == 2 && p.unbekannt[0] == "zukunft etwas neues" && p.unbekannt[1] == "itemkey kaputt keineart",
           "unverstandene Zeilen (neue Angabe, unbekannte Art) bleiben erhalten");
    ModProfil q;
    pruefe(liesProfil(profilText(p), q, zeile) && profilText(q) == profilText(p) && q.unbekannt == p.unbekannt, "Rundlauf der Profildatei");
    pruefe(!liesProfil("menuDef { }\n", q, zeile) && zeile == 1, "keine Profildatei: Zeile 1");
    pruefe(!liesProfil("jksprofile 2\n", q, zeile), "neuere Fassung abgelehnt");
    // eingebaute Profile: die Menuelisten der Spiele (vorher fest im Editor)
    const auto& e = eingebauteProfile();
    pruefe(e.size() == 3 && eingebautesProfil(Spiel::JA).menuelistenMp[0] == "ui/jampmenus.txt" &&
               eingebautesProfil(Spiel::MD).menuelistenMp[0] == "ui/MovieDuels-MP-Menus.txt" &&
               eingebautesProfil(Spiel::JK2).menuelistenMp[0] == "ui/jk2mpmenus.txt" && eingebautesProfil(Spiel::JA).menuelistenSp[0] == "ui/menus.txt",
           "eingebaute Profile mit den Menuelisten von Jedi Academy, Jedi Outcast und Movie Duels");
    const auto mp = menuelisten(p, Dialekt::MP);
    pruefe(mp.size() >= 4 && mp[0] == "ui/meinemod_mp.txt" && std::find(mp.begin(), mp.end(), "ui/jampmenus.txt") != mp.end() &&
               std::count(mp.begin(), mp.end(), "ui/jampmenus.txt") == 1,
           "Menuelisten: die des Profils zuerst, dann die eingebauten ohne Doppelte");
    pruefe(profilVorlage("Galactic Legacy", Spiel::JA).find("name \"Galactic Legacy\"") != std::string::npos &&
               liesProfil(profilVorlage("Galactic Legacy", Spiel::JA), q, zeile) && q.name == "Galactic Legacy" && q.unbekannt.empty(),
           "Vorlage zum Ausfuellen ist ein gueltiges Profil");

    // Parser: Schluessel der Mod
    const std::string menu =
        "{\n menuDef\n {\n  name m\n  rect 0 0 640 480\n  glMusic \"music/gl.mp3\"\n  visible 1\n"
        "  itemDef\n  {\n   name a\n   glScale 1.5\n   glOnHold { show b ; play x }\n   rect 10 10 50 20\n   visible 1\n  }\n"
        "  itemDef\n  {\n   name b\n   type ITEM_TYPE_OWNERDRAW\n   ownerdraw 300\n   rect 100 10 50 20\n   visible 1\n  }\n"
        " }\n}\n";
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const Datei ohne = lies("ui/m.menu", menu, o);
    pruefe(ohne.meldungen.size() >= 3, "ohne Profil meldet der Parser die Mod-Schluessel wie die Engine (Unknown keyword)");
    o.profil = std::make_shared<const ModProfil>(p);
    const Datei mit = lies("ui/m.menu", menu, o);
    pruefe(mit.meldungen.empty() && mit.menus.size() == 1 && mit.menus[0].items.size() == 2, "mit Profil: keine Meldung, beide Items gelesen");
    bool skriptAusProfil = false, menuAusProfil = false;
    for (const Eigenschaft& x : mit.menus[0].items[0].eigenschaften) {
        if (x.kanon == "glOnHold") { skriptAusProfil = x.ausProfil && x.ok && x.args.size() >= 6; }
    }
    for (const Eigenschaft& x : mit.menus[0].eigenschaften) {
        if (x.kanon == "glMusic") { menuAusProfil = x.ausProfil && x.ok; }
    }
    pruefe(skriptAusProfil && menuAusProfil, "Profil-Schluessel als solche markiert, Skript ganz gelesen");
    pruefe(schreib(mit) == menu, "Rundlauf: Text byte-gleich");
    pruefe(mit.menus[0].items[0].window.rectClient.w == 50, "Werte nach dem Profil-Schluessel richtig gelesen");
    // Vorschau: Ownerdraw 300 als beschrifteter Platzhalter mit dem Namen aus dem Profil
    Umgebung u;
    std::vector<Prim> prims;
    zeichneMenu(mit, 0, u, prims);
    const bool beschriftet = std::any_of(prims.begin(), prims.end(), [](const Prim& x) {
        return x.art == PrimArt::Platzhalter && x.shader == "ownerdraw 300 GL_MAP_PREVIEW";
    });
    pruefe(beschriftet, "Ownerdraw der Mod: Platzhalter mit Namen aus dem Profil");
    // Laufzeit: uiScript der Mod mit 2 Woertern, danach laeuft das Skript weiter; Cvar-Vorgabe
    const std::string sk = "{ menuDef { name m rect 0 0 640 480 visible 1\n"
                           "  itemDef { name k type ITEM_TYPE_BUTTON rect 0 0 50 20 visible 1 action { uiScript glSetClass jedi 2 hide k } } } }\n";
    const Datei ds = lies("ui/s.menu", sk, o);
    Laufzeit lz;
    lz.start(ds);
    lz.oeffne("m");
    lz.fuehreAus(0, 0, ds.menus[0].items[0].action);
    pruefe((lz.items[0][0].flags & WINDOW_VISIBLE) == 0, "uiScript der Mod liest 2 Woerter, danach wirkt hide k");
    pruefe(lz.cvar("ui_gl_class") == "3", "Cvar-Vorgabe der Mod");
    // Fund des Generators (tools/gen_menue_profil.py an den Movie-Duels-Quellen): MenuParse_ownerdrawID
    // hat den Rumpf auskommentiert und gibt qfalse zurueck - wie ItemParse_ownerdrawID
    LeseOptionen om;
    om.dialekt = Dialekt::SP;
    om.spiel = Spiel::MD;
    const Datei dm = lies("ui/o.menu", "{ menuDef { name m rect 0 0 640 480 ownerdrawID 5 visible 1 } }\n", om);
    const bool gemeldet = std::any_of(dm.meldungen.begin(), dm.meldungen.end(),
                                      [](const Meldung& x) { return x.text.find("Couldn't parse menu keyword ownerdrawID") != std::string::npos; });
    pruefe(gemeldet, "Movie Duels: menuDef ownerdrawID scheitert wie MenuParse_ownerdrawID (return qfalse)");
    std::printf("menueprofil: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
