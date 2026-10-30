// pmanim.h - Beine und Oberkoerper einer gesteuerten Figur, wie im Spiel.
//
// Zeitleiste 4 Punkt 3: beim Aufnehmen (gui/app_mission.cpp) laeuft die Figur
// mit dem Pmove-Port (navSpielerPmove). Was das Modell dabei zeigt, waehlt im
// Spiel danach
//
//   PM_Footsteps        die Beinanimation aus forwardmove/rightmove,
//                       BUTTON_WALKING, PMF_BACKWARDS_RUN, Waffe, Klinge,
//                       NPC_class (code/game/bg_pmove.cpp)
//   PM_SetAnimFinal     wann sie wirklich gesetzt wird (Timer), mit welchem
//                       Tempo (g_noFootSlide) und mit Uebergang
//                       (BONE_ANIM_BLEND, SETANIM_BLEND_DEFAULT 100 ms)
//                       (code/game/bg_panimate.cpp)
//   PM_TorsoAnimation   den Oberkoerper (ohne Waffe folgt er den Beinen)
//   cgame               die Beine drehen sich zur Laufrichtung, der Rumpf
//                       zurueck zum Blick (CG_PlayerLegsYawFromMovement,
//                       CG_G2ClientSpineAngles, code/cgame/cg_players.cpp)
//
// Portiert fuer OpenJK SP und Movie Duels (mdgit, g_ActivateAnimationStyle 0,
// g_SerenityJediEngineMode 0 - wirksam im installierten Spiel, siehe
// saberHaltungFuerStil in scene.h -, g_noIgniteTwirl 1 wie in der DLL)
// fuer einen NPC am Boden, nicht geduckt, nicht im Wasser, ohne Leiter: so
// steuert ihn die Aufnahme (W/A/S/D, Umschalt, Q/E). Im Stand am Hang
// PM_AdjustStandAnimForSlope (PmBeinStand::hang, die Rechnung der Szene).
#ifndef BHED_PMANIM_H
#define BHED_PMANIM_H

#include <functional>
#include <array>
#include <string>

