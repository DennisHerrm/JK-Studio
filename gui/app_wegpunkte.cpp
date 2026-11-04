// app_wegpunkte.cpp - das Wegnetz der NPCs in der Kartenansicht
//
// Gewuenscht (11.10.2026): "npc waypoints erstellen ... bzw. er die automatisch
// erstellt". Der Teil ohne Oberflaeche steht in bhed/wegpunkte.h (dort auch,
// wie die Engine das Netz baut). Hier:
//
//   - das Netz in der Ansicht: Wegpunkte, Ziele, Kampfpunkte, Kanten (gruen
//     vom Designer, blau automatisch, orange Sprung), Befunde rot umrandet
//   - Setzen: Klick auf den Boden setzt einen Wegpunkt und haengt ihn an den
//     vorigen (eine Kette, wie die Designer es tun); Umschalt beginnt neu
//   - Verbinden: erst einen, dann einen zweiten Punkt anklicken - target in
//     einem freien Feld (Strg: targetJump); schon verbunden = trennen
//   - Erzeugen: vom Spielerstart, den Figuren und Zielen aus ueber den Boden
//   - Pruefen: im Festen, schwebend, allein, target fehlt
//   - rc609 W1 (Mike): Loeschen raeumt die Verweise ab, neue und verschobene
//     Wegpunkte verbinden sich mit den Nachbarn, die die Kantenpruefung des
//     Spiels bestehen (NavKantenPruefer), Kopien zeigen untereinander auf die
//     neuen Namen, Umbenennen nach Muster
//   - rc609 W3b (Mike: "highlight/draw an area and then click generate. Like
//     paint a path around the map."): Bereiche malen (Pinsel, Lasso; Sperr-
//     bereiche), Pfad malen, Startpunkt per Klick, Sprungkanten, nicht
//     angebundene Teile farbig mit "Anschliessen"; die Erzeugung rechnet im
//     Hintergrund mit Fortschritt und "Abbrechen"
#include "app_internal.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <set>
#include <cctype>
#include <tuple>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "bhed/sicherschreiben.h"
#include "bhed/utf8pfad.h"

