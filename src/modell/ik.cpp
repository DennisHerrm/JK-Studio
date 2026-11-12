// modell/ik.cpp - IK/FK, Fussrolle, Einrasten, Space Switching (siehe bhed/modell/ik.h)
//
//   This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later (LICENSE).
//   Verhalten nach Blender (blender/blender 67e5a5f8, IK_QJacobianSolver.cpp ConstrainPoleVector) und Rigify
//   (blender/blender-addons b42d6862, rigify/rigs/limbs/leg.py, rigify/rig_ui_template.py), beide GPL-2.0-or-later;
//   kein Code uebernommen, nur die Regeln.
#include "bhed/modell/ik.h"

#include "bhed/modell/animkurven.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace bhed::modell {
namespace {

constexpr double kKlein = 1e-9;

struct V3 {
    double x = 0.0, y = 0.0, z = 0.0;
};
V3 v3(const float p[3]) { return V3{p[0], p[1], p[2]}; }
V3 operator+(V3 a, V3 b) { return V3{a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return V3{a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, double s) { return V3{a.x * s, a.y * s, a.z * s}; }
double punkt(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 kreuz(V3 a, V3 b) { return V3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double laenge(V3 a) { return std::sqrt(punkt(a, a)); }
V3 norm(V3 a) {
    const double l = laenge(a);
    return l > kKlein ? a * (1.0 / l) : V3{};
}
void schreib(V3 a, float p[3]) {
    p[0] = static_cast<float>(a.x);
    p[1] = static_cast<float>(a.y);
    p[2] = static_cast<float>(a.z);
}
V3 ortVon(const BoneMatrix& m) { return V3{m.m[0][3], m.m[1][3], m.m[2][3]}; }

// Eine reine Drehung (Spalten 0..2) aus drei Zeilen-Basen: R = Neu * Alt^T (Basen als Spalten)
BoneMatrix basisDrehung(const V3 alt[3], const V3 neu[3]) {
    BoneMatrix r = BoneMatrix::identity();
    const auto k = [](V3 v, int i) { return i == 0 ? v.x : (i == 1 ? v.y : v.z); };
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int b = 0; b < 3; ++b) { s += k(neu[b], i) * k(alt[b], j); }
            r.m[i][j] = static_cast<float>(s);
        }
    }
    return r;
}

// Die kleinste Drehung, die Richtung u nach v bringt
BoneMatrix drehungVonNach(V3 u, V3 v) {
    u = norm(u);
    v = norm(v);
    const V3 a = kreuz(u, v);
    const double s = laenge(a);
    const double c = punkt(u, v);
    if (s < 1e-12) {
        if (c > 0.0) { return BoneMatrix::identity(); }
        // entgegengesetzt: 180 Grad um eine Senkrechte
        V3 p = std::fabs(u.x) < 0.9 ? kreuz(u, V3{1, 0, 0}) : kreuz(u, V3{0, 1, 0});
        float ax[3];
        schreib(norm(p), ax);
        return drehungUm(ax, 180.0F);
    }
    float ax[3];
    schreib(a * (1.0 / s), ax);
    return drehungUm(ax, static_cast<float>(std::atan2(s, c) * 180.0 / 3.14159265358979323846));
}

BoneMatrix verschiebung(V3 d) {
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][3] = static_cast<float>(d.x);
    m.m[1][3] = static_cast<float>(d.y);
    m.m[2][3] = static_cast<float>(d.z);
    return m;
}

BoneMatrix umPunkt(const BoneMatrix& r, V3 p) {
    BoneMatrix rr = r;
    rr.m[0][3] = rr.m[1][3] = rr.m[2][3] = 0.0F;
    return verschiebung(p) * rr * verschiebung(p * -1.0);
}

// REST N14: nur der Ort genullt, Mass und Scherung bleiben (anders als nurDrehungStarr in retarget.cpp, bis REST beide nurDrehung)
BoneMatrix ohneOrt(const BoneMatrix& m) {
    BoneMatrix r = m;
    r.m[0][3] = r.m[1][3] = r.m[2][3] = 0.0F;
    return r;
}

BoneMatrix transponiert(const BoneMatrix& m) {
    BoneMatrix r = BoneMatrix::identity();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { r.m[i][j] = m.m[j][i]; }
    }
    return r;
}

// Der Zustand einer Auswertung: Welt und M dicht je Knochen, Kinderlisten, geaenderte Knochen
struct Zustand {
    const GlaAnimation* g = nullptr;
    std::vector<BoneMatrix>* welt = nullptr;
    std::vector<BoneMatrix> m;
    std::vector<char> hatM;
    std::vector<char> geaendert;
    std::vector<std::vector<int>> kinder;
};

void zustandAnlegen(const GlaAnimation& g, std::vector<BoneMatrix>& welt, const std::vector<std::pair<int, BoneMatrix>>& mEff, Zustand& z) {
    const std::size_t n = g.bones.size();
    z.g = &g;
    z.welt = &welt;
    z.m.assign(n, BoneMatrix::identity());
    z.hatM.assign(n, 0);
    z.geaendert.assign(n, 0);
    z.kinder.assign(n, {});
    for (std::size_t b = 0; b < n; ++b) {
        const int p = g.bones[b].parent;
        if (p >= 0 && static_cast<std::size_t>(p) < n) { z.kinder[static_cast<std::size_t>(p)].push_back(static_cast<int>(b)); }
    }
    for (const auto& [k, mm] : mEff) {
        if (k >= 0 && static_cast<std::size_t>(k) < n) {
            z.m[static_cast<std::size_t>(k)] = mm;
            z.hatM[static_cast<std::size_t>(k)] = 1;
        }
    }
}

// X'(b) = U X(b): M_neu = M * X^-1 U X (wie weltDrehungAuf in rig.cpp), der Teilbaum erbt X'(d) = U X(d)
void anwenden(Zustand& z, int b, const BoneMatrix& u) {
    std::vector<BoneMatrix>& w = *z.welt;
    if (b < 0 || static_cast<std::size_t>(b) >= w.size()) { return; }
    const BoneMatrix& x = w[static_cast<std::size_t>(b)];
    z.m[static_cast<std::size_t>(b)] = z.m[static_cast<std::size_t>(b)] * (starrInvers(x) * u * x);
    z.hatM[static_cast<std::size_t>(b)] = 1;
    z.geaendert[static_cast<std::size_t>(b)] = 1;
    std::vector<int> stapel{b};
    while (!stapel.empty()) {
        const int d = stapel.back();
        stapel.pop_back();
        w[static_cast<std::size_t>(d)] = u * w[static_cast<std::size_t>(d)];
        for (const int c : z.kinder[static_cast<std::size_t>(d)]) { stapel.push_back(c); }
    }
}

// Ein Glied wie rigGriffDrehen: der Knochen, die mitgedrehten, der Drehknochen mit seinem Anteil der Drehung um die Achse
// des Segments (Swing-Twist), alles aus der Welt vor dem Schritt.
void gliedAnwenden(Zustand& z, const IkGlied& gl, const BoneMatrix& u) {
    std::vector<BoneMatrix>& w = *z.welt;
    const auto gueltig = [&](int k) { return k >= 0 && static_cast<std::size_t>(k) < w.size(); };
    if (!gueltig(gl.knochen)) { return; }
    BoneMatrix twistU = BoneMatrix::identity();
    bool twist = false;
    if (gueltig(gl.twist) && gueltig(gl.twistVon)) {
        const V3 a = ortVon(w[static_cast<std::size_t>(gl.twistVon)]);
        const V3 e = ortVon(w[static_cast<std::size_t>(gl.knochen)]);
        const V3 achse = norm(e - a);
        if (laenge(e - a) > 1e-5) {
            float q[4];
            quatAusMatrix(u, q);
            const double proj = q[1] * achse.x + q[2] * achse.y + q[3] * achse.z;
            const double grad = 2.0 * std::atan2(proj, static_cast<double>(q[0])) * 180.0 / 3.14159265358979323846;
            float ax[3];
            schreib(achse, ax);
            twistU = umPunkt(drehungUm(ax, static_cast<float>(grad) * gl.twistAnteil), ortVon(w[static_cast<std::size_t>(gl.twist)]));
            twist = true;
        }
    }
    anwenden(z, gl.knochen, u);
    for (const int c : gl.mit) { anwenden(z, c, u); }
    if (twist) { anwenden(z, gl.twist, twistU); }
}

