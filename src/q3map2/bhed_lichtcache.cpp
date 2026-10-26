// bhed_lichtcache.cpp - BehavEd stage 8a: -light -bhedlightcache (see bhed_lichtcache.h).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "q3map2.h"
#include "bhed_lichtcache.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {

bool g_an = false;
std::vector<std::string> g_optionen;   // light switches without the post-processing ones
std::string g_bspName;
std::string g_cachePfad;
std::uint64_t g_geo[2] = {0, 0};
bool g_cacheGilt = false;              // the read cache has the same geometry key
bool g_benutzbar = false;              // switch on, no bounce
std::string g_alt;                     // the read cache file

// read cache, parsed
struct LmAlt {
	std::uint64_t sig = 0;
	std::size_t pos = 0;   // offset of the data in g_alt
};
std::uint64_t g_altGitterSig = 0;
std::size_t g_altGitterPos = 0;
std::size_t g_altGitterAnzahl = 0;
std::vector<LmAlt> g_altLm;
std::vector<LmAlt> g_altFlaeche;

// this run
std::uint64_t g_gitterSig = 0;
std::vector<std::uint64_t> g_lmSig;
std::vector<std::uint64_t> g_flaecheSig;
std::string g_lmDaten;                 // lightmap part, written before StitchSurfaceLightmaps
bool g_lmDatenDa = false;
std::string g_gitterDaten;
bool g_gitterDa = false;
int g_lmAusCache = 0;
int g_flaecheAusCache = 0;
bool g_gitterAusCache = false;
std::unordered_map<const light_t*, std::uint64_t> g_lichtHash;
std::mutex g_sperre;

