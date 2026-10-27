// bhed_gpubacken.hlsl - BehavEd Stufe 8c/8g: die Compute-Shader des GPU-Backens
// (q3map2 -light -bhedgpubake, src/q3map2/bhed_gpubacken.cpp). Im Bau mit fxc aus dem
// Windows SDK vorab uebersetzt (src/q3map2/CMakeLists.txt, Bytecode als Header in der
// exe) - zur Laufzeit wird nichts uebersetzt, d3dcompiler_47.dll wird nicht gebraucht.
//
// LightContributionToSample, SetupTrace, TraceLine/TraceLine_r/TraceTriangle und
// PointToPolygonFormFactor aus q3map2 (light.cpp, light_trace.cpp), Zeile fuer Zeile
// (siehe bhed_gpubacken.cpp), seit Stufe 8g dazu SubsampleRawLuxel_r, SubmapRawLuxel,
// ClusterForPointExtFilter und der Rest der Lichtschleife von IlluminateRawLightmap
// (light_ydnar.cpp): Erstabtastung, Vorhersage und Unterabtastung, der genaue Stempellauf,
// Filtern und Aufsummieren in Lichtreihenfolge. Einstiege: erst, kandidaten, unter,
// stempellauf, filtern, gitterKern (TraceGrid). Makros: DOPPELT (die GPU rechnet double), MESS (Messschalter
// BHED_GPUBAKE_MESS), DIAG (BHED_GPUBAKE_DIAG).
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors.
//   fdlibm e_acos.c: Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//   Developed at SunSoft, a Sun Microsystems, Inc. business. Permission to use, copy,
//   modify, and distribute this software is freely granted, provided that this notice
//   is preserved.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
// X4714: nur ein Leistungshinweis zum Stapel der Traversierung (64 Eintraege je Faden)
#pragma warning(disable : 4714)
// X3557: im Gitter hat die Spur keine eigenen Flaechen (Schleife ueber 0 Flaechen faellt weg)
#pragma warning(disable : 3557)
#ifndef DOPPELT
#define DOPPELT 1
#endif
#ifndef MESS
#define MESS 0
#endif
#ifndef DIAG
#define DIAG 0
#endif
struct Knoten { int typ; float nx; float ny; float nz; float dist; int k0; int k1; int anzahl; int erstes; int pad; };
struct Dreieck { float v0x; float v0y; float v0z; float e1x; float e1y; float e1z; float e2x; float e2y; float e2z; int info; };
struct Info { int cast; int skipGrid; int flaeche; int flags; int skyIndex; int p0; int p1; int p2; };
struct Licht { int typ; int flags; int cluster; int windErste; float ox; float oy; float oz; float nx; float ny; float nz; float dist; float photons;
               float cr; float cg; float cb; float radiusByDist; float fade; float angleScale; float extraDist; float add; float envelope; float falloffTol;
               int skyIndex; int windAnzahl;
               float minx; float miny; float minz; float maxx; float maxy; float maxz; int style; int lpad; };
// Ein abgebildeter Luxel (Cluster >= 0) einer Roh-Lightmap im Block: Super-Ursprung, -Normale, Cluster
struct Luxel { float ox; float oy; float oz; float nx; float ny; float nz; int cluster; int lm; int x; int y; };
// Eine Roh-Lightmap im Block (BhedBakeLm in bhed_gpubacken.h)
struct Lm
{
    int recv; int sErste; int sAnzahl; int twoSided;
    int sw; int sh; int m; int luxelBase;
    int idxBase; int lichtBase; int nLichter; int paarBase;
    int clusterBase; int nClusters; int aktivBase; int nAktiv;
    float ar; float ag; float ab; int zeilenBase;
    int aus0; int aus1; int aus2; int aus3;
    int start; int lpad0; int lpad1; int lpad2;   // start: Stilplaetze ab startWert (Teil einer grossen Lightmap)
};
// Ein Licht der Lightmap, das die Stil-Pruefung bestanden und etwas beleuchtet hat (in Lichtreihenfolge)
struct Aktiv { int j; int slot; int radius; int negativ; };
// BSP fuer ClusterForPointExt (Unterabtastung): Knoten, Ebenen, Blaetter, Brushes
struct BspKnoten { int ebene; int k0; int k1; int pad; };
struct Ebene { float nx; float ny; float nz; float dist; };
struct Blatt { int cluster; int ersterBrush; int anzahlBrushes; int pad; };
struct Brush { int ersteSeite; int anzahlSeiten; int opak; int pad; };
// ein Punkt des Lichtgitters (rawGridPoint_t): Ursprung und Cluster aus dem Vorlauf, dir, ambient,
// directed, styles (4 Byte); zurueck dazu neu (in diesem Lauf vergebene Stile) und flag (1: die CPU)
struct GitterPunkt
{
    float ox; float oy; float oz; int cluster;
    float dx; float dy; float dz; uint stile;
    float a0; float a1; float a2; float a3; float a4; float a5; float a6; float a7; float a8; float a9; float a10; float a11;
    float g0; float g1; float g2; float g3; float g4; float g5; float g6; float g7; float g8; float g9; float g10; float g11;
    uint neu; int flag; int gpad0; int gpad1;
};
// der Stand eines Gitterpunkts zwischen den Dispatches (je einer fuer einen Teil der Lichter)
struct GitterZustand
{
    int numCon; int abbruch; int gezaehlt; int numStyles;
    float bx; float by; float bz; float tx;
    float ty; float tz; int zpad0; int zpad1;
};
cbuffer Kopf : register(b0)
{
    uint anzahl; uint basis; int kopfKnoten; int faster;
    int lightAngleHL; int bouncing; int testOcclusion; int oneSky;
    int noSurfaces; float linearScale; uint visBytes; int leafBytes;
    int nLms; int lightSamples; float suchBox; int maxOpaqueBrush;
    int nAktivGesamt; int nLuxel; int nZeilen; float inhibitGitter;
    float gridAmbientDirectionality; float gridDirectionality; int cheapgrid; int nGitterLichter;
    int gitterPhase; int lichtVon; int lichtBis; int kpad2;
};
StructuredBuffer<Knoten> knoten : register(t0);
StructuredBuffer<int> items : register(t1);
StructuredBuffer<Dreieck> dreiecke : register(t2);
StructuredBuffer<Info> infos : register(t3);
StructuredBuffer<int> flaechen : register(t4);
ByteAddressBuffer vis : register(t5);
StructuredBuffer<Licht> lichter : register(t6);
StructuredBuffer<float> windungen : register(t7);
StructuredBuffer<Luxel> luxel : register(t8);
StructuredBuffer<Lm> lms : register(t9);
StructuredBuffer<int> idx : register(t10);
StructuredBuffer<int> lichtListe : register(t11);
StructuredBuffer<int> lmCluster : register(t12);
StructuredBuffer<Aktiv> aktiv : register(t13);
StructuredBuffer<int> aktivNr : register(t14);
StructuredBuffer<BspKnoten> bspKnoten : register(t15);
StructuredBuffer<Ebene> ebenen : register(t16);
StructuredBuffer<Blatt> blaetter : register(t17);
StructuredBuffer<int> blattBrushes : register(t18);
StructuredBuffer<Brush> brushes : register(t19);
StructuredBuffer<int> seiten : register(t20);
StructuredBuffer<float4> startWert : register(t21);
RWStructuredBuffer<float4> P : register(u0);       // je Paar: Farbe der Erstabtastung (danach nachabgetastet)
RWStructuredBuffer<uint> F : register(u1);         // je Paar: 1 FORCE_SUBSAMPLING, 2 ALREADY_SUBSAMPLED, 4 Kandidat
RWStructuredBuffer<float4> S : register(u2);       // je Paar: vorab nachabgetastete Farbe der Kandidaten
RWStructuredBuffer<uint> zaehler : register(u3);   // je Licht der Lightmap: beleuchtete Luxel (totalLighted)
RWStructuredBuffer<uint> lmFlag : register(u4);    // je Lightmap: 1 = die CPU rechnet sie (Alpha-Schatten, unsicher)
RWStructuredBuffer<float4> R : register(u5);       // fertige Luxel je benutztem Stil
RWStructuredBuffer<GitterPunkt> gitter : register(u6);   // Lichtgitter
RWStructuredBuffer<GitterZustand> zustand : register(u7); // Stand je Gitterpunkt
// dieselben Register in den Lightmap-Kernen (die Gitter-Puffer brauchen sie nicht; D3D11.0 hat 8 UAVs):
RWStructuredBuffer<uint> kandListe : register(u6);   // die vorhergesagten Paare (Unterabtastung)
RWStructuredBuffer<uint> hilfe : register(u7);       // [0] Zahl der Kandidaten - 1, [1 + a] erster vorhergesagter Stempel je aktivem Licht

static bool g_unsicher;
static float g_inhibit = 1.5f;     // trace->inhibitRadius (Lightmaps 1,5; Gitter die halbe Gitterweite)
static bool g_imGitter = false;    // inGrid: Patches (skipGrid) werfen keinen Schatten aufs Gitter

