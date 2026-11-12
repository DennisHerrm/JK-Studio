// bhed_lichtcache.cpp - BehavEd stage 8a: -light -bhedlightcache (see bhed_lichtcache.h).
// lcb1: every pass on its own (also -bounce), the file holds one section per pass.
// lcb2: lightmaps are found by their content, also after changes to geometry, shaders and brush entities;
//       each pass is stored packed (byte planes, zlib) - lossless.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "q3map2.h"
#include "bhed_lichtcache.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <zlib.h>

namespace {

using Sig = BhedSig;
using Hash = BhedHash;

bool istNachbearbeitung( const char* s ){
	static const char* const kSchalter[] = { "-gamma", "-exposure", "-compensate", "-saturation", "-contrast", "-gridscale", "-gridambientscale" };
	for ( const char* k : kSchalter )
		if ( striEqual( s, k ) )
			return true;
	return false;
}

bool istNachbearbeitungsSchluessel( const char* k ){
	return striEqual( k, "_minlight" ) || striEqual( k, "_minvertexlight" ) || striEqual( k, "_mingridlight" ) || striEqual( k, "_maxlight" )
	    || striEqualPrefix( k, "_q3map2_" );
}

std::unordered_map<const shaderInfo_t*, Sig> g_shaderSigs;

/* a shader by content: name, script, the flags the light code reads and, for the shadow of alpha shadow and
   light filter surfaces, the image (TraceTriangle reads its pixels) */
Sig shaderSigRechnen( const shaderInfo_t* si ){
	Hash h;
	h.text( "shader" );
	if ( si == nullptr )
		return h.ende();
	h.text( si->shader.c_str() );
	h.text( si->shaderText );
	h.wert( si->surfaceFlags );
	h.wert( si->contentFlags );
	h.wert( si->compileFlags );
	h.wert( si->skyIndex );
	h.wert( si->twoSided );
	if ( si->lightImage != nullptr && si->lightImage->pixels != nullptr ) {
		h.wert( si->lightImage->width );
		h.wert( si->lightImage->height );
		h.daten( si->lightImage->pixels, std::size_t( si->lightImage->width ) * si->lightImage->height * 4 );
	}
	return h.ende();
}

Sig shaderSig( const shaderInfo_t* si ){
	const auto it = g_shaderSigs.find( si );
	return it != g_shaderSigs.end() ? it->second : shaderSigRechnen( si );
}

/* every field of a light that LightContributionToSample/Point reads except the cluster (lcb2: a number of the
   BSP - the fingerprint of a lightmap carries the clusters as they relate) */
Sig lichtHashOhneCluster( const light_t& l ){
	Hash h;
	h.wert( l.type );
	h.wert( l.flags );
	h.wert( shaderSig( l.si ) );
	h.wert( l.origin );
	h.wert( l.normal );
	h.wert( l.dist );
	h.wert( l.photons );
	h.wert( l.style );
	h.wert( l.color );
	h.wert( l.radiusByDist );
	h.wert( l.fade );
	h.wert( l.angleScale );
	h.wert( l.extraDist );
	h.wert( l.add );
	h.wert( l.envelope );
	h.wert( l.envelope2 );
	h.wert( l.minmax );
	h.feld( l.w );
	h.wert( l.falloffTolerance );
	h.wert( l.filterRadius );
	h.wert( l.skyIndex );
	return h.ende();
}

Sig lichtHash( const light_t& l ){   // with the cluster number (vertex lit surfaces and the grid: same geometry only)
	Hash h;
	h.wert( lichtHashOhneCluster( l ) );
	h.wert( l.cluster );
	return h.ende();
}

// --- state ---------------------------------------------------------------------------------------

bool g_an = false;
std::vector<std::string> g_optionen;   // light switches without the post-processing ones
std::string g_bspName;
std::string g_cachePfad;
std::string g_neuPfad;                 // written while the run goes, renamed at the end
Sig g_opt;                             // the light switches
Sig g_welt;                            // worldspawn without the post-processing keys
Sig g_geo;                             // the geometry (lumps, .srf, shader scripts, brush entities)
bool g_geoGleich = false;              // ... the same as in the cache
int g_paesse = 1;                      // first pass and the bounces

// the old cache: its passes in the file, the loaded one parsed
FILE* g_altDatei = nullptr;
struct PassOrt {
	long long pos = 0;
	std::uint64_t laenge = 0;
};
std::unordered_map<int, PassOrt> g_altPaesse;
bool g_cacheGilt = false;
struct Alt {
	Sig sig;
	std::size_t pos = 0;   // offset of the data in g_alt
};
std::string g_alt;                     // the payload of the cached pass g_pass
bool g_altDa = false;
Sig g_altGitterSig;
std::size_t g_altGitterPos = 0;
std::size_t g_altGitterAnzahl = 0;
struct SigHasher {
	std::size_t operator()( const Sig& s ) const { return std::size_t( s.a ^ ( s.b * 0x9E3779B97F4A7C15ULL ) ); }
};
std::unordered_map<Sig, std::size_t, SigHasher> g_altLm;   // fingerprint -> offset of its data (lcb2: by content)
struct LmZeit {
	float kosten = -1.f;   // seconds of one thread (-1: unknown)
	float wand = -1.f;     // seconds from its start to its end, however many threads lit it
};
std::unordered_map<Sig, LmZeit, SigHasher> g_altZeit;      // lcb2: content id (LmFest::sig) -> what the cached run took for it
std::vector<Alt> g_altFlaeche;

// this run
FILE* g_neu = nullptr;
int g_pass = -1;
Sig g_gitterSig;
std::string g_gitterDaten;
bool g_gitterDa = false;
bool g_gitterAusCache = false;
std::vector<Sig> g_lmSig;
std::vector<Sig> g_flaecheSig;
std::string g_lmDaten;                 // lightmap part of the pass, made before StitchSurfaceLightmaps
std::vector<LmZeit> g_lmZeit;          // lcb2: what the processor took per raw lightmap in this pass (-1: not lit by it)
std::vector<unsigned long long> g_lmArbeit;   // luxels x lights
std::vector<char> g_lmTreffer;         // taken from the cache
bool g_lmDatenDa = false;
std::unordered_map<const light_t*, Sig> g_lichtHash;          // with the cluster
std::unordered_map<const light_t*, Sig> g_lichtHashOC;        // without (lcb2)
// lcb2: content ids and the fixed part of every raw lightmap's fingerprint (made in the first pass)
std::vector<Sig> g_flaecheId;          // per draw surface: its content, unique (equal ones get their rank)
struct LmFest {
	Sig sig;                           // all of the lightmap that is the same in every pass
	std::vector<int> roh;              // its clusters in order of appearance (canonical id = index)
	std::vector<std::pair<int, int>> nachId;   // cluster -> id, sorted
	double box[6] = { 1, 1, 1, 0, 0, 0 };      // where its samples can be (empty: lo > hi)
	bool da = false;
};
std::vector<LmFest> g_fest;
std::mutex g_sperre;
std::atomic<int> g_lmAus{ 0 };
std::atomic<int> g_flaecheAus{ 0 };
// all passes
int g_summeLmAus = 0, g_summeLmAlle = 0, g_summeFlAus = 0, g_summeFlAlle = 0;
int g_gitterPaesse = 0, g_gitterPaesseAus = 0;
double g_geschriebenMB = 0.0;

Sig listenSigRoh( const trace_t& trace ){
	Hash h;
	h.wert( trace.numLights );
	for ( int i = 0; i < trace.numLights; ++i ) {
		const auto it = g_lichtHash.find( trace.lights[i] );
		const Sig v = it != g_lichtHash.end() ? it->second : lichtHash( *trace.lights[i] );
		h.wert( v );
	}
	h.wert( trace.numSurfaces );
	if ( trace.numSurfaces > 0 && trace.surfaces != nullptr )
		h.daten( trace.surfaces, sizeof( int ) * trace.numSurfaces );
	h.wert( trace.recvShadows );
	h.wert( trace.twoSided );
	h.wert( trace.testOcclusion );
	h.wert( trace.forceSunlight );
	return h.ende();
}

bool debugFuellung(){
	return debugSurfaces || debugAxis || debugCluster || debugOrigin || dirtDebug || normalmap;
}

/* what IlluminateRawLightmap reads of a raw lightmap's own state at the beginning of a pass (all else of
   the lightmap is fixed by the geometry key): the styles (style check), the ambient (zero in a bounce),
   of the style lightmap 0 the luxels that are not mapped (a mapped one gets ambient and count 1 first;
   without the debug fills the style lightmaps 1-3 are cleared, only whether they exist counts), the flags,
   the clusters (the filter pass floods unmapped luxels), the deluxels. Stitching writes into neighbouring
   lightmaps' luxels between the passes - the mapped ones are overwritten, a changed unmapped one counts. */
int kanonId( const LmFest& f, int c ){
	const auto it = std::lower_bound( f.nachId.begin(), f.nachId.end(), std::pair<int, int>( c, -1 ) );
	return it != f.nachId.end() && it->first == c ? it->second : -1000000;
}

void lmZustand( Hash& h, const rawLightmap_t* lm, const LmFest& f ){
	const std::size_t n = std::size_t( lm->sw ) * lm->sh;
	const bool debug = debugFuellung();
	h.wert( lm->styles );
	h.wert( lm->ambientColor );
	unsigned char maske = 0;
	for ( int l = 0; l < MAX_LIGHTMAPS; ++l )
		if ( lm->superLuxels[l] != nullptr )
			maske |= 1 << l;
	if ( lm->superFlags != nullptr ) maske |= 16;
	if ( lm->superDeluxels != nullptr ) maske |= 32;
	h.wert( maske );
	if ( lm->superLuxels[0] != nullptr ) {
		if ( debug )
			h.daten( lm->superLuxels[0], n * sizeof( SuperLuxel ) );
		else
			for ( std::size_t i = 0; i < n; ++i )
				if ( lm->superClusters[i] < CLUSTER_NORMAL ) {
					h.wert( i );
					h.wert( lm->superLuxels[0][i] );
				}
	}
	if ( debug )
		for ( int l = 1; l < MAX_LIGHTMAPS; ++l )
			if ( lm->superLuxels[l] != nullptr )
				h.daten( lm->superLuxels[l], n * sizeof( SuperLuxel ) );
	if ( lm->superFlags != nullptr )
		h.daten( lm->superFlags, n );
	/* lcb2: a cluster >= 0 as its canonical id (the numbers are the BSP's), the flags < 0 as they are */
	for ( std::size_t i = 0; i < n; ++i ) {
		const int c = lm->superClusters[i];
		h.wert( c < 0 ? c : kanonId( f, c ) );
	}
	if ( debug )   // -debugcluster colours by the number
		h.daten( lm->superClusters, n * sizeof( int ) );
	if ( lm->superDeluxels != nullptr )
		h.daten( lm->superDeluxels, n * sizeof( Vector3 ) );
}


// --- lcb2: content ids, the fixed part of a raw lightmap ----------------------------------------------

/* a draw surface by content: shader, fog, type, vertices, indexes, the plane of a planar one, its surface info
   (from the .srf and the shader: shadows, sample size, axis, patch iterations, portal sky); equal ones get
   their rank, so the ids are unique within a run (TraceTriangle compares surface numbers for equality) */
Sig flaecheInhalt( std::size_t i ){
	const bspDrawSurface_t& ds = bspDrawSurfaces[i];
	const surfaceInfo_t& info = surfaceInfos[i];
	Hash h;
	h.text( "flaeche" );
	if ( ds.shaderNum >= 0 && ds.shaderNum < int( bspShaders.size() ) ) {
		h.text( bspShaders[ds.shaderNum].shader );
		h.wert( bspShaders[ds.shaderNum].surfaceFlags );
		h.wert( bspShaders[ds.shaderNum].contentFlags );
	}
	h.wert( ds.fogNum );
	h.wert( ds.surfaceType );
	h.wert( ds.numVerts );
	h.wert( ds.numIndexes );
	h.wert( ds.lightmapVecs[2] );
	h.wert( ds.patchWidth );
	h.wert( ds.patchHeight );
	for ( int v = 0; v < ds.numVerts; ++v ) {
		const std::size_t k = std::size_t( ds.firstVert ) + v;
		if ( k < bspDrawVerts.size() ) {
			h.wert( bspDrawVerts[k].xyz );
			h.wert( bspDrawVerts[k].st );
			h.wert( bspDrawVerts[k].normal );
		}
	}
	if ( ds.numIndexes > 0 && ds.firstIndex >= 0 && std::size_t( ds.firstIndex ) + ds.numIndexes <= bspDrawIndexes.size() )
		h.daten( &bspDrawIndexes[ds.firstIndex], std::size_t( ds.numIndexes ) * sizeof( bspDrawIndexes[0] ) );
	h.wert( shaderSig( info.si ) );
	h.wert( info.parentSurfaceNum >= 0 );
	h.wert( info.childSurfaceNum < 0 );
	h.wert( info.castShadows );
	h.wert( info.recvShadows );
	h.wert( info.sampleSize );
	h.wert( info.patchIterations );
	h.wert( info.ambientColor );
	h.wert( info.longestCurve );
	h.wert( info.axis );
	h.wert( info.minmax );
	h.wert( info.hasLightmap );
	h.wert( info.approximated );
	if ( info.plane != nullptr )
		h.wert( *info.plane );
	return h.ende();
}

// a surface number as TraceTriangle compares it with the lightmap's surfaces (out of range: none of them)
Sig flaecheVonNummer( int n ){
	if ( n >= 0 && n < int( g_flaecheId.size() ) )
		return g_flaecheId[n];
	Hash h;
	h.text( "keine flaeche" );
	return h.ende();
}

Sig shaderVonZeiger( const shaderInfo_t* si ){ return shaderSig( si ); }

// once per run, in the first pass (after SetupSurfaceLightmaps, SetupTraceNodes, MapRawLightmap)
void inhaltAnlegen(){
	g_shaderSigs.clear();
	for ( const shaderInfo_t& si : shaderInfo )
		g_shaderSigs.emplace( &si, shaderSigRechnen( &si ) );
	const std::size_t n = bspDrawSurfaces.size();
	std::vector<Sig> roh( n );
	for ( std::size_t i = 0; i < n; ++i )
		roh[i] = flaecheInhalt( i );
	std::unordered_map<Sig, int, SigHasher> rang;
	g_flaecheId.assign( n, Sig{} );
	for ( std::size_t i = 0; i < n; ++i ) {
		Hash h;
		h.wert( roh[i] );
		h.wert( rang[roh[i]]++ );
		g_flaecheId[i] = h.ende();
	}
	BhedSpurHashesAnlegen( flaecheVonNummer, shaderVonZeiger );
	g_fest.assign( numRawLightmaps, LmFest{} );
	BhedZeit( "Licht-Cache: Inhalt, Spurbaum" );
}

constexpr double kRand = 1.0;   // > the float rounding of a sample point and of the BSP plane distance

/* the BSP below node as far as a point query (PointInLeafNum_r: front above 0.1, back below -0.1, both in
   between; ClusterForPointExt: the leaf's cluster and its opaque brushes in order) inside box can see it */
template<typename F> void bspWeg( Hash& h, int node, const double* box, F& kl ){
	while ( true ) {
		if ( node < 0 ) {
			const std::size_t leafNum = std::size_t( -node - 1 );
			h.wert( 'L' );
			if ( leafNum >= bspLeafs.size() ) {
				h.wert( leafNum );
				return;
			}
			const bspLeaf_t& leaf = bspLeafs[leafNum];
			kl( h, leaf.cluster );
			if ( leaf.cluster > CLUSTER_OPAQUE )
				for ( int k = 0; k < leaf.numBSPLeafBrushes; ++k ) {
					const int b = bspLeafBrushes[std::size_t( leaf.firstBSPLeafBrush ) + k];
					if ( b > maxOpaqueBrush || b < 0 || std::size_t( b ) >= opaqueBrushes.size() || !opaqueBrushes[b] )
						continue;
					const bspBrush_t& brush = bspBrushes[b];
					h.wert( brush.numSides );
					for ( int j = 0; j < brush.numSides; ++j )
						h.wert( bspPlanes[bspBrushSides[std::size_t( brush.firstSide ) + j].planeNum] );
				}
			h.wert( 'l' );
			return;
		}
		const bspNode_t& nd = bspNodes[node];
		const bspPlane_t& pl = bspPlanes[nd.planeNum];
		double mn = -double( pl.dist() ), mx = -double( pl.dist() );
		for ( int a = 0; a < 3; ++a ) {
			const double nv = pl.normal()[a];
			mn += nv * ( nv >= 0 ? box[a] : box[3 + a] );
			mx += nv * ( nv >= 0 ? box[3 + a] : box[a] );
		}
		if ( mn - kRand > 0.1 ) {
			node = nd.children[0];
			continue;
		}
		if ( mx + kRand < -0.1 ) {
			node = nd.children[1];
			continue;
		}
		h.wert( 'X' );
		h.wert( pl );
		bspWeg( h, nd.children[0], box, kl );
		node = nd.children[1];
	}
}

/* the fixed part of raw lightmap r (the same in every pass; made in the first pass before it lights anything):
   every field and array IlluminateRawLightmap reads of it (also the luxel origins, normals, dirt and floodlight
   that MapRawLightmap, DirtyRawLightmap and FloodlightRawLightmaps made), its surfaces by content, its clusters
   as they relate (canonical ids in order of appearance, cluster 0 marked - StoreSurfaceLightmaps treats it
   apart), the box its samples can be in (SubmapRawLuxel moves a sample by at most 2.74 * bias times the
   distance to a mapped neighbour per axis: random subsamples, bias 0.5 * lightSamplesSearchBoxSize; the
   recursive ones at most 2 * 0.25 * that), the BSP a point query in that box can see, and the PVS among its
   clusters. */
void festRechnen( int r ){
	const rawLightmap_t* lm = &rawLightmaps[r];
	LmFest& f = g_fest[r];
	const int sw = lm->sw, sh = lm->sh;
	const std::size_t n = std::size_t( sw ) * sh;
	std::unordered_map<int, int> ids;
	auto kl = [&]( Hash& h, int c ){
		if ( c < 0 ) {
			h.wert( 'n' );
			h.wert( c );
			return;
		}
		const auto [it, neu] = ids.emplace( c, int( f.roh.size() ) );
		if ( neu )
			f.roh.push_back( c );
		h.wert( 'p' );
		h.wert( it->second );
		h.wert( c == 0 );
	};
	Hash h;
	h.text( "lmfest" );
	if ( debugSurfaces )   // -debugsurfaces colours by the number
		h.wert( r );
	h.wert( lm->splotchFix );
	h.wert( lm->wrap );
	h.wert( lm->customWidth );
	h.wert( lm->customHeight );
	h.wert( lm->brightness );
	h.wert( lm->filterRadius );
	h.wert( lm->numLightSurfaces );
	h.wert( lm->numLightClusters );
	h.wert( lm->sampleSize );
	h.wert( lm->actualSampleSize );
	h.wert( lm->axisNum );
	h.wert( lm->ambientColor );
	h.wert( lm->recvShadows );
	h.wert( lm->minmax );
	h.wert( lm->axis );
	h.wert( lm->origin );
	h.wert( lm->vecs != nullptr );
	if ( lm->vecs != nullptr )
		h.daten( lm->vecs, 3 * sizeof( Vector3 ) );
	h.wert( lm->plane != nullptr );
	if ( lm->plane != nullptr )
		h.wert( *lm->plane );
	h.wert( lm->w );
	h.wert( lm->h );
	h.wert( sw );
	h.wert( sh );
	for ( int k = 0; k < lm->numLightSurfaces; ++k )
		h.wert( flaecheVonNummer( lightSurfaces[std::size_t( lm->firstLightSurface ) + k] ) );
	unsigned char maske = 0;
	if ( lm->superOrigins != nullptr ) maske |= 1;
	if ( lm->superNormals != nullptr ) maske |= 2;
	if ( lm->superDirt != nullptr ) maske |= 4;
	if ( lm->superFloodLight != nullptr ) maske |= 8;
	h.wert( maske );
	if ( lm->superOrigins != nullptr )
		h.daten( lm->superOrigins, n * sizeof( Vector3 ) );
	if ( lm->superNormals != nullptr )
		h.daten( lm->superNormals, n * sizeof( Vector3 ) );
	if ( lm->superDirt != nullptr )
		h.daten( lm->superDirt, n * sizeof( float ) );
	if ( lm->superFloodLight != nullptr )
		h.daten( lm->superFloodLight, n * sizeof( SuperFloodLight ) );
	for ( std::size_t i = 0; i < n; ++i )
		kl( h, lm->superClusters[i] );
	for ( int k = 0; k < lm->numLightClusters; ++k )
		kl( h, lm->lightClusters[k] );
	// where the samples can be
	double* b = f.box;
	b[0] = b[1] = b[2] = 1e30;
	b[3] = b[4] = b[5] = -1e30;
	double d0[3] = { 0, 0, 0 }, d1[3] = { 0, 0, 0 };
	if ( lm->superOrigins != nullptr )
		for ( int y = 0; y < sh; ++y )
			for ( int x = 0; x < sw; ++x ) {
				const std::size_t i = std::size_t( y ) * sw + x;
				if ( lm->superClusters[i] < CLUSTER_NORMAL )
					continue;
				const Vector3& o = lm->superOrigins[i];
				for ( int a = 0; a < 3; ++a ) {
					b[a] = std::min( b[a], double( o[a] ) );
					b[3 + a] = std::max( b[3 + a], double( o[a] ) );
					if ( x + 1 < sw && lm->superClusters[i + 1] >= CLUSTER_NORMAL )
						d0[a] = std::max( d0[a], std::fabs( double( lm->superOrigins[i + 1][a] ) - o[a] ) );
					if ( y + 1 < sh && lm->superClusters[i + sw] >= CLUSTER_NORMAL )
						d1[a] = std::max( d1[a], std::fabs( double( lm->superOrigins[i + sw][a] ) - o[a] ) );
				}
			}
	if ( b[0] <= b[3] ) {
		const double faktor = 3.0 * std::max( 1, lightSamplesSearchBoxSize );   // > 2.74 * 0.5 * size
		for ( int a = 0; a < 3; ++a ) {
			const double e = faktor * ( d0[a] + d1[a] ) + kRand;
			b[a] -= e;
			b[3 + a] += e;
		}
		h.daten( b, 6 * sizeof( double ) );
		bspWeg( h, 0, b, kl );
	}
	// the PVS among its clusters (ClusterForPointExtFilter, the light list)
	for ( const int ca : f.roh )
		for ( const int cb : f.roh )
			h.wert( ClusterVisible( ca, cb ) );
	f.sig = h.ende();
	f.nachId.clear();
	for ( std::size_t i = 0; i < f.roh.size(); ++i )
		f.nachId.emplace_back( f.roh[i], int( i ) );
	std::sort( f.nachId.begin(), f.nachId.end() );
	f.da = true;
}

template<typename T> void anhaengen( std::string& s, const T& v ){ s.append( reinterpret_cast<const char*>( &v ), sizeof( v ) ); }
template<typename T> bool lesen( std::size_t& pos, T& v ){
	if ( pos + sizeof( T ) > g_alt.size() )
		return false;
	memcpy( &v, g_alt.data() + pos, sizeof( T ) );
	pos += sizeof( T );
	return true;
}

std::size_t lmGroesse( std::size_t pos ){   // bytes of one lightmap record after the signature
	std::uint32_t sw = 0, sh = 0;
	unsigned char maske = 0;
	std::size_t p = pos + 4;   // styles
	if ( !lesen( p, sw ) || !lesen( p, sh ) || !lesen( p, maske ) )
		return 0;
	std::size_t n = std::size_t( sw ) * sh;
	std::size_t g = 4 + 8 + 1;
	for ( int l = 0; l < MAX_LIGHTMAPS; ++l )
		if ( maske & ( 1 << l ) )
			g += n * sizeof( SuperLuxel );
	if ( maske & 16 ) g += n * sizeof( Vector3 );
	if ( maske & 32 ) g += n;
	if ( maske & 64 ) g += n * sizeof( int );
	return g;
}

// the payload of one pass: grid, lightmaps, vertex lit surfaces
bool passZerlegen(){
	std::size_t pos = 0;
	unsigned char gitterDa = 0;
	if ( !lesen( pos, gitterDa ) )
		return false;
	g_altGitterSig = Sig{};
	g_altGitterAnzahl = 0;
	if ( gitterDa ) {
		std::uint32_t gp = 0;
		if ( !lesen( pos, g_altGitterSig ) || !lesen( pos, gp ) )
			return false;
		g_altGitterAnzahl = gp;
		g_altGitterPos = pos;
		pos += std::size_t( gp ) * ( sizeof( rawGridPoint_t ) + 4 );
		if ( pos > g_alt.size() )
			return false;
	}
	std::uint32_t nlm = 0;
	if ( !lesen( pos, nlm ) )
		return false;
	g_altLm.clear();
	g_altLm.reserve( nlm );
	for ( std::uint32_t i = 0; i < nlm; ++i ) {
		Sig sig;
		if ( !lesen( pos, sig ) )
			return false;
		if ( !sig.da() )
			continue;
		g_altLm.emplace( sig, pos );
		const std::size_t g = lmGroesse( pos );
		if ( g == 0 || pos + g > g_alt.size() )
			return false;
		pos += g;
	}
	std::uint32_t nf = 0;
	if ( !lesen( pos, nf ) )
		return false;
	g_altFlaeche.assign( nf, Alt{} );
	for ( std::uint32_t i = 0; i < nf; ++i ) {
		if ( !lesen( pos, g_altFlaeche[i].sig ) )
			return false;
		if ( !g_altFlaeche[i].sig.da() )
			continue;
		g_altFlaeche[i].pos = pos;
		std::uint32_t nv = 0;
		std::size_t p = pos + 4;
		if ( !lesen( p, nv ) )
			return false;
		const std::size_t g = 4 + 4 + std::size_t( nv ) * MAX_LIGHTMAPS * 2 * sizeof( Vector3 );
		if ( pos + g > g_alt.size() )
			return false;
		pos += g;
	}
	std::uint32_t nz = 0;   // lcb2: the times
	if ( !lesen( pos, nz ) )
		return false;
	for ( std::uint32_t i = 0; i < nz; ++i ) {
		Sig fs;
		LmZeit z;
		if ( !lesen( pos, fs ) || !lesen( pos, z.kosten ) || !lesen( pos, z.wand ) )
			return false;
		if ( fs.da() && z.kosten >= 0 && z.wand >= 0 )
			g_altZeit.emplace( fs, z );
	}
	return pos == g_alt.size();
}

/* lcb2: a pass is stored packed, lossless: the bytes of each 4 byte word (floats, ints) are sorted into four planes
   (the sign and exponent bytes repeat, the low mantissa bytes do not), then deflated in blocks of 4 MB on all
   threads - each block on its own, so the bytes do not depend on the threads. Blob: u64 payload length, u32
   blocks, u32 packed length per block, the blocks.
   Measured on duel_mos_espa ([Final] Fast, 9 passes, 65 000 raw lightmaps): 786 MB unpacked, 152 MB packed.
   Storing a pass as its XOR with the pass before was tried and made it bigger (199 MB): a bounce does not
   reach the same luxels as the one before, and where one of the two is zero the XOR is the other one. */
constexpr std::size_t kBlock = std::size_t( 4 ) << 20;

template<typename F> void aufAllen( std::size_t n, const F& f ){
	const std::size_t t = std::min<std::size_t>( n, std::max( 1, numthreads ) );
	std::atomic<std::size_t> naechster{ 0 };
	const auto arbeit = [&]{
		for ( std::size_t k; ( k = naechster++ ) < n; )
			f( k );
	};
	std::vector<std::thread> faeden;
	for ( std::size_t i = 1; i < t; ++i )
		faeden.emplace_back( arbeit );
	arbeit();
	for ( std::thread& th : faeden )
		th.join();
}

// the four byte planes of a block (a tail of 1-3 bytes stays where it is) and back
void ebenen( const unsigned char* von, unsigned char* nach, std::size_t l, bool hin ){
	const std::size_t w = l / 4;
	for ( std::size_t i = 0; i < w; ++i )
		for ( std::size_t e = 0; e < 4; ++e ) {
			if ( hin )
				nach[e * w + i] = von[i * 4 + e];
			else
				nach[i * 4 + e] = von[e * w + i];
		}
	for ( std::size_t i = w * 4; i < l; ++i )
		nach[i] = von[i];
}

bool packen( const std::string& roh, std::string& blob ){
	const std::size_t n = roh.size();
	const std::size_t bloecke = ( n + kBlock - 1 ) / kBlock;
	std::vector<std::string> teile( bloecke );
	std::atomic<bool> ok{ true };
	aufAllen( bloecke, [&]( std::size_t k ){
		const std::size_t a = k * kBlock, l = std::min( kBlock, n - a );
		std::string e( l, '\0' );
		ebenen( reinterpret_cast<const unsigned char*>( roh.data() ) + a, reinterpret_cast<unsigned char*>( e.data() ), l, true );
		uLongf cl = compressBound( uLong( l ) );
		teile[k].resize( cl );
		if ( compress2( reinterpret_cast<Bytef*>( teile[k].data() ), &cl, reinterpret_cast<const Bytef*>( e.data() ), uLong( l ), 1 ) != Z_OK )
			ok = false;
		teile[k].resize( cl );
	} );
	if ( !ok )
		return false;
	blob.clear();
	anhaengen( blob, std::uint64_t( n ) );
	anhaengen( blob, std::uint32_t( bloecke ) );
	for ( const std::string& t : teile )
		anhaengen( blob, std::uint32_t( t.size() ) );
	for ( const std::string& t : teile )
		blob += t;
	return true;
}

bool entpacken( const std::string& blob, std::string& roh ){
	std::size_t pos = 0;
	const auto nimm = [&]( auto& v ){
		if ( pos + sizeof( v ) > blob.size() )
			return false;
		memcpy( &v, blob.data() + pos, sizeof( v ) );
		pos += sizeof( v );
		return true;
	};
	std::uint64_t n = 0;
	std::uint32_t bloecke = 0;
	if ( !nimm( n ) || !nimm( bloecke ) || bloecke != ( n + kBlock - 1 ) / kBlock )
		return false;
	if ( n / 1100 > blob.size() )   // deflate packs at most 1032 : 1 - a damaged length must not allocate
		return false;
	std::vector<std::size_t> start( std::size_t( bloecke ) + 1 );
	start[0] = pos + std::size_t( bloecke ) * 4;
	for ( std::uint32_t k = 0; k < bloecke; ++k ) {
		std::uint32_t cl = 0;
		if ( !nimm( cl ) )
			return false;
		start[k + 1] = start[k] + cl;
	}
	if ( start[bloecke] != blob.size() )
		return false;
	roh.assign( std::size_t( n ), '\0' );
	std::atomic<bool> ok{ true };
	aufAllen( bloecke, [&]( std::size_t k ){
		const std::size_t a = k * kBlock, l = std::min<std::size_t>( kBlock, std::size_t( n ) - a );
		std::string e( l, '\0' );
		uLongf dl = uLong( l );
		if ( uncompress( reinterpret_cast<Bytef*>( e.data() ), &dl, reinterpret_cast<const Bytef*>( blob.data() + start[k] ), uLong( start[k + 1] - start[k] ) ) != Z_OK ||
		     dl != l ) {
			ok = false;
			return;
		}
		ebenen( reinterpret_cast<const unsigned char*>( e.data() ), reinterpret_cast<unsigned char*>( roh.data() ) + a, l, false );
	} );
	return ok;
}

void altVergessen(){
	g_alt.clear();
	g_alt.shrink_to_fit();
	g_altDa = false;
	g_altLm.clear();
	g_altZeit.clear();
	g_altFlaeche.clear();
	g_altGitterSig = Sig{};
	g_altGitterAnzahl = 0;
}

// 3: one section per pass (lcb1), 4: by content, 5: packed (lcb2), 6: the hash keeps the last byte of a 7 byte tail (lcfix P4-2)
constexpr char kKopf[8] = { 'B', 'H', 'L', 'I', 'C', 'H', 'T', '6' };

}  // namespace

