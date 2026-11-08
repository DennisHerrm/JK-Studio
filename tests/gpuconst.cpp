// Probe fuer das Packen der Konstanten.
//
// Was hier schiefgehen kann
// -------------------------
// HLSL sortiert alles in Bloecke zu vier float. Ein `float gDeform[4]` war
// die Falle: vier Viererbloecke, aber das letzte Element nur mit .x belegt -
// das naechste Skalar lag in .y dahinter (29), nicht bei 32. Seit rc568 ist
// es ein float4. Wer die Lagen verschiebt, bekommt keinen Absturz, sondern
// ein Bild, in dem etwas falsch wandert oder nur pulsiert.
//
// Auf dem Rechner mit Grafikkarte waere das sehr schwer einzukreisen. Hier
// ist es eine Zahl, die man vergleichen kann.

#include "bhed/gpuconst.h"

#include "bhed/gpushader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::gpu;

namespace {

int fehler = 0;

void erwarte(const char* was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was);
    if (!ok) {
        ++fehler;
    }
}

// FIX2 P16: die tcMod-Kette wie rd-vanilla - ComputeTexCoords (code/rd-vanilla/tr_shade.cpp:1725ff) wendet jeden Schritt der
// Reihe nach an, turb (RB_CalcTurbulentTexCoords, tr_shade_calc.cpp:936) und rotate (RB_CalcRotateTexCoords :1009) ueber
// tr.sinTable (tr_init.cpp:1727: sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1)))), 1024 Eintraege, Index & FUNCTABLE_MASK).
// FIX4 P33: alles in float wie dort - DEG2RAD mit PI_DIV_180 als float (shared/qcommon/q_math.h:85-88), sin ueber die
// float-Ueberladung aus <cmath> (code/qcommon/q_shared.h:120) = sinf.
float sinTab(int i) {
    return std::sin(static_cast<float>(i & 1023) * 360.0F / static_cast<float>(1024 - 1) * 0.017453292519943295769236907684886F);
}
// FIX4 P33: der Versatz EINES turb an einer Ecke wie RB_CalcTurbulentTexCoords (tr_shade_calc.cpp:948): p = x + z (float) bzw. y,
// ((p * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) in double, (int), & MASK, mal amplitude in float
float spielTurb(float p, float now, float amp) { return sinTab(static_cast<int>((p * 1.0 / 128 * 0.125 + now) * 1024)) * amp; }
bool bitgleich(float a, float b) {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}
void spielSt(const TexMod* m, int n, float zeit, const float xyz[3], float st[2]) {
    for (int k = 0; k < n; ++k) {
        const TexMod& t = m[k];
        switch (t.kind) {
            case TexModKind::Turb: {
                const float now = t.wave.phase + zeit * t.wave.frequency;
                const float s = st[0];
                const float u = st[1];
                st[0] = s + sinTab(static_cast<int>(((xyz[0] + xyz[2]) * 1.0 / 128 * 0.125 + now) * 1024)) * t.wave.amplitude;
                st[1] = u + sinTab(static_cast<int>((xyz[1] * 1.0 / 128 * 0.125 + now) * 1024)) * t.wave.amplitude;
                break;
            }
            case TexModKind::Scale:
                st[0] *= t.a[0];
                st[1] *= t.a[1];
                break;
            case TexModKind::Scroll: {
                float a = t.a[0] * zeit;
                float b = t.a[1] * zeit;
                a = a - std::floor(a);
                b = b - std::floor(b);
                st[0] += a;
                st[1] += b;
                break;
            }
            case TexModKind::Rotate: {
                const float degs = -t.a[0] * zeit;
                const int index = static_cast<int>(degs * (1024 / 360.0F));
                const float sn = sinTab(index);
                const float cs = sinTab(index + 1024 / 4);
                const float s = st[0];
                const float u = st[1];
                // tmi.translate in double gerechnet, als float abgelegt; RB_CalcTransformTexCoords (:997) s * m00 + t * m10 + tr
                const auto tr0 = static_cast<float>(0.5 - 0.5 * cs + 0.5 * sn);
                const auto tr1 = static_cast<float>(0.5 - 0.5 * sn - 0.5 * cs);
                st[0] = s * cs + u * -sn + tr0;
                st[1] = s * sn + u * cs + tr1;
                break;
            }
            case TexModKind::Transform: {
                const float s = st[0];
                const float u = st[1];
                st[0] = s * t.a[0] + u * t.a[2] + t.a[4];
                st[1] = s * t.a[1] + u * t.a[3] + t.a[5];
                break;
            }
            default: break;
        }
    }
}
// Der Shader (gpushader.cpp turbIndex), Zeile fuer Zeile in float; die Grafikkarte selbst prueft tests/turbgpu.cpp
int shaderIndex(float p, float q) {
    const float s = p + q;
    const float b = s - p;
    const float e = (p - (s - b)) + (q - b);
    const float a = std::fabs(s);
    const float ea = (s < 0.0F) ? -e : e;
    if (!(a < 2147483648.0F)) { return 0; }
    const float g = std::floor(a);
    auto r = static_cast<std::uint32_t>(g);
    if (a == g && a > 0.0F) {
        const float f = std::floor(ea);
        const float c = f + 1.0F;
        std::uint32_t u = 0;
        std::memcpy(&u, &a, 4);
        const std::uint32_t h = ((u >> 23U) - ((((u & 0x7FFFFFU) == 0U) && (c <= 0.0F)) ? 54U : 53U)) << 23U;
        float hf = 0.0F;
        std::memcpy(&hf, &h, 4);
        r += static_cast<std::uint32_t>(static_cast<int>((c - ea <= hf) ? c : f));
        if (r >= 0x80000000U) { return 0; }
    }
    const auto i = static_cast<int>(r);
    return ((s < 0.0F) ? -i : i) & 1023;
}
// Der Shader (gpushader.cpp turbVersatz): sinTable[index] * amp
void shaderTurb(const float ort[2], float q, float amp, float v[2]) {
    v[0] = sinTafel(shaderIndex(ort[0], q)) * amp;
    v[1] = sinTafel(shaderIndex(ort[1], q)) * amp;
}
// Was die Shader aus dem gepackten Block machen (gpushader.cpp tcSchritt/tcKette; JeStufe und karte_gpu_win32.cpp stufeSt lesen
// dieselben Werte): FIX4 P33 jeder Schritt der Reihe nach mit seiner Art aus gTcArt
void shaderSt(const TexMod* m, int n, float zeit, const float xyz[3], float st[2]) {
    BatchState bs{};
    bs.numTexMods = n;
    for (int i = 0; i < n; ++i) { bs.texMods[i] = m[i]; }
    std::vector<float> k(kKonstFloats, 0.0F);
    packeKonstanten(bs, k.data(), zeit);
    const float ort[2] = {xyz[0] + xyz[2], xyz[1]};
    const std::size_t mAt[4] = {kTcM0At, kTcM1At, kTcM2At, kTcM3At};
    const std::size_t oAt[4] = {kTcO01At, kTcO01At + 2U, kTcO23At, kTcO23At + 2U};
    for (int i = 0; i < 4; ++i) {
        const float art = k[kTcArtAt + static_cast<std::size_t>(i)];
        const float* mm = &k[mAt[i]];
        const float* oo = &k[oAt[i]];
        const float s = st[0];
        const float t = st[1];
        if (art == 1.0F) {
            st[0] = s * mm[0] + t * mm[2] + oo[0];
            st[1] = s * mm[1] + t * mm[3] + oo[1];
        } else if (art == 2.0F) {
            float v[2];
            shaderTurb(ort, mm[0], mm[1], v);
            st[0] = s + v[0];
            st[1] = t + v[1];
        } else if (art == 3.0F) {
            st[0] = s * mm[0];
            st[1] = t * mm[1];
        } else if (art == 4.0F) {
            st[0] = s + oo[0];
            st[1] = t + oo[1];
        }
    }
}

}  // namespace

