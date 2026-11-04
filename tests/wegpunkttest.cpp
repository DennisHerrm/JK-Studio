// wegpunkttest.cpp - Wegnetz lesen (wie die Engine), erzeugen, verteilen, pruefen
//
// Die Welt ist hier eine Handvoll Quader statt einer Karte: ein Boden, eine
// Wand mit schmaler Tuer, eine Treppe, ein Absatz zu hoch fuer eine Stufe, ein
// Turm, ein Loch. Der Strahl trifft Quader wie traceRay die Dreiecke der Karte
// - auch von innen (dort die Austrittsseite); die Spur mit Kasten wie gi.trace.
#include "bhed/wegpunkte.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

struct Quader {
    std::array<float, 3> mn;
    std::array<float, 3> mx;
};

std::vector<Quader> g_welt;

bool strahl(const std::array<float, 3>& a, const std::array<float, 3>& b, std::array<float, 3>& t,
            std::array<float, 3>& n) {
    float best = 2.0F;
    for (const Quader& q : g_welt) {
        float t0 = -1.0e9F;
        float t1 = 1.0e9F;
        int achse0 = -1;
        int achse1 = -1;
        float s0 = 0.0F;
        float s1 = 0.0F;
        bool leer = false;
        for (int k = 0; k < 3; ++k) {
            const float d = b[k] - a[k];
            if (std::fabs(d) < 1.0e-9F) {
                if (a[k] < q.mn[k] || a[k] > q.mx[k]) { leer = true; }
                continue;
            }
            float u0 = (q.mn[k] - a[k]) / d;
            float u1 = (q.mx[k] - a[k]) / d;
            float sg0 = -1.0F;
            float sg1 = 1.0F;
            if (u0 > u1) {
                std::swap(u0, u1);
                std::swap(sg0, sg1);
            }
            if (u0 > t0) { t0 = u0; achse0 = k; s0 = sg0; }
            if (u1 < t1) { t1 = u1; achse1 = k; s1 = sg1; }
        }
        if (leer || t0 > t1 || t1 < 0.0F) { continue; }
        const bool innen = t0 < 0.0F;
        const float tt = innen ? t1 : t0;
        if (tt > 1.0F || tt >= best) { continue; }
        best = tt;
        for (int k = 0; k < 3; ++k) { t[k] = a[k] + (b[k] - a[k]) * tt; }
        n = {0.0F, 0.0F, 0.0F};
        const int ax = innen ? achse1 : achse0;
        // Die Normale der Flaeche zeigt nach AUSSEN - auch wenn sie von innen
        // getroffen wird (wie bei den Dreiecken der Karte).
        if (ax >= 0) { n[static_cast<std::size_t>(ax)] = innen ? s1 : s0; }
    }
    return best <= 1.0F;
}

