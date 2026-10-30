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
#include <vector>

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
    // Pruefung 3: flrand(min, max) (q_math.c:233-244 OpenJK, :226-238 MD)
    [[nodiscard]] float flrand(float lo, float hi) noexcept;

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

// Pruefung 3 - was player_die ausser der Animation tut, in der Reihenfolge
// des Spiels: Klaenge (G_AddEvent EV_DEATH1-3 -> CG_TryPlayCustomSound
// "*deathN.wav" auf CHAN_VOICE, cg_event.cpp:794-805 OJ; G_SoundOnEnt;
// G_Sound/G_SoundIndexOnEnt), DeathFX (G_PlayEffect) und was mit Waffe und
// Schwert geschieht.
struct TodKlang {
    // "*death2.wav": ein eigener Klang der Figur (CG_CustomSound, .npc "snd"),
    // sonst der Pfad wie im Spiel.
    std::string datei;
    enum class Kanal : std::uint8_t { Auto, Voice, VoiceAtten, Body } kanal = Kanal::Auto;
};
struct TodEffekt {
    std::string datei;   // G_PlayEffect("env/med_explode") - G_EffectIndex haengt .efx an
    NavVec ort{};
};
enum class TodWaffe : std::uint8_t {
    Bleibt,   // bleibt in der Hand (WP_BRYAR_PISTOL u. a.; ein Schwert, das nicht faellt)
    Weg,      // G_RemoveWeaponModels: die Waffe verschwindet aus der Hand
    Faellt,   // TossClientItems -> Drop_Item: sie liegt als Gegenstand (abwurf)
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
    // Pruefung 3:
    std::string waffe;             // s.weapon ("WP_SABER", ...; leer WP_NONE)
    bool klingeAn = false;         // ps.SaberActive()
    std::string schwertAusKlang;   // ps.saber[0].soundOff (.sab "soundOff")
    bool nichtEntwaffnbar = false; // ps.saber[0].saberFlags & SFL_NOT_DISARMABLE
    bool schwertImFlug = false;    // ps.saberInFlight
    std::string npcTyp;            // NPC_type
    std::string karte;             // mapname (Movie Duels TossClientItems)
    bool spielerTeam = false;      // client->playerTeam == TEAM_PLAYER (JK2: Klang beim Ausschalten)
    bool keinAbwurf = false;       // gi.pointcontents(currentOrigin) & CONTENTS_NODROP
    bool kotorWaffen = false;      // client->charKOTORWeapons (Movie Duels: nichts faellt)
    // WP_SaberLaunch (wp_saber.cpp OJ): renderInfo.handRPoint nicht im Festen
    // (Kasten 3) und vom Ursprung aus frei erreichbar. Leer: frei.
    std::function<bool()> handFrei;
};

struct TodErgebnis {
    std::string anim;          // leer: keine neue Animation (anim < 0)
    bool neuStart = false;     // SETANIM_FLAG_RESTART (Zuckung einer Leiche)
    bool timerNie = false;     // PM_SetLegsAnimTimer(-1) / PM_SetTorsoAnimTimer(-1)
    bool verschwindet = false; // Mark1 als Leiche: EF_NODRAW, G_FreeEntity
    int kante = 0;             // cliff_fall aus G_CheckLedgeDive
    NavVec tempo{};            // ps.velocity danach (Wurf ueber die Kante)
    // Pruefung 3:
    std::vector<TodKlang> klaenge;     // in der Reihenfolge des Spiels
    std::vector<TodEffekt> effekte;    // DeathFX
    TodWaffe waffe = TodWaffe::Bleibt;
    // TossClientItems -> WP_SaberLose: WP_SaberLaunch, WP_SaberDrop (Klinge
    // aus, soundOff am Schwert, TR_GRAVITY mit EF_BOUNCE_HALF) - das Schwert
    // faellt aus der Hand. Pruefung 4a: auch mit ausgeschalteter Klinge - dann
    // nur WP_SaberLaunch, WP_SaberDrop erst im naechsten Denkbild (WP_SaberThrow).
    bool schwertFaellt = false;
    bool schwertKlingeAus = false;
    bool kristall = false;             // Shadowtrooper: Drop_Item(FindItemForAmmo(AMMO_FORCE))
    // Die Zuege von Q_flrand(-1, 1) fuer Drop_Item (velocity[2] += 200 +
    // flrand * 50) und LaunchItem (Gier flrand * 180) der fallenden Waffe
    // bzw. des Kristalls, in dieser Folge.
    float abwurfZufall[3]{};
    bool aufprallTod = false;          // NPCAI_DIE_ON_IMPACT gesetzt
    float reibung = -1.0F;             // ps.friction danach (-1: unveraendert)
};

[[nodiscard]] TodErgebnis todesAnimation(const TodLage& lage, EngineZufall& zufall);

// Pruefung 3: vectoangles (q_math.c:616-654 OpenJK, atan2f/sqrtf).
void engineVectoangles(const float v[3], float winkel[3]) noexcept;

// Pruefung 3: renderInfo.torsoAngles aus dem chestBolt (cg_players.cpp:
// 7516-7522 OJ): G2API_GetBoltMatrix(ghoul2, playerModel, chestBolt,
// &boltMatrix, tempAngles, ent.origin, cg.time ...), dann
// G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Z, tempAxis) - die dritte
// Spalte, verneint - und vectoangles(tempAxis, torsoAngles). `bolzen` ist die
// Bolzenmatrix in der Welt (3x4, Spalten = Achsen, vierte = Ort).
void rumpfWinkelAusBolzen(const float bolzen[3][4], float winkel[3]) noexcept;

}  // namespace bhed
#endif