static const float BARY_EPS = 0.01f;
static const float BARY_EINS = 1.01f;
static const float COPLANAR = 0.25f;
static const float SELBST = 0.5f;
static const float TRACE_ON = 0.1f;
static const float INHIBIT = 1.5f;

// IEEE-754 float-Division, genau gerundet (round to nearest even) - nur normale Ergebnisse
float teilen(float a, float b)
{
    uint ua = asuint(a);
    uint ub = asuint(b);
    uint s = (ua ^ ub) & 0x80000000u;
    int ea = int((ua >> 23) & 0xffu);
    int eb = int((ub >> 23) & 0xffu);
    uint ma = ua & 0x7fffffu;
    uint mb = ub & 0x7fffffu;
    if (ea == 255 || eb == 255 || (eb == 0 && mb == 0)) { g_unsicher = true; return 0.0f; }
    if (ea == 0 && ma == 0) { return asfloat(s); }
    if (ea == 0) { ea = 1; [loop] while ((ma & 0x800000u) == 0u) { ma <<= 1; ea -= 1; } } else { ma |= 0x800000u; }
    if (eb == 0) { eb = 1; [loop] while ((mb & 0x800000u) == 0u) { mb <<= 1; eb -= 1; } } else { mb |= 0x800000u; }
    int e = ea - eb + 127;
    if (ma < mb) { ma <<= 1; e -= 1; }
    uint q = 0u;
    uint r = ma;
    [loop] for (int k = 0; k < 25; ++k) { q <<= 1; if (r >= mb) { r -= mb; q |= 1u; } r <<= 1; }
    uint guard = q & 1u;
    q >>= 1;
    if (guard != 0u && (r != 0u || (q & 1u) != 0u)) { q += 1u; if (q == 0x1000000u) { q >>= 1; e += 1; } }
    if (e <= 0 || e >= 255) { g_unsicher = true; return 0.0f; }
    return asfloat(s | (uint(e) << 23) | (q & 0x7fffffu));
}

float punkt3(float3 a, float3 b) { precise float d = a.x * b.x + a.y * b.y + a.z * b.z; return d; }
float3 kreuz(float3 a, float3 b)
{
    precise float x = a.y * b.z - a.z * b.y;
    precise float y = a.z * b.x - a.x * b.z;
    precise float z = a.x * b.y - a.y * b.x;
    return float3(x, y, z);
}

// ---------------------------------------------------------------------------------------
// Stufe 8g: float-float-Rechnung (Dekker/Knuth, nur float-Addition und -Multiplikation, die
// D3D11 genau rundet; "precise" verhindert mad). Damit entscheiden Wurzel und Normalisieren
// ihr genau gerundetes Ergebnis fast immer ohne double-Befehle (die auf Verbraucher-GPUs
// sehr langsam sind); nur wenn der Wert sehr nah an einer float-Mitte liegt, rechnet der
// genaue double-Weg (ohne double: die CPU).
// ---------------------------------------------------------------------------------------

// a * b = p + e genau (Dekker, Teilung mit 4097), |a|, |b| < 2^60
void zweiProd(float a, float b, out float p, out float e)
{
    precise float ca = a * 4097.0f;
    precise float ahi = ca - (ca - a);
    precise float alo = a - ahi;
    precise float cb = b * 4097.0f;
    precise float bhi = cb - (cb - b);
    precise float blo = b - bhi;
    precise float pp = a * b;
    precise float t1 = ahi * bhi - pp;
    precise float t2 = t1 + ahi * blo;
    precise float t3 = t2 + alo * bhi;
    precise float ee = t3 + alo * blo;
    p = pp;
    e = ee;
}
// a + b = s + e genau (Knuth)
void zweiSumme(float a, float b, out float s, out float e)
{
    precise float ss = a + b;
    precise float bb = ss - a;
    precise float ee = (a - (ss - bb)) + (b - bb);
    s = ss;
    e = ee;
}
// halbe ulp von f zur Seite des Rests d hin (unterhalb einer Zweierpotenz die Haelfte); 0: zu klein/gross
float halbeUlp(float f, float d)
{
    const uint u = asuint(f);
    const uint e = (u >> 23) & 0xffu;
    if (e <= 25u || e >= 254u) { return 0.0f; }
    float h = asfloat((e - 24u) << 23);   // ulp / 2
    if ((u & 0x7fffffu) == 0u && ((d < 0.0f) == (f > 0.0f))) { h = h * 0.5f; }
    return h;
}
// der Wert hi + lo (relativer Fehler unter 2^-43 gegen den Wert der CPU) als float: false, wenn
// er naeher als 2^-38 * |Wert| an einer float-Mitte liegt (dann der genaue Weg)
bool ffNachFloat(float hi, float lo, out float f)
{
    precise float ff = hi + lo;
    f = ff;
    precise float d0 = hi - ff;      // genau (Sterbenz)
    precise float d = d0 + lo;
    const float h = halbeUlp(ff, d);
    if (h == 0.0f) { return false; }
    precise float rest = h - abs(d);
    precise float schranke = abs(ff) * 3.637979e-12f;   // 2^-38
    return rest > schranke;
}
// genau gerundete float-Wurzel ohne double, wenn eindeutig: x > 0; false: knapp (der genaue Weg)
bool fwurzelSchnell(float x, out float s)
{
    s = sqrt(x);
    if (!(x > 1e-20f && x < 1e30f)) { return false; }
    [unroll] for (int k = 0; k < 3; ++k)
    {
        float ph; float pl;
        zweiProd(s, s, ph, pl);
        precise float r1 = x - ph;   // genau (Sterbenz)
        precise float r = r1 - pl;   // x - s*s mit einer Rundung
        const uint e = (asuint(s) >> 23) & 0xffu;
        const float u = asfloat((e - 23u) << 23);
        const float uUnten = (asuint(s) & 0x7fffffu) == 0u ? u * 0.5f : u;
        precise float oben = s * u;            // genau
        precise float unten = s * uUnten;      // genau
        precise float tol = abs(r) * 2.3841858e-7f + u * u;   // 2^-22
        if (r < oben - tol && r > -unten + tol) { return true; }
        if (r > oben + tol) { s = asfloat(asuint(s) + 1u); continue; }
        if (r < -unten - tol) { s = asfloat(asuint(s) - 1u); continue; }
        return false;
    }
    return false;
}
// x / (q + ql) als float, wenn eindeutig (normalisierenSchnell)
bool komponente(float x, float q, float ql, out float f)
{
    f = x;
    if (x == 0.0f) { return true; }
    const float t = x / q;
    float p; float pe;
    zweiProd(t, q, p, pe);
    precise float rx = ((x - p) - pe) - t * ql;
    precise float t2 = rx / q;
    return ffNachFloat(t, t2, f);
}
// VectorNormalize ohne double, wenn eindeutig: v != 0; false: knapp (der genaue Weg)
bool normalisierenSchnell(float3 v, out float3 n, out float laenge)
{
    n = v;
    laenge = 0.0f;
    const float m = max(abs(v.x), max(abs(v.y), abs(v.z)));
    if (!(m > 1e-10f && m < 1e15f)) { return false; }
    float a; float ae; float b; float be; float c; float ce;
    zweiProd(v.x, v.x, a, ae);
    zweiProd(v.y, v.y, b, be);
    zweiProd(v.z, v.z, c, ce);
    float s1; float e1; float s2; float e2;
    zweiSumme(a, b, s1, e1);
    zweiSumme(s1, c, s2, e2);
    precise float fehler = ((e1 + e2) + (ae + be)) + ce;
    float h; float l;
    zweiSumme(s2, fehler, h, l);                 // l2 = h + l
    const float q = sqrt(h);
    float qq; float qe;
    zweiProd(q, q, qq, qe);
    precise float rest = ((h - qq) - qe) + l;
    precise float ql = rest / (2.0f * q);        // sqrt(l2) = q + ql
    float fl;
    if (!ffNachFloat(q, ql, fl)) { return false; }
    float fa; float fb; float fc;
    if (!komponente(v.x, q, ql, fa) || !komponente(v.y, q, ql, fb) || !komponente(v.z, q, ql, fc)) { return false; }
    const float3 erg = float3(fa, fb, fc);
    n = erg;
    laenge = fl;
    return true;
}

#if DOPPELT
double dk(uint lo, uint hi) { return asdouble(lo, hi); }
double naechster(double x, int richtung)   // x > 0
{
    uint lo; uint hi;
    asuint(x, lo, hi);
    if (richtung > 0) { lo += 1u; if (lo == 0u) { hi += 1u; } }
    else { if (lo == 0u) { hi -= 1u; } lo -= 1u; }
    return asdouble(lo, hi);
}
// Stufe 8g: dieselben genau gerundeten Ergebnisse wie zuvor, aber ohne double-Division
// (die kostet auf Verbraucher-GPUs ein Vielfaches): Startwerte aus float, Verfeinern mit
// double-fma, am Ende der Test nach Tuckerman bzw. ein Mitten-Test, der nur in den sehr
// seltenen knappen Faellen auf den langsamen genauen Weg geht.

