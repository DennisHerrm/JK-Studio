// wavetest.cpp - die Wellenformen der Shader, Bit fuer Bit wie rd-vanilla.
//
// FIX5: Die Gegenprobe ist der Code des Spiels, hier abgeschrieben (code/rd-vanilla, MD9 gleich):
//
//   WAVEVALUE (tr_shade_calc.cpp:30)   base + table[Q_ftol((phase + floatTime * freq) * FUNCTABLE_SIZE) & MASK] * amplitude
//   EvalWaveForm (:60)                 noise: base + R_NoiseGet4f(0, 0, 0, (floatTime + phase) * frequency) * amplitude,
//                                      random: GetNoiseTime(refdef.time + phase) <= frequency ? base + amplitude : base
//   R_InitFuncTables (tr_init.cpp:1724-1747), R_NoiseInit/R_NoiseGet4f/GetNoiseTime (rd-common/tr_noise.cpp) mit rand() der
//   MSVC-Laufzeit, RB_CalcWaveColor (:689) und RB_CalcWaveAlpha (:724): das Byte in den Ecken.
//
// Geprueft wird ueber die Wege, die das Programm benutzt: waveValue (stretch, deformVertexes move) und batchStateFor
// (rgbGen wave, alphaGen wave der Vorschau). Bis FIX5 rechnete waveValue mit der Formel (sin des Rechners, triangle nur
// 0..1..0, noise als Mitte, random unbekannt) und die Farbe ohne das Byte.
#include "bhed/mapview.h"
#include "bhed/wave.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>

