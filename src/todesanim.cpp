// todesanim.cpp - Pruefung 2: die Todesanimation wie im Spiel (bhed/todesanim.h).
//
// Quellen (je Fassung, wie sie dort steht):
//   OpenJK      code/game/g_combat.cpp, bg_pmove.cpp, bg_panimate.cpp,
//               wp_saber.cpp (G_Throw), g_utils.cpp (vectoyaw), AI_Mark1.cpp,
//               AI_Interrogator.cpp
//   Movie Duels MovieDuels-SJE-SP-Rend2 code/game/ dieselben Dateien
//   Jedi Outcast codeJK2/game/ dieselben Dateien
// Zeilen stehen an den Funktionen (OJ = OpenJK, MD = Movie Duels, JK2 =
// codeJK2).
#include "bhed/todesanim.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string_view>

namespace bhed {

// irand (q_math.c:248-260 OpenJK, :241-267 MD): max++, holdrand = holdrand *
// 214013 + 2531011, result = holdrand >> 17, ((result * (max - min)) >> 15) + min.
int EngineZufall::irand(int lo, int hi) noexcept {
    const int max = hi + 1;
    holdrand_ = (holdrand_ * 214013U) + 2531011U;
    const auto ergebnis = static_cast<int>(holdrand_ >> 17U);
    return ((ergebnis * (max - lo)) >> 15) + lo;
}

// flrand (q_math.c:233-244 OpenJK): holdrand wie irand, result = holdrand >>
// 17, ( max - min ) * (float)result / 32768.0f + min.
float EngineZufall::flrand(float lo, float hi) noexcept {
    holdrand_ = (holdrand_ * 214013U) + 2531011U;
    const auto ergebnis = static_cast<int>(holdrand_ >> 17U);
    return (hi - lo) * static_cast<float>(ergebnis) / 32768.0F + lo;
}

// FNV-1a ueber die Nummer der Entity und die Todeszeit in ms.
std::uint32_t todesSaat(int entity, double ms) noexcept {
    std::uint32_t h = 2166136261U;
    const auto dazu = [&h](std::uint64_t wert) {
        for (int i = 0; i < 8; ++i) {
            h ^= static_cast<std::uint32_t>((wert >> (8 * i)) & 0xFFU);
            h *= 16777619U;
        }
    };
    dazu(static_cast<std::uint64_t>(static_cast<std::int64_t>(entity)));
    dazu(static_cast<std::uint64_t>(std::llround(ms)));
    return h;
}

namespace {

// Hockhaltungen: PM_CrouchAnim (bg_pmove.cpp:6713 OpenJK, bg_pmove.cpp:9850 MD, codeJK2 bg_pmove.cpp:3942)
constexpr const char* kHockenOj[] = {"BOTH_SIT1", "BOTH_SIT2", "BOTH_SIT3", "BOTH_CROUCH1", "BOTH_CROUCH1IDLE",
    "BOTH_CROUCH1WALK", "BOTH_CROUCH1WALKBACK", "BOTH_CROUCH2TOSTAND1", "BOTH_CROUCH3", "BOTH_KNEES1",
    "BOTH_CROUCHATTACKBACK1", "BOTH_ROLL_STAB", "BOTH_STAND_TO_KNEEL", "BOTH_KNEEL_TO_STAND", "BOTH_TURNCROUCH1",
    "BOTH_CROUCH4", "BOTH_KNEES2", "BOTH_KNEES2TO1"};
constexpr const char* kHockenMd[] = {"BOTH_SIT1", "BOTH_SIT2", "BOTH_SIT3", "BOTH_CROUCH1", "BOTH_CROUCH1IDLE",
    "BOTH_CROUCH1WALK", "BOTH_CROUCH1WALKBACK", "BOTH_CROUCH2", "BOTH_CROUCH2TOSTAND1", "BOTH_CROUCH3", "BOTH_KNEES1",
    "BOTH_CROUCHATTACKBACK1", "BOTH_ROLL_STAB", "BOTH_STAND_TO_KNEEL", "BOTH_KNEEL_TO_STAND", "BOTH_TURNCROUCH1",
    "BOTH_CROUCH4", "BOTH_KNEES2", "BOTH_KNEES2TO1"};
constexpr const char* kHockenJk[] = {"BOTH_SIT1", "BOTH_SIT2", "BOTH_SIT3", "BOTH_SIT2TO3", "BOTH_SIT3TO1",
    "BOTH_SIT3TO2", "BOTH_SIT4TO5", "BOTH_SIT4TO6", "BOTH_SIT5TO4", "BOTH_SIT5TO6", "BOTH_SIT6TO4", "BOTH_SIT6TO5",
    "BOTH_SIT7", "BOTH_CROUCH1", "BOTH_CROUCH1IDLE", "BOTH_CROUCH1WALK", "BOTH_CROUCH1WALKBACK", "BOTH_CROUCH2IDLE",
    "BOTH_CROUCH2TOSTAND1", "BOTH_CROUCH3", "BOTH_KNEES1", "LEGS_CRLEAN_LEFT1", "LEGS_CRLEAN_RIGHT1",
    "BOTH_CROUCHATTACKBACK1"};
// Saltos: PM_FlippingAnim (bg_pmove.cpp:6877 OpenJK, bg_pmove.cpp:10235 MD, codeJK2 bg_pmove.cpp:4075)
constexpr const char* kSaltoOj[] = {"BOTH_FLIP_F", "BOTH_FLIP_B", "BOTH_FLIP_L", "BOTH_FLIP_R", "BOTH_ALORA_FLIP_1",
    "BOTH_ALORA_FLIP_2", "BOTH_ALORA_FLIP_3", "BOTH_WALL_RUN_RIGHT_FLIP", "BOTH_WALL_RUN_LEFT_FLIP",
    "BOTH_WALL_FLIP_RIGHT", "BOTH_WALL_FLIP_LEFT", "BOTH_FLIP_BACK1", "BOTH_FLIP_BACK2", "BOTH_FLIP_BACK3",
    "BOTH_WALL_FLIP_BACK1", "BOTH_ALORA_FLIP_B", "BOTH_WALL_RUN_RIGHT", "BOTH_WALL_RUN_LEFT",
    "BOTH_WALL_RUN_RIGHT_STOP", "BOTH_WALL_RUN_LEFT_STOP", "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT",
    "BOTH_BUTTERFLY_FL1", "BOTH_BUTTERFLY_FR1", "BOTH_ARIAL_LEFT", "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1",
    "BOTH_CARTWHEEL_LEFT", "BOTH_CARTWHEEL_RIGHT", "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN",
    "BOTH_JUMPATTACK6", "BOTH_JUMPATTACK7", "BOTH_FORCEWALLRUNFLIP_END", "BOTH_FORCEWALLRUNFLIP_ALT",
    "BOTH_FLIP_ATTACK7", "BOTH_A7_SOULCAL"};
constexpr const char* kSaltoMd[] = {"BOTH_FLIP_F", "BOTH_FLIP_B", "BOTH_FLIP_L", "BOTH_FLIP_R", "BOTH_FLIP_B_ANI",
    "BOTH_FLIP_B_GALEN", "BOTH_FLIP_F_ANI", "BOTH_FLIP_F_GALEN", "BOTH_FLIP_L_ANI", "BOTH_FLIP_L_GALEN",
    "BOTH_FLIP_R_ANI", "BOTH_FLIP_R_GALEN", "BOTH_ALORA_FLIP_1", "BOTH_ALORA_FLIP_2", "BOTH_ALORA_FLIP_3",
    "BOTH_WALL_RUN_RIGHT_FLIP", "BOTH_WALL_RUN_LEFT_FLIP", "BOTH_WALL_FLIP_RIGHT", "BOTH_WALL_FLIP_LEFT",
    "BOTH_FLIP_BACK1", "BOTH_FLIP_BACK2", "BOTH_FLIP_BACK3", "BOTH_WALL_FLIP_BACK1", "BOTH_ALORA_FLIP_B",
    "BOTH_WALL_RUN_RIGHT", "BOTH_WALL_RUN_LEFT", "BOTH_WALL_RUN_RIGHT_STOP", "BOTH_WALL_RUN_LEFT_STOP",
    "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_BUTTERFLY_FL1", "BOTH_BUTTERFLY_FR1", "BOTH_ARIAL_LEFT",
    "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1", "BOTH_CARTWHEEL_LEFT", "BOTH_CARTWHEEL_RIGHT", "BOTH_JUMPFLIPSLASHDOWN1",
    "BOTH_JUMPFLIPSTABDOWN", "BOTH_JUMPATTACK6", "BOTH_JUMPATTACK6_GRIEV", "BOTH_JUMPATTACK7",
    "BOTH_FORCEWALLRUNFLIP_END", "BOTH_FORCEWALLRUNFLIP_ALT", "BOTH_FLIP_ATTACK7", "BOTH_A7_SOULCAL"};
constexpr const char* kSaltoJk[] = {"BOTH_FLIP_F", "BOTH_FLIP_B", "BOTH_FLIP_L", "BOTH_FLIP_R",
    "BOTH_WALL_RUN_RIGHT_FLIP", "BOTH_WALL_RUN_LEFT_FLIP", "BOTH_WALL_FLIP_RIGHT", "BOTH_WALL_FLIP_LEFT",
    "BOTH_FLIP_BACK1", "BOTH_FLIP_BACK2", "BOTH_FLIP_BACK3", "BOTH_WALL_FLIP_BACK1", "BOTH_WALL_RUN_RIGHT",
    "BOTH_WALL_RUN_LEFT", "BOTH_WALL_RUN_RIGHT_STOP", "BOTH_WALL_RUN_LEFT_STOP", "BOTH_BUTTERFLY_LEFT",
    "BOTH_BUTTERFLY_RIGHT", "BOTH_ARIAL_LEFT", "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1", "BOTH_CARTWHEEL_LEFT",
    "BOTH_CARTWHEEL_RIGHT", "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN"};
// Drehangriffe (PM_SpinningAnim): PM_SpinningSaberAnim (bg_pmove.cpp:7022 OpenJK, bg_pmove.cpp:10867 MD, codeJK2 bg_pmove.cpp:4184)
constexpr const char* kDrehungOj[] = {"BOTH_T1_BR_BL", "BOTH_T1__R__L", "BOTH_T1__R_BL", "BOTH_T1_TR_BL",
    "BOTH_T1_BR_TL", "BOTH_T1_BR__L", "BOTH_T1_TL_BR", "BOTH_T1__L_BR", "BOTH_T1__L__R", "BOTH_T1_BL_BR",
    "BOTH_T1_BL__R", "BOTH_T1_BL_TR", "BOTH_T2_BR__L", "BOTH_T2_BR_BL", "BOTH_T2__R_BL", "BOTH_T2__L_BR",
    "BOTH_T2_BL_BR", "BOTH_T2_BL__R", "BOTH_T3_BR__L", "BOTH_T3_BR_BL", "BOTH_T3__R_BL", "BOTH_T3__L_BR",
    "BOTH_T3_BL_BR", "BOTH_T3_BL__R", "BOTH_T4_BR__L", "BOTH_T4_BR_BL", "BOTH_T4__R_BL", "BOTH_T4__L_BR",
    "BOTH_T4_BL_BR", "BOTH_T4_BL__R", "BOTH_T5_BR_BL", "BOTH_T5__R__L", "BOTH_T5__R_BL", "BOTH_T5_TR_BL",
    "BOTH_T5_BR_TL", "BOTH_T5_BR__L", "BOTH_T5_TL_BR", "BOTH_T5__L_BR", "BOTH_T5__L__R", "BOTH_T5_BL_BR",
    "BOTH_T5_BL__R", "BOTH_T5_BL_TR", "BOTH_T6_BR_TL", "BOTH_T6__R_TL", "BOTH_T6__R__L", "BOTH_T6__R_BL",
    "BOTH_T6_TR_TL", "BOTH_T6_TR__L", "BOTH_T6_TR_BL", "BOTH_T6_T__TL", "BOTH_T6_T__BL", "BOTH_T6_TL_BR",
    "BOTH_T6__L_BR", "BOTH_T6__L__R", "BOTH_T6_TL__R", "BOTH_T6_TL_TR", "BOTH_T6__L_TR", "BOTH_T6__L_T_",
    "BOTH_T6_BL_T_", "BOTH_T6_BR__L", "BOTH_T6_BR_BL", "BOTH_T6_BL_BR", "BOTH_T6_BL__R", "BOTH_T6_BL_TR",
    "BOTH_T7_BR_TL", "BOTH_T7_BR__L", "BOTH_T7_BR_BL", "BOTH_T7__R__L", "BOTH_T7__R_BL", "BOTH_T7_TR__L",
    "BOTH_T7_T___R", "BOTH_T7_TL_BR", "BOTH_T7__L_BR", "BOTH_T7__L__R", "BOTH_T7_BL_BR", "BOTH_T7_BL__R",
    "BOTH_T7_BL_TR", "BOTH_T7_TL_TR", "BOTH_T7_T__BR", "BOTH_T7__L_TR", "BOTH_V7_BL_S7", "BOTH_ATTACK_BACK",
    "BOTH_CROUCHATTACKBACK1", "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_FJSS_TR_BL", "BOTH_FJSS_TL_BR",
    "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN"};
constexpr const char* kDrehungMd[] = {"BOTH_T1_BR_BL", "BOTH_T1__R__L", "BOTH_T1__R_BL", "BOTH_T1_TR_BL",
    "BOTH_T1_BR_TL", "BOTH_T1_BR__L", "BOTH_T1_TL_BR", "BOTH_T1__L_BR", "BOTH_T1__L__R", "BOTH_T1_BL_BR",
    "BOTH_T1_BL__R", "BOTH_T1_BL_TR", "BOTH_T2_BR__L", "BOTH_T2_BR_BL", "BOTH_T2__R_BL", "BOTH_T2__L_BR",
    "BOTH_T2_BL_BR", "BOTH_T2_BL__R", "BOTH_T3_BR__L", "BOTH_T3_BR_BL", "BOTH_T3__R_BL", "BOTH_T3__L_BR",
    "BOTH_T3_BL_BR", "BOTH_T3_BL__R", "BOTH_T4_BR__L", "BOTH_T4_BR_BL", "BOTH_T4__R_BL", "BOTH_T4__L_BR",
    "BOTH_T4_BL_BR", "BOTH_T4_BL__R", "BOTH_T5_BR_BL", "BOTH_T5__R__L", "BOTH_T5__R__L_YODA", "BOTH_T5__R_BL",
    "BOTH_T5_TR_BL", "BOTH_T5_BR_TL", "BOTH_T5_BR_TL_YODA", "BOTH_T5_BR__L", "BOTH_T5_TL_BR", "BOTH_T5_TL_BR_YODA",
    "BOTH_T5__L_BR", "BOTH_T5__L__R", "BOTH_T5__L__R_YODA", "BOTH_T5_BL_BR", "BOTH_T5_BL__R", "BOTH_T5_BL__R_YODA",
    "BOTH_T5_BL_TR", "BOTH_T5_BL_TR_YODA", "BOTH_T6_BR_TL", "BOTH_T6__R_TL", "BOTH_T6__R__L", "BOTH_T6__R_BL",
    "BOTH_T6_TR_TL", "BOTH_T6_TR__L", "BOTH_T6_TR_BL", "BOTH_T6_T__TL", "BOTH_T6_T__BL", "BOTH_T6_TL_BR",
    "BOTH_T6__L_BR", "BOTH_T6__L__R", "BOTH_T6_TL__R", "BOTH_T6_TL_TR", "BOTH_T6__L_TR", "BOTH_T6__L_T_",
    "BOTH_T6_BL_T_", "BOTH_T6_BR__L", "BOTH_T6_BR_BL", "BOTH_T6_BL_BR", "BOTH_T6_BL__R", "BOTH_T6_BL_TR",
    "BOTH_T7_BR_TL", "BOTH_T7_BR__L", "BOTH_T7_BR_BL", "BOTH_T7__R__L", "BOTH_T7__R_BL", "BOTH_T7_TR__L",
    "BOTH_T7_T___R", "BOTH_T7_TL_BR", "BOTH_T7__L_BR", "BOTH_T7__L__R", "BOTH_T7_BL_BR", "BOTH_T7_BL__R",
    "BOTH_T7_BL_TR", "BOTH_T7_TL_TR", "BOTH_T7_T__BR", "BOTH_T7__L_TR", "BOTH_V7_BL_S7", "BOTH_ATTACK_BACK",
    "BOTH_CROUCHATTACKBACK1", "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_FJSS_TR_BL", "BOTH_FJSS_TL_BR",
    "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN"};
constexpr const char* kDrehungJk[] = {"BOTH_T1_BR_BL", "BOTH_T1__R__L", "BOTH_T1__R_BL", "BOTH_T1_TR_BL",
    "BOTH_T1_BR_TL", "BOTH_T1_BR__L", "BOTH_T1_TL_BR", "BOTH_T1__L_BR", "BOTH_T1__L__R", "BOTH_T1_BL_BR",
    "BOTH_T1_BL__R", "BOTH_T1_BL_TR", "BOTH_T2_BR__L", "BOTH_T2_BR_BL", "BOTH_T2__R_BL", "BOTH_T2__L_BR",
    "BOTH_T2_BL_BR", "BOTH_T2_BL__R", "BOTH_T3_BR__L", "BOTH_T3_BR_BL", "BOTH_T3__R_BL", "BOTH_T3__L_BR",
    "BOTH_T3_BL_BR", "BOTH_T3_BL__R", "BOTH_T4_BR__L", "BOTH_T4_BR_BL", "BOTH_T4__R_BL", "BOTH_T4__L_BR",
    "BOTH_T4_BL_BR", "BOTH_T4_BL__R", "BOTH_T5_BR_BL", "BOTH_T5__R__L", "BOTH_T5__R_BL", "BOTH_T5_TR_BL",
    "BOTH_T5_BR_TL", "BOTH_T5_BR__L", "BOTH_T5_TL_BR", "BOTH_T5__L_BR", "BOTH_T5__L__R", "BOTH_T5_BL_BR",
    "BOTH_T5_BL__R", "BOTH_T5_BL_TR", "BOTH_ATTACK_BACK", "BOTH_CROUCHATTACKBACK1", "BOTH_BUTTERFLY_LEFT",
    "BOTH_BUTTERFLY_RIGHT", "BOTH_FJSS_TR_BL", "BOTH_FJSS_TL_BR", "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN"};
// G_PickDeathAnim: schon in einer Todesanimation -> -2 (g_combat.cpp:3149-3196 OJ, MD gleich)
constexpr const char* kSchonTotOj[] = {"BOTH_DEAD3", "BOTH_DEAD4", "BOTH_DEAD5", "BOTH_DEAD6", "BOTH_DEAD7",
    "BOTH_DEAD9", "BOTH_DEAD11", "BOTH_DEAD12", "BOTH_DEAD17", "BOTH_DEAD18", "BOTH_DEAD19", "BOTH_DEAD20",
    "BOTH_DEAD21", "BOTH_DEAD22", "BOTH_DEAD23", "BOTH_DEAD24", "BOTH_DEAD25", "BOTH_LYINGDEAD1", "BOTH_STUMBLEDEAD1",
    "BOTH_FALLDEAD1LAND", "BOTH_DEATH3", "BOTH_DEATH4", "BOTH_DEATH5", "BOTH_DEATH6", "BOTH_DEATH7", "BOTH_DEATH9",
    "BOTH_DEATH11", "BOTH_DEATH12", "BOTH_DEATH17", "BOTH_DEATH18", "BOTH_DEATH19", "BOTH_DEATH20", "BOTH_DEATH21",
    "BOTH_DEATH22", "BOTH_DEATH23", "BOTH_DEATH24", "BOTH_DEATH25", "BOTH_DEATHFORWARD1", "BOTH_DEATHFORWARD2",
    "BOTH_DEATHFORWARD3", "BOTH_DEATHBACKWARD1", "BOTH_DEATHBACKWARD2", "BOTH_DEATH1IDLE", "BOTH_LYINGDEATH1",
    "BOTH_STUMBLEDEATH1", "BOTH_FALLDEATH1", "BOTH_FALLDEATH1INAIR", "BOTH_FALLDEATH1LAND", "BOTH_DEATH_ROLL",
    "BOTH_DEATH_FLIP", "BOTH_DEATH_SPIN_90_R", "BOTH_DEATH_SPIN_90_L", "BOTH_DEATH_SPIN_180", "BOTH_DEATH_LYING_UP",
    "BOTH_DEATH_LYING_DN", "BOTH_DEATH_FALLING_DN", "BOTH_DEATH_FALLING_UP", "BOTH_DEATH_CROUCHED",
    "BOTH_RIGHTHANDCHOPPEDOFF"};
// G_PickDeathAnim: jede Todes- und Liegeanimation -> -2 (codeJK2 g_combat.cpp:2844-2962)
constexpr const char* kSchonTotJk[] = {"BOTH_DEATH1", "BOTH_DEAD1", "BOTH_DEATH2", "BOTH_DEAD2", "BOTH_DEATH8",
    "BOTH_DEAD8", "BOTH_DEATH13", "BOTH_DEAD13", "BOTH_DEATH14", "BOTH_DEAD14", "BOTH_DEATH16", "BOTH_DEAD16",
    "BOTH_DEADBACKWARD1", "BOTH_DEADBACKWARD2", "BOTH_DEATH10", "BOTH_DEAD10", "BOTH_DEATH15", "BOTH_DEAD15",
    "BOTH_DEADFORWARD1", "BOTH_DEADFORWARD2", "BOTH_DEADFLOP1", "BOTH_DEAD3", "BOTH_DEAD4", "BOTH_DEAD5",
    "BOTH_DEAD6", "BOTH_DEAD7", "BOTH_DEAD9", "BOTH_DEAD11", "BOTH_DEAD12", "BOTH_DEAD17", "BOTH_DEAD18",
    "BOTH_DEAD19", "BOTH_DEAD20", "BOTH_DEAD21", "BOTH_DEAD22", "BOTH_DEAD23", "BOTH_DEAD24", "BOTH_DEAD25",
    "BOTH_LYINGDEAD1", "BOTH_STUMBLEDEAD1", "BOTH_FALLDEAD1LAND", "BOTH_DEATH3", "BOTH_DEATH4", "BOTH_DEATH5",
    "BOTH_DEATH6", "BOTH_DEATH7", "BOTH_DEATH9", "BOTH_DEATH11", "BOTH_DEATH12", "BOTH_DEATH17", "BOTH_DEATH18",
    "BOTH_DEATH19", "BOTH_DEATH20", "BOTH_DEATH21", "BOTH_DEATH22", "BOTH_DEATH23", "BOTH_DEATH24", "BOTH_DEATH25",
    "BOTH_DEATHFORWARD1", "BOTH_DEATHFORWARD2", "BOTH_DEATHFORWARD3", "BOTH_DEATHBACKWARD1", "BOTH_DEATHBACKWARD2",
    "BOTH_DEATH1IDLE", "BOTH_LYINGDEATH1", "BOTH_STUMBLEDEATH1", "BOTH_FALLDEATH1", "BOTH_FALLDEATH1INAIR",
    "BOTH_FALLDEATH1LAND", "BOTH_DEATH_ROLL", "BOTH_DEATH_FLIP", "BOTH_DEATH_SPIN_90_R", "BOTH_DEATH_SPIN_90_L",
    "BOTH_DEATH_SPIN_180", "BOTH_DEATH_LYING_UP", "BOTH_DEATH_LYING_DN", "BOTH_DEATH_FALLING_DN",
    "BOTH_DEATH_FALLING_UP", "BOTH_DEATH_CROUCHED", "BOTH_RIGHTHANDCHOPPEDOFF"};
// PM_InOnGroundAnim: was Jedi Outcast dazu als liegend zaehlt (codeJK2 bg_panimate.cpp:3865)
constexpr const char* kLiegenJk[] = {"BOTH_PAIN2WRITHE1", "BOTH_WRITHING1", "BOTH_WRITHING1RLEG",
    "BOTH_WRITHING1LLEG", "BOTH_WRITHING2", "BOTH_INJURED1", "BOTH_CRAWLBACK1", "BOTH_INJURED2", "BOTH_INJURED3",
    "BOTH_INJURED6", "BOTH_INJURED6ATTACKSTART", "BOTH_INJURED6ATTACKSTOP", "BOTH_INJURED6COMBADGE",
    "BOTH_INJURED6POINT", "BOTH_SLEEP2", "BOTH_SLEEP5", "BOTH_SLEEP_IDLE1", "BOTH_SLEEP_IDLE2", "BOTH_SLEEP1_NOSE",
    "BOTH_SLEEP2_SHIFT"};

template <std::size_t N>
bool inListe(const std::string& a, const char* const (&liste)[N]) {
    for (const char* n : liste) {
        if (a == n) { return true; }
    }
    return false;
}
bool ist(const std::string& a, std::initializer_list<std::string_view> liste) {
    for (const std::string_view n : liste) {
        if (a == n) { return true; }
    }
    return false;
}

// -2 aus G_PickDeathAnim: schon in einer Todesanimation.
constexpr const char* kSchon = "#-2";

// hitLocation_t (hitlocs.h)
enum Treffer : std::uint8_t {
    HL_NONE, HL_FOOT_RT, HL_FOOT_LT, HL_LEG_RT, HL_LEG_LT, HL_WAIST, HL_BACK_RT, HL_BACK_LT, HL_BACK,
    HL_CHEST_RT, HL_CHEST_LT, HL_CHEST, HL_ARM_RT, HL_ARM_LT, HL_HAND_RT, HL_HAND_LT, HL_HEAD
};

constexpr float kKnockback = 1000.0F;   // g_knockback (g_main.cpp:632 OJ, :753 MD, JK2 :572)
constexpr float kGrad = 3.14159265358979323846F * 2.0F / 360.0F;

// AngleNormalize360/180 (q_math.c:511-527)
float winkel360(float w) { return (360.0F / 65536) * static_cast<float>(static_cast<int>(w * (65536 / 360.0F)) & 65535); }
float winkel180(float w) {
    w = winkel360(w);
    if (w > 180.0F) { w -= 360.0F; }
    return w;
}
// AngleVectors mit PITCH = ROLL = 0 (player_die setzt currentAngles[PITCH/ROLL]
// auf 0, g_combat.cpp:4125-4126 OJ): forward = (cy, sy, 0), right = (sy, -cy, 0).
NavVec vorwaerts(float gier) {
    const float w = gier * kGrad;
    return {std::cos(w), std::sin(w), 0.0F};
}
float punkt(const NavVec& a, const NavVec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
// VectorNormalize: Laenge, und der Vektor durch sie (0 bleibt 0).
float normiere(NavVec& v) {
    const float l = std::sqrt(punkt(v, v));
    if (l != 0.0F) {
        const float il = 1.0F / l;
        for (float& x : v) { x *= il; }
    }
    return l;
}
// vectoyaw (g_utils.cpp OJ/MD/JK2)
float vectoyaw(const NavVec& v) {
    float yaw = 0.0F;
    if (v[1] == 0.0F && v[0] == 0.0F) { return 0.0F; }
    if (v[0] != 0.0F) {
        yaw = static_cast<float>(std::atan2(v[1], v[0]) * 180.0 / 3.14159265358979323846);
    } else if (v[1] > 0.0F) {
        yaw = 90.0F;
    } else {
        yaw = 270.0F;
    }
    if (yaw < 0.0F) { yaw += 360.0F; }
    return yaw;
}

class Kette {
public:
    Kette(const TodLage& l, EngineZufall& z) : l_(l), z_(z), tempo_(l.tempo), pmZeit_(l.pmZeit) {}
    TodErgebnis lauf();

private:
    const TodLage& l_;
    EngineZufall& z_;
    NavVec tempo_;          // ps.velocity
    int pmZeit_ = 0;        // ps.pm_time
    bool rueckstoss_ = false;   // ps.pm_flags & PMF_TIME_KNOCKBACK

    [[nodiscard]] bool oj() const { return l_.regel == TodRegelwerk::OpenJK; }
    [[nodiscard]] bool md() const { return l_.regel == TodRegelwerk::MovieDuels; }
    [[nodiscard]] bool jk() const { return l_.regel == TodRegelwerk::JediOutcast; }
    int qi(int a, int b) { return z_.irand(a, b); }
    // Q_flrand (q_math.c:233-244 OpenJK): dasselbe holdrand, Bits 17..31 als
    // Bruchteil von 32768.
    float qf(float a, float b) { return z_.flrand(a, b); }
    // PM_HasAnimation (bg_panimate.cpp:4325 OJ): eine gueltige Nummer mit Bildern.
    [[nodiscard]] bool hat(const std::string& a) const { return !a.empty() && a != kSchon && l_.hat && l_.hat(a); }
    [[nodiscard]] static bool gesetzt(const std::string& a) { return !a.empty() && a != kSchon; }   // anim >= 0
    // PM_AnimLength: numFrames * abs(frameLerp), als int
    [[nodiscard]] int animLaenge(const std::string& a) const { return l_.laenge ? static_cast<int>(l_.laenge(a)) : 0; }
    [[nodiscard]] bool klasse(std::string_view k) const { return l_.klasse == k; }
    [[nodiscard]] double timer() const { return l_.beineTimer; }
    // float thrown = DotProduct( fwd, self->client->ps.velocity )
    [[nodiscard]] float wurf() const { return punkt(vorwaerts(l_.gier), tempo_); }

    // --- Haltungen (bg_pmove.cpp / bg_panimate.cpp) ------------------------
    // PM_InForceGetUp (bg_pmove.cpp:5925 OJ, :8382 MD, JK2 :3654)
    [[nodiscard]] bool inForceGetUp() const {
        const std::string& b = l_.beine;
        const bool liste = ist(b, {"BOTH_FORCE_GETUP_F1", "BOTH_FORCE_GETUP_F2", "BOTH_FORCE_GETUP_B1", "BOTH_FORCE_GETUP_B2",
                                   "BOTH_FORCE_GETUP_B3", "BOTH_FORCE_GETUP_B4", "BOTH_FORCE_GETUP_B5", "BOTH_FORCE_GETUP_B6"}) ||
                           (!jk() && ist(b, {"BOTH_GETUP_BROLL_B", "BOTH_GETUP_BROLL_F", "BOTH_GETUP_BROLL_L", "BOTH_GETUP_BROLL_R",
                                             "BOTH_GETUP_FROLL_B", "BOTH_GETUP_FROLL_F", "BOTH_GETUP_FROLL_L", "BOTH_GETUP_FROLL_R"}));
        return liste && timer() != 0.0;
    }
    // PM_InGetUp (bg_pmove.cpp:5954 OJ, :8412 MD, JK2 :3675)
    [[nodiscard]] bool inGetUp() const {
        const std::string& b = l_.beine;
        const bool liste = ist(b, {"BOTH_GETUP1", "BOTH_GETUP2", "BOTH_GETUP3", "BOTH_GETUP4", "BOTH_GETUP5",
                                   "BOTH_GETUP_CROUCH_F1", "BOTH_GETUP_CROUCH_B1"}) ||
                           (!jk() && ist(b, {"BOTH_GETUP_BROLL_B", "BOTH_GETUP_BROLL_F", "BOTH_GETUP_BROLL_L", "BOTH_GETUP_BROLL_R",
                                             "BOTH_GETUP_FROLL_B", "BOTH_GETUP_FROLL_F", "BOTH_GETUP_FROLL_L", "BOTH_GETUP_FROLL_R"}));
        if (liste) { return timer() != 0.0; }
        return inForceGetUp();
    }
    // PM_InKnockDown (bg_pmove.cpp:6014 OJ, :8472 MD, JK2 :3699)
    [[nodiscard]] bool inKnockDown() const {
        const std::string& b = l_.beine;
        if (ist(b, {"BOTH_KNOCKDOWN1", "BOTH_KNOCKDOWN2", "BOTH_KNOCKDOWN3", "BOTH_KNOCKDOWN4", "BOTH_KNOCKDOWN5"}) ||
            (!jk() && b == "BOTH_RELEASED") || (md() && ist(b, {"BOTH_SLAPDOWNRIGHT", "BOTH_SLAPDOWNLEFT"}))) {
            return true;
        }
        if (!jk() && b == "BOTH_LK_DL_ST_T_SB_1_L") { return timer() < 550.0; }
        if (!jk() && b == "BOTH_PLAYER_PA_3_FLY") { return timer() < 300.0; }
        return inGetUp();
    }
    // PM_InRoll (bg_pmove.cpp:6703 mit PM_InRollIgnoreTimer :6682 OJ; MD :9639/:9611;
    // JK2 :3925 nur die vier Rollen)
    [[nodiscard]] bool inRoll() const {
        const std::string& b = l_.beine;
        const bool rolle = ist(b, {"BOTH_ROLL_F", "BOTH_ROLL_B", "BOTH_ROLL_R", "BOTH_ROLL_L"}) ||
                           (!jk() && ist(b, {"BOTH_GETUP_BROLL_B", "BOTH_GETUP_BROLL_F", "BOTH_GETUP_BROLL_L", "BOTH_GETUP_BROLL_R",
                                             "BOTH_GETUP_FROLL_B", "BOTH_GETUP_FROLL_F", "BOTH_GETUP_FROLL_L", "BOTH_GETUP_FROLL_R"}));
        return rolle && timer() != 0.0;
    }
    // PM_InOnGroundAnim (bg_panimate.cpp:6573 OJ, :13806 MD, JK2 :3865)
    [[nodiscard]] bool inOnGroundAnim() const {
        const std::string& b = l_.beine;
        if (ist(b, {"BOTH_DEAD1", "BOTH_DEAD2", "BOTH_DEAD3", "BOTH_DEAD4", "BOTH_DEAD5", "BOTH_DEADFORWARD1", "BOTH_DEADBACKWARD1",
                    "BOTH_DEADFORWARD2", "BOTH_DEADBACKWARD2", "BOTH_LYINGDEATH1", "BOTH_LYINGDEAD1", "BOTH_SLEEP1"}) ||
            (jk() && inListe(b, kLiegenJk))) {
            return true;
        }
        if (ist(b, {"BOTH_KNOCKDOWN1", "BOTH_KNOCKDOWN2", "BOTH_KNOCKDOWN3", "BOTH_KNOCKDOWN4", "BOTH_KNOCKDOWN5"}) ||
            (!jk() && ist(b, {"BOTH_LK_DL_ST_T_SB_1_L", "BOTH_RELEASED"})) ||
            (md() && ist(b, {"BOTH_SLAPDOWNRIGHT", "BOTH_SLAPDOWNLEFT"}))) {
            return timer() < 500.0;
        }
        if (!jk() && b == "BOTH_PLAYER_PA_3_FLY") { return timer() < 300.0; }
        if (ist(b, {"BOTH_GETUP1", "BOTH_GETUP2", "BOTH_GETUP3", "BOTH_GETUP4", "BOTH_GETUP5", "BOTH_GETUP_CROUCH_F1",
                    "BOTH_GETUP_CROUCH_B1", "BOTH_FORCE_GETUP_F1", "BOTH_FORCE_GETUP_F2", "BOTH_FORCE_GETUP_B1",
                    "BOTH_FORCE_GETUP_B2", "BOTH_FORCE_GETUP_B3", "BOTH_FORCE_GETUP_B4", "BOTH_FORCE_GETUP_B5",
                    "BOTH_FORCE_GETUP_B6"})) {
            return timer() > animLaenge(b) - 400;
        }
        return false;
    }
    [[nodiscard]] bool crouchAnim(const std::string& a) const {
        return jk() ? inListe(a, kHockenJk) : md() ? inListe(a, kHockenMd) : inListe(a, kHockenOj);
    }
    [[nodiscard]] bool flippingAnim(const std::string& a) const {
        return jk() ? inListe(a, kSaltoJk) : md() ? inListe(a, kSaltoMd) : inListe(a, kSaltoOj);
    }
    [[nodiscard]] bool spinningAnim(const std::string& a) const {
        return jk() ? inListe(a, kDrehungJk) : md() ? inListe(a, kDrehungMd) : inListe(a, kDrehungOj);
    }
    // PM_FinishedCurrentLegsAnim (bg_panimate.cpp:4295 OJ, :6174 MD, JK2 :1957):
    // curFrame >= firstFrame + numFrames - 2. Eine Todesanimation steht mit
    // HOLD am letzten Bild still.
    [[nodiscard]] bool beineFertig() const {
        const double lerp = l_.bild ? l_.bild(l_.beine) : 0.0;
        const double bilder = lerp > 0.0 ? std::round(animLaenge(l_.beine) / lerp) : 0.0;
        double nr = lerp > 0.0 ? std::floor(l_.beineSeitMs / lerp) : 0.0;
        if (nr > bilder - 1.0) { nr = bilder - 1.0; }
        return nr >= bilder - 2.0;
    }
    // PM_PickAnim (bg_panimate.cpp:4350 OJ): Q_irand, bis die Figur sie hat
    // (hoechstens 1000 Zuege). BOTH_DEATH1..25 stehen in allen drei anims.h
    // hintereinander.
    std::string pickAnim(int von = 1, int bis = 25) {
        std::string anim;
        int zahl = 0;
        do {
            anim = "BOTH_DEATH" + std::to_string(qi(von, bis));
            ++zahl;
        } while (!hat(anim) && zahl < 1000);
        return anim;
    }

    [[nodiscard]] int trefferOrt() const;
    std::string besondere();
    std::string waehle();
    int kanteSpringen();
    void werfen(const NavVec& richtung);
    // Pruefung 3
    void waffeVorher(TodErgebnis& e, bool& schwertInHand);
    void waffeNachher(TodErgebnis& e, bool schwertInHand);
    void tossClientItems(TodErgebnis& e, bool schwert);
    void deathFx(TodErgebnis& e);
    static void klang(TodErgebnis& e, std::string datei, TodKlang::Kanal k = TodKlang::Kanal::Auto) {
        e.klaenge.push_back(TodKlang{std::move(datei), k});
    }
    // Q_irand(EV_DEATH1, EV_DEATH3) -> "*deathN.wav" auf CHAN_VOICE
    void todesKlang(TodErgebnis& e) { klang(e, "*death" + std::to_string(qi(1, 3)) + ".wav", TodKlang::Kanal::Voice); }
};

// G_GetHitLocation (g_combat.cpp:1319 OJ, :1678 MD, JK2 :1232 - gleich)
int Kette::trefferOrt() const {
    const NavVec& p = l_.treffpunkt;
    if (p[0] == 0.0F && p[1] == 0.0F && p[2] == 0.0F) { return HL_NONE; }
    const NavVec fwd = vorwaerts(l_.gier);
    const NavVec rechts{fwd[1], -fwd[0], 0.0F};
    const NavVec oben{0.0F, 0.0F, 1.0F};
    NavVec dir{};
    for (std::size_t k = 0; k < 3; ++k) {
        // absmin/absmax: SV_LinkEntity zieht je 1 ab bzw. zu
        const float absmin = l_.ort[k] + l_.mins[k] - 1.0F;
        const float absmax = l_.ort[k] + l_.maxs[k] + 1.0F;
        dir[k] = p[k] - (absmin + absmax) * 0.5F;
    }
    (void)normiere(dir);
    const float udot = punkt(oben, dir);
    const int vertikal = udot > .800F ? 4 : udot > .400F ? 3 : udot > -.333F ? 2 : udot > -.666F ? 1 : 0;
    const float fdot = punkt(fwd, dir);
    const int vorn = fdot > .666F ? 4 : fdot > .333F ? 3 : fdot > -.333F ? 2 : fdot > -.666F ? 1 : 0;
    const float rdot = punkt(rechts, dir);
    const int seite = rdot > .666F ? 4 : rdot > .333F ? 3 : rdot > -.333F ? 2 : rdot > -.666F ? 1 : 0;
    const int ort = vertikal * 25 + vorn * 5 + seite;
    if (ort <= 10) { return rdot > 0 ? HL_FOOT_RT : HL_FOOT_LT; }
    if (ort <= 50) { return rdot > 0 ? HL_LEG_RT : HL_LEG_LT; }
    if (ort == 56 || ort == 60 || ort == 61 || ort == 65 || ort == 66 || ort == 70) { return rdot > 0 ? HL_HAND_RT : HL_HAND_LT; }
    if (ort == 83 || ort == 87 || ort == 88 || ort == 92 || ort == 93 || ort == 97) { return rdot > 0 ? HL_ARM_RT : HL_ARM_LT; }
    if ((ort >= 107 && ort <= 109) || (ort >= 112 && ort <= 114) || (ort >= 117 && ort <= 119)) { return HL_HEAD; }
    if (udot < 0.3F) { return HL_WAIST; }
    if (fdot < 0) { return rdot > 0.4F ? HL_BACK_RT : rdot < -0.4F ? HL_BACK_LT : HL_BACK; }
    return rdot > 0.3F ? HL_CHEST_RT : rdot < -0.3F ? HL_CHEST_LT : HL_CHEST;
}

// G_CheckSpecialDeathAnim (g_combat.cpp:2514 OJ, :3246 MD, JK2 :2327). MD wie
// OpenJK mit BOTH_SLAPDOWNRIGHT/LEFT; JK2 ohne die Rollen am Anfang, ohne
// die Spielerverlaengerung und ohne PA_3_FLY/RELEASED/LK_DL_ST_T_SB_1_L.
std::string Kette::besondere() {
    const std::string& b = l_.beine;
    const double t = timer();
    const auto kauern = [this]() -> std::string {
        return wurf() < -150 ? "BOTH_DEATHBACKWARD1" : "BOTH_DEATH_CROUCHED";
    };
    if (!jk() && ist(b, {"BOTH_GETUP_BROLL_L", "BOTH_GETUP_BROLL_R"})) { return "BOTH_DEATH_LYING_UP"; }
    if (!jk() && ist(b, {"BOTH_GETUP_FROLL_L", "BOTH_GETUP_FROLL_R"})) { return "BOTH_DEATH_LYING_DN"; }
    if (!jk() && b == "BOTH_GETUP_BROLL_F" && t > 350) { return "BOTH_DEATH_FALLING_UP"; }
    if (!jk() && b == "BOTH_GETUP_BROLL_B" && t > 950) { return "BOTH_DEATH_LYING_UP"; }
    if (!jk() && b == "BOTH_GETUP_BROLL_B" && t <= 950 && t > 250) { return "BOTH_FALLDEATH1LAND"; }
    if (!jk() && b == "BOTH_GETUP_FROLL_B" && t <= 1100 && t > 250) { return "BOTH_FALLDEATH1LAND"; }
    if (inRoll()) { return "BOTH_DEATH_ROLL"; }
    if (flippingAnim(b)) { return "BOTH_DEATH_FLIP"; }
    if (spinningAnim(b)) {
        const float d = winkel180(winkel180(l_.rumpfWinkel[1]) - winkel180(l_.gier));
        if (d > 135 || d < -135) { return "BOTH_DEATH_SPIN_180"; }
        if (d < -60) { return "BOTH_DEATH_SPIN_90_R"; }
        if (d > 60) { return "BOTH_DEATH_SPIN_90_L"; }
        return {};
    }
    if (inKnockDown()) {
        int len = animLaenge(b);
        if (!jk() && l_.spieler &&
            (ist(b, {"BOTH_KNOCKDOWN1", "BOTH_KNOCKDOWN2", "BOTH_KNOCKDOWN3", "BOTH_KNOCKDOWN4", "BOTH_KNOCKDOWN5"}) ||
             (md() && ist(b, {"BOTH_SLAPDOWNRIGHT", "BOTH_SLAPDOWNLEFT"})))) {
            // PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME: 4000 (wp_saber.h:143 OJ), 300 (wp_saber.h:205 MD)
            len += md() ? 300 : 4000;
        }
        const double rest = len - t;
        if (b == "BOTH_KNOCKDOWN1") {
            if (rest > 100) { return t > 600 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP"; }
        } else if (b == "BOTH_KNOCKDOWN2" || (!jk() && b == "BOTH_PLAYER_PA_3_FLY")) {
            if (rest > 700) { return t > 600 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP"; }
        } else if (b == "BOTH_KNOCKDOWN3") {
            if (rest > 100) { return t > 1300 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN"; }
        } else if (b == "BOTH_KNOCKDOWN4" || (!jk() && b == "BOTH_RELEASED")) {
            if (rest > 300) { return t > 350 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP"; }
            return kauern();
        } else if (b == "BOTH_KNOCKDOWN5" || (md() && ist(b, {"BOTH_SLAPDOWNRIGHT", "BOTH_SLAPDOWNLEFT"})) ||
                   (!jk() && b == "BOTH_LK_DL_ST_T_SB_1_L")) {
            if (t < 750) { return "BOTH_DEATH_LYING_DN"; }
        } else if (b == "BOTH_GETUP1") {
            if (t < 350) { return {}; }
            if (t < 800) { return kauern(); }
            return rest > 450 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_GETUP2") {
            if (t < 150) { return {}; }
            if (t < 850) { return kauern(); }
            return rest > 500 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_GETUP3") {
            if (t < 250) { return {}; }
            if (t < 600) { return kauern(); }
            return rest > 150 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN";
        } else if (b == "BOTH_GETUP4") {
            if (t < 250) { return {}; }
            if (t < 600) { return kauern(); }
            return rest > 850 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_GETUP5") {
            if (t > 850) { return rest > 1500 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN"; }
        } else if (b == "BOTH_GETUP_CROUCH_B1") {
            if (t < 800) { return kauern(); }
            return rest > 400 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_GETUP_CROUCH_F1") {
            if (t < 800) { return kauern(); }
            return rest > 150 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN";
        } else if (b == "BOTH_FORCE_GETUP_B1") {
            if (t < 325) { return {}; }
            if (t < 725) { return "BOTH_DEATH_SPIN_180"; }
            if (t < 900) { return kauern(); }
            return rest > 50 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_FORCE_GETUP_B2") {
            if (t < 575) { return {}; }
            if (t < 875) { return "BOTH_DEATH_SPIN_180"; }
            if (t < 900) { return kauern(); }
            return "BOTH_DEATH_FALLING_UP";
        } else if (b == "BOTH_FORCE_GETUP_B3") {
            if (t < 150) { return {}; }
            if (t < 775) { return "BOTH_DEATHBACKWARD2"; }
            return "BOTH_DEATH_FALLING_UP";
        } else if (b == "BOTH_FORCE_GETUP_B4") {
            if (t < 325) { return {}; }
            return rest > 150 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_FORCE_GETUP_B5") {
            if (t < 550) { return {}; }
            if (t < 1025) { return "BOTH_DEATHBACKWARD2"; }
            return rest > 50 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_FORCE_GETUP_B6") {
            if (t < 225) { return {}; }
            if (t < 425) { return kauern(); }
            if (t < 825) { return "BOTH_DEATHFORWARD3"; }
            return rest > 225 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_LYING_UP";
        } else if (b == "BOTH_FORCE_GETUP_F1") {
            if (t < 275) { return {}; }
            if (t < 750) { return "BOTH_DEATH14"; }
            return rest > 100 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN";
        } else if (b == "BOTH_FORCE_GETUP_F2") {
            if (t < 1200) { return {}; }
            return rest > 225 ? "BOTH_DEATH_FALLING_DN" : "BOTH_DEATH_LYING_DN";
        }
        return {};
    }
    if (inOnGroundAnim()) { return winkel180(l_.rumpfWinkel[0]) < 0 ? "BOTH_DEATH_LYING_UP" : "BOTH_DEATH_LYING_DN"; }
    if (crouchAnim(b)) {
        if (wurf() < -200) {
            if (tempo_[2] > 0 && tempo_[2] < 100) { tempo_[2] = 100; }
            return "BOTH_DEATHBACKWARD1";
        }
        return "BOTH_DEATH_CROUCHED";
    }
    return {};
}

// G_Throw (wp_saber.cpp:478 OJ, :1666 MD, JK2 :308) mit push 85, wie
// G_CheckLedgeDive ihn ruft: vorher VectorClear(velocity), danach
// velocity[2] = 100 und groundEntityNum = ENTITYNUM_NONE.
void Kette::werfen(const NavVec& richtung) {
    tempo_ = NavVec{};
    const bool zuGross = klasse("CLASS_ATST") || klasse("CLASS_RANCOR") || klasse("CLASS_SAND_CREATURE");
    if (jk() || !zuGross) {
        constexpr float kStoss = 85.0F;
        const float masse = l_.physicsBounce > 0.0F ? l_.physicsBounce : 200.0F;
        // g_gravity > 0; MD: jedi_is_kick_resistant (charNoKnockback, AI_Character.cpp:187)
        const float faktor = (md() && l_.keinRueckstoss) ? kKnockback * 5.0F / masse : kKnockback * kStoss / masse * 0.8F;
        NavVec kvel{richtung[0] * faktor, richtung[1] * faktor, richtung[2] * faktor};
        // JK2 immer, sonst nur am Boden (er steht noch, groundEntityNum folgt danach)
        kvel[2] = richtung[2] * kKnockback * kStoss / masse * 1.5F;
        for (std::size_t k = 0; k < 3; ++k) { tempo_[k] += kvel[k]; }
        if (pmZeit_ == 0) {
            pmZeit_ = 170;   // push * 2, 50..200
            rueckstoss_ = true;
        }
    }
    tempo_[2] = 100;
}

// G_CheckLedgeDive( self, 128, ps.velocity, qtrue, qfalse ) (g_combat.cpp:3432 OJ,
// :4347 MD, JK2 :3246 - gleich) mit G_CheckForLedge (:5228 OJ): Spur 128 nach
// vorn mit dem Kasten, von dort 256 nach unten; trifft sie nichts, ist es 256.
int Kette::kanteSpringen() {
    const auto kante = [this](const NavVec& richtung) -> int {
        // Ohne Kartengeometrie keine Kante: die Vorschau kennt dann auch keinen
        // Boden und kein Fallen (NavWelt::spur leer, siehe planeLauf).
        if (!l_.spur) { return 0; }
        // self->clipmask: MASK_NPCSOLID ohne CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP
        // (player_die, g_combat.cpp:4119 OJ)
        constexpr std::uint32_t kMaske = kNavMaskNpcSolid & ~(kNavMonsterClip | kNavBotClip);
        NavVec ende{};
        for (std::size_t k = 0; k < 3; ++k) { ende[k] = l_.ort[k] + 128.0F * richtung[k]; }
        const NavSpur tr = l_.spur(l_.ort, l_.mins, l_.maxs, ende, l_.entity, kMaske);
        if (tr.allsolid || tr.startsolid) { return 0; }
        const NavVec start = tr.endpos;
        ende = start;
        ende[2] -= 256.0F;
        const NavSpur tr2 = l_.spur(start, l_.mins, l_.maxs, ende, l_.entity, kMaske);
        if (tr2.allsolid || tr2.startsolid) { return 0; }
        if (tr2.fraction >= 1.0F) { return static_cast<int>(start[2] - tr2.endpos[2]); }
        return 0;
    };
    const float gier = (tempo_[0] != 0.0F || tempo_[1] != 0.0F || tempo_[2] != 0.0F) ? vectoyaw(tempo_) : l_.gier;
    NavVec vor = vorwaerts(gier);
    (void)normiere(vor);
    int fall = kante(vor);
    if (fall >= 128) {
        werfen(vor);
    } else {
        for (float& x : vor) { x = -x; }
        fall = kante(vor);
        if (fall >= 128) { werfen(vor); }
    }
    return fall >= 256 ? 2 : fall >= 128 ? 1 : 0;
}

// G_PickDeathAnim (g_combat.cpp:3072 OJ, :3903 MD - gleich; JK2 :2836)
std::string Kette::waehle() {
    const int ort = trefferOrt();   // hitLoc == HL_NONE (kill, trigger_hurt)
    const std::string& b = l_.beine;
    if (jk()) {
        if (inListe(b, kSchonTotJk)) { return kSchon; }
    } else {
        if (ist(b, {"BOTH_DEATH1", "BOTH_DEAD1", "BOTH_DEATH2", "BOTH_DEAD2", "BOTH_DEATH8", "BOTH_DEAD8", "BOTH_DEATH13",
                    "BOTH_DEAD13", "BOTH_DEATH14", "BOTH_DEAD14", "BOTH_DEATH16", "BOTH_DEAD16", "BOTH_DEADBACKWARD1",
                    "BOTH_DEADBACKWARD2"})) {
            return beineFertig() ? "BOTH_DEADFLOP2" : kSchon;
        }
        if (b == "BOTH_DEADFLOP2") { return b; }
        if (ist(b, {"BOTH_DEATH10", "BOTH_DEAD10", "BOTH_DEATH15", "BOTH_DEAD15", "BOTH_DEADFORWARD1", "BOTH_DEADFORWARD2"})) {
            return beineFertig() ? "BOTH_DEADFLOP1" : kSchon;
        }
        if (b == "BOTH_DEADFLOP1") { return b; }
        if (inListe(b, kSchonTotOj)) { return kSchon; }
    }
    std::string d = besondere();
    if (d.empty()) {
        const float w = wurf();
        const double schaden = l_.schaden;
        const double maxG = l_.maxGesundheit;
        const auto zahl = [](int n) { return "BOTH_DEATH" + std::to_string(n); };
        switch (ort) {
            case HL_FOOT_RT:
            case HL_FOOT_LT:
                if (!qi(0, 2) && w < 250) {
                    d = zahl(ort == HL_FOOT_RT ? 24 : 25);
                } else if (!qi(0, 1)) {
                    d = (jk() || !qi(0, 1)) ? zahl(4) : zahl(16);
                } else {
                    d = zahl(5);
                }
                break;
            case HL_LEG_RT:
            case HL_LEG_LT:
                if (!qi(0, 2) && w < 250) {
                    d = zahl(ort == HL_LEG_RT ? 3 : 7);
                } else if (!qi(0, 1)) {
                    d = zahl(5);
                } else {
                    d = (jk() || !qi(0, 1)) ? zahl(4) : zahl(16);
                }
                break;
            case HL_BACK:
                if (std::fabs(w) < 50 || (std::fabs(w) < 200 && !qi(0, 3))) {
                    d = qi(0, 1) ? zahl(17) : zahl(10);
                } else if (jk()) {
                    d = !qi(0, 1) ? zahl(4) : zahl(5);
                } else if (!qi(0, 2)) {
                    d = zahl(4);
                } else if (!qi(0, 1)) {
                    d = zahl(5);
                } else {
                    d = zahl(16);
                }
                break;
            case HL_HAND_RT:
            case HL_CHEST_RT:
            case HL_ARM_RT:
            case HL_BACK_LT:
            case HL_CHEST_LT:
            case HL_ARM_LT:
            case HL_HAND_LT:
            case HL_BACK_RT: {
                const bool r = ort == HL_HAND_RT || ort == HL_CHEST_RT || ort == HL_ARM_RT || ort == HL_BACK_LT;
                const int a1 = r ? 9 : 11;
                const int a2 = r ? 20 : 21;
                const int b1 = r ? 3 : 7;
                const int c1 = r ? 6 : 12;
                const int e1 = r ? 8 : 14;
                if ((schaden <= maxG * 0.25 && qi(0, 1)) || (std::fabs(w) < 200 && !qi(0, 2)) || !qi(0, 10)) {
                    d = qi(0, 1) ? zahl(a1) : zahl(a2);
                } else if ((schaden <= maxG * 0.5 && qi(0, 1)) || !qi(0, 10)) {
                    d = zahl(b1);
                } else if ((schaden <= maxG * 0.75 && qi(0, 1)) || !qi(0, 10)) {
                    d = zahl(c1);
                } else if (qi(0, 1)) {
                    d = zahl(e1);
                } else {
                    const int wahl[4] = {a1, b1, c1, a2};
                    d = zahl(wahl[qi(0, 3)]);
                }
                break;
            }
            case HL_CHEST:
            case HL_WAIST:
                if ((schaden <= maxG * 0.25 && qi(0, 1)) || w > -50) {
                    d = !qi(0, 1) ? zahl(18) : zahl(19);
                } else if ((schaden <= maxG * 0.5 && !qi(0, 1)) || (std::fabs(w) < 200 && !qi(0, 3))) {
                    if (qi(0, 2)) {
                        d = zahl(2);
                    } else if (qi(0, 1)) {
                        d = zahl(22);
                    } else {
                        d = zahl(23);
                    }
                } else if (w < -300 && qi(0, 1)) {
                    d = qi(0, 1) ? "BOTH_DEATHBACKWARD1" : "BOTH_DEATHBACKWARD2";
                } else if (w < -200 && qi(0, 1)) {
                    d = zahl(15);
                } else if (jk()) {
                    d = !qi(0, 1) ? zahl(1) : zahl(16);
                } else {
                    d = zahl(1);
                }
                break;
            case HL_HEAD:
                if (schaden <= maxG * 0.5 && qi(0, 2)) {
                    d = zahl(17);
                } else {
                    d = qi(0, 2) ? zahl(13) : zahl(10);
                }
                break;
            default:
                break;
        }
    }
    if (d.empty() || !hat(d)) {
        if (!jk() && (d == "BOTH_DEADFLOP1" || d == "BOTH_DEADFLOP2")) {
            d.clear();
        } else {
            d = pickAnim();
        }
    }
    return d;
}

// --- Pruefung 3: Waffe und Schwert --------------------------------------------
//
// Die Folge der Waffen je Regelwerk (weapons.h), fuer "weapon > WP_SABER &&
// weapon <= MAX_PLAYER_WEAPONS" (OpenJK, JK2: MAX_PLAYER_WEAPONS =
// WP_STUN_BATON) bzw. playerUsableWeapons[] (g_weaponLoad.cpp:128 MD).
namespace {
constexpr const char* kWaffenOj[] = {"WP_NONE", "WP_SABER", "WP_BLASTER_PISTOL", "WP_BLASTER", "WP_DISRUPTOR",
    "WP_BOWCASTER", "WP_REPEATER", "WP_DEMP2", "WP_FLECHETTE", "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE",
    "WP_DET_PACK", "WP_CONCUSSION", "WP_MELEE", "WP_ATST_MAIN", "WP_ATST_SIDE", "WP_STUN_BATON"};
constexpr const char* kWaffenJk[] = {"WP_NONE", "WP_SABER", "WP_BRYAR_PISTOL", "WP_BLASTER", "WP_DISRUPTOR",
    "WP_BOWCASTER", "WP_REPEATER", "WP_DEMP2", "WP_FLECHETTE", "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE",
    "WP_DET_PACK", "WP_STUN_BATON"};
// playerUsableWeapons[] = qtrue (g_weaponLoad.cpp:128-176 MD)
constexpr const char* kWaffenMdNutzbar[] = {"WP_SABER", "WP_BLASTER_PISTOL", "WP_BLASTER", "WP_DISRUPTOR", "WP_BOWCASTER",
    "WP_REPEATER", "WP_DEMP2", "WP_FLECHETTE", "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE", "WP_DET_PACK",
    "WP_CONCUSSION", "WP_MELEE", "WP_ATST_MAIN", "WP_ATST_SIDE", "WP_STUN_BATON", "WP_BRYAR_PISTOL", "WP_DUAL_PISTOL",
    "WP_DUAL_CLONEPISTOL", "WP_TUSKEN_RIFLE", "WP_NOGHRI_STICK", "WP_BATTLEDROID", "WP_THEFIRSTORDER", "WP_CLONECARBINE",
    "WP_REBELBLASTER", "WP_CLONERIFLE", "WP_CLONECOMMANDO", "WP_Z6_ROTARY_CANNON", "WP_REBELRIFLE", "WP_REY", "WP_JANGO",
    "WP_BOBA", "WP_CLONEPISTOL"};
}  // namespace

// TossClientItems (g_combat.cpp:128-260 OJ, :201-345 MD, JK2 :101-170) fuer
// die Waffe in der Hand.
void Kette::tossClientItems(TodErgebnis& e, bool schwert) {
    const std::string& w = l_.waffe;
    if (klasse("CLASS_SEEKER") || klasse("CLASS_REMOTE")) { return; }
    if (!jk() && (klasse("CLASS_SABER_DROID") || klasse("CLASS_VEHICLE") || klasse("CLASS_ATST"))) { return; }
    if (md()) {
        // g_AllowWeaponDropping "1", com_kotor "0" (g_main.cpp:929, 950 MD),
        // dazu die Karten, auf denen Movie Duels nichts fallen laesst
        // (g_combat.cpp:216-224 MD: "reduce memory use to increase fps").
        for (const char* k : {"md_ga_jedi", "md_ga_sith", "md_gb_sith", "md_gb_jedi", "md_arena1", "md_arena2", "md_arena3"}) {
            if (l_.karte == k) { return; }
        }
        if (l_.kotorWaffen) { return; }   // "if (self->client->charKOTORWeapons == 1)"
        if (klasse("CLASS_SBD") || klasse("CLASS_DROIDEKA") || klasse("CLASS_OBJECT")) { return; }
    }
    if (schwert || w == "WP_SABER") {
        // weaponModel[0] < 0 kennt die Vorschau nicht (eine Figur mit WP_SABER
        // hat ihren Griff). JK2: "if ( self->weaponModel < 0 || WP_SaberLose(...) )".
        if (!jk() && l_.nichtEntwaffnbar) { return; }
        // WP_SaberLose: Saber-Droiden nie; WP_SaberLaunch prueft die Hand
        if (!jk() && klasse("CLASS_SABER_DROID")) { return; }
        if (l_.handFrei && !l_.handFrei()) { return; }
        // WP_SaberDrop nur mit brennender Klinge (WP_SaberLose: "if ( SaberActive() )")
        e.schwertFaellt = l_.klingeAn;
        if (l_.klingeAn) {
            if (jk()) {
                // codeJK2 WP_SaberDrop (wp_saber.cpp:4328-4336): nach dem Team
                klang(e, l_.spielerTeam ? "sound/weapons/saber/saberoff.wav" : "sound/weapons/saber/enemy_saber_off.wav");
            } else if (!l_.schwertAusKlang.empty()) {
                klang(e, l_.schwertAusKlang);   // G_SoundIndexOnEnt( saber, CHAN_AUTO, soundOff )
            }
        }
        return;
    }
    bool faellt = false;
    if (jk()) {
        for (std::size_t i = 2; i < std::size(kWaffenJk); ++i) { faellt = faellt || w == kWaffenJk[i]; }
    } else if (md()) {
        if (w == "WP_BLASTER_PISTOL" || w == "WP_STUN_BATON" || w == "WP_MELEE" || w == "WP_DUAL_PISTOL" ||
            w == "WP_DUAL_CLONEPISTOL" || w == "WP_DROIDEKA") {
            return;
        }
        for (const char* n : kWaffenMdNutzbar) { faellt = faellt || (w == n && w != "WP_SABER"); }
    } else {
        if (w == "WP_BLASTER_PISTOL" || w == "WP_STUN_BATON" || w == "WP_MELEE") { return; }
        for (std::size_t i = 2; i < std::size(kWaffenOj); ++i) { faellt = faellt || w == kWaffenOj[i]; }
    }
    if (!faellt) { return; }
    // Drop_Item: velocity[2] += 200 + Q_flrand(-1, 1) * 50; LaunchItem dreht
    // eine Waffe (ausser Bowcaster, Thermal, Mine, Sprengsatz) um Q_flrand(-1, 1) * 180.
    e.waffe = TodWaffe::Faellt;
    e.abwurfZufall[0] = qf(-1.0F, 1.0F);
    if (w != "WP_BOWCASTER" && w != "WP_THERMAL" && w != "WP_TRIP_MINE" && w != "WP_DET_PACK") { e.abwurfZufall[1] = qf(-1.0F, 1.0F); }
}

// player_die vor der Animation (g_combat.cpp:3877-3936 OJ, :4884-4950 MD,
// JK2 :3428-3475): das Schwert in der Hand (Q_irand(0, 1)), der Kristall des
// Shadowtroopers, TossClientItems.
void Kette::waffeVorher(TodErgebnis& e, bool& schwertInHand) {
    schwertInHand = l_.waffe == "WP_SABER";   // holdingSaber
    // Ein Schwert im Flug ("just drop it": Deactivate) wirft die Vorschau nicht:
    // dort laesst player_die es los, ohne zu wuerfeln.
    if (schwertInHand && !l_.schwertImFlug) {
        // g_saberPickuppableDroppedSabers "0" (g_main.cpp:675 OJ, :807 MD).
        // hitLoc ist HL_NONE (kill, trigger_hurt) - der erste Teil gilt, dann
        // holdingSaber und "Q_irand( 0, 1 ) || meansOfDeath == ... MOD_FALLING ...";
        // danach "ps.weapon = s.weapon = WP_NONE" - das Griffmodell bleibt in der
        // Hand, wenn WP_SaberLose nicht gelang.
        if (qi(0, 1) != 0 || l_.mittel == TodMittel::Fallen) { tossClientItems(e, true); }
    }
    // Shadowtrooper (nicht JK2): Drop_Item(FindItemForAmmo(AMMO_FORCE))
    if (!jk() && klasse("CLASS_SHADOWTROOPER")) {
        std::string t;
        for (const char c : l_.npcTyp.substr(0, 13)) { t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c)))); }
        if (t == "shadowtrooper") {
            e.kristall = true;
            e.abwurfZufall[2] = qf(-1.0F, 1.0F);
        }
    }
    // "if client is in a nodrop area, don't drop anything"
    if (!schwertInHand && !l_.keinAbwurf && !klasse("CLASS_GALAKMECH")) { tossClientItems(e, false); }
}

// player_die nach TossClientItems (g_combat.cpp:4092-4105 OJ, :5150-5166 MD,
// JK2 :3619-3640): die Klinge geht aus, sonst verschwindet die Waffe.
void Kette::waffeNachher(TodErgebnis& e, bool schwertInHand) {
    if (schwertInHand) {
        if (l_.klingeAn && !e.schwertFaellt) {
            if (jk()) {
                klang(e, l_.spielerTeam ? "sound/weapons/saber/saberoff.wav" : "sound/weapons/saber/enemy_saber_off.wav");
            } else if (!l_.schwertAusKlang.empty()) {
                klang(e, l_.schwertAusKlang);   // G_SoundIndexOnEnt( self, CHAN_AUTO, saber[0].soundOff )
            }
        }
        return;
    }
    if (e.waffe == TodWaffe::Faellt) { return; }   // s.weapon WP_NONE, das Modell geht mit
    const std::string& w = l_.waffe;
    bool bleibt = false;
    if (jk()) {
        bleibt = w == "WP_BLASTER_PISTOL";
    } else if (md()) {
        bleibt = w == "WP_BRYAR_PISTOL" || w == "WP_SBD_BLASTER" || w == "WP_JAWA" || w == "WP_DUAL_PISTOL" ||
                 w == "WP_DUAL_CLONEPISTOL" || w == "WP_WRIST_BLASTER" || w == "WP_DROIDEKA";
    } else {
        bleibt = w == "WP_BRYAR_PISTOL";
    }
    if (!bleibt && !w.empty() && w != "WP_NONE") { e.waffe = TodWaffe::Weg; }
}

// DeathFX (g_combat.cpp:599-730 OJ, :813-960 MD, JK2 :660-780).
void Kette::deathFx(TodErgebnis& e) {
    const NavVec& o = l_.ort;
    const float g = l_.gier * kGrad;   // currentAngles mit PITCH = ROLL = 0
    const NavVec rechts{std::sin(g), -std::cos(g), 0.0F};
    const auto bei = [&](float rechtsAb, float hoch) {
        return NavVec{o[0] + rechts[0] * rechtsAb, o[1] + rechts[1] * rechtsAb, o[2] + hoch};
    };
    const auto fx = [&](const char* datei, const NavVec& p) { e.effekte.push_back(TodEffekt{datei, p}); };
    // Movie Duels: NPC_type "cultist_destroyer" (g_combat.cpp:832-840 MD; das
    // Paar davor fragt jedi_cultist_destroyer( NPC ) mit dem globalen NPC - der
    // Figur, die gerade denkt, nicht unbedingt der sterbenden).
    if (md()) {
        std::string t;
        for (const char c : l_.npcTyp) { t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c)))); }
        if (t == "cultist_destroyer") {
            fx("explosions/droidexplosion1", bei(10.0F, -15.0F));
            klang(e, "sound/chars/cultist1/misc/anger1");
        }
    }
    // JK2 schreibt die Explosionen ohne Ordner (G_PlayEffect "droidexplosion1")
    const char* droid = jk() ? "droidexplosion1" : "explosions/droidexplosion1";
    if (klasse("CLASS_MOUSE")) {
        fx("env/small_explode", bei(0.0F, -20.0F));
        klang(e, "sound/chars/mouse/misc/death1");
    } else if (klasse("CLASS_PROBE")) {
        fx(jk() ? "probeexplosion1" : "explosions/probeexplosion1", bei(0.0F, 50.0F));
    } else if (klasse("CLASS_ATST")) {
        fx(droid, bei(20.0F, 180.0F));
        fx(droid, bei(-20.0F, 180.0F));
    } else if (klasse("CLASS_SEEKER") || klasse("CLASS_REMOTE")) {
        fx("env/small_explode", o);
    } else if (klasse("CLASS_GONK")) {
        klang(e, "sound/chars/gonk/misc/death" + std::to_string(qi(1, 3)) + ".wav");
        fx("env/med_explode", bei(0.0F, -5.0F));
    } else if (klasse("CLASS_R2D2") || klasse("CLASS_PROTOCOL") || klasse("CLASS_R5D2")) {
        fx("env/med_explode", bei(0.0F, -10.0F));
        klang(e, "sound/chars/mark2/misc/mark2_explo");
    } else if (klasse("CLASS_MARK2")) {
        fx(droid, bei(0.0F, -15.0F));
        klang(e, "sound/chars/mark2/misc/mark2_explo");
    } else if (klasse("CLASS_INTERROGATOR")) {
        fx(droid, bei(0.0F, -15.0F));
        klang(e, "sound/chars/interrogator/misc/int_droid_explo");
    } else if (klasse("CLASS_MARK1")) {
        fx(droid, bei(10.0F, -15.0F));
        fx(droid, bei(-10.0F, -15.0F));
        fx(droid, bei(-30.0F, -15.0F));
        klang(e, "sound/chars/mark1/misc/mark1_explo");
    } else if (klasse("CLASS_SENTRY")) {
        klang(e, "sound/chars/sentry/misc/sentry_explo");
        fx("env/med_explode", o);
    } else if (md() && (klasse("CLASS_BOBAFETT") || klasse("CLASS_MANDALORIAN") || klasse("CLASS_JANGO") ||
                        klasse("CLASS_JANGODUAL") || klasse("CLASS_ROCKETTROOPER"))) {
        fx("explosions/droidexplosion1", bei(10.0F, -15.0F));
        klang(e, "sound/chars/mark1/misc/mark1_explo");
    } else if (md() && klasse("CLASS_SBD")) {
        klang(e, "sound/chars/SBD/misc/death2");
        fx("env/small_explode", bei(10.0F, -15.0F));
    } else if (md() && (klasse("CLASS_ASSASSIN_DROID") || klasse("CLASS_HAZARD_TROOPER"))) {
        klang(e, "sound/chars/battledroid/misc/death3");
        fx("explosions/droidexplosion1", bei(10.0F, -15.0F));
    } else if (md() && klasse("CLASS_BATTLEDROID")) {
        fx("env/small_explode", o);
        klang(e, "sound/chars/battledroid/misc/death2");
    } else if (md() && klasse("CLASS_DROIDEKA")) {
        fx("env/small_explode", o);
        klang(e, "sound/chars/droideka/foldin");
    }
}

