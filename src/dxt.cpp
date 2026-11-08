// dxt.cpp - siehe bhed/dxt.h

#include "bhed/dxt.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace bhed::dxt {

namespace {

// 8 Bit -> 5/6 Bit (naechster Wert) und zurueck wie der Chip (Bits oben unten wiederholt)
int q5(int v) { return (v * 31 + 127) / 255; }
int q6(int v) { return (v * 63 + 127) / 255; }
int e5(int q) { return (q << 3) | (q >> 2); }
int e6(int q) { return (q << 2) | (q >> 4); }

struct Punkt {
    std::uint8_t* p = nullptr;
};

// Farbblock: zwei Endfarben in 5:6:5 auf der Hauptachse der Punkte, dazu (2a+b)/3 und (a+2b)/3; jeder Punkt die naechste
void farbe(std::array<Punkt, 16>& b, int n) {
    double m[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < 3; ++k) { m[k] += b[i].p[k]; }
    }
    for (double& x : m) { x /= n; }
    double c[3][3] = {};
    for (int i = 0; i < n; ++i) {
        const double d[3] = {b[i].p[0] - m[0], b[i].p[1] - m[1], b[i].p[2] - m[2]};
        for (int r = 0; r < 3; ++r) {
            for (int s = 0; s < 3; ++s) { c[r][s] += d[r] * d[s]; }
        }
    }
    double a[3] = {1, 1, 1};
    for (int it = 0; it < 8; ++it) {   // Potenzverfahren: groesste Streuung
        const double v[3] = {c[0][0] * a[0] + c[0][1] * a[1] + c[0][2] * a[2], c[1][0] * a[0] + c[1][1] * a[1] + c[1][2] * a[2],
                             c[2][0] * a[0] + c[2][1] * a[1] + c[2][2] * a[2]};
        const double l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l < 1e-9) { break; }
        for (int k = 0; k < 3; ++k) { a[k] = v[k] / l; }
    }
    double tMin = 1e30;
    double tMax = -1e30;
    for (int i = 0; i < n; ++i) {
        const double t = (b[i].p[0] - m[0]) * a[0] + (b[i].p[1] - m[1]) * a[1] + (b[i].p[2] - m[2]) * a[2];
        tMin = std::min(tMin, t);
        tMax = std::max(tMax, t);
    }
    int ende[2][3];
    for (int e = 0; e < 2; ++e) {
        const double t = e == 0 ? tMax : tMin;
        for (int k = 0; k < 3; ++k) {
            const int v = std::clamp(static_cast<int>(std::lround(m[k] + a[k] * t)), 0, 255);
            ende[e][k] = k == 1 ? e6(q6(v)) : e5(q5(v));
        }
    }
    int stufe[4][3];
    for (int k = 0; k < 3; ++k) {
        stufe[0][k] = ende[0][k];
        stufe[1][k] = ende[1][k];
        stufe[2][k] = (2 * ende[0][k] + ende[1][k] + 1) / 3;
        stufe[3][k] = (ende[0][k] + 2 * ende[1][k] + 1) / 3;
    }
    for (int i = 0; i < n; ++i) {
        int best = 0;
        int bestD = 1 << 30;
        for (int s = 0; s < 4; ++s) {
            const int dr = b[i].p[0] - stufe[s][0];
            const int dg = b[i].p[1] - stufe[s][1];
            const int db = b[i].p[2] - stufe[s][2];
            const int d = dr * dr + dg * dg + db * db;
            if (d < bestD) {
                bestD = d;
                best = s;
            }
        }
        for (int k = 0; k < 3; ++k) { b[i].p[k] = static_cast<std::uint8_t>(stufe[best][k]); }
    }
}

// Alphablock: groesster und kleinster Wert, sechs Stufen dazwischen (a0 > a1); gleiche Werte bleiben genau
void alpha(std::array<Punkt, 16>& b, int n) {
    int lo = 255;
    int hi = 0;
    for (int i = 0; i < n; ++i) {
        lo = std::min<int>(lo, b[i].p[3]);
        hi = std::max<int>(hi, b[i].p[3]);
    }
    if (lo == hi) { return; }
    int stufe[8] = {hi, lo, 0, 0, 0, 0, 0, 0};
    for (int s = 1; s <= 6; ++s) { stufe[s + 1] = ((7 - s) * hi + s * lo + 3) / 7; }
    for (int i = 0; i < n; ++i) {
        int best = 0;
        for (int s = 1; s < 8; ++s) {
            if (std::abs(b[i].p[3] - stufe[s]) < std::abs(b[i].p[3] - stufe[best])) { best = s; }
        }
        b[i].p[3] = static_cast<std::uint8_t>(stufe[best]);
    }
}

}  // namespace

void dxt5Rundlauf(std::uint8_t* rgba, int breite, int hoehe) {
    if (rgba == nullptr || breite <= 0 || hoehe <= 0) { return; }
    std::array<Punkt, 16> b{};
    for (int by = 0; by < hoehe; by += 4) {
        for (int bx = 0; bx < breite; bx += 4) {
            int n = 0;
            for (int y = by; y < std::min(by + 4, hoehe); ++y) {
                for (int x = bx; x < std::min(bx + 4, breite); ++x) {
                    b[static_cast<std::size_t>(n++)].p = rgba + (static_cast<std::size_t>(y) * static_cast<std::size_t>(breite) + static_cast<std::size_t>(x)) * 4U;
                }
            }
            farbe(b, n);
            alpha(b, n);
        }
    }
}

}  // namespace bhed::dxt
