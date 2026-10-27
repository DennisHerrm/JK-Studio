// kartelogik.cpp - Karten-Editor Stufe 8b Punkt 4: der Graph der Logik-Ansicht.
//
// Eine Karte mit Ausloeser -> Relais -> Tuer, target2 ohne Ziel, einem
// NPC_spawner (NPC_targetname, deathscript, spawnscript), einem Ausloeser auf
// dessen NPC-Namen, einem Kreis a -> b -> a, einer Tuer mit opentarget, einem
// target5 (aus den Definitionen) und einer Entity ohne Verknuepfung: die
// Kanten, die fehlenden Ziele, die Skripte, die Spalten (laengster Weg, Kreis
// gebrochen) und dass zwei Laeufe dieselbe Lage geben.
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/logik.h"

#include <algorithm>
#include <cstdio>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) { ++g_fehler; }
    std::printf("%s  %s\n", ok ? "OK  " : "FEHL", was.c_str());
}

std::string ent(const std::string& klasse, const std::string& rest) { return "{\n\"classname\" \"" + klasse + "\"\n" + rest + "}\n"; }

int finde(const LogikGraph& g, const std::string& name) {
    for (std::size_t i = 0; i < g.knoten.size(); ++i) {
        if (g.knoten[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}
bool kante(const LogikGraph& g, int a, int b, const std::string& s) {
    return std::any_of(g.kanten.begin(), g.kanten.end(), [&](const LogikKante& k) { return k.von == a && k.zu == b && k.schluessel == s; });
}

}  // namespace

int main() {
    std::string t = "{\n\"classname\" \"worldspawn\"\n}\n";
    t += ent("trigger_once", "\"targetname\" \"start\"\n\"target\" \"relais\"\n\"target2\" \"gibtsnicht\"\n\"origin\" \"0 0 0\"\n");
    t += ent("target_relay", "\"targetname\" \"relais\"\n\"target\" \"tuer\"\n\"origin\" \"64 0 0\"\n");
    t += ent("func_door", "\"targetname\" \"tuer\"\n\"opentarget\" \"licht\"\n\"origin\" \"128 0 0\"\n");
    t += ent("light", "\"targetname\" \"licht\"\n\"origin\" \"192 0 0\"\n");
    t += ent("NPC_spawner", "\"NPC_targetname\" \"wache\"\n\"deathscript\" \"test/tot\"\n\"spawnscript\" \"test/da\"\n\"origin\" \"0 64 0\"\n");
    t += ent("trigger_multiple", "\"targetname\" \"aufruf\"\n\"target\" \"wache\"\n\"origin\" \"0 128 0\"\n");
    t += ent("target_relay", "\"targetname\" \"a\"\n\"target\" \"b\"\n\"origin\" \"0 192 0\"\n");
    t += ent("target_relay", "\"targetname\" \"b\"\n\"target\" \"a\"\n\"origin\" \"64 192 0\"\n");
    t += ent("target_relay", "\"targetname\" \"fuenf\"\n\"target5\" \"licht\"\n\"origin\" \"0 256 0\"\n");
    t += ent("info_null", "\"origin\" \"0 320 0\"\n");
    Karte k;
    (void)karteLesen(t, k);
    k.anmelden();
    EntitaetsKlassen klassen;
    const std::string def = "/*QUAKED target_relay (0 .5 .8) (-8 -8 -8) (8 8 8)\n\"target5\" also fired\n\"goaltarget\" where to go\n*/\n";
    (void)klassen.defLesen(def, "test.def");
    const std::vector<std::string> v = logikVerknuepfungen(&klassen);
    pruefe(std::find(v.begin(), v.end(), "target5") != v.end() && std::find(v.begin(), v.end(), "goaltarget") != v.end() &&
               std::find(v.begin(), v.end(), "NPC_target") != v.end() && std::find(v.begin(), v.end(), "targetname") == v.end(),
           "Verknuepfungsschluessel: OpenJK-Felder und *target* aus den Definitionen (target5, goaltarget), nicht targetname");
    const LogikGraph g = logikGraph(k, &klassen);
    std::printf("Graph: %zu Knoten, %zu Kanten, %d ohne Ziel\n", g.knoten.size(), g.kanten.size(), g.fehlend);
    const int start = finde(g, "start");
    const int relais = finde(g, "relais");
    const int tuer = finde(g, "tuer");
    const int licht = finde(g, "licht");
    const int wache = finde(g, "wache");
    const int aufruf = finde(g, "aufruf");
    const int fehlt = finde(g, "gibtsnicht");
    const int a = finde(g, "a");
    const int b = finde(g, "b");
    const int fuenf = finde(g, "fuenf");
    pruefe(g.knoten.size() == 10, "10 Knoten (9 Entities mit Namen/Verknuepfung, info_null ohne nicht, 1 fehlendes Ziel)");
    pruefe(kante(g, start, relais, "target") && kante(g, relais, tuer, "target") && kante(g, tuer, licht, "opentarget"), "Kette Ausloeser -> Relais -> Tuer -> Licht");
    pruefe(fehlt >= 0 && g.knoten[static_cast<std::size_t>(fehlt)].entitaet.expired() && kante(g, start, fehlt, "target2") && g.fehlend == 1,
           "target2 ohne Ziel: ein Knoten \"? gibtsnicht\", 1 fehlend");
    pruefe(kante(g, aufruf, wache, "target"), "target auf den NPC_targetname des Spawners");
    pruefe(wache >= 0 && g.knoten[static_cast<std::size_t>(wache)].skripte.size() == 2 &&
               g.knoten[static_cast<std::size_t>(wache)].skripte[0].first == "deathscript" &&
               g.knoten[static_cast<std::size_t>(wache)].skripte[0].second == "test/tot",
           "Skript-Schluessel des Spawners: deathscript test/tot, spawnscript test/da");
    pruefe(kante(g, a, b, "target") && kante(g, b, a, "target"), "Kreis a -> b -> a: beide Kanten");
    pruefe(kante(g, fuenf, licht, "target5"), "target5 (aus den Definitionen) verknuepft");
    const auto eb = [&g](int i) { return g.knoten[static_cast<std::size_t>(i)].ebene; };
    pruefe(eb(start) == 0 && eb(relais) == 1 && eb(tuer) == 2 && eb(licht) == 3 && eb(fehlt) == 1, "Spalten nach dem laengsten Weg (0, 1, 2, 3)");
    pruefe(eb(a) != eb(b), "Kreis gebrochen: a und b in verschiedenen Spalten");
    bool ueberlappt = false;
    for (std::size_t i = 0; i < g.knoten.size(); ++i) {
        for (std::size_t j = i + 1; j < g.knoten.size(); ++j) {
            const LogikKnoten& x = g.knoten[i];
            const LogikKnoten& y = g.knoten[j];
            if (x.x < y.x + y.breite && y.x < x.x + x.breite && x.y < y.y + y.hoehe && y.y < x.y + x.hoehe) { ueberlappt = true; }
        }
    }
    pruefe(!ueberlappt, "keine Kaesten ueberlappen");
    const LogikGraph g2 = logikGraph(k, &klassen);
    bool gleich = g2.knoten.size() == g.knoten.size();
    for (std::size_t i = 0; gleich && i < g.knoten.size(); ++i) { gleich = g.knoten[i].x == g2.knoten[i].x && g.knoten[i].y == g2.knoten[i].y; }
    pruefe(gleich, "zweiter Lauf: dieselbe Lage");
    std::printf("kartelogik: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
