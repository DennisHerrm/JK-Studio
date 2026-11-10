// modell/posiert.cpp - Bearbeiten in der Pose (siehe bhed/modell/posiert.h)
//
//   This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later (LICENSE).
#include "bhed/modell/posiert.h"

#include <cmath>
#include <cstring>

namespace bhed::modell {
namespace {

// Umkehrung ueber die Adjunkte; false bei Determinante 0 (wie Blender invert_m3_m3: "success = (det != 0.0f)").
bool umkehren3(const double a[3][3], double out[3][3]) {
    const double c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1];
    const double c01 = a[1][2] * a[2][0] - a[1][0] * a[2][2];
    const double c02 = a[1][0] * a[2][1] - a[1][1] * a[2][0];
    const double det = a[0][0] * c00 + a[0][1] * c01 + a[0][2] * c02;
    if (det == 0.0 || !std::isfinite(det)) { return false; }
    const double k = 1.0 / det;
    out[0][0] = c00 * k;
    out[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) * k;
    out[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * k;
    out[1][0] = c01 * k;
    out[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) * k;
    out[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) * k;
    out[2][0] = c02 * k;
    out[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) * k;
    out[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * k;
    return true;
}

// Affine Umkehrung einer 3x4-Matrix
bool umkehren34(const double a[3][4], double out[3][4]) {
    const double l[3][3] = {{a[0][0], a[0][1], a[0][2]}, {a[1][0], a[1][1], a[1][2]}, {a[2][0], a[2][1], a[2][2]}};
    double li[3][3];
    if (!umkehren3(l, li)) { return false; }
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { out[r][c] = li[r][c]; }
        out[r][3] = -(li[r][0] * a[0][3] + li[r][1] * a[1][3] + li[r][2] * a[2][3]);
    }
    return true;
}

bool gilt(const std::vector<BoneMatrix>& knochen, const GlmVertex& v, int k) {
    return v.weights[k] > 0.0F && v.bones[k] < knochen.size();   // wie haut() (verforme)
}

// inv * (p, 1) als float; false, wenn das keine Koordinate des Spiels ist
bool zurueckMit(const double inv[3][4], const double p[3], float v[3]) {
    double z[3];
    for (int r = 0; r < 3; ++r) {
        z[r] = inv[r][0] * p[0] + inv[r][1] * p[1] + inv[r][2] * p[2] + inv[r][3];
        if (!(std::fabs(z[r]) <= kWeltGrenze)) { return false; }   // auch NaN
    }
    for (int r = 0; r < 3; ++r) { v[r] = static_cast<float>(z[r]); }
    return true;
}

bool gleich(const BoneMatrix& a, const BoneMatrix& b) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (!(a.m[r][c] == b.m[r][c])) { return false; }
        }
    }
    return true;
}

}  // namespace

bool bindepose(const std::vector<GlaBone>& bones, const std::vector<BoneMatrix>& welt) {
    const std::size_t n = bones.size();
    if (welt.size() != n) { return false; }
    // die Welt der Grundstellung ueber dieselbe Kette wie GlaAnimation::worldMatricesLerp, lokale Matrix = Einheit
    // (X * Einheit ist X, Wert fuer Wert): Wurzel BasePose, sonst Welt(Eltern) * BasePoseInv(Eltern) * BasePose
    std::vector<BoneMatrix> ruhe(n);
    std::vector<char> fertig(n, 0);
    for (std::size_t start = 0; start < n; ++start) {
        // die Kette von `start` zur Wurzel, dann von oben nach unten (Eltern stehen in der Datei auch HINTER dem Kind)
        std::vector<std::size_t> kette;
        for (std::size_t b = start; fertig[b] == 0;) {
            kette.push_back(b);
            fertig[b] = 1;
            const int p = bones[b].parent;
            if (p < 0 || static_cast<std::size_t>(p) >= n) { break; }
            b = static_cast<std::size_t>(p);
        }
        for (std::size_t i = kette.size(); i-- > 0;) {
            const std::size_t b = kette[i];
            const int p = bones[b].parent;
            if (p < 0 || static_cast<std::size_t>(p) >= n) {
                ruhe[b] = bones[b].basePose;
            } else {
                const auto up = static_cast<std::size_t>(p);
                ruhe[b] = ruhe[up] * bones[up].basePoseInv * bones[b].basePose;
            }
            if (!gleich(ruhe[b], welt[b])) { return false; }
        }
    }
    return true;
}

