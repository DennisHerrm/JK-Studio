// gpushader.cpp - HLSL aus den Merkmalsbits
//
// Siehe bhed/gpushader.h. Diese Datei braucht kein Direct3D: sie baut
// Zeichenketten. Geprueft wird sie in tests/gpushader.cpp.

#include "bhed/gpushader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "bhed/gpuconst.h"
#include "bhed/gpustate.h"
#include "bhed/wave.h"

namespace bhed::gpu {

namespace {

// Die Konstanten, die jeder Shader bekommt.
//
// Reihenfolge und Groesse muessen zum Puffer im Backend passen. Deshalb
// steht der Block hier EINMAL und nicht je Variante - sonst laufen die
// Varianten auseinander, und das faellt erst als verschobenes Bild auf.
const char* kKonstanten = R"(
cbuffer JeBild : register(b0) {
    // row_major ist KEIN Schoenheitsfehler.
    //
    // Direct3D packt Matrizen in Konstantenpuffern standardmaessig
    // SPALTENweise. Die Matrix kommt aus baueViewProj() aber zeilenweise
    // (bhed/gpucam.h). Ohne dieses Wort liest der Shader sie transponiert -
    // kein Absturz, sondern ein Bild, das aussieht, als stuende die Kamera
    // woanders.
    row_major float4x4 gViewProj;
    float3   gKamera;      // Weltkoordinaten, fuer tcGen environment
    float    gZeit;        // Sekunden, fuer die Wellen
    // --- Helligkeit, wie im Rasterer -----------------------------------
    //
    // Der Rasterer rechnet (mapview.cpp:2769):
    //
    //     r = lightmap * brightness
    //     r = max(r, minLight * 255)      // Grundhelligkeit als UNTERgrenze
    //
    // Ohne diese beiden kommt die Karte viel zu dunkel heraus - shanks
    // erster Versuch mit richtigen Lightmaps sah aus wie eine Nachtszene,
    // weil sein Regler auf 2,0x steht und der Shader mit 1,0 rechnete.
    //
    // Die Grundhelligkeit ist ausdruecklich ein max() und keine Addition:
    // sie hebt die dunkelsten Stellen an, ohne die hellen zu veraendern.
    float    gHelligkeit;
    float    gGrundlicht;
    // --- Fuer den Himmel ------------------------------------------------
    //
    // Der Rasterer nimmt die Farbe NICHT aus den Koordinaten der Flaeche,
    // sondern aus der Blickrichtung des Bildpunktes (mapview.cpp:2860). Das
    // ist gleichwertig dazu, die Box in unendlicher Entfernung zu zeichnen.
    // Gezeichnet werden nur die Rechtecke aus bhed/himmelbox.h.
    //
    // Dafuer braucht der Shader dieselben Groessen wie die Handrechnung:
    // die drei Kameraachsen, die Brennweite und die halbe Bildgroesse.
    float2   gHalb;        // halfW, halfH
    float    gFokus;
    float    gFuellung;
    float3   gVorn;
    float    gFuellung2;
    float3   gRechts;
    float    gFuellung3;
    float3   gHoch;
    float    gFuellung4;
    // Schnittebene (rc593): Spiegelbild ohne das, was hinter dem Spiegel
    // liegt. dot(float4(welt, 1), gClip) < 0 wird verworfen; (0,0,0,1) laesst alles.
    float4   gClip;
};
cbuffer JeStapel : register(b1) {
    float4 gTcM0;          // FIX4 P33: erster Schritt der tcMod-Kette (gpuconst.h kTcM0At)
    float4 gTcO01;         // FIX4 P33: Verschiebung des ersten und zweiten Schritts
    float4 gRgb;           // rgbGen const / wave, .a = alphaConst
    float  gAlphaSchwelle;
    // EIN float4, kein `float gDeform[4]`. Das Feld belegte vier
    // Viererbloecke, das letzte Element davon aber nur .x - und
    // gDeformSpread rutschte in .y dahinter (Lage 29). Der Packer schrieb
    // nach 32, der Shader las fuer den Ortsanteil der Welle null: der
    // Lavafall in md_am_sith pulsierte im Gleichtakt, statt zu wogen.
    // Mit float4 gibt es keine Packregel zu erraten: 16..19, dann 20.
    // FIX5/DEF1 P44: die deformVertexes wie gpu::verformung (gpuconst.h) sie packt - Platz 0 gDeform, Platz 1 (gDeformSpread,
    // gDeformArt, gDeformNormal.xy), Platz 2 gBulge, gDeformMove.xyz die Spreads, .w die Folge. Die Namen stammen aus rc592.
    float4 gDeform;        // Platz 0
    float  gDeformSpread;  // Platz 1 .x (20)
    float  gWolkenHoehe;   // Wolkenkuppel, siehe gpuconst.h (21)
    float  gPortalWeite;   // alphaGen portal, siehe gpuconst.h (22)
    float  gDeformArt;     // Platz 1 .y (23)
    // rc592, siehe gpuconst.h: jedes ein eigener Viererblock (24, 28, 32).
    float4 gDeformMove;    // die Spreads der drei Plaetze, .w die Folge
    float4 gBulge;         // Platz 2
    float4 gDeformNormal;  // .xy: Platz 1 .zw
    float4 gTcGenS;        // rc605: tcGen vector, s-Vektor (gpuconst.h 36)
    float4 gTcGenT;        // rc605: tcGen vector, t-Vektor (40)
    float4 gTcM1;          // FIX4 P33: zweiter bis vierter Schritt der tcMod-Kette (44, 48, 52)
    float4 gTcM2;
    float4 gTcM3;
    float4 gTcO23;         // FIX4 P33: Verschiebung des dritten und vierten Schritts (56)
    float4 gTcArt;         // FIX4 P33: die Art je Schritt, gpu::kTcKeine ... kTcRoll (60)
};
)";

// FIX2 P16 / FIX3 / FIX4 P33: die tcMod-Kette im Shader (in vertexShaderHlsl, figurVertexShaderHlsl, der Wolkenstufe und den
// Shadern des Echtzeit-Lichts, karte_gpu_win32.cpp): Schritt fuer Schritt wie rd-vanilla ComputeTexCoords, jeder in float und in
// derselben Reihenfolge wie dort (precise: kein mad, nichts umgestellt; Direct3D 11 rundet add und mul in float zur naechsten
// Zahl). Davor die Tafel tr.sinTable als Bitmuster (gpu::sinTafel auf der CPU).
const char* kTcText = R"(
// RB_CalcTurbulentTexCoords (tr_shade_calc.cpp:936-951): sinTable[(int)(((p * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & MASK],
// p = x + z bzw. y in float, der Ausdruck in double. / 1024 und * 1024 sind exakt, also ist der Index (int) des auf double
// gerundeten p + q (q = 1024 now, exakt in float). Ohne double auf der Grafikkarte: s + e = p + q exakt (TwoSum), a = |s|. Ist a
// nicht ganz, bleibt (int) = Vorzeichen * floor(a) (e ist kleiner als der Abstand zur naechsten ganzen Zahl). Ist a ganz, rundet
// double a + e auf die naechste ganze Zahl c ueber e, wenn c - e hoechstens die halbe double-Stufe unter a + c ist (Gleichstand:
// a + c ist gerade), sonst floor(a + e) = a + floor(e). Ausserhalb von int (auch NaN) gibt (int) 0x80000000, & 1023 = 0.
int turbIndex(float p, float q) {
    precise float s = p + q;
    precise float b = s - p;
    precise float e = (p - (s - b)) + (q - b);
    precise float a = abs(s);
    precise float ea = (s < 0.0) ? -e : e;
    if (!(a < 2147483648.0)) { return 0; }
    precise float g = floor(a);
    uint r = (uint)g;
    if (a == g && a > 0.0) {
        precise float f = floor(ea);
        precise float c = f + 1.0;
        uint u = asuint(a);
        uint h = ((u >> 23) - ((((u & 0x7FFFFFu) == 0u) && (c <= 0.0)) ? 54u : 53u)) << 23;
        precise float abstand = c - ea;
        r += (uint)(int)((abstand <= asfloat(h)) ? c : f);
        if (r >= 0x80000000u) { return 0; }
    }
    int i = (int)r;
    return ((s < 0.0) ? -i : i) & 1023;
}
// Der Versatz eines turb an seinem Ort: sinTable[index] * amplitude in float wie im Spiel
float2 turbVersatz(float2 ort, float q, float amp) {
    precise float2 d = float2(asfloat(kSinTafel[turbIndex(ort.x, q)]), asfloat(kSinTafel[turbIndex(ort.y, q)])) * amp;
    return d;
}
// Ein Schritt der Kette, wie ComputeTexCoords (tr_shade.cpp:1725) ihn rechnet (art: gpu::kTcAffin 1 RB_CalcTransformTexCoords
// st[0] = s * m[0][0] + t * m[1][0] + translate[0] fuer rotate, stretch und transform; 2 turb; 3 RB_CalcScaleTexCoords st *= scale;
// 4 RB_CalcScrollTexCoords st += scroll; 0 kein Schritt). Ohne turb (mitTurb false) bleibt st bei turb unveraendert.
// FIX5 P38: [branch] - die Art ist fuer alle Ecken eines Aufrufs dieselbe (Konstantenpuffer); verzweigt statt eingeebnet rechnet eine
// Ecke ohne turb den turb-Teil nicht mit (Bild unveraendert, nur der Weg)
float2 tcSchritt(float2 st, float art, float4 m, float2 o, float2 ort, bool mitTurb) {
    precise float2 r = st;
    [branch] if (art == 1.0) {
        r = float2(st.x * m.x + st.y * m.z + o.x, st.x * m.y + st.y * m.w + o.y);
    } else if (art == 2.0) {
        [branch] if (mitTurb) { r = st + turbVersatz(ort, m.x, m.y); }
    } else if (art == 3.0) {
        r = st * m.xy;
    } else if (art == 4.0) {
        r = st + o;
    }
    return r;
}
// Die ganze Kette (bis TR_MAX_TEXMODS 4) der Reihe nach; ort = (x + z, y) der Ecke wie tess.xyz
float2 tcKette(float2 st, float4 m0, float4 o01, float4 m1, float4 m2, float4 m3, float4 o23, float4 art, float2 ort, bool mitTurb) {
    precise float2 r = tcSchritt(st, art.x, m0, o01.xy, ort, mitTurb);
    r = tcSchritt(r, art.y, m1, o01.zw, ort, mitTurb);
    r = tcSchritt(r, art.z, m2, o23.xy, ort, mitTurb);
    r = tcSchritt(r, art.w, m3, o23.zw, ort, mitTurb);
    return r;
}
)";

// Der HLSL-Text oben mit der Tafel davor: tr.sinTable[1024] als Bitmuster der floats von gpu::sinTafel (R_InitFuncTables)
const std::string& tcTextMitTafel() {
    static const std::string text = [] {
        std::string t = "// tr.sinTable (R_InitFuncTables, code/rd-vanilla/tr_init.cpp:1727), Bit fuer Bit von der CPU (gpu::sinTafel)\n"
                        "static const uint kSinTafel[1024] = {";
        for (int i = 0; i < 1024; ++i) {
            const float w = sinTafel(i);
            std::uint32_t b = 0;
            std::memcpy(&b, &w, sizeof b);
            char z[16];
            std::snprintf(z, sizeof z, "%s0x%08xu", (i % 8 == 0) ? "\n    " : " ", b);
            t += z;
            if (i < 1023) { t += ','; }
        }
        t += "\n};\n";
        t += kTcText;
        return t;
    }();
    return text;
}

