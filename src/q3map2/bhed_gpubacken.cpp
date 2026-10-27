// bhed_gpubacken.cpp - BehavEd Stufe 8c Punkt 2: GPU-Backen fuer q3map2 -light
// (siehe bhed_gpubacken.h). Der Compute-Shader ist LightContributionToSample,
// SetupTrace, TraceLine/TraceLine_r/TraceTriangle und PointToPolygonFormFactor
// aus q3map2 (light.cpp, light_trace.cpp), Zeile fuer Zeile:
//
//   - float-Rechnungen "precise" in der Reihenfolge von MSVC /fp:precise (dot und
//     cross wie libs/math/vector.h von links nach rechts, keine mad-Zusammenfassung);
//   - float-Division genau gerundet mit Ganzzahlen (teilen, wie Stufe 8b);
//   - VectorNormalize von q3map2 rechnet in double: hier ebenso, die Wurzel genau
//     gerundet (Newton, dann der Test nach Tuckerman mit fma); float-Wurzeln als
//     genau gerundete double-Wurzel, auf float gerundet (53 >= 2*24+2: kein
//     schaedliches doppeltes Runden); acos in double nach fdlibm;
//   - GPUs ohne double (ExtendedDoublesShaderInstructions): float-Naeherungen,
//     der Bericht nennt es.
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
#include "q3map2.h"
#include "bhed_gpubacken.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// Hybrid-Notebooks: die schnelle GPU fuer diese exe (wie jkstudio.exe, gui/main_win32.cpp).
// Die Exporte muessen in der exe stehen, nicht in einer DLL.
extern "C" {
__declspec( dllexport ) DWORD NvOptimusEnablement = 1;
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{

const char *const g_shader = R"HLSL(
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
            return pi + two * pio2_lo;
        }
        g_unsicher = true;
        return 0.0;
    }
    precise double z; precise double p; precise double q; precise double r; precise double w; precise double s;
    if (ix < 0x3fe00000u)
    {
        if (ix <= 0x3c600000u) { return pio2_hi + pio2_lo; }
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
                if (noSurfaces == 0)
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
    Strahl r;
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
)HLSL";

struct Gpu
{
	HMODULE d3d = nullptr, compiler = nullptr, dxgi = nullptr;
	ID3D11Device *geraet = nullptr;
	ID3D11DeviceContext *ktx = nullptr;
	ID3D11ComputeShader *cs = nullptr;
	ID3D11Buffer *szene[ 6 ] = {};
	ID3D11ShaderResourceView *szeneSrv[ 6 ] = {};
	ID3D11Buffer *licht[ 2 ] = {};
	ID3D11ShaderResourceView *lichtSrv[ 2 ] = {};
	ID3D11Buffer *block[ 3 ] = {};
	ID3D11ShaderResourceView *blockSrv[ 3 ] = {};
	ID3D11Buffer *erg = nullptr, *lesen = nullptr, *kopf = nullptr;
	ID3D11UnorderedAccessView *ergUav = nullptr;
	UINT ergKapazitaet = 0;
	int kopfKnoten = 0;
	UINT visBytes = 0;
	int leafBytes = 0;
	BhedBakeSchalter schalter{};
	bool doppelt = false;
	bool bereit = false;
	std::string name;
	BhedBakeZahlen zahlen;
	long long verlustNach = -1;   // Test: nach so vielen Dispatches einen Geraeteverlust vortaeuschen
};
Gpu g;

template<typename T>
void frei( T *&p ){
	if ( p != nullptr ) {
		p->Release();
		p = nullptr;
	}
}

