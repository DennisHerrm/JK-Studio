// holster.cpp - siehe bhed/holster.h
#include "bhed/holster.h"

#include <cctype>
#include <cmath>
#include <initializer_list>

namespace bhed {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool nennt(const std::string& name, std::initializer_list<const char*> liste) {
    const std::string k = klein(name);
    for (const char* n : liste) {
        if (k == n) { return true; }
    }
    return false;
}

// DarthMaulHolster, CalKestisHolster (wp_saber.cpp MD): die NPC_type-Listen.
bool maul(const std::string& n) { return nennt(n, {"md_maul", "md_maul_robed", "md_maul_hooded", "md_maul_rebels", "md_maul_rebels2", "md_maul_rebels3", "md_maul_rebels4", "md_maul_rebels5", "md_maul_rebels6", "md_maul_wots", "md_maul_tcw_staff", "md_maul_solo", "md_maul_solo_hood", "md_mau_luke", "md_mau_luke_robed", "md_mau_luke_hooded"}); }
bool cal(const std::string& n) { return nennt(n, {"cal_kestis", "cal_kestis_cape", "cal_inquisitor", "cal_kestis_staff"}); }

bool staffTyp(const std::string& t) {
    return t == "SABER_STAFF" || t == "SABER_STAFF_SFX" || t == "SABER_STAFF_UNSTABLE" || t == "SABER_STAFF_THIN";
}

// AnglesToAxis der Engine, die Achsen als Spalten, dazu die Verschiebung.
BoneMatrix matrixAusAchsen(const float winkel[3], const float versatz[3]) {
    constexpr float kRad = 3.14159265358979F / 180.0F;
    const float sy = std::sin(winkel[1] * kRad);
    const float cy = std::cos(winkel[1] * kRad);
    const float sp = std::sin(winkel[0] * kRad);
    const float cp = std::cos(winkel[0] * kRad);
    const float sr = std::sin(winkel[2] * kRad);
    const float cr = std::cos(winkel[2] * kRad);
    const float vorn[3] = {cp * cy, cp * sy, -sp};
    const float rechts[3] = {-sr * sp * cy + cr * sy, -sr * sp * sy - cr * cy, -sr * cp};
    const float hoch[3] = {cr * sp * cy + sr * sy, cr * sp * sy - sr * cy, cr * cp};
    BoneMatrix m{};
    for (int i = 0; i < 3; ++i) {
        m.m[i][0] = vorn[i];
        m.m[i][1] = -rechts[i];   // AnglesToAxis: axis[1] zeigt nach links
        m.m[i][2] = hoch[i];
        m.m[i][3] = versatz[i];
    }
    return m;
}

}  // namespace

std::string holsterPlatzVorgabe(const std::string& saberTyp) {
    if (saberTyp == "SABER_SITH_SWORD") { return "HOLSTER_LHIP"; }
    return staffTyp(saberTyp) ? "HOLSTER_RHIPB" : "HOLSTER_LHIPF";
}

