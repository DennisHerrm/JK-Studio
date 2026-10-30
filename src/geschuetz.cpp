// geschuetz.cpp - Pruefung 4b: Geschuetze mit Feind und ihre Geschosse
// (bhed/geschuetz.h). Quellen: OpenJK code/game/g_turret.cpp, g_missile.cpp,
// g_combat.cpp, g_weapon.cpp, wp_emplaced_gun.cpp, q_math.c; Movie Duels
// code/game/g_turret.cpp (misc_turretG2, turret_target_point/-team); Jedi
// Outcast codeJK2/game/g_turret.cpp.
#include "bhed/geschuetz.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace bhed {

namespace {

constexpr double kFrametime = 100.0;   // FRAMETIME (g_local.h)
constexpr double kLevelBeimSpawn = 1000.0;   // level.time beim Spawnen (sv.time 1000, sv_init.cpp:288 OJ)
constexpr float kStartDis = 15.0F;     // START_DIS (g_turret.cpp:160 OJ)
constexpr int kTurbo = 4;              // SPF_TURRETG2_TURBO
constexpr int kLead = 8;               // SPF_TURRETG2_LEAD_ENEMY
constexpr int kKannWieder = 16;        // SPF_TURRETG2_CANRESPAWN (MD)
constexpr int kMpRegeln = 32;          // SPF_TURRETG2_MP_RULES (MD)
constexpr int kPasSchaden = 2;         // PAS_DAMAGE (g_turret.cpp:1392 OJ)

bool gleichOhneFall(const std::string& a, const char* b) {
    const std::size_t n = std::strlen(b);
    if (a.size() != n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}
// Q_stricmpn(p, s, n) == 0
bool beginntMit(const std::string& s, const char* p) {
    const std::size_t n = std::strlen(p);
    if (s.size() < n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(p[i]))) { return false; }
    }
    return true;
}
bool klasseIst(const std::string& k, const char* name) {
    // "CLASS_ATST" wie "atst" (die .npc schreibt die Klasse ohne Vorsatz).
    if (gleichOhneFall(k, name)) { return true; }
    return std::strncmp(name, "CLASS_", 6) == 0 && gleichOhneFall(k, name + 6);
}

float abstandQuadrat(const float a[3], const float b[3]) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}

// EvaluateTrajectory (bg_misc.cpp OJ): TR_STATIONARY / TR_LINEAR.
void bahnBei(bool linear, const float basis[3], const float delta[3], double zeit, double bei, float out[3]) {
    if (!linear) {
        for (int k = 0; k < 3; ++k) { out[k] = basis[k]; }
        return;
    }
    const auto dt = static_cast<float>((bei - zeit) * 0.001);
    for (int k = 0; k < 3; ++k) { out[k] = basis[k] + dt * delta[k]; }
}

// vectoangles (q_math.c:616-654 OJ)
void vektorWinkel(const float v[3], float w[3]) { engineVectoangles(v, w); }

float vektorNormieren(float v[3]) {
    const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l != 0.0F) {
        const float il = 1.0F / l;
        for (int k = 0; k < 3; ++k) { v[k] *= il; }
    }
    return l;
}

}  // namespace

// --- Teams ---------------------------------------------------------------------

int teamAusName(const std::string& name, TodRegelwerk r) {
    if (r == TodRegelwerk::JediOutcast) {
        const char* namen[] = {"player", "enemy", "neutral"};
        for (int n = 0; n < 3; ++n) {
            if (gleichOhneFall(name, namen[n])) { return n + 1; }
        }
        return kTeamFrei;
    }
    struct Eintrag {
        const char* name;
        int id;
    };
    static const Eintrag tabelle[] = {{"free", 0},       {"TEAM_FREE", 0},       {"player", 1},     {"TEAM_PLAYER", 1},
                                      {"enemy", 2},      {"TEAM_ENEMY", 2},      {"neutral", 3},    {"TEAM_NEUTRAL", 3},
                                      {"solo", 4},       {"TEAM_SOLO", 4},       {"projection", 5}, {"TEAM_PROJECTION", 5}};
    // TEAM_SOLO und TEAM_PROJECTION gibt es nur in Movie Duels (NPC_stats.cpp MD).
    const std::size_t n = r == TodRegelwerk::MovieDuels ? 12U : 8U;
    for (std::size_t i = 0; i < n; ++i) {
        if (gleichOhneFall(name, tabelle[i].name)) { return tabelle[i].id; }
    }
    return -1;
}

int teamAusSkript(const std::string& name) {
    const char* namen[] = {"TEAM_FREE", "TEAM_PLAYER", "TEAM_ENEMY", "TEAM_NEUTRAL"};
    for (int n = 0; n < 4; ++n) {
        if (gleichOhneFall(name, namen[n])) { return n; }
    }
    return -1;
}

// --- Trefferorte -----------------------------------------------------------------

float trefferOrtFaktor(int ort, TodRegelwerk r) {
    static const float faktor[kOrtAnzahl] = {1.0F,  0.25F, 0.25F, 0.75F, 0.75F, 1.0F, 1.0F, 1.0F,
                                             1.0F,  1.0F,  1.0F,  1.0F,  0.5F,  0.5F, 0.25F, 0.25F,
                                             2.0F,  1.0F,  1.0F,  1.0F,  1.0F,  1.0F, 1.0F};
    if (ort < 0 || ort >= kOrtAnzahl) { return 1.0F; }
    if (ort == kOrtKopf && r == TodRegelwerk::MovieDuels) { return 1.0F; }
    return faktor[ort];
}