void BhedLichtCacheSchalter( bool an, const std::vector<const char*>& args, const char* bspName ){
	g_an = an;
	g_optionen.clear();
	for ( std::size_t i = 0; i < args.size(); ++i ) {
		if ( istNachbearbeitung( args[i] ) ) {
			++i;   // and its value
			continue;
		}
		g_optionen.emplace_back( args[i] );
	}
	g_bspName = bspName;
}

bool BhedLichtCacheAn(){ return g_an; }

std::uint64_t BhedLichtCacheHash( const void* daten, std::size_t n, std::uint64_t start ){
	Hash h;
	h.wert( start );
	h.daten( daten, n );
	return h.ende().a;
}

void BhedLichtCacheGeometrie(){
	if ( !g_an )
		return;
	g_paesse = bounce + 1;
	{
		Hash h;
		h.text( "bhedlicht4" );
		h.text( g_game->arg );
		for ( const std::string& o : g_optionen )
			h.text( o.c_str() );
		g_opt = h.ende();
	}
	{   // lcb2: worldspawn on its own (ambient, colour, scales ... reach every lightmap)
		Hash w;
		if ( !entities.empty() )
			for ( const auto& ep : entities[0].epairs ) {
				if ( istNachbearbeitungsSchluessel( ep.key.c_str() ) )
					continue;
				w.text( ep.key.c_str() );
				w.text( ep.value.c_str() );
			}
		g_welt = w.ende();
	}
	Hash h;
	h.feld( bspShaders );
	h.feld( bspPlanes );
	h.feld( bspNodes );
	h.feld( bspLeafs );
	h.feld( bspLeafSurfaces );
	h.feld( bspLeafBrushes );
	h.feld( bspModels );
	h.feld( bspBrushes );
	h.feld( bspBrushSides );
	h.feld( bspVisBytes );
	h.feld( bspFogs );
	{
		const std::size_t n = bspDrawIndexes.size();
		h.wert( n );
		if ( n != 0 )
			h.daten( bspDrawIndexes.data(), n * sizeof( bspDrawIndexes[0] ) );
	}
	h.wert( bspDrawVerts.size() );
	for ( const bspDrawVert_t& v : bspDrawVerts ) {   // without lightmap coordinates and colours (written by -light)
		h.wert( v.xyz );
		h.wert( v.st );
		h.wert( v.normal );
	}
	h.wert( bspDrawSurfaces.size() );
	for ( const bspDrawSurface_t& s : bspDrawSurfaces ) {   // without the lightmap fields (written by -light)
		h.wert( s.shaderNum );
		h.wert( s.fogNum );
		h.wert( s.surfaceType );
		h.wert( s.firstVert );
		h.wert( s.numVerts );
		h.wert( s.firstIndex );
		h.wert( s.numIndexes );
		/* lightmapOrigin and lightmapVecs[0..1] are written by -light (StoreSurfaceLightmaps) and not read;
		   lightmapVecs[2] is the plane of a planar surface (SetupSurfaceLightmaps reads it) */
		h.wert( s.lightmapVecs[2] );
		h.wert( s.patchWidth );
		h.wert( s.patchHeight );
	}
	// .srf (sample sizes, shadows per surface)
	{
		char srf[1024];
		strcpy( srf, g_bspName.c_str() );
		bhed_set_map_extension( srf, ".srf" );
		std::string d;
		if ( FILE* f = fopen( srf, "rb" ) ) {
			char buf[65536];
			std::size_t n;
			while ( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
				d.append( buf, n );
			fclose( f );
		}
		h.text( d.c_str() );
	}
	// every shader script (surface lights, shadows, sample sizes ...)
	for ( const shaderInfo_t& si : shaderInfo ) {
		h.text( si.shader.c_str() );
		h.text( si.shaderText );
	}
	// worldspawn (without the post-processing keys) and the brush entities (origins move their surfaces)
	for ( std::size_t i = 0; i < entities.size(); ++i ) {
		const entity_t& e = entities[i];
		const char* model = "";
		const bool brushModel = e.read_keyvalue( model, "model" ) && model[0] == '*';
		if ( i != 0 && ( !brushModel || e.classname_prefixed( "light" ) ) )
			continue;
		h.wert( i );
		for ( const auto& ep : e.epairs ) {
			if ( i == 0 && istNachbearbeitungsSchluessel( ep.key.c_str() ) )
				continue;
			h.text( ep.key.c_str() );
			h.text( ep.value.c_str() );
		}
	}
	g_geo = h.ende();
	// the old cache: its header and where its passes are (each is loaded when its pass begins)
	char pfad[1024];
	strcpy( pfad, g_bspName.c_str() );
	bhed_set_map_extension( pfad, ".bhedlight" );
	g_cachePfad = pfad;
	g_neuPfad = g_cachePfad + ".neu";
	g_altPaesse.clear();
	altVergessen();
	g_cacheGilt = false;
	g_geoGleich = false;
	const char* grund = "no cache yet";
	g_altDatei = fopen( pfad, "rb" );
	if ( g_altDatei != nullptr ) {
		char kopf[8] = {};
		Sig opt, welt, geo;
		if ( fread( kopf, 1, 8, g_altDatei ) != 8 || memcmp( kopf, kKopf, 8 ) != 0 ) {
			grund = "older cache format";
		}
		else if ( fread( &opt, sizeof( opt ), 1, g_altDatei ) != 1 || fread( &welt, sizeof( welt ), 1, g_altDatei ) != 1 ||
		          fread( &geo, sizeof( geo ), 1, g_altDatei ) != 1 ) {
			grund = "cache file damaged";
		}
		else if ( !( opt == g_opt ) ) {
			grund = "light switches changed";
		}
		else if ( !( welt == g_welt ) ) {
			grund = "worldspawn keys changed";
		}
		else {
			/* lcb2: geometry, shaders or brush entities changed - the lightmaps are found by their content */
			g_cacheGilt = true;
			g_geoGleich = geo == g_geo;
			/* a section that the file does not hold to its end is not kept: the cache of a light step that was stopped
			   (the map editor makes it from <map>.bhedlight.neu, see BhedLichtCacheDurchgangEnde) ends in the pass
			   that was being written */
			const long long anfang = _ftelli64( g_altDatei );
			_fseeki64( g_altDatei, 0, SEEK_END );
			const long long ende = _ftelli64( g_altDatei );
			_fseeki64( g_altDatei, anfang, SEEK_SET );
			while ( true ) {
				std::int32_t b = 0;
				std::uint64_t laenge = 0;
				if ( fread( &b, sizeof( b ), 1, g_altDatei ) != 1 || fread( &laenge, sizeof( laenge ), 1, g_altDatei ) != 1 )
					break;
				const long long pos = _ftelli64( g_altDatei );
				if ( pos < 0 || pos > ende || laenge > static_cast<std::uint64_t>( ende - pos ) )
					break;
				g_altPaesse[b] = PassOrt{ pos, laenge };
				if ( _fseeki64( g_altDatei, static_cast<long long>( laenge ), SEEK_CUR ) != 0 )
					break;
			}
		}
		if ( !g_cacheGilt ) {
			fclose( g_altDatei );
			g_altDatei = nullptr;
		}
	}
	if ( g_cacheGilt && g_geoGleich )
		Sys_Printf( "BehavEd: light cache found (geometry and switches unchanged, %d of %d passes kept)\n", int( g_altPaesse.size() ), g_paesse );
	else if ( g_cacheGilt )
		Sys_Printf( "BehavEd: light cache found (geometry, shaders or brush entities changed: lightmaps are found by content, "
		            "vertex lit surfaces and the grid are traced; %d of %d passes kept)\n", int( g_altPaesse.size() ), g_paesse );
	else
		Sys_Printf( "BehavEd: light cache not usable: %s - everything is traced\n", grund );
	// the new cache, written pass by pass
	g_neu = fopen( g_neuPfad.c_str(), "wb" );
	if ( g_neu != nullptr ) {
		fwrite( kKopf, 1, 8, g_neu );
		fwrite( &g_opt, sizeof( g_opt ), 1, g_neu );
		fwrite( &g_welt, sizeof( g_welt ), 1, g_neu );
		fwrite( &g_geo, sizeof( g_geo ), 1, g_neu );
	}
	g_summeLmAus = g_summeLmAlle = g_summeFlAus = g_summeFlAlle = 0;
	g_gitterPaesse = g_gitterPaesseAus = 0;
	g_geschriebenMB = 0.0;
	g_pass = -1;
}

void BhedLichtCacheDurchgang( int b ){
	if ( !g_an )
		return;
	g_pass = b;
	altVergessen();
	if ( g_cacheGilt && g_altDatei != nullptr ) {
		const auto it = g_altPaesse.find( b );
		if ( it != g_altPaesse.end() ) {
			std::string blob( static_cast<std::size_t>( it->second.laenge ), '\0' );
			g_altDa = _fseeki64( g_altDatei, it->second.pos, SEEK_SET ) == 0 && fread( blob.data(), 1, blob.size(), g_altDatei ) == blob.size() &&
			          entpacken( blob, g_alt ) && passZerlegen();
			if ( !g_altDa )
				altVergessen();
			BhedZeit( "Licht-Cache: lesen, entpacken" );
		}
	}
	g_gitterDa = false;
	g_gitterAusCache = false;
	g_gitterDaten.clear();
	g_lmDaten.clear();
	g_lmDatenDa = false;
	g_lmSig.clear();
	g_flaecheSig.clear();
	g_lmAus = 0;
	g_flaecheAus = 0;
}

bool BhedGitterAusCache( std::uint64_t ursprung ){
	if ( !g_an )
		return false;
	Hash h;
	h.text( "gitter" );
	h.wert( g_geo );   // lcb2: the grid traces against everything - same geometry only
	h.wert( g_pass );
	h.wert( lights.size() );
	for ( const light_t& l : lights )
		h.wert( lichtHash( l ) );
	h.wert( rawGridPoints.size() );
	if ( !rawGridPoints.empty() )
		h.daten( rawGridPoints.data(), rawGridPoints.size() * sizeof( rawGridPoint_t ) );
	for ( const bspGridPoint_t& g : bspGridPoints )
		h.wert( g.styles );
	h.wert( ursprung );
	g_gitterSig = h.ende();
	++g_gitterPaesse;
	if ( !g_altDa || !g_altGitterSig.da() || !( g_altGitterSig == g_gitterSig ) || g_altGitterAnzahl != rawGridPoints.size() )
		return false;
	std::size_t pos = g_altGitterPos;
	for ( std::size_t i = 0; i < rawGridPoints.size(); ++i ) {
		memcpy( &rawGridPoints[i], g_alt.data() + pos, sizeof( rawGridPoint_t ) );
		pos += sizeof( rawGridPoint_t );
		memcpy( bspGridPoints[i].styles.data(), g_alt.data() + pos, 4 );
		pos += 4;
	}
	g_gitterAusCache = true;
	++g_gitterPaesseAus;
	return true;
}

void BhedGitterMerken(){
	if ( !g_an )
		return;
	g_gitterDaten.clear();
	g_gitterDaten.reserve( rawGridPoints.size() * ( sizeof( rawGridPoint_t ) + 4 ) );
	for ( std::size_t i = 0; i < rawGridPoints.size(); ++i ) {
		g_gitterDaten.append( reinterpret_cast<const char*>( &rawGridPoints[i] ), sizeof( rawGridPoint_t ) );
		g_gitterDaten.append( reinterpret_cast<const char*>( bspGridPoints[i].styles.data() ), 4 );
	}
	g_gitterDa = true;
}

void BhedLichtCacheLichter(){
	g_lichtHash.clear();
	if ( !g_an )
		return;
	g_lichtHashOC.clear();
	for ( const light_t& l : lights ) {
		const Sig oc = lichtHashOhneCluster( l );
		g_lichtHashOC.emplace( &l, oc );
		Hash h;
		h.wert( oc );
		h.wert( l.cluster );
		g_lichtHash.emplace( &l, h.ende() );
	}
	if ( g_pass == 0 || g_fest.size() != std::size_t( numRawLightmaps ) )
		inhaltAnlegen();   // lcb2
	g_lmSig.assign( numRawLightmaps, Sig{} );
	g_lmZeit.assign( numRawLightmaps, LmZeit{} );
	g_lmArbeit.assign( numRawLightmaps, 0 );
	g_lmTreffer.assign( numRawLightmaps, 0 );
	g_flaecheSig.assign( bspDrawSurfaces.size(), Sig{} );
}

bool BhedLmVorab( int lmNum, const trace_t& trace, unsigned int seed ){
	if ( !g_an || lmNum < 0 || lmNum >= int( g_lmSig.size() ) )
		return false;
	rawLightmap_t* lm = &rawLightmaps[lmNum];
	LmFest& f = g_fest[lmNum];
	if ( !f.da )
		festRechnen( lmNum );   // lcb2: before the first pass lights it
	const unsigned long long arbeit = static_cast<unsigned long long>( lm->sw ) * lm->sh * static_cast<unsigned long long>( std::max( 1, trace.numLights ) );
	g_lmArbeit[lmNum] = arbeit;
	/* lcb2: the fingerprint of the pass - pass number, the generator (only random subsamples draw from it), the
	   fixed part, the culled light list (every light with every field, in list order, its cluster as it relates
	   to the lightmap's), the PVS from the lightmap's clusters to the lights', where the rays can run with the
	   trace tree there (BhedSpurRaumSig) and the own state at the beginning of the pass */
	Hash h;
	h.text( "lm5" );   // 5: the trace tree signature with the sweeps narrowed on the way down
	h.wert( g_pass );
	if ( lightRandomSamples )
		h.wert( seed );
	h.wert( f.sig );
	h.wert( trace.recvShadows );
	h.wert( trace.twoSided );
	h.wert( trace.testOcclusion );
	h.wert( trace.forceSunlight );
	h.wert( trace.numSurfaces );
	h.wert( trace.numLights );
	const bool proben = f.box[0] <= f.box[3];
	BhedStrahlRaum raum;
	/* A: the box from the lightmap's samples to its point and area lights - kept here and put in front of the suns
	   below. As a reference to raum.komp[0] it dangled as soon as a sun was appended (the vector moved): the box
	   was lost or stopped at the lights (a lightmap could come from the cache although something between it and a
	   point light had changed), and the writes went to freed memory (duel_jakku, 71 suns: the partial build
	   crashed in three of five runs). */
	BhedStrahlRaum::Komp A{ { 1e30, 1e30, 1e30 }, { -1e30, -1e30, -1e30 }, { 0, 0, 0 } };
	raum.komp.push_back( A );   // its place
	bool punkt = false;
	std::vector<int> lichtCl;   // the light clusters that are none of the lightmap's, in order of appearance
	std::unordered_map<int, int> ids;
	if ( trace.numLights > 0 )
		for ( std::size_t i = 0; i < f.roh.size(); ++i )
			ids.emplace( f.roh[i], int( i ) );
	const auto ziel = [&]( const Vector3& p ){
		for ( int a = 0; a < 3; ++a ) {
			A.lo[a] = std::min( A.lo[a], double( p[a] ) - kRand );
			A.hi[a] = std::max( A.hi[a], double( p[a] ) + kRand );
		}
	};
	for ( int i = 0; i < trace.numLights; ++i ) {
		const light_t* l = trace.lights[i];
		const auto it = g_lichtHashOC.find( l );
		h.wert( it != g_lichtHashOC.end() ? it->second : lichtHashOhneCluster( *l ) );
		const int c = l->cluster;
		if ( c < 0 ) {
			h.wert( 'n' );
			h.wert( c );
		}
		else {
			const auto [jt, neu] = ids.emplace( c, int( f.roh.size() + lichtCl.size() ) );
			if ( neu )
				lichtCl.push_back( c );
			h.wert( 'p' );
			h.wert( jt->second );
			h.wert( c == 0 );
		}
		/* the rays (LightContributionToSample): to the origin, an area light also to origin - normal * 2,
		   a sun from the sample along its vector */
		if ( l->type == ELightType::Sun ) {
			raum.himmel = true;
			if ( proben )
				raum.komp.push_back( BhedStrahlRaum::Komp{ { f.box[0], f.box[1], f.box[2] }, { f.box[3], f.box[4], f.box[5] },
				                                           { l->origin[0], l->origin[1], l->origin[2] } } );
		}
		else {
			punkt = true;
			ziel( l->origin );
			if ( l->type == ELightType::Area )
				ziel( Vector3( l->origin - l->normal * 2.f ) );
		}
	}
	if ( punkt && proben )
		for ( int a = 0; a < 3; ++a ) {
			A.lo[a] = std::min( A.lo[a], f.box[a] );
			A.hi[a] = std::max( A.hi[a], f.box[3 + a] );
		}
	else {
		A.lo[0] = 1;
		A.hi[0] = 0;
	}
	raum.komp[0] = A;
	for ( const int ca : f.roh )
		for ( const int cb : lichtCl )
			h.wert( ClusterVisible( ca, cb ) );
	const Sig spur = BhedSpurRaumSig( raum );
	if ( !spur.da() ) {   // no trace tree hashes: traced, not kept
		return false;
	}
	h.wert( spur );
	lmZustand( h, lm, f );
	const Sig sig = h.ende();
	g_lmSig[lmNum] = sig;
	const auto alt = g_altDa ? g_altLm.find( sig ) : g_altLm.end();
	if ( alt == g_altLm.end() ) {
		return false;
	}
	std::size_t pos = alt->second;
	Array4<byte> styles;
	memcpy( styles.data(), g_alt.data() + pos, 4 );
	pos += 4;
	std::uint32_t sw = 0, sh = 0;
	unsigned char maske = 0;
	lesen( pos, sw );
	lesen( pos, sh );
	lesen( pos, maske );
	if ( int( sw ) != lm->sw || int( sh ) != lm->sh ) {
		return false;
	}
	const std::size_t n = std::size_t( sw ) * sh;
	lm->styles = styles;
	for ( int l = 0; l < MAX_LIGHTMAPS; ++l ) {
		if ( maske & ( 1 << l ) ) {
			if ( lm->superLuxels[l] == nullptr )
				lm->superLuxels[l] = safe_calloc( n * sizeof( SuperLuxel ) );
			memcpy( lm->superLuxels[l], g_alt.data() + pos, n * sizeof( SuperLuxel ) );
			pos += n * sizeof( SuperLuxel );
		}
		else if ( lm->superLuxels[l] != nullptr ) {
			free( lm->superLuxels[l] );
			lm->superLuxels[l] = nullptr;
		}
	}
	if ( maske & 16 ) {
		if ( lm->superDeluxels == nullptr )
			lm->superDeluxels = safe_calloc( n * sizeof( Vector3 ) );
		memcpy( lm->superDeluxels, g_alt.data() + pos, n * sizeof( Vector3 ) );
		pos += n * sizeof( Vector3 );
	}
	if ( maske & 32 ) {
		if ( lm->superFlags == nullptr )
			lm->superFlags = safe_calloc( n );
		memcpy( lm->superFlags, g_alt.data() + pos, n );
		pos += n;
	}
	/* the filter pass of IlluminateRawLightmap marks filled unmapped luxels CLUSTER_FLOODED (cluster < 0 ->
	   CLUSTER_FLOODED, nothing else); StoreSurfaceLightmaps reads that when it averages super samples (-super),
	   the next pass too. lcb2: only that mark is taken - the cluster numbers are the BSP's of this run */
	if ( maske & 64 ) {
		for ( std::size_t i = 0; i < n; ++i ) {
			int c;
			memcpy( &c, g_alt.data() + pos + i * sizeof( int ), sizeof( int ) );
			if ( c == CLUSTER_FLOODED && lm->superClusters[i] < CLUSTER_NORMAL )
				lm->superClusters[i] = CLUSTER_FLOODED;
		}
		pos += n * sizeof( int );
	}
	g_lmTreffer[lmNum] = 1;
	++g_lmAus;
	return true;
}

void BhedLmZeit( int lmNum, double kosten, double wand ){
	if ( g_an && lmNum >= 0 && lmNum < int( g_lmZeit.size() ) )
		g_lmZeit[lmNum] = LmZeit{ float( kosten ), float( wand ) };
}

void BhedLmVorabFertig(){
	if ( !g_an )
		return;
	/* lcb2: the share of the time, from what the cached run measured per lightmap (luxels x lights underrated the
	   big floor lightmaps: 16 % of the work, 60 % of the time on duel_mos_espa). A pass takes as long as its
	   thread seconds spread over the threads - or as long as its longest lightmap, when that is longer (one
	   lightmap was 31 of the 32 s of a bounce there). Both for the lightmaps left to light and for all of them;
	   a lightmap without a time (new content) counts with its luxels x lights at the rate of those that have one. */
	const int n = int( std::min( { g_lmArbeit.size(), g_lmTreffer.size(), g_fest.size() } ) );
	std::vector<const LmZeit*> alt( n, nullptr );
	double zeitAlt = 0, arbeitAlt = 0;
	for ( int i = 0; i < n; ++i )
		if ( g_fest[i].da ) {
			const auto it = g_altZeit.find( g_fest[i].sig );
			if ( it != g_altZeit.end() ) {
				alt[i] = &it->second;
				zeitAlt += it->second.kosten;
				arbeitAlt += double( g_lmArbeit[i] );
			}
		}
	const double rate = arbeitAlt > 0 && zeitAlt > 0 ? zeitAlt / arbeitAlt : 0.0;
	double alle = 0, neu = 0, laengsteAlle = 0, laengsteNeu = 0;
	for ( int i = 0; i < n; ++i ) {
		const double k = alt[i] != nullptr ? alt[i]->kosten : ( rate > 0 ? double( g_lmArbeit[i] ) * rate : double( g_lmArbeit[i] ) );
		const double w = alt[i] != nullptr ? alt[i]->wand : 0.0;
		alle += k;
		laengsteAlle = std::max( laengsteAlle, w );
		if ( !g_lmTreffer[i] ) {
			neu += k;
			laengsteNeu = std::max( laengsteNeu, w );
		}
	}
	const double threads = std::max( 1, numthreads );
	const double dauerAlle = rate > 0 ? std::max( alle / threads, laengsteAlle ) : alle, dauerNeu = rate > 0 ? std::max( neu / threads, laengsteNeu ) : neu;
	const double anteil = dauerAlle > 0 ? 100.0 * dauerNeu / dauerAlle : 0.0;
	Sys_Printf( "BehavEd: light cache pass %d of %d: %d of %d lightmaps from the cache (%.1f %% of the time to trace)\n", g_pass + 1, g_paesse,
	            g_lmAus.load(), numRawLightmaps, anteil );
}

// after BhedIlluminateRawLightmaps, before StitchSurfaceLightmaps (stitching runs on the raw values again)
void BhedLmDatenMerken(){
	if ( !g_an )
		return;
	g_lmDaten.clear();
	anhaengen( g_lmDaten, std::uint32_t( numRawLightmaps ) );
	for ( int i = 0; i < numRawLightmaps; ++i ) {
		const rawLightmap_t* lm = &rawLightmaps[i];
		const Sig sig = i < int( g_lmSig.size() ) ? g_lmSig[i] : Sig{};
		anhaengen( g_lmDaten, sig );
		if ( !sig.da() )
			continue;
		g_lmDaten.append( reinterpret_cast<const char*>( lm->styles.data() ), 4 );
		anhaengen( g_lmDaten, std::uint32_t( lm->sw ) );
		anhaengen( g_lmDaten, std::uint32_t( lm->sh ) );
		unsigned char maske = 0;
		for ( int l = 0; l < MAX_LIGHTMAPS; ++l )
			if ( lm->superLuxels[l] != nullptr )
				maske |= 1 << l;
		if ( lm->superDeluxels != nullptr ) maske |= 16;
		if ( lm->superFlags != nullptr ) maske |= 32;
		if ( lm->superClusters != nullptr ) maske |= 64;
		anhaengen( g_lmDaten, maske );
		const std::size_t n = std::size_t( lm->sw ) * lm->sh;
		for ( int l = 0; l < MAX_LIGHTMAPS; ++l )
			if ( lm->superLuxels[l] != nullptr )
				g_lmDaten.append( reinterpret_cast<const char*>( lm->superLuxels[l] ), n * sizeof( SuperLuxel ) );
		if ( lm->superDeluxels != nullptr )
			g_lmDaten.append( reinterpret_cast<const char*>( lm->superDeluxels ), n * sizeof( Vector3 ) );
		if ( lm->superFlags != nullptr )
			g_lmDaten.append( reinterpret_cast<const char*>( lm->superFlags ), n );
		if ( lm->superClusters != nullptr )
			g_lmDaten.append( reinterpret_cast<const char*>( lm->superClusters ), n * sizeof( int ) );
	}
	g_lmDatenDa = true;
	g_summeLmAus += g_lmAus.load();
	g_summeLmAlle += numRawLightmaps;
	BhedZeit( "Licht-Cache: Daten merken" );
}

/* a vertex lit surface reads its vertex styles and adds the pass to its vertex luxels (IlluminateVertexes:
   vertLuxel += radVertLuxel) - those, the list and the generator of its work item decide its result */
bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace, unsigned int seed ){
	if ( !g_an || surfaceNum < 0 || surfaceNum >= int( g_flaecheSig.size() ) )
		return false;
	bspDrawSurface_t& ds = bspDrawSurfaces[surfaceNum];
	Hash h;
	h.text( "vertex" );
	h.wert( g_geo );   // lcb2: raw cluster and surface numbers - same geometry only
	h.wert( g_pass );
	h.wert( seed );
	h.wert( listenSigRoh( trace ) );
	h.wert( ds.vertexStyles );
	h.wert( ds.numVerts );
	for ( int i = 0; i < ds.numVerts; ++i )
		for ( int l = 0; l < MAX_LIGHTMAPS; ++l ) {
			h.wert( getVertexLuxel( l, ds.firstVert + i ) );
			h.wert( getRadVertexLuxel( l, ds.firstVert + i ) );
		}
	const Sig sig = h.ende();
	g_flaecheSig[surfaceNum] = sig;
	if ( !g_altDa || surfaceNum >= int( g_altFlaeche.size() ) || !( g_altFlaeche[surfaceNum].sig == sig ) )
		return false;
	const surfaceInfo_t& info = surfaceInfos[surfaceNum];
	std::size_t pos = g_altFlaeche[surfaceNum].pos;
	Array4<byte> vstyles;
	memcpy( vstyles.data(), g_alt.data() + pos, 4 );
	pos += 4;
	std::uint32_t nv = 0;
	lesen( pos, nv );
	if ( int( nv ) != ds.numVerts )
		return false;
	ds.vertexStyles = vstyles;
	bspDrawVert_t* verts = &yDrawVerts[ds.firstVert];
	for ( int i = 0; i < ds.numVerts; ++i ) {
		for ( int l = 0; l < MAX_LIGHTMAPS; ++l ) {
			Vector3& vertLuxel = getVertexLuxel( l, ds.firstVert + i );
			Vector3& radVertLuxel = getRadVertexLuxel( l, ds.firstVert + i );
			memcpy( &vertLuxel, g_alt.data() + pos, sizeof( Vector3 ) );
			pos += sizeof( Vector3 );
			memcpy( &radVertLuxel, g_alt.data() + pos, sizeof( Vector3 ) );
			pos += sizeof( Vector3 );
			/* IlluminateVertexes, "store it": the colour from the (restored) floating point value */
			if ( !info.si->noVertexLight )
				verts[i].color[l].rgb() = ColorToBytes( vertLuxel, info.si->vertexScale );
		}
	}
	++g_flaecheAus;
	return true;
}

