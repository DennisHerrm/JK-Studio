// kartemissiontest.cpp - rc609 B2: Abgleich der Entities nach dem Bau (Regeln in bhed/kartemission.h)
//
//   - unveraenderte Entities der Mission: die Fassung der neuen Karte gilt (auch *N neu nummeriert)
//   - geaenderte: nur die geaenderten Schluessel kommen auf die neue Entity, model *N bleibt neu
//   - eigene: angehaengt, oder - steht sie schon in der neuen Karte - nicht doppelt
//   - geloeschte bleiben geloescht
//   - Befunde: Name fehlt (Skript, target), *N fehlt/verschoben/angepasst, Entity nicht mehr in der Karte
//   - Auswahl-Nummern: neuVonMission
#include "bhed/kartemission.h"
#include "bhed/script.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace bhed;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
MapEntity ent(std::vector<std::pair<std::string, std::string>> keys) {
    MapEntity e;
    for (const auto& [k, v] : keys) { setKey(e, k, v); }
    return e;
}
std::string w(const MapEntity& e, const char* k) {
    const std::string* s = e.find(k);
    return s != nullptr ? *s : std::string("-");
}
int finde(const std::vector<MapEntity>& v, const char* klasse, const char* name) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].classname == klasse && (name == nullptr || v[i].targetname == name)) { return static_cast<int>(i); }
    }
    return -1;
}
int zaehle(const std::vector<MapEntity>& v, const char* klasse) {
    int n = 0;
    for (const MapEntity& e : v) { n += e.classname == klasse ? 1 : 0; }
    return n;
}
bool befund(const KarteMissionErgebnis& e, KarteMissionBefund::Art art, const std::string& was) {
    for (const KarteMissionBefund& b : e.befunde) {
        if (b.art == art && b.was == was) { return true; }
    }
    return false;
}
}  // namespace