int trefferOrtAusFlaeche(const TrefferOrtLage& l, const std::string& flaeche, const float punkt[3], TodRegelwerk r) {
    if (flaeche.empty()) { return kOrtKeiner; }
    const std::string& k = l.klasse;
    // "we don't care about per-surface hit-locations or dismemberment for these guys"
    static const char* const ohneOJ[] = {"CLASS_R2D2", "CLASS_R5D2", "CLASS_GONK", "CLASS_MOUSE",
                                         "CLASS_SENTRY", "CLASS_INTERROGATOR", "CLASS_PROBE"};
    for (const char* c : ohneOJ) {
        if (klasseIst(k, c)) { return kOrtKeiner; }
    }
    if (r != TodRegelwerk::OpenJK && klasseIst(k, "CLASS_SEEKER")) { return kOrtKeiner; }
    if (r == TodRegelwerk::MovieDuels) {
        for (const char* c : {"CLASS_SBD", "CLASS_DROIDEKA", "CLASS_PROJECTION"}) {
            if (klasseIst(k, c)) { return kOrtKeiner; }
        }
        for (const char* t : {"md_grie_egg", "md_grie2_egg", "md_grie3_egg", "md_grie4_egg", "md_grievous", "md_grievous4",
                              "md_grievous_robed", "md_dro_am"}) {
            if (gleichOhneFall(l.npcType, t)) { return kOrtKeiner; }
        }
    }
    const auto ist = [&flaeche](const char* n) { return gleichOhneFall(flaeche, n); };
    if (klasseIst(k, "CLASS_ATST")) {
        if (ist("head_light_blaster_cann")) { return kOrtArmL; }
        if (ist("head_concussion_charger")) { return kOrtArmR; }
        if (ist("torso_light_blaster_cann")) { return kOrtArmL; }
        if (ist("torso_concussion_charger")) { return kOrtArmR; }
        return kOrtKeiner;
    }
    if (klasseIst(k, "CLASS_MARK1")) {
        if (ist("l_arm")) { return kOrtArmL; }
        if (ist("r_arm")) { return kOrtArmR; }
        if (ist("torso_front")) { return kOrtBrust; }
        const char* rohre[] = {"torso_tube1", "torso_tube2", "torso_tube3", "torso_tube4", "torso_tube5", "torso_tube6"};
        for (int i = 0; i < 6; ++i) {
            if (ist(rohre[i])) { return kOrtAllg1 + i; }
        }
        return kOrtKeiner;
    }
    if (klasseIst(k, "CLASS_MARK2")) {
        const char* dosen[] = {"torso_canister1", "torso_canister2", "torso_canister3"};
        for (int i = 0; i < 3; ++i) {
            if (ist(dosen[i])) { return kOrtAllg1 + i; }
        }
        return kOrtKeiner;
    }
    if (klasseIst(k, "CLASS_GALAKMECH")) {
        if (ist("torso_antenna") || ist("torso_antenna_base")) { return kOrtAllg1; }
        if (ist("torso_shield")) { return kOrtAllg2; }
        return kOrtBrust;
    }
    const auto nah = [punkt](const float ort[3], float grenze) { return abstandQuadrat(punkt, ort) < grenze; };
    if (beginntMit(flaeche, "hips")) {
        if (l.knieL && nah(l.knieLOrt, 100.0F)) { return kOrtBeinL; }
        if (l.knieR && nah(l.knieROrt, 100.0F)) { return kOrtBeinR; }
        return kOrtHuefte;
    }
    if (beginntMit(flaeche, "torso")) {
        float vorn[3];
        float rechts[3];
        float oben[3];
        winkelVektoren(l.rumpfWinkel, vorn, rechts, oben);
        const float d[3] = {punkt[0] - l.rumpfOrt[0], punkt[1] - l.rumpfOrt[1], punkt[2] - l.rumpfOrt[2]};
        const float frontSide = vorn[0] * d[0] + vorn[1] * d[1] + vorn[2] * d[2];
        const float rightSide = rechts[0] * d[0] + rechts[1] * d[1] + rechts[2] * d[2];
        const float upSide = oben[0] * d[0] + oben[1] * d[1] + oben[2] * d[2];
        if (upSide < -10.0F) { return kOrtHuefte; }
        if (rightSide > 4.0F) { return kOrtArmR; }
        if (rightSide < -4.0F) { return kOrtArmL; }
        if (rightSide > 2.0F) { return frontSide > 0.0F ? kOrtBrustR : kOrtRueckenR; }
        if (rightSide < -2.0F) { return frontSide > 0.0F ? kOrtBrustL : kOrtRueckenL; }
        return frontSide > 0.0F ? kOrtBrust : kOrtRuecken;   // (upSide > -3 && MOD_SABER: Kopf - kein Schwert hier)
    }
    if (beginntMit(flaeche, "head")) { return kOrtKopf; }
    if (beginntMit(flaeche, "r_arm")) { return l.handR && nah(l.handROrt, 256.0F) ? kOrtHandR : kOrtArmR; }
    if (beginntMit(flaeche, "l_arm")) { return l.handL && nah(l.handLOrt, 256.0F) ? kOrtHandL : kOrtArmL; }
    if (beginntMit(flaeche, "r_leg")) { return l.fussR && nah(l.fussROrt, 100.0F) ? kOrtFussR : kOrtBeinR; }
    if (beginntMit(flaeche, "l_leg")) { return l.fussL && nah(l.fussLOrt, 100.0F) ? kOrtFussL : kOrtBeinL; }
    if (beginntMit(flaeche, "r_hand") || beginntMit(flaeche, "w_")) { return kOrtHandR; }
    if (beginntMit(flaeche, "l_hand")) { return kOrtHandL; }
    // "force_shield" nur mit PW_GALAK_SHIELD - Galak oben.
    return kOrtKeiner;
}

// --- Mathematik -------------------------------------------------------------------

float winkelNorm360(float a) noexcept {
    return static_cast<float>((360.0 / 65536) * (static_cast<int>(a * (65536 / 360.0)) & 65535));
}

float winkelAbzug(float a1, float a2) noexcept {
    float a = a1 - a2;
    while (a > 180.0F) { a -= 360.0F; }
    while (a < -180.0F) { a += 360.0F; }
    return a;
}

