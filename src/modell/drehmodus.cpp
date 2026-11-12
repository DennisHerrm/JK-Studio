// modell/drehmodus.cpp - Drehmodi eines Knochens wie Blender (ENTSCHEID3A B9). Formeln nach Blenders BLI_math_rotation
// (Graphics Gems IV "Euler Angle Conversion" fuer die Euler-Reihenfolgen); Blender rechnet dort mit spaltenweisen Matrizen
// (mat[Spalte][Zeile]) - hier steht m[Zeile][Spalte], jeder Zugriff ist entsprechend gespiegelt.
#include "bhed/modell/drehmodus.h"

#include <cmath>
#include <cfloat>

namespace bhed::modell {
namespace {

// Blender RotOrderInfo: die drei Achsen in der Reihenfolge des Drehens und die Paritaet der Vertauschung
struct Ordnung {
    int i, j, k;
    bool paritaet;
};

Ordnung ordnung(DrehModus o) {
    switch (o) {
        case DrehModus::Xzy: return {0, 2, 1, true};
        case DrehModus::Yxz: return {1, 0, 2, true};
        case DrehModus::Yzx: return {1, 2, 0, false};
        case DrehModus::Zxy: return {2, 0, 1, false};
        case DrehModus::Zyx: return {2, 1, 0, true};
        default: break;
    }
    return {0, 1, 2, false};
}

void einheit(float m[3][3]) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { m[r][c] = r == c ? 1.0F : 0.0F; }
    }
}

}  // namespace

int drehWerteZahl(DrehModus m) { return istEuler(m) ? 3 : 4; }

bool istEuler(DrehModus m) { return m != DrehModus::Quaternion && m != DrehModus::AchseWinkel; }

void matrixAusEuler(const float e[3], DrehModus o, float m[3][3]) {
    const Ordnung r = ordnung(o);
    double ti = e[r.i];
    double tj = e[r.j];
    double th = e[r.k];
    if (r.paritaet) {
        ti = -ti;
        tj = -tj;
        th = -th;
    }
    const double ci = std::cos(ti);
    const double cj = std::cos(tj);
    const double ch = std::cos(th);
    const double si = std::sin(ti);
    const double sj = std::sin(tj);
    const double sh = std::sin(th);
    const double cc = ci * ch;
    const double cs = ci * sh;
    const double sc = si * ch;
    const double ss = si * sh;
    // Blender M[Spalte][Zeile] = m[Zeile][Spalte]
    m[r.i][r.i] = static_cast<float>(cj * ch);
    m[r.i][r.j] = static_cast<float>(sj * sc - cs);
    m[r.i][r.k] = static_cast<float>(sj * cc + ss);
    m[r.j][r.i] = static_cast<float>(cj * sh);
    m[r.j][r.j] = static_cast<float>(sj * ss + cc);
    m[r.j][r.k] = static_cast<float>(sj * cs - sc);
    m[r.k][r.i] = static_cast<float>(-sj);
    m[r.k][r.j] = static_cast<float>(cj * si);
    m[r.k][r.k] = static_cast<float>(cj * ci);
}

void eulerAusMatrix(const float m[3][3], DrehModus o, float e[3]) {
    const Ordnung r = ordnung(o);
    const int i = r.i;
    const int j = r.j;
    const int k = r.k;
    float e1[3];
    float e2[3];
    // mat3_normalized_to_eulo2 (Blender mat[a][b] = m[b][a])
    const float cy = std::hypot(m[i][i], m[j][i]);
    if (cy > 16.0F * FLT_EPSILON) {
        e1[i] = std::atan2(m[k][j], m[k][k]);
        e1[j] = std::atan2(-m[k][i], cy);
        e1[k] = std::atan2(m[j][i], m[i][i]);
        e2[i] = std::atan2(-m[k][j], -m[k][k]);
        e2[j] = std::atan2(-m[k][i], -cy);
        e2[k] = std::atan2(-m[j][i], -m[i][i]);
    } else {
        e1[i] = std::atan2(-m[j][k], m[j][j]);
        e1[j] = std::atan2(-m[k][i], cy);
        e1[k] = 0.0F;
        for (int a = 0; a < 3; ++a) { e2[a] = e1[a]; }
    }
    if (r.paritaet) {
        for (int a = 0; a < 3; ++a) {
            e1[a] = -e1[a];
            e2[a] = -e2[a];
        }
    }
    // mat3_normalized_to_eulO: die Loesung mit den kleineren Werten
    const float d1 = std::fabs(e1[0]) + std::fabs(e1[1]) + std::fabs(e1[2]);
    const float d2 = std::fabs(e2[0]) + std::fabs(e2[1]) + std::fabs(e2[2]);
    const float* best = d1 > d2 ? e2 : e1;
    for (int a = 0; a < 3; ++a) { e[a] = best[a]; }
}