// FIX5: deformVertexes wave, normal, bulge und move je Ecke wie rd-vanilla RB_DeformTessGeometry (code/rd-vanilla/tr_shade_calc.cpp:553,
// MD9 gleich), fuer die Karte (vertexShaderHlsl) und die Figurenstufe (figurVertexShaderHlsl) dieselben Funktionen. Die Werte packt
// gpu::verformung (gpuconst.h: d0..d3); alles in float wie im Spiel (precise: kein mad, nichts umgestellt). Davor stehen die Tafel
// tr.sinTable (kSinTafel, tcTextMitTafel) und die Zufallstafel aus R_NoiseInit (kRauschTafel, kRauschPerm, wave.cpp).
const char* kVerformText = R"(
// (int) einer float wie cvttss2si im Spiel: zur Null hin, ausserhalb von int (auch NaN) 0x80000000
int ganzWieX86(float x) {
    return (abs(x) < 2147483648.0) ? (int)x : (int)0x80000000;
}
// Q_ftol(x) & FUNCTABLE_MASK (WAVEVALUE, tr_shade_calc.cpp:30)
int funcIndex(float x) {
    return ganzWieX86(x) & 1023;
}
// R_InitFuncTables (tr_init.cpp:1724-1747): art = WaveFunc (1 sin, 2 triangle, 3 square, 4 sawtooth, 5 inversesawtooth). Ausser sin
// sind alle Eintraege in float exakt: i / 256, 1 - i / 256, i / 1024 (mal einer Zweierpotenz).
float funcTafel(float art, int i) {
    i = i & 1023;
    if (art < 1.5) { return asfloat(kSinTafel[i]); }
    if (art < 2.5) {
        precise float v = (float)(i & 255) * 0.00390625;
        if ((i & 256) != 0) { v = 1.0 - v; }
        return (i < 512) ? v : -v;
    }
    if (art < 3.5) { return (i < 512) ? 1.0 : -1.0; }
    precise float saege = (float)i * 0.0009765625;
    if (art < 4.5) { return saege; }
    precise float invers = 1.0 - saege;
    return invers;
}
// WAVEVALUE(table, base, amplitude, phase, freq) mit w = (base, amplitude, phase, floatTime * freq) und phase + zusatz
float wellenWert(float4 w, float art, float zusatz) {
    precise float p = w.z + zusatz;
    precise float x = (p + w.w) * 1024.0;
    precise float r = w.x + funcTafel(art, funcIndex(x)) * w.y;
    return r;
}
// DEF1 P47: R_NoiseGet4f und Q_rsqrt braucht nur deformVertexes normal - nur in Varianten mit VERFORM_NORMAL (kDeformNormal, Figur)
#ifdef VERFORM_NORMAL
// R_NoiseGet4f (rd-common/tr_noise.cpp:60): VAL/INDEX ueber s_noise_perm, LERP(a, b, w) = a * (1.0f - w) + b * w
uint rauschVal(uint a) { return kRauschPerm[a & 255u]; }
float rauschWert(uint x, uint y, uint z, uint t) {
    return asfloat(kRauschTafel[rauschVal(x + rauschVal(y + rauschVal(z + rauschVal(t))))]);
}
float rauschLerp(float a, float b, float w) {
    precise float r = a * (1.0 - w) + b * w;
    return r;
}
float rauschen4(float x, float y, float z, float t) {
    int ix = ganzWieX86(floor(x));
    precise float fx = x - (float)ix;
    int iy = ganzWieX86(floor(y));
    precise float fy = y - (float)iy;
    int iz = ganzWieX86(floor(z));
    precise float fz = z - (float)iz;
    int it = ganzWieX86(floor(t));
    precise float ft = t - (float)it;
    uint ux = (uint)ix;
    uint uy = (uint)iy;
    uint uz = (uint)iz;
    precise float wert[2];
    [unroll] for (uint i = 0; i < 2; ++i) {
        uint ut = (uint)it + i;
        precise float fv = rauschLerp(rauschLerp(rauschWert(ux, uy, uz, ut), rauschWert(ux + 1u, uy, uz, ut), fx),
                                      rauschLerp(rauschWert(ux, uy + 1u, uz, ut), rauschWert(ux + 1u, uy + 1u, uz, ut), fx), fy);
        precise float bv = rauschLerp(rauschLerp(rauschWert(ux, uy, uz + 1u, ut), rauschWert(ux + 1u, uy, uz + 1u, ut), fx),
                                      rauschLerp(rauschWert(ux, uy + 1u, uz + 1u, ut), rauschWert(ux + 1u, uy + 1u, uz + 1u, ut), fx), fy);
        wert[i] = rauschLerp(fv, bv, fz);
    }
    return rauschLerp(wert[0], wert[1], ft);
}
// Q_rsqrt (shared/qcommon/q_math.cpp:360): 0x5f3759df und eine Newton-Stufe, y * (1.5f - ((x2 * y) * y))
float qRsqrtSpiel(float zahl) {
    precise float x2 = zahl * 0.5;
    precise float y = asfloat(0x5f3759df - (asint(zahl) >> 1));
    precise float r = y * (1.5 - ((x2 * y) * y));
    return r;
}
#endif
// Die Verformung einer Ecke (xyz und Normale im Raum von tess.xyz, st = tess.texCoords[0]) durch die Eintraege von..bis-1 der Folge;
// d0..d3 aus gpu::verformung (je Platz ein float4, d3.xyz die Spreads der Wellen, d3.w die Folge zu je vier Bit, kDefCode...).
// autosprite/autosprite2 (Code 10, 11) stehen nur an ihrer Stelle: an der Karte rechnet solche Shader die CPU (gpu::verformeTess),
// an Figuren fehlen sie (Kartenpruefung).
float3 verformeEcke(float3 xyz, inout float3 nrm, float2 st, float4 d0, float4 d1, float4 d2, float4 d3, int von, int bis) {
    precise float3 p = xyz;
    precise float3 n = nrm;
    uint folge = (uint)d3.w;
    // [loop]: jede Art steht einmal im Code (abgerollt wuerde das Rauschen dreimal eingesetzt - die Uebersetzung dauerte Minuten)
    [loop] for (int k = von; k < bis; ++k) {
        uint art = (folge >> (4 * k)) & 15u;
        if (art == 0u) { break; }   // DEF1 P47: die Folge ist dicht gepackt (gpu::verformung)
        float4 d = (k == 0) ? d0 : ((k == 1) ? d1 : d2);
        if (art <= 6u) {
            // RB_CalcDeformVertexes (:129): bei frequency 0 EvalWaveForm fuer alle (Code 1, d.x), sonst
            // off = (x + y + z) * deformationSpread, WAVEVALUE(table, base, amplitude, phase + off, frequency) mit der Tafel art - 1
            precise float scale = d.x;
            if (art > 1u) {
                // REST3 Punkt 7: der Spread des Platzes als Auswahl - d3[k] mit laufendem k uebersetzte fxc als dp4 mit einer
                // Einheitszeile (D3D11.3 3.1.3.2: dp4 ist nicht genau gerundet; mit 0 und 1 zwar genau, aber nicht zugesichert)
                precise float spread = (k == 0) ? d3.x : ((k == 1) ? d3.y : d3.z);
                precise float off = (p.x + p.y + p.z) * spread;
                scale = wellenWert(d, (float)(art - 1u), off);
            }
            precise float3 v = n * scale;
            p = p + v;
#ifdef VERFORM_NORMAL
        } else if (art == 7u) {
            // RB_CalcDeformNormals (:182): normal += amplitude * R_NoiseGet4f(xyz * 0.98f (+100/+200), floatTime * frequency),
            // dann VectorNormalizeFast (q_math.cpp:1147) mit Q_rsqrt
            precise float3 q = p * kNormalSchritt;
            precise float3 rr = float3(0.0, 0.0, 0.0);
            [loop] for (int a = 0; a < 3; ++a) {
                precise float x = (a == 0) ? q.x : (((a == 1) ? 100.0 : 200.0) + q.x);
                precise float v = rauschen4(x, q.y, q.z, d.y);
                if (a == 0) { rr.x = v; } else if (a == 1) { rr.y = v; } else { rr.z = v; }
            }
            precise float n0 = n.x + d.x * rr.x;
            precise float n1 = n.y + d.x * rr.y;
            precise float n2 = n.z + d.x * rr.z;
            precise float il = qRsqrtSpiel(n0 * n0 + n1 * n1 + n2 * n2);
            n = float3(n0 * il, n1 * il, n2 * il);
#endif
        } else if (art == 8u) {
            // RB_CalcBulgeVertexes (:216): ohne speed und width xyz += normal * height, sonst
            // off = (int)(FUNCTABLE_SIZE / (M_PI * 2) * (st[0] * width + now)), scale = sinTable[off & MASK] * height
            precise float scale = d.y;
            if (!(d.z == 0.0 && d.x == 0.0)) {
                precise float a = st.x * d.x + d.w;
                precise float o = kBeulenFaktor * a;
                precise float t = asfloat(kSinTafel[ganzWieX86(o) & 1023]) * d.y;
                scale = t;
            }
            precise float3 v = n * scale;
            p = p + v;
        } else if (art == 9u) {
            // RB_CalcMoveVertexes (:263): xyz += moveVector * WAVEVALUE (d.xyz von der CPU)
            p = p + d.xyz;
        }
    }
    nrm = n;
    return p;
}
)";

// Der Text oben mit seinen Tafeln: die Zufallstafel aus R_NoiseInit (wave.cpp), FUNCTABLE_SIZE / (M_PI * 2) in float wie im Spiel
// (M_PI ist dort 3.14159265358979323846f, shared/qcommon/q_math.h:40; OpenJK castet auf float, MD9 rechnet ohne Cast gleich in float)
// und 0.98f (RB_CalcDeformNormals). kSinTafel muss davor stehen (tcTextMitTafel).
const std::string& verformText() {
    static const std::string text = [] {
        std::string t = "// FIX5: s_noise_table und s_noise_perm (R_NoiseInit, rd-common/tr_noise.cpp:50), Bit fuer Bit von der CPU (wave.cpp)\n";
        const auto bits = [](float w) {
            std::uint32_t b = 0;
            std::memcpy(&b, &w, sizeof b);
            return b;
        };
        const auto feld = [&t](const char* name, const auto& wert) {
            t += "static const uint ";
            t += name;
            t += "[256] = {";
            for (int i = 0; i < 256; ++i) {
                char z[16];
                std::snprintf(z, sizeof z, "%s0x%08xu", (i % 8 == 0) ? "\n    " : " ", static_cast<unsigned>(wert(i)));
                t += z;
                if (i < 255) { t += ','; }
            }
            t += "\n};\n";
        };
        feld("kRauschTafel", [&bits](int i) { return bits(rauschTafel()[i]); });
        feld("kRauschPerm", [](int i) { return static_cast<std::uint32_t>(rauschPerm()[i]); });
        const float faktor = 1024.0F / (3.14159265358979323846F * 2.0F);
        char z[96];
        std::snprintf(z, sizeof z, "static const float kBeulenFaktor = asfloat(0x%08xu);\n", static_cast<unsigned>(bits(faktor)));
        t += z;
        std::snprintf(z, sizeof z, "static const float kNormalSchritt = asfloat(0x%08xu);\n", static_cast<unsigned>(bits(0.98F)));
        t += z;
        t += kVerformText;
        return t;
    }();
    return text;
}

// Eine Zeile nur anhaengen, wenn das Bit gesetzt ist. Die Pruefung liest
// sich dadurch wie die Merkmalsliste, und ein vergessenes Bit faellt beim
// Lesen auf.
void wenn(std::string& s, std::uint32_t features, std::uint32_t bit,
          const char* zeile) {
    if ((features & bit) != 0U) {
        s += zeile;
    }
}

// Der Knochenblock. Eigener Puffer (b2), weil er nur bei Figuren gebraucht
// wird und mit 6 KiB der groesste von allen ist.
//
// DREI float4 je Knochen: BoneMatrix ist 3x4 (gla.h:33), die vierte Zeile
// waere immer 0,0,0,1. Sie mitzuschicken kostete ein Drittel mehr Platz fuer
// nichts.
const char* kKnochen = R"(
cbuffer JeFigur : register(b2) {
    float4 gKnochen[384];      // 128 Knochen x 3 Zeilen
    row_major float4x4 gFigurWelt;
    // Das Licht an der Stelle der Figur, schon mal Helligkeit und /255
    // (sampleLightGrid + addDynamicLights). gLichtRichtung.w: 1 = aus dem
    // Gitter, 0 = keins da - dann die alte Schattierung aus der Normalen.
    float4 gLichtUmgebung;
    float4 gLichtGerichtet;
    float4 gLichtRichtung;
};
)";

// Ein Dreieck, das den ganzen Bildschirm bedeckt - fuer die Durchgaenge, die
// nur Bildpunkte verarbeiten (Verkleinern, Weichzeichnen, Auflegen).
//
// Ein Dreieck und kein Viereck: das spart einen Zeichenaufruf und hat keine
// Naht in der Mitte, an der zwei Dreiecke sich beruehren. Die Ecken kommen
// aus SV_VertexID, es braucht also gar keinen Puffer.
const char* kVollbildVs = R"(
struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
VSOut main(uint id : SV_VertexID) {
    VSOut aus;
    aus.uv = float2((id << 1) & 2, id & 2);
    aus.pos = float4(aus.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return aus;
}
)";

}  // namespace

std::string vollbildVertexShaderHlsl() { return kVollbildVs; }
std::string tcModHlsl() { return tcTextMitTafel(); }   // FIX3, FIX4 P33

// DEF1 P46: wie kartenVsZiel - ps_4_x kennt kein precise je Anweisung; die Intel UHD Graphics rechnete die Wolkenkette im ps_4_0
// anders als das Spiel (ctest turbgpu: bis 12867 von 32768 Werten), ps_5_0 traegt [precise] und rechnet bitgleich.
const char* wolkenPsZiel(bool ab11) { return ab11 ? "ps_5_0" : "ps_4_0"; }
std::string verformHlsl() { return tcTextMitTafel() + "#define VERFORM_NORMAL\n" + verformText(); }   // FIX5

// --- Die Klinge eines Lichtschwerts --------------------------------------
//
// Zwei Baender je Klinge, schon in Weltkoordinaten und zur Kamera gedreht
// (gpu::baueKlingenband). Die Farbe kommt aus der Textur (blue_glow2,
// blue_line), und nur wenn sie fehlt, aus der Ersatzfarbe - genau wie im
// Rasterer. gFarbe.a sagt, welches von beiden: 1 Textur, 0 Farbe. Ab 2 (rc592,
// Schwertspur): das Bild EINGEFAERBT und mit (a - 2) ausgeblendet.
std::string klingeVertexShaderHlsl() {
    std::string s;
    s += kKonstanten;
    s += R"(
struct VSIn {
    float3 pos   : POSITION;
    float2 uv    : TEXCOORD0;
    float4 farbe : COLOR0;
};
struct VSOut {
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 farbe : COLOR0;
    float  clip  : SV_ClipDistance0;
};
VSOut main(VSIn ein) {
    VSOut aus;
    aus.pos = mul(gViewProj, float4(ein.pos, 1.0));
    aus.clip = dot(float4(ein.pos, 1.0), gClip);
    aus.uv = ein.uv;
    aus.farbe = ein.farbe;
    return aus;
}
)";
    return s;
}