void winkelVektoren(const float w[3], float vorn[3], float rechts[3], float oben[3]) noexcept {
    // AngleVectors (q_math.c, eine C-Datei: sin/cos in double)
    const auto bogen = [](float g) { return static_cast<float>(static_cast<double>(g) * (3.14159265358979323846 * 2 / 360)); };
    const auto si = [](float x) { return static_cast<float>(std::sin(static_cast<double>(x))); };
    const auto co = [](float x) { return static_cast<float>(std::cos(static_cast<double>(x))); };
    float angle = bogen(w[1]);
    const float sy = si(angle);
    const float cy = co(angle);
    angle = bogen(w[0]);
    const float sp = si(angle);
    const float cp = co(angle);
    angle = bogen(w[2]);
    const float sr = si(angle);
    const float cr = co(angle);
    if (vorn != nullptr) {
        vorn[0] = cp * cy;
        vorn[1] = cp * sy;
        vorn[2] = -sp;
    }
    if (rechts != nullptr) {
        rechts[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
        rechts[1] = (-1 * sr * sp * sy + -1 * cr * cy);
        rechts[2] = -1 * sr * cp;
    }
    if (oben != nullptr) {
        oben[0] = (cr * sp * cy + -sr * -sy);
        oben[1] = (cr * sp * sy + -sr * cy);
        oben[2] = cr * cp;
    }
}

void g2WinkelMatrix(const float winkel[3], G2Achse oben, G2Achse links, G2Achse vorn, float m[3][3]) noexcept {
    float n[3] = {0.0F, 0.0F, 0.0F};
    switch (oben) {
        case G2Achse::NegX: n[1] = winkel[2] + 180.0F; break;
        case G2Achse::PosX: n[1] = winkel[2]; break;
        case G2Achse::NegY:
        case G2Achse::PosY: n[1] = winkel[0]; break;
        case G2Achse::NegZ: n[1] = winkel[1] + 180.0F; break;
        case G2Achse::PosZ: n[1] = winkel[1]; break;
    }
    switch (links) {
        case G2Achse::NegX: n[0] = winkel[2]; break;
        case G2Achse::PosX: n[0] = winkel[2] + 180.0F; break;
        case G2Achse::NegY: n[0] = winkel[0]; break;
        case G2Achse::PosY: n[0] = winkel[0] + 180.0F; break;
        case G2Achse::NegZ:
        case G2Achse::PosZ: n[0] = winkel[1]; break;
    }
    switch (vorn) {
        case G2Achse::NegX:
        case G2Achse::PosX: n[2] = winkel[2]; break;
        case G2Achse::NegY: n[2] = winkel[0]; break;
        case G2Achse::PosY: n[2] = winkel[0] + 180.0F; break;
        case G2Achse::NegZ: n[2] = winkel[1]; break;
        case G2Achse::PosZ: n[2] = winkel[1] + 180.0F; break;
    }
    // Create_Matrix: AnglesToAxis, axis[c] als Spalte c.
    float f[3];
    float r[3];
    float u[3];
    winkelVektoren(n, f, r, u);
    for (int z = 0; z < 3; ++z) {
        m[z][0] = f[z];
        m[z][1] = -r[z];
        m[z][2] = u[z];
    }
}

// --- Die Simulation -----------------------------------------------------------------

void GeschuetzSim::setze(std::vector<GeschuetzDaten> geschuetze, TodRegelwerk regel, double spawnMs, bool snapRunden) {
    regel_ = regel;
    snapRunden_ = snapRunden;
    spawnMs_ = spawnMs;
    laeufe_.clear();
    zustand_.clear();
    geschosse_.clear();
    benutzt_.clear();
    std::sort(geschuetze.begin(), geschuetze.end(), [](const GeschuetzDaten& a, const GeschuetzDaten& b) { return a.nummer < b.nummer; });
    const double lt0 = kLevelBeimSpawn;   // level.time beim Spawnen
    for (GeschuetzDaten& d : geschuetze) {
        std::sort(d.uses.begin(), d.uses.end());
        GeschuetzLauf l;
        Zustand z;
        z.zufall = EngineZufall(todesSaat(d.nummer, 0.0));
        for (int k = 0; k < 3; ++k) {
            l.ursprung[k] = d.ursprung[k];
            l.winkel[k] = d.winkel[k];
        }
        z.sf = d.spawnflags;
        z.health = d.health;
        z.count = d.count;
        z.radius = d.radius;
        z.wait = d.wait;
        z.random = d.random;
        z.damage = d.damage;
        z.splashDamage = d.splashDamage;
        z.splashRadius = d.splashRadius;
        if (d.art == GeschuetzArt::Wache) {
            // SP_PAS (g_turret.cpp:1784-1866 OJ)
            z.speed = d.winkel[1];
            if (z.radius == 0.0F) { z.radius = 512.0F; }
            if (z.count == 0) { z.count = 150; }
            z.aufgeklappt = false;
            z.mins[0] = z.mins[1] = -8.0F;
            z.mins[2] = 0.0F;
            z.maxs[0] = z.maxs[1] = 8.0F;
            z.maxs[2] = 18.0F;
            if ((z.sf & 1) == 0) {
                z.denkt = true;
                z.naechstes = lt0 + 1000.0;
            }
            if (z.health == 0) { z.health = 50; }
            z.maxHealth = z.health;
            z.takedamage = true;
            z.sf |= 2;   // "hack this flag on so that when it calls the turret die code, it will orient the effect up"
            z.noDamageTeam = kTeamNeutral;
            if (!d.team.empty()) { z.noDamageTeam = teamAusName(d.team, regel); }
        } else {
            int mapFlags = d.spawnflags;
            int respawn = 0;
            if (d.art == GeschuetzArt::TurmG2) {
                // SP_misc_turretG2 (MD): START_OFF 1, UPSIDE_DOWN 2, CANRESPAWN 4, TURBO 8, LEAD 16
                z.sf = (mapFlags & 3) | kMpRegeln;
                if ((mapFlags & 8) != 0) { z.sf |= kTurbo; }
                if ((mapFlags & 16) != 0) { z.sf |= kLead; }
                if ((mapFlags & 4) != 0) { z.sf |= kKannWieder; }
                respawn = d.count != 0 ? d.count : 20000;
            }
            if (regel == TodRegelwerk::JediOutcast) { z.sf &= ~kTurbo; }   // JK2 kennt keinen Turbo
            // finish_spawning_turret
            if ((z.sf & 2) != 0) {
                l.winkel[2] += 180.0F;
                l.ursprung[2] -= 22.0F;
            }
            z.noDamageTeam = kTeamFeind;
            if (!d.team.empty()) { z.noDamageTeam = teamAusName(d.team, regel); }
            z.denkt = true;
            z.naechstes = lt0 + kFrametime * 5.0;
            z.speed = 0.0F;
            z.masse = d.shotspeed;
            if ((z.sf & kTurbo) != 0) {
                if (z.random == 0.0F) { z.random = 2.0F; }
                if (z.masse == 0.0F) { z.masse = 4000.0F; }
                if (z.health == 0) { z.health = 2000; }
                if (z.radius == 0.0F) { z.radius = 32768.0F; }
                if (z.wait == 0.0F) { z.wait = 500.0F; }
                if (z.splashDamage == 0) { z.splashDamage = 200; }
                if (z.splashRadius == 0) { z.splashRadius = 500; }
                if (z.damage == 0) { z.damage = 10; }
                l.skala = 2.0F;
                for (int k = 0; k < 3; ++k) {
                    z.maxs[k] = k == 2 ? 120.0F : 128.0F;
                    z.mins[k] = -z.maxs[k];
                }
                z.nurSchwer = true;   // flags = FL_DMG_BY_HEAVY_WEAP_ONLY
                z.takedamage = false;
                z.noDamageTeam = kTeamNeutral;
                z.knochen.turboVon = 4;   // TurboLaser_SetBoneAnim(4, 5): "off"
                z.knochen.turboBis = 5;
                z.knochen.turboAb = lt0;
                l.turboAnims.emplace_back(spawnMs, std::make_pair(4, 5));
            } else {
                z.count = static_cast<int>(z.zufall.flrand(0.0F, 1.0F) * 9000.0F);
                if (z.health == 0) { z.health = 100; }
                if (z.radius == 0.0F) { z.radius = 512.0F; }
                if (z.wait == 0.0F) { z.wait = 150.0F + z.zufall.flrand(0.0F, 1.0F) * 55.0F; }
                if (z.splashDamage == 0) { z.splashDamage = 10; }
                if (z.splashRadius == 0) { z.splashRadius = 25; }
                if (z.damage == 0) { z.damage = 5; }
                if ((z.sf & 2) != 0) {
                    z.mins[0] = z.mins[1] = -10.0F;
                    z.mins[2] = 0.0F;
                    z.maxs[0] = z.maxs[1] = 10.0F;
                    z.maxs[2] = 30.0F;
                } else {
                    z.mins[0] = z.mins[1] = -10.0F;
                    z.mins[2] = -30.0F;
                    z.maxs[0] = z.maxs[1] = 10.0F;
                    z.maxs[2] = 0.0F;
                }
                z.takedamage = true;
            }
            z.maxHealth = z.health;
            if (d.art == GeschuetzArt::TurmG2) {
                // SP_misc_turretG2 nach finish_spawning_turret
                z.respawnMs = respawn;
                z.painwait = d.painwait;
                int verbuendet = d.alliedTeam != 0 ? d.alliedTeam : d.teamnodmg;
                if (verbuendet == 1) {
                    z.noDamageTeam = kTeamSpieler;
                } else if (verbuendet == 2) {
                    z.noDamageTeam = kTeamFeind;
                }
                const int cs = std::min(d.customscale, 1023);
                const float skala = cs > 0 ? static_cast<float>(cs) / 100.0F : 1.0F;
                if ((mapFlags & 8) != 0) {
                    z.masse = d.shotspeed != 0.0F ? d.shotspeed : 20000.0F;
                    if (d.wait == 0.0F) { z.wait = 1000.0F; }
                    if (d.damage == 0) { z.damage = 500; }
                    if (verbuendet != 1 && verbuendet != 2) { z.noDamageTeam = kTeamFeind; }
                    z.nurSchwer = false;
                    z.takedamage = true;
                    z.maxs[0] = z.maxs[1] = 64.0F;
                    z.maxs[2] = 30.0F;
                    z.mins[0] = z.mins[1] = -64.0F;
                    z.mins[2] = -30.0F;
                } else {
                    z.masse = 1100.0F;
                }
                if (cs > 0 || (mapFlags & 8) != 0) {
                    l.skala = skala;
                    for (int k = 0; k < 3; ++k) {
                        z.mins[k] *= skala;
                        z.maxs[k] *= skala;
                    }
                }
            }
            z.bild = (z.sf & 1) != 0 ? 1 : 0;
        }
        for (int k = 0; k < 3; ++k) {
            z.aktWinkel[k] = l.winkel[k];
            z.aposBasis[k] = l.winkel[k];
            z.knochen.winkel[k] = l.winkel[k];
            z.knochen.ort[k] = l.ursprung[k];
        }
        l.daten = std::move(d);
        laeufe_.push_back(std::move(l));
        zustand_.push_back(z);
        merke(laeufe_.size() - 1, spawnMs);
    }
}

double GeschuetzSim::levelZeit(double t) const { return t - spawnMs_ + kLevelBeimSpawn; }
double GeschuetzSim::szenenZeit(double lt) const { return lt + spawnMs_ - kLevelBeimSpawn; }

void GeschuetzSim::merke(std::size_t i, double t, bool gedacht) {
    const Zustand& z = zustand_[i];
    GeschuetzStand s;
    s.ms = t;
    for (int k = 0; k < 3; ++k) {
        s.aposBasis[k] = z.aposBasis[k];
        s.aposDelta[k] = z.aposDelta[k];
    }
    s.aposZeit = szenenZeit(z.aposZeit);
    s.aposLinear = z.aposLinear;
    s.nick = z.knochen.nick;
    s.pasGier = z.knochen.pasGier;
    s.pasNick = z.knochen.pasNick;
    s.feind = z.feind != kSchussKeine;
    s.bild = z.bild;
    s.gedacht = gedacht;
    std::vector<GeschuetzStand>& st = laeufe_[i].staende;
    if (!st.empty() && st.back().ms == t) {
        s.gedacht = s.gedacht || st.back().gedacht;
        st.back() = s;
    } else {
        st.push_back(s);
    }
}

bool GeschuetzSim::nimmtSchaden(int nummer, float lo[3], float hi[3]) const {
    for (std::size_t i = 0; i < laeufe_.size(); ++i) {
        if (laeufe_[i].daten.nummer != nummer) { continue; }
        const Zustand& z = zustand_[i];
        for (int k = 0; k < 3; ++k) {
            lo[k] = laeufe_[i].ursprung[k] + z.mins[k] - 1.0F;   // absmin (SV_LinkEntity)
            hi[k] = laeufe_[i].ursprung[k] + z.maxs[k] + 1.0F;
        }
        return z.takedamage && !z.frei;
    }
    return false;
}

bool GeschuetzSim::steht(int nummer, double ms, float lo[3], float hi[3]) const {
    for (std::size_t i = 0; i < laeufe_.size(); ++i) {
        const GeschuetzLauf& l = laeufe_[i];
        if (l.daten.nummer != nummer) { continue; }
        if (l.freiMs >= 0.0 && ms >= l.freiMs) { return false; }
        const Zustand& z = zustand_[i];
        for (int k = 0; k < 3; ++k) {
            lo[k] = l.ursprung[k] + z.mins[k];
            hi[k] = l.ursprung[k] + z.maxs[k];
        }
        return true;
    }
    return false;
}

void GeschuetzSim::bild(double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w,
                        std::vector<SchadenAuftrag>& auftraege) {
    const double lt = levelZeit(t);
    for (std::size_t i = 0; i < laeufe_.size(); ++i) {
        Zustand& z = zustand_[i];
        GeschuetzLauf& l = laeufe_[i];
        if (z.frei) { continue; }
        // G_RunThink am Platz der Entity: ein faelliges think vor den uses dieses
        // Bilds (die Reihenfolge der Benutzer kennt die Vorschau nicht).
        if (z.denkt && z.naechstes > 0.0 && z.naechstes <= lt + 1e-6) {
            z.denkt = false;   // "ent->nextthink = 0" vor dem think
            z.naechstes = 0.0;
            denke(i, lt, ziele, w);
            merke(i, t, true);
        }
        while (z.naechsterUse < l.daten.uses.size() && l.daten.uses[z.naechsterUse] <= t + 1e-6) {
            ++z.naechsterUse;
            if (z.tot) { continue; }   // e_UseFunc = NULL (turret_die)
            z.sf ^= 1;
            if (l.daten.art == GeschuetzArt::Wache) {
                // pas_use: an -> think in 50 ms, aus -> kein think
                if ((z.sf & 1) != 0) {
                    z.denkt = false;
                    z.naechstes = 0.0;
                } else {
                    z.denkt = true;
                    z.naechstes = lt + 50.0;
                }
            } else {
                // turret_base_use: s.frame 1 (schwarz) bei aus mit EF_SHADER_ANIM
                z.bild = (z.sf & 1) != 0 ? 1 : 0;
            }
            merke(i, t);
        }
    }
    for (Geschoss& g : geschosse_) {
        if (g.fliegt && g.abMs < t - 1e-6) { fliege(g, lt, ziele, w, auftraege); }
    }
}

void GeschuetzSim::denke(std::size_t i, double lt, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w) {
    if (laeufe_[i].daten.art == GeschuetzArt::Wache) {
        pasDenken(i, lt, ziele, w);
    } else {
        turmDenken(i, lt, ziele, w);
    }
}

namespace {
// G_RadiusList: der Kasten (absmin/absmax) naeher als radius am Punkt.
bool imRadius(const SchussZiel& z, const float org[3], float radius) {
    float v[3];
    for (int k = 0; k < 3; ++k) {
        v[k] = org[k] < z.lo[k] ? z.lo[k] - org[k] : (org[k] > z.hi[k] ? org[k] - z.hi[k] : 0.0F);
    }
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) < radius;
}
const SchussZiel* zielMit(const std::vector<SchussZiel>& ziele, int nummer) {
    for (const SchussZiel& z : ziele) {
        if (z.nummer == nummer) { return &z; }
    }
    return nullptr;
}
}  // namespace