// Die Ruecknahme in die Liste der M: nur geaenderte Knochen, die anderen bleiben Bit fuer Bit
std::vector<int> zurueck(const Zustand& z, std::vector<std::pair<int, BoneMatrix>>& mEff) {
    std::vector<int> geaendert;
    for (std::size_t b = 0; b < z.geaendert.size(); ++b) {
        if (z.geaendert[b] == 0) { continue; }
        geaendert.push_back(static_cast<int>(b));
        bool da = false;
        for (auto& [k, mm] : mEff) {
            if (k == static_cast<int>(b)) {
                mm = z.m[b];
                da = true;
                break;
            }
        }
        if (!da) { mEff.emplace_back(static_cast<int>(b), z.m[b]); }
    }
    return geaendert;
}

void slerp4(const float a0[4], const float b0[4], float t, float out[4]) {
    double d = 0.0;
    for (int k = 0; k < 4; ++k) { d += static_cast<double>(a0[k]) * b0[k]; }
    float b[4] = {b0[0], b0[1], b0[2], b0[3]};
    if (d < 0.0) {
        for (float& v : b) { v = -v; }
        d = -d;
    }
    double wa = 1.0 - t;
    double wb = t;
    if (d < 0.9995) {
        const double th = std::acos(std::min(d, 1.0));
        wa = std::sin((1.0 - t) * th) / std::sin(th);
        wb = std::sin(t * th) / std::sin(th);
    }
    for (int k = 0; k < 4; ++k) { out[k] = static_cast<float>(wa * a0[k] + wb * b[k]); }
}

// Das Ziel im Raum am Bild (vor der Umrechnung in die Welt) und die Werte
struct Lokal {
    BoneMatrix ziel = BoneMatrix::identity();
    IkWert w;
    bool da = false;
};

Lokal lokalAm(const IkSpur& sp, float bild) {
    Lokal l;
    const std::size_t n = sp.ziel.size();
    if (n == 0 || sp.wert.size() != n) { return l; }
    l.da = true;
    // der letzte Schluessel mit Bild <= bild (sonst der erste)
    const auto it = std::upper_bound(sp.ziel.begin(), sp.ziel.end(), bild, [](float b, const RigSchluessel& k) { return b < static_cast<float>(k.bild); });
    const std::size_t i = it == sp.ziel.begin() ? 0U : static_cast<std::size_t>(it - sp.ziel.begin()) - 1U;
    l.w = sp.wert[i];
    if (bild <= static_cast<float>(sp.ziel.front().bild) || i + 1 >= n || sp.wert[i].raum != sp.wert[i + 1].raum) {
        // vor dem ersten, nach dem letzten oder bis zum Raumwechsel: der Schluessel haelt
        l.ziel = schluesselMatrix(sp.ziel[bild <= static_cast<float>(sp.ziel.front().bild) ? 0 : i]);
        if (bild <= static_cast<float>(sp.ziel.front().bild)) { l.w = sp.wert.front(); }
        return l;
    }
    (void)schluesselWert(sp.ziel, bild, l.ziel);
    const float t = std::clamp((bild - static_cast<float>(sp.ziel[i].bild)) / static_cast<float>(std::max(sp.ziel[i + 1].bild - sp.ziel[i].bild, 1)), 0.0F, 1.0F);
    const IkWert& a = sp.wert[i];
    const IkWert& b = sp.wert[i + 1];
    for (int k = 0; k < 3; ++k) { l.w.pol[k] = a.pol[k] + (b.pol[k] - a.pol[k]) * t; }
    l.w.rolle = a.rolle + (b.rolle - a.rolle) * t;
    l.w.spitze = a.spitze + (b.spitze - a.spitze) * t;
    slerp4(a.zehen, b.zehen, t, l.w.zehen);
    return l;
}

const BoneMatrix& raumWelt(const std::vector<BoneMatrix>& welt, int raum, BoneMatrix& puffer) {
    if (raum < 0 || static_cast<std::size_t>(raum) >= welt.size()) {
        puffer = BoneMatrix::identity();
        return puffer;
    }
    puffer = starr(welt[static_cast<std::size_t>(raum)]);
    return puffer;
}

// Die Knochen, die eine Spur bewegen kann (Teilbaeume ihrer Glieder)
std::vector<char> spurKnochen(const Zustand& z, const IkSpur& sp) {
    std::vector<char> in(z.kinder.size(), 0);
    std::vector<int> stapel;
    for (const IkGlied* gl : {&sp.oben, &sp.unten, &sp.ende, &sp.zehen}) {
        if (gl->knochen >= 0) { stapel.push_back(gl->knochen); }
        for (const int c : gl->mit) { stapel.push_back(c); }
        if (gl->twist >= 0) { stapel.push_back(gl->twist); }
    }
    while (!stapel.empty()) {
        const int d = stapel.back();
        stapel.pop_back();
        if (d < 0 || static_cast<std::size_t>(d) >= in.size() || in[static_cast<std::size_t>(d)] != 0) { continue; }
        in[static_cast<std::size_t>(d)] = 1;
        for (const int c : z.kinder[static_cast<std::size_t>(d)]) { stapel.push_back(c); }
    }
    return in;
}

