// kartespielen.cpp - Karten-Editor Stufe 8b Punkt 3: "Im Editor spielen".
//
// Die Kollision aus den Brushes und Patches der Karte (kollisionAusKarte) gegen
// die aus der mit bhed_q3map2 gebauten .bsp (leseKollision, wie die Vorschau sie
// laedt), dazu der Spieler mit dem vorhandenen Pmove-Port (navSpielerPmove):
//   - 20000 Kastenspuren mit dem Spielerkasten zwischen Rasterpunkten: gleiche
//     fraction, startsolid/allsolid, Inhalte (Welt + Tuer der .bsp);
//   - eingespielte Laeufe mit beiden Kollisionen Bild fuer Bild gleich: Fallen
//     und Landen, gegen die Wand (haelt an), Stufe 16 (gestiegen) und 24
//     (haelt an), Rampe 30 Grad, playerclip (haelt an), monsterclip und
//     nonsolid (durch), func_door (haelt an), trigger_multiple (durch), Patch
//     (darueber), Sprung (Scheitel gleich dem Port mit der .bsp, 31,6 Einheiten
//     wie JUMP_VELOCITY 225 bei 800), Ducken (12 Augenhoehe, halbe Geschwindigkeit),
//     Noclip (durch die Wand).
//
//   kartespielen <tests/karte/q3map2>
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/kollision.h"
#include "bhed/navlauf.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) { ++g_fehler; }
    std::printf("%s  %s\n", ok ? "OK  " : "FEHL", was.c_str());
}

std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// Die surfaceparms aus einer Shaderdatei (Name -> Liste)
std::map<std::string, std::vector<std::string>> shaderParms(const std::string& text) {
    std::map<std::string, std::vector<std::string>> m;
    std::istringstream s(text);
    std::string tok;
    std::string name;
    int tiefe = 0;
    while (s >> tok) {
        if (tok == "{") {
            ++tiefe;
        } else if (tok == "}") {
            --tiefe;
        } else if (tiefe == 0) {
            name = tok;
            m[name];
        } else if (tiefe == 1 && tok == "surfaceparm") {
            std::string p;
            s >> p;
            m[name].push_back(p);
        }
    }
    return m;
}

struct Welt {
    const KollisionsKarte* k = nullptr;
    bool bsp = false;   // .bsp: Welt (Modell 0) und die Tuer (Modell 1)
    NavSpur spur(const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, std::uint32_t maske) const {
        KollisionsSpur t = kollisionsSpur(*k, a.data(), mi.data(), ma.data(), b.data(), maske, 0);
        if (bsp && k->modelle.size() > 1) {
            const float null[3] = {0, 0, 0};
            const KollisionsSpur u = kollisionsSpur(*k, a.data(), mi.data(), ma.data(), b.data(), maske, 1, null, null);
            // SV_ClipMoveToEntities: die naechste Beruehrung gewinnt
            if (u.allsolid || u.startsolid || u.fraction < t.fraction) {
                const bool alt = t.startsolid;
                if (u.fraction < t.fraction) {
                    t.fraction = u.fraction;
                    t.endpos = u.endpos;
                    t.normale = u.normale;
                    t.inhalt = u.inhalt;
                }
                t.allsolid = t.allsolid || u.allsolid;
                t.startsolid = alt || u.startsolid;
            }
        }
        NavSpur r;
        r.allsolid = t.allsolid;
        r.startsolid = t.startsolid;
        r.fraction = t.fraction;
        r.endpos = t.endpos;
        r.normal = t.normale;
        r.contents = t.inhalt;
        r.entity = t.fraction < 1.0F ? kNavWeltEntity : kNavKeineEntity;
        return r;
    }
    NavSpurFn fn() const {
        return [this](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t m) { return spur(a, mi, ma, b, m); };
    }
};

struct Lauf {
    std::vector<NavVec> bahn;
    NavSpieler s;
    float zMax = -1e9F;
};

