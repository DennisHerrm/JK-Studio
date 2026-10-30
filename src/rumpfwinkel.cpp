#include "bhed/rumpfwinkel.h"

#include "bhed/todesanim.h"   // engineVectoangles

#include <algorithm>
#include <cmath>
#include <string_view>

namespace bhed {

namespace {

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
// PM_InLedgeMove (nur Movie Duels)
constexpr std::string_view kKanteMd[] = {
    "BOTH_LEDGE_GRAB", "BOTH_LEDGE_GRAB_GALEN", "BOTH_LEDGE_HOLD", "BOTH_LEDGE_HOLD_GALEN",
    "BOTH_LEDGE_HOLD_IDLE_GALEN", "BOTH_LEDGE_LEFT", "BOTH_LEDGE_LEFT_GALEN", "BOTH_LEDGE_RIGHT",
    "BOTH_LEDGE_RIGHT_GALEN", "BOTH_LEDGE_MERCPULL", "BOTH_LEDGE_MERCPULL_GALEN", "BOTH_LEDGE_JEDIPULL",
    "BOTH_LEDGE_JEDIPULL_GALEN",
};

template <std::size_t N>
bool steht(const std::string_view (&liste)[N], const std::string& anim) {
    return std::find(std::begin(liste), std::end(liste), std::string_view(anim)) != std::end(liste);
}

// q_math.c: AngleNormalize360 auf 65536 Stufen, AngleNormalize180
float normal180(float a) noexcept {
    a = (360.0F / 65536) * static_cast<float>(static_cast<int>(a * (65536 / 360.0F)) & 65535);
    if (a > 180.0F) { a -= 360.0F; }
    return a;
}

}  // namespace

RumpfKlasse rumpfKlasseFuer(const std::string& klasse, bool md) noexcept {
    if (klasse == "CLASS_HAZARD_TROOPER") { return RumpfKlasse::HazardTrooper; }
    if (klasse == "CLASS_ASSASSIN_DROID") { return RumpfKlasse::AssassinDroid; }
    if (klasse == "CLASS_DROIDEKA" && md) { return RumpfKlasse::Droideka; }
    if (klasse == "CLASS_SABER_DROID") { return RumpfKlasse::SaberDroid; }
    if (klasse == "CLASS_RANCOR") { return RumpfKlasse::Rancor; }
    if (klasse == "CLASS_WAMPA") { return RumpfKlasse::Wampa; }
    if (klasse == "CLASS_ROCKETTROOPER") { return RumpfKlasse::RocketTrooper; }
    // CG_G2PlayerAngles: diese Klassen nehmen weder den Rumpf- noch den Halszweig
    // (ATST nur als Spieler, und die Vorschau zeigt NPCs)
    for (const char* k : {"CLASS_GONK", "CLASS_INTERROGATOR", "CLASS_SENTRY", "CLASS_PROBE", "CLASS_R2D2", "CLASS_R5D2",
                          "CLASS_ATST"}) {
        if (klasse == k) { return RumpfKlasse::Ohne; }
    }
    return RumpfKlasse::Mensch;
}

bool rumpfSchlechteKnochen(RumpfKlasse k) noexcept {
    switch (k) {
        case RumpfKlasse::Wampa:
        case RumpfKlasse::RocketTrooper:
        case RumpfKlasse::SaberDroid:
        case RumpfKlasse::HazardTrooper:
        case RumpfKlasse::AssassinDroid:
        case RumpfKlasse::Droideka:
        case RumpfKlasse::Rancor:
            return true;
        case RumpfKlasse::Mensch:
        case RumpfKlasse::Ohne:
            break;
    }
    return false;
}

void rumpfKnochenAchsen(RumpfKlasse k, const std::string& knochen, G2Achse& oben, G2Achse& rechts, G2Achse& vorn) noexcept {
    oben = G2Achse::PosX;
    rechts = G2Achse::NegY;
    vorn = G2Achse::NegZ;
    if (!rumpfSchlechteKnochen(k)) { return; }   // BG_G2SetBoneAngles mit POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z
    const auto setze = [&](G2Achse o, G2Achse r, G2Achse v) {
        oben = o;
        rechts = r;
        vorn = v;
    };
    const bool becken = knochen == "pelvis";
    switch (k) {
        case RumpfKlasse::Rancor:
            setze(G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
            break;
        case RumpfKlasse::RocketTrooper:
        case RumpfKlasse::HazardTrooper:
            if (becken) {
                setze(G2Achse::PosZ, G2Achse::NegX, G2Achse::NegY);
            } else {
                setze(G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
            }
            break;
        case RumpfKlasse::SaberDroid:
            if (becken || knochen == "thoracic") {
                setze(G2Achse::NegX, G2Achse::NegZ, G2Achse::NegY);
            } else {
                setze(G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
            }
            break;
        case RumpfKlasse::Wampa:
            if (becken) {
                setze(G2Achse::NegX, G2Achse::PosY, G2Achse::NegZ);
            } else {
                setze(G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
            }
            break;
        case RumpfKlasse::AssassinDroid:
        case RumpfKlasse::Droideka:
            if (becken || knochen == "lower_lumbar" || knochen == "upper_lumbar") {
                setze(G2Achse::NegX, G2Achse::PosY, G2Achse::PosZ);
            }
            break;
        case RumpfKlasse::Mensch:
        case RumpfKlasse::Ohne:
            break;
    }
}

bool rumpfMotionAusgleich(RumpfKlasse k, const std::string& beine, const std::string& torso, bool md) {
    const auto salto = [md](const std::string& a) { return steht(kSalto, a) || (md && steht(kSaltoMd, a)); };
    const auto dreh = [md](const std::string& a) { return steht(kDrehSchwert, a) || (md && steht(kDrehSchwertMd, a)); };
    return k != RumpfKlasse::Ohne && k != RumpfKlasse::SaberDroid && !salto(beine) && !dreh(beine) && !dreh(torso) &&
           beine != torso && !rumpfSchlechteKnochen(k);
}

bool rumpfAnDerKante(const std::string& beine, bool md) { return md && steht(kKanteMd, beine); }

void motionWinkelAusBolzen(const float bolzen[3][4], float winkel[3]) noexcept {
    // G2API_GiveMeVectorFromMatrix: NEGATIVE_Y = -Spalte 1, NEGATIVE_X = -Spalte 0
    const float vorn[3] = {-bolzen[0][1], -bolzen[1][1], -bolzen[2][1]};
    const float rechts[3] = {-bolzen[0][0], -bolzen[1][0], -bolzen[2][0]};
    engineVectoangles(vorn, winkel);
    float hilf[3];
    engineVectoangles(rechts, hilf);
    winkel[2] = -hilf[0];
}

RumpfWinkel rumpfWinkel(const RumpfEingabe& e) noexcept {
    RumpfWinkel w;
    const RumpfKlasse k = e.klasse;
    if (k == RumpfKlasse::Ohne) { return w; }
    constexpr int P = 0;
    constexpr int Y = 1;
    constexpr int R = 2;
    // viewAngles[YAW] = AngleDelta(lerpAngles[YAW], angles[YAW])
    float v[3] = {e.blick[0], normal180(e.blick[1]), e.blick[2]};
    float* th = w.thoracic;
    float* ul = w.upper;
    float* ll = w.lower;
    // --- CG_G2ClientSpineAngles ---
    if (k != RumpfKlasse::SaberDroid) {   // "don't use lower bones": nichts gesetzt
        if (e.motion) {
            for (int a = 0; a < 3; ++a) { v[a] = normal180(v[a] - normal180(e.motionWinkel[a])); }
        }
        // motionBoneCorrectAngles bleibt 0 (nur cg_motionBoneComp > 2 rechnet es)
        if (k == RumpfKlasse::HazardTrooper) {
            th[P] = th[Y] = th[R] = 0.0F;
            ul[P] = v[P] * 0.50F;
            ll[P] = v[P] * 0.50F;
            ul[Y] = v[Y] * 0.45F;
            ll[Y] = v[Y] * 0.55F;
            ul[R] = v[R] * 0.45F;
            ll[R] = v[R] * 0.55F;
        } else if (k == RumpfKlasse::AssassinDroid || k == RumpfKlasse::Droideka) {
            th[P] = v[P] * 0.40F;
            ul[P] = 0.0F;
            ll[P] = v[P] * 0.60F;
            th[Y] = 0.0F;
            ul[Y] = v[Y];
            ll[Y] = 0.0F;
            th[R] = 0.0F;
            ul[R] = 0.0F;
            ll[R] = 0.0F;
        } else {
            if (e.kante) {   // Movie Duels
                th[P] = ll[P] = ul[P] = 0.0F;
            } else {
                th[P] = v[P] * 0.20F;
                ul[P] = v[P] * 0.40F;
                ll[P] = v[P] * 0.40F;
            }
            th[Y] = v[Y] * 0.20F;
            ul[Y] = v[Y] * 0.35F;
            ll[Y] = v[Y] * 0.45F;
            th[R] = v[R] * 0.20F;
            ul[R] = v[R] * 0.35F;
            ll[R] = v[R] * 0.45F;
        }
        if (k == RumpfKlasse::Rancor) {
            ll[Y] = ll[R] = 0.0F;
            ul[Y] = ul[R] = 0.0F;
        }
        w.lumbarSetzen = true;
    }
    // --- CG_G2ClientNeckAngles ---
    if (k == RumpfKlasse::HazardTrooper) { return w; }   // "don't use upper bones"
    float la[3] = {e.kopf[0], e.kopf[1], e.kopf[2]};
    if (k == RumpfKlasse::Rancor) {
        // "always use the viewAngles we calced", CG_UpdateLookAngles kappt auf -50..50, -70..70, -30..30
        la[P] = std::clamp(v[P], -50.0F, 50.0F);
        la[Y] = std::clamp(v[Y], -70.0F, 70.0F);
        la[R] = std::clamp(v[R], -30.0F, 30.0F);
    }
    static constexpr float kMin[3] = {-25.0F, -55.0F, -10.0F};
    static constexpr float kMax[3] = {50.0F, 50.0F, 10.0F};
    for (int a = 0; a < 3; ++a) {
        if (la[a] < kMin[a]) {
            la[a] = kMin[a];
        } else if (la[a] > kMax[a]) {
            la[a] = kMax[a];
        }
    }
    float* hals = w.cervical;
    float* kopf = w.cranium;
    // "already been set above, blend them" - auch wenn der Blick 0 ist
    const auto mische = [](float t, float l) { return t != 0.0F ? (t + l) * 0.5F : l; };
    if (k == RumpfKlasse::AssassinDroid || k == RumpfKlasse::Droideka) {
        th[P] = mische(th[P], la[P] * 0.5F);
        th[Y] = th[R] = 0.0F;
        hals[P] = la[P] * 0.5F;
        hals[Y] = 0.0F;
        hals[R] = 0.0F;
        kopf[P] = 0.0F;
        kopf[Y] = la[Y];
        kopf[R] = 0.0F;
    } else if (k == RumpfKlasse::SaberDroid) {
        th[P] = th[Y] = th[R] = 0.0F;
        hals[P] = 0.0F;
        hals[Y] = la[Y];
        hals[R] = 0.0F;
        kopf[P] = la[P];
        kopf[Y] = 0.0F;
        kopf[R] = 0.0F;
    } else {
        th[P] = e.kante ? 0.0F : mische(th[P], la[P] * 0.4F);
        th[Y] = mische(th[Y], la[Y] * 0.1F);
        th[R] = mische(th[R], la[R] * 0.1F);
        if (!e.kante) {
            hals[P] = la[P] * 0.2F;
            hals[Y] = la[Y] * 0.3F;
            hals[R] = la[R] * 0.3F;
            kopf[P] = la[P] * 0.4F;
            kopf[Y] = la[Y] * 0.6F;
            kopf[R] = la[R] * 0.6F;
        }
    }
    if (k == RumpfKlasse::Rancor) {
        th[Y] = th[R] = 0.0F;   // Rancor: ohne cranium und cervical
    } else {
        w.halsSetzen = true;
    }
    w.thoracicSetzen = k != RumpfKlasse::SaberDroid;
    return w;
}

void rumpfWinkelAnlegen(const GlaAnimation& anim, std::vector<BoneMatrix>& world, RumpfKlasse k, const RumpfWinkel& w) {
    const auto lege = [&](const char* name, const float* winkel) {
        if (winkel[0] == 0.0F && winkel[1] == 0.0F && winkel[2] == 0.0F) { return; }
        G2Achse oben;
        G2Achse rechts;
        G2Achse vorn;
        rumpfKnochenAchsen(k, name, oben, rechts, vorn);
        knochenNachMal(anim, world, boneIndex(anim, name), g2WinkelMatrix(winkel, oben, rechts, vorn));
    };
    if (w.lumbarSetzen) {
        lege("lower_lumbar", w.lower);
        lege("upper_lumbar", w.upper);
    }
    if (w.thoracicSetzen) { lege("thoracic", w.thoracic); }
    if (w.halsSetzen) {
        lege("cervical", w.cervical);
        lege("cranium", w.cranium);
    }
}

}  // namespace bhed