bool puffer( const void *daten, UINT stride, UINT anzahl, ID3D11Buffer *&buf, ID3D11ShaderResourceView *&srv, bool roh = false ){
	frei( srv );
	frei( buf );
	if ( anzahl == 0 ) {
		anzahl = 1;
	}
	std::vector<unsigned char> leer;
	if ( daten == nullptr ) {
		leer.assign( size_t( stride ) * anzahl, 0 );
		daten = leer.data();
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = stride * anzahl;
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = roh ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = roh ? 0 : stride;
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = daten;
	if ( FAILED( g.geraet->CreateBuffer( &bd, &sd, &buf ) ) ) {
		return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
	if ( roh ) {
		vd.Format = DXGI_FORMAT_R32_TYPELESS;
		vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		vd.BufferEx.NumElements = bd.ByteWidth / 4;
		vd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	}
	else {
		vd.Format = DXGI_FORMAT_UNKNOWN;
		vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		vd.Buffer.NumElements = anzahl;
	}
	return SUCCEEDED( g.geraet->CreateShaderResourceView( buf, &vd, &srv ) );
}

bool ergebnisPlatz( UINT n ){
	if ( n <= g.ergKapazitaet ) {
		return true;
	}
	frei( g.ergUav );
	frei( g.erg );
	frei( g.lesen );
	g.ergKapazitaet = 0;
	UINT k = 65536;
	while ( k < n ) {
		k *= 2;
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = UINT( sizeof( BhedBakeErg ) ) * k;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof( BhedBakeErg );
	if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &g.erg ) ) ) {
		return false;
	}
	D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
	ud.Format = DXGI_FORMAT_UNKNOWN;
	ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	ud.Buffer.NumElements = k;
	if ( FAILED( g.geraet->CreateUnorderedAccessView( g.erg, &ud, &g.ergUav ) ) ) {
		return false;
	}
	bd.Usage = D3D11_USAGE_STAGING;
	bd.BindFlags = 0;
	bd.MiscFlags = 0;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &g.lesen ) ) ) {
		return false;
	}
	g.ergKapazitaet = k;
	return true;
}

void allesFrei(){
	for ( int i = 0; i < 6; ++i ) {
		frei( g.szeneSrv[ i ] );
		frei( g.szene[ i ] );
	}
	for ( int i = 0; i < 2; ++i ) {
		frei( g.lichtSrv[ i ] );
		frei( g.licht[ i ] );
	}
	for ( int i = 0; i < 3; ++i ) {
		frei( g.blockSrv[ i ] );
		frei( g.block[ i ] );
	}
	frei( g.ergUav );
	frei( g.erg );
	frei( g.lesen );
	frei( g.kopf );
	frei( g.cs );
	frei( g.ktx );
	frei( g.geraet );
	g.ergKapazitaet = 0;
	g.bereit = false;
}

void ausfall( const char *grund ){
	if ( !g.zahlen.ausgefallen ) {
		g.zahlen.ausgefallen = true;
		g.zahlen.grund = grund;
		Sys_Printf( "BehavEd GPU bake: %s - the rest is computed on the CPU\n", grund );
	}
	allesFrei();
}

std::string utf8( const wchar_t *w ){
	char z[ 512 ] = {};
	WideCharToMultiByte( CP_UTF8, 0, w, -1, z, sizeof( z ) - 1, nullptr, nullptr );
	return z;
}