void quatAusMatrix(const float m[3][3], float q[4]) {
    // mat3_normalized_to_quat_fast (Blender mat[a][b] = m[b][a]); w ist nie negativ
    const auto M = [&](int a, int b) { return m[b][a]; };
    if (M(2, 2) < 0.0F) {
        if (M(0, 0) > M(1, 1)) {
            const float spur = 1.0F + M(0, 0) - M(1, 1) - M(2, 2);
            float s = 2.0F * std::sqrt(spur);
            if (M(1, 2) < M(2, 1)) { s = -s; }
            q[1] = 0.25F * s;
            s = 1.0F / s;
            q[0] = (M(1, 2) - M(2, 1)) * s;
            q[2] = (M(0, 1) + M(1, 0)) * s;
            q[3] = (M(2, 0) + M(0, 2)) * s;
            if (spur == 1.0F && q[0] == 0.0F && q[2] == 0.0F && q[3] == 0.0F) { q[1] = 1.0F; }
        } else {
            const float spur = 1.0F - M(0, 0) + M(1, 1) - M(2, 2);
            float s = 2.0F * std::sqrt(spur);
            if (M(2, 0) < M(0, 2)) { s = -s; }
            q[2] = 0.25F * s;
            s = 1.0F / s;
            q[0] = (M(2, 0) - M(0, 2)) * s;
            q[1] = (M(0, 1) + M(1, 0)) * s;
            q[3] = (M(1, 2) + M(2, 1)) * s;
            if (spur == 1.0F && q[0] == 0.0F && q[1] == 0.0F && q[3] == 0.0F) { q[2] = 1.0F; }
        }
    } else {
        if (M(0, 0) < -M(1, 1)) {
            const float spur = 1.0F - M(0, 0) - M(1, 1) + M(2, 2);
            float s = 2.0F * std::sqrt(spur);
            if (M(0, 1) < M(1, 0)) { s = -s; }
            q[3] = 0.25F * s;
            s = 1.0F / s;
            q[0] = (M(0, 1) - M(1, 0)) * s;
            q[1] = (M(2, 0) + M(0, 2)) * s;
            q[2] = (M(1, 2) + M(2, 1)) * s;
            if (spur == 1.0F && q[0] == 0.0F && q[1] == 0.0F && q[2] == 0.0F) { q[3] = 1.0F; }
        } else {
            const float spur = 1.0F + M(0, 0) + M(1, 1) + M(2, 2);
            float s = 2.0F * std::sqrt(spur);
            q[0] = 0.25F * s;
            s = 1.0F / s;
            q[1] = (M(1, 2) - M(2, 1)) * s;
            q[2] = (M(2, 0) - M(0, 2)) * s;
            q[3] = (M(0, 1) - M(1, 0)) * s;
            if (spur == 1.0F && q[1] == 0.0F && q[2] == 0.0F && q[3] == 0.0F) { q[0] = 1.0F; }
        }
    }
    const float l = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (l > 0.0F) {
        for (int a = 0; a < 4; ++a) { q[a] /= l; }
    }
}

void matrixAusQuat(const float qe[4], float m[3][3]) {
    const double l = std::sqrt(static_cast<double>(qe[0]) * qe[0] + static_cast<double>(qe[1]) * qe[1] + static_cast<double>(qe[2]) * qe[2] +
                               static_cast<double>(qe[3]) * qe[3]);
    if (l < 1e-10) {
        einheit(m);
        return;
    }
    const double w = qe[0] / l;
    const double x = qe[1] / l;
    const double y = qe[2] / l;
    const double z = qe[3] / l;
    m[0][0] = static_cast<float>(1.0 - 2.0 * (y * y + z * z));
    m[0][1] = static_cast<float>(2.0 * (x * y - w * z));
    m[0][2] = static_cast<float>(2.0 * (x * z + w * y));
    m[1][0] = static_cast<float>(2.0 * (x * y + w * z));
    m[1][1] = static_cast<float>(1.0 - 2.0 * (x * x + z * z));
    m[1][2] = static_cast<float>(2.0 * (y * z - w * x));
    m[2][0] = static_cast<float>(2.0 * (x * z - w * y));
    m[2][1] = static_cast<float>(2.0 * (y * z + w * x));
    m[2][2] = static_cast<float>(1.0 - 2.0 * (x * x + y * y));
}

void achseWinkelAusQuat(const float qe[4], float aw[4]) {
    float q[4] = {qe[0], qe[1], qe[2], qe[3]};
    const float l = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (l > 0.0F) {
        for (float& v : q) { v /= l; }
    }
    const float ha = std::acos(std::fmax(-1.0F, std::fmin(1.0F, q[0])));
    float si = std::sin(ha);
    aw[0] = ha * 2.0F;
    if (std::fabs(si) < 0.0005F) { si = 1.0F; }
    aw[1] = q[1] / si;
    aw[2] = q[2] / si;
    aw[3] = q[3] / si;
    if (aw[1] == 0.0F && aw[2] == 0.0F && aw[3] == 0.0F) { aw[2] = 1.0F; }
}

void matrixAusAchseWinkel(const float aw[4], float m[3][3]) {
    const double l = std::sqrt(static_cast<double>(aw[1]) * aw[1] + static_cast<double>(aw[2]) * aw[2] + static_cast<double>(aw[3]) * aw[3]);
    if (l < 1e-10) {
        einheit(m);
        return;
    }
    const double h = static_cast<double>(aw[0]) * 0.5;
    const double s = std::sin(h) / l;
    const float q[4] = {static_cast<float>(std::cos(h)), static_cast<float>(aw[1] * s), static_cast<float>(aw[2] * s), static_cast<float>(aw[3] * s)};
    matrixAusQuat(q, m);
}

void drehWerte(const float m[3][3], DrehModus o, float werte[4]) {
    werte[3] = 0.0F;
    if (istEuler(o)) {
        eulerAusMatrix(m, o, werte);
        return;
    }
    float q[4];
    quatAusMatrix(m, q);
    if (o == DrehModus::Quaternion) {
        for (int a = 0; a < 4; ++a) { werte[a] = q[a]; }
        return;
    }
    achseWinkelAusQuat(q, werte);
}

void matrixAusDrehWerten(const float werte[4], DrehModus o, float m[3][3]) {
    if (istEuler(o)) {
        matrixAusEuler(werte, o, m);
    } else if (o == DrehModus::Quaternion) {
        matrixAusQuat(werte, m);
    } else {
        matrixAusAchseWinkel(werte, m);
    }
}

}  // namespace bhed::modell
