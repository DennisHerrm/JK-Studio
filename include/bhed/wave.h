// wave.h - die Wellenformen der Shader.
//
// Ein JKA-Shader laesst Dinge wogen und pulsen. Beides steht in derselben
// Form da:
//
//     <func> <base> <amplitude> <phase> <frequency>
//
// und beides wird mit derselben Rechnung ausgewertet. Sie steht in
// code/rd-vanilla/tr_shade_calc.cpp, Zeile 30 (MD9 gleich):
//
//     #define WAVEVALUE( table, base, amplitude, phase, freq )
//         ((base) + table[ Q_ftol( ( ( (phase) + floatTime * (freq) )
//             * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))
//
// Also: base + f(phase + zeit * frequenz) * amplitude, wobei f eine der
// fuenf Tafeln mit 1024 Eintraegen ist (R_InitFuncTables, tr_init.cpp:1724-
// 1747). FIX5: wie dort - die Tafeln in float, der Index mit Q_ftol (long,
// zur Null hin) und & 1023, alles in float. Bis FIX5 stand hier die Formel
// ohne Tafel (sin(t * 2 pi) des Rechners statt der Tafel, triangle nur
// 0..1..0 statt 0..1..0..-1..0, noise als feste Mitte).
//
// Die Formen sind die von NameToGenFunc (tr_shader.cpp:406-433): sin,
// triangle, square, sawtooth, inversesawtooth, noise und random.
#ifndef BHED_WAVE_H
#define BHED_WAVE_H

#include <cstdint>
#include <string_view>

namespace bhed {

enum class WaveFunc : std::uint8_t {
    None,
    Sin,
    Triangle,
    Square,
    Sawtooth,
    InverseSawtooth,
    Noise,
    Random,   // GF_RAND (FIX5)
};

struct Wave {
    WaveFunc func = WaveFunc::None;
    float base = 0.0F;
    float amplitude = 0.0F;
    float phase = 0.0F;
    float frequency = 0.0F;

    [[nodiscard]] bool active() const noexcept {
        return func != WaveFunc::None;
    }
    // FIX5: hat die Form eine Tafel (TableForFunc)? noise und random nicht -
    // WAVEVALUE mit ihnen bricht im Spiel mit ERR_DROP ab.
    [[nodiscard]] bool mitTafel() const noexcept {
        return func != WaveFunc::None && func != WaveFunc::Noise && func != WaveFunc::Random;
    }
};

// "sin", "triangle", "square", "sawtooth", "inversesawtooth", "noise",
// "random". Alles andere ergibt None - so wie die Engine dann mit einer
// Warnung weitermacht, statt den Shader zu verwerfen.
[[nodiscard]] WaveFunc waveFuncFromName(std::string_view name) noexcept;

// FIX5: tr.sinTable, triangleTable, squareTable, sawToothTable und
// inverseSawToothTable (R_InitFuncTables, tr_init.cpp:1724-1747), Eintrag
// i & 1023; fuer noise, random und None 0.
[[nodiscard]] float funcTafel(WaveFunc f, int i) noexcept;

// FIX5: Q_ftol(x) & FUNCTABLE_MASK (q_math.h:97, `(long)f`): zur Null hin
// abgeschnitten; ausserhalb von long (auch NaN) gibt cvttss2si 0x80000000,
// & 1023 also 0.
[[nodiscard]] int funcIndex(float x) noexcept;

// FIX5: R_NoiseGet4f (code/rd-common/tr_noise.cpp:60, MD9 gleich) mit den
// Tafeln aus R_NoiseInit: srand(1001), dann rand() der MSVC-Laufzeit (die
// Spiele sind mit MSVC gebaut).
[[nodiscard]] float rauschen4(float x, float y, float z, float t) noexcept;
// FIX5: GetNoiseTime (tr_noise.cpp:47): 1 + s_noise_table[perm[t & 255]].
[[nodiscard]] float rauschZeit(int t) noexcept;
// Die Tafeln selbst (je 256), fuer die Shader (gpushader.cpp).
[[nodiscard]] const float* rauschTafel() noexcept;
[[nodiscard]] const int* rauschPerm() noexcept;

// FIX5: refdef.time (ganze ms) zu einer Zeit in Sekunden. Die Vorschau
// rechnet in Sekunden; das Spiel fuehrt die ms als int und nimmt sie fuer
// random (EvalWaveForm) und bulge (RB_CalcBulgeVertexes) statt floatTime.
[[nodiscard]] int spielZeitMs(float sekunden) noexcept;

// Der Wert der Welle zu einem Zeitpunkt, in Sekunden: EvalWaveForm
// (tr_shade_calc.cpp:60) - WAVEVALUE mit der Tafel, noise ueber
// R_NoiseGet4f(0, 0, 0, (zeit + phase) * frequency), random ueber
// GetNoiseTime(refdef.time + phase) <= frequency.
[[nodiscard]] float waveValue(const Wave& w, float timeSeconds) noexcept;

// FIX5: EvalWaveFormClamped (tr_shade_calc.cpp:79) und die Farbe daraus,
// wie das Spiel sie in die Ecken schreibt: RB_CalcWaveColor (:689) v =
// Q_ftol(255 * glow), RB_CalcWaveAlpha (:724) v = 255 * glow als int - ein
// Byte, am Schirm v / 255. rgbGen wave noise (RB_CalcWaveColor) und
// alphaGen wave rechnen ohne identityLight; die Vorschau rechnet ohne
// Overbright (identityLight 1), also fuer alle Formen gleich.
[[nodiscard]] float waveValueClamped(const Wave& w, float timeSeconds) noexcept;
[[nodiscard]] float waveFarbe(const Wave& w, float timeSeconds) noexcept;

}  // namespace bhed
#endif
