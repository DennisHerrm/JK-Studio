// wave.cpp - siehe bhed/wave.h
#include "bhed/wave.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace bhed {
namespace {

constexpr int kTafel = 1024;          // FUNCTABLE_SIZE
constexpr int kMaske = kTafel - 1;    // FUNCTABLE_MASK

// R_InitFuncTables (code/rd-vanilla/tr_init.cpp:1724-1747, MD9 :1939-1962), Zeile fuer Zeile in float: DEG2RAD mit PI_DIV_180
// als float (shared/qcommon/q_math.h:85-88), sin ueber die float-Ueberladung aus <cmath> (code/qcommon/q_shared.h:120).
struct Tafeln {
    std::array<float, kTafel> sin{};
    std::array<float, kTafel> square{};
    std::array<float, kTafel> saw{};
    std::array<float, kTafel> invSaw{};
    std::array<float, kTafel> triangle{};
    Tafeln() {
        for (int i = 0; i < kTafel; ++i) {
            const auto u = static_cast<std::size_t>(i);
            sin[u] = std::sin(static_cast<float>(i) * 360.0F / static_cast<float>(kTafel - 1) * 0.017453292519943295769236907684886F);
            square[u] = (i < kTafel / 2) ? 1.0F : -1.0F;
            saw[u] = static_cast<float>(i) / kTafel;
            invSaw[u] = 1.0F - saw[u];
            if (i < kTafel / 2) {
                if (i < kTafel / 4) {
                    triangle[u] = static_cast<float>(i) / (kTafel / 4);
                } else {
                    triangle[u] = 1.0F - triangle[u - kTafel / 4];
                }
            } else {
                triangle[u] = -triangle[u - kTafel / 2];
            }
        }
    }
};

const Tafeln& tafeln() {
    static const Tafeln t;
    return t;
}

// R_NoiseInit (code/rd-common/tr_noise.cpp:50, MD9 gleich): srand(1001), je Eintrag erst der Wert, dann die Vertauschung, beide aus
// rand() der MSVC-Laufzeit (holdrand = holdrand * 214013 + 2531011, Wert (holdrand >> 16) & 0x7fff, RAND_MAX 32767).
constexpr int kRausch = 256;   // NOISE_SIZE
struct Rauschen {
    std::array<float, kRausch> tafel{};
    std::array<int, kRausch> perm{};
    Rauschen() {
        std::uint32_t halt = 1001U;
        const auto zufall = [&halt] {
            halt = halt * 214013U + 2531011U;
            return static_cast<int>((halt >> 16U) & 0x7fffU);
        };
        constexpr int kRandMax = 32767;
        for (std::size_t i = 0; i < kRausch; ++i) {
            tafel[i] = static_cast<float>(((static_cast<float>(zufall()) / static_cast<float>(kRandMax)) * 2.0 - 1.0));
            perm[i] = static_cast<unsigned char>(static_cast<float>(zufall()) / static_cast<float>(kRandMax) * 255);
        }
    }
};

const Rauschen& rauschen() {
    static const Rauschen r;
    return r;
}

// VAL(a) = s_noise_perm[(a) & NOISE_MASK], INDEX(x, y, z, t) = VAL(x + VAL(y + VAL(z + VAL(t)))) (tr_noise.cpp:31-32); die Summen
// ohne Vorzeichen (nur die unteren 8 Bit zaehlen, kein Ueberlauf in int)
std::uint32_t rauschVal(std::uint32_t a) { return static_cast<std::uint32_t>(rauschen().perm[a & (kRausch - 1U)]); }
float rauschWert(std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t t) {
    return rauschen().tafel[rauschVal(x + rauschVal(y + rauschVal(z + rauschVal(t))))];
}
// (int) einer float wie cvttss2si: zur Null hin, ausserhalb von int (auch NaN) 0x80000000
int ganzWieX86(float x) {
    if (!(std::fabs(x) < 2147483648.0F)) { return std::numeric_limits<int>::min(); }
    return static_cast<int>(x);
}
// LERP(a, b, w) (a * (1.0f - w) + b * w) (tr_noise.cpp:37)
float rauschLerp(float a, float b, float w) { return a * (1.0F - w) + b * w; }

}  // namespace

WaveFunc waveFuncFromName(std::string_view name) noexcept {
    if (name == "sin") { return WaveFunc::Sin; }
    if (name == "triangle") { return WaveFunc::Triangle; }
    if (name == "square") { return WaveFunc::Square; }
    if (name == "sawtooth") { return WaveFunc::Sawtooth; }
    if (name == "inversesawtooth") { return WaveFunc::InverseSawtooth; }
    if (name == "noise") { return WaveFunc::Noise; }
    if (name == "random") { return WaveFunc::Random; }
    return WaveFunc::None;
}

float funcTafel(WaveFunc f, int i) noexcept {
    const auto u = static_cast<std::size_t>(i & kMaske);
    const Tafeln& t = tafeln();
    switch (f) {
        case WaveFunc::Sin: return t.sin[u];
        case WaveFunc::Triangle: return t.triangle[u];
        case WaveFunc::Square: return t.square[u];
        case WaveFunc::Sawtooth: return t.saw[u];
        case WaveFunc::InverseSawtooth: return t.invSaw[u];
        case WaveFunc::None:
        case WaveFunc::Noise:
        case WaveFunc::Random:
        default: return 0.0F;
    }
}

