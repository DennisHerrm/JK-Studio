// bhed_lichtcache.h - BehavEd stage 8a (partial compiles, not in NetRadiant-Custom):
// -light -bhedlightcache keeps the traced raw lighting of a run in <map>.bhedlight and
// reuses it in the next run where it cannot have changed:
//
//   * the whole input that tracing depends on (geometry lumps without the fields -light
//     writes, the .srf file, every shader script, the brush entities, worldspawn keys
//     except the post-processing ones, the light switches except the post-processing
//     ones) must be equal ("geometry key"), and -bounce must be 0;
//   * a raw lightmap is taken from the cache when its culled light list
//     (CreateTraceLightsForBounds: the same lights with every field equal, in the same
//     order) is the same - then its super luxels cannot differ; otherwise it is traced;
//   * a vertex lit surface likewise (CreateTraceLightsForSurface);
//   * the light grid is taken when the whole grid light list is the same, else traced.
//
// The post-processing (gamma, exposure, contrast, compensate, saturation, minlight,
// minvertexlight, mingridlight, maxlight, gridscale, gridambientscale) runs on the raw
// values in StoreSurfaceLightmaps and when the grid is stored - so changing only these
// retraces nothing, and the result is byte-identical to a full -light with the same
// values. Everything else falls back to tracing, so the output always equals a full run.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <cstdint>
#include <vector>

struct trace_t;

// LightMain: the switch was given; args = the light switches (argsToInject) and the map
void BhedLichtCacheSchalter( bool an, const std::vector<const char*>& args, const char* bspName );
[[nodiscard]] bool BhedLichtCacheAn();
// LightMain after LoadBSPFile, ParseEntities, LoadMapFile, SetEntityOrigins: geometry key, read the cache
void BhedLichtCacheGeometrie();
// LightWorld before TraceGrid (after the grid envelopes): true = the grid comes from the cache
// (raw points restored, the caller still stores them)
[[nodiscard]] bool BhedGitterAusCache();
// BhedIlluminateRawLightmaps begins (after the lightmap envelopes): hash every light
void BhedLichtCacheLichter();
// IlluminateRawLightmap_ after CreateTraceLightsForBounds: true = restored from the cache
[[nodiscard]] bool BhedLmAusCache( int lmNum, const trace_t& trace );
// after BhedIlluminateRawLightmaps, before StitchSurfaceLightmaps: copy the raw lightmaps for the new cache
void BhedLmDatenMerken();
// IlluminateVertexes (vertex lit surfaces) after CreateTraceLightsForSurface: true = restored
[[nodiscard]] bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace );
// LightWorld before the final StoreSurfaceLightmaps: write <map>.bhedlight, print the summary
void BhedLichtCacheSchreiben();