namespace bhed::gui {
namespace {

constexpr const char* kErzeugtPrefix = "wpauto_";
const char* const kZielFelder[4] = {"target", "target2", "target3", "target4"};

bool wegKlasse(const MapEntity& e) { return istWegKlasse(e.classname); }

std::size_t pruefsumme() {
    std::size_t h = std::hash<std::string>{}(g_app->map.path);
    // Die NUMMER gehoert dazu: WegKnoten::entity ist ein Index. Ohne sie
    // blieb das Netz nach dem Loeschen einer Entity davor stehen und zeigte
    // eine Stelle zu weit (Code-Pruefung rc592).
    h = h * 1099511628211ULL ^ g_app->map.entities.size();
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        if (!wegKlasse(e)) { continue; }
        h = h * 1099511628211ULL ^ i;
        for (const auto& [k, v] : e.keys) {
            h = h * 1099511628211ULL ^ std::hash<std::string>{}(k);
            h = h * 1099511628211ULL ^ std::hash<std::string>{}(v);
        }
    }
    return h;
}

WegStrahl kartenStrahl() {
    return [](const std::array<float, 3>& a, const std::array<float, 3>& b, std::array<float, 3>& t,
              std::array<float, 3>& n) {
        if (g_app->geo.empty()) { return false; }
        const TraceTreffer r = traceRay(g_app->geo, a.data(), b.data());
        if (!r.hit) { return false; }
        for (int k = 0; k < 3; ++k) {
            t[static_cast<std::size_t>(k)] = r.point[k];
            n[static_cast<std::size_t>(k)] = r.normal[k];
        }
        return true;
    };
}

// rc609 W3b: der Boden fuer "schwebend" wie die Engine - Drop To Floor spurt mit allen Entities (ViewTrace,
// 1,5 x MAX_EDGE_FLOOR_DIST tief, g_navigator.cpp:1573-1595 MD), auch gegen Boeden aus func_static.
WegStrahl kartenBodenStrahl() {
    auto h = std::make_shared<NavBauHilfe>(navBauHilfeKarte());
    return [h](const std::array<float, 3>& a, const std::array<float, 3>& b, std::array<float, 3>& t, std::array<float, 3>& n) {
        if (!h->spur) { return false; }
        const NavSpur s = h->spur(a, {0, 0, 0}, {0, 0, 0}, b, kNavKeineEntity, kNavSolid | kNavTerrain | kNavMonsterClip);
        if (s.fraction >= 1.0F) { return false; }
        t = s.endpos;
        n = s.normal;
        return true;
    };
}

// Freie Sichtlinie auf Brusthoehe (fuer die automatischen Kanten).
WegSicht kartenSicht() {
    return [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
        if (g_app->geo.empty()) { return true; }
        const float pa[3] = {a[0], a[1], a[2] + 16.0F};
        const float pb[3] = {b[0], b[1], b[2] + 16.0F};
        return !traceRay(g_app->geo, pa, pb).hit;
    };
}

int entityMitName(const std::string& name) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (g_app->map.entities[i].targetname == name && wegKlasse(g_app->map.entities[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Hat e ein target-Feld auf `name`? Dann welches (0..3, 4 = targetJump), sonst -1.
int zielFeld(const MapEntity& e, const std::string& name) {
    for (int z = 0; z < 4; ++z) {
        if (const std::string* t = e.find(kZielFelder[z]); t != nullptr && *t == name) { return z; }
    }
    if (const std::string* t = e.find("targetJump"); t != nullptr && *t == name) { return 4; }
    return -1;
}

void keyWeg(MapEntity& e, const std::string& key) {
    e.keys.erase(std::remove_if(e.keys.begin(), e.keys.end(), [&](const auto& kv) { return kv.first == key; }),
                 e.keys.end());
}

// Einen Namen geben, wenn er fehlt (eine Kante braucht den targetname).
std::string nameVon(int i) {
    MapEntity& e = g_app->map.entities[static_cast<std::size_t>(i)];
    if (e.targetname.empty()) { setKey(e, "targetname", freierEntityName("wp")); }
    return e.targetname;
}

// rc609 W1: die Felder, ueber die das Spiel Kanten bildet (NUM_TARGETS,
// g_navigator.cpp:1607-1686 MD: target..target4, targetJump als Sprung).
const char* const kVerweisFelder[5] = {"target", "target2", "target3", "target4", "targetJump"};

std::string kleinW(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Wegpunkte im engeren Sinn: die das Spiel NICHT selbst verbindet (nur Kampf-
// und Zielpunkte bekommen Auto-Connect, g_navigator.cpp:1688-1800 MD).
bool gehWegpunkt(const MapEntity& e) { return e.classname == "waypoint" || e.classname == "waypoint_small"; }

int freiesFeld(const MapEntity& e) {
    for (int z = 0; z < 4; ++z) {
        const std::string* t = e.find(kZielFelder[z]);
        if (t == nullptr || t->empty()) { return z; }
    }
    return -1;
}

bool verbunden(int a, int b) {
    const MapEntity& ea = g_app->map.entities[static_cast<std::size_t>(a)];
    const MapEntity& eb = g_app->map.entities[static_cast<std::size_t>(b)];
    return (!eb.targetname.empty() && zielFeld(ea, eb.targetname) >= 0) ||
           (!ea.targetname.empty() && zielFeld(eb, ea.targetname) >= 0);
}

float abstandZwischen(int a, int b) {
    float oa[3];
    float ob[3];
    if (!entityOrt(g_app->map.entities[static_cast<std::size_t>(a)], oa) ||
        !entityOrt(g_app->map.entities[static_cast<std::size_t>(b)], ob)) {
        return 1.0e9F;
    }
    return std::sqrt((oa[0] - ob[0]) * (oa[0] - ob[0]) + (oa[1] - ob[1]) * (oa[1] - ob[1]) + (oa[2] - ob[2]) * (oa[2] - ob[2]));
}

// Eine Kante a <-> b schreiben, wenn sie fehlt und das Spiel sie behaelt: in ein
// freies Feld von a, sonst von b (das Spiel verbindet in beide Richtungen,
// connect_node, g_navigator.cpp:1652-1684 MD - eine Kante genuegt). 1 neu, 0 nicht begehbar
// oder schon da, -1 beide voll.
int kanteSchreiben(int a, int b, NavKantenPruefer& p) {
    if (verbunden(a, b)) { return 0; }
    int halter = a;
    int ziel = b;
    if (freiesFeld(g_app->map.entities[static_cast<std::size_t>(a)]) < 0) {
        halter = b;
        ziel = a;
    }
    const int feld = freiesFeld(g_app->map.entities[static_cast<std::size_t>(halter)]);
    if (feld < 0) { return -1; }
    // Die Spur geht vom Punkt mit dem Feld zum Ziel (CWayEdge mNodeA -> mNodeB).
    if (!p.pruefe(halter, ziel).begehbar) { return 0; }
    const std::string name = nameVon(ziel);
    setKey(g_app->map.entities[static_cast<std::size_t>(halter)], kZielFelder[feld], name);
    return 1;
}

void wegTeileBerechnen();

}  // namespace

// --- rc609 W3a: die Welt der Erzeugung ------------------------------------------

std::unique_ptr<WegWeltKarte> wegWeltKarte() { return wegWeltKarte(g_app->map.entities); }

std::unique_ptr<WegWeltKarte> wegWeltKarte(std::vector<MapEntity> ents) {
    auto k = std::make_unique<WegWeltKarte>();
    k->ents = std::move(ents);
    k->hilfe = navBauHilfeKarte(k->ents);
    if (!k->hilfe.spur) { return k; }
    // rc609 W3b: Die Welt rechnet auch in einem anderen Faden - gi.inPVS braucht der Netzbau der Wegpunkte nicht (nur die
    // Auto-Kanten der Kampf- und Zielpunkte, die hier nicht entstehen), und es liest die geladene Karte.
    k->hilfe.pvs = nullptr;
    // Tueren offen, wie NAV::TestEdge eine Tuerkette fuer die zweite Spur oeffnet ("door_is_start_open ?
    // slave->pos1 : slave->pos2", g_navigator.cpp:1313-1343 MD): sie oeffnen sich vor der Figur.
    for (std::size_t i = 0; i < k->ents.size(); ++i) {
        const MapEntity& e = k->ents[i];
        NavVec p1{};
        NavVec p2{};
        if (e.classname != "func_door" || !k->hilfe.tuerStellungen || !k->hilfe.tuerStellungen(static_cast<int>(i), p1, p2)) { continue; }
        const std::string* sf = e.find("spawnflags");
        const bool startOffen = sf != nullptr && (std::atoi(sf->c_str()) & 1) != 0;
        k->tueren.push_back(NavVersetztEintrag{static_cast<int>(i), startOffen ? p1 : p2, true});
    }
    k->pruefer = std::make_unique<NavKantenPruefer>(k->ents, k->hilfe);
    WegWeltKarte* w = k.get();
    // Pmove der NPCs: pm->tracemask = MASK_NPCSOLID.
    k->welt.spur = [w](const std::array<float, 3>& a, const std::array<float, 3>& mi, const std::array<float, 3>& ma,
                       const std::array<float, 3>& b) {
        const NavSpur s = w->hilfe.spurVersetzt ? w->hilfe.spurVersetzt(a, mi, ma, b, kNavKeineEntity, kNavMaskNpcSolid, w->tueren)
                                                : w->hilfe.spur(a, mi, ma, b, kNavKeineEntity, kNavMaskNpcSolid);
        WegSpurTreffer t;
        t.anteil = s.fraction;
        t.imFesten = s.startsolid || s.allsolid;
        t.ende = s.endpos;
        t.normale = s.normal;
        return t;
    };
    // Ein Wegpunkt, dessen Radius (waypoint_getRadius) kleiner als SC_MEDIUM_RADIUS ist, bekommt in Jedi Academy keine
    // Kante ("Size Too Big", NAV::TestEdge g_navigator.cpp:1211 OpenJK); HardConnect in Jedi Outcast fragt nicht danach.
    // Einen schwebenden (Drop To Floor And Mark Floating, mit den Brush-Entities wie in PHASE 0) nimmt nur ein Flieger.
    k->welt.knoten = [w](const std::array<float, 3>& o) {
        float r = -1.0F;
        bool schwebt = false;
        return w->pruefer->knotenAn(o, &r, &schwebt) && !schwebt && (navJk2Regeln() || r >= 20.0F);
    };
    k->welt.kante = [w](const std::array<float, 3>& a, const std::array<float, 3>& b) { return w->pruefer->pruefeOrte(a, b).begehbar; };
    // rc609 W3b: Sprung von Wegpunkt a nach b, wie STEER::Path ihn nimmt (NPC_TryJump(NextPosition), g_navigator.cpp:5027-5034
    // OpenJK): die Figur steht auf dem Boden unter a (ihr Kasten -24..40 wie der des Wegpunkts, Drop To Floor 90 tief) und
    // springt zum Punkt b. Die Welt am Spawn (PHASE 0); Jedi Outcast kennt keine Sprungkanten.
    if (!navJk2Regeln()) {
        k->welt.sprung = [w](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            const NavSpur boden = w->hilfe.spur(a, {-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 40.0F}, {a[0], a[1], a[2] - 90.0F}, kNavKeineEntity,
                                                kNavMaskNpcSolid);
            if (boden.startsolid || boden.allsolid || boden.fraction >= 1.0F) { return false; }
            return navNpcSprungMoeglich(w->hilfe.spur, boden.endpos, b, kNavMaskNpcSolid);
        };
    }
    return k;
}

// --- Netz -----------------------------------------------------------------------

void wegNetzAktuell() {
    const std::size_t s = pruefsumme();
    // Auch ein LEERES Netz ist gebaut - vorher wurde es dann in jedem Bild
    // neu gebaut, und das Ergebnis von "Netz pruefen" stand nur ein Bild lang.
    if (s == g_app->wegNetzSchl && g_app->wegNetzGebaut) { return; }
    g_app->wegNetzSchl = s;
    g_app->wegNetzGebaut = true;
    g_app->wegNetz = baueWegNetz(g_app->map.entities, kartenSicht());
    g_app->wegGeprueft = false;
    g_app->wegBefunde.clear();
    wegTeileBerechnen();   // rc609 W3b
}

// --- Bearbeiten (je ein Rueckgaengig-Schritt) ------------------------------------

bool wegVerbinden(int von, int nach, bool sprung) {
    const int n = static_cast<int>(g_app->map.entities.size());
    if (von < 0 || nach < 0 || von >= n || nach >= n || von == nach) { return false; }
    std::vector<MapEntity> vorher = g_app->map.entities;
    const std::string nameNach = nameVon(nach);
    const std::string nameVonE = nameVon(von);
    MapEntity& a = g_app->map.entities[static_cast<std::size_t>(von)];
    MapEntity& b = g_app->map.entities[static_cast<std::size_t>(nach)];
    // Schon verbunden (in welcher Richtung auch immer)? Dann trennen.
    const int fa = zielFeld(a, nameNach);
    const int fb = zielFeld(b, nameVonE);
    if (fa >= 0 || fb >= 0) {
        if (fa >= 0) { keyWeg(a, fa == 4 ? "targetJump" : kZielFelder[fa]); }
        if (fb >= 0) { keyWeg(b, fb == 4 ? "targetJump" : kZielFelder[fb]); }
        entitiesGeaendert(std::move(vorher));
        return true;
    }
    if (sprung) {
        setKey(a, "targetJump", nameNach);
        entitiesGeaendert(std::move(vorher));
        return true;
    }
    // Ein freies Feld - erst beim ersten Punkt, sonst andersherum beim zweiten.
    for (MapEntity* e : {&a, &b}) {
        const std::string& ziel = (e == &a) ? nameNach : nameVonE;
        for (const char* f : kZielFelder) {
            const std::string* t = e->find(f);
            if (t == nullptr || t->empty()) {
                setKey(*e, f, ziel);
                entitiesGeaendert(std::move(vorher));
                return true;
            }
        }
    }
    g_app->map.entities = std::move(vorher);
    meldung(tr(Str::WegVoll), "", true);
    return false;
}

int wegSetzen(const float boden[3], int verbindeMit) {
    std::vector<MapEntity> vorher = g_app->map.entities;
    MapEntity e;
    setKey(e, "classname", "waypoint");
    char o[96];
    // Kasten -24..45: Unterkante eine Einheit ueber dem Boden.
    std::snprintf(o, sizeof(o), "%d %d %d", static_cast<int>(std::lround(boden[0])),
                  static_cast<int>(std::lround(boden[1])), static_cast<int>(std::lround(boden[2] + 25.0F)));
    setKey(e, "origin", o);
    setKey(e, "targetname", freierEntityName("wp"));
    const std::string neuName = e.targetname;
    g_app->map.entities.push_back(e);
    const int neu = static_cast<int>(g_app->map.entities.size()) - 1;
    if (verbindeMit >= 0 && verbindeMit < neu && wegKlasse(g_app->map.entities[static_cast<std::size_t>(verbindeMit)])) {
        MapEntity& v = g_app->map.entities[static_cast<std::size_t>(verbindeMit)];
        if (v.targetname.empty()) { setKey(v, "targetname", freierEntityName("wp")); }
        bool gesetzt = false;
        for (const char* f : kZielFelder) {
            const std::string* t = v.find(f);
            if (t == nullptr || t->empty()) {
                setKey(v, f, neuName);
                gesetzt = true;
                break;
            }
        }
        if (!gesetzt) { setKey(g_app->map.entities[static_cast<std::size_t>(neu)], "target", v.targetname); }
    }
    if (g_app->wegAutoVerbinden) { (void)wegKantenAbgleichen({neu}, false); }   // rc609 W1
    entitiesGeaendert(std::move(vorher));
    g_app->hiddenClasses.erase("waypoint");
    return neu;
}

int wegGenerierteEntfernen() {
    std::vector<MapEntity> vorher = g_app->map.entities;
    std::set<std::string> weg;
    for (const MapEntity& e : g_app->map.entities) {
        if (e.targetname.rfind(kErzeugtPrefix, 0) == 0) { weg.insert(e.targetname); }
    }
    if (weg.empty()) { return 0; }
    auto& v = g_app->map.entities;
    v.erase(std::remove_if(v.begin(), v.end(), [&](const MapEntity& e) { return weg.count(e.targetname) != 0U; }),
            v.end());
    // Verweise auf die erzeugten gehen mit (rc609 W1: dieselbe Funktion wie beim Loeschen).
    std::set<std::string> wegKlein;
    for (const std::string& n : weg) { wegKlein.insert(kleinW(n)); }
    (void)verweiseEntfernen(wegKlein);
    g_app->pickedEntity = -1;
    g_app->hiddenEntities.clear();
    g_app->entAuswahl.clear();
    g_app->wegVon = -1;
    g_app->wegLetzter = -1;
    entitiesGeaendert(std::move(vorher));
    entityNummernPruefen();
    return static_cast<int>(weg.size());
}

// --- rc609 W3b: Erzeugung im Hintergrund, Bereiche, Startpunkt per Klick, Sprungkanten -------
//
// Die Erzeugung dauert seit W3a Sekunden (jeditemple_garden 5 s, t1_danger 13 s). Sie rechnet jetzt in einem eigenen
// Faden auf einer Kopie der Entities (WegWeltKarte), die Oberflaeche bleibt bedienbar; am Ende uebernimmt
// wegAuftragPruefen das Ergebnis in EINEM Rueckgaengig-Schritt - nur, wenn sich die Entities inzwischen nicht geaendert
// haben. Gerechnet wird dasselbe wie vorher im Hauptfaden (erzeugeWegpunkte, verteileZiele), das Netz ist gleich.

struct WegAuftrag {
    WegAuftragArt art = WegAuftragArt::Voll;
    std::string karte;
    std::size_t schluessel = 0;              // entitiesSchluessel() beim Start
    std::vector<MapEntity> vorher;           // die Entities beim Start (Rueckgaengig)
    std::vector<MapEntity> basis;            // darauf kommen die neuen Punkte (Voll: ohne die frueher erzeugten)
    bool alteWeg = false;                    // Voll: es wurden erzeugte entfernt
    std::vector<std::array<float, 3>> starts;
    std::vector<int> startEntity;            // je Startpunkt der vorhandene Wegpunkt (Index in basis), sonst -1
    WegErzeugung p;
    int navPunkte = 0;
    float abstand = 192.0F;
    bool mitSprung = false;
    bool anschliessen = false;               // Bereich: danach mit den Nachbarn verbinden (wegKantenAbgleichen)
    std::vector<int> teilVon;                // je Entity von basis: Teil des Netzes beim Start (-1: kein Wegpunkt)
    std::unique_ptr<WegWeltKarte> welt;
    // Ergebnis
    std::vector<NeuerWegpunkt> wp;
    WegErzeugungInfo info;
    std::vector<std::vector<int>> ziele;
    // Sprungkanten: die Knoten (vorhandene Wegpunkte = Entity in basis, neue = -1 - Index in wp)
    std::vector<int> sprungKnoten;
    std::vector<WegSprung> spruenge;
    double sekunden = 0.0;
    double protokollAb = 0.0;                // Hauptfaden: seit wann (ImGui::GetTime), fuer das Protokoll
    int protokollZahl = 0;
    std::atomic<float> anteil{0.0F};
    std::atomic<int> schritt{0};
    std::atomic<bool> abbrechen{false};
    std::atomic<bool> fertig{false};
    std::thread faden;
    WegAuftrag() = default;
    WegAuftrag(const WegAuftrag&) = delete;
    WegAuftrag& operator=(const WegAuftrag&) = delete;
    ~WegAuftrag() {
        abbrechen = true;
        if (faden.joinable()) { faden.join(); }
    }
};

namespace {

// Alle Entities (Klasse, Schluessel, Werte): hat sich waehrend der Erzeugung etwas geaendert?
std::size_t entitiesSchluessel(const std::vector<MapEntity>& v) {
    std::size_t h = std::hash<std::string>{}(g_app->map.path) ^ v.size();
    for (const MapEntity& e : v) {
        for (const auto& [k, w] : e.keys) {
            h = h * 1099511628211ULL ^ std::hash<std::string>{}(k);
            h = h * 1099511628211ULL ^ std::hash<std::string>{}(w);
        }
        h = h * 1099511628211ULL ^ 0x5bd1e995ULL;
    }
    return h;
}

// Wegpunkte (waypoint, waypoint_small) einer Entity-Liste und ihre Kanten (target..target4, targetJump).
struct WegGraph {
    std::vector<int> entity;
    std::vector<std::array<float, 3>> ort;
    std::vector<std::vector<int>> nachbarn;
    std::vector<char> hatSprung;
};
WegGraph wegGraph(const std::vector<MapEntity>& v) {
    WegGraph g;
    std::map<std::string, int> nr;
    for (std::size_t i = 0; i < v.size(); ++i) {
        float o[3];
        if (!gehWegpunkt(v[i]) || !entityOrt(v[i], o)) { continue; }
        if (!v[i].targetname.empty()) { nr[v[i].targetname] = static_cast<int>(g.entity.size()); }
        g.entity.push_back(static_cast<int>(i));
        g.ort.push_back({o[0], o[1], o[2]});
        const std::string* j = v[i].find("targetJump");
        g.hatSprung.push_back(j != nullptr && !j->empty() ? 1 : 0);
    }
    g.nachbarn.resize(g.entity.size());
    for (std::size_t k = 0; k < g.entity.size(); ++k) {
        for (const char* f : kVerweisFelder) {
            const std::string* t = v[static_cast<std::size_t>(g.entity[k])].find(f);
            if (t == nullptr) { continue; }
            const auto it = nr.find(*t);
            if (it == nr.end() || it->second == static_cast<int>(k)) { continue; }
            g.nachbarn[k].push_back(it->second);
            g.nachbarn[static_cast<std::size_t>(it->second)].push_back(static_cast<int>(k));
        }
    }
    return g;
}
// Je Wegpunkt der Teil des Netzes (Zusammenhang ueber alle Kanten, Sprungkanten eingeschlossen).
std::vector<int> wegGraphTeile(const WegGraph& g) {
    std::vector<int> t(g.entity.size(), -1);
    for (std::size_t s0 = 0; s0 < t.size(); ++s0) {
        if (t[s0] >= 0) { continue; }
        std::vector<int> st{static_cast<int>(s0)};
        t[s0] = static_cast<int>(s0);
        while (!st.empty()) {
            const int i = st.back();
            st.pop_back();
            for (const int j : g.nachbarn[static_cast<std::size_t>(i)]) {
                if (t[static_cast<std::size_t>(j)] < 0) {
                    t[static_cast<std::size_t>(j)] = static_cast<int>(s0);
                    st.push_back(j);
                }
            }
        }
    }
    return t;
}

// Der Hauptteil: der Teil mit dem Wegpunkt, der dem Spielerstart am naechsten liegt (info_player_start, sonst ein anderer
// info_player*), ohne Spielerstart der groesste.
int wegHauptTeil(const WegGraph& g, const std::vector<int>& teile, const std::vector<MapEntity>& v) {
    float st[3] = {0.0F, 0.0F, 0.0F};
    bool da = false;
    for (const MapEntity& e : v) {
        if (!da && e.classname == "info_player_start" && entityOrt(e, st)) { da = true; }
    }
    for (const MapEntity& e : v) {
        if (!da && e.classname.rfind("info_player", 0) == 0 && entityOrt(e, st)) { da = true; }
    }
    int haupt = -1;
    if (da) {
        float best = 1.0e30F;
        for (std::size_t k = 0; k < g.ort.size(); ++k) {
            const float d = std::hypot(std::hypot(g.ort[k][0] - st[0], g.ort[k][1] - st[1]), g.ort[k][2] - st[2]);
            if (d < best) {
                best = d;
                haupt = teile[k];
            }
        }
    }
    if (haupt < 0) {
        std::map<int, int> groesse;
        for (const int t : teile) { ++groesse[t]; }
        for (const auto& [t, n] : groesse) {
            if (haupt < 0 || n > groesse[haupt]) { haupt = t; }
        }
    }
    return haupt;
}

bool inSperre(const std::array<float, 3>& boden) {
    for (const WegBereich& b : g_app->wegBereiche) {
        if (b.art == WegBereich::Art::Sperre && imWegBereich(b, boden)) { return true; }
    }
    return false;
}

// Die Teile des Netzes fuer die Ansicht: Wegpunkte ausserhalb des Hauptteils (wegAbseits) und wie viele Teile.
void wegTeileBerechnen() {
    const WegGraph g = wegGraph(g_app->map.entities);
    const std::vector<int> t = wegGraphTeile(g);
    const int haupt = wegHauptTeil(g, t, g_app->map.entities);
    g_app->wegAbseits.assign(g_app->map.entities.size(), 0);
    std::set<int> fremd;
    for (std::size_t k = 0; k < g.entity.size(); ++k) {
        if (t[k] == haupt) { continue; }
        g_app->wegAbseits[static_cast<std::size_t>(g.entity[k])] = 1;
        fremd.insert(t[k]);
    }
    g_app->wegTeileZahl = static_cast<int>(fremd.size());
}

// Rechnet im Faden des Auftrags (nur die Kopien, kein g_app).
void wegAuftragRechnen(WegAuftrag& a) {
    const auto t0 = std::chrono::steady_clock::now();
    a.p.fortschritt = [&a](float anteil, int schritt) {
        a.anteil = (a.mitSprung ? 0.8F : 1.0F) * anteil;
        a.schritt = schritt;
        return !a.abbrechen.load();
    };
    if (a.art != WegAuftragArt::Sprung) {
        a.wp = erzeugeWegpunkte(a.starts, a.welt->welt, a.p, &a.info);
        if (!a.info.abgebrochen) { a.ziele = verteileZiele(a.wp, 4); }
    }
    if (a.mitSprung && !a.info.abgebrochen && !a.abbrechen) {
        a.schritt = 5;
        a.anteil = 0.8F;
        // Das Netz nach dem Erzeugen: die vorhandenen Wegpunkte mit ihren Kanten, dazu die neuen Punkte.
        const WegGraph g = wegGraph(a.basis);
        std::map<int, int> knotenVonEntity;
        for (std::size_t k = 0; k < g.entity.size(); ++k) { knotenVonEntity[g.entity[k]] = static_cast<int>(k); }
        std::vector<std::array<float, 3>> orte = g.ort;
        std::vector<std::vector<int>> nachbarn = g.nachbarn;
        std::vector<char> hatSprung = g.hatSprung;
        a.sprungKnoten = g.entity;
        std::vector<int> knotenVonWp(a.wp.size(), -1);
        for (std::size_t i = 0; i < a.wp.size(); ++i) {
            const int se = a.wp[i].start >= 0 ? a.startEntity[static_cast<std::size_t>(a.wp[i].start)] : -1;
            if (se >= 0 && knotenVonEntity.count(se) != 0U) {
                knotenVonWp[i] = knotenVonEntity[se];
                continue;
            }
            knotenVonWp[i] = static_cast<int>(orte.size());
            orte.push_back(a.wp[i].ort);
            nachbarn.emplace_back();
            hatSprung.push_back(0);
            a.sprungKnoten.push_back(-1 - static_cast<int>(i));
        }
        for (std::size_t i = 0; i < a.wp.size(); ++i) {
            for (const int j : a.wp[i].nachbarn) { nachbarn[static_cast<std::size_t>(knotenVonWp[i])].push_back(knotenVonWp[static_cast<std::size_t>(j)]); }
        }
        a.spruenge = sprungKanten(orte, nachbarn, hatSprung, a.welt->welt, [&a] { return !a.abbrechen.load(); });
    }
    a.sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    a.anteil = 1.0F;
}

// Uebernimmt das Ergebnis (Hauptfaden): ein Rueckgaengig-Schritt.
void wegAuftragAnwenden(WegAuftrag& a) {
    if (a.abbrechen || a.info.abgebrochen) {
        g_app->wegMeldung = tr(Str::WegAbgebrochen);
        return;
    }
    if (g_app->map.path != a.karte || entitiesSchluessel(g_app->map.entities) != a.schluessel) {
        g_app->wegMeldung = tr(Str::WegVerworfen);
        meldung(g_app->wegMeldung, "", true);
        return;
    }
    const std::size_t vorherZahl = a.basis.size();
    g_app->map.entities = a.basis;
    if (a.alteWeg) {
        g_app->pickedEntity = -1;
        g_app->hiddenEntities.clear();
        g_app->entAuswahl.clear();
        g_app->wegVon = -1;
        g_app->wegLetzter = -1;
    }
    if (a.art != WegAuftragArt::Sprung) {
        g_app->wegAbstandGenutzt = a.info.abstand;
        g_app->wegLuecken = a.info.luecken;
    }
    const std::vector<NeuerWegpunkt>& wp = a.wp;
    // Ein Knoten aus einem vorhandenen Wegpunkt ist dieser Wegpunkt (keine
    // Kopie an derselben Stelle): die erzeugten Nachbarn nennen ihn.
    std::vector<int> vorhanden(wp.size(), -1);
    std::vector<std::string> namen(wp.size());
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (wp[i].start >= 0) { vorhanden[i] = a.startEntity[static_cast<std::size_t>(wp[i].start)]; }
        if (vorhanden[i] >= 0) { namen[i] = nameVon(vorhanden[i]); }
    }
    // Namen: wpauto_1 ... (frei, falls die Karte schon solche hat)
    int nr = 1;
    std::set<std::string> belegt;
    for (const MapEntity& e : g_app->map.entities) { belegt.insert(e.targetname); }
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (vorhanden[i] >= 0) { continue; }
        std::string n;
        do { n = std::string(kErzeugtPrefix) + std::to_string(nr++); } while (belegt.count(n) != 0U);
        namen[i] = n;
    }
    int kanten = 0;
    int neuZahl = 0;
    std::vector<int> index(wp.size(), -1);   // Entity je Knoten
    std::vector<int> neue;
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (vorhanden[i] >= 0) {
            index[i] = vorhanden[i];
            continue;
        }
        MapEntity e;
        setKey(e, "classname", "waypoint");
        char o[96];
        std::snprintf(o, sizeof(o), "%d %d %d", static_cast<int>(std::lround(wp[i].ort[0])),
                      static_cast<int>(std::lround(wp[i].ort[1])), static_cast<int>(std::lround(wp[i].ort[2])));
        setKey(e, "origin", o);
        setKey(e, "targetname", namen[i]);
        for (std::size_t z = 0; z < a.ziele[i].size() && z < 4; ++z) {
            setKey(e, kZielFelder[z], namen[static_cast<std::size_t>(a.ziele[i][z])]);
            ++kanten;
        }
        g_app->map.entities.push_back(e);
        index[i] = static_cast<int>(g_app->map.entities.size()) - 1;
        neue.push_back(index[i]);
        ++neuZahl;
    }
    // Die Kanten, die ein vorhandener Wegpunkt tragen soll: in seine freien
    // Felder, sonst in ein freies des Nachbarn. rc609 W3b: ausser der vollen Erzeugung keine neue Kante zwischen zwei
    // vorhandenen Wegpunkten desselben Teils - das Netz, das schon zusammenhaengt, bleibt, wie es ist.
    const auto teilGleich = [&](int x, int y) {
        return a.art != WegAuftragArt::Voll && x < static_cast<int>(a.teilVon.size()) && y < static_cast<int>(a.teilVon.size()) &&
               a.teilVon[static_cast<std::size_t>(x)] >= 0 && a.teilVon[static_cast<std::size_t>(x)] == a.teilVon[static_cast<std::size_t>(y)];
    };
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (vorhanden[i] < 0) { continue; }
        for (const int j : a.ziele[i]) {
            const int ea = index[i];
            const int eb = index[static_cast<std::size_t>(j)];
            if (verbunden(ea, eb) || (vorhanden[static_cast<std::size_t>(j)] >= 0 && teilGleich(ea, eb))) { continue; }
            int halter = ea;
            int feld = freiesFeld(g_app->map.entities[static_cast<std::size_t>(ea)]);
            if (feld < 0) {
                halter = eb;
                feld = freiesFeld(g_app->map.entities[static_cast<std::size_t>(eb)]);
            }
            if (feld < 0) { continue; }
            setKey(g_app->map.entities[static_cast<std::size_t>(halter)], kZielFelder[feld],
                   namen[halter == ea ? static_cast<std::size_t>(j) : i]);
            ++kanten;
        }
    }
    // Sprungkanten: targetJump am Traeger (g_navigator.cpp:2381 OpenJK: mTargets[4] = targetJump).
    int spruenge = 0;
    const auto entityVonKnoten = [&](int k) {
        const int s = a.sprungKnoten[static_cast<std::size_t>(k)];
        return s >= 0 ? s : index[static_cast<std::size_t>(-1 - s)];
    };
    for (const WegSprung& s : a.spruenge) {
        const int von = entityVonKnoten(s.von);
        const int nach = entityVonKnoten(s.nach);
        if (von < 0 || nach < 0) { continue; }
        setKey(g_app->map.entities[static_cast<std::size_t>(von)], "targetJump", nameVon(nach));
        ++spruenge;
    }
    // Bereich: an die Wegpunkte ringsum in zwei Rasterweiten (so weit reichen die Kanten der Erzeugung, W3a).
    if (a.anschliessen && !neue.empty()) { (void)wegKantenAbgleichen(neue, false, {}, 2.0F * a.abstand); }
    if (a.alteWeg) { entityNummernPruefen(); }
    entitiesGeaendert(std::move(a.vorher));
    g_app->hiddenClasses.erase("waypoint");
    g_app->zeigeWegnetz = true;
    // Bericht
    char z[240];
    if (a.art == WegAuftragArt::Voll) {
        std::snprintf(z, sizeof(z), trN(Str::WegErzeugt, neuZahl, kanten), neuZahl, kanten, a.sekunden);
    } else {
        std::snprintf(z, sizeof(z), trN(Str::WegErzeugtTeil, neuZahl, kanten, spruenge), neuZahl, kanten, spruenge, a.sekunden);
    }
    std::string bericht = z;
    if (a.art != WegAuftragArt::Sprung && a.info.abstand > a.abstand + 0.5F) {
        std::snprintf(z, sizeof(z), tr(Str::WegRasterWeiter), static_cast<double>(a.info.abstand), a.p.maxKnoten + a.navPunkte);
        bericht += " - " + std::string(z);
    }
    if (!a.info.luecken.empty()) {
        const auto& l = a.info.luecken.front().anker;
        const int n = static_cast<int>(a.info.luecken.size());
        std::snprintf(z, sizeof(z), trN(Str::WegNichtAngebunden, n), n, static_cast<double>(l[0]), static_cast<double>(l[1]),
                      static_cast<double>(l[2]));
        bericht += " - " + std::string(z);
    }
    g_app->wegMeldung = bericht;
    meldung(g_app->wegMeldung, "");
    diag::detail("Wegpunkte: " + bericht + " (" + std::to_string(g_app->map.entities.size() - vorherZahl) + " Entities neu)");
}

}  // namespace

bool wegAuftragLaeuft() { return g_app->wegAuftrag != nullptr; }

void wegAuftragAbbrechen() {
    if (g_app->wegAuftrag) { g_app->wegAuftrag->abbrechen = true; }
}

void wegAuftragPruefen() {
    if (!g_app->wegAuftrag) { return; }
    WegAuftrag& l = *g_app->wegAuftrag;
    // Im Protokoll alle 5 s der Stand (die Erzeugung dauert auf grossen Karten Sekunden bis Minuten).
    const double jetzt = ImGui::GetTime();
    if (l.protokollAb == 0.0) { l.protokollAb = jetzt; }
    if (!l.fertig && jetzt - l.protokollAb >= 5.0 * static_cast<double>(l.protokollZahl + 1)) {
        ++l.protokollZahl;
        char z[160];
        std::snprintf(z, sizeof(z), "Wegpunkte: Erzeugung laeuft seit %.0f s - %.0f %%, Schritt %d", jetzt - l.protokollAb,
                      100.0 * static_cast<double>(l.anteil.load()), l.schritt.load());
        diag::detail(z);
    }
    if (!l.fertig) { return; }
    const std::shared_ptr<WegAuftrag> a = std::move(g_app->wegAuftrag);
    if (a->faden.joinable()) { a->faden.join(); }
    wegAuftragAnwenden(*a);
}

float wegAuftragAnteil() { return g_app->wegAuftrag ? g_app->wegAuftrag->anteil.load() : 0.0F; }
int wegAuftragSchritt() { return g_app->wegAuftrag ? g_app->wegAuftrag->schritt.load() : 0; }

bool wegAuftragStarten(WegAuftragArt art, const std::vector<std::array<float, 3>>& saat, int bereich, bool hintergrund) {
    if (g_app->geo.empty() || g_app->wegAuftrag) { return false; }
    auto a = std::make_shared<WegAuftrag>();
    a->art = art;
    a->karte = g_app->map.path;
    a->vorher = g_app->map.entities;
    a->schluessel = entitiesSchluessel(a->vorher);
    a->basis = g_app->map.entities;
    a->abstand = g_app->wegAbstand;
    if (art == WegAuftragArt::Bereich) {
        if (bereich < 0 || bereich >= static_cast<int>(g_app->wegBereiche.size())) { return false; }
        a->abstand = g_app->wegBereiche[static_cast<std::size_t>(bereich)].raster;
    }
    // Voll: die frueher erzeugten weg - im selben Rueckgaengig-Schritt (wie wegGenerierteEntfernen, ohne eigenen Schritt).
    if (art == WegAuftragArt::Voll) {
        std::set<std::string> weg;
        for (const MapEntity& e : a->basis) {
            if (e.targetname.rfind(kErzeugtPrefix, 0) == 0) { weg.insert(e.targetname); }
        }
        if (!weg.empty()) {
            a->alteWeg = true;
            auto& v = a->basis;
            v.erase(std::remove_if(v.begin(), v.end(), [&](const MapEntity& e) { return weg.count(e.targetname) != 0U; }), v.end());
            std::set<std::string> wegKlein;
            for (const std::string& n : weg) { wegKlein.insert(kleinW(n)); }
            for (MapEntity& e : v) {
                for (const char* f : kVerweisFelder) {
                    if (const std::string* t = e.find(f); t != nullptr && !t->empty() && wegKlein.count(kleinW(*t)) != 0U) { keyWeg(e, f); }
                }
            }
        }
    }
    // Teile des Netzes beim Start (fuer Anschliessen und die Regel "keine neue Kante im selben Teil").
    const WegGraph g = wegGraph(a->basis);
    const std::vector<int> teile = wegGraphTeile(g);
    a->teilVon.assign(a->basis.size(), -1);
    for (std::size_t k = 0; k < g.entity.size(); ++k) { a->teilVon[static_cast<std::size_t>(g.entity[k])] = teile[k]; }
    int navPunkte = 0;   // was das Spiel schon als Knoten zaehlt (ausser den Wegpunkten, die Startpunkte werden)
    for (const MapEntity& e : a->basis) {
        if (istWegKlasse(e.classname) && !gehWegpunkt(e)) { ++navPunkte; }
    }
    a->navPunkte = navPunkte;
    const auto vorhandeneWegpunkte = [&] {
        for (std::size_t i = 0; i < a->basis.size(); ++i) {
            float o[3];
            if (gehWegpunkt(a->basis[i]) && entityOrt(a->basis[i], o)) {
                a->starts.push_back({o[0], o[1], o[2]});
                a->startEntity.push_back(static_cast<int>(i));
            }
        }
    };
    if (art == WegAuftragArt::Voll) {
        // Startpunkte: Spielerstart, Figuren, Ziele, vorhandene Wegpunkte, die Kamera.
        for (std::size_t i = 0; i < a->basis.size(); ++i) {
            const MapEntity& e = a->basis[i];
            const std::string& c = e.classname;
            // rc609 W1: auch waypoint/waypoint_small (vorher nur die Ziele - ein von
            // Hand gesetzter Wegpunkt in einem Raum ohne Figur blieb ohne Netz).
            const bool start = c.rfind("info_player", 0) == 0 || c.rfind("NPC_", 0) == 0 || c.rfind("npc_", 0) == 0 ||
                               c.rfind("waypoint_navgoal", 0) == 0 || gehWegpunkt(e);
            float o[3];
            if (start && entityOrt(e, o)) {
                a->starts.push_back({o[0], o[1], o[2]});
                a->startEntity.push_back(gehWegpunkt(e) ? static_cast<int>(i) : -1);
            }
        }
        a->starts.push_back({g_app->cam.pos[0], g_app->cam.pos[1], g_app->cam.pos[2]});
        a->startEntity.push_back(-1);
    } else if (art == WegAuftragArt::Bereich) {
        // Saat: die gemalten Punkte, je eine Rasterweite auseinander (jeder Startpunkt wird selbst ein Punkt).
        const WegBereich& b = g_app->wegBereiche[static_cast<std::size_t>(bereich)];
        for (const std::array<float, 3>& q : b.punkte) {
            bool nah = false;
            for (const std::array<float, 3>& s : a->starts) { nah = nah || std::hypot(s[0] - q[0], s[1] - q[1]) < b.raster; }
            if (!nah && !inSperre(q)) {
                a->starts.push_back(q);
                a->startEntity.push_back(-1);
            }
        }
        a->p.saat = static_cast<int>(a->starts.size());
        vorhandeneWegpunkte();
        const WegBereich kopie = b;
        a->p.laufErlaubt = [kopie](const std::array<float, 3>& boden) { return imWegBereich(kopie, boden); };
        a->anschliessen = true;
    } else if (art == WegAuftragArt::Start || art == WegAuftragArt::Anschluss) {
        // Start: der angeklickte Boden; Anschluss: je Teil ausser dem Hauptteil sein erster Wegpunkt. Das Gitter waechst nur
        // von dort; vorhandene Wegpunkte kommen dazu, wenn es sie erreicht. Anschluss bleibt in zwei Rasterweiten um die
        // Teile (so weit reichen die Kanten der Erzeugung).
        std::vector<std::array<float, 3>> saatOrte = saat;
        std::vector<std::array<float, 3>> nahe;
        if (art == WegAuftragArt::Anschluss) {
            const int haupt = wegHauptTeil(g, teile, a->basis);
            std::set<int> gesehen;
            for (std::size_t k = 0; k < g.entity.size(); ++k) {
                if (teile[k] == haupt) { continue; }
                nahe.push_back(g.ort[k]);
                if (gesehen.insert(teile[k]).second) { saatOrte.push_back(g.ort[k]); }
            }
            if (saatOrte.empty()) { return false; }
        }
        for (const std::array<float, 3>& s : saatOrte) {
            a->starts.push_back(s);
            a->startEntity.push_back(-1);
        }
        a->p.saat = static_cast<int>(a->starts.size());
        vorhandeneWegpunkte();
        // Ein Saatpunkt auf einem vorhandenen Wegpunkt ist dieser Wegpunkt.
        for (std::size_t si = 0; si < saatOrte.size(); ++si) {
            for (std::size_t k = saatOrte.size(); k < a->starts.size(); ++k) {
                if (a->starts[k] == a->starts[si]) { a->startEntity[si] = a->startEntity[k]; }
            }
        }
        if (art == WegAuftragArt::Anschluss) {
            const float weite = 2.0F * a->abstand;
            a->p.laufErlaubt = [nahe, weite](const std::array<float, 3>& boden) {
                for (const std::array<float, 3>& q : nahe) {
                    if (std::hypot(q[0] - boden[0], q[1] - boden[1]) <= weite) { return true; }
                }
                return false;
            };
        }
        a->mitSprung = true;
    } else {
        a->mitSprung = true;   // Sprung: nur Sprungkanten
    }
    if (art != WegAuftragArt::Voll) { a->p.nurUnbedeckt = true; }
    a->p.abstand = a->abstand;
    a->p.maxKnoten = std::max(0, a->p.maxKnoten - navPunkte);   // "Max Nav points reached" (NUM_NODES)
    for (const int se : a->startEntity) { a->p.startIstWegpunkt.push_back(se >= 0 ? 1 : 0); }
    // Sperrbereiche: dort nie ein neuer Punkt (auch bei der vollen Erzeugung).
    std::vector<WegBereich> sperren;
    for (const WegBereich& x : g_app->wegBereiche) {
        if (x.art == WegBereich::Art::Sperre) { sperren.push_back(x); }
    }
    if (!sperren.empty()) {
        a->p.punktErlaubt = [sperren](const std::array<float, 3>& boden) {
            for (const WegBereich& b : sperren) {
                if (imWegBereich(b, boden)) { return false; }
            }
            return true;
        };
    }
    a->welt = wegWeltKarte(a->basis);
    if (!a->welt->welt.spur) { return false; }
    g_app->wegMeldung.clear();
    if (!hintergrund) {
        wegAuftragRechnen(*a);
        wegAuftragAnwenden(*a);
        return true;
    }
    WegAuftrag* roh = a.get();
    a->faden = std::thread([roh] {
        wegAuftragRechnen(*roh);
        roh->fertig = true;
    });
    g_app->wegAuftrag = a;
    return true;
}

// --- rc609 W3b: Pfad malen, Bereiche merken ------------------------------------------

int wegPfadSetzen(const std::vector<std::array<float, 3>>& linie) {
    if (g_app->geo.empty() || linie.size() < 2 || g_app->wegAuftrag) { return 0; }
    const std::unique_ptr<WegWeltKarte> welt = wegWeltKarte();
    if (!welt->welt.spur) { return 0; }
    WegPfad pfad = erzeugePfad(linie, welt->welt, g_app->wegAbstand);
    // Sperrbereiche: dort kein Punkt - die Nachbarn davor und danach bleiben getrennt.
    std::vector<int> neuNr(pfad.punkte.size(), -1);
    std::vector<NeuerWegpunkt> punkte;
    for (std::size_t i = 0; i < pfad.punkte.size(); ++i) {
        if (inSperre(pfad.punkte[i].boden)) {
            pfad.ohnePunkt.push_back(pfad.punkte[i].boden);
            continue;
        }
        neuNr[i] = static_cast<int>(punkte.size());
        punkte.push_back(pfad.punkte[i]);
    }
    for (NeuerWegpunkt& w : punkte) {
        std::vector<int> nb;
        for (const int j : w.nachbarn) {
            if (neuNr[static_cast<std::size_t>(j)] >= 0) { nb.push_back(neuNr[static_cast<std::size_t>(j)]); }
        }
        w.nachbarn = nb;
    }
    g_app->wegMarken = pfad.brueche;
    for (const auto& q : pfad.ohnePunkt) { g_app->wegMarken.push_back(q); }
    if (punkte.empty()) {
        g_app->wegMeldung = tr(Str::WegPfadLeer);
        meldung(g_app->wegMeldung, "", true);
        return 0;
    }
    std::vector<MapEntity> vorher = g_app->map.entities;
    const std::vector<std::vector<int>> ziele = verteileZiele(punkte, 4);
    std::set<std::string> belegt;
    for (const MapEntity& e : g_app->map.entities) { belegt.insert(e.targetname); }
    std::vector<std::string> namen(punkte.size());
    int nr = 1;
    for (std::string& n : namen) {
        do { n = std::string(kErzeugtPrefix) + std::to_string(nr++); } while (belegt.count(n) != 0U);
        belegt.insert(n);
    }
    std::vector<int> neue;
    for (std::size_t i = 0; i < punkte.size(); ++i) {
        MapEntity e;
        setKey(e, "classname", "waypoint");
        char o[96];
        std::snprintf(o, sizeof(o), "%d %d %d", static_cast<int>(std::lround(punkte[i].ort[0])), static_cast<int>(std::lround(punkte[i].ort[1])),
                      static_cast<int>(std::lround(punkte[i].ort[2])));
        setKey(e, "origin", o);
        setKey(e, "targetname", namen[i]);
        for (std::size_t z = 0; z < ziele[i].size() && z < 4; ++z) { setKey(e, kZielFelder[z], namen[static_cast<std::size_t>(ziele[i][z])]); }
        g_app->map.entities.push_back(e);
        neue.push_back(static_cast<int>(g_app->map.entities.size()) - 1);
    }
    // An das vorhandene Netz: Auto-Verbinden aus W1 (Umkreis, Kantenpruefung des Spiels), die neuen untereinander nicht.
    for (const int i : neue) {
        std::vector<int> ohne;
        for (const int j : neue) {
            if (j != i) { ohne.push_back(j); }
        }
        (void)wegKantenAbgleichen({i}, false, ohne);
    }
    entitiesGeaendert(std::move(vorher));
    g_app->hiddenClasses.erase("waypoint");
    g_app->zeigeWegnetz = true;
    char z[240];
    const int luecken = static_cast<int>(pfad.brueche.size() + pfad.ohnePunkt.size());
    std::snprintf(z, sizeof(z), trN(Str::WegPfadErgebnis, static_cast<int>(punkte.size()), luecken), static_cast<int>(punkte.size()), luecken);
    g_app->wegMeldung = z;
    if (!g_app->wegMarken.empty()) {
        const auto& m = g_app->wegMarken.front();
        std::snprintf(z, sizeof(z), tr(Str::WegPfadBruch), static_cast<double>(m[0]), static_cast<double>(m[1]), static_cast<double>(m[2]));
        g_app->wegMeldung += " - " + std::string(z);
    }
    meldung(g_app->wegMeldung, "", luecken > 0);
    return static_cast<int>(punkte.size());
}

namespace {
// Wo die Bereiche einer Karte liegen: im Ordner der Einstellungen, je Karte (Name in der .pk3 bzw. Datei) eine Datei.
std::string wegBereichSchluessel() {
    if (g_app->map.path.empty()) { return {}; }
    std::string her = g_app->kartenHerkunft.pfad;
    const std::size_t s = her.find_last_of("/\\");
    if (s != std::string::npos) { her = her.substr(s + 1); }
    return g_app->map.path + "|" + her;
}
std::string wegBereichDatei(const std::string& schluessel) {
    std::string stamm = g_app->map.path;
    const std::size_t s = stamm.find_last_of("/\\");
    if (s != std::string::npos) { stamm = stamm.substr(s + 1); }
    for (char& c : stamm) {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_' && c != '-') { c = '_'; }
    }
    char h[24];
    std::snprintf(h, sizeof(h), "%016llx", static_cast<unsigned long long>(std::hash<std::string>{}(schluessel)));
    return platform::settingsDirectory() + "\\wegbereiche\\" + stamm + "_" + h + ".txt";
}
}  // namespace

void wegBereicheSpeichern() {
    const std::string sch = wegBereichSchluessel();
    if (sch.empty() || sch != g_app->wegBereicheKarte) { return; }
    const std::string datei = wegBereichDatei(sch);
    if (g_app->wegBereiche.empty()) {
        std::error_code ec;
        std::filesystem::remove(utf8Pfad(datei), ec);
        return;
    }
    std::string fehler;
    if (!sicherSchreiben(datei, wegBereicheText(g_app->wegBereiche), &fehler)) { diag::detail("Wegpunkte: Bereiche nicht gespeichert - " + fehler); }
}

void wegBereicheAktuell() {
    const std::string sch = wegBereichSchluessel();
    if (sch == g_app->wegBereicheKarte) { return; }
    g_app->wegBereicheKarte = sch;
    g_app->wegBereiche = sch.empty() ? std::vector<WegBereich>{} : wegBereicheLesen(slurp(wegBereichDatei(sch)));
    g_app->wegMarken.clear();
}

// --- rc609 W1: Verweise, Kanten wie im Spiel, Kopien, Umbenennen -------------------

int verweiseZaehlen(const std::set<std::string>& namenKlein) {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) {
        for (const char* f : kVerweisFelder) {
            if (const std::string* t = e.find(f); t != nullptr && !t->empty() && namenKlein.count(kleinW(*t)) != 0U) { ++n; }
        }
    }
    return n;
}