std::string klingePixelShaderHlsl() {
    return R"(
Texture2D    gBild    : register(t0);
SamplerState gSampler : register(s0);
struct VSOut {
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 farbe : COLOR0;
};
float4 main(VSOut ein) : SV_TARGET {
    float4 b = gBild.Sample(gSampler, ein.uv);
    float3 t = b.rgb;
    if (ein.farbe.a > 3.5) {
        return float4(t * ein.farbe.rgb, b.a * (ein.farbe.a - 4.0));
    }
    if (ein.farbe.a > 1.5) {
        return float4(t * ein.farbe.rgb * (ein.farbe.a - 2.0), 1);
    }
    return float4(lerp(ein.farbe.rgb, t, ein.farbe.a), 1);
}
)";
}

// --- Dynamisches Licht und Blobschatten ------------------------------------
//
// Beides legt die Engine NACH den Stufen einer Flaeche obendrauf, und beides
// nur dort, wo die Flaeche schon im Tiefenpuffer steht (GLS_DEPTHFUNC_EQUAL
// in ProjectDlightTexture2, polygonOffset beim markShadow). Deshalb ein
// eigener Durchgang ueber die deckenden Kartenstapel statt einer weiteren
// Merkmalsvariante: die Kartenshader bleiben, wie sie sind.
//
// Bis zu 32 Lichter (MAX_DLIGHTS, tr_local.h) und 32 Schatten je Aufruf -
// der Packer gibt jedem Stapel nur die, deren Kugel bzw. Kasten seinen
// Huellkasten beruehrt.
namespace {
const char* kLichtKonstanten = R"(
cbuffer JeLicht : register(b4) {
    float4 gLichtInfo;            // x Lichter, y Schatten, z Alphaschwelle, w 1 = mit Bild
    float4 gLichtOrt[32];         // xyz Ort, w Radius
    float4 gLichtFarbe[32];       // rgb
    float4 gSchattenOrt[32];      // xyz Auftreffpunkt, w Radius
    float4 gSchattenNormale[32];  // xyz Normale der getroffenen Ebene, w Alpha
};
struct VSLicht {
    float4 pos  : SV_POSITION;
    float3 welt : TEXCOORD0;
    float3 nrm  : TEXCOORD1;
    float2 uv   : TEXCOORD2;
};
)";
}  // namespace

std::string lichtVertexShaderHlsl() {
    std::string s = kKonstanten;
    s += kLichtKonstanten;
    s += R"(
struct VSIn {
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
};
VSLicht main(VSIn ein) {
    VSLicht aus;
    // Dieselbe Rechnung wie im Kartenshader, damit die Tiefe auf die
    // gezeichnete Flaeche faellt (LESS_EQUAL).
    aus.pos = mul(gViewProj, float4(ein.pos, 1.0));
    aus.welt = ein.pos;
    aus.nrm = ein.normal;
    aus.uv = ein.uv;
    return aus;
}
)";
    return s;
}

// ProjectDlightTexture2 (rd-vanilla/tr_shade.cpp:660, r_dlightStyle 1), je
// Dreieck:
//
//     fac      = Abstand des Lichts von der Ebene des Dreiecks
//                (Rueckseite oder fac >= radius: kein Licht)
//     modulate = 1 - fac^2 / radius^2
//     Textur   = tr.dlightImage, in der Ebene zentriert unter dem Licht,
//                Massstab 0.5 / sqrt(radius^2 - fac^2)
//     Farbe    = dl->color * modulate
//
// Die Farbe geht als Byte an die Ecken: colorTemp[k] = Q_ftol(floatColor[k] * modulate) mit `byte colorTemp[4]`
// (tr_shade.cpp:684/842 OpenJK, :670/824 MD) - abgeschnitten und modulo 256. Bei dl->color bis 1 aendert das nur
// die Nachkommastellen; ein staerkeres Licht (cg_saberLightIntensity ueber 1 in Movie Duels) laeuft nahe der Mitte
// ueber und wird dort DUNKLER, wie im Spiel gemessen (rc608 Teil 10b, Boden neben dem Spieler bei 2).
//
// und das MAL dem ersten deckenden Nicht-Lightmap-Bild der Flaeche,
// GL_ONE GL_ONE. Hat die Flaeche kein solches Bild (tcMod, tcGen
// environment), wird statt dessen GL_DST_COLOR GL_ONE gemischt - dann
// kommt die Farbe ohne Bild heraus (gLichtInfo.w = 0).
//
// gfx/2d/dlight ist kreisrund; der Abgriff laeuft deshalb ueber den
// Abstand zur Mitte, nicht ueber die Kantenrichtung des Dreiecks, die die
// Engine als Achse nimmt.
std::string lichtPixelShaderHlsl() {
    std::string s = kLichtKonstanten;
    s += R"(
Texture2D    gBild    : register(t0);
Texture2D    gDlicht  : register(t1);
SamplerState gSampler : register(s0);
SamplerState gRand    : register(s1);
float4 main(VSLicht ein) : SV_TARGET {
    float4 t = gBild.Sample(gSampler, ein.uv);
    if (gLichtInfo.z > 0.0) {
        clip(t.a - gLichtInfo.z);
    }
    float nl = length(ein.nrm);
    if (nl < 1e-6) {
        discard;
    }
    float3 n = ein.nrm / nl;
    float3 summe = float3(0, 0, 0);
    int anzahl = (int)gLichtInfo.x;
    for (int i = 0; i < anzahl; ++i) {
        float r = gLichtOrt[i].w;
        float3 zumLicht = gLichtOrt[i].xyz - ein.welt;
        float fac = dot(n, zumLicht);
        if (fac <= 0.0 || fac >= r) {
            continue;
        }
        float modulate = 1.0 - (fac * fac) / (r * r);
        float rr = sqrt(r * r - fac * fac);
        float3 inEbene = zumLicht - fac * n;
        float u = length(inEbene) * 0.5 / rr;
        if (u >= 0.5) {
            continue;
        }
        float b = gDlicht.Sample(gRand, float2(0.5 + u, 0.5)).r;
        float3 ecke = floor(gLichtFarbe[i].rgb * (255.0 * modulate));
        ecke -= 256.0 * floor(ecke / 256.0);
        summe += ecke * (b / 255.0);
    }
    if (gLichtInfo.w > 0.5) {
        return float4(t.rgb * summe, 1);
    }
    return float4(summe, 1);
}
)";
    return s;
}

// Der Blobschatten (CG_PlayerShadow -> CG_ImpactMark, cg_players.cpp und
// cg_marks.cpp). R_MarkFragments nimmt Flaechen, die
//
//   * zur getroffenen Ebene zeigen (dot >= 0.5),
//   * hoechstens 32 Einheiten darueber und 20 darunter liegen
//     (die beiden Schnittebenen, tr_marks.cpp:306 ff.),
//   * im Quadrat aus +-radius liegen,
//
// und legt darauf `markShadow`: clampmap gfx/damage/shadow, blendFunc
// GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA, rgbGen identity (das Bild ist
// schwarz), alphaGen vertex = 1 - trace.fraction. Am Ziel bleibt also
// ziel * (1 - a); mehrere Schatten multiplizieren sich.
std::string schattenPixelShaderHlsl() {
    std::string s = kLichtKonstanten;
    s += R"(
Texture2D    gBild     : register(t0);
Texture2D    gSchatten : register(t1);
SamplerState gSampler  : register(s0);
SamplerState gRand     : register(s1);
float4 main(VSLicht ein) : SV_TARGET {
    if (gLichtInfo.z > 0.0) {
        clip(gBild.Sample(gSampler, ein.uv).a - gLichtInfo.z);
    }
    float nl = length(ein.nrm);
    if (nl < 1e-6) {
        discard;
    }
    float3 n = ein.nrm / nl;
    float bleibt = 1.0;
    int anzahl = (int)gLichtInfo.y;
    for (int i = 0; i < anzahl; ++i) {
        float3 hn = gSchattenNormale[i].xyz;
        if (dot(n, hn) < 0.5) {
            continue;
        }
        float3 d = ein.welt - gSchattenOrt[i].xyz;
        float h = dot(d, hn);
        if (h > 32.0 || h < -20.0) {
            continue;
        }
        float r = gSchattenOrt[i].w;
        float u = length(d - h * hn) * 0.5 / r;
        if (u >= 0.5) {
            continue;
        }
        float a = gSchatten.Sample(gRand, float2(0.5 + u, 0.5)).a *
                  gSchattenNormale[i].w;
        bleibt *= (1.0 - a);
    }
    return float4(bleibt, bleibt, bleibt, 1);
}
)";
    return s;
}

// --- Die Schattenarten 2 und 3 (cg_shadows) ----------------------------------
//
// Beide arbeiten an der FIGUR selbst, nicht an der Karte: dieselben Ecken und
// Knochen wie figurVertexShaderHlsl, die Haut wird hier noch einmal gerechnet.
// Die Engine nimmt dafuer jede Flaeche der Sortierstufe SS_OPAQUE
// (tr_ghoul2.cpp, RenderSurfaces).
//
// JeSchatten (b5):
//     gSchattenLicht.xyz  Richtung (Art 2: schon (x*0.3, y*0.3, 1), Art 3:
//                         die Lichtrichtung der Figur)
//     gSchattenLicht.w    shadowPlane - Hoehe des Bodens unter der Figur + 1
namespace {
const char* kSchattenHaut = R"(
cbuffer JeSchatten : register(b5) {
    float4 gSchattenLicht;
};
struct VSIn {
    float3 pos     : POSITION;
    float3 normal  : NORMAL;
    float2 uv      : TEXCOORD0;
    float4 bones   : BLENDINDICES;
    float4 weights : BLENDWEIGHT;
};
float3 hautInWelt(VSIn ein) {
    float3 welt = float3(0, 0, 0);
    [unroll] for (int i = 0; i < 4; ++i) {
        float w = ein.weights[i];
        if (w <= 0.0) { continue; }
        int b = (int)(ein.bones[i] + 0.5) * 3;
        float4 r0 = gKnochen[b + 0];
        float4 r1 = gKnochen[b + 1];
        float4 r2 = gKnochen[b + 2];
        welt += float3(dot(r0.xyz, ein.pos) + r0.w,
                       dot(r1.xyz, ein.pos) + r1.w,
                       dot(r2.xyz, ein.pos) + r2.w) * w;
    }
    return mul(gFigurWelt, float4(welt, 1.0)).xyz;
}
)";
}  // namespace

// Art 2: die Ecke nur in die Welt bringen - das Volumen baut der
// Geometrie-Shader.
std::string schattenVolumenVsHlsl() {
    std::string s = kKonstanten;
    s += kKnochen;
    s += kSchattenHaut;
    s += R"(
struct VSOut {
    float4 pos  : SV_POSITION;
    float3 welt : TEXCOORD0;
};
VSOut main(VSIn ein) {
    VSOut aus;
    aus.welt = hautInWelt(ein);
    aus.pos = mul(gViewProj, float4(aus.welt, 1.0));
    return aus;
}
)";
    return s;
}

// RB_DoShadowTessEnd und R_RenderShadowEdges (tr_shadows.cpp der Mod),
// Dreieck fuer Dreieck:
//
//     normal = (v2 - v1) x (v3 - v1);  facing = dot(normal, lightDir) > 0
//     shadowXyz = xyz - lightDir * (z - shadowPlane + 100)
//
// Fuer jedes zugewandte Dreieck ALLE drei Kanten als Band zwischen Ecke und
// verschobener Ecke ("we are going to render all edges even though it is a
// tiny bit slower") und beide Deckel - die Engine zeichnet mit
// _STENCIL_REVERSE (z-fail), und das braucht ein geschlossenes Volumen.
// Innere Kanten kommen zweimal mit umgekehrtem Umlauf und heben sich auf.
std::string schattenVolumenGsHlsl() {
    std::string s = kKonstanten;
    s += R"(
cbuffer JeSchatten : register(b5) {
    float4 gSchattenLicht;
};
struct GSIn {
    float4 pos  : SV_POSITION;
    float3 welt : TEXCOORD0;
};
struct GSOut {
    float4 pos : SV_POSITION;
};
float3 weg(float3 p) {
    return p - gSchattenLicht.xyz * (p.z - gSchattenLicht.w + 100.0);
}
void ecke(float3 p, inout TriangleStream<GSOut> aus) {
    GSOut o;
    o.pos = mul(gViewProj, float4(p, 1.0));
    aus.Append(o);
}
[maxvertexcount(18)]
void main(triangle GSIn ein[3], inout TriangleStream<GSOut> aus) {
    float3 v0 = ein[0].welt;
    float3 v1 = ein[1].welt;
    float3 v2 = ein[2].welt;
    float3 n = cross(v1 - v0, v2 - v0);
    if (dot(n, gSchattenLicht.xyz) <= 0.0) {
        return;
    }
    float3 v[3] = {v0, v1, v2};
    float3 w[3] = {weg(v0), weg(v1), weg(v2)};
    [unroll] for (int i = 0; i < 3; ++i) {
        int j = (i + 1) % 3;
        ecke(v[i], aus);
        ecke(w[i], aus);
        ecke(v[j], aus);
        ecke(w[j], aus);
        aus.RestartStrip();
    }
    ecke(v0, aus); ecke(v1, aus); ecke(v2, aus);
    aus.RestartStrip();
    ecke(w[2], aus); ecke(w[1], aus); ecke(w[0], aus);
    aus.RestartStrip();
}
)";
    return s;
}

