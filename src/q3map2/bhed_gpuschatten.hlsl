// bhed_gpuschatten.hlsl - BehavEd Stufe 8b/8g: der Compute-Shader von q3map2 -light -bhedgpu
// (Schattentests der ersten Abtastung auf der GPU, src/q3map2/bhed_gpuschatten.cpp): TraceLine_r
// und TraceTriangle aus q3map2 light_trace.cpp. Seit Stufe 8g im Bau mit fxc vorab uebersetzt.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
// X4714: nur ein Leistungshinweis zum Stapel der Traversierung
#pragma warning(disable : 4714)
struct Knoten { int typ; float nx; float ny; float nz; float dist; int k0; int k1; int anzahl; int erstes; };
struct Dreieck { float v0x; float v0y; float v0z; float e1x; float e1y; float e1z; float e2x; float e2y; float e2z; int info; };
struct Info { int cast; int skipGrid; int flaeche; int flags; };
struct Strahl { float ox; float oy; float oz; float ex; float ey; float ez; float dx; float dy; float dz; float dist; float inhibit;
                int recv; int sErste; int sAnzahl; int flags; int rest; };
struct Ergebnis { int status; float tiefe; float hx; float hy; float hz; int p0; int p1; int p2; };
cbuffer Kopf : register(b0) { uint anzahl; int kopfKnoten; float baryEps; float baryEins; float coplanar; float selbst; float traceOn; float aslfFrei; };
StructuredBuffer<Knoten> knoten : register(t0);
StructuredBuffer<int> items : register(t1);
StructuredBuffer<Dreieck> dreiecke : register(t2);
StructuredBuffer<Info> infos : register(t3);
StructuredBuffer<int> flaechen : register(t4);
StructuredBuffer<Strahl> strahlen : register(t5);
RWStructuredBuffer<Ergebnis> aus : register(u0);

static bool g_unsicher;

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

// TraceTriangle fuer alle Dreiecke eines Blatts: 0 nichts, 1 undurchsichtig (tiefe), 3 die CPU
int blattTesten(int knr, Strahl r, float3 start, float3 richtung, inout float tiefeAus)
{
    const Knoten n = knoten[knr];
    [loop] for (int j = 0; j < n.anzahl; ++j)
    {
        const Dreieck d = dreiecke[items[n.erstes + j]];
        const Info f = infos[d.info];
        if ((f.flags & 1) != 0) { continue; }   // Himmel: TraceTriangle gibt immer false
        if (r.recv == 1) { if (f.cast <= 0) { continue; } }
        else if (r.recv > 1) { if (f.cast != 1 && abs(f.cast) != abs(r.recv)) { continue; } }
        else { if (abs(f.cast) != abs(r.recv)) { continue; } }
        if ((r.flags & 1) != 0 && f.skipGrid != 0) { continue; }
        const float3 v0 = float3(d.v0x, d.v0y, d.v0z);
        const float3 e1 = float3(d.e1x, d.e1y, d.e1z);
        const float3 e2 = float3(d.e2x, d.e2y, d.e2z);
        precise float3 pvec = kreuz(richtung, e2);
        precise float det = punkt3(e1, pvec);
        if (abs(det) < coplanar) { continue; }
        precise float3 tvec = start - v0;
        precise float uu = punkt3(tvec, pvec);
        // Vorpruefung mit der Hardware-Division: nur verwerfen, was auch genau gerechnet
        // sicher verworfen wird (Abstand zur Grenze 1e-3 relativ, die Division weicht
        // hoechstens wenige ULP ab); alles andere rechnet unten genau.
        {
            const float invA = 1.0f / det;
            const float uA = uu * invA;
            const float tolU = 1e-3f * max(1.0f, abs(uA));
            if (uA < -baryEps - tolU || uA > baryEins + tolU) { continue; }
        }
        precise float invDet = teilen(1.0f, det);
        precise float u = uu * invDet;
        if (u < -baryEps || u > baryEins) { continue; }
        precise float3 qvec = kreuz(tvec, e1);
        precise float vv = punkt3(richtung, qvec);
        precise float v = vv * invDet;
        precise float uv = u + v;
        if (v < -baryEps || uv > baryEins) { continue; }
        precise float tt = punkt3(e2, qvec);
        precise float tiefe = tt * invDet;
        if (tiefe <= r.inhibit || tiefe >= r.dist) { continue; }
        if (tiefe <= selbst)
        {
            bool eigen = false;
            [loop] for (int s = 0; s < r.sAnzahl; ++s) { if (flaechen[r.sErste + s] == f.flaeche) { eigen = true; break; } }
            if (eigen) { continue; }
        }
        if (g_unsicher) { return 3; }
        if ((f.flags & 2) != 0) { return 3; }   // Alpha-Schatten / Lichtfilter: die CPU
        tiefeAus = tiefe;
        return 1;
    }
    return g_unsicher ? 3 : 0;
}

