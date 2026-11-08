// Probe: Glanz und tcGen environment einer skalierten Figur - der Ersatzweg ohne entSkala rechnet wie der Weg mit (nur Windows/MSVC)
//
// FIX6 P50: Die Figurstufe rechnet Glanz und Umgebung im Raum der Entity (R_RotateForEntity, tr_main.cpp:384-403 MD9; MERGE2).
// Bringt der Aufrufer Ursprung und Massstab nicht mit (FigurLicht::entSkala[0] == 0, entityWerte schreibt nichts), nimmt der
// Shader beides aus gFigurWelt, die Achsen gAchse0..2 sind dann die normierten SPALTEN 0, 1, 2 von welt (zeichneFigurIn,
// gpumap_win32.cpp). Die Massstaebe muessen in derselben Reihenfolge stehen - vorher kam |Spalte 1| zu gAchse0.
// P61: Traegt welt die Ghoul2-Wurzel (g2WurzelWelt, baueFigurWelt: Spalte 0 = axis[1], Spalte 1 = -axis[0]), ist |Spalte 0| nicht
// |axis[0]|, und die Spalten sind nicht die Achsen der Entity (tess.normal liegt im Spiel samt Wurzel in deren Raum). Dann gibt
// zeichneFigurIn Achsen, Massstab und Ursprung aus gpu::entityAusWelt(welt, true) als Angabe mit - hier dieselbe Funktion.
// P60: Die Lagen in b3 kommen aus gpushader.h (kStufe...At); dieselbe Probe liest sie per D3DReflect aus dem uebersetzten Shader
// (cbLagenPruefen wie pruefeStufenLagen im Programm) - ein Merge, der den Puffer verschiebt, wird hier rot statt still gruen.
// Hier laeuft der echte Figur-Vertex-Shader (gpu::figurVertexShaderHlsl) wie im Programm ab Merkmalsstufe 11_0 (vs_5_0, Fahnen 0,
// kartenVsZiel in gpumap_win32.cpp) auf WARP ueber Stream-Output, mit ungleichem Massstab (2, 1, 1) und gedrehten Achsen: einmal
// mit entSkala, einmal ohne. Die Umgebungs-Koordinaten (TEXCOORD0, gStufe.z = 1) muessen bitgleich sein.

#include "bhed/cblagen_win32.h"
#include "bhed/gpushader.h"
#include "bhed/gpuskin.h"

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
constexpr const char* kProfil = "vs_5_0";   // kartenVsZiel ab D3D_FEATURE_LEVEL_11_0 (WARP kann 11_0)