// Naeherung von sqrt(x) und 1/sqrt(x) in double (relativer Fehler um 2^-50), 1e-30 < x < 1e30
void wurzelNah(double x, out double l, out double r)
{
    const float lf = sqrt((float)x);
    const float rf = 1.0f / lf;
    precise double s = (double)lf;
    precise double q = (double)rf;
    const double halb = dk(0u, 0x3fe00000u);
    const double eins = dk(0u, 0x3ff00000u);
    [unroll] for (int k = 0; k < 2; ++k)
    {
        precise double d = fma(-s, s, x);
        precise double hq = halb * q;
        s = fma(hq, d, s);
        precise double e = fma(-s, q, eins);
        q = fma(q, e, q);
    }
    l = s;
    r = q;
}
// genau gerundete double-Wurzel, x > 0 normal
double dwurzel(double x)
{
    if (!(x > dk(0u, 0x39b4484bu) && x < dk(0u, 0x46293e59u))) { g_unsicher = true; return 1.0; }   // 1e-30 .. 1e30
    precise double s;
    precise double r;
    wurzelNah(x, s, r);
    [unroll] for (int k = 0; k < 3; ++k)
    {
        precise double sm = naechster(s, -1);
        precise double sp = naechster(s, 1);
        if (fma(s, sm, -x) >= 0.0) { s = sm; }
        else if (fma(s, sp, -x) < 0.0) { s = sp; }
    }
    return s;
}
float naechsterF(float s, int richtung)   // s > 0 normal
{
    return asfloat(asuint(s) + (richtung > 0 ? 1u : 0xffffffffu));
}
// genau gerundete float-Wurzel: erst ohne double (fwurzelSchnell), in knappen Faellen mit den
// Mitten zu den Nachbarn in double (deren Quadrat ist in double genau: 25 Bit * 25 Bit)
float fwurzel(float x)
{
    if (x <= 0.0f) { return 0.0f; }
    float s;
    if (fwurzelSchnell(x, s)) { return s; }
    if (!(x > 1e-30f && x < 1e30f)) { g_unsicher = true; return 1.0f; }
    s = sqrt(x);
    const double dx = (double)x;
    const double halb = dk(0u, 0x3fe00000u);
    [unroll] for (int k = 0; k < 3; ++k)
    {
        precise double oben = ((double)s + (double)naechsterF(s, 1)) * halb;
        precise double unten = ((double)s + (double)naechsterF(s, -1)) * halb;
        precise double o2 = oben * oben;
        precise double u2 = unten * unten;
        if (o2 < dx) { s = naechsterF(s, 1); }
        else if (u2 > dx) { s = naechsterF(s, -1); }
    }
    precise double o = ((double)s + (double)naechsterF(s, 1)) * halb;
    precise double u = ((double)s + (double)naechsterF(s, -1)) * halb;
    precise double oo = o * o;
    precise double uu = u * u;
    if (oo < dx || uu > dx) { g_unsicher = true; }
    return s;
}
// VectorNormalize (common/qmath.h): in double; die Laenge als float. Erst ohne double
// (normalisierenSchnell), in knappen Faellen genau in double.
float normalisieren(inout float3 v)
{
    if (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f) { v = float3(0.0f, 0.0f, 0.0f); return 0.0f; }
    float3 n;
    float laenge;
    if (normalisierenSchnell(v, n, laenge)) { v = n; return laenge; }
    precise double x = (double)v.x;
    precise double y = (double)v.y;
    precise double z = (double)v.z;
    precise double l2 = x * x + y * y + z * z;
    if (l2 == 0.0) { v = float3(0.0f, 0.0f, 0.0f); return 0.0f; }
    precise double l = dwurzel(l2);
    precise double a = x / l;
    precise double b = y / l;
    precise double c = z / l;
    v = float3((float)a, (float)b, (float)c);
    return (float)l;
}
// fdlibm __ieee754_acos, |x| <= 1
double dacos(double x)
{
    const double pio2_hi = dk(0x54442d18u, 0x3ff921fbu);
    const double pio2_lo = dk(0x33145c07u, 0x3c91a626u);
    const double pi = dk(0x54442d18u, 0x400921fbu);
    const double pS0 = dk(0x55555555u, 0x3fc55555u);
    const double pS1 = dk(0x03eb6f7du, 0xbfd4d612u);
    const double pS2 = dk(0x0e884455u, 0x3fc9c155u);
    const double pS3 = dk(0xb5688f3bu, 0xbfa48228u);
    const double pS4 = dk(0x7501b288u, 0x3f49efe0u);
    const double pS5 = dk(0x0dfdf709u, 0x3f023de1u);
    const double qS1 = dk(0x1c8a2d4bu, 0xc0033a27u);
    const double qS2 = dk(0x9c598ac8u, 0x40002ae5u);
    const double qS3 = dk(0x1b8d0159u, 0xbfe6066cu);
    const double qS4 = dk(0xb12e9282u, 0x3fb3b8c5u);
    const double one = dk(0u, 0x3ff00000u);
    const double half = dk(0u, 0x3fe00000u);
    const double two = dk(0u, 0x40000000u);
    uint lo; uint hx;
    asuint(x, lo, hx);
    uint ix = hx & 0x7fffffffu;
    if (ix >= 0x3ff00000u)
    {
        if (((ix - 0x3ff00000u) | lo) == 0u)
        {
            if ((hx & 0x80000000u) == 0u) { return 0.0; }
            return pi;   // fdlibm: pi + 2 * pio2_lo, in double genau pi (fxc faltet es sonst mit Warnung X4122)
        }
        g_unsicher = true;
        return 0.0;
    }
    precise double z; precise double p; precise double q; precise double r; precise double w; precise double s;
    if (ix < 0x3fe00000u)
    {
        if (ix <= 0x3c600000u) { return pio2_hi; }   // fdlibm: pio2_hi + pio2_lo, in double genau pio2_hi
        z = x * x;
        p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
        q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
        r = p / q;
        return pio2_hi - (x - (pio2_lo - x * r));
    }
    if ((hx & 0x80000000u) != 0u)
    {
        z = (one + x) * half;
        p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
        q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
        s = dwurzel(z);
        r = p / q;
        w = r * s - pio2_lo;
        return pi - two * (s + w);
    }
    z = (one - x) * half;
    s = dwurzel(z);
    uint slo; uint shi;
    asuint(s, slo, shi);
    precise double df = asdouble(0u, shi);
    precise double c = (z - df * df) / (s + df);
    p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
    q = one + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
    r = p / q;
    w = r * s + c;
    return two * (df + w);
}
#else
float fwurzel(float x) { if (x <= 0.0f) { return 0.0f; } float s; fwurzelSchnell(x, s); return s; }
float normalisieren(inout float3 v)
{
    if (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f) { v = float3(0.0f, 0.0f, 0.0f); return 0.0f; }
    float3 n;
    float laenge;
    if (normalisierenSchnell(v, n, laenge)) { v = n; return laenge; }
    precise float l2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (l2 == 0.0f) { v = float3(0.0f, 0.0f, 0.0f); return 0.0f; }
    precise float l = sqrt(l2);
    v = float3(teilen(v.x, l), teilen(v.y, l), teilen(v.z, l));
    return l;
}
#endif

bool clusterSichtbar(int a, int b)
{
    if (a < 0 || b < 0) { return false; }
    if (a == b) { return true; }
    if (visBytes <= 8u) { return true; }
    const uint stelle = 8u + uint(a) * uint(leafBytes) + (uint(b) >> 3);
    const uint wort = vis.Load(stelle & ~3u);
    const uint byte = (wort >> ((stelle & 3u) * 8u)) & 0xffu;
    return (byte & (1u << (uint(b) & 7u))) != 0u;
}

struct Strahl
{
    float3 o; float3 e; float3 d; float3 disp; float dist;
};

float strahlSetzen(float3 o, float3 e, out Strahl s)   // SetupTrace
{
    precise float3 disp = e - o;
    s.o = o; s.e = e; s.disp = disp;
    float3 d = disp;
    s.dist = normalisieren(d);
    s.d = d;
    return s.dist;
}

