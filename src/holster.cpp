// holster.cpp - siehe bhed/holster.h
#include "bhed/holster.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <sstream>

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
                      HolsterLage& aus, bool huefteLinksBelegt, bool huefteRechtsBelegt) {
    // SaberCanNotHolster
    if (saberTyp0 == "SABER_DAGGER" || saberTyp0 == "SABER_STAFF_SFX" || saberTyp0 == "SABER_DUAL_GRIE" || klasse == "CLASS_JANGO" || klasse == "CLASS_JANGODUAL" ||
        nennt(npcTyp, {"md_grievous4", "md_gungan_warrior"})) {
        return false;
    }
    const std::string platz0 = platz0Roh.empty() ? holsterPlatzVorgabe(saberTyp0) : platz0Roh;
    const std::string& platz = platzRoh.empty() ? platz0 : platzRoh;
    if (platz0 == "HOLSTER_NONE") { return false; }
    float w[3] = {0.0F, 0.0F, 0.0F};
    // Der Versatz: G2_Generate_Matrix schreibt ihn in die vierte Spalte des Overrides (G2_bones.cpp:316).
    float o[3] = {0.0F, 0.0F, 0.0F};
    const bool staff = staffTyp(saberTyp0) && !nennt(npcTyp, {"md_galen","md_galen_jt","md_galencjr","md_starkiller","md_sithstalker","md_stk_lord","md_stk_tat"});   // SaberStaffWeapon: nicht GalenHolster
    const bool hueft = platz == "HOLSTER_RHIP" || platz == "HOLSTER_RHIPF" || platz == "HOLSTER_RHIPB" || platz == "HOLSTER_LHIP" || platz == "HOLSTER_LHIPF" || platz == "HOLSTER_LHIPB";
    const bool brust = platz == "HOLSTER_LCHESTL" || platz == "HOLSTER_RCHESTL" || platz == "HOLSTER_LCHESTU" || platz == "HOLSTER_RCHESTU";
    if (hueft || brust) {
        w[0] = 180.0F;
        w[2] = 180.0F;
        if (brust) {
            o[1] = -2.0F;
            o[2] = -5.0F;
        } else {
            o[1] = -1.0F;
            // "facing down": ein Doppelschwert 10 hoeher, ausser Maul an der hinteren und Cal an jeder Huefte
            const bool rechts = platz == "HOLSTER_RHIP" || platz == "HOLSTER_RHIPF" || platz == "HOLSTER_RHIPB";
            const bool tief = !staff || (rechts && ((maul(npcTyp) && platz == "HOLSTER_RHIPB") || cal(npcTyp)));
            o[2] = tief ? -10.0F : 10.0F;
        }
    } else if (platz == "HOLSTER_BACKU" || platz == "HOLSTER_BACKL") {
        w[2] = 90.0F;
        w[1] = 90.0F;
        o[1] = -2.0F;
        o[2] = platz == "HOLSTER_BACKU" ? 4.0F : -10.0F;
    } else if (platz == "HOLSTER_BACK") {
        o[1] = -2.0F;
        o[2] = nennt(npcTyp, {"md_royalguard","md_7thsister","md_8thbrother"}) ? -12.0F : -4.0F;   // RoyalguardHolster
        if (nennt(npcTyp, {"md_2ndsister"})) {
            w[2] = 90.0F;
            o[2] = -4.0F;
        } else if (nennt(npcTyp, {"md_5thbrother", "md_8thbrother", "md_7thsister", "md_inquisitor"})) {
            w[2] = 90.0F;
            w[1] = 90.0F;
            o[2] = nennt(npcTyp, {"md_inquisitor"}) ? 4.0F : nennt(npcTyp, {"md_5thbrother"}) ? 0.0F : -4.0F;
        } else if (nennt(npcTyp, {"gamorrean"})) {
            w[2] = 90.0F;
            w[1] = 20.0F;
            o[1] = -8.0F;
            o[2] = -12.0F;
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
    const int seite = (bo == "*hip_r" || bo == "*hip_br") ? 1 : (bo == "*hip_l" || bo == "*hip_bl") ? 2 : 0;
    if (seite != 0) {
        const float g2[3] = {w[0], w[2], w[1]};
        // "out of the leg: 2 away from the body, 2 forward, 6 down", wenn eine Waffe dort haengt
        const bool belegt = seite == 2 ? huefteLinksBelegt : huefteRechtsBelegt;
        const float seitlich[3] = {seite == 1 ? -2.0F : 2.0F, -2.0F, -6.0F};
        const float tief[3] = {0.0F, 0.0F, -5.0F};
        aus.drehung = matrixAusAchsen(g2, belegt ? seitlich : tief);
        if (belegt) { aus.bolzen = seite == 2 ? "*hip_fl" : "*hip_fr"; }
    } else {
        aus.drehung = g2WinkelMatrix(w, G2Achse::PosX, G2Achse::NegY, G2Achse::NegZ);
        for (int r = 0; r < 3; ++r) { aus.drehung.m[r][3] = o[r]; }
    }
    return true;
}

namespace {

// Die Reihenfolge von hlsTypeTable (cg_holster.cpp): die Nummern von cg_holsterdebug.
constexpr const char* kHolsterNamen[kHolsterTypen] = {"HLR_NONE", "HLR_SINGLESABER_1", "HLR_SINGLESABER_2", "HLR_STAFFSABER", "HLR_PISTOL_L", "HLR_PISTOL_R",
                                                      "HLR_BLASTER_L", "HLR_BLASTER_R", "HLR_BRYARPISTOL_L", "HLR_BRYARPISTOL_R", "HLR_BOWCASTER", "HLR_ROCKET_LAUNCHER",
                                                      "HLR_DEMP2", "HLR_CONCUSSION", "HLR_REPEATER", "HLR_FLECHETTE", "HLR_DISRUPTOR", "HLR_Z6"};
constexpr const char* kKnochenNamen[5] = {"HOLSTER_NONE", "HOLSTER_UPPERBACK", "HOLSTER_LOWERBACK", "HOLSTER_LEFTHIP", "HOLSTER_RIGHTHIP"};
constexpr const char* kKnochenBolzen[5] = {nullptr, "*chestg", "lower_lumbar", "lfemurYZ", "rfemurYZ"};

struct GunEintrag {
    const char* waffe;
    bool ruecken;
    int rechts;   // der Typ auf der rechten Huefte oder dem Ruecken
    int links;
};
// hlsGuns: "the ones that come first get a place first"
constexpr GunEintrag kGuns[] = {
    {"WP_BLASTER", false, 7, 6}, {"WP_BATTLEDROID", false, 7, 6}, {"WP_THEFIRSTORDER", false, 7, 6}, {"WP_CLONECARBINE", false, 7, 6}, {"WP_REBELBLASTER", false, 7, 6},
    {"WP_BLASTER_PISTOL", false, 5, 4}, {"WP_REY", false, 5, 4}, {"WP_JANGO", false, 5, 4}, {"WP_BOBA", false, 5, 4}, {"WP_CLONEPISTOL", false, 5, 4},
    {"WP_BRYAR_PISTOL", false, 9, 8},
    {"WP_ROCKET_LAUNCHER", true, 11, 11}, {"WP_CONCUSSION", true, 13, 13}, {"WP_REPEATER", true, 14, 14}, {"WP_FLECHETTE", true, 15, 15},
    {"WP_DISRUPTOR", true, 16, 16}, {"WP_BOWCASTER", true, 10, 10}, {"WP_DEMP2", true, 12, 12}, {"WP_Z6_ROTARY_CANNON", true, 17, 17},
    {"WP_CLONECOMMANDO", true, 14, 14}, {"WP_CLONERIFLE", true, 16, 16}, {"WP_REBELRIFLE", true, 16, 16},
};

const GunEintrag* gunVon(const std::string& w) {
    for (const GunEintrag& g : kGuns) {
        if (w == g.waffe) { return &g; }
    }
    return nullptr;
}

bool leseVektor(const std::string& text, float aus[3]) {
    return std::sscanf(text.c_str(), "%f , %f , %f", &aus[0], &aus[1], &aus[2]) == 3 || std::sscanf(text.c_str(), "%f %f %f", &aus[0], &aus[1], &aus[2]) == 3;
}

// COM_ParseExt fuer diese Dateien: Worte, "..." ein Wort, // bis zum Zeilenende.
std::vector<std::string> worte(const std::string& text) {
    std::vector<std::string> aus;
    std::size_t i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            ++i;
        } else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n') { ++i; }
        } else if (c == '"') {
            const std::size_t e = text.find('"', i + 1);
            aus.push_back(text.substr(i + 1, e == std::string::npos ? std::string::npos : e - i - 1));
            i = e == std::string::npos ? text.size() : e + 1;
        } else {
            std::size_t e = i;
            while (e < text.size() && std::isspace(static_cast<unsigned char>(text[e])) == 0) { ++e; }
            aus.push_back(text.substr(i, e - i));
            i = e;
        }
    }
    return aus;
}