void GeschuetzSim::pasDenken(std::size_t i, double lt, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w) {
    Zustand& z = zustand_[i];
    GeschuetzLauf& l = laeufe_[i];
    const double t = szenenZeit(lt);
    if (!z.aufgeklappt) {
        // "let us do our animation": damage = 1, Animation 0..11 mit 0.8, nextthink + 1200
        z.aufgeklappt = true;
        z.knochen.aufklappAb = lt;
        l.aufklappen.emplace_back(t, 1);
        z.denkt = true;
        z.naechstes = lt + 1200.0;
        return;
    }
    if (z.count == 0) { return; }   // "turrets that have no ammo may as well do nothing"
    z.denkt = true;
    z.naechstes = lt + kFrametime;
    const auto augeBolzen = [&](float org2[3]) {
        float m[3][4];
        for (int k = 0; k < 3; ++k) { z.knochen.ort[k] = l.ursprung[k]; }
        if (w.bolzen && w.bolzen(i, t, z.knochen, "*flash02", m)) {
            for (int k = 0; k < 3; ++k) { org2[k] = m[k][3]; }
            return true;
        }
        for (int k = 0; k < 3; ++k) { org2[k] = l.ursprung[k]; }
        return false;
    };
    const auto zielPunkt = [&](int nr, float org[3], float abzug) {
        // client: renderInfo.eyePoint - abzug, sonst currentOrigin
        if (const SchussZiel* zz = zielMit(ziele, nr)) {
            for (int k = 0; k < 3; ++k) { org[k] = zz->auge[k]; }
            org[2] -= abzug;
            return;
        }
        float lo[3];
        float hi[3];
        for (int k = 0; k < 3; ++k) { org[k] = 0.0F; }
        for (std::size_t j = 0; j < laeufe_.size(); ++j) {
            if (laeufe_[j].daten.nummer == nr && steht(nr, t, lo, hi)) {
                for (int k = 0; k < 3; ++k) { org[k] = laeufe_[j].ursprung[k]; }
            }
        }
    };
    const auto feindHealth = [&](int nr) {
        if (const SchussZiel* zz = zielMit(ziele, nr)) { return zz->health; }
        for (std::size_t j = 0; j < laeufe_.size(); ++j) {
            if (laeufe_[j].daten.nummer == nr) { return zustand_[j].health; }
        }
        return 0;   // die Welt (ENTITYNUM_WORLD) hat health 0
    };
    if (z.feind != kSchussKeine) {
        // pas_adjust_enemy
        bool behalten = true;
        if (feindHealth(z.feind) <= 0) {
            behalten = false;
        } else {
            float org2[3];
            float org[3];
            (void)augeBolzen(org2);
            zielPunkt(z.feind, org, 15.0F);
            const float null3[3] = {0.0F, 0.0F, 0.0F};
            const SchussSpur tr = w.spur ? w.spur(t, org2, null3, null3, org, l.daten.nummer, false) : SchussSpur{};
            if (tr.allsolid || tr.startsolid || tr.entity != z.feind) { behalten = false; }
        }
        if (behalten) {
            z.bounceCount = lt + 500.0 + static_cast<double>(z.zufall.flrand(0.0F, 1.0F) * 150.0F);
        } else if (z.bounceCount < lt) {
            z.feind = kSchussKeine;   // shutdown.wav
            z.aimDebounce = lt + 5000.0;
        }
    }
    if (z.feind == kSchussKeine) {
        // pas_find_enemies
        if (z.aimDebounce > lt && z.painDebounce < lt) { z.painDebounce = lt + 1000.0; }   // ping.wav
        float org2[3];
        (void)augeBolzen(org2);
        float besterAbstand = z.radius * z.radius;
        bool gefunden = false;
        for (const SchussZiel& zz : ziele) {
            if (!zz.takedamage || !imRadius(zz, org2, z.radius)) { continue; }
            if (zz.health <= 0 || zz.notarget) { continue; }
            if (zz.team == z.noDamageTeam) { continue; }
            if (w.pvs && !w.pvs(org2, zz.ort)) { continue; }
            float org[3] = {zz.auge[0], zz.auge[1], zz.auge[2] - 15.0F};
            const float null3[3] = {0.0F, 0.0F, 0.0F};
            const SchussSpur tr = w.spur ? w.spur(t, org2, null3, null3, org, l.daten.nummer, false) : SchussSpur{};
            if (tr.allsolid || tr.startsolid || !(tr.fraction == 1.0F || tr.entity == zz.nummer)) { continue; }
            const float abstand = abstandQuadrat(zz.ort, l.ursprung);
            // (G_StartFlee fuer NPCs: KI - nicht nachgebildet)
            if (abstand < besterAbstand) {
                if (z.attackDebounce + 2000.0 < lt) {
                    // startup.wav, "Wind up turrets for a bit"
                    z.attackDebounce = lt + 900.0 + static_cast<double>(z.zufall.flrand(0.0F, 1.0F) * 200.0F);
                }
                z.feind = zz.nummer;   // G_SetEnemy
                besterAbstand = abstand;
                gefunden = true;
            }
        }
        if (gefunden && l.daten.hatTarget2) { benutzt_.push_back(GeschuetzBenutzung{t, l.daten.nummer, 1}); }
    }
    z.speed = winkelNorm360(z.speed);
    z.random = winkelNorm360(z.random);
    float diffYaw = 0.0F;
    float diffPitch = 0.0F;
    if (z.feind != kSchussKeine) {
        float org[3];
        zielPunkt(z.feind, org, 40.0F);
        float richtung[3] = {org[0] - l.ursprung[0], org[1] - l.ursprung[1], org[2] - l.ursprung[2]};
        float ziel[3];
        vektorWinkel(richtung, ziel);
        diffYaw = winkelAbzug(z.speed, ziel[1]);
        diffPitch = winkelAbzug(z.random, ziel[0]);
    } else {
        // "no enemy, so make us slowly sweep back and forth as if searching for a new one"
        diffYaw = std::sin(static_cast<float>(static_cast<int>(lt)) * 0.0001F + static_cast<float>(z.count)) * 2.0F;
    }
    if (std::fabs(diffYaw) > 0.25F) {
        if (std::fabs(diffYaw) > 10.0F) {
            z.speed += diffYaw > 0.0F ? -10.0F : 10.0F;
        } else {
            z.speed -= diffYaw;
        }
    }
    if (std::fabs(diffPitch) > 0.25F) {
        if (std::fabs(diffPitch) > 4.0F) {
            z.random += diffPitch > 0.0F ? -4.0F : 4.0F;
        } else {
            z.random -= diffPitch;
        }
    }
    // frontAngles (-random, 0, 0) an bone_barrel/bone_gback, backAngles (0, 0, speed - s.angles[YAW]) an bone_hinge
    z.knochen.pasNick = z.random;
    z.knochen.pasGier = z.speed - l.winkel[1];
    if (z.feind != kSchussKeine && z.attackDebounce < lt && z.zufall.flrand(0.0F, 1.0F) > 0.3F) {
        --z.count;
        if (z.count != 0) {
            // pas_fire
            float m[3][4];
            float org[3] = {l.ursprung[0], l.ursprung[1], l.ursprung[2]};
            float vorn[3] = {0.0F, 1.0F, 0.0F};
            for (int k = 0; k < 3; ++k) { z.knochen.ort[k] = l.ursprung[k]; }
            if (w.bolzen && w.bolzen(i, t, z.knochen, "*flash02", m)) {
                for (int k = 0; k < 3; ++k) {
                    org[k] = m[k][3];
                    vorn[k] = m[k][1];   // POSITIVE_Y
                }
            }
            GeschuetzSchuss s;
            s.ms = t;
            for (int k = 0; k < 3; ++k) {
                s.ort[k] = org[k];
                s.richtung[k] = vorn[k];
            }
            s.effekt = "turret/muzzle_flash";
            l.schuesse.push_back(s);
            Geschoss g;
            g.geschuetz = i;
            g.waffe = 1;   // WP_TURRET
            g.abMs = t;
            g.besitzer = l.daten.nummer;
            g.damage = kPasSchaden;
            g.dflags = kSchadenOhneRueckstoss;
            g.mittel = SchussMittel::Energie;
            g.besitzerAnfang = g.besitzer;
            GeschossBahn b;
            b.abMs = t;
            b.zeit = t;
            for (int k = 0; k < 3; ++k) {
                g.maxs[k] = 1.0F;
                g.mins[k] = -1.0F;
                b.basis[k] = org[k];
                g.cur[k] = org[k];
                b.delta[k] = vorn[k] * 900.0F;
            }
            schnappe(b.delta);
            g.bahnen.push_back(b);
            g.lebenBis = lt + 10000.0;
            geschosse_.push_back(g);
        } else {
            z.denkt = false;   // nextthink = 0, shutdown.wav
            z.naechstes = 0.0;
        }
    }
}