// TraceTriangle fuer die Dreiecke eines Blatts. 0 weiter, 1 undurchsichtig, 3 die CPU.
// himmel/skyOk: compileFlags & C_SKY und das skyIndex-Bit des Lichts.
int blattTesten(int knr, Strahl r, Lm lm, int lichtSky, inout bool himmel, inout bool skyOk)
{
    const Knoten n = knoten[knr];
    [loop] for (int j = 0; j < n.anzahl; ++j)
    {
        const Dreieck d = dreiecke[items[n.erstes + j]];
        const Info f = infos[d.info];
        const bool istHimmel = (f.flags & 1) != 0;
        if (himmel && istHimmel) { continue; }
        if (lm.recv == 1) { if (f.cast <= 0) { continue; } }
        else if (lm.recv > 1) { if (f.cast != 1 && abs(f.cast) != abs(lm.recv)) { continue; } }
        else { if (abs(f.cast) != abs(lm.recv)) { continue; } }
        if (g_imGitter && f.skipGrid != 0) { continue; }
        const float3 v0 = float3(d.v0x, d.v0y, d.v0z);
        const float3 e1 = float3(d.e1x, d.e1y, d.e1z);
        const float3 e2 = float3(d.e2x, d.e2y, d.e2z);
        precise float3 pvec = kreuz(r.d, e2);
        precise float det = punkt3(e1, pvec);
        if (abs(det) < COPLANAR) { continue; }
        precise float3 tvec = r.o - v0;
        precise float uu = punkt3(tvec, pvec);
        {
            const float invA = 1.0f / det;
            const float uA = uu * invA;
            const float tolU = 1e-3f * max(1.0f, abs(uA));
            if (uA < -BARY_EPS - tolU || uA > BARY_EINS + tolU) { continue; }
        }
        precise float invDet = teilen(1.0f, det);
        precise float u = uu * invDet;
        if (u < -BARY_EPS || u > BARY_EINS) { continue; }
        precise float3 qvec = kreuz(tvec, e1);
        precise float vv = punkt3(r.d, qvec);
        precise float v = vv * invDet;
        precise float uv = u + v;
        if (v < -BARY_EPS || uv > BARY_EINS) { continue; }
        precise float tt = punkt3(e2, qvec);
        precise float tiefe = tt * invDet;
        if (tiefe <= g_inhibit || tiefe >= r.dist) { continue; }
        if (tiefe <= SELBST)
        {
            bool eigen = false;
            for (int s = 0; s < lm.sAnzahl && !eigen; ++s) { eigen = flaechen[lm.sErste + s] == f.flaeche; }
            if (eigen) { continue; }
        }
        if (istHimmel)
        {
            himmel = true;
            if (oneSky == 0 && lichtSky != -1 && f.skyIndex == lichtSky) { skyOk = true; }
            continue;
        }
        if ((f.flags & 2) != 0) { return 3; }
        return 1;
    }
    return 0;
}

// TraceLine: 0 frei, 1 undurchsichtig (Dreieck oder festes Blatt bei !testAll), 3 die CPU
int strahlTesten(Strahl r, Lm lm, bool testAll, int lichtSky, out bool himmel, out bool skyOk)
{
    himmel = false;
    skyOk = false;
    if (lm.recv == 0 || testOcclusion == 0 || r.dist <= 0.00001f) { return 0; }
#if MESS == 2
    himmel = true;
    return 0;
#endif
    int nTest = 0;
    int stK[64];
    float3 stO[64];
    float3 stE[64];
    int sp = 0;
    int k = kopfKnoten;
    precise float3 o = r.o;
    precise float3 en = r.e;
    [loop] while (true)
    {
        if (k < 0) { return testAll ? 0 : 2; }
        const Knoten n = knoten[k];
        if (n.typ == -2) { return testAll ? 0 : 2; }
        if (n.typ < 0)
        {
            if (n.anzahl > 0 && nTest < 256)
            {
                nTest += 1;
                if (noSurfaces == 0 && MESS == 0)
                {
                    const int b = blattTesten(k, r, lm, lichtSky, himmel, skyOk);
                    if (b != 0) { return b; }
                }
            }
            if (sp == 0) { break; }
            sp -= 1;
            k = stK[sp]; o = stO[sp]; en = stE[sp];
            continue;
        }
        if (testAll && n.anzahl == 0)
        {
            if (sp == 0) { break; }
            sp -= 1;
            k = stK[sp]; o = stO[sp]; en = stE[sp];
            continue;
        }
        precise float vorn;
        precise float hinten;
        if (n.typ == 0) { vorn = o.x - n.dist; hinten = en.x - n.dist; }
        else if (n.typ == 1) { vorn = o.y - n.dist; hinten = en.y - n.dist; }
        else if (n.typ == 2) { vorn = o.z - n.dist; hinten = en.z - n.dist; }
        else
        {
            const float3 nn = float3(n.nx, n.ny, n.nz);
            precise float a = punkt3(o, nn);
            precise float b = punkt3(en, nn);
            vorn = a - n.dist;
            hinten = b - n.dist;
        }
        if (vorn >= -TRACE_ON && hinten >= -TRACE_ON) { k = n.k0; continue; }
        if (vorn < TRACE_ON && hinten < TRACE_ON) { k = n.k1; continue; }
        const int seite = vorn < 0.0f ? 1 : 0;
        precise float nenner = vorn - hinten;
        precise float frac = teilen(vorn, nenner);
        precise float3 mitte = o + (en - o) * frac;
        if (sp >= 64) { g_unsicher = true; return 3; }
        stK[sp] = seite == 1 ? n.k0 : n.k1; stO[sp] = mitte; stE[sp] = en; sp += 1;
        k = seite == 1 ? n.k1 : n.k0;
        en = mitte;
    }
    return 0;
}

// PointToPolygonFormFactor (light.cpp)
float formFaktor(float3 punkt, float3 normale, Licht L)
{
#if DOPPELT
    precise double total = 0.0;
#else
    precise float total = 0.0f;
#endif
    float3 erste = float3(windungen[L.windErste * 3 + 0], windungen[L.windErste * 3 + 1], windungen[L.windErste * 3 + 2]) - punkt;
    normalisieren(erste);
    float3 vorige = erste;
    [loop] for (int i = 0; i < L.windAnzahl; ++i)
    {
        float3 naechste;
        if (i + 1 < L.windAnzahl)
        {
            const int w = L.windErste + i + 1;
            precise float3 dd = float3(windungen[w * 3 + 0], windungen[w * 3 + 1], windungen[w * 3 + 2]) - punkt;
            naechste = dd;
            normalisieren(naechste);
        }
        else
        {
            naechste = erste;
        }
        float3 tri = kreuz(vorige, naechste);
        const float lt = normalisieren(tri);
        if (lt < 0.0001f) { vorige = naechste; continue; }
        const float dp = punkt3(vorige, naechste);
#if DOPPELT
        const double eins = dk(0u, 0x3ff00000u);
        precise double c = clamp((double)dp, -eins, eins);
        precise double winkel = dacos(c);
        precise double facing = (double)punkt3(normale, tri);
        total = total + facing * winkel;
        if (total > dk(0x33333333u, 0x40193333u) || total < -dk(0x33333333u, 0x40193333u)) { return 0.0f; }
#else
        precise float winkel = acos(clamp(dp, -1.0f, 1.0f));
        precise float facing = punkt3(normale, tri);
        total = total + facing * winkel;
        if (total > 6.3f || total < -6.3f) { return 0.0f; }
#endif
        vorige = naechste;
    }
#if DOPPELT
    precise double erg = total * dk(0x6dc9c883u, 0x3fc45f30u);
    return (float)erg;
#else
    return total * 0.15915494f;
#endif
}