// player_die (g_combat.cpp:3610 OJ, :4497 MD, JK2 :3335) - was die Animation
// bestimmt. Die Mittel der Vorschau sind nie MOD_SNIPER/MOD_PROJECTION_END/
// MOD_CONC_ALT/MOD_ELECTROCUTE/MOD_CRUSH/MOD_WATER/MOD_GAS/MOD_FORCE_DRAIN/
// MOD_KNOCKOUT; Festhalten (EF_HELD_BY_*), Fahrzeuge und Amputation kennt sie
// nicht.
TodErgebnis Kette::lauf() {
    TodErgebnis e;
    // Schon tot: Zuckungen (:3621-3645 OJ, :4521-4575 MD, JK2 :3343-3366)
    if (l_.schonTot) {
        if (klasse("CLASS_MARK1")) {
            deathFx(e);   // DeathFX(self) (g_combat.cpp:3625 OJ)
            e.verschwindet = true;
        } else {
            const std::string a = waehle();
            if (gesetzt(a)) {
                e.anim = a;
                e.neuStart = true;
            }
        }
        e.tempo = tempo_;
        return e;
    }
    // Pruefung 3: Schwert (Q_irand(0, 1)), Kristall, TossClientItems, Klinge aus
    // oder Waffe weg - im Spiel vor der Animation (g_combat.cpp:3877-4105 OJ).
    bool schwertInHand = false;
    waffeVorher(e, schwertInHand);
    waffeNachher(e, schwertInHand);
    e.timerNie = true;   // PM_SetLegsAnimTimer/PM_SetTorsoAnimTimer(-1) am Ende
    std::string anim;
    const bool fallTod = l_.mittel == TodMittel::Fallen ||
                         (l_.beine == "BOTH_FALLDEATH1INAIR" && l_.torso == "BOTH_FALLDEATH1INAIR") ||
                         (l_.beine == "BOTH_FALLDEATH1" && l_.torso == "BOTH_FALLDEATH1");
    if (klasse("CLASS_MARK1")) {
        // Mark1_die (AI_Mark1.cpp:225 OJ): erst der Klang death%d (Q_irand(1, 2))
        klang(e, "sound/chars/mark1/misc/death" + std::to_string(qi(1, 2)) + ".wav");
        anim = qi(1, 10) > 5 ? "BOTH_DEATH2" : "BOTH_DEATH1";
    } else if (klasse("CLASS_INTERROGATOR")) {
        // Interrogator_die (AI_Interrogator.cpp:55 OJ): keine Animation, er faellt
        (void)qi(-20, -10);
        (void)qi(-20, -10);
    } else if (klasse("CLASS_GALAKMECH")) {
        anim = "BOTH_DEATH1";
        todesKlang(e);
    } else if (klasse("CLASS_ATST")) {
        anim = pickAnim();
    } else if (md() && klasse("CLASS_DROIDEKA")) {
        anim = pickAnim(1, 1);   // PM_PickAnim(self, BOTH_DEATH1, BOTH_DEATH1) (:5272 MD)
    } else if (!l_.spieler && l_.schluessel) {
        // Schluessel am Arm: eine Animation, die ihn sichtbar laesst
        static constexpr const char* kSchluessel[4] = {"BOTH_DEATH4", "BOTH_DEATH21", "BOTH_DEATH17", "BOTH_DEATH18"};
        anim = kSchluessel[qi(0, 3)];
        todesKlang(e);   // weder MOD_KNOCKOUT/MELEE noch GAS/FORCE_DRAIN
    } else if (fallTod) {
        if (!l_.amBoden && tempo_[2] < 0 && l_.beine != "BOTH_FALLDEATH1INAIR" && l_.torso != "BOTH_FALLDEATH1INAIR") {
            anim = "BOTH_FALLDEATH1INAIR";
            // "*falling1.wav" auf CHAN_VOICE; ein NPC setzt NPCAI_DIE_ON_IMPACT und
            // ps.friction = 1 (g_combat.cpp:4252-4262 OJ, :5329-5341 MD)
            klang(e, "*falling1.wav", TodKlang::Kanal::Voice);
            if (!l_.spieler) {
                e.aufprallTod = true;
                e.reibung = 1.0F;
            }
        } else if (inOnGroundAnim()) {
            anim = winkel180(l_.rumpfWinkel[0]) < 0 ? "BOTH_DEATH_LYING_UP" : "BOTH_DEATH_LYING_DN";
        } else if (inKnockDown()) {
            anim = winkel180(l_.rumpfWinkel[0]) < 0 ? "BOTH_DEATH_FALLING_UP" : "BOTH_DEATH_FALLING_DN";
        } else {
            anim = "BOTH_FALLDEATH1LAND";
        }
        if (anim != "BOTH_FALLDEATH1INAIR") {
            // "HMM: check for nodrop?" - fallsplat auf CHAN_BODY; *pain100 nur,
            // wenn die Figur spricht UND schon NPCAI_DIE_ON_IMPACT traegt - das
            // setzt erst dieser Tod (g_combat.cpp:4294-4300 OJ)
            klang(e, "sound/player/fallsplat.wav", TodKlang::Kanal::Body);
        }
    } else {
        // normal death (:4302-4482 OJ, :5383-5522 MD, JK2 :3814-3994)
        anim = besondere();
        if (anim.empty()) {
            if (jk() && (l_.beine == "BOTH_RESTRAINED1" || l_.beine == "BOTH_RESTRAINED1POINT")) {
                anim = "BOTH_RESTRAINED1";
            } else if (inOnGroundAnim() && hat("BOTH_LYINGDEATH1")) {
                anim = "BOTH_LYINGDEATH1";
            } else if (l_.mittel == TodMittel::TriggerHurt && l_.geschockt) {
                anim = "BOTH_DEATH17";
            } else {
                e.kante = kanteSpringen();
                // "*falling1.wav", NPCAI_DIE_ON_IMPACT und ps.friction = 0
                // (g_combat.cpp:4323-4337 OJ) - g_gravity > 0, kein Flieger
                if (e.kante == 2) {
                    klang(e, "*falling1.wav", TodKlang::Kanal::Voice);
                    if (!l_.spieler) {
                        e.aufprallTod = true;
                        e.reibung = 0.0F;
                    }
                }
                if (pmZeit_ > 0 && rueckstoss_ && tempo_[2] > 0) {
                    NavVec richtung = tempo_;
                    const float geworfen = normiere(richtung);
                    const float dot = punkt(vorwaerts(l_.gier), richtung);
                    if (geworfen > 100) {
                        if (dot > 0.3F) {
                            if (e.kante == 2 && hat("BOTH_FALLDEATH1")) {
                                anim = "BOTH_FALLDEATH1";
                            } else {
                                // OJ/MD: 0,1 DEATH4  2 DEATH16  3-5 DEATH5  6 DEATH8  7 DEATH14;
                                // JK2: 0-2 DEATH4  3-5 DEATH5  6 DEATH8  7 DEATH14
                                static constexpr int kVorOj[8] = {4, 4, 16, 5, 5, 5, 8, 14};
                                static constexpr int kVorJk[8] = {4, 4, 4, 5, 5, 5, 8, 14};
                                const int w = qi(0, 7);
                                anim = "BOTH_DEATH" + std::to_string(jk() ? kVorJk[w] : kVorOj[w]);
                                if (hat(anim)) {
                                    if (tempo_[2] > 0 && tempo_[2] < 100) { tempo_[2] = 100; }
                                } else {
                                    anim.clear();
                                }
                            }
                        } else if (dot < -0.3F) {
                            if (geworfen >= 250 && !qi(0, 3)) {
                                anim = qi(0, 1) ? "BOTH_DEATHBACKWARD1" : "BOTH_DEATHBACKWARD2";
                            } else {
                                static constexpr int kZurueck[8] = {1, 1, 2, 2, 22, 22, 23, 23};
                                anim = "BOTH_DEATH" + std::to_string(kZurueck[qi(0, 7)]);
                            }
                            if (hat(anim)) {
                                if (tempo_[2] > 0 && tempo_[2] < 100) { tempo_[2] = 100; }
                            } else {
                                anim.clear();
                            }
                        } else if (e.kante == 2 && hat("BOTH_FALLDEATH1")) {
                            anim = "BOTH_FALLDEATH1";
                            if (tempo_[2] > 0 && tempo_[2] < 100) { tempo_[2] = 100; }
                        }
                    }
                }
            }
        }
        // OJ/JK2: sonst G_PickDeathAnim; MD faellt gleich auf PM_PickAnim (:5498-5522 MD)
        if (anim.empty() && !md()) { anim = waehle(); }
        if (anim.empty()) {
            anim = pickAnim();
            // TEMP HACK: these spinny deaths should happen less often
            if ((anim == "BOTH_DEATH8" || anim == "BOTH_DEATH14") && qi(0, 1)) { anim = pickAnim(); }
        } else if (!jk() && !hat(anim)) {
            anim = pickAnim();
        }
        // EV_DEATH1-3 - hitLoc HL_NONE, nicht gewuergt; ueber die Kante (cliff_fall
        // 2) schreit schon *falling1 (g_combat.cpp:4528-4568 OJ, :5690-5722 MD)
        if (e.kante != 2) { todesKlang(e); }
    }
    if (gesetzt(anim)) { e.anim = anim; }
    deathFx(e);   // "Start any necessary death fx for this entity" (g_combat.cpp:4694 OJ)
    e.tempo = tempo_;
    return e;
}

}  // namespace

