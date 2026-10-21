// jk2navtest.cpp - rc604 fix604 (JK2 Stufe 3): Laufen wie in Jedi Outcast.
//
// Mit dem Regelwerk "Jedi Outcast" laeuft ein NPC wie in codeJK2: NPC_MoveToGoal
// mit dem alten Navigator (NPC_move.cpp, g_navnew.cpp, g_navigator.cpp), das
// Tempo aus ClientThink_real (g_active.cpp:2358-2484, NPC_Accelerate), das Ziel
// mit NAV_HitNavGoal (g_nav.cpp:156-205) und das Wegnetz aus maps/<karte>.nav.
//
// Teil 1 rechnet ohne Karte. Teil 2 liest kejim_post aus der echten
// JK2-Installation (nur lesend; fehlt sie, wird Teil 2 uebersprungen).
//
//   jk2navtest            die Pruefungen (gruen)
//   jk2navtest --alt      dieselben Pruefungen mit den OpenJK-Wegen unter dem
//                         JK2-Regelwerk - so rechnete behaved bis fix604 (rot)
//   jk2navtest <base>     ein anderer JK2-Ordner fuer Teil 2
#include "bhed/bsp.h"
#include "bhed/kollision.h"
#include "bhed/navlauf.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int gFehler = 0;
int gOk = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (ok) {
        ++gOk;
    } else {
        ++gFehler;
    }
}

float laengeQ(const NavVec& a, const NavVec& b) {
    return (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]);
}

MapEntity ent(const std::string& klasse, const std::string& ort, const std::string& name = {}) {
    MapEntity e;
    e.classname = klasse;
    e.origin = ort;
    e.targetname = name;
    e.keys.push_back({"classname", klasse});
    e.keys.push_back({"origin", ort});
    if (!name.empty()) { e.keys.push_back({"targetname", name}); }
    return e;
}

LaufWerte werteFuer(const NavVec& ziel, float lauf, float geh, int gang) {
    LaufWerte w;
    w.zielGesetzt = true;
    w.ziel = ziel;
    w.zielRadius = 12.0F;
    w.maxTempo = gang == 1 ? geh : lauf;
    w.gehTempo = geh;
    w.gangSchalter = gang;
    w.verhaltenTaktMs = 100;
    w.fallen = false;
    return w;
}