Lauf laufen(const Welt& w, NavSpieler start, float sekunden, NavSpielerEingabe e, int msec = 16, int sprungBild = -1) {
    Lauf l;
    l.s = start;
    const int bilder = static_cast<int>(sekunden * 1000.0F) / msec;
    for (int i = 0; i < bilder; ++i) {
        NavSpielerEingabe ei = e;
        if (sprungBild >= 0) { ei.hoch = i == sprungBild ? static_cast<signed char>(127) : static_cast<signed char>(0); }
        navSpielerPmove(l.s, w.fn(), ei, msec);
        l.bahn.push_back(l.s.ort);
        l.zMax = std::max(l.zMax, l.s.ort[2]);
    }
    return l;
}

float abweichung(const Lauf& a, const Lauf& b) {
    float m = 0;
    for (std::size_t i = 0; i < a.bahn.size() && i < b.bahn.size(); ++i) {
        for (std::size_t k = 0; k < 3; ++k) { m = std::max(m, std::fabs(a.bahn[i][k] - b.bahn[i][k])); }
    }
    return a.bahn.size() == b.bahn.size() ? m : 1e9F;
}

NavSpieler spieler(float x, float y, float z, float gier) {
    NavSpieler s;
    s.ort = {x, y, z};
    s.gier = gier;
    return s;
}

