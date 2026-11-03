// rc608 Teil 5: die Leiche gegen die Formeln der Engine.
//
// planeTotenlauf (PM_DEAD: PM_DeadMove, PM_Friction mit ps.friction, PMF_TIME_KNOCKBACK, PM_CrashLand),
// PM_GroundTraceMissed -> PM_FallToDeath in planeLauf und leichenEntfernen (DeadThink, NPC_RemoveBody).
#include "bhed/bsp.h"
#include "bhed/holster.h"
#include "bhed/navlauf.h"
#include "bhed/regelwerk.h"
#include "bhed/scene.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

namespace {

using namespace bhed;

int g_fehler = 0;

void erwarte(const char* name, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", name);
    if (!ok) { ++g_fehler; }
}

// Ein Boden: z = 0 bis x = kante, dahinter z = tief.
NavWelt welt(float kante, float tief) {
    NavWelt w;
    const auto hoehe = [kante, tief](float x) { return x < kante ? 0.0F : tief; };
    w.boden = [hoehe](float x, float, float, float zUnten, float& b) {
        b = hoehe(x);
        return b >= zUnten;
    };
    w.spur = [hoehe, kante](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& e, int, std::uint32_t) {
        NavSpur s;
        s.endpos = e;
        const auto drin = [&](const NavVec& p) {
            // Die Kastenkante zaehlt: steht noch etwas vom Kasten ueber der Kante, traegt der hohe Boden.
            const float h = (p[0] + mi[0] < kante) ? 0.0F : hoehe(p[0] + mi[0]);
            return p[2] + mi[2] < h;
        };
        (void)ma;
        if (drin(a)) {
            s.fraction = 0.0F;
            s.startsolid = true;
            s.endpos = a;
            s.normal = NavVec{0.0F, 0.0F, 1.0F};
            return s;
        }
        constexpr int kSchritte = 256;
        for (int i = 1; i <= kSchritte; ++i) {
            const float f = static_cast<float>(i) / kSchritte;
            const NavVec p{a[0] + (e[0] - a[0]) * f, a[1] + (e[1] - a[1]) * f, a[2] + (e[2] - a[2]) * f};
            if (drin(p)) {
                const float g = static_cast<float>(i - 1) / kSchritte;
                s.fraction = g;
                s.endpos = NavVec{a[0] + (e[0] - a[0]) * g, a[1] + (e[1] - a[1]) * g, a[2] + (e[2] - a[2]) * g};
                s.normal = NavVec{0.0F, 0.0F, 1.0F};
                s.entity = kNavWeltEntity;
                return s;
            }
        }
        s.fraction = 1.0F;
        return s;
    };
    return w;
}

LaufWerte werte() {
    LaufWerte w;
    w.halbBreite = 15.0F;
    w.hoeheOben = 40.0F;
    w.hockHoehe = 16.0F;
    w.stufe = 18.0F;
    w.schwerkraft = 800.0F;
    w.weltSchwerkraft = 800.0F;
    return w;
}

// PM_DeadMove und PM_Friction von Hand, wie die Engine sie je Denkbild rechnet: die Strecke bis zum Stillstand.
float handStrecke(float tempo, float reibung, int rueckstossMs) {
    float weg = 0.0F;
    bool erstes = true;
    for (int i = 0; i < 2000 && tempo > 0.0F; ++i) {
        const float dt = erstes ? 0.001F : 0.05F;
        const int ms = erstes ? 1 : 50;
        erstes = false;
        if (rueckstossMs != 0) { rueckstossMs = ms >= rueckstossMs ? 0 : rueckstossMs - ms; }
        tempo -= 20.0F;   // PM_DeadMove
        if (tempo <= 0.0F) { break; }
        if (tempo < 1.0F) {
            tempo = 0.0F;
            break;
        }
        if (rueckstossMs == 0) {
            const float control = tempo < 100.0F ? 100.0F : tempo;
            tempo = std::max(0.0F, tempo - control * reibung * dt);
        }
        weg += tempo * dt;
    }
    return weg;
}

void totenlauf() {
    std::printf("Totenlauf\n");
    const NavWelt w = welt(100000.0F, 0.0F);
    LaufZustand z;
    z.ort = NavVec{0.0F, 0.0F, 24.0F};
    z.amBoden = true;
    z.tempoVek = NavVec{300.0F, 0.0F, 0.0F};
    const LaufPlan p = planeTotenlauf(w, z, werte(), 1000.0, 61000.0);
    erwarte("es gibt einen Plan", !p.takte.empty() && p.hoeheGilt);
    const float strecke = p.takte.empty() ? 0.0F : p.takte.back().ort[0];
    char t[120];
    std::snprintf(t, sizeof(t), "Friction 6 und PM_DeadMove: Strecke %.2f, von Hand %.2f", strecke, handStrecke(300.0F, 6.0F, 0));
    erwarte(t, std::fabs(strecke - handStrecke(300.0F, 6.0F, 0)) < 1.0F);
    erwarte("das erste Pmove dauert 1 ms, der Takt liegt auf 50", !p.takte.empty() && p.takte.front().t == 1050.0);

    // Der Kantenwurf: ps.friction 0.
    LaufZustand z2 = z;
    z2.reibung = 0.0F;
    const LaufPlan p2 = planeTotenlauf(w, z2, werte(), 1000.0, 61000.0);
    std::snprintf(t, sizeof(t), "ps.friction 0: Strecke %.2f, von Hand %.2f", p2.takte.back().ort[0], handStrecke(300.0F, 0.0F, 0));
    erwarte(t, std::fabs(p2.takte.back().ort[0] - handStrecke(300.0F, 0.0F, 0)) < 1.0F);

    // PMF_TIME_KNOCKBACK: keine Reibung, bis pm_time abgelaufen ist.
    LaufZustand z3 = z;
    z3.rueckstoss = true;
    z3.pmZeit = 170;
    const LaufPlan p3 = planeTotenlauf(w, z3, werte(), 1000.0, 61000.0);
    std::snprintf(t, sizeof(t), "PMF_TIME_KNOCKBACK: Strecke %.2f, von Hand %.2f", p3.takte.back().ort[0], handStrecke(300.0F, 6.0F, 170));
    erwarte(t, std::fabs(p3.takte.back().ort[0] - handStrecke(300.0F, 6.0F, 170)) < 1.0F);
    erwarte("ein Rueckstoss traegt weiter als ohne", p3.takte.back().ort[0] > p.takte.back().ort[0]);

    // Ueber die Kante: die Leiche faellt und setzt auf; PM_CrashLandDelta = |velocity[2]| / 10.
    const NavWelt tief = welt(60.0F, -600.0F);
    LaufZustand z4 = z;
    z4.reibung = 0.0F;
    const LaufPlan p4 = planeTotenlauf(tief, z4, werte(), 1000.0, 61000.0);
    erwarte("die Leiche liegt unten", !p4.takte.empty() && std::fabs(p4.takte.back().ort[2] - (-576.0F)) < 1.0F);
    float delta = -1.0F;
    for (const LaufTakt& k : p4.takte) {
        if (k.landung >= 0.0F) { delta = k.landung; }
    }
    // Freier Fall 600 tief bei 800: gut 98; fallsplat ab 75.
    std::snprintf(t, sizeof(t), "PM_CrashLand: delta %.1f", delta);
    erwarte(t, delta > 75.0F && delta < 110.0F);
}

void fallTod() {
    std::printf("PM_FallToDeath\n");
    const NavWelt grube = welt(-100000.0F, -5000.0F);
    LaufZustand z;
    z.ort = NavVec{0.0F, 0.0F, 24.0F};
    z.amBoden = false;
    z.tempoVek = NavVec{0.0F, 0.0F, -100.0F};
    LaufWerte w = werte();
    w.fallen = true;
    w.fallTod.pruefen = true;
    w.fallTod.animMs = 1000;
    const LaufPlan p = planeLauf(grube, z, w, 0.0, 10000.0);
    erwarte("ohne Boden in 400: PM_FallToDeath", p.fallTodMs >= 0.0);
    erwarte("der Aufprall beendet den Plan", p.aufprallMs > p.fallTodMs);
    LaufWerte w2 = w;
    w2.fallTod.pruefen = false;
    const LaufPlan p2 = planeLauf(grube, z, w2, 0.0, 10000.0);
    erwarte("Desann, FL_UNDYING, FL_GODMODE, SCF_NO_FALLTODEATH: kein PM_FallToDeath", p2.fallTodMs < 0.0);
    LaufWerte w3 = w;
    w3.fallTod.pruefen = true;
    LaufZustand z3 = z;
    const NavWelt flach = welt(-100000.0F, -200.0F);
    const LaufPlan p3 = planeLauf(flach, z3, w3, 0.0, 10000.0);
    erwarte("unter 256 Fall ohne Schaden: kein PM_FallToDeath", p3.fallTodMs < 0.0);
    // npcFallTodKlasse
    LaufWerte::FallTod f;
    npcFallTodKlasse(true, false, "CLASS_VADER", f);
    erwarte("Movie Duels: CLASS_VADER faellt nicht in den Tod", !f.pruefen);
    npcFallTodKlasse(false, false, "CLASS_VADER", f);
    erwarte("OpenJK: CLASS_VADER schon", f.pruefen);
    npcFallTodKlasse(false, false, "CLASS_DESANN", f);
    erwarte("OpenJK: CLASS_DESANN nicht", !f.pruefen);
}

Scene szene(const char* klasse, double todMs, bool feind) {
    Scene s;
    Actor a;
    a.name = "opfer";
    a.npcType = "reborn";
    a.klasse = klasse;
    a.haveStart = true;
    a.getoetetMs = todMs;
    a.feindBeimTod = feind;
    s.actors.push_back(a);
    Actor sp;
    sp.name = "player";
    sp.npcType = "player";
    sp.echterSpieler = true;
    sp.haveStart = true;
    sp.start[0] = 2000.0F;
    s.actors.push_back(sp);
    return s;
}

double weg(const Scene& s) {
    for (const ActorStep& st : s.actors[0].steps) {
        if (st.kind == ActorStep::Kind::Gone && st.leicheWeg) { return st.startMs; }
    }
    return -1.0;
}

void leichen() {
    std::printf("NPC_RemoveBody\n");
    MapData karte;
    LeichenHilfe h;
    h.karte = "t1_fatal";
    h.team = [](const Actor&) { return std::string("TEAM_ENEMY"); };
    {
        Scene s = szene("CLASS_REBORN", 1000.0, false);
        leichenEntfernen(s, karte, h, 60000.0);
        erwarte("kill aus ICARUS (kein self->enemy): die Leiche bleibt", weg(s) < 0.0);
    }
    {
        Scene s = szene("CLASS_REBORN", 1000.0, true);
        leichenEntfernen(s, karte, h, 60000.0);
        char t[80];
        std::snprintf(t, sizeof(t), "mit Feind: g_corpseRemovalTime 10 s, dann FRAMETIME * 4 (%.0f)", weg(s));
        erwarte(t, weg(s) >= 11200.0 && weg(s) <= 11300.0);
    }
    {
        Scene s = szene("CLASS_R2D2", 1000.0, false);
        leichenEntfernen(s, karte, h, 60000.0);
        erwarte("Droide: Pad 0 und EF_NODRAW, auch ohne Feind", weg(s) == 1000.0);
        // Endrunde rc608: DeadThink setzt nur EF_NODRAW (NPC.cpp:653-690 MD) - unsichtbar, aber nicht entfernt (fuer Spuren da).
        const ActorState st = s.actors[0].at(1000.0);
        erwarte("Droide: bei EF_NODRAW unsichtbar, die Entity bleibt", !st.visible && st.nurUnsichtbar && !st.removed);
    }
    {
        Scene s = szene("CLASS_PROBE", 1000.0, false);
        leichenEntfernen(s, karte, h, 60000.0);
        erwarte("CLASS_PROBE: EF_NODRAW, beim naechsten NPC_RemoveBody G_FreeEntity", s.actors[0].at(1050.0).nurUnsichtbar && s.actors[0].at(1100.0).removed);
    }
    {
        Scene s = szene("CLASS_SBD", 1000.0, true);
        LeichenHilfe h2 = h;
        h2.karte = "md_ga_jedi";
        regelwerkAnwenden(true, 1);
        leichenEntfernen(s, karte, h2, 60000.0);
        erwarte("Movie Duels md_ga_jedi: SBD nach 3 s", weg(s) == 4000.0);
        Scene s2 = szene("CLASS_SBD", 1000.0, true);
        regelwerkAnwenden(false, 1);
        leichenEntfernen(s2, karte, h2, 60000.0);
        erwarte("OpenJK: SBD ist kein Droide der Liste, 10 s", weg(s2) >= 11200.0);
        regelwerkAnwenden(true, 1);
    }
    {
        Scene s = szene("CLASS_REBORN", 1000.0, true);
        s.actors[1].start[0] = 50.0F;
        leichenEntfernen(s, karte, h, 60000.0);
        erwarte("naeher als 128 am Spieler: bleibt", weg(s) < 0.0);
    }
    {
        Scene s = szene("CLASS_REBORN", 1000.0, true);
        LeichenHilfe h2 = h;
        h2.blick = [](double, float ort[3], float winkel[3]) {
            ort[0] = 2000.0F;
            ort[1] = 0.0F;
            ort[2] = 36.0F;
            winkel[0] = 0.0F;
            winkel[1] = 180.0F;
            winkel[2] = 0.0F;
            return true;
        };
        leichenEntfernen(s, karte, h2, 60000.0);
        erwarte("im Blick des Spielers (ohne Spur frei): bleibt", weg(s) < 0.0);
        Scene s2 = szene("CLASS_REBORN", 1000.0, true);
        h2.blick = [](double, float ort[3], float winkel[3]) {
            ort[0] = 2000.0F;
            ort[1] = 0.0F;
            ort[2] = 36.0F;
            winkel[0] = 0.0F;
            winkel[1] = 0.0F;
            winkel[2] = 0.0F;
            return true;
        };
        leichenEntfernen(s2, karte, h2, 60000.0);
        erwarte("hinter dem Spieler: weg", weg(s2) > 0.0);
    }
    {
        // Endrunde rc608: NPC_ClearLOS vom Auge des Spielers (CalcEntitySpot SPOT_HEAD_LEAN): eyePoint, sonst Ursprung +
        // ps.viewheight 36. Eine Mauer verdeckt nur, was unter 34 ueber dem Ursprung des Spielers losgeht.
        LeichenHilfe h2 = h;
        h2.blick = [](double, float ort[3], float winkel[3]) {
            ort[0] = 2000.0F;
            ort[1] = 0.0F;
            ort[2] = 36.0F;
            winkel[0] = 0.0F;
            winkel[1] = 180.0F;
            winkel[2] = 0.0F;
            return true;
        };
        h2.spur = [](const NavVec& a, const NavVec&, const NavVec&, const NavVec& e, int, std::uint32_t) {
            NavSpur sp;
            sp.endpos = e;
            sp.fraction = a[2] < 34.0F ? 0.5F : 1.0F;
            return sp;
        };
        Scene s = szene("CLASS_REBORN", 1000.0, true);
        leichenEntfernen(s, karte, h2, 60000.0);
        erwarte("ohne eyePoint: Auge auf ps.viewheight 36, die niedrige Mauer verdeckt nicht, die Leiche bleibt", weg(s) < 0.0);
        h2.augenPunkt = [](const Actor& f, double, float auge[3]) {
            auge[0] = f.start[0];
            auge[1] = f.start[1];
            auge[2] = 30.0F;
            return true;
        };
        Scene s2 = szene("CLASS_REBORN", 1000.0, true);
        leichenEntfernen(s2, karte, h2, 60000.0);
        erwarte("mit eyePoint unter der Mauer: verdeckt, die Leiche geht", weg(s2) > 0.0);
    }
    {
        Scene s = szene("CLASS_REBORN", 1000.0, true);
        ActorStep w;
        w.kind = ActorStep::Kind::Hold;
        w.startMs = 30000.0;
        w.endMs = 30000.0;
        w.path = Path{0U};
        s.actors[0].steps.push_back(w);
        leichenEntfernen(s, karte, h, 60000.0);
        erwarte("solange ihr Skript laeuft: bleibt", weg(s) >= 30000.0);
    }
}

void holster() {
    std::printf("Holster (Movie Duels)\n");
    HolsterLage l;
    erwarte("ein Griff: HOLSTER_LHIPF mit g_CannonHolsteredSabers 1 an *hip_fl",
            saberHolsterLage("reborn", "CLASS_REBORN", "SABER_SINGLE", "", "", 0, l) && std::string(l.bolzen) == "*hip_fl");
    erwarte("Doppelklinge: HOLSTER_RHIPB an *hip_br", saberHolsterLage("x", "CLASS_REBORN", "SABER_STAFF", "", "", 0, l) && std::string(l.bolzen) == "*hip_br");
    erwarte("Maul: an *hip_r", saberHolsterLage("md_maul", "CLASS_REBORN", "SABER_STAFF", "", "", 0, l) && std::string(l.bolzen) == "*hip_r");
    erwarte("an der Seite der Huefte 5 tiefer", std::fabs(l.drehung.m[2][3] + 5.0F) < 1.0e-4F);
    erwarte("zweites Schwert an der anderen Seite", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_LHIP", "HOLSTER_LHIP", 1, l) && std::string(l.bolzen) == "*hip_r");
    erwarte("HOLSTER_BACK an *back", saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_BACK", "HOLSTER_BACK", 0, l) && std::string(l.bolzen) == "*back");
    erwarte("zweites Schwert: nicht auch auf den Ruecken", !saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_BACK", "HOLSTER_BACK", 1, l));
    erwarte("SaberCanNotHolster: Dolch", !saberHolsterLage("x", "CLASS_REBORN", "SABER_DAGGER", "", "", 0, l));
    erwarte("SaberCanNotHolster: CLASS_JANGO", !saberHolsterLage("x", "CLASS_JANGO", "SABER_SINGLE", "", "", 0, l));
    erwarte("HOLSTER_NONE", !saberHolsterLage("x", "CLASS_REBORN", "SABER_SINGLE", "HOLSTER_NONE", "", 0, l));
    erwarte("Sith-Schwert: HOLSTER_LHIP", holsterPlatzVorgabe("SABER_SITH_SWORD") == "HOLSTER_LHIP");
}

}  // namespace

int main() {
    regelwerkAnwenden(true, 1);
    totenlauf();
    fallTod();
    leichen();
    holster();
    if (g_fehler != 0) {
        std::printf("FEHLGESCHLAGEN (%d)\n", g_fehler);
        return 1;
    }
    std::printf("alle Proben bestanden\n");
    return 0;
}