// Art 3: RB_ProjectionShadowDeform (tr_shadows.cpp) - jede Ecke entlang der
// Lichtrichtung auf die Ebene des Bodens gedrueckt. Die Richtung wird so
// gekippt, dass sie mindestens 0.5 nach oben zeigt ("don't let the shadows
// get too long or go negative"). Gezeichnet mit dem Shader
// `projectionShadow`: polygonOffset, schwarz, deckend.
std::string schattenFlachVsHlsl() {
    std::string s = kKonstanten;
    s += kKnochen;
    s += kSchattenHaut;
    s += R"(
struct VSOut {
    float4 pos : SV_POSITION;
};
VSOut main(VSIn ein) {
    VSOut aus;
    float3 p = hautInWelt(ein);
    float3 l = gSchattenLicht.xyz;
    float d = l.z;
    if (d < 0.5) {
        l.z += 0.5 - d;
        d = l.z;
    }
    l /= d;
    p -= l * (p.z - gSchattenLicht.w);
    aus.pos = mul(gViewProj, float4(p, 1.0));
    return aus;
}
)";
    return s;
}

// Schwarz und deckend (`rgbGen wave square 0 0 0 0`, blendFunc GL_ONE
// GL_ZERO) fuer Art 3; fuer Art 2 dasselbe mit Alpha 0.5 auf allem, was im
// Volumen liegt (RB_ShadowFinish: qglColor4f(0, 0, 0, 0.5), GL_SRC_ALPHA
// GL_ONE_MINUS_SRC_ALPHA, Stencil ungleich null).
std::string schattenSchwarzPsHlsl() {
    return R"(
float4 main(float4 pos : SV_POSITION) : SV_TARGET {
    return float4(0, 0, 0, 1);
}
)";
}

std::string schattenAbdunkelnPsHlsl() {
    return R"(
struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(VSOut ein) : SV_TARGET {
    return float4(0, 0, 0, 0.5);
}
)";
}

// --- Nebel (r_drawfog 1) -----------------------------------------------------
//
// RB_FogPass (tr_shade.cpp): die Flaeche ein zweites Mal, in der Farbe des
// Nebels, mit tr.fogImage als Alpha, GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA,
// auf der schon gezeichneten Tiefe. Die beiden Koordinaten rechnet
// RB_CalcFogTexCoords JE ECKE (tr_shade_calc.cpp) - hier im Vertex-Shader,
// damit sie genauso ueber das Dreieck laufen wie in der Engine:
//
//     s = tcScale * (Abstand entlang der Blickrichtung) + 1/512
//     t = Tiefe unter der Nebeloberflaeche, je nachdem, ob das Auge im
//         Nebel steht, auf 1/32 .. 31/32 abgebildet
//
// und das Bild selbst ist R_FogFactor (tr_image.cpp) mit der Tabelle aus
// R_InitFogTable (Wurzel) - hier je Texel gerechnet und linear gefiltert wie das Bild.
//
// JeNebel (b6):
//     gNebelFarbe   rgb, w = tcScale
//     gNebelEbene   fog->surface als Tiefenvektor (xyz, w)
//     gNebelInfo    x 1 = mit Oberflaeche, y eye_t, z Alphaschwelle
namespace {
const char* kNebelKonstanten = R"(
cbuffer JeNebel : register(b6) {
    float4 gNebelFarbe;
    float4 gNebelEbene;
    float4 gNebelInfo;
};
struct VSNebel {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float2 st  : TEXCOORD1;
};
float2 nebelST(float3 v) {
    float s = gNebelFarbe.w * (dot(v, gVorn) - dot(gKamera, gVorn)) + 1.0 / 512.0;
    float t = 1.0;
    float augeT = 1.0;
    if (gNebelInfo.x > 0.5) {
        t = dot(v, gNebelEbene.xyz) + gNebelEbene.w;
        augeT = gNebelInfo.y;
    }
    if (augeT < 0.0) {
        t = (t < 1.0) ? 1.0 / 32.0 : 1.0 / 32.0 + 30.0 / 32.0 * t / (t - augeT);
    } else {
        t = (t < 0.0) ? 1.0 / 32.0 : 31.0 / 32.0;
    }
    return float2(s, t);
}
)";
}  // namespace

std::string nebelVertexShaderHlsl() {
    std::string s = kKonstanten;
    s += kNebelKonstanten;
    s += R"(
struct VSIn {
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
};
VSNebel main(VSIn ein) {
    VSNebel aus;
    aus.pos = mul(gViewProj, float4(ein.pos, 1.0));
    aus.uv = ein.uv;
    aus.st = nebelST(ein.pos);
    return aus;
}
)";
    return s;
}

std::string nebelFigurVertexShaderHlsl() {
    std::string s = kKonstanten;
    s += kKnochen;
    s += kSchattenHaut;
    s += kNebelKonstanten;
    s += R"(
VSNebel main(VSIn ein) {
    VSNebel aus;
    float3 p = hautInWelt(ein);
    aus.pos = mul(gViewProj, float4(p, 1.0));
    aus.uv = ein.uv;
    aus.st = nebelST(p);
    return aus;
}
)";
    return s;
}

std::string nebelPixelShaderHlsl() {
    std::string s = kKonstanten;
    s += kNebelKonstanten;
    s += R"(
Texture2D    gBild    : register(t0);
SamplerState gSampler : register(s0);
// Ein Texel von tr.fogImage (R_CreateFogImage): 256 x 32, R_FogFactor an seiner Mitte mit tr.fogTable
// (256 Stufen, Wurzel, abgeschnitten), mal 255 in ein Byte.
float nebelTexel(float x, float y) {
    float s = clamp(x, 0.0, 255.0);
    float t = (clamp(y, 0.0, 31.0) + 0.5) / 32.0;
    s = (s + 0.5) / 256.0 - 1.0 / 512.0;
    if (s < 0.0 || t < 1.0 / 32.0) {
        return 0.0;
    }
    if (t < 31.0 / 32.0) {
        s *= (t - 1.0 / 32.0) / (30.0 / 32.0);
    }
    s = min(s * 8.0, 1.0);
    return floor(255.0 * sqrt(floor(s * 255.0) / 255.0)) / 255.0;
}
float4 main(VSNebel ein) : SV_TARGET {
    if (gNebelInfo.z > 0.0) {
        clip(gBild.Sample(gSampler, ein.uv).a - gNebelInfo.z);
    }
    // GL_LINEAR, GL_CLAMP: nahe am Auge ist die Wurzel steil, die Rechnung ohne das Bild war dort zu stark.
    float2 stelle = ein.st * float2(256.0, 32.0) - 0.5;
    float2 g = floor(stelle);
    float2 f = stelle - g;
    float a = lerp(lerp(nebelTexel(g.x, g.y), nebelTexel(g.x + 1.0, g.y), f.x),
                   lerp(nebelTexel(g.x, g.y + 1.0), nebelTexel(g.x + 1.0, g.y + 1.0), f.x), f.y);
    if (a <= 0.0) {
        discard;
    }
    return float4(gNebelFarbe.rgb, a);
}
)";
    return s;
}

// Der Hintergrund des Modellfensters: ein senkrechter Verlauf, wie ihn
// renderModel zeichnete - oben 56, unten 34 (von 255), gleich in allen
// drei Kanaelen.
std::string modellHintergrundPixelShaderHlsl() {
    return R"(
struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(VSOut ein) : SV_TARGET {
    float v = (34.0 + 22.0 * (1.0 - saturate(ein.uv.y))) / 255.0;
    return float4(v, v, v, 1);
}
)";
}

// --- MBRASTER: das Bodenraster des Model-Tabs ----------------------------
//
// Wie 3ds Max (Home Grid: Nebenlinien, dunklere Hauptlinie jede n-te, die Weltachsen) bzw. Blender (Overlays Grid/Floor/Axes).
// Der Pixel-Shader folgt Blender 5.0 overlay_grid_frag.glsl (Copyright Blender Authors, GPL-2.0-or-later): Dreieckswelle je
// Stufe durch die Aenderung je Bildpunkt (fwidth) geteilt - die Linien sind auch schraeg gesehen gleich duenn -, kantengeglaettet
// mit LINE_STEP (DISC_RADIUS = M_1_SQRTPI * 1,05, overlay_shader_shared.hh), die Stufe aus grid_res (4 Bildpunkte), drei Stufen
// A/B/C ueberblendet, Ausblenden nach Blickwinkel und Entfernung. Die Tiefe prueft die Grafikkarte (gpumap_win32.cpp).
// Das Viereck aus 8 x 8 Teilen: "We draw multiple triangles to avoid float precision issues due to perspective interpolation"
// (Blender overlay_grid_vert.glsl).
std::string modellRasterVertexShaderHlsl() {
    std::string s = R"(cbuffer JeRaster : register(b8) {
    row_major float4x4 gRasterVP;
    float4 gU;          // xyz Richtung u der Ebene, w halbe Kante des Vierecks
    float4 gV;          // xyz Richtung v, w 1: Blender, 0: 3ds Max
    float4 gMitte;      // xyz Mitte des Vierecks, w 1: Linien des Rasters (sonst nur Achsen)
    float4 gUrsprung;   // xyz Ursprung des Rasters, w 3ds Max: halbe Ausdehnung in der Perspektive (0: ohne Grenze)
    float4 gKamera;     // xyz Ort der Kamera, w 1: perspektivisch
    float4 gRechts;     // xyz Rechts der Kamera (Welt), w Blender distance (ab hier blendet der Boden aus)
    float4 gNormale;    // xyz Normale der Ebene, w line_size in Bildpunkten (Blender: (pixelsize - 1) / 2 = 0)
    float4 gRueck;      // xyz Rueckwaerts der Kamera (Blender viewinv[2]), w 1: XY-Ebene in der freien Orthoansicht
    float4 gStufe0;     // Blender: Stufen 0..3; 3ds Max: x Nebenlinie, y Hauptlinie
    float4 gStufe1;     // Blender: Stufen 4..7
    float4 gNeben;      // Farbe der Nebenlinien (Blender theme grid)
    float4 gHaupt;      // Farbe der Hauptlinien (Blender theme grid_emphasis)
    float4 gAchseU;     // Farbe der Achse entlang u (a = 0: aus)
    float4 gAchseV;     // Farbe der Achse entlang v
};
struct VSOut {
    float4 pos  : SV_POSITION;
    float3 welt : TEXCOORD0;
};
)";
    s += R"(
VSOut main(uint id : SV_VertexID) {
    static const float2 ecke[6] = {float2(0, 0), float2(1, 0), float2(1, 1), float2(0, 0), float2(1, 1), float2(0, 1)};
    const uint q = id / 6u;
    const float2 g = (float2(q % 8u, q / 8u) + ecke[id % 6u]) * 0.25 - 1.0;
    const float3 p = gMitte.xyz + (gU.xyz * g.x + gV.xyz * g.y) * gU.w;
    VSOut aus;
    aus.welt = p;
    aus.pos = mul(gRasterVP, float4(p, 1.0));
    return aus;
}
)";
    return s;
}

std::string modellRasterPixelShaderHlsl() {
    std::string s = R"(cbuffer JeRaster : register(b8) {
    row_major float4x4 gRasterVP;
    float4 gU;          // xyz Richtung u der Ebene, w halbe Kante des Vierecks
    float4 gV;          // xyz Richtung v, w 1: Blender, 0: 3ds Max
    float4 gMitte;      // xyz Mitte des Vierecks, w 1: Linien des Rasters (sonst nur Achsen)
    float4 gUrsprung;   // xyz Ursprung des Rasters, w 3ds Max: halbe Ausdehnung in der Perspektive (0: ohne Grenze)
    float4 gKamera;     // xyz Ort der Kamera, w 1: perspektivisch
    float4 gRechts;     // xyz Rechts der Kamera (Welt), w Blender distance (ab hier blendet der Boden aus)
    float4 gNormale;    // xyz Normale der Ebene, w line_size in Bildpunkten (Blender: (pixelsize - 1) / 2 = 0)
    float4 gRueck;      // xyz Rueckwaerts der Kamera (Blender viewinv[2]), w 1: XY-Ebene in der freien Orthoansicht
    float4 gStufe0;     // Blender: Stufen 0..3; 3ds Max: x Nebenlinie, y Hauptlinie
    float4 gStufe1;     // Blender: Stufen 4..7
    float4 gNeben;      // Farbe der Nebenlinien (Blender theme grid)
    float4 gHaupt;      // Farbe der Hauptlinien (Blender theme grid_emphasis)
    float4 gAchseU;     // Farbe der Achse entlang u (a = 0: aus)
    float4 gAchseV;     // Farbe der Achse entlang v
};
struct VSOut {
    float4 pos  : SV_POSITION;
    float3 welt : TEXCOORD0;
};
)";
    s += R"(