void GeschuetzSim::schnappe(float v[3]) const {
    // SnapVector (q_math.c:1240-1265 OJ): Windows x86 mit fistp (zum Naechsten,
    // bei Gleichstand gerade), sonst (int).
    for (int k = 0; k < 3; ++k) {
        v[k] = snapRunden_ ? static_cast<float>(std::nearbyint(v[k])) : static_cast<float>(static_cast<int>(v[k]));
    }
}

void GeschuetzSim::turmDenken(std::size_t i, double lt, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w) {
    Zustand& z = zustand_[i];
    GeschuetzLauf& l = laeufe_[i];
    const double t = szenenZeit(lt);
    const bool md = regel_ == TodRegelwerk::MovieDuels;
    const bool jk2 = regel_ == TodRegelwerk::JediOutcast;
    const bool turbo = (z.sf & kTurbo) != 0;
    const bool mp = (z.sf & kMpRegeln) != 0;
    z.denkt = true;
    z.naechstes = lt + kFrametime;
    if (z.tot) {
        // MD: "destroyed: only a misc_turretG2 that can respawn still thinks"
        if ((z.sf & kKannWieder) != 0) {
            // turretG2_respawn
            z.tot = false;
            z.takedamage = true;
            z.health = z.maxHealth;
            z.feind = kSchussKeine;
            z.bild = 0;
            l.wiederMs.push_back(t);
            l.health.emplace_back(t, z.health);
            z.naechstes = lt + kFrametime;
        } else {
            z.denkt = false;
            z.naechstes = 0.0;
        }
        return;
    }
    const auto istClient = [&](int nr) { return zielMit(ziele, nr) != nullptr; };
    const auto zielPunkt = [&](int nr, float org[3]) {
        // turret_target_point (MD) / renderInfo.eyePoint bzw. currentOrigin
        if (const SchussZiel* zz = zielMit(ziele, nr)) {
            const bool auge = !(md && zz->fahrzeug);
            for (int k = 0; k < 3; ++k) { org[k] = auge ? zz->auge[k] : zz->ort[k]; }
            return;
        }
        for (int k = 0; k < 3; ++k) { org[k] = 0.0F; }
        for (std::size_t j = 0; j < laeufe_.size(); ++j) {
            if (laeufe_[j].daten.nummer == nr) {
                for (int k = 0; k < 3; ++k) { org[k] = laeufe_[j].ursprung[k]; }
            }
        }
    };
    const auto zielOrt = [&](int nr, float org[3]) {
        if (const SchussZiel* zz = zielMit(ziele, nr)) {
            for (int k = 0; k < 3; ++k) { org[k] = zz->ort[k]; }
            return;
        }
        zielPunkt(nr, org);
    };
    const auto feindHealth = [&](int nr) {
        if (const SchussZiel* zz = zielMit(ziele, nr)) { return zz->health; }
        for (std::size_t j = 0; j < laeufe_.size(); ++j) {
            if (laeufe_[j].daten.nummer == nr) { return zustand_[j].health; }
        }
        return 0;
    };
    const char* muendung = turbo ? (z.altFire ? "*muzzle2" : "*muzzle1") : "*flash03";
    const auto bolzen = [&](const float ort[3], float m[3][4]) {
        for (int k = 0; k < 3; ++k) {
            z.knochen.winkel[k] = z.aktWinkel[k];
            z.knochen.ort[k] = ort[k];
        }
        z.knochen.nick = z.speed;
        if (w.bolzen && w.bolzen(i, t, z.knochen, muendung, m)) { return true; }
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) { m[r][c] = c == 3 ? ort[r] : (r == c ? 1.0F : 0.0F); }
        }
        return false;
    };
    const auto turnoff = [&]() {
        if (z.feind == kSchussKeine) { return; }
        if (turbo) {
            z.knochen.turboVon = 4;   // TurboLaser_SetBoneAnim(4, 5)
            z.knochen.turboBis = 5;
            z.knochen.turboAb = lt;
            l.turboAnims.emplace_back(t, std::make_pair(4, 5));
        }
        z.aimDebounce = lt + 5000.0;   // shutdown.wav
        z.feind = kSchussKeine;
    };
    const auto zielen = [&]() {
        // turret_aim
        const float maxGier = turbo ? 30.0F : 14.0F;
        const float maxNick = turbo ? 15.0F : 3.0F;
        bahnBei(z.aposLinear, z.aposBasis, z.aposDelta, z.aposZeit, lt, z.aktWinkel);
        z.aktWinkel[1] = winkelNorm360(z.aktWinkel[1]);
        z.speed = winkelNorm360(z.speed);
        float diffYaw = 0.0F;
        float diffPitch = 0.0F;
        if (z.feind != kSchussKeine) {
            float org[3];
            zielPunkt(z.feind, org);
            org[2] -= (z.sf & 2) != 0 ? 15.0F : 5.0F;
            if (md && (z.sf & kLead) != 0 && mp && z.masse > 0.0F) {
                // "aim ahead of a moving target"
                float diff[3] = {org[0] - l.ursprung[0], org[1] - l.ursprung[1], org[2] - l.ursprung[2]};
                const float dist = vektorNormieren(diff);
                float tempo[3] = {0.0F, 0.0F, 0.0F};
                if (const SchussZiel* zz = zielMit(ziele, z.feind)) {
                    for (int k = 0; k < 3; ++k) { tempo[k] = zz->tempo[k]; }
                }
                for (int k = 0; k < 3; ++k) { org[k] += dist / z.masse * tempo[k]; }
            }
            float m[3][4];
            (void)bolzen(l.ursprung, m);   // mit s.origin
            float richtung[3] = {org[0] - m[0][3], org[1] - m[1][3], org[2] - m[2][3]};
            float ziel[3];
            vektorWinkel(richtung, ziel);
            diffYaw = winkelAbzug(z.aktWinkel[1], ziel[1]);
            diffPitch = winkelAbzug(z.speed, ziel[0]);
        }
        if (diffYaw != 0.0F) {
            if (std::fabs(diffYaw) > maxGier) { diffYaw = diffYaw >= 0.0F ? maxGier : -maxGier; }
            for (int k = 0; k < 3; ++k) {
                z.aposBasis[k] = z.aktWinkel[k];
                z.aposDelta[k] = 0.0F;
            }
            z.aposDelta[1] = diffYaw * -5.0F;
            z.aposZeit = lt;
            z.aposLinear = true;
        }
        if (diffPitch != 0.0F) {
            if (std::fabs(diffPitch) > maxNick) {
                z.speed += diffPitch > 0.0F ? -maxNick : maxNick;
            } else {
                z.speed -= diffPitch;
            }
            z.knochen.nick = z.speed;
        }
    };
    if ((z.sf & 1) != 0) {
        // "not turned on"
        turnoff();
        zielen();
        z.notarget = true;
        return;
    }
    z.notarget = false;
    bool abschalten = true;
    if (z.feind == kSchussKeine) {
        // turret_find_enemies
        if (!(turbo && !(md && mp))) {
            if (z.aimDebounce > lt && z.painDebounce < lt) { z.painDebounce = lt + 1000.0; }   // ping.wav
            float org2[3] = {l.ursprung[0], l.ursprung[1], l.ursprung[2]};
            org2[2] += (z.sf & 2) != 0 ? 20.0F : -20.0F;
            float besterAbstand = z.radius * z.radius;
            int bester = kSchussKeine;
            for (const SchussZiel& zz : ziele) {
                if (!zz.takedamage || !imRadius(zz, org2, z.radius)) { continue; }
                if (zz.health <= 0 || zz.notarget) { continue; }
                if (md && mp) {
                    if (zz.fahrzeug) { continue; }   // "a ship: only with someone in it" - ohne Pilot
                    if (zz.team == z.noDamageTeam) { continue; }
                } else if (zz.team == z.noDamageTeam) {
                    continue;
                }
                if (w.pvs && !w.pvs(org2, zz.ort)) { continue; }
                float org[3];
                zielPunkt(zz.nummer, org);
                org[2] += (z.sf & 2) != 0 ? -15.0F : 5.0F;
                const float null3[3] = {0.0F, 0.0F, 0.0F};
                const SchussSpur tr = w.spur ? w.spur(t, org2, null3, null3, org, l.daten.nummer, false) : SchussSpur{};
                if (tr.allsolid || tr.startsolid || !(tr.fraction == 1.0F || tr.entity == zz.nummer)) { continue; }
                const float abstand = abstandQuadrat(zz.ort, l.ursprung);
                if (abstand < besterAbstand) {
                    if (z.attackDebounce < lt) { z.attackDebounce = lt + 1400.0; }   // startup.wav
                    bester = zz.nummer;
                    besterAbstand = abstand;
                }
            }
            if (bester != kSchussKeine) {
                // AddSoundEvent/AddSightEvent (KI); G_SetEnemy
                z.feind = bester;
                if (l.daten.hatTarget2) { benutzt_.push_back(GeschuetzBenutzung{t, l.daten.nummer, 1}); }
                abschalten = false;
            }
        }
    } else {
        if (feindHealth(z.feind) > 0) {
            float fo[3];
            zielOrt(z.feind, fo);
            if (abstandQuadrat(fo, l.ursprung) < z.radius * z.radius) {
                float org2[3] = {l.ursprung[0], l.ursprung[1], l.ursprung[2]};
                if (md) {
                    // MD: von dort, wo turret_find_enemies sucht (nicht der Turbo ohne MP_RULES)
                    if (!turbo || mp) { org2[2] += (z.sf & 2) != 0 ? 20.0F : -20.0F; }
                }
                // OJ/JK2: inPVS(currentOrigin, ...), die Spur von currentOrigin +-10
                float pvsVon[3] = {org2[0], org2[1], org2[2]};
                if (!md) {
                    for (int k = 0; k < 3; ++k) { pvsVon[k] = l.ursprung[k]; }
                }
                if (!w.pvs || w.pvs(pvsVon, fo)) {
                    float org[3];
                    zielPunkt(z.feind, org);
                    if (!md) { org2[2] += (z.sf & 2) != 0 ? 10.0F : -10.0F; }
                    const float null3[3] = {0.0F, 0.0F, 0.0F};
                    const SchussSpur tr = w.spur ? w.spur(t, org2, null3, null3, org, l.daten.nummer, false) : SchussSpur{};
                    const bool turboOhneSicht = turbo && !(md && mp) && !jk2;
                    if (turboOhneSicht || (!tr.allsolid && !tr.startsolid && tr.entity == z.feind)) { abschalten = false; }
                }
            }
        }
        // turret_head_think
        if (z.pushDebounce < lt && z.attackDebounce < lt) {
            z.pushDebounce = lt + z.wait;
            float m[3][4];
            (void)bolzen(l.ursprung, m);   // currentOrigin
            if (turbo) { z.altFire = !z.altFire; }
            float org[3];
            float fwd[3];
            for (int k = 0; k < 3; ++k) {
                org[k] = m[k][3];
                fwd[k] = turbo ? -m[k][1] : m[k][1];   // NEGATIVE_Y / POSITIVE_Y
            }
            for (int k = 0; k < 3; ++k) { org[k] += kStartDis * fwd[k]; }
            feuere(i, lt, org, fwd, w);
        }
    }
    if (abschalten) {
        if (z.bounceCount < lt) { turnoff(); }
    } else {
        z.bounceCount = lt + 2000.0 + static_cast<double>(z.zufall.flrand(0.0F, 1.0F) * 150.0F);
    }
    zielen();
}