void gezeigterOrt(const std::vector<BoneMatrix>& knochen, const GlmVertex& v, float out[3]) {
    if (knochen.empty()) {
        std::memcpy(out, v.xyz, sizeof(float) * 3U);
        return;
    }
    out[0] = out[1] = out[2] = 0.0F;
    for (int k = 0; k < 4; ++k) {
        if (!gilt(knochen, v, k)) { continue; }
        float p[3];
        knochen[v.bones[k]].transform(v.xyz, p);
        for (int i = 0; i < 3; ++i) { out[i] += p[i] * v.weights[k]; }
    }
}

Mischung mischung(const std::vector<BoneMatrix>& knochen, const GlmVertex& v) {
    Mischung m;
    int staerkster = -1;
    for (int k = 0; k < 4; ++k) {
        if (!gilt(knochen, v, k)) { continue; }
        const BoneMatrix& b = knochen[v.bones[k]];
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) { m.a[r][c] += static_cast<double>(b.m[r][c]) * static_cast<double>(v.weights[k]); }
        }
        if (staerkster < 0 || v.weights[k] > v.weights[staerkster]) { staerkster = k; }
    }
    m.umkehrbar = staerkster >= 0 && umkehren34(m.a, m.inv);
    if (staerkster >= 0) {
        double s[3][4];
        const BoneMatrix& b = knochen[v.bones[staerkster]];
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) { s[r][c] = b.m[r][c]; }
        }
        m.hatStark = umkehren34(s, m.stark);
    }
    // die Normale: der erste Knochen, auch mit Gewicht 0 (G2_SkinSurface; gpuskin.cpp packeEcken)
    const bool erster = v.bones[0] < knochen.size();
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { m.n[r][c] = erster ? static_cast<double>(knochen[v.bones[0]].m[r][c]) : (r == c ? 1.0 : 0.0); }
    }
    if (!umkehren3(m.n, m.nInv)) {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) { m.nInv[r][c] = r == c ? 1.0 : 0.0; }
        }
    }
    return m;
}

void ortGezeigt(const Mischung& m, const float v[3], double p[3]) {
    for (int r = 0; r < 3; ++r) {
        p[r] = m.a[r][0] * static_cast<double>(v[0]) + m.a[r][1] * static_cast<double>(v[1]) + m.a[r][2] * static_cast<double>(v[2]) + m.a[r][3];
    }
}

Rueckweg ortZurueck(const Mischung& m, const double p[3], float v[3]) {
    if (m.umkehrbar && zurueckMit(m.inv, p, v)) { return Rueckweg::Exakt; }
    if (m.hatStark && zurueckMit(m.stark, p, v)) { return Rueckweg::Staerkster; }
    return Rueckweg::Keiner;
}

void normaleGezeigt(const Mischung& m, const float normale[3], double out[3]) {
    for (int r = 0; r < 3; ++r) {
        out[r] = m.n[r][0] * static_cast<double>(normale[0]) + m.n[r][1] * static_cast<double>(normale[1]) + m.n[r][2] * static_cast<double>(normale[2]);
    }
}

void normaleZurueck(const Mischung& m, const double gezeigt[3], float out[3]) {
    double z[3];
    for (int r = 0; r < 3; ++r) { z[r] = m.nInv[r][0] * gezeigt[0] + m.nInv[r][1] * gezeigt[1] + m.nInv[r][2] * gezeigt[2]; }
    for (int r = 0; r < 3; ++r) { out[r] = static_cast<float>(z[r]); }
}