bool saberHolsterLage(const std::string& npcTyp, const std::string& klasse, const std::string& saberTyp0, const std::string& platz0Roh, const std::string& platzRoh, int nr,
                      HolsterLage& aus) {
    // SaberCanNotHolster
    if (saberTyp0 == "SABER_DAGGER" || saberTyp0 == "SABER_STAFF_SFX" || saberTyp0 == "SABER_DUAL_GRIE" || klasse == "CLASS_JANGO" || klasse == "CLASS_JANGODUAL" ||
        nennt(npcTyp, {"md_grievous4", "md_gungan_warrior"})) {
        return false;
    }
    const std::string platz0 = platz0Roh.empty() ? holsterPlatzVorgabe(saberTyp0) : platz0Roh;
    const std::string& platz = platzRoh.empty() ? platz0 : platzRoh;
    if (platz0 == "HOLSTER_NONE") { return false; }
    float w[3] = {0.0F, 0.0F, 0.0F};
    const bool hueft = platz == "HOLSTER_RHIP" || platz == "HOLSTER_RHIPF" || platz == "HOLSTER_RHIPB" || platz == "HOLSTER_LHIP" || platz == "HOLSTER_LHIPF" || platz == "HOLSTER_LHIPB";
    const bool brust = platz == "HOLSTER_LCHESTL" || platz == "HOLSTER_RCHESTL" || platz == "HOLSTER_LCHESTU" || platz == "HOLSTER_RCHESTU";
    if (hueft || brust) {
        w[0] = 180.0F;
        w[2] = 180.0F;
    } else if (platz == "HOLSTER_BACKU" || platz == "HOLSTER_BACKL") {
        w[2] = 90.0F;
        w[1] = 90.0F;
    } else if (platz == "HOLSTER_BACK") {
        if (nennt(npcTyp, {"md_2ndsister"})) {
            w[2] = 90.0F;
        } else if (nennt(npcTyp, {"md_5thbrother", "md_8thbrother", "md_7thsister", "md_inquisitor"})) {
            w[2] = 90.0F;
            w[1] = 90.0F;
        } else if (nennt(npcTyp, {"gamorrean"})) {
            w[2] = 90.0F;
            w[1] = 20.0F;
        }
        w[0] = 22.5F;
    } else {
        return false;   // HOLSTER_NONE
    }
    const char* b = nullptr;
    const bool hintenPlatz = platz == "HOLSTER_BACKU" || platz == "HOLSTER_BACKL" || platz == "HOLSTER_BACK";
    if (nr == 0) {
        if (platz == "HOLSTER_LHIP") { b = "*hip_l"; }
        else if (platz == "HOLSTER_LHIPF") { b = cal(npcTyp) ? "*hip_r" : "*hip_fl"; }
        else if (platz == "HOLSTER_LHIPB") { b = "*hip_bl"; }
        else if (platz == "HOLSTER_RHIP") { b = "*hip_r"; }
        else if (platz == "HOLSTER_RHIPF") { b = "*hip_fr"; }
        else if (platz == "HOLSTER_RHIPB") { b = (maul(npcTyp) || cal(npcTyp)) ? "*hip_r" : "*hip_br"; }
    } else {
        if (platz == "HOLSTER_RHIP" || platz == "HOLSTER_LHIP") {
            b = platz0 == "HOLSTER_LHIP" ? "*hip_r" : "*hip_l";
        } else if (platz == "HOLSTER_RHIPF" || platz == "HOLSTER_LHIPF") {
            b = platz0 == "HOLSTER_LHIPF" ? "*hip_l" : "*hip_fl";
        } else if (platz == "HOLSTER_RHIPB" || platz == "HOLSTER_LHIPB") {
            if (platz0 == "HOLSTER_LHIPB") {
                b = maul(npcTyp) ? "*hip_r" : "*hip_br";
            } else {
                b = cal(npcTyp) ? "*hip_r" : "*hip_bl";
            }
        }
        if (hintenPlatz && (platz0 == "HOLSTER_BACKU" || platz0 == "HOLSTER_BACKL" || platz0 == "HOLSTER_BACK")) { return false; }
    }
    if (platz == "HOLSTER_LCHESTL") { b = "*lchest_l"; }
    else if (platz == "HOLSTER_RCHESTL") { b = "*lchest_r"; }
    else if (platz == "HOLSTER_LCHESTU") { b = "*uchest_l"; }
    else if (platz == "HOLSTER_RCHESTU") { b = "*uchest_r"; }
    else if (hintenPlatz) { b = nennt(npcTyp, {"md_2ndsister"}) ? "*hip_l" : "*back"; }
    if (b == nullptr) { return false; }
    aus.bolzen = b;
    // WP_SaberHolsterAvoidGuns: an der Seite der Huefte 5 tiefer, als Matrix ("the same rotation
    // G2API_SetBoneAnglesOffset makes ... plus a translation").
    const std::string bo = b;
    const bool seite = bo == "*hip_r" || bo == "*hip_br" || bo == "*hip_l" || bo == "*hip_bl";
    if (hueft || brust) {
        // Die Winkel 180/0/180 ergeben nach der Quelle eine Drehung um X. Im Spielfoto (Movie Duels,
        // HOLSTER_LHIPF an *hip_fl) haengt der Griff aber nach unten: gemessen ist die Drehung um die
        // Laengsachse des Griffs. Die Basispose von "ModView internal default" war nicht lesbar.
        aus.drehung = BoneMatrix{};
        aus.drehung.m[0][0] = -1.0F;
        aus.drehung.m[1][1] = -1.0F;
        aus.drehung.m[2][2] = 1.0F;
        if (seite) { aus.drehung.m[2][3] = -5.0F; }
    } else if (seite) {
        const float g2[3] = {w[0], w[2], w[1]};
        const float tief[3] = {0.0F, 0.0F, -5.0F};
        aus.drehung = matrixAusAchsen(g2, tief);
    } else {
        aus.drehung = g2WinkelMatrix(w, G2Achse::PosX, G2Achse::NegY, G2Achse::NegZ);
    }
    return true;
}

}  // namespace bhed
