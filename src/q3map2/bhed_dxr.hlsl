// bhed_dxr.hlsl - BehavEd Stufe 8g Punkt 3: DXR-Vortest (D3D12 RayQuery, Shader Model 6.5).
// Im Bau mit dxc aus dem Windows SDK vorab als DXIL uebersetzt (src/q3map2/CMakeLists.txt).
//
// Ein Faden je Schattenstrahl (BhedGpuStrahl, src/q3map2/bhed_gpuschatten.h): trifft der Strahl
// zwischen inhibit und dist ein Dreieck der Szene (Beschleunigungsstruktur aus q3map2s
// Trace-Dreiecken, die Schatten werfen koennen)? Die Hardware testet ohne q3map2s Epsilons -
// gemessen wird, wie schnell das ist und wie oft es mit dem genauen Weg uebereinstimmt.
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#define WURZEL "SRV(t0), SRV(t1), UAV(u0), RootConstants(num32BitConstants=1, b0)"

struct Strahl
{
    float ox; float oy; float oz;
    float ex; float ey; float ez;
    float dx; float dy; float dz;
    float dist; float inhibit;
    int recv; int sErste; int sAnzahl; int flags; int rest;
};
RaytracingAccelerationStructure szene : register(t0);
StructuredBuffer<Strahl> strahlen : register(t1);
RWStructuredBuffer<uint> treffer : register(u0);
cbuffer Kopf : register(b0) { uint anzahl; };

[RootSignature(WURZEL)]
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= anzahl) { return; }
    const Strahl s = strahlen[id.x];
    RayDesc r;
    r.Origin = float3(s.ox, s.oy, s.oz);
    r.Direction = float3(s.dx, s.dy, s.dz);
    r.TMin = s.inhibit;
    r.TMax = s.dist;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(szene, 0, 0xFF, r);
    q.Proceed();
    treffer[id.x] = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 1u : 0u;
}
