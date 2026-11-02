// schmerz.cpp - siehe bhed/schmerz.h
#include "bhed/schmerz.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bhed {

namespace {
bool gleich(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != 0; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return i == a.size() && b[i] == 0;
}
}  // namespace

float knockbackWert(int schaden, int dflags, bool quetschen) {
    if (quetschen || (dflags & kSchadenOhneRueckstoss) != 0) { return 0.0F; }
    int k = schaden;
    constexpr int kExtraRueckstoss = 0x00000040;   // DAMAGE_EXTRA_KNOCKBACK
    if ((dflags & kExtraRueckstoss) != 0) { k *= 2; }
    return static_cast<float>(std::min(k, 200));
}

bool stossTempo(float knockback, const float richtung[3], const std::string& klasse, TodRegelwerk regel, float masse, float schwerkraft, StossErgebnis& aus) {
    if (knockback <= 0.0F) { return false; }
    if (regel != TodRegelwerk::JediOutcast &&
        (gleich(klasse, "CLASS_ATST") || gleich(klasse, "CLASS_RANCOR") || gleich(klasse, "CLASS_SAND_CREATURE") || gleich(klasse, "CLASS_WAMPA"))) {
        return false;   // "much too large to *ever* throw"
    }
    constexpr float kGKnockback = 1000.0F;   // g_knockback
    float dir[3] = {richtung[0], richtung[1], richtung[2]};
    if (dir[2] <= 0.0F) { dir[2] += (0.0F - dir[2]) * 1.2F; }
    const float kb = std::min(knockback * 2.0F, 120.0F);
    const float m = masse > 0.0F ? masse : 200.0F;
    if (schwerkraft > 0.0F) {
        for (int k = 0; k < 3; ++k) { aus.tempo[k] = dir[k] * (kGKnockback * kb / m * 0.8F); }
        aus.tempo[2] = dir[2] * (kGKnockback * kb) / (m * 1.5F) + 20.0F;
    } else {
        for (int k = 0; k < 3; ++k) { aus.tempo[k] = dir[k] * (kGKnockback * kb / m); }
    }
    aus.pmZeit = std::clamp(static_cast<int>(kb) * 2, 50, 200);
    return true;
}

std::string schmerzAnimNachOrt(int ort, EngineZufall& zufall) {
    switch (ort) {
        case kOrtFussR: return "BOTH_PAIN12";
        case kOrtBeinR: return zufall.irand(0, 1) == 0 ? "BOTH_PAIN11" : "BOTH_PAIN13";
        case kOrtBeinL: return "BOTH_PAIN14";
        case kOrtRueckenR: return "BOTH_PAIN7";
        case kOrtRueckenL: return "BOTH_PAIN" + std::to_string(zufall.irand(15, 16));
        case kOrtRuecken: return zufall.irand(0, 1) == 0 ? "BOTH_PAIN1" : "BOTH_PAIN5";
        case kOrtBrustR: return "BOTH_PAIN3";
        case kOrtBrustL: return "BOTH_PAIN2";
        case kOrtHuefte:
        case kOrtBrust:
            if (zufall.irand(0, 3) == 0) { return "BOTH_PAIN6"; }
            if (zufall.irand(0, 2) == 0) { return "BOTH_PAIN8"; }
            return zufall.irand(0, 1) == 0 ? "BOTH_PAIN17" : "BOTH_PAIN19";
        case kOrtArmR:
        case kOrtHandR: return "BOTH_PAIN9";
        case kOrtArmL:
        case kOrtHandL: return "BOTH_PAIN10";
        case kOrtKopf: return "BOTH_PAIN4";
        default: return {};
    }
}

void leichenNeigung(const NavVec& normale, float gier, float& nick, float& roll) {
    constexpr float kGrad = 3.14159265358979F / 180.0F;
    // vectoangles
    float gierN = 0.0F;
    float nickN = 0.0F;
    if (normale[0] == 0.0F && normale[1] == 0.0F) {
        nickN = normale[2] > 0.0F ? 90.0F : 270.0F;
    } else {
        gierN = normale[0] != 0.0F ? std::atan2(normale[1], normale[0]) / kGrad : (normale[1] > 0.0F ? 90.0F : 270.0F);
        if (gierN < 0.0F) { gierN += 360.0F; }
        nickN = std::atan2(normale[2], std::sqrt(normale[0] * normale[0] + normale[1] * normale[1])) / kGrad;
        if (nickN < 0.0F) { nickN += 360.0F; }
    }
    const float pitch = -nickN + 90.0F;
    const float nvf[2] = {std::cos(gierN * kGrad), std::sin(gierN * kGrad)};
    const float ovf[2] = {std::cos(gier * kGrad), std::sin(gier * kGrad)};
    const float ovr[2] = {std::sin(gier * kGrad), -std::cos(gier * kGrad)};
    const float mod = nvf[0] * ovr[0] + nvf[1] * ovr[1] < 0.0F ? -1.0F : 1.0F;
    const float dot = nvf[0] * ovf[0] + nvf[1] * ovf[1];
    nick = dot * pitch;
    roll = (1.0F - std::fabs(dot)) * pitch * mod;
}

float leichenSenkung(float nick) { return 12.0F * std::fabs(nick) / 180.0F; }

float npcSchmerzChance(const std::string& klasse, bool movieDuels) {
    float chance = 1.0F;   // NPC_GetPainChance ohne Feind; CLASS_PROTOCOL ebenso
    if (gleich(klasse, "CLASS_DESANN") || (movieDuels && (gleich(klasse, "CLASS_SITHLORD") || gleich(klasse, "CLASS_VADER")))) { chance *= 0.5F; }
    return chance;
}

std::string schmerzKlang(int wert) {
    if (wert < 25) { return "*pain100.wav"; }
    if (wert < 50) { return "*pain75.wav"; }
    if (wert < 75) { return "*pain50.wav"; }
    return "*pain25.wav";
}

}  // namespace bhed
