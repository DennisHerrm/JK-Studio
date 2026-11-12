// karte_dxr.hlsl - Karten-Editor, Echtzeit-Licht: Schatten per Raytracing (DXR 1.1, RayQuery, Shader Model 6.5).
//
// Die zweite Schattenart neben den Schattenkarten (Build > Ray-traced shadows; nur Direct3D 12 mit DXR 1.1). Dieser Text steht
// VOR dem Echtzeit-Licht-Shader (kShader4 in gui/karte_gpu_win32.cpp); der Bau setzt beide zusammen (tools/dxr/dxr_quelle.cpp)
// und uebersetzt sie mit dxc aus dem Windows SDK zu DXIL (CMakeLists.txt, Block DXR). Im Shader steht an der Stelle der
// Schattenkarten (lichtVon) unter DXR_SCHATTEN der Aufruf dxrSicht - alles andere (Lichtformeln, Kacheln, Nebel) ist derselbe
// Text wie mit den Schattenkarten.
//
// Die Strahlen folgen q3map2 -light (third_party/q3map2/tools/quake3/q3map2):
//   - Ursprung: die Stelle plus Normale mal DEFAULT_LIGHTMAP_SAMPLE_OFFSET 1.0 (q3map2.h:169) - dort liegt der Luxel.
//   - Treffer zaehlen zwischen DEFAULT_INHIBIT_RADIUS 1.5 (q3map2.h:161) und dem Abstand zum Licht, beide Grenzen
//     ausgeschlossen (TraceTriangle, light_trace.cpp:1287: depth <= inhibitRadius || depth >= distance); DXR prueft Dreiecke
//     ebenso mit TMin < t < TMax.
//   - Punkt-, Spot- und Flaechenlicht (TraceLine ohne testAll): jedes Dreieck der Schattengeometrie zwischen Stelle und Licht
//     verdeckt - dieselben Arten, die im Schattenwuerfel stehen (deckend, Volumen, Himmel, Himmelsvolumen).
//   - Sonne (testAll, LightContributionToSample light.cpp:1124: "!( trace->compileFlags & C_SKY ) || trace->opaque" -> dunkel): hell nur,
//     wenn der Strahl zur Sonne (Laenge MAX_WORLD_COORD * 8, light.cpp:116) zuerst eine Himmelsflaeche trifft, die von der
//     Sonne wegzeigt, und davor weder ein deckendes Dreieck noch das Volumen eines strukturell deckenden Pinsels - dieselbe
//     Regel wie die Karten S (Himmelstiefe) und O (Verdecker darunter) der Schattenkarten.
//   - alphashadow und lightfilter (TraceTriangle, light_trace.cpp:1325-1364): st aus den Schwerpunktkoordinaten, der Texel
//     ohne Filter und wiederholt; lightfilter mal Farbe des Texels, alphashadow mal (255 - Alpha) / 255; der Strahl geht
//     weiter, bis alle drei Anteile hoechstens 0.001 sind. (Die Schattenkarten kennen hier nur einen Alpha-Test bei 0.5.)
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors (q3map2: die Regeln der Strahlen).
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
//   License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).

// gui/karte_dxr_win32.cpp bindet diese drei Plaetze (die Plaetze t0 bis t10 gehoeren dem Echtzeit-Licht).
RaytracingAccelerationStructure dxrSzene : register(t11);
Texture2DArray<float4> dxrBilder : register(t12);   // die Bilder der alphashadow-/lightfilter-Teile (Seiten eines Atlas)
// [0].x: Zahl der Filter-Teile. Je Teil zwei Werte ab [1 + 2 * Teil]: (Seite, Ecke x, Ecke y, Art: 1 alphashadow, 2 lightfilter,
// 3 beide) und (Breite, Hoehe, erste Zeile seiner st). Je Dreieck zwei Werte: (st0, st1) und (st2, 0, 0).
Buffer<float4> dxrDaten : register(t13);

// Maske einer Instanz: 1 << SchattenTeil::art (gui/karte_gpu.h)
static const uint kDxrDeckend = 1u;
static const uint kDxrVolumen = 2u;
static const uint kDxrHimmel = 4u;
static const uint kDxrFilter = 8u;
static const uint kDxrHimmelVolumen = 16u;
// Kennung einer Instanz ohne Filter: kDxrKennungArt + art; mit Filter: die Nummer des Filter-Teils
static const uint kDxrKennungArt = 0xFFFF00u;

