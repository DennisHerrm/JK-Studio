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
#include <cstring>
#include <vector>

struct trace_t;
struct shaderInfo_t;

// --- the fingerprint (128 bit) ----------------------------------------------------------------------
struct BhedSig {
	std::uint64_t a = 0;
	std::uint64_t b = 0;
	[[nodiscard]] bool da() const { return a != 0 || b != 0; }
	bool operator==( const BhedSig& o ) const { return a == o.a && b == o.b; }
};

// two 64 bit lanes over 8 byte words (the old byte wise FNV took seconds on big maps)
struct BhedHash {
	std::uint64_t a = 0x243F6A8885A308D3ULL;
	std::uint64_t b = 0x13198A2E03707344ULL;
	static std::uint64_t rotl( std::uint64_t x, int r ){ return ( x << r ) | ( x >> ( 64 - r ) ); }
	static std::uint64_t mix( std::uint64_t x ){
		x ^= x >> 32;
		x *= 0xD6E8FEB86659FD93ULL;
		x ^= x >> 32;
		x *= 0xD6E8FEB86659FD93ULL;
		x ^= x >> 32;
		return x;
	}
	void wort( std::uint64_t w ){
		a = rotl( ( a ^ w ) * 0x9E3779B97F4A7C15ULL, 29 );
		b = ( b + w ) * 0xC2B2AE3D27D4EB4FULL;
		b ^= b >> 31;
	}
	void daten( const void* p, std::size_t n ){
		const unsigned char* c = static_cast<const unsigned char*>( p );
		while ( n >= 8 ) {
			std::uint64_t w;
			memcpy( &w, c, 8 );
			wort( w );
			c += 8;
			n -= 8;
		}
		std::uint64_t rest = 0x80ULL << 56 | std::uint64_t( n ) << 48;   // the tail and its length
		for ( std::size_t i = 0; i < n; ++i )
			rest |= std::uint64_t( c[i] ) << ( 8 * i );
		wort( rest );
	}
	void text( const char* s ){
		const std::size_t n = s != nullptr ? strlen( s ) : 0;
		wert( n );
		if ( n != 0 )
			daten( s, n );
	}
	template<typename T> void wert( const T& v ){ daten( &v, sizeof( v ) ); }
	template<typename T> void feld( const std::vector<T>& v ){
		const std::size_t n = v.size();
		wert( n );
		if ( n != 0 )
			daten( v.data(), n * sizeof( T ) );
	}
	[[nodiscard]] BhedSig ende() const {
		BhedSig s{ mix( a ^ rotl( b, 17 ) ), mix( b + a * 3 ) };
		if ( !s.da() )
			s.a = 1;
		return s;
	}
};

// lcb2 (light_trace.cpp): where the rays of one raw lightmap can run - a box A (the samples and every
// light target that is a point) and, per sun, the samples swept by the sun's ray vector
struct BhedStrahlRaum {
	struct Komp {
		double lo[3], hi[3];
		double v[3];        // the sweep (0 for the box)
	};
	std::vector<Komp> komp;   // [0] = A (may be empty: lo > hi), then the suns
	bool himmel = false;      // a sun ray traces on through the portal sky (testAll)
};
// after SetupTraceNodes: a full hash per trace node (its subtree with planes, leaves, triangles and their infos) and a
// conservative box per node; flaeche/shader give the content ids of a surface number and a shader
void BhedSpurHashesAnlegen( BhedSig ( *flaeche )( int surfaceNum ), BhedSig ( *shader )( const shaderInfo_t* si ) );
// the part of the trace tree the rays of raum can meet, with every plane they are cut at and every leaf they can reach
[[nodiscard]] BhedSig BhedSpurRaumSig( const BhedStrahlRaum& raum );

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
// lcb2: IlluminateRawLightmap_ lit lightmap lmNum on the processor - kosten: the seconds of one thread it took,
// wand: the seconds from its start to its end (with the pool less than kosten); kept for the next estimate
void BhedLmZeit( int lmNum, double kosten, double wand );
// after BhedIlluminateRawLightmaps, before StitchSurfaceLightmaps: copy the raw lightmaps for the new cache
void BhedLmDatenMerken();
// IlluminateVertexes (vertex lit surfaces) after CreateTraceLightsForSurface: true = restored
[[nodiscard]] bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace, unsigned int seed );
// after IlluminateVertexes of a pass: write the pass into the new cache
void BhedLichtCacheDurchgangEnde();
// LightWorld before the final StoreSurfaceLightmaps (or when radiosity ends early): <map>.bhedlight, the summary
void BhedLichtCacheSchreiben();
