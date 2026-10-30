// abwurf.cpp - Pruefung 3: Waffe und Schwert, die eine sterbende Figur fallen
// laesst (bhed/abwurf.h). Quellen: OpenJK code/game/g_items.cpp (LaunchItem,
// Drop_Item, G_BounceItem, G_RunItem), wp_saber.cpp (WP_SaberLaunch,
// WP_SaberDrop, WP_RunSaber, WP_SaberImpact, WP_SaberFallSound, WP_SaberUpdate),
// g_missile.cpp (G_BounceMissile), NPC.cpp (pitch_roll_for_slope),
// bg_misc.cpp (EvaluateTrajectory); Movie Duels und codeJK2 dieselben Dateien.
#include "bhed/abwurf.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace bhed {

namespace {

constexpr double kServerMs = 50.0;   // FRAMETIME
constexpr std::uint32_t kMaskSolid = kNavSolid | kNavTerrain;   // MASK_SOLID
constexpr float kGrad = 3.14159265358979323846F / 180.0F;

float punkt(const NavVec& a, const NavVec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// AngleVectors (q_math.c): vorwaerts und rechts
void winkelVektoren(const NavVec& w, NavVec* vorn, NavVec* rechts) {
    const float sy = std::sin(w[1] * kGrad);
    const float cy = std::cos(w[1] * kGrad);
    const float sp = std::sin(w[0] * kGrad);
    const float cp = std::cos(w[0] * kGrad);
    const float sr = std::sin(w[2] * kGrad);
    const float cr = std::cos(w[2] * kGrad);
    if (vorn != nullptr) { *vorn = {cp * cy, cp * sy, -sp}; }
    if (rechts != nullptr) { *rechts = {-1 * sr * sp * cy + -1 * cr * -sy, -1 * sr * sp * sy + -1 * cr * cy, -1 * sr * cp}; }
}

using Bahn = AbwurfTrajektorie;

// pitch_roll_for_slope (NPC.cpp:412-506 OJ) mit storeAngles.
void neigungFuerHang(const NavVec& hang, NavVec& winkel, bool nickBehalten) {
    const float alterNick = winkel[0];
    NavVec vorn{};
    NavVec rechts{};
    winkelVektoren(winkel, &vorn, &rechts);
    float neu[3]{};
    const float h[3] = {hang[0], hang[1], hang[2]};
    engineVectoangles(h, neu);
    float nick = neu[0] + 90.0F;
    if (nickBehalten) { nick += alterNick; }
    const NavVec flach{0.0F, neu[1], 0.0F};
    NavVec nvf{};
    winkelVektoren(flach, &nvf, nullptr);
    const float mod = punkt(nvf, rechts) < 0.0F ? -1.0F : 1.0F;
    const float dot = punkt(nvf, vorn);
    winkel[0] = dot * nick;
    winkel[2] = (1.0F - std::fabs(dot)) * nick * mod;
}

void merke(AbwurfBahn& b, double t, const Bahn& pos, const Bahn& apos) { b.punkte.push_back({t, pos, apos}); }

}  // namespace

NavVec AbwurfTrajektorie::bei(double t, float g) const {
    const auto d = static_cast<float>((t - zeit) * 0.001);
    switch (art) {
        case Art::Steht: return basis;
        case Art::Linear: return {basis[0] + d * delta[0], basis[1] + d * delta[1], basis[2] + d * delta[2]};
        case Art::Schwer: return {basis[0] + d * delta[0], basis[1] + d * delta[1], basis[2] + d * delta[2] - 0.5F * g * d * d};
    }
    return basis;
}

NavVec AbwurfTrajektorie::deltaBei(double t, float g) const {
    if (art == Art::Steht) { return {0.0F, 0.0F, 0.0F}; }
    NavVec v = delta;
    if (art == Art::Schwer) { v[2] -= g * static_cast<float>((t - zeit) * 0.001); }
    return v;
}

bool AbwurfBahn::bei(double ms, NavVec& ort, NavVec& winkel) const {
    if (punkte.empty() || ms < punkte.front().ms || (wegMs >= 0.0 && ms >= wegMs)) { return false; }
    std::size_t i = punkte.size() - 1U;
    while (i > 0U && punkte[i].ms > ms) { --i; }
    ort = punkte[i].pos.bei(ms, schwerkraft);
    winkel = punkte[i].apos.bei(ms, schwerkraft);
    return true;
}

AbwurfStart gegenstandAbwurf(const NavVec& ort, float gier, const float zufall[2], bool waffeDrehen) {
    AbwurfStart s;
    s.ort = ort;
    // Drop_Item: angles = s.apos.trBase, PITCH 0 ("always forward")
    NavVec vorn{};
    winkelVektoren({0.0F, gier, 0.0F}, &vorn, nullptr);
    s.tempo = {vorn[0] * 150.0F, vorn[1] * 150.0F, vorn[2] * 150.0F + 200.0F + zufall[0] * 50.0F};
    // LaunchItem: VectorSet( dropped->s.angles, 0, Q_flrand(-1, 1) * 180, 90 )
    if (waffeDrehen) { s.winkel = {0.0F, zufall[1] * 180.0F, 90.0F}; }
    return s;
}

AbwurfBahn wirfGegenstand(const AbwurfStart& s, const NavSpurFn& spur, double grenzeMs) {
    AbwurfBahn b;
    b.schwerkraft = s.schwerkraft;
    EngineZufall zufall(s.saat);
    const bool jk2 = s.regel == TodRegelwerk::JediOutcast;
    Bahn pos;
    pos.art = Bahn::Art::Schwer;   // LaunchItem: TR_GRAVITY, trTime = level.time
    pos.zeit = s.ms;
    pos.basis = s.ort;
    pos.delta = s.tempo;
    Bahn apos;   // G_SetAngles: TR_STATIONARY
    apos.basis = s.winkel;
    NavVec ort = s.ort;
    NavVec winkel = s.winkel;
    merke(b, s.ms, pos, apos);
    // LaunchItem setzt die Entity erst; G_RunItem rechnet ab dem naechsten Bild
    for (double t = s.ms + kServerMs; t <= s.ms + grenzeMs; t += kServerMs) {
        const NavVec ziel = pos.bei(t, s.schwerkraft);
        if (apos.art != Bahn::Art::Steht) { winkel = apos.bei(t, s.schwerkraft); }
        // MASK_SOLID|CONTENTS_PLAYERCLIP, ignore = activator (die Figur)
        NavSpur tr = spur ? spur(ort, s.mins, s.maxs, ziel, s.entity, kMaskSolid | kNavPlayerClip) : NavSpur{};
        if (!spur) { tr.endpos = ziel; }
        ort = tr.endpos;
        if (tr.startsolid) { tr.fraction = 0.0F; }
        // "if ( !tr.startsolid ) G_BounceItem" - sonst bleibt er haengen
        if (tr.fraction >= 1.0F || tr.startsolid) { continue; }
        // G_BounceItem: die Geschwindigkeit beim Aufprall, gespiegelt, mal
        // physicsBounce - ein abgeworfener Gegenstand hat 0 (G_Spawn nullt, nur
        // FinishSpawningItem setzt 0,5), also steht er danach still.
        const NavVec neu{0.0F, 0.0F, 0.0F};
        // OJ/MD: "qboolean droppedSaber = qtrue" - der Zweig fuer ein Schwert
        // gilt fuer JEDEN Gegenstand (der Vergleich danach setzt nur dasselbe):
        // WP_SaberFallSound( NULL, ent ) ohne NPC_type -> bounceN.wav
        if (!jk2) {
            b.klaenge.push_back({t, "sound/weapons/saber/bounce" + std::to_string(zufall.irand(1, 3)) + ".wav", ort});
        }
        if (tr.normal[2] > 0.0F && neu[2] < 40.0F) {
            // G_SetOrigin(trace->endpos): TR_STATIONARY
            if (!jk2) {
                // currentAngles[PITCH] = SABER_PITCH_HACK (90), ROLL 0, an den Hang
                winkel[0] = 90.0F;
                winkel[2] = 0.0F;
                neigungFuerHang(tr.normal, winkel, true);
            }
            pos.art = Bahn::Art::Steht;
            pos.basis = ort;
            apos.art = Bahn::Art::Steht;
            apos.basis = winkel;
            merke(b, t, pos, apos);
            b.ruheMs = t;
            return b;
        }
        if (!jk2) {
            apos.basis = winkel;
            apos.art = Bahn::Art::Linear;
            apos.zeit = t;
            const int x = zufall.irand(-300, 300);
            const int y = zufall.irand(-300, 300);
            const int z = zufall.irand(-300, 300);
            apos.delta = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
        }
        for (std::size_t k = 0; k < 3; ++k) { ort[k] += tr.normal[k]; }
        pos.basis = ort;
        pos.zeit = t;
        pos.delta = neu;
        merke(b, t, pos, apos);
    }
    return b;
}

AbwurfBahn wirfSchwert(const AbwurfStart& s, const NavSpurFn& spur, const SchwertUmfeld& umfeld, double grenzeMs) {
    AbwurfBahn b;
    b.schwerkraft = s.schwerkraft;
    EngineZufall zufall(s.saat);
    const bool jk2 = s.regel == TodRegelwerk::JediOutcast;
    // saberThrowDist (wp_saber.cpp:306-312 OJ, MD gleich mit FP_TableLevel; JK2
    // :231): die Stufen 4 und 5 (eine .npc erlaubt bis 5) lesen in OpenJK und
    // JK2 ueber das Ende der Tabelle - hier wie Movie Duels die Stufe 3.
    static constexpr float kWurfWeite[4] = {0.0F, 256.0F, 400.0F, 400.0F};
    float wurfWeite = kWurfWeite[std::clamp(s.wurfStufe, 0, 3)];   // ps.saberEntityDist
    bool zurueck = false;    // ps.saberEntityState == SES_RETURNING
    double fallMs = -1.0;    // saber->s.time (WP_SaberDrop mit totem Besitzer)
    // WP_SaberLaunch: an der Hand, TR_LINEAR ohne Tempo; apos TR_LINEAR ab
    // vectoangles(muzzleDir) mit dem Drall nach FP_SABERTHROW
    Bahn pos;
    pos.art = Bahn::Art::Linear;
    pos.zeit = s.ms;
    pos.basis = s.ort;
    Bahn apos;
    apos.art = Bahn::Art::Linear;
    apos.zeit = s.ms;
    apos.basis = s.winkel;
    apos.delta = {0.0F, s.drehTempo[1], 0.0F};
    NavVec ort = s.ort;
    // saber->currentAngles: nur WP_RunSaber wertet s.apos aus (und WP_SaberImpact
    // setzt sie) - in der Hand hat das Schwert keine, G_Spawn nullt sie
    NavVec winkel{};
    const auto fallKlang = [&](double t) {
        // WP_SaberFallSound (wp_saber.cpp:634-680 OJ) mit Besitzer: fallSound der
        // .sab oder je nach Art
        const std::string datei = !s.fallKlang[0].empty() ? s.fallKlang[static_cast<std::size_t>(zufall.irand(0, 2))]
                                  : s.sithSchwert          ? "sound/weapons/sword/fall" + std::to_string(zufall.irand(1, 7)) + ".wav"
                                                           : "sound/weapons/saber/bounce" + std::to_string(zufall.irand(1, 3)) + ".wav";
        b.klaenge.push_back({t, datei, ort});
    };
    // WP_SaberDrop (wp_saber.cpp:6922-6956 OJ, JK2 :4303): TR_GRAVITY (trTime,
    // trBase, trDelta bleiben), EF_BOUNCE_HALF, apos TR_LINEAR ab currentAngles
    // mit (Q_irand(-300, 300), apos.trDelta[1], Q_irand(-300, 300)) - VectorSet
    // war im Spiel ein Makro, die Zuege also in dieser Folge.
    const auto fallenLassen = [&](double t) {
        pos.art = Bahn::Art::Schwer;
        apos.basis = winkel;
        apos.art = Bahn::Art::Linear;
        apos.zeit = t;
        const int x = zufall.irand(-300, 300);
        const float y = apos.delta[1];
        const int z = zufall.irand(-300, 300);
        apos.delta = {static_cast<float>(x), y, static_cast<float>(z)};
        if (apos.delta[1] == 0.0F) { apos.delta[1] = static_cast<float>(zufall.irand(-300, 300)); }
        wurfWeite = 0.0F;
        zurueck = true;
        fallMs = t;
    };
    // WP_SaberLose: mit brennender Klinge "drop it instantly" (der Ausschaltklang
    // steht in der Todeskette), sonst fliegt es nur
    if (!s.klingeAus) { fallenLassen(s.ms); }
    merke(b, s.ms, pos, apos);
    // WP_RunSaber (wp_saber.cpp:6409-6573 OJ, MD :14392, JK2 :3923)
    const auto lauf = [&](double t, const NavVec& hand, const NavVec& auge) {
        const NavVec ziel = pos.bei(t, s.schwerkraft);
        winkel = apos.bei(t, s.schwerkraft);
        // saber->clipmask MASK_SOLID | CONTENTS_LIGHTSABER, ohne Klinge ohne
        // CONTENTS_LIGHTSABER; ignore = owner
        NavSpur tr = spur ? spur(ort, s.mins, s.maxs, ziel, s.entity, kMaskSolid) : NavSpur{};
        if (!spur) { tr.endpos = ziel; }
        ort = tr.endpos;
        if (tr.startsolid) { tr.fraction = 0.0F; }
        bool geaendert = false;
        if (tr.fraction < 1.0F && pos.art == Bahn::Art::Schwer) {
            // WP_SaberImpact -> G_BounceMissile (EF_BOUNCE_HALF): hitTime ist ein
            // int (level.previousTime + 50 * fraction)
            const auto treff = static_cast<double>(static_cast<int>(static_cast<float>(t - kServerMs) + static_cast<float>(kServerMs) * tr.fraction));
            const NavVec v = pos.deltaBei(treff, s.schwerkraft);
            const float dot = punkt(v, tr.normal);
            const NavVec neu{(v[0] - 2.0F * dot * tr.normal[0]) * 0.5F, (v[1] - 2.0F * dot * tr.normal[1]) * 0.5F,
                             (v[2] - 2.0F * dot * tr.normal[2]) * 0.5F};
            geaendert = true;
            if (tr.normal[2] > 0.7F && neu[2] < 40.0F) {
                // G_SetOrigin(endpos): TR_STATIONARY; WP_SaberImpact: Klang,
                // Drehung aus, an den Hang, PITCH + SABER_PITCH_HACK
                pos.art = Bahn::Art::Steht;
                pos.basis = ort;
                pos.delta = neu;
                fallKlang(t);
                neigungFuerHang(tr.normal, winkel, false);
                winkel[0] += 90.0F;
                apos.basis = winkel;
                apos.delta = {0.0F, 0.0F, 0.0F};
                b.ruheMs = t;
            } else {
                // "push the missile away from the impact plane a bit"
                for (std::size_t k = 0; k < 3; ++k) { ort[k] = tr.endpos[k] + tr.normal[k]; }
                pos.basis = ort;
                pos.delta = neu;
                pos.zeit = treff >= t ? t - 10.0 : treff - 10.0;
                fallKlang(t);
                apos.basis = winkel;
                apos.zeit = t;
                const int x = zufall.irand(-300, 300);
                const int y = zufall.irand(-300, 300);
                const int z = zufall.irand(-300, 300);
                apos.delta = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
            }
        }
        if (pos.art != Bahn::Art::Linear) { return geaendert; }
        // "home": fwdangles - 8 fuer einen NPC (der Spieler in der dritten Person - 5)
        NavVec blick = s.blick;
        blick[0] -= s.spieler ? 5.0F : 8.0F;
        if (s.wurfStufe > 1 || zurueck || (pos.delta[0] == 0.0F && pos.delta[1] == 0.0F && pos.delta[2] == 0.0F)) {
            float tempo = 500.0F;
            NavVec vorn{};
            winkelVektoren(blick, &vorn, nullptr);
            const NavVec& heim = wurfWeite < 100.0F ? hand : auge;
            NavVec ziel2{heim[0] + wurfWeite * vorn[0], heim[1] + wurfWeite * vorn[1], heim[2] + wurfWeite * vorn[2]};
            bool feind = false;
            float feindBreite = 0.0F;
            if (s.wurfStufe > 2 && !zurueck && umfeld.feind) {
                NavVec fo{};
                float fh = 0.0F;
                if (umfeld.feind(t, ort, hand, auge, fo, feindBreite, fh)) {
                    feind = true;
                    const NavVec hf{hand[0] - fo[0], hand[1] - fo[1], hand[2] - fo[2]};
                    wurfWeite = std::sqrt(punkt(hf, hf));
                    ziel2 = {fo[0], fo[1], fo[2] + fh / 2.0F};
                }
            }
            NavVec d{ziel2[0] - ort[0], ziel2[1] - ort[1], ziel2[2] - ort[2]};
            // VectorNormalize: Laenge 0 laesst den Vektor 0
            const float dist = std::sqrt(punkt(d, d));
            if (dist != 0.0F) {
                for (float& x : d) { x /= dist; }
            }
            if (s.wurfStufe > 2 && !zurueck && !feind) {
                if (dist < 200.0F) { tempo = 400.0F - dist * 2.0F; }
            } else if (!zurueck && dist < 50.0F) {
                tempo = dist * 2.0F + 30.0F;
                if (((feind && dist > feindBreite) || (!feind && dist > 24.0F)) && tempo < 120.0F) { tempo = 120.0F; }
            } else if (jk2 && zurueck) {
                // codeJK2 WP_RunSaber: "if returning, move faster"
                tempo = 800.0F;
                if (dist < 200.0F) { tempo -= 400.0F - dist * 2.0F; }
            }
            pos.delta = {d[0] * tempo, d[1] * tempo, d[2] * tempo};
        }
        pos.basis = ort;
        pos.zeit = t;
        return true;
    };
    for (double t = s.ms + kServerMs; t <= s.ms + grenzeMs; t += kServerMs) {
        // renderInfo der toten Figur in diesem Bild
        NavVec hand = s.ort;
        NavVec auge = s.ort;
        if (umfeld.haltung) { umfeld.haltung(t, hand, auge); }
        bool geaendert = false;
        // WP_SaberUpdate dreimal im Denkbild (gleiches level.time) -> WP_SaberThrow
        for (int durchgang = 0; durchgang < 3; ++durchgang) {
            if (zurueck) {
                // "see if we're close enough to pick it up": WP_SaberCatch faengt
                // nicht (health <= 0) - return
                const NavVec d{hand[0] - ort[0], hand[1] - ort[1], hand[2] - ort[2]};
                if (punkt(d, d) <= 256.0F) { continue; }
            }
            if (pos.art == Bahn::Art::Steht) {
                // "it fell on the ground": der Besitzer ist tot -> G_FreeEntity
                // (Jedi Outcast erst 5 s nach WP_SaberDrop: level.time > s.time + 5000)
                if (!jk2 || t > fallMs + 5000.0) {
                    b.wegMs = t;
                    return b;
                }
                continue;   // JK2: WP_SaberCatch (tot: nichts) - return
            }
            geaendert = lauf(t, hand, auge) || geaendert;
            // ps.weapon != WP_SABER, saberInFlight: "switched away while controlling
            // it, just drop the saber"
            if (pos.art == Bahn::Art::Linear) {
                fallenLassen(t);
                if (!s.ausKlang.empty()) { b.klaenge.push_back({t, s.ausKlang, ort}); }
                geaendert = true;
            }
        }
        if (geaendert) { merke(b, t, pos, apos); }
    }
    return b;
}

}  // namespace bhed