namespace bhed {

// PMF_BACKWARDS_RUN nach PmoveSingle (bg_pmove.cpp:14806-14811 OpenJK): bei
// forwardmove < 0 an, bei forwardmove > 0 oder rein seitwaerts aus, ohne
// Eingabe unveraendert.
void pmRueckwaertsSetzen(bool& rueckwaerts, int vor, int seit) noexcept;

// PM_WalkingAnim / PM_RunningAnim (bg_pmove.cpp:6927-6968 OpenJK,
// 10305-10547 MD) - die Listen 1:1.
[[nodiscard]] bool pmGehAnim(const std::string& anim, bool md);
[[nodiscard]] bool pmLaufAnim(const std::string& anim, bool md);

// Was PM_Footsteps und PM_SetAnimFinal von der Figur lesen.
struct PmFigur {
    std::string klasse;          // client->NPC_class ("CLASS_IMPERIAL"), leer = keine
    std::string waffe;           // ps.weapon, leer = WP_NONE
    bool klingeAn = false;       // ps.SaberActive()
    bool klingeLaenge = false;   // ps.SaberLength() > 0 (auch beim Ein- und Ausfahren)
    int saberStil = 0;           // ps.saberAnimLevel: SS_NONE 0, FAST 1, MEDIUM 2, STRONG 3,
                                 // DESANN 4, TAVION 5, DUAL 6, STAFF 7
    bool zweiPistolen = false;   // gent->weaponModel[1] > 0
    float laufTempo = 0.0F;      // NPC->stats.runSpeed (CLASS_WAMPA: 300 = auf allen vieren)
    bool md = false;             // Regelwerk Movie Duels
    // Zeitleiste 6: die Schwerter fuer PM_SetSaberMove(LS_READY) (saberInfo_t, .sab)
    std::string saberTyp;                    // saber[0].type ("SABER_ARC", leer = SABER_SINGLE)
    std::array<std::string, 2> bereitAnim{}; // saber[n].readyAnim, leer = -1
    bool zweiSchwerter = false;              // ps.dualSabers
    bool zweitesAn = false;                  // saber[1].Active()
    bool stab = false;                       // ps.SaberStaff(): saber[0] oder (zwei) saber[1] ist SABER_STAFF
    bool eineKlingeStil = false;             // saber[0].singleBladeStyle != 0
    bool klinge2An = false;                  // saber[0].blade[1].active
    bool blocken = false;                    // saberBlockingTime > cg.time (manuelles Blocken)
    // Movie Duels: client->animationstyle ("CS_GALEN_MAREK" ..., leer = CS_DEFAULT, NPC-Datei
    // "animationstyle") und g_ActivateAnimationStyle (aus g_activateanimationstyle, Vorgabe 0)
    std::string animStil;
    bool animStilAn = false;
};

// Was PM_Footsteps im Zweig am Boden setzt: die Animation, ob es SETANIM_BOTH
// ist (CLASS_GALAKMECH, CLASS_SBD) und ob mit SETANIM_FLAG_HOLD (Anlauf und
// Auslauf von ATST und Droideka: BOTH_RUN1START, BOTH_RUN1STOP). Leer: es
// setzt nichts (die Wachhaltung eines NPC mit Waffe bleibt stehen, der Anlauf
// laeuft noch).
struct PmBeinWahl {
    std::string anim;
    bool beide = false;
    bool halten = false;
};
// Was PM_Footsteps vom Zustand liest: ps.legsAnim, ob legsAnimTimer laeuft,
// ps.torsoAnim (PM_SaberDrawPutawayAnim) und in_camera (Movie Duels: der An-
// und Auslauf der Droideka nur ausserhalb einer Kamerafahrt).
struct PmBeinStand {
    std::string beine;
    bool beineTimer = false;
    std::string torso;
    bool inKamera = false;
    // Zeitleiste 5: PM_AdjustStandAnimForSlope - gerufen nur dort, wo
    // PM_Footsteps es im Stand ruft (ATST, mit Klinge, mit Waffe ausser den
    // Wachhaltungen, sonst; nicht CLASS_GALAKMECH), denn es stellt
    // slopeRecalcTime. Liefert die Hang-Beinanimation (SETANIM_LEGS) oder leer
    // (qfalse, dann die Grundhaltung). Leer gelassen: kein Hang.
    std::function<std::string()> hang;
};
[[nodiscard]] PmBeinWahl pmFootstepsBeine(const PmFigur& f, int vor, int seit, bool gehen, bool rueckwaerts,
                                          const PmBeinStand& z);

// g_noFootSlide in PM_SetAnimFinal: das Tempo einer Lauf- oder Gehanimation
// (PM_WalkingAnim, PM_RunningAnim, BOTH_CROUCH1WALK/-BACK) gegenueber ihrer
// .cfg - animSpeed = 50/frameLerp * resultspeed/moveSpeedOfAnim, 0,01 .. 1,5.
// frameLerpMs: ms je Bild der Animation (0 = unbekannt, dann 50). inKamera:
// Movie Duels skaliert in einer Kamerafahrt erst ab resultspeed 1.
// Sonst 1 (das Tempo der .cfg).
[[nodiscard]] float pmFussTempo(const std::string& anim, float resultSpeed, const PmFigur& f, double frameLerpMs,
                                bool inKamera);

// Ein Knochen, wie PM_SetAnimFinal ihn fuehrt (Beine: rootBone, Oberkoerper:
// lowerLumbarBone). Zeiten in ms einer eigenen Uhr; die Stelle in der
// Animation (ms der .cfg) ist (jetzt - startMs) * tempo.
struct PmKnochen {
    std::string anim;
    double startMs = 0.0;
    float tempo = 1.0F;
    // legsAnimTimer/torsoAnimTimer als Endzeit: < 0 kein Timer, kPmNie = -1
    // der Engine (haelt immer).
    double timerBisMs = -1.0;
    // Der Uebergang: das eingefrorene Bild der vorigen Animation (ghoul2
    // mischt gegen ein Standbild), ab wann und wie lange.
    std::string vorher;
    double vorherStelleMs = 0.0;
    double blendAbMs = 0.0;
    double blendMs = 0.0;
};
inline constexpr double kPmNie = 1.0e300;
inline constexpr double kPmBlendVorgabe = 100.0;   // SETANIM_BLEND_DEFAULT (bg_public.h)

[[nodiscard]] double pmStelle(const PmKnochen& k, double jetztMs) noexcept;
// PM_SetAnim mit SETANIM_FLAG_NORMAL (PM_Footsteps, PM_TorsoAnimation):
// laeuft der Timer, bleibt alles (bodyPlay = animOverride || !bodyTimerOn).
// Dieselbe Animation mit demselben Tempo: nichts; mit anderem Tempo laeuft
// sie an derselben Stelle weiter, ohne Uebergang. Eine andere beginnt vorn,
// mit Uebergang vom jetzigen Bild. true, wenn sie gesetzt wurde.
// halteMs >= 0: SETANIM_FLAG_HOLD - der Timer laeuft so lange (animHoldMSec:
// (numFrames - 1) * frameLerp).
bool pmSetAnimNormal(PmKnochen& k, const std::string& anim, float tempo, double jetztMs, double blendMs = kPmBlendVorgabe,
                     double halteMs = -1.0);

// PM_TorsoAnimLightsaber (bg_panimate.cpp:5160-5505 OpenJK, :9205 MD) fuer eine
// Figur mit Klinge (ps.SaberLength(), WEAPON_READY, kein Angriff, keine Macht).
// Springt, landet, schlaegt sie ein Rad oder einen Salto, oder laeuft sie
// (PM_RunningAnim) bzw. geht mit Stab oder zwei Klingen und blockt nicht,
// folgt der Oberkoerper den Beinen (folgen = true). Sonst PM_SetSaberMove(
// LS_READY) (bg_pmove.cpp OpenJK, :22582 MD): beim Blocken BOTH_P1_S1_T_
// (Klaue, zweites Schwert: BOTH_INAIR1; MD: die Kampfhaltung bzw. BOTH_STAND2),
// sonst readyAnim der .sab, SABER_ARC, zwei Schwerter, Stab, SABER_LANCE/
// TRIDENT oder die Kampfhaltung (Movie Duels: PM_ReadyPoseForSaberAnimLevelNPC
// mit den Figurenvarianten von g_ActivateAnimationStyle 1, geduckt _Ducked).
// Haltungen wie BOTH_STAND2 und die Kampfhaltungen nehmen beim Gehen und Laufen
// die Beinanimation ("match torso anim to walk/run anim"). Ohne Eingabe
// (Movie Duels auch bei Tempo 0) und mit ruhigen Beinen, beim Blocken immer:
// SETANIM_BOTH. Blende saberMoveData[LS_READY] 350 ms.
struct PmTorsoWahl {
    bool folgen = false;     // PM_SetAnim(SETANIM_TORSO, legsAnim)
    std::string anim;        // sonst diese (leer: nichts)
    double blendMs = kPmBlendVorgabe;
    bool beide = false;      // SETANIM_BOTH
};
inline constexpr double kPmBlendSaberBereit = 350.0;   // saberMoveData[LS_READY].blendTime
// Was PM_SetSaberMove vom Bewegungsschritt liest: ob cmd forward-, right- oder
// upmove hat, ps.speed == 0 (Movie Duels), PMF_DUCKED und PM_InRoll ||
// PM_InKnockDown (Zeitgeber der Beine - die Aufnahme setzt diese Animationen nie).
struct PmTorsoLage {
    bool eingabe = false;
    bool tempoNull = false;
    bool geduckt = false;
    bool rolleOderLiegen = false;
};
[[nodiscard]] PmTorsoWahl pmTorsoLichtschwert(const PmFigur& f, const std::string& beine, const PmTorsoLage& lage);
// PM_FlippingAnim und PM_SpinningSaberAnim (bg_pmove.cpp OpenJK; Movie Duels mit Zusaetzen).
[[nodiscard]] bool pmSaltoAnim(const std::string& a, bool md);
[[nodiscard]] bool pmDrehSchwertAnim(const std::string& a, bool md);

// CG_PlayerLegsYawFromMovement (cg_players.cpp, OpenJK und MD gleich) fuer
// ein Clientbild, mit Grenzen -60..60 und alwaysFace: die Beine drehen sich
// zur Bewegung (rueckwaerts gespiegelt, ueber 150 Grad gar nicht), hoechstens
// 5 Grad je Bild in Bewegung, 10 im Stehen, und um frameInterpolation des
// Rests. Gibt die neue Gier der Beine zurueck.
[[nodiscard]] float pmBeinGier(float beinGier, float blickGier, float vx, float vy, bool rueckwaerts,
                               double frameInterpolation);
}  // namespace bhed
#endif
