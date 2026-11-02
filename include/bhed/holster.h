// holster.h - rc608 Teil 5: wo Movie Duels ein weggestecktes Schwert anhaengt.
//
// WP_SaberAddHolsteredG2SaberModels (wp_saber.cpp:1170-1650 MD), Vorgaben g_InvertedHolsteredSabers 0 und
// g_CannonHolsteredSabers 1. OpenJK und Jedi Outcast haben das nicht: dort verschwindet der Griff.
// Kein Griff hat den Bolzen *holsterorigin (alle .glm unter models/weapons2 in Movie Duels und JKA durchsucht):
// dieser Zweig von G_CreateG2HolsteredWeaponModel kommt nicht vor.
// Die Waffen im Holster: CG_HolsteredWeapons (cg_holster.cpp MD), cg_holsteredweapons 2.
#pragma once

#include <array>
#include <string>
#include <vector>

#include "bhed/gla.h"

namespace bhed {

struct HolsterLage {
    const char* bolzen = nullptr;   // Bolzen der Figur, nullptr: kein Griff
    BoneMatrix drehung{};           // BONE_ANGLES_PREMULT auf "ModView internal default" des Griffs
};

// saberTyp0/platz0: saber[0] (type, holsterPlace, leer: Vorgabe nach dem Typ), platz: saber[nr].
// huefte*Belegt: CG_HolsterHipTaken - WP_SaberHolsterAvoidGuns weicht nach vorn aus.
[[nodiscard]] bool saberHolsterLage(const std::string& npcTyp, const std::string& klasse, const std::string& saberTyp0, const std::string& platz0,
                                    const std::string& platz, int nr, HolsterLage& aus,
                                    bool huefteLinksBelegt = false, bool huefteRechtsBelegt = false);
// saber->holsterPlace, wenn die .sab keine nennt (wp_saberLoad.cpp:2994-3014 MD).
[[nodiscard]] std::string holsterPlatzVorgabe(const std::string& saberTyp);

// --- Die Waffen im Holster (cg_holster.cpp MD) ---------------------------------------------
inline constexpr int kHolsterTypen = 18;   // MAX_HOLSTER
struct WaffenHolsterEintrag {
    int knochen = 0;   // HLB_*: 0 keiner
    float pos[3]{};
    float ang[3]{};
};
struct HolsterCfg {
    std::array<WaffenHolsterEintrag, kHolsterTypen> typ{};
};
// Hls_Parse: Bloecke { holsterType HLR_* boneIndex HOLSTER_*|disabled posOffset "x, y, z" angOffset "p, y, r" }
void leseHolsterCfg(const std::string& text, HolsterCfg& aus);
// hlsPlaces: rechte Huefte, linke Huefte, Ruecken; leer: frei. Eine Waffe behaelt ihren Platz, solange die Figur sie hat.
using WaffenPlaetze = std::array<std::string, 3>;
void waffenPlaetzeAendern(WaffenPlaetze& plaetze, const HolsterCfg& cfg, const std::vector<std::string>& hat, const std::string& inHand, bool rueckenBelegt);
struct WaffeImHolster {
    std::string waffe;
    const char* bolzen = nullptr;   // hlsBoneBolt: Flaeche oder Knochen
    BoneMatrix lage{};              // Hls_Render: Achsen und Versatz im Raum des Bolzens
};
[[nodiscard]] std::vector<WaffeImHolster> waffenImHolster(const WaffenPlaetze& plaetze, const HolsterCfg& cfg, const std::string& inHand, bool rueckenBelegt);

}  // namespace bhed