std::string z3(const NavVec& v) {
    char b[96];
    std::snprintf(b, sizeof(b), "(%.3f %.3f %.3f)", v[0], v[1], v[2]);
    return b;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: kartespielen <tests/karte/q3map2>\n");
        return 2;
    }
    const std::string ordner = argv[1];
    Karte karte;
    (void)karteLesen(lies(ordner + "/spielraum.map"), karte);
    karte.anmelden();
    const auto parms = shaderParms(lies(ordner + "/spielraum.shader"));
    KollisionsKarte ed;
    const SpielKollisionsBericht ber = kollisionAusKarte(
        karte,
        [&parms](const std::string& sh) {
            const auto it = parms.find(sh.rfind("textures/", 0) == 0 ? sh : "textures/" + sh);
            return it != parms.end() ? it->second : std::vector<std::string>{};
        },
        true, ed);
    std::printf("Editor: %d Brushes, %d Seiten, %d Bevels, %d Patches, %d ausgelassen\n", ber.pinsel, ber.seiten, ber.bevels, ber.patches, ber.ausgelassen);
    KollisionsKarte bsp;
    pruefe(leseKollision(lies(ordner + "/spielraum.bsp"), bsp), "spielraum.bsp geladen (CM-Port)");
    pruefe(ber.pinsel == 9 && ber.patches == 1 && ber.ausgelassen == 1, "Editor: 9 Brushes (8 Welt + Tuer), 1 Patch, trigger_multiple ausgelassen");
    const Welt we{&ed, false};
    const Welt wb{&bsp, true};

    // 1. Kastenspuren
    {
        const NavVec mi{-16, -16, -24};
        const NavVec ma{16, 16, 40};
        int gleich = 0;
        int alle = 0;
        int inhaltStart = 0;   // startsolid: trace.contents |= je Brush, in dem der Kasten beginnt - haengt an der Reihenfolge
        float maxDf = 0;
        std::string erster;
        unsigned zufall = 12345U;
        const auto r = [&zufall](float lo, float hi) {
            zufall = zufall * 1103515245U + 12345U;
            return lo + (hi - lo) * static_cast<float>((zufall >> 8) & 0xFFFF) / 65535.0F;
        };
        for (int i = 0; i < 20000; ++i) {
            const NavVec a{r(-700, 700), r(-800, 800), r(-10, 200)};
            const NavVec b{r(-700, 700), r(-800, 800), r(-10, 200)};
            const std::uint32_t maske = (i % 3 == 0) ? kNavMaskNpcSolid : kNavMaskPlayerSolid;
            const NavSpur x = we.spur(a, mi, ma, b, maske);
            const NavSpur y = wb.spur(a, mi, ma, b, maske);
            ++alle;
            const float df = std::fabs(x.fraction - y.fraction);
            maxDf = std::max(maxDf, df);
            const bool inhaltGleich = x.contents == y.contents || x.startsolid;
            if (x.startsolid && x.contents != y.contents) { ++inhaltStart; }
            if (x.allsolid == y.allsolid && x.startsolid == y.startsolid && df == 0.0F && inhaltGleich &&
                (x.fraction == 1.0F || x.startsolid || x.normal == y.normal)) {
                ++gleich;
            } else if (erster.empty()) {
                erster = z3(a) + " -> " + z3(b) + ": " + std::to_string(x.fraction) + "/" + std::to_string(y.fraction) + " solid " +
                         std::to_string(x.startsolid) + "/" + std::to_string(y.startsolid) + " inhalt " + std::to_string(x.contents) + "/" +
                         std::to_string(y.contents);
            }
        }
        std::printf("Spuren: %d von %d gleich (fraction, solid, Normale, Inhalt), groesste Abweichung der fraction %g; %d mit startsolid und "
                    "anderem Inhalt (Reihenfolge der Brushes) %s\n",
                    gleich, alle, maxDf, inhaltStart, erster.c_str());
        pruefe(gleich == alle, "20000 Kastenspuren: Kollision aus den Brushes gleich der aus der .bsp");
    }

    // 2. Laeufe
    const NavSpielerEingabe vor{127, 0, 0};
    const auto beide = [&](const std::string& name, NavSpieler s, float sek, NavSpielerEingabe e, int sprungBild = -1) {
        const Lauf a = laufen(we, s, sek, e, 16, sprungBild);
        const Lauf b = laufen(wb, s, sek, e, 16, sprungBild);
        const float d = abweichung(a, b);
        pruefe(d == 0.0F, name + ": Bild fuer Bild gleich mit der .bsp (Abweichung " + std::to_string(d) + ")");
        return a;
    };
    {
        const Lauf l = beide("Fallen", spieler(0, -200, 100, 0), 1.0F, NavSpielerEingabe{});
        std::printf("Fallen: Ende %s am Boden %d\n", z3(l.s.ort).c_str(), static_cast<int>(l.s.amBoden));
        pruefe(l.s.amBoden && std::fabs(l.s.ort[2] - 24.125F) < 0.01F, "Fallen: steht auf dem Boden (z 24,125 = 0 + 24 + SURFACE_CLIP_EPSILON)");
    }
    {
        const Lauf l = beide("Wand", spieler(300, 0, 24.125F, 0), 2.0F, vor);
        std::printf("Wand: Ende %s Tempo x %.3f\n", z3(l.s.ort).c_str(), l.s.tempo[0]);
        pruefe(l.s.ort[0] > 495.5F && l.s.ort[0] < 496.0F && std::fabs(l.s.tempo[0]) < 1.0F, "Laufen gegen die Wand (x 512) haelt bei 512 - 16 an");
    }
    {
        const Lauf l = beide("Stufe 16", spieler(0, 0, 24.125F, 0), 0.8F, vor);
        std::printf("Stufe 16: Ende %s, hoechstes z %.3f\n", z3(l.s.ort).c_str(), l.zMax);
        pruefe(l.s.ort[0] > 150.0F && std::fabs(l.zMax - 40.125F) < 0.01F, "Stufe 16 (STEPSIZE 18) wird gestiegen");
    }
    {
        const Lauf l = beide("Stufe 24", spieler(0, 0, 24.125F, 180), 1.5F, vor);
        std::printf("Stufe 24: Ende %s\n", z3(l.s.ort).c_str());
        pruefe(l.s.ort[0] < -110.0F && l.s.ort[0] > -112.5F && l.zMax < 24.2F, "Stufe 24 haelt den Spieler an");
    }
    {
        const Lauf l = beide("Rampe", spieler(-250, 320, 24.125F, 0), 1.2F, vor);
        std::printf("Rampe: Ende %s, hoechstes z %.3f\n", z3(l.s.ort).c_str(), l.zMax);
        pruefe(l.zMax > 90.0F, "Rampe 30 Grad wird hinaufgegangen");
    }
    {
        const Lauf l = beide("playerclip", spieler(0, -300, 24.125F, 270), 1.5F, vor);
        std::printf("playerclip: Ende %s\n", z3(l.s.ort).c_str());
        pruefe(l.s.ort[1] > -368.5F && l.s.ort[1] < -366.0F, "playerclip haelt den Spieler an");
    }
    {
        const Lauf l = beide("monsterclip", spieler(0, -500, 24.125F, 270), 1.5F, vor);
        pruefe(l.s.ort[1] < -650.0F, "monsterclip haelt den Spieler nicht (MASK_PLAYERSOLID)");
    }
    {
        const Lauf l = beide("nonsolid", spieler(0, 500, 24.125F, 90), 1.5F, vor);
        pruefe(l.s.ort[1] > 650.0F, "nonsolid haelt nicht");
    }
    {
        const Lauf l = beide("func_door", spieler(288, 450, 24.125F, 90), 1.5F, vor);
        std::printf("func_door: Ende %s\n", z3(l.s.ort).c_str());
        pruefe(l.s.ort[1] < 560.0F && l.s.ort[1] > 558.0F, "func_door (Brush-Entity, zu) haelt an");
    }
    {
        const Lauf l = beide("trigger_multiple", spieler(288, -450, 24.125F, 270), 1.5F, vor);
        pruefe(l.s.ort[1] < -650.0F, "trigger_multiple haelt nicht");
    }
    {
        const Lauf l = beide("Patch", spieler(-224, -420, 24.125F, 90), 1.5F, vor);
        std::printf("Patch: Ende %s, hoechstes z %.3f\n", z3(l.s.ort).c_str(), l.zMax);
        pruefe(l.zMax > 44.0F && l.s.ort[1] > -150.0F, "Patch-Bogen wird ueberquert (CM_GeneratePatchCollide)");
    }
    {
        const Lauf l = beide("Sprung", spieler(0, -200, 24.125F, 0), 1.2F, NavSpielerEingabe{}, 5);
        const float hoch = l.zMax - 24.125F;
        std::printf("Sprung: Scheitel %.4f ueber dem Boden\n", hoch);
        // die Integration von PM_SlideMove: v = (v + (v - g dt)) / 2 je Bild
        float v = 225.0F;
        float zz = 0;
        float best = 0;
        for (int i = 0; i < 200; ++i) {
            const float neu = v - 800.0F * 0.016F;
            zz += (v + neu) * 0.5F * 0.016F;
            v = neu;
            best = std::max(best, zz);
        }
        std::printf("Sprung: erwartet aus der Integration %.4f\n", best);
        pruefe(std::fabs(hoch - best) < 0.05F && hoch > 31.0F && hoch < 32.0F, "Sprunghoehe 31,6 (JUMP_VELOCITY 225, Schwerkraft 800) wie der Port mit der .bsp");
    }
    {
        NavSpieler s = spieler(0, -200, 24.125F, 0);
        Lauf l = laufen(we, s, 1.0F, NavSpielerEingabe{127, 0, -127});
        const float tempo = std::sqrt(l.s.tempo[0] * l.s.tempo[0] + l.s.tempo[1] * l.s.tempo[1]);
        std::printf("Ducken: Tempo %.2f, Augenhoehe %.1f\n", tempo, navSpielerAugenhoehe(l.s));
        pruefe(l.s.geduckt && navSpielerAugenhoehe(l.s) == 12.0F && tempo > 120.0F && tempo <= 125.01F, "Ducken: Augenhoehe 12, hoechstens halbe Geschwindigkeit (pm_duckScale)");
        l = laufen(we, l.s, 0.5F, NavSpielerEingabe{});
        pruefe(!l.s.geduckt && navSpielerAugenhoehe(l.s) == 36.0F, "Aufstehen: Augenhoehe 36 (DEFAULT_MAXS_2 40 - 4)");
    }
    {
        NavSpieler s = spieler(300, 0, 60, 0);
        s.noclip = true;
        const Lauf l = laufen(we, s, 2.0F, vor);
        pruefe(l.s.ort[0] > 600.0F, "Noclip: durch die Wand");
    }
    {
        // JK2: derselbe Lauf gegen die Wand haelt ebenso an
        NavSpieler s = spieler(300, 0, 24.125F, 0);
        s.jk2 = true;
        const Lauf l = laufen(we, s, 2.0F, vor);
        pruefe(l.s.ort[0] > 495.5F && l.s.ort[0] < 496.0F, "Regelwerk JK2: die Wand haelt an");
    }
    std::printf("kartespielen: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
