/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "q3map2.h"
#include "bspfile_abstract.h"
#include <ctime>
#include <cstdint> // BehavEd stage 7: lightgrid buckets
#include <unordered_map> // BehavEd stage 7: lightgrid buckets




/* -------------------------------------------------------------------------------

   this file handles translating the bsp file format used by quake 3, rtcw, and ef
   into the abstracted bsp file used by q3map2.

   ------------------------------------------------------------------------------- */

/* constants */
#define LUMP_ENTITIES       0
#define LUMP_SHADERS        1
#define LUMP_PLANES         2
#define LUMP_NODES          3
#define LUMP_LEAFS          4
#define LUMP_LEAFSURFACES   5
#define LUMP_LEAFBRUSHES    6
#define LUMP_MODELS         7
#define LUMP_BRUSHES        8
#define LUMP_BRUSHSIDES     9
#define LUMP_DRAWVERTS      10
#define LUMP_DRAWINDEXES    11
#define LUMP_FOGS           12
#define LUMP_SURFACES       13
#define LUMP_LIGHTMAPS      14
#define LUMP_LIGHTGRID      15
#define LUMP_VISIBILITY     16
#define LUMP_LIGHTARRAY     17
#define HEADER_LUMPS        18


/* types */
struct rbspHeader_t
{
	char ident[ 4 ];
	int version;

	bspLump_t lumps[ HEADER_LUMPS ];
};



/* light grid */
#define MAX_MAP_GRID        0xffff
#define MAX_MAP_GRIDARRAY   0x100000
#define LG_EPSILON          4


static void CopyLightGridLumps( rbspHeader_t *header ){
	std::vector<bspGridPoint_t> gridPoints;
	std::vector<unsigned short> gridArray;
	CopyLump( (bspHeader_t*) header, LUMP_LIGHTGRID, gridPoints );
	CopyLump( (bspHeader_t*) header, LUMP_LIGHTARRAY, gridArray );

	bspGridPoints.clear();
	bspGridPoints.reserve( gridArray.size() );

	for( const auto id : gridArray )
		bspGridPoints.push_back( gridPoints[ id ] );
}


