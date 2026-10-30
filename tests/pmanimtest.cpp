// pmanimtest - die Beine und der Oberkoerper einer gesteuerten Figur gegen die
// Regeln der Engine (Zeitleiste 4 Punkt 3, src/pmanim.cpp):
//   PM_Footsteps (bg_pmove.cpp OpenJK 7931-8724, MD 13921-16377 mit
//   g_ActivateAnimationStyle 0), PmoveSingle (PMF_BACKWARDS_RUN),
//   PM_SetAnimFinal (bg_panimate.cpp OpenJK 4622-5052: Timer, g_noFootSlide,
//   BONE_ANIM_BLEND), CG_PlayerLegsYawFromMovement (cg_players.cpp).
#include "bhed/pmanim.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed;

namespace {

int fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

bool nah(double a, double b, double tol = 1e-3) { return std::fabs(a - b) <= tol; }

PmFigur figur(const char* waffe, bool md, const char* klasse = "") {
    PmFigur f;
    f.waffe = waffe;
    f.md = md;
    f.klasse = klasse;
    return f;
}

std::string beine(const PmFigur& f, int vor, int seit, bool gehen, bool rueck, const std::string& jetzt = "BOTH_STAND1",
                  bool timer = false) {
    PmBeinStand z;
    z.beine = jetzt;
    z.beineTimer = timer;
    return pmFootstepsBeine(f, vor, seit, gehen, rueck, z).anim;
}

}  // namespace