// LightContributionToSample (light.cpp, ohne Deluxemap)
int beitrag(float3 o, float3 nrm, int luxelCluster, Lm lm, Licht L, out float3 farbe)
{
    farbe = float3(0.0f, 0.0f, 0.0f);
    const float3 lo = float3(L.ox, L.oy, L.oz);
    const float3 ln = float3(L.nx, L.ny, L.nz);
    const bool zweiseitig = lm.twoSided != 0;
    if ((L.flags & 32) == 0 || L.envelope <= 0.0f) { return 0; }
    if (L.typ != 3)
    {
        if (!zweiseitig)
        {
            if (punkt3(lo, nrm) < punkt3(o, nrm)) { return 0; }
        }
        if (!clusterSichtbar(luxelCluster, L.cluster)) { return 0; }
    }
    Strahl r = (Strahl)0;
    precise float add = 0.0f;
    precise float dist;
    precise float winkel;
    if (L.typ == 1)   // Area
    {
        precise float d = punkt3(o, ln) - L.dist;
        if (d < 3.0f)
        {
            if ((L.flags & 8) == 0 && d < -1.0f) { return 0; }
            if (d > -3.0f && punkt3(nrm, ln) > 0.9f) { return 0; }
        }
        precise float3 schub = o;
        if (d > -8.0f && d < 8.0f) { precise float f8 = 8.0f - d; schub = o + ln * f8; }
        dist = strahlSetzen(o, lo, r);
        if (dist >= L.envelope) { return 0; }
        if (faster != 0)
        {
            winkel = punkt3(nrm, r.d);
            if (zweiseitig && winkel < 0.0f) { winkel = -winkel; }
            winkel = winkel * -punkt3(ln, r.d);
            if (winkel == 0.0f) { return 0; }
            else if (winkel < 0.0f && (zweiseitig || (L.flags & 8) != 0)) { winkel = -winkel; }
            precise float q = dist * dist + L.extraDist * L.extraDist;
            dist = max(16.0f, fwurzel(q));
            add = teilen(L.photons, dist * dist) * winkel;
        }
        else
        {
            precise float faktor = formFaktor(schub, nrm, L);
            if (faktor == 0.0f) { return 0; }
            else if (faktor < 0.0f)
            {
                if (zweiseitig || (L.flags & 8) != 0)
                {
                    faktor = -faktor;
                    precise float3 ende2 = lo - ln * 2.0f;
                    dist = strahlSetzen(o, ende2, r);
                    if (dist >= L.envelope) { return 0; }
                }
                else { return 0; }
            }
            add = faktor * L.add;
        }
    }
    else if (L.typ == 0 || L.typ == 2)   // Point, Spot
    {
        dist = strahlSetzen(o, lo, r);
        if (dist >= L.envelope) { return 0; }
        precise float q = dist * dist + L.extraDist * L.extraDist;
        dist = max(16.0f, fwurzel(q));
        if ((L.flags & 2) != 0)
        {
            precise float dot = punkt3(nrm, r.d);
            if (zweiseitig && dot < 0.0f) { dot = -dot; }
            if (lightAngleHL != 0)
            {
                if (dot > 0.001f) { dot = min(dot, 1.0f); dot = (dot * 0.5f) + 0.5f; dot = dot * dot; }
                else { dot = 0.0f; }
            }
            winkel = dot;
        }
        else { winkel = 1.0f; }
        if (L.angleScale != 0.0f) { winkel = teilen(winkel, L.angleScale); winkel = min(winkel, 1.0f); }
        if ((L.flags & 1) != 0)
        {
            precise float a1 = winkel * L.photons;
            precise float a2 = a1 * linearScale;
            precise float a3 = dist * L.fade;
            add = max(0.0f, a2 - a3);
        }
        else
        {
            precise float pq = teilen(L.photons, dist * dist);
            add = max(0.0f, pq * winkel);
        }
        if (L.typ == 2)
        {
            precise float distByNormal = -punkt3(r.disp, ln);
            if (distByNormal < 0.0f) { return 0; }
            precise float3 pointAtDist = lo + ln * distByNormal;
            precise float radiusAtDist = L.radiusByDist * distByNormal;
            precise float3 distToSample = o - pointAtDist;
            precise float sampleRadius = fwurzel(punkt3(distToSample, distToSample));
            if (sampleRadius >= radiusAtDist) { return 0; }
            if (sampleRadius > (radiusAtDist - 32.0f))
            {
                precise float t = teilen(radiusAtDist - sampleRadius, 32.0f);
                add = add * t;
                add = max(add, 0.0f);
            }
        }
    }
    else   // Sun
    {
        precise float3 ende = o + lo;
        dist = strahlSetzen(o, ende, r);
        if ((L.flags & 2) != 0)
        {
            precise float dot = punkt3(nrm, r.d);
            if (zweiseitig && dot < 0.0f) { dot = -dot; }
            if (lightAngleHL != 0)
            {
                if (dot > 0.001f) { dot = min(dot, 1.0f); dot = (dot * 0.5f) + 0.5f; dot = dot * dot; }
                else { dot = 0.0f; }
            }
            winkel = dot;
        }
        else { winkel = 1.0f; }
        add = L.photons * winkel;
        if (add <= 0.0f) { return 0; }
        farbe = float3(L.cr, L.cg, L.cb) * add;
        if (testOcclusion != 0)
        {
            bool himmel; bool skyOk;
            const int t = strahlTesten(r, lm, true, L.skyIndex, himmel, skyOk);
            if (t == 3) { return 9; }
            if (!himmel || t == 1 || (oneSky == 0 && L.skyIndex != -1 && !skyOk))
            {
                farbe = float3(0.0f, 0.0f, 0.0f);
#if DIAG
                farbe.x = !himmel ? 1.0f : (t == 1 ? 2.0f : 3.0f);
                farbe.y = (float)L.skyIndex;
                farbe.z = (float)oneSky;
#endif
                return -1;
            }
        }
        return 1;
    }
    if (add <= 0.0f || (add <= L.falloffTol && (L.flags & (128 | 256)) != 0)) { return 0; }
    farbe = float3(L.cr, L.cg, L.cb) * add;
    bool himmel2; bool skyOk2;
    const int t2 = strahlTesten(r, lm, false, L.skyIndex, himmel2, skyOk2);
    if (t2 == 3) { return 9; }
    if (t2 == 1 || t2 == 2)
    {
        farbe = float3(0.0f, 0.0f, 0.0f);
        return -1;
    }
    return 1;
}

// ---------------------------------------------------------------------------------------
// Stufe 8g Punkt 2: die ganze Lichtschleife von IlluminateRawLightmap auf der GPU.
// Die Paare (Licht j der Lightmap, abgebildeter Luxel k) eines Blocks liegen in P/F/S an
// paarBase + j * m + k; nichts davon geht zurueck an die CPU, nur die fertigen Luxel (R).
// ---------------------------------------------------------------------------------------

static const uint F_FORCE = 1u;    // FLAG_FORCE_SUBSAMPLING (auf der GPU nie: Alpha-Schatten rechnet die CPU)
static const uint F_SCHON = 2u;    // FLAG_ALREADY_SUBSAMPLED
static const uint F_KAND = 4u;     // vorab nachabgetastet (Vorhersage mit den Werten der Erstabtastung)

#define MAXTIEFE 6                  // -samples bis 7 auf der GPU (Stapel der Unterabtastung)

int lmZuPaar(uint p)
{
    int lo = 0;
    int hi = nLms - 1;
    [loop] while (lo < hi) { const int mid = (lo + hi + 1) >> 1; if ((uint)lms[mid].paarBase <= p) { lo = mid; } else { hi = mid - 1; } }
    return lo;
}
int lmZuZeile(uint z)
{
    int lo = 0;
    int hi = nLms - 1;
    [loop] while (lo < hi) { const int mid = (lo + hi + 1) >> 1; if ((uint)lms[mid].zeilenBase <= z) { lo = mid; } else { hi = mid - 1; } }
    return lo;
}
int lmZuAktiv(uint a)
{
    int lo = 0;
    int hi = nLms - 1;
    [loop] while (lo < hi) { const int mid = (lo + hi + 1) >> 1; if ((uint)lms[mid].aktivBase <= a) { lo = mid; } else { hi = mid - 1; } }
    return lo;
}

int idxAt(Lm lm, int x, int y) { return idx[lm.idxBase + y * lm.sw + x]; }
float3 luxelUrsprung(Lm lm, int k) { const Luxel l = luxel[lm.luxelBase + k]; return float3(l.ox, l.oy, l.oz); }

// plane3_distance_to_point: (double)(float-dot) - (double)dist
#if DOPPELT
double abstand(Ebene e, float3 p) { precise double d = (double)punkt3(p, float3(e.nx, e.ny, e.nz)) - (double)e.dist; return d; }
#else
float abstand(Ebene e, float3 p) { precise float d = punkt3(p, float3(e.nx, e.ny, e.nz)) - e.dist; return d; }
#endif

// PointInLeafNum_r (light_ydnar.cpp): innerhalb 0,1 einer Ebene erst vorn, und wenn das Blatt
// dort undurchsichtig ist, hinten weiter - mit einem Stapel statt Rekursion
int punktBlatt(float3 p)
{
    int stapel[32];
    int sp = 0;
    int n = 0;
#if DOPPELT
    const double grenze = dk(0x9999999au, 0x3fb99999u);   // 0.1 in double
#else
    const float grenze = 0.1f;
#endif
    [loop] while (true)
    {
        [loop] while (n >= 0)
        {
            const BspKnoten k = bspKnoten[n];
#if DOPPELT
            const double d = abstand(ebenen[k.ebene], p);
#else
            const float d = abstand(ebenen[k.ebene], p);
#endif
            if (d > grenze) { n = k.k0; }
            else if (d < -grenze) { n = k.k1; }
            else
            {
                if (sp >= 32) { g_unsicher = true; return 0; }
                stapel[sp] = k.k1;
                sp += 1;
                n = k.k0;
            }
        }
        const int blatt = -n - 1;
        if (blaetter[blatt].cluster != -1 || sp == 0) { return blatt; }
        sp -= 1;
        n = stapel[sp];
    }
    return 0;
}

// ClusterForPointExt
int clusterExt(float3 p, float eps)
{
    const int blatt = punktBlatt(p);
    const Blatt bl = blaetter[blatt];
    if (bl.cluster <= -1) { return -1; }
    [loop] for (int i = 0; i < bl.anzahlBrushes; ++i)
    {
        const int b = blattBrushes[bl.ersterBrush + i];
        if (b > maxOpaqueBrush) { continue; }
        const Brush br = brushes[b];
        if (br.opak == 0) { continue; }
        bool innen = true;
        [loop] for (int s = 0; s < br.anzahlSeiten; ++s)
        {
#if DOPPELT
            if (abstand(ebenen[seiten[br.ersteSeite + s]], p) > (double)eps) { innen = false; break; }
#else
            if (abstand(ebenen[seiten[br.ersteSeite + s]], p) > eps) { innen = false; break; }
#endif
        }
        if (innen) { return -1 - b; }
    }
    return bl.cluster;
}

// ClusterForPointExtFilter
int clusterFilter(float3 p, float eps, Lm lm)
{
    const int c = clusterExt(p, eps);
    if (c <= -1 || lm.nClusters <= 0) { return c; }
    [loop] for (int i = 0; i < lm.nClusters; ++i)
    {
        const int cc = lmCluster[lm.clusterBase + i];
        if (c == cc || clusterSichtbar(c, cc)) { return c; }
    }
    return -1;
}

