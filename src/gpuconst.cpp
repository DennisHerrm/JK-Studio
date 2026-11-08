// gpuconst.cpp - siehe bhed/gpuconst.h

#include "bhed/gpuconst.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "bhed/wave.h"

namespace bhed::gpu {


// Der Anteil einer Periode, wie `fraction` in wave.cpp - RB_CalcScroll
// kuerzt so, damit die Koordinaten nicht unbegrenzt wachsen.
float kuerze(float x) { return x - std::floor(x); }

// FIX2 P16: tr.sinTable (R_InitFuncTables, code/rd-vanilla/tr_init.cpp:1727). FIX4 P33: alles in float wie dort. FIX5: die Tafeln
// stehen in wave.cpp (funcTafel), fuer WAVEVALUE und die Shader dieselben.
float sinTafel(int i) { return funcTafel(WaveFunc::Sin, i); }

TcKette tcKette(const TexMod* mods, int anzahl, float zeitSekunden) {
    TcKette k;
    const int n = std::min(anzahl, kMaxTexMods);
    for (int i = 0; i < n; ++i) {
        const TexMod& t = mods[i];
        float* m = k.m + 4 * i;
        float* o = k.o + 2 * i;
        switch (t.kind) {
            case TexModKind::Turb: {
                // RB_CalcTurbulentTexCoords (tr_shade_calc.cpp:936): now = phase + floatTime * frequency in float; `base` geht nicht
                // ein. 1024 now ist in float exakt; der Shader rundet ort + 1024 now wie den double-Ausdruck (turbIndex).
                const float now = t.wave.phase + zeitSekunden * t.wave.frequency;
                k.art[i] = kTcTurb;
                m[0] = now * 1024.0F;
                m[1] = t.wave.amplitude;
                break;
            }
            case TexModKind::Scroll:
                // RB_CalcScrollTexCoords: kuerzt auf [0,1), damit die Koordinaten nicht unbegrenzt wachsen
                k.art[i] = kTcRoll;
                o[0] = kuerze(t.a[0] * zeitSekunden);
                o[1] = kuerze(t.a[1] * zeitSekunden);
                break;
            case TexModKind::Scale:
                k.art[i] = kTcSkal;   // RB_CalcScaleTexCoords: st *= scale
                m[0] = t.a[0];
                m[1] = t.a[1];
                break;
            case TexModKind::Stretch: {
                // RB_CalcStretchTexCoords: p = 1 / Welle, um die Mitte (0,5) statt um den Ursprung (translate in float)
                const float w = waveValue(t.wave, zeitSekunden);
                const float p = (w != 0.0F) ? (1.0F / w) : 1.0F;
                k.art[i] = kTcAffin;
                m[0] = p;
                m[3] = p;
                o[0] = 0.5F - 0.5F * p;
                o[1] = 0.5F - 0.5F * p;
                break;
            }
            case TexModKind::Rotate: {
                // RB_CalcRotateTexCoords (tr_shade_calc.cpp:1009): NEGATIVE Grad je Sekunde, index = degs * (1024 / 360.0f) als int,
                // sin = sinTable[index & MASK], cos = sinTable[(index + 256) & MASK], um die Mitte; tmi.translate in double
                // gerechnet (0.5 ist double, :1025/:1029) und als float abgelegt
                const float grad = -t.a[0] * zeitSekunden;
                const auto index = static_cast<int>(grad * (1024 / 360.0F));
                const float c = sinTafel(index + 1024 / 4);
                const float si = sinTafel(index);
                k.art[i] = kTcAffin;
                m[0] = c;
                m[1] = si;
                m[2] = -si;
                m[3] = c;
                o[0] = static_cast<float>(0.5 - 0.5 * c + 0.5 * si);
                o[1] = static_cast<float>(0.5 - 0.5 * si - 0.5 * c);
                break;
            }
            case TexModKind::Transform:
                // RB_CalcTransformTexCoords: a[0..3] = matrix[0][0], [0][1], [1][0], [1][1]; a[4], a[5] = translate
                k.art[i] = kTcAffin;
                std::copy(t.a, t.a + 4, m);
                o[0] = t.a[4];
                o[1] = t.a[5];
                break;
        }
    }
    return k;
}

Verformung verformung(const Wave& welle, float spread, const DeformZusatz& zusatz, float zeitSekunden) {
    Verformung v;
    const bool welleAn = welle.active() && (welle.frequency == 0.0F || welle.mitTafel());
    const bool wegAn = zusatz.moveWave.mitTafel();
    const auto dabei = [&](std::uint8_t art) {
        switch (art) {
            case kFolgeWelle: return welleAn;
            case kFolgeNormal: return zusatz.normalAn();
            case kFolgeBeule: return zusatz.bulgeAn();
            case kFolgeWeg: return wegAn;
            default: return false;
        }
    };
    // Die Folge: wie im Shader, sonst (ohne Folge, z. B. von Hand gebaute Zustaende) wave, normal, bulge, move
    std::uint8_t folge[4] = {zusatz.folge[0], zusatz.folge[1], zusatz.folge[2], zusatz.folge[3]};
    if (folge[0] == 0U) {
        folge[0] = kFolgeWelle;
        folge[1] = kFolgeNormal;
        folge[2] = kFolgeBeule;
        folge[3] = kFolgeWeg;
    }
    std::uint32_t code = 0;
    int platz = 0;
    for (const std::uint8_t art : folge) {
        if (dabei(art)) {
            code |= static_cast<std::uint32_t>(art) << (3 * platz);
            ++platz;
        }
    }
    if (code == 0U) { return v; }
    v.an = true;
    if (welleAn) {
        // RB_CalcDeformVertexes (tr_shade_calc.cpp:129): frequency 0 -> EvalWaveForm fuer alle Ecken, sonst WAVEVALUE mit
        // phase + off je Ecke; floatTime * frequency ist fuer alle Ecken dasselbe Produkt in float
        if (welle.frequency == 0.0F) {
            v.d[0] = waveValue(welle, zeitSekunden);
            v.d[5] = 0.0F;
        } else {
            v.d[0] = welle.base;
            v.d[1] = welle.amplitude;
            v.d[2] = welle.phase;
            v.d[3] = zeitSekunden * welle.frequency;
            v.d[5] = static_cast<float>(welle.func);
        }
        v.d[4] = spread;
    }
    if (zusatz.normalAn()) {
        // RB_CalcDeformNormals (:182): amplitude, floatTime * frequency
        v.d[6] = zusatz.normal[0];
        v.d[7] = zeitSekunden * zusatz.normal[1];
    }
    if (zusatz.bulgeAn()) {
        // RB_CalcBulgeVertexes (:216): now = backEnd.refdef.time * bulgeSpeed * 0.001f - refdef.time ist int (ms)
        v.d[8] = zusatz.bulge[0];
        v.d[9] = zusatz.bulge[1];
        v.d[10] = zusatz.bulge[2];
        v.d[11] = static_cast<float>(spielZeitMs(zeitSekunden)) * zusatz.bulge[2] * 0.001F;
    }
    if (wegAn) {
        // RB_CalcMoveVertexes (:263): VectorScale(moveVector, WAVEVALUE, offset) - fuer alle Ecken derselbe Versatz
        const float w = waveValue(zusatz.moveWave, zeitSekunden);
        for (int k = 0; k < 3; ++k) { v.d[12 + k] = zusatz.move[k] * w; }
    }
    v.d[15] = static_cast<float>(code);
    return v;
}

void tcKetteLegen(const TcKette& k, float vorn[8], float hinten[20]) {
    static_assert(kMaxTexMods == 4, "gTcM0..3, gTcO01/23 und gTcArt fassen vier Schritte (TR_MAX_TEXMODS)");
    std::copy(k.m, k.m + 4, vorn);
    std::copy(k.o, k.o + 4, vorn + 4);
    std::copy(k.m + 4, k.m + 16, hinten);
    std::copy(k.o + 4, k.o + 8, hinten + 12);
    std::copy(k.art, k.art + 4, hinten + 16);
}

void tcKetteOhneTurb(const TcKette& k, float st[2]) {
    for (int i = 0; i < kMaxTexMods; ++i) {
        const float* m = k.m + 4 * i;
        const float* o = k.o + 2 * i;
        const float s = st[0];
        const float t = st[1];
        if (k.art[i] == kTcAffin) {
            st[0] = s * m[0] + t * m[2] + o[0];
            st[1] = s * m[1] + t * m[3] + o[1];
        } else if (k.art[i] == kTcSkal) {
            st[0] = s * m[0];
            st[1] = t * m[1];
        } else if (k.art[i] == kTcRoll) {
            st[0] = s + o[0];
            st[1] = t + o[1];
        }
    }
}

void packeKonstanten(const BatchState& bs, float* ziel,
                     float zeitSekunden) {
    if (ziel == nullptr) {
        return;
    }
    // Erst alles auf Null. Ein nicht beschriebenes Feld enthielte sonst den
    // Wert des vorigen Stapels - ein Fehler, der nur manchmal auftritt und
    // sich deshalb kaum einkreisen laesst.
    std::fill(ziel, ziel + kKonstFloats, 0.0F);

    // --- Die tcMod-Kette, je Schritt (FIX4 P33) ----------------------------
    //
    // Wie rd-vanilla ComputeTexCoords (tr_shade.cpp:1725): jeder Schritt mit den Werten, die das Spiel an seine Rechenfunktion
    // gibt; der Shader wendet sie der Reihe nach in float an (tcKette, gpushader.cpp). Bis FIX3 zu einer Abbildung
    // zusammengerechnet wie rend2 - das rundete anders als das Spiel.
    static_assert(kTcO01At == kTcM0At + 4U && kTcM2At == kTcM1At + 4U && kTcM3At == kTcM2At + 4U && kTcO23At == kTcM3At + 4U &&
                      kTcArtAt == kTcO23At + 4U,
                  "tcKetteLegen schreibt vorn 8 und hinten 20 float am Stueck");
    tcKetteLegen(tcKette(bs.texMods, bs.numTexMods, zeitSekunden), ziel + kTcM0At, ziel + kTcM1At);

    // --- gRgb: Farbe und alphaConst ---------------------------------------
    //
    // --- rgbGen const und rgbGen wave ------------------------------------
    //
    // Beide ERSETZEN die Eckenfarbe. Hier stand "const setzt die Farbe,
    // wave multipliziert sie" - das war falsch, und der Shader machte
    // daraus zwei verschiedene Zeilen (`=` und `*=`).
    //
    // Der Rasterer schreibt es ausdruecklich hin (mapview.cpp:1818):
    // "rgbGen wave ERSETZT die Vertexfarbe, sie multipliziert sie nicht.
    // RB_CalcWaveColor() schreibt den Wert direkt in alle drei Kanaele."
    //
    // Was das ausmacht, an der Lava von md_am_sith gemessen: die Ecken dort
    // haben im Mittel 103 von 255, also 0,40. Mit `rgbGen wave sin 0.8 ...`
    // rechnete der GPU-Weg 0,40 x 0,8 = 0,32, der Rasterer 0,8. Faktor
    // zweieinhalb - genau die zu dunkle Lava.
    //
    // Und WAVE GEHT VOR: steht beides in einer Stufe, gewinnt die Welle
    // (mapview.cpp:1833). Hier stand es andersherum.
    if (bs.haveRgbWave) {
        ziel[kRgbAt + 0] = bs.rgbGlow;
        ziel[kRgbAt + 1] = bs.rgbGlow;
        ziel[kRgbAt + 2] = bs.rgbGlow;
    } else if (bs.haveRgbConst) {
        ziel[kRgbAt + 0] = bs.rgbConst[0];
        ziel[kRgbAt + 1] = bs.rgbConst[1];
        ziel[kRgbAt + 2] = bs.rgbConst[2];
    } else {
        ziel[kRgbAt + 0] = 1.0F;
        ziel[kRgbAt + 1] = 1.0F;
        ziel[kRgbAt + 2] = 1.0F;
    }
    // alphaConst: negativ heisst "nicht gesetzt", dann volle Deckung.
    ziel[kRgbAt + 3] = (bs.alphaConst >= 0.0F) ? bs.alphaConst : 1.0F;

    // --- Die Schwelle fuer alphaFunc --------------------------------------
    //
    // Sie kommt aus der Pipeline, nicht aus dem Zustand - dort ist sie schon
    // auf 0..1 gebracht. Hier wird sie aus demselben Weg geholt, damit es
    // nicht zwei Umrechnungen gibt.
    ziel[kAlphaAt] = pipelineFor(bs, false).alphaSchwelle;

    // --- deformVertexes ---------------------------------------------------
    //
    // FIX5: wave, normal, bulge und move wie RB_DeformTessGeometry - die Werte aus `verformung` (gpuconst.h), verteilt auf die
    // Felder, die es seit rc592 gibt: d0 gDeform, d1 gDeformSpread/gDeformArt/gDeformNormal.xy, d2 gBulge, d3 gDeformMove.
    {
        const Verformung v = verformung(bs.deformWave, bs.deformSpread, bs.deform2, zeitSekunden);
        std::copy(v.d, v.d + 4, ziel + kDeformAt);
        ziel[kDeformSpreadAt] = v.d[4];
        ziel[kDeformArtAt] = v.d[5];
        ziel[kDeformNormalAt + 0] = v.d[6];
        ziel[kDeformNormalAt + 1] = v.d[7];
        std::copy(v.d + 8, v.d + 12, ziel + kBulgeAt);
        std::copy(v.d + 12, v.d + 16, ziel + kDeformMoveAt);
    }
    ziel[kWolkenAt] = bs.wolkenHoehe;
    ziel[kPortalWeiteAt] = bs.portalWeite;
    // rc605: tcGen vector (ComputeTexCoords TCGEN_VECTOR, tr_shade.cpp:1675-1680 MD).
    if (bs.texGen == TexGen::Vector) {
        for (int k = 0; k < 3; ++k) {
            ziel[kTcGenSAt + static_cast<std::size_t>(k)] = bs.tcGenVec[k];
            ziel[kTcGenTAt + static_cast<std::size_t>(k)] = bs.tcGenVec[3 + k];
        }
    }
}

}  // namespace bhed::gpu