#define LINIE_START (0.5 - 0.5641896 * 1.05)
#define LINIE_ENDE (0.5 + 0.5641896 * 1.05)
float linie(float d) { return smoothstep(LINIE_START, LINIE_ENDE, d); }
// Blender get_grid: Dreieckswelle [0, halbe Stufe], durch die Aenderung je Bildpunkt geteilt
float gitter(float2 co, float2 fw, float2 st) {
    const float2 h = st * 0.5;
    const float2 x = co + h;
    float2 d = abs(x - st * floor(x / st) - h);
    d /= fw;
    return 1.0 - linie(min(d.x, d.y) - gNormale.w);
}
// Blender get_axes: die Achse etwas kraeftiger (line_size + 0,1)
float achse(float d, float fw) { return 1.0 - linie(abs(d) / fw - (0.1 + gNormale.w)); }
float linearstep(float a, float b, float v) { return saturate((v - a) / abs(b - a)); }
float4 main(VSOut ein) : SV_TARGET {
    const float3 P = ein.welt;
    const float3 dx = ddx(P);
    const float3 dy = ddy(P);
    const float3 rel = P - gUrsprung.xyz;
    const float2 co = float2(dot(rel, gU.xyz), dot(rel, gV.xyz));
    const float2 fw = max(float2(abs(dot(dx, gU.xyz)) + abs(dot(dy, gU.xyz)), abs(dot(dx, gV.xyz)) + abs(dot(dy, gV.xyz))), 1e-6);
    const bool blender = gV.w > 0.5;
    // 3ds Max, Perspektive: das Home Grid endet bei der Ausdehnung - die Linie am Rand noch ganz
    if (!blender && gUrsprung.w > 0.0 && (abs(co.x) > gUrsprung.w + 0.6 * fw.x || abs(co.y) > gUrsprung.w + 0.6 * fw.y)) { discard; }
    float aus = 1.0;
    if (blender) {
        if (gKamera.w > 0.5) {
            float3 V = gKamera.xyz - P;
            const float dist = length(V);
            V /= dist;
            float w = 1.0 - abs(dot(V, gNormale.xyz));
            w *= w;
            aus = 1.0 - w * w;
            aus *= 1.0 - smoothstep(0.0, gRechts.w, dist - gRechts.w);
        } else if (gRueck.w > 0.5) {
            float w = 1.0 - abs(gRueck.z);
            w *= w;
            aus = 1.0 - w * w;
        }
    }
    float4 farbe = float4(gNeben.rgb, 0.0);
    if (gMitte.w > 0.5) {
        if (blender) {
            // grid_res: Einheiten auf 4 Bildpunkten; die feinste Stufe, die groesser ist
            const float res = dot(dx, gRechts.xyz) * 4.0;
            const float st[8] = {gStufe0.x, gStufe0.y, gStufe0.z, gStufe0.w, gStufe1.x, gStufe1.y, gStufe1.z, gStufe1.w};
            int k = 7;
            [unroll] for (int i = 6; i >= 0; --i) { k = (res < st[i]) ? i : k; }
            const float s0 = k > 0 ? st[max(k - 1, 0)] : 0.0;
            const float sA = st[k];
            const float sB = st[min(k + 1, 7)];
            const float sC = st[min(k + 2, 7)];
            float b = 1.0 - linearstep(s0, sA, res);
            b = b * b * b;
            const float gA = gitter(co, fw, float2(sA, sA));
            const float gB = gitter(co, fw, float2(sB, sB));
            const float gC = gitter(co, fw, float2(sC, sC));
            farbe = gNeben;
            farbe.a *= gA * b;
            farbe = lerp(farbe, lerp(gNeben, gHaupt, b), gB);
            farbe = lerp(farbe, gHaupt, gC);
        } else {
            const float gA = gitter(co, fw, gStufe0.xx);
            const float gB = gitter(co, fw, gStufe0.yy);
            farbe = gNeben;
            farbe.a *= gA;
            farbe = lerp(farbe, gHaupt, gB);
        }
    }
    // die Achse entlang u liegt bei co.y = 0, die entlang v bei co.x = 0
    if (gAchseU.a > 0.0) {
        const float a = achse(co.y, fw.y);
        farbe.a = max(farbe.a, a);
        farbe.rgb = a < 1e-8 ? farbe.rgb : gAchseU.rgb;
    }
    if (gAchseV.a > 0.0) {
        const float a = achse(co.x, fw.x);
        farbe.a = max(farbe.a, a);
        farbe.rgb = a < 1e-8 ? farbe.rgb : gAchseV.rgb;
    }
    farbe.a *= aus;
    if (farbe.a <= 0.0) { discard; }
    return farbe;
}
)";
    return s;
}

std::string glowBlurPixelShaderHlsl() {
    // Woertlich wie glowBlur (src/glow.cpp:109): ein Vier-Punkt-Kern in den
    // Ecken, Gewicht intensity*0.25 je Punkt.
    //
    // Der Abstand waechst je Durchgang um `delta` (0.1, 0.9, 1.7, 2.5, 3.3, RB_BlurGlowTexture) und bleibt
    // ungerundet, mit GL_LINEAR. Gerundet (bis rc608 Teil 5) gab es Streifen, die das Spiel nicht hat.
    return R"(
cbuffer JeDurchgang : register(b3) {
    float2 gSchritt;      // 1/Breite, 1/Hoehe des kleinen Puffers
    float  gAbstand;      // in BILDPUNKTEN
    float  gGewicht;      // intensity * 0.25
};
Texture2D    gQuelle  : register(t0);
SamplerState gSampler : register(s0);
struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(VSOut ein) : SV_TARGET {
    float3 acc = float3(0, 0, 0);
    [unroll] for (int sy = -1; sy <= 1; sy += 2) {
        [unroll] for (int sx = -1; sx <= 1; sx += 2) {
            float2 uv = ein.uv + float2(sx, sy) * gAbstand * gSchritt;
            acc += gQuelle.Sample(gSampler, saturate(uv)).rgb * gGewicht;
        }
    }
    return float4(acc, 1);
}
)";
}

float spielGammaWert(float wert) {
    const float u = std::clamp(wert * 64.0F - 0.5F, 0.0F, 63.0F);
    const float i = std::floor(u);
    const float tief = std::floor(i * 255.0F / 63.0F + 0.5F) / 255.0F;
    const float hoch = std::floor(std::min(i + 1.0F, 63.0F) * 255.0F / 63.0F + 0.5F) / 255.0F;
    return tief + (hoch - tief) * (u - i);
}

std::string spielGammaPixelShaderHlsl() {
    return R"(
Texture2D gBild : register(t0);
SamplerState gAbtaster : register(s0);
struct Ein { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
float4 main(Ein ein) : SV_TARGET {
    float4 c = gBild.Sample(gAbtaster, ein.uv);
    float3 u = clamp(c.rgb * 64.0 - 0.5, 0.0, 63.0);
    float3 i = floor(u);
    float3 tief = floor(i * 255.0 / 63.0 + 0.5) / 255.0;
    float3 hoch = floor(min(i + 1.0, 63.0) * 255.0 / 63.0 + 0.5) / 255.0;
    return float4(lerp(tief, hoch, u - i), c.a);
}
)";
}

std::string tiefeProbePixelShaderHlsl() {
    return R"(
Texture2DMS<float> gTiefe : register(t0);
struct Ein { float4 pos : SV_POSITION; };
float main(Ein ein) : SV_TARGET {
    return gTiefe.Load(int2(ein.pos.xy), 0);
}
)";
}

std::string glowCompositePixelShaderHlsl() {
    // Woertlich wie glowComposite (src/glow.cpp:255):
    //
    //     soft:  out = g + s * (1 - g/255)
    //     hart:  out = g + s
    //
    // `soft` ist die uebliche Einstellung. Die Form mit den vertauschten
    // Rollen steht so im Quelltext des Rasterers - und deshalb auch hier.
    return R"(
cbuffer JeDurchgang : register(b3) {
    float2 gSchritt;
    float  gAbstand;
    float  gGewicht;
    float  gWeich;        // 1 = soft, 0 = hart
    float3 gFuellung;
};
Texture2D    gGlow    : register(t0);
Texture2D    gSzene   : register(t1);
SamplerState gSampler : register(s0);
struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(VSOut ein) : SV_TARGET {
    float3 g = gGlow.Sample(gSampler, ein.uv).rgb;
    float3 s = gSzene.Sample(gSampler, ein.uv).rgb;
    float3 aus = (gWeich > 0.5) ? (g + s * (1.0 - g)) : (g + s);
    return float4(saturate(aus), 1);
}
)";
}

std::string figurVertexShaderHlsl(std::uint32_t features) {
    std::string s;
    s += kKonstanten;
    s += kKnochen;
    s += tcTextMitTafel();   // FIX3, FIX4 P33
    s += "#define VERFORM_NORMAL\n";   // DEF1 P47: ein Figuren-Shader fuer alle Stufen, also mit deformVertexes normal
    s += verformText();      // FIX5: deformVertexes wie an der Karte (verformeEcke)
    // rc602 kartenpruefung: die Shaderstufe einer Figurenflaeche (b3), siehe
    // bhed/figurstufen.h. gStufe.x Farbe (0 wie bisher, 1 weiss, 2 gStufeFarbe,
    // 3 schwarz), .y Alpha (0 eins, 1 lightingSpecular, 2 gStufeFarbe.a),
    // .z tcGen (0 base, 1 environment). Alles 0 = der alte Weg.
    // rc604 fix604b (B4): .w 1 = tcMod - gStufeTcM0..3, gStufeTcO01/23 und
    // gStufeTcArt wie gTcM0.. der Karte (packeKonstanten, FIX4 P33 je Schritt).
    s += R"(
// P60: die Lagen stehen in bhed/gpushader.h (kStufenLagen, kStufeFloats) - gpumap_win32.cpp prueft sie per D3DReflect.
cbuffer JeStufe : register(b3) {
    float4 gStufe;
    float4 gStufeFarbe;
    float4 gAuge;
    float4 gAchse0;
    float4 gAchse1;
    float4 gAchse2;
    float4 gStufeTcM0;    // FIX4 P33: die tcMod-Kette je Schritt wie gTcM0.. der Karte
    float4 gStufeTcO01;
    // rc604: deformVertexes (RB_DeformTessGeometry). FIX5: d0..d3 aus gpu::verformung wie an der Karte
    // (deformWerte, gpumap_win32.cpp); gDef3.w 0 = keine.
    float4 gDef0;
    float4 gDef1;
    float4 gDef2;
    float4 gDef3;
    float4 gStufeTcM1;    // FIX4 P33: zweiter bis vierter Schritt, Verschiebungen, Arten (wie gTcM1.. der Karte)
    float4 gStufeTcM2;
    float4 gStufeTcM3;
    float4 gStufeTcO23;
    float4 gStufeTcArt;
    float4 gEntSkala;     // MERGE3: Massstab von axis[0..2] der Entity (MERGE2), .x 0 = keiner gegeben
};
// rc603 Durchsicht: Q_rsqrt (q_math.c:375 MD / 360 OJ) - 0x5f3759df und EINE
// Newton-Stufe, bis zu 0.175 Prozent zu klein. RB_CalcSpecularAlpha und
// VectorNormalizeFast in RB_CalcEnvironmentTexCoords rechnen damit, nicht
// mit der genauen Wurzel.
float qRsqrt(float zahl) {
    float x2 = zahl * 0.5;
    float y = asfloat(0x5f3759df - (asint(zahl) >> 1));
    return y * (1.5 - (x2 * y * y));
}
)";
    s += R"(
