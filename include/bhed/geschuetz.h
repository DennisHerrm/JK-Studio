// geschuetz.h - Pruefung 4b: Geschuetze mit Feind, ihre Geschosse und was die
// Vorschau dafuer vom Schadensmodell des Spiels braucht.
//
// Nachgebildet (je Regelwerk; OpenJK code/game, Movie Duels code/game, Jedi
// Outcast codeJK2/game):
//   misc_sentry_turret  SP_PAS, pas_use, pas_think, pas_find_enemies,
//                       pas_adjust_enemy, pas_fire (g_turret.cpp:1395-1866 OJ,
//                       1800-2300 MD, 1008-1480 JK2)
//   misc_turret         SP_misc_turret, finish_spawning_turret, turret_base_use,
//                       turret_base_think, turret_find_enemies, turret_aim,
//                       turret_turnoff, turret_head_think, turret_fire (OJ
//                       :47-1114, MD :57-1510 mit turret_fire_JKO, JK2 :40-725)
//   misc_turretG2       SP_misc_turretG2 (nur Movie Duels :1151-1283): die
//                       Spawnflags der MP-Karte, MP_RULES, LEAD, CANRESPAWN
//   TurretPain, turret_die, ObjectDie (g_combat.cpp:328), turretG2_respawn
//   die Geschosse       G_RunMissile, G_MissileImpact, G_MissileImpacted,
//                       G_BounceMissile (g_missile.cpp OJ), CreateMissile /
//                       WP_FireTurboLaserMissile (wp_emplaced_gun.cpp)
//   G_Damage            der Teil fuer ein Geschuetz (takedamage, health,
//                       FL_DMG_BY_HEAVY_WEAP_ONLY, die/pain) - und fuer Figuren
//                       damageModifier (Trefferort) und G_GetHitLocFromSurfName.
//
// Was die Simulation von der Welt wissen muss (Spuren, PVS, Inhalt, die Bolzen
// des Ghoul2-Modells), fragt sie ueber Rueckrufe; die Figuren kommen je Bild als
// Liste. G_Damage auf Figuren, Brush-Entities und misc_model_breakable macht der
// Rufer: die Simulation gibt ihm Auftraege (direkter Schaden, Radiusschaden).
//
// Zufall: Q_flrand der Engine (EngineZufall) mit einer festen Saat je Geschuetz
// - im Spiel teilen sich alle Aufrufe eine Folge, die nicht nachzurechnen ist.
#ifndef BHED_GESCHUETZ_H
#define BHED_GESCHUETZ_H

#include "bhed/todesanim.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed {

// --- Teams (team_t, teams.h) -------------------------------------------------
inline constexpr int kTeamFrei = 0;
inline constexpr int kTeamSpieler = 1;
inline constexpr int kTeamFeind = 2;
inline constexpr int kTeamNeutral = 3;
// "team" einer Karte und "playerTeam" einer .npc: GetIDForString(TeamTable, s)
// (NPC_stats.cpp:95-106 OJ/MD: "free", "player", "enemy", "neutral" und die
// TEAM_*-Namen; unbekannt -1). Jedi Outcast: TranslateTeamName (NPC_stats.cpp:
// 139-150 JK2: "player", "enemy", "neutral", sonst TEAM_FREE).
[[nodiscard]] int teamAusName(const std::string& name, TodRegelwerk r);
// SET_PLAYER_TEAM: GetIDForString(teamTable, s) (Q3_Interface.cpp:2192-2229 OJ):
// nur die TEAM_*-Namen, unbekannt -1.
[[nodiscard]] int teamAusSkript(const std::string& name);

// --- Trefferorte (hitLocation_t, g_local.h) ----------------------------------
enum TrefferOrt : int {
    kOrtKeiner = 0,
    kOrtFussR, kOrtFussL, kOrtBeinR, kOrtBeinL, kOrtHuefte,
    kOrtRueckenR, kOrtRueckenL, kOrtRuecken, kOrtBrustR, kOrtBrustL, kOrtBrust,
    kOrtArmR, kOrtArmL, kOrtHandR, kOrtHandL, kOrtKopf,
    kOrtAllg1, kOrtAllg2, kOrtAllg3, kOrtAllg4, kOrtAllg5, kOrtAllg6,
    kOrtAnzahl
};
// damageModifier[hitLoc] (g_combat.cpp:5284-5309 OJ; MD :7582 mit HL_HEAD 1.0;
// JK2 :4632 wie OJ).
[[nodiscard]] float trefferOrtFaktor(int ort, TodRegelwerk r);