int verweiseEntfernen(const std::set<std::string>& namenKlein) {
    int n = 0;
    for (MapEntity& e : g_app->map.entities) {
        for (const char* f : kVerweisFelder) {
            if (const std::string* t = e.find(f); t != nullptr && !t->empty() && namenKlein.count(kleinW(*t)) != 0U) {
                keyWeg(e, f);
                ++n;
            }
        }
    }
    return n;
}

std::vector<std::string> wegNachbarNamen(int i) {
    std::vector<std::string> aus;
    const int zahl = static_cast<int>(g_app->map.entities.size());
    if (i < 0 || i >= zahl || !gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(i)])) { return aus; }
    for (int j = 0; j < zahl; ++j) {
        if (j == i || !gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(j)])) { continue; }
        const MapEntity& ei = g_app->map.entities[static_cast<std::size_t>(i)];
        const MapEntity& ej = g_app->map.entities[static_cast<std::size_t>(j)];
        const int fi = ej.targetname.empty() ? -1 : zielFeld(ei, ej.targetname);
        const int fj = ei.targetname.empty() ? -1 : zielFeld(ej, ei.targetname);
        if ((fi >= 0 && fi < 4) || (fj >= 0 && fj < 4)) {
            const std::string n = nameVon(j);
            if (std::find(aus.begin(), aus.end(), n) == aus.end()) { aus.push_back(n); }
        }
    }
    return aus;
}