// Die volle IK einer Spur (Anteil 1) auf den Zustand
void loesen(Zustand& z, const IkSpur& sp, const IkZiel& ziel) {
    std::vector<BoneMatrix>& w = *z.welt;
    const auto gueltig = [&](int k) { return k >= 0 && static_cast<std::size_t>(k) < w.size(); };
    if (!gueltig(sp.oben.knochen) || !gueltig(sp.unten.knochen) || !gueltig(sp.ende.knochen)) { return; }
    BoneMatrix g = ziel.ziel;
    FussRolle fr;
    const bool rolle = !ikArm(sp.art) && ziel.rolle != 0.0F;
    // die Zehen bei der Rolle: ihre Drehung zum ungerollten Fuss steht im Schluessel (aus der FK-Pose beim Einrasten) - so
    // aendern die FK-Schluessel, die das Ergebnis traegt, sie nicht (wie der eigene IK-Zehenregler von Rigify)
    const bool zehen = rolle && gueltig(sp.zehen.knochen);
    const BoneMatrix gOhne = g;
    if (rolle) {
        const BoneMatrix& fx = w[static_cast<std::size_t>(sp.ende.knochen)];
        fr = fussRolle(*z.g, sp, g, ziel.rolle, ziel.spitze, starrInvers(starr(fx)) * fx);
        g = fr.fuss * g;
    }
    float a[3];
    float b[3];
    float c[3];
    float zielOrt[3] = {g.m[0][3], g.m[1][3], g.m[2][3]};
    schreib(ortVon(w[static_cast<std::size_t>(sp.oben.knochen)]), a);
    schreib(ortVon(w[static_cast<std::size_t>(sp.unten.knochen)]), b);
    schreib(ortVon(w[static_cast<std::size_t>(sp.ende.knochen)]), c);
    float bn[3];
    float cn[3];
    (void)zweiGliedIk(a, b, c, zielOrt, ziel.pol, bn, cn);
    // oberes Glied: Basis (Glied, Beugerichtung des unteren) alt -> neu
    const V3 A = v3(a);
    const V3 B = v3(b);
    const V3 C = v3(c);
    const V3 Bn = v3(bn);
    const V3 Cn = v3(cn);
    const V3 e1a = norm(B - A);
    const V3 e1n = norm(Bn - A);
    const V3 va = (C - B) - e1a * punkt(C - B, e1a);
    const V3 vn = (Cn - Bn) - e1n * punkt(Cn - Bn, e1n);
    const double l2 = laenge(C - B);
    BoneMatrix r1;
    if (laenge(va) > 1e-4 * std::max(l2, 1.0) && laenge(vn) > 1e-4 * std::max(l2, 1.0)) {
        const V3 e2a = norm(va);
        const V3 e2n = norm(vn);
        const V3 alt[3] = {e1a, e2a, kreuz(e1a, e2a)};
        const V3 neu[3] = {e1n, e2n, kreuz(e1n, e2n)};
        r1 = basisDrehung(alt, neu);
    } else {
        r1 = drehungVonNach(e1a, e1n);
    }
    gliedAnwenden(z, sp.oben, umPunkt(r1, A));
    // unteres Glied: Scharnier am Gelenk zum neuen Ende
    const V3 B1 = ortVon(w[static_cast<std::size_t>(sp.unten.knochen)]);
    const V3 C1 = ortVon(w[static_cast<std::size_t>(sp.ende.knochen)]);
    gliedAnwenden(z, sp.unten, umPunkt(drehungVonNach(C1 - B1, Cn - B1), B1));
    // Endknochen: die Drehung des Ziels
    const BoneMatrix jetzt = starr(w[static_cast<std::size_t>(sp.ende.knochen)]);
    gliedAnwenden(z, sp.ende, umPunkt(ohneOrt(g) * transponiert(ohneOrt(jetzt)), ortVon(jetzt)));
    // Zehen: liegen bleiben bei der Rolle um den Ballen (R_Ferse R_Spitze auf die Lage am ungerollten Ziel)
    if (zehen) {
        const BoneMatrix soll = fr.zehen * gOhne * matrixAusQuat(ziel.zehen);
        const BoneMatrix ist = starr(w[static_cast<std::size_t>(sp.zehen.knochen)]);
        gliedAnwenden(z, sp.zehen, umPunkt(ohneOrt(soll) * transponiert(ohneOrt(ist)), ortVon(ist)));
    }
}

// Die naechste Drehung zu einer 3x3 (polare Zerlegung, Newton-Folge R <- (R + R^-T) / 2 in double bis zur Ruhe); der Ort
// bleibt
BoneMatrix naechsteDrehung(const BoneMatrix& m) {
    double r[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { r[i][j] = m.m[i][j]; }
    }
    for (int schritt = 0; schritt < 30; ++schritt) {
        const double c00 = r[1][1] * r[2][2] - r[1][2] * r[2][1];
        const double c01 = r[1][2] * r[2][0] - r[1][0] * r[2][2];
        const double c02 = r[1][0] * r[2][1] - r[1][1] * r[2][0];
        const double det = r[0][0] * c00 + r[0][1] * c01 + r[0][2] * c02;
        if (std::fabs(det) < 1e-12) { return starr(m); }
        // R^-T = Kofaktoren / det
        double k[3][3];
        k[0][0] = c00;
        k[0][1] = c01;
        k[0][2] = c02;
        k[1][0] = r[0][2] * r[2][1] - r[0][1] * r[2][2];
        k[1][1] = r[0][0] * r[2][2] - r[0][2] * r[2][0];
        k[1][2] = r[0][1] * r[2][0] - r[0][0] * r[2][1];
        k[2][0] = r[0][1] * r[1][2] - r[0][2] * r[1][1];
        k[2][1] = r[0][2] * r[1][0] - r[0][0] * r[1][2];
        k[2][2] = r[0][0] * r[1][1] - r[0][1] * r[1][0];
        double d = 0.0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                const double n = 0.5 * (r[i][j] + k[i][j] / det);
                d = std::max(d, std::fabs(n - r[i][j]));
                r[i][j] = n;
            }
        }
        if (d < 1e-12) { break; }
    }
    BoneMatrix o = m;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { o.m[i][j] = static_cast<float>(r[i][j]); }
    }
    return o;
}

}  // namespace

bool ikArm(IkArt a) noexcept { return a == IkArt::ArmLinks || a == IkArt::ArmRechts; }

BoneMatrix starr(const BoneMatrix& m) noexcept {
    // Die Drehung einer Weltmatrix, die Mass und Scherung tragen kann (die Knochen der Spiele: Grundstellung * Bild): die
    // Spalten nach Gram-Schmidt wie rotationPart in G2C (third_party/g2c/src/compress.cpp) - matrixToQuat allein gibt bei
    // einer gestreckten Matrix einen anderen Winkel. Gram-Schmidt geht mit jeder Drehung von links mit (starr(U X) =
    // U starr(X)), darauf baut die IK.
    V3 x{m.m[0][0], m.m[1][0], m.m[2][0]};
    V3 y{m.m[0][1], m.m[1][1], m.m[2][1]};
    x = norm(x);
    y = norm(y - x * punkt(x, y));
    if (laenge(x) < 0.5 || laenge(y) < 0.5) {
        BoneMatrix e = BoneMatrix::identity();
        e.m[0][3] = m.m[0][3];
        e.m[1][3] = m.m[1][3];
        e.m[2][3] = m.m[2][3];
        return e;
    }
    const V3 z = kreuz(x, y);
    BoneMatrix r = BoneMatrix::identity();
    const V3 sp[3] = {x, y, z};
    for (int c = 0; c < 3; ++c) {
        r.m[0][c] = static_cast<float>(sp[c].x);
        r.m[1][c] = static_cast<float>(sp[c].y);
        r.m[2][c] = static_cast<float>(sp[c].z);
    }
    r.m[0][3] = m.m[0][3];
    r.m[1][3] = m.m[1][3];
    r.m[2][3] = m.m[2][3];
    return r;
}

bool zweiGliedIk(const float a[3], const float b[3], const float c[3], const float ziel[3], const float pol[3], float bNeu[3], float cNeu[3]) {
    const V3 A = v3(a);
    const V3 B = v3(b);
    const V3 C = v3(c);
    const double l1 = laenge(B - A);
    const double l2 = laenge(C - B);
    const V3 d = v3(ziel) - A;
    const double dl = laenge(d);
    std::memcpy(bNeu, b, sizeof(float) * 3U);
    std::memcpy(cNeu, c, sizeof(float) * 3U);
    if (dl < kKlein || l1 < kKlein || l2 < kKlein) { return false; }
    const V3 u = d * (1.0 / dl);
    // die Ebene: Wurzel -> Ziel und Wurzel -> Pol (ConstrainPoleVector); ohne Ebene die der jetzigen Kette
    bool ebene = true;
    V3 p = v3(pol) - A;
    V3 v = p - u * punkt(p, u);
    if (laenge(v) < 1e-6 * std::max(laenge(p), 1.0)) {
        ebene = false;
        v = (B - A) - u * punkt(B - A, u);
        if (laenge(v) < 1e-9) { v = std::fabs(u.x) < 0.9 ? kreuz(u, V3{1, 0, 0}) : kreuz(u, V3{0, 1, 0}); }
    }
    v = norm(v);
    const double lo = std::fabs(l1 - l2);
    const double hi = l1 + l2;
    const double dc = std::clamp(dl, lo, hi);
    const bool erreicht = dl >= lo - 1e-6 && dl <= hi + 1e-6;
    double cosA = (l1 * l1 + dc * dc - l2 * l2) / (2.0 * l1 * dc);
    cosA = std::clamp(cosA, -1.0, 1.0);
    const double sinA = std::sqrt(std::max(0.0, 1.0 - cosA * cosA));
    schreib(A + (u * cosA + v * sinA) * l1, bNeu);
    schreib(A + u * dc, cNeu);
    return erreicht && ebene;
}