// SubmapRawLuxel: o ist der Ursprung der Abtastung und wird verschoben; Rueckgabe der Cluster
// (<= -1: keiner). Wie q3map2 prueft auch der T-Vektor bx (nicht by).
int submap(Lm lm, int x, int y, float bx, float by, inout float3 o)
{
    const int k0 = idxAt(lm, x, y);
    const float3 oxy = luxelUrsprung(lm, k0);
    float3 a;
    float3 b;
    if ((x < lm.sw - 1 && bx >= 0.0f) || (x == 0 && bx <= 0.0f))
    {
        a = oxy;
        const int k = idxAt(lm, x + 1, y);
        b = k < 0 ? oxy : luxelUrsprung(lm, k);
    }
    else
    {
        const int k = idxAt(lm, x - 1, y);
        a = k < 0 ? oxy : luxelUrsprung(lm, k);
        b = oxy;
    }
    precise float3 v0 = b - a;
    if ((y < lm.sh - 1 && bx >= 0.0f) || (y == 0 && bx <= 0.0f))
    {
        a = oxy;
        const int k = idxAt(lm, x, y + 1);
        b = k < 0 ? oxy : luxelUrsprung(lm, k);
    }
    else
    {
        const int k = idxAt(lm, x, y - 1);
        a = k < 0 ? oxy : luxelUrsprung(lm, k);
        b = oxy;
    }
    precise float3 v1 = b - a;
    precise float3 s0 = v0 * bx;
    precise float3 s1 = v1 * by;
    precise float3 s = s0 + s1;
    precise float3 neu = o + s;
    o = neu;
    return clusterFilter(neu, 0.25f, lm);   // LUXEL_EPSILON * 2
}

// SubsampleRawLuxel_r fuer den Luxel (x, y) mit dem Wert v0 der Erstabtastung (count 1), ohne
// Rekursion: Stapel von hoechstens MAXTIEFE Ebenen, je Ebene die vier Unterabtastungen.
float3 unterabtasten(Lm lm, int li, int x, int y, float3 o0, float3 nrm, float3 v0)
{
    if (lightSamples <= 1) { return v0; }
    if (lightSamples > MAXTIEFE + 1) { g_unsicher = true; return v0; }
    const Licht L = lichter[li];
    float3 fo[MAXTIEFE * 4];
    float3 fv[MAXTIEFE * 4];
    int fc[MAXTIEFE * 4];
    float3 selbst[MAXTIEFE];
    float3 urs[MAXTIEFE];
    float fb[MAXTIEFE];
    int fz[MAXTIEFE];
    int fn[MAXTIEFE];
    bool fr[MAXTIEFE];
    int d = 0;
    selbst[0] = v0;
    urs[0] = o0;
    fb[0] = suchBox;
    fz[0] = 1;
    bool neu = true;
    [loop] while (true)
    {
        if (neu)
        {
            neu = false;
            int mapped = 0;
            int lighted = 0;
            precise float3 total = float3(0.0f, 0.0f, 0.0f);
            int c0 = -1;
            [loop] for (int b = 0; b < 4; ++b)
            {
                const float dx = (b & 1) != 0 ? 1.0f : -1.0f;
                const float dy = (b & 2) != 0 ? 1.0f : -1.0f;
                precise float bx = fb[d] * dx;
                precise float by = fb[d] * dy;
                float3 o = urs[d];
                const int c = submap(lm, x, y, bx, by, o);
                if (c <= -1)
                {
                    fc[d * 4 + b] = -1;
                    if (b == 0) { c0 = -1; }
                    continue;
                }
                if (b == 0) { c0 = c; }
                fc[d * 4 + b] = c;
                fo[d * 4 + b] = o;
                mapped += 1;
                float3 farbe;
                const int code = beitrag(o, nrm, c0, lm, L, farbe);   // q3map2: trace->cluster = *cluster (cluster[0])
                if (code == 9) { g_unsicher = true; }
                fv[d * 4 + b] = farbe;
                total = total + farbe;
                precise float summe = farbe.x + farbe.y + farbe.z;
                if (summe > 0.0f) { lighted += 1; }
            }
            fr[d] = (fz[d] + 1) < lightSamples && (total.x > 4.0f || total.y > 4.0f || total.z > 4.0f) && lighted != 0 && lighted != mapped;
            fn[d] = 0;
        }
        if (fr[d] && fn[d] < 4)
        {
            const int b = fn[d];
            fn[d] = b + 1;
            if (fc[d * 4 + b] < 0) { continue; }
            selbst[d + 1] = fv[d * 4 + b];
            urs[d + 1] = fo[d * 4 + b];
            precise float halb = fb[d] * 0.5f;
            fb[d + 1] = halb;
            fz[d + 1] = fz[d] + 1;
            d += 1;
            neu = true;
            continue;
        }
        precise float3 farbe = selbst[d];
        int samples = 1;
        [unroll] for (int b2 = 0; b2 < 4; ++b2)
        {
            if (fc[d * 4 + b2] >= 0) { farbe = farbe + fv[d * 4 + b2]; samples += 1; }
        }
        const float fs = (float)samples;
        const float3 erg = float3(teilen(farbe.x, fs), teilen(farbe.y, fs), teilen(farbe.z, fs));
        if (d == 0) { return erg; }
        d -= 1;
        fv[d * 4 + fn[d] - 1] = erg;
    }
    return v0;
}

// der 2x2-Stempeltest des zweiten Durchgangs (pb: erstes Paar dieses Lichts)
bool stempel(Lm lm, uint pb, int x, int y)
{
    int mapped = 0;
    int lighted = 0;
    precise float3 total = float3(0.0f, 0.0f, 0.0f);
    [unroll] for (int t = 0; t < 4; ++t)
    {
        const int k = idxAt(lm, x + (t & 1), y + (t >> 1));
        if (k < 0) { continue; }
        mapped += 1;
        if ((F[pb + (uint)k] & F_FORCE) != 0u) { lighted += 1; mapped += 2; }
        const float3 v = P[pb + (uint)k].xyz;
        total = total + v;
        precise float summe = v.x + v.y + v.z;
        if (summe > 0.0f) { lighted += 1; }
    }
    return !(total.x <= 4.0f && total.y <= 4.0f && total.z <= 4.0f) && lighted != 0 && lighted != mapped;
}

// 1. Erstabtastung: ein Faden je Paar
[numthreads(64, 1, 1)]
void erst(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const uint p = id.x + basis;
    g_unsicher = false;
    const int li = lmZuPaar(p);
    const Lm lm = lms[li];
    const uint rel = p - (uint)lm.paarBase;
    const uint j = rel / (uint)lm.m;
    const uint k = rel - j * (uint)lm.m;
    const Luxel lx = luxel[lm.luxelBase + (int)k];
    float3 farbe;
    const int code = beitrag(float3(lx.ox, lx.oy, lx.oz), float3(lx.nx, lx.ny, lx.nz), lx.cluster, lm, lichter[lichtListe[lm.lichtBase + (int)j]], farbe);
    if (g_unsicher || code == 9)
    {
        InterlockedOr(lmFlag[li], 1u);
        farbe = float3(0.0f, 0.0f, 0.0f);
    }
    P[p] = float4(farbe, 0.0f);
    F[p] = 0u;
    if (farbe.x != 0.0f || farbe.y != 0.0f || farbe.z != 0.0f) { InterlockedAdd(zaehler[lm.lichtBase + (int)j], 1u); }
}

// 2. Vorhersage der Unterabtastung: ein Faden je (aktives Licht, Stempelzeile); markiert die
//    Luxel der Stempel, die mit den Werten der Erstabtastung nachabgetastet wuerden
[numthreads(64, 1, 1)]
void kandidaten(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const uint z = id.x + basis;
    const int li = lmZuZeile(z);
    const Lm lm = lms[li];
    const uint rel = z - (uint)lm.zeilenBase;
    const uint a = rel / (uint)(lm.sh - 1);
    const int y = (int)(rel - a * (uint)(lm.sh - 1));
    const Aktiv A = aktiv[lm.aktivBase + (int)a];
    const uint pb = (uint)lm.paarBase + (uint)A.j * (uint)lm.m;
    bool erster = true;
    [loop] for (int x = 0; x < lm.sw - 1; ++x)
    {
        if (!stempel(lm, pb, x, y)) { continue; }
        if (erster)
        {
            erster = false;
            InterlockedMin(hilfe[1u + (uint)lm.aktivBase + a], (uint)(y * (lm.sw - 1) + x));
        }
        [unroll] for (int t = 0; t < 4; ++t)
        {
            const int k = idxAt(lm, x + (t & 1), y + (t >> 1));
            if (k >= 0)
            {
                uint alt;
                InterlockedOr(F[pb + (uint)k], F_KAND, alt);
                if ((alt & F_KAND) == 0u)
                {
                    uint platz;
                    InterlockedAdd(hilfe[0], 1u, platz);
                    kandListe[platz + 1u] = pb + (uint)k;   // hilfe[0] beginnt bei 0xffffffff
                }
            }
        }
    }
}