bool wegKantenAbgleichen(const std::vector<int>& punkte, bool trennen, const std::vector<int>& ohne, float umkreisWunsch) {
    const int zahl = static_cast<int>(g_app->map.entities.size());
    std::vector<int> liste;
    for (const int i : punkte) {
        if (i >= 0 && i < zahl && gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(i)])) { liste.push_back(i); }
    }
    if (liste.empty()) { return false; }
    // Die Spuren und Entities von jetzt (nach dem Setzen/Verschieben).
    const NavBauHilfe hilfe = navBauHilfeKarte();
    NavKantenPruefer pruefer(g_app->map.entities, hilfe);
    int getrennt = 0;
    int neu = 0;
    int voll = 0;
    int imFesten = 0;
    if (trennen) {
        // Kanten, die TestEdge nicht mehr besteht (Sprungkanten prueft das Spiel nicht).
        for (const int i : liste) {
            for (int j = 0; j < zahl; ++j) {
                if (j == i || !istWegKlasse(g_app->map.entities[static_cast<std::size_t>(j)].classname)) { continue; }
                const std::string ni = g_app->map.entities[static_cast<std::size_t>(i)].targetname;
                const std::string nj = g_app->map.entities[static_cast<std::size_t>(j)].targetname;
                const int fi = nj.empty() ? -1 : zielFeld(g_app->map.entities[static_cast<std::size_t>(i)], nj);
                if (fi >= 0 && fi < 4 && !pruefer.pruefe(i, j).begehbar) {
                    keyWeg(g_app->map.entities[static_cast<std::size_t>(i)], kZielFelder[fi]);
                    ++getrennt;
                }
                const int fj = ni.empty() ? -1 : zielFeld(g_app->map.entities[static_cast<std::size_t>(j)], ni);
                if (fj >= 0 && fj < 4 && !pruefer.pruefe(j, i).begehbar) {
                    keyWeg(g_app->map.entities[static_cast<std::size_t>(j)], kZielFelder[fj]);
                    ++getrennt;
                }
            }
        }
    }
    // Verbinden: die Wegpunkte im Umkreis (Rasterweite der Erzeugung), die naechsten zuerst.
    const float umkreis = umkreisWunsch > 0.0F ? umkreisWunsch : g_app->wegAbstand;
    for (const int i : liste) {
        if (!pruefer.hatKnoten(i)) {   // im Festen: das Spiel verwirft den Punkt ("Waypoint in solid!")
            ++imFesten;
            continue;
        }
        std::vector<std::pair<float, int>> nachbarn;
        for (int j = 0; j < zahl; ++j) {
            if (j == i || !gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(j)]) ||
                std::find(ohne.begin(), ohne.end(), j) != ohne.end()) {
                continue;
            }
            const float d = abstandZwischen(i, j);
            if (d <= umkreis) { nachbarn.emplace_back(d, j); }
        }
        std::sort(nachbarn.begin(), nachbarn.end());
        for (const auto& [d, j] : nachbarn) {
            const int r = kanteSchreiben(i, j, pruefer);
            if (r > 0) { neu += r; }
            if (r < 0) { ++voll; }
        }
    }
    char z[200];
    if (imFesten > 0) {
        std::snprintf(z, sizeof(z), trN(Str::WegImFesten, imFesten), imFesten);
        addStatus(z);
    }
    if (voll > 0) {
        std::snprintf(z, sizeof(z), trN(Str::WegAutoVoll, voll), voll);
        addStatus(z);
    }
    if (neu > 0 || getrennt > 0) {
        std::snprintf(z, sizeof(z), trN(Str::WegAutoErgebnis, neu, getrennt), neu, getrennt);
        addStatus(z);
    }
    return neu > 0 || getrennt > 0;
}