namespace wof {
namespace {

// Je Punkt die erste Ecke eines Vielecks (ihre Gewichte gelten dem Punkt)
std::vector<const Ecke*> eckeJePunkt(const PolyNetz& n) {
    std::vector<const Ecke*> e(n.punkte.size(), nullptr);
    for (const PFlaeche& f : n.flaechen) {
        if (f.weg) { continue; }
        for (const PEcke& c : f.ecken) {
            if (c.punkt >= 0 && static_cast<std::size_t>(c.punkt) < e.size() && e[static_cast<std::size_t>(c.punkt)] == nullptr) {
                e[static_cast<std::size_t>(c.punkt)] = &c.e;
            }
        }
    }
    return e;
}

GlmVertex alsVertex(const Ecke& e, const float ort[3]) {
    GlmVertex v;
    std::memcpy(v.xyz, ort, sizeof v.xyz);
    std::memcpy(v.bones, e.knochen, sizeof v.bones);
    std::memcpy(v.weights, e.gewicht, sizeof v.weights);
    return v;
}

}  // namespace

NetzPose inPose(PolyNetz& n, const std::vector<BoneMatrix>& knochen) {
    NetzPose p;
    if (knochen.empty()) { return p; }
    p.aktiv = true;
    p.punkte = n.punkte.size();
    p.bind.resize(p.punkte);
    p.gezeigt.resize(p.punkte);
    const std::vector<const Ecke*> e = eckeJePunkt(n);
    for (std::size_t i = 0; i < p.punkte; ++i) {
        float* o = n.punkte[i].ort;
        std::memcpy(p.bind[i].data(), o, sizeof(float) * 3U);
        if (e[i] != nullptr) { gezeigterOrt(knochen, alsVertex(*e[i], o), o); }
        std::memcpy(p.gezeigt[i].data(), o, sizeof(float) * 3U);
    }
    return p;
}

std::vector<char> normalenPunkte(const PolyNetz& n) {
    std::vector<char> neu(n.punkte.size(), 0);
    for (const PFlaeche& f : n.flaechen) {
        if (f.weg || !f.neu) { continue; }
        for (const PEcke& c : f.ecken) {
            if (c.punkt >= 0 && static_cast<std::size_t>(c.punkt) < neu.size()) { neu[static_cast<std::size_t>(c.punkt)] = 1; }
        }
    }
    return neu;
}

int ausPose(PolyNetz& n, const NetzPose& p, const std::vector<BoneMatrix>& knochen, const std::vector<char>& normalenNeu) {
    if (!p.aktiv) { return 0; }
    int nichtUmkehrbar = 0;
    const std::vector<const Ecke*> e = eckeJePunkt(n);
    for (std::size_t i = 0; i < n.punkte.size(); ++i) {
        float* o = n.punkte[i].ort;
        const bool alt = i < p.punkte;
        if (alt && std::memcmp(o, p.gezeigt[i].data(), sizeof(float) * 3U) == 0) {
            std::memcpy(o, p.bind[i].data(), sizeof(float) * 3U);   // unveraendert: bitgleich wie vorher
            continue;
        }
        Rueckweg weg = Rueckweg::Keiner;
        if (e[i] != nullptr) {
            const double q[3] = {static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2])};
            weg = ortZurueck(mischung(knochen, alsVertex(*e[i], o)), q, o);
        }
        if (weg == Rueckweg::Keiner && alt) { std::memcpy(o, p.bind[i].data(), sizeof(float) * 3U); }
        // ein Punkt ohne Vieleck faellt beim Verdichten weg: er zaehlt nicht
        if (weg != Rueckweg::Exakt && e[i] != nullptr) { ++nichtUmkehrbar; }
    }
    // die Normalen, die im Gezeigten gerechnet wurden: von normalenNeu des Aufrufers (`normalenNeu`) und an neuen Punkten
    // (ein Werkzeug, das seine Normalen selbst rechnet - grundkoerper -, rechnete sie ebenfalls im Gezeigten)
    for (PFlaeche& f : n.flaechen) {
        if (f.weg) { continue; }
        for (PEcke& c : f.ecken) {
            if (c.punkt < 0) { continue; }
            const auto punkt = static_cast<std::size_t>(c.punkt);
            if (punkt < p.punkte && (punkt >= normalenNeu.size() || normalenNeu[punkt] == 0)) { continue; }
            const double gezeigt[3] = {static_cast<double>(c.e.normale[0]), static_cast<double>(c.e.normale[1]), static_cast<double>(c.e.normale[2])};
            float z[3];
            normaleZurueck(mischung(knochen, alsVertex(c.e, c.e.ort)), gezeigt, z);
            const float l = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
            if (!(l > 0.0F)) { continue; }
            for (int r = 0; r < 3; ++r) { c.e.normale[r] = z[r] / l; }
        }
    }
    return nichtUmkehrbar;
}

}  // namespace wof
}  // namespace bhed::modell
