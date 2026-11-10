// bhed_lichtcache.h - BehavEd stage 8a (partial compiles, not in NetRadiant-Custom):
// -light -bhedlightcache keeps the traced raw lighting of a run in <map>.bhedlight and
// reuses it in the next run where it cannot have changed. lcb1: also with -bounce.
//
//   * the whole input that tracing depends on (geometry lumps without the fields -light
//     writes, the .srf file, every shader script, the brush entities, worldspawn keys
//     except the post-processing ones) and the light switches except the post-processing
//     ones must be equal - otherwise nothing is reused (the run says which part changed);
//   * every pass is kept on its own: the first one (direct light) and each -bounce pass
//     (the diffuse lights RadCreateDiffuseLights made from the pass before);
//   * a raw lightmap of a pass is taken from the cache when all of this is the same as in
//     the cached run: the pass number, the random generator of its work item, its culled
//     light list (CreateTraceLightsForBounds: the same lights with every field equal, in the
//     same order - in a bounce pass that is every diffuse light that can reach it) and its
//     own state at the beginning of the pass that IlluminateRawLightmap reads (styles,
//     ambient, the luxels that are not overwritten, flags, clusters, deluxels). Then the
//     state after the pass cannot differ - it is copied bit for bit; otherwise it is traced;
//   * a vertex lit surface likewise (CreateTraceLightsForSurface, its vertex luxels and styles);
//   * the light grid of a pass is taken when every light, the grid origins and the raw
//     grid before the pass are the same, else traced.
//
// Stitching, StoreSurfaceLightmaps and RadCreateDiffuseLights run on every lightmap in every
// pass as always (they are not cached); they only read what the passes left, which is the
// same as in a full run. The post-processing (gamma, exposure, contrast, compensate,
// saturation, minlight, minvertexlight, mingridlight, maxlight, gridscale, gridambientscale)
// runs on the raw values in StoreSurfaceLightmaps and when the grid is stored - so changing
// only these retraces nothing of the first pass. Everything else falls back to tracing, so
// the output always equals a full run.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct trace_t;

// LightMain: the switch was given; args = the light switches (argsToInject) and the map
void BhedLichtCacheSchalter( bool an, const std::vector<const char*>& args, const char* bspName );
[[nodiscard]] bool BhedLichtCacheAn();
// LightMain after LoadBSPFile, ParseEntities, LoadMapFile, SetEntityOrigins: keys, read the old cache's passes
void BhedLichtCacheGeometrie();
// a hash for the callers' own inputs (the grid origins)
[[nodiscard]] std::uint64_t BhedLichtCacheHash( const void* daten, std::size_t n, std::uint64_t start );
// LightWorld: pass b begins (0 = the first, b = bounce b) - loads the cached pass b
void BhedLichtCacheDurchgang( int b );
// before TraceGrid of a pass (after the grid envelopes): true = the grid comes from the cache
// (raw points restored, the caller still stores them); ursprung = BhedLichtCacheHash of the grid origins
[[nodiscard]] bool BhedGitterAusCache( std::uint64_t ursprung );
// after the grid of a pass (traced or restored): keep the raw grid for the new cache
void BhedGitterMerken();
// BhedIlluminateRawLightmaps begins (after the lightmap envelopes): hash every light of the pass
void BhedLichtCacheLichter();
// before the pass, for every raw lightmap: its culled light list (as IlluminateRawLightmap makes it) and the
// generator of its work item; true = restored from the cache, the pass skips it
[[nodiscard]] bool BhedLmVorab( int lmNum, const trace_t& trace, unsigned int seed );
// after all BhedLmVorab of the pass: the line for the build window
void BhedLmVorabFertig();
// after BhedIlluminateRawLightmaps, before StitchSurfaceLightmaps: copy the raw lightmaps for the new cache
void BhedLmDatenMerken();
// IlluminateVertexes (vertex lit surfaces) after CreateTraceLightsForSurface: true = restored
[[nodiscard]] bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace, unsigned int seed );
// after IlluminateVertexes of a pass: write the pass into the new cache
void BhedLichtCacheDurchgangEnde();
// LightWorld before the final StoreSurfaceLightmaps (or when radiosity ends early): <map>.bhedlight, the summary
void BhedLichtCacheSchreiben();