// Was G_GetHitLocFromSurfName von einer Figur liest (g_combat.cpp:877-1290 OJ,
// :1204 ff. MD): Klasse, NPC_type (MD), die Bolzen der Knie, Haende und Fuesse
// (G_SetG2PlayerModelInfo) in der Welt und renderInfo.torsoPoint/torsoAngles.
struct TrefferOrtLage {
    std::string klasse;    // "CLASS_..." gross oder klein
    std::string npcType;
    bool knieL = false, knieR = false, handL = false, handR = false, fussL = false, fussR = false;
    float knieLOrt[3]{}, knieROrt[3]{}, handLOrt[3]{}, handROrt[3]{}, fussLOrt[3]{}, fussROrt[3]{};
    float rumpfOrt[3]{};
    float rumpfWinkel[3]{};
};
// Der Trefferort einer Flaeche (mod ist nie MOD_SABER: die Schwert-Zweige
// fehlen). Ohne Flaechenname kOrtKeiner.
[[nodiscard]] int trefferOrtAusFlaeche(const TrefferOrtLage& l, const std::string& flaeche, const float punkt[3],
                                       TodRegelwerk r);

// --- Engine-Mathematik (q_math.cpp, bg_misc.cpp) -----------------------------
[[nodiscard]] float winkelNorm360(float a) noexcept;           // AngleNormalize360
[[nodiscard]] float winkelAbzug(float a1, float a2) noexcept;  // AngleSubtract
void winkelVektoren(const float w[3], float vorn[3], float rechts[3], float oben[3]) noexcept;   // AngleVectors

// --- Geschuetze --------------------------------------------------------------
enum class GeschuetzArt : std::uint8_t { Wache, Turm, TurmG2 };   // misc_sentry_turret, misc_turret, misc_turretG2

// Mittel (meansOfDeath), soweit hier gebraucht.
enum class SchussMittel : std::uint8_t { Unbekannt, Energie, Platziert };   // MOD_UNKNOWN, MOD_ENERGY, MOD_EMPLACED
// dflags (g_local.h)
inline constexpr int kSchadenRadius = 0x00000001;          // DAMAGE_RADIUS
inline constexpr int kSchadenOhneRuestung = 0x00000002;    // DAMAGE_NO_ARMOR
inline constexpr int kSchadenOhneRueckstoss = 0x00000004;  // DAMAGE_NO_KNOCKBACK
inline constexpr int kSchadenTodesRueckstoss = 0x00000080; // DAMAGE_DEATH_KNOCKBACK
inline constexpr int kSchadenSchwereWaffe = 0x00000800;    // DAMAGE_HEAVY_WEAP_CLASS

// Entity-Nummern der Spuren und Auftraege: Index in MapData::entities (Brushes,
// Geschuetze, Fallen, misc_model_breakable), Figuren ab kSchussFigur + Index in
// Scene::actors.
inline constexpr int kSchussKeine = -1;   // ENTITYNUM_NONE
inline constexpr int kSchussWelt = -2;    // ENTITYNUM_WORLD
inline constexpr int kSchussFigur = 1 << 20;

// gi.trace, wie die Simulation es liest.
struct SchussSpur {
    bool allsolid = false;
    bool startsolid = false;
    float fraction = 1.0F;
    float endpos[3]{};
    float normale[3]{};
    int entity = kSchussKeine;
    std::uint32_t flaechenFlags = 0;   // surfaceFlags (SURF_NOIMPACT, SURF_FORCEFIELD)
    // G2_COLLIDE: die naechste Kollision am Netz (G2CollisionMap[0], nach Abstand):
    std::string flaeche;               // Name der Flaeche
    float g2Punkt[3]{};                // mCollisionPosition
};
inline constexpr std::uint32_t kFlaecheKraftfeld = 0x00020000U;   // SURF_FORCEFIELD (surfaceflags.h)
inline constexpr std::uint32_t kFlaecheOhneEinschlag = 0x00080000U;   // SURF_NOIMPACT

// Was an Modell und Entity eines Geschuetzes gerade gesetzt ist - fuer
// G2API_GetBoltMatrix.
struct GeschuetzKnochen {
    float winkel[3]{};   // currentAngles
    float ort[3]{};      // currentOrigin bzw. s.origin
    float nick = 0.0F;   // misc_turret: speed (Bone_body bzw. "pitch" beim Turbo)
    float pasGier = 0.0F;   // misc_sentry_turret: speed - s.angles[YAW] (bone_hinge)
    float pasNick = 0.0F;   // random (bone_barrel, bone_gback mit -random)
    double aufklappAb = -1.0;   // pas_think: G2API_SetBoneAnimIndex(0, 11, 0.8) ab
    // TurboLaser_SetBoneAnim(start, end) ab turboAb (BONE_ANIM_OVERRIDE_FREEZE|BLEND)
    int turboVon = 4;
    int turboBis = 5;
    double turboAb = -1.0;
};