static const float kDxrVersatz = 1.0;     // DEFAULT_LIGHTMAP_SAMPLE_OFFSET
static const float kDxrNah = 1.5;         // DEFAULT_INHIBIT_RADIUS
static const float kDxrFern = 524288.0;   // MAX_WORLD_COORD * 8 (65536 * 8)

// TraceTriangle: was ein alphashadow-/lightfilter-Dreieck vom Licht durchlaesst
float3 dxrFilter(uint teil, uint dreieck, float2 uv) {
    float4 a = dxrDaten[1u + teil * 2u];
    float4 b = dxrDaten[2u + teil * 2u];
    uint zeile = (uint)b.z + dreieck * 2u;
    float4 s01 = dxrDaten[zeile];
    float4 s2 = dxrDaten[zeile + 1u];
    float w = 1.0 - (uv.x + uv.y);
    float2 st = w * s01.xy + uv.x * s01.zw + uv.y * s2.xy;
    st = st - floor(st);
    int ix = clamp((int)(st.x * b.x), 0, (int)b.x - 1);
    int iy = clamp((int)(st.y * b.y), 0, (int)b.y - 1);
    float4 texel = dxrBilder.Load(int4((int)a.y + ix, (int)a.z + iy, (int)a.x, 0));
    uint art = (uint)a.w;
    float3 f = float3(1.0, 1.0, 1.0);
    if ((art & 2u) != 0u) { f *= texel.rgb; }
    if ((art & 1u) != 0u) { f *= 1.0 - texel.a; }
    return f;
}

bool dxrDicht(float3 f) { return f.r <= 0.001 && f.g <= 0.001 && f.b <= 0.001; }

// Punkt-, Spot- und Flaechenlicht: was von q bis zum Licht durchkommt (0: verdeckt)
float3 dxrZumLicht(float3 q, float3 licht) {
    float3 d = licht - q;
    float abstand = length(d);
    if (abstand <= kDxrNah) { return float3(1.0, 1.0, 1.0); }
    RayDesc r;
    r.Origin = q;
    r.Direction = d / abstand;
    r.TMin = kDxrNah;
    r.TMax = abstand;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> s;
    s.TraceRayInline(dxrSzene, RAY_FLAG_NONE, kDxrDeckend | kDxrVolumen | kDxrHimmel | kDxrFilter | kDxrHimmelVolumen, r);
    float3 f = float3(1.0, 1.0, 1.0);
    while (s.Proceed()) {
        if (s.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
            f *= dxrFilter(s.CandidateInstanceID(), s.CandidatePrimitiveIndex(), s.CandidateTriangleBarycentrics());
            if (dxrDicht(f)) { s.CommitNonOpaqueTriangleHit(); }
        }
    }
    return s.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? float3(0.0, 0.0, 0.0) : f;
}

// Sonne: hell, wenn der Strahl zuerst den Himmel trifft; davor liegende Filter faerben
float3 dxrZurSonne(float3 q, float3 zurSonne) {
    RayDesc r;
    r.Origin = q;
    r.Direction = normalize(zurSonne);
    r.TMin = kDxrNah;
    r.TMax = kDxrFern;
    // Himmelsflaechen zaehlen nur von vorn (die Instanzen der anderen Arten schalten das Wegschneiden ab)
    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> s;
    s.TraceRayInline(dxrSzene, RAY_FLAG_NONE, kDxrDeckend | kDxrVolumen | kDxrHimmel, r);
    s.Proceed();
    if (s.CommittedStatus() != COMMITTED_TRIANGLE_HIT || s.CommittedInstanceID() != kDxrKennungArt + 2u) { return float3(0.0, 0.0, 0.0); }
    float3 f = float3(1.0, 1.0, 1.0);
    if (dxrDaten[0].x < 0.5) { return f; }
    r.TMax = s.CommittedRayT();
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> t;
    t.TraceRayInline(dxrSzene, RAY_FLAG_NONE, kDxrFilter, r);
    while (t.Proceed()) {
        if (t.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
            f *= dxrFilter(t.CandidateInstanceID(), t.CandidatePrimitiveIndex(), t.CandidateTriangleBarycentrics());
            if (dxrDicht(f)) {
                t.Abort();
                return float3(0.0, 0.0, 0.0);
            }
        }
    }
    return f;
}

// Der Anteil des Lichts, der an der Stelle p (Normale n) ankommt. art: 2 Sonne (richtung = zur Sonne), sonst das Licht am Ort.
float3 dxrSicht(int art, float3 ort, float3 richtung, float3 p, float3 n) {
    float3 q = p + n * kDxrVersatz;
    if (art == 2) { return dxrZurSonne(q, richtung); }
    return dxrZumLicht(q, ort);
}
