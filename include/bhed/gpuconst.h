// gpuconst.h - die Konstanten je Stapel, so gepackt wie HLSL sie liest
//
// Warum das eine eigene Datei ist
// -------------------------------
// HLSL packt `cbuffer` nach festen Regeln: alles wird in Bloecke zu vier
// float einsortiert, und ein Wert darf keinen Blockrand ueberschreiten. Wer
// die Struktur auf der C++-Seite "so aehnlich" hinschreibt, bekommt eine
// Verschiebung - und die sieht man nicht als Absturz, sondern als Bild, in
// dem eine Textur langsam wandert, weil sie ihre Werte aus dem falschen Feld
// liest.
//
// Diese Klasse von Fehlern ist besonders unangenehm, weil sie erst auf dem
// Rechner mit Grafikkarte auftritt und dort schwer einzukreisen ist. Also
// wird sie hier gepackt, wo man sie pruefen kann (tests/gpuconst.cpp), und
// der Direct3D-Teil kopiert nur noch einen fertigen Block.
//
// Der Aufbau muss zu `kKonstanten` in src/gpushader.cpp passen:
//
//     cbuffer JeStapel : register(b1) {
//         float4 gTcM0;
//         float4 gTcO01;
//         float4 gRgb;
//         float  gAlphaSchwelle;
//         float  gDeform[4];
//         float  gDeformSpread;
//     };
#ifndef BHED_GPUCONST_H
#define BHED_GPUCONST_H

#include <cstddef>

#include "bhed/gpustate.h"
#include "bhed/mapview.h"