void GeschuetzSim::feuere(std::size_t i, double lt, const float start[3], const float richtung[3], const GeschuetzWelt& w) {
    // turret_fire (OJ, MD) / turret_fire (JK2: gi.pointcontents(start, MASK_SHOT) -
    // MASK_SHOT als passEntityNum, also jeder Inhalt; keine Streuung, kein Turbo).
    Zustand& z = zustand_[i];
    GeschuetzLauf& l = laeufe_[i];
    const double t = szenenZeit(lt);
    const bool jk2 = regel_ == TodRegelwerk::JediOutcast;
    const bool turbo = (z.sf & kTurbo) != 0;
    constexpr std::uint32_t kMaskeSchuss = 0x00000001U | 0x00000100U | 0x00000200U | 0x00000080U | 0x00001000U;   // MASK_SHOT
    if (w.inhalt) {
        const std::uint32_t c = w.inhalt(start, jk2 ? kSchussKeine : l.daten.nummer);
        if (jk2 ? c != 0U : (c & kMaskeSchuss) != 0U) { return; }
    }
    float dir[3] = {richtung[0], richtung[1], richtung[2]};
    float org[3];
    for (int k = 0; k < 3; ++k) { org[k] = start[k] - kStartDis * dir[k]; }   // "dumb...."
    float ang[3];
    if (!jk2 && z.random != 0.0F) {
        vektorWinkel(dir, ang);
        ang[0] += z.zufall.flrand(-z.random, z.random);
        ang[1] += z.zufall.flrand(-z.random, z.random);
        winkelVektoren(ang, dir, nullptr, nullptr);
    }
    vektorWinkel(dir, ang);
    GeschuetzSchuss s;
    s.ms = t;
    for (int k = 0; k < 3; ++k) { s.ort[k] = org[k]; }
    Geschoss g;
    g.geschuetz = i;
    g.abMs = t;
    g.besitzer = l.daten.nummer;
    g.besitzerAnfang = g.besitzer;
    g.lebenBis = lt + 10000.0;
    GeschossBahn b;
    b.abMs = t;
    b.zeit = t;
    for (int k = 0; k < 3; ++k) {
        b.basis[k] = start[k];
        g.cur[k] = start[k];
    }
    if (turbo) {
        // G_PlayEffect("turret/turb_muzzle_flash", org, ang) - OpenJK gibt die
        // WINKEL als Richtung (MD: dir).
        s.effekt = "turret/turb_muzzle_flash";
        for (int k = 0; k < 3; ++k) { s.richtung[k] = regel_ == TodRegelwerk::MovieDuels ? dir[k] : ang[k]; }
        // WP_FireTurboLaserMissile: CreateMissile(start, dir, (int)mass, 10000) ohne SnapVector
        const auto tempo = static_cast<float>(static_cast<int>(z.masse));
        g.waffe = 2;   // WP_TIE_FIGHTER
        for (int k = 0; k < 3; ++k) { b.delta[k] = dir[k] * tempo; }
        g.damage = z.damage;
        g.splashDamage = z.splashDamage;
        g.splashRadius = z.splashRadius;
        g.dflags = kSchadenTodesRueckstoss;
        g.mittel = SchussMittel::Platziert;
        g.abprallZahl = 8;
        // TurboLaser_SetBoneAnim(alt_fire ? 2,3 : 0,1)
        z.knochen.turboVon = z.altFire ? 2 : 0;
        z.knochen.turboBis = z.altFire ? 3 : 1;
        z.knochen.turboAb = lt;
        l.turboAnims.emplace_back(t, std::make_pair(z.knochen.turboVon, z.knochen.turboBis));
    } else {
        s.effekt = "blaster/muzzle_flash";
        for (int k = 0; k < 3; ++k) { s.richtung[k] = dir[k]; }
        g.waffe = 0;   // WP_BLASTER
        g.damage = z.damage;
        g.dflags = kSchadenOhneRueckstoss | kSchadenSchwereWaffe;
        g.mittel = SchussMittel::Energie;
        for (int k = 0; k < 3; ++k) {
            g.maxs[k] = 1.5F;
            g.mins[k] = -1.5F;
            b.delta[k] = dir[k] * 1100.0F;
        }
        schnappe(b.delta);
    }
    g.bahnen.push_back(b);
    l.schuesse.push_back(s);
    geschosse_.push_back(g);
}