struct Hash {
	std::uint64_t a = 1469598103934665603ULL;
	std::uint64_t b = 0x9E3779B97F4A7C15ULL;
	void daten( const void* p, std::size_t n ){
		const unsigned char* c = static_cast<const unsigned char*>( p );
		for ( std::size_t i = 0; i < n; ++i ) {
			a ^= c[i];
			a *= 1099511628211ULL;
			b = ( b ^ c[i] ) * 0x100000001B3ULL + 0x9E3779B97F4A7C15ULL;
			b ^= b >> 29;
		}
	}
	void text( const char* s ){
		const std::size_t n = s != nullptr ? strlen( s ) : 0;
		daten( &n, sizeof( n ) );
		if ( n != 0 )
			daten( s, n );
	}
	template<typename T> void wert( const T& v ){ daten( &v, sizeof( v ) ); }
	template<typename T> void feld( const std::vector<T>& v ){
		const std::size_t n = v.size();
		daten( &n, sizeof( n ) );
		if ( n != 0 )
			daten( v.data(), n * sizeof( T ) );
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

std::uint64_t lichtHash( const light_t& l ){
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
	return h.a ^ ( h.b * 31 );
}

std::uint64_t listenSig( const trace_t& trace ){
	Hash h;
	h.wert( trace.numLights );
	for ( int i = 0; i < trace.numLights; ++i ) {
		const auto it = g_lichtHash.find( trace.lights[i] );
		const std::uint64_t v = it != g_lichtHash.end() ? it->second : lichtHash( *trace.lights[i] );
		h.wert( v );
	}
	h.wert( trace.numSurfaces );
	if ( trace.numSurfaces > 0 && trace.surfaces != nullptr )
		h.daten( trace.surfaces, sizeof( int ) * trace.numSurfaces );
	h.wert( trace.recvShadows );
	h.wert( trace.twoSided );
	h.wert( trace.testOcclusion );
	h.wert( trace.forceSunlight );
	const std::uint64_t s = h.a ^ ( h.b * 31 );
	return s == 0 ? 1 : s;
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
	return g;
}

bool cacheZerlegen(){
	std::size_t pos = 0;
	char kopf[8];
	if ( g_alt.size() < 8 + 16 )
		return false;
	memcpy( kopf, g_alt.data(), 8 );
	if ( memcmp( kopf, "BHLICHT1", 8 ) != 0 )
		return false;
	pos = 8;
	std::uint64_t geo[2];
	if ( !lesen( pos, geo[0] ) || !lesen( pos, geo[1] ) || geo[0] != g_geo[0] || geo[1] != g_geo[1] )
		return false;
	// grid
	std::uint32_t gp = 0;
	unsigned char gitterDa = 0;
	if ( !lesen( pos, gitterDa ) )
		return false;
	if ( gitterDa ) {
		if ( !lesen( pos, g_altGitterSig ) || !lesen( pos, gp ) )
			return false;
		g_altGitterAnzahl = gp;
		g_altGitterPos = pos;
		pos += std::size_t( gp ) * ( sizeof( rawGridPoint_t ) + 4 );
		if ( pos > g_alt.size() )
			return false;
	}
	// lightmaps
	std::uint32_t nlm = 0;
	if ( !lesen( pos, nlm ) )
		return false;
	g_altLm.assign( nlm, LmAlt{} );
	for ( std::uint32_t i = 0; i < nlm; ++i ) {
		if ( !lesen( pos, g_altLm[i].sig ) )
			return false;
		if ( g_altLm[i].sig == 0 )
			continue;
		g_altLm[i].pos = pos;
		const std::size_t g = lmGroesse( pos );
		if ( g == 0 || pos + g > g_alt.size() )
			return false;
		pos += g;
	}
	// vertex lit surfaces
	std::uint32_t nf = 0;
	if ( !lesen( pos, nf ) )
		return false;
	g_altFlaeche.assign( nf, LmAlt{} );
	for ( std::uint32_t i = 0; i < nf; ++i ) {
		if ( !lesen( pos, g_altFlaeche[i].sig ) )
			return false;
		if ( g_altFlaeche[i].sig == 0 )
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
	return true;
}

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

void BhedLichtCacheGeometrie(){
	if ( !g_an )
		return;
	g_benutzbar = bounce == 0;
	if ( !g_benutzbar ) {
		Sys_Printf( "BehavEd: light cache off (-bounce: radiosity depends on every light)\n" );
		return;
	}
	Hash h;
	h.text( "bhedlicht1" );
	h.text( g_game->arg );
	for ( const std::string& o : g_optionen )
		h.text( o.c_str() );
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
	g_geo[0] = h.a;
	g_geo[1] = h.b;
	// read the old cache
	char pfad[1024];
	strcpy( pfad, g_bspName.c_str() );
	bhed_set_map_extension( pfad, ".bhedlight" );
	g_cachePfad = pfad;
	g_alt.clear();
	if ( FILE* f = fopen( pfad, "rb" ) ) {
		char buf[1 << 16];
		std::size_t n;
		while ( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
			g_alt.append( buf, n );
		fclose( f );
	}
	g_cacheGilt = !g_alt.empty() && cacheZerlegen();
	if ( !g_cacheGilt ) {
		g_alt.clear();
		g_altLm.clear();
		g_altFlaeche.clear();
		g_altGitterSig = 0;
	}
	Sys_Printf( "BehavEd: light cache %s\n", g_cacheGilt ? "found (geometry and switches unchanged)" : "not usable (none, or geometry/switches changed)" );
}

bool BhedGitterAusCache(){
	if ( !g_an || !g_benutzbar )
		return false;
	Hash h;
	h.wert( lights.size() );
	for ( const light_t& l : lights )
		h.wert( lichtHash( l ) );
	h.wert( rawGridPoints.size() );
	g_gitterSig = h.a ^ ( h.b * 31 );
	g_gitterDaten.clear();
	g_gitterDa = true;
	if ( !g_cacheGilt || g_altGitterSig != g_gitterSig || g_altGitterAnzahl != rawGridPoints.size() )
		return false;
	std::size_t pos = g_altGitterPos;
	for ( std::size_t i = 0; i < rawGridPoints.size(); ++i ) {
		memcpy( &rawGridPoints[i], g_alt.data() + pos, sizeof( rawGridPoint_t ) );
		pos += sizeof( rawGridPoint_t );
		memcpy( bspGridPoints[i].styles.data(), g_alt.data() + pos, 4 );
		pos += 4;
	}
	g_gitterAusCache = true;
	return true;
}

void BhedLichtCacheLichter(){
	g_lichtHash.clear();
	for ( const light_t& l : lights )
		g_lichtHash.emplace( &l, lichtHash( l ) );
	g_lmSig.assign( numRawLightmaps, 0 );
	g_flaecheSig.assign( bspDrawSurfaces.size(), 0 );
}

bool BhedLmAusCache( int lmNum, const trace_t& trace ){
	if ( !g_an || !g_benutzbar || lmNum < 0 || lmNum >= int( g_lmSig.size() ) )
		return false;
	const std::uint64_t sig = listenSig( trace );
	g_lmSig[lmNum] = sig;
	if ( !g_cacheGilt || lmNum >= int( g_altLm.size() ) || g_altLm[lmNum].sig != sig )
		return false;
	rawLightmap_t* lm = &rawLightmaps[lmNum];
	std::size_t pos = g_altLm[lmNum].pos;
	Array4<byte> styles;
	memcpy( styles.data(), g_alt.data() + pos, 4 );
	pos += 4;
	std::uint32_t sw = 0, sh = 0;
	unsigned char maske = 0;
	lesen( pos, sw );
	lesen( pos, sh );
	lesen( pos, maske );
	if ( int( sw ) != lm->sw || int( sh ) != lm->sh )
		return false;
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
	std::lock_guard<std::mutex> l( g_sperre );
	++g_lmAusCache;
	return true;
}

// after BhedIlluminateRawLightmaps, before StitchSurfaceLightmaps (stitching runs on the raw values again)
void BhedLmDatenMerken(){
	if ( !g_an || !g_benutzbar )
		return;
	g_lmDaten.clear();
	anhaengen( g_lmDaten, std::uint32_t( numRawLightmaps ) );
	for ( int i = 0; i < numRawLightmaps; ++i ) {
		const rawLightmap_t* lm = &rawLightmaps[i];
		const std::uint64_t sig = i < int( g_lmSig.size() ) ? g_lmSig[i] : 0;
		anhaengen( g_lmDaten, sig );
		if ( sig == 0 )
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
		anhaengen( g_lmDaten, maske );
		const std::size_t n = std::size_t( lm->sw ) * lm->sh;
		for ( int l = 0; l < MAX_LIGHTMAPS; ++l )
			if ( lm->superLuxels[l] != nullptr )
				g_lmDaten.append( reinterpret_cast<const char*>( lm->superLuxels[l] ), n * sizeof( SuperLuxel ) );
		if ( lm->superDeluxels != nullptr )
			g_lmDaten.append( reinterpret_cast<const char*>( lm->superDeluxels ), n * sizeof( Vector3 ) );
		if ( lm->superFlags != nullptr )
			g_lmDaten.append( reinterpret_cast<const char*>( lm->superFlags ), n );
	}
	g_lmDatenDa = true;
}

bool BhedFlaecheAusCache( int surfaceNum, const trace_t& trace ){
	if ( !g_an || !g_benutzbar || surfaceNum < 0 || surfaceNum >= int( g_flaecheSig.size() ) )
		return false;
	const std::uint64_t sig = listenSig( trace );
	g_flaecheSig[surfaceNum] = sig;
	if ( !g_cacheGilt || surfaceNum >= int( g_altFlaeche.size() ) || g_altFlaeche[surfaceNum].sig != sig )
		return false;
	bspDrawSurface_t& ds = bspDrawSurfaces[surfaceNum];
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
	std::lock_guard<std::mutex> l( g_sperre );
	++g_flaecheAusCache;
	return true;
}

void BhedLichtCacheSchreiben(){
	if ( !g_an || !g_benutzbar )
		return;
	std::string d( "BHLICHT1", 8 );
	anhaengen( d, g_geo[0] );
	anhaengen( d, g_geo[1] );
	const unsigned char gitter = g_gitterDa ? 1 : 0;
	anhaengen( d, gitter );
	if ( g_gitterDa ) {
		anhaengen( d, g_gitterSig );
		anhaengen( d, std::uint32_t( rawGridPoints.size() ) );
		for ( std::size_t i = 0; i < rawGridPoints.size(); ++i ) {
			d.append( reinterpret_cast<const char*>( &rawGridPoints[i] ), sizeof( rawGridPoint_t ) );
			d.append( reinterpret_cast<const char*>( bspGridPoints[i].styles.data() ), 4 );
		}
	}
	if ( g_lmDatenDa )
		d += g_lmDaten;
	else
		anhaengen( d, std::uint32_t( 0 ) );
	// vertex lit surfaces (IlluminateVertexes has run)
	anhaengen( d, std::uint32_t( bspDrawSurfaces.size() ) );
	for ( std::size_t s = 0; s < bspDrawSurfaces.size(); ++s ) {
		const std::uint64_t sig = s < g_flaecheSig.size() ? g_flaecheSig[s] : 0;
		anhaengen( d, sig );
		if ( sig == 0 )
			continue;
		const bspDrawSurface_t& ds = bspDrawSurfaces[s];
		d.append( reinterpret_cast<const char*>( ds.vertexStyles.data() ), 4 );
		anhaengen( d, std::uint32_t( ds.numVerts ) );
		for ( int i = 0; i < ds.numVerts; ++i )
			for ( int l = 0; l < MAX_LIGHTMAPS; ++l ) {
				d.append( reinterpret_cast<const char*>( &getVertexLuxel( l, ds.firstVert + i ) ), sizeof( Vector3 ) );
				d.append( reinterpret_cast<const char*>( &getRadVertexLuxel( l, ds.firstVert + i ) ), sizeof( Vector3 ) );
			}
	}
	if ( FILE* f = fopen( g_cachePfad.c_str(), "wb" ) ) {
		fwrite( d.data(), 1, d.size(), f );
		fclose( f );
	}
	int flaechen = 0;
	for ( const std::uint64_t s : g_flaecheSig )
		flaechen += s != 0 ? 1 : 0;
	Sys_Printf( "BehavEd: light cache: %d of %d lightmaps and %d of %d vertex lit surfaces taken from the cache, grid %s; %.1f MB written\n",
	            g_lmAusCache, numRawLightmaps, g_flaecheAusCache, flaechen, g_gitterAusCache ? "from the cache" : ( g_gitterDa ? "traced" : "none" ),
	            d.size() / ( 1024.0 * 1024.0 ) );
}
