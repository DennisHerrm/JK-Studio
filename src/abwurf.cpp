// abwurf.cpp - Pruefung 3: Waffe und Schwert, die eine sterbende Figur fallen
// laesst (bhed/abwurf.h). Quellen: OpenJK code/game/g_items.cpp (LaunchItem,
// Drop_Item, G_BounceItem, G_RunItem), wp_saber.cpp (WP_SaberLaunch,
// WP_SaberDrop, WP_RunSaber, WP_SaberImpact, WP_SaberFallSound, WP_SaberUpdate),
// g_missile.cpp (G_BounceMissile), NPC.cpp (pitch_roll_for_slope),
// bg_misc.cpp (EvaluateTrajectory); Movie Duels und codeJK2 dieselben Dateien.
#include "bhed/abwurf.h"

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

// Die Bahn eines Ortes oder Winkels (trajectory_t) - TR_STATIONARY, TR_LINEAR, TR_GRAVITY.
struct Bahn {
    enum class Art : std::uint8_t { Steht, Linear, Schwer } art = Art::Steht;
    double zeit = 0.0;   // trTime
    NavVec basis{};      // trBase
    NavVec delta{};      // trDelta
    // EvaluateTrajectory (bg_misc.cpp:285-340 OJ)
    [[nodiscard]] NavVec bei(double t, float g) const {
        const auto d = static_cast<float>((t - zeit) * 0.001);
        switch (art) {
            case Art::Steht: return basis;
            case Art::Linear: return {basis[0] + d * delta[0], basis[1] + d * delta[1], basis[2] + d * delta[2]};
            case Art::Schwer: return {basis[0] + d * delta[0], basis[1] + d * delta[1], basis[2] + d * delta[2] - 0.5F * g * d * d};
        }
        return basis;
    }
    // EvaluateTrajectoryDelta
    [[nodiscard]] NavVec deltaBei(double t, float g) const {
        if (art == Art::Steht) { return {0.0F, 0.0F, 0.0F}; }
        NavVec v = delta;
        if (art == Art::Schwer) { v[2] -= g * static_cast<float>((t - zeit) * 0.001); }
        return v;
    }
};

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

void merke(AbwurfBahn& b, double t, const NavVec& ort, const NavVec& winkel) { b.punkte.push_back({t, ort, winkel}); }

}  // namespace

