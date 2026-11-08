// dxttest.cpp - MR4 Punkt 7: DXT5 wie das Spiel (bhed/dxt.h).
// Gemessen im Original-JKA (r_ext_compress_textures 1, "using GL_EXT_texture_compression_s3tc"): tr.defaultImage (16x16, Grau 32,
// Rand 255; R_CreateDefaultImage tr_image.cpp:1281-1310) liegt im Spielfoto als (33, 32, 33) auf dem Bild (Laeufe pj/jka_mit1,
// jka_ohne1); Movie Duels ohne Kompression 32 (vor seiner Gamma-Stufe). 5:6:5: 32 -> 4 (5 Bit) -> 33, 32 -> 8 (6 Bit) -> 32.
//
// MR5 Punkt 2: der schnelle Weg (Arbeitsfaeden, einfarbige Bloecke direkt, alle Stufen in einem Auftrag) muss Byte fuer Byte
// dasselbe liefern wie die erste Fassung aus MR4 (unten als `referenz` wortgleich abgelegt) - an Zufallsbildern, Verlaeufen,
// Flaechen, Teilbloecken und einer ganzen Pyramide.

#include "bhed/dxt.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

namespace {

int g_fehler = 0;

void pruefe(bool ok, const char* was) {
    if (!ok) {
        std::printf("FEHLER: %s\n", was);
        ++g_fehler;
    }
}

std::vector<std::uint8_t> standardBild() {
    constexpr int k = 16;
    std::vector<std::uint8_t> b(static_cast<std::size_t>(k * k * 4), 32);
    for (int x = 0; x < k; ++x) {
        for (const int p : {x, x * k, (k - 1) * k + x, x * k + k - 1}) {
            for (int c = 0; c < 4; ++c) { b[static_cast<std::size_t>(p * 4 + c)] = 255; }
        }
    }
    return b;
}

// --- Die Fassung aus MR4 (src/dxt.cpp, c5d16c76b), unveraendert: Massstab fuer "bytegleich" ---------------------------------
namespace referenz {

int q5(int v) { return (v * 31 + 127) / 255; }
int q6(int v) { return (v * 63 + 127) / 255; }
int e5(int q) { return (q << 3) | (q >> 2); }
int e6(int q) { return (q << 2) | (q >> 4); }

struct Punkt {
    std::uint8_t* p = nullptr;
};

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
    for (int it = 0; it < 8; ++it) {
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

}  // namespace referenz

// Ein Pruefbild: Rauschen, Verlauf, einfarbige Flaechen (auch nur in der Farbe gleich, Alpha verschieden), Kanten
std::vector<std::uint8_t> pruefBild(int w, int h, unsigned saat) {
    std::mt19937 z(saat);
    std::vector<std::uint8_t> b(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            std::uint8_t* p = &b[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U];
            const int art = ((x / 8) + (y / 8) * 3 + static_cast<int>(saat)) % 5;
            for (int c = 0; c < 4; ++c) {
                int v = 0;
                switch (art) {
                    case 0: v = static_cast<int>(z() & 255U); break;                                     // Rauschen
                    case 1: v = (x * 7 + y * 3 + c * 40) & 255; break;                                   // Verlauf
                    case 2: v = 32 + c * 50; break;                                                      // Flaeche
                    case 3: v = c == 3 ? static_cast<int>(z() & 255U) : 200 - c * 30; break;             // Farbe gleich, Alpha nicht
                    default: v = ((x + y) & 4) != 0 ? 250 - c : 5 + c + static_cast<int>(z() % 3U); break;   // harte Kanten
                }
                p[c] = static_cast<std::uint8_t>(v);
            }
        }
    }
    return b;
}

// Wie Tex::buildMips (Kasten 2x2), nur damit die Pyramide echte Groessen hat - der Inhalt ist hier gleichgueltig
std::vector<std::vector<std::uint8_t>> pyramide(const std::vector<std::uint8_t>& b0, int w, int h, std::vector<std::array<int, 2>>& groesse) {
    std::vector<std::vector<std::uint8_t>> r{b0};
    groesse = {{w, h}};
    while (w > 1 || h > 1) {
        const int nw = std::max(1, w / 2);
        const int nh = std::max(1, h / 2);
        std::vector<std::uint8_t> n(static_cast<std::size_t>(nw) * static_cast<std::size_t>(nh) * 4U);
        const std::vector<std::uint8_t>& a = r.back();
        for (int y = 0; y < nh; ++y) {
            for (int x = 0; x < nw; ++x) {
                for (int c = 0; c < 4; ++c) {
                    const int x0 = std::min(x * 2, w - 1);
                    const int x1 = std::min(x * 2 + 1, w - 1);
                    const int y0 = std::min(y * 2, h - 1);
                    const int y1 = std::min(y * 2 + 1, h - 1);
                    auto at = [&](int xx, int yy) { return static_cast<int>(a[(static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(xx)) * 4U + static_cast<std::size_t>(c)]); };
                    n[(static_cast<std::size_t>(y) * static_cast<std::size_t>(nw) + static_cast<std::size_t>(x)) * 4U + static_cast<std::size_t>(c)] =
                        static_cast<std::uint8_t>((at(x0, y0) + at(x1, y0) + at(x0, y1) + at(x1, y1) + 2) / 4);
                }
            }
        }
        r.push_back(std::move(n));
        groesse.push_back({nw, nh});
        w = nw;
        h = nh;
    }
    return r;
}

}  // namespace

