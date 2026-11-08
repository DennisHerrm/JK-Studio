// Probe: Glanz und tcGen environment einer skalierten Figur - der Ersatzweg ohne entSkala rechnet wie der Weg mit (nur Windows/MSVC)
//
// FIX6 P50: Die Figurstufe rechnet Glanz und Umgebung im Raum der Entity (R_RotateForEntity, tr_main.cpp:384-403 MD9; MERGE2).
// Bringt der Aufrufer Ursprung und Massstab nicht mit (FigurLicht::entSkala[0] == 0, entityWerte schreibt nichts), nimmt der
// Shader beides aus gFigurWelt, die Achsen gAchse0..2 sind dann die normierten SPALTEN 0, 1, 2 von welt (zeichneFigurIn,
// gpumap_win32.cpp). Die Massstaebe muessen in derselben Reihenfolge stehen - vorher kam |Spalte 1| zu gAchse0.
// Hier laeuft der echte Figur-Vertex-Shader (gpu::figurVertexShaderHlsl, vs_4_0 ohne Fahnen wie gpumap_win32.cpp) auf WARP
// ueber Stream-Output, mit ungleichem Massstab (2, 1, 1) und gedrehten Achsen: einmal mit entSkala, einmal ohne. Die
// Umgebungs-Koordinaten (TEXCOORD0, gStufe.z = 1) muessen bitgleich sein.

#include "bhed/gpushader.h"

#include "vsstrom.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace bhed::gpu;

namespace {

int fehler = 0;
constexpr std::size_t kStufeFloats = 76;   // JeStufe (b3), kStufeFloats in gpumap_win32.cpp

void erwarte(const std::string& was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

// Spalten von welt (Zeilen-Ablage wie zeichneFigurIn): Spalte 0 = axis[0] mal 2, Spalte 1 = axis[1], Spalte 2 = axis[2],
// axis[0] zeigt nach +y (90 Grad gedreht), Ursprung (10, 20, 30).
constexpr float kWelt[16] = {0.0F, -1.0F, 0.0F, 10.0F,   //
                             2.0F, 0.0F, 0.0F, 20.0F,    //
                             0.0F, 0.0F, 1.0F, 30.0F,    //
                             0.0F, 0.0F, 0.0F, 1.0F};

std::vector<float> jeBild() {
    std::vector<float> b(64, 0.0F);
    for (int i = 0; i < 4; ++i) { b[static_cast<std::size_t>(i * 5)] = 1.0F; }   // gViewProj: Einheit
    return b;
}

std::vector<float> jeFigur() {
    std::vector<float> f(1568, 0.0F);
    for (int r = 0; r < 3; ++r) { f[static_cast<std::size_t>(r * 5)] = 1.0F; }   // gKnochen[0..2]: Einheit
    for (int i = 0; i < 16; ++i) { f[1536U + static_cast<std::size_t>(i)] = kWelt[i]; }
    return f;
}

// b3 wie zeichneFigurIn: gStufe.z = 1 (tcGen environment), gAuge, gAchse0..2 = normierte Spalten von welt; mitEnt wie
// entityWerte (gAuge.w und gStufeTurbQ4.zw die Massstaebe, gAchse0..2.w der Ursprung).
std::vector<float> jeStufe(bool mitEnt) {
    std::vector<float> sd(kStufeFloats, 0.0F);
    sd[2] = 1.0F;
    sd[8] = 200.0F;
    sd[9] = -150.0F;
    sd[10] = 120.0F;
    const float achsen[3][3] = {{0.0F, 1.0F, 0.0F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};
    for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < 3; ++j) { sd[static_cast<std::size_t>(12 + k * 4 + j)] = achsen[k][j]; }
    }
    if (mitEnt) {
        sd[11] = 2.0F;
        sd[74] = 1.0F;
        sd[75] = 1.0F;
        for (int k = 0; k < 3; ++k) { sd[static_cast<std::size_t>(15 + k * 4)] = kWelt[k * 4 + 3]; }
    }
    return sd;
}

}  // namespace

int main() {
    std::printf("figurglanz\n");
    std::vector<vsstrom::Ecke> ecken;
    const float orte[][6] = {{4.0F, -3.0F, 10.0F, 0.6F, 0.0F, 0.8F},  {-7.0F, 5.0F, 30.0F, 0.0F, -1.0F, 0.0F},
                             {12.0F, 9.0F, -4.0F, 0.48F, 0.6F, 0.64F}, {0.0F, 0.0F, 50.0F, -0.8F, 0.0F, 0.6F},
                             {-3.0F, -11.0F, 22.0F, 0.0F, 0.6F, -0.8F}};
    for (const auto& o : orte) {
        vsstrom::Ecke e{{o[0], o[1], o[2]}, {o[3], o[4], o[5]}, {0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 0.0F}};
        ecken.push_back(e);
    }
    const std::string quelle = figurVertexShaderHlsl(0);
    std::vector<std::uint32_t> mit;
    std::vector<std::uint32_t> ohne;
    std::string name;
    const bool a = vsstrom::laufen(D3D_DRIVER_TYPE_WARP, quelle, "vs_4_0", 0, vsstrom::Eingabe::Figur,
                                   {{0, jeBild()}, {2, jeFigur()}, {3, jeStufe(true)}}, ecken, mit, name);
    const bool b = vsstrom::laufen(D3D_DRIVER_TYPE_WARP, quelle, "vs_4_0", 0, vsstrom::Eingabe::Figur,
                                   {{0, jeBild()}, {2, jeFigur()}, {3, jeStufe(false)}}, ecken, ohne, name);
    erwarte("Figur-Vertex-Shader laeuft auf WARP (" + name + ")", a && b);
    if (a && b) {
        int gleich = 0;
        for (std::size_t i = 0; i < ecken.size(); ++i) {
            const bool ok = mit[i * 6 + 4] == ohne[i * 6 + 4] && mit[i * 6 + 5] == ohne[i * 6 + 5];
            gleich += ok ? 1 : 0;
            if (!ok) {
                float u[4];
                std::memcpy(&u[0], &mit[i * 6 + 4], 8);
                std::memcpy(&u[2], &ohne[i * 6 + 4], 8);
                std::printf("        Ecke %zu: mit entSkala (%.6f, %.6f), ohne (%.6f, %.6f)\n", i, static_cast<double>(u[0]),
                            static_cast<double>(u[1]), static_cast<double>(u[2]), static_cast<double>(u[3]));
            }
        }
        erwarte("P50: Massstab (2, 1, 1), gedreht - tcGen environment ohne entSkala bitgleich wie mit (" + std::to_string(gleich) +
                    " von " + std::to_string(ecken.size()) + " Ecken)",
                gleich == static_cast<int>(ecken.size()));
    }
    std::printf("%s: %d Fehler\n", fehler == 0 ? "OK" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
