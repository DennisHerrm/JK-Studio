// dxttest.cpp - MR4 Punkt 7: DXT5 wie das Spiel (bhed/dxt.h).
// Gemessen im Original-JKA (r_ext_compress_textures 1, "using GL_EXT_texture_compression_s3tc"): tr.defaultImage (16x16, Grau 32,
// Rand 255; R_CreateDefaultImage tr_image.cpp:1281-1310) liegt im Spielfoto als (33, 32, 33) auf dem Bild (Laeufe pj/jka_mit1,
// jka_ohne1); Movie Duels ohne Kompression 32 (vor seiner Gamma-Stufe). 5:6:5: 32 -> 4 (5 Bit) -> 33, 32 -> 8 (6 Bit) -> 32.

#include "bhed/dxt.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
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
    std::printf("dxttest: %s (%d Fehler)\n", g_fehler == 0 ? "ok" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