int ikSpurIndex(const RigSchicht& s, IkArt art) noexcept {
    for (std::size_t i = 0; i < s.ik.size(); ++i) {
        if (s.ik[i].art == art) { return static_cast<int>(i); }
    }
    return -1;
}

int ikSpurVonKnochen(const RigSchicht& s, int knochen) noexcept {
    if (knochen < 0) { return -1; }
    for (std::size_t i = 0; i < s.ik.size(); ++i) {
        const IkSpur& sp = s.ik[i];
        for (const IkGlied* gl : {&sp.oben, &sp.unten, &sp.ende, &sp.zehen}) {
            if (gl->knochen == knochen || gl->twist == knochen || std::find(gl->mit.begin(), gl->mit.end(), knochen) != gl->mit.end()) { return static_cast<int>(i); }
        }
    }
    return -1;
}

bool ikSpurAnlegen(const GlaAnimation& g, const Rig& r, IkArt art, IkSpur& aus) {
    const bool arm = ikArm(art);
    const Seite seite = (art == IkArt::ArmLinks || art == IkArt::BeinLinks) ? Seite::Links : Seite::Rechts;
    const auto griff = [&](GriffArt a) -> const Griff* {
        const Griff* best = nullptr;
        for (const Griff& x : r.griffe) {
            if (x.art == a && x.seite == seite && (best == nullptr || x.nummer < best->nummer)) { best = &x; }
        }
        return best;
    };
    const Griff* o = griff(arm ? GriffArt::Oberarm : GriffArt::Oberschenkel);
    const Griff* u = griff(arm ? GriffArt::Unterarm : GriffArt::Unterschenkel);
    const Griff* e = griff(arm ? GriffArt::Hand : GriffArt::Fuss);
    if (o == nullptr || u == nullptr || e == nullptr || o->knochen < 0 || u->knochen < 0 || e->knochen < 0) { return false; }
    const auto glied = [](const Griff& x) {
        IkGlied gl;
        gl.knochen = x.knochen;
        gl.mit = x.mit;
        gl.twist = x.twist;
        gl.twistVon = x.twistVon;
        gl.twistAnteil = x.twistAnteil;
        return gl;
    };
    aus = IkSpur{};
    aus.art = art;
    aus.oben = glied(*o);
    aus.unten = glied(*u);
    aus.ende = glied(*e);
    if (!arm) {
        if (const Griff* z = griff(GriffArt::Zehen); z != nullptr) { aus.zehen = glied(*z); }
    }
    const int nb = static_cast<int>(g.bones.size());
    aus.polRaum = (o->knochen < nb) ? g.bones[static_cast<std::size_t>(o->knochen)].parent : -1;
    const auto bindeOrt = [&](int k) { return V3{g.bones[static_cast<std::size_t>(k)].basePose.m[0][3], g.bones[static_cast<std::size_t>(k)].basePose.m[1][3],
                                                 g.bones[static_cast<std::size_t>(k)].basePose.m[2][3]}; };
    aus.polAbstand = static_cast<float>(laenge(bindeOrt(u->knochen) - bindeOrt(o->knochen)));
    if (!arm) {
        // Fussrolle: Drehpunkte aus dem Rig (rigAnlegen: Ferse/Spitze aus den Ecken des Fusses, Ballen am Zehenknochen);
        // Achse oben x vorn, vorn aus dem Knie-Pol (rigAnlegen legt ihn vorn an)
        V3 vorn{0.0, -1.0, 0.0};
        for (const Griff& x : r.griffe) {
            if (x.seite != seite) { continue; }
            if (x.art == GriffArt::Ferse) {
                std::memcpy(aus.ferse, x.ort, sizeof aus.ferse);
                aus.hatFerse = true;
            }
            if (x.art == GriffArt::Ballen) {
                std::memcpy(aus.ballen, x.ort, sizeof aus.ballen);
                aus.hatBallen = true;
            }
            if (x.art == GriffArt::Zehe) {
                std::memcpy(aus.spitzePunkt, x.ort, sizeof aus.spitzePunkt);
                aus.hatSpitze = true;
            }
            if (x.art == GriffArt::PolKnie) {
                const V3 d = v3(x.ort) - bindeOrt(u->knochen);
                const V3 h{d.x, d.y, 0.0};
                if (laenge(h) > 1e-4) { vorn = norm(h); }
            }
        }
        schreib(norm(kreuz(V3{0.0, 0.0, 1.0}, vorn)), aus.rollAchse);
    }
    return true;
}

float ikAnteilAm(const IkSpur& sp, float bild) noexcept {
    const std::size_t n = sp.ziel.size();
    if (n == 0 || sp.wert.size() != n) { return 0.0F; }
    const auto it = std::upper_bound(sp.ziel.begin(), sp.ziel.end(), bild, [](float b, const RigSchluessel& k) { return b < static_cast<float>(k.bild); });
    const std::size_t i = it == sp.ziel.begin() ? 0U : static_cast<std::size_t>(it - sp.ziel.begin()) - 1U;
    return std::clamp(sp.wert[i].ik, 0.0F, 1.0F);
}

IkZiel ikZielAm(const IkSpur& sp, float bild, const std::vector<BoneMatrix>& welt) {
    IkZiel z;
    const Lokal l = lokalAm(sp, bild);
    if (!l.da) { return z; }
    z.da = true;
    z.ik = std::clamp(l.w.ik, 0.0F, 1.0F);
    z.raum = l.w.raum;
    z.rolle = l.w.rolle;
    z.spitze = std::clamp(l.w.spitze, 0.0F, 1.0F);
    std::memcpy(z.zehen, l.w.zehen, sizeof z.zehen);
    z.ziel = starr(ikAusRaum(welt, l.w.raum, l.ziel));
    BoneMatrix p;
    raumWelt(welt, sp.polRaum, p).transform(l.w.pol, z.pol);
    return z;
}

FussRolle fussRolle(const GlaAnimation& g, const IkSpur& sp, const BoneMatrix& ziel, float rolle, float spitze, const BoneMatrix& form) {
    FussRolle f;
    if (ikArm(sp.art) || sp.ende.knochen < 0 || static_cast<std::size_t>(sp.ende.knochen) >= g.bones.size() || rolle == 0.0F) { return f; }
    // X * B^-1: Grundstellung -> Welt, mit X = Ziel * Form (Mass und Scherung des Knochens, die die starre Lage nicht traegt)
    const BoneMatrix lage = ziel * form * g.bones[static_cast<std::size_t>(sp.ende.knochen)].basePoseInv;
    const auto punktWelt = [&](const float p[3]) {
        float w[3];
        lage.transform(p, w);
        return v3(w);
    };
    float achse[3];
    {
        const BoneMatrix d = ohneOrt(lage);
        d.transform(sp.rollAchse, achse);
    }
    const float zurueck = std::min(rolle, 0.0F);
    const float vor = std::max(rolle, 0.0F);
    const float s = sp.hatBallen ? std::clamp(spitze, 0.0F, 1.0F) : 1.0F;
    const BoneMatrix rf = sp.hatFerse && zurueck != 0.0F ? umPunkt(drehungUm(achse, zurueck), punktWelt(sp.ferse)) : BoneMatrix::identity();
    const BoneMatrix rs = sp.hatSpitze && vor * s != 0.0F ? umPunkt(drehungUm(achse, vor * s), punktWelt(sp.spitzePunkt)) : BoneMatrix::identity();
    const BoneMatrix rb = sp.hatBallen && vor * (1.0F - s) != 0.0F ? umPunkt(drehungUm(achse, vor * (1.0F - s)), punktWelt(sp.ballen)) : BoneMatrix::identity();
    f.zehen = rf * rs;
    f.fuss = f.zehen * rb;
    return f;
}