static void AddLightGridLumps( FILE *file, rbspHeader_t& header ){
	/* allocate temporary buffers */
	const size_t maxGridPoints = std::min( bspGridPoints.size(), size_t( MAX_MAP_GRID ) );
	std::vector<bspGridPoint_t> gridPoints;
	std::vector<unsigned short> gridArray( bspGridPoints.size() );

	/* BehavEd stage 7 (speed, same result): the twin is the FIRST stored point within LG_EPSILON.
	   NRC walked the whole list for every grid point (kejim_post: 496800 points, 2 s on every
	   write of the bsp, also after each bounce). A twin needs the same styles and ambient[ 0 ]
	   within LG_EPSILON per channel, so it lies in the same or a neighbouring bucket of width
	   LG_EPSILON + 1; the smallest matching index over those buckets is NRC's first match. */
	const auto twin = []( const bspGridPoint_t& in, const bspGridPoint_t& out ){
		/* compare styles */
		if ( in.styles != out.styles ) {
			return false;
		}
		/* compare direction */
		if ( const int d = abs( in.latLong[ 0 ] - out.latLong[ 0 ] );
			d < ( 255 - LG_EPSILON ) && d > LG_EPSILON ) {
			return false;
		}
		if ( const int d = abs( in.latLong[ 1 ] - out.latLong[ 1 ] );
			d < 255 - LG_EPSILON && d > LG_EPSILON ) {
			return false;
		}
		/* compare light */
		for ( int k = 0; k < MAX_LIGHTMAPS; ++k )
			for ( int c = 0; c < 3; ++c )
				if ( abs( (int) in.ambient [ k ][ c ] - (int) out.ambient [ k ][ c ] ) > LG_EPSILON ||
				     abs( (int) in.directed[ k ][ c ] - (int) out.directed[ k ][ c ] ) > LG_EPSILON ) {
					return false;
				}
		return true;
	};
	/* bucket of a byte value: a twin differs by at most LG_EPSILON, so it lies in the same or a
	   neighbouring bucket */
	const auto eimerVon = []( int v ){ return v / ( LG_EPSILON + 1 ) + 1; };
	/* outer key: styles and the ambient[ 0 ] buckets; inner key: the directed[ 0 ] buckets */
	const auto aussen = [&]( const bspGridPoint_t& p, int dr, int dg, int db ){
		std::uint64_t s = 0;
		for ( int k = 0; k < MAX_LIGHTMAPS; ++k )
			s = ( s << 8 ) | p.styles[ k ];
		s = ( s << 8 ) | std::uint8_t( eimerVon( p.ambient[ 0 ][ 0 ] ) + dr );
		s = ( s << 8 ) | std::uint8_t( eimerVon( p.ambient[ 0 ][ 1 ] ) + dg );
		s = ( s << 8 ) | std::uint8_t( eimerVon( p.ambient[ 0 ][ 2 ] ) + db );
		return s;
	};
	const auto innen = [&]( const bspGridPoint_t& p, int dr, int dg, int db ){
		return ( unsigned( eimerVon( p.directed[ 0 ][ 0 ] ) + dr ) << 16 ) |
		       ( unsigned( eimerVon( p.directed[ 0 ][ 1 ] ) + dg ) << 8 ) |
		         unsigned( eimerVon( p.directed[ 0 ][ 2 ] ) + db );
	};
	std::unordered_map<std::uint64_t, std::unordered_map<unsigned, std::vector<unsigned>>> eimer;
	/* the same bytes give the same twin: the entries before the earlier result did not match
	   then and still do not, the earlier result still does (it is the point itself when it was
	   added) */
	struct Gleich
	{
		std::size_t operator()( const bspGridPoint_t& p ) const {
			std::size_t h = 1469598103934665603ULL;
			const byte *b = reinterpret_cast<const byte*>( &p );
			for ( std::size_t i = 0; i < sizeof( p ); ++i )
				h = ( h ^ b[ i ] ) * 1099511628211ULL;
			return h;
		}
		bool operator()( const bspGridPoint_t& a, const bspGridPoint_t& b ) const {
			return memcmp( &a, &b, sizeof( a ) ) == 0;
		}
	};
	std::unordered_map<bspGridPoint_t, std::size_t, Gleich, Gleich> schonGesehen;

	/* for each bsp grid point, find an approximate twin */
	Sys_Printf( "Storing lightgrid: %zu points\n", bspGridPoints.size() );
	for ( size_t i = 0; i < gridArray.size(); ++i )
	{
		/* get points */
		const bspGridPoint_t& in = bspGridPoints[ i ];

		size_t j = gridPoints.size();
		if ( const auto gesehen = schonGesehen.find( in ); gesehen != schonGesehen.end() ) {
			j = gesehen->second;
		}
		else
		{
			for ( int ar = -1; ar <= 1; ++ar )
				for ( int ag = -1; ag <= 1; ++ag )
					for ( int ab = -1; ab <= 1; ++ab )
					{
						const auto au = eimer.find( aussen( in, ar, ag, ab ) );
						if ( au == eimer.end() ) {
							continue;
						}
						for ( int dr = -1; dr <= 1; ++dr )
							for ( int dg = -1; dg <= 1; ++dg )
								for ( int db = -1; db <= 1; ++db )
								{
									const auto it = au->second.find( innen( in, dr, dg, db ) );
									if ( it == au->second.end() ) {
										continue;
									}
									for ( const unsigned k : it->second ) /* ascending */
									{
										if ( k >= j ) {
											break;
										}
										if ( twin( in, gridPoints[ k ] ) ) {
											j = k;
											break;
										}
									}
								}
					}
			schonGesehen.emplace( in, j );
		}

		/* set sample index */
		gridArray[ i ] = (unsigned short) j;

		/* if no sample found, add a new one */
		if ( j >= gridPoints.size() && gridPoints.size() < maxGridPoints ) {
			eimer[ aussen( in, 0, 0, 0 ) ][ innen( in, 0, 0, 0 ) ].push_back( unsigned( gridPoints.size() ) );
			gridPoints.push_back( in );
		}
	}

	/* swap array */
	for ( auto&& a : gridArray )
		a = LittleShort( a );

	/* write lumps */
	AddLump( file, header.lumps[LUMP_LIGHTGRID], gridPoints );
	AddLump( file, header.lumps[LUMP_LIGHTARRAY], gridArray );
}



/*
   LoadRBSPFile()
   loads a raven bsp file into memory
 */