int main() {
    std::vector<std::uint8_t> b = standardBild();
    bhed::dxt::dxt5Rundlauf(b.data(), 16, 16);
    // Mitte (Block ganz grau) und Punkt neben dem Rand (Block mit Weiss)
    const std::uint8_t* mitte = &b[static_cast<std::size_t>((6 * 16 + 6) * 4)];
    const std::uint8_t* rand = &b[static_cast<std::size_t>((1 * 16 + 1) * 4)];
    const std::uint8_t* weiss = &b[0];
    pruefe(mitte[0] == 33 && mitte[1] == 32 && mitte[2] == 33 && mitte[3] == 32, "Grau 32 im grauen Block -> 33 32 33 (wie das Spielfoto), Alpha 32 bleibt (memset 32)");
    pruefe(rand[0] == 33 && rand[1] == 32 && rand[2] == 33, "Grau 32 neben dem weissen Rand -> 33 32 33");
    pruefe(weiss[0] == 255 && weiss[1] == 255 && weiss[2] == 255 && weiss[3] == 255, "Weiss bleibt 255");
    std::printf("Mitte %d %d %d, am Rand %d %d %d\n", mitte[0], mitte[1], mitte[2], rand[0], rand[1], rand[2]);
    // Alpha: zwei Werte genau, Zwischenwerte auf acht Stufen
    std::vector<std::uint8_t> a(4 * 4 * 4, 128);
    for (int i = 0; i < 16; ++i) { a[static_cast<std::size_t>(i * 4 + 3)] = static_cast<std::uint8_t>(i < 8 ? 0 : 255); }
    a[3] = 100;
    bhed::dxt::dxt5Rundlauf(a.data(), 4, 4);
    pruefe(a[7] == 0 && a[63] == 255, "Alpha 0 und 255 bleiben");
    pruefe(a[3] == 109, "Alpha 100 -> naechste Stufe (6*0+... : 109)");
    std::printf("Alpha 100 -> %d\n", a[3]);
    // Teilblock (3x2)
    std::vector<std::uint8_t> t(3 * 2 * 4, 200);
    bhed::dxt::dxt5Rundlauf(t.data(), 3, 2);
    pruefe(t[0] == 198 && t[1] == 199 && t[2] == 198 && t[23] == 200, "Teilblock 3x2: Grau 200 -> 198 199 198, Alpha bleibt");
    std::printf("200 -> %d %d %d\n", t[0], t[1], t[2]);

    // MR5 Punkt 2: bytegleich mit der Fassung aus MR4 - einzelne Bilder aller Groessen
    int bilder = 0;
    int abweichend = 0;
    const std::array<std::array<int, 2>, 12> groessen{{{1, 1}, {2, 3}, {3, 2}, {4, 4}, {5, 7}, {13, 9}, {16, 16}, {31, 17}, {64, 64}, {100, 37}, {256, 128}, {512, 512}}};
    for (unsigned saat = 1; saat <= 4; ++saat) {
        for (const auto& g : groessen) {
            std::vector<std::uint8_t> soll = pruefBild(g[0], g[1], saat);
            std::vector<std::uint8_t> ist = soll;
            referenz::dxt5Rundlauf(soll.data(), g[0], g[1]);
            bhed::dxt::dxt5Rundlauf(ist.data(), g[0], g[1]);
            ++bilder;
            abweichend += soll == ist ? 0 : 1;
        }
    }
    std::printf("Einzelbilder: %d, davon abweichend %d\n", bilder, abweichend);
    pruefe(abweichend == 0, "Einzelbilder bytegleich mit der Fassung aus MR4");

    // ... und eine ganze Pyramide in einem Auftrag (wie ladeMitStufen), auch ungerade Seiten
    int stufenAbw = 0;
    int stufen = 0;
    for (const auto& g : std::array<std::array<int, 2>, 3>{{{1024, 1024}, {300, 77}, {8, 512}}}) {
        std::vector<std::array<int, 2>> gr;
        std::vector<std::vector<std::uint8_t>> soll = pyramide(pruefBild(g[0], g[1], 7), g[0], g[1], gr);
        std::vector<std::vector<std::uint8_t>> ist = soll;
        std::vector<bhed::dxt::Ebene> ebenen;
        for (std::size_t i = 0; i < soll.size(); ++i) {
            referenz::dxt5Rundlauf(soll[i].data(), gr[i][0], gr[i][1]);
            ebenen.push_back({ist[i].data(), gr[i][0], gr[i][1]});
        }
        bhed::dxt::dxt5Rundlauf(ebenen.data(), ebenen.size());
        for (std::size_t i = 0; i < soll.size(); ++i) {
            ++stufen;
            stufenAbw += soll[i] == ist[i] ? 0 : 1;
        }
    }
    std::printf("Pyramiden: %d Stufen, davon abweichend %d\n", stufen, stufenAbw);
    pruefe(stufenAbw == 0, "Pyramide in einem Auftrag bytegleich mit der Fassung aus MR4");

    // Zeit (nur Auskunft, keine Pruefung: das Ergebnis haengt an der Last des Rechners): 1024x1024 samt Pyramide
    {
        std::vector<std::array<int, 2>> gr;
        const std::vector<std::vector<std::uint8_t>> roh = pyramide(pruefBild(1024, 1024, 11), 1024, 1024, gr);
        double alt = 1e30;
        double neu = 1e30;
        for (int runde = 0; runde < 3; ++runde) {
            std::vector<std::vector<std::uint8_t>> x = roh;
            auto t0 = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < x.size(); ++i) { referenz::dxt5Rundlauf(x[i].data(), gr[i][0], gr[i][1]); }
            auto t1 = std::chrono::steady_clock::now();
            alt = std::min(alt, std::chrono::duration<double, std::milli>(t1 - t0).count());
            std::vector<std::vector<std::uint8_t>> y = roh;
            std::vector<bhed::dxt::Ebene> ebenen;
            for (std::size_t i = 0; i < y.size(); ++i) { ebenen.push_back({y[i].data(), gr[i][0], gr[i][1]}); }
            t0 = std::chrono::steady_clock::now();
            bhed::dxt::dxt5Rundlauf(ebenen.data(), ebenen.size());
            t1 = std::chrono::steady_clock::now();
            neu = std::min(neu, std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        std::printf("Zeit 1024x1024 samt Pyramide: MR4 %.2f ms, jetzt %.2f ms\n", alt, neu);
    }

    std::printf("dxttest: %s (%d Fehler)\n", g_fehler == 0 ? "ok" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
