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
#include "app_internal.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <set>
#include <cctype>
#include <tuple>
#include <map>
#include <string>
#include <vector>

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

}  // namespace

// --- rc609 W3a: die Welt der Erzeugung ------------------------------------------

std::unique_ptr<WegWeltKarte> wegWeltKarte() {
    auto k = std::make_unique<WegWeltKarte>();
    k->hilfe = navBauHilfeKarte();
    if (!k->hilfe.spur) { return k; }
    // Tueren offen, wie NAV::TestEdge eine Tuerkette fuer die zweite Spur oeffnet ("door_is_start_open ?
    // slave->pos1 : slave->pos2", g_navigator.cpp:1313-1343 MD): sie oeffnen sich vor der Figur.
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        NavVec p1{};
        NavVec p2{};
        if (e.classname != "func_door" || !k->hilfe.tuerStellungen || !k->hilfe.tuerStellungen(static_cast<int>(i), p1, p2)) { continue; }
        const std::string* sf = e.find("spawnflags");
        const bool startOffen = sf != nullptr && (std::atoi(sf->c_str()) & 1) != 0;
        k->tueren.push_back(NavVersetztEintrag{static_cast<int>(i), startOffen ? p1 : p2, true});
    }
    k->pruefer = std::make_unique<NavKantenPruefer>(g_app->map.entities, k->hilfe);
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

int wegErzeugen(float abstand, bool alteWeg, std::string* bericht) {
    if (g_app->geo.empty()) { return 0; }
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<MapEntity> vorher = g_app->map.entities;
    // Alte erzeugte zuerst weg - im SELBEN Rueckgaengig-Schritt.
    if (alteWeg && wegGenerierteEntfernen() > 0 && !g_app->entUndo.empty()) {
        g_app->entUndo.pop_back();   // ein Schritt fuer alles (vorher steht davor)
        if (g_app->entUndoStempel.size() > g_app->entUndo.size()) { g_app->entUndoStempel.resize(g_app->entUndo.size()); }   // mission1
    }
    // Startpunkte: Spielerstart, Figuren, Ziele, vorhandene Wegpunkte, die Kamera.
    std::vector<std::array<float, 3>> starts;
    int navPunkte = 0;   // was das Spiel schon als Knoten zaehlt (ausser den Wegpunkten, die Startpunkte werden)
    std::vector<int> startEntity;   // rc609 W1: der vorhandene Wegpunkt des Starts, sonst -1
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const std::string& c = e.classname;
        // rc609 W1: auch waypoint/waypoint_small (vorher nur die Ziele - ein von
        // Hand gesetzter Wegpunkt in einem Raum ohne Figur blieb ohne Netz).
        const bool start = c.rfind("info_player", 0) == 0 || c.rfind("NPC_", 0) == 0 || c.rfind("npc_", 0) == 0 ||
                           c.rfind("waypoint_navgoal", 0) == 0 || gehWegpunkt(e);
        float o[3];
        if (start && entityOrt(e, o)) {
            starts.push_back({o[0], o[1], o[2]});
            startEntity.push_back(gehWegpunkt(e) ? static_cast<int>(i) : -1);
        }
        if (istWegKlasse(c) && !gehWegpunkt(e)) { ++navPunkte; }
    }
    starts.push_back({g_app->cam.pos[0], g_app->cam.pos[1], g_app->cam.pos[2]});
    startEntity.push_back(-1);
    WegErzeugung p;
    p.abstand = abstand;
    p.maxKnoten = std::max(0, p.maxKnoten - navPunkte);   // "Max Nav points reached" (NUM_NODES)
    for (const int se : startEntity) { p.startIstWegpunkt.push_back(se >= 0 ? 1 : 0); }
    const std::unique_ptr<WegWeltKarte> welt = wegWeltKarte();
    WegErzeugungInfo info;
    const std::vector<NeuerWegpunkt> wp = erzeugeWegpunkte(starts, welt->welt, p, &info);
    const float genutzt = info.abstand;
    g_app->wegAbstandGenutzt = genutzt;
    g_app->wegLuecken = info.luecken;
    const std::vector<std::vector<int>> ziele = verteileZiele(wp, 4);
    // Ein Knoten aus einem vorhandenen Wegpunkt ist dieser Wegpunkt (keine
    // Kopie an derselben Stelle): die erzeugten Nachbarn nennen ihn.
    std::vector<int> vorhanden(wp.size(), -1);
    std::vector<std::string> namen(wp.size());
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (wp[i].start >= 0) { vorhanden[i] = startEntity[static_cast<std::size_t>(wp[i].start)]; }
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
        for (std::size_t z = 0; z < ziele[i].size() && z < 4; ++z) {
            setKey(e, kZielFelder[z], namen[static_cast<std::size_t>(ziele[i][z])]);
            ++kanten;
        }
        g_app->map.entities.push_back(e);
        index[i] = static_cast<int>(g_app->map.entities.size()) - 1;
        ++neuZahl;
    }
    // Die Kanten, die ein vorhandener Wegpunkt tragen soll: in seine freien
    // Felder, sonst in ein freies des Nachbarn.
    for (std::size_t i = 0; i < wp.size(); ++i) {
        if (vorhanden[i] < 0) { continue; }
        for (const int j : ziele[i]) {
            const int a = index[i];
            const int b = index[static_cast<std::size_t>(j)];
            if (verbunden(a, b)) { continue; }
            int halter = a;
            int feld = freiesFeld(g_app->map.entities[static_cast<std::size_t>(a)]);
            if (feld < 0) {
                halter = b;
                feld = freiesFeld(g_app->map.entities[static_cast<std::size_t>(b)]);
            }
            if (feld < 0) { continue; }
            setKey(g_app->map.entities[static_cast<std::size_t>(halter)], kZielFelder[feld],
                   namen[halter == a ? static_cast<std::size_t>(j) : i]);
            ++kanten;
        }
    }
    entitiesGeaendert(std::move(vorher));
    g_app->hiddenClasses.erase("waypoint");
    g_app->zeigeWegnetz = true;
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (bericht != nullptr) {
        char z[200];
        std::snprintf(z, sizeof(z), trN(Str::WegErzeugt, neuZahl, kanten), neuZahl, kanten, s);
        *bericht = z;
        if (genutzt > abstand + 0.5F) {
            std::snprintf(z, sizeof(z), tr(Str::WegRasterWeiter), static_cast<double>(genutzt), p.maxKnoten + navPunkte);
            *bericht += " - " + std::string(z);
        }
        if (!info.luecken.empty()) {
            const auto& a = info.luecken.front().anker;
            const int n = static_cast<int>(info.luecken.size());
            std::snprintf(z, sizeof(z), trN(Str::WegNichtAngebunden, n), n, static_cast<double>(a[0]), static_cast<double>(a[1]),
                          static_cast<double>(a[2]));
            *bericht += " - " + std::string(z);
        }
    }
    return neuZahl;
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

