// rumpfwinkel.h - Rumpf, Hals und Kopf einer Figur wie im Spiel.
//
// Zeitleiste 6 Punkt 1: cgame dreht die Wirbelsaeule jeder Figur ueber
// Knochenwinkel (BONE_ANGLES_POSTMULT, siehe g2WinkelMatrix in gla.h):
//
//   CG_G2ClientSpineAngles   lower_lumbar, upper_lumbar, thoracic: der Rumpf
//                            gegen die Beine (CG_PlayerLegsYawFromMovement)
//                            und die Motion-Bone-Korrektur
//   CG_G2ClientNeckAngles    thoracic, cervical, cranium: der Blick
//
// (code/cgame/cg_players.cpp OpenJK; Movie Duels mit CLASS_DROIDEKA und den
// Kantenanimationen). Die Motion-Bone-Korrektur (cg_motionBoneComp 2, die
// Vorgabe beider Spiele) gilt fuer JEDE Figur, deren Beine und Oberkoerper
// verschiedene Animationen spielen: der Motion-Knochen der Beinanimation
// (G2API_GetBoltMatrix ohne Drehung) gibt Nick-, Gier- und Rollwinkel, die vom
// Blick abgezogen und auf die drei Knochen verteilt werden.
#ifndef BHED_RUMPFWINKEL_H
#define BHED_RUMPFWINKEL_H

#include "bhed/gla.h"

#include <cstdint>
#include <string>

namespace bhed {

// Was CG_G2PlayerAngles je NPC_class anders macht.
enum class RumpfKlasse : std::uint8_t {
    Mensch,          // alle anderen: drei Knochen, Achsen POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z
    HazardTrooper,   // nur die Lendenwirbel, kein Hals
    AssassinDroid,   // je Knochen eine Achse
    Droideka,        // Movie Duels: wie der Assassin Droid
    SaberDroid,      // keine Lendenwirbel, kein thoracic
    Rancor,
    Wampa,
    RocketTrooper,
    Ohne,            // GONK, INTERROGATOR, SENTRY, PROBE, R2D2, R5D2, ATST: kein Rumpf, kein Hals
};

// "CLASS_..." (gross) nach RumpfKlasse; CLASS_DROIDEKA gibt es nur in Movie Duels.
[[nodiscard]] RumpfKlasse rumpfKlasseFuer(const std::string& klasse, bool md) noexcept;

// G_ClassHasBadBones (g_client.cpp).
[[nodiscard]] bool rumpfSchlechteKnochen(RumpfKlasse k) noexcept;

// G_BoneOrientationsForClass (g_client.cpp): die Achsen fuer BG_G2SetBoneAngles.
void rumpfKnochenAchsen(RumpfKlasse k, const std::string& knochen, G2Achse& oben, G2Achse& rechts, G2Achse& vorn) noexcept;

// Ob cg_motionBoneComp wirkt: !PM_FlippingAnim(Beine), !PM_SpinningSaberAnim
// (Beine und Oberkoerper), Beine != Oberkoerper, keine schlechten Knochen.
[[nodiscard]] bool rumpfMotionAusgleich(RumpfKlasse k, const std::string& beine, const std::string& torso, bool md);

// Movie Duels PM_InLedgeMove (bg_pmove.cpp): an der Kante nickt der Rumpf nicht.
[[nodiscard]] bool rumpfAnDerKante(const std::string& beine, bool md);

// motionAngles aus dem Motion-Bolzen ohne Drehung (cg_motionBoneComp 2):
// vectoangles(NEGATIVE_Y), Roll = -Nick von vectoangles(NEGATIVE_X).
void motionWinkelAusBolzen(const float bolzen[3][4], float winkel[3]) noexcept;

struct RumpfEingabe {
    RumpfKlasse klasse = RumpfKlasse::Mensch;
    float blick[3]{};          // viewAngles: Nick (lerpAngles * 0,5), Gier = Rumpf gegen Beine, Roll 0
    bool motion = false;       // rumpfMotionAusgleich
    float motionWinkel[3]{};   // motionWinkelAusBolzen
    bool kante = false;        // rumpfAnDerKante
    float kopf[3]{};           // lookAngles (Blick des Kopfs gegen den Rumpf)
};

// Die Winkel (PITCH, YAW, ROLL) je Knochen und ob die Engine ihn setzt.
struct RumpfWinkel {
    float lower[3]{};
    float upper[3]{};
    float thoracic[3]{};
    float cervical[3]{};
    float cranium[3]{};
    bool lumbarSetzen = false;
    bool thoracicSetzen = false;
    bool halsSetzen = false;   // cervical und cranium
};

[[nodiscard]] RumpfWinkel rumpfWinkel(const RumpfEingabe& e) noexcept;

// Die Winkel auf eine fertige Stellung legen (knochenNachMal), von innen nach
// aussen: lower_lumbar, upper_lumbar, thoracic, cervical, cranium.
void rumpfWinkelAnlegen(const GlaAnimation& anim, std::vector<BoneMatrix>& world, RumpfKlasse k, const RumpfWinkel& w);

}  // namespace bhed
#endif