void BhedLichtCacheDurchgangEnde(){
	if ( !g_an || g_pass < 0 )
		return;
	std::string d;
	const unsigned char gitter = g_gitterDa ? 1 : 0;
	anhaengen( d, gitter );
	if ( g_gitterDa ) {
		anhaengen( d, g_gitterSig );
		anhaengen( d, std::uint32_t( rawGridPoints.size() ) );
		d += g_gitterDaten;
	}
	if ( g_lmDatenDa )
		d += g_lmDaten;
	else
		anhaengen( d, std::uint32_t( 0 ) );
	g_lmDaten.clear();
	g_lmDaten.shrink_to_fit();
	// vertex lit surfaces (IlluminateVertexes has run)
	int flaechen = 0;
	anhaengen( d, std::uint32_t( bspDrawSurfaces.size() ) );
	for ( std::size_t s = 0; s < bspDrawSurfaces.size(); ++s ) {
		const Sig sig = s < g_flaecheSig.size() ? g_flaecheSig[s] : Sig{};
		anhaengen( d, sig );
		if ( !sig.da() )
			continue;
		++flaechen;
		const bspDrawSurface_t& ds = bspDrawSurfaces[s];
		d.append( reinterpret_cast<const char*>( ds.vertexStyles.data() ), 4 );
		anhaengen( d, std::uint32_t( ds.numVerts ) );
		for ( int i = 0; i < ds.numVerts; ++i )
			for ( int l = 0; l < MAX_LIGHTMAPS; ++l ) {
				d.append( reinterpret_cast<const char*>( &getVertexLuxel( l, ds.firstVert + i ) ), sizeof( Vector3 ) );
				d.append( reinterpret_cast<const char*>( &getRadVertexLuxel( l, ds.firstVert + i ) ), sizeof( Vector3 ) );
			}
	}
	/* lcb2: the time of every lightmap: measured, else the cached run's for the same content (taken from the cache,
	   or lit on the GPU), else unknown (-1) */
	const bool zeiten = g_lmZeit.size() == std::size_t( numRawLightmaps ) && g_fest.size() == std::size_t( numRawLightmaps );
	anhaengen( d, std::uint32_t( zeiten ? numRawLightmaps : 0 ) );
	for ( int i = 0; zeiten && i < numRawLightmaps; ++i ) {
		const Sig fs = g_fest[i].da ? g_fest[i].sig : Sig{};
		LmZeit z = g_lmZeit[i];
		if ( z.kosten < 0 && fs.da() ) {
			const auto it = g_altZeit.find( fs );
			if ( it != g_altZeit.end() )
				z = it->second;
		}
		anhaengen( d, fs );
		anhaengen( d, z.kosten );
		anhaengen( d, z.wand );
	}
	g_summeFlAus += g_flaecheAus.load();
	g_summeFlAlle += flaechen;
	if ( g_neu != nullptr ) {
		const std::int32_t b = g_pass;
		std::string blob;
		const bool gepackt = packen( d, blob );   // lcb2
		const std::uint64_t laenge = blob.size();
		const bool ok = gepackt && fwrite( &b, sizeof( b ), 1, g_neu ) == 1 && fwrite( &laenge, sizeof( laenge ), 1, g_neu ) == 1 &&
		                fwrite( blob.data(), 1, blob.size(), g_neu ) == blob.size();
		if ( !ok ) {   // a full disk: no new cache (the old one stays)
			fclose( g_neu );
			g_neu = nullptr;
			remove( g_neuPfad.c_str() );
			Sys_Warning( "BehavEd: light cache could not be written (%s)\n", g_neuPfad.c_str() );
		}
		g_geschriebenMB += ( 12.0 + blob.size() ) / ( 1024.0 * 1024.0 );
	}
	if ( g_neu != nullptr )
		fflush( g_neu );   // the pass is on disk before its line below is printed: a light step that is stopped there keeps it (<map>.bhedlight.neu)
	Sys_Printf( "BehavEd: light cache pass %d of %d: %d of %d vertex lit surfaces from the cache, grid %s\n", g_pass + 1, g_paesse, g_flaecheAus.load(),
	            flaechen, g_gitterAusCache ? "from the cache" : ( g_gitterDa ? "traced" : "none" ) );
	altVergessen();
	g_pass = -1;
	BhedZeit( "Licht-Cache: packen, schreiben" );
}

void BhedLichtCacheSchreiben(){
	if ( !g_an )
		return;
	altVergessen();
	if ( g_altDatei != nullptr ) {
		fclose( g_altDatei );
		g_altDatei = nullptr;
	}
	bool geschrieben = false;
	if ( g_neu != nullptr ) {
		geschrieben = fclose( g_neu ) == 0;
		g_neu = nullptr;
		if ( geschrieben ) {
			remove( g_cachePfad.c_str() );
			geschrieben = rename( g_neuPfad.c_str(), g_cachePfad.c_str() ) == 0;
		}
	}
	Sys_Printf( "BehavEd: light cache: %d of %d lightmaps and %d of %d vertex lit surfaces taken from the cache, grid %s; %.1f MB written\n",
	            g_summeLmAus, g_summeLmAlle, g_summeFlAus, g_summeFlAlle,
	            g_gitterPaesse == 0 ? "none" : ( g_gitterPaesseAus == g_gitterPaesse ? "from the cache" : "traced" ), geschrieben ? g_geschriebenMB : 0.0 );
	g_lichtHash.clear();
}
