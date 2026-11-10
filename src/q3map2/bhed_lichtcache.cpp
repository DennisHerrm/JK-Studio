// bhed_lichtcache.cpp - BehavEd stage 8a: -light -bhedlightcache (see bhed_lichtcache.h).
// lcb1: every pass on its own (also -bounce), the file holds one section per pass.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "q3map2.h"
#include "bhed_lichtcache.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {

// --- the fingerprint ---------------------------------------------------------------------------

struct Sig {
	std::uint64_t a = 0;
	std::uint64_t b = 0;
	[[nodiscard]] bool da() const { return a != 0 || b != 0; }
	bool operator==( const Sig& o ) const { return a == o.a && b == o.b; }
};

// two 64 bit lanes over 8 byte words (the old byte wise FNV took seconds on big maps)
struct Hash {
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
	[[nodiscard]] Sig ende() const {
		Sig s{ mix( a ^ rotl( b, 17 ) ), mix( b + a * 3 ) };
		if ( !s.da() )
			s.a = 1;
		return s;
	}
};

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

// every field of a light that LightContributionToSample/Point reads (its shader by name, the scripts are in the geometry key)
Sig lichtHash( const light_t& l ){
	Hash h;
	h.wert( l.type );
	h.wert( l.flags );
	h.text( l.si != nullptr ? l.si->shader.c_str() : "" );
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
	h.wert( l.cluster );
	h.feld( l.w );
	h.wert( l.falloffTolerance );
	h.wert( l.filterRadius );
	h.wert( l.skyIndex );
	return h.ende();
}

// --- state ---------------------------------------------------------------------------------------

bool g_an = false;
std::vector<std::string> g_optionen;   // light switches without the post-processing ones
std::string g_bspName;
std::string g_cachePfad;
std::string g_neuPfad;                 // written while the run goes, renamed at the end
Sig g_opt;                             // the light switches
Sig g_geo;                             // the geometry
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
std::vector<Alt> g_altLm;
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
bool g_lmDatenDa = false;
std::unordered_map<const light_t*, Sig> g_lichtHash;
std::mutex g_sperre;
std::atomic<int> g_lmAus{ 0 };
std::atomic<int> g_flaecheAus{ 0 };
std::atomic<unsigned long long> g_arbeitAlle{ 0 };
std::atomic<unsigned long long> g_arbeitNeu{ 0 };
// all passes
int g_summeLmAus = 0, g_summeLmAlle = 0, g_summeFlAus = 0, g_summeFlAlle = 0;
int g_gitterPaesse = 0, g_gitterPaesseAus = 0;
double g_geschriebenMB = 0.0;