// Adapter: -bhedgpuadapter <n|Teil des Namens>, sonst die schnellste GPU
// (EnumAdapterByGpuPreference HIGH_PERFORMANCE), ohne IDXGIFactory6 der Adapter
// mit dem meisten eigenen Grafikspeicher. Software-Adapter nie.
IDXGIAdapter1 *adapterWaehlen( const std::string& wahl ){
	g.dxgi = LoadLibraryW( L"dxgi.dll" );
	if ( g.dxgi == nullptr ) {
		return nullptr;
	}
	using Fabrik = HRESULT( WINAPI * )( REFIID, void ** );
	const auto fabrik = reinterpret_cast<Fabrik>( reinterpret_cast<void *>( GetProcAddress( g.dxgi, "CreateDXGIFactory1" ) ) );
	IDXGIFactory1 *f = nullptr;
	if ( fabrik == nullptr || FAILED( fabrik( __uuidof( IDXGIFactory1 ), reinterpret_cast<void **>( &f ) ) ) ) {
		return nullptr;
	}
	std::vector<IDXGIAdapter1 *> liste;
	IDXGIFactory6 *f6 = nullptr;
	if ( SUCCEEDED( f->QueryInterface( __uuidof( IDXGIFactory6 ), reinterpret_cast<void **>( &f6 ) ) ) ) {
		IDXGIAdapter1 *a = nullptr;
		for ( UINT i = 0; SUCCEEDED( f6->EnumAdapterByGpuPreference( i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof( IDXGIAdapter1 ), reinterpret_cast<void **>( &a ) ) ); ++i ) {
			liste.push_back( a );
		}
		f6->Release();
	}
	else {
		IDXGIAdapter1 *a = nullptr;
		for ( UINT i = 0; f->EnumAdapters1( i, &a ) != DXGI_ERROR_NOT_FOUND; ++i ) {
			liste.push_back( a );
		}
		std::stable_sort( liste.begin(), liste.end(), []( IDXGIAdapter1 *x, IDXGIAdapter1 *y ){
			DXGI_ADAPTER_DESC1 a{}, b{};
			x->GetDesc1( &a );
			y->GetDesc1( &b );
			return a.DedicatedVideoMemory > b.DedicatedVideoMemory;
		} );
	}
	f->Release();
	std::vector<IDXGIAdapter1 *> echt;
	for ( IDXGIAdapter1 *a : liste ) {
		DXGI_ADAPTER_DESC1 d{};
		if ( SUCCEEDED( a->GetDesc1( &d ) ) && !( d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) ) {
			echt.push_back( a );
		}
		else {
			a->Release();
		}
	}
	IDXGIAdapter1 *gewaehlt = nullptr;
	if ( !wahl.empty() ) {
		char *rest = nullptr;
		const long n = std::strtol( wahl.c_str(), &rest, 10 );
		if ( rest != nullptr && *rest == 0 && n >= 0 && size_t( n ) < echt.size() ) {
			gewaehlt = echt[ size_t( n ) ];
		}
		else {
			std::string w = wahl;
			std::transform( w.begin(), w.end(), w.begin(), []( unsigned char c ){ return char( std::tolower( c ) ); } );
			for ( IDXGIAdapter1 *a : echt ) {
				DXGI_ADAPTER_DESC1 d{};
				a->GetDesc1( &d );
				std::string name = utf8( d.Description );
				std::transform( name.begin(), name.end(), name.begin(), []( unsigned char c ){ return char( std::tolower( c ) ); } );
				if ( name.find( w ) != std::string::npos ) {
					gewaehlt = a;
					break;
				}
			}
		}
		if ( gewaehlt == nullptr ) {
			Sys_Printf( "BehavEd GPU bake: no adapter \"%s\" - the fastest one\n", wahl.c_str() );
		}
	}
	if ( gewaehlt == nullptr && !echt.empty() ) {
		gewaehlt = echt.front();
	}
	for ( size_t i = 0; i < echt.size(); ++i ) {
		DXGI_ADAPTER_DESC1 d{};
		echt[ i ]->GetDesc1( &d );
		Sys_Printf( "BehavEd GPU bake: adapter %zu: %s (%llu MB)%s\n", i, utf8( d.Description ).c_str(),
		             static_cast<unsigned long long>( d.DedicatedVideoMemory >> 20 ), echt[ i ] == gewaehlt ? " - chosen" : "" );
		if ( echt[ i ] != gewaehlt ) {
			echt[ i ]->Release();
		}
	}
	return gewaehlt;
}

bool dispatchen( UINT basis, UINT anzahl ){
	struct Kopf
	{
		UINT anzahl, basis;
		int kopfKnoten, faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
		float linearScale;
		UINT visBytes;
		int leafBytes;
	} kopf{ anzahl, basis, g.kopfKnoten, g.schalter.faster, g.schalter.lightAngleHL, g.schalter.bouncing, g.schalter.testOcclusion, g.schalter.oneSky,
	        g.schalter.noSurfaces, g.schalter.linearScale, g.visBytes, g.leafBytes };
	g.ktx->UpdateSubresource( g.kopf, 0, nullptr, &kopf, 0, 0 );
	g.ktx->Dispatch( ( anzahl + 63 ) / 64, 1, 1 );
	++g.zahlen.dispatches;
	if ( g.verlustNach >= 0 && g.zahlen.dispatches > g.verlustNach ) {
		return false;   // Test: Geraeteverlust vorgetaeuscht (BHED_GPUBAKE_VERLUST)
	}
	return g.geraet->GetDeviceRemovedReason() == S_OK;
}

} // namespace

