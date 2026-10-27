// bhed_gpubacken.hlsl - BehavEd Stufe 8c/8g: die Compute-Shader des GPU-Backens
// (q3map2 -light -bhedgpubake, src/q3map2/bhed_gpubacken.cpp). Im Bau mit fxc aus dem
// Windows SDK vorab uebersetzt (src/q3map2/CMakeLists.txt, Bytecode als Header in der
// exe) - zur Laufzeit wird nichts uebersetzt, d3dcompiler_47.dll wird nicht gebraucht.
//
// LightContributionToSample, SetupTrace, TraceLine/TraceLine_r/TraceTriangle und
// PointToPolygonFormFactor aus q3map2 (light.cpp, light_trace.cpp), Zeile fuer Zeile
// (siehe bhed_gpubacken.cpp). Makros: DOPPELT (die GPU rechnet double), MESS
// (Messschalter BHED_GPUBAKE_MESS), DIAG (BHED_GPUBAKE_DIAG).
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
               int skyIndex; int windAnzahl; };
struct Luxel { float ox; float oy; float oz; float nx; float ny; float nz; int cluster; int lm; };
struct Lm { int recv; int sErste; int sAnzahl; int twoSided; };
struct Paar { int luxel; int licht; };
struct Ergebnis { float r; float g; float b; int code; };
cbuffer Kopf : register(b0)
{
    uint anzahl; uint basis; int kopfKnoten; int faster;
    int lightAngleHL; int bouncing; int testOcclusion; int oneSky;
    int noSurfaces; float linearScale; uint visBytes; int leafBytes;
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
StructuredBuffer<Paar> paare : register(t10);
RWStructuredBuffer<Ergebnis> aus : register(u0);

static bool g_unsicher;

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
// genau gerundete double-Wurzel, x > 0 normal
double dwurzel(double x)
{
    precise double s = (double)sqrt((float)x);
    s = dk(0u, 0x3fe00000u) * (s + x / s);
    s = dk(0u, 0x3fe00000u) * (s + x / s);
    [unroll] for (int k = 0; k < 3; ++k)
    {
        precise double sm = naechster(s, -1);
        precise double sp = naechster(s, 1);
        if (fma(s, sm, -x) >= 0.0) { s = sm; }
        else if (fma(s, sp, -x) < 0.0) { s = sp; }
    }
    return s;
}
float fwurzel(float x)
{
    if (x <= 0.0f) { return 0.0f; }
    return (float)dwurzel((double)x);
}
// VectorNormalize (common/qmath.h): in double; die Laenge als float
float normalisieren(inout float3 v)
{
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
float fwurzel(float x) { if (x <= 0.0f) { return 0.0f; } return sqrt(x); }
float normalisieren(inout float3 v)
{
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
        if (tiefe <= INHIBIT || tiefe >= r.dist) { continue; }
        if (tiefe <= SELBST)
        {
            bool eigen = false;
            [loop] for (int s = 0; s < lm.sAnzahl; ++s) { if (flaechen[lm.sErste + s] == f.flaeche) { eigen = true; break; } }
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
        if (k < 0) { return testAll ? 0 : 1; }
        const Knoten n = knoten[k];
        if (n.typ == -2) { return testAll ? 0 : 1; }
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
int beitrag(Luxel lx, Lm lm, Licht L, out float3 farbe)
{
    farbe = float3(0.0f, 0.0f, 0.0f);
    const float3 o = float3(lx.ox, lx.oy, lx.oz);
    const float3 nrm = float3(lx.nx, lx.ny, lx.nz);
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
        if (!clusterSichtbar(lx.cluster, L.cluster)) { return 0; }
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
    if (t2 == 1)
    {
        farbe = float3(0.0f, 0.0f, 0.0f);
        return -1;
    }
    return 1;
}

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint i = id.x + basis;
    if (id.x >= anzahl) { return; }
    g_unsicher = false;
    const Paar p = paare[i];
    const Luxel lx = luxel[p.luxel];
    const Lm lm = lms[lx.lm];
    const Licht L = lichter[p.licht];
    float3 farbe;
    int code = beitrag(lx, lm, L, farbe);
    if (g_unsicher) { code = 9; }
    Ergebnis e;
    e.r = farbe.x; e.g = farbe.y; e.b = farbe.z; e.code = code;
    aus[i] = e;
}