// --- Teil 1: ohne Karte ------------------------------------------------------
void teil1() {
    std::vector<MapEntity> ents;
    ents.push_back(ent("worldspawn", ""));
    const std::shared_ptr<const NavWelt> welt = navWeltAusEntities(ents);
    LaufZustand z;
    z.ort = {0.0F, 0.0F, 0.0F};
    z.denkNullMs = 0.0;
    // 1a: gehen (SCF_WALKING, walkSpeed 50) - NPC_Accelerate: unter walkSpeed
    // je ClientThink (alle 50 ms) +15: 15, 30, 45, 50.
    {
        const LaufPlan p = planeLauf(*welt, z, werteFuer({400.0F, 0.0F, 0.0F}, 150.0F, 50.0F, 1), 0.0, 20000.0);
        std::vector<int> tempi;
        for (const LaufTakt& t : p.takte) {
            if (t.tempo > 0 || !tempi.empty()) { tempi.push_back(t.tempo); }
            if (tempi.size() >= 5U) { break; }
        }
        std::string s;
        for (const int v : tempi) { s += std::to_string(v) + " "; }
        pruefe(tempi.size() >= 5U && tempi[0] == 15 && tempi[1] == 30 && tempi[2] == 45 && tempi[3] == 50 && tempi[4] == 50,
               "1a gehen beschleunigt je Bild um 15 (NPC_Accelerate): " + s);
    }
    // 1b: laufen (runSpeed 150): sofort volles Tempo, vor dem Ziel langsamer
    // (SLOWDOWN_DIST 128: desiredSpeed = ceil(150 * distToGoal / 128)), unter
    // walkSpeed die Gehanimation (BUTTON_WALKING aus currentSpeed).
    {
        const LaufPlan p = planeLauf(*welt, z, werteFuer({400.0F, 0.0F, 0.0F}, 150.0F, 50.0F, 0), 0.0, 20000.0);
        if (std::getenv("JK2DBG") != nullptr) {
            for (const LaufTakt& t : p.takte) {
                std::printf("t=%.0f x=%.2f tempo=%d bewegt=%d geh=%d\n", t.t, t.ort[0], t.tempo, t.bewegt ? 1 : 0, t.gehAnim ? 1 : 0);
            }
            std::printf("ankunft %.0f nie %d ruhe %.0f\n", p.ankunftMs, p.nie ? 1 : 0, p.ruheMs);
        }
        int erstes = -1;
        bool langsam = false;
        bool gehAnimLangsam = false;
        for (const LaufTakt& t : p.takte) {
            if (erstes < 0 && t.tempo > 0) { erstes = t.tempo; }
            if (t.tempo > 16 && t.tempo < 150 && t.t < p.ankunftMs) {
                langsam = true;
                if (t.tempo <= 50 && t.gehAnim) { gehAnimLangsam = true; }
            }
        }
        pruefe(erstes == 150, "1b laufen beginnt sofort mit runSpeed (\"accelerate instantly\"): " + std::to_string(erstes));
        pruefe(langsam, "1b vor dem Ziel langsamer (SLOWDOWN_DIST)");
        pruefe(gehAnimLangsam, "1b langsam unter walkSpeed: Gehanimation (BUTTON_WALKING)");
        // NAV_HitNavGoal ohne NAVGOAL_USE_RADIUS: der Kasten der Figur (16)
        // beruehrt den Wuerfel +-12 um das Ziel - ab x >= 372.
        const NavVec bei = ortBei(p, p.ankunftMs);
        pruefe(p.ankunftMs > 0.0 && bei[0] >= 372.0F && bei[0] < 383.0F,
               "1b angekommen, sobald sich die Kaesten beruehren (x " + std::to_string(bei[0]) + ")");
    }
    // 1c: NAVGOAL_USE_RADIUS (waypoint_navgoal mit "radius" 40): Abstand,
    // Hoehe bis 24 egal.
    {
        LaufWerte w = werteFuer({400.0F, 0.0F, 20.0F}, 150.0F, 50.0F, 0);
        w.zielRadius = 40.0F;
        w.zielMitRadius = true;
        const LaufPlan p = planeLauf(*welt, z, w, 0.0, 20000.0);
        const NavVec bei = ortBei(p, p.ankunftMs);
        pruefe(p.ankunftMs > 0.0 && bei[0] >= 360.0F - 0.01F && bei[0] < 368.0F,
               "1c mit Radius 40: angekommen bei Abstand 40 (x " + std::to_string(bei[0]) + ")");
    }
    // 1d: das Ziel hinter der Figur - sie dreht sich nur in den Denkbildern mit
    // Verhalten (alle 100 ms), dazwischen ueberschreibt last_ucmd die Winkel
    // (codeJK2/game/NPC.cpp:2113-2115); je Schritt (60 + 3*yawSpeed) * 0,05.
    {
        LaufZustand z2 = z;
        z2.gier = 0.0F;
        z2.wunschGier = 0.0F;
        LaufWerte w = werteFuer({-400.0F, 10.0F, 0.0F}, 150.0F, 50.0F, 0);
        w.drehRate = 330.0F;
        const LaufPlan p = planeLauf(*welt, z2, w, 0.0, 20000.0);
        int wechsel = 0;
        int gleich = 0;
        float vorher = z2.gier;
        for (std::size_t i = 0; i < p.zustaende.size() && i < 8U; ++i) {
            const float g = p.zustaende[i].gier;
            if (std::fabs(g - vorher) > 0.001F) {
                ++wechsel;
            } else {
                ++gleich;
            }
            vorher = g;
        }
        pruefe(wechsel >= 3 && gleich >= 3,
               "1d drehen nur alle 100 ms: " + std::to_string(wechsel) + " Bilder mit, " + std::to_string(gleich) + " ohne Drehung");
    }
}

// --- Teil 2: kejim_post aus der echten Installation ----------------------------
bool lies(const std::string& base, const std::string& name, std::string& aus) {
    for (const char* pk : {"assets5.pk3", "assets2.pk3", "assets1.pk3", "assets0.pk3"}) {
        Pk3 a;
        if (!readPk3Directory(base + "/" + pk, a)) { continue; }
        const Pk3Entry* e = a.find(name);
        if (e != nullptr && readPk3File(a, *e, aus)) { return true; }
    }
    return false;
}