std::vector<int> ikAnwenden(const GlaAnimation& g, const RigSchicht& s, float bild, std::vector<BoneMatrix>& welt, std::vector<std::pair<int, BoneMatrix>>& mEff,
                            int ohneSpur) {
    if (s.ik.empty() || welt.size() != g.bones.size()) { return {}; }
    Zustand z;
    zustandAnlegen(g, welt, mEff, z);
    // Reihenfolge: eine Spur, deren Raum in einer anderen Spur liegt, nach dieser (Saebel an der anderen Hand)
    const std::size_t n = s.ik.size();
    std::vector<std::vector<char>> knochen(n);
    for (std::size_t i = 0; i < n; ++i) { knochen[i] = spurKnochen(z, s.ik[i]); }
    std::vector<char> fertig(n, 0);
    std::vector<std::size_t> folge;
    for (std::size_t runde = 0; runde < n; ++runde) {
        for (std::size_t i = 0; i < n; ++i) {
            if (fertig[i] != 0) { continue; }
            const int raum = s.ik[i].wert.empty() ? -1 : lokalAm(s.ik[i], bild).w.raum;
            bool warte = false;
            for (std::size_t j = 0; j < n && raum >= 0; ++j) {
                if (j != i && fertig[j] == 0 && static_cast<std::size_t>(raum) < knochen[j].size() && knochen[j][static_cast<std::size_t>(raum)] != 0) { warte = true; }
            }
            if (!warte || runde + 1 == n) {
                fertig[i] = 1;
                folge.push_back(i);
            }
        }
    }
    for (const std::size_t i : folge) {
        if (static_cast<int>(i) == ohneSpur) { continue; }
        const IkSpur& sp = s.ik[i];
        const IkZiel ziel = ikZielAm(sp, bild, welt);
        if (!ziel.da || ziel.ik <= 0.0F) { continue; }
        if (ziel.ik >= 1.0F) {
            loesen(z, sp, ziel);
            continue;
        }
        // Mischen (Regler IK-FK zwischen 0 und 1) wie Blender: je Knochen die lokale Aenderung K = M_alt^-1 M_neu der vollen
        // IK zwischen Einheit und K (iksolver_plugin.cc execute_posetree: blend_m3_m3m3(basis_change, identity,
        // basis_change, resultinf)), Drehung slerp, die Verschiebung (flache Glieder drehen um einen anderen Punkt) linear;
        // von oben nach unten, die Kinder erben
        std::vector<BoneMatrix> weltIk = welt;
        Zustand zi = z;
        zi.welt = &weltIk;
        loesen(zi, sp, ziel);
        for (const int b : g.topologicalOrder()) {
            if (b < 0 || static_cast<std::size_t>(b) >= welt.size() || zi.geaendert[static_cast<std::size_t>(b)] == 0) { continue; }
            const BoneMatrix k = starrInvers(z.m[static_cast<std::size_t>(b)]) * zi.m[static_cast<std::size_t>(b)];
            float q[4];
            float qm[4];
            quatAusMatrix(k, q);
            const float eins[4] = {1.0F, 0.0F, 0.0F, 0.0F};
            slerp4(eins, q, ziel.ik, qm);
            BoneMatrix kw = matrixAusQuat(qm);
            for (int r = 0; r < 3; ++r) { kw.m[r][3] = k.m[r][3] * ziel.ik; }
            const BoneMatrix& x = welt[static_cast<std::size_t>(b)];
            anwenden(z, b, x * kw * starrInvers(x));
        }
    }
    return zurueck(z, mEff);
}

void ikHenkelNeu(IkSpur& sp) {
    std::size_t a = 0;
    while (a < sp.ziel.size()) {
        std::size_t e = a + 1;
        while (e < sp.ziel.size() && e < sp.wert.size() && sp.wert[e].raum == sp.wert[a].raum) { ++e; }
        std::vector<RigSchluessel> lauf(sp.ziel.begin() + static_cast<std::ptrdiff_t>(a), sp.ziel.begin() + static_cast<std::ptrdiff_t>(e));
        henkelNeu(lauf);
        std::copy(lauf.begin(), lauf.end(), sp.ziel.begin() + static_cast<std::ptrdiff_t>(a));
        a = e;
    }
}

void ikSchluesselSetzen(IkSpur& sp, int bild, const BoneMatrix& zielImRaum, const IkWert& w) {
    RigSchluessel neu;
    neu.bild = bild;
    neu.interp = Interp::Bezier;   // wie neue Schluessel der Schicht (Max und Blender: Bezier, Auto Clamped)
    quatAusMatrix(zielImRaum, neu.q);
    neu.t[0] = zielImRaum.m[0][3];
    neu.t[1] = zielImRaum.m[1][3];
    neu.t[2] = zielImRaum.m[2][3];
    const auto it = std::lower_bound(sp.ziel.begin(), sp.ziel.end(), bild, [](const RigSchluessel& x, int b) { return x.bild < b; });
    const std::size_t i = static_cast<std::size_t>(it - sp.ziel.begin());
    // q in der Halbkugel des Nachbarn gleichen Raums (die Kanaele liefen sonst ueber 0)
    const RigSchluessel* nachbar = nullptr;
    if (i > 0 && sp.wert[i - 1].raum == w.raum) {
        nachbar = &sp.ziel[i - 1];
    } else if (i < sp.ziel.size() && sp.ziel[i].bild != bild && sp.wert[i].raum == w.raum) {
        nachbar = &sp.ziel[i];
    }
    if (nachbar != nullptr && neu.q[0] * nachbar->q[0] + neu.q[1] * nachbar->q[1] + neu.q[2] * nachbar->q[2] + neu.q[3] * nachbar->q[3] < 0.0F) {
        for (float& x : neu.q) { x = -x; }
    }
    if (it != sp.ziel.end() && it->bild == bild) {
        std::memcpy(it->q, neu.q, sizeof neu.q);
        std::memcpy(it->t, neu.t, sizeof neu.t);
        sp.wert[i] = w;
    } else {
        sp.ziel.insert(it, neu);
        sp.wert.insert(sp.wert.begin() + static_cast<std::ptrdiff_t>(i), w);
    }
    ikHenkelNeu(sp);
}

void ikSchluesselLoeschen(IkSpur& sp, int bild) {
    for (std::size_t i = 0; i < sp.ziel.size(); ++i) {
        if (sp.ziel[i].bild == bild) {
            sp.ziel.erase(sp.ziel.begin() + static_cast<std::ptrdiff_t>(i));
            if (i < sp.wert.size()) { sp.wert.erase(sp.wert.begin() + static_cast<std::ptrdiff_t>(i)); }
            break;
        }
    }
    ikHenkelNeu(sp);
}

