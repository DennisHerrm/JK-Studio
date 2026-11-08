// gpuconst.cpp - siehe bhed/gpuconst.h

#include "bhed/gpuconst.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "bhed/wave.h"

namespace bhed::gpu {


// Der Anteil einer Periode, wie `fraction` in wave.cpp - RB_CalcScroll
// kuerzt so, damit die Koordinaten nicht unbegrenzt wachsen.
float kuerze(float x) { return x - std::floor(x); }

// FIX2 P16: tr.sinTable (R_InitFuncTables, code/rd-vanilla/tr_init.cpp:1727): sin(DEG2RAD(i * 360 / (FUNCTABLE_SIZE - 1)))
float sinTafel(int i) {
    return static_cast<float>(std::sin(static_cast<double>(static_cast<float>(i & 1023) * 360.0F / 1023.0F) * 3.14159265358979323846 / 180.0));
}

void texModAbbildung(const TexMod* mods, int anzahl, float zeitSekunden, float mat[4], float off[2], TurbKette& turb) {
    mat[0] = 1.0F;   // m0, m1, m2, m3
    mat[1] = 0.0F;
    mat[2] = 0.0F;
    mat[3] = 1.0F;
    off[0] = 0.0F;   // m4, m5
    off[1] = 0.0F;
    turb = TurbKette{};

    const int n = std::min(anzahl, kMaxTexMods);
    for (int i = 0; i < n; ++i) {
        const TexMod& m = mods[i];
        // Der Schritt als 2x3, in derselben Anordnung wie rend2:
        //   s[0] s[2] s[4]
        //   s[1] s[3] s[5]
        float s6[6] = {1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
        switch (m.kind) {
            case TexModKind::Turb:
                // RB_CalcTurbulentFactors: now = phase + zeit * frequenz,
                // amplitude unveraendert. `base` geht NICHT ein.
                // FIX2 P16: die Schritte danach nehmen den Versatz mit (mat ab hier die Einheit); FIX3: jeder turb
                // bekommt seinen Platz (ComputeTexCoords, tr_shade.cpp:1725, ruft RB_CalcTurbulentTexCoords je turb)
                turb.s[turb.anzahl].amp = m.wave.amplitude;
                turb.s[turb.anzahl].phase = m.wave.phase + zeitSekunden * m.wave.frequency;
                ++turb.anzahl;
                continue;   // keine Matrix - nicht anhaengen
            case TexModKind::Scroll: {
                // RB_CalcScrollTexMatrix kuerzt auf [0,1), damit die
                // Koordinaten nicht unbegrenzt wachsen.
                const float sx = kuerze(m.a[0] * zeitSekunden);
                const float sy = kuerze(m.a[1] * zeitSekunden);
                s6[4] = sx;
                s6[5] = sy;
                break;
            }
            case TexModKind::Scale:
                s6[0] = m.a[0];
                s6[3] = m.a[1];
                break;
            case TexModKind::Stretch: {
                // RB_CalcStretchTexMatrix: p = 1 / wellenwert, und die
                // Skalierung geht um die Mitte (0,5) statt um den Ursprung.
                const float w = waveValue(m.wave, zeitSekunden);
                const float p = (w != 0.0F) ? (1.0F / w) : 1.0F;
                s6[0] = p;
                s6[3] = p;
                s6[4] = 0.5F - 0.5F * p;
                s6[5] = 0.5F - 0.5F * p;
                break;
            }
            case TexModKind::Rotate: {
                // RB_CalcRotateTexMatrix: NEGATIVE Grad je Sekunde, und die
                // Drehung geht um die Mitte.
                // FIX2 P16: ueber die Sinustafel wie RB_CalcRotateTexCoords (tr_shade_calc.cpp:1009): index = degs * (1024 / 360),
                // sin = sinTable[index & MASK], cos = sinTable[(index + 256) & MASK]
                const float grad = -m.a[0] * zeitSekunden;
                const auto index = static_cast<int>(grad * (1024 / 360.0F));
                const float c = sinTafel(index + 1024 / 4);
                const float si = sinTafel(index);
                s6[0] = c;
                s6[1] = si;
                s6[2] = -si;
                s6[3] = c;
                s6[4] = 0.5F - 0.5F * c + 0.5F * si;
                s6[5] = 0.5F - 0.5F * si - 0.5F * c;
                break;
            }
            case TexModKind::Transform:
                // RB_CalcTransformTexMatrix. Die sechs Werte stehen in a[],
                // Matrix in a[0..3], Verschiebung in a[4], a[5].
                s6[0] = m.a[0];
                s6[1] = m.a[1];
                s6[2] = m.a[2];
                s6[3] = m.a[3];
                s6[4] = m.a[4];
                s6[5] = m.a[5];
                break;
            default:
                continue;
        }
        // Anhaengen, genau in der Reihenfolge von rend2.
        const float n0 = s6[0] * mat[0] + s6[2] * mat[1];
        const float n1 = s6[1] * mat[0] + s6[3] * mat[1];
        const float n2 = s6[0] * mat[2] + s6[2] * mat[3];
        const float n3 = s6[1] * mat[2] + s6[3] * mat[3];
        const float o0 = s6[0] * off[0] + s6[2] * off[1] + s6[4];
        const float o1 = s6[1] * off[0] + s6[3] * off[1] + s6[5];
        mat[0] = n0;
        mat[1] = n1;
        mat[2] = n2;
        mat[3] = n3;
        off[0] = o0;
        off[1] = o1;
        for (int k = 0; k < turb.anzahl; ++k) {   // FIX3: jeder turb davor geht durch diesen Schritt
            float* tm = turb.s[k].mat;
            const float t0 = s6[0] * tm[0] + s6[2] * tm[1];
            const float t1 = s6[1] * tm[0] + s6[3] * tm[1];
            const float t2 = s6[0] * tm[2] + s6[2] * tm[3];
            const float t3 = s6[1] * tm[2] + s6[3] * tm[3];
            tm[0] = t0;
            tm[1] = t1;
            tm[2] = t2;
            tm[3] = t3;
        }
    }
}

void turbWerte(const TurbSchritt& t, float& q, float& qRest, float matAmp[4]) {
    const double qd = static_cast<double>(t.phase) * 1024.0;   // now ist float (RB_CalcTurbulentTexCoords), das Produkt in double
    q = static_cast<float>(qd);
    qRest = static_cast<float>(qd - 1024.0 * std::floor(qd / 1024.0));
    for (int k = 0; k < 4; ++k) { matAmp[k] = t.mat[k] * t.amp; }
}

void turbFelder(const TurbKette& t, float qq[2 * kMaxTexMods], float mats[4 * kMaxTexMods]) {
    std::fill(qq, qq + 2 * kMaxTexMods, 0.0F);
    std::fill(mats, mats + 4 * kMaxTexMods, 0.0F);
    for (int k = 0; k < std::min(t.anzahl, kMaxTexMods); ++k) { turbWerte(t.s[k], qq[2 * k], qq[2 * k + 1], mats + 4 * k); }
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

    // --- gTexMod: je Schritt ein Viererblock, Art in .w -------------------
    // --- Die tcMod-Kette zu EINER Abbildung zusammenrechnen --------------
    //
    // Nach `ComputeTexMods` aus rd-rend2 (tr_shade.cpp:177). Jeder Schritt
    // liefert eine affine 2x3-Abbildung
    //
    //     | m0  m2  m4 |        neu.x = st.x*m0 + st.y*m2 + m4
    //     | m1  m3  m5 |        neu.y = st.x*m1 + st.y*m3 + m5
    //
    // und die Schritte werden der Reihe nach hintereinandergehaengt. Uebrig
    // bleiben vier Werte fuer die Matrix und zwei fuer die Verschiebung -
    // fuer BELIEBIG VIELE Schritte.
    //
    // `turb` ist der einzige, der nicht in eine Matrix passt: er verschiebt
    // je nach ORT, nicht linear in st. Er steht deshalb daneben, mit
    // Amplitude und Phase, und wird im Shader danach aufgeschlagen - genau
    // wie in rend2.
    //
    // Damit erledigt sich `tcMod transform`: es IST eine 2x3-Abbildung und
    // passt hinein wie jeder andere Schritt. Bis rc442 stand es auf null.
    float mat[4];
    float off[2];
    TurbKette turb;
    texModAbbildung(bs.texMods, bs.numTexMods, zeitSekunden, mat, off, turb);
    ziel[kTexMatrixAt + 0] = mat[0];
    ziel[kTexMatrixAt + 1] = mat[1];
    ziel[kTexMatrixAt + 2] = mat[2];
    ziel[kTexMatrixAt + 3] = mat[3];
    ziel[kTexOffTurbAt + 0] = off[0];
    ziel[kTexOffTurbAt + 1] = off[1];
    static_assert(kMaxTexMods == 4, "gTurbMat2..4 und gTurbQ23/Q4 fassen drei turb neben dem ersten");
    // FIX2 P16 / FIX3: der erste turb in gTexOffTurb.zw und gTexTurbMat, der zweite bis vierte in gTurbMat2..4, gTurbQ23, gTurbQ4
    float qq[2 * kMaxTexMods];
    float mats[4 * kMaxTexMods];
    turbFelder(turb, qq, mats);
    ziel[kTexOffTurbAt + 2] = qq[0];
    ziel[kTexOffTurbAt + 3] = qq[1];
    std::copy(mats, mats + 4, ziel + kTexTurbMatAt);
    std::copy(mats + 4, mats + 16, ziel + kTurbMat2At);
    std::copy(qq + 2, qq + 6, ziel + kTurbQ23At);
    ziel[kTurbQ4At + 0] = qq[6];
    ziel[kTurbQ4At + 1] = qq[7];

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
    // Vier Werte in EINEM float4 (gDeform), der Spread gleich dahinter.
    // Frueher ein `float gDeform[4]` mit Schritt vier - und der Spread lag
    // dabei nicht, wo er hier hingeschrieben wurde (siehe gpuconst.h).
    if (bs.deformWave.active()) {
        ziel[kDeformAt + 0] = bs.deformWave.base;
        ziel[kDeformAt + 1] = bs.deformWave.amplitude;
        ziel[kDeformAt + 2] = bs.deformWave.phase;
        ziel[kDeformAt + 3] = bs.deformWave.frequency;
        ziel[kDeformSpreadAt] = bs.deformSpread;
    }
    ziel[kWolkenAt] = bs.wolkenHoehe;
    ziel[kPortalWeiteAt] = bs.portalWeite;
    // move: RB_CalcMoveVertexes - der Versatz ist fuer alle Ecken gleich,
    // also hier einmal gerechnet statt in jeder Ecke.
    if (bs.deform2.moveWave.active()) {
        const float w = waveValue(bs.deform2.moveWave, zeitSekunden);
        for (int k = 0; k < 3; ++k) {
            ziel[kDeformMoveAt + static_cast<std::size_t>(k)] = bs.deform2.move[k] * w;
        }
    }
    if (bs.deform2.bulgeAn()) {
        for (int k = 0; k < 3; ++k) {
            ziel[kBulgeAt + static_cast<std::size_t>(k)] = bs.deform2.bulge[k];
        }
    }
    if (bs.deform2.normalAn()) {
        ziel[kDeformNormalAt + 0] = bs.deform2.normal[0];
        ziel[kDeformNormalAt + 1] = bs.deform2.normal[1];
    }
    // rc605: tcGen vector (ComputeTexCoords TCGEN_VECTOR, tr_shade.cpp:1675-1680 MD).
    if (bs.texGen == TexGen::Vector) {
        for (int k = 0; k < 3; ++k) {
            ziel[kTcGenSAt + static_cast<std::size_t>(k)] = bs.tcGenVec[k];
            ziel[kTcGenTAt + static_cast<std::size_t>(k)] = bs.tcGenVec[3 + k];
        }
    }
}

}  // namespace bhed::gpu