// Eine Figur (client), wie ein Geschuetz sie sieht - zur Zeit des Bilds.
struct SchussZiel {
    int nummer = kSchussKeine;   // kSchussFigur + Index
    float ort[3]{};              // currentOrigin
    float lo[3]{};               // absmin
    float hi[3]{};               // absmax
    float auge[3]{};             // renderInfo.eyePoint
    float tempo[3]{};            // ps.velocity (LEAD)
    bool fahrzeug = false;       // m_pVehicle
    // rc610 F46 B4-2 (Movie Duels, misc_turretG2 mit mp_rules, g_turret.cpp:681-704 MD): der Pilot eines Fahrzeugs
    // (m_pVehicle->m_pPilot) mit seinem Team und FL_NOTARGET; bei einem Reiter, ob sein Fahrzeug ihn verbirgt
    // (s.m_iVehicleNum, m_pVehicleInfo->hideRider)
    bool hatPilot = false;
    int pilotTeam = kTeamFrei;
    bool pilotNotarget = false;
    bool reiterVerborgen = false;
    int team = kTeamFrei;        // client->playerTeam
    int health = 100;
    bool takedamage = true;
    bool notarget = false;       // FL_NOTARGET
};

struct GeschuetzDaten {
    GeschuetzArt art = GeschuetzArt::Wache;
    int nummer = 0;                // Index in MapData::entities
    float ursprung[3]{};           // "origin"
    float winkel[3]{};             // "angles" bzw. "angle"
    int spawnflags = 0;
    float radius = 0.0F;
    float wait = 0.0F;
    float random = 0.0F;
    float shotspeed = 0.0F;
    int damage = 0;                // "dmg"
    int health = 0;
    int count = 0;
    int splashDamage = 0;
    int splashRadius = 0;
    std::string team;
    int alliedTeam = 0;            // misc_turretG2
    int teamnodmg = 0;
    int customscale = 0;
    int painwait = 0;
    bool hatTarget = false;
    bool hatTarget2 = false;
    bool hatPaintarget = false;
    std::vector<double> uses;      // nach Zeit
};

// Ein Stand je think (fuer die Zeichnung): Entity-Winkel als Bahn (s.apos,
// TR_STATIONARY oder TR_LINEAR) und die Knochenwinkel.
struct GeschuetzStand {
    double ms = 0.0;
    float aposBasis[3]{};
    float aposDelta[3]{};
    double aposZeit = 0.0;
    bool aposLinear = false;
    float nick = 0.0F;
    float pasGier = 0.0F;
    float pasNick = 0.0F;
    bool feind = false;
    int bild = 0;   // s.frame (EF_SHADER_ANIM: 0 leuchtet, 1 schwarz)
    bool gedacht = false;   // ein think in diesem Bild (sonst use, Schaden, Tod)
    bool dreht = false;     // s.loopSound (turret_aim: move.wav bzw. turn.wav)
};
// G_Sound / G_SoundOnEnt am Geschuetz.
struct GeschuetzKlang {
    double ms = 0.0;
    const char* datei = "";
};
struct GeschuetzSchuss {
    double ms = 0.0;
    float ort[3]{};
    float richtung[3]{};
    const char* effekt = "";   // G_PlayEffect
};
// Ein Effekt der Simulation (Tod, Funken bei Schmerz).
struct GeschuetzEffekt {
    double ms = 0.0;
    float ort[3]{};
    float richtung[3]{0.0F, 0.0F, 1.0F};
    const char* effekt = "";
};

struct GeschuetzLauf {
    GeschuetzDaten daten;
    float ursprung[3]{};         // currentOrigin (UPSIDE_DOWN: -22)
    float winkel[3]{};           // s.angles nach finish_spawning_turret
    float skala = 1.0F;          // s.modelScale
    bool turbo = false;          // SPF_TURRETG2_TURBO (laser_cannon_model.glm)
    std::vector<GeschuetzStand> staende;
    std::vector<GeschuetzKlang> klaenge;
    std::vector<GeschuetzSchuss> schuesse;
    std::vector<GeschuetzEffekt> effekte;
    std::vector<std::pair<double, std::uint8_t>> aufklappen;   // PAS: Animation ab
    std::vector<std::pair<double, std::pair<int, int>>> turboAnims;
    double totMs = -1.0;         // turret_die
    double freiMs = -1.0;        // G_FreeEntity (ObjectDie)
    std::vector<double> wiederMs;   // turretG2_respawn
    std::vector<std::pair<double, int>> health;   // nach jedem Schaden
};