BoneMatrix ikImRaum(const std::vector<BoneMatrix>& welt, int raum, const BoneMatrix& weltLage) {
    BoneMatrix p;
    return starrInvers(raumWelt(welt, raum, p)) * weltLage;
}

BoneMatrix ikAusRaum(const std::vector<BoneMatrix>& welt, int raum, const BoneMatrix& imRaum) {
    BoneMatrix p;
    return raumWelt(welt, raum, p) * imRaum;
}

namespace {

// Die Welt am Bild mit allen Spuren ausser `ohne` (-1: alle)
void weltMit(const GlaAnimation& g, const RigSchicht& s, int bild, int ohne, std::vector<BoneMatrix>& welt, std::vector<std::pair<int, BoneMatrix>>* m = nullptr) {
    std::vector<std::pair<int, BoneMatrix>> eigene;
    std::vector<std::pair<int, BoneMatrix>>& nach = m != nullptr ? *m : eigene;
    rigNachMalFk(g, s, bild, bild, 0.0F, nach);
    g.worldMatricesLerp(bild, bild, 0.0F, {}, nach, welt);
    if (!ikAnwenden(g, s, static_cast<float>(bild), welt, nach, ohne).empty()) { g.worldMatricesLerp(bild, bild, 0.0F, {}, nach, welt); }
}

// Der Pol in der Ebene der Kette (rigify match_pole_target: Mitte plus Senkrechte), auf der Seite des Gelenks
bool polAusKette(const IkSpur& sp, const std::vector<BoneMatrix>& welt, float pol[3]) {
    const V3 A = ortVon(welt[static_cast<std::size_t>(sp.oben.knochen)]);
    const V3 B = ortVon(welt[static_cast<std::size_t>(sp.unten.knochen)]);
    const V3 C = ortVon(welt[static_cast<std::size_t>(sp.ende.knochen)]);
    const V3 u = norm(C - A);
    const V3 v = (B - A) - u * punkt(B - A, u);
    if (laenge(v) < 1e-9) { return false; }   // jede messbare Beugung gibt die Ebene (sonst springt das Gelenk in eine andere)
    schreib(B + norm(v) * std::max(static_cast<double>(sp.polAbstand), 1.0), pol);
    return true;
}

bool kette(const IkSpur& sp, const std::vector<BoneMatrix>& welt) {
    for (const int k : {sp.oben.knochen, sp.unten.knochen, sp.ende.knochen}) {
        if (k < 0 || static_cast<std::size_t>(k) >= welt.size()) { return false; }
    }
    return true;
}

}  // namespace

namespace {

// Ziel und Wert eines Schluessels "IK -> FK" am Bild (ohne ihn zu setzen)
bool zielAusFk(const GlaAnimation& g, const RigSchicht& s, int spur, int bild, float ik, int raum, BoneMatrix& zielImRaum, IkWert& w) {
    std::vector<BoneMatrix> welt;
    weltMit(g, s, bild, spur, welt);
    const IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    if (!kette(sp, welt)) { return false; }
    const Lokal l = lokalAm(sp, static_cast<float>(bild));
    w = l.da ? l.w : IkWert{};
    w.ik = ik;
    if (raum >= -1) { w.raum = raum; }
    w.rolle = 0.0F;
    float pol[3];
    if (!polAusKette(sp, welt, pol)) {
        // gestreckt: der bisherige Pol (oder vor dem Gelenk, eine Gliedlaenge weit)
        if (l.da) {
            BoneMatrix p;
            raumWelt(welt, sp.polRaum, p).transform(l.w.pol, pol);
        } else {
            const V3 B = ortVon(welt[static_cast<std::size_t>(sp.unten.knochen)]);
            schreib(B + V3{0.0, 0.0, 1.0} * std::max(static_cast<double>(sp.polAbstand), 1.0), pol);
        }
    }
    BoneMatrix pr;
    starrInvers(raumWelt(welt, sp.polRaum, pr)).transform(pol, w.pol);
    const BoneMatrix ende = starr(welt[static_cast<std::size_t>(sp.ende.knochen)]);
    if (sp.zehen.knochen >= 0 && static_cast<std::size_t>(sp.zehen.knochen) < welt.size()) {
        quatAusMatrix(transponiert(ohneOrt(ende)) * ohneOrt(starr(welt[static_cast<std::size_t>(sp.zehen.knochen)])), w.zehen);
    }
    zielImRaum = ikImRaum(welt, w.raum, ende);
    return true;
}

}  // namespace

void ikAufFk(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, int raum) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    BoneMatrix z;
    IkWert w;
    if (zielAusFk(g, s, spur, bild, ik, raum, z, w)) { ikSchluesselSetzen(s.ik[static_cast<std::size_t>(spur)], bild, z, w); }
}

void ikAufFkBereich(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis, float ik) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size() || bis < von) { return; }
    // erst alle Bilder aus der unveraenderten Schicht (die FK-Pose haengt nicht an dieser Spur), dann die Schluessel mit
    // einmal Henkeln - ein Schluessel je Bild, die der Spur in der Spanne ersetzt
    std::vector<RigSchluessel> neuZiel;
    std::vector<IkWert> neuWert;
    for (int bild = von; bild <= bis; ++bild) {
        BoneMatrix z;
        IkWert w;
        if (!zielAusFk(g, s, spur, bild, ik, -2, z, w)) { continue; }
        RigSchluessel k;
        k.bild = bild;
        k.interp = Interp::Bezier;
        quatAusMatrix(z, k.q);
        k.t[0] = z.m[0][3];
        k.t[1] = z.m[1][3];
        k.t[2] = z.m[2][3];
        if (!neuZiel.empty() && neuWert.back().raum == w.raum && k.q[0] * neuZiel.back().q[0] + k.q[1] * neuZiel.back().q[1] + k.q[2] * neuZiel.back().q[2] +
                                                                   k.q[3] * neuZiel.back().q[3] < 0.0F) {
            for (float& x : k.q) { x = -x; }
        }
        neuZiel.push_back(k);
        neuWert.push_back(w);
    }
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    std::vector<RigSchluessel> z2;
    std::vector<IkWert> w2;
    std::size_t j = 0;
    for (std::size_t i = 0; i < sp.ziel.size(); ++i) {
        const int b = sp.ziel[i].bild;
        if (b >= von && b <= bis) { continue; }
        while (j < neuZiel.size() && neuZiel[j].bild < b) {
            z2.push_back(neuZiel[j]);
            w2.push_back(neuWert[j]);
            ++j;
        }
        z2.push_back(sp.ziel[i]);
        w2.push_back(sp.wert[i]);
    }
    for (; j < neuZiel.size(); ++j) {
        z2.push_back(neuZiel[j]);
        w2.push_back(neuWert[j]);
    }
    sp.ziel = std::move(z2);
    sp.wert = std::move(w2);
    ikHenkelNeu(sp);
}

