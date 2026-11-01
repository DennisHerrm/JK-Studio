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
//
// Pruefung 4a: cgame zeichnet beide nach ihrer Bahn (CG_CalcEntityLerpPositions
// wertet s.pos/s.apos zur Zeit cg.time aus) - nicht nach dem Ort, den der Server
// je Bild ablegt. Die Bahn steht deshalb je Serverbild als trajectory_t da
// (AbwurfTrajektorie). WP_SaberThrow (wp_saber.cpp:6986-7232 OJ) fuer ein
// Schwert, dessen Besitzer tot ist: nach WP_SaberDrop (SES_RETURNING) bleibt es
// in jedem Durchgang stehen, solange es hoechstens 16 von renderInfo.handRPoint
// ist (WP_SaberCatch faengt nicht, der Besitzer ist tot - die Bahn laeuft im
// Bild weiter, der Server prueft sie nicht); mit ausgeschalteter Klinge
// wirft WP_SaberLose es nur (WP_SaberLaunch, TR_LINEAR ohne Tempo) - im
// naechsten Denkbild zielt WP_RunSaber es (saberEntityDist), dann laesst
// WP_SaberThrow es fallen ("switched away while controlling it").
#ifndef BHED_ABWURF_H
#define BHED_ABWURF_H

#include "bhed/navlauf.h"
#include "bhed/todesanim.h"

#include <array>
#include <cstdint>
#include <functional>
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
    // Pruefung 4a - nur Schwert:
    // Die Klinge war aus: WP_SaberLose wirft nur (WP_SaberLaunch), WP_SaberDrop
    // kommt erst im naechsten Denkbild (WP_SaberThrow, ps.weapon != WP_SABER).
    bool klingeAus = false;
    // forcePowerLevel[FP_SABERTHROW] (.npc, JK2 nach Klasse/Rang; JK2s
    // WP_SaberLose setzt FORCE_LEVEL_1): saberEntityDist = saberThrowDist[] und
    // der Drall von WP_SaberLaunch (600/800/1200, drehTempo).
    int wurfStufe = 0;
    NavVec blick{};          // ps.viewangles der Figur (fwdangles in WP_RunSaber)
    bool spieler = false;    // s.number == 0 (fwdangles - 5 statt - 8)
    std::string ausKlang;    // WP_SaberDrop: soundOff (JK2 nach Team) am Schwert
};

// Pruefung 4a: was WP_RunSaber/WP_SaberThrow je Denkbild von der toten Figur
// fragen - nicht in AbwurfStart, weil die Szene den Start aufhebt und die
// Rueckrufe nur waehrend des Rechnens gelten.
struct SchwertUmfeld {
    // renderInfo.handRPoint und eyePoint der toten Figur zur Zeit t (die
    // Stellung des gezeichneten Skeletts). Leer: beide bleiben bei `ort`.
    std::function<void(double t, NavVec& hand, NavVec& auge)> haltung;
    // WP_SaberFindEnemy (FP_SABERTHROW > 2, SES_LEAVING): currentOrigin und
    // maxs[0]/maxs[2] des besten Feindes zur Zeit t um den Ort des Schwerts
    // (hand: handRPoint dazu). Leer oder false: keiner.
    std::function<bool(double t, const NavVec& schwert, const NavVec& hand, const NavVec& auge, NavVec& ort, float& breite,
                       float& hoehe)>
        feind;
};

// trajectory_t (q_shared.h) mit EvaluateTrajectory (bg_misc.cpp:285-340 OJ):
// TR_STATIONARY, TR_LINEAR, TR_GRAVITY.
struct AbwurfTrajektorie {
    enum class Art : std::uint8_t { Steht, Linear, Schwer } art = Art::Steht;
    double zeit = 0.0;   // trTime
    NavVec basis{};      // trBase
    NavVec delta{};      // trDelta
    [[nodiscard]] NavVec bei(double t, float g) const;
    [[nodiscard]] NavVec deltaBei(double t, float g) const;   // EvaluateTrajectoryDelta
};

struct AbwurfBahn {
    // Pruefung 4a: s.pos und s.apos ab dem Serverbild ms (ein Eintrag je
    // Aenderung) - so wie der Schnappschuss sie cgame bringt.
    struct Punkt {
        double ms = 0.0;
        AbwurfTrajektorie pos;
        AbwurfTrajektorie apos;
    };
    std::vector<Punkt> punkte;
    float schwerkraft = 800.0F;   // g_gravity fuer TR_GRAVITY
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
    // Ort und Winkel zur Zeit ms: die Bahn des letzten Serverbilds bis ms,
    // ausgewertet zur Zeit ms (CG_CalcEntityLerpPositions, cg_ents.cpp OJ).
    [[nodiscard]] bool bei(double ms, NavVec& ort, NavVec& winkel) const;
};

// Drop_Item (g_items.cpp:1022 OJ): velocity = forward(Gier) * 150, z + 200 +
// flrand * 50; LaunchItem dreht eine Waffe auf (0, flrand * 180, 90). zufall
// sind die zwei Zuege von Q_flrand aus der Todeskette (TodErgebnis::abwurfZufall).
[[nodiscard]] AbwurfStart gegenstandAbwurf(const NavVec& ort, float gier, const float zufall[2], bool waffeDrehen);

// G_RunItem/G_BounceItem bis zum Stillstand (hoechstens grenzeMs nach dem Abwurf).
[[nodiscard]] AbwurfBahn wirfGegenstand(const AbwurfStart& s, const NavSpurFn& spur, double grenzeMs = 20000.0);

// WP_SaberLaunch/WP_SaberDrop/WP_SaberThrow/WP_RunSaber: das Schwert geht aus
// der Hand (ort = renderInfo.handRPoint, winkel = vectoangles(renderInfo.
// muzzleDir) fuer WP_SaberLaunch, drehTempo[1] nach FP_SABERTHROW: 600/800/1200)
// und verschwindet, sobald es liegt (Jedi Outcast: 5 s nach WP_SaberDrop).
[[nodiscard]] AbwurfBahn wirfSchwert(const AbwurfStart& s, const NavSpurFn& spur, const SchwertUmfeld& umfeld = {},
                                     double grenzeMs = 20000.0);

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