// Hls_RotateAxis
void dreheAchsen(float a[3][3], int art, float grad) {
    if (grad == 0.0F) { return; }
    const float g = grad * 3.14159265358979F / 180.0F;
    const float sn = std::sin(g);
    const float cs = std::cos(g);
    float d[3][3];
    if (art == 2) {   // ROLL
        const float t[3][3] = {{1, 0, 0}, {0, cs, -sn}, {0, sn, cs}};
        for (int i = 0; i < 9; ++i) { d[i / 3][i % 3] = t[i / 3][i % 3]; }
    } else if (art == 0) {   // PITCH
        const float t[3][3] = {{cs, 0, sn}, {0, 1, 0}, {-sn, 0, cs}};
        for (int i = 0; i < 9; ++i) { d[i / 3][i % 3] = t[i / 3][i % 3]; }
    } else {   // YAW
        const float t[3][3] = {{cs, -sn, 0}, {sn, cs, 0}, {0, 0, 1}};
        for (int i = 0; i < 9; ++i) { d[i / 3][i % 3] = t[i / 3][i % 3]; }
    }
    float neu[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { neu[i][j] = d[i][0] * a[0][j] + d[i][1] * a[1][j] + d[i][2] * a[2][j]; }
    }
    for (int i = 0; i < 9; ++i) { a[i / 3][i % 3] = neu[i / 3][i % 3]; }
}

}  // namespace