namespace {

// "FK -> IK" an den Bildern der Liste: erst jedes Bild aus der unveraenderten Schicht (ein geschriebener Schluessel aendert
// die FK-Pose der Nachbarn), dann die Schluessel - wie "Snap & Bake" von Rigify ueber eine Spanne
int fkAufIkBilder(const GlaAnimation& g, RigSchicht& s, int spur, const std::vector<int>& bilder) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size() || bilder.empty()) { return 0; }
    std::vector<std::vector<std::pair<int, BoneMatrix>>> roh(bilder.size());
    for (std::size_t j = 0; j < bilder.size(); ++j) {
        const int bild = bilder[j];
        std::vector<std::pair<int, BoneMatrix>> m;
        rigNachMalFk(g, s, bild, bild, 0.0F, m);
        std::vector<BoneMatrix> welt;
        g.worldMatricesLerp(bild, bild, 0.0F, {}, m, welt);
        for (const int b : ikAnwenden(g, s, static_cast<float>(bild), welt, m)) {
            if (ikSpurVonKnochen(s, b) != spur) { continue; }
            BoneMatrix eff = BoneMatrix::identity();
            for (const auto& [k, mm] : m) {
                if (k == b) { eff = mm; }
            }
            if (s.bindeBasis) {
                const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
                eff = kb.basePoseInv * g.localMatrix(bild, b) * kb.basePose * eff;   // M = B^-1 A B M_eff (rigNachMalAuf umgekehrt)
            }
            // der Schluessel traegt eine Drehung: die naechste zu M (polare Zerlegung, R = M (M^T M)^-1/2 ueber die Newton-Folge
            // R <- (R + R^-T) / 2 bis zur Ruhe) - die IK dreht Knochen, deren Weltmatrix nicht orthonormal ist (Raven-Daten,
            // MC_UnCompressQuat normiert nicht), dann ist M = X^-1 U X keine reine Drehung (mb6_irl_jka)
            roh[j].emplace_back(b, naechsteDrehung(eff));
        }
    }
    int n = 0;
    for (std::size_t j = 0; j < bilder.size(); ++j) {
        for (const auto& [b, m] : roh[j]) {
            rigSchluesselSetzen(s, b, bilder[j], m);
            ++n;
        }
    }
    return n;
}

}  // namespace

int fkAufIk(const GlaAnimation& g, RigSchicht& s, int spur, int bild) { return fkAufIkBilder(g, s, spur, {bild}); }

int fkAufIkBereich(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis) {
    if (bis < von) { return 0; }
    std::vector<int> bilder;
    for (int bild = von; bild <= bis; ++bild) { bilder.push_back(bild); }
    return fkAufIkBilder(g, s, spur, bilder);
}

void ikFkWechseln(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, bool einrasten) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    if (ik > 0.0F) {
        if (einrasten || sp.ziel.empty()) {
            ikAufFk(g, s, spur, bild, ik);
            return;
        }
    } else if (einrasten) {
        (void)fkAufIk(g, s, spur, bild);
    }
    const Lokal l = lokalAm(sp, static_cast<float>(bild));
    IkWert w = l.w;
    w.ik = ik;
    ikSchluesselSetzen(sp, bild, l.ziel, w);
}

int ikFkUmschaltenAb(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float ik, bool einrasten) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return bild; }
    ik = std::clamp(ik, 0.0F, 1.0F);
    // die Bilder: dieses und die folgenden Schluessel, die noch den alten Anteil tragen - wie ein Schluessel der IK/FK-Spur
    // eines CAT-Glieds, der bis zum naechsten gilt (Autodesk-Hilfe "Limb Animation Controls": IK/FK 0 = IK, 1 = FK, ein
    // Schluessel des Reglers; im Rig dieser Szenen die Spur layerIKFKRatio je Glied, Y:/werkzeuge/ent2/cat_ikfk.txt)
    const IkSpur& sp0 = s.ik[static_cast<std::size_t>(spur)];
    const float alt = sp0.ziel.empty() ? 0.0F : ikAnteilAm(sp0, static_cast<float>(bild));
    std::vector<int> bilder{bild};
    for (std::size_t i = 0; i < sp0.ziel.size() && i < sp0.wert.size(); ++i) {
        if (sp0.ziel[i].bild <= bild) { continue; }
        if (std::fabs(std::clamp(sp0.wert[i].ik, 0.0F, 1.0F) - alt) > 1e-4F) { break; }   // dort schon anders umgeschaltet
        bilder.push_back(sp0.ziel[i].bild);
    }
    if (einrasten) {
        // die sichtbare Pose in die FK-Schluessel (CAT "Match IK and FK" aus IK, Rigify fk2ik) - bei FK (Anteil 0) wirkt keine IK,
        // dann aendert das nichts
        (void)fkAufIkBilder(g, s, spur, bilder);
        if (ik > 0.0F) {
            // nach IK: Ziel und Pol auf diese FK-Pose (CAT "Move IK Target", Rigify ik2fk); erst alle Bilder ausrechnen (die FK-Pose
            // haengt nicht an dieser Spur), dann setzen
            std::vector<std::pair<BoneMatrix, IkWert>> neu;
            std::vector<int> mit;
            for (const int b : bilder) {
                BoneMatrix z;
                IkWert w;
                if (zielAusFk(g, s, spur, b, ik, -2, z, w)) {
                    neu.emplace_back(z, w);
                    mit.push_back(b);
                }
            }
            for (std::size_t j = 0; j < mit.size(); ++j) { ikSchluesselSetzen(s.ik[static_cast<std::size_t>(spur)], mit[j], neu[j].first, neu[j].second); }
            if (!mit.empty()) { return bilder.back(); }
        }
    }
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    if (sp.ziel.empty()) {
        // noch keine Spur: ein Schluessel am Bild mit dem Ziel auf der Pose
        ikAufFk(g, s, spur, bild, ik);
        return bild;
    }
    for (const int b : bilder) {
        const auto it = std::find_if(sp.ziel.begin(), sp.ziel.end(), [b](const RigSchluessel& k) { return k.bild == b; });
        if (it != sp.ziel.end()) {
            sp.wert[static_cast<std::size_t>(it - sp.ziel.begin())].ik = ik;
            continue;
        }
        const Lokal l = lokalAm(sp, static_cast<float>(b));
        IkWert w = l.w;
        w.ik = ik;
        ikSchluesselSetzen(sp, b, l.ziel, w);
    }
    return bilder.back();
}

void ikKetteJedesBild(const GlaAnimation& g, RigSchicht& s, int spur, int von, int bis) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size() || bis < von) { return; }
    std::vector<int> kette;
    {
        const IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
        for (const IkGlied* gl : {&sp.oben, &sp.unten, &sp.ende, &sp.zehen}) {
            if (gl->knochen >= 0) { kette.push_back(gl->knochen); }
            kette.insert(kette.end(), gl->mit.begin(), gl->mit.end());
            if (gl->twist >= 0) { kette.push_back(gl->twist); }
        }
    }
    std::vector<std::pair<int, std::pair<int, BoneMatrix>>> neu;
    for (const int b : kette) {
        if (b < 0 || static_cast<std::size_t>(b) >= g.bones.size()) { continue; }
        for (int n = von; n <= bis; ++n) {
            if (s.hatSchluessel(b, n)) { continue; }
            BoneMatrix m = BoneMatrix::identity();
            if (!rigWert(s, b, static_cast<float>(n), m) && s.bindeBasis) {
                const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
                m = kb.basePoseInv * g.localMatrix(n, b) * kb.basePose;   // ohne Schluessel: das Bild der GLA (wie ikNurAmBild)
            }
            neu.push_back({b, {n, m}});
        }
    }
    for (const auto& [b, nm] : neu) { rigSchluesselSetzen(s, b, nm.first, nm.second); }
}