void teil2(const std::string& base) {
    std::string bsp;
    std::string nav;
    if (!lies(base, "maps/kejim_post.bsp", bsp) || !lies(base, "maps/kejim_post.nav", nav)) {
        std::printf("--   Teil 2 uebersprungen (keine JK2-Installation unter %s)\n", base.c_str());
        return;
    }
    MapData m;
    std::string err;
    if (!readBsp(bsp, m, &err)) {
        pruefe(false, "2 kejim_post.bsp lesbar: " + err);
        return;
    }
    const int summe = navBlockPruefsumme(bsp);
    int dateiSumme = 0;
    const std::shared_ptr<NavNetzJk2> datei = navJk2Laden(nav, dateiSumme);
    pruefe(datei != nullptr && summe == dateiSumme && summe == -801285701,
           "2a kejim_post.nav passt zur .bsp (Com_BlockChecksum " + std::to_string(summe) + ")");
    if (!datei) { return; }
    pruefe(datei->knoten.size() == 239U, "2a .nav: 239 Knoten (" + std::to_string(datei->knoten.size()) + ")");
    // Das Netz der Vorschau: mit passender .nav die Datei.
    auto k = std::make_shared<KollisionsKarte>();
    const bool mitSpur = leseKollision(bsp, *k);
    NavBauHilfe h;
    if (mitSpur) {
        h.spur = [k](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t maske) {
            const KollisionsSpur s = kollisionsSpur(*k, a.data(), mi.data(), ma.data(), b.data(), maske);
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
    }
    h.jk2Nav = nav;
    h.jk2SummeDa = true;
    h.jk2KartenSumme = summe;
    const std::shared_ptr<NavNetz> netz = baueNavNetz(m.entities, h);
    pruefe(netz && netz->jk2 && netz->jk2->ausDatei && netz->jk2->knoten.size() == 239U,
           "2b mit dem JK2-Regelwerk gilt das Netz aus der .nav");
    // rc605: CNavigator::Load liest die 32 failedEdges der Datei (g_navigator.cpp:612-616),
    // G_InitGame loescht sie gleich danach (ClearAllFailedEdges, g_main.cpp:716-719,
    // ausser beim Laden eines Spielstands) - die Vorschau startet ohne gescheiterte Kanten.
    pruefe(datei->fehl.size() == 10U && netz && netz->jk2 && netz->jk2->fehl.empty(),
           "2b2 die .nav speichert 10 gescheiterte Kanten, das Spiel startet ohne (" + std::to_string(datei->fehl.size()) + " / " +
               std::to_string(netz && netz->jk2 ? netz->jk2->fehl.size() : 999U) + ")");
    // Dasselbe Netz aus den Wegpunkten gebaut (ohne .nav, nur die Welt):
    // dieselben Knoten und Verbindungen wie Ravens Datei.
    NavBauHilfe ohne = h;
    ohne.jk2Nav.clear();
    const std::shared_ptr<NavNetz> gebaut = baueNavNetz(m.entities, ohne);
    if (!gebaut || !gebaut->jk2) {
        pruefe(false, "2c Netz aus den Wegpunkten gebaut");
        return;
    }
    const NavNetzJk2& g = *gebaut->jk2;
    const NavNetzJk2& d = *datei;
    int ortGleich = 0;
    int kantenGleich = 0;
    int radiusGleich = 0;
    int rangGleich = 0;
    bool selbstKante = false;
    const std::size_t n = std::min(g.knoten.size(), d.knoten.size());
    for (std::size_t i = 0; i < n; ++i) {
        const auto& a = g.knoten[i];
        const auto& b = d.knoten[i];
        if (std::fabs(a.punkt[0] - b.punkt[0]) < 0.01F && std::fabs(a.punkt[1] - b.punkt[1]) < 0.01F &&
            std::fabs(a.punkt[2] - b.punkt[2]) < 0.01F) {
            ++ortGleich;
        }
        if (a.radius == b.radius) { ++radiusGleich; }
        // Ravens .nav hat an Knoten 0 eine Kante zu sich selbst (Kosten 0):
        // ClearAllFailedEdges lief damals ohne das memset auf WAYPOINT_NONE
        // (OpenJK g_navigator.cpp:1828-1835), SetEdgeCost(0, 0, -1) legte sie
        // an. Das Spiel laedt die .nav - behaved auch; gebaut wird nach OpenJK.
        std::vector<NavNetzJk2::Kante> bk = b.kanten;
        if (i == 0 && !bk.empty() && bk.front().id == 0 && bk.front().kosten == 0) {
            bk.erase(bk.begin());
            selbstKante = true;
        }
        bool gleich = a.kanten.size() == bk.size();
        for (std::size_t e = 0; gleich && e < a.kanten.size(); ++e) {
            gleich = a.kanten[e].id == bk[e].id && a.kanten[e].kosten == bk[e].kosten;
        }
        if (gleich) {
            ++kantenGleich;
        } else if (std::getenv("JK2DBG") != nullptr) {
            std::printf("Knoten %zu: gebaut", i);
            for (const auto& e : a.kanten) { std::printf(" %d/%d/%d", e.id, e.kosten, e.flags); }
            std::printf(" - .nav");
            for (const auto& e : b.kanten) { std::printf(" %d/%d/%d", e.id, e.kosten, e.flags); }
            std::printf("\n");
        }
        if (a.raenge == b.raenge || (i == 0 && selbstKante)) { ++rangGleich; }
    }
    std::printf("--   gebaut %zu Knoten: Ort %d, Radius %d, Kanten %d, Raenge %d gleich der .nav\n", g.knoten.size(), ortGleich,
                radiusGleich, kantenGleich, rangGleich);
    pruefe(g.knoten.size() == d.knoten.size() && kantenGleich == static_cast<int>(n),
           "2c aus den Wegpunkten: dieselben Knoten und Verbindungen wie die .nav (HardConnect)");
    pruefe(selbstKante, "2c die .nav hat Ravens Kante 0->0 (das Spiel laedt sie so)");
    pruefe(rangGleich == static_cast<int>(n), "2c ... und dieselben Raenge (CalculatePath)");

    // 2d: ein Weg, den keine gerade Linie schafft - die Figur folgt den
    // Knoten der .nav (GetBestPathBetweenEnts / GetBestNodeAltRoute: der
    // Nachbar mit dem kleinsten Rang zum Zielknoten).
    if (!mitSpur) { return; }
    NavWelt welt;
    welt.netz = netz;
    welt.spur = h.spur;
    welt.boden = [k](float x, float y, float zOben, float zUnten, float& bodenZ) {
        const float a[3] = {x, y, zOben};
        const float b[3] = {x, y, zUnten};
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const KollisionsSpur s = kollisionsSpur(*k, a, null, null, b, kNavSolid);
        if (s.startsolid || s.fraction >= 1.0F) { return false; }
        bodenZ = s.endpos[2];
        return true;
    };
    const NavVec mi{-16.0F, -16.0F, -24.0F + 18.0F};
    const NavVec ma{16.0F, 16.0F, 40.0F};
    int von = -1;
    int nach = -1;
    std::vector<int> route;
    for (std::size_t i = 0; i < d.knoten.size() && von < 0; ++i) {
        for (std::size_t j = 0; j < d.knoten.size(); ++j) {
            const NavVec& a = d.knoten[i].punkt;
            const NavVec& b = d.knoten[j].punkt;
            const float dx = b[0] - a[0];
            const float dy = b[1] - a[1];
            if (std::fabs(b[2] - a[2]) > 8.0F || dx * dx + dy * dy < 400.0F * 400.0F || dx * dx + dy * dy > 700.0F * 700.0F) {
                continue;
            }
            if (h.spur(a, mi, ma, b, kNavKeineEntity, kNavSolid | kNavMonsterClip | kNavBotClip).fraction > 0.5F) { continue; }
            // die Route ueber die Raenge
            std::vector<int> r{static_cast<int>(i)};
            int jetzt = static_cast<int>(i);
            bool gut = true;
            while (jetzt != static_cast<int>(j) && gut && r.size() < 12U) {
                int best = -1;
                int bestRang = 1 << 30;
                for (const auto& e : d.knoten[static_cast<std::size_t>(jetzt)].kanten) {
                    if (e.id == jetzt) { continue; }
                    const int rg = e.id == static_cast<int>(j) ? -1 : d.knoten[j].raenge[static_cast<std::size_t>(e.id)];
                    if (rg != -1 && rg < 0) { continue; }
                    if (e.id == static_cast<int>(j)) {
                        best = e.id;
                        break;
                    }
                    if (rg < bestRang) {
                        best = e.id;
                        bestRang = rg;
                    }
                }
                gut = best >= 0;
                jetzt = best;
                r.push_back(best);
            }
            if (!gut || jetzt != static_cast<int>(j) || r.size() < 4U) { continue; }
            von = static_cast<int>(i);
            nach = static_cast<int>(j);
            route = r;
            break;
        }
    }
    if (von < 0) {
        pruefe(false, "2d ein Paar ohne gerade Linie gefunden");
        return;
    }
    LaufZustand z;
    z.ort = d.knoten[static_cast<std::size_t>(von)].punkt;
    z.denkNullMs = 0.0;
    LaufWerte w = werteFuer(d.knoten[static_cast<std::size_t>(nach)].punkt, 150.0F, 50.0F, 0);
    w.fallen = true;
    const LaufPlan p = planeLauf(welt, z, w, 0.0, 60000.0);
    if (std::getenv("JK2DBG") != nullptr) {
        for (const LaufTakt& t : p.takte) {
            std::printf("t=%.0f %.0f %.0f %.0f tempo=%d stueck=%d\n", t.t, t.ort[0], t.ort[1], t.ort[2], t.tempo, t.stueck);
        }
        for (const int r : route) {
            const NavVec& q = d.knoten[static_cast<std::size_t>(r)].punkt;
            std::printf("route %d: %.0f %.0f %.0f\n", r, q[0], q[1], q[2]);
        }
    }
    // Die gerade Linie ist zu (NPC_ClearPathToGoal) - NAVNEW_MoveToGoal geht
    // zu einem Knoten der .nav (LaufTakt::stueck = Knoten + 1), bis die Linie
    // frei ist; die Figur steckt nie in der Wand und kommt an.
    int ersterKnoten = 0;
    bool frei = true;
    for (const LaufTakt& t : p.takte) {
        if (ersterKnoten == 0 && t.tempo > 0) { ersterKnoten = t.stueck; }
        const NavVec fuss{-16.0F, -16.0F, -24.0F};
        if (h.spur(t.ort, fuss, ma, t.ort, kNavKeineEntity, kNavSolid).startsolid) { frei = false; }
    }
    if (std::getenv("JK2SCAN") != nullptr) {
        // Stichprobe (nicht in ctest): viele Knotenpaare, wie viele kommen an?
        int an = 0;
        int nie = 0;
        int sonst = 0;
        for (std::size_t i = 0; i < d.knoten.size(); i += 7) {
            for (std::size_t j = 3; j < d.knoten.size(); j += 11) {
                if (i == j) { continue; }
                const float dd = std::sqrt(laengeQ(d.knoten[i].punkt, d.knoten[j].punkt));
                if (dd > 1500.0F || std::fabs(d.knoten[i].punkt[2] - d.knoten[j].punkt[2]) > 64.0F) { continue; }
                if (d.knoten[j].raenge[i] < 0) { continue; }   // nicht verbunden
                LaufZustand zz;
                zz.ort = d.knoten[i].punkt;
                zz.denkNullMs = 0.0;
                LaufWerte ww = werteFuer(d.knoten[j].punkt, 150.0F, 50.0F, 0);
                ww.fallen = true;
                const LaufPlan pp = planeLauf(welt, zz, ww, 0.0, 60000.0);
                if (pp.ankunftMs > 0.0) {
                    ++an;
                } else if (pp.nie) {
                    ++nie;
                    std::printf("nie %zu -> %zu (%.0f)\n", i, j, dd);
                    if (std::getenv("JK2SCAN2") != nullptr) {
                        for (std::size_t q = 0; q < pp.takte.size(); q += 4) {
                            const LaufTakt& t = pp.takte[q];
                            std::printf("  t=%.0f %.0f %.0f %.0f tempo=%d stueck=%d\n", t.t, t.ort[0], t.ort[1], t.ort[2], t.tempo,
                                        t.stueck);
                        }
                        std::printf("  von %.0f %.0f %.0f nach %.0f %.0f %.0f\n", d.knoten[i].punkt[0], d.knoten[i].punkt[1],
                                    d.knoten[i].punkt[2], d.knoten[j].punkt[0], d.knoten[j].punkt[1], d.knoten[j].punkt[2]);
                    }
                } else {
                    ++sonst;
                }
            }
        }
        std::printf("Stichprobe: %d an, %d nie, %d offen\n", an, nie, sonst);
    }
    pruefe(p.ankunftMs > 0.0 && ersterKnoten > 0 && frei,
           "2d Knoten " + std::to_string(von) + " -> " + std::to_string(nach) + " (gerade Linie zu): erst zum Knoten " +
               std::to_string(ersterKnoten - 1) + ", nie in der Wand, angekommen nach " +
               std::to_string(static_cast<int>(p.ankunftMs)) + " ms");
}

// --- Teil 3: Wasser in yavin_swamp (SURF_SLICK) --------------------------------
void teil3(const std::string& base, bool alt) {
    std::string bsp;
    if (!lies(base, "maps/yavin_swamp.bsp", bsp)) {
        std::printf("--   Teil 3 uebersprungen (keine JK2-Installation)\n");
        return;
    }
    const auto zaehle = [&bsp](bool jk2) {
        if (jk2) {
            regelwerkAnwendenSpiel(kRegelwerkJediOutcast, -1);
        } else {
            regelwerkAnwenden(false, -1);
        }
        KollisionsKarte k;
        if (!leseKollision(bsp, k)) { return -1; }
        int fest = 0;
        for (const auto& p : k.pinsel) {
            if (p.shader >= 0 && static_cast<std::size_t>(p.shader) < k.shaderFlaeche.size() &&
                (k.shaderFlaeche[static_cast<std::size_t>(p.shader)] & 0x00004000U) != 0U && (p.inhalt & 1U) != 0U) {
                ++fest;
            }
        }
        return fest;
    };
    const int openjk = zaehle(false);
    const int jk2 = zaehle(!alt);
    regelwerkAnwendenSpiel(kRegelwerkJediOutcast, -1);
    if (alt) { navJk2Regeln(false); }
    pruefe(openjk > 0 && jk2 == 0, "3 yavin_swamp: Brushes mit SURF_SLICK unter OpenJK fest (" + std::to_string(openjk) +
                                       "), unter Jedi Outcast nicht (" + std::to_string(jk2) + ", cm_load.cpp:272-278)");
    // mindtrickscript (g_spawn.cpp:287) steht unter den Skripten der Karte.
    std::string bespin;
    MapData m;
    if (lies(base, "maps/bespin_undercity.bsp", bespin) && readBsp(bespin, m, nullptr)) {
        int mit = 0;
        int gefunden = 0;
        for (const MapEntity& e : m.entities) {
            const std::string* v = e.find("mindtrickscript");
            if (v == nullptr || v->empty()) { continue; }
            ++mit;
            for (const MapData::ScriptRef& r : m.scripts) {
                if (r.path == *v) {
                    ++gefunden;
                    break;
                }
            }
        }
        pruefe(mit > 0 && gefunden == mit, "3 bespin_undercity: " + std::to_string(gefunden) + " von " + std::to_string(mit) +
                                               " mindtrickscript unter den Skripten der Karte");
    }
}

}  // namespace