// 3. Unterabtastung der vorhergesagten Luxel: ein Faden je Kandidat (kandListe)
[numthreads(64, 1, 1)]
void unter(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const uint p = kandListe[id.x + basis];
    const int li = lmZuPaar(p);
    const Lm lm = lms[li];
    const uint rel = p - (uint)lm.paarBase;
    const uint j = rel / (uint)lm.m;
    const uint k = rel - j * (uint)lm.m;
    g_unsicher = false;
    const Luxel lx = luxel[lm.luxelBase + (int)k];
    const float3 v = unterabtasten(lm, lichtListe[lm.lichtBase + (int)j], lx.x, lx.y, float3(lx.ox, lx.oy, lx.oz), float3(lx.nx, lx.ny, lx.nz), P[p].xyz);
    if (g_unsicher) { InterlockedOr(lmFlag[li], 1u); }
    S[p] = float4(v, 0.0f);
}

// 4. Der genaue Stempellauf in Zeilenfolge: ein Faden je aktives Licht. Vorhergesagte Luxel
//    nehmen das Ergebnis aus S, andere werden hier nachabgetastet (wie Stufe 7).
[numthreads(64, 1, 1)]
void stempellauf(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const uint g = id.x + basis;
    g_unsicher = false;
    const int li = lmZuAktiv(g);
    const Lm lm = lms[li];
    const Aktiv A = aktiv[g];
    const uint pb = (uint)lm.paarBase + (uint)A.j * (uint)lm.m;
    const int licht = lichtListe[lm.lichtBase + A.j];
    // Vor dem ersten vorhergesagten Stempel sieht der Lauf nur Werte der Erstabtastung (nachabgetastet wird
    // erst dort) - wie die Vorhersage: kein Stempel davor schlaegt an. Ohne Vorhersage gar keiner.
    const uint start = hilfe[1u + g];
    if (start == 0xffffffffu) { return; }
    const int y0 = (int)(start / (uint)(lm.sw - 1));
    const int x0 = (int)(start - (uint)y0 * (uint)(lm.sw - 1));
    [loop] for (int y = y0; y < lm.sh - 1; ++y)
    {
        [loop] for (int x = (y == y0 ? x0 : 0); x < lm.sw - 1; ++x)
        {
            if (!stempel(lm, pb, x, y)) { continue; }
            [loop] for (int t = 0; t < 4; ++t)
            {
                const int sx = x + (t & 1);
                const int sy = y + (t >> 1);
                const int k = idxAt(lm, sx, sy);
                if (k < 0) { continue; }
                const uint q = pb + (uint)k;
                const uint f = F[q];
                if ((f & F_SCHON) != 0u) { continue; }
                if ((f & F_KAND) != 0u) { P[q] = S[q]; }
                else
                {
                    const Luxel lx = luxel[lm.luxelBase + k];
                    P[q] = float4(unterabtasten(lm, licht, sx, sy, float3(lx.ox, lx.oy, lx.oz), float3(lx.nx, lx.ny, lx.nz), P[q].xyz), 0.0f);
                }
                F[q] = f | F_SCHON;
            }
        }
    }
    if (g_unsicher) { InterlockedOr(lmFlag[li], 1u); }
}

// 5. Filtern und Aufsummieren je Luxel, die Lichter in ihrer Reihenfolge (dritter Durchgang):
//    ein Faden je abgebildeter Luxel; Stil 0 beginnt mit dem Umgebungslicht, die anderen mit 0
[numthreads(64, 1, 1)]
void filtern(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const int g = (int)(id.x + basis);
    g_unsicher = false;
    const Luxel lx = luxel[g];
    const int li = lx.lm;
    const Lm lm = lms[li];
    const int k = g - lm.luxelBase;
    const int x = lx.x;
    const int y = lx.y;
    float3 wert[4];
    wert[0] = float3(lm.ar, lm.ag, lm.ab);
    wert[1] = float3(0.0f, 0.0f, 0.0f);
    wert[2] = float3(0.0f, 0.0f, 0.0f);
    wert[3] = float3(0.0f, 0.0f, 0.0f);
    if (lm.start != 0)
    {
        if (lm.aus0 >= 0) { wert[0] = startWert[lm.aus0 + k].xyz; }
        if (lm.aus1 >= 0) { wert[1] = startWert[lm.aus1 + k].xyz; }
        if (lm.aus2 >= 0) { wert[2] = startWert[lm.aus2 + k].xyz; }
        if (lm.aus3 >= 0) { wert[3] = startWert[lm.aus3 + k].xyz; }
    }
    [loop] for (int a = 0; a < lm.nAktiv; ++a)
    {
        const Aktiv A = aktiv[lm.aktivBase + a];
        const uint pb = (uint)lm.paarBase + (uint)A.j * (uint)lm.m;
        const int r = A.radius;
        float3 b;
        if (r != 0)
        {
            precise float3 summe = float3(0.0f, 0.0f, 0.0f);
            precise float samples = 0.0f;
            [loop] for (int sy = y - r; sy <= y + r; ++sy)
            {
                if (sy < 0 || sy >= lm.sh) { continue; }
                [loop] for (int sx = x - r; sx <= x + r; ++sx)
                {
                    if (sx < 0 || sx >= lm.sw) { continue; }
                    const int kk = idxAt(lm, sx, sy);
                    if (kk < 0) { continue; }
                    precise float w = abs(sx - x) == r ? 0.5f : 1.0f;
                    w = w * (abs(sy - y) == r ? 0.5f : 1.0f);
                    precise float3 v = P[pb + (uint)kk].xyz * w;
                    summe = summe + v;
                    samples = samples + w;
                }
            }
            if (samples <= 0.0f) { continue; }
            b = float3(teilen(summe.x, samples), teilen(summe.y, samples), teilen(summe.z, samples));
        }
        else
        {
            b = P[pb + (uint)k].xyz;
        }
        precise float3 neu;
        if (A.negativ != 0) { neu = wert[A.slot] - b; }
        else { neu = wert[A.slot] + b; }
        wert[A.slot] = neu;
    }
    if (g_unsicher) { InterlockedOr(lmFlag[li], 1u); }
    if (lm.aus0 >= 0) { R[lm.aus0 + k] = float4(wert[0], 1.0f); }
    if (lm.aus1 >= 0) { R[lm.aus1 + k] = float4(wert[1], 1.0f); }
    if (lm.aus2 >= 0) { R[lm.aus2 + k] = float4(wert[2], 1.0f); }
    if (lm.aus3 >= 0) { R[lm.aus3 + k] = float4(wert[3], 1.0f); }
}

// ---------------------------------------------------------------------------------------
// Stufe 8g: das Lichtgitter (TraceGrid, light.cpp) - ein Faden je Gitterpunkt, alle Lichter
// in ihrer Reihenfolge. Erster Durchlauf: LightContributionToPoint, Richtung (gp.dir) und
// Zahl der Beitraege; zweiter Durchlauf rechnet dieselben Beitraege noch einmal und teilt sie
// wie q3map2 in gerichtetes und Umgebungslicht je Stil.
// ---------------------------------------------------------------------------------------

// LightContributionToPoint: 0 kein Beitrag, 1 Beitrag (farbe, richtung), 9 die CPU
int punktBeitrag(float3 o, int cluster, Licht L, out float3 farbe, out float3 richtung)
{
    farbe = float3(0.0f, 0.0f, 0.0f);
    richtung = float3(0.0f, 0.0f, 0.0f);
    if ((L.flags & 16) == 0 || L.envelope <= 0.0f) { return 0; }
    if (L.typ != 3)
    {
        if (!clusterSichtbar(cluster, L.cluster)) { return 0; }
    }
    if (!(o.x >= L.minx && o.y >= L.miny && o.z >= L.minz && o.x <= L.maxx && o.y <= L.maxy && o.z <= L.maxz)) { return 0; }
    const float3 lo = float3(L.ox, L.oy, L.oz);
    const float3 ln = float3(L.nx, L.ny, L.nz);
    precise float3 ende = lo;
    if (L.typ == 3) { ende = o + lo; }
    Strahl r = (Strahl)0;
    precise float dist = strahlSetzen(o, ende, r);
    richtung = r.d;
    if (dist > L.envelope) { return 0; }
    precise float add = 0.0f;
    if (L.typ == 1 && faster != 0)
    {
        precise float q = dist * dist + L.extraDist * L.extraDist;
        dist = max(16.0f, fwurzel(q));
        add = teilen(L.photons, dist * dist);
    }
    else if (L.typ == 1)
    {
        precise float d = punkt3(o, ln) - L.dist;
        if ((L.flags & 8) == 0 && d < -1.0f) { return 0; }
        precise float3 schub = o;
        if (d > -8.0f && d < 8.0f) { precise float f8 = 8.0f - d; schub = o + ln * f8; }
        precise float faktor = formFaktor(schub, r.d, L);
        if (faktor == 0.0f) { return 0; }
        else if (faktor < 0.0f)
        {
            if ((L.flags & 8) != 0) { faktor = -faktor; }
            else { return 0; }
        }
        add = faktor * L.add;
    }
    else if (L.typ == 0 || L.typ == 2)
    {
        precise float q = dist * dist + L.extraDist * L.extraDist;
        dist = max(16.0f, fwurzel(q));
        if ((L.flags & 1) != 0)
        {
            precise float a1 = L.photons * linearScale;
            precise float a2 = dist * L.fade;
            add = max(0.0f, a1 - a2);
        }
        else
        {
            add = teilen(L.photons, dist * dist);
        }
        if (L.typ == 2)
        {
            precise float distByNormal = -punkt3(r.disp, ln);
            if (distByNormal < 0.0f) { return 0; }
            precise float3 pointAtDist = lo + ln * distByNormal;
            precise float radiusAtDist = L.radiusByDist * distByNormal;
            precise float3 distToSample = o - pointAtDist;
            precise float sampleRadius = fwurzel(punkt3(distToSample, distToSample));
            if (sampleRadius >= radiusAtDist) { return 0; }
            if (sampleRadius > (radiusAtDist - 32.0f))
            {
                precise float t = teilen(radiusAtDist - sampleRadius, 32.0f);
                add = add * t;
            }
        }
    }
    else if (L.typ == 3)
    {
        add = L.photons;
        if (add <= 0.0f) { return 0; }
        farbe = float3(L.cr, L.cg, L.cb) * add;
        if (testOcclusion != 0)
        {
            Lm lm = (Lm)0;
            lm.recv = 1;
            bool himmel; bool skyOk;
            const int t = strahlTesten(r, lm, true, L.skyIndex, himmel, skyOk);
            if (t == 3) { return 9; }
            if (!himmel || t == 1 || (oneSky == 0 && L.skyIndex != -1 && !skyOk))
            {
                farbe = float3(0.0f, 0.0f, 0.0f);
                return 0;
            }
        }
        return 1;
    }
    else { return 0; }
    if (add <= 0.0f || (add <= L.falloffTol && (L.flags & (128 | 256)) != 0)) { return 0; }
    farbe = float3(L.cr, L.cg, L.cb) * add;
    Lm lm2 = (Lm)0;
    lm2.recv = 1;
    bool himmel2; bool skyOk2;
    const int t2 = strahlTesten(r, lm2, false, L.skyIndex, himmel2, skyOk2);
    if (t2 == 3) { return 9; }
    if (t2 == 2) { farbe = float3(0.0f, 0.0f, 0.0f); return 0; }   // passSolid: kein Beitrag
    if (t2 == 1) { farbe = float3(0.0f, 0.0f, 0.0f); }             // Dreieck: TraceTriangle setzt die Farbe 0, der Beitrag zaehlt
    return 1;
}

