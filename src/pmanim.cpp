// pmanim.cpp - siehe bhed/pmanim.h
#include "bhed/pmanim.h"
#include "bhed/regelwerk.h"
#include "bhed/scene.h"   // saberHaltungFuerStil: die Kampfhaltung

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string_view>

namespace bhed {
namespace {

// Zeitleiste 6: die Listen der Engine (OpenJK bg_pmove.cpp/bg_panimate.cpp, Movie Duels mit
// den Zusaetzen dahinter; mit <SP>/zl6/faelle.py aus den Quellen gelesen).
// PM_FlippingAnim (bg_pmove.cpp OpenJK), Movie Duels mit den Zusaetzen dahinter.
constexpr std::string_view kSalto[] = {
    "BOTH_FLIP_F", "BOTH_FLIP_B", "BOTH_FLIP_L", "BOTH_FLIP_R", "BOTH_ALORA_FLIP_1", "BOTH_ALORA_FLIP_2",
    "BOTH_ALORA_FLIP_3", "BOTH_WALL_RUN_RIGHT_FLIP", "BOTH_WALL_RUN_LEFT_FLIP", "BOTH_WALL_FLIP_RIGHT",
    "BOTH_WALL_FLIP_LEFT", "BOTH_FLIP_BACK1", "BOTH_FLIP_BACK2", "BOTH_FLIP_BACK3", "BOTH_WALL_FLIP_BACK1",
    "BOTH_ALORA_FLIP_B", "BOTH_WALL_RUN_RIGHT", "BOTH_WALL_RUN_LEFT", "BOTH_WALL_RUN_RIGHT_STOP",
    "BOTH_WALL_RUN_LEFT_STOP", "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_BUTTERFLY_FL1",
    "BOTH_BUTTERFLY_FR1", "BOTH_ARIAL_LEFT", "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1", "BOTH_CARTWHEEL_LEFT",
    "BOTH_CARTWHEEL_RIGHT", "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN", "BOTH_JUMPATTACK6",
    "BOTH_JUMPATTACK7", "BOTH_FORCEWALLRUNFLIP_END", "BOTH_FORCEWALLRUNFLIP_ALT", "BOTH_FLIP_ATTACK7",
    "BOTH_A7_SOULCAL",
};
constexpr std::string_view kSaltoMd[] = {
    "BOTH_FLIP_B_ANI", "BOTH_FLIP_B_GALEN", "BOTH_FLIP_F_ANI", "BOTH_FLIP_F_GALEN", "BOTH_FLIP_L_ANI",
    "BOTH_FLIP_L_GALEN", "BOTH_FLIP_R_ANI", "BOTH_FLIP_R_GALEN", "BOTH_JUMPATTACK6_GRIEV",
};
// PM_SpinningSaberAnim
constexpr std::string_view kDrehSchwert[] = {
    "BOTH_T1_BR_BL", "BOTH_T1__R__L", "BOTH_T1__R_BL", "BOTH_T1_TR_BL", "BOTH_T1_BR_TL", "BOTH_T1_BR__L",
    "BOTH_T1_TL_BR", "BOTH_T1__L_BR", "BOTH_T1__L__R", "BOTH_T1_BL_BR", "BOTH_T1_BL__R", "BOTH_T1_BL_TR",
    "BOTH_T2_BR__L", "BOTH_T2_BR_BL", "BOTH_T2__R_BL", "BOTH_T2__L_BR", "BOTH_T2_BL_BR", "BOTH_T2_BL__R",
    "BOTH_T3_BR__L", "BOTH_T3_BR_BL", "BOTH_T3__R_BL", "BOTH_T3__L_BR", "BOTH_T3_BL_BR", "BOTH_T3_BL__R",
    "BOTH_T4_BR__L", "BOTH_T4_BR_BL", "BOTH_T4__R_BL", "BOTH_T4__L_BR", "BOTH_T4_BL_BR", "BOTH_T4_BL__R",
    "BOTH_T5_BR_BL", "BOTH_T5__R__L", "BOTH_T5__R_BL", "BOTH_T5_TR_BL", "BOTH_T5_BR_TL", "BOTH_T5_BR__L",
    "BOTH_T5_TL_BR", "BOTH_T5__L_BR", "BOTH_T5__L__R", "BOTH_T5_BL_BR", "BOTH_T5_BL__R", "BOTH_T5_BL_TR",
    "BOTH_T6_BR_TL", "BOTH_T6__R_TL", "BOTH_T6__R__L", "BOTH_T6__R_BL", "BOTH_T6_TR_TL", "BOTH_T6_TR__L",
    "BOTH_T6_TR_BL", "BOTH_T6_T__TL", "BOTH_T6_T__BL", "BOTH_T6_TL_BR", "BOTH_T6__L_BR", "BOTH_T6__L__R",
    "BOTH_T6_TL__R", "BOTH_T6_TL_TR", "BOTH_T6__L_TR", "BOTH_T6__L_T_", "BOTH_T6_BL_T_", "BOTH_T6_BR__L",
    "BOTH_T6_BR_BL", "BOTH_T6_BL_BR", "BOTH_T6_BL__R", "BOTH_T6_BL_TR", "BOTH_T7_BR_TL", "BOTH_T7_BR__L",
    "BOTH_T7_BR_BL", "BOTH_T7__R__L", "BOTH_T7__R_BL", "BOTH_T7_TR__L", "BOTH_T7_T___R", "BOTH_T7_TL_BR",
    "BOTH_T7__L_BR", "BOTH_T7__L__R", "BOTH_T7_BL_BR", "BOTH_T7_BL__R", "BOTH_T7_BL_TR", "BOTH_T7_TL_TR",
    "BOTH_T7_T__BR", "BOTH_T7__L_TR", "BOTH_V7_BL_S7", "BOTH_ATTACK_BACK", "BOTH_CROUCHATTACKBACK1",
    "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_FJSS_TR_BL", "BOTH_FJSS_TL_BR",
    "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN",
};
constexpr std::string_view kDrehSchwertMd[] = {
    "BOTH_T5_BL_TR_YODA", "BOTH_T5_BL__R_YODA", "BOTH_T5_BR_TL_YODA", "BOTH_T5_TL_BR_YODA",
    "BOTH_T5__L__R_YODA", "BOTH_T5__R__L_YODA",
};
// PM_JumpingAnim
constexpr std::string_view kSprung[] = {
    "BOTH_JUMP1", "BOTH_INAIR1", "BOTH_LAND1", "BOTH_LAND2", "BOTH_JUMPBACK1", "BOTH_INAIRBACK1",
    "BOTH_LANDBACK1", "BOTH_JUMPLEFT1", "BOTH_INAIRLEFT1", "BOTH_LANDLEFT1", "BOTH_JUMPRIGHT1",
    "BOTH_INAIRRIGHT1", "BOTH_LANDRIGHT1",
};
constexpr std::string_view kSprungMd[] = {
    "BOTH_JUMP1_ANI", "BOTH_JUMP1_YODA", "BOTH_JUMP1_VADER", "BOTH_JUMP1_GALEN", "BOTH_JUMP1_MAUL",
    "BOTH_INAIR1_ANI", "BOTH_INAIR1_YODA", "BOTH_INAIR1_VADER", "BOTH_INAIR1_GALEN", "BOTH_INAIR1_MAUL",
    "BOTH_LAND1_ANI", "BOTH_LAND1_YODA", "BOTH_LAND1_VADER", "BOTH_LAND1_GALEN", "BOTH_LAND1_MAUL",
    "BOTH_INAIRLEFT1_GALEN", "BOTH_INAIRRIGHT1_GALEN",
};
// PM_LandingAnim
constexpr std::string_view kLandung[] = {
    "BOTH_LAND1", "BOTH_LAND2", "BOTH_LANDBACK1", "BOTH_LANDLEFT1", "BOTH_LANDRIGHT1", "BOTH_FORCELAND1",
    "BOTH_FORCELANDBACK1", "BOTH_FORCELANDLEFT1", "BOTH_FORCELANDRIGHT1",
};
constexpr std::string_view kLandungMd[] = {
    "BOTH_LAND1_ANI", "BOTH_LAND1_YODA", "BOTH_LAND1_VADER", "BOTH_LAND1_GALEN", "BOTH_LAND1_MAUL",
    "BOTH_FORCELAND1_VADER", "BOTH_FORCELAND1_PAL",
};
// PM_InCartwheel
constexpr std::string_view kRad[] = {
    "BOTH_ARIAL_LEFT", "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1", "BOTH_CARTWHEEL_LEFT", "BOTH_CARTWHEEL_RIGHT",
};
// PM_PainAnim
constexpr std::string_view kSchmerz[] = {
    "BOTH_PAIN1", "BOTH_PAIN2", "BOTH_PAIN3", "BOTH_PAIN4", "BOTH_PAIN5", "BOTH_PAIN6", "BOTH_PAIN7",
    "BOTH_PAIN8", "BOTH_PAIN9", "BOTH_PAIN10", "BOTH_PAIN11", "BOTH_PAIN12", "BOTH_PAIN13", "BOTH_PAIN14",
    "BOTH_PAIN15", "BOTH_PAIN16", "BOTH_PAIN17", "BOTH_PAIN18",
};
constexpr std::string_view kSchmerzMd[] = {
    "BOTH_PAIN19",
};
// PM_InSpecialJump
constexpr std::string_view kSondersprung[] = {
    "BOTH_WALL_RUN_RIGHT", "BOTH_WALL_RUN_RIGHT_STOP", "BOTH_WALL_RUN_RIGHT_FLIP", "BOTH_WALL_RUN_LEFT",
    "BOTH_WALL_RUN_LEFT_STOP", "BOTH_WALL_RUN_LEFT_FLIP", "BOTH_WALL_FLIP_RIGHT", "BOTH_WALL_FLIP_LEFT",
    "BOTH_WALL_FLIP_BACK1", "BOTH_BUTTERFLY_LEFT", "BOTH_BUTTERFLY_RIGHT", "BOTH_BUTTERFLY_FL1",
    "BOTH_BUTTERFLY_FR1", "BOTH_FJSS_TR_BL", "BOTH_FJSS_TL_BR", "BOTH_FORCELEAP2_T__B_",
    "BOTH_JUMPFLIPSLASHDOWN1", "BOTH_JUMPFLIPSTABDOWN", "BOTH_JUMPATTACK6", "BOTH_JUMPATTACK7",
    "BOTH_ARIAL_LEFT", "BOTH_ARIAL_RIGHT", "BOTH_ARIAL_F1", "BOTH_CARTWHEEL_LEFT", "BOTH_CARTWHEEL_RIGHT",
    "BOTH_FORCELONGLEAP_START", "BOTH_FORCELONGLEAP_ATTACK", "BOTH_FORCEWALLRUNFLIP_START",
    "BOTH_FORCEWALLRUNFLIP_END", "BOTH_FORCEWALLRUNFLIP_ALT", "BOTH_FLIP_ATTACK7", "BOTH_FLIP_HOLD7",
    "BOTH_FLIP_LAND", "BOTH_A7_SOULCAL",
};
constexpr std::string_view kSondersprungMd[] = {
    "BOTH_FORCELEAP2_T__B__PAL", "BOTH_JUMPATTACK6_GRIEV", "BOTH_FORCEJUMPDASH_START",
    "BOTH_FORCEJUMPDASH_ATTACK",
};
// PM_InSlopeAnim
constexpr std::string_view kHang[] = {
    "LEGS_LEFTUP1", "LEGS_LEFTUP2", "LEGS_LEFTUP3", "LEGS_LEFTUP4", "LEGS_LEFTUP5", "LEGS_RIGHTUP1",
    "LEGS_RIGHTUP2", "LEGS_RIGHTUP3", "LEGS_RIGHTUP4", "LEGS_RIGHTUP5", "LEGS_S1_LUP1", "LEGS_S1_LUP2",
    "LEGS_S1_LUP3", "LEGS_S1_LUP4", "LEGS_S1_LUP5", "LEGS_S1_RUP1", "LEGS_S1_RUP2", "LEGS_S1_RUP3",
    "LEGS_S1_RUP4", "LEGS_S1_RUP5", "LEGS_S3_LUP1", "LEGS_S3_LUP2", "LEGS_S3_LUP3", "LEGS_S3_LUP4",
    "LEGS_S3_LUP5", "LEGS_S3_RUP1", "LEGS_S3_RUP2", "LEGS_S3_RUP3", "LEGS_S3_RUP4", "LEGS_S3_RUP5",
    "LEGS_S4_LUP1", "LEGS_S4_LUP2", "LEGS_S4_LUP3", "LEGS_S4_LUP4", "LEGS_S4_LUP5", "LEGS_S4_RUP1",
    "LEGS_S4_RUP2", "LEGS_S4_RUP3", "LEGS_S4_RUP4", "LEGS_S4_RUP5", "LEGS_S5_LUP1", "LEGS_S5_LUP2",
    "LEGS_S5_LUP3", "LEGS_S5_LUP4", "LEGS_S5_LUP5", "LEGS_S5_RUP1", "LEGS_S5_RUP2", "LEGS_S5_RUP3",
    "LEGS_S5_RUP4", "LEGS_S5_RUP5", "LEGS_S6_LUP1", "LEGS_S6_LUP2", "LEGS_S6_LUP3", "LEGS_S6_LUP4",
    "LEGS_S6_LUP5", "LEGS_S6_RUP1", "LEGS_S6_RUP2", "LEGS_S6_RUP3", "LEGS_S6_RUP4", "LEGS_S6_RUP5",
    "LEGS_S7_LUP1", "LEGS_S7_LUP2", "LEGS_S7_LUP3", "LEGS_S7_LUP4", "LEGS_S7_LUP5", "LEGS_S7_RUP1",
    "LEGS_S7_RUP2", "LEGS_S7_RUP3", "LEGS_S7_RUP4", "LEGS_S7_RUP5",
};
// PM_SetSaberMove: "match torso anim to walk/run anim" - Haltungen (MD) und Beine (MD)
constexpr std::string_view kBereitPassungMd[] = {
    "BOTH_STAND2", "BOTH_SABERDUAL_STANCE", "BOTH_SABERFAST_STANCE_JKA", "BOTH_SABERFAST_STANCE_JKA_ANI",
    "BOTH_SABERFAST_STANCE_JKA_BEN", "BOTH_SABERFAST_STANCE_JKA_VADER", "BOTH_SABERFAST_STANCE_JKA_PAL",
    "BOTH_SABERFAST_STANCE_JKA_DOOKU", "BOTH_SABERFAST_STANCE_JKA_MAUL", "BOTH_SABERFAST_STANCE_JKA_LUKE",
    "BOTH_SABERFAST_STANCE_JKA_MACE", "BOTH_SABERFAST_STANCE_JKA_OBI", "BOTH_SABERFAST_STANCE_JKA_KOTOR",
    "BOTH_SABERFAST_STANCE_JKA_DF2", "BOTH_SABERFAST_STANCE_JKA_OBI3", "BOTH_SABERFAST_STANCE_JKA_REN",
    "BOTH_SABERFAST_STANCE_JKA_REY", "BOTH_SABERFAST_STANCE_JKA_CAL", "BOTH_SABERFAST_STANCE_JKA_GRIEV",
    "BOTH_SABERSLOW_STANCE", "BOTH_SABERSLOW_STANCE_JKA", "BOTH_SABERSLOW_STANCE_JKA_ANI",
    "BOTH_SABERSLOW_STANCE_JKA_BEN", "BOTH_SABERSLOW_STANCE_JKA_VADER", "BOTH_SABERSLOW_STANCE_JKA_GALEN",
    "BOTH_SABERSLOW_STANCE_JKA_LUKE", "BOTH_SABERSLOW_STANCE_JKA_GON", "BOTH_SABERSLOW_STANCE_JKA_OBI",
    "BOTH_SABERSLOW_STANCE_JKA_KOTOR", "BOTH_SABERSLOW_STANCE_JKA_DF2", "BOTH_SABERSLOW_STANCE_JKA_OBI3",
    "BOTH_SABERSLOW_STANCE_JKA_REN", "BOTH_SABERSLOW_STANCE_JKA_REY", "BOTH_SABERSLOW_STANCE_JKA_PAL",
    "BOTH_SABERSLOW_STANCE_JKA_MAUL", "BOTH_SABERTAVION_STANCE", "BOTH_SABERTAVION_STANCE_JKA",
    "BOTH_SABERTAVION_STANCE_JKA_BEN", "BOTH_SABERTAVION_STANCE_JKA_YODA", "BOTH_SABERTAVION_STANCE_JKA_VADER",
    "BOTH_SABERTAVION_STANCE_JKA_MAUL", "BOTH_SABERTAVION_STANCE_JKA_MACE", "BOTH_SABERTAVION_STANCE_JKA_DF2",
    "BOTH_SABERTAVION_STANCE_JKA_OBI3", "BOTH_SABERTAVION_STANCE_JKA_REN", "BOTH_SABERDESANN_STANCE",
    "BOTH_SABERDESANN_STANCE_JKA", "BOTH_SABERDESANN_STANCE_JKA_ANI", "BOTH_SABERDESANN_STANCE_JKA_DOOKU",
    "BOTH_SABERDESANN_STANCE_JKA_MACE", "BOTH_SABERDESANN_STANCE_JKA_DF2", "BOTH_SABERDESANN_STANCE_JKA_CAL",
};
constexpr std::string_view kGehPassungMd[] = {
    "BOTH_WALK1", "BOTH_WALK1_VADER", "BOTH_WALK1_GALEN", "BOTH_WALK1_OBI3", "BOTH_WALK1_REN", "BOTH_WALK1_REY",
    "BOTH_WALK1_JANGO", "BOTH_WALK1_BDROID", "BOTH_WALK1_CAL", "BOTH_WALK1_GRIEV", "BOTH_WALK1_PAL",
    "BOTH_WALK1_DOOKU", "BOTH_WALK1_MAUL", "BOTH_WALK1_YODA", "BOTH_WALK1_MDA", "BOTH_WALK1_MDA_PAL",
    "BOTH_WALK1_MDA_BDROID", "BOTH_WALK2", "BOTH_WALK2_VADER", "BOTH_WALK2_GALEN", "BOTH_WALK2_OBI3",
    "BOTH_WALK2_REN", "BOTH_WALK2_REY", "BOTH_WALK2_JANGO", "BOTH_WALK2_BDROID", "BOTH_WALK2_CAL",
    "BOTH_WALK2_GRIEV", "BOTH_WALK2_PAL", "BOTH_WALK2_DOOKU", "BOTH_WALK2_MAUL", "BOTH_WALK2_YODA",
    "BOTH_WALK_STAFF", "BOTH_WALK_STAFF_BEN//#Normalwalkwithstaff", "BOTH_WALK_STAFF_PAL",
    "BOTH_WALK_STAFF_MAUL", "BOTH_WALK_STAFF_AMD", "BOTH_WALK_DUAL", "BOTH_WALK_DUAL_AMD", "BOTH_WALKBACK1",
    "BOTH_WALKBACK1_VADER", "BOTH_WALKBACK1_GRIEV", "BOTH_WALKBACK_PISTOL", "BOTH_WALKBACK_BLASTER",
    "BOTH_WALKBACK_HEAVY", "BOTH_WALKBACK_GRENADE", "BOTH_WALKBACK2", "BOTH_WALKBACK2_YODA",
    "BOTH_WALKBACK2_GRIEV", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_STAFF_MAUL", "BOTH_WALKBACK_DUAL",
    "BOTH_WALKBACK_DUAL_GALEN", "BOTH_WALKBACK_DUAL_MAUL", "BOTH_WALKBACK_DUALPISTOL", "BOTH_RUN1",
    "BOTH_RUN1_YODA", "BOTH_RUN1_VADER", "BOTH_RUN1_PAL", "BOTH_RUN1_MAUL", "BOTH_RUN1_OBI3", "BOTH_RUN1_REY",
    "BOTH_RUN1_BDROID", "BOTH_RUN1_GRIEV", "BOTH_SPRINT", "BOTH_SPRINT_BEN", "BOTH_SPRINT_YODA",
    "BOTH_SPRINT_VADER", "BOTH_SPRINT_GALEN", "BOTH_SPRINT_BDROID", "BOTH_SPRINT_GRIEV", "BOTH_SPRINT_PAL",
    "BOTH_SPRINT_MAUL", "BOTH_SPRINT_SINGLE_LIGHTSABER", "BOTH_SPRINT_SINGLE_LIGHTSABER_BEN",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_YODA", "BOTH_SPRINT_SINGLE_LIGHTSABER_VADER",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_GALEN", "BOTH_SPRINT_SINGLE_LIGHTSABER_OBI3",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_REN", "BOTH_SPRINT_SINGLE_LIGHTSABER_REY",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_BDROID", "BOTH_SPRINT_SINGLE_LIGHTSABER_CAL",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_GRIEV", "BOTH_SPRINT_SINGLE_LIGHTSABER_PAL",
    "BOTH_SPRINT_SINGLE_LIGHTSABER_DOOKU", "BOTH_SPRINT_SINGLE_LIGHTSABER_MAUL", "BOTH_SPRINT_STAFF_LIGHTSABER",
    "BOTH_SPRINT_DUAL_LIGHTSABER", "BOTH_SPRINT_DUAL_LIGHTSABER_BEN", "BOTH_SPRINT_DUAL_LIGHTSABER_GALEN",
    "BOTH_SPRINT_DUAL_LIGHTSABER_BDROID", "BOTH_SPRINT_DUAL_LIGHTSABER_PAL", "BOTH_SPRINT_DUAL_LIGHTSABER_MAUL",
    "BOTH_RUN2", "BOTH_RUN2_YODA", "BOTH_RUN_STAFF", "BOTH_RUN_STAFF_BEN", "BOTH_RUN_STAFF_MAUL",
    "BOTH_RUN_STAFF_REY", "BOTH_RUN_STAFF_BDROID", "BOTH_RUN_DUAL", "BOTH_RUN_DUAL_BEN", "BOTH_RUNBACK1",
    "BOTH_RUNBACK1_VADER", "BOTH_RUNBACK2", "BOTH_RUNBACK2_YODA", "BOTH_RUNBACK2_VADER", "BOTH_RUNBACK2_GALEN",
    "BOTH_RUNBACK2_GRIEV", "BOTH_RUNBACK_DUAL_GALEN", "BOTH_RUNBACK_STAFF", "BOTH_RUNBACK_STAFF_MAUL",
    "BOTH_MENUIDLE1", "BOTH_SABERSINGLECROUCH", "BOTH_SABERSINGLECROUCH_ANI", "BOTH_SABERSINGLECROUCH_VADER",
    "BOTH_SABERSINGLECROUCH_OBI", "BOTH_SABERSINGLECROUCH_DF2", "BOTH_SABERSINGLECROUCH_OBI3",
    "BOTH_SABERSINGLECROUCH_REN", "BOTH_SABERSINGLECROUCH_REY", "BOTH_SABERDUALCROUCH",
    "BOTH_SABERDUALCROUCH_GALEN", "BOTH_SABERDUALCROUCH_KOTOR", "BOTH_SABERDUALCROUCH_DF2",
    "BOTH_SABERSTAFFCROUCH", "BOTH_SABERSTAFFCROUCH_BDROID", "BOTH_SABERSTAFFCROUCH_KOTOR", "BOTH_WALK1_STICK",
    "BOTH_WALK1_STICK_YODA", "BOTH_WALK1_ANI", "BOTH_WALK2_ANI", "BOTH_WALK_DUAL_ANI", "BOTH_WALK_DUAL_GALEN",
    "BOTH_WALK_DUAL_BDROID", "BOTH_WALK_DUAL_MAUL", "BOTH_RUN1_ANI", "BOTH_RUN2_ANI", "BOTH_RUN2_GALEN",
    "BOTH_RUN2_OBI3", "BOTH_RUN2_REN", "BOTH_RUN2_REY", "BOTH_RUN2_BDROID", "BOTH_RUN2_CAL", "BOTH_RUN2_GRIEV",
    "BOTH_RUN2_DOOKU", "BOTH_RUN_DUAL_ANI", "BOTH_RUN_DUAL_GALEN", "BOTH_RUN_DUAL_REY", "BOTH_RUN_DUAL_BDROID",
    "BOTH_RUN_STAFF_ANI", "BOTH_SPRINT_SINGLE_LIGHTSABER_ANI", "BOTH_SPRINT_STAFF_LIGHTSABER_ANI",
    "BOTH_SPRINT_STAFF_LIGHTSABER_GALEN", "BOTH_SPRINT_STAFF_LIGHTSABER_BDROID",
    "BOTH_SPRINT_STAFF_LIGHTSABER_CAL", "BOTH_SPRINT_STAFF_LIGHTSABER_PAL", "BOTH_SPRINT_STAFF_LIGHTSABER_MAUL",
    "BOTH_RUN1_BEN", "BOTH_RUN2_BEN", "BOTH_WALK1_BEN", "BOTH_WALK2_BEN",
};
// PM_TorsoAnimLightsaber MD: mit Stab oder zwei Klingen gehen (bg_panimate.cpp:9244)
constexpr std::string_view kStabGehenMd[] = {
    "BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_STAFF_MAUL",
    "BOTH_WALKBACK_DUAL", "BOTH_WALKBACK_DUAL_GALEN", "BOTH_WALKBACK_DUAL_MAUL", "BOTH_WALK_STAFF_AMD",
    "BOTH_WALK_STAFF_BEN", "BOTH_WALK_STAFF_PAL", "BOTH_WALK_STAFF_MAUL", "BOTH_WALK_DUAL_AMD", "BOTH_WALK1",
    "BOTH_WALK1_VADER", "BOTH_WALK1_GALEN", "BOTH_WALK1_OBI3", "BOTH_WALK1_REN", "BOTH_WALK1_REY",
    "BOTH_WALK1_JANGO", "BOTH_WALK1_BDROID", "BOTH_WALK1_CAL", "BOTH_WALK1_GRIEV", "BOTH_WALK1_PAL",
    "BOTH_WALK1_DOOKU", "BOTH_WALK1_MAUL", "BOTH_WALK1_YODA", "BOTH_WALK1_MDA", "BOTH_WALK1_MDA_PAL",
    "BOTH_WALK1_MDA_BDROID", "BOTH_WALKBACK1", "BOTH_WALKBACK1_VADER", "BOTH_WALKBACK1_GRIEV", "BOTH_WALK2",
    "BOTH_WALK2_VADER", "BOTH_WALK2_GALEN", "BOTH_WALK2_OBI3", "BOTH_WALK2_REN", "BOTH_WALK2_REY",
    "BOTH_WALK2_JANGO", "BOTH_WALK2_BDROID", "BOTH_WALK2_CAL", "BOTH_WALK2_GRIEV", "BOTH_WALK2_PAL",
    "BOTH_WALK2_DOOKU", "BOTH_WALK2_MAUL", "BOTH_WALK2_YODA", "BOTH_WALK1_ANI", "BOTH_WALK2_ANI",
    "BOTH_WALK_DUAL_ANI", "BOTH_WALK_DUAL_GALEN", "BOTH_WALK_DUAL_BDROID", "BOTH_WALK_DUAL_MAUL",
    "BOTH_WALK1_BEN", "BOTH_WALK2_BEN", "BOTH_WALKBACK2", "BOTH_WALKBACK2_YODA", "BOTH_WALKBACK2_GRIEV",
};

template <std::size_t N>
bool steht(const std::string_view (&liste)[N], const std::string& anim) {
    return std::find(std::begin(liste), std::end(liste), std::string_view(anim)) != std::end(liste);
}


bool einer(const std::string& a, std::initializer_list<const char*> namen) {
    for (const char* n : namen) {
        if (a == n) { return true; }
    }
    return false;
}

// AngleNormalize180 / AngleDelta (q_math.cpp).
float winkel180(float a) {
    a = std::fmod(a, 360.0F);
    if (a < 0.0F) { a += 360.0F; }
    return a > 180.0F ? a - 360.0F : a;
}
float winkelDelta(float a, float b) { return winkel180(a - b); }

// Waffengruppen, wie PM_Footsteps sie in Movie Duels abfragt (bg_pmove.cpp
// 13921 ff. mdgit, mit g_ActivateAnimationStyle 0).
bool mdPistole(const std::string& w, bool laufen) {
    return einer(w, {"WP_BLASTER_PISTOL", "WP_BRYAR_PISTOL", "WP_DUAL_PISTOL", "WP_DUAL_CLONEPISTOL", "WP_REY", "WP_CLONEPISTOL",
                     "WP_REBELBLASTER", "WP_JANGO", "WP_STUN_BATON"}) ||
           (laufen && w == "WP_SBD_BLASTER");
}
bool mdGewehr(const std::string& w) {
    return einer(w, {"WP_DISRUPTOR", "WP_BLASTER", "WP_BATTLEDROID", "WP_THEFIRSTORDER", "WP_CLONECARBINE", "WP_CLONERIFLE",
                     "WP_CLONECOMMANDO", "WP_BOBA", "WP_REBELRIFLE", "WP_REPEATER"});
}
bool mdSchwer(const std::string& w) {
    return einer(w, {"WP_CONCUSSION", "WP_ROCKET_LAUNCHER", "WP_BOWCASTER", "WP_FLECHETTE", "WP_DEMP2", "WP_Z6_ROTARY_CANNON"});
}
bool mdGranate(const std::string& w) { return einer(w, {"WP_THERMAL", "WP_DET_PACK", "WP_TRIP_MINE"}); }

// PM_SaberDrawPutawayAnim (bg_pmove.cpp OpenJK; MD mit den Figurenvarianten).
bool ziehtSchwert(const std::string& a, bool md) {
    if (einer(a, {"BOTH_STAND1TO2", "BOTH_STAND2TO1", "BOTH_S1_S7", "BOTH_S7_S1", "BOTH_S1_S6", "BOTH_S6_S1"})) { return true; }
    return md && einer(a, {"BOTH_DOOKU_FULLDRAW", "BOTH_DOOKU_SMALLDRAW", "BOTH_GRIEVOUS_SABERON", "BOTH_S1_S6_GALEN",
                           "BOTH_S1_S6_GRIEV", "BOTH_S1_S6_MAUL", "BOTH_S1_S6_PAL", "BOTH_S1_S7_AMD", "BOTH_S1_S7_CAL",
                           "BOTH_S1_S7_GALEN", "BOTH_S1_S7_GRIEV", "BOTH_S1_S7_MAUL", "BOTH_S6_S1_GALEN", "BOTH_S6_S1_MAUL",
                           "BOTH_S6_S1_PAL", "BOTH_S7_S1_GALEN", "BOTH_S7_S1_MAUL", "BOTH_SABER_BACKHAND_IGNITION",
                           "BOTH_SABER_IGNITION", "BOTH_SABER_IGNITION_JFA", "BOTH_STAND1TO2_CAL", "BOTH_STAND1TO2_DOOKU",
                           "BOTH_STAND1TO2_GALEN", "BOTH_STAND1TO2_GRIEV", "BOTH_STAND1TO2_MAUL", "BOTH_STAND1TO2_PAL",
                           "BOTH_STAND1TO2_REN", "BOTH_STAND1TO2_VADER", "BOTH_STAND1TO2_YODA", "BOTH_STAND2TO1_CAL",
                           "BOTH_STAND2TO1_DOOKU", "BOTH_STAND2TO1_GALEN", "BOTH_STAND2TO1_MAUL", "BOTH_STAND2TO1_PAL",
                           "BOTH_STAND2TO1_REN", "BOTH_STAND2TO1_VADER", "BOTH_STAND2TO1_YODA"});
}

// ps.weapon > WP_SABER && ps.weapon < WP_DET_PACK (weapons.h): die Folge
// WP_BLASTER_PISTOL .. WP_TRIP_MINE; Movie Duels haengt seine Waffen hinter
// WP_DET_PACK an.
bool waffeVorSprengsatz(const std::string& w) {
    return einer(w, {"WP_BLASTER_PISTOL", "WP_BLASTER", "WP_DISRUPTOR", "WP_BOWCASTER", "WP_REPEATER", "WP_DEMP2", "WP_FLECHETTE",
                     "WP_ROCKET_LAUNCHER", "WP_THERMAL", "WP_TRIP_MINE"});
}

PmBeinWahl beine(const char* a) { return PmBeinWahl{a, false, false}; }

// Anlauf (ATST, Movie Duels auch Droideka): erst ziel_START mit HOLD, wenn
// dessen Timer abgelaufen ist das Ziel (PM_Footsteps, "he should really start
// slow and have to accelerate").
PmBeinWahl anlauf(const PmBeinStand& z, const char* ziel) {
    if (z.beine == ziel) { return beine(ziel); }
    if (z.beine != "BOTH_RUN1START") { return PmBeinWahl{"BOTH_RUN1START", false, true}; }
    if (!z.beineTimer) { return beine(ziel); }
    return PmBeinWahl{};
}
// Auslauf (Movie Duels, ATST und Droideka): aus dem Lauf erst BOTH_RUN1STOP mit HOLD.
PmBeinWahl auslauf(const PmBeinStand& z, const char* ziel, const char* lauf) {
    if (z.beine == ziel) { return beine(ziel); }
    if (z.beine != "BOTH_RUN1STOP" && z.beine == lauf) { return PmBeinWahl{"BOTH_RUN1STOP", false, true}; }
    if (!z.beineTimer) { return beine(ziel); }
    return PmBeinWahl{};
}

// --- Im Stand -------------------------------------------------------------
// Wo PM_Footsteps im Stand "if ( !PM_AdjustStandAnimForSlope() )" schreibt:
// am Hang dessen Beinanimation (leer: qfalse).
std::string hangJetzt(const PmBeinStand& z) { return z.hang ? z.hang() : std::string(); }
PmBeinWahl standOderHang(const PmBeinStand& z, const char* stand) {
    std::string h = hangJetzt(z);
    return h.empty() ? beine(stand) : PmBeinWahl{std::move(h), false, false};
}

PmBeinWahl imStand(const PmFigur& f, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    const std::string& beineJetzt = z.beine;
    if (k == "CLASS_ATST" || (f.md && k == "CLASS_DROIDEKA")) { return standOderHang(z, "BOTH_STAND1"); }
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{"BOTH_STAND2", true, false}; }
    // Mit Klinge (saberInFlight gibt es beim Steuern nicht). Movie Duels:
    // SaberActive() || (!g_noIgniteTwirl && ...) - g_noIgniteTwirl ist 1.
    if (w == "WP_SABER" && f.klingeLaenge && (!f.md || f.klingeAn) && !ziehtSchwert(beineJetzt, f.md)) {
        return standOderHang(z, saberHaltungFuerStil(f.saberStil, f.md));
    }
    if (waffeVorSprengsatz(w)) {
        // "Don't auto-override the guard idles"
        if (einer(beineJetzt, {"BOTH_GUARD_LOOKAROUND1", "BOTH_GUARD_IDLE1", "BOTH_STAND2TO4", "BOTH_STAND4TO2", "BOTH_STAND4"}) ||
            (f.md && einer(beineJetzt, {"BOTH_GUARD_IDLE1_BDROID", "BOTH_GUARD_IDLE1_JANGO"}))) {
            return PmBeinWahl{};
        }
        return standOderHang(z, "BOTH_STAND3");
    }
    // sonst: erst der Hang, dann die Grundhaltung nach Klasse und Waffe
    if (std::string h = hangJetzt(z); !h.empty()) { return PmBeinWahl{std::move(h), false, false}; }
    if (k == "CLASS_IMPERIAL") { return beine("BOTH_STAND4"); }
    if (w == "WP_TUSKEN_STAFF") { return beine("BOTH_STAND1"); }
    // CLASS_RANCOR (count, enemy, wait) und CLASS_WAMPA (count): ohne Opfer und
    // Feind die Grundhaltung.
    if (k == "CLASS_RANCOR" || k == "CLASS_WAMPA") { return beine("BOTH_STAND1"); }
    if (f.md && k == "CLASS_YODA") { return beine(w == "WP_MELEE" ? "BOTH_STANDYODA_STICK" : "BOTH_STAND1"); }
    if (f.md && w == "WP_MELEE") { return beine("BOTH_MD_CIN_1"); }
    return beine("BOTH_STAND1");
}

// --- Rueckwaerts (PMF_BACKWARDS_RUN) --------------------------------------
PmBeinWahl rueckwaertsOj(const PmFigur& f, bool gehen) {
    const bool klinge = f.waffe == "WP_SABER" && f.klingeAn;
    if (!gehen) {
        if (klinge) { return beine("BOTH_RUNBACK2"); }
        if (f.klasse == "CLASS_RANCOR") { return beine("BOTH_WALKBACK1"); }
        return beine("BOTH_RUNBACK1");
    }
    if (klinge) {
        if (f.saberStil == 6) { return beine("BOTH_WALKBACK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALKBACK_STAFF"); }
        return beine("BOTH_WALKBACK2");
    }
    return beine("BOTH_WALKBACK1");
}

PmBeinWahl rueckwaertsMd(const PmFigur& f, bool gehen, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    if (!gehen) {
        // Ein NPC (nicht vom Spieler gesteuert) mit Klinge: BOTH_RUNBACK1.
        if (w == "WP_SABER" && f.klingeAn) { return beine("BOTH_RUNBACK1"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALKBACK1"); }
        if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return anlauf(z, "BOTH_RUNBACK1"); }
        if (mdPistole(w, false)) {
            if ((k == "CLASS_REBORN" && w == "WP_BLASTER_PISTOL") || f.zweiPistolen ||
                (k == "CLASS_CALONORD" && w == "WP_REBELBLASTER")) {
                return beine("BOTH_RUNBACK_DUALPISTOL");
            }
            return beine("BOTH_RUNBACK_PISTOL");
        }
        if (mdGewehr(w)) { return beine("BOTH_RUNBACK_BLASTER"); }
        if (mdSchwer(w)) { return beine("BOTH_RUNBACK_HEAVY"); }
        if (mdGranate(w)) { return beine("BOTH_RUNBACK_GRENADE"); }
        return beine("BOTH_RUNBACK1");
    }
    if (w == "WP_SABER" && f.klingeAn) {
        if (ziehtSchwert(z.torso, true)) { return beine("BOTH_WALKBACK1"); }
        if (f.saberStil == 6) { return beine("BOTH_WALKBACK_DUAL"); }
        // SS_STAFF mit g_SerenityJediEngineMode 0 (wirksam, siehe scene.h): BOTH_WALKBACK_STAFF.
        if (f.saberStil == 7) { return beine("BOTH_WALKBACK_STAFF"); }
        return beine("BOTH_WALKBACK2");
    }
    if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return auslauf(z, "BOTH_WALKBACK1", "BOTH_RUNBACK1"); }
    if (mdPistole(w, false)) {
        if ((k == "CLASS_REBORN" && w == "WP_BLASTER_PISTOL") || f.zweiPistolen ||
            (k == "CLASS_CALONORD" && w == "WP_REBELBLASTER")) {
            return beine("BOTH_WALKBACK_DUALPISTOL");
        }
        return beine("BOTH_WALKBACK_PISTOL");
    }
    if (mdGewehr(w)) { return beine("BOTH_WALKBACK_BLASTER"); }
    if (mdSchwer(w)) { return beine("BOTH_WALKBACK_HEAVY"); }
    if (mdGranate(w)) { return beine("BOTH_WALKBACK_GRENADE"); }
    return beine("BOTH_WALKBACK1");
}

// --- Vorwaerts und seitwaerts ---------------------------------------------
PmBeinWahl vorwaertsOj(const PmFigur& f, bool gehen, const PmBeinStand& z) {
    const std::string& k = f.klasse;
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{f.waffe.empty() || f.waffe == "WP_NONE" ? "BOTH_WALK1" : "BOTH_WALK2", true, false}; }
    const bool klinge = f.waffe == "WP_SABER" && f.klingeAn;
    if (!gehen) {
        if (klinge) {
            if (f.saberStil == 6) { return beine("BOTH_RUN_DUAL"); }
            if (f.saberStil == 7) { return beine("BOTH_RUN_STAFF"); }
            return beine("BOTH_RUN2");
        }
        if (k == "CLASS_JAWA") { return beine("BOTH_RUN4"); }
        if (k == "CLASS_ATST") { return anlauf(z, "BOTH_RUN1"); }
        if (k == "CLASS_WAMPA") { return beine(f.laufTempo == 300.0F ? "BOTH_RUN1" : "BOTH_RUN2"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALK1"); }
        return beine("BOTH_RUN1");
    }
    if (klinge) {
        if (f.saberStil == 6) { return beine("BOTH_WALK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALK_STAFF"); }
        return beine("BOTH_WALK2");
    }
    // CLASS_WAMPA geht mit health <= 50 BOTH_WALK2 - die Figur ist beim Steuern heil.
    return beine("BOTH_WALK1");
}

PmBeinWahl vorwaertsMd(const PmFigur& f, int vor, bool gehen, const PmBeinStand& z) {
    const std::string& w = f.waffe;
    const std::string& k = f.klasse;
    if (k == "CLASS_GALAKMECH") { return PmBeinWahl{w.empty() || w == "WP_NONE" ? "BOTH_WALK1" : "BOTH_WALK2", true, false}; }
    if (!gehen) {
        if (w == "WP_SABER") {
            // PM_GetRunAnim
            if (ziehtSchwert(z.torso, true) || !f.klingeAn) { return beine("BOTH_RUN1"); }
            if (f.saberStil == 6) { return beine("BOTH_RUN_DUAL"); }
            if (f.saberStil == 7) { return beine("BOTH_RUN_STAFF"); }
            return beine("BOTH_RUN2");
        }
        if (w.empty() || w == "WP_MELEE" || w == "WP_NONE") { return beine("BOTH_RUN1"); }
        if (k == "CLASS_JAWA") { return beine("BOTH_RUN4"); }
        if (mdPistole(w, true)) { return beine(f.zweiPistolen ? "BOTH_JOG_DOUBLE_PISTOL" : "BOTH_JOG_PISTOL"); }
        if (mdGewehr(w)) { return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_RUN1" : "BOTH_JOG_BLASTER"); }
        if (mdSchwer(w)) { return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_RUN1" : "BOTH_JOG_HEAVY"); }
        if (mdGranate(w)) { return beine("BOTH_JOG_GRENADE"); }
        if (k == "CLASS_ATST" || (k == "CLASS_DROIDEKA" && !z.inKamera)) { return anlauf(z, "BOTH_RUN1"); }
        if (k == "CLASS_WAMPA") { return beine(f.laufTempo == 300.0F ? "BOTH_RUN1" : "BOTH_RUN2"); }
        if (k == "CLASS_RANCOR") { return beine("BOTH_WALK1"); }
        if (k == "CLASS_SBD") { return PmBeinWahl{"BOTH_RUN1", true, true}; }
        return beine("BOTH_RUN1");
    }
    if (w == "WP_SABER" && f.klingeAn) {
        // PM_GetWalkWithSaberOnAnim fuer einen NPC
        if (ziehtSchwert(z.torso, true)) { return beine("BOTH_WALK1"); }
        if (f.saberStil == 6) { return beine("BOTH_WALK_DUAL"); }
        if (f.saberStil == 7) { return beine("BOTH_WALK_STAFF"); }
        return beine("BOTH_WALK1");
    }
    if (k == "CLASS_DROIDEKA" && !z.inKamera) {
        if (z.beine != "BOTH_WALK1" && vor > 0) {
            if (z.beine != "BOTH_RUN1STOP" && z.beine == "BOTH_RUN1") { return PmBeinWahl{"BOTH_RUN1STOP", false, true}; }
            if (!z.beineTimer) { return beine("BOTH_WALK1"); }
            return PmBeinWahl{};
        }
        return beine("BOTH_WALK1");
    }
    if (k == "CLASS_WAMPA") { return beine("BOTH_WALK1"); }   // health > 50
    if (mdPistole(w, false)) { return beine(f.zweiPistolen ? "BOTH_WALK_DOUBLE_PISTOL" : "BOTH_WALK_PISTOL"); }
    if (mdGewehr(w) || einer(w, {"WP_DEMP2", "WP_BOWCASTER", "WP_FLECHETTE"})) {
        return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_WALK1" : "BOTH_WALK_BLASTER");
    }
    if (einer(w, {"WP_CONCUSSION", "WP_ROCKET_LAUNCHER", "WP_Z6_ROTARY_CANNON"})) {
        return beine(k == "CLASS_HAZARD_TROOPER" ? "BOTH_WALK1" : "BOTH_WALK_HEAVY");
    }
    if (mdGranate(w)) { return beine("BOTH_WALK_GRENADE"); }
    if (w == "WP_MELEE") {
        if (k == "CLASS_YODA") { return beine("BOTH_WALK1_YODA"); }
        if (k == "CLASS_VADER") { return beine("BOTH_WALK1_VADER"); }
        if (k == "CLASS_SITHLORD") { return beine("BOTH_WALK1_MDA"); }
        return beine("BOTH_WALK1");
    }
    return beine("BOTH_WALK1");
}

}  // namespace

void pmRueckwaertsSetzen(bool& rueckwaerts, int vor, int seit) noexcept {
    if (vor < 0) {
        rueckwaerts = true;
    } else if (vor > 0 || (vor == 0 && seit != 0)) {
        rueckwaerts = false;
    }
}

bool pmGehAnim(const std::string& a, bool md) {
    if (einer(a, {"BOTH_WALK1", "BOTH_WALK2", "BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALK5", "BOTH_WALK6", "BOTH_WALK7",
                  "BOTH_WALKBACK1", "BOTH_WALKBACK2", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL"})) {
        return true;
    }
    return md &&
           einer(a, {"BOTH_MENUIDLE1", "BOTH_WALK1_ANI", "BOTH_WALK1_BDROID", "BOTH_WALK1_BEN", "BOTH_WALK1_CAL",
                     "BOTH_WALK1_DOOKU", "BOTH_WALK1_GALEN", "BOTH_WALK1_GRIEV", "BOTH_WALK1_JANGO", "BOTH_WALK1_MAUL",
                     "BOTH_WALK1_MDA", "BOTH_WALK1_MDA_BDROID", "BOTH_WALK1_MDA_PAL", "BOTH_WALK1_OBI3", "BOTH_WALK1_PAL",
                     "BOTH_WALK1_REN", "BOTH_WALK1_REY", "BOTH_WALK1_STICK", "BOTH_WALK1_STICK_YODA", "BOTH_WALK1_VADER",
                     "BOTH_WALK1_YODA", "BOTH_WALK2_ANI", "BOTH_WALK2_BDROID", "BOTH_WALK2_BEN", "BOTH_WALK2_CAL",
                     "BOTH_WALK2_DOOKU", "BOTH_WALK2_GALEN", "BOTH_WALK2_GRIEV", "BOTH_WALK2_JANGO", "BOTH_WALK2_MAUL",
                     "BOTH_WALK2_OBI3", "BOTH_WALK2_PAL", "BOTH_WALK2_REN", "BOTH_WALK2_REY", "BOTH_WALK2_VADER",
                     "BOTH_WALK2_YODA", "BOTH_WALKBACK1_GRIEV", "BOTH_WALKBACK1_VADER", "BOTH_WALKBACK2_GRIEV",
                     "BOTH_WALKBACK2_YODA", "BOTH_WALKBACK_BLASTER", "BOTH_WALKBACK_DUALPISTOL", "BOTH_WALKBACK_DUAL_GALEN",
                     "BOTH_WALKBACK_DUAL_MAUL", "BOTH_WALKBACK_GRENADE", "BOTH_WALKBACK_HEAVY", "BOTH_WALKBACK_PISTOL",
                     "BOTH_WALKBACK_STAFF_MAUL", "BOTH_WALK_BAZOOKA", "BOTH_WALK_BAZOOKA_BDROID", "BOTH_WALK_BLASTER",
                     "BOTH_WALK_BLASTER_BDROID", "BOTH_WALK_DOUBLE_PISTOL", "BOTH_WALK_DOUBLE_PISTOL_BDROID",
                     "BOTH_WALK_DUAL_AMD", "BOTH_WALK_DUAL_ANI", "BOTH_WALK_DUAL_BDROID", "BOTH_WALK_DUAL_GALEN",
                     "BOTH_WALK_DUAL_MAUL", "BOTH_WALK_GRENADE", "BOTH_WALK_GRENADE_BDROID", "BOTH_WALK_HEAVY",
                     "BOTH_WALK_HEAVY_BDROID", "BOTH_WALK_MINIGUN", "BOTH_WALK_MINIGUN_BDROID", "BOTH_WALK_PISTOL",
                     "BOTH_WALK_PISTOL_BDROID", "BOTH_WALK_STAFF_AMD", "BOTH_WALK_STAFF_BEN", "BOTH_WALK_STAFF_MAUL",
                     "BOTH_WALK_STAFF_PAL"});
}

bool pmLaufAnim(const std::string& a, bool md) {
    if (einer(a, {"BOTH_RUN1", "BOTH_RUN2", "BOTH_RUN4", "BOTH_RUN_STAFF", "BOTH_RUN_DUAL", "BOTH_RUNBACK1", "BOTH_RUNBACK2",
                  "BOTH_RUNBACK_STAFF", "BOTH_RUN1START", "BOTH_RUN1STOP", "BOTH_RUNSTRAFE_LEFT1", "BOTH_RUNSTRAFE_RIGHT1"})) {
        return true;
    }
    if (!md) { return false; }
    // Movie Duels: dieselben, dazu JOG, SPRINT und die Figurenvarianten
    // (PM_RunningAnim, bg_pmove.cpp:10403-10547 mdgit, ohne Ausnahme).
    return einer(a, {"BOTH_JOG_BAZOOKA", "BOTH_JOG_BAZOOKA_GALEN", "BOTH_JOG_BLASTER", "BOTH_JOG_BLASTER_GALEN",
                     "BOTH_JOG_DOUBLE_PISTOL", "BOTH_JOG_DOUBLE_PISTOL_GALEN", "BOTH_JOG_GRENADE", "BOTH_JOG_GRENADE_GALEN",
                     "BOTH_JOG_HEAVY", "BOTH_JOG_HEAVY_GALEN", "BOTH_JOG_MINIGUN", "BOTH_JOG_MINIGUN_GALEN", "BOTH_JOG_PISTOL",
                     "BOTH_JOG_PISTOL_GALEN", "BOTH_RUN10", "BOTH_RUN1_ANI", "BOTH_RUN1_BDROID", "BOTH_RUN1_BEN",
                     "BOTH_RUN1_GRIEV", "BOTH_RUN1_MAUL", "BOTH_RUN1_OBI3", "BOTH_RUN1_PAL", "BOTH_RUN1_REY", "BOTH_RUN1_VADER",
                     "BOTH_RUN1_YODA", "BOTH_RUN2_ANI", "BOTH_RUN2_BDROID", "BOTH_RUN2_BEN", "BOTH_RUN2_CAL", "BOTH_RUN2_DOOKU",
                     "BOTH_RUN2_GALEN", "BOTH_RUN2_GRIEV", "BOTH_RUN2_OBI3", "BOTH_RUN2_REN", "BOTH_RUN2_REY", "BOTH_RUN2_YODA",
                     "BOTH_RUN5", "BOTH_RUN6", "BOTH_RUN7", "BOTH_RUN8", "BOTH_RUN9", "BOTH_RUNBACK1_VADER",
                     "BOTH_RUNBACK2_GALEN", "BOTH_RUNBACK2_GRIEV", "BOTH_RUNBACK2_VADER", "BOTH_RUNBACK2_YODA",
                     "BOTH_RUNBACK_BLASTER", "BOTH_RUNBACK_DUALPISTOL", "BOTH_RUNBACK_DUAL_GALEN", "BOTH_RUNBACK_GRENADE",
                     "BOTH_RUNBACK_HEAVY", "BOTH_RUNBACK_PISTOL", "BOTH_RUNBACK_STAFF_MAUL", "BOTH_RUN_DUAL_ANI",
                     "BOTH_RUN_DUAL_BDROID", "BOTH_RUN_DUAL_BEN", "BOTH_RUN_DUAL_GALEN", "BOTH_RUN_DUAL_REY",
                     "BOTH_RUN_STAFF_ANI", "BOTH_RUN_STAFF_BDROID", "BOTH_RUN_STAFF_BEN", "BOTH_RUN_STAFF_MAUL",
                     "BOTH_RUN_STAFF_REY"}) ||
           a.rfind("BOTH_SPRINT", 0) == 0;
}

PmBeinWahl pmFootstepsBeine(const PmFigur& f, int vor, int seit, bool gehen, bool rueckwaerts, const PmBeinStand& z) {
    if (vor == 0 && seit == 0) { return imStand(f, z); }
    if (rueckwaerts) { return f.md ? rueckwaertsMd(f, gehen, z) : rueckwaertsOj(f, gehen); }
    return f.md ? vorwaertsMd(f, vor, gehen, z) : vorwaertsOj(f, gehen, z);
}

float pmFussTempo(const std::string& anim, float resultSpeed, const PmFigur& f, double frameLerpMs, bool inKamera) {
    const bool kriechen = anim == "BOTH_CROUCH1WALK" || anim == "BOTH_CROUCH1WALKBACK";
    const bool geht = pmGehAnim(anim, f.md);
    if (!(geht || pmLaufAnim(anim, f.md) || kriechen)) { return 1.0F; }
    if (einer(f.klasse, {"CLASS_HOWLER", "CLASS_WAMPA", "CLASS_GONK", "CLASS_MOUSE", "CLASS_PROBE", "CLASS_PROTOCOL", "CLASS_R2D2",
                         "CLASS_R5D2", "CLASS_SEEKER"})) {
        return 1.0F;
    }
    if (f.md && resultSpeed < 1.0F && inKamera) { return 1.0F; }
    // F4b (B4b-15): animSpeed = 50 / frameLerp - bei negativen fps (frameLerp < 0, rueckwaerts) passt PM_SetAnimFinal
    // NICHT an ("&& !(animSpeed<0.0f)", bg_panimate.cpp:4756-4760 OpenJK; "animSpeed >= 0.0f" MD).
    if (frameLerpMs < 0.0) { return 1.0F; }
    float soll = 150.0F;
    if (kriechen) {
        soll = 75.0F;
    } else if (f.klasse == "CLASS_HAZARD_TROOPER") {
        soll = 50.0F;
    } else if (f.klasse == "CLASS_RANCOR") {
        soll = 173.0F;
    } else if (geht) {
        soll = (f.saberStil == 6 || f.saberStil == 7) ? 100.0F : 50.0F;
    } else {
        soll = f.saberStil == 7 ? 250.0F : 150.0F;
    }
    // animSpeed der Engine = 50/frameLerp * resultspeed/soll, auf 0,01 .. 1,5;
    // hier gegenueber der .cfg (50/frameLerp herausgerechnet).
    const double lerp = frameLerpMs > 0.0 ? frameLerpMs : 50.0;
    const double engine = std::clamp(50.0 / lerp * static_cast<double>(resultSpeed / soll), 0.01, 1.5);
    return static_cast<float>(engine * lerp / 50.0);
}

double pmStelle(const PmKnochen& k, double jetztMs) noexcept { return (jetztMs - k.startMs) * static_cast<double>(k.tempo); }

bool pmSetAnimNormal(PmKnochen& k, const std::string& anim, float tempo, double jetztMs, double blendMs, double halteMs) {
    if (anim.empty()) { return false; }
    if (k.timerBisMs >= kPmNie || (k.timerBisMs >= 0.0 && jetztMs < k.timerBisMs)) { return false; }
    if (k.anim == anim) {
        if (k.tempo == tempo) { return false; }
        // An derselben Stelle weiter (animCurrent = bodyCurrent), ohne BONE_ANIM_BLEND.
        const double stelle = pmStelle(k, jetztMs);
        k.tempo = tempo;
        k.startMs = jetztMs - (tempo > 0.0F ? stelle / static_cast<double>(tempo) : 0.0);
        return true;
    }
    if (!k.anim.empty() && blendMs > 0.0) {
        k.vorher = k.anim;
        k.vorherStelleMs = pmStelle(k, jetztMs);
        k.blendAbMs = jetztMs;
        k.blendMs = blendMs;
    } else {
        k.vorher.clear();
    }
    k.anim = anim;
    k.startMs = jetztMs;
    k.tempo = tempo;
    k.timerBisMs = halteMs >= 0.0 ? jetztMs + halteMs : -1.0;
    return true;
}

bool pmSaltoAnim(const std::string& a, bool md) { return steht(kSalto, a) || (md && steht(kSaltoMd, a)); }

bool pmDrehSchwertAnim(const std::string& a, bool md) { return steht(kDrehSchwert, a) || (md && steht(kDrehSchwertMd, a)); }

namespace {

bool sprungAnim(const std::string& a, bool md) { return steht(kSprung, a) || (md && steht(kSprungMd, a)); }
bool landungAnim(const std::string& a, bool md) { return steht(kLandung, a) || (md && steht(kLandungMd, a)); }
bool schmerzAnim(const std::string& a, bool md) { return steht(kSchmerz, a) || (md && steht(kSchmerzMd, a)); }
bool sondersprungAnim(const std::string& a, bool md) { return steht(kSondersprung, a) || (md && steht(kSondersprungMd, a)); }

// PM_Animationstyletable (MD): MATCH(s) = client->animationstyle == s oder g_animationstyle == s
// (Vorgabe "0" = CS_DEFAULT, das keine der Haltungen hier abfragt).
// rc608: PM_TurnAnimForLegsAnim (bg_panimate.cpp), die Tabellen aus den Quellen.
struct DrehEintrag {
    const char* beine;
    const char* anim;
};
const DrehEintrag kDrehOpenJK[] = {
    {"BOTH_STAND1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE2", "BOTH_TURNSTAND2"},
    {"BOTH_STAND3", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3IDLE1", "BOTH_TURNSTAND3"},
    {"BOTH_STAND4", "BOTH_TURNSTAND4"},
    {"BOTH_STAND5", "BOTH_TURNSTAND5"},
    {"BOTH_STAND5IDLE1", "BOTH_TURNSTAND5"},
    {"BOTH_CROUCH1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH1IDLE", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH2TOSTAND1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH3", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH3", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH4", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH4", "BOTH_TURNCROUCH1"},
};
const DrehEintrag kDrehMovieDuels[] = {
    {"BOTH_STAND1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1_BDROID", "BOTH_TURNSTAND1"},
    {"BOTH_STAND9", "BOTH_TURNSTAND1"},
    {"BOTH_STAND9IDLE1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND9IDLE1_ANI", "BOTH_TURNSTAND1"},
    {"BOTH_STAND9IDLE1_MD", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_YODA", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_VADER", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_GALEN", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_JANGO", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_BDROID", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_CAL", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_GRIEV", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_PAL", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1_MAUL", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_IDLE", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_IDLE_GRIEV", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_IDLE_MD", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_IDLE_DUELS", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_IDLE_STAFF", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_DUELS", "BOTH_TURNSTAND1"},
    {"BOTH_STAND_SABER_ON_STAFF", "BOTH_TURNSTAND1"},
    {"BOTH_STANDYODA_STICK", "BOTH_TURNSTAND1"},
    {"BOTH_STANDYODAIDLE_STICK", "BOTH_TURNSTAND1"},
    {"BOTH_STANDYODAIDLE_STICK_YODA", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2_JANGO", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2_CLO", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2_REB", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2P", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE2P_JANGO", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE3", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE3_BDROID", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE4", "BOTH_TURNSTAND1"},
    {"TORSO_WEAPONIDLE4_BDROID", "BOTH_TURNSTAND1"},
    {"BOTH_STANCE_MINIGUN", "BOTH_TURNSTAND1"},
    {"BOTH_STANCE_READY_MINIGUN_IDLE", "BOTH_TURNSTAND1"},
    {"BOTH_STAND2", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2_JKA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_JANGO", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_JANGO", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_FORWARD_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_JANGO", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_BACK_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_LEFT_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_RIGHT_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_FORWARD", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_FORWARD_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_FORWARD_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_FORWARD_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_FORWARD_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BACK", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BACK_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BACK_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BACK_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_BACK_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_RIGHT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_RIGHT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_RIGHT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_RIGHT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_RIGHT_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_LEFT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_LEFT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_LEFT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_LEFT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_DUAL_LEFT_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_FORWARD", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_FORWARD_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_FORWARD_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_FORWARD_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_FORWARD_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BACK", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BACK_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BACK_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BACK_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_BACK_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_RIGHT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_RIGHT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_RIGHT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_RIGHT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_RIGHT_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_LEFT", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_LEFT_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_LEFT_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_LEFT_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND_BLOCKING_ON_STAFF_LEFT_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_PAL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_LUKE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_MACE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_OBI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_KOTOR", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_OBI3", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_REN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_REY", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE_JKA_GRIEV", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_LUKE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_GON", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_OBI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_KOTOR", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_OBI3", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_REN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_REY", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_PAL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE_JKA_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_YODA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_MAUL", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_MACE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_OBI3", "BOTH_TURNSTAND2"},
    {"BOTH_SABERTAVION_STANCE_JKA_REN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA_DOOKU", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA_MACE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDESANN_STANCE_JKA_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_MD", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_LUKE", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_GON", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_MACE", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_OBI", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_KOTOR", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_OBI3", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_REN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_REY", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_CAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_PAL", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1_BEN", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_ANI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_VADER", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_OBI", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_OBI3", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_REN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSINGLECROUCH_REY", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDUALCROUCH", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDUALCROUCH_GALEN", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDUALCROUCH_KOTOR", "BOTH_TURNSTAND2"},
    {"BOTH_SABERDUALCROUCH_DF2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSTAFFCROUCH", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSTAFFCROUCH_BDROID", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSTAFFCROUCH_KOTOR", "BOTH_TURNSTAND2"},
    {"BOTH_STAND3", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3_BDROID", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3IDLE1", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3IDLE1_BDROID", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3IDLE1_JANGO", "BOTH_TURNSTAND3"},
    {"BOTH_STAND4", "BOTH_TURNSTAND4"},
    {"BOTH_STAND5", "BOTH_TURNSTAND5"},
    {"BOTH_STAND5IDLE1", "BOTH_TURNSTAND5"},
    {"BOTH_MENUIDLE1", "BOTH_TURNSTAND5"},
    {"BOTH_CROUCH1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH1IDLE", "BOTH_TURNCROUCH1"},
};
const DrehEintrag kDrehJk2[] = {
    {"BOTH_STAND1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND1IDLE1", "BOTH_TURNSTAND1"},
    {"BOTH_STAND2", "BOTH_TURNSTAND2"},
    {"BOTH_SABERFAST_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_SABERSLOW_STANCE", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE1", "BOTH_TURNSTAND2"},
    {"BOTH_STAND2IDLE2", "BOTH_TURNSTAND2"},
    {"BOTH_STAND3", "BOTH_TURNSTAND3"},
    {"BOTH_STAND3IDLE1", "BOTH_TURNSTAND3"},
    {"BOTH_STAND4", "BOTH_TURNSTAND4"},
    {"BOTH_STAND4IDLE1", "BOTH_TURNSTAND4"},
    {"BOTH_STAND5", "BOTH_TURNSTAND5"},
    {"BOTH_STAND5IDLE1", "BOTH_TURNSTAND5"},
    {"BOTH_CROUCH1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH1IDLE", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH2IDLE", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH2TOSTAND1", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH3", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH3", "BOTH_TURNCROUCH1"},
    {"BOTH_CROUCH4", "BOTH_TURNCROUCH1"},
    {"BOTH_UNCROUCH4", "BOTH_TURNCROUCH1"},
};
bool mdStil(const PmFigur& f, const char* stil) { return f.animStilAn && f.animStil == stil; }

// PM_ReadyPoseForSaberAnimLevelDucked (MD bg_pmove.cpp:17301)
std::string mdHaltungGeduckt(const PmFigur& f) {
    switch (f.saberStil) {
        case 6:   // SS_DUAL
            if (mdStil(f, "CS_GALEN_MAREK")) { return "BOTH_SABERDUALCROUCH_GALEN"; }
            if (mdStil(f, "CS_KOTOR")) { return "BOTH_SABERDUALCROUCH_KOTOR"; }
            if (mdStil(f, "CS_DARKFORCES2")) { return "BOTH_SABERDUALCROUCH_DF2"; }
            return "BOTH_SABERDUALCROUCH";
        case 7:   // SS_STAFF
            if (mdStil(f, "CS_BATTLEDROID")) { return "BOTH_SABERSTAFFCROUCH_BDROID"; }
            if (mdStil(f, "CS_KOTOR")) { return "BOTH_SABERSTAFFCROUCH_KOTOR"; }
            return "BOTH_SABERSTAFFCROUCH";
        case 1: case 2: case 3: case 4: case 5: {
            static const char* const kStil[][2] = {{"CS_ANAKIN", "_ANI"}, {"CS_VADER", "_VADER"}, {"CS_OBIWAN", "_OBI"},
                                                   {"CS_DARKFORCES2", "_DF2"}, {"CS_OBIWAN_EP3", "_OBI3"},
                                                   {"CS_KYLO_REN", "_REN"}, {"CS_REY", "_REY"}};
            for (const auto& s : kStil) {
                if (mdStil(f, s[0])) { return std::string("BOTH_SABERSINGLECROUCH") + s[1]; }
            }
            return "BOTH_SABERSINGLECROUCH";
        }
        default:
            return "BOTH_SABERSINGLECROUCH";
    }
}

// PM_ReadyPoseForSaberAnimLevel: OpenJK der switch, Movie Duels fuer NPCs
// PM_ReadyPoseForSaberAnimLevelNPC mit g_SerenityJediEngineMode 0 (die "JKA fallback"-
// Tabelle, bg_pmove.cpp:18688; Figurenvarianten nur fuer SS_DUAL und SS_STAFF).
std::string bereitHaltung(const PmFigur& f, const PmTorsoLage& lage) {
    if (!f.md) { return saberHaltungFuerStil(f.saberStil, false); }
    if (lage.geduckt) { return mdHaltungGeduckt(f); }
    if (f.saberStil == 6 || f.saberStil == 7) {
        static const char* const kStil[][2] = {{"CS_GALEN_MAREK", "_GALEN"}, {"CS_MOVIEDUELS", "_MD"}, {"CS_KOTOR", "_KOTOR"},
                                               {"CS_DARKFORCES2", "_DF2"}, {"CS_BATTLEDROID", "_BDROID"},
                                               {"CS_CAL_KESTIS", "_CAL"}, {"CS_GRIEVOUS", "_GRIEV"}, {"CS_PALPATINE", "_PAL"},
                                               {"CS_MAUL", "_MAUL"}};
        const bool dual = f.saberStil == 6;
        for (const auto& s : kStil) {
            const std::string_view n = s[0];
            // SS_DUAL ohne Kampfdroide, SS_STAFF ohne Grievous und Palpatine
            if ((dual && n == "CS_BATTLEDROID") || (!dual && (n == "CS_GRIEVOUS" || n == "CS_PALPATINE"))) { continue; }
            if (mdStil(f, s[0])) { return std::string(dual ? "BOTH_SABERDUAL_STANCE_JKA" : "BOTH_SABERSTAFF_STANCE_JKA") + s[1]; }
        }
    }
    return saberHaltungFuerStil(f.saberStil, true);
}

}  // namespace

PmTorsoWahl pmTorsoLichtschwert(const PmFigur& f, const std::string& beine, const PmTorsoLage& lage) {
    PmTorsoWahl w;
    const bool md = f.md;
    // springend, landend, Rad, Salto: der Oberkoerper folgt (PM_SetAnim SETANIM_TORSO legsAnim)
    if (sprungAnim(beine, md) || landungAnim(beine, md) || steht(kRad, beine) || pmSaltoAnim(beine, md)) {
        w.folgen = true;
        return w;
    }
    // "running w/1-handed weapon uses full-body anim" - nur ohne Blocken (saberBlockingTime < cg.time)
    const bool stabGehen = md ? steht(kStabGehenMd, beine)
                              : einer(beine, {"BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL"});
    if ((pmLaufAnim(beine, md) || stabGehen) && !f.blocken) {
        w.folgen = true;
        return w;
    }
    // --- PM_SetSaberMove(LS_READY) ---
    const bool arc = f.saberTyp == "SABER_ARC";
    const bool zweites = f.zweiSchwerter && f.zweitesAn;
    const bool stabLage = f.stab && (!f.eineKlingeStil || f.klinge2An);
    std::string a;
    if (!md) {
        if (f.blocken) {
            a = (f.saberTyp == "SABER_CLAW" || zweites) ? "BOTH_INAIR1" : "BOTH_P1_S1_T_";
        } else if (!f.bereitAnim[0].empty()) {
            a = f.bereitAnim[0];
        } else if (f.zweiSchwerter && !f.bereitAnim[1].empty()) {
            a = f.bereitAnim[1];
        } else if (arc) {
            a = "BOTH_SABERFAST_STANCE";
        } else if (zweites) {
            a = "BOTH_SABERDUAL_STANCE";
        } else if (stabLage) {
            a = "BOTH_SABERSTAFF_STANCE";
        } else if (f.saberTyp == "SABER_LANCE" || f.saberTyp == "SABER_TRIDENT") {
            a = "BOTH_STAND1";
        } else {
            a = bereitHaltung(f, lage);
        }
    } else {
        // ein NPC haelt nie MBF_HOLDINGBLOCK (nur ent->NPC == NULL, g_active.cpp)
        if (f.blocken) {
            a = (f.saberTyp == "SABER_CLAW" || zweites || stabLage || arc) ? bereitHaltung(f, lage)
                                                                           : "BOTH_STAND2";   // saberMoveData[LS_READY].animToUse
        } else if (!f.bereitAnim[0].empty()) {
            a = f.bereitAnim[0];
        } else if (f.zweiSchwerter && !f.bereitAnim[1].empty()) {
            a = f.bereitAnim[1];
        } else {
            a = bereitHaltung(f, lage);
        }
    }
    // "match torso anim to walk/run anim if newMove is just LS_READY"
    const bool stand1 = (a == "BOTH_STAND1" || (md && a == "BOTH_STAND1_BDROID")) && (arc || zweites);
    const bool passt = md ? (stand1 || steht(kBereitPassungMd, a))
                          : (stand1 || einer(a, {"BOTH_STAND2", "BOTH_SABERDUAL_STANCE", "BOTH_SABERFAST_STANCE", "BOTH_SABERSLOW_STANCE"}));
    if (passt &&
        (md ? steht(kGehPassungMd, beine)
            : einer(beine, {"BOTH_WALK1", "BOTH_WALK2", "BOTH_WALK_STAFF", "BOTH_WALK_DUAL", "BOTH_WALKBACK1", "BOTH_WALKBACK2",
                            "BOTH_WALKBACK_STAFF", "BOTH_WALKBACK_DUAL", "BOTH_RUN1", "BOTH_RUN2", "BOTH_RUN_STAFF", "BOTH_RUN_DUAL",
                            "BOTH_RUNBACK1", "BOTH_RUNBACK2", "BOTH_RUNBACK_STAFF"}))) {
        a = beine;
    }
    // SETANIM_TORSO oder SETANIM_BOTH
    if (f.blocken) {
        w.beide = true;   // !saberBlocked: beim Halten der Haltung ist kein Treffer abzuwehren
    } else if (pmDrehSchwertAnim(a, md) || (md && pmDrehSchwertAnim(beine, md))) {
        w.beide = true;
    } else if (!lage.eingabe || (md && lage.tempoNull)) {
        w.beide = !pmSaltoAnim(beine, md) && !lage.rolleOderLiegen && !sprungAnim(beine, md) && !schmerzAnim(beine, md) &&
                  !sondersprungAnim(beine, md) && !steht(kHang, beine) && !lage.geduckt;
    }
    w.anim = a;
    w.blendMs = kPmBlendSaberBereit;
    return w;
}

float pmBeinGier(float beinGier, float blickGier, float vx, float vy, bool rueckwaerts, double frameInterpolation) {
    constexpr float kMin = -60.0F;
    constexpr float kMax = 60.0F;
    float drehung = 10.0F;
    float dazu = 0.0F;
    if (vx != 0.0F || vy != 0.0F) {
        // vectoyaw
        float gier = std::atan2(vy, vx) * 180.0F / 3.14159265358979F;
        if (gier < 0.0F) { gier += 360.0F; }
        dazu = -winkelDelta(blickGier, gier);
        if (dazu > 150.0F || dazu < -150.0F) {
            dazu = 0.0F;
        } else {
            dazu = std::clamp(dazu, kMin, kMax);
            if (rueckwaerts) { dazu = -dazu; }
            drehung = 5.0F;
        }
    }
    float diff = winkelDelta(beinGier, blickGier + dazu);
    float neu = static_cast<float>(diff * frameInterpolation * -1.0);
    if (std::fabs(neu) > std::fabs(diff)) { neu = -diff; }
    neu = std::clamp(neu, -drehung, drehung);
    float gier = beinGier + neu;
    diff = winkelDelta(blickGier, gier);
    if (diff > kMax) {
        gier = blickGier - kMax;
    } else if (diff < kMin) {
        gier = blickGier - kMin;
    }
    return gier;
}

std::string pmDrehAnim(const std::string& beine, int spiel) {
    const auto suche = [&beine](const DrehEintrag* von, const DrehEintrag* bis) {
        for (const DrehEintrag* e = von; e != bis; ++e) {
            if (beine == e->beine) { return std::string(e->anim); }
        }
        return std::string();
    };
    if (spiel == kRegelwerkMovieDuels) { return suche(std::begin(kDrehMovieDuels), std::end(kDrehMovieDuels)); }
    if (spiel == kRegelwerkJediOutcast) { return suche(std::begin(kDrehJk2), std::end(kDrehJk2)); }
    return suche(std::begin(kDrehOpenJK), std::end(kDrehOpenJK));
}

}  // namespace bhed