// rc605 (Rest): CheckFailedEdge mit Pause. Eine gescheiterte Kante (Netz ohne
// .nav, CheckBlockedEdges), die Spur trifft bis 2000 ms eine Entity: Pruefung im
// ersten Bild nach 750 (800, gesperrt), dann nach 800 + 1500 (2350, frei) - nicht
// schon bei 2000.
void teil4() {
    auto netz = std::make_shared<NavNetz>();
    auto j2 = std::make_shared<NavNetzJk2>();
    j2->knoten.resize(2);
    j2->knoten[0].punkt = {0.0F, 0.0F, 0.0F};
    j2->knoten[1].punkt = {200.0F, 0.0F, 0.0F};
    j2->fehl.push_back(NavNetzJk2::Fehl{0, 1, 1023});
    netz->jk2 = j2;
    NavWelt w;
    w.netz = netz;
    w.spurZeit = [](const NavVec& a, const NavVec&, const NavVec&, const NavVec& b, int, std::uint32_t, double t) {
        NavSpur s;
        s.endpos = b;
        if (t < 2000.0) {
            s.fraction = 0.5F;
            s.entity = 5;
            for (int k = 0; k < 3; ++k) { s.endpos[static_cast<std::size_t>(k)] = 0.5F * (a[static_cast<std::size_t>(k)] + b[static_cast<std::size_t>(k)]); }
        }
        return s;
    };
    const double frei = navJk2FehlFreiAb(w, 0, 10000.0);
    const double vorher = navJk2FehlFreiAb(w, 0, 2300.0);
    pruefe(frei == 2350.0 && vorher < 0.0,
           "4a CheckFailedEdge: gesperrt bei 800, frei erst bei 2350 (nicht 2000): " + std::to_string(frei) + " / bis 2300 " +
               std::to_string(vorher));
}

