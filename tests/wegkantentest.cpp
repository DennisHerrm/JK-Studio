// wegkantentest.cpp - rc609 W1: die Kantenpruefung des Spiels fuer den Editor
//
// NavKantenPruefer (bhed/navlauf.h) muss dasselbe sagen wie der Netzbau der
// Simulation (baueNavNetz: PHASE III mit NAV::TestEdge, unter den Regeln von
// Jedi Outcast HardConnect). Beide laufen hier auf derselben kleinen
// Testgeometrie aus echten Brushes (kollisionsSpur, wie im Programm):
//
//   Boden      flach, 150 Einheiten
//   Treppe     16er-Stufen, 32 tief, 8 Stufen (128 hoch)
//   Kante      eine 60 hohe Stufe (MAX_EDGE_FLOOR_DIST)
//   Wand       16 dick, 256 hoch, zwischen den Punkten
//   Rampe      Neigung mit Normale z 0,7 (MIN_WALK_NORMAL)
//   Gang       60 breit (der grosse Kasten passt nicht, der mittlere schon)
//
// Je Stelle die Kante in beide Richtungen, je Spiel (JKA/Movie Duels und
// Jedi Outcast). Dazu Erwartungen aus dem Port, damit ein Fehler in beiden
// zugleich auffaellt.
#include "bhed/bsp.h"
#include "bhed/kollision.h"
#include "bhed/navlauf.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int gFehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++gFehler; }
}

// Eine Welt aus Brushes in Modell 1 (alle in einem "Blatt" wie CMod_LoadSubmodels).
struct Welt {
    KollisionsKarte k;
    Welt() {
        k.knoten.resize(1);
        k.modelle.resize(2);
    }
    int ebene(float nx, float ny, float nz, float d) {
        KollisionsKarte::Ebene e;
        e.normale[0] = nx;
        e.normale[1] = ny;
        e.normale[2] = nz;
        e.abstand = d;
        // PLANE_X/Y/Z nur fuer die positiven Achsen, wie kollisionsSpurKasten
        int achse = -1;
        if (nx == 1.0F && ny == 0.0F && nz == 0.0F) { achse = 0; }
        if (nx == 0.0F && ny == 1.0F && nz == 0.0F) { achse = 1; }
        if (nx == 0.0F && ny == 0.0F && nz == 1.0F) { achse = 2; }
        e.typ = achse >= 0 ? achse : 3;
        e.vorzeichen = (nx < 0.0F ? 1 : 0) | (ny < 0.0F ? 2 : 0) | (nz < 0.0F ? 4 : 0);
        k.ebenen.push_back(e);
        return static_cast<int>(k.ebenen.size()) - 1;
    }
    void pinsel(const std::vector<int>& seiten, const NavVec& mi, const NavVec& ma) {
        KollisionsKarte::Pinsel p;
        p.ersteSeite = static_cast<int>(k.seiten.size());
        p.seiten = static_cast<int>(seiten.size());
        p.inhalt = kNavSolid;
        for (int a = 0; a < 3; ++a) {
            p.mins[a] = mi[static_cast<std::size_t>(a)];
            p.maxs[a] = ma[static_cast<std::size_t>(a)];
        }
        for (const int s : seiten) { k.seiten.push_back(KollisionsKarte::Seite{s, -1}); }
        k.pinsel.push_back(p);
        k.modelle[1].erster = 0;
        k.modelle[1].zahl = static_cast<int>(k.pinsel.size());
    }
    void kasten(const NavVec& mi, const NavVec& ma) {
        pinsel({ebene(1, 0, 0, ma[0]), ebene(-1, 0, 0, -mi[0]), ebene(0, 1, 0, ma[1]), ebene(0, -1, 0, -mi[1]),
                ebene(0, 0, 1, ma[2]), ebene(0, 0, -1, -mi[2])},
               mi, ma);
    }
    // Keil: steigt in +x von x0 (Hoehe z0) bis x1, Normale z = nz.
    void rampe(float x0, float x1, float y0, float y1, float z0, float nz) {
        const float nx = -std::sqrt(1.0F - nz * nz);
        const float hoch = (x1 - x0) * (-nx) / nz;
        pinsel({ebene(1, 0, 0, x1), ebene(-1, 0, 0, -x0), ebene(0, 1, 0, y1), ebene(0, -1, 0, -y0),
                ebene(0, 0, 1, z0 + hoch), ebene(0, 0, -1, -z0), ebene(nx, 0, nz, nx * x0 + nz * z0)},
               {x0, y0, z0}, {x1, y1, z0 + hoch});
    }
    [[nodiscard]] NavBauHilfe hilfe() const {
        NavBauHilfe h;
        const KollisionsKarte* kp = &k;
        h.spur = [kp](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t maske) {
            const KollisionsSpur s = kollisionsSpur(*kp, a.data(), mi.data(), ma.data(), b.data(), maske, 1);
            NavSpur r;
            r.allsolid = s.allsolid;
            r.startsolid = s.startsolid;
            r.fraction = s.fraction;
            r.endpos = s.endpos;
            r.normal = s.normale;
            r.contents = s.inhalt;
            r.entity = s.fraction < 1.0F || s.startsolid ? kNavWeltEntity : kNavKeineEntity;
            return r;
        };
        return h;
    }
};