void GeschuetzSim::fliege(Geschoss& g, double lt, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w,
                          std::vector<SchadenAuftrag>& auftraege) {
    // G_RunMissile (g_missile.cpp:1285-1500 OJ) fuer TR_LINEAR
    const double t = szenenZeit(lt);
    float origin[3];
    const GeschossBahn& b = g.bahnen.back();
    float delta[3] = {b.delta[0], b.delta[1], b.delta[2]};
    bahnBei(true, b.basis, b.delta, levelZeit(b.zeit), lt, origin);
    // gi.trace(currentOrigin, mins, maxs, origin, owner, clipmask, G2_COLLIDE, 10)
    SchussSpur tr;
    if (w.spur) {
        tr = w.spur(t, g.cur, g.mins, g.maxs, origin, g.besitzer, true);
    } else {
        for (int k = 0; k < 3; ++k) { tr.endpos[k] = origin[k]; }
    }
    for (int k = 0; k < 3; ++k) { g.cur[k] = tr.endpos[k]; }
    // G2-Kollisionen: der Trefferort der ersten (hitEnt->health != 0)
    int trefferOrt = kOrtKeiner;
    if (!tr.flaeche.empty()) {
        const SchussZiel* zz = zielMit(ziele, tr.entity);
        if (zz != nullptr && zz->health != 0 && w.trefferOrt) { trefferOrt = w.trefferOrt(tr.entity, t, tr.flaeche, tr.g2Punkt); }
    }
    if (tr.startsolid) { tr.fraction = 0.0F; }
    // G_RunThink: nextthink = G_FreeEntity
    if (g.lebenBis <= lt + 1e-6) {
        g.fliegt = false;
        g.endeMs = t;
        for (int k = 0; k < 3; ++k) { g.ende[k] = g.cur[k]; }
        return;
    }
    if (tr.fraction == 1.0F) { return; }
    if ((tr.flaechenFlags & kFlaecheOhneEinschlag) != 0U) {
        // "never explode or bounce on sky"
        g.fliegt = false;
        g.endeMs = t;
        for (int k = 0; k < 3; ++k) { g.ende[k] = g.cur[k]; }
        return;
    }
    // G_MissileImpact
    const int other = tr.entity;
    if (tr.normale[0] == 0.0F && tr.normale[1] == 0.0F && tr.normale[2] == 0.0F) {
        // "model moved into missile in flight probably..."
        for (int k = 0; k < 3; ++k) { tr.normale[k] = -delta[k]; }
        (void)vektorNormieren(tr.normale);
    }
    float lo[3];
    float hi[3];
    const SchussZiel* zielFigur = zielMit(ziele, other);
    bool takedamage = zielFigur != nullptr ? zielFigur->takedamage : nimmtSchaden(other, lo, hi);
    if (w.nimmtSchaden && zielFigur == nullptr && !takedamage) { takedamage = w.nimmtSchaden(other, t); }
    const bool kraftfeld = (tr.flaechenFlags & kFlaecheKraftfeld) != 0U;
    bool abprall = kraftfeld && g.splashDamage == 0 && g.splashRadius == 0;   // (FL_SHIELDED setzt hier niemand)
    if ((g.dflags & kSchadenSchwereWaffe) != 0) { abprall = false; }
    const auto prallAb = [&]() {
        // G_BounceMissile: velocity = EvaluateTrajectoryDelta (TR_LINEAR: trDelta),
        // an der Ebene gespiegelt; hitTime = previousTime + 50 * fraction (int)
        const float dot = delta[0] * tr.normale[0] + delta[1] * tr.normale[1] + delta[2] * tr.normale[2];
        float neu[3];
        for (int k = 0; k < 3; ++k) { neu[k] = delta[k] - 2.0F * dot * tr.normale[k]; }
        float ort[3];
        for (int k = 0; k < 3; ++k) { ort[k] = tr.endpos[k] + tr.normale[k]; }
        const double hit = (lt - 50.0) + static_cast<double>(static_cast<int>(50.0 * static_cast<double>(tr.fraction)));
        GeschossBahn nb;
        nb.abMs = t;
        nb.zeit = szenenZeit(hit >= lt ? lt - 10.0 : hit - 10.0);
        for (int k = 0; k < 3; ++k) {
            nb.basis[k] = ort[k];
            nb.delta[k] = neu[k];
            g.cur[k] = ort[k];
        }
        g.bahnen.push_back(nb);
        g.besitzer = kSchussKeine;   // "now you can damage the guy you came from"
    };
    if (abprall) {
        if (g.abprallZahl != 0) { --g.abprallZahl; }
        prallAb();
        return;
    }
    if (kraftfeld && g.splashDamage == 0 && g.splashRadius == 0) {
        // EF_BOUNCE_SHRAPNEL / SURF_FORCEFIELD ohne Splash (auch schwere Waffen);
        // Schwerter (CONTENTS_LIGHTSABER) kennt die Vorschau hier nicht.
        prallAb();
        --g.abprallZahl;
        return;
    }
    // G_MissileImpacted
    if (takedamage && g.damage != 0) {
        SchadenAuftrag a;
        a.ms = t;
        a.ziel = other;
        a.angreifer = g.besitzerAnfang;
        a.schaden = g.damage;
        a.dflags = g.dflags;
        a.mittel = g.mittel;
        a.trefferOrt = trefferOrt;
        for (int k = 0; k < 3; ++k) {
            a.punkt[k] = tr.endpos[k];
            a.richtung[k] = delta[k];
        }
        if (delta[0] == 0.0F && delta[1] == 0.0F && delta[2] == 0.0F) { a.richtung[2] = 1.0F; }
        auftraege.push_back(a);
    }
    g.fleisch = takedamage && zielFigur != nullptr;
    g.einschlag = true;
    g.getroffen = other;
    g.fliegt = false;
    g.endeMs = t;
    for (int k = 0; k < 3; ++k) {
        g.ende[k] = tr.endpos[k];
        g.normale[k] = tr.normale[k];
    }
    if (g.splashDamage != 0) {
        SchadenAuftrag a;
        a.radius = true;
        a.ms = t;
        a.angreifer = g.besitzerAnfang;
        a.ignoriere = other;
        a.schaden = g.splashDamage;
        a.weite = static_cast<float>(g.splashRadius);
        a.dflags = kSchadenRadius;
        a.mittel = g.mittel;
        for (int k = 0; k < 3; ++k) { a.punkt[k] = tr.endpos[k]; }
        auftraege.push_back(a);
    }
}