// --- Teil 5: rc605 (Ende) - AddFailedEdge zur Laufzeit ----------------------
// Wegpunkte A(0,0) - B(200,0) - C(400,0), Umweg A - D(100,300) - E(300,300) - C.
// Ein func_breakable steht zwischen B und C (x 290..310). NAV_TestBestNode trifft
// ihn von B aus: AddFailedEdge(B, C) (codeJK2/game/g_nav.cpp:1031-1038), SetEdgeCost
// Q3_INFINITE und NF_RECALC (g_navigator.cpp:2004-2008) - nach den neuen Raengen
// geht die Figur den Umweg. Vorher blieb sie bei B stehen.
MapEntity entZiel(const std::string& klasse, const std::string& ort, const std::string& name,
                  const std::vector<std::pair<std::string, std::string>>& ziele) {
    MapEntity e = ent(klasse, ort, name);
    for (const auto& [k, v] : ziele) { e.keys.push_back({k, v}); }
    return e;
}
std::shared_ptr<NavWelt> weltKiste() {
    std::vector<MapEntity> ents;
    ents.push_back(ent("worldspawn", ""));
    ents.push_back(entZiel("waypoint_small", "0 0 24", "a", {{"target", "b"}, {"target2", "d"}}));
    ents.push_back(entZiel("waypoint_small", "200 0 24", "b", {{"target", "c"}}));
    ents.push_back(entZiel("waypoint_small", "400 0 24", "c", {}));
    ents.push_back(entZiel("waypoint_small", "100 300 24", "d", {{"target", "e"}}));
    ents.push_back(entZiel("waypoint_small", "300 300 24", "e", {{"target", "c"}}));
    ents.push_back(entZiel("func_breakable", "0 0 0", "kiste", {{"model", "*1"}}));
    const int kiste = static_cast<int>(ents.size()) - 1;
    const std::shared_ptr<const NavWelt> basis = navWeltAusEntities(ents);
    auto welt = std::make_shared<NavWelt>(*basis);
    welt->kennung = 0;
    // Boden (Welt) und die Kiste (Entity kiste): Kaesten, um den Kasten der Spur erweitert.
    struct Klotz {
        float lo[3];
        float hi[3];
        int entity;
    };
    const std::vector<Klotz> kloetze = {{{-3000.0F, -3000.0F, -16.0F}, {3000.0F, 3000.0F, 0.0F}, kNavWeltEntity},
                                        {{290.0F, -60.0F, 0.0F}, {310.0F, 60.0F, 500.0F}, kiste}};
    welt->spur = [kloetze](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t m) {
        NavSpur sp;
        sp.endpos = b;
        if ((m & kNavSolid) == 0U) { return sp; }
        for (const Klotz& kl : kloetze) {
            float lo[3];
            float hi[3];
            for (int k = 0; k < 3; ++k) {
                lo[k] = kl.lo[k] - ma[static_cast<std::size_t>(k)];
                hi[k] = kl.hi[k] - mi[static_cast<std::size_t>(k)];
            }
            float ein = -1.0e9F;
            float aus = 1.0e9F;
            int achse = -1;
            bool weg = false;
            for (int k = 0; k < 3; ++k) {
                const float da = a[static_cast<std::size_t>(k)];
                const float d = b[static_cast<std::size_t>(k)] - da;
                if (std::fabs(d) < 1.0e-6F) {
                    if (da <= lo[k] || da >= hi[k]) { weg = true; }
                    continue;
                }
                float t0 = (lo[k] - da) / d;
                float t1 = (hi[k] - da) / d;
                if (t0 > t1) { std::swap(t0, t1); }
                if (t0 > ein) {
                    ein = t0;
                    achse = k;
                }
                aus = std::min(aus, t1);
            }
            if (weg || ein >= aus || aus <= 0.0F || ein >= 1.0F) { continue; }
            if (ein < 0.0F) {
                sp.startsolid = true;
                sp.fraction = 0.0F;
                sp.endpos = a;
                sp.entity = kl.entity;
                sp.contents = kNavSolid;
                continue;
            }
            const float f = std::max(0.0F, ein - 0.001F);
            if (f < sp.fraction) {
                sp.fraction = f;
                sp.entity = kl.entity;
                sp.contents = kNavSolid;
                sp.normal = {0.0F, 0.0F, 0.0F};
                sp.normal[static_cast<std::size_t>(achse)] = (b[static_cast<std::size_t>(achse)] > a[static_cast<std::size_t>(achse)]) ? -1.0F : 1.0F;
                for (int k = 0; k < 3; ++k) {
                    sp.endpos[static_cast<std::size_t>(k)] = a[static_cast<std::size_t>(k)] + f * (b[static_cast<std::size_t>(k)] - a[static_cast<std::size_t>(k)]);
                }
            }
        }
        return sp;
    };
    welt->boden = [](float, float, float zOben, float zUnten, float& zb) {
        if (zOben < 0.0F || zUnten > 0.0F) { return false; }
        zb = 0.0F;
        return true;
    };
    return welt;
}