bool BhedBakeStart( const std::vector<BhedBakeKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedBakeDreieck>& dreiecke,
                    const std::vector<BhedBakeInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten,
                    const unsigned char *vis, std::size_t visBytes, const std::string& adapterWahl ){
	if ( const char *v = std::getenv( "BHED_GPUBAKE_VERLUST" ) ) {
		g.verlustNach = std::atoll( v );
	}
	g.d3d = LoadLibraryW( L"d3d11.dll" );
	g.compiler = LoadLibraryW( L"d3dcompiler_47.dll" );
	if ( g.d3d == nullptr || g.compiler == nullptr ) {
		Sys_Printf( "BehavEd GPU bake: d3d11.dll or d3dcompiler_47.dll missing - CPU\n" );
		return false;
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( g.d3d, "D3D11CreateDevice" ) ) );
	const auto kompilieren = reinterpret_cast<pD3DCompile>( reinterpret_cast<void *>( GetProcAddress( g.compiler, "D3DCompile" ) ) );
	if ( erzeugen == nullptr || kompilieren == nullptr ) {
		return false;
	}
	IDXGIAdapter1 *adapter = adapterWaehlen( adapterWahl );
	const D3D_FEATURE_LEVEL stufen[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL stufe{};
	const HRESULT erzeugt = erzeugen( adapter, adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, stufen, 2, D3D11_SDK_VERSION,
	                                  &g.geraet, &stufe, &g.ktx );
	if ( adapter != nullptr ) {
		DXGI_ADAPTER_DESC1 d{};
		adapter->GetDesc1( &d );
		g.name = utf8( d.Description );
		adapter->Release();
	}
	if ( FAILED( erzeugt ) ) {
		Sys_Printf( "BehavEd GPU bake: no Direct3D 11 hardware device - CPU\n" );
		return false;
	}
	D3D11_FEATURE_DATA_DOUBLES dd{};
	D3D11_FEATURE_DATA_D3D11_OPTIONS op{};
	const bool d1 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_DOUBLES, &dd, sizeof( dd ) ) ) && dd.DoublePrecisionFloatShaderOps;
	const bool d2 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &op, sizeof( op ) ) ) && op.ExtendedDoublesShaderInstructions;
	g.doppelt = d1 && d2 && std::getenv( "BHED_GPUBAKE_OHNE_DOUBLE" ) == nullptr;
	const D3D_SHADER_MACRO makros[] = { { "DOPPELT", g.doppelt ? "1" : "0" }, { "DIAG", std::getenv( "BHED_GPUBAKE_DIAG" ) != nullptr ? "1" : "0" }, { nullptr, nullptr } };
	ID3DBlob *code = nullptr;
	ID3DBlob *fehler = nullptr;
	const UINT flags = D3DCOMPILE_IEEE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	if ( FAILED( kompilieren( g_shader, std::strlen( g_shader ), "bhed_gpubacken", makros, nullptr, "main", "cs_5_0", flags, 0, &code, &fehler ) ) ) {
		Sys_Printf( "BehavEd GPU bake: shader error: %s\n", fehler != nullptr ? static_cast<const char *>( fehler->GetBufferPointer() ) : "?" );
		frei( fehler );
		allesFrei();
		return false;
	}
	frei( fehler );
	const HRESULT hr = g.geraet->CreateComputeShader( code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g.cs );
	code->Release();
	if ( FAILED( hr ) ) {
		allesFrei();
		return false;
	}
	// Speicherbedarf der Szene (BVH-Ersatz: Knoten, Dreiecke) gegen den Grafikspeicher
	const size_t bedarf = knoten.size() * sizeof( BhedBakeKnoten ) + items.size() * 4 + dreiecke.size() * sizeof( BhedBakeDreieck ) +
	                      infos.size() * sizeof( BhedBakeInfo ) + visBytes;
	std::vector<unsigned char> visKopie( vis != nullptr ? vis : reinterpret_cast<const unsigned char *>( "" ), vis != nullptr ? vis + visBytes : reinterpret_cast<const unsigned char *>( "" ) );
	visKopie.resize( ( visKopie.size() + 3 ) & ~size_t( 3 ), 0 );
	if ( visKopie.empty() ) {
		visKopie.assign( 4, 0 );
	}
	bool ok = puffer( knoten.data(), sizeof( BhedBakeKnoten ), UINT( knoten.size() ), g.szene[ 0 ], g.szeneSrv[ 0 ] ) &&
	          puffer( items.data(), sizeof( int ), UINT( items.size() ), g.szene[ 1 ], g.szeneSrv[ 1 ] ) &&
	          puffer( dreiecke.data(), sizeof( BhedBakeDreieck ), UINT( dreiecke.size() ), g.szene[ 2 ], g.szeneSrv[ 2 ] ) &&
	          puffer( infos.data(), sizeof( BhedBakeInfo ), UINT( infos.size() ), g.szene[ 3 ], g.szeneSrv[ 3 ] ) &&
	          puffer( anzahlFlaechen > 0 ? flaechen : nullptr, sizeof( int ), UINT( anzahlFlaechen ), g.szene[ 4 ], g.szeneSrv[ 4 ] ) &&
	          puffer( visKopie.data(), 4, UINT( visKopie.size() / 4 ), g.szene[ 5 ], g.szeneSrv[ 5 ], true );
	if ( ok ) {
		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = 48;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ok = SUCCEEDED( g.geraet->CreateBuffer( &bd, nullptr, &g.kopf ) ) && ergebnisPlatz( 1u << 20 );
	}
	if ( !ok ) {
		Sys_Printf( "BehavEd GPU bake: not enough graphics memory for the scene (%zu MB) - CPU\n", bedarf >> 20 );
		allesFrei();
		return false;
	}
	g.kopfKnoten = kopfKnoten;
	g.visBytes = UINT( visBytes );
	g.leafBytes = visBytes > 8 ? reinterpret_cast<const int *>( vis )[ 1 ] : 0;
	g.bereit = true;
	Sys_Printf( "BehavEd GPU bake: %s (feature level %x, %s), %zu nodes, %zu triangles, scene %zu KB\n", g.name.c_str(), unsigned( stufe ),
	             g.doppelt ? "double precision" : "no double precision - float approximations", knoten.size(), dreiecke.size(), bedarf >> 10 );
	return true;
}