void leseHolsterCfg(const std::string& text, HolsterCfg& aus) {
    const std::vector<std::string> w = worte(text);
    const auto gleich = [](const std::string& x, const char* y) { return klein(x) == klein(y); };
    for (std::size_t i = 0; i < w.size(); ++i) {
        if (w[i] != "{") { continue; }
        int typ = -1;
        int knochen = -1;
        float pos[3] = {};
        float ang[3] = {};
        bool hatPos = false;
        bool hatAng = false;
        for (++i; i < w.size() && w[i] != "}"; i += 2) {
            if (i + 1 >= w.size()) { break; }
            const std::string& v = w[i + 1];
            if (gleich(w[i], "holsterType")) {
                for (int k = 0; k < kHolsterTypen; ++k) {
                    if (gleich(v, kHolsterNamen[k])) { typ = k; }
                }
            } else if (gleich(w[i], "boneIndex")) {
                knochen = gleich(v, "disabled") ? 0 : -1;
                for (int k = 0; k < 5; ++k) {
                    if (gleich(v, kKnochenNamen[k])) { knochen = k; }
                }
            } else if (gleich(w[i], "posOffset")) {
                hatPos = leseVektor(v, pos);
            } else if (gleich(w[i], "angOffset")) {
                hatAng = leseVektor(v, ang);
            }
        }
        if (typ <= 0) { continue; }   // "no or an unknown holsterType: skip the block"
        WaffenHolsterEintrag& h = aus.typ[static_cast<std::size_t>(typ)];
        if (knochen >= 0) { h.knochen = knochen; }
        for (int k = 0; k < 3; ++k) {
            if (hatPos) { h.pos[k] = pos[k]; }
            if (hatAng) { h.ang[k] = ang[k]; }
        }
    }
}