void teil5(bool alt) {
    const std::shared_ptr<NavWelt> welt = weltKiste();
    LaufZustand z;
    z.ort = {10.0F, 0.0F, 24.125F};
    z.amBoden = true;
    z.denkNullMs = 0.0;
    LaufWerte w5 = werteFuer({400.0F, 0.0F, 24.125F}, 150.0F, 90.0F, 0);
    w5.fallen = true;
    w5.drehRate = 330.0F;
    const LaufPlan p = planeLauf(*welt, z, w5, 0.0, 30000.0);
    float yMax = 0.0F;
    float xMax = 0.0F;
    for (const LaufTakt& t : p.takte) {
        yMax = std::max(yMax, t.ort[1]);
        xMax = std::max(xMax, t.ort[0]);
    }
    std::printf("  5a angekommen %.0f, nie %d, hoechstens y %.1f, x %.1f, AddFailedEdge %zu\n", p.ankunftMs, p.nie ? 1 : 0,
                static_cast<double>(yMax), static_cast<double>(xMax), p.jk2Dazu.size());
    if (!alt) {
        pruefe(p.ankunftMs > 0.0 && yMax > 150.0F && !p.jk2Dazu.empty() && p.jk2Dazu.front().a == 1 && p.jk2Dazu.front().b == 2,
               "5a Kiste zwischen B und C: AddFailedEdge(B, C), neue Raenge, die Figur geht ueber E und kommt an");
    }
}