// TraceLine: die Blaetter werden gleich beim Erreichen getestet (die ersten 256 mit
// Dreiecken, MAX_TRACE_TEST_NODES). Fuer die Entscheidung ist das gleichwertig:
// undurchsichtig, wenn ein festes Blatt erreicht wird ODER ein undurchsichtiges Dreieck
// eines der ersten 256 Blaetter die Tests besteht; ein Alpha-Dreieck gibt Status 3.
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint i = id.x;
    if (i >= anzahl) { return; }
    g_unsicher = false;
    const Strahl r = strahlen[i];
    if (r.rest != 0) { Ergebnis x; x.status = 3; x.tiefe = 0.0f; x.hx = 0.0f; x.hy = 0.0f; x.hz = 0.0f; x.p0 = 0; x.p1 = 0; x.p2 = 0; aus[i] = x; return; }
    precise float3 start = float3(r.ox, r.oy, r.oz);
    precise float3 ende = float3(r.ex, r.ey, r.ez);
    precise float3 richtung = float3(r.dx, r.dy, r.dz);
    Ergebnis e;
    e.status = 0; e.tiefe = 0.0f; e.hx = 0.0f; e.hy = 0.0f; e.hz = 0.0f; e.p0 = 0; e.p1 = 0; e.p2 = 0;
    const bool flaechenTesten = (r.flags & 2) == 0;   // noSurfaces

    // TraceLine_r (halb iterativ; die zweite Seite als Fortsetzung auf dem Stapel)
    int nTest = 0;
    int stK[96];
    float3 stO[96];
    float3 stE[96];
    int sp = 0;
    int k = kopfKnoten;
    precise float3 o = start;
    precise float3 en = ende;
    [loop] while (true)
    {
        if (k < 0) { e.status = 2; e.hx = o.x; e.hy = o.y; e.hz = o.z; break; }
        const Knoten n = knoten[k];
        if (n.typ == -2) { e.status = 2; e.hx = o.x; e.hy = o.y; e.hz = o.z; break; }
        if (n.typ < 0)
        {
            if (n.anzahl > 0 && nTest < 256)
            {
                nTest += 1;
                if (flaechenTesten)
                {
                    float tiefe = 0.0f;
                    const int b = blattTesten(k, r, start, richtung, tiefe);
                    if (b == 1) { e.status = 1; e.tiefe = tiefe; break; }
                    if (b == 3) { e.status = 3; break; }
                }
            }
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
        if (vorn >= -traceOn && hinten >= -traceOn) { k = n.k0; continue; }
        if (vorn < traceOn && hinten < traceOn) { k = n.k1; continue; }
        const int seite = vorn < 0.0f ? 1 : 0;
        precise float nenner = vorn - hinten;
        precise float frac = teilen(vorn, nenner);
        precise float3 mitte = o + (en - o) * frac;
        if (sp >= 96) { g_unsicher = true; break; }
        stK[sp] = seite == 1 ? n.k0 : n.k1; stO[sp] = mitte; stE[sp] = en; sp += 1;
        k = seite == 1 ? n.k1 : n.k0;
        en = mitte;
    }
    if (g_unsicher) { e.status = 3; }
    aus[i] = e;
}