int main() {
    std::printf("gpuconst\n");

    // --- Die Lage der Felder ---------------------------------------------
    //
    // Diese Zahlen stehen doppelt: hier und im cbuffer in gpushader.cpp.
    // Genau deshalb werden sie geprueft - eine Verschiebung faellt sonst
    // erst als schiefes Bild auf.
    erwarte("gTcM0 steht bei 0", kTcM0At == 0U);
    erwarte("gTcO01 steht bei 4", kTcO01At == 4U);
    erwarte("gRgb steht bei 8", kRgbAt == 8U);
    erwarte("gAlphaSchwelle steht bei 12", kAlphaAt == 12U);
    // Ein float4 beginnt auf einer Viererblockgrenze: nach
    // gAlphaSchwelle (12) waere 13 frei, HLSL schiebt auf 16.
    erwarte("gDeform steht bei 16, nicht bei 13", kDeformAt == 16U);
    // float4 gDeform: Spread gleich dahinter. Mit `float gDeform[4]` lag er
    // bei 29 (D3DReflect), gerechnet war 32 - der Lavafall pulsierte nur.
    erwarte("gDeformSpread steht bei 20", kDeformSpreadAt == 20U);
    erwarte("gWolkenHoehe gleich dahinter bei 21", kWolkenAt == 21U);
    // rc605: gTcGenS/gTcGenT; FIX4 P33: gTcM1..3, gTcO23, gTcArt (die tcMod-Kette je Schritt) - genau ein Eintrag im Ring (256 Bytes)
    erwarte("der Block ist 64 float gross", kKonstFloats == 64U);
    erwarte("gTcM1..3 stehen bei 44, 48, 52 (FIX4 P33)", kTcM1At == 44U && kTcM2At == 48U && kTcM3At == 52U);
    erwarte("gTcO23 bei 56, gTcArt bei 60 (FIX4 P33)", kTcO23At == 56U && kTcArtAt == 60U);
    {
        // rc605: tcGen vector - die zwei Vektoren landen bei 36 und 40, der
        // Vertexshader rechnet st aus dem Ort.
        BatchState bs{};
        bs.texGen = TexGen::Vector;
        const float v[6] = {0.0F, 0.0009765625F, 0.0F, 0.0F, 0.0F, -0.0009765625F};
        for (int k = 0; k < 6; ++k) { bs.tcGenVec[k] = v[k]; }
        std::vector<float> f(kKonstFloats, -1.0F);
        packeKonstanten(bs, f.data(), 0.0F);
        erwarte("tcGen vector: s-Vektor bei 36", f[kTcGenSAt + 1] == v[1] && f[kTcGenSAt + 0] == 0.0F && f[kTcGenSAt + 2] == 0.0F);
        erwarte("tcGen vector: t-Vektor bei 40", f[kTcGenTAt + 2] == v[5] && f[kTcGenTAt + 1] == 0.0F);
        erwarte("tcGen vector: der Shader rechnet dot(welt, ...)",
                vertexShaderHlsl(kTexGenVec, 0).find("dot(welt, gTcGenS.xyz)") != std::string::npos);
    }
    erwarte("der Shader hat gDeform als float4",
            vertexShaderHlsl(kDeform, 0).find("float4 gDeform;") != std::string::npos);
    erwarte("und der Shader kennt keine eigene Modellmatrix",
            vertexShaderHlsl(0U, 0).find("gModellWelt") == std::string::npos);

    std::vector<float> buf(kKonstFloats + 8U, -99.0F);

    // --- Alles wird geschrieben, nichts bleibt stehen ---------------------
    //
    // Ein nicht beschriebenes Feld enthielte den Wert des vorigen Stapels -
    // ein Fehler, der nur manchmal auftritt.
    {
        BatchState bs;
        packeKonstanten(bs, buf.data(), 0.0F);
        bool sauber = true;
        for (std::size_t i = 0; i < kKonstFloats; ++i) {
            if (buf[i] == -99.0F) {
                sauber = false;
            }
        }
        erwarte("kein Feld bleibt unbeschrieben", sauber);
        erwarte("und dahinter wird nichts angefasst",
                buf[kKonstFloats] == -99.0F);
    }

    // --- Die tcMod-Kette je Schritt (FIX4 P33) ------------------------------
    //
    // Wie rd-vanilla ComputeTexCoords: jeder Schritt mit seiner Art und den Werten, die das Spiel an seine Rechenfunktion gibt
    // (bis FIX3 zusammengerechnet wie rend2).
    {
        BatchState bs;
        bs.numTexMods = 2;
        bs.texMods[0].kind = TexModKind::Scroll;
        bs.texMods[0].a[0] = 0.25F;
        bs.texMods[0].a[1] = 0.5F;
        bs.texMods[1].kind = TexModKind::Turb;
        bs.texMods[1].wave.func = WaveFunc::Sin;
        bs.texMods[1].wave.amplitude = 0.3F;
        bs.texMods[1].wave.phase = 1.0F;
        bs.texMods[1].wave.frequency = 0.1F;
        packeKonstanten(bs, buf.data(), 2.0F);
        erwarte("Arten: scroll, turb, keine, keine",
                buf[kTcArtAt + 0] == kTcRoll && buf[kTcArtAt + 1] == kTcTurb && buf[kTcArtAt + 2] == kTcKeine && buf[kTcArtAt + 3] == kTcKeine);
        // scroll gekuerzt auf [0,1): 0.25*2 = 0.5, 0.5*2 = 1.0 -> 0.0
        erwarte("scroll steht in gTcO01.xy, gekuerzt", buf[kTcO01At + 0] == 0.5F && buf[kTcO01At + 1] == 0.0F);
        // now = phase + zeit * frequenz = 1.0 + 2.0*0.1 = 1.2: 1024 now (in float wie now im Spiel) und Amplitude
        erwarte("turb steht in gTcM1: 1024 now und Amplitude",
                buf[kTcM1At + 0] == (1.0F + 2.0F * 0.1F) * 1024.0F && buf[kTcM1At + 1] == 0.3F);
    }
    {
        // tcMod transform: sechs Werte, bis rc442 auf null (13 Vorkommen in 43 Karten) - je Schritt mit Matrix und Verschiebung
        BatchState bs;
        bs.numTexMods = 1;
        bs.texMods[0].kind = TexModKind::Transform;
        bs.texMods[0].a[0] = 2.0F;   // m00
        bs.texMods[0].a[1] = 3.0F;   // m01
        bs.texMods[0].a[2] = 4.0F;   // m10
        bs.texMods[0].a[3] = 5.0F;   // m11
        bs.texMods[0].a[4] = 6.0F;   // tx
        bs.texMods[0].a[5] = 7.0F;   // ty
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("transform: Matrix in gTcM0", buf[kTcM0At + 0] == 2.0F && buf[kTcM0At + 1] == 3.0F && buf[kTcM0At + 2] == 4.0F && buf[kTcM0At + 3] == 5.0F);
        erwarte("transform: Verschiebung in gTcO01.xy, Art affin", buf[kTcO01At + 0] == 6.0F && buf[kTcO01At + 1] == 7.0F && buf[kTcArtAt] == kTcAffin);
    }
    {
        // Zwei Schritte hintereinander: erst doppelt so gross, dann verschoben - in der Reihenfolge des Shaderskripts
        BatchState bs;
        bs.numTexMods = 2;
        bs.texMods[0].kind = TexModKind::Scale;
        bs.texMods[0].a[0] = 2.0F;
        bs.texMods[0].a[1] = 2.0F;
        bs.texMods[1].kind = TexModKind::Scroll;
        bs.texMods[1].a[0] = 0.25F;
        bs.texMods[1].a[1] = 0.0F;
        packeKonstanten(bs, buf.data(), 1.0F);
        erwarte("scale zuerst (gTcM0.xy)", buf[kTcArtAt + 0] == kTcSkal && buf[kTcM0At + 0] == 2.0F && buf[kTcM0At + 1] == 2.0F);
        erwarte("das spaetere scroll danach (gTcO01.zw)", buf[kTcArtAt + 1] == kTcRoll && buf[kTcO01At + 2] == 0.25F);
    }
    {
        // Ohne tcMod: keine Schritte - der Shader laesst st, wie es ist
        BatchState bs;
        packeKonstanten(bs, buf.data(), 5.0F);
        erwarte("ohne tcMod: alle Arten keine",
                buf[kTcArtAt + 0] == kTcKeine && buf[kTcArtAt + 1] == kTcKeine && buf[kTcArtAt + 2] == kTcKeine && buf[kTcArtAt + 3] == kTcKeine);
    }

    // --- rgbGen ------------------------------------------------------------
    {
        BatchState bs;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("ohne rgbGen ist die Farbe weiss",
                buf[kRgbAt] == 1.0F && buf[kRgbAt + 1] == 1.0F);
        erwarte("und alphaConst voll", buf[kRgbAt + 3] == 1.0F);

        bs.haveRgbConst = true;
        bs.rgbConst[0] = 0.25F;
        bs.rgbConst[1] = 0.5F;
        bs.rgbConst[2] = 0.75F;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("rgbGen const kommt an",
                buf[kRgbAt] == 0.25F && buf[kRgbAt + 2] == 0.75F);
    }
    {
        BatchState bs;
        bs.haveRgbWave = true;
        bs.rgbGlow = 0.4F;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("rgbGen wave setzt alle drei Kanaele",
                buf[kRgbAt] == 0.4F && buf[kRgbAt + 1] == 0.4F &&
                    buf[kRgbAt + 2] == 0.4F);
    }

    // --- alphaFunc: dieselbe Umrechnung wie in der Pipeline ----------------
    //
    // Zwei Umrechnungen derselben Schwelle waeren zwei Wahrheiten.
    {
        BatchState bs;
        bs.alphaTest = AlphaTest::Ge128;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("die Schwelle kommt aus derselben Quelle",
                buf[kAlphaAt] == pipelineFor(bs, false).alphaSchwelle);
        erwarte("und ist 128/255",
                buf[kAlphaAt] > 0.501F && buf[kAlphaAt] < 0.503F);
    }

    // --- deformVertexes: ein float4, dicht gepackt --------------------------
    {
        BatchState bs;
        bs.deformWave.func = WaveFunc::Sin;
        bs.deformWave.base = 1.0F;
        bs.deformWave.amplitude = 2.0F;
        bs.deformWave.phase = 3.0F;
        bs.deformWave.frequency = 4.0F;
        bs.deformSpread = 0.4F;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("base bei +0", buf[kDeformAt + 0] == 1.0F);
        erwarte("amplitude bei +1", buf[kDeformAt + 1] == 2.0F);
        erwarte("phase bei +2", buf[kDeformAt + 2] == 3.0F);
        erwarte("frequency bei +3", buf[kDeformAt + 3] == 4.0F);
        erwarte("spread dahinter bei 20", buf[kDeformSpreadAt] == 0.4F);
    }
    {
        // Ohne Welle darf nichts stehenbleiben.
        BatchState bs;
        bs.deformSpread = 0.4F;
        packeKonstanten(bs, buf.data(), 0.0F);
        erwarte("ohne deformVertexes bleibt alles null",
                buf[kDeformAt] == 0.0F && buf[kDeformSpreadAt] == 0.0F);
    }

    // --- Ein Nullzeiger stuerzt nicht ab -----------------------------------
    {
        BatchState bs;
        packeKonstanten(bs, nullptr, 0.0F);
        erwarte("nullptr wird abgefangen", true);
    }

    // --- FIX2 P16: die Kette wie rd-vanilla an einer Ecke ------------------
    //
    // turb vor scale: im Spiel wird der Versatz mitskaliert ((st + turb) * 2); rotate und turb lesen die Sinustafel.
    {
        struct Fall {
            const char* was;
            TexMod m[4];
            int n;
        };
        TexMod turb;
        turb.kind = TexModKind::Turb;
        turb.wave.func = WaveFunc::Sin;
        turb.wave.amplitude = 0.3F;
        turb.wave.phase = 0.1F;
        turb.wave.frequency = 0.5F;
        TexMod skal;
        skal.kind = TexModKind::Scale;
        skal.a[0] = 2.0F;
        skal.a[1] = 3.0F;
        TexMod dreh;
        dreh.kind = TexModKind::Rotate;
        dreh.a[0] = 30.0F;
        TexMod rollen;
        rollen.kind = TexModKind::Scroll;
        rollen.a[0] = 0.25F;
        rollen.a[1] = 0.5F;
        // FIX3 Punkt 1: mehrere turb in einer Stufe - rd-vanilla wendet jeden an seiner Stelle an (TR_MAX_TEXMODS 4)
        TexMod turb2 = turb;
        turb2.wave.amplitude = 0.07F;
        turb2.wave.phase = 0.6F;
        turb2.wave.frequency = 2.25F;
        TexMod turb3 = turb;
        turb3.wave.amplitude = 0.11F;
        turb3.wave.phase = -0.35F;
        turb3.wave.frequency = -1.5F;
        TexMod turb4 = turb;
        turb4.wave.amplitude = 0.02F;
        turb4.wave.phase = 0.0F;
        turb4.wave.frequency = 7.0F;
        const Fall faelle[] = {{"turb, scale 2 3", {turb, skal, {}, {}}, 2},
                               {"rotate 30", {dreh, {}, {}, {}}, 1},
                               {"scroll, turb", {rollen, turb, {}, {}}, 2},
                               {"turb, rotate 30, scale 2 3", {turb, dreh, skal, {}}, 3},
                               {"turb, scale 2 3, turb", {turb, skal, turb2, {}}, 3},
                               {"turb, turb, rotate 30", {turb, turb2, dreh, {}}, 3},
                               {"turb, rotate 30, turb, scale 2 3", {turb, dreh, turb3, skal}, 4},
                               {"turb, turb, turb, turb", {turb, turb2, turb3, turb4}, 4}};
        // FIX4 P33: Bit fuer Bit statt bis 1e-4, an drei festen und 4000 gezogenen Ecken und Zeiten (bis 50000 s); daneben der
        // groesste Abstand in float-Stufen
        std::mt19937 zz(20261110U);
        const auto zahl = [&](double a, double b) { return static_cast<float>(a + (b - a) * (static_cast<double>(zz()) / 4294967296.0)); };
        const auto stufen = [](float a, float b) {
            std::int32_t x = 0;
            std::int32_t y = 0;
            std::memcpy(&x, &a, 4);
            std::memcpy(&y, &b, 4);
            if (x < 0) { x = static_cast<std::int32_t>(0x80000000U - static_cast<std::uint32_t>(x)); }
            if (y < 0) { y = static_cast<std::int32_t>(0x80000000U - static_cast<std::uint32_t>(y)); }
            return std::abs(static_cast<long long>(x) - static_cast<long long>(y));
        };
        const float ecken[3][3] = {{37.0F, 211.0F, 90.0F}, {-512.5F, 64.25F, 1000.0F}, {3.0F, -700.0F, -41.0F}};
        for (const Fall& f : faelle) {
            int falsch = 0;
            int zahlEcken = 0;
            long long weit = 0;
            for (int i = 0; i < 4009; ++i) {
                const float xyz[3] = {i < 9 ? ecken[i / 3][0] : zahl(-9000.0, 9000.0), i < 9 ? ecken[i / 3][1] : zahl(-9000.0, 9000.0),
                                      i < 9 ? ecken[i / 3][2] : zahl(-9000.0, 9000.0)};
                const float zeiten[3] = {0.0F, 1.3F, 17.77F};
                const float zeit = i < 9 ? zeiten[i % 3] : (i % 2 == 0) ? zahl(0.0, 100.0) : zahl(0.0, 50000.0);
                float a[2] = {0.3F, 0.7F};
                if (i >= 9) {
                    a[0] = zahl(-4.0, 4.0);
                    a[1] = zahl(-4.0, 4.0);
                }
                float b[2] = {a[0], a[1]};
                spielSt(f.m, f.n, zeit, xyz, a);
                shaderSt(f.m, f.n, zeit, xyz, b);
                ++zahlEcken;
                if (!bitgleich(a[0], b[0]) || !bitgleich(a[1], b[1])) { ++falsch; }
                weit = std::max(weit, std::max(stufen(a[0], b[0]), stufen(a[1], b[1])));
            }
            char z[200];
            std::snprintf(z, sizeof z, "P16/P33 %s: Shader bitgleich zu rd-vanilla: %d von %d Ecken abweichend (hoechstens %lld float-Stufen)", f.was,
                          falsch, zahlEcken, weit);
            erwarte(z, falsch == 0);
        }
    }

    // --- FIX4 P33: turb und rotate bitgleich zu rd-vanilla -------------------
    //
    // Die Tafel (R_InitFuncTables in float), der Versatz eines turb an vielen Orten und Zeiten (auch: ort + 1024 now dicht an einer
    // ganzen Zahl, Vorzeichenwechsel, 1024 now ueber 2^24), die Werte von rotate, und jeder einzelne Schritt durch den Shader.
    {
        int tafelFalsch = 0;
        for (int i = 0; i < 1024; ++i) {
            if (!bitgleich(sinTafel(i), sinTab(i))) { ++tafelFalsch; }
        }
        char z[200];
        std::snprintf(z, sizeof z, "P33 Sinustafel bitgleich zu R_InitFuncTables: %d von 1024 abweichend", tafelFalsch);
        erwarte(z, tafelFalsch == 0);

        // Orte und Zeiten: feste Ziehung (mt19937 ist genormt; die Abbildung auf float hier selbst, nicht uniform_real_distribution)
        std::mt19937 zz(20261108U);
        const auto zahl = [&](double a, double b) { return static_cast<float>(a + (b - a) * (static_cast<double>(zz()) / 4294967296.0)); };
        struct Probe {
            float p;
            float now;
            float amp;
        };
        std::vector<Probe> proben;
        for (int i = 0; i < 200000; ++i) {
            const int art = i % 8;
            float p = 0.0F;
            float now = 0.0F;
            switch (art) {
                case 0:
                    p = zahl(-70000.0, 70000.0);
                    now = zahl(-50.0, 50.0);
                    break;
                case 1:
                    p = std::round(zahl(-70000.0, 70000.0)) + static_cast<float>(zz() % 64U) / 64.0F;
                    now = zahl(0.0, 3000.0);
                    break;
                case 2:
                    p = zahl(-1.0, 1.0);
                    now = zahl(-2.0, 2.0);
                    break;
                case 3:
                    p = std::round(zahl(-9000.0, 9000.0));
                    now = zahl(-20.0, 20.0);
                    break;
                case 4:   // 1024 now ueber 2^24
                    p = zahl(-70000.0, 70000.0);
                    now = zahl(16384.0, 2.0e6);
                    break;
                case 5:   // ort + 1024 now wechselt das Vorzeichen gegenueber ort
                    p = zahl(-5000.0, -200.0);
                    now = zahl(0.2, 6.0);
                    break;
                default: {
                    // ort + 1024 now dicht an einer ganzen Zahl: now so gewaehlt, dann um wenige float-Stufen verschoben
                    p = zahl(-70000.0, 70000.0);
                    const double ganz = std::round(static_cast<double>(p) + zahl(-3000.0, 3000.0));
                    now = static_cast<float>((ganz - static_cast<double>(p)) / 1024.0);
                    const int schritte = static_cast<int>(zz() % 7U) - 3;
                    for (int s = 0; s < std::abs(schritte); ++s) { now = std::nextafter(now, schritte > 0 ? 1.0e30F : -1.0e30F); }
                    break;
                }
            }
            proben.push_back({p, now, zahl(0.01, 1.0)});
        }
        int turbFalsch = 0;
        for (std::size_t i = 0; i + 1 < proben.size(); i += 2) {
            // ein turb allein, now = phase (frequency 0); x mit dem Ort der einen Probe, y mit dem der naechsten
            const Probe& a = proben[i];
            const Probe& b = proben[i + 1];
            TexMod t;
            t.kind = TexModKind::Turb;
            t.wave.func = WaveFunc::Sin;
            t.wave.amplitude = a.amp;
            t.wave.phase = a.now;
            t.wave.frequency = 0.0F;
            BatchState bs{};
            bs.numTexMods = 1;
            bs.texMods[0] = t;
            std::vector<float> k(kKonstFloats, 0.0F);
            packeKonstanten(bs, k.data(), 0.0F);
            const float ort[2] = {a.p, b.p};
            float v[2];
            shaderTurb(ort, k[kTcM0At + 0], k[kTcM0At + 1], v);
            const float sx = spielTurb(a.p, a.now, a.amp);
            const float sy = spielTurb(b.p, a.now, a.amp);
            if (!bitgleich(v[0], sx) || !bitgleich(v[1], sy)) {
                if (turbFalsch < 4) {
                    std::printf("        turb: ort %.9g/%.9g now %.9g amp %.9g: Shader %.9g/%.9g, Spiel %.9g/%.9g\n", static_cast<double>(a.p),
                                static_cast<double>(b.p), static_cast<double>(a.now), static_cast<double>(a.amp), static_cast<double>(v[0]),
                                static_cast<double>(v[1]), static_cast<double>(sx), static_cast<double>(sy));
                }
                ++turbFalsch;
            }
        }
        std::snprintf(z, sizeof z, "P33 turb-Versatz bitgleich zu RB_CalcTurbulentTexCoords: %d von %zu Ecken abweichend", turbFalsch, proben.size() / 2);
        erwarte(z, turbFalsch == 0);

        // rotate: Matrix und Verschiebung wie RB_CalcRotateTexCoords (tr_shade_calc.cpp:1009-1032; translate in double)
        int drehFalsch = 0;
        int drehZahl = 0;
        for (const float grad : {30.0F, -45.5F, 7.0F, 360.25F, 0.001F, 1234.5F}) {
            for (const float zeit : {0.0F, 1.3F, 17.77F, 1234.5F, 86399.9F}) {
                TexMod r;
                r.kind = TexModKind::Rotate;
                r.a[0] = grad;
                BatchState bs{};
                bs.numTexMods = 1;
                bs.texMods[0] = r;
                std::vector<float> k(kKonstFloats, 0.0F);
                packeKonstanten(bs, k.data(), zeit);
                const float degs = -grad * zeit;
                const auto index = static_cast<int>(degs * (1024 / 360.0F));
                const float sn = sinTab(index);
                const float cs = sinTab(index + 1024 / 4);
                const float soll[6] = {cs, sn, -sn, cs, static_cast<float>(0.5 - 0.5 * cs + 0.5 * sn), static_cast<float>(0.5 - 0.5 * sn - 0.5 * cs)};
                const float ist[6] = {k[kTcM0At], k[kTcM0At + 1], k[kTcM0At + 2], k[kTcM0At + 3], k[kTcO01At], k[kTcO01At + 1]};
                ++drehZahl;
                for (int j = 0; j < 6; ++j) {
                    if (!bitgleich(ist[j], soll[j])) {
                        ++drehFalsch;
                        break;
                    }
                }
            }
        }
        std::snprintf(z, sizeof z, "P33 rotate: Matrix und Verschiebung bitgleich zu RB_CalcRotateTexCoords: %d von %d abweichend", drehFalsch, drehZahl);
        erwarte(z, drehFalsch == 0);

        // Jeder einzelne Schritt (und turb hinter scroll) durch den Shader: st bitgleich an vielen Ecken und Zeiten
        TexMod turb;
        turb.kind = TexModKind::Turb;
        turb.wave.func = WaveFunc::Sin;
        turb.wave.amplitude = 0.3F;
        turb.wave.phase = 0.1F;
        turb.wave.frequency = 0.5F;
        TexMod skal;
        skal.kind = TexModKind::Scale;
        skal.a[0] = 2.0F;
        skal.a[1] = 3.0F;
        TexMod dreh;
        dreh.kind = TexModKind::Rotate;
        dreh.a[0] = 30.0F;
        TexMod rollen;
        rollen.kind = TexModKind::Scroll;
        rollen.a[0] = 0.25F;
        rollen.a[1] = 0.5F;
        TexMod trans;
        trans.kind = TexModKind::Transform;
        const float tw[6] = {0.7F, 0.2F, -0.3F, 1.1F, 0.05F, 0.4F};
        for (int j = 0; j < 6; ++j) { trans.a[j] = tw[j]; }
        struct Einzel {
            const char* was;
            TexMod m[2];
            int n;
        };
        const Einzel einzeln[] = {{"turb", {turb, {}}, 1}, {"scroll, turb", {rollen, turb}, 2}, {"rotate 30", {dreh, {}}, 1},
                                  {"scale 2 3", {skal, {}}, 1}, {"scroll", {rollen, {}}, 1}, {"transform", {trans, {}}, 1}};
        for (const Einzel& e : einzeln) {
            int falsch = 0;
            int zahlEcken = 0;
            for (int i = 0; i < 4000; ++i) {
                const float xyz[3] = {zahl(-9000.0, 9000.0), zahl(-9000.0, 9000.0), zahl(-9000.0, 9000.0)};
                const float zeit = (i % 2 == 0) ? zahl(0.0, 100.0) : zahl(0.0, 50000.0);
                float a[2] = {zahl(-4.0, 4.0), zahl(-4.0, 4.0)};
                float b[2] = {a[0], a[1]};
                spielSt(e.m, e.n, zeit, xyz, a);
                shaderSt(e.m, e.n, zeit, xyz, b);
                ++zahlEcken;
                if (!bitgleich(a[0], b[0]) || !bitgleich(a[1], b[1])) { ++falsch; }
            }
            std::snprintf(z, sizeof z, "P33 %s: Shader bitgleich zu rd-vanilla: %d von %d Ecken abweichend", e.was, falsch, zahlEcken);
            erwarte(z, falsch == 0);
        }
    }

    // --- Der Shader und dieser Packer muessen denselben Block meinen ------
    //
    // Sie stehen in zwei Dateien. Diese Probe haelt sie zusammen: was hier
    // gepackt wird, muss im erzeugten HLSL auch vorkommen - sonst schreibt
    // der eine in ein Feld, das der andere nicht liest.
    {
        const std::string hlsl = vertexShaderHlsl(kTexMod | kDeform, 1);
        // FIX4 P33: die Kette je Schritt - jedes Feld wird gelesen
        for (const char* feld : {"gTcM0", "gTcO01", "gTcM1", "gTcM2", "gTcM3", "gTcO23", "gTcArt"}) {
            erwarte((std::string("der Shader kennt ") + feld).c_str(), hlsl.find(std::string("float4 ") + feld + ";") != std::string::npos);
        }
        // FIX3: jeder der vier turb-Plaetze wird gelesen, in der Karte und an der Figur
        const auto zaehle = [](const std::string& h, const char* was) {
            std::size_t n = 0;
            for (std::size_t p = h.find(was); p != std::string::npos; p = h.find(was, p + 1)) { ++n; }
            return n;
        };
        erwarte("der Kartenshader rechnet die Kette je Schritt (FIX4 P33)",
                zaehle(hlsl, "aus.uv = tcKette(aus.uv, gTcM0, gTcO01, gTcM1, gTcM2, gTcM3, gTcO23, gTcArt, ort, true);") == 1U);
        const std::string fig = figurVertexShaderHlsl(0U);
        erwarte("der Figurenshader addiert vier turb (FIX3)",
                zaehle(fig, "tcKette(aus.uv, gStufeTcM0, gStufeTcO01, gStufeTcM1, gStufeTcM2, gStufeTcM3, gStufeTcO23, gStufeTcArt, ort, true)") == 1U);
        erwarte("der Shader kennt gDeform", hlsl.find("gDeform[") != std::string::npos);
        erwarte("der Shader kennt gDeformSpread",
                hlsl.find("gDeformSpread") != std::string::npos);
        const std::string ps = pixelShaderHlsl(kAlphaTest);
        erwarte("der Pixel-Shader kennt gAlphaSchwelle",
                ps.find("gAlphaSchwelle") != std::string::npos);
        erwarte("und gRgb", ps.find("gRgb") != std::string::npos);
    }

    // --- Bei rgbGen wave UND const geht die Welle vor --------------------
    //
    // mapview.cpp:1833: "`wave` geht vor: steht beides in einer Stufe ..."
    // Der Packer liess bis rc439 den const-Wert vorgehen.
    {
        BatchState bs{};
        bs.haveRgbWave = true;
        bs.rgbGlow = 0.8F;
        bs.haveRgbConst = true;
        bs.rgbConst[0] = 0.1F;
        bs.rgbConst[1] = 0.2F;
        bs.rgbConst[2] = 0.3F;
        std::vector<float> f(kKonstFloats, -1.0F);
        packeKonstanten(bs, f.data(), 0.0F);
        erwarte("die Welle gewinnt gegen const - rot",
                f[kRgbAt + 0] > 0.79F && f[kRgbAt + 0] < 0.81F);
        erwarte("die Welle gewinnt gegen const - gruen",
                f[kRgbAt + 1] > 0.79F && f[kRgbAt + 1] < 0.81F);
    }
    {
        // Ohne Welle gilt const weiter.
        BatchState bs{};
        bs.haveRgbConst = true;
        bs.rgbConst[0] = 0.25F;
        std::vector<float> f(kKonstFloats, -1.0F);
        packeKonstanten(bs, f.data(), 0.0F);
        erwarte("ohne Welle steht der const-Wert da",
                f[kRgbAt + 0] > 0.24F && f[kRgbAt + 0] < 0.26F);
    }

    if (fehler != 0) {
        std::printf("FEHLGESCHLAGEN (%d)\n", fehler);
        return 1;
    }
    std::printf("alle Gegenproben bestanden (0 Fehlschlaege)\n");
    return 0;
}