MapEntity ent(const std::string& klasse, const std::string& ort, const std::string& name, const std::string& ziel) {
    MapEntity e;
    e.classname = klasse;
    e.keys.push_back({"classname", klasse});
    if (!ort.empty()) {
        e.origin = ort;
        e.keys.push_back({"origin", ort});
    }
    if (!name.empty()) {
        e.targetname = name;
        e.keys.push_back({"targetname", name});
    }
    if (!ziel.empty()) { e.keys.push_back({"target", ziel}); }
    return e;
}
std::string ort(float x, float y, float z) {
    char b[96];
    std::snprintf(b, sizeof(b), "%g %g %g", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
    return b;
}

struct Ergebnis {
    bool simulation = false;   // Kante bleibt im Netz der Simulation
    bool simGross = false;
    NavKantenErgebnis editor;
    bool knotenA = false;      // das Spiel legt die Knoten an (nicht im Festen)
    bool knotenB = false;
};

// Zwei Wegpunkte a -> b (a traegt das target), dazu zwei ferne, damit das Spiel
// ein Netz baut ("size_nodes() <= 3": keines).
Ergebnis vergleiche(const Welt& w, const NavVec& a, const NavVec& b) {
    std::vector<MapEntity> ents;
    ents.push_back(ent("worldspawn", "", "", ""));
    ents.push_back(ent("waypoint", ort(a[0], a[1], a[2]), "a", "b"));
    ents.push_back(ent("waypoint", ort(b[0], b[1], b[2]), "b", ""));
    ents.push_back(ent("waypoint", ort(9000, 9000, 25), "fern1", ""));
    ents.push_back(ent("waypoint", ort(9400, 9000, 25), "fern2", ""));
    const NavBauHilfe h = w.hilfe();
    Ergebnis r;
    const std::shared_ptr<NavNetz> netz = baueNavNetz(ents, h);
    if (navJk2Regeln()) {
        // HardConnect: Kante immer da, EFLAG_BLOCKED (flags & 1) nach der Spur.
        if (netz && netz->jk2 && netz->jk2->knoten.size() >= 2U) {
            for (const NavNetzJk2::Kante& k : netz->jk2->knoten[0].kanten) {
                if (k.id == 1) {
                    r.simulation = (k.flags & 1U) == 0U;
                    r.simGross = r.simulation;
                }
            }
        }
    } else if (netz) {
        const int ka = netz->knotenNamens("a");
        const int kb = netz->knotenNamens("b");
        const int e = (ka > 0 && kb > 0) ? netz->kanteZwischen(ka, kb) : 0;
        r.simulation = e > 0;
        r.simGross = e > 0 && netz->kanten[static_cast<std::size_t>(e)].groesse() == 2;
    }
    NavKantenPruefer p(ents, h);
    r.editor = p.pruefe(1, 2);
    r.knotenA = p.hatKnoten(1);
    r.knotenB = p.hatKnoten(2);
    return r;
}

void stelle(const char* name, const Welt& w, const NavVec& a, const NavVec& b, int erwartet, bool jk2) {
    // erwartet: 0 nicht begehbar, 1 mittel, 2 gross (JKA); JK2: 0/1 = gesperrt/frei
    const Ergebnis hin = vergleiche(w, a, b);
    const Ergebnis rueck = vergleiche(w, b, a);
    const auto text = [](const Ergebnis& e) {
        return std::string(e.simulation ? (e.simGross ? "gross" : "mittel") : "weg") + " / " +
               (e.editor.begehbar ? (e.editor.gross ? "gross" : "mittel") : "weg") + (e.knotenA ? "" : ", a im Festen") +
               (e.knotenB ? "" : ", b im Festen");
    };
    const std::string spiel = jk2 ? "JK2" : "JKA";
    pruefe(hin.simulation == hin.editor.begehbar && hin.simGross == hin.editor.gross && rueck.simulation == rueck.editor.begehbar &&
               rueck.simGross == rueck.editor.gross,
           spiel + " " + name + ": Editor wie Simulation (hin " + text(hin) + ", rueck " + text(rueck) + ")");
    const int ist = !hin.editor.begehbar ? 0 : jk2 ? 1 : (hin.editor.gross ? 2 : 1);
    pruefe(ist == erwartet, spiel + " " + name + ": erwartet " + std::to_string(erwartet) + ", ist " + std::to_string(ist));
}

void alleStellen(bool jk2) {
    navJk2Regeln(jk2);
    // Boden
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        stelle("Boden 150", w, {-75, 0, 25}, {75, 0, 25}, jk2 ? 1 : 2, jk2);
    }
    // Treppe: 8 Stufen je 16 hoch, 32 tief, ab x = 0; oben ein Absatz.
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        for (int i = 0; i < 8; ++i) {
            w.kasten({static_cast<float>(32 * i), -200, 0}, {1000, 200, static_cast<float>(16 * (i + 1))});
        }
        // kurz: vom Fuss auf die erste Stufe und von Stufe zu Stufe
        stelle("Treppe Fuss -> 2. Stufe", w, {-48, 0, 25}, {48, 0, 32 + 25}, jk2 ? 1 : 2, jk2);
        // ueber die ganze Treppe: die Gerade schneidet die Stufenkanten
        stelle("Treppe ganz", w, {-120, 0, 25}, {380, 0, 128 + 25}, 0, jk2);
    }
    // 60er-Kante
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        w.kasten({0, -1000, 0}, {1000, 1000, 60});
        stelle("Kante 60", w, {-100, 0, 25}, {100, 0, 85}, 0, jk2);
    }
    // Wand
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        w.kasten({-8, -1000, 0}, {8, 1000, 256});
        stelle("Wand", w, {-100, 0, 25}, {100, 0, 25}, 0, jk2);
    }
    // Rampe 0,7 (gut 45 Grad): auf der Schraege hinauf
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        w.rampe(0, 400, -300, 300, 0, 0.7F);
        // Steigung je Einheit; die Punkte 42 ueber der Schraege: der Kasten
        // (-16..16) beruehrt sie dann beim G_CheckInSolid nicht (16 * t < 42 - 24).
        const float t = std::sqrt(1.0F - 0.49F) / 0.7F;
        stelle("Rampe 0,7", w, {100, 0, 100 * t + 42}, {300, 0, 300 * t + 42}, 1, jk2);
    }
    // Gang 60 breit
    {
        Welt w;
        w.kasten({-1000, -1000, -16}, {1000, 1000, 0});
        w.kasten({-1000, 30, 0}, {1000, 1000, 256});
        w.kasten({-1000, -1000, 0}, {1000, -30, 256});
        stelle("Gang 60", w, {-100, 0, 25}, {100, 0, 25}, 1, jk2);
    }
    navJk2Regeln(false);
}

}  // namespace

int main() {
    std::printf("== wegkanten: JKA/Movie Duels (NAV::TestEdge) ==\n");
    alleStellen(false);
    std::printf("== wegkanten: Jedi Outcast (HardConnect) ==\n");
    alleStellen(true);
    std::printf("%s (%d Fehler)\n", gFehler == 0 ? "ALLES OK" : "FEHLER", gFehler);
    return gFehler == 0 ? 0 : 1;
}
