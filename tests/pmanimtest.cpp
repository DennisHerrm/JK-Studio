// pmanimtest - die Beine und der Oberkoerper einer gesteuerten Figur gegen die
// Regeln der Engine (Zeitleiste 4 Punkt 3, src/pmanim.cpp):
//   PM_Footsteps (bg_pmove.cpp OpenJK 7931-8724, MD 13921-16377 mit
//   g_ActivateAnimationStyle 0), PmoveSingle (PMF_BACKWARDS_RUN),
//   PM_SetAnimFinal (bg_panimate.cpp OpenJK 4622-5052: Timer, g_noFootSlide,
//   BONE_ANIM_BLEND), CG_PlayerLegsYawFromMovement (cg_players.cpp).
#include "bhed/pmanim.h"
#include "bhed/scene.h"   // parseSaberFile, parseNpcFile

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
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_SABERDESANN_STANCE_JKA",
               "Klinge SS_DESANN: BOTH_SABERDESANN_STANCE_JKA (g_SerenityJediEngineMode 0)");
        pruefe(beine(klinge, -127, 0, false, true) == "BOTH_RUNBACK1", "NPC mit Klinge, S: BOTH_RUNBACK1");
        pruefe(beine(klinge, 127, 0, true, false) == "BOTH_WALK1", "NPC mit Klinge, Umschalt+W: BOTH_WALK1");
        klinge.saberStil = 7;
        pruefe(beine(klinge, -127, 0, true, true) == "BOTH_WALKBACK_STAFF", "Stab, Umschalt+S: BOTH_WALKBACK_STAFF (Modus 0)");
        klinge.saberStil = 4;
        klinge.klingeAn = false;
        pruefe(beine(klinge, 0, 0, false, false) == "BOTH_STAND1", "Klinge faehrt ein (g_noIgniteTwirl 1): BOTH_STAND1");
        pruefe(beine(klinge, 127, 0, false, false) == "BOTH_RUN1", "Klinge aus, W: BOTH_RUN1 (PM_GetRunAnim)");
    }

    // Zeitleiste 5: PM_AdjustStandAnimForSlope ruft PM_Footsteps nur im Stand und
    // dort nur in diesen Zweigen (bg_pmove.cpp:8165-8330 OJ, 14246-14540 MD).
    // Zeitleiste 5/6: PM_TorsoAnimLightsaber (bg_panimate.cpp:5160-5505 OJ, :9205 MD) und
    // PM_SetSaberMove(LS_READY) (bg_pmove.cpp OJ, :22582 MD) - Erwartungen aus dem Quelltext.
    std::printf("PM_TorsoAnimLightsaber / PM_SetSaberMove(LS_READY)\n");
    {
        PmFigur oj = figur("WP_SABER", false);
        oj.klingeAn = true;
        oj.klingeLaenge = true;
        oj.saberStil = 1;
        PmTorsoLage still;
        PmTorsoLage geht;
        geht.eingabe = true;
        pruefe(pmTorsoLichtschwert(oj, "BOTH_RUN2", geht).folgen, "OJ laeuft (PM_RunningAnim): der Oberkoerper folgt");
        pruefe(pmTorsoLichtschwert(oj, "BOTH_WALK_STAFF", geht).folgen, "OJ Stab geht: der Oberkoerper folgt");
        pruefe(pmTorsoLichtschwert(oj, "BOTH_JUMP1", still).folgen, "OJ springt (PM_JumpingAnim): folgt");
        PmTorsoWahl t = pmTorsoLichtschwert(oj, "BOTH_SABERFAST_STANCE", still);
        pruefe(!t.folgen && t.anim == "BOTH_SABERFAST_STANCE" && t.blendMs == 350.0 && t.beide,
               "OJ steht ohne Eingabe: LS_READY, Kampfhaltung, 350 ms, SETANIM_BOTH");
        t = pmTorsoLichtschwert(oj, "BOTH_SABERFAST_STANCE", geht);
        pruefe(t.anim == "BOTH_SABERFAST_STANCE" && !t.beide, "mit Eingabe nur SETANIM_TORSO");
        t = pmTorsoLichtschwert(oj, "LEGS_S1_LUP1", still);
        pruefe(!t.beide, "am Hang (PM_InSlopeAnim) nur SETANIM_TORSO");
        PmTorsoLage duck;
        duck.geduckt = true;
        pruefe(!pmTorsoLichtschwert(oj, "BOTH_CROUCH1IDLE", duck).beide, "geduckt nur SETANIM_TORSO");
        t = pmTorsoLichtschwert(oj, "BOTH_WALK1", geht);
        pruefe(t.anim == "BOTH_WALK1", "OJ geht (SS_FAST): \"match torso anim to walk/run anim\" - BOTH_WALK1");
        PmFigur f = oj;
        f.bereitAnim[0] = "BOTH_STAND5";
        pruefe(pmTorsoLichtschwert(f, "BOTH_WALK1", geht).anim == "BOTH_STAND5", "readyAnim der .sab, beim Gehen ohne Angleich");
        f = oj;
        f.zweiSchwerter = true;
        f.bereitAnim[1] = "BOTH_STAND6";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_STAND6", "readyAnim des zweiten Schwerts");
        f = oj;
        f.saberTyp = "SABER_ARC";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERFAST_STANCE", "SABER_ARC: BOTH_SABERFAST_STANCE");
        f = oj;
        f.zweiSchwerter = true;
        f.zweitesAn = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERDUAL_STANCE", "zwei Schwerter an: BOTH_SABERDUAL_STANCE");
        f = oj;
        f.saberTyp = "SABER_STAFF";
        f.stab = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERSTAFF_STANCE", "Stab: BOTH_SABERSTAFF_STANCE");
        pruefe(pmTorsoLichtschwert(f, "BOTH_WALK1", geht).anim == "BOTH_SABERSTAFF_STANCE", "Stab: kein Angleich beim Gehen");
        f.eineKlingeStil = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERFAST_STANCE",
               "Stab mit singleBladeStyle, nur die erste Klinge: die Haltung des Stils");
        f.klinge2An = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERSTAFF_STANCE", "... mit zweiter Klinge wieder der Stab");
        f = oj;
        f.saberTyp = "SABER_LANCE";
        pruefe(pmTorsoLichtschwert(f, "BOTH_WALK1", geht).anim == "BOTH_STAND1", "SABER_LANCE: BOTH_STAND1, kein Angleich");
        f.saberTyp = "SABER_TRIDENT";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_STAND1", "SABER_TRIDENT: BOTH_STAND1");
        f = oj;
        f.blocken = true;
        t = pmTorsoLichtschwert(f, "BOTH_RUN1", geht);
        pruefe(!t.folgen && t.anim == "BOTH_P1_S1_T_" && t.beide, "Blocken: laeuft trotzdem LS_READY, BOTH_P1_S1_T_ ueber beide");
        f.saberTyp = "SABER_CLAW";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", geht).anim == "BOTH_INAIR1", "Blocken mit Klaue: BOTH_INAIR1");
        f = oj;
        f.bereitAnim[0] = "BOTH_T1_BR_BL";
        pruefe(pmTorsoLichtschwert(f, "BOTH_WALK1", geht).beide, "PM_SpinningSaberAnim: ueber beide");

        PmFigur md = oj;
        md.md = true;
        t = pmTorsoLichtschwert(md, "BOTH_SABERFAST_STANCE_JKA", still);
        pruefe(t.anim == "BOTH_SABERFAST_STANCE_JKA" && t.beide && t.blendMs == 350.0, "MD steht (SS_FAST): JKA-Haltung ueber beide");
        pruefe(pmTorsoLichtschwert(md, "BOTH_WALK1", geht).folgen, "MD geht (BOTH_WALK1 in der Liste :9244): der Oberkoerper folgt");
        t = pmTorsoLichtschwert(md, "BOTH_SABERSINGLECROUCH", geht);
        pruefe(t.anim == "BOTH_SABERSINGLECROUCH" && !t.beide, "MD: Angleich an die Beine (BOTH_SABERSINGLECROUCH in der MD-Liste)");
        PmTorsoLage mdNull = geht;
        mdNull.tempoNull = true;
        pruefe(pmTorsoLichtschwert(md, "BOTH_SABERFAST_STANCE_JKA", mdNull).beide, "MD: ps->speed 0 wie ohne Eingabe");
        f = md;
        f.blocken = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", geht).anim == "BOTH_STAND2", "MD blockt (ein Schwert): animToUse BOTH_STAND2");
        f.saberTyp = "SABER_ARC";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", geht).anim == "BOTH_SABERFAST_STANCE_JKA", "MD blockt mit SABER_ARC: die Haltung");
        f = md;
        f.saberTyp = "SABER_LANCE";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERFAST_STANCE_JKA", "MD SABER_LANCE: die Haltung (nicht STAND1)");
        f = md;
        f.saberStil = 6;
        f.animStil = "CS_GALEN_MAREK";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERDUAL_STANCE_JKA", "MD SS_DUAL ohne g_ActivateAnimationStyle");
        f.animStilAn = true;
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERDUAL_STANCE_JKA_GALEN", "... mit 1: Galen Marek");
        f.saberStil = 7;
        f.animStil = "CS_GRIEVOUS";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERSTAFF_STANCE_JKA", "SS_STAFF kennt Grievous nicht");
        f.animStil = "CS_BATTLEDROID";
        pruefe(pmTorsoLichtschwert(f, "BOTH_STAND1", still).anim == "BOTH_SABERSTAFF_STANCE_JKA_BDROID", "SS_STAFF Kampfdroide");
        f.saberStil = 1;
        f.animStil = "CS_VADER";
        t = pmTorsoLichtschwert(f, "BOTH_CROUCH1IDLE", duck);
        pruefe(t.anim == "BOTH_SABERSINGLECROUCH_VADER" && !t.beide, "MD geduckt (_Ducked): Vader, nur der Oberkoerper");
        f.animStilAn = false;
        pruefe(pmTorsoLichtschwert(f, "BOTH_CROUCH1IDLE", duck).anim == "BOTH_SABERSINGLECROUCH", "... ohne Stil: BOTH_SABERSINGLECROUCH");
        pruefe(pmTorsoLichtschwert(md, "BOTH_WALKBACK_STAFF_MAUL", geht).folgen, "MD: Maul-Stab rueckwaerts gehend folgt");
        pruefe(!pmTorsoLichtschwert(oj, "BOTH_WALKBACK_STAFF_MAUL", geht).folgen, "OpenJK kennt die Maul-Animation nicht");
        pruefe(pmTorsoLichtschwert(md, "BOTH_T5_BL_TR_YODA", geht).beide && !pmTorsoLichtschwert(oj, "BOTH_T5_BL_TR_YODA", geht).beide,
               "MD: Drehschwung der Beine (nur MD-Liste) ueber beide");
    }

    std::printf(".sab readyAnim und NPC animationstyle\n");
    {
        SaberMap s;
        parseSaberFile("zl6_test\n{\nname \"t\"\nsaberType SABER_ARC\nreadyAnim both_stand5\n}\n", s);
        const auto it = s.find("zl6_test");
        pruefe(it != s.end() && it->second.readyAnim == "BOTH_STAND5" && it->second.type == "SABER_ARC", "readyAnim gross, saberType");
        NpcMap n;
        parseNpcFile("zl6_npc\n{\nplayerModel kyle\nanimationstyle galen_marek\n}\nzl6_npc2\n{\nplayerModel kyle\nanimationstyle CS_MAUL\n}\n", n);
        pruefe(n.count("zl6_npc") != 0 && n["zl6_npc"].animStil == "CS_GALEN_MAREK" && n["zl6_npc2"].animStil == "CS_MAUL",
               "animationstyle: Kurzname und CS_-Name");
    }

    std::printf("PM_AdjustStandAnimForSlope im Stand\n");
    {
        int rufe = 0;
        const auto hangBeine = [&rufe](const PmFigur& f, int vor, const std::string& jetzt, const char* hang) {
            PmBeinStand z;
            z.beine = jetzt;
            z.hang = [&rufe, hang] {
                ++rufe;
                return std::string(hang);
            };
            return pmFootstepsBeine(f, vor, 0, false, false, z).anim;
        };
        pruefe(hangBeine(figur("WP_NONE", false), 0, "BOTH_STAND1", "LEGS_S1_LUP1") == "LEGS_S1_LUP1", "ohne Waffe am Hang: LEGS_S1_LUP1");
        pruefe(hangBeine(figur("WP_BLASTER", false), 0, "BOTH_STAND3", "LEGS_S3_RUP2") == "LEGS_S3_RUP2", "mit Gewehr am Hang");
        pruefe(hangBeine(figur("WP_BLASTER", false), 0, "BOTH_STAND3", "") == "BOTH_STAND3", "eben (qfalse): BOTH_STAND3");
        PmFigur klinge = figur("WP_SABER", false);
        klinge.klingeAn = true;
        klinge.klingeLaenge = true;
        pruefe(hangBeine(klinge, 0, "BOTH_STAND2", "LEGS_LEFTUP1") == "LEGS_LEFTUP1", "mit Klinge am Hang");
        pruefe(hangBeine(figur("WP_ATST_MAIN", false, "CLASS_ATST"), 0, "BOTH_STAND1", "LEGS_LEFTUP3") == "LEGS_LEFTUP3", "ATST am Hang");
        pruefe(hangBeine(figur("WP_NONE", false, "CLASS_IMPERIAL"), 0, "BOTH_STAND4", "LEGS_S4_LUP1") == "LEGS_S4_LUP1", "CLASS_IMPERIAL am Hang");
        const int vorher = rufe;
        pruefe(hangBeine(figur("WP_BLASTER", false), 0, "BOTH_GUARD_IDLE1", "LEGS_S3_LUP1").empty(), "Wachhaltung: nichts gesetzt");
        pruefe(hangBeine(figur("WP_NONE", false, "CLASS_GALAKMECH"), 0, "BOTH_STAND1", "LEGS_S1_LUP1") == "BOTH_STAND2", "Galak: BOTH_STAND2");
        pruefe(hangBeine(figur("WP_NONE", false), 127, "BOTH_STAND1", "LEGS_S1_LUP1") == "BOTH_RUN1", "in Bewegung: BOTH_RUN1");
        pruefe(rufe == vorher, "Wachhaltung, Galak und Bewegung rufen PM_AdjustStandAnimForSlope nicht (slopeRecalcTime bleibt)");
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
        // F4b (B4b-15): PM_SetAnimFinal (bg_panimate.cpp:4719-4760 OpenJK; MD "animSpeed >= 0.0f") passt nur bei
        // !(animSpeed < 0) an. BOTH_WALKBACK_STAFF und BOTH_RUNBACK_DUAL haben in _humanoid/animation.cfg -20 fps
        // (assets1.pk3 und MD_Assets2.pk3): frameLerp floor(1000 / -20) = -50, sie spielen mit dem Tempo der .cfg.
        PmFigur dual = f;
        dual.saberStil = 6;
        pruefe(nah(pmFussTempo("BOTH_WALKBACK_STAFF", 64.0F, stab, -50.0, false), 1.0),
               "BOTH_WALKBACK_STAFF (-20 fps) rueckwaerts mit 64: ungeskaliert");
        // BOTH_RUNBACK_DUAL steht nicht in PM_RunningAnim (bg_panimate.cpp OpenJK): ungeskaliert, auch ohne die -20 fps.
        pruefe(nah(pmFussTempo("BOTH_RUNBACK_DUAL", 100.0F, dual, -50.0, false), 1.0),
               "BOTH_RUNBACK_DUAL (-20 fps, keine Laufanimation) rueckwaerts mit 100: ungeskaliert");
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
    }

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
