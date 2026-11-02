// holster.h - rc608 Teil 5: wo Movie Duels ein weggestecktes Schwert anhaengt.
//
// WP_SaberAddHolsteredG2SaberModels (wp_saber.cpp:1170-1650 MD), Vorgaben g_InvertedHolsteredSabers 0 und
// g_CannonHolsteredSabers 1. OpenJK und Jedi Outcast haben das nicht: dort verschwindet der Griff.
// Die Waffen im Holster (cg_holster.cpp) sind nicht dabei, darum auch nicht WP_SaberHolsterAvoidGuns'
// Ausweichen nach vorn.
#pragma once

#include <string>

#include "bhed/gla.h"

namespace bhed {

struct HolsterLage {
    const char* bolzen = nullptr;   // Bolzen der Figur, nullptr: kein Griff
    BoneMatrix drehung{};           // BONE_ANGLES_PREMULT auf "ModView internal default" des Griffs
};

// saberTyp0/platz0: saber[0] (type, holsterPlace, leer: Vorgabe nach dem Typ), platz: saber[nr].
[[nodiscard]] bool saberHolsterLage(const std::string& npcTyp, const std::string& klasse, const std::string& saberTyp0, const std::string& platz0,
                                    const std::string& platz, int nr, HolsterLage& aus);
// saber->holsterPlace, wenn die .sab keine nennt (wp_saberLoad.cpp:2994-3014 MD).
[[nodiscard]] std::string holsterPlatzVorgabe(const std::string& saberTyp);

}  // namespace bhed