Sig listenSig( const trace_t& trace ){
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
void lmZustand( Hash& h, const rawLightmap_t* lm ){
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
	h.daten( lm->superClusters, n * sizeof( int ) );
	if ( lm->superDeluxels != nullptr )
		h.daten( lm->superDeluxels, n * sizeof( Vector3 ) );
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
	g_altLm.assign( nlm, Alt{} );
	for ( std::uint32_t i = 0; i < nlm; ++i ) {
		if ( !lesen( pos, g_altLm[i].sig ) )
			return false;
		if ( !g_altLm[i].sig.da() )
			continue;
		g_altLm[i].pos = pos;
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
	return pos == g_alt.size();
}

void altVergessen(){
	g_alt.clear();
	g_alt.shrink_to_fit();
	g_altDa = false;
	g_altLm.clear();
	g_altFlaeche.clear();
	g_altGitterSig = Sig{};
	g_altGitterAnzahl = 0;
}

constexpr char kKopf[8] = { 'B', 'H', 'L', 'I', 'C', 'H', 'T', '3' };   // 3: one section per pass (lcb1)

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
		h.text( "bhedlicht3" );
		h.text( g_game->arg );
		for ( const std::string& o : g_optionen )
			h.text( o.c_str() );
		g_opt = h.ende();
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
	const char* grund = "no cache yet";
	g_altDatei = fopen( pfad, "rb" );
	if ( g_altDatei != nullptr ) {
		char kopf[8] = {};
		Sig opt, geo;
		if ( fread( kopf, 1, 8, g_altDatei ) != 8 || memcmp( kopf, kKopf, 8 ) != 0 ) {
			grund = "older cache format";
		}
		else if ( fread( &opt, sizeof( opt ), 1, g_altDatei ) != 1 || fread( &geo, sizeof( geo ), 1, g_altDatei ) != 1 ) {
			grund = "cache file damaged";
		}
		else if ( !( opt == g_opt ) ) {
			grund = "light switches changed";
		}
		else if ( !( geo == g_geo ) ) {
			grund = "geometry, shaders or brush entities changed";
		}
		else {
			g_cacheGilt = true;
			while ( true ) {
				std::int32_t b = 0;
				std::uint64_t laenge = 0;
				if ( fread( &b, sizeof( b ), 1, g_altDatei ) != 1 || fread( &laenge, sizeof( laenge ), 1, g_altDatei ) != 1 )
					break;
				const long long pos = _ftelli64( g_altDatei );
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
	if ( g_cacheGilt )
		Sys_Printf( "BehavEd: light cache found (geometry and switches unchanged, %d of %d passes kept)\n", int( g_altPaesse.size() ), g_paesse );
	else
		Sys_Printf( "BehavEd: light cache not usable: %s - everything is traced\n", grund );
	// the new cache, written pass by pass
	g_neu = fopen( g_neuPfad.c_str(), "wb" );
	if ( g_neu != nullptr ) {
		fwrite( kKopf, 1, 8, g_neu );
		fwrite( &g_opt, sizeof( g_opt ), 1, g_neu );
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
			g_alt.resize( static_cast<std::size_t>( it->second.laenge ) );
			g_altDa = _fseeki64( g_altDatei, it->second.pos, SEEK_SET ) == 0 && fread( g_alt.data(), 1, g_alt.size(), g_altDatei ) == g_alt.size() &&
			          passZerlegen();
			if ( !g_altDa )
				altVergessen();
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
	for ( const light_t& l : lights )
		g_lichtHash.emplace( &l, lichtHash( l ) );
	g_lmSig.assign( numRawLightmaps, Sig{} );
	g_flaecheSig.assign( bspDrawSurfaces.size(), Sig{} );
	g_arbeitAlle = 0;
	g_arbeitNeu = 0;
}

bool BhedLmVorab( int lmNum, const trace_t& trace, unsigned int seed ){
	if ( !g_an || lmNum < 0 || lmNum >= int( g_lmSig.size() ) )
		return false;
	rawLightmap_t* lm = &rawLightmaps[lmNum];
	Hash h;
	h.text( "lm" );
	h.wert( g_pass );
	h.wert( seed );
	h.wert( listenSig( trace ) );
	lmZustand( h, lm );
	const Sig sig = h.ende();
	g_lmSig[lmNum] = sig;
	const unsigned long long arbeit = static_cast<unsigned long long>( lm->sw ) * lm->sh * static_cast<unsigned long long>( std::max( 1, trace.numLights ) );
	g_arbeitAlle += arbeit;
	if ( !g_altDa || lmNum >= int( g_altLm.size() ) || !( g_altLm[lmNum].sig == sig ) ) {
		g_arbeitNeu += arbeit;
		return false;
	}
	std::size_t pos = g_altLm[lmNum].pos;
	Array4<byte> styles;
	memcpy( styles.data(), g_alt.data() + pos, 4 );
	pos += 4;
	std::uint32_t sw = 0, sh = 0;
	unsigned char maske = 0;
	lesen( pos, sw );
	lesen( pos, sh );
	lesen( pos, maske );
	if ( int( sw ) != lm->sw || int( sh ) != lm->sh ) {
		g_arbeitNeu += arbeit;
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
	/* the filter pass of IlluminateRawLightmap marks filled unmapped luxels CLUSTER_FLOODED;
	   StoreSurfaceLightmaps reads that when it averages super samples (-super), the next pass too */
	if ( maske & 64 ) {
		memcpy( lm->superClusters, g_alt.data() + pos, n * sizeof( int ) );
		pos += n * sizeof( int );
	}
	++g_lmAus;
	return true;
}

void BhedLmVorabFertig(){
	if ( !g_an )
		return;
	const unsigned long long alle = g_arbeitAlle.load();
	const double anteil = alle > 0 ? 100.0 * double( g_arbeitNeu.load() ) / double( alle ) : 0.0;
	Sys_Printf( "BehavEd: light cache pass %d of %d: %d of %d lightmaps from the cache (%.1f %% of the work to trace)\n", g_pass + 1, g_paesse,
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
}

/* a vertex lit surface reads its vertex styles and adds the pass to its vertex luxels (IlluminateVertexes:
   vertLuxel += radVertLuxel) - those, the list and the generator of its work item decide its result */
bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace, unsigned int seed ){
	if ( !g_an || surfaceNum < 0 || surfaceNum >= int( g_flaecheSig.size() ) )
		return false;
	bspDrawSurface_t& ds = bspDrawSurfaces[surfaceNum];
	Hash h;
	h.text( "vertex" );
	h.wert( g_pass );
	h.wert( seed );
	h.wert( listenSig( trace ) );
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
	g_summeFlAus += g_flaecheAus.load();
	g_summeFlAlle += flaechen;
	if ( g_neu != nullptr ) {
		const std::int32_t b = g_pass;
		const std::uint64_t laenge = d.size();
		const bool ok = fwrite( &b, sizeof( b ), 1, g_neu ) == 1 && fwrite( &laenge, sizeof( laenge ), 1, g_neu ) == 1 &&
		                fwrite( d.data(), 1, d.size(), g_neu ) == d.size();
		if ( !ok ) {   // a full disk: no new cache (the old one stays)
			fclose( g_neu );
			g_neu = nullptr;
			remove( g_neuPfad.c_str() );
			Sys_Warning( "BehavEd: light cache could not be written (%s)\n", g_neuPfad.c_str() );
		}
		g_geschriebenMB += ( 12.0 + d.size() ) / ( 1024.0 * 1024.0 );
	}
	Sys_Printf( "BehavEd: light cache pass %d of %d: %d of %d vertex lit surfaces from the cache, grid %s\n", g_pass + 1, g_paesse, g_flaecheAus.load(),
	            flaechen, g_gitterAusCache ? "from the cache" : ( g_gitterDa ? "traced" : "none" ) );
	altVergessen();
	g_pass = -1;
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