struct VSIn {
    float3 pos     : POSITION;
    float3 normal  : NORMAL;
    float2 uv      : TEXCOORD0;
    float4 bones   : BLENDINDICES;
    float4 weights : BLENDWEIGHT;
};
struct VSOut {
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
    float  clip   : SV_ClipDistance0;
};
VSOut main(VSIn ein) {
    VSOut aus;
    // --- Die Verformung, wie BoneMatrix::transform (gla.cpp:87) ----------
    //
    //     out[i] = m[i][0]*in[0] + m[i][1]*in[1] + m[i][2]*in[2] + m[i][3]
    //
    // Also drei Zeilen mit je vier Werten, die vierte Spalte ist die
    // Verschiebung. Genau so liegen sie im Puffer.
    //
    // Die Gewichte summieren sich zu eins - dafuer sorgt packeEcken()
    // (gpuskin.cpp). Hier wird NICHT noch einmal normiert: eine zweite
    // Normierung wuerde einen Fehler in der ersten verstecken.
    float3 welt = float3(0, 0, 0);
    float3 nrm = float3(0, 0, 0);
    [unroll] for (int i = 0; i < 4; ++i) {
        float w = ein.weights[i];
        if (w <= 0.0) { continue; }
        int b = (int)(ein.bones[i] + 0.5) * 3;
        float4 r0 = gKnochen[b + 0];
        float4 r1 = gKnochen[b + 1];
        float4 r2 = gKnochen[b + 2];
        float3 p = float3(dot(r0.xyz, ein.pos) + r0.w,
                          dot(r1.xyz, ein.pos) + r1.w,
                          dot(r2.xyz, ein.pos) + r2.w);
        welt += p * w;
    }
    // REST3d: die Normale dreht das Spiel NUR mit dem ersten Knochen der Ecke, ungewichtet (G2_SkinSurface,
    // code/rd-vanilla/tr_ghoul2.cpp:3799-3803 MD9; OpenJK RB_SurfaceGhoul :2992-3002 - der gewichtete Zweig :2941-2985 steht
    // unter "#if 0"). Gewichtet war das Glanzband am Unterarm des Protokolldroiden verschoben (p27f/p27u/p27v gegen das
    // Spielbild). OHNE die Verschiebung gedreht - sie ist eine Richtung, kein Ort.
    {
        int b0 = (int)(ein.bones[0] + 0.5) * 3;
        nrm = float3(dot(gKnochen[b0 + 0].xyz, ein.normal),
                     dot(gKnochen[b0 + 1].xyz, ein.normal),
                     dot(gKnochen[b0 + 2].xyz, ein.normal));
    }
    // --- rc604: deformVertexes, im Raum des Modells wie tess.xyz/tess.normal. FIX5: dieselbe Rechnung wie an der Karte
    // (verformeEcke: RB_DeformTessGeometry je Ecke in der Reihenfolge des Shaders, Tafeln und Rauschen des Spiels); bis FIX5 eine
    // eigene Fassung mit sin der Grafikkarte, bulge mit Sekunden statt ms, ohne normal.
    if (gDef3.w > 0.5) {
        welt = verformeEcke(welt, nrm, ein.uv, gDef0, gDef1, gDef2, gDef3, 0, 3);
    }
    // Danach die Stellung der Figur in der Welt.
    float4 w4 = mul(gFigurWelt, float4(welt, 1.0));
    aus.pos = mul(gViewProj, w4);
    aus.clip = dot(w4, gClip);
    aus.uv = ein.uv;
    aus.uvLm = float2(0.0, 0.0);
    // --- Die Beleuchtung aus der Normalen --------------------------------
    //
    // Hier stand `aus.farbe = float4(1, 1, 1, 1)`. Die Normale wurde oben
    // ueber die Knochen gedreht - und dann WEGGEWORFEN. Figuren waren damit
    // voellig flach: Textur mal Helligkeit, ohne jede Schattierung.
    //
    // Sichtbar wurde es an den Battledroids in md_am_sith. Sie sahen mit
    // ausgeschaltetem Gluehen wie schwarze Silhouetten aus und mit
    // eingeschaltetem weiss - eine flache, dunkle Textur mal zwei, die der
    // Gluehdurchgang dann ausblies. Der Bericht zu rc453 hat alle anderen
    // Erklaerungen ausgeschlossen: sechzig Flaechen, alle deckend, alle mit
    // gefundener Textur, keine additiv.
    //
    // Der Rasterer rechnet (mapview.cpp:3362):
    //
    //     shade = 0.22 + 0.78 * |dot(normale, licht)|
    //     licht = normalise(0.4, 0.55, 0.72)
    //
    // Der Betrag ist Absicht: eine Figur, die sich dreht, soll nicht zur
    // Haelfte schwarz werden. Die 0.22 sind das Grundlicht, damit die
    // abgewandte Seite nicht verschwindet.
    //
    // DIESELBE Formel, sonst waeren es zwei Fassungen - und die sind hier
    // schon fuenfmal auseinandergelaufen.
    //
    // --- Das Licht aus dem Gitter der Karte --------------------------------
    //
    // Wie der Rasterer (mapview.cpp, renderActors) und RB_CalcDiffuseColor
    // (tr_shade_calc.cpp): Umgebung + max(0, n.l) * gerichtet, geklemmt.
    // Vorher fehlte es auf dem GPU-Weg ganz - jede Figur war gleich hell,
    // ob im Schatten oder unter einer Lampe.
    //
    // Die Normale erst in die WELT drehen (gFigurWelt ohne Verschiebung):
    // das Gitterlicht kommt aus einer Weltrichtung.
    {
        // Eine Normale der Laenge null (in 684 Modellen der Mod, vor allem
        // Kartenobjekte - tests/meshmasse.cpp) bleibt null: normalize()
        // gaebe NaN und damit schwarze Bildpunkte. Die Engine rechnet mit
        // ihr einfach dot = 0, also nur Umgebungslicht - so auch hier.
        float3 nw = mul((float3x3)gFigurWelt, nrm);
        float nl = length(nw);
        // G2_SkinSurface legt die mit dem ersten Knochen gedrehte Normale ungekuerzt in tess.normal (REST3d, siehe oben) -
        // RB_CalcDiffuseColor nimmt das Punktprodukt mit ihrer Laenge.
        float3 n = (nl > 1e-6) ? nw / nl * length(nrm) : float3(0, 0, 0);
        // PTFIX3 P27: R_SetupEntityLighting dreht die Lichtrichtung mit den Achsen der Entity in den Raum des Modells
        // (tr_light.cpp:492-496 OpenJK, MD9 :486-489), ohne die Laenge auszugleichen - die Achsen tragen den Massstab
        // (ScaleModelAxis, cg_players.cpp:16901 MD9: die "scale" der .npc), die Normalen nicht: dot(n_Modell, A^T l) ist
        // dot(A n_Modell, l), das gerichtete Licht waechst mit der Groesse (gemessen in Movie Duels, Testkarte p27).
        // Glanz und Umgebung rechnen im Spiel mit der Normalen des Modells (n).
        float3 nLicht = nw;
        if (gLichtRichtung.w > 1.5) {
            // rc602: eine Leuchtstufe ohne rgbGen - identityLighting, am
            // Schirm die Bildfarbe selbst (siehe lichtFuer, gpustate.h).
            aus.farbe = float4(1.0, 1.0, 1.0, 1.0);
        } else if (gLichtRichtung.w > 0.5) {
            float ein = max(0.0, dot(nLicht, gLichtRichtung.xyz));
            // RB_CalcDiffuseColor: Q_ftol, dann auf 255 geklemmt. Das 0.001 faengt die Rundung
            // ab, weil die Werte hier durch 255 geteilt ankommen.
            float3 lit = min(float3(1, 1, 1),
                             floor((gLichtUmgebung.rgb + ein * gLichtGerichtet.rgb) * 255.0 + 0.001) / 255.0);
            aus.farbe = float4(lit, 1);
        } else {
            float3 licht = normalize(float3(0.4, 0.55, 0.72));
            float schatten = 0.22 + 0.78 * abs(dot(n, licht));
            aus.farbe = float4(schatten, schatten, schatten, 1);
        }
        // --- rc602 kartenpruefung: die Stufe ---------------------------------
        // ComputeColors (tr_shade.cpp:1403 MD): die Farbe der Stufe.
        if (gStufe.x > 0.5 && gStufe.x < 1.5) {
            aus.farbe.rgb = float3(1.0, 1.0, 1.0);
        } else if (gStufe.x > 1.5 && gStufe.x < 2.5) {
            aus.farbe.rgb = gStufeFarbe.rgb;
        } else if (gStufe.x > 2.5) {
            aus.farbe.rgb = float3(0.0, 0.0, 0.0);
        }
        float3 zumAuge = gAuge.xyz - w4.xyz;
        // rc603 Durchsicht: 1/|viewer| ueber Q_rsqrt wie die Engine (siehe oben).
        float il = qRsqrt(dot(zumAuge, zumAuge));
        // PTFIX3 P27 / MERGE2: eine skalierte Entity (ScaleModelAxis, cg_ents.cpp:332-351 MD9: die Achsen tragen den Massstab,
        // nonNormalizedAxes) rechnet die Engine fuer Glanz und Umgebung im Raum des Modells (R_RotateForEntity,
        // tr_main.cpp:384-403 MD9): A = R diag(s), der Augpunkt viewOrigin = A^T delta / |axis[0]|, die Ecke tess.xyz
        // unskaliert = diag(1/s) R^T (Weltort - Ursprung), lightDir = A^T l (R_SetupEntityLighting, tr_light.cpp:492-496),
        // die Normale die des Modells. R sind die Achsen der Entity (gAchse, ohne den Zeichnerausgleich), Ursprung und s
        // kommen aus gAchse0..2.w / gEntSkala.xyz (Griffe in der Hand: die der Figur), sonst aus gFigurWelt:
        // FIX6 P50: dann sind gAchse0..2 die normierten Spalten 0, 1, 2 von welt (zeichneFigurIn ohne achsenGesetzt), die
        // Massstaebe daher die Spaltenlaengen in derselben Reihenfolge. Unskaliert ist es dieselbe Rechnung wie in
        // Weltkoordinaten (die feste Drehung aendert kein Punktprodukt) - dort bleibt der alte Weg bildpunktgleich.
        float3x3 mw = (float3x3)gFigurWelt;
        float3 achsLaenge = float3(length(mw._m00_m10_m20), length(mw._m01_m11_m21), length(mw._m02_m12_m22));
        bool entGegeben = gEntSkala.x > 0.0;
        float3 sE = entGegeben ? gEntSkala.xyz : achsLaenge;
        float3 oE = entGegeben ? float3(gAchse0.w, gAchse1.w, gAchse2.w)
                               : float3(gFigurWelt._m03, gFigurWelt._m13, gFigurWelt._m23);
        bool skaliert = any(abs(sE - 1.0) > 1e-4);
        float3 dE = gAuge.xyz - oE;
        float3 pE = w4.xyz - oE;
        float3 eL = float3(dot(dE, gAchse0.xyz), dot(dE, gAchse1.xyz), dot(dE, gAchse2.xyz)) * sE / max(sE.x, 1e-6);
        float3 mE = float3(dot(pE, gAchse0.xyz), dot(pE, gAchse1.xyz), dot(pE, gAchse2.xyz)) / max(sE, 1e-6);
        float3 viewerE = eL - mE;
        // REST3c: die Normale des Modells wie tess.normal (G2_SkinSurface: Knochenmatrix mal Normale, ohne den Massstab der
        // Entity - der steckt nur in deren Achsen, ScaleModelAxis): aus der gezeichneten Normalen nw = R diag(s) B n zurueck
        // mit diag(1/s) R^T - auch bei ungleichem Massstab je Achse genau, ohne Normierung (bis REST3c R^T der normierten nw)
        float3 nE = float3(dot(nw, gAchse0.xyz), dot(nw, gAchse1.xyz), dot(nw, gAchse2.xyz)) / max(sE, 1e-6);
        if (gStufe.y > 0.5 && gStufe.y < 1.5) {
            // RB_CalcSpecularAlpha (tr_shade_calc.cpp:994 MD): ein Modell nimmt
            // die Lichtrichtung der Entity (ent->lightDir, aus dem Gitter).
            float3 l = (gLichtRichtung.w > 0.5 && gLichtRichtung.w < 1.5)
                           ? gLichtRichtung.xyz : normalize(float3(0.4, 0.55, 0.72));
            float d = 2.0 * dot(n, l);
            float3 r = n * d - l;
            // ilength = Q_rsqrt(viewer.viewer); l = (reflected . viewer) * ilength;
            // l = l*l; l = l*l; b = l * 255 (abgeschnitten), hoechstens 255.
            float sp = dot(r, zumAuge) * il;
            // PTFIX3 P27: eine skalierte Entity rechnet die Engine im Raum des Modells - lightDir = A^T l mit den skalierten
            // Achsen (tr_light.cpp:492-496), der Augpunkt A^T delta / |axis[0]| (R_RotateForEntity, tr_main.cpp:384-403 MD9)
            // gegen die unskalierte Ecke. Gemessen in Movie Duels (Testkarte p27, scale 150): der Glanz ist deutlich staerker.
            // Unskaliert ist es dieselbe Rechnung wie oben in Weltkoordinaten (die feste Drehung der Figur aendert kein
            // Punktprodukt) - dort bleibt der alte Weg, damit sich kein Bildpunkt aendert.
            if (skaliert) {
                float3 lE = float3(dot(l, gAchse0.xyz), dot(l, gAchse1.xyz), dot(l, gAchse2.xyz)) * sE;
                float3 rE = nE * (2.0 * dot(nE, lE)) - lE;
                sp = dot(rE, viewerE) * qRsqrt(dot(viewerE, viewerE));
            }
            float sp2 = sp * sp;
            sp2 = sp2 * sp2;
            sp = (sp < 0.0) ? 0.0 : min(1.0, floor(sp2 * 255.0) / 255.0);
            aus.farbe.a = sp;
        } else if (gStufe.y > 1.5) {
            aus.farbe.a = gStufeFarbe.a;
        }
        if (gStufe.z > 0.5) {
            // RB_CalcEnvironmentTexCoords (tr_shade_calc.cpp:868 MD) in den
            // Koordinaten der Entity (backEnd.ori): Normale und Blick auf
            // ihre Achsen gedreht.
            // VectorNormalizeFast(viewer): viewer *= Q_rsqrt(viewer.viewer).
            float3 v = zumAuge * il;
            float3 nL = float3(dot(n, gAchse0.xyz), dot(n, gAchse1.xyz), dot(n, gAchse2.xyz));
            float3 vL = float3(dot(v, gAchse0.xyz), dot(v, gAchse1.xyz), dot(v, gAchse2.xyz));
            float dd = dot(nL, vL);
            aus.uv = float2(nL.x * dd - 0.5 * vL.x, nL.y * dd - 0.5 * vL.y);
            if (skaliert) {
                // MERGE2: RB_CalcEnvironmentTexCoords (tr_shade_calc.cpp:902-931 OpenJK) mit tess.normal und dem Blick
                // viewOrigin - tess.xyz im Raum der skalierten Entity (siehe oben).
                float3 vM = viewerE * qRsqrt(dot(viewerE, viewerE));
                float dM = dot(nE, vM);
                aus.uv = float2(nE.x * dM - 0.5 * vM.x, nE.y * dM - 0.5 * vM.y);
            }
        }
        if (gStufe.w > 0.5) {
            // rc604 fix604b (B4): tcMod der Stufe nach ComputeTexCoords
            // (code/rd-vanilla/tr_shade.cpp: erst tcGen, dann die tcMod-Kette),
            // FIX4 P33 Schritt fuer Schritt wie bei der Karte (tcKette). turb
            // (RB_CalcTurbulentTexCoords, tr_shade_calc.cpp) liest tess.xyz -
            // an einem Modell die Ecke im Raum der Entity (RB_SurfaceGhoul legt
            // die gewichteten Knochenorte dort ab), also `welt` VOR gFigurWelt.
            precise float2 ort = float2(welt.x + welt.z, welt.y);
            aus.uv = tcKette(aus.uv, gStufeTcM0, gStufeTcO01, gStufeTcM1, gStufeTcM2, gStufeTcM3, gStufeTcO23, gStufeTcArt, ort, true);
        }
    }
    return aus;
}
)";
    (void)features;
    return s;
}