TodErgebnis todesAnimation(const TodLage& lage, EngineZufall& zufall) {
    Kette k(lage, zufall);
    return k.lauf();
}

void engineVectoangles(const float v[3], float winkel[3]) noexcept {
    constexpr float kPi = 3.14159265358979323846F;   // M_PI
    float gier = 0.0F;
    float nick = 0.0F;
    if (v[1] == 0.0F && v[0] == 0.0F) {
        nick = v[2] > 0.0F ? 90.0F : 270.0F;
    } else {
        if (v[0] != 0.0F) {
            gier = std::atan2(v[1], v[0]) * 180.0F / kPi;
        } else {
            gier = v[1] > 0.0F ? 90.0F : 270.0F;
        }
        if (gier < 0.0F) { gier += 360.0F; }
        const float vorn = std::sqrt(v[0] * v[0] + v[1] * v[1]);
        nick = std::atan2(v[2], vorn) * 180.0F / kPi;
        if (nick < 0.0F) { nick += 360.0F; }
    }
    winkel[0] = -nick;
    winkel[1] = gier;
    winkel[2] = 0.0F;
}

void rumpfWinkelAusBolzen(const float bolzen[3][4], float winkel[3]) noexcept {
    // NEGATIVE_Z: vec[i] = -boltMatrix.matrix[i][2] (G2_API.cpp, G2API_GiveMeVectorFromMatrix)
    const float achse[3] = {-bolzen[0][2], -bolzen[1][2], -bolzen[2][2]};
    engineVectoangles(achse, winkel);
}

}  // namespace bhed