// Ein Geschoss (turret_proj / turbo_proj) und wie es endet. Jede Bahn gilt ab
// dem Serverbild abMs (G_BounceMissile setzt eine neue mit trTime = hitTime - 10).
struct GeschossBahn {
    double abMs = 0.0;
    double zeit = 0.0;           // trTime (Szenenzeit)
    float basis[3]{};            // trBase
    float delta[3]{};            // trDelta
};
struct Geschoss {
    std::size_t geschuetz = 0;   // Index in laeufe()
    int waffe = 0;               // 0 WP_BLASTER, 1 WP_TURRET, 2 WP_TIE_FIGHTER
    double abMs = 0.0;           // Abschuss
    std::vector<GeschossBahn> bahnen;
    double endeMs = -1.0;        // Einschlag oder Ende
    float ende[3]{};
    float normale[3]{0.0F, 0.0F, 1.0F};
    int getroffen = kSchussKeine;
    bool fleisch = false;        // EV_MISSILE_HIT (Client mit takedamage), sonst MISS
    bool einschlag = false;      // false: G_FreeEntity (Lebenszeit, SURF_NOIMPACT)
    // Laufzustand
    float cur[3]{};              // currentOrigin
    float mins[3]{};
    float maxs[3]{};
    int damage = 0;
    int dflags = 0;
    SchussMittel mittel = SchussMittel::Energie;
    int splashDamage = 0;
    int splashRadius = 0;
    int abprallZahl = 0;         // bounceCount
    double lebenBis = 0.0;       // nextthink (G_FreeEntity), level.time
    bool fliegt = true;
    int besitzer = kSchussKeine;        // owner (fuer die Spur; nach einem Abprall NULL)
    int besitzerAnfang = kSchussKeine;  // wer schoss (attacker)
};

// Was der Rufer tun soll: G_Damage(ziel, ...) oder G_RadiusDamage(ort, ...).
struct SchadenAuftrag {
    bool radius = false;
    double ms = 0.0;
    int ziel = kSchussKeine;     // direkt: wer
    int angreifer = kSchussKeine;
    int ignoriere = kSchussKeine;   // Radius: ignore
    int schaden = 0;
    float weite = 0.0F;          // Radius
    int dflags = 0;
    SchussMittel mittel = SchussMittel::Unbekannt;
    int trefferOrt = kOrtKeiner;
    float punkt[3]{};
    float richtung[3]{};
};
// G_UseTargets eines Geschuetzes (target bei turret_die/ObjectDie, target2 beim
// Finden, paintarget bei Schmerz).
struct GeschuetzBenutzung {
    double ms = 0.0;
    int geschuetz = 0;   // Index in MapData::entities
    int art = 0;         // 0 target, 1 target2, 2 paintarget
};

struct GeschuetzWelt {
    // gi.trace(start, mins, maxs, ende, passEntityNum, contentmask, eG2TraceType, traceLod)
    // zur Zeit ms. g2: G2_COLLIDE (Netz der Figuren, Name der ersten Flaeche).
    std::function<SchussSpur(double ms, const float start[3], const float mins[3], const float maxs[3], const float ende[3],
                             int ignoriere, bool g2)>
        spur;
    std::function<bool(const float a[3], const float b[3])> pvs;   // gi.inPVS
    std::function<std::uint32_t(const float p[3], int ignoriere)> inhalt;   // gi.pointcontents
    // G2API_GetBoltMatrix(ghoul2, 0, bolzen, ..., k.winkel, k.ort, ms, NULL, modelScale):
    // m = 3x4 (Spalten POSITIVE_X/Y/Z, vierte = ORIGIN). false: Modell oder Bolzen fehlen.
    std::function<bool(std::size_t geschuetz, double ms, const GeschuetzKnochen& k, const char* bolzen, float m[3][4])> bolzen;
    // Die Figur, die eine G2-Spur traf: Trefferort (G_GetHitLocFromSurfName).
    std::function<int(int ziel, double ms, const std::string& flaeche, const float punkt[3])> trefferOrt;
    // other->takedamage einer Entity, die weder Figur noch Geschuetz ist
    // (func_breakable, misc_model_breakable, misc_trip_mine).
    std::function<bool(int entity, double ms)> nimmtSchaden;
};

