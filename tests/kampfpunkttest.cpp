// kampfpunkttest.cpp - rc609 W4: Kampfpunkte (point_combat) wie im Spiel
//
// navKampfPunkte (bhed/navlauf.h) gegen Faelle, deren Antwort aus der Quelle feststeht, auf kleiner Testgeometrie aus
// echten Brushes (kollisionsSpur, wie im Programm; Welt wie tests/wegkantentest.cpp):
//   JKA/Movie Duels (SP_point_combat, NAV::SpawnedPoint, CP_FindCombatPointWaypoints -> NAV::GetNearestNode):
//     frei         der Punkt ist selbst ein Knoten und wird sich selbst zugeordnet, mit Auto-Kanten zu den Wegpunkten
//     an der Wand  der Kasten von SpawnedPoint (+-20) steckt in der Wand: kein Knoten; zugeordnet wird der naechste
//                  Knoten, zu dem die Sicht (ViewNavTrace) frei ist - nicht der naehere hinter der Wand
//     ohne Kanten  ein Netz ohne eine einzige Kante: "has no waypoint" (size_edges() == 0)
//     zu viele     ueber MAX_COMBAT_POINTS 512 nicht im Spiel
//   Jedi Outcast (NAV_FindClosestWaypointForPoint): der naechste Knoten mit freiem Weg des Markers; ein gescheiterter
//     Knoten bleibt fuer die folgenden Punkte gescheitert (nodeChecked des wiederverwendeten Markers).
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