int main() {
    std::printf("PMF_BACKWARDS_RUN\n");
    {
        bool r = false;
        pmRueckwaertsSetzen(r, -127, 0);
        pruefe(r, "S: rueckwaerts");
        pmRueckwaertsSetzen(r, 0, 0);
        pruefe(r, "ohne Eingabe: bleibt");
        pmRueckwaertsSetzen(r, -127, 127);
        pruefe(r, "S+D: rueckwaerts");
        pmRueckwaertsSetzen(r, 0, 127);
        pruefe(!r, "nur D: vorwaerts (forwardmove 0 und rightmove)");
        pmRueckwaertsSetzen(r, 127, 0);
        pruefe(!r, "W: vorwaerts");
    }

    std::printf("PM_Footsteps OpenJK\n");
    {
        const PmFigur ohne = figur("WP_NONE", false);
        pruefe(beine(ohne, 0, 0, false, false) == "BOTH_STAND1", "Stand ohne Waffe: BOTH_STAND1");
        pruefe(beine(ohne, 127, 0, false, false) == "BOTH_RUN1", "W: BOTH_RUN1");
        pruefe(beine(ohne, 127, 0, true, false) == "BOTH_WALK1", "Umschalt+W: BOTH_WALK1");
        pruefe(beine(ohne, 0, 127, false, false) == "BOTH_RUN1", "nur D: BOTH_RUN1 (keine Seitwaertsanimation, die Beine dreht cgame)");
        pruefe(beine(ohne, -127, 0, false, true) == "BOTH_RUNBACK1", "S: BOTH_RUNBACK1");
        pruefe(beine(ohne, -127, 0, true, true) == "BOTH_WALKBACK1", "Umschalt+S: BOTH_WALKBACK1");
        pruefe(beine(figur("WP_NONE", false, "CLASS_IMPERIAL"), 0, 0, false, false) == "BOTH_STAND4", "CLASS_IMPERIAL steht in BOTH_STAND4");
        const PmFigur gewehr = figur("WP_BLASTER", false);
        pruefe(beine(gewehr, 0, 0, false, false) == "BOTH_STAND3", "NPC mit Gewehr: BOTH_STAND3");
        pruefe(beine(gewehr, 0, 0, false, false, "BOTH_GUARD_IDLE1").empty(), "die Wachhaltung bleibt");
        PmFigur klinge = figur("WP_SABER", false);
        klinge.klingeAn = true;
        klinge.klingeLaenge = true;
        klinge.saberStil = 1;
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_SABERFAST_STANCE", "Klinge SS_FAST: BOTH_SABERFAST_STANCE");
        klinge.saberStil = 4;
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_STAND2", "Klinge SS_DESANN: BOTH_STAND2");
        pruefe(beine(klinge, 127, 0, false, false) == "BOTH_RUN2", "Klinge, W: BOTH_RUN2");
        pruefe(beine(klinge, -127, 0, false, true) == "BOTH_RUNBACK2", "Klinge, S: BOTH_RUNBACK2");
        klinge.saberStil = 7;
        pruefe(beine(klinge, 127, 0, true, false) == "BOTH_WALK_STAFF", "Stab, Umschalt+W: BOTH_WALK_STAFF");
        pruefe(beine(klinge, 0, 0, false, false, "BOTH_STAND1TO2") == "BOTH_STAND1", "beim Ziehen keine Kampfhaltung");
        const PmFigur atst = figur("WP_ATST_MAIN", false, "CLASS_ATST");
        pruefe(beine(atst, 127, 0, false, false) == "BOTH_RUN1START", "ATST: erst BOTH_RUN1START");
        pruefe(beine(atst, 127, 0, false, false, "BOTH_RUN1START", true).empty(), "ATST: der Anlauf haelt");
        pruefe(beine(atst, 127, 0, false, false, "BOTH_RUN1START", false) == "BOTH_RUN1", "ATST: danach BOTH_RUN1");
        PmBeinStand z;
        z.beine = "BOTH_STAND1";
        pruefe(pmFootstepsBeine(atst, 127, 0, false, false, z).halten, "ATST: BOTH_RUN1START mit SETANIM_FLAG_HOLD");
        PmFigur wampa = figur("WP_NONE", false, "CLASS_WAMPA");
        wampa.laufTempo = 300.0F;
        pruefe(beine(wampa, 127, 0, false, false) == "BOTH_RUN1", "Wampa mit runSpeed 300: auf allen vieren");
        wampa.laufTempo = 200.0F;
        pruefe(beine(wampa, 127, 0, false, false) == "BOTH_RUN2", "Wampa sonst: BOTH_RUN2");
    }

    std::printf("PM_Footsteps Movie Duels\n");
    {
        const PmFigur gewehr = figur("WP_BLASTER", true);
        pruefe(beine(gewehr, 127, 0, false, false) == "BOTH_JOG_BLASTER", "Gewehr, W: BOTH_JOG_BLASTER");
        pruefe(beine(gewehr, 127, 0, true, false) == "BOTH_WALK_BLASTER", "Gewehr, Umschalt+W: BOTH_WALK_BLASTER");
        pruefe(beine(gewehr, -127, 0, false, true) == "BOTH_RUNBACK_BLASTER", "Gewehr, S: BOTH_RUNBACK_BLASTER");
        pruefe(beine(figur("WP_BLASTER", true, "CLASS_HAZARD_TROOPER"), 127, 0, false, false) == "BOTH_RUN1",
               "Hazard Trooper mit Gewehr: BOTH_RUN1");
        PmFigur zwei = figur("WP_BLASTER_PISTOL", true);
        zwei.zweiPistolen = true;
        pruefe(beine(zwei, 127, 0, false, false) == "BOTH_JOG_DOUBLE_PISTOL", "zwei Pistolen: BOTH_JOG_DOUBLE_PISTOL");
        pruefe(beine(zwei, -127, 0, true, true) == "BOTH_WALKBACK_DUALPISTOL", "zwei Pistolen rueckwaerts: BOTH_WALKBACK_DUALPISTOL");
        pruefe(beine(figur("WP_THERMAL", true), 127, 0, false, false) == "BOTH_JOG_GRENADE", "Granate: BOTH_JOG_GRENADE");
        pruefe(beine(figur("WP_MELEE", true), 0, 0, false, false) == "BOTH_MD_CIN_1", "WP_MELEE steht in BOTH_MD_CIN_1");
        pruefe(beine(figur("WP_MELEE", true, "CLASS_VADER"), 127, 0, true, false) == "BOTH_WALK1_VADER", "Vader geht BOTH_WALK1_VADER");
        PmFigur klinge = figur("WP_SABER", true);
        klinge.klingeAn = true;
        klinge.klingeLaenge = true;
        klinge.saberStil = 4;
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_SABERDESANN_STANCE", "Klinge SS_DESANN: BOTH_SABERDESANN_STANCE");
        pruefe(beine(klinge, -127, 0, false, true) == "BOTH_RUNBACK1", "NPC mit Klinge, S: BOTH_RUNBACK1");
        pruefe(beine(klinge, 127, 0, true, false) == "BOTH_WALK1", "NPC mit Klinge, Umschalt+W: BOTH_WALK1");
        klinge.klingeAn = false;
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_STAND1", "Klinge faehrt ein (g_noIgniteTwirl 1): BOTH_STAND1");
        pruefe(beine(klinge, 127, 0, false, false) == "BOTH_RUN1", "Klinge aus, W: BOTH_RUN1 (PM_GetRunAnim)");
    }

    std::printf("g_noFootSlide\n");
    {
        const PmFigur f = figur("WP_NONE", false);
        pruefe(nah(pmFussTempo("BOTH_RUN1", 300.0F, f, 34.0, false), 1.5 * 34.0 / 50.0), "BOTH_RUN1 mit 300: gekappt bei 1,5");
        pruefe(nah(pmFussTempo("BOTH_WALK1", 45.0F, f, 50.0, false), 0.9), "BOTH_WALK1 mit 45: 45/50");
        pruefe(nah(pmFussTempo("BOTH_STAND1", 45.0F, f, 50.0, false), 1.0), "Stand: das Tempo der .cfg");
        PmFigur stab = f;
        stab.saberStil = 7;
        pruefe(nah(pmFussTempo("BOTH_WALK1", 50.0F, stab, 50.0, false), 0.5), "Stab geht: moveSpeedOfAnim 100");
        pruefe(nah(pmFussTempo("BOTH_RUN1", 0.0F, f, 50.0, false), 0.01), "steht trotz Laufanimation: 0,01");
        PmFigur md = f;
        md.md = true;
        pruefe(nah(pmFussTempo("BOTH_RUN1", 0.0F, md, 50.0, true), 1.0), "Movie Duels in der Kamerafahrt unter 1: ungeskaliert");
        pruefe(nah(pmFussTempo("BOTH_RUN1", 75.0F, figur("WP_NONE", false, "CLASS_WAMPA"), 50.0, false), 1.0), "Wampa: ungeskaliert");
        pruefe(pmLaufAnim("BOTH_JOG_BLASTER", true) && !pmLaufAnim("BOTH_JOG_BLASTER", false), "BOTH_JOG_BLASTER laeuft nur in Movie Duels");
    }

    std::printf("PM_SetAnimFinal\n");
    {
        PmKnochen k;
        k.anim = "BOTH_STAND1";
        k.startMs = -400.0;
        pruefe(pmSetAnimNormal(k, "BOTH_RUN1", 1.0F, 100.0), "neue Animation gesetzt");
        pruefe(k.vorher == "BOTH_STAND1" && nah(k.vorherStelleMs, 500.0) && nah(k.blendAbMs, 100.0) && nah(k.blendMs, 100.0),
               "Uebergang von BOTH_STAND1, eingefroren bei 500 ms, 100 ms lang");
        pruefe(!pmSetAnimNormal(k, "BOTH_RUN1", 1.0F, 150.0), "dieselbe mit demselben Tempo: nichts");
        pruefe(pmSetAnimNormal(k, "BOTH_RUN1", 0.5F, 300.0) && nah(pmStelle(k, 300.0), 200.0) && k.vorher == "BOTH_STAND1" &&
                   nah(k.blendAbMs, 100.0),
               "anderes Tempo: weiter an der Stelle 200, kein neuer Uebergang");
        k.timerBisMs = 1000.0;
        pruefe(!pmSetAnimNormal(k, "BOTH_STAND1", 1.0F, 500.0), "Timer laeuft: PM_Footsteps setzt nichts");
        pruefe(pmSetAnimNormal(k, "BOTH_STAND1", 1.0F, 1000.0), "Timer abgelaufen: gesetzt");
        k.timerBisMs = kPmNie;
        pruefe(!pmSetAnimNormal(k, "BOTH_RUN1", 1.0F, 1.0e9), "SET_ANIM_HOLDTIME -1: nie");
        PmKnochen h;
        pruefe(pmSetAnimNormal(h, "BOTH_RUN1START", 1.0F, 0.0, kPmBlendVorgabe, 650.0) && nah(h.timerBisMs, 650.0) && h.vorher.empty(),
               "mit HOLD: der Timer laeuft animHoldMSec, ohne vorige Animation kein Uebergang");
    }

    std::printf("Beine zur Laufrichtung (cgame)\n");
    {
        float g = 0.0F;
        g = pmBeinGier(g, 0.0F, 0.0F, 300.0F, false, 1.0);
        pruefe(nah(g, 5.0F), "nach links laufen: 5 Grad je Bild");
        for (int i = 0; i < 30; ++i) { g = pmBeinGier(g, 0.0F, 0.0F, 300.0F, false, 1.0); }
        pruefe(nah(g, 60.0F), "hoechstens 60 Grad");
        g = pmBeinGier(g, 0.0F, 0.0F, 0.0F, false, 1.0);
        pruefe(nah(g, 50.0F), "steht: zurueck mit 10 Grad je Bild");
        float r = 0.0F;
        for (int i = 0; i < 30; ++i) { r = pmBeinGier(r, 0.0F, -300.0F, 300.0F, true, 1.0); }
        pruefe(nah(r, -60.0F), "rueckwaerts nach links hinten: gespiegelt, -60");
        float gerade = 10.0F;
        gerade = pmBeinGier(gerade, 0.0F, -300.0F, 0.0F, true, 1.0);
        pruefe(nah(gerade, 0.0F), "gerade rueckwaerts (180 Grad, ueber 150): zur Blickrichtung, 10 Grad je Bild");
        float halb = 0.0F;
        halb = pmBeinGier(halb, 0.0F, 0.0F, 300.0F, false, 0.5);
        pruefe(nah(halb, 5.0F), "frameInterpolation 0,5: 30, gekappt auf 5");
        float teil = 58.0F;
        teil = pmBeinGier(teil, 0.0F, 0.0F, 300.0F, false, 0.25);
        pruefe(nah(teil, 58.5F), "Rest 2 Grad, frameInterpolation 0,25: 0,5");
        pruefe(nah(kPmRumpfAnteil[0] + kPmRumpfAnteil[1] + kPmRumpfAnteil[2], 1.0), "der Rumpf: Anteile zusammen 1");
    }

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