namespace {

// misc_model_breakable_gravity_init: clipmask (MASK_SOLID | CONTENTS_BODY |
// CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP) und mass.
constexpr std::uint32_t kObjektMaske = kMaskSolid | kNavBody | kNavMonsterClip | kNavBotClip;

float objektMasse(const ObjektStart& s) {
    if (s.masse != 0.0F) { return s.masse; }
    const auto laenge = [](const NavVec& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    return laenge(s.maxs) + laenge(s.mins);
}

// Ein G_RunObject (g_object.cpp:77-263 OpenJK) zur Zeit t; false: nichts mehr
// zu tun (in Ruhe). EF_BOUNCE_HALF, apos bleibt stehen (kein Drehen: der
// think ist G_RunObject).
struct ObjektLauf {
    Bahn pos;
    NavVec ort{};
    NavVec winkel{};
    float masse = 0.0F;
};

void objektDenken(ObjektLauf& o, const ObjektStart& s, double t, const NavSpurFn& spur, AbwurfBahn* b) {
    if (o.pos.art == Bahn::Art::Steht) {
        o.pos.art = Bahn::Art::Schwer;
        o.pos.basis = o.ort;
        o.pos.zeit = t - kServerMs;   // previousTime
    }
    const NavVec alt = o.ort;
    const NavVec ziel = o.pos.bei(t, s.schwerkraft);
    if (ziel == o.ort) { return; }   // "error - didn't move at all!"
    NavSpur tr = spur ? spur(o.ort, s.mins, s.maxs, ziel, s.entity, kObjektMaske) : NavSpur{};
    if (!spur) { tr.endpos = ziel; }
    if (!tr.startsolid && !tr.allsolid && tr.fraction != 0.0F) {
        o.ort = tr.endpos;
    } else {
        tr.fraction = 0.0F;
    }
    if (tr.fraction == 1.0F) { return; }   // g_gravity > 0
    if (tr.fraction != 0.0F && o.ort != alt && b != nullptr) {
        // getroffen und bewegt: die Welt (takedamage 0) - kick_impact_silent mit
        // der Normalen; objectHitHeavy ab mass 100
        b->effekte.push_back({t, "melee/kick_impact_silent", tr.endpos, tr.normal});
        b->klaenge.push_back({t, o.masse > 100.0F ? "sound/movers/objects/objectHitHeavy.wav"
                                                  : "sound/movers/objects/objectHit.wav",
                              o.ort});
    }
    if (o.pos.art != Bahn::Art::Schwer) { return; }
    if (tr.normal[2] < 0.7F) {
        // EF_BOUNCE_HALF
        if (tr.fraction <= 0.0F) {
            o.pos.basis = tr.endpos;
            o.ort = tr.endpos;
            o.pos.delta = {0.0F, 0.0F, 0.0F};
            o.pos.zeit = t;
            return;
        }
        // G_BounceObject (g_object.cpp:36-70)
        const double hit = (t - kServerMs) + kServerMs * static_cast<double>(tr.fraction);
        const NavVec v = o.pos.deltaBei(hit, s.schwerkraft);
        const float dot = punkt(v, tr.normal);
        float faktor = 60.0F / o.masse;
        if (faktor > 1.0F) { faktor = 1.0F; }
        for (std::size_t k = 0; k < 3; ++k) { o.pos.delta[k] = (v[k] + -2.0F * dot * faktor * tr.normal[k]) * 0.5F; }
        if (tr.normal[2] > 0.7F && o.pos.delta[2] < 40.0F) {
            o.ort = tr.endpos;
            o.pos.basis = tr.endpos;
            o.pos.zeit = t;
            return;
        }
        o.ort = tr.endpos;
        o.pos.zeit = hit;
        o.pos.basis = o.ort;
        return;
    }
    // auf dem Boden: pitch_roll_for_slope(ent, normal), G_StopObjectMoving
    neigungFuerHang(tr.normal, o.winkel, false);
    o.pos.art = Bahn::Art::Steht;
    o.pos.basis = o.ort;
    o.pos.delta = {0.0F, 0.0F, 0.0F};
}

}  // namespace

ObjektStart objektAbsetzen(const ObjektStart& s, const NavSpurFn& spur) {
    ObjektStart r = s;
    if (spur) {
        const NavVec oben{s.ort[0], s.ort[1], s.ort[2] + 1.0F};
        const NavVec unten{s.ort[0], s.ort[1], -65536.0F};   // MIN_WORLD_COORD
        const NavSpur tr = spur(oben, s.mins, s.maxs, unten, s.entity, kNavMaskNpcSolid);
        if (!tr.allsolid && !tr.startsolid && tr.fraction < 1.0F) { r.ort = tr.endpos; }
    }
    ObjektLauf o;
    o.ort = r.ort;
    o.winkel = r.winkel;
    o.masse = objektMasse(r);
    objektDenken(o, r, kServerMs, spur, nullptr);
    r.ort = o.ort;
    r.winkel = o.winkel;
    return r;
}

AbwurfBahn wirfObjekt(const ObjektStart& s, const std::vector<double>& wuerfe, const NavVec& ziel,
                      const NavSpurFn& spur, double grenzeMs) {
    AbwurfBahn b;
    if (wuerfe.empty()) { return b; }
    ObjektLauf o;
    o.ort = s.ort;
    o.winkel = s.winkel;
    o.masse = objektMasse(s);
    std::size_t naechster = 0;
    const auto werfen = [&](double ms) {
        // misc_model_throw_at_target4
        NavVec dir{ziel[0] - o.ort[0], ziel[1] - o.ort[1], ziel[2] - o.ort[2]};
        float knockback = 200.0F;
        const float l = std::sqrt(punkt(dir, dir));
        if (l != 0.0F) {
            for (float& d : dir) { d /= l; }
        }
        knockback -= l;
        if (knockback < 100.0F) { knockback = 100.0F; }
        float masse = o.masse;
        if (masse < 50.0F) { masse = 50.0F; }
        constexpr float kRueckstoss = 1000.0F;   // g_knockback
        o.pos.art = Bahn::Art::Schwer;
        o.pos.zeit = ms;
        o.pos.basis = o.ort;
        NavVec kvel{};
        for (std::size_t k = 0; k < 3; ++k) { kvel[k] = dir[k] * (kRueckstoss * knockback / masse * 0.8F); }
        kvel[2] = dir[2] * kRueckstoss * knockback / masse * 1.5F;
        for (std::size_t k = 0; k < 3; ++k) { o.pos.delta[k] += kvel[k]; }
        if (o.pos.delta[2] < knockback) { o.pos.delta[2] = knockback; }
    };
    werfen(wuerfe[naechster++]);
    merke(b, wuerfe.front(), o.ort, o.winkel);
    // das naechste G_RunObject ein Bild spaeter; ein use dazwischen wirft neu
    for (double t = wuerfe.front() + kServerMs;; t += kServerMs) {
        while (naechster < wuerfe.size() && wuerfe[naechster] < t - 1e-6) { werfen(wuerfe[naechster++]); }
        objektDenken(o, s, t, spur, &b);
        merke(b, t, o.ort, o.winkel);
        const bool ruht = o.pos.art == Bahn::Art::Steht;
        if (ruht && naechster >= wuerfe.size()) {
            b.ruheMs = t;
            break;
        }
        if (t > wuerfe.back() + grenzeMs) { break; }
    }
    return b;
}

bool AbwurfBahn::bei(double ms, NavVec& ort, NavVec& winkel) const {
    if (punkte.empty() || ms < punkte.front().ms || (wegMs >= 0.0 && ms >= wegMs)) { return false; }
    for (std::size_t i = 1; i < punkte.size(); ++i) {
        if (ms < punkte[i].ms) {
            const Punkt& a = punkte[i - 1];
            const Punkt& c = punkte[i];
            const auto f = static_cast<float>((ms - a.ms) / (c.ms - a.ms));
            for (std::size_t k = 0; k < 3; ++k) {
                ort[k] = a.ort[k] + (c.ort[k] - a.ort[k]) * f;
                winkel[k] = a.winkel[k] + (c.winkel[k] - a.winkel[k]) * f;
            }
            return true;
        }
    }
    ort = punkte.back().ort;
    winkel = punkte.back().winkel;
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
    merke(b, s.ms, ort, winkel);
    // LaunchItem setzt die Entity erst; G_RunItem rechnet ab dem naechsten Bild
    for (double t = s.ms + kServerMs; t <= s.ms + grenzeMs; t += kServerMs) {
        const NavVec ziel = pos.bei(t, s.schwerkraft);
        if (apos.art != Bahn::Art::Steht) { winkel = apos.bei(t, s.schwerkraft); }
        // MASK_SOLID|CONTENTS_PLAYERCLIP, ignore = activator (die Figur)
        NavSpur tr = spur ? spur(ort, s.mins, s.maxs, ziel, s.entity, kMaskSolid | kNavPlayerClip) : NavSpur{};
        if (!spur) { tr.endpos = ziel; }
        ort = tr.endpos;
        if (tr.startsolid) { tr.fraction = 0.0F; }
        if (tr.fraction >= 1.0F) {
            merke(b, t, ort, winkel);
            continue;
        }
        if (tr.startsolid) {   // "if ( !tr.startsolid ) G_BounceItem" - bleibt haengen
            merke(b, t, ort, winkel);
            continue;
        }
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
            merke(b, t, ort, winkel);
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
        merke(b, t, ort, winkel);
    }
    return b;
}

AbwurfBahn wirfSchwert(const AbwurfStart& s, const NavSpurFn& spur, double grenzeMs) {
    AbwurfBahn b;
    EngineZufall zufall(s.saat);
    // WP_SaberLaunch: an der Hand, TR_LINEAR ohne Tempo; WP_SaberDrop:
    // TR_GRAVITY (trTime bleibt), EF_BOUNCE_HALF, apos TR_LINEAR ab jetzt mit
    // (Q_irand(-300, 300), apos.trDelta[1], Q_irand(-300, 300))
    Bahn pos;
    pos.art = Bahn::Art::Schwer;
    pos.zeit = s.ms;
    pos.basis = s.ort;
    pos.delta = s.tempo;
    Bahn apos;
    apos.art = Bahn::Art::Linear;
    apos.zeit = s.ms;
    apos.basis = s.winkel;
    const int dx = zufall.irand(-300, 300);
    float dy = s.drehTempo[1];
    const int dz = zufall.irand(-300, 300);
    if (dy == 0.0F) { dy = static_cast<float>(zufall.irand(-300, 300)); }
    apos.delta = {static_cast<float>(dx), dy, static_cast<float>(dz)};
    NavVec ort = s.ort;
    NavVec winkel = s.winkel;
    merke(b, s.ms, ort, winkel);
    const auto fallKlang = [&](double t) {
        // WP_SaberFallSound (wp_saber.cpp:634-680 OJ) mit Besitzer: fallSound der
        // .sab oder je nach Art
        const std::string datei = !s.fallKlang[0].empty() ? s.fallKlang[static_cast<std::size_t>(zufall.irand(0, 2))]
                                  : s.sithSchwert          ? "sound/weapons/sword/fall" + std::to_string(zufall.irand(1, 7)) + ".wav"
                                                           : "sound/weapons/saber/bounce" + std::to_string(zufall.irand(1, 3)) + ".wav";
        b.klaenge.push_back({t, datei, ort});
    };
    for (double t = s.ms + kServerMs; t <= s.ms + grenzeMs; t += kServerMs) {
        // WP_SaberUpdate dreimal im Denkbild (gleiches level.time)
        for (int lauf = 0; lauf < 3; ++lauf) {
            if (pos.art == Bahn::Art::Steht) {
                // "it fell on the ground": der Besitzer ist tot -> G_FreeEntity
                b.wegMs = t;
                merke(b, t, ort, winkel);
                return b;
            }
            const NavVec ziel = pos.bei(t, s.schwerkraft);
            winkel = apos.bei(t, s.schwerkraft);
            // saber->clipmask MASK_SOLID | CONTENTS_LIGHTSABER, ohne Klinge ohne
            // CONTENTS_LIGHTSABER; ignore = owner
            NavSpur tr = spur ? spur(ort, s.mins, s.maxs, ziel, s.entity, kMaskSolid) : NavSpur{};
            if (!spur) { tr.endpos = ziel; }
            ort = tr.endpos;
            if (tr.startsolid) { tr.fraction = 0.0F; }
            if (tr.fraction >= 1.0F) { continue; }
            // WP_SaberImpact -> G_BounceMissile (EF_BOUNCE_HALF)
            const double treff = (t - kServerMs) + kServerMs * static_cast<double>(tr.fraction);
            const NavVec v = pos.deltaBei(treff, s.schwerkraft);
            const float dot = punkt(v, tr.normal);
            NavVec neu{(v[0] - 2.0F * dot * tr.normal[0]) * 0.5F, (v[1] - 2.0F * dot * tr.normal[1]) * 0.5F,
                       (v[2] - 2.0F * dot * tr.normal[2]) * 0.5F};
            if (tr.normal[2] > 0.7F && neu[2] < 40.0F) {
                // G_SetOrigin(endpos): TR_STATIONARY; WP_SaberImpact: Klang,
                // Drehung aus, an den Hang, PITCH + SABER_PITCH_HACK
                pos.art = Bahn::Art::Steht;
                pos.basis = ort;
                fallKlang(t);
                neigungFuerHang(tr.normal, winkel, false);
                winkel[0] += 90.0F;
                apos.art = Bahn::Art::Linear;
                apos.basis = winkel;
                apos.delta = {0.0F, 0.0F, 0.0F};
                apos.zeit = t;
                b.ruheMs = t;
                continue;
            }
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
        merke(b, t, ort, winkel);
    }
    return b;
}

}  // namespace bhed