void waffenPlaetzeAendern(WaffenPlaetze& plaetze, const HolsterCfg& cfg, const std::vector<std::string>& hat, const std::string& inHand, bool rueckenBelegt) {
    const auto hatWaffe = [&hat](const std::string& w) {
        for (const std::string& x : hat) {
            if (x == w) { return true; }
        }
        return false;
    };
    const auto frei = [&cfg](int typ) { return cfg.typ[static_cast<std::size_t>(typ)].knochen > 0; };
    int belegt = 0;
    for (std::size_t p = 0; p < plaetze.size(); ++p) {
        if (plaetze[p].empty()) { continue; }
        const GunEintrag* g = gunVon(plaetze[p]);
        if (!hatWaffe(plaetze[p]) || g == nullptr || !frei(p == 1 ? g->links : g->rechts)) {
            plaetze[p].clear();
        } else {
            ++belegt;
        }
    }
    for (const GunEintrag& g : kGuns) {
        if (belegt >= 2) { break; }   // MAX_HOLSTERED
        const std::string w = g.waffe;
        if (w == inHand || !hatWaffe(w) || plaetze[0] == w || plaetze[1] == w || plaetze[2] == w) { continue; }
        if (g.ruecken) {
            if (plaetze[2].empty() && !rueckenBelegt && frei(g.rechts)) {
                plaetze[2] = w;
                ++belegt;
            }
        } else if (plaetze[0].empty() && frei(g.rechts)) {
            plaetze[0] = w;
            ++belegt;
        } else if (plaetze[1].empty() && frei(g.links)) {
            plaetze[1] = w;
            ++belegt;
        }
    }
}

std::vector<WaffeImHolster> waffenImHolster(const WaffenPlaetze& plaetze, const HolsterCfg& cfg, const std::string& inHand, bool rueckenBelegt) {
    std::vector<WaffeImHolster> aus;
    for (std::size_t p = 0; p < plaetze.size(); ++p) {
        if (plaetze[p].empty() || plaetze[p] == inHand || (p == 2 && rueckenBelegt)) { continue; }
        const GunEintrag* g = gunVon(plaetze[p]);
        if (g == nullptr) { continue; }
        const WaffenHolsterEintrag& h = cfg.typ[static_cast<std::size_t>(p == 1 ? g->links : g->rechts)];
        if (h.knochen <= 0 || h.knochen >= 5) { continue; }
        // Hls_Render: im Raum des Bolzens axis[0] = NEGATIVE_Y, axis[1] = NEGATIVE_Z, axis[2] = POSITIVE_X.
        const float achse[3][3] = {{0.0F, -1.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F, 0.0F}};
        float ent[3][3];
        for (int k = 0; k < 3; ++k) {
            ent[0][k] = achse[1][k];
            ent[1][k] = -achse[0][k];   // "reversed, right-hand rule"
            ent[2][k] = achse[2][k];
        }
        dreheAchsen(ent, 0, h.ang[0]);
        dreheAchsen(ent, 1, h.ang[1]);
        dreheAchsen(ent, 2, h.ang[2]);
        WaffeImHolster w;
        w.waffe = plaetze[p];
        w.bolzen = kKnochenBolzen[h.knochen];
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) { w.lage.m[r][c] = ent[c][r]; }
            w.lage.m[r][3] = h.pos[0] * achse[0][r] + h.pos[1] * achse[1][r] + h.pos[2] * achse[2][r];
        }
        aus.push_back(w);
    }
    return aus;
}

}  // namespace bhed