int wegLueckenSchliessen(const std::vector<std::vector<std::string>>& gruppen) {
    const NavBauHilfe hilfe = navBauHilfeKarte();
    NavKantenPruefer pruefer(g_app->map.entities, hilfe);
    int neu = 0;
    for (const std::vector<std::string>& g : gruppen) {
        std::vector<int> punkte;
        for (const std::string& n : g) {
            const int i = entityMitName(n);
            if (i >= 0 && gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(i)])) { punkte.push_back(i); }
        }
        // Je Paar, das kuerzeste zuerst (die Felder reichen nicht immer fuer alle).
        std::vector<std::tuple<float, int, int>> paare;
        for (std::size_t a = 0; a < punkte.size(); ++a) {
            for (std::size_t b = a + 1; b < punkte.size(); ++b) {
                paare.emplace_back(abstandZwischen(punkte[a], punkte[b]), punkte[a], punkte[b]);
            }
        }
        std::sort(paare.begin(), paare.end());
        for (const auto& [d, a, b] : paare) {
            if (kanteSchreiben(a, b, pruefer) > 0) { ++neu; }
        }
    }
    return neu;
}

void wegKopienUmschreiben(const std::vector<int>& quellen, const std::vector<int>& kopien) {
    std::map<std::string, std::string> neuName;   // alter Name (klein) -> Name der Kopie
    for (std::size_t k = 0; k < quellen.size() && k < kopien.size(); ++k) {
        const MapEntity& q = g_app->map.entities[static_cast<std::size_t>(quellen[k])];
        const MapEntity& c = g_app->map.entities[static_cast<std::size_t>(kopien[k])];
        if (istWegKlasse(q.classname) && !q.targetname.empty() && c.targetname != q.targetname) {
            neuName[kleinW(q.targetname)] = c.targetname;
        }
    }
    for (const int k : kopien) {
        MapEntity& c = g_app->map.entities[static_cast<std::size_t>(k)];
        if (!istWegKlasse(c.classname)) { continue; }
        for (const char* f : kVerweisFelder) {
            const std::string* t = c.find(f);
            if (t == nullptr) { continue; }
            const auto it = neuName.find(kleinW(*t));
            if (it != neuName.end()) { setKey(c, f, it->second); }
        }
    }
}