namespace {

int g_fails = 0;

void expect(const std::string& what, bool ok) {
    if (!ok) {
        ++g_fails;
    }
    std::printf("  %-4s %s\n", ok ? "ok" : "FEHL", what.c_str());
}

bool gleich(float a, float b) {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

// --- Das Spiel ----------------------------------------------------------------------------------------------------------
constexpr int kFuncTableSize = 1024;   // FUNCTABLE_SIZE
constexpr int kFuncTableMask = kFuncTableSize - 1;
float sinTable[kFuncTableSize];
float squareTable[kFuncTableSize];
float triangleTable[kFuncTableSize];
float sawToothTable[kFuncTableSize];
float inverseSawToothTable[kFuncTableSize];
float noiseTable[256];
int noisePerm[256];
std::uint32_t holdrand = 1;
int spielRand() {   // rand() der MSVC-Laufzeit
    holdrand = holdrand * 214013U + 2531011U;
    return static_cast<int>((holdrand >> 16U) & 0x7fffU);
}
constexpr int kRandMax = 32767;
// GF_RAND: im Programm WaveFunc::Random (die achte Form); als Zahl, damit die Probe auch gegen den Stand vor FIX5 baut
const auto kRandom = static_cast<bhed::WaveFunc>(7);

void spielInit() {
    for (int i = 0; i < kFuncTableSize; i++) {
        sinTable[i] = std::sin((static_cast<float>(i) * 360.0F / static_cast<float>(kFuncTableSize - 1)) * 0.017453292519943295769236907684886F);
        squareTable[i] = (i < kFuncTableSize / 2) ? 1.0F : -1.0F;
        sawToothTable[i] = static_cast<float>(i) / kFuncTableSize;
        inverseSawToothTable[i] = 1.0F - sawToothTable[i];
        if (i < kFuncTableSize / 2) {
            if (i < kFuncTableSize / 4) {
                triangleTable[i] = static_cast<float>(i) / (kFuncTableSize / 4);
            } else {
                triangleTable[i] = 1.0F - triangleTable[i - kFuncTableSize / 4];
            }
        } else {
            triangleTable[i] = -triangleTable[i - kFuncTableSize / 2];
        }
    }
    holdrand = 1001U;   // srand(1001)
    for (int i = 0; i < 256; i++) {
        noiseTable[i] = static_cast<float>(((static_cast<float>(spielRand()) / static_cast<float>(kRandMax)) * 2.0 - 1.0));
        noisePerm[i] = static_cast<unsigned char>(static_cast<float>(spielRand()) / static_cast<float>(kRandMax) * 255);
    }
}
// (long)f wie cvttss2si
int qFtol(float f) { return (std::fabs(f) < 2147483648.0F) ? static_cast<int>(f) : (-2147483647 - 1); }
unsigned val(unsigned a) { return static_cast<unsigned>(noisePerm[a & 255U]); }
float getNoiseValue(int x, int y, int z, int t) {
    const auto u = [](int v) { return static_cast<unsigned>(v); };
    return noiseTable[val(u(x) + val(u(y) + val(u(z) + val(u(t)))))];
}
float getNoiseTime(int t) { return (1 + noiseTable[val(static_cast<unsigned>(t))]); }
float lerp(float a, float b, float w) { return a * (1.0F - w) + b * w; }
float rNoiseGet4f(float x, float y, float z, float t) {
    const int ix = qFtol(std::floor(x));
    const float fx = x - static_cast<float>(ix);
    const int iy = qFtol(std::floor(y));
    const float fy = y - static_cast<float>(iy);
    const int iz = qFtol(std::floor(z));
    const float fz = z - static_cast<float>(iz);
    const int it = qFtol(std::floor(t));
    const float ft = t - static_cast<float>(it);
    float front[4];
    float back[4];
    float value[2];
    for (int i = 0; i < 2; i++) {
        front[0] = getNoiseValue(ix, iy, iz, it + i);
        front[1] = getNoiseValue(ix + 1, iy, iz, it + i);
        front[2] = getNoiseValue(ix, iy + 1, iz, it + i);
        front[3] = getNoiseValue(ix + 1, iy + 1, iz, it + i);
        back[0] = getNoiseValue(ix, iy, iz + 1, it + i);
        back[1] = getNoiseValue(ix + 1, iy, iz + 1, it + i);
        back[2] = getNoiseValue(ix, iy + 1, iz + 1, it + i);
        back[3] = getNoiseValue(ix + 1, iy + 1, iz + 1, it + i);
        const float fvalue = lerp(lerp(front[0], front[1], fx), lerp(front[2], front[3], fx), fy);
        const float bvalue = lerp(lerp(back[0], back[1], fx), lerp(back[2], back[3], fx), fy);
        value[i] = lerp(fvalue, bvalue, fz);
    }
    return lerp(value[0], value[1], ft);
}
const float* tableForFunc(bhed::WaveFunc f) {
    switch (f) {
        case bhed::WaveFunc::Sin: return sinTable;
        case bhed::WaveFunc::Triangle: return triangleTable;
        case bhed::WaveFunc::Square: return squareTable;
        case bhed::WaveFunc::Sawtooth: return sawToothTable;
        case bhed::WaveFunc::InverseSawtooth: return inverseSawToothTable;
        default: return nullptr;
    }
}
// EvalWaveForm mit floatTime und refdef.time
float evalWaveForm(const bhed::Wave& wf, float floatTime, int time) {
    if (wf.func == bhed::WaveFunc::Noise) {
        return (wf.base + rNoiseGet4f(0, 0, 0, (floatTime + wf.phase) * wf.frequency) * wf.amplitude);
    }
    if (wf.func == kRandom) {
        if (getNoiseTime(qFtol(static_cast<float>(time) + wf.phase)) <= wf.frequency) {
            return (wf.base + wf.amplitude);
        }
        return wf.base;
    }
    const float* table = tableForFunc(wf.func);
    return ((wf.base) + table[qFtol((((wf.phase) + floatTime * (wf.frequency)) * kFuncTableSize)) & kFuncTableMask] * (wf.amplitude));
}
float evalWaveFormClamped(const bhed::Wave& wf, float floatTime, int time) {
    const float glow = evalWaveForm(wf, floatTime, time);
    if (glow < 0) { return 0; }
    if (glow > 1) { return 1; }
    return glow;
}
// RB_CalcWaveColor / RB_CalcWaveAlpha: das Byte, am Schirm / 255
float farbByte(const bhed::Wave& wf, float floatTime, int time) {
    const int v = qFtol(255 * evalWaveFormClamped(wf, floatTime, time));
    return static_cast<float>(static_cast<unsigned char>(v)) / 255.0F;
}

bhed::Wave mk(bhed::WaveFunc f, float base, float amp, float phase, float freq) {
    bhed::Wave w;
    w.func = f;
    w.base = base;
    w.amplitude = amp;
    w.phase = phase;
    w.frequency = freq;
    return w;
}

}  // namespace

int main() {
    using bhed::WaveFunc;
    spielInit();

    // --- Die Namen (NameToGenFunc, tr_shader.cpp:406-433) ------------------------------------------------------------------
    expect("sin", bhed::waveFuncFromName("sin") == WaveFunc::Sin);
    expect("triangle", bhed::waveFuncFromName("triangle") == WaveFunc::Triangle);
    expect("square", bhed::waveFuncFromName("square") == WaveFunc::Square);
    expect("sawtooth", bhed::waveFuncFromName("sawtooth") == WaveFunc::Sawtooth);
    expect("inversesawtooth", bhed::waveFuncFromName("inversesawtooth") == WaveFunc::InverseSawtooth);
    expect("noise", bhed::waveFuncFromName("noise") == WaveFunc::Noise);
    expect("random (GF_RAND)", bhed::waveFuncFromName("random") != WaveFunc::None);
    expect("ein unbekannter Name ergibt None - die Engine warnt nur", bhed::waveFuncFromName("gibtsnicht") == WaveFunc::None);

    // --- waveValue und die Farbe Bit fuer Bit gegen EvalWaveForm ----------------------------------------------------------
    // Feste Ziehung: Werte wie in den Shadern der Spiele und daneben, Zeiten in ganzen ms (floatTime = time * 0.001 in double,
    // als float - tr_scene.cpp:353), auch lange Laufzeiten
    std::mt19937 zz(20261108U);
    const auto zahl = [&](double a, double b) { return static_cast<float>(a + (b - a) * (static_cast<double>(zz()) / 4294967296.0)); };
    const WaveFunc formen[] = {WaveFunc::Sin, WaveFunc::Triangle, WaveFunc::Square, WaveFunc::Sawtooth, WaveFunc::InverseSawtooth,
                               WaveFunc::Noise, bhed::waveFuncFromName("random")};
    const char* namen[] = {"sin", "triangle", "square", "sawtooth", "inversesawtooth", "noise", "random"};
    for (int f = 0; f < 7; ++f) {
        int falsch = 0;
        int farbeFalsch = 0;
        int alphaFalsch = 0;
        const int n = 20000;
        for (int i = 0; i < n; ++i) {
            const bool klein = (i % 2) == 0;
            bhed::Wave w = mk(formen[f], zahl(-1.0, 1.5), zahl(-2.0, 2.0), klein ? zahl(0.0, 1.0) : zahl(-40.0, 40.0),
                              klein ? zahl(0.0, 4.0) : zahl(-30.0, 30.0));
            if (f == 6) { w.frequency = zahl(0.0, 2.0); }   // random: GetNoiseTime liegt in [0, 2]
            const int ms = (i % 5 == 0) ? static_cast<int>(zz() % 3600000U) : static_cast<int>(zz() % 60000U);
            const auto floatTime = static_cast<float>(ms * 0.001);
            bhed::Wave spiel = w;
            spiel.func = (f == 6) ? kRandom : formen[f];   // random im Spiel auch, wenn das Programm den Namen nicht kennt
            const float soll = evalWaveForm(spiel, floatTime, ms);
            const float ist = bhed::waveValue(w, floatTime);
            if (!gleich(ist, soll)) {
                if (falsch < 2) {
                    std::printf("       %s t=%d ms (%.9g %.9g %.9g %.9g): Programm %.9g, Spiel %.9g\n", namen[f], ms, static_cast<double>(w.base),
                                static_cast<double>(w.amplitude), static_cast<double>(w.phase), static_cast<double>(w.frequency),
                                static_cast<double>(ist), static_cast<double>(soll));
                }
                ++falsch;
            }
            // rgbGen wave / alphaGen wave ueber den Weg der Vorschau
            bhed::TextureSet::Tex t;
            t.rgbWave = w;
            t.alphaWave = w;
            const bhed::BatchState bs = bhed::batchStateFor(t, floatTime);
            const float byteSoll = farbByte(spiel, floatTime, ms);
            if (!gleich(bs.rgbGlow, byteSoll)) { ++farbeFalsch; }
            if (!gleich(bs.alphaConst, byteSoll)) { ++alphaFalsch; }
        }
        char z[200];
        std::snprintf(z, sizeof z, "%s: waveValue bitgleich zu EvalWaveForm: %d von %d abweichend", namen[f], falsch, n);
        expect(z, falsch == 0);
        std::snprintf(z, sizeof z, "%s: rgbGen wave (batchStateFor) = Byte aus RB_CalcWaveColor: %d von %d abweichend", namen[f], farbeFalsch, n);
        expect(z, farbeFalsch == 0);
        std::snprintf(z, sizeof z, "%s: alphaGen wave (batchStateFor) = Byte aus RB_CalcWaveAlpha: %d von %d abweichend", namen[f], alphaFalsch, n);
        expect(z, alphaFalsch == 0);
    }

    // --- Ein paar Stellen zum Nachrechnen --------------------------------------------------------------------------------------
    {
        // triangle laeuft 0 -> 1 -> 0 -> -1 -> 0 (triangleTable, tr_init.cpp:1732-1746): bei t = 0,75 ist es -1
        const bhed::Wave tr = mk(WaveFunc::Triangle, 0.0F, 1.0F, 0.0F, 1.0F);
        expect("triangle bei t=0.25 ist 1", bhed::waveValue(tr, 0.25F) == 1.0F);
        expect("triangle bei t=0.75 ist -1", bhed::waveValue(tr, 0.75F) == -1.0F);
        // Die Sinustafel hat 1024 Eintraege ueber 360 * 1024/1023 Grad: der Eintrag 256 ist nicht genau 1
        const bhed::Wave sn = mk(WaveFunc::Sin, 0.0F, 1.0F, 0.0F, 1.0F);
        expect("sin bei t=0.25 ist sinTable[256]", gleich(bhed::waveValue(sn, 0.25F), sinTable[256]) && sinTable[256] != 1.0F);
        expect("ohne Form kommt null heraus", bhed::waveValue(bhed::Wave{}, 3.0F) == 0.0F);
        // Lava von Mustafar "rgbGen wave sin 0.8 0.1 0 0.3": pendelt zwischen 0,7 und 0,9
        const bhed::Wave lava = mk(WaveFunc::Sin, 0.8F, 0.1F, 0.0F, 0.3F);
        float lo = 9.0F;
        float hi = -9.0F;
        for (int k = 0; k < 4000; ++k) {
            const float v = bhed::waveValue(lava, static_cast<float>(k) * 0.001F);
            lo = std::fmin(lo, v);
            hi = std::fmax(hi, v);
        }
        expect("Lava pendelt zwischen 0.7 und 0.9", std::fabs(lo - 0.7F) < 0.001F && std::fabs(hi - 0.9F) < 0.001F);
    }

    std::printf("\n%s (%d Fehlschlaege)\n", g_fails == 0 ? "alle Wellenproben bestanden" : "FEHLGESCHLAGEN", g_fails);
    return g_fails == 0 ? 0 : 1;
}