// der Beitrag des Lichts li mit Vorzeichen (negative Lichter); false: keiner
bool gitterBeitrag(GitterPunkt gp, int li, out float3 farbe, out float3 richtung, out int code)
{
    const Licht L = lichter[li];
    code = punktBeitrag(float3(gp.ox, gp.oy, gp.oz), gp.cluster, L, farbe, richtung);
    if (code != 1) { return false; }
    if ((L.flags & 512) != 0) { farbe = -farbe; }
    return true;
}

// Ein Faden je Gitterpunkt, die Lichter lichtVon..lichtBis (eine Runde vieler Dispatches, damit
// jeder kurz bleibt und alle Punkte gleichzeitig laufen); der Stand je Punkt liegt in zustand.
// Phase 0: Richtung und Zahl der Beitraege; Phase 1: dieselben Beitraege nach Stilen aufteilen.
[numthreads(64, 1, 1)]
void gitterKern(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const uint nr = id.x + basis;
    g_unsicher = false;
    g_inhibit = inhibitGitter;
    g_imGitter = true;
    GitterPunkt gp = gitter[nr];
    if (gp.flag != 0) { return; }
    GitterZustand z = zustand[nr];
    if (gitterPhase == 0)
    {
        if (lichtVon == 0) { z = (GitterZustand)0; }
        if (z.abbruch != 0) { return; }
        precise float3 dir = float3(gp.dx, gp.dy, gp.dz);
        precise float3 billig = float3(z.bx, z.by, z.bz);
        [loop] for (int li = lichtVon; li < lichtBis; ++li)
        {
            float3 farbe; float3 richtung; int code;
            if (!gitterBeitrag(gp, li, farbe, richtung, code))
            {
                if (code == 9 || g_unsicher) { gp.flag = 1; gitter[nr] = gp; return; }
                continue;
            }
            z.numCon += 1;
            const float addSize = fwurzel(punkt3(farbe, farbe));
            precise float3 schub = richtung * addSize;
            dir = dir + schub;
            if (z.numCon >= 32767) { z.abbruch = 1; break; }   // MAX_CONTRIBUTIONS - 1
            billig = billig + farbe;
            if (cheapgrid != 0 && billig.x >= 255.0f && billig.y >= 255.0f && billig.z >= 255.0f) { z.abbruch = 1; break; }
        }
        if (g_unsicher) { gp.flag = 1; gitter[nr] = gp; return; }
        gp.dx = dir.x; gp.dy = dir.y; gp.dz = dir.z;
        z.bx = billig.x; z.by = billig.y; z.bz = billig.z;
        gitter[nr] = gp;
        zustand[nr] = z;
        return;
    }
    if (lichtVon == 0)
    {
        float3 t = float3(gp.dx, gp.dy, gp.dz);
        normalisieren(t);
        if (g_unsicher) { gp.flag = 1; gitter[nr] = gp; return; }
        z.tx = t.x; z.ty = t.y; z.tz = t.z;
        z.gezaehlt = 0;
        z.numStyles = 1;
    }
    if (z.gezaehlt >= z.numCon) { zustand[nr] = z; return; }
    const float3 thisdir = float3(z.tx, z.ty, z.tz);
    float3 amb[4];
    float3 gerichtet[4];
    amb[0] = float3(gp.a0, gp.a1, gp.a2);
    amb[1] = float3(gp.a3, gp.a4, gp.a5);
    amb[2] = float3(gp.a6, gp.a7, gp.a8);
    amb[3] = float3(gp.a9, gp.a10, gp.a11);
    gerichtet[0] = float3(gp.g0, gp.g1, gp.g2);
    gerichtet[1] = float3(gp.g3, gp.g4, gp.g5);
    gerichtet[2] = float3(gp.g6, gp.g7, gp.g8);
    gerichtet[3] = float3(gp.g9, gp.g10, gp.g11);
    uint stile = gp.stile;
    uint neu = gp.neu;
    int numStyles = z.numStyles;
    int gezaehlt = z.gezaehlt;
    [loop] for (int li2 = lichtVon; li2 < lichtBis && gezaehlt < z.numCon; ++li2)
    {
        float3 farbe; float3 richtung; int code;
        if (!gitterBeitrag(gp, li2, farbe, richtung, code)) { continue; }
        gezaehlt += 1;
        const int stil = lichter[li2].style;
        precise float d = punkt3(richtung, thisdir);
        precise float spanne = gridDirectionality - gridAmbientDirectionality;
        precise float dd = d * spanne;
        precise float dv = gridAmbientDirectionality + dd;
        d = max(0.0f, dv);
        int j;
        for (j = 0; j < numStyles; ++j)
        {
            if ((int)((stile >> (8u * (uint)j)) & 0xffu) == stil) { break; }
        }
        if (j >= numStyles)
        {
            if (numStyles < 4)
            {
                const uint sh = 8u * (uint)numStyles;
                stile = (stile & ~(0xffu << sh)) | (((uint)stil & 0xffu) << sh);
                neu |= 1u << (uint)numStyles;
                numStyles += 1;
            }
            else { j = 0; }
        }
        precise float3 gd = farbe * d;
        precise float3 gneu = gerichtet[j] + gd;
        gerichtet[j] = gneu;
        precise float d2 = 0.25f * (1.0f - d);
        precise float3 ad = farbe * d2;
        precise float3 aneu = amb[j] + ad;
        // + contributions[ i ].ambient (0): aus -0 wird +0 wie auf der CPU (bitweise, das faltet fxc nicht weg)
        const uint3 bits = asuint(aneu);
        amb[j] = asfloat(uint3(bits.x == 0x80000000u ? 0u : bits.x, bits.y == 0x80000000u ? 0u : bits.y, bits.z == 0x80000000u ? 0u : bits.z));
    }
    if (g_unsicher) { gp.flag = 1; gitter[nr] = gp; return; }
    gp.a0 = amb[0].x; gp.a1 = amb[0].y; gp.a2 = amb[0].z;
    gp.a3 = amb[1].x; gp.a4 = amb[1].y; gp.a5 = amb[1].z;
    gp.a6 = amb[2].x; gp.a7 = amb[2].y; gp.a8 = amb[2].z;
    gp.a9 = amb[3].x; gp.a10 = amb[3].y; gp.a11 = amb[3].z;
    gp.g0 = gerichtet[0].x; gp.g1 = gerichtet[0].y; gp.g2 = gerichtet[0].z;
    gp.g3 = gerichtet[1].x; gp.g4 = gerichtet[1].y; gp.g5 = gerichtet[1].z;
    gp.g6 = gerichtet[2].x; gp.g7 = gerichtet[2].y; gp.g8 = gerichtet[2].z;
    gp.g9 = gerichtet[3].x; gp.g10 = gerichtet[3].y; gp.g11 = gerichtet[3].z;
    gp.stile = stile;
    gp.neu = neu;
    z.numStyles = numStyles;
    z.gezaehlt = gezaehlt;
    gitter[nr] = gp;
    zustand[nr] = z;
}