class GeschuetzSim {
public:
    // regel: Regelwerk; snapRunden: SnapVector mit fistp (Windows x86) statt (int).
    void setze(std::vector<GeschuetzDaten> geschuetze, TodRegelwerk regel, double spawnMs, bool snapRunden);
    // Ein Serverbild (G_RunFrame, 50 ms) zur Zeit t: erst die Geschuetze (nach
    // Nummer), dann die Geschosse. Auftraege fuer G_Damage/G_RadiusDamage hinten an.
    void bild(double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w, std::vector<SchadenAuftrag>& auftraege);
    // G_Damage auf ein Geschuetz (nummer = Index in MapData::entities). true: es
    // nimmt Schaden. Folgen (Radiusschaden von turret_die) hinten an.
    bool schaden(int nummer, double t, int schaden, int dflags, SchussMittel mittel, int angreifer, const float punkt[3],
                 std::vector<SchadenAuftrag>& folgen);
    // Ist nummer ein Geschuetz, das Schaden nimmt (takedamage), und wo steht sein Kasten?
    [[nodiscard]] bool nimmtSchaden(int nummer, float lo[3], float hi[3]) const;
    // Steht das Geschuetz (nicht freigegeben) - fuer die Spuren der anderen.
    [[nodiscard]] bool steht(int nummer, double ms, float lo[3], float hi[3]) const;
    [[nodiscard]] const std::vector<GeschuetzLauf>& laeufe() const { return laeufe_; }
    [[nodiscard]] const std::vector<Geschoss>& geschosse() const { return geschosse_; }
    [[nodiscard]] const std::vector<GeschuetzBenutzung>& benutzungen() const { return benutzt_; }

private:
    // Was die Engine je Geschuetz fuehrt (gentity_t); Zeiten in level.time.
    struct Zustand {
        EngineZufall zufall{1U};
        bool denkt = false;          // e_ThinkFunc gesetzt
        double naechstes = 0.0;      // nextthink
        int sf = 0;                  // spawnflags (pas_use / turret_base_use kippen Bit 0)
        bool aufgeklappt = false;    // PAS: damage
        int count = 0;
        float speed = 0.0F;
        float random = 0.0F;
        int feind = kSchussKeine;    // enemy
        double aimDebounce = 0.0;
        double painDebounce = 0.0;
        double attackDebounce = 0.0;
        double pushDebounce = 0.0;
        double bounceCount = 0.0;
        double paintargetBis = 0.0;  // disconnectDebounceTime (MD)
        bool altFire = false;
        bool dreht = false;          // s.loopSound
        int health = 0;
        int maxHealth = 0;
        bool takedamage = false;
        bool notarget = false;       // FL_NOTARGET (turret_base_think)
        bool nurSchwer = false;      // FL_DMG_BY_HEAVY_WEAP_ONLY
        bool tot = false;            // turret_die (e_DieFunc NULL)
        bool frei = false;           // G_FreeEntity
        int noDamageTeam = kTeamFeind;
        float radius = 0.0F;
        float wait = 0.0F;
        float masse = 0.0F;          // mass: shotspeed
        int damage = 0;
        int splashDamage = 0;
        int splashRadius = 0;
        int respawnMs = 0;           // misc_turretG2: count
        int painwait = 0;            // fx_time
        float mins[3]{};
        float maxs[3]{};
        // s.apos
        bool aposLinear = false;
        float aposBasis[3]{};
        float aposDelta[3]{};
        double aposZeit = 0.0;
        float aktWinkel[3]{};        // currentAngles
        int bild = 0;                // s.frame
        GeschuetzKnochen knochen;
        std::size_t naechsterUse = 0;
    };
    void denke(std::size_t i, double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w);
    void pasDenken(std::size_t i, double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w);
    void turmDenken(std::size_t i, double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w);
    void fliege(Geschoss& g, double t, const std::vector<SchussZiel>& ziele, const GeschuetzWelt& w,
                std::vector<SchadenAuftrag>& auftraege);
    void feuere(std::size_t i, double lt, const float start[3], const float richtung[3], const GeschuetzWelt& w);
    void stirb(std::size_t i, double lt, int angreifer, std::vector<SchadenAuftrag>& folgen);
    void merke(std::size_t i, double t, bool gedacht = false);
    void schnappe(float v[3]) const;
    [[nodiscard]] double levelZeit(double t) const;
    [[nodiscard]] double szenenZeit(double lt) const;

    std::vector<GeschuetzLauf> laeufe_;
    std::vector<Zustand> zustand_;
    std::vector<Geschoss> geschosse_;
    std::vector<GeschuetzBenutzung> benutzt_;
    TodRegelwerk regel_ = TodRegelwerk::OpenJK;
    bool snapRunden_ = true;
    double spawnMs_ = 0.0;
};

}  // namespace bhed
#endif