namespace bhed::gpu {

// Die Groesse des Blocks in float.
//
//   gTcM0                =  4
//   gTcO01               =  4
//   gRgb                 =  4
//   gAlphaSchwelle       =  1  -> beginnt einen neuen Viererblock
//   gDeform              =  4  -> float4, beginnt auf der Blockgrenze 16
//   gDeformSpread        =  1  -> gleich dahinter, 20
//
// Bis rc567 stand dort `float gDeform[4]`, und hier war mit vier
// Viererbloecken gerechnet (Spread bei 32). Das stimmte nur halb: das
// LETZTE Element eines float-Feldes belegt nur .x seines Blocks, und das
// naechste Skalar packt HLSL in .y dahinter - gDeformSpread lag bei 29.
// Nachgemessen mit D3DReflect (gpumap_win32.cpp, pruefeStapelLagen). Seitdem
// ist es ein float4: keine Packregel mehr, die man falsch erraten kann.
//
// --- Die tcMod-Kette: Schritt fuer Schritt wie rd-vanilla ------------
//
// Bis rc442 stand hier `float4 gTexMod[8]`: EIN Viererblock je Schritt, die
// Art in .w - `tcMod transform` braucht sechs Werte und stand auf null. Von
// rc443 bis FIX3 wurde die Kette wie in rd-rend2 (`ComputeTexMods`) auf der
// CPU zu EINER affinen Abbildung zusammengerechnet, turb daneben. Das rundet
// anders als das Spiel: rd-vanilla (`ComputeTexCoords`, tr_shade.cpp:1725)
// wendet jeden Schritt der Reihe nach in float auf st an - eine Kette wie
// "turb, scale 2 3" wich um bis zu 48 float-Stufen ab, "turb, turb,
// rotate" um 5120 (FIX4 P33, ctest gpuconst).
//
// Seit FIX4 P33 wieder je Schritt, jetzt mit allen sechs Werten
// (TR_MAX_TEXMODS 4, tr_local.h:279):
//
//   gTcM0, gTcM1..3  float4   je Schritt: rotate/stretch/transform tmi.matrix (m00, m01, m10, m11), scale (s, t),
//                             turb (1024 now, amplitude)
//   gTcO01, gTcO23   float4   je Schritt zwei: tmi.translate bzw. scroll
//   gTcArt           float4   die Art je Schritt (kTcKeine ... kTcRoll)
//
// gTcM0 und gTcO01 stehen vorn (0, 4), der Rest am Ende des Blocks (44..63)
// - 28 float, zwei weniger als die zusammengerechnete Kette mit vier turb.
inline constexpr std::size_t kTcM0At = 0U;
inline constexpr std::size_t kTcO01At = 4U;
inline constexpr std::size_t kRgbAt = 8U;
inline constexpr std::size_t kAlphaAt = kRgbAt + 4U;          // 12
// Ein float4 darf keine Blockgrenze ueberschreiten: nach gAlphaSchwelle
// (12) waere 13 frei, HLSL schiebt auf 16.
inline constexpr std::size_t kDeformAt = 16U;
inline constexpr std::size_t kDeformSpreadAt = kDeformAt + 4U;  // 20
// Die Hoehe der Wolkenkuppel (rc592) - ein float gleich hinter dem Spread,
// im selben Viererblock (HLSL packt float hinter float).
inline constexpr std::size_t kWolkenAt = kDeformSpreadAt + 1U;  // 21
// Die Weite aus alphaGen portal (rc593), gleich dahinter.
inline constexpr std::size_t kPortalWeiteAt = kWolkenAt + 1U;  // 22
// FIX5: die Tafel der Welle von deformVertexes wave (gDeformArt, siehe Verformung), im Rest desselben Viererblocks.
inline constexpr std::size_t kDeformArtAt = kPortalWeiteAt + 1U;  // 23
// Die Stellung eines Movers steht NICHT hier: sie wird vor dem Zeichnen in
// die Kameramatrix multipliziert (siehe gpushader.cpp). Eine eigene Matrix
// im Stapelpuffer hatte 16 Felder gekostet und ein Bild erzeugt, in dem alle
// Ecken in einen Punkt liefen.
// rc592: deformVertexes move (Versatz, auf der CPU aus der Welle), bulge
// (breite, hoehe, tempo) und normal (amplitude, frequenz) - je ein float4.
// FIX5: so belegt, wie `Verformung` es beschreibt (gDeformMove.w die Folge,
// gBulge.w now, gDeformNormal amplitude und floatTime * frequency).
inline constexpr std::size_t kDeformMoveAt = kDeformSpreadAt + 4U;    // 24
inline constexpr std::size_t kBulgeAt = kDeformMoveAt + 4U;           // 28
inline constexpr std::size_t kDeformNormalAt = kBulgeAt + 4U;         // 32
// rc605: tcGen vector - die zwei Vektoren (tcGenVectors[0], [1]) je ein float4.
inline constexpr std::size_t kTcGenSAt = kDeformNormalAt + 4U;        // 36
inline constexpr std::size_t kTcGenTAt = kTcGenSAt + 4U;              // 40
// FIX4 P33: der zweite bis vierte Schritt der tcMod-Kette, ihre Verschiebungen und die Arten (siehe oben). Der Block bleibt
// 64 float - ein Eintrag im Ring (256 Bytes).
inline constexpr std::size_t kTcM1At = kTcGenTAt + 4U;              // 44
inline constexpr std::size_t kTcM2At = kTcM1At + 4U;                // 48
inline constexpr std::size_t kTcM3At = kTcM2At + 4U;                // 52
inline constexpr std::size_t kTcO23At = kTcM3At + 4U;               // 56
inline constexpr std::size_t kTcArtAt = kTcO23At + 4U;              // 60
inline constexpr std::size_t kKonstFloats = kTcArtAt + 4U;          // 64

// FIX4 P33: die Art eines Schritts, wie der Shader sie in gTcArt liest (tcSchritt, gpushader.cpp), je nach der Funktion, die
// rd-vanilla dafuer ruft (tr_shade_calc.cpp): RB_CalcTransformTexCoords (rotate, stretch, transform), RB_CalcTurbulentTexCoords,
// RB_CalcScaleTexCoords, RB_CalcScrollTexCoords.
inline constexpr float kTcKeine = 0.0F;
inline constexpr float kTcAffin = 1.0F;
inline constexpr float kTcTurb = 2.0F;
inline constexpr float kTcSkal = 3.0F;
inline constexpr float kTcRoll = 4.0F;

// Fuellt `ziel` (mindestens kKonstFloats Werte) aus dem Zustand.
//
// Alles, was nicht gesetzt ist, wird auf Null geschrieben - nicht
// uebersprungen. Ein nicht beschriebenes Feld enthaelt sonst den Wert des
// vorigen Stapels, und das ist ein Fehler, der nur manchmal auftritt.
// `zeitSekunden` ist die Zeit DIESES Stapels (timeSeconds - Batch::shaderTime).
//
// Seit rc443 wird die tcMod-Kette hier auf der CPU ausgerechnet statt im
// Shader abgelaufen - scroll, rotate und stretch haengen von der Zeit ab,
// also braucht sie der Packer.
void packeKonstanten(const BatchState& bs, float* ziel, float zeitSekunden);

// FIX4 P33: die tcMod-Kette einer Stufe zur Zeit `zeitSekunden`, je Schritt die Werte, die rd-vanilla in ComputeTexCoords
// (tr_shade.cpp:1725) an die Rechenfunktion gibt (tr_shade_calc.cpp, MD9 gleich), Bit fuer Bit: turb 1024 now (now in float wie
// RB_CalcTurbulentTexCoords, mal 1024 exakt) und amplitude; scroll gekuerzt; rotate aus der Sinustafel mit translate in double;
// stretch (FIX5: EvalWaveForm ueber die Tafeln, p = 1.0f / Welle ohne Pruefung), transform. Die Shader wenden die Schritte der
// Reihe nach an (tcKette, gpushader.cpp). Fuer Karte, Figurenstufe und das Echtzeit-Licht des Karten-Editors. Bitgleich heisst: bei
// derselben Ecke (Ort fuer turb, st) dasselbe Ergebnis (ctest gpuconst, turbgpu, defgpu). An Figuren kommt der Ort aus der Haut auf
// der Grafikkarte (nicht aus RB_SurfaceGhoul), an Flaechen mit deformVertexes aus der Verformung davor - dort ist schon die Ecke
// nicht Bit fuer Bit die des Spiels (FIX5 P36).
struct TcKette {
    float art[kMaxTexMods]{};      // kTcKeine ... kTcRoll
    float m[4 * kMaxTexMods]{};    // je Schritt vier (gTcM0..3)
    float o[2 * kMaxTexMods]{};    // je Schritt zwei (gTcO01, gTcO23)
};
[[nodiscard]] TcKette tcKette(const TexMod* mods, int anzahl, float zeitSekunden);
// Die Lage in den Konstanten der Shader: vorn gTcM0, gTcO01 (8 float), hinten gTcM1, gTcM2, gTcM3, gTcO23, gTcArt (20 float) -
// im Stapelblock, in der Figurenstufe und im Echtzeit-Licht in dieser Reihenfolge.
void tcKetteLegen(const TcKette& k, float vorn[8], float hinten[20]);
// Die Kette auf der CPU ohne turb (die Ecken im Menue-Editor haben keinen Ort im Raum): st der Reihe nach wie die Shader.
void tcKetteOhneTurb(const TcKette& k, float st[2]);
// FIX2 P16: tr.sinTable[i & 1023] (R_InitFuncTables, code/rd-vanilla/tr_init.cpp:1727)
[[nodiscard]] float sinTafel(int i);

// FIX5: die deformVertexes eines Shaders (wave, normal, bulge, move) zur Zeit `zeitSekunden`, so gepackt, wie die Shader sie lesen
// (verformeEcke, gpushader.cpp; Karte und Figurenstufe gleich). Je Ecke rechnet der Shader wie RB_DeformTessGeometry
// (code/rd-vanilla/tr_shade_calc.cpp:553, MD9 gleich) in der Reihenfolge des Shaders; was fuer alle Ecken gleich ist, rechnet die
// CPU hier in float wie das Spiel:
//
//   d0  wave: base, amplitude, phase, floatTime * frequency - bei frequency 0 steht in .x EvalWaveForm (RB_CalcDeformVertexes
//       nimmt dann fuer alle Ecken diesen Wert, auch noise und random)
//   d1  wave: deformationSpread, Tafel (0 = der feste Wert aus .x, sonst WaveFunc sin ... inversesawtooth);
//       normal: amplitude, floatTime * frequency (RB_CalcDeformNormals)
//   d2  bulge: width, height, speed, now = refdef.time * speed * 0.001f (RB_CalcBulgeVertexes, ganze ms)
//   d3  move: moveVector * WAVEVALUE (RB_CalcMoveVertexes); .w die Folge: je drei Bit kFolgeWelle ... kFolgeWeg
//
// wave mit frequency ungleich 0 und move mit noise/random bleiben weg: TableForFunc bricht dort im Spiel mit ERR_DROP ab.
struct Verformung {
    float d[16]{};
    bool an = false;
};
[[nodiscard]] Verformung verformung(const Wave& welle, float spread, const DeformZusatz& zusatz, float zeitSekunden);

}  // namespace bhed::gpu

#endif  // BHED_GPUCONST_H