std::string vertexShaderHlsl(std::uint32_t features, int numTexMods) {
    std::string s;
    s += kKonstanten;
    s += tcTextMitTafel();   // FIX3, FIX4 P33
    s += R"(
struct VSIn {
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
};
struct VSOut {
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
    float  clip   : SV_ClipDistance0;
};
)";
    // FIX5: deformVertexes (verformeEcke) - die Funktionen samt Zufallstafel nur in den Varianten, die sie brauchen
    if ((features & (kDeform | kDeformNormal | kBulge | kDeformMove)) != 0U) {
        // DEF1 P47: ohne kDeformNormal kommt Code 7 nie vor - der Zweig samt Rauschen faellt beim Uebersetzen weg
        if ((features & kDeformNormal) != 0U) { s += "#define VERFORM_NORMAL\n"; }
        s += verformText();
    }
    // rc608: R_SetupEntityLighting legt ambientLight, directedLight und lightDir an die
    // Entity. lightDir ist im Raum des Modells: das Punktprodukt mit e.axis, und die Achsen
    // tragen den Massstab (ScaleModelAxis), die Normalen nicht.
    wenn(s, features, kModellLicht, R"(cbuffer JeModell : register(b7) {
    float4 gModellUmgebung;    // 0..255
    float4 gModellGerichtet;   // 0..255
    float4 gModellRichtung;
};
)");
    s += R"(VSOut main(VSIn ein) {
    VSOut aus;
    float3 welt = ein.pos;
    float3 nrm = ein.normal;
)";

    // deformVertexes wave, normal, bulge, move - FIX5: je Ecke wie RB_DeformTessGeometry in der Reihenfolge des Shaders
    // (verformeEcke, kVerformText), die Welle aus den Tafeln des Spiels, normal mit R_NoiseGet4f, bulge mit refdef.time. Bis FIX5
    // stand hier je Art ein eigener Block: wave mit frac() und sin(t * 2 pi) der Grafikkarte (jede Form als Sinus, frequency 0 wie
    // jede andere), normal mit einer Streufunktion, bulge mit sin(s * breite + Sekunden * tempo), feste Reihenfolge.
    // REST3: autosprite/autosprite2 (beide kAutosprite) rechnet die CPU samt aller deformVertexes des Shaders wie das Spiel
    // (gpu::verformeTess, gpudraw.h; pipelineFor setzt dann keine Verformungsmerkmale) - die Ecke kommt fertig an. Bis REST3 drehte
    // dieser Shader die beim Hochladen vorbereiteten Ecken zur Kamera (Mitte/Richtung/Radius bzw. Mitte der kurzen Kante/Achse).
    const bool verform = (features & (kDeform | kDeformNormal | kBulge | kDeformMove)) != 0U;
    if (verform) {
        s += "    welt = verformeEcke(welt, nrm, ein.uv, gDeform, float4(gDeformSpread, gDeformArt, gDeformNormal.xy), gBulge, gDeformMove, 0, 3);\n";
    }

    // --- Die Stellung des Netzes steckt in gViewProj ---------------------
    //
    // Fruehere Fassung hatte eine eigene `gModellWelt` im STAPEL-Puffer.
    // Das brachte den Puffer von 60 auf 76 float und ein Bild, in dem alle
    // Ecken in einen Punkt liefen - ein Faecher aus roten Strahlen, auch
    // wenn Himmel, Effekte, Gluehen und Figuren abgeschaltet waren.
    //
    // Ein Mover braucht keine zweite Matrix: seine Stellung laesst sich VOR
    // dem Zeichnen in die Kameramatrix multiplizieren. Ein Wert weniger im
    // Puffer ist ein Wert weniger, der falsch liegen kann - und der
    // Stapelpuffer bleibt bei den 60 float, die seit rc392 stimmen.
    s += "    aus.pos = mul(gViewProj, float4(welt, 1.0));\n";

    // tcGen environment - RB_CalcEnvironmentTexCoords (tr_shade_calc.cpp:868 MD, OpenJK gleich): nicht die
    // Spiegelung von Quake 3, sondern st = normal.xy * d - 0.5 * viewer.xy, viewer aus VectorNormalizeFast
    // (Q_rsqrt). Ein gedrehter Mover rechnet in der Engine in seinen Achsen, hier in der Welt.
    if ((features & kTexGenEnv) != 0U) {
        s += R"(    {
        float3 zur = gKamera - welt;
        float zahl = dot(zur, zur);
        float y = asfloat(0x5f3759df - (asint(zahl) >> 1));
        float3 v = zur * (y * (1.5 - (zahl * 0.5 * y * y)));
        float d = dot(nrm, v);
        aus.uv = float2(nrm.x * d - 0.5 * v.x, nrm.y * d - 0.5 * v.y);
    }
)";
    } else if ((features & kTexGenVec) != 0U) {
        // rc605: tcGen vector - ComputeTexCoords TCGEN_VECTOR
        // (code/rd-vanilla/tr_shade.cpp:1675-1680 MD): st = (xyz . v0, xyz . v1)
        // mit tess.xyz nach den deformVertexes; an einem Brush-Modell im Raum
        // des Modells - wie `welt` hier (die Stellung steckt in gViewProj).
        // REST3 Punkt 7: DotProduct (q_math.h) wie im Spiel x*v0 + y*v1 + z*v2 von links, getrennt und precise - dot() wurde dp3
        // (D3D11.3 3.1.3.2: verschmolzen, nicht genau gerundet)
        s += "    {\n";
        s += "        precise float tvS = welt.x * gTcGenS.x + welt.y * gTcGenS.y + welt.z * gTcGenS.z;\n";
        s += "        precise float tvT = welt.x * gTcGenT.x + welt.y * gTcGenT.y + welt.z * gTcGenT.z;\n";
        s += "        aus.uv = float2(tvS, tvT);\n";
        s += "    }\n";
    } else if ((features & kTexGenLm) != 0U) {
        // REST3c: tcGen lightmap - ComputeTexCoords TCGEN_LIGHTMAP (tr_shade.cpp OpenJK/MD): st = tess.texCoords[i][1]
        s += "    aus.uv = ein.uvLm;\n";
    } else {
        s += "    aus.uv = ein.uv;\n";
    }

    // --- Die tcMod-Kette: Schritt fuer Schritt wie rd-vanilla -------------
    //
    // Bis rc442 lief hier eine Schleife ueber bis zu acht Schritte, ohne
    // `tcMod transform`. Von rc443 bis FIX3 stand hier EINE affine Abbildung,
    // auf der CPU zusammengerechnet wie rd-rend2 (`ComputeTexMods`,
    // `ModTexCoords`), turb danach obendrauf durch den linearen Teil der
    // Schritte hinter ihm. Das rundet anders als das Spiel. FIX4 P33: wieder
    // je Schritt, alle sechs Werte, in der Reihenfolge und den Typen von
    // rd-vanilla ComputeTexCoords (tcKette, gpuconst.h kTcM0At).
    // FIX5 Tempo (gleiche Bits): nur so viele Schritte, wie die Stufe hat (numTexMods steht ohnehin im Shadernamen; ein Schritt
    // dahinter hat die Art 0 und laesst st, wie es ist), und der turb-Teil (turbIndex, je turb und Achse rund 30 Rechenschritte) nur
    // in Varianten mit kTurb - ohne turb kommt die Art 2 nie vor, der Zweig faellt beim Uebersetzen weg.
    if ((features & kTexMod) != 0U) {
        static const char* const kSchritt[kMaxTexMods] = {
            "        r = tcSchritt(r, gTcArt.x, gTcM0, gTcO01.xy, ort, %s);\n",
            "        r = tcSchritt(r, gTcArt.y, gTcM1, gTcO01.zw, ort, %s);\n",
            "        r = tcSchritt(r, gTcArt.z, gTcM2, gTcO23.xy, ort, %s);\n",
            "        r = tcSchritt(r, gTcArt.w, gTcM3, gTcO23.zw, ort, %s);\n",
        };
        const char* mitTurb = (features & kTurb) != 0U ? "true" : "false";
        s += "    {\n        precise float2 ort = float2(welt.x + welt.z, welt.y);\n        precise float2 r = aus.uv;\n";
        const int schritte = std::clamp(numTexMods, 1, kMaxTexMods);
        for (int i = 0; i < schritte; ++i) {
            char z[128];
            std::snprintf(z, sizeof z, kSchritt[i], mitTurb);
            s += z;
        }
        s += "        aus.uv = r;\n    }\n";
    }

    wenn(s, features, kLightmap, "    aus.uvLm = ein.uvLm;\n");
    if ((features & kLightmap) == 0U) {
        s += "    aus.uvLm = float2(0.0, 0.0);\n";
    }

    s += "    aus.farbe = ein.farbe;\n";
    // RB_CalcDiffuseColor: je Ecke, incoming <= 0 nur die Umgebung, Q_ftol, dann auf 255 geklemmt.
    wenn(s, features, kModellLicht,
         "    aus.farbe.rgb = min(255.0, floor(gModellUmgebung.rgb + max(0.0, dot(nrm, gModellRichtung.xyz)) * "
         "gModellGerichtet.rgb)) / 255.0;\n");
    // rgbGen const UND rgbGen wave ersetzen die Eckenfarbe - keins von
    // beiden multipliziert sie. Hier stand `*=` fuer die Welle; das machte
    // die Lava um den Faktor der Eckenhelligkeit zu dunkel (gemessen: 0,40).
    //
    // Der Wert steht fuer beide im selben Feld, und der Packer laesst die
    // Welle vorgehen - wie der Rasterer. Deshalb reicht EINE Zeile, und sie
    // darf nicht zweimal dastehen.
    if ((features & (kRgbConst | kRgbWave)) != 0U) {
        s += "    aus.farbe.rgb = gRgb.rgb;\n";
    }
    // alphaGen portal: der Abstand zum Auge durch die Weite, hoechstens 1.
    wenn(s, features, kAlphaPortal,
         "    aus.farbe.a = saturate(length(welt - gKamera) / max(gPortalWeite, 1.0));\n");
    s += "    aus.clip = dot(float4(welt, 1.0), gClip);\n";
    s += "    return aus;\n}\n";
    return s;
}

// --- Welcher Alphawert gilt? -----------------------------------------
//
// Der Rasterer (mapview.cpp:2950):
//
//     alpha = (textur.a / 255) * vonAlphaGen
//     vonAlphaGen = alphaKonst >= 0 ? alphaKonst
//                 : alphaGen == Identity ? 1.0
//                 : eckAlpha
//
// Der Alphakanal der Textur multipliziert IMMER; was davor steht,
// entscheidet alphaGen. Der Shader nahm bis rc443 durchweg den Eckenwert -
// `alphaGen const 0` (unter anderem in der Lava) hatte gar keine Wirkung.
std::string alphaZeile(std::uint32_t features) {
    if ((features & (kAlphaKonst | kAlphaEins)) == (kAlphaKonst | kAlphaEins)) {
        return "    c.a *= 1.0 - ein.farbe.a;\n";   // alphaGen oneMinusVertex
    }
    if ((features & kAlphaKonst) != 0U) {
        return "    c.a *= gRgb.a;\n";
    }
    if ((features & kAlphaEins) != 0U) {
        // alphaGen identity: der Eckenwert gilt NICHT. Die Zeile faellt
        // ganz weg - der Alphakanal der Textur bleibt allein stehen.
        return "";
    }
    return "    c.a *= ein.farbe.a;\n";
}