// --- Teil 6: rc605 (Ende) - die gescheiterte Kante gilt fuer alle NPCs ------
// Figur 1 geht von C nach A und markiert (C, B) gescheitert; Figur 2 geht ab
// 3000 ms von A nach C. Ohne die anderen markiert Figur 2 zuerst selbst (B, C);
// im gemeinsamen Takt kennt sie die Kante schon (EdgeFailed findet (C, B) ueber
// den zweiten Schluessel, g_navigator.cpp:1851-1858) und markiert sie nicht neu.
void teil6() {
    const std::shared_ptr<NavWelt> welt = weltKiste();
    const auto abschnitt = [&](int figur, NavVec von, NavVec nach, double ab) {
        NavAbschnitt a;
        a.figur = figur;
        a.abMs = ab;
        a.grenzeMs = ab + 30000.0;
        a.start.ort = von;
        a.start.amBoden = true;
        a.werte = werteFuer(nach, 150.0F, 90.0F, 0);
        a.werte.fallen = true;
        a.werte.drehRate = 330.0F;
        a.sprung = true;
        a.allein = std::make_shared<const LaufPlan>(planeLauf(*welt, a.start, a.werte, a.abMs, a.grenzeMs));
        return a;
    };
    std::vector<NavFigurKoerper> figuren(2);
    figuren[1].reihe = 1;
    const std::vector<NavAbschnitt> abs = {abschnitt(0, {390.0F, 0.0F, 24.125F}, {0.0F, 0.0F, 24.125F}, 0.0),
                                           abschnitt(1, {10.0F, 0.0F, 24.125F}, {400.0F, 0.0F, 24.125F}, 3000.0)};
    const NavKoerperErgebnis e = navKoerperLauf(*welt, figuren, abs);
    const auto hat = [](const LaufPlan& p, int x, int y) {
        for (const NavJk2FehlWelt::Dazu& d : p.jk2Dazu) {
            if ((d.a == x && d.b == y) || (d.a == y && d.b == x)) { return true; }
        }
        return false;
    };
    const LaufPlan& allein2 = *abs[1].allein;
    const LaufPlan& geteilt2 = *e.plaene[1];
    std::printf("  6a Figur 1 allein: %zu AddFailedEdge (zuerst %d-%d); Figur 2 allein: %zu (zuerst %d-%d), gemeinsam: %zu, angekommen %.0f / %.0f\n",
                abs[0].allein->jk2Dazu.size(), abs[0].allein->jk2Dazu.empty() ? -1 : abs[0].allein->jk2Dazu.front().a,
                abs[0].allein->jk2Dazu.empty() ? -1 : abs[0].allein->jk2Dazu.front().b, allein2.jk2Dazu.size(),
                allein2.jk2Dazu.empty() ? -1 : allein2.jk2Dazu.front().a, allein2.jk2Dazu.empty() ? -1 : allein2.jk2Dazu.front().b,
                geteilt2.jk2Dazu.size(), allein2.ankunftMs, geteilt2.ankunftMs);
    pruefe(hat(*abs[0].allein, 1, 2) && hat(allein2, 1, 2) && !hat(geteilt2, 1, 2) && geteilt2.ankunftMs > 0.0,
           "6a die Kante, die Figur 1 markierte, gilt im gemeinsamen Takt auch fuer Figur 2 (keine eigene Markierung, sie kommt an)");
    // 6b: eine einzige Figur mit zwei Plaenen - die Kante aus dem ersten gilt im
    // zweiten weiter (navKoerperLauf rechnet auch mit einer Figur, wenn ein Plan
    // zur Laufzeit eine Kante markierte).
    {
        std::vector<NavFigurKoerper> eine(1);
        NavAbschnitt a1 = abschnitt(0, {390.0F, 0.0F, 24.125F}, {0.0F, 0.0F, 24.125F}, 0.0);
        a1.schnittMs = 10000.0;
        const NavAbschnitt a2 = abschnitt(0, {10.0F, 0.0F, 24.125F}, {400.0F, 0.0F, 24.125F}, 10000.0);
        const NavKoerperErgebnis e1 = navKoerperLauf(*welt, eine, {a1, a2});
        std::printf("  6b eine Figur: Plan 2 allein %zu AddFailedEdge, gemeinsam %zu, angekommen %.0f\n", a2.allein->jk2Dazu.size(),
                    e1.plaene[1]->jk2Dazu.size(), e1.plaene[1]->ankunftMs);
        pruefe(hat(*a2.allein, 1, 2) && !hat(*e1.plaene[1], 1, 2) && e1.plaene[1]->ankunftMs > 0.0,
               "6b eine Figur: die Kante aus ihrem ersten Plan gilt in ihrem zweiten weiter");
    }
}

int main(int argc, char** argv) {
    bool alt = false;
    std::string base = "C:/Program Files (x86)/Steam/steamapps/common/Jedi Outcast/GameData/base";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--alt") == 0) {
            alt = true;
        } else {
            base = argv[i];
        }
    }
    regelwerkAnwendenSpiel(kRegelwerkJediOutcast, -1);
    if (alt) { navJk2Regeln(false); }   // so rechnete behaved bis fix604
    teil1();
    teil2(base);
    teil3(base, alt);
    teil4();   // rc605 (Rest)
    teil5(alt);   // rc605 (Ende)
    if (!alt) { teil6(); }   // rc605 (Ende)
    std::printf("%d ok, %d FEHL\n", gOk, gFehler);
    return gFehler == 0 ? 0 : 1;
}