int wegUmbenennen(const std::string& muster) {
    std::vector<int> liste;
    for (const int i : entityAuswahl()) {
        if (gehWegpunkt(g_app->map.entities[static_cast<std::size_t>(i)])) { liste.push_back(i); }
    }
    if (liste.empty() || muster.empty()) { return 0; }
    std::vector<MapEntity> vorher = g_app->map.entities;
    std::set<std::string> belegt;   // Namen der anderen (klein)
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        if (std::find(liste.begin(), liste.end(), static_cast<int>(i)) != liste.end()) { continue; }
        belegt.insert(kleinW(g_app->map.entities[i].targetname));
        belegt.insert(kleinW(entitySkriptName(g_app->map.entities[i])));
    }
    const std::size_t raute = muster.find('#');
    std::map<std::string, std::string> neuName;   // alt (klein) -> neu
    int nr = 1;
    for (const int i : liste) {
        std::string n;
        do {
            const std::string zahlText = std::to_string(nr++);
            n = raute == std::string::npos ? muster + zahlText : muster.substr(0, raute) + zahlText + muster.substr(raute + 1);
        } while (belegt.count(kleinW(n)) != 0U);
        belegt.insert(kleinW(n));
        MapEntity& e = g_app->map.entities[static_cast<std::size_t>(i)];
        if (!e.targetname.empty()) { neuName[kleinW(e.targetname)] = n; }
        setKey(e, "targetname", n);
    }
    for (MapEntity& e : g_app->map.entities) {
        for (const char* f : kVerweisFelder) {
            const std::string* t = e.find(f);
            if (t == nullptr) { continue; }
            const auto it = neuName.find(kleinW(*t));
            if (it != neuName.end()) { setKey(e, f, it->second); }
        }
    }
    entitiesGeaendert(std::move(vorher));
    return static_cast<int>(liste.size());
}

bool wegAuswahlVerbinden() {
    std::vector<MapEntity> vorher = g_app->map.entities;
    if (!wegKantenAbgleichen(entityAuswahl(), false)) {
        g_app->map.entities = std::move(vorher);   // nichts geaendert (auch keine neuen Namen)
        return false;
    }
    entitiesGeaendert(std::move(vorher));
    return true;
}

// --- In der Ansicht ----------------------------------------------------------------

void wegUeberlagerung() {
    if (!g_app->zeigeWegnetz || g_app->camViewActive || g_app->geo.empty()) { return; }
    wegNetzAktuell();
    const WegNetz& netz = g_app->wegNetz;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(ImVec2{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1]},
                     ImVec2{g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]}, true);
    const float px = std::max(1.0F, ImGui::GetFontSize() / 13.0F);
    std::vector<ImVec2> schirm(netz.knoten.size());
    std::vector<char> da(netz.knoten.size(), 0);
    for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
        const auto& o = netz.knoten[i].ort;
        if (g_app->draufsicht && o[2] - 24.0F > g_app->draufSchnitt) { continue; }   // weggeschnitten
        const float w[3] = {o[0], o[1], o[2] - 20.0F};   // nahe am Boden zeichnen
        da[i] = kartenPunktAufSchirm(w, schirm[i]) ? 1 : 0;
    }
    for (const WegKante& k : netz.kanten) {
        const auto a = static_cast<std::size_t>(k.a);
        const auto b = static_cast<std::size_t>(k.b);
        if (da[a] == 0 || da[b] == 0) { continue; }
        const ImU32 f = k.sprung ? IM_COL32(255, 160, 50, 230)
                        : k.designer ? IM_COL32(90, 220, 130, 210)
                                     : IM_COL32(100, 160, 255, 170);
        if (k.sprung) {
            // gestrichelt
            const ImVec2 p0 = schirm[a];
            const ImVec2 p1 = schirm[b];
            for (int s = 0; s < 12; s += 2) {
                const float t0 = static_cast<float>(s) / 12.0F;
                const float t1 = static_cast<float>(s + 1) / 12.0F;
                dl->AddLine(ImVec2{p0.x + (p1.x - p0.x) * t0, p0.y + (p1.y - p0.y) * t0},
                            ImVec2{p0.x + (p1.x - p0.x) * t1, p0.y + (p1.y - p0.y) * t1}, f, 3.0F * px);   // rc609 W3b: kraeftiger
            }
        } else {
            dl->AddLine(schirm[a], schirm[b], f, 1.6F * px);
        }
    }
    // rc609 W3b: die Bereiche (Erzeugen blau, Sperre rot), der Strich, der gerade entsteht (gelb), und die Stellen ohne
    // Kante oder Punkt (rote Kreuze).
    const auto strich = [&](const std::vector<std::array<float, 3>>& pk, bool lasso, float pinsel, ImU32 farbe) {
        std::vector<ImVec2> sp;
        for (const auto& q : pk) {
            const float w[3] = {q[0], q[1], q[2] + 2.0F};
            ImVec2 a;
            if (!kartenPunktAufSchirm(w, a)) { continue; }
            sp.push_back(a);
            if (!lasso) {
                const float w2[3] = {q[0] + pinsel, q[1], q[2] + 2.0F};
                ImVec2 r;
                const float rad = kartenPunktAufSchirm(w2, r) ? std::hypot(r.x - a.x, r.y - a.y) : 4.0F * px;
                dl->AddCircleFilled(a, std::min(rad, 4000.0F), (farbe & 0x00FFFFFFU) | 0x30000000U);
            }
        }
        if (sp.size() >= 2) { dl->AddPolyline(sp.data(), static_cast<int>(sp.size()), farbe, lasso ? ImDrawFlags_Closed : ImDrawFlags_None, 2.5F * px); }
        for (const ImVec2& a : sp) { dl->AddCircleFilled(a, 2.5F * px, farbe); }
    };
    for (const WegBereich& b : g_app->wegBereiche) {
        strich(b.punkte, b.lasso, b.pinsel, b.art == WegBereich::Art::Sperre ? IM_COL32(255, 70, 70, 230) : IM_COL32(80, 190, 255, 230));
    }
    if (g_app->wegStrichAktiv) {
        strich(g_app->wegStrich, g_app->wegWerkzeug == 4, g_app->wegWerkzeug == 3 ? g_app->wegPinsel : 0.0F, IM_COL32(255, 220, 60, 240));
    }
    for (const auto& q : g_app->wegMarken) {
        const float w[3] = {q[0], q[1], q[2] + 4.0F};
        ImVec2 a;
        if (!kartenPunktAufSchirm(w, a)) { continue; }
        const float r = 7.0F * px;
        dl->AddLine(ImVec2{a.x - r, a.y - r}, ImVec2{a.x + r, a.y + r}, IM_COL32(255, 60, 60, 255), 3.0F * px);
        dl->AddLine(ImVec2{a.x - r, a.y + r}, ImVec2{a.x + r, a.y - r}, IM_COL32(255, 60, 60, 255), 3.0F * px);
    }
    std::set<int> befundKnoten;
    for (const WegBefund& b : g_app->wegBefunde) { befundKnoten.insert(b.knoten); }
    for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
        if (da[i] == 0) { continue; }
        const WegKnoten& n = netz.knoten[i];
        const ImVec2 c = schirm[i];
        const bool gewaehlt = n.entity == g_app->pickedEntity || n.entity == g_app->wegVon;
        switch (n.art) {
            case WegKnoten::Art::Wegpunkt:
                if (!gewaehlt && n.entity >= 0 && static_cast<std::size_t>(n.entity) < g_app->wegAbseits.size() &&
                    g_app->wegAbseits[static_cast<std::size_t>(n.entity)] != 0) {
                    // rc609 W3b: nicht am Hauptteil des Netzes (vom Spielerstart aus nicht erreichbar)
                    dl->AddCircleFilled(c, 5.0F * px, IM_COL32(255, 60, 220, 255));
                    dl->AddCircle(c, 8.0F * px, IM_COL32(255, 60, 220, 200), 0, 1.6F * px);
                } else {
                    dl->AddCircleFilled(c, 3.5F * px, gewaehlt ? IM_COL32(255, 165, 40, 255) : IM_COL32(110, 220, 240, 255));
                }
                break;
            case WegKnoten::Art::Ziel:
                dl->AddQuadFilled(ImVec2{c.x, c.y - 5.0F * px}, ImVec2{c.x + 5.0F * px, c.y}, ImVec2{c.x, c.y + 5.0F * px},
                                  ImVec2{c.x - 5.0F * px, c.y}, gewaehlt ? IM_COL32(255, 165, 40, 255) : IM_COL32(90, 220, 120, 255));
                break;
            case WegKnoten::Art::Kampf:
                dl->AddRectFilled(ImVec2{c.x - 3.0F * px, c.y - 3.0F * px}, ImVec2{c.x + 3.0F * px, c.y + 3.0F * px},
                                  gewaehlt ? IM_COL32(255, 165, 40, 255) : IM_COL32(240, 100, 90, 230));
                break;
        }
        if (befundKnoten.count(static_cast<int>(i)) != 0U) {
            dl->AddCircle(c, 8.0F * px, IM_COL32(255, 70, 70, 255), 0, 2.0F * px);
        }
    }
    // Verbinden: Gummiband vom ersten Punkt zur Maus.
    if (g_app->wegWerkzeug == 2 && g_app->wegVon >= 0) {
        for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
            if (netz.knoten[i].entity == g_app->wegVon && da[i] != 0) {
                dl->AddLine(schirm[i], ImGui::GetIO().MousePos,
                            ImGui::GetIO().KeyCtrl ? IM_COL32(255, 160, 50, 230) : IM_COL32(90, 220, 130, 230), 2.0F * px);
            }
        }
    }
    dl->PopClipRect();
}

