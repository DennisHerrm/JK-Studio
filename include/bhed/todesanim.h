// todesanim.h - Pruefung 2: die Todesanimation wie im Spiel.
//
// kill aus ICARUS (CQuake3GameInterface::Kill, Q3_Interface.cpp:9661 OpenJK)
// setzt health auf 0 und ruft die Sterbefunktion mit GEntity_DieFunc(victim,
// NULL, NULL, o_health, MOD_UNKNOWN) - dflags 0, hitLoc HL_NONE
// (g_functions.h:735 MD). Fuer eine Figur ist das player_die (NPC_spawn.cpp:
// 1166 OpenJK / 1911 MD), und dort waehlt die Kette
//
//     player_die -> G_CheckSpecialDeathAnim -> G_CheckLedgeDive
//                -> G_PickDeathAnim -> G_GetHitLocation -> PM_PickAnim
//
// die Animation nach Haltung (Beinanimation und ihr Timer), Trefferort
// (pos1), Richtung (ps.velocity), Mittel und Klasse. Ein trigger_hurt toetet
// ueber G_Damage mit MOD_TRIGGER_HURT bzw. MOD_FALLING (hurt_touch,
// g_trigger.cpp:1126 OpenJK) und pos1 = currentOrigin (g_combat.cpp:6739).
//
// Hier steht diese Kette fuer OpenJK (code/game/g_combat.cpp), Movie Duels
// (MovieDuels-SJE-SP-Rend2 code/game/g_combat.cpp) und Jedi Outcast
// (codeJK2/game/g_combat.cpp) - jede Fassung, wie sie dort steht.
#ifndef BHED_TODESANIM_H
#define BHED_TODESANIM_H

#include "bhed/navlauf.h"

#include <cstdint>
#include <functional>
#include <string>

namespace bhed {

enum class TodRegelwerk : std::uint8_t { OpenJK, MovieDuels, JediOutcast };

// Q_irand der Engine: irand (q_math.c:248 OpenJK, :241 MD) - das LCG des
// VC-rand(), holdrand = holdrand * 214013 + 2531011, Ergebnis aus den Bits
// 17..31. Im Spiel setzt Rand_Init den Anfang aus der Uhr (InitGame), und
// jeder Zug irgendwo im Spiel schiebt die Folge weiter - welche Zahl ein kill
// zieht, ist nicht nachzurechnen. Die Vorschau zieht deshalb mit derselben
// Formel aus einer festen Saat je Tod (todesSaat), damit sie beim Spulen
// gleich bleibt - wie MoverSim je Bruch (mover.cpp, Wuerfel). Die Saat
// kommt aus der Nummer der Entity und der Zeit, nie aus ihrem Namen.
class EngineZufall {
public:
    explicit EngineZufall(std::uint32_t saat) noexcept : holdrand_(saat) {}
    // irand(min, max), beide eingeschlossen
    [[nodiscard]] int irand(int lo, int hi) noexcept;

private:
    std::uint32_t holdrand_;
};
[[nodiscard]] std::uint32_t todesSaat(int entity, double ms) noexcept;

// Das Mittel (meansOfDeath), soweit die Vorschau Tode kennt.
enum class TodMittel : std::uint8_t {
    Unbekannt,     // MOD_UNKNOWN: kill aus ICARUS
    TriggerHurt,   // MOD_TRIGGER_HURT: trigger_hurt
    Fallen,        // MOD_FALLING: trigger_hurt mit FALLING (spawnflags 32)
};

// Die Lage der Figur im Augenblick des Todes - was player_die von `self` liest.
struct TodLage {
    TodRegelwerk regel = TodRegelwerk::OpenJK;
    std::string klasse;          // client->NPC_class ("CLASS_...", gross)
    bool spieler = false;        // s.number == 0 (< MAX_CLIENTS)
    bool schluessel = false;     // self->message (Schluessel am Arm, NPC_Spawn_Do)
    bool schonTot = false;       // ps.pm_type == PM_DEAD (ein frueherer Tod)
    std::string beine;           // ps.legsAnim
    std::string torso;           // ps.torsoAnim
    double beineTimer = 0.0;     // ps.legsAnimTimer in ms, -1 nie
    double beineSeitMs = 0.0;    // seit Beginn der Beinanimation (PM_FinishedCurrentLegsAnim)
    float gier = 0.0F;           // currentAngles[YAW] = ps.viewangles[YAW]
    NavVec tempo{};              // ps.velocity
    bool amBoden = true;         // ps.groundEntityNum != ENTITYNUM_NONE
    int pmZeit = 0;              // ps.pm_time (G_Throw setzt PMF_TIME_KNOCKBACK nur bei 0)
    NavVec ort{};                // currentOrigin
    NavVec mins{-16.0F, -16.0F, -24.0F};
    NavVec maxs{16.0F, 16.0F, 40.0F};
    NavVec treffpunkt{};         // pos1; vec3_origin heisst keiner (G_GetHitLocation)
    NavVec rumpfWinkel{};        // renderInfo.torsoAngles
    TodMittel mittel = TodMittel::Unbekannt;
    bool geschockt = false;      // s.powerups & (1 << PW_SHOCKED)
    int schaden = 0;             // damage (kill: o_health)
    int maxGesundheit = 100;     // max_health
    float physicsBounce = 0.0F;  // G_Throw: die Masse, 0 = 200
    bool keinRueckstoss = false; // MD: charNoKnockback (.npc "noKnockback", NPC_stats.cpp:4115 MD)
    int entity = kNavKeineEntity;   // s.number fuer die Spuren (passEntityNum)
    // PM_HasAnimation, PM_AnimLength (numFrames * |frameLerp|) und frameLerp
    // der Animationsdatei der Figur.
    std::function<bool(const std::string&)> hat;
    std::function<double(const std::string&)> laenge;
    std::function<double(const std::string&)> bild;
    // gi.trace fuer G_CheckForLedge. Leer: nichts im Weg (wie NavWelt::spur).
    NavSpurFn spur;
};

struct TodErgebnis {
    std::string anim;          // leer: keine neue Animation (anim < 0)
    bool neuStart = false;     // SETANIM_FLAG_RESTART (Zuckung einer Leiche)
    bool timerNie = false;     // PM_SetLegsAnimTimer(-1) / PM_SetTorsoAnimTimer(-1)
    bool verschwindet = false; // Mark1 als Leiche: EF_NODRAW, G_FreeEntity
    int kante = 0;             // cliff_fall aus G_CheckLedgeDive
    NavVec tempo{};            // ps.velocity danach (Wurf ueber die Kante)
};

[[nodiscard]] TodErgebnis todesAnimation(const TodLage& lage, EngineZufall& zufall);

}  // namespace bhed
#endif