MapEntity ent(const std::string& klasse, float x, float y, float z, const std::string& name = "", const std::string& ziel = "") {
    MapEntity e;
    e.classname = klasse;
    e.keys.push_back({"classname", klasse});
    char b[96];
    std::snprintf(b, sizeof(b), "%g %g %g", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
    e.origin = b;
    e.keys.push_back({"origin", b});
    if (!name.empty()) {
        e.targetname = name;
        e.keys.push_back({"targetname", name});
    }
    if (!ziel.empty()) { e.keys.push_back({"target", ziel}); }
    return e;
}

std::vector<NavKampfPunkt> rechne(const Welt& w, const std::vector<MapEntity>& ents, std::shared_ptr<NavNetz>& netz) {
    const NavBauHilfe h = w.hilfe();
    NavWelt welt;
    netz = baueNavNetz(ents, h);
    welt.netz = netz;
    welt.spur = h.spur;
    return navKampfPunkte(ents, welt);
}

int entityVonKnoten(const NavNetz& n, int k) {
    return k > 0 && static_cast<std::size_t>(k) < n.knoten.size() ? n.knoten[static_cast<std::size_t>(k)].entity : -1;
}

void jka() {
    navJk2Regeln(false);
    Welt w;
    w.kasten({-2000, -2000, -16}, {2000, 2000, 0});
    w.kasten({400, -100, 0}, {416, 100, 200});   // Wand
    std::vector<MapEntity> ents;
    ents.push_back(ent("waypoint", 0, 0, 25, "a", "b"));      // 0
    ents.push_back(ent("waypoint", 200, 0, 25, "b"));         // 1
    ents.push_back(ent("waypoint", 520, 0, 25, "e", "f"));    // 2 hinter der Wand, naeher an Punkt 5 als b
    ents.push_back(ent("waypoint", 700, 0, 25, "f"));         // 3
    ents.push_back(ent("point_combat", 100, 150, 25));        // 4 frei
    ents.push_back(ent("point_combat", 390, 0, 25));          // 5 an der Wand
    std::shared_ptr<NavNetz> netz;
    const std::vector<NavKampfPunkt> k = rechne(w, ents, netz);
    pruefe(k.size() == 2U && netz != nullptr, "JKA: zwei Kampfpunkte");
    if (k.size() != 2U || !netz) { return; }
    const NavKampfPunkt& frei = k[0];
    const std::size_t kanten = frei.knoten > 0 ? netz->links[static_cast<std::size_t>(frei.knoten)].size() : 0U;
    std::printf("     frei: Knoten %d, Zuordnung %d (Entity %d), Kanten %zu, origin z %.3f\n", frei.knoten, frei.zuordnung, frei.zuEntity,
                kanten, static_cast<double>(frei.ort[2]));
    pruefe(frei.ort[2] == 25.125F, "JKA frei: origin + 0.125 (SP_point_combat)");
    pruefe(frei.knoten > 0 && entityVonKnoten(*netz, frei.knoten) == 4, "JKA frei: eigener NAV-Knoten (SpawnedPoint PT_COMBATNODE)");
    pruefe(!frei.ohneWegpunkt && frei.zuordnung == frei.knoten && frei.zuEntity == 4, "JKA frei: GetNearestNode gibt den Punkt selbst");
    pruefe(frei.knoten > 0 && !netz->knoten[static_cast<std::size_t>(frei.knoten)].insel && kanten >= 2U,
           "JKA frei: Auto-Kanten zu den Wegpunkten (MIN_WAY_NEIGHBORS), keine Insel");
    const NavKampfPunkt& wand = k[1];
    std::printf("     Wand: Knoten %d, Zuordnung %d (Entity %d)\n", wand.knoten, wand.zuordnung, wand.zuEntity);
    pruefe(wand.knoten == 0, "JKA Wand: kein Knoten (SpawnedPoint in solid)");
    pruefe(!wand.ohneWegpunkt && wand.zuEntity == 1, "JKA Wand: zugeordnet b (Sicht frei), nicht e hinter der Wand (naeher)");

    // ohne Kanten: ein Kampfpunkt und vier Wegpunkte ohne target, weit auseinander (keine Auto-Kanten)
    std::vector<MapEntity> leer;
    leer.push_back(ent("waypoint", -1500, -1500, 25, "p"));
    leer.push_back(ent("waypoint", 1500, -1500, 25, "q"));
    leer.push_back(ent("waypoint", -1500, 1500, 25, "r"));
    leer.push_back(ent("waypoint", 1500, 1500, 25, "s"));
    leer.push_back(ent("point_combat", 0, 0, 25));
    std::shared_ptr<NavNetz> n2;
    const std::vector<NavKampfPunkt> k2 = rechne(w, leer, n2);
    std::printf("     ohne Kanten: %d Kanten im Netz\n", n2 ? n2->zahlKanten() : -1);
    pruefe(k2.size() == 1U && k2[0].ohneWegpunkt && k2[0].zuordnung == 0, "JKA ohne Kanten: has no waypoint (size_edges() == 0)");

    // zu viele: 513 Kampfpunkte, ohne Netz gezaehlt
    std::vector<MapEntity> viele;
    for (int i = 0; i < 513; ++i) { viele.push_back(ent("point_combat", static_cast<float>(i) * 50.0F, 0, 25)); }
    const std::vector<NavKampfPunkt> k3 = navKampfPunkte(viele, NavWelt{});
    pruefe(k3.size() == 513U && !k3[511].zuViele && k3[512].zuViele, "JKA: der 513. ist zu viel (MAX_COMBAT_POINTS)");
}

void jk2() {
    navJk2Regeln(true);
    Welt w;
    w.kasten({-2000, -2000, -16}, {2000, 2000, 0});
    w.kasten({90, -40, 0}, {106, 40, 200});   // Wand zwischen n1 und n2
    std::vector<MapEntity> ents;
    ents.push_back(ent("waypoint", 0, 0, 25, "n1", "n2"));
    ents.push_back(ent("waypoint", 280, 0, 25, "n2"));
    ents.push_back(ent("waypoint", 1500, 1500, 25, "fern1", "fern2"));
    ents.push_back(ent("waypoint", 1800, 1500, 25, "fern2"));
    ents.push_back(ent("point_combat", 130, 0, 25));    // naechster n1 (hinter der Wand), dann n2 (frei)
    ents.push_back(ent("point_combat", -100, 0, 25));   // naechster n1 (frei) - aber schon gescheitert
    std::shared_ptr<NavNetz> netz;
    const std::vector<NavKampfPunkt> k = rechne(w, ents, netz);
    pruefe(netz && netz->jk2 && k.size() == 2U, "JK2: Netz und zwei Kampfpunkte");
    if (!netz || !netz->jk2 || k.size() != 2U) {
        navJk2Regeln(false);
        return;
    }
    std::printf("     Radius n1 %d; Punkt 1 -> %d, Punkt 2 -> %d\n", netz->jk2->knoten[0].radius, k[0].zuordnung, k[1].zuordnung);
    pruefe(k[0].knoten == 0, "JK2: kein eigener Knoten");
    pruefe(k[0].zuordnung == 1 && !k[0].ohneWegpunkt, "JK2 Punkt 1: n1 gescheitert (Wand), n2 frei");
    pruefe(k[1].zuordnung == -1 && k[1].ohneWegpunkt, "JK2 Punkt 2: n1 bleibt gescheitert (nodeChecked), n2 hinter der Wand - has no waypoint");
    navJk2Regeln(false);
}

}  // namespace

int main() {
    std::printf("== kampfpunkt: JKA/Movie Duels ==\n");
    jka();
    std::printf("== kampfpunkt: Jedi Outcast ==\n");
    jk2();
    std::printf("%s (%d Fehler)\n", gFehler == 0 ? "ALLES OK" : "FEHLER", gFehler);
    return gFehler == 0 ? 0 : 1;
}