void wegWerkzeugMaus(float rasterSkal) {
    (void)rasterSkal;
    if (g_app->wegKlickVerbraucht) { return; }   // schon vergeben (Animation)
    if (g_app->wegWerkzeug == 0 || g_app->geo.empty() || g_app->camViewActive) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] &&
                        io.MousePos.y >= g_app->kartenAnsicht[1] && io.MousePos.y < g_app->kartenAnsicht[3] &&
                        ImGui::IsWindowHovered();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        g_app->wegVon = -1;
        g_app->wegLetzter = -1;
        g_app->wegStrichAktiv = false;
        g_app->wegStrich.clear();
        if (g_app->wegWerkzeug != 0) { g_app->wegWerkzeug = 0; }
        return;
    }
    // rc609 W3b: Pinsel (3), Lasso (4), Pfad (5) - mit gedrueckter Taste ueber den Boden ziehen; Startpunkt (6) - ein Klick.
    if (g_app->wegWerkzeug >= 3 && g_app->wegWerkzeug <= 5) {
        const bool lasso = g_app->wegWerkzeug == 4;
        const auto dazu = [&] {
            TraceTreffer t;
            if (!kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t) || (!lasso && t.normal[2] < 0.5F)) { return; }
            const std::array<float, 3> q{t.point[0], t.point[1], t.point[2]};
            // je 24 Einheiten ein Punkt (ein Viertel des Pinsels, die Haelfte bei einer Rasterweite von 48)
            if (!g_app->wegStrich.empty() && std::hypot(g_app->wegStrich.back()[0] - q[0], g_app->wegStrich.back()[1] - q[1]) < 24.0F) { return; }
            g_app->wegStrich.push_back(q);
        };
        if (g_app->wegStrichAktiv) {
            g_app->wegKlickVerbraucht = true;
            dazu();
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) { return; }
            g_app->wegStrichAktiv = false;
            const std::vector<std::array<float, 3>> punkte = std::move(g_app->wegStrich);
            g_app->wegStrich.clear();
            if (g_app->wegWerkzeug == 5) {
                (void)wegPfadSetzen(punkte);
            } else if (punkte.size() >= (lasso ? 3U : 1U)) {
                WegBereich b;
                b.art = g_app->wegNeuSperre ? WegBereich::Art::Sperre : WegBereich::Art::Erzeugen;
                b.lasso = lasso;
                b.raster = g_app->wegAbstand;
                b.pinsel = g_app->wegPinsel;
                b.punkte = punkte;
                g_app->wegBereiche.push_back(std::move(b));
                wegBereicheSpeichern();
            }
            return;
        }
        if (!imBild || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return; }
        g_app->wegKlickVerbraucht = true;
        g_app->wegStrichAktiv = true;
        g_app->wegStrich.clear();
        dazu();
        return;
    }
    if (!imBild || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return; }
    g_app->wegKlickVerbraucht = true;
    if (g_app->wegWerkzeug == 6) {
        TraceTreffer t;
        if (!kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t) || t.normal[2] < 0.5F) {
            meldung(tr(Str::WegKeinBoden), "", true);
            return;
        }
        (void)wegAuftragStarten(WegAuftragArt::Start, {{t.point[0], t.point[1], t.point[2]}});
        return;
    }
    if (g_app->wegWerkzeug == 1) {
        TraceTreffer t;
        if (!kartenStrahlUnterMaus(io.MousePos.x, io.MousePos.y, t) || t.normal[2] < 0.5F) {
            meldung(tr(Str::WegKeinBoden), "", true);
            return;
        }
        const int vorige = io.KeyShift ? -1 : g_app->wegLetzter;
        g_app->wegLetzter = wegSetzen(t.point, vorige);
        g_app->pickedEntity = g_app->wegLetzter;
        g_app->zeigeWegnetz = true;
        return;
    }
    // Verbinden: der naechste Netzknoten unter der Maus.
    wegNetzAktuell();
    int best = -1;
    float bestD = ImGui::GetFontSize() * 1.2F;
    bestD *= bestD;
    for (const WegKnoten& n : g_app->wegNetz.knoten) {
        const float w[3] = {n.ort[0], n.ort[1], n.ort[2] - 20.0F};
        ImVec2 s;
        if (!kartenPunktAufSchirm(w, s)) { continue; }
        const float dx = s.x - io.MousePos.x;
        const float dy = s.y - io.MousePos.y;
        if (dx * dx + dy * dy < bestD) {
            bestD = dx * dx + dy * dy;
            best = n.entity;
        }
    }
    if (best < 0) {
        g_app->wegVon = -1;
        return;
    }
    if (g_app->wegVon < 0) {
        g_app->wegVon = best;
        g_app->pickedEntity = best;
        return;
    }
    wegVerbinden(g_app->wegVon, best, io.KeyCtrl);
    // Weiter vom zweiten Punkt aus - so verbindet man eine Kette Klick fuer Klick.
    g_app->wegVon = best;
    g_app->pickedEntity = best;
}

// --- Das Fenster ---------------------------------------------------------------------

