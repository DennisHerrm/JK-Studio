// schmerz.h - G_Damage auf Figuren: Rueckstoss und Schmerz.
//
//   G_Damage (g_combat.cpp:7901 MD, OJ, JK2)  knockback = damage, G_ApplyKnockback vor dem Gottmodus
//   PlayerPain (:5897 MD)                     Animation nur bei Schwert, Faust und Quetschen
//   NPC_Pain, NPC_ChoosePainAnimation         NPC_reactions.cpp
//   G_MissileImpacted (g_missile.cpp:2056 MD) "Universal directional pain animation": nur Movie Duels,
//                                             auch im Gottmodus, 75 %, 2 s Pause, Torso 400 ms
//   G_PickPainAnim (g_combat.cpp:1844 MD)     Animation nach Trefferort
#ifndef BHED_SCHMERZ_H
#define BHED_SCHMERZ_H

#include "bhed/geschuetz.h"
#include "bhed/todesanim.h"

#include <string>
#include <vector>

namespace bhed {

// knockback = damage, DAMAGE_EXTRA_KNOCKBACK, hoechstens 200; MOD_CRUSH, DAMAGE_NO_KNOCKBACK: 0.
[[nodiscard]] float knockbackWert(int schaden, int dflags, bool quetschen);

// G_ApplyKnockback. richtung: normiert (VectorNormalize2). false: nichts (zu gross, kein Stoss).
struct StossErgebnis {
    float tempo[3]{};
    int pmZeit = 0;
};
[[nodiscard]] bool stossTempo(float knockback, const float richtung[3], const std::string& klasse, TodRegelwerk regel, float masse, float schwerkraft,
                              StossErgebnis& aus);

// G_PickPainAnim: leer heisst -1 (dann PM_PickAnim).
[[nodiscard]] std::string schmerzAnimNachOrt(int ort, EngineZufall& zufall);
// PM_PickAnim(self, BOTH_PAINvon, BOTH_PAINbis): eine, die das Modell hat (hat: die Namen aus dem Modell).
template <class Hat>
[[nodiscard]] std::string schmerzAnimZufall(int von, int bis, EngineZufall& zufall, Hat hat) {
    for (int versuch = 0; versuch < 64; ++versuch) {
        std::string n = "BOTH_PAIN" + std::to_string(zufall.irand(von, bis));
        if (hat(n)) { return n; }
    }
    return {};
}

// NPC_GetPainChance mit self->enemy == NULL ("surprised, always take pain") bzw. Ziehen nach Klasse.
[[nodiscard]] float npcSchmerzChance(const std::string& klasse, bool movieDuels);

// pitch_roll_for_slope (NPC.cpp:527 MD): Nick und Roll der Leiche aus der Normale unter ihr.
void leichenNeigung(const NavVec& normale, float gier, float& nick, float& roll);
// pitch_roll_for_slope (NPC.cpp): mins[2] = -24 + 12 * |nick| / 180. Der Boden des Kastens steigt, die Leiche sinkt um so viel.
[[nodiscard]] float leichenSenkung(float nick);
// DeadThink (NPC.cpp MD, OpenJK gleich): maxs[2] der Leiche aus renderInfo.eyePoint + 4, mindestens -8.
[[nodiscard]] float leichenKastenOben(float augeZ, float ursprungZ);
// ps.friction der Leiche: was player_die gesetzt hat (Absturz 1, Kantenwurf 0), sonst die 6 aus ClientThink.
[[nodiscard]] float leichenReibung(float todReibung, int kante);

// CG_PainEvent: der Klang zu Lebenspunkten (Spieler) oder Prozent (NPC_SetPainEvent).
[[nodiscard]] std::string schmerzKlang(int wert);

}  // namespace bhed
#endif