int main() {
    // alt: worldspawn, Spielerstart, Tuer *1 "tuer", Tuer *2 ohne Namen, Licht, Relais "relais" -> tuer
    const std::vector<MapEntity> alt = {
        ent({{"classname", "worldspawn"}, {"message", "alt"}}),
        ent({{"classname", "info_player_start"}, {"origin", "-384 128 24"}}),
        ent({{"classname", "func_door"}, {"targetname", "tuer"}, {"model", "*1"}}),
        ent({{"classname", "func_door"}, {"model", "*2"}, {"speed", "100"}}),
        ent({{"classname", "light"}, {"origin", "0 0 128"}, {"light", "300"}}),
        ent({{"classname", "target_relay"}, {"targetname", "relais"}, {"target", "tuer"}, {"origin", "16 16 16"}}),
    };
    const std::vector<ModellGrenzen> altModelle = {{-512, -256, 0, 512, 256, 256}, {384, 128, 0, 448, 192, 96}, {100, 100, 0, 164, 164, 96}};

    // Mission: Licht heller (geaendert), unbenannte Tuer schneller (geaendert), eigene NPC "kyle2" und
    // eigener Skriptstarter, Relais geloescht, worldspawn unveraendert
    std::vector<MapEntity> mission = alt;
    setKey(mission[4], "light", "500");
    setKey(mission[3], "speed", "250");
    mission.erase(mission.begin() + 5);
    mission.push_back(ent({{"classname", "NPC_Kyle"}, {"NPC_targetname", "kyle2"}, {"origin", "64 0 24"}}));
    mission.push_back(ent({{"classname", "target_scriptrunner"}, {"targetname", "start"}, {"usescript", "b2/intro"}, {"origin", "8 8 8"}}));

    // neu (Map-Tab): ein neuer Brush als func_wall *1 - die Tueren rutschen auf *2/*3; worldspawn-Nachricht
    // geaendert; der Skriptstarter "start" steht schon in der Karte (vom Map-Tab gesetzt); Relais noch da
    const std::vector<MapEntity> neu = {
        ent({{"classname", "worldspawn"}, {"message", "neu"}}),
        ent({{"classname", "info_player_start"}, {"origin", "-384 128 24"}}),
        ent({{"classname", "func_wall"}, {"model", "*1"}}),
        ent({{"classname", "func_door"}, {"targetname", "tuer"}, {"model", "*2"}}),
        ent({{"classname", "func_door"}, {"model", "*3"}, {"speed", "100"}}),
        ent({{"classname", "light"}, {"origin", "0 0 128"}, {"light", "300"}}),
        ent({{"classname", "target_relay"}, {"targetname", "relais"}, {"target", "tuer"}, {"origin", "16 16 16"}}),
        ent({{"classname", "target_scriptrunner"}, {"targetname", "start"}, {"usescript", "b2/alt"}, {"origin", "8 8 8"}}),
    };
    const std::vector<ModellGrenzen> neuModelle = {
        {-512, -256, 0, 512, 256, 256}, {0, -32, 0, 64, 32, 128}, {384, 128, 0, 448, 192, 96}, {100, 100, 0, 164, 164, 96}};

    std::vector<SkriptVerweis> verweise;
    {
        Script sc;
        std::vector<Diag> d;
        (void)readScript("affect ( \"kyle2\", FLUSH )\n{\n\tuse ( \"tuer\" );\n}\n", sc, d);
        skriptVerweiseSammeln(sc, "b2/intro", verweise);
    }
    pruefe(verweise.size() == 2 && verweise[0].name == "kyle2" && verweise[0].befehl == "affect" && verweise[1].name == "tuer" && verweise[1].befehl == "use",
           "Skriptverweise: affect und use (auch im Block)");

    const KarteMissionErgebnis e = karteMissionAbgleich(alt, altModelle, mission, neu, neuModelle, verweise);
    const auto& r = e.entities;
    std::printf("  Ergebnis: %zu Entities, unveraendert %d, geaendert %d, eigene %d angehaengt / %d schon da, entfernt %d, Befunde %zu\n", r.size(),
                e.unveraendert, e.geaendert, e.eigeneAngehaengt, e.eigeneSchonDa, e.entfernt, e.befunde.size());
    pruefe(!r.empty() && r[0].classname == "worldspawn" && w(r[0], "message") == "neu", "worldspawn: unveraendert in der Mission - die neue Karte gilt");
    pruefe(finde(r, "func_wall", nullptr) >= 0, "der neue Brush (func_wall) ist da");
    const int t = finde(r, "func_door", "tuer");
    pruefe(t >= 0 && w(r[static_cast<std::size_t>(t)], "model") == "*2", "Tuer \"tuer\": unveraendert, model jetzt *2");
    int unbenannt = -1;
    for (std::size_t i = 0; i < r.size(); ++i) {
        if (r[i].classname == "func_door" && r[i].targetname.empty()) { unbenannt = static_cast<int>(i); }
    }
    pruefe(unbenannt >= 0 && w(r[static_cast<std::size_t>(unbenannt)], "speed") == "250" && w(r[static_cast<std::size_t>(unbenannt)], "model") == "*3",
           "unbenannte Tuer (Regel 3: Grenzen von *2 = neue *3): speed 250 aus der Mission, model *3");
    pruefe(befund(e, KarteMissionBefund::Art::ModellAngepasst, "*2"), "Befund: *2 zu *3 angepasst (Hinweis)");
    const int l = finde(r, "light", nullptr);
    pruefe(l >= 0 && w(r[static_cast<std::size_t>(l)], "light") == "500", "Licht (Regel 4 Ort): light 500 aus der Mission");
    pruefe(finde(r, "target_relay", "relais") < 0, "Relais: von der Mission geloescht, auch in der neuen Karte weg");
    pruefe(zaehle(r, "target_scriptrunner") == 1 && w(r[static_cast<std::size_t>(finde(r, "target_scriptrunner", "start"))], "usescript") == "b2/intro",
           "eigener Skriptstarter \"start\" stand schon in der Karte: nicht doppelt, usescript der Mission");
    pruefe(zaehle(r, "NPC_Kyle") == 1, "eigene Figur kyle2 angehaengt");
    pruefe(e.unveraendert == 3 && e.geaendert == 2 && e.eigeneAngehaengt == 1 && e.eigeneSchonDa == 1 && e.entfernt == 1, "Zaehler stimmen");
    pruefe(!e.gebrochen(), "nichts gebrochen (nur der Hinweis *2 -> *3)");
    // Auswahl: mission[2] (tuer) -> neue Nummer, mission[5] (kyle2) -> angehaengt
    pruefe(e.neuVonMission.size() == mission.size() && e.neuVonMission[2] == t && r[static_cast<std::size_t>(e.neuVonMission[5])].classname == "NPC_Kyle",
           "neuVonMission: Auswahl findet dieselben Entities wieder");

    // Gegenprobe: im Map-Tab "tuer" umbenannt, Tuer *2 ohne Namen geloescht, eine eigene Entity der Mission zeigt auf *9
    {
        std::vector<MapEntity> neu2 = neu;
        setKey(neu2[3], "targetname", "tor");
        neu2.erase(neu2.begin() + 4);
        std::vector<MapEntity> m2 = mission;
        m2.push_back(ent({{"classname", "func_static"}, {"model", "*9"}}));
        m2.push_back(ent({{"classname", "func_static"}, {"model", "*1"}, {"targetname", "statue"}}));
        const KarteMissionErgebnis g = karteMissionAbgleich(alt, altModelle, m2, neu2, neuModelle, verweise);
        for (const KarteMissionBefund& b : g.befunde) { std::printf("    Befund %d %s @ %s %s\n", static_cast<int>(b.art), b.was.c_str(), b.wo.c_str(), b.neu.c_str()); }
        pruefe(befund(g, KarteMissionBefund::Art::NameFehlt, "tuer"), "Gegenprobe: \"tuer\" umbenannt - Skript use(\"tuer\") gemeldet");
        pruefe(befund(g, KarteMissionBefund::Art::EntityNichtInKarte, "func_door"), "Gegenprobe: geaenderte Tuer ohne Namen im Map-Tab geloescht - gemeldet, angehaengt");
        pruefe(befund(g, KarteMissionBefund::Art::ModellFehlt, "*9"), "Gegenprobe: eigene Entity mit *9 - fehlt in der neuen Karte");
        pruefe(befund(g, KarteMissionBefund::Art::ModellVerschoben, "*1"), "Gegenprobe: eigene Entity mit *1 - meint jetzt andere Geometrie");
        pruefe(g.gebrochen(), "Gegenprobe: gebrochen");
    }
    // Ohne jede Aenderung: Ergebnis = neue Karte, keine Befunde
    {
        const KarteMissionErgebnis g = karteMissionAbgleich(alt, altModelle, alt, neu, neuModelle, {});
        bool gleich = g.entities.size() == neu.size();
        for (std::size_t i = 0; gleich && i < neu.size(); ++i) { gleich = g.entities[i].keys == neu[i].keys; }
        pruefe(gleich && g.befunde.empty(), "Mission ohne Aenderung: genau die neue Karte, keine Befunde");
    }
    // Modellgrenzen aus einer .bsp (Lump 7)
    {
        std::string bsp(8 + 18 * 8, '\0');
        bsp.replace(0, 4, "RBSP");
        const std::int32_t ofs = static_cast<std::int32_t>(bsp.size());
        const std::int32_t len = 80;
        std::memcpy(bsp.data() + 8 + 7 * 8, &ofs, 4);
        std::memcpy(bsp.data() + 8 + 7 * 8 + 4, &len, 4);
        const float m[20] = {-1, -2, -3, 4, 5, 6, 0, 0, 0, 0, 10, 20, 30, 40, 50, 60, 0, 0, 0, 0};
        bsp.append(reinterpret_cast<const char*>(m), sizeof(m));
        const std::vector<ModellGrenzen> g = bspModellGrenzen(bsp);
        pruefe(g.size() == 2 && g[0][0] == -1.0F && g[0][5] == 6.0F && g[1][0] == 10.0F && g[1][5] == 60.0F, "bspModellGrenzen: zwei Modelle aus Lump 7");
        pruefe(bspModellGrenzen("xxxx").empty(), "bspModellGrenzen: keine .bsp - leer");
    }
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "kartemission: alles gut" : "kartemission: FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