int funcIndex(float x) noexcept {
    // (long)f: in C++ ausserhalb von long undefiniert, auf x86/x64 (cvttss2si) 0x80000000 - & 1023 ist dann 0
    return ganzWieX86(x) & kMaske;
}

float rauschen4(float x, float y, float z, float t) noexcept {
    // R_NoiseGet4f (tr_noise.cpp:60-102), dieselben Ausdruecke: (int)floor, Rest in float (float - int), je t und t + 1 vier
    // Werte vorn (z) und hinten (z + 1), erst in x, dann y, dann z, zuletzt t gemischt
    const int ix = ganzWieX86(std::floor(x));
    const float fx = x - static_cast<float>(ix);
    const int iy = ganzWieX86(std::floor(y));
    const float fy = y - static_cast<float>(iy);
    const int iz = ganzWieX86(std::floor(z));
    const float fz = z - static_cast<float>(iz);
    const int it = ganzWieX86(std::floor(t));
    const float ft = t - static_cast<float>(it);
    // ix + 1, it + i usw. ohne Vorzeichen (im Spiel int; nur die unteren 8 Bit zaehlen, gleiche Bits ohne Ueberlauf)
    const auto ux = static_cast<std::uint32_t>(ix);
    const auto uy = static_cast<std::uint32_t>(iy);
    const auto uz = static_cast<std::uint32_t>(iz);
    float wert[2];
    for (std::uint32_t i = 0; i < 2U; ++i) {
        const auto ut = static_cast<std::uint32_t>(it) + i;
        const float vorn0 = rauschWert(ux, uy, uz, ut);
        const float vorn1 = rauschWert(ux + 1U, uy, uz, ut);
        const float vorn2 = rauschWert(ux, uy + 1U, uz, ut);
        const float vorn3 = rauschWert(ux + 1U, uy + 1U, uz, ut);
        const float hinten0 = rauschWert(ux, uy, uz + 1U, ut);
        const float hinten1 = rauschWert(ux + 1U, uy, uz + 1U, ut);
        const float hinten2 = rauschWert(ux, uy + 1U, uz + 1U, ut);
        const float hinten3 = rauschWert(ux + 1U, uy + 1U, uz + 1U, ut);
        const float fv = rauschLerp(rauschLerp(vorn0, vorn1, fx), rauschLerp(vorn2, vorn3, fx), fy);
        const float bv = rauschLerp(rauschLerp(hinten0, hinten1, fx), rauschLerp(hinten2, hinten3, fx), fy);
        wert[i] = rauschLerp(fv, bv, fz);
    }
    return rauschLerp(wert[0], wert[1], ft);
}

float rauschZeit(int t) noexcept { return 1.0F + rauschen().tafel[rauschVal(static_cast<std::uint32_t>(t))]; }

const float* rauschTafel() noexcept { return rauschen().tafel.data(); }
const int* rauschPerm() noexcept { return rauschen().perm.data(); }

int spielZeitMs(float sekunden) noexcept {
    const double ms = std::round(static_cast<double>(sekunden) * 1000.0);
    if (!(std::fabs(ms) < 2147483647.0)) { return 0; }
    return static_cast<int>(ms);
}

float waveValue(const Wave& w, float timeSeconds) noexcept {
    switch (w.func) {
        case WaveFunc::None: return 0.0F;
        case WaveFunc::Noise:
            // EvalWaveForm (tr_shade_calc.cpp:64): base + R_NoiseGet4f(0, 0, 0, (floatTime + phase) * frequency) * amplitude
            return w.base + rauschen4(0.0F, 0.0F, 0.0F, (timeSeconds + w.phase) * w.frequency) * w.amplitude;
        case WaveFunc::Random: {
            // EvalWaveForm (:66): GetNoiseTime(refdef.time + phase) - int + float in float, an den int-Parameter zur Null hin
            const float zeit = static_cast<float>(spielZeitMs(timeSeconds)) + w.phase;
            return (rauschZeit(ganzWieX86(zeit)) <= w.frequency) ? (w.base + w.amplitude) : w.base;
        }
        case WaveFunc::Sin:
        case WaveFunc::Triangle:
        case WaveFunc::Square:
        case WaveFunc::Sawtooth:
        case WaveFunc::InverseSawtooth:
        default:
            // WAVEVALUE (tr_shade_calc.cpp:30): (phase + floatTime * freq) * FUNCTABLE_SIZE in float, Q_ftol, & MASK
            return w.base + funcTafel(w.func, funcIndex((w.phase + timeSeconds * w.frequency) * static_cast<float>(kTafel))) * w.amplitude;
    }
}

float waveValueClamped(const Wave& w, float timeSeconds) noexcept {
    // EvalWaveFormClamped (:79): unter 0 -> 0, ueber 1 -> 1 (NaN bleibt NaN wie dort)
    const float g = waveValue(w, timeSeconds);
    if (g < 0.0F) { return 0.0F; }
    if (g > 1.0F) { return 1.0F; }
    return g;
}

float waveFarbe(const Wave& w, float timeSeconds) noexcept {
    // RB_CalcWaveColor (:703-710) / RB_CalcWaveAlpha (:730-732): geklemmt, v = (int)(255 * glow) als Byte
    const float g = waveValueClamped(w, timeSeconds);
    const float v = 255.0F * g;
    const int b = (v >= 0.0F && v <= 255.0F) ? static_cast<int>(v) : 0;
    return static_cast<float>(b) / 255.0F;
}

}  // namespace bhed