void erwarte(const std::string& was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

// P50: Spalten von welt (Zeilen-Ablage wie zeichneFigurIn): Spalte 0 = axis[0] mal 2, Spalte 1 = axis[1], Spalte 2 = axis[2],
// axis[0] zeigt nach +y (90 Grad gedreht), Ursprung (10, 20, 30) - G2_GenerateWorldMatrix ohne Wurzel.
constexpr float kWelt[16] = {0.0F, -1.0F, 0.0F, 10.0F,   //
                             2.0F, 0.0F, 0.0F, 20.0F,    //
                             0.0F, 0.0F, 1.0F, 30.0F,    //
                             0.0F, 0.0F, 0.0F, 1.0F};
// P61: dieselbe Entity (axis[0] = (0, 1, 0) mal 2, axis[1] = (-1, 0, 0), axis[2] = (0, 0, 1)) mit der Ghoul2-Wurzel wie
// g2WurzelWelt: Spalte 0 = axis[1] * s1 = (-1, 0, 0), Spalte 1 = -axis[0] * s0 = (0, -2, 0).
constexpr float kWeltWurzel[16] = {-1.0F, 0.0F, 0.0F, 10.0F,   //
                                   0.0F, -2.0F, 0.0F, 20.0F,   //
                                   0.0F, 0.0F, 1.0F, 30.0F,    //
                                   0.0F, 0.0F, 0.0F, 1.0F};
constexpr float kAchsen[3][3] = {{0.0F, 1.0F, 0.0F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};
constexpr float kSkala[3] = {2.0F, 1.0F, 1.0F};
constexpr float kUrsprung[3] = {10.0F, 20.0F, 30.0F};

std::vector<float> jeBild() {
    std::vector<float> b(64, 0.0F);
    for (int i = 0; i < 4; ++i) { b[static_cast<std::size_t>(i * 5)] = 1.0F; }   // gViewProj: Einheit
    return b;
}

std::vector<float> jeFigur(const float* welt) {
    std::vector<float> f(1568, 0.0F);
    for (int r = 0; r < 3; ++r) { f[static_cast<std::size_t>(r * 5)] = 1.0F; }   // gKnochen[0..2]: Einheit
    for (int i = 0; i < 16; ++i) { f[1536U + static_cast<std::size_t>(i)] = welt[i]; }
    return f;
}

// b3 wie zeichneFigurIn (raumWerte): gStufe.z = 1 (tcGen environment), gAuge, gAchse0..2; skala != nullptr wie entityWerte
// (gEntSkala.xyz die Massstaebe, gAchse0..2.w der Ursprung).
std::vector<float> jeStufe(const float (*achsen)[3], const float* skala, const float* ursprung) {
    std::vector<float> sd(kStufeFloats, 0.0F);
    sd[kStufeArtAt + 2U] = 1.0F;
    sd[kStufeAugeAt + 0U] = 200.0F;
    sd[kStufeAugeAt + 1U] = -150.0F;
    sd[kStufeAugeAt + 2U] = 120.0F;
    for (std::size_t k = 0; k < 3; ++k) {
        for (std::size_t j = 0; j < 3; ++j) { sd[kStufeAchse0At + k * 4U + j] = achsen[k][j]; }
    }
    if (skala != nullptr) {
        for (std::size_t k = 0; k < 3; ++k) {
            sd[kStufeEntSkalaAt + k] = skala[k];
            sd[kStufeAchse0At + 3U + k * 4U] = ursprung[k];
        }
    }
    return sd;
}

bool laufen(const std::string& quelle, const float* welt, const std::vector<float>& stufe, const std::vector<vsstrom::Ecke>& ecken,
            std::vector<std::uint32_t>& aus, std::string& name) {
    return vsstrom::laufen(D3D_DRIVER_TYPE_WARP, quelle, kProfil, 0, vsstrom::Eingabe::Figur,
                           {{0, jeBild()}, {2, jeFigur(welt)}, {3, stufe}}, ecken, aus, name);
}

// Zaehlt die Ecken mit bitgleichem TEXCOORD0 und nennt die anderen.
int gleicheEcken(const std::vector<std::uint32_t>& mit, const std::vector<std::uint32_t>& ohne, std::size_t n) {
    int gleich = 0;
    for (std::size_t i = 0; i < n; ++i) {
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
    return gleich;
}

}  // namespace

int main() {
    std::printf("figurglanz\n");
    const std::string quelle = figurVertexShaderHlsl(0);

    // P60: die Lagen von JeStufe aus dem uebersetzten Shader gegen gpushader.h
    {
        ID3DBlob* blob = nullptr;
        ID3DBlob* err = nullptr;
        std::string bericht;
        bool gut = SUCCEEDED(D3DCompile(quelle.data(), quelle.size(), "figur", nullptr, nullptr, "main", kProfil, 0, 0, &blob, &err));
        if (gut) {
            gut = cbLagenPruefen(blob->GetBufferPointer(), blob->GetBufferSize(), "JeStufe", kStufenLagen, kStufeFloats, true, bericht);
        }
        vsstrom::frei(blob);
        vsstrom::frei(err);
        erwarte("P60: JeStufe (b3) im Shader wie kStufenLagen/kStufeFloats -" + bericht, gut);
    }

    std::vector<vsstrom::Ecke> ecken;
    const float orte[][6] = {{4.0F, -3.0F, 10.0F, 0.6F, 0.0F, 0.8F},  {-7.0F, 5.0F, 30.0F, 0.0F, -1.0F, 0.0F},
                             {12.0F, 9.0F, -4.0F, 0.48F, 0.6F, 0.64F}, {0.0F, 0.0F, 50.0F, -0.8F, 0.0F, 0.6F},
                             {-3.0F, -11.0F, 22.0F, 0.0F, 0.6F, -0.8F}};
    for (const auto& o : orte) {
        vsstrom::Ecke e{{o[0], o[1], o[2]}, {o[3], o[4], o[5]}, {0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 0.0F}};
        ecken.push_back(e);
    }
    std::string name;

    // P50: welt ohne Wurzel - zeichneFigurIn nimmt die normierten Spalten (entityAusWelt ohne Wurzel), der Shader den Massstab aus
    // gFigurWelt; gegen dieselben Achsen mit entSkala
    {
        float achsen[3][3];
        float skala[3];
        float ursprung[3];
        entityAusWelt(kWelt, false, achsen, skala, ursprung);
        std::vector<std::uint32_t> mit;
        std::vector<std::uint32_t> ohne;
        const bool a = laufen(quelle, kWelt, jeStufe(achsen, kSkala, kUrsprung), ecken, mit, name);
        const bool b = laufen(quelle, kWelt, jeStufe(achsen, nullptr, nullptr), ecken, ohne, name);
        erwarte("Figur-Vertex-Shader laeuft auf WARP (" + name + ", " + kProfil + ")", a && b);
        if (a && b) {
            const int gleich = gleicheEcken(mit, ohne, ecken.size());
            erwarte("P50: Massstab (2, 1, 1), gedreht - tcGen environment ohne entSkala bitgleich wie mit (" + std::to_string(gleich) +
                        " von " + std::to_string(ecken.size()) + " Ecken)",
                    gleich == static_cast<int>(ecken.size()));
        }
    }

    // P61: welt mit Wurzel (g2WurzelWelt/baueFigurWelt) - zeichneFigurIn gibt entityAusWelt(welt, true) als Angabe mit; gegen die
    // Achsen der Entity mit entSkala wie bei einer Figur (achsenGesetzt)
    {
        float achsen[3][3];
        float skala[3];
        float ursprung[3];
        entityAusWelt(kWeltWurzel, true, achsen, skala, ursprung);
        std::vector<std::uint32_t> mit;
        std::vector<std::uint32_t> ohne;
        const bool a = laufen(quelle, kWeltWurzel, jeStufe(kAchsen, kSkala, kUrsprung), ecken, mit, name);
        const bool b = laufen(quelle, kWeltWurzel, jeStufe(achsen, skala, ursprung), ecken, ohne, name);
        erwarte("Figur-Vertex-Shader laeuft auf WARP (Wurzel)", a && b);
        if (a && b) {
            const int gleich = gleicheEcken(mit, ohne, ecken.size());
            erwarte("P61: Wurzel in welt, Massstab (2, 1, 1) - tcGen environment aus welt bitgleich wie mit den Achsen der Entity (" +
                        std::to_string(gleich) + " von " + std::to_string(ecken.size()) + " Ecken)",
                    gleich == static_cast<int>(ecken.size()));
        }
    }
    std::printf("%s: %d Fehler\n", fehler == 0 ? "OK" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
