// abwurf.h - Pruefung 3: was eine sterbende Figur fallen laesst, mit der
// Bewegung des Spiels.
//
// player_die laesst die Waffe als Gegenstand fallen (TossClientItems ->
// Drop_Item -> LaunchItem, g_items.cpp) und ein Schwert aus der Hand
// (WP_SaberLose -> WP_SaberLaunch/WP_SaberDrop, wp_saber.cpp). Beides fliegt
// danach je Serverbild (50 ms) nach der Bahn (EvaluateTrajectory TR_GRAVITY,
// bg_misc.cpp) und prallt ab:
//   Gegenstand  G_RunItem / G_BounceItem (g_items.cpp:1407-1550 OJ, MD gleich;
//               JK2 ohne die Schwert-Zweige)
//   Schwert     WP_SaberUpdate (dreimal je Denkbild eines NPC, g_active.cpp:
//               5245-5259 OJ) -> WP_RunSaber -> WP_SaberImpact ->
//               G_BounceMissile (EF_BOUNCE_HALF) - liegt es, gibt WP_SaberUpdate
//               es frei, weil sein Besitzer tot ist (wp_saber.cpp:7090-7105 OJ).
#ifndef BHED_ABWURF_H
#define BHED_ABWURF_H

#include "bhed/navlauf.h"
#include "bhed/todesanim.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed {

struct AbwurfStart {
    double ms = 0.0;           // level.time des Abwurfs (der Tod)
    NavVec ort{};              // s.pos.trBase
    NavVec tempo{};            // s.pos.trDelta
    NavVec winkel{};           // s.apos.trBase / currentAngles
    NavVec drehTempo{};        // s.apos.trDelta (TR_LINEAR), 0: steht
    NavVec mins{-3.0F, -3.0F, -3.0F};
    NavVec maxs{3.0F, 3.0F, 3.0F};
    int entity = kNavKeineEntity;   // owner/activator - die Spur uebergeht sie
    TodRegelwerk regel = TodRegelwerk::OpenJK;
    float schwerkraft = 800.0F;     // g_gravity
    std::uint32_t saat = 1U;        // fuer Q_irand der Drehung und der Klaenge
    // Schwert: der Klang beim Aufprall (WP_SaberFallSound mit Besitzer,
    // wp_saber.cpp:634-654 OJ): fallSound[Q_irand(0, 2)] der .sab, wenn der
    // erste gesetzt ist; sonst ein SABER_SITH_SWORD sword/fallN (N 1..7), sonst
    // saber/bounceN (N 1..3).
    std::array<std::string, 3> fallKlang{};
    bool sithSchwert = false;
};

struct AbwurfBahn {
    struct Punkt {
        double ms = 0.0;
        NavVec ort{};
        NavVec winkel{};
    };
    std::vector<Punkt> punkte;    // je Serverbild ab dem Abwurf, zuletzt der Ruhestand
    double ruheMs = -1.0;         // TR_STATIONARY ab
    double wegMs = -1.0;          // G_FreeEntity ab (-1: bleibt)
    struct Klang {
        double ms = 0.0;
        std::string datei;
        NavVec ort{};
    };
    std::vector<Klang> klaenge;   // G_Sound am Gegenstand
    // Pruefung 4b: G_PlayEffect beim Aufprall (G_RunObject).
    struct Effekt {
        double ms = 0.0;
        std::string datei;     // "melee/kick_impact_silent"
        NavVec ort{};
        NavVec richtung{};
    };
    std::vector<Effekt> effekte;
    // Ort und Winkel zur Zeit ms (zwischen zwei Serverbildern linear, wie
    // cgame zwischen zwei Schnappschuessen mittelt).
    [[nodiscard]] bool bei(double ms, NavVec& ort, NavVec& winkel) const;
};

// Drop_Item (g_items.cpp:1022 OJ): velocity = forward(Gier) * 150, z + 200 +
// flrand * 50; LaunchItem dreht eine Waffe auf (0, flrand * 180, 90). zufall
// sind die zwei Zuege von Q_flrand aus der Todeskette (TodErgebnis::abwurfZufall).
[[nodiscard]] AbwurfStart gegenstandAbwurf(const NavVec& ort, float gier, const float zufall[2], bool waffeDrehen);

// G_RunItem/G_BounceItem bis zum Stillstand (hoechstens grenzeMs nach dem Abwurf).
[[nodiscard]] AbwurfBahn wirfGegenstand(const AbwurfStart& s, const NavSpurFn& spur, double grenzeMs = 20000.0);

// WP_SaberDrop/WP_RunSaber: das Schwert faellt aus der Hand (ort = renderInfo.
// handRPoint, winkel = vectoangles(renderInfo.muzzleDir), drehTempo[1] nach
// FP_SABERTHROW: 600/800/1200) und verschwindet, sobald es liegt.
[[nodiscard]] AbwurfBahn wirfSchwert(const AbwurfStart& s, const NavSpurFn& spur, double grenzeMs = 20000.0);

// --- Pruefung 4b: misc_model_breakable mit "gravity" (target4) -------------
//
// misc_model_breakable_gravity_init (g_breakable.cpp:1035-1103 OpenJK):
// EF_BOUNCE_HALF, clipmask MASK_SOLID|CONTENTS_BODY|MONSTERCLIP|BOTCLIP, mass
// (ohne Angabe |maxs| + |mins|), mit dropToFloor eine Spur MASK_NPCSOLID von
// origin + 1 bis MIN_WORLD_COORD; danach G_RunObject je FRAMETIME
// (g_object.cpp:77-263): in Ruhe TR_GRAVITY ab previousTime, Spur, auf
// begehbarem Boden pitch_roll_for_slope und G_StopObjectMoving.
struct ObjektStart {
    NavVec ort{};             // currentOrigin
    NavVec winkel{};          // currentAngles
    NavVec mins{-16.0F, -16.0F, -16.0F};
    NavVec maxs{16.0F, 16.0F, 16.0F};
    float masse = 0.0F;       // "mass"; 0: |maxs| + |mins|
    int entity = kNavKeineEntity;
    float schwerkraft = 800.0F;   // g_gravity
};
// Spawn: auf den Boden (dropToFloor) und das erste G_RunObject (spawn +
// FRAMETIME) an den Hang gelegt - Ort und Winkel danach.
[[nodiscard]] ObjektStart objektAbsetzen(const ObjektStart& s, const NavSpurFn& spur);
// misc_model_throw_at_target4 (g_breakable.cpp:629-695): bei jedem use (ms,
// aufsteigend) zum Ziel geworfen - trBase = currentOrigin, trTime = ms,
// TR_GRAVITY, trDelta += kvel (g_knockback 1000) -, dazwischen G_RunObject
// je FRAMETIME bis zur Ruhe.
[[nodiscard]] AbwurfBahn wirfObjekt(const ObjektStart& s, const std::vector<double>& wuerfe, const NavVec& ziel,
                                    const NavSpurFn& spur, double grenzeMs = 20000.0);

}  // namespace bhed
#endif