void drawWegFenster() {
    wegAuftragPruefen();    // rc609 W3b: Ergebnis der Erzeugung im Hintergrund uebernehmen
    wegBereicheAktuell();   // rc609 W3b: die Bereiche der geladenen Karte
    if (!g_app->wegFensterOffen) {
        g_app->wegWerkzeug = 0;
        return;
    }
    // Waechst mit dem Inhalt (Ergebniszeile, Befundliste) - mit fester Hoehe
    // war "Netz pruefen" abgeschnitten (Wegpunkttest).
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSizeConstraints(ImVec2{fs * 24.0F, 0.0F}, ImVec2{fs * 32.0F, FLT_MAX});
    if (!ImGui::Begin((std::string(tr(Str::WegTitel)) + "###wegpunkte").c_str(), &g_app->wegFensterOffen,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    wegNetzAktuell();
    const WegNetz& netz = g_app->wegNetz;
    int wp = 0;
    int ziele = 0;
    int kampf = 0;
    for (const WegKnoten& n : netz.knoten) {
        wp += n.art == WegKnoten::Art::Wegpunkt ? 1 : 0;
        ziele += n.art == WegKnoten::Art::Ziel ? 1 : 0;
        kampf += n.art == WegKnoten::Art::Kampf ? 1 : 0;
    }
    ImGui::TextDisabled(trN(Str::WegStand, wp, ziele, kampf, netz.kanten.size()), wp, ziele, kampf, static_cast<int>(netz.kanten.size()));
    if (iconTextKnopf(tr(Str::WegZeigen), UiIcon::Pfad, tr(Str::WegZeigenHint))) {
        g_app->zeigeWegnetz = !g_app->zeigeWegnetz;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", g_app->zeigeWegnetz ? tr(Str::WegAn) : tr(Str::WegAus));
    ImGui::SeparatorText(tr(Str::WegWerkzeuge));
    if (iconKnopf(tr(Str::WegSetzen), UiIcon::Wegpunkt, tr(Str::WegSetzenHint), g_app->wegWerkzeug == 1, true, true)) {
        g_app->wegWerkzeug = g_app->wegWerkzeug == 1 ? 0 : 1;
        g_app->wegLetzter = -1;
        g_app->zeigeWegnetz = true;
    }
    ImGui::SameLine();
    if (iconKnopf(tr(Str::WegVerbinden), UiIcon::Pfad, tr(Str::WegVerbindenHint), g_app->wegWerkzeug == 2, true, true)) {
        g_app->wegWerkzeug = g_app->wegWerkzeug == 2 ? 0 : 2;
        g_app->wegVon = -1;
        g_app->zeigeWegnetz = true;
    }
    // Feste Umbruchbreite: im mitwachsenden Fenster schaukelte sich ein
    // Umbruch "bis zum Rand" mit der Fensterbreite auf.
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + fs * 23.0F);
    ImGui::TextDisabled("%s", g_app->wegWerkzeug == 1   ? tr(Str::WegSetzenHint)
                              : g_app->wegWerkzeug == 2 ? tr(Str::WegVerbindenHint)
                                                        : tr(Str::WegWerkzeugHint));
    ImGui::PopTextWrapPos();
    // rc609 W1 (Mike): Verbindungen wie im Spiel
    ImGui::SeparatorText(tr(Str::WegKantenTitel));
    ImGui::Checkbox(tr(Str::WegAutoVerbinden), &g_app->wegAutoVerbinden);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegAutoVerbindenHint)); }
    ImGui::Checkbox(tr(Str::WegLuecke), &g_app->wegLueckeSchliessen);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegLueckeHint)); }
    bool wegGewaehlt = false;
    for (const int i : entityAuswahl()) {
        const std::string& c = g_app->map.entities[static_cast<std::size_t>(i)].classname;
        wegGewaehlt = wegGewaehlt || c == "waypoint" || c == "waypoint_small";
    }
    if (iconTextKnopf(tr(Str::WegNachbarn), UiIcon::Pfad, tr(Str::WegNachbarnHint), wegGewaehlt)) {
        (void)wegAuswahlVerbinden();
    }
    char muster[64];
    std::snprintf(muster, sizeof(muster), "%s", g_app->wegMuster.c_str());
    ImGui::SetNextItemWidth(fs * 9.0F);
    if (ImGui::InputText("##wegmuster", muster, sizeof(muster))) { g_app->wegMuster = muster; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegMusterHint)); }
    ImGui::SameLine();
    if (iconTextKnopf(tr(Str::WegUmbenennen), UiIcon::Namen, tr(Str::WegMusterHint), wegGewaehlt && !g_app->wegMuster.empty())) {
        const int n = wegUmbenennen(g_app->wegMuster);
        char z[120];
        std::snprintf(z, sizeof(z), trN(Str::WegUmbenannt, n), n);
        addStatus(z);
    }
    ImGui::SeparatorText(tr(Str::WegErzeugenTitel));
    if (g_app->geo.empty()) {   // H2: warum "Erzeugen" grau ist
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 23.0F);
        ImGui::TextDisabled("%s", tr(Str::WegOhneKarte));
        ImGui::PopTextWrapPos();
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0F);
    ImGui::SliderFloat("##wegabstand", &g_app->wegAbstand, 96.0F, 384.0F, tr(Str::WegAbstand));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegAbstandHint)); }
    ImGui::SameLine();
    const bool laeuft = wegAuftragLaeuft();
    if (iconTextKnopf(tr(Str::WegErzeugen), UiIcon::Erzeugen, tr(Str::WegErzeugenTipp), !g_app->geo.empty() && !laeuft)) {
        (void)wegAuftragStarten(WegAuftragArt::Voll);
    }
    if (iconTextKnopf(tr(Str::WegEntfernen), UiIcon::Loeschen, tr(Str::WegEntfernenHint), !laeuft)) {
        const int n = wegGenerierteEntfernen();
        char z[120];
        std::snprintf(z, sizeof(z), trN(Str::WegEntfernt, n), n);
        g_app->wegMeldung = z;
    }
    // rc609 W3b: Fortschritt und Abbrechen
    if (laeuft) {
        static const Str kSchritt[6] = {Str::WegSchritt1, Str::WegSchritt1, Str::WegSchritt2, Str::WegSchritt3, Str::WegSchritt4, Str::WegSchritt5};
        const int st = std::clamp(wegAuftragSchritt(), 0, 5);
        ImGui::ProgressBar(wegAuftragAnteil(), ImVec2{fs * 15.0F, 0.0F}, tr(kSchritt[st]));
        ImGui::SameLine();
        if (iconTextKnopf(tr(Str::WegAbbrechen), UiIcon::Schliessen, tr(Str::WegAbbrechenHint))) { wegAuftragAbbrechen(); }
    }
    if (!g_app->wegMeldung.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + fs * 23.0F);
        ImGui::TextDisabled("%s", g_app->wegMeldung.c_str());
        ImGui::PopTextWrapPos();
    }
    // rc609 W3b (Mike): Bereiche und Pfade malen, Startpunkt per Klick
    ImGui::SeparatorText(tr(Str::WegMalenTitel));
    const auto werkzeug = [&](int nr, const char* text, UiIcon icon, const char* hinweis) {
        if (iconKnopf(text, icon, hinweis, g_app->wegWerkzeug == nr, !g_app->geo.empty(), true)) {
            g_app->wegWerkzeug = g_app->wegWerkzeug == nr ? 0 : nr;
            g_app->wegStrichAktiv = false;
            g_app->wegStrich.clear();
            g_app->zeigeWegnetz = true;
        }
    };
    werkzeug(3, tr(Str::WegPinsel), UiIcon::Boden, tr(Str::WegPinselHint));
    ImGui::SameLine();
    werkzeug(4, tr(Str::WegLasso), UiIcon::Raster, tr(Str::WegLassoHint));
    ImGui::SameLine();
    werkzeug(5, tr(Str::WegPfad), UiIcon::Pfad, tr(Str::WegPfadHint));
    ImGui::SameLine();
    werkzeug(6, tr(Str::WegStartpunkt), UiIcon::Zielen, tr(Str::WegStartpunktHint));
    ImGui::SameLine();
    ImGui::Checkbox(tr(Str::WegSperre), &g_app->wegNeuSperre);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegSperreHint)); }
    if (g_app->wegWerkzeug == 3) {
        ImGui::SetNextItemWidth(fs * 9.0F);
        ImGui::SliderFloat("##wegpinsel", &g_app->wegPinsel, 32.0F, 512.0F, tr(Str::WegPinselGroesse));
    }
    int weg = -1;
    for (std::size_t i = 0; i < g_app->wegBereiche.size(); ++i) {
        WegBereich& b = g_app->wegBereiche[i];
        ImGui::PushID(static_cast<int>(i));
        const bool sperre = b.art == WegBereich::Art::Sperre;
        ImGui::TextColored(sperre ? ImVec4{1.0F, 0.45F, 0.45F, 1.0F} : ImVec4{0.45F, 0.8F, 1.0F, 1.0F}, "%d %s", static_cast<int>(i) + 1,
                           sperre ? tr(Str::WegBereichSperre) : (b.lasso ? tr(Str::WegLasso) : tr(Str::WegPinsel)));
        ImGui::SameLine();
        if (!sperre) {
            ImGui::SetNextItemWidth(fs * 6.0F);
            if (ImGui::SliderFloat("##raster", &b.raster, 96.0F, 384.0F, "%.0f")) { b.raster = std::round(b.raster); }
            if (ImGui::IsItemDeactivatedAfterEdit()) { wegBereicheSpeichern(); }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegBereichRasterHint)); }
            ImGui::SameLine();
            if (iconTextKnopf(tr(Str::WegHierErzeugen), UiIcon::Erzeugen, tr(Str::WegHierErzeugenHint), !laeuft)) {
                (void)wegAuftragStarten(WegAuftragArt::Bereich, {}, static_cast<int>(i));
            }
            ImGui::SameLine();
        }
        if (iconKnopf(tr(Str::WegBereichLoeschen), UiIcon::Loeschen, tr(Str::WegBereichLoeschen))) { weg = static_cast<int>(i); }
        ImGui::PopID();
    }
    if (weg >= 0) {
        g_app->wegBereiche.erase(g_app->wegBereiche.begin() + weg);
        wegBereicheSpeichern();
    }
    // rc609 W3b: Teile, die vom Spielerstart aus nicht erreichbar sind (in der Ansicht violett), und Sprungkanten
    if (g_app->wegTeileZahl > 0) {
        ImGui::TextColored(ImVec4{1.0F, 0.35F, 0.85F, 1.0F}, trN(Str::WegTeileAbseits, g_app->wegTeileZahl), g_app->wegTeileZahl);
        ImGui::SameLine();
        if (iconTextKnopf(tr(Str::WegAnschliessen), UiIcon::Pfad, tr(Str::WegAnschliessenHint), !laeuft)) {
            (void)wegAuftragStarten(WegAuftragArt::Anschluss);
        }
    }
    if (iconTextKnopf(tr(Str::WegSprungkanten), UiIcon::Oben, tr(Str::WegSprungkantenHint), !laeuft && !g_app->geo.empty() && !navJk2Regeln())) {
        (void)wegAuftragStarten(WegAuftragArt::Sprung);
    }
    ImGui::SeparatorText(tr(Str::WegPruefenTitel));
    if (iconTextKnopf(tr(Str::WegPruefen), UiIcon::Haken, tr(Str::WegPruefenHint))) {
        g_app->wegBefunde = pruefeWegNetz(netz, g_app->map.entities, kartenStrahl(), kartenBodenStrahl());
        g_app->wegGeprueft = true;
        g_app->zeigeWegnetz = true;
    }
    if (g_app->wegGeprueft) {
        if (g_app->wegBefunde.empty()) {
            ImGui::TextColored(ImVec4{0.45F, 0.85F, 0.5F, 1.0F}, "%s", tr(Str::WegAllesGut));
        } else {
            ImGui::TextColored(ImVec4{1.0F, 0.6F, 0.35F, 1.0F}, trN(Str::WegBefunde, g_app->wegBefunde.size()),
                               static_cast<int>(g_app->wegBefunde.size()));
            ImGui::BeginChild("##wegbefunde", ImVec2{-1.0F, ImGui::GetTextLineHeightWithSpacing() * 8.0F},
                              ImGuiChildFlags_Borders);
            for (std::size_t i = 0; i < g_app->wegBefunde.size(); ++i) {
                const WegBefund& b = g_app->wegBefunde[i];
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(b.text.c_str())) {
                    if (b.knoten >= 0 && static_cast<std::size_t>(b.knoten) < netz.knoten.size()) {
                        const int e = netz.knoten[static_cast<std::size_t>(b.knoten)].entity;
                        g_app->pickedEntity = e;
                        zurEntityFliegen(static_cast<std::size_t>(e));
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

// --- Draufsicht -----------------------------------------------------------------------

void draufsichtSchalten(bool an) {
    if (an == g_app->draufsicht) { return; }
    if (an) {
        // Paket kamera, rc601 (Review): im Pilot ohne dessen Bildwinkel.
        g_app->vorDraufsicht = freieKamera();
        // Schnitt: etwas ueber dem Boden unter der Kamera (eine Etage).
        float boden = g_app->cam.pos[2] - 64.0F;
        if (!g_app->geo.empty()) {
            const float a[3] = {g_app->cam.pos[0], g_app->cam.pos[1], g_app->cam.pos[2]};
            const float b[3] = {g_app->cam.pos[0], g_app->cam.pos[1], g_app->cam.pos[2] - 4096.0F};
            const TraceTreffer t = traceRay(g_app->geo, a, b);
            if (t.hit) { boden = t.point[2]; }
        }
        g_app->draufSchnitt = boden + 160.0F;
        g_app->cam.pos[2] = g_app->draufSchnitt + 1200.0F;
        g_app->cam.angles[0] = 89.0F;
        g_app->cam.angles[2] = 0.0F;
        g_app->throughCamera = false;
    } else {
        g_app->cam = g_app->vorDraufsicht;
    }
    g_app->draufsicht = an;
    g_app->mapDirty = true;
}

}  // namespace bhed::gui
