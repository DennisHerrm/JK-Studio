// pmanim.cpp - siehe bhed/pmanim.h
#include "bhed/pmanim.h"
#include "bhed/scene.h"   // saberHaltungFuerStil: die Kampfhaltung

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace bhed {
namespace {

bool einer(const std::string& a, std::initializer_list<const char*> namen) {
    for (const char* n : namen) {
        if (a == n) { return true; }
    }
    return false;
}

// AngleNormalize180 / AngleDelta (q_math.cpp).
float winkel180(float a) {
    a = std::fmod(a, 360.0F);
    if (a < 0.0F) { a += 360.0F; }
    return a > 180.0F ? a - 360.0F : a;
}
float winkelDelta(float a, float b) { return winkel180(a - b); }

// Waffengruppen, wie PM_Footsteps sie in Movie Duels abfragt (bg_pmove.cpp
// 13921 ff. mdgit, mit g_ActivateAnimationStyle 0).
bool mdPistole(const std::string& w, bool laufen) {
    return einer(w, {"WP_BLASTER_PISTOL", "WP_BRYAR_PISTOL", "WP_DUAL_PISTOL", "WP_DUAL_CLONEPISTOL", "WP_REY", "WP_CLONEPISTOL",
                     "WP_REBELBLASTER", "WP_JANGO", "WP_STUN_BATON"}) ||
           (laufen && w == "WP_SBD_BLASTER");
}
bool mdGewehr(const std::string& w) {
    return einer(w, {"WP_DISRUPTOR", "WP_BLASTER", "WP_BATTLEDROID", "WP_THEFIRSTORDER", "WP_CLONECARBINE", "WP_CLONERIFLE",
                     "WP_CLONECOMMANDO", "WP_BOBA", "WP_REBELRIFLE", "WP_REPEATER"});
}
bool mdSchwer(const std::string& w) {
    return einer(w, {"WP_CONCUSSION", "WP_ROCKET_LAUNCHER", "WP_BOWCASTER", "WP_FLECHETTE", "WP_DEMP2", "WP_Z6_ROTARY_CANNON"});
}
bool mdGranate(const std::string& w) { return einer(w, {"WP_THERMAL", "WP_DET_PACK", "WP_TRIP_MINE"}); }

// PM_SaberDrawPutawayAnim (bg_pmove.cpp OpenJK; MD mit den Figurenvarianten).
bool ziehtSchwert(const std::string& a, bool md) {
    if (einer(a, {"BOTH_STAND1TO2", "BOTH_STAND2TO1", "BOTH_S1_S7", "BOTH_S7_S1", "BOTH_S1_S6", "BOTH_S6_S1"})) { return true; }
    return md && einer(a, {"BOTH_DOOKU_FULLDRAW", "BOTH_DOOKU_SMALLDRAW", "BOTH_GRIEVOUS_SABERON", "BOTH_S1_S6_GALEN",
                           "BOTH_S1_S6_GRIEV", "BOTH_S1_S6_MAUL", "BOTH_S1_S6_PAL", "BOTH_S1_S7_AMD", "BOTH_S1_S7_CAL",
                           "BOTH_S1_S7_GALEN", "BOTH_S1_S7_GRIEV", "BOTH_S1_S7_MAUL", "BOTH_S6_S1_GALEN", "BOTH_S6_S1_MAUL",
                           "BOTH_S6_S1_PAL", "BOTH_S7_S1_GALEN", "BOTH_S7_S1_MAUL", "BOTH_SABER_BACKHAND_IGNITION",
                           "BOTH_SABER_IGNITION", "BOTH_SABER_IGNITION_JFA", "BOTH_STAND1TO2_CAL", "BOTH_STAND1TO2_DOOKU",
                           "BOTH_STAND1TO2_GALEN", "BOTH_STAND1TO2_GRIEV", "BOTH_STAND1TO2_MAUL", "BOTH_STAND1TO2_PAL",
                           "BOTH_STAND1TO2_REN", "BOTH_STAND1TO2_VADER", "BOTH_STAND1TO2_YODA", "BOTH_STAND2TO1_CAL",
                           "BOTH_STAND2TO1_DOOKU", "BOTH_STAND2TO1_GALEN", "BOTH_STAND2TO1_MAUL", "BOTH_STAND2TO1_PAL",
                           "BOTH_STAND2TO1_REN", "BOTH_STAND2TO1_VADER", "BOTH_STAND2TO1_YODA"});
}

// ps.weapon > WP_SABER && ps.weapon < WP_DET_PACK (weapons.h): die Folge
// WP_BLASTER_PISTOL .. WP_TRIP_MINE; Movie Duels haengt seine Waffen hinter
// WP_DET_PACK an.
bool waffeVorSprengsatz(const std::string& w) {
    return einer(w, {"WP_BLASTER_PISTOL", "WP_BLASTER", "WP_DISRUPTOR", "WP_BOWCASTER", "WP_REPEATER", "WP_DEMP2", "WP_FLECHETTE",
                     "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE"});
}

PmBeinWahl beine(const char* a) { return PmBeinWahl{a, false, false}; }

// Anlauf (ATST, Movie Duels auch Droideka): erst ziel_START mit HOLD, wenn
// dessen Timer abgelaufen ist das Ziel (PM_Footsteps, "he should really start
// slow and have to accelerate").
PmBeinWahl anlauf(const PmBeinStand& z, const char* ziel) {
    if (z.beine == ziel) { return beine(ziel); }
    if (z.beine != "BOTH_RUN1START") { return PmBeinWahl{"BOTH_RUN1START", false, true}; }
    if (!z.beineTimer) { return beine(ziel); }
    return PmBeinWahl{};
}
// Auslauf (Movie Duels, ATST und Droideka): aus dem Lauf erst BOTH_RUN1STOP mit HOLD.
PmBeinWahl auslauf(const PmBeinStand& z, const char* ziel, const char* lauf) {
    if (z.beine == ziel) { return beine(ziel); }
    if (z.beine != "BOTH_RUN1STOP" && z.beine == lauf) { return PmBeinWahl{"BOTH_RUN1STOP", false, true}; }
    if (!z.beineTimer) { return beine(ziel); }
    return PmBeinWahl{};
}

// --- Im Stand -------------------------------------------------------------
// Wo PM_Footsteps im Stand "if ( !PM_AdjustStandAnimForSlope() )" schreibt:
// am Hang dessen Beinanimation (leer: qfalse).
std::string hangJetzt(const PmBeinStand& z) { return z.hang ? z.hang() : std::string(); }
PmBeinWahl standOderHang(const PmBeinStand& z, const char* stand) {
    std::string h = hangJetzt(z);
    return h.empty() ? beine(stand) : PmBeinWahl{std::move(h), false, false};
}

PmBeinWahl imStand(const PmFigur& f, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    const std::string& beineJetzt = z.beine;
    if (k == "CLASS_ATST" || (f.md && k == "CLASS_DROIDEKA")) { return standOderHang(z, "BOTH_STAND1"); }
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{"BOTH_STAND2", true, false}; }
    // Mit Klinge (saberInFlight gibt es beim Steuern nicht). Movie Duels:
    // SaberActive() || (!g_noIgniteTwirl && ...) - g_noIgniteTwirl ist 1.
    if (w == "WP_SABER" && f.klingeLaenge && (!f.md || f.klingeAn) && !ziehtSchwert(beineJetzt, f.md)) {
        return standOderHang(z, saberHaltungFuerStil(f.saberStil, f.md));
    }
    if (waffeVorSprengsatz(w)) {
        // "Don't auto-override the guard idles"
        if (einer(beineJetzt, {"BOTH_GUARD_LOOKAROUND1", "BOTH_GUARD_IDLE1", "BOTH_STAND2TO4", "BOTH_STAND4TO2", "BOTH_STAND4"}) ||
            (f.md && einer(beineJetzt, {"BOTH_GUARD_IDLE1_BDROID", "BOTH_GUARD_IDLE1_JANGO"}))) {
            return PmBeinWahl{};
        }
        return standOderHang(z, "BOTH_STAND3");
    }
    // sonst: erst der Hang, dann die Grundhaltung nach Klasse und Waffe
    if (std::string h = hangJetzt(z); !h.empty()) { return PmBeinWahl{std::move(h), false, false}; }
    if (k == "CLASS_IMPERIAL") { return beine("BOTH_STAND4"); }
    if (w == "WP_TUSKEN_STAFF") { return beine("BOTH_STAND1"); }
    // CLASS_RANCOR (count, enemy, wait) und CLASS_WAMPA (count): ohne Opfer und
    // Feind die Grundhaltung.
    if (k == "CLASS_RANCOR" || k == "CLASS_WAMPA") { return beine("BOTH_STAND1"); }
    if (f.md && k == "CLASS_YODA") { return beine(w == "WP_MELEE" ? "BOTH_STANDYODA_STICK" : "BOTH_STAND1"); }
    if (f.md && w == "WP_MELEE") { return beine("BOTH_MD_CIN_1"); }
    return beine("BOTH_STAND1");
}

// --- Rueckwaerts (PMF_BACKWARDS_RUN) --------------------------------------
PmBeinWahl rueckwaertsOj(const PmFigur& f, bool gehen) {
    const bool klinge = f.waffe == "WP_SABER" && f.klingeAn;
    if (!gehen) {
        if (klinge) { return beine("BOTH_RUNBACK2"); }
        if (f.klasse == "CLASS_RANCOR") { return beine("BOTH_WALKBACK1"); }
        return beine("BOTH_RUNBACK1");
    }
    if (klinge) {
        if (f.saberStil == 6) { return beine("BOTH_WALKBACK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALKBACK_STAFF"); }
        return beine("BOTH_WALKBACK2");
    }
    return beine("BOTH_WALKBACK1");
}

PmBeinWahl rueckwaertsMd(const PmFigur& f, bool gehen, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    if (!gehen) {
        // Ein NPC (nicht vom Spieler gesteuert) mit Klinge: BOTH_RUNBACK1.
        if (w == "WP_SABER" && f.klingeAn) { return beine("BOTH_RUNBACK1"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALKBACK1"); }
        if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return anlauf(z, "BOTH_RUNBACK1"); }
        if (mdPistole(w, false)) {
            if ((k == "CLASS_REBORN" && w == "WP_BLASTER_PISTOL") || f.zweiPistolen ||
                (k == "CLASS_CALONORD" && w == "WP_REBELBLASTER")) {
                return beine("BOTH_RUNBACK_DUALPISTOL");
            }
            return beine("BOTH_RUNBACK_PISTOL");
        }
        if (mdGewehr(w)) { return beine("BOTH_RUNBACK_BLASTER"); }
        if (mdSchwer(w)) { return beine("BOTH_RUNBACK_HEAVY"); }
        if (mdGranate(w)) { return beine("BOTH_RUNBACK_GRENADE"); }
        return beine("BOTH_RUNBACK1");
    }
    if (w == "WP_SABER" && f.klingeAn) {
        if (ziehtSchwert(z.torso, true)) { return beine("BOTH_WALKBACK1"); }
        if (f.saberStil == 6) { return beine("BOTH_WALKBACK_DUAL"); }
        // SS_STAFF mit g_SerenityJediEngineMode 0 (wirksam, siehe scene.h): BOTH_WALKBACK_STAFF.
        if (f.saberStil == 7) { return beine("BOTH_WALKBACK_STAFF"); }
        return beine("BOTH_WALKBACK2");
    }
    if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return auslauf(z, "BOTH_WALKBACK1", "BOTH_RUNBACK1"); }
    if (mdPistole(w, false)) {
        if ((k == "CLASS_REBORN" && w == "WP_BLASTER_PISTOL") || f.zweiPistolen ||
            (k == "CLASS_CALONORD" && w == "WP_REBELBLASTER")) {
            return beine("BOTH_WALKBACK_DUALPISTOL");
        }
        return beine("BOTH_WALKBACK_PISTOL");
    }
    if (mdGewehr(w)) { return beine("BOTH_WALKBACK_BLASTER"); }
    if (mdSchwer(w)) { return beine("BOTH_WALKBACK_HEAVY"); }
    if (mdGranate(w)) { return beine("BOTH_WALKBACK_GRENADE"); }
    return beine("BOTH_WALKBACK1");
}

// --- Vorwaerts und seitwaerts ---------------------------------------------
PmBeinWahl vorwaertsOj(const PmFigur& f, bool gehen, const PmBeinStand& z) {
    const std::string& k = f.klasse;
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{f.waffe.empty() || f.waffe == "WP_NONE" ? "BOTH_WALK1" : "BOTH_WALK2", true, false}; }
    const bool klinge = f.waffe == "WP_SABER" && f.klingeAn;
    if (!gehen) {
        if (klinge) {
            if (f.saberStil == 6) { return beine("BOTH_RUN_DUAL"); }
            if (f.saberStil == 7) { return beine("BOTH_RUN_STAFF"); }
            return beine("BOTH_RUN2");
        }
        if (k == "CLASS_JAWA") { return beine("BOTH_RUN4"); }
        if (k == "CLASS_ATST") { return anlauf(z, "BOTH_RUN1"); }
        if (k == "CLASS_WAMPA") { return beine(f.laufTempo == 300.0F ? "BOTH_RUN1" : "BOTH_RUN2"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALK1"); }
        return beine("BOTH_RUN1");
    }
    if (klinge) {
        if (f.saberStil == 6) { return beine("BOTH_WALK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALK_STAFF"); }
        return beine("BOTH_WALK2");
    }
    // CLASS_WAMPA geht mit health <= 50 BOTH_WALK2 - die Figur ist beim Steuern heil.
    return beine("BOTH_WALK1");
}

PmBeinWahl vorwaertsMd(const PmFigur& f, int vor, bool gehen, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{w.empty() || w == "WP_NONE" ? "BOTH_WALK1" : "BOTH_WALK2", true, false}; }
    if (!gehen) {
        if (w == "WP_SABER") {
            // PM_GetRunAnim
            if (ziehtSchwert(z.torso, true) || !f.klingeAn) { return beine("BOTH_RUN1"); }
            if (f.saberStil == 6) { return beine("BOTH_RUN_DUAL"); }
            if (f.saberStil == 7) { return beine("BOTH_RUN_STAFF"); }
            return beine("BOTH_RUN2");
        }
        if (w.empty() || w == "WP_MELEE" || w == "WP_NONE") { return beine("BOTH_RUN1"); }
        if (k == "CLASS_JAWA") { return beine("BOTH_RUN4"); }
        if (mdPistole(w, true)) { return beine(f.zweiPistolen ? "BOTH_JOG_DOUBLE_PISTOL" : "BOTH_JOG_PISTOL"); }
        if (mdGewehr(w)) { return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_RUN1" : "BOTH_JOG_BLASTER"); }
        if (mdSchwer(w)) { return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_RUN1" : "BOTH_JOG_HEAVY"); }
        if (mdGranate(w)) { return beine("BOTH_JOG_GRENADE"); }
        if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return anlauf(z, "BOTH_RUN1"); }
        if (k == "CLASS_WAMPA") { return beine(f.laufTempo == 300.0F ? "BOTH_RUN1" : "BOTH_RUN2"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALK1"); }
        if (k == "CLASS_SBD") { return PmBeinWahl{"BOTH_RUN1", true, true}; }
        return beine("BOTH_RUN1");
    }
    if (w == "WP_SABER" && f.klingeAn) {
        // PM_GetWalkWithSaberOnAnim fuer einen NPC
        if (ziehtSchwert(z.torso, true)) { return beine("BOTH_WALK1"); }
        if (f.saberStil == 6) { return beine("BOTH_WALK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALK_STAFF"); }
        return beine("BOTH_WALK1");
    }
    if (k == "CLASS_DROIDEKA" && !z.inKamera) {
        if (z.beine != "BOTH_WALK1" && vor > 0) {
            if (z.beine != "BOTH_RUN1STOP" && z.beine == "BOTH_RUN1") { return PmBeinWahl{"BOTH_RUN1STOP", false, true}; }
            if (!z.beineTimer) { return beine("BOTH_WALK1"); }
            return PmBeinWahl{};
        }
        return beine("BOTH_WALK1");
    }
    if (k == "CLASS_WAMPA") { return beine("BOTH_WALK1"); }   // health > 50
    if (mdPistole(w, false)) { return beine(f.zweiPistolen ? "BOTH_WALK_DOUBLE_PISTOL" : "BOTH_WALK_PISTOL"); }
    if (mdGewehr(w) || einer(w, {"WP_DEMP2", "WP_BOWCASTER", "WP_FLECHETTE"})) {
        return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_WALK1" : "BOTH_WALK_BLASTER");
    }
    if (einer(w, {"WP_CONCUSSION", "WP_ROCKET_LAUNCHER", "WP_Z6_ROTARY_CANNON"})) {
        return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_WALK1" : "BOTH_WALK_HEAVY");
    }
    if (mdGranate(w)) { return beine("BOTH_WALK_GRENADE"); }
    if (w == "WP_MELEE") {
        if (k == "CLASS_YODA") { return beine("BOTH_WALK1_YODA"); }
        if (k == "CLASS_VADER") { return beine("BOTH_WALK1_VADER"); }
        if (k == "CLASS_SITHLORD") { return beine("BOTH_WALK1_MDA"); }
        return beine("BOTH_WALK1");
    }
    return beine("BOTH_WALK1");
}

}  // namespace

void pmRueckwaertsSetzen(bool& rueckwaerts, int vor, int seit) noexcept {
    if (vor < 0) {
        rueckwaerts = true;
    } else if (vor > 0 || (vor == 0 && seit != 0)) {
        rueckwaerts = false;
    }
}

bool pmGehAnim(const std::string& a, bool md) {
    if (einer(a, {"BOTH_WALK1", "BOTH_WALK2", "BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALK5", "BOTH_WALK6", "BOTH_WALK7",
                  "BOTH_WALKBACK1", "BOTH_WALKBACK2", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL"})) {
        return true;
    }
    return md &&
           einer(a, {"BOTH_MENUIDLE1", "BOTH_WALK1_ANI", "BOTH_WALK1_BDROID", "BOTH_WALK1_BEN", "BOTH_WALK1_CAL",
                     "BOTH_WALK1_DOOKU", "BOTH_WALK1_GALEN", "BOTH_WALK1_GRIEV", "BOTH_WALK1_JANGO", "BOTH_WALK1_MAUL",
                     "BOTH_WALK1_MDA", "BOTH_WALK1_MDA_BDROID", "BOTH_WALK1_MDA_PAL", "BOTH_WALK1_OBI3", "BOTH_WALK1_PAL",
                     "BOTH_WALK1_REN", "BOTH_WALK1_REY", "BOTH_WALK1_STICK", "BOTH_WALK1_STICK_YODA", "BOTH_WALK1_VADER",
                     "BOTH_WALK1_YODA", "BOTH_WALK2_ANI", "BOTH_WALK2_BDROID", "BOTH_WALK2_BEN", "BOTH_WALK2_CAL",
                     "BOTH_WALK2_DOOKU", "BOTH_WALK2_GALEN", "BOTH_WALK2_GRIEV", "BOTH_WALK2_JANGO", "BOTH_WALK2_MAUL",
                     "BOTH_WALK2_OBI3", "BOTH_WALK2_PAL", "BOTH_WALK2_REN", "BOTH_WALK2_REY", "BOTH_WALK2_VADER",
                     "BOTH_WALK2_YODA", "BOTH_WALKBACK1_GRIEV", "BOTH_WALKBACK1_VADER", "BOTH_WALKBACK2_GRIEV",
                     "BOTH_WALKBACK2_YODA", "BOTH_WALKBACK_BLASTER", "BOTH_WALKBACK_DUALPISTOL", "BOTH_WALKBACK_DUAL_GALEN",
                     "BOTH_WALKBACK_DUAL_MAUL", "BOTH_WALKBACK_GRENADE", "BOTH_WALKBACK_HEAVY", "BOTH_WALKBACK_PISTOL",
                     "BOTH_WALKBACK_STAFF_MAUL", "BOTH_WALK_BAZOOKA", "BOTH_WALK_BAZOOKA_BDROID", "BOTH_WALK_BLASTER",
                     "BOTH_WALK_BLASTER_BDROID", "BOTH_WALK_DOUBLE_PISTOL", "BOTH_WALK_DOUBLE_PISTOL_BDROID",
                     "BOTH_WALK_DUAL_AMD", "BOTH_WALK_DUAL_ANI", "BOTH_WALK_DUAL_BDROID", "BOTH_WALK_DUAL_GALEN",
                     "BOTH_WALK_DUAL_MAUL", "BOTH_WALK_GRENADE", "BOTH_WALK_GRENADE_BDROID", "BOTH_WALK_HEAVY",
                     "BOTH_WALK_HEAVY_BDROID", "BOTH_WALK_MINIGUN", "BOTH_WALK_MINIGUN_BDROID", "BOTH_WALK_PISTOL",
                     "BOTH_WALK_PISTOL_BDROID", "BOTH_WALK_STAFF_AMD", "BOTH_WALK_STAFF_BEN", "BOTH_WALK_STAFF_MAUL",
                     "BOTH_WALK_STAFF_PAL"});
}

bool pmLaufAnim(const std::string& a, bool md) {
    if (einer(a, {"BOTH_RUN1", "BOTH_RUN2", "BOTH_RUN4", "BOTH_RUN_STAFF", "BOTH_RUN_DUAL", "BOTH_RUNBACK1", "BOTH_RUNBACK2",
                  "BOTH_RUNBACK_STAFF", "BOTH_RUN1START", "BOTH_RUN1STOP", "BOTH_RUNSTRAFE_LEFT1", "BOTH_RUNSTRAFE_RIGHT1"})) {
        return true;
    }
    if (!md) { return false; }
    // Movie Duels: dieselben, dazu JOG, SPRINT und die Figurenvarianten
    // (PM_RunningAnim, bg_pmove.cpp:10403-10547 mdgit, ohne Ausnahme).
    return einer(a, {"BOTH_JOG_BAZOOKA", "BOTH_JOG_BAZOOKA_GALEN", "BOTH_JOG_BLASTER", "BOTH_JOG_BLASTER_GALEN",
                     "BOTH_JOG_DOUBLE_PISTOL", "BOTH_JOG_DOUBLE_PISTOL_GALEN", "BOTH_JOG_GRENADE", "BOTH_JOG_GRENADE_GALEN",
                     "BOTH_JOG_HEAVY", "BOTH_JOG_HEAVY_GALEN", "BOTH_JOG_MINIGUN", "BOTH_JOG_MINIGUN_GALEN", "BOTH_JOG_PISTOL",
                     "BOTH_JOG_PISTOL_GALEN", "BOTH_RUN10", "BOTH_RUN1_ANI", "BOTH_RUN1_BDROID", "BOTH_RUN1_BEN",
                     "BOTH_RUN1_GRIEV", "BOTH_RUN1_MAUL", "BOTH_RUN1_OBI3", "BOTH_RUN1_PAL", "BOTH_RUN1_REY", "BOTH_RUN1_VADER",
                     "BOTH_RUN1_YODA", "BOTH_RUN2_ANI", "BOTH_RUN2_BDROID", "BOTH_RUN2_BEN", "BOTH_RUN2_CAL", "BOTH_RUN2_DOOKU",
                     "BOTH_RUN2_GALEN", "BOTH_RUN2_GRIEV", "BOTH_RUN2_OBI3", "BOTH_RUN2_REN", "BOTH_RUN2_REY", "BOTH_RUN2_YODA",
                     "BOTH_RUN5", "BOTH_RUN6", "BOTH_RUN7", "BOTH_RUN8", "BOTH_RUN9", "BOTH_RUNBACK1_VADER",
                     "BOTH_RUNBACK2_GALEN", "BOTH_RUNBACK2_GRIEV", "BOTH_RUNBACK2_VADER", "BOTH_RUNBACK2_YODA",
                     "BOTH_RUNBACK_BLASTER", "BOTH_RUNBACK_DUALPISTOL", "BOTH_RUNBACK_DUAL_GALEN", "BOTH_RUNBACK_GRENADE",
                     "BOTH_RUNBACK_HEAVY", "BOTH_RUNBACK_PISTOL", "BOTH_RUNBACK_STAFF_MAUL", "BOTH_RUN_DUAL_ANI",
                     "BOTH_RUN_DUAL_BDROID", "BOTH_RUN_DUAL_BEN", "BOTH_RUN_DUAL_GALEN", "BOTH_RUN_DUAL_REY",
                     "BOTH_RUN_STAFF_ANI", "BOTH_RUN_STAFF_BDROID", "BOTH_RUN_STAFF_BEN", "BOTH_RUN_STAFF_MAUL",
                     "BOTH_RUN_STAFF_REY"}) ||
           a.rfind("BOTH_SPRINT", 0) == 0;
}

PmBeinWahl pmFootstepsBeine(const PmFigur& f, int vor, int seit, bool gehen, bool rueckwaerts, const PmBeinStand& z) {
    if (vor == 0 && seit == 0) { return imStand(f, z); }
    if (rueckwaerts) { return f.md ? rueckwaertsMd(f, gehen, z) : rueckwaertsOj(f, gehen); }
    return f.md ? vorwaertsMd(f, vor, gehen, z) : vorwaertsOj(f, gehen, z);
}

float pmFussTempo(const std::string& anim, float resultSpeed, const PmFigur& f, double frameLerpMs, bool inKamera) {
    const bool kriechen = anim == "BOTH_CROUCH1WALK" || anim == "BOTH_CROUCH1WALKBACK";
    const bool geht = pmGehAnim(anim, f.md);
    if (!(geht || pmLaufAnim(anim, f.md) || kriechen)) { return 1.0F; }
    if (einer(f.klasse, {"CLASS_HOWLER", "CLASS_WAMPA", "CLASS_GONK", "CLASS_MOUSE", "CLASS_PROBE", "CLASS_PROTOCOL", "CLASS_R2D2",
                         "CLASS_R5D2", "CLASS_SEEKER"})) {
        return 1.0F;
    }
    if (f.md && resultSpeed < 1.0F && inKamera) { return 1.0F; }
    float soll = 150.0F;
    if (kriechen) {
        soll = 75.0F;
    } else if (f.klasse == "CLASS_HAZARD_TROOPER") {
        soll = 50.0F;
    } else if (f.klasse == "CLASS_RANCOR") {
        soll = 173.0F;
    } else if (geht) {
        soll = (f.saberStil == 6 || f.saberStil == 7) ? 100.0F : 50.0F;
    } else {
        soll = f.saberStil == 7 ? 250.0F : 150.0F;
    }
    // animSpeed der Engine = 50/frameLerp * resultspeed/soll, auf 0,01 .. 1,5;
    // hier gegenueber der .cfg (50/frameLerp herausgerechnet).
    const double lerp = frameLerpMs > 0.0 ? frameLerpMs : 50.0;
    const double engine = std::clamp(50.0 / lerp * static_cast<double>(resultSpeed / soll), 0.01, 1.5);
    return static_cast<float>(engine * lerp / 50.0);
}

double pmStelle(const PmKnochen& k, double jetztMs) noexcept { return (jetztMs - k.startMs) * static_cast<double>(k.tempo); }

bool pmSetAnimNormal(PmKnochen& k, const std::string& anim, float tempo, double jetztMs, double blendMs, double halteMs) {
    if (anim.empty()) { return false; }
    if (k.timerBisMs >= kPmNie || (k.timerBisMs >= 0.0 && jetztMs < k.timerBisMs)) { return false; }
    if (k.anim == anim) {
        if (k.tempo == tempo) { return false; }
        // An derselben Stelle weiter (animCurrent = bodyCurrent), ohne BONE_ANIM_BLEND.
        const double stelle = pmStelle(k, jetztMs);
        k.tempo = tempo;
        k.startMs = jetztMs - (tempo > 0.0F ? stelle / static_cast<double>(tempo) : 0.0);
        return true;
    }
    if (!k.anim.empty() && blendMs > 0.0) {
        k.vorher = k.anim;
        k.vorherStelleMs = pmStelle(k, jetztMs);
        k.blendAbMs = jetztMs;
        k.blendMs = blendMs;
    } else {
        k.vorher.clear();
    }
    k.anim = anim;
    k.startMs = jetztMs;
    k.tempo = tempo;
    k.timerBisMs = halteMs >= 0.0 ? jetztMs + halteMs : -1.0;
    return true;
}

PmTorsoWahl pmTorsoLichtschwert(const PmFigur& f, const std::string& beine, const std::string& torso) {
    PmTorsoWahl w;
    // PM_RunningAnim oder Stab/zwei Klingen beim Gehen: der Oberkoerper folgt
    // (saberBlockingTime < cg.time - ohne Blocken immer).
    if (pmLaufAnim(beine, f.md) ||
        einer(beine, {"BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL"})) {
        w.folgen = true;
        return w;
    }
    // PM_SetSaberMove(LS_READY): PM_ReadyPoseForSaberAnimLevel
    std::string bereit = saberHaltungFuerStil(f.saberStil, f.md);
    w.blendMs = kPmBlendSaberBereit;
    if (!f.md) {
        // "match torso anim to walk/run anim if newMove is just LS_READY"
        if (einer(bereit, {"BOTH_STAND2", "BOTH_SABERDUAL_STANCE", "BOTH_SABERFAST_STANCE", "BOTH_SABERSLOW_STANCE"}) &&
            einer(beine, {"BOTH_WALK1", "BOTH_WALK2", "BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALKBACK1", "BOTH_WALKBACK2",
                          "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL", "BOTH_RUN1", "BOTH_RUN2", "BOTH_RUN_STAFF", "BOTH_RUN_DUAL",
                          "BOTH_RUNBACK1", "BOTH_RUNBACK2", "BOTH_RUNBACK_STAFF"})) {
            bereit = beine;
        }
        w.anim = bereit;
        return w;
    }
    // Movie Duels: stehen die Beine in einer Kampfhaltung (PM_SaberStanceAnim,
    // ausser BOTH_STAND1), bekommen beide Knochen die Haltung; laeuft der
    // Oberkoerper noch (PM_RunningAnim torsoAnim), bleibt er.
    if (einer(beine, {"BOTH_STAND2", "BOTH_STAND2_JKA", "BOTH_SABERFAST_STANCE", "BOTH_SABERSLOW_STANCE", "BOTH_SABERSTAFF_STANCE",
                      "BOTH_SABERDUAL_STANCE"})) {
        w.anim = bereit;
        w.beide = true;
        return w;
    }
    if (pmLaufAnim(torso, true)) { return w; }
    w.anim = bereit;
    return w;
}

float pmBeinGier(float beinGier, float blickGier, float vx, float vy, bool rueckwaerts, double frameInterpolation) {
    constexpr float kMin = -60.0F;
    constexpr float kMax = 60.0F;
    float drehung = 10.0F;
    float dazu = 0.0F;
    if (vx != 0.0F || vy != 0.0F) {
        // vectoyaw
        float gier = std::atan2(vy, vx) * 180.0F / 3.14159265358979F;
        if (gier < 0.0F) { gier += 360.0F; }
        dazu = -winkelDelta(blickGier, gier);
        if (dazu > 150.0F || dazu < -150.0F) {
            dazu = 0.0F;
        } else {
            dazu = std::clamp(dazu, kMin, kMax);
            if (rueckwaerts) { dazu = -dazu; }
            drehung = 5.0F;
        }
    }
    float diff = winkelDelta(beinGier, blickGier + dazu);
    float neu = static_cast<float>(diff * frameInterpolation * -1.0);
    if (std::fabs(neu) > std::fabs(diff)) { neu = -diff; }
    neu = std::clamp(neu, -drehung, drehung);
    float gier = beinGier + neu;
    diff = winkelDelta(blickGier, gier);
    if (diff > kMax) {
        gier = blickGier - kMax;
    } else if (diff < kMin) {
        gier = blickGier - kMin;
    }
    return gier;
}

}  // namespace bhed