// rc609 W3a: gi.trace mit Kasten gegen die Quader: der Ursprung bewegt sich gegen die um den Kasten vergroesserten
// Quader; die Spur endet wie CM_BoxTrace kurz vor der Flaeche (DIST_EPSILON 0.03125).
WegSpurTreffer spur(const std::array<float, 3>& a, const std::array<float, 3>& mins, const std::array<float, 3>& maxs,
                    const std::array<float, 3>& b) {
    WegSpurTreffer r;
    float best = 1.0F;
    const std::array<float, 3> d{b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    for (const Quader& q : g_welt) {
        std::array<float, 3> mn{};
        std::array<float, 3> mx{};
        bool innen = true;
        for (std::size_t k = 0; k < 3; ++k) {
            mn[k] = q.mn[k] - maxs[k];
            mx[k] = q.mx[k] - mins[k];
            innen = innen && a[k] > mn[k] && a[k] < mx[k];
        }
        if (innen) {
            r.imFesten = true;
            r.anteil = 0.0F;
            r.ende = a;
            return r;
        }
        float t0 = -1.0e9F;
        float t1 = 1.0e9F;
        int achse = -1;
        float seite = 0.0F;
        bool leer = false;
        for (std::size_t k = 0; k < 3 && !leer; ++k) {
            if (std::fabs(d[k]) < 1.0e-9F) {
                leer = a[k] <= mn[k] || a[k] >= mx[k];
                continue;
            }
            float u0 = (mn[k] - a[k]) / d[k];
            float u1 = (mx[k] - a[k]) / d[k];
            float s0 = -1.0F;
            if (u0 > u1) {
                std::swap(u0, u1);
                s0 = 1.0F;
            }
            if (u0 > t0) {
                t0 = u0;
                achse = static_cast<int>(k);
                seite = s0;
            }
            t1 = std::min(t1, u1);
        }
        if (leer || t0 >= t1 || t0 < 0.0F || t0 >= best || achse < 0) { continue; }
        best = t0;
        r.normale = {0.0F, 0.0F, 0.0F};
        r.normale[static_cast<std::size_t>(achse)] = seite;
    }
    if (best < 1.0F) {
        const float dn = std::fabs(d[0] * r.normale[0] + d[1] * r.normale[1] + d[2] * r.normale[2]);
        best = std::max(0.0F, best - 0.03125F / std::max(1.0e-6F, dn));
    }
    r.anteil = best;
    for (std::size_t k = 0; k < 3; ++k) { r.ende[k] = a[k] + d[k] * best; }
    return r;
}

MapEntity ent(const std::string& klasse, const std::string& name, const std::string& ort,
              std::vector<std::pair<std::string, std::string>> mehr = {}) {
    MapEntity e;
    e.classname = klasse;
    e.targetname = name;
    e.origin = ort;
    e.keys.emplace_back("classname", klasse);
    if (!name.empty()) { e.keys.emplace_back("targetname", name); }
    e.keys.emplace_back("origin", ort);
    for (auto& kv : mehr) { e.keys.push_back(kv); }
    return e;
}

bool hatKante(const WegNetz& n, int a, int b, bool* designer = nullptr, bool* sprung = nullptr) {
    for (const WegKante& k : n.kanten) {
        if ((k.a == a && k.b == b) || (k.a == b && k.b == a)) {
            if (designer != nullptr) { *designer = k.designer; }
            if (sprung != nullptr) { *sprung = k.sprung; }
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    // --- 1. Lesen wie die Engine -------------------------------------------------
    {
        std::vector<MapEntity> e;
        e.push_back(ent("worldspawn", "", ""));
        e.push_back(ent("waypoint", "w1", "0 0 24", {{"target", "w2"}}));                 // 0
        e.push_back(ent("waypoint", "w2", "200 0 24", {{"target", "w3"}, {"targetJump", "w4"}}));   // 1
        e.push_back(ent("waypoint", "w3", "400 0 24", {{"target", "fehlt"}}));             // 2
        e.push_back(ent("waypoint", "w4", "600 0 24"));                                    // 3
        e.push_back(ent("waypoint_navgoal", "g1", "300 100 24"));                          // 4: automatisch
        e.push_back(ent("waypoint", "w5", "0 2000 24"));                                   // 5: weit weg
        e.push_back(ent("point_combat", "", "0 2100 24", {{"spawnflags", "4"}}));          // 6: NO_AUTO_CONNECT
        e.push_back(ent("waypoint", "w6", "300 100 200"));                                 // 7: eine Etage hoeher
        e.push_back(ent("NPC_spawner", "npc", "0 0 24"));
        const WegNetz n = baueWegNetz(e);
        pruefe(n.knoten.size() == 8U, "acht Knoten (waypoint, navgoal, point_combat; kein NPC, kein worldspawn)");
        bool des = false;
        bool spr = false;
        pruefe(hatKante(n, 0, 1, &des, &spr) && des && !spr, "target w1 -> w2: Designerkante");
        pruefe(hatKante(n, 1, 3, &des, &spr) && des && spr, "targetJump w2 -> w4: Sprungkante");
        pruefe(n.fehlendeZiele.size() == 1U && n.fehlendeZiele[0].second == "fehlt", "fehlendes target gemeldet");
        int autoG1 = 0;
        for (const WegKante& k : n.kanten) {
            if (!k.designer && (k.a == 4 || k.b == 4)) { ++autoG1; }
        }
        pruefe(autoG1 == 4, "navgoal verbindet sich automatisch mit den vier naechsten (" + std::to_string(autoG1) + ")");
        pruefe(!hatKante(n, 4, 7), "nicht zur Etage darueber (mehr als 60 hoeher)");
        pruefe(!hatKante(n, 4, 5), "nicht weiter als 500");
        bool sechs = false;
        for (const WegKante& k : n.kanten) { sechs = sechs || k.a == 6 || k.b == 6; }
        pruefe(!sechs, "NO_AUTO_CONNECT verbindet nichts");
        int autoWp = 0;
        for (const WegKante& k : n.kanten) {
            if (!k.designer && n.knoten[static_cast<std::size_t>(k.a)].art == WegKnoten::Art::Wegpunkt &&
                n.knoten[static_cast<std::size_t>(k.b)].art == WegKnoten::Art::Wegpunkt) {
                ++autoWp;
            }
        }
        pruefe(autoWp == 0, "normale Wegpunkte werden NICHT automatisch verbunden");
        // Mit Sicht: eine verdeckte Verbindung faellt weg.
        const WegNetz n2 = baueWegNetz(e, [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            return !(a[0] < 100.0F || b[0] < 100.0F);   // alles links von x=100 "verdeckt"
        });
        pruefe(!hatKante(n2, 4, 0), "Sichtpruefung: verdeckter Nachbar wird nicht genommen");
    }

    // --- 2. Erzeugen ---------------------------------------------------------------
    //
    // rc609 W3a: Boden 2000 x 2000 bei z=0, Wand bei x=300..340 mit einer Tuer von 64 bei y in [233, 297] (zwischen den
    // Rasterlinien bei 192 und 288), Treppe aus sechs Stufen von 16 (je 16 tief) auf ein Podest 96 hoch, ein Absatz
    // +40 (hoeher als STEPSIZE 18: zu Fuss nicht erreichbar), Turm +200, Loch bei x in [-900, -700].
    g_welt.clear();
    g_welt.push_back({{-1000, -1000, -16}, {-900, 1000, 0}});
    g_welt.push_back({{-700, -1000, -16}, {1000, 1000, 0}});
    g_welt.push_back({{300, -1000, 0}, {340, 233, 300}});
    g_welt.push_back({{300, 297, 0}, {340, 1000, 300}});
    for (int i = 0; i < 6; ++i) {   // Treppe nach +y: Stufe i beginnt bei y = -400 + 16 i
        g_welt.push_back({{-500, -400.0F + 16.0F * static_cast<float>(i), 0}, {-300, -100, 16.0F * static_cast<float>(i + 1)}});
    }
    g_welt.push_back({{-200, 600, 0}, {0, 800, 40}});         // Absatz +40
    g_welt.push_back({{500, -600, 0}, {700, -400, 200}});      // Turm
    WegWelt welt;
    welt.spur = spur;
    // Wie das Spiel: waypoint_getRadius (16 Richtungen, Kasten 16 ab STEPSIZE ueber dem Boden) mindestens
    // SC_MEDIUM_RADIUS 20, sonst bekommt der Punkt keine Kante.
    welt.knoten = [](const std::array<float, 3>& o) {
        for (int y = 0; y < 16; ++y) {
            const float w = static_cast<float>(y) * 3.14159265F / 8.0F;
            const WegSpurTreffer t = spur(o, {-16.0F, -16.0F, -6.0F}, {16.0F, 16.0F, 40.0F}, {o[0] + std::cos(w) * 4.0F, o[1] + std::sin(w) * 4.0F, o[2]});
            if (t.imFesten || t.anteil < 1.0F) { return false; }
        }
        return true;
    };
    WegErzeugung p;
    p.abstand = 96.0F;
    const auto wp = erzeugeWegpunkte({{0.0F, 0.0F, 50.0F}}, welt, p);
    pruefe(wp.size() > 150U, "erzeugt: " + std::to_string(wp.size()) + " Wegpunkte");
    bool aufBoden = true;
    bool turmFrei = true;
    bool lochFrei = true;
    bool absatzFrei = true;
    int treppeUnten = -1;
    int treppeOben = -1;
    for (std::size_t i = 0; i < wp.size(); ++i) {
        const auto& w = wp[i];
        const float bz = w.boden[2];
        // an der Treppe: ihr Kasten, um Kastenbreite und Gitter (16 + 32) erweitert
        const bool aufTreppe = w.boden[0] > -548 && w.boden[0] < -252 && w.boden[1] > -448 && w.boden[1] < -52;
        aufBoden = aufBoden && (std::fabs(bz) < 0.5F || (aufTreppe && std::fabs(bz / 16.0F - std::round(bz / 16.0F)) < 0.05F));
        aufBoden = aufBoden && std::fabs(w.ort[2] - w.boden[2] - 25.0F) < 0.01F;
        turmFrei = turmFrei && !(w.boden[0] > 484 && w.boden[0] < 716 && w.boden[1] > -616 && w.boden[1] < -384 && bz > 1.0F);
        lochFrei = lochFrei && !(w.boden[0] > -900 && w.boden[0] < -700);
        absatzFrei = absatzFrei && std::fabs(bz - 40.0F) > 0.5F;
        if (aufTreppe && std::fabs(bz) < 0.5F && treppeUnten < 0) { treppeUnten = static_cast<int>(i); }
        if (aufTreppe && std::fabs(bz - 96.0F) < 0.5F && treppeOben < 0) { treppeOben = static_cast<int>(i); }
    }
    pruefe(aufBoden, "jeder Wegpunkt steht auf einem Boden oder einer Stufe, origin = Boden + 25");
    pruefe(absatzFrei, "nicht auf dem Absatz +40 (hoeher als STEPSIZE)");
    pruefe(turmFrei, "nicht auf dem Turm (zu hoch)");
    pruefe(lochFrei, "nicht im Loch");
    pruefe(treppeUnten >= 0 && treppeOben >= 0, "ein Punkt unten und einer oben an der Treppe");
    bool durchWand = false;
    bool symmetrisch = true;
    bool jenseits = false;
    for (std::size_t i = 0; i < wp.size(); ++i) {
        jenseits = jenseits || wp[i].boden[0] > 340.0F;
        for (const int j : wp[i].nachbarn) {
            const auto& a = wp[i].boden;
            const auto& b = wp[static_cast<std::size_t>(j)].boden;
            if ((a[0] < 300.0F) != (b[0] < 300.0F)) {
                const float t = (300.0F - a[0]) / (b[0] - a[0]);
                const float y = a[1] + (b[1] - a[1]) * t;
                if (y < 233.0F || y > 297.0F) { durchWand = true; }
            }
            const auto& nb = wp[static_cast<std::size_t>(j)].nachbarn;
            symmetrisch = symmetrisch && std::find(nb.begin(), nb.end(), static_cast<int>(i)) != nb.end();
        }
    }
    pruefe(!durchWand, "keine Verbindung durch die Wand");
    pruefe(jenseits, "durch die schmale Tuer neben dem Raster auf die andere Seite der Wand");
    pruefe(symmetrisch, "Nachbarschaft in beide Richtungen");
    // zusammenhaengend, und die Treppe: unten und oben ueber hoechstens zwei Kanten verbunden
    {
        std::vector<int> tiefe(wp.size(), -1);
        std::vector<int> schlange{0};
        tiefe[0] = 0;
        for (std::size_t q = 0; q < schlange.size(); ++q) {
            const int i = schlange[q];
            for (const int j : wp[static_cast<std::size_t>(i)].nachbarn) {
                if (tiefe[static_cast<std::size_t>(j)] < 0) {
                    tiefe[static_cast<std::size_t>(j)] = tiefe[static_cast<std::size_t>(i)] + 1;
                    schlange.push_back(j);
                }
            }
        }
        pruefe(schlange.size() == wp.size(), "ein zusammenhaengendes Netz");
        bool nah = false;
        if (treppeUnten >= 0 && treppeOben >= 0) {
            for (const int j : wp[static_cast<std::size_t>(treppeUnten)].nachbarn) {
                nah = nah || j == treppeOben;
                for (const int k : wp[static_cast<std::size_t>(j)].nachbarn) { nah = nah || k == treppeOben; }
            }
        }
        pruefe(nah, "Treppe: unten und oben ueber hoechstens zwei Kanten verbunden");
    }
    p.maxKnoten = 50;
    const std::size_t begrenzt = erzeugeWegpunkte({{0.0F, 0.0F, 50.0F}}, welt, p).size();
    pruefe(begrenzt > 0U && begrenzt <= 50U, "Obergrenze maxKnoten haelt (" + std::to_string(begrenzt) + ")");

    // --- 3. Verteilen auf target..target4 -----------------------------------------
    p.maxKnoten = 1023;
    const auto wp2 = erzeugeWegpunkte({{0.0F, 0.0F, 50.0F}}, welt, p);
    const auto ziele = verteileZiele(wp2, 4);
    std::size_t kanten = 0;
    std::size_t vergeben = 0;
    bool hoechstens4 = true;
    for (std::size_t i = 0; i < wp2.size(); ++i) {
        kanten += wp2[i].nachbarn.size();
        vergeben += ziele[i].size();
        hoechstens4 = hoechstens4 && ziele[i].size() <= 4U;
    }
    kanten /= 2;
    pruefe(hoechstens4, "hoechstens vier targets je Wegpunkt");
    pruefe(vergeben * 10 >= kanten * 9, "mindestens 90 % der Kanten bekommen ein target (" + std::to_string(vergeben) +
                                            " von " + std::to_string(kanten) + ")");
    // Das Netz aus den targets wieder lesen: zusammenhaengend?
    {
        std::vector<MapEntity> e;
        for (std::size_t i = 0; i < wp2.size(); ++i) {
            char o[64];
            std::snprintf(o, sizeof(o), "%d %d %d", static_cast<int>(wp2[i].ort[0]), static_cast<int>(wp2[i].ort[1]),
                          static_cast<int>(wp2[i].ort[2]));
            std::vector<std::pair<std::string, std::string>> mehr;
            static const char* const k[4] = {"target", "target2", "target3", "target4"};
            for (std::size_t z = 0; z < ziele[i].size(); ++z) {
                mehr.emplace_back(k[z], "wp" + std::to_string(ziele[i][z]));
            }
            e.push_back(ent("waypoint", "wp" + std::to_string(i), o, mehr));
        }
        const WegNetz n = baueWegNetz(e);
        std::vector<std::vector<int>> adj(n.knoten.size());
        for (const WegKante& kk : n.kanten) {
            adj[static_cast<std::size_t>(kk.a)].push_back(kk.b);
            adj[static_cast<std::size_t>(kk.b)].push_back(kk.a);
        }
        std::vector<int> st{0};
        std::set<int> ges{0};
        while (!st.empty()) {
            const int i = st.back();
            st.pop_back();
            for (const int j : adj[static_cast<std::size_t>(i)]) {
                if (ges.insert(j).second) { st.push_back(j); }
            }
        }
        pruefe(ges.size() == n.knoten.size() && n.fehlendeZiele.empty(),
               "wie die Engine gelesen: alle Wegpunkte erreichbar, kein target fehlt");
        pruefe(pruefeWegNetz(n, e, strahl).empty(), "das erzeugte Netz besteht die Pruefung");
    }

    // --- 4. Pruefen ----------------------------------------------------------------
    {
        std::vector<MapEntity> e;
        e.push_back(ent("waypoint", "imturm", "600 -500 100"));               // im Turm
        e.push_back(ent("waypoint", "schwebt", "0 0 400", {{"target", "imturm"}}));
        e.push_back(ent("waypoint", "allein", "100 100 25"));
        e.push_back(ent("waypoint", "solidok", "600 -450 100", {{"spawnflags", "1"}, {"target", "allein"}}));
        const WegNetz n = baueWegNetz(e);
        const auto bef = pruefeWegNetz(n, e, strahl);
        const auto hat = [&](WegBefund::Art a, int k) {
            for (const auto& b : bef) {
                if (b.art == a && b.knoten == k) { return true; }
            }
            return false;
        };
        pruefe(hat(WegBefund::Art::ImFesten, 0), "im Festen erkannt");
        pruefe(!hat(WegBefund::Art::ImFesten, 3), "SOLID_OK wird nicht als im Festen gemeldet");
        pruefe(hat(WegBefund::Art::Schwebend, 1), "schwebend erkannt");
        pruefe(!hat(WegBefund::Art::Allein, 2), "durch ein target verbunden = nicht allein");
        std::vector<MapEntity> e2{ent("waypoint", "x", "0 0 25")};
        const auto b2 = pruefeWegNetz(baueWegNetz(e2), e2, strahl);
        pruefe(b2.size() == 1U && b2[0].art == WegBefund::Art::Allein, "ein einzelner Wegpunkt ist allein");
    }

    std::printf("\n%s: %d Fehler\n", g_fehler == 0 ? "wegpunkte OK" : "wegpunkte FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