bool BhedBakeLichter( const std::vector<BhedBakeLicht>& lichter, const std::vector<float>& windungen, const BhedBakeSchalter& s,
                      const std::vector<BhedBakeInfo>& infos ){
	if ( !g.bereit ) {
		return false;
	}
	g.schalter = s;
	if ( !puffer( infos.data(), sizeof( BhedBakeInfo ), UINT( infos.size() ), g.szene[ 3 ], g.szeneSrv[ 3 ] ) ||
	     !puffer( lichter.data(), sizeof( BhedBakeLicht ), UINT( lichter.size() ), g.licht[ 0 ], g.lichtSrv[ 0 ] ) ||
	     !puffer( windungen.data(), sizeof( float ), UINT( windungen.size() ), g.licht[ 1 ], g.lichtSrv[ 1 ] ) ) {
		ausfall( "not enough graphics memory for the lights" );
		return false;
	}
	return true;
}

bool BhedBakeRechnen( const std::vector<BhedBakeLuxel>& luxel, const std::vector<BhedBakeLm>& lms, const std::vector<BhedBakePaar>& paare,
                      std::vector<BhedBakeErg>& erg ){
	const size_t n = paare.size();
	erg.resize( n );
	const auto alleCpu = [&](){
		for ( BhedBakeErg& e : erg ) {
			e = BhedBakeErg{ { 0, 0, 0 }, 9 };
		}
		g.zahlen.cpuPaare += static_cast<long long>( n );
	};
	if ( !g.bereit ) {
		alleCpu();
		return false;
	}
	if ( n == 0 ) {
		return true;
	}
	const auto t0 = std::chrono::steady_clock::now();
	if ( !puffer( luxel.data(), sizeof( BhedBakeLuxel ), UINT( luxel.size() ), g.block[ 0 ], g.blockSrv[ 0 ] ) ||
	     !puffer( lms.data(), sizeof( BhedBakeLm ), UINT( lms.size() ), g.block[ 1 ], g.blockSrv[ 1 ] ) ||
	     !puffer( paare.data(), sizeof( BhedBakePaar ), UINT( n ), g.block[ 2 ], g.blockSrv[ 2 ] ) || !ergebnisPlatz( UINT( n ) ) ) {
		ausfall( "not enough graphics memory for a block" );
		alleCpu();
		return false;
	}
	ID3D11ShaderResourceView *srv[ 11 ] = { g.szeneSrv[ 0 ], g.szeneSrv[ 1 ], g.szeneSrv[ 2 ], g.szeneSrv[ 3 ], g.szeneSrv[ 4 ], g.szeneSrv[ 5 ],
	                                        g.lichtSrv[ 0 ], g.lichtSrv[ 1 ], g.blockSrv[ 0 ], g.blockSrv[ 1 ], g.blockSrv[ 2 ] };
	g.ktx->CSSetShader( g.cs, nullptr, 0 );
	g.ktx->CSSetShaderResources( 0, 11, srv );
	g.ktx->CSSetUnorderedAccessViews( 0, 1, &g.ergUav, nullptr );
	g.ktx->CSSetConstantBuffers( 0, 1, &g.kopf );
	// Kurze Dispatches (Windows-Zeitlimit TDR 2 s, die Oberflaeche nutzt dieselbe GPU):
	// je 32768 Paare, nach je 16 ein Flush mit Warten auf die GPU.
	constexpr UINT kStueck = 32768;
	bool ok = true;
	ID3D11Query *ende = nullptr;
	D3D11_QUERY_DESC qd{ D3D11_QUERY_EVENT, 0 };
	(void)g.geraet->CreateQuery( &qd, &ende );
	int seitWarten = 0;
	for ( UINT basis = 0; basis < UINT( n ) && ok; basis += kStueck ) {
		ok = dispatchen( basis, std::min<UINT>( kStueck, UINT( n ) - basis ) );
		if ( ok && ++seitWarten >= 16 && ende != nullptr ) {
			seitWarten = 0;
			g.ktx->End( ende );
			BOOL fertig = FALSE;
			while ( g.ktx->GetData( ende, &fertig, sizeof( fertig ), 0 ) == S_FALSE ) {
				if ( g.geraet->GetDeviceRemovedReason() != S_OK ) {
					ok = false;
					break;
				}
				SwitchToThread();
			}
		}
	}
	frei( ende );
	if ( ok ) {
		ID3D11UnorderedAccessView *keine = nullptr;
		g.ktx->CSSetUnorderedAccessViews( 0, 1, &keine, nullptr );
		D3D11_BOX box{ 0, 0, 0, UINT( n * sizeof( BhedBakeErg ) ), 1, 1 };
		g.ktx->CopySubresourceRegion( g.lesen, 0, 0, 0, 0, g.erg, 0, &box );
		D3D11_MAPPED_SUBRESOURCE m{};
		if ( SUCCEEDED( g.ktx->Map( g.lesen, 0, D3D11_MAP_READ, 0, &m ) ) ) {
			std::memcpy( erg.data(), m.pData, n * sizeof( BhedBakeErg ) );
			g.ktx->Unmap( g.lesen, 0 );
		}
		else {
			ok = false;
		}
	}
	g.zahlen.gpuMikros += std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - t0 ).count();
	++g.zahlen.bloecke;
	if ( !ok ) {
		char z[ 160 ];
		std::snprintf( z, sizeof( z ), "the graphics device was lost (%s)", g.verlustNach >= 0 ? "simulated for the test" : "TDR or driver reset" );
		ausfall( z );
		alleCpu();
		return false;
	}
	// Treiberfehler: nicht endliche Werte rechnet die CPU nach
	for ( BhedBakeErg& e : erg ) {
		if ( e.code != 9 && !( std::isfinite( e.c[ 0 ] ) && std::isfinite( e.c[ 1 ] ) && std::isfinite( e.c[ 2 ] ) ) ) {
			e.code = 9;
			++g.zahlen.nichtEndlich;
		}
		if ( e.code == 9 ) {
			++g.zahlen.cpuPaare;
		}
	}
	g.zahlen.paare += static_cast<long long>( n );
	return true;
}

bool BhedBakeBereit(){
	return g.bereit;
}

void BhedBakeEnde(){
	allesFrei();
}

std::string BhedBakeAdapter(){
	return g.name;
}

bool BhedBakeDoppelt(){
	return g.doppelt;
}

BhedBakeZahlen BhedBakeStatistik(){
	return g.zahlen;
}
