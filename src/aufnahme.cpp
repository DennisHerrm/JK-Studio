// aufnahme.cpp - Zeitleiste 2 Punkt 1: eine gesteuerte Figur -> Laufziele
// (siehe bhed/aufnahme.h).
#include "bhed/aufnahme.h"

#include <cmath>
#include <cstddef>

namespace bhed {
namespace {

constexpr float kStufe = 18.0F;   // STEPSIZE
constexpr float kStillstand = 16.0F;   // naeher am Start: nicht bewegt (halbe Breite einer Figur)

// Abstand eines Punktes von der Strecke a-b.
float abstand(const NavVec& p, const NavVec& a, const NavVec& b) {
    const float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float v[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    const float ll = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    float t = ll > 0.0F ? (v[0] * d[0] + v[1] * d[1] + v[2] * d[2]) / ll : 0.0F;
    t = t < 0.0F ? 0.0F : (t > 1.0F ? 1.0F : t);
    const float q[3] = {a[0] + d[0] * t - p[0], a[1] + d[1] * t - p[1], a[2] + d[2] * t - p[2]};
    return std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
}

// NAV::ClearPathToPoint: der Kasten der Figur mit angehobenem Boden (Stufen).
bool frei(const NavVec& a, const NavVec& b, const NavSpurFn& spur) {
    if (!spur) { return true; }
    const NavVec mins{-15.0F, -15.0F, -24.0F + kStufe};
    const NavVec maxs{15.0F, 15.0F, 40.0F};
    const NavSpur t = spur(a, mins, maxs, b, kNavKeineEntity, kNavMaskNpcSolid);
    return !t.startsolid && !t.allsolid && t.fraction >= 1.0F;
}

void teilen(const std::vector<AufnahmeProbe>& p, std::size_t a, std::size_t b, float tol, const NavSpurFn& spur,
            std::vector<bool>& behalten) {
    if (b <= a + 1) { return; }
    std::size_t weit = a;
    float max = -1.0F;
    for (std::size_t i = a + 1; i < b; ++i) {
        const float d = abstand(p[i].ort, p[a].ort, p[b].ort);
        if (d > max) {
            max = d;
            weit = i;
        }
    }
    if (max <= tol && frei(p[a].ort, p[b].ort, spur)) { return; }
    behalten[weit] = true;
    teilen(p, a, weit, tol, spur, behalten);
    teilen(p, weit, b, tol, spur, behalten);
}

}  // namespace

std::vector<std::size_t> aufnahmeVereinfachen(const std::vector<AufnahmeProbe>& p, float toleranz, const NavSpurFn& spur) {
    std::vector<std::size_t> aus;
    if (p.size() < 2) { return aus; }
    const float tol = toleranz > 1.0F ? toleranz : 1.0F;
    // Hat sich die Figur ueberhaupt bewegt?
    float weitest = 0.0F;
    for (const AufnahmeProbe& x : p) { weitest = std::fmax(weitest, abstand(x.ort, p[0].ort, p[0].ort)); }
    if (weitest < kStillstand) { return aus; }
    std::vector<bool> behalten(p.size(), false);
    behalten.back() = true;
    // Gangart: der Punkt, an dem sie wechselt, bleibt - dort steht im Skript
    // der neue set ( SET_WALKING / SET_RUNNING ).
    for (std::size_t i = 1; i < p.size(); ++i) {
        if (p[i].gehen != p[i - 1].gehen) { behalten[i - 1] = true; }
    }
    std::size_t a = 0;
    for (std::size_t i = 1; i < p.size(); ++i) {
        if (!behalten[i]) { continue; }
        teilen(p, a, i, tol, spur, behalten);
        a = i;
    }
    for (std::size_t i = 1; i < p.size(); ++i) {
        if (behalten[i]) { aus.push_back(i); }
    }
    return aus;
}

}  // namespace bhed