bool wegKantenAbgleichen(const std::vector<int>& punkte, bool trennen, const std::vector<int>& ohne) {
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
    const float umkreis = g_app->wegAbstand;
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
                            ImVec2{p0.x + (p1.x - p0.x) * t1, p0.y + (p1.y - p0.y) * t1}, f, 2.0F * px);
            }
        } else {
            dl->AddLine(schirm[a], schirm[b], f, 1.6F * px);
        }
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
                dl->AddCircleFilled(c, 3.5F * px, gewaehlt ? IM_COL32(255, 165, 40, 255) : IM_COL32(110, 220, 240, 255));
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
        if (g_app->wegWerkzeug != 0) { g_app->wegWerkzeug = 0; }
        return;
    }
    if (!imBild || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return; }
    g_app->wegKlickVerbraucht = true;
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
    if (iconTextKnopf(tr(Str::WegErzeugen), UiIcon::Erzeugen, tr(Str::WegErzeugenTipp), !g_app->geo.empty())) {
        wegErzeugen(g_app->wegAbstand, true, &g_app->wegMeldung);
        meldung(g_app->wegMeldung, "");
    }
    if (iconTextKnopf(tr(Str::WegEntfernen), UiIcon::Loeschen, tr(Str::WegEntfernenHint))) {
        const int n = wegGenerierteEntfernen();
        char z[120];
        std::snprintf(z, sizeof(z), trN(Str::WegEntfernt, n), n);
        g_app->wegMeldung = z;
    }
    if (!g_app->wegMeldung.empty()) { ImGui::TextDisabled("%s", g_app->wegMeldung.c_str()); }
    ImGui::SeparatorText(tr(Str::WegPruefenTitel));
    if (iconTextKnopf(tr(Str::WegPruefen), UiIcon::Haken, tr(Str::WegPruefenHint))) {
        g_app->wegBefunde = pruefeWegNetz(netz, g_app->map.entities, kartenStrahl());
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