void LoadRBSPFile( const char *filename ){
	/* load the file */
	MemBuffer file = LoadFile( filename );

	rbspHeader_t    *header = file.data();

	/* swap the header (except the first 4 bytes) */
	SwapBlock( (int*) ( (byte*) header + 4 ), sizeof( *header ) - 4 );

	/* make sure it matches the format we're trying to load */
	if ( !force && memcmp( header->ident, g_game->bspIdent, 4 ) ) {
		Error( "%s is not a %s file", filename, g_game->bspIdent );
	}
	if ( !force && header->version != g_game->bspVersion ) {
		Error( "%s is version %d, not %d", filename, header->version, g_game->bspVersion );
	}

	/* load/convert lumps */
	CopyLump( (bspHeader_t*) header, LUMP_SHADERS, bspShaders );
	CopyLump( (bspHeader_t*) header, LUMP_MODELS, bspModels );
	CopyLump( (bspHeader_t*) header, LUMP_PLANES, bspPlanes );
	CopyLump( (bspHeader_t*) header, LUMP_LEAFS, bspLeafs );
	CopyLump( (bspHeader_t*) header, LUMP_NODES, bspNodes );
	CopyLump( (bspHeader_t*) header, LUMP_LEAFSURFACES, bspLeafSurfaces );
	CopyLump( (bspHeader_t*) header, LUMP_LEAFBRUSHES, bspLeafBrushes );
	CopyLump( (bspHeader_t*) header, LUMP_BRUSHES, bspBrushes );
	CopyLump( (bspHeader_t*) header, LUMP_BRUSHSIDES, bspBrushSides );
	CopyLump( (bspHeader_t*) header, LUMP_DRAWVERTS, bspDrawVerts );
	CopyLump( (bspHeader_t*) header, LUMP_SURFACES, bspDrawSurfaces );
	CopyLump( (bspHeader_t*) header, LUMP_FOGS, bspFogs );
	CopyLump( (bspHeader_t*) header, LUMP_DRAWINDEXES, bspDrawIndexes );
	CopyLump( (bspHeader_t*) header, LUMP_VISIBILITY, bspVisBytes );
	CopyLump( (bspHeader_t*) header, LUMP_LIGHTMAPS, bspLightBytes );
	CopyLump( (bspHeader_t*) header, LUMP_ENTITIES, bspEntData );
	CopyLightGridLumps( header );
}



/*
   WriteRBSPFile()
   writes a raven bsp file
 */

void WriteRBSPFile( const char *filename ){
	rbspHeader_t header{};

	//%	Swapfile();

	/* set up header */
	memcpy( header.ident, g_game->bspIdent, 4 );
	header.version = LittleLong( g_game->bspVersion );

	/* write initial header */
	FILE *file = SafeOpenWrite( filename );
	SafeWrite( file, &header, sizeof( header ) );    /* overwritten later */

	{ /* add marker lump */
		time_t t;
		time( &t );
		/* asctime adds an implicit trailing \n */
		const auto marker = StringStream( "I LOVE MY Q3MAP2 " Q3MAP_VERSION " on ", asctime( localtime( &t ) ) );
		AddLump( file, header.lumps[0], std::vector<char>( marker.cbegin(), marker.cend() + 1 ) );
	}

	/* add lumps */
	AddLump( file, header.lumps[LUMP_SHADERS], bspShaders );
	AddLump( file, header.lumps[LUMP_PLANES], bspPlanes );
	AddLump( file, header.lumps[LUMP_LEAFS], bspLeafs );
	AddLump( file, header.lumps[LUMP_NODES], bspNodes );
	AddLump( file, header.lumps[LUMP_BRUSHES], bspBrushes );
	AddLump( file, header.lumps[LUMP_BRUSHSIDES], bspBrushSides );
	AddLump( file, header.lumps[LUMP_LEAFSURFACES], bspLeafSurfaces );
	AddLump( file, header.lumps[LUMP_LEAFBRUSHES], bspLeafBrushes );
	AddLump( file, header.lumps[LUMP_MODELS], bspModels );
	AddLump( file, header.lumps[LUMP_DRAWVERTS], bspDrawVerts );
	AddLump( file, header.lumps[LUMP_SURFACES], bspDrawSurfaces );
	AddLump( file, header.lumps[LUMP_VISIBILITY], bspVisBytes );
	AddLump( file, header.lumps[LUMP_LIGHTMAPS], bspLightBytes );
	AddLightGridLumps( file, header );
	AddLump( file, header.lumps[LUMP_ENTITIES], bspEntData );
	AddLump( file, header.lumps[LUMP_FOGS], bspFogs );
	AddLump( file, header.lumps[LUMP_DRAWINDEXES], bspDrawIndexes );

	/* emit bsp size */
	const int size = ftell( file );
	Sys_Printf( "Wrote %.1f MB (%d bytes)\n", (float) size / ( 1024 * 1024 ), size );

	/* write the completed header */
	fseek( file, 0, SEEK_SET );
	SafeWrite( file, &header, sizeof( header ) );

	/* close the file */
	fclose( file );
}