bool GeschuetzSim::schaden(int nummer, double t, int schaden, int dflags, SchussMittel mittel, int angreifer,
                           const float punkt[3], std::vector<SchadenAuftrag>& folgen) {
    (void)mittel;
    const double lt = levelZeit(t);
    for (std::size_t i = 0; i < laeufe_.size(); ++i) {
        GeschuetzLauf& l = laeufe_[i];
        Zustand& z = zustand_[i];
        if (l.daten.nummer != nummer) { continue; }
        // G_Damage: takedamage, health <= 0 (kein client), FL_DMG_BY_HEAVY_WEAP_ONLY
        if (!z.takedamage || z.frei || z.health <= 0) { return false; }
        if (z.nurSchwer && (dflags & kSchadenSchwereWaffe) == 0) { return false; }
        // take = damage (ohne Ruestung, HL_NONE: Faktor 1)
        const int take = schaden;
        if (take == 0) { return false; }
        z.health = std::max(0, z.health - take);
        l.health.emplace_back(t, z.health);
        if (z.health <= 0) {
            if (z.feind == kSchussKeine) { z.feind = angreifer; }   // "just killed and didn't have an enemy before"
            stirb(i, lt, angreifer, folgen);
            return true;
        }
        // TurretPain: Funken; OJ/MD: ohne Feind G_SetEnemy(self, attacker)
        GeschuetzEffekt fx;
        fx.ms = t;
        float dir[3] = {punkt[0] - l.ursprung[0], punkt[1] - l.ursprung[1], punkt[2] - l.ursprung[2]};
        (void)vektorNormieren(dir);
        for (int k = 0; k < 3; ++k) {
            fx.ort[k] = punkt[k];
            fx.richtung[k] = dir[k];
        }
        fx.effekt = regel_ == TodRegelwerk::JediOutcast ? "spark_exp_nosnd" : "sparks/spark_exp_nosnd";
        l.effekte.push_back(fx);
        if (regel_ != TodRegelwerk::JediOutcast && z.feind == kSchussKeine && angreifer != kSchussKeine) { z.feind = angreifer; }
        if (regel_ == TodRegelwerk::MovieDuels && l.daten.hatPaintarget && z.paintargetBis <= lt) {
            benutzt_.push_back(GeschuetzBenutzung{t, l.daten.nummer, 2});
            z.paintargetBis = lt + z.painwait;
        }
        merke(i, t);
        return true;
    }
    return false;
}

void GeschuetzSim::stirb(std::size_t i, double lt, int angreifer, std::vector<SchadenAuftrag>& folgen) {
    // turret_die (g_turret.cpp:71-137 OJ, :97-179 MD)
    Zustand& z = zustand_[i];
    GeschuetzLauf& l = laeufe_[i];
    const double t = szenenZeit(lt);
    z.denkt = false;
    z.naechstes = 0.0;
    z.tot = true;
    z.takedamage = false;
    z.health = 0;
    l.totMs = t;
    float vorn[3] = {0.0F, 0.0F, -1.0F};
    if ((z.sf & 2) != 0) { vorn[2] = 1.0F; }
    GeschuetzEffekt fx;
    fx.ms = t;
    if ((z.sf & kTurbo) != 0) {
        fx.effekt = "explosions/fighter_explosion2";
        for (int k = 0; k < 3; ++k) {
            fx.ort[k] = l.ursprung[k];
            // OJ: currentAngles als Richtung; MD: hoch bzw. runter
            fx.richtung[k] = regel_ == TodRegelwerk::MovieDuels ? (k == 2 ? ((z.sf & 2) != 0 ? -1.0F : 1.0F) : 0.0F)
                                                                 : z.aktWinkel[k];
        }
    } else {
        fx.effekt = "turret/explode";   // fxID
        for (int k = 0; k < 3; ++k) {
            fx.ort[k] = l.ursprung[k] + 12.0F * vorn[k];
            fx.richtung[k] = vorn[k];
        }
    }
    l.effekte.push_back(fx);
    if (z.splashDamage > 0 && z.splashRadius > 0) {
        SchadenAuftrag a;
        a.radius = true;
        a.ms = t;
        a.angreifer = angreifer;
        a.ignoriere = angreifer;
        a.schaden = z.splashDamage;
        a.weite = static_cast<float>(z.splashRadius);
        a.dflags = kSchadenRadius;
        a.mittel = SchussMittel::Unbekannt;
        for (int k = 0; k < 3; ++k) { a.punkt[k] = l.ursprung[k]; }
        folgen.push_back(a);
    }
    z.bild = 1;   // EF_SHADER_ANIM: schwarz
    if (l.daten.art != GeschuetzArt::Wache && (z.sf & kTurbo) == 0) {
        // s.modelindex2: das Schadensmodell (der Ghoul2-Stand bleibt sichtbar), apos steht
        for (int k = 0; k < 3; ++k) {
            z.aposBasis[k] = z.aktWinkel[k];
            z.aposDelta[k] = 0.0F;
        }
        z.aposZeit = lt;
        if (l.daten.hatTarget) { benutzt_.push_back(GeschuetzBenutzung{t, l.daten.nummer, 0}); }
        if ((z.sf & kKannWieder) != 0) {
            z.denkt = true;
            z.naechstes = lt + static_cast<double>(z.respawnMs);
        }
    } else {
        // ObjectDie: G_UseTargets, G_FreeEntity
        if (l.daten.hatTarget) { benutzt_.push_back(GeschuetzBenutzung{t, l.daten.nummer, 0}); }
        z.frei = true;
        l.freiMs = t;
    }
    merke(i, t);
}

}  // namespace bhed