std::vector<float> ikGliedAbweichung(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b, int von, int bis) {
    std::vector<float> aus(b.ik.size(), 0.0F);
    if (b.ik.empty() || bis < von || g.bones.empty()) { return aus; }
    // je Spur die Knochen: das obere Glied und alles darunter (Unterarm, Hand, Finger bzw. Unterschenkel, Fuss, Zehen)
    std::vector<std::vector<char>> knochen(b.ik.size(), std::vector<char>(g.bones.size(), 0));
    for (std::size_t i = 0; i < b.ik.size(); ++i) {
        const int oben = b.ik[i].oben.knochen;
        if (oben < 0 || static_cast<std::size_t>(oben) >= g.bones.size()) { continue; }
        knochen[i][static_cast<std::size_t>(oben)] = 1;
        for (const int k : g.topologicalOrder()) {
            const int p = k >= 0 && static_cast<std::size_t>(k) < g.bones.size() ? g.bones[static_cast<std::size_t>(k)].parent : -1;
            if (p >= 0 && knochen[i][static_cast<std::size_t>(p)] != 0) { knochen[i][static_cast<std::size_t>(k)] = 1; }
        }
    }
    std::vector<BoneMatrix> wa;
    std::vector<BoneMatrix> wb;
    for (int bild = von; bild <= bis; ++bild) {
        rigWelt(g, a, bild, bild, 0.0F, wa);
        rigWelt(g, b, bild, bild, 0.0F, wb);
        if (wa.size() != g.bones.size() || wb.size() != g.bones.size()) { continue; }
        for (std::size_t i = 0; i < b.ik.size(); ++i) {
            for (std::size_t k = 0; k < g.bones.size(); ++k) {
                if (knochen[i][k] == 0) { continue; }
                float d = 0.0F;
                for (int r = 0; r < 3; ++r) { d += (wa[k].m[r][3] - wb[k].m[r][3]) * (wa[k].m[r][3] - wb[k].m[r][3]); }
                aus[i] = std::max(aus[i], std::sqrt(d));
            }
        }
    }
    return aus;
}

void ikRaumWechseln(const GlaAnimation& g, RigSchicht& s, int spur, int bild, int raum, int erstesBild) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    std::vector<BoneMatrix> welt;
    weltMit(g, s, bild, -1, welt);
    if (s.ik[static_cast<std::size_t>(spur)].ziel.empty()) { ikAufFk(g, s, spur, bild, 1.0F); }
    // der Raum darf nicht in der eigenen Kette liegen (er wuerde sich selbst ziehen)
    if (raum >= 0 && ikSpurVonKnochen(s, raum) == spur) { raum = -1; }
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    const IkZiel z = ikZielAm(sp, static_cast<float>(bild), welt);
    if (bild - 1 >= erstesBild) {
        bool da = false;
        for (const RigSchluessel& k : sp.ziel) { da = da || k.bild == bild - 1; }
        if (!da) {
            const Lokal vor = lokalAm(sp, static_cast<float>(bild - 1));
            ikSchluesselSetzen(sp, bild - 1, vor.ziel, vor.w);
        }
    }
    Lokal l = lokalAm(sp, static_cast<float>(bild));
    l.w.raum = raum;
    ikSchluesselSetzen(sp, bild, ikImRaum(welt, raum, z.ziel), l.w);
}

void ikNurAmBild(const GlaAnimation& g, RigSchicht& s, int spur, int bild, int erstes, int letztes) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    if (ikAnteilAm(s.ik[static_cast<std::size_t>(spur)], static_cast<float>(bild)) >= 0.5F && !s.ik[static_cast<std::size_t>(spur)].ziel.empty()) { return; }
    // die Nachbarbilder halten die jetzige Pose (FK-Schluessel der Kette mit ihrem Wert, IK-Schluessel mit Anteil 0): ein
    // Schluessel am Bild wirkt dann nur dort (wie ein Schluessel auf der Trajektorie von 3ds Max zwischen seinen Nachbarn)
    std::vector<int> kette;
    {
        const IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
        for (const IkGlied* gl : {&sp.oben, &sp.unten, &sp.ende, &sp.zehen}) {
            if (gl->knochen >= 0) { kette.push_back(gl->knochen); }
            kette.insert(kette.end(), gl->mit.begin(), gl->mit.end());
            if (gl->twist >= 0) { kette.push_back(gl->twist); }
        }
    }
    for (const int n : {bild - 1, bild + 1}) {
        if (n < erstes || n > letztes) { continue; }
        const IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
        if (std::any_of(sp.ziel.begin(), sp.ziel.end(), [n](const RigSchluessel& k) { return k.bild == n; })) { continue; }
        for (const int b : kette) {
            if (s.hatSchluessel(b, n)) { continue; }
            BoneMatrix m = BoneMatrix::identity();
            if (!rigWert(s, b, static_cast<float>(n), m) && s.bindeBasis && static_cast<std::size_t>(b) < g.bones.size()) {
                const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
                m = kb.basePoseInv * g.localMatrix(n, b) * kb.basePose;
            }
            rigSchluesselSetzen(s, b, n, m);
        }
        ikAufFk(g, s, spur, n, 0.0F);
    }
}

void ikZielSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, const BoneMatrix& weltZiel) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    IkSpur& sp0 = s.ik[static_cast<std::size_t>(spur)];
    // Hand greifen = IK (wie 3ds Max): ist das Glied hier FK, erst ohne Sprung einrasten
    if (sp0.ziel.empty() || ikAnteilAm(sp0, static_cast<float>(bild)) < 1.0F) { ikAufFk(g, s, spur, bild, 1.0F); }
    std::vector<BoneMatrix> welt;
    weltMit(g, s, bild, spur, welt);
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    Lokal l = lokalAm(sp, static_cast<float>(bild));
    l.w.ik = 1.0F;
    ikSchluesselSetzen(sp, bild, ikImRaum(welt, l.w.raum, starr(weltZiel)), l.w);
    (void)fkAufIk(g, s, spur, bild);
}

void ikPolSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, const float weltPol[3]) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    if (s.ik[static_cast<std::size_t>(spur)].ziel.empty() || ikAnteilAm(s.ik[static_cast<std::size_t>(spur)], static_cast<float>(bild)) < 1.0F) {
        ikAufFk(g, s, spur, bild, 1.0F);
    }
    std::vector<BoneMatrix> welt;
    weltMit(g, s, bild, spur, welt);
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    Lokal l = lokalAm(sp, static_cast<float>(bild));
    BoneMatrix pr;
    starrInvers(raumWelt(welt, sp.polRaum, pr)).transform(weltPol, l.w.pol);
    ikSchluesselSetzen(sp, bild, l.ziel, l.w);
    (void)fkAufIk(g, s, spur, bild);
}

void ikRolleSetzen(const GlaAnimation& g, RigSchicht& s, int spur, int bild, float rolle, float spitze) {
    if (spur < 0 || static_cast<std::size_t>(spur) >= s.ik.size()) { return; }
    if (s.ik[static_cast<std::size_t>(spur)].ziel.empty() || ikAnteilAm(s.ik[static_cast<std::size_t>(spur)], static_cast<float>(bild)) < 1.0F) {
        ikAufFk(g, s, spur, bild, 1.0F);
    }
    IkSpur& sp = s.ik[static_cast<std::size_t>(spur)];
    Lokal l = lokalAm(sp, static_cast<float>(bild));
    l.w.rolle = rolle;
    l.w.spitze = std::clamp(spitze, 0.0F, 1.0F);
    ikSchluesselSetzen(sp, bild, l.ziel, l.w);
    (void)fkAufIk(g, s, spur, bild);
}

float ikSprung(const GlaAnimation& g, const RigSchicht& a, const RigSchicht& b, int bild) {
    std::vector<BoneMatrix> wa;
    std::vector<BoneMatrix> wb;
    rigWelt(g, a, bild, bild, 0.0F, wa);
    rigWelt(g, b, bild, bild, 0.0F, wb);
    float d = 0.0F;
    for (std::size_t k = 0; k < wa.size() && k < wb.size(); ++k) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(wa[k].m[i][j] - wb[k].m[i][j])); }
        }
    }
    return d;
}

}  // namespace bhed::modell