std::string pixelShaderHlsl(std::uint32_t features) {
    std::string s;
    s += kKonstanten;
    if ((features & kWolke) != 0U) { s += tcTextMitTafel(); }   // FIX4 P33: die tcMod-Kette der Wolkenstufe (tcKette)
    s += R"(
Texture2D    gBild     : register(t0);
Texture2D    gLightmap : register(t1);
// Die sechs Himmelsseiten als Feld - die Seitennummern sind die aus
// skyFace(), damit die Zuordnung nicht zweimal erfunden wird.
Texture2DArray gHimmel : register(t2);
// Das Spiegelbild, so gross wie das Ziel (rc593).
Texture2D    gPortal   : register(t3);
SamplerState gSampler  : register(s0);
// Lightmaps: bilinear, geklemmt, ohne Anisotropie (R_LoadLightmaps ohne Mipmaps, GL_CLAMP; gpumap_win32.cpp g_samplerLm).
SamplerState gLmAbtaster : register(s2);
struct VSOut {
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
    float2 uvLm   : TEXCOORD1;
    float4 farbe  : COLOR0;
};
float4 main(VSOut ein) : SV_TARGET {
)";
    // --- Wolkenschicht (rc592) - R_InitSkyTexCoords ------------------------
    //
    // Die Richtung des Bildpunktes wie beim Himmel unten. Die Engine
    // schneidet sie mit einer Kugel vom Radius 4096 + Hoehe, deren Mitte
    // 4096 unter dem Auge liegt, und nimmt vom Schnittpunkt (normiert, von
    // der Kugelmitte aus) st = (acos x, acos y). Darauf kommt die
    // tcMod-Kette der Stufe - hier im Pixelshader, weil st erst hier
    // entsteht. Unter dem Horizont gibt es keine Wolken (FillCloudBox
    // nimmt von den Seiten nur die obere Haelfte).
    if ((features & kWolke) != 0U) {
        s += R"(    float2 wuv;
    {
        float2 sp = ein.pos.xy;
        float a = (sp.x - gHalb.x) / (gFokus * gHalb.y);
        float b = (gHalb.y - sp.y) / (gFokus * gHalb.y);
        float3 d = normalize(gVorn + gRechts * a + gHoch * b);
        clip(d.z);
        float R = 4096.0;
        float h = gWolkenHoehe;
        float p = -R * d.z + sqrt(R * R * d.z * d.z + 2.0 * R * h + h * h);
        float3 v = normalize(d * p + float3(0.0, 0.0, R));
        float2 st = float2(acos(clamp(v.x, -1.0, 1.0)), acos(clamp(v.y, -1.0, 1.0)));
        // FIX4 P33: Schritt fuer Schritt wie die Karte; turb bleibt hier wie bisher aussen vor (die Ecke der Wolkenkuppel kennt
        // dieser Bildpunkt nicht)
        wuv = tcKette(st, gTcM0, gTcO01, gTcM1, gTcM2, gTcM3, gTcO23, gTcArt, float2(0.0, 0.0), false);
    }
    float4 c = gBild.Sample(gSampler, wuv);
)";
    } else {
        if ((features & kSpiegelBild) != 0U) {
            s += "    float4 c = float4(gPortal.Load(int3(int2(ein.pos.xy), 0)).rgb, 1.0);\n";
        } else if ((features & kLightmapBild) != 0U) {
            // REST3c: das Bild einer weiteren `map $lightmap`-Stufe ist die Lightmap (tr.lightmaps, GL_CLAMP, ohne Mipmaps).
            // REST7: mit ihrem Alpha - aus der .bsp 255 (R_LoadLightmaps, tr_bsp.cpp), extern so, wie es im Bild steht (R_FindLightmap,
            // tr_shader.cpp:3345-3358 MD9: R_FindImageFile laedt lm_%04d.tga mit Alphakanal). duel_exegol: alphaGen const 0.125 mit
            // GL_SRC_ALPHA GL_ONE auf textures/exegol/front - das Alpha der Lightmap (an der Wand im Mittel 8 von 255) haelt die
            // Stufe fast ganz zurueck.
            s += "    float4 c = gLightmap.Sample(gLmAbtaster, ein.uv);\n";
        } else {
            s += "    float4 c = gBild.Sample(gSampler, ein.uv);\n";
        }
    }

    // --- Der Himmel, woertlich wie skyFace() ------------------------------
    //
    // Die Richtung rueckwaerts aus der Projektion:
    //
    //     dot(q,R)/z = (sx - halfW) / (focal * halfH)
    //     dot(q,U)/z = (halfH - sy) / (focal * halfH)
    //     dot(q,F)/z = 1
    //
    // Die Zuordnung Richtung -> Seite und die Koordinaten darauf sind Zeile
    // fuer Zeile aus mapview.cpp:55 uebernommen. Wer hier eine Achse
    // vertauscht, bekommt einen Himmel, der beim Drehen springt - und das
    // faellt erst auf, wenn man sich umsieht.
    if ((features & kSky) != 0U) {
        s += R"(    {
        float2 sp = ein.pos.xy;
        float a = (sp.x - gHalb.x) / (gFokus * gHalb.y);
        float b = (gHalb.y - sp.y) / (gFokus * gHalb.y);
        float3 d = gVorn + gRechts * a + gHoch * b;
        float ax = abs(d.x); float ay = abs(d.y); float az = abs(d.z);
        int face; float sN; float tN;
        if (ax >= ay && ax >= az) {
            if (d.x > 0) { face = 0; sN = -d.y/ax; tN =  d.z/ax; }
            else         { face = 1; sN =  d.y/ax; tN =  d.z/ax; }
        } else if (ay >= az) {
            if (d.y > 0) { face = 2; sN =  d.x/ay; tN =  d.z/ay; }
            else         { face = 3; sN = -d.x/ay; tN =  d.z/ay; }
        } else {
            if (d.z > 0) { face = 4; sN = -d.y/az; tN = -d.x/az; }
            else         { face = 5; sN = -d.y/az; tN =  d.x/az; }
        }
        // Die Seiten sind im Spiel GL_CLAMP und MakeSkyVec rechnet je Ecke, ohne Sprung in den Ableitungen.
        // Sample mit dem umlaufenden, anisotropen Abtaster holte an der Kante zweier Seiten das ganze Bild in
        // einen Bildpunkt: eine Haarlinie im Himmel. Darum Stufe 0 und am Rand der halbe Texel wie CLAMP_TO_EDGE.
        float hb, hh, hn;
        gHimmel.GetDimensions(hb, hh, hn);
        float2 hrand = float2(0.5 / hb, 0.5 / hh);
        float2 huv = clamp(float2((sN + 1.0) * 0.5, 1.0 - (tN + 1.0) * 0.5), hrand, 1.0 - hrand);
        c = gHimmel.SampleLevel(gSampler, float3(huv, face), 0.0);
        return c;
    }
)";
    }
    s += "";

    // alphaFunc. NUR wenn der Shader es verlangt - das war rc385: der Test
    // lief ohne Bedingung, und ueber die Haelfte jeder Figur fiel weg, weil
    // JKA-Texturen fast alle einen Alphakanal tragen.
    wenn(s, features, kAlphaTest,
         "    clip(c.a - gAlphaSchwelle);\n");

    if ((features & kLightmap) != 0U) {
        // --- Mit Lightmap gilt die Eckenfarbe NICHT --------------------
        //
        // Hier stand `c *= ein.farbe;` VOR diesem Block, also fuer beide
        // Zweige. Die lightmapbeleuchteten Flaechen bekamen damit
        //
        //     Textur x Eckenfarbe x Lightmap
        //
        // Der Rasterer nimmt entweder das eine ODER das andere
        // (mapview.cpp:2669, `if (byVertex) ... else if (lm != nullptr)`) -
        // niemals beides.
        //
        // Wie viel das ausmacht, an md_am_sith.bsp nachgemessen: auf
        // lightmapbeleuchteten Flaechen liegt die mittlere Eckenhelligkeit
        // bei 46 von 255, also 0,18. Der GPU-Weg zeigte diese Flaechen
        // damit bei rund einem Fuenftel der richtigen Helligkeit. Genau
        // das war "das Lightning sieht immer noch nicht richtig aus, es
        // ist sehr dunkel".
        //
        // Der ALPHAWERT der Ecke bleibt: er traegt durchscheinende
        // Flaechen, und den nimmt der Rasterer unabhaengig vom Licht
        // (`eckAlpha`, mapview.cpp).
        //
        // Damit faellt auch rgbGen const/wave auf lightmapbeleuchteten
        // Flaechen weg - genau wie im Rasterer, wo der Lightmap-Zweig die
        // Eckenfarbe gar nicht erst ansieht.
        s += alphaZeile(features);
        s += R"(    {
        float3 lm = gLightmap.Sample(gLmAbtaster, ein.uvLm).rgb * gHelligkeit;
        lm = max(lm, gGrundlicht.xxx);
        c.rgb *= lm;
    }
)";
        // rgbGen const/wave der Bildstufe gilt AUCH mit Lightmap: in der
        // Engine sind es zwei Stufen, Lightmap x (Textur x rgbGen).
        if ((features & (kRgbConst | kRgbWave)) != 0U) {
            s += "    c.rgb *= ein.farbe.rgb;\n";
        }
    } else {
        // Farbe UND Alpha aus der Ecke - aber der Alphaanteil haengt an
        // alphaGen, deshalb getrennt.
        s += "    c.rgb *= ein.farbe.rgb;\n";
        s += alphaZeile(features);
        // --- Ohne Lightmap traegt die ECKENFARBE das Licht -----------------
        //
        // Der Rasterer multipliziert sie mit `opt.brightness`
        // (mapview.cpp:2687, `kVertexBoost`). Der Shader tat das nicht - und
        // Flaechen ohne Lightmap kamen dadurch um den Faktor 2 zu dunkel
        // heraus.
        //
        // Gemeldet: "die Lava ist schwarz". Die Lava hat keine Lightmap; ihre
        // Eckenfarben sind dunkel und sollten erst durch den Faktor zum
        // Leuchten kommen.
        //
        // Die Grundhelligkeit gilt hier genauso: sie hebt die dunkelsten
        // Stellen an, ohne die hellen zu veraendern.
        if ((features & kRohEcke) != 0U) {
            // Eine Effektfarbe ist schon fertig - sie kommt aus dem
            // rgb-Block der .efx und darf nicht aufgehellt werden. Der
            // Rasterer setzt hier ausdruecklich den Faktor eins
            // (`kVertexBoost = p.rohEcke ? 1.0F : opt.brightness`,
            // mapview.cpp:2687). Ohne diesen Zweig waren die Effekte auf
            // dem GPU-Weg um den ganzen Helligkeitsfaktor zu hell - bei
            // shanks Einstellung von 2,0x also doppelt.
        } else {
            s += R"(    {
        float3 vf = c.rgb * gHelligkeit;
        c.rgb = max(vf, gGrundlicht.xxx * c.rgb);
    }
)";
        }
    }

    // alphaGen lightingSpecular - eigener Weg, kein Sonderfall der Farbe.
    //
    // Hier stand `gRgb.a`, und darin steckt der alphaConst. Zwei
    // verschiedene alphaGen-Formen teilten sich also einen Wert; eine
    // Flaeche mit lightingSpecular und OHNE alphaGen const bekam die Eins
    // aus dem Vorgabezweig, eine MIT beidem den falschen Wert.
    //
    // Der Rasterer rechnet das Glanzlicht gar nicht (AlphaGen::Unsupported,
    // shaderscript.h). Bis der GPU-Weg es rechnet, bleibt der Alphawert
    // unangetastet - das ist derselbe Stand wie im Rasterer und nicht
    // stillschweigend etwas anderes.
    (void)kSpecular;

    // --- Ueberlauf proportional herunterrechnen, nicht abschneiden --------
    //
    // Der Rasterer (mapview.cpp, "Ueberlauf NICHT einzeln abschneiden")
    // teilt alle drei Kanaele durch den groessten, sobald einer ueber 1
    // steigt - wie R_ColorShiftLightingBytes: "normalize by color instead
    // of saturating to white". Das Renderziel schnitt dagegen JEDEN Kanal
    // einzeln bei 1 ab.
    //
    // Bei 2,0x Helligkeit ist das der Unterschied zwischen Orange und Gelb:
    // die Lava in md_am_sith (rgbGen wave 0,8 x 2,0 = 1,6 x Textur) hat Rot
    // weit ueber 1 und Gruen knapp darunter. Abgeschnitten bleibt Rot bei 1
    // und Gruen holt auf - gelb. Heruntergerechnet bleibt das Verhaeltnis -
    // orange, wie im Rasterer.
    s += R"(    {
        float spitze = max(c.r, max(c.g, c.b));
        if (spitze > 1.0) {
            c.rgb /= spitze;
        }
    }
)";

    s += "    return c;\n}\n";
    return s;
}

std::string shaderName(std::uint32_t features, int numTexMods) {
    std::string n = "fx";
    struct Eintrag {
        std::uint32_t bit;
        const char* kuerzel;
    };
    // Feste Reihenfolge: gleiche Bits, gleicher Name - sonst erkennt man
    // wiederverwendbare Shader nicht wieder.
    static const Eintrag kListe[] = {
        {kLightmap, "_lm"},      {kTexMod, "_tc"},
        {kRgbWave, "_rw"},       {kCullNone, "_c2"},
        {kCullBack, "_cb"},      {kTexGenEnv, "_env"},
        {kAlphaTest, "_at"},     {kPolygonOffset, "_po"},
        {kAutosprite, "_as"},    {kDeform, "_df"},
        {kRgbConst, "_rc"},      {kSpecular, "_sp"},
        // alphaGen: DREI verschiedene Quelltexte, also drei Kuerzel.
        {kAlphaKonst, "_ak"},    {kAlphaEins, "_a1"},
        {kSky, "_sky"},
        {kBulge, "_bu"},
        {kDeformMove, "_mv"},    {kDeformNormal, "_dn"},
        {kWolke, "_wk"},
        {kAlphaPortal, "_ap"},      {kSpiegelBild, "_spb"},
        {kTexGenVec, "_tcv"},   // rc605
        {kModellLicht, "_ml"},  // rc608
        {kTurb, "_tb"},         // FIX5
        // REST6: tcGen lightmap und das Lightmap-Bild einer weiteren `map $lightmap`-Stufe (REST3c) aendern den Quelltext - ohne Kuerzel
        // galt fuer sie der zuerst uebersetzte Shader gleichen Namens (etwa der einer Stufe mit rgbGen const ohne Lightmap).
        {kTexGenLm, "_tlm"},    {kLightmapBild, "_lmb"},
        // kRohEcke MUSS ein Kuerzel bekommen: es aendert den erzeugten
        // Quelltext (kein Helligkeitsfaktor auf der Eckenfarbe). Ohne
        // Kuerzel bekaemen zwei verschiedene Shader denselben Namen und
        // teilten sich einen Zwischenspeicher - der erste, der uebersetzt
        // wird, gaelte dann fuer beide.
        {kRohEcke, "_re"},
        // kGlow bekommt KEIN Kuerzel: es aendert nicht, WIE gezeichnet wird,
        // sondern ob der Stapel in den Gluehdurchgang gehoert. Zwei Shader
        // dafuer waeren zwei Uebersetzungen desselben Quelltexts.
    };
    for (const Eintrag& e : kListe) {
        if ((features & e.bit) != 0U) {
            n += e.kuerzel;
        }
    }
    if (numTexMods > 0) {
        n += std::to_string(numTexMods);
    }
    return n;
}

}  // namespace bhed::gpu
