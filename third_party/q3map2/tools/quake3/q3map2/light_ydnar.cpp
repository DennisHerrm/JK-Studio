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
#include "bhed_gpuschatten.h" /* BehavEd stage 8b: -bhedgpu */
#include "bhed_gpubacken.h" /* BehavEd stage 8c: -bhedgpubake */
#include <atomic>
#include <unordered_map>
#include "bhed_lichtcache.h" /* BehavEd stage 8a: -bhedlightcache */
#include "timer.h"
#include "bhed_parallel.h" /* BehavEd stage 7: src/q3map2/kompat, parallel rows of big lightmaps */
#include <mutex> /* BehavEd stage 8g */
#include <array> /* BehavEd stage 8g */
#include <limits> /* BehavEd stage 8g */
#include <chrono> /* BehavEd stage 8g */


// http://www.graficaobscura.com/matrix/index.html
static void color_saturate( Vector3& color, float saturation ){
	/* This is the luminance vector. Notice here that we do not use the standard NTSC weights of 0.299, 0.587, and 0.114.
	The NTSC weights are only applicable to RGB colors in a gamma 2.2 color space. For linear RGB colors these values are better. */
	const Vector3 rgb2gray( 0.3086, 0.6094, 0.0820 );
	Matrix4 tra( g_matrix4_identity );
	tra.x().vec3().set( rgb2gray.x() * ( 1 - saturation ) );
	tra.y().vec3().set( rgb2gray.y() * ( 1 - saturation ) );
	tra.z().vec3().set( rgb2gray.z() * ( 1 - saturation ) );
	tra.xx() += saturation;
	tra.yy() += saturation;
	tra.zz() += saturation;
	matrix4_transform_direction( tra, color );
}



/*
   ColorToBytes()
   ydnar: moved to here 2001-02-04
 */
/*
	Standard pipeline is Exposure->Brightness->Contrast->Gamma->Saturation->Clamp
	lightmapExposure is something special here, doing clamp/bump, god knows what's proper order for it
	lmscale, lightmapCompensate are applied last, since they try to emulate engine effects (bitshift, clamp)
*/
Vector3b ColorToBytes( const Vector3& color, float scale/* = 1*/, float lmscale/* = 1*/ ){
	/* user inputs are not range checked, sanitize here... */
	if ( scale <= 0 ) scale = 1;
	if ( lmscale <= 0 ) lmscale = 1;

	/* make a local copy */
	Vector3 sample = color * scale; // apply brightness

	/* contrast */
	if ( lightmapContrast != 1 )
		for ( int i = 0; i < 3; ++i )
			sample[i] = lightmapContrast * ( sample[i] - 128 ) + 128;

	/* clamp negative light (required by pow() in gamma) */
	for ( int i = 0; i < 3; ++i )
		value_maximize( sample[ i ], 0.f );

	/* gamma */
	if( lightmapGamma != 1 ){
		const float gamma = 1.0f / lightmapGamma;
		for ( int i = 0; i < 3; ++i )
			sample[ i ] = std::pow( sample[ i ] / 255.0f, gamma ) * 255.0f;
	}

	/* Exposure */
	if ( lightmapExposure != 0 ) {
		if( const float max = vector3_max_component( sample ); max > 0 ){
			const float dif = 1 - std::exp( -max / lightmapExposure );
			sample *= ( dif * 255.0f / max );
		}
	}

	if( g_lightmapSaturation != 1 ){
		color_saturate( sample, g_lightmapSaturation );
		/* clamp negative light possibly produced here */
		for ( int i = 0; i < 3; ++i )
			value_maximize( sample[ i ], 0.f );
	}

	/* lmscale is necessary for simulating r_mapOverbrightBits on external lightmaps */
	/* compensate for ingame overbrighting/bitshifting */
	sample *= ( lmscale / lightmapCompensate );

	/* clamp with color normalization */
	if ( const float max = vector3_max_component( sample ); max > maxLight )
		sample *= ( maxLight / max );

	/* sRGB lightmaps */
	if ( lightmapsRGB ) {
		sample[0] = floor( Image_sRGBFloatFromLinearFloat( sample[0] * ( 1.0 / 255.0 ) ) * 255.0 + 0.5 );
		sample[1] = floor( Image_sRGBFloatFromLinearFloat( sample[1] * ( 1.0 / 255.0 ) ) * 255.0 + 0.5 );
		sample[2] = floor( Image_sRGBFloatFromLinearFloat( sample[2] * ( 1.0 / 255.0 ) ) * 255.0 + 0.5 );
	}

	/* store it off */
	return sample;
}



/* -------------------------------------------------------------------------------

   this section deals with phong shading (normal interpolation across brush faces)

   ------------------------------------------------------------------------------- */

/*
   SmoothNormals()
   smooths together coincident vertex normals across the bsp
 */

#define MAX_SAMPLES             256
#define THETA_EPSILON           0.000001
#define EQUAL_NORMAL_EPSILON    0.01f

void SmoothNormals(){
	float shadeAngle, defaultShadeAngle, maxShadeAngle;
	int indexes[ MAX_SAMPLES ];
	Vector3 votes[ MAX_SAMPLES ];
	const int numBSPDrawVerts = bspDrawVerts.size();


	/* allocate shade angle table */
	std::vector<float> shadeAngles( numBSPDrawVerts, 0 );

	/* allocate smoothed table */
	std::vector<std::uint8_t> smoothed( numBSPDrawVerts, false );

	/* set default shade angle */
	defaultShadeAngle = degrees_to_radians( shadeAngleDegrees );
	maxShadeAngle = 0;

	/* run through every surface and flag verts belonging to non-lightmapped surfaces
	   and set per-vertex smoothing angle */
	for ( size_t i = 0; i < bspDrawSurfaces.size(); ++i )
	{
		/* get drawsurf */
		bspDrawSurface_t& ds = bspDrawSurfaces[ i ];

		/* get shader for shade angle */
		const shaderInfo_t *si = surfaceInfos[ i ].si;
		if ( si->shadeAngleDegrees ) {
			shadeAngle = degrees_to_radians( si->shadeAngleDegrees );
		}
		else{
			shadeAngle = defaultShadeAngle;
		}
		value_maximize( maxShadeAngle, shadeAngle );

		/* flag its verts */
		for ( int j = 0; j < ds.numVerts; ++j )
		{
			const int f = ds.firstVert + j;
			shadeAngles[ f ] = shadeAngle;
			if ( ds.surfaceType == MST_TRIANGLE_SOUP ) {
				smoothed[ f ] = true;
			}
		}
	}

	/* bail if no surfaces have a shade angle */
	if ( maxShadeAngle == 0 ) {
		return;
	}

	/* init pacifier */
	Pacifier pacifier( numBSPDrawVerts );
	Timer timer;

	/* go through the list of vertexes */
	for ( int i = 0; i < numBSPDrawVerts; ++i )
	{
		/* print pacifier */
		++pacifier;

		/* already smoothed? */
		if ( smoothed[ i ] ) {
			continue;
		}

		/* clear */
		Vector3 average( 0 );
		int numVerts = 0;
		int numVotes = 0;

		/* build a table of coincident vertexes */
		for ( int j = i; j < numBSPDrawVerts && numVerts < MAX_SAMPLES; ++j )
		{
			/* already smoothed? */
			if ( smoothed[ j ] ) {
				continue;
			}

			/* test vertexes */
			if ( !VectorCompare( yDrawVerts[ i ].xyz, yDrawVerts[ j ].xyz ) ) {
				continue;
			}

			/* use smallest shade angle */
			shadeAngle = std::min( shadeAngles[ i ], shadeAngles[ j ] );

			/* check shade angle */
			const double dot = std::clamp( vector3_dot( bspDrawVerts[ i ].normal, bspDrawVerts[ j ].normal ), -1.0, 1.0 );
			if ( acos( dot ) + THETA_EPSILON >= shadeAngle ) {
				//Sys_Printf( "F(%3.3f >= %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );
				continue;
			}
			//Sys_Printf( "P(%3.3f < %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );

			/* add to the list */
			indexes[ numVerts++ ] = j;

			/* flag vertex */
			smoothed[ j ] = true;

			/* see if this normal has already been voted */
			int k;
			for ( k = 0; k < numVotes; ++k )
			{
				if ( vector3_equal_epsilon( bspDrawVerts[ j ].normal, votes[ k ], EQUAL_NORMAL_EPSILON ) ) {
					break;
				}
			}

			/* add a new vote? */
			if ( k == numVotes && numVotes < MAX_SAMPLES ) {
				average += bspDrawVerts[ j ].normal;
				votes[ numVotes ] = bspDrawVerts[ j ].normal;
				numVotes++;
			}
		}

		/* don't average for less than 2 verts */
		if ( numVerts < 2 ) {
			continue;
		}

		/* average normal */
		if ( VectorNormalize( average ) != 0 ) {
			/* smooth */
			for ( int j = 0; j < numVerts; ++j )
				yDrawVerts[ indexes[ j ] ].normal = average;
		}
	}

	/* print time */
	Sys_Printf( " (%i)\n", int( timer.elapsed_sec() ) );
}



/* ------------------------------------------------------------------------------- */

/*
   ClusterVisible()
   determines if two clusters are visible to each other using the PVS
 */

bool ClusterVisible( int a, int b ){
	/* dummy check */
	if ( a < CLUSTER_NORMAL || b < CLUSTER_NORMAL ) {
		return false;
	}

	/* early out */
	if ( a == b ) {
		return true;
	}

	/* not vised? */
	if ( bspVisBytes.size() <= 8 ) {
		return true;
	}

	/* get pvs data */
	/* portalClusters = ( (int *) bspVisBytes )[ 0 ]; */
	const int leafBytes = ( (int*) bspVisBytes.data() )[ 1 ];
	const byte *pvs = bspVisBytes.data() + VIS_HEADER_SIZE + ( a * leafBytes );

	/* check */
	return bit_is_enabled( pvs, b );
}



/*
   PointInLeafNum_r()
   borrowed from vlight.c
 */

static int PointInLeafNum_r( const Vector3& point, int nodenum ){
	int leafnum;

	while ( nodenum >= 0 )
	{
		const bspNode_t& node = bspNodes[ nodenum ];
		const bspPlane_t& plane = bspPlanes[ node.planeNum ];
		const double dist = plane3_distance_to_point( plane, point );
		if ( dist > 0.1 ) {
			nodenum = node.children[eFront];
		}
		else if ( dist < -0.1 ) {
			nodenum = node.children[eBack];
		}
		else
		{
			leafnum = PointInLeafNum_r( point, node.children[eFront] );
			if ( bspLeafs[ leafnum ].cluster != CLUSTER_OPAQUE ) {
				return leafnum;
			}
			nodenum = node.children[eBack];
		}
	}

	leafnum = -nodenum - 1;
	return leafnum;
}



/*
   PointInLeafnum()
   borrowed from vlight.c
 */

static int PointInLeafNum( const Vector3& point ){
	return PointInLeafNum_r( point, 0 );
}



/*
   ClusterForPoint() - ydnar
   returns the pvs cluster for point
 */

static int ClusterForPoint( const Vector3& point ){
	/* get leafNum for point */
	const int leafNum = PointInLeafNum( point );
	if ( leafNum < 0 ) {
		return CLUSTER_OPAQUE;
	}

	/* return the cluster */
	return bspLeafs[ leafNum ].cluster;
}



/*
   ClusterVisibleToPoint() - ydnar
   returns true if point can "see" cluster
 */

static bool ClusterVisibleToPoint( const Vector3& point, int cluster ){
	/* get leafNum for point */
	const int pointCluster = ClusterForPoint( point );
	if ( pointCluster <= CLUSTER_OPAQUE ) {
		return false;
	}

	/* check pvs */
	return ClusterVisible( pointCluster, cluster );
}



/*
   ClusterForPointExt() - ydnar
   also takes brushes into account for occlusion testing
 */

int ClusterForPointExt( const Vector3& point, float epsilon ){
	/* get leaf for point */
	const int leafNum = PointInLeafNum( point );
	if ( leafNum < 0 ) {
		return CLUSTER_OPAQUE;
	}
	const bspLeaf_t& leaf = bspLeafs[ leafNum ];

	/* get the cluster */
	const int cluster = leaf.cluster;
	if ( cluster <= CLUSTER_OPAQUE ) {
		return CLUSTER_OPAQUE;
	}

	/* transparent leaf, so check point against all brushes in the leaf */
	for ( const int b : Span( &bspLeafBrushes[ leaf.firstBSPLeafBrush ], leaf.numBSPLeafBrushes ) )
	{
		if ( b > maxOpaqueBrush ) {
			continue;
		}
		if ( !opaqueBrushes[ b ] ) {
			continue;
		}

		const bspBrush_t& brush = bspBrushes[ b ];
		/* check point against all planes */
		bool inside = true;
		for ( const bspBrushSide_t& side : Span( &bspBrushSides[ brush.firstSide ], brush.numSides ) )
		{
			const bspPlane_t& plane = bspPlanes[ side.planeNum ];
			if ( plane3_distance_to_point( plane, point ) > epsilon ) {
				inside = false;
				break;
			}
		}

		/* if inside, return bogus cluster */
		if ( inside ) {
			return CLUSTER_OPAQUE - b;
		}
	}

	/* if the point made it this far, it's not inside any opaque brushes */
	return cluster;
}



/*
   ClusterForPointExtFilter() - ydnar
   adds cluster checking against a list of known valid clusters
 */

static int ClusterForPointExtFilter( const Vector3& point, float epsilon, int numClusters, int *clusters ){
	/* get cluster for point */
	const int cluster = ClusterForPointExt( point, epsilon );

	/* check if filtering is necessary */
	if ( cluster <= CLUSTER_OPAQUE || numClusters <= 0 || clusters == nullptr ) {
		return cluster;
	}

	/* filter */
	for ( int i = 0; i < numClusters; ++i )
	{
		if ( cluster == clusters[ i ] || ClusterVisible( cluster, clusters[ i ] ) ) {
			return cluster;
		}
	}

	/* failed */
	return CLUSTER_OPAQUE;
}



/*
   ShaderForPointInLeaf() - ydnar
   checks a point against all brushes in a leaf, returning the shader of the brush
   also sets the cumulative surface and content flags for the brush hit
 */

static int ShaderForPointInLeaf( const Vector3& point, int leafNum, float epsilon, int wantContentFlags, int wantSurfaceFlags, int *contentFlags, int *surfaceFlags ){
	int allSurfaceFlags, allContentFlags;


	/* clear things out first */
	*surfaceFlags = 0;
	*contentFlags = 0;

	/* get leaf */
	if ( leafNum < 0 ) {
		return -1;
	}
	const bspLeaf_t& leaf = bspLeafs[ leafNum ];

	/* transparent leaf, so check point against all brushes in the leaf */
	for ( const int b : Span( &bspLeafBrushes[ leaf.firstBSPLeafBrush ], leaf.numBSPLeafBrushes ) )
	{
		/* get parts */
		const bspBrush_t& brush = bspBrushes[ b ];

		/* check point against all planes */
		bool inside = true;
		allSurfaceFlags = 0;
		allContentFlags = 0;
		for ( const bspBrushSide_t& side : Span( &bspBrushSides[ brush.firstSide ], brush.numSides ) )
		{
			const bspPlane_t& plane = bspPlanes[ side.planeNum ];
			if ( plane3_distance_to_point( plane, point ) > epsilon ) {
				inside = false;
				break;
			}
			else
			{
				const bspShader_t& shader = bspShaders[ side.shaderNum ];
				allSurfaceFlags |= shader.surfaceFlags;
				allContentFlags |= shader.contentFlags;
			}
		}

		/* handle if inside */
		if ( inside ) {
			/* if there are desired flags, check for same and continue if they aren't matched */
			if ( wantContentFlags && !( wantContentFlags & allContentFlags ) ) {
				continue;
			}
			if ( wantSurfaceFlags && !( wantSurfaceFlags & allSurfaceFlags ) ) {
				continue;
			}

			/* store the cumulative flags and return the brush shader (which is mostly useless) */
			*surfaceFlags = allSurfaceFlags;
			*contentFlags = allContentFlags;
			return brush.shaderNum;
		}
	}

	/* if the point made it this far, it's not inside any brushes */
	return -1;
}



/* -------------------------------------------------------------------------------

   this section deals with phong shaded lightmap tracing

   ------------------------------------------------------------------------------- */

/* 9th rewrite (recursive subdivision of a lightmap triangle) */

/*
   CalcTangentVectors()
   calculates the st tangent vectors for normalmapping
 */

template<std::size_t numVerts>
bool CalcTangentVectors( const std::array<const bspDrawVert_t *, numVerts>& dv, Vector3 (&stv)[numVerts], Vector3 (&ttv)[numVerts] ){
	float bb, s, t;
	Vector3 bary;


	/* calculate barycentric basis for the triangle */
	bb = ( dv[ 1 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ] ) * ( dv[ 2 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ] ) - ( dv[ 2 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ] ) * ( dv[ 1 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ] );
	if ( std::fabs( bb ) < 0.00000001f ) {
		return false;
	}

	/* do each vertex */
	for ( std::size_t i = 0; i < numVerts; ++i )
	{
		/* calculate s tangent vector */
		s = dv[ i ]->st[ 0 ] + 10.0f;
		t = dv[ i ]->st[ 1 ];
		bary[ 0 ] = ( ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) - ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) ) / bb;
		bary[ 1 ] = ( ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) - ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) ) / bb;
		bary[ 2 ] = ( ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) - ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) ) / bb;

		stv[ i ][ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		stv[ i ][ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		stv[ i ][ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];

		stv[ i ] -= dv[ i ]->xyz;
		VectorNormalize( stv[ i ] );

		/* calculate t tangent vector */
		s = dv[ i ]->st[ 0 ];
		t = dv[ i ]->st[ 1 ] + 10.0f;
		bary[ 0 ] = ( ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) - ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) ) / bb;
		bary[ 1 ] = ( ( dv[ 2 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) - ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 2 ]->st[ 1 ] - t ) ) / bb;
		bary[ 2 ] = ( ( dv[ 0 ]->st[ 0 ] - s ) * ( dv[ 1 ]->st[ 1 ] - t ) - ( dv[ 1 ]->st[ 0 ] - s ) * ( dv[ 0 ]->st[ 1 ] - t ) ) / bb;

		ttv[ i ][ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		ttv[ i ][ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		ttv[ i ][ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];

		ttv[ i ] -= dv[ i ]->xyz;
		VectorNormalize( ttv[ i ] );

		/* debug code */
		//%	Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%		stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}

	/* return to caller */
	return true;
}




/*
   PerturbNormal()
   perterbs the normal by the shader's normalmap in tangent space
 */

static void PerturbNormal( const bspDrawVert_t& dv, const shaderInfo_t *si, Vector3& pNormal, const Vector3 stv[ 3 ], const Vector3 ttv[ 3 ] ){
	/* passthrough */
	pNormal = dv.normal;

	/* sample normalmap */
	Color4f bump;
	if ( !RadSampleImage( si->normalImage->pixels, si->normalImage->width, si->normalImage->height, dv.st, bump ) ) {
		return;
	}

	/* remap sampled normal from [0,255] to [-1,-1] */
	bump.rgb() = ( bump.rgb() - Vector3( 127.0f ) ) * ( 1.0f / 127.5f );

	/* scale tangent vectors and add to original normal */
	pNormal = dv.normal + stv[ 0 ] * bump[ 0 ] + ttv[ 0 ] * bump[ 1 ] + dv.normal * bump[ 2 ];

	/* renormalize and return */
	VectorNormalize( pNormal );
}



/*
   MapSingleLuxel()
   maps a luxel for triangle bv at
 */

#define NUDGE           0.5f
#define BOGUS_NUDGE     -99999.0f

static int MapSingleLuxel( rawLightmap_t *lm, const surfaceInfo_t *info, const bspDrawVert_t& dv, const Plane3f* plane, float pass, const Vector3 stv[ 3 ], const Vector3 ttv[ 3 ], const Vector3 worldverts[ 3 ] ){
	int i, numClusters, *clusters, pointCluster;
	float           lightmapSampleOffset;
	const shaderInfo_t    *si;
	Vector3 pNormal;
	Vector3 vecs[ 3 ];
	Vector3 nudged;
	Vector3 origintwo;
	int j;
	float           *nudge;
	static float nudges[][ 2 ] =
	{
		//%{ 0, 0 },		/* try center first */
		{ -NUDGE, 0 },                      /* left */
		{ NUDGE, 0 },                       /* right */
		{ 0, NUDGE },                       /* up */
		{ 0, -NUDGE },                      /* down */
		{ -NUDGE, NUDGE },                  /* left/up */
		{ NUDGE, -NUDGE },                  /* right/down */
		{ NUDGE, NUDGE },                   /* right/up */
		{ -NUDGE, -NUDGE },                 /* left/down */
		{ BOGUS_NUDGE, BOGUS_NUDGE }
	};


	/* find luxel xy coords (fixme: subtract 0.5?) */
	const int x = std::clamp( int( dv.lightmap[ 0 ][ 0 ] ), 0, lm->sw - 1 );
	const int y = std::clamp( int( dv.lightmap[ 0 ][ 1 ] ), 0, lm->sh - 1 );

	/* set shader and cluster list */
	if ( info != nullptr ) {
		si = info->si;
		numClusters = info->numSurfaceClusters;
		clusters = &surfaceClusters[ info->firstSurfaceCluster ];
	}
	else
	{
		si = nullptr;
		numClusters = 0;
		clusters = nullptr;
	}

	/* get luxel, origin, cluster, and normal */
	SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );
	Vector3& origin = lm->getSuperOrigin( x, y );
	Vector3& normal = lm->getSuperNormal( x, y );
	int& cluster = lm->getSuperCluster( x, y );

	/* don't attempt to remap occluded luxels for planar surfaces */
	if ( cluster == CLUSTER_OCCLUDED && lm->plane != nullptr ) {
		return cluster;
	}

	/* only average the normal for premapped luxels */
	else if ( cluster >= CLUSTER_NORMAL ) {
		/* do bumpmap calculations */
		if ( stv != nullptr ) {
			PerturbNormal( dv, si, pNormal, stv, ttv );
		}
		else{
			pNormal = dv.normal;
		}

		/* add the additional normal data */
		normal += pNormal;
		luxel.count += 1;
		return cluster;
	}

	/* otherwise, unmapped luxels (*cluster == CLUSTER_UNMAPPED) will have their full attributes calculated */

	/* get origin */

	/* axial lightmap projection */
	if ( lm->vecs != nullptr ) {
		/* calculate an origin for the sample from the lightmap vectors */
		origin = lm->origin;
		for ( i = 0; i < 3; ++i )
		{
			/* add unless it's the axis, which is taken care of later */
			if ( i == lm->axisNum ) {
				continue;
			}
			origin[ i ] += ( x * lm->vecs[ 0 ][ i ] ) + ( y * lm->vecs[ 1 ][ i ] );
		}

		/* project the origin onto the plane */
		origin[ lm->axisNum ] -= plane3_distance_to_point( *plane, origin ) / plane->normal()[ lm->axisNum ];
	}

	/* non axial lightmap projection (explicit xyz) */
	else{
		origin = dv.xyz;
	}

	//////////////////////
	//27's test to make sure samples stay within the triangle boundaries
	//1) Test the sample origin to see if it lays on the wrong side of any edge (x/y)
	//2) if it does, nudge it onto the correct side.

	if ( worldverts != nullptr && lightmapTriangleCheck ) {
		Plane3f hostplane;
		PlaneFromPoints( hostplane, worldverts );

		for ( j = 0; j < 3; ++j )
		{
			for ( i = 0; i < 3; ++i )
			{
				Plane3f sideplane;
				//build plane using 2 edges and a normal
				const int next = ( i + 1 ) % 3;
				PlaneFromPoints( sideplane, worldverts[i], worldverts[ next ], worldverts[ next ] + hostplane.normal() );

				//planetest sample point
				const float e = plane3_distance_to_point( sideplane, origin );
				if ( e > -LUXEL_EPSILON ) {
					//we're bad.
					//Move the sample point back inside triangle bounds
					origin -= sideplane.normal() * ( e + 1 );
#ifdef DEBUG_27_1
					origin.set( 0 );
#endif
				}
			}
		}
	}

	////////////////////////

	/* planar surfaces have precalculated lightmap vectors for nudging */
	if ( lm->plane != nullptr ) {
		vecs[ 0 ] = lm->vecs[ 0 ];
		vecs[ 1 ] = lm->vecs[ 1 ];
		vecs[ 2 ] = lm->plane->normal();
	}

	/* non-planar surfaces must calculate them */
	else
	{
		if ( plane != nullptr ) {
			vecs[ 2 ] = plane->normal();
		}
		else{
			vecs[ 2 ] = dv.normal;
		}
		MakeNormalVectors( vecs[ 2 ], vecs[ 0 ], vecs[ 1 ] );
	}

	/* push the origin off the surface a bit */
	if ( si != nullptr ) {
		lightmapSampleOffset = si->lightmapSampleOffset;
	}
	else{
		lightmapSampleOffset = DEFAULT_LIGHTMAP_SAMPLE_OFFSET;
	}
	if ( lm->axisNum < 0 ) {
		origin += vecs[ 2 ] * lightmapSampleOffset;
	}
	else if ( vecs[ 2 ][ lm->axisNum ] < 0 ) {
		origin[ lm->axisNum ] -= lightmapSampleOffset;
	}
	else{
		origin[ lm->axisNum ] += lightmapSampleOffset;
	}

	origintwo = origin;
	if ( lightmapExtraVisClusterNudge ) {
		origintwo += vecs[2];
	}

	/* get cluster */
	pointCluster = ClusterForPointExtFilter( origintwo, LUXEL_EPSILON, numClusters, clusters );

	/* another retarded hack, storing nudge count in luxel[ 1 ] */
	luxel.value[ 1 ] = 0;

	/* point in solid? (except in dark mode) */
	if ( pointCluster <= CLUSTER_OPAQUE && !dark ) {
		/* nudge the the location around */
		nudge = nudges[ 0 ];
		while ( nudge[ 0 ] > BOGUS_NUDGE && pointCluster <= CLUSTER_OPAQUE )
		{
			/* nudge the vector around a bit */
			/* set nudged point*/
			nudged = origintwo + vecs[ 0 ] * nudge[ 0 ] + vecs[ 1 ] * nudge[ 1 ];
			nudge += 2;

			/* get pvs cluster */
			pointCluster = ClusterForPointExtFilter( nudged, LUXEL_EPSILON, numClusters, clusters ); //% + 0.625 );
			if ( pointCluster > CLUSTER_OPAQUE ) {
				origin = nudged;
			}
			luxel.value[ 1 ] += 1;
		}
	}

	/* as a last resort, if still in solid, try drawvert origin offset by normal (except in dark mode) */
	if ( pointCluster <= CLUSTER_OPAQUE && si != nullptr && !dark ) {
		nudged = dv.xyz + dv.normal * lightmapSampleOffset;
		pointCluster = ClusterForPointExtFilter( nudged, LUXEL_EPSILON, numClusters, clusters );
		if ( pointCluster > CLUSTER_OPAQUE ) {
			origin = nudged;
		}
		luxel.value[ 1 ] += 1;
	}

	/* valid? */
	if ( pointCluster <= CLUSTER_OPAQUE ) {
		cluster = CLUSTER_OCCLUDED;
		origin.set( 0 );
		normal.set( 0 );
		numLuxelsOccluded++;
		return cluster;
	}

	/* debug code */
	//%	Sys_Printf( "%f %f %f\n", origin[ 0 ], origin[ 1 ], origin[ 2 ] );

	/* do bumpmap calculations */
	if ( stv ) {
		PerturbNormal( dv, si, pNormal, stv, ttv );
	}
	else{
		pNormal = dv.normal;
	}

	/* store the cluster and normal */
	cluster = pointCluster;
	normal = pNormal;

	/* store explicit mapping pass and implicit mapping pass */
	luxel.value[ 0 ] = pass;
	luxel.count = 1;

	/* add to count */
	numLuxelsMapped++;

	/* return ok */
	return cluster;
}



/*
   MapTriangle_r()
   recursively subdivides a triangle until its edges are shorter
   than the distance between two luxels (thanks jc :)
 */

static void MapTriangle_r( rawLightmap_t *lm, const surfaceInfo_t *info, const TriRef& tri, Plane3f *plane, const Vector3 stv[ 3 ], const Vector3 ttv[ 3 ], const Vector3 worldverts[ 3 ] ){
	/* map the vertexes */
	#if 0
	MapSingleLuxel( lm, info, *tri[ 0 ], plane, 1, stv, ttv );
	MapSingleLuxel( lm, info, *tri[ 1 ], plane, 1, stv, ttv );
	MapSingleLuxel( lm, info, *tri[ 2 ], plane, 1, stv, ttv );
	#endif

	/* subdivide calc */
	int max = -1;
	{
		/* find the longest edge and split it */
		float maxDist = 0;
		for ( int i = 0; i < 3; ++i )
		{
			/* get dist */
			const float dist = vector2_length_squared( tri[ i ]->lightmap[ 0 ] - tri[ ( i + 1 ) % 3 ]->lightmap[ 0 ] );
			/* longer? */
			if ( dist > maxDist ) {
				maxDist = dist;
				max = i;
			}
		}

		/* try to early out */
		if ( max < 0 || maxDist <= subdivideThreshold ) { /* ydnar: was i < 0 instead of max < 0 (?) */
			return;
		}
	}

	/* split the longest edge and map it */
	const bspDrawVert_t mid = LerpDrawVert( *tri[ max ], *tri[ ( max + 1 ) % 3 ] );
	MapSingleLuxel( lm, info, mid, plane, 1, stv, ttv, worldverts );

	/* push the point up a little bit to account for fp creep (fixme: revisit this) */
	//%	VectorMA( mid.xyz, 2.0f, mid.normal, mid.xyz );

	/* recurse to first triangle */
	TriRef tri2 = tri;
	tri2[ max ] = &mid;
	MapTriangle_r( lm, info, tri2, plane, stv, ttv, worldverts );

	/* recurse to second triangle */
	tri2 = tri;
	tri2[ ( max + 1 ) % 3 ] = &mid;
	MapTriangle_r( lm, info, tri2, plane, stv, ttv, worldverts );
}



/*
   MapTriangle()
   seed function for MapTriangle_r()
   requires a cw ordered triangle
 */

static bool MapTriangle( rawLightmap_t *lm, const surfaceInfo_t *info, const TriRef& tri, bool mapNonAxial ){
	Plane3f plane;
	/* get plane if possible */
	if ( lm->plane != nullptr ) {
		plane = *lm->plane;
	}
	/* otherwise make one from the points */
	else if ( !PlaneFromPoints( plane, tri[ 0 ]->xyz, tri[ 1 ]->xyz, tri[ 2 ]->xyz ) ) {
		return false;
	}

	/* this must not happen in the first place, but it does and spreads result of division by zero in MapSingleLuxel all over the map during -bounce */
	if( lm->vecs != nullptr && plane.normal()[lm->axisNum] == 0 ){
		Sys_Warning( "plane[lm->axisNum] == 0\n" );
		return false;
	}

	Vector3          *stv, *ttv, stvStatic[ 3 ], ttvStatic[ 3 ];
	/* check to see if we need to calculate texture->world tangent vectors */
	if ( info->si->normalImage != nullptr && CalcTangentVectors( tri, stvStatic, ttvStatic ) ) {
		stv = stvStatic;
		ttv = ttvStatic;
	}
	else
	{
		stv = nullptr;
		ttv = nullptr;
	}

	const Vector3 worldverts[ 3 ] = { tri[ 0 ]->xyz, tri[ 1 ]->xyz, tri[ 2 ]->xyz };

	/* map the vertexes */
	MapSingleLuxel( lm, info, *tri[ 0 ], &plane, 1, stv, ttv, worldverts );
	MapSingleLuxel( lm, info, *tri[ 1 ], &plane, 1, stv, ttv, worldverts );
	MapSingleLuxel( lm, info, *tri[ 2 ], &plane, 1, stv, ttv, worldverts );

	/* 2002-11-20: prefer axial triangle edges */
	if ( mapNonAxial ) {
		/* subdivide the triangle */
		MapTriangle_r( lm, info, tri, &plane, stv, ttv, worldverts );
		return true;
	}

	for ( int i = 0; i < 3; ++i )
	{
		/* get verts */
		const Vector2& a = tri[ i ]->lightmap[ 0 ];
		const Vector2& b = tri[ ( i + 1 ) % 3 ]->lightmap[ 0 ];

		/* make degenerate triangles for mapping edges */
		if ( float_equal_epsilon( a[ 0 ], b[ 0 ], 0.01f ) || float_equal_epsilon( a[ 1 ], b[ 1 ], 0.01f ) )
			/* map the degenerate triangle */
			MapTriangle_r( lm, info, TriRef{
				tri[ i ],
				tri[ ( i + 1 ) % 3 ],
				tri[ ( i + 1 ) % 3 ] }, &plane, stv, ttv, worldverts );
	}

	return true;
}



/*
   MapQuad_r()
   recursively subdivides a quad until its edges are shorter
   than the distance between two luxels
 */

static void MapQuad_r( rawLightmap_t *lm, const surfaceInfo_t *info, const QuadRef& quad, Plane3f *plane, const Vector3 stv[ 4 ], const Vector3 ttv[ 4 ] ){
	/* subdivide calc */
	int max = -1;
	{
		/* find the longest edge and split it */
		float maxDist = 0;
		for ( int i = 0; i < 4; ++i )
		{
			/* get dist */
			const float dist = vector2_length_squared( quad[ i ]->lightmap[ 0 ] - quad[ ( i + 1 ) % 4 ]->lightmap[ 0 ] );
			/* longer? */
			if ( dist > maxDist ) {
				maxDist = dist;
				max = i;
			}
		}

		/* try to early out */
		if ( max < 0 || maxDist <= subdivideThreshold ) {
			return;
		}
	}

	/* we only care about even/odd edges */
	max &= 1;

	/* split the longest edges */
	const bspDrawVert_t mid[ 2 ] = {
		LerpDrawVert( *quad[ max + 0 ], *quad[ ( max + 1 ) % 4 ] ),
		LerpDrawVert( *quad[ max + 2 ], *quad[ ( max + 3 ) % 4 ] ) };

	/* map the vertexes */
	MapSingleLuxel( lm, info, mid[ 0 ], plane, 1, stv, ttv, nullptr );
	MapSingleLuxel( lm, info, mid[ 1 ], plane, 1, stv, ttv, nullptr );

	/* 0 and 2 */
	if ( max == 0 ) {
		/* recurse to first quad */
		MapQuad_r( lm, info, QuadRef{ quad[ 0 ], &mid[ 0 ], &mid[ 1 ], quad[ 3 ] }, plane, stv, ttv );

		/* recurse to second quad */
		MapQuad_r( lm, info, QuadRef{ &mid[ 0 ], quad[ 1 ], quad[ 2 ], &mid[ 1 ] }, plane, stv, ttv );
	}

	/* 1 and 3 */
	else
	{
		/* recurse to first quad */
		MapQuad_r( lm, info, QuadRef{ quad[ 0 ], quad[ 1 ], &mid[ 0 ], &mid[ 1 ] }, plane, stv, ttv );

		/* recurse to second quad */
		MapQuad_r( lm, info, QuadRef{ &mid[ 1 ], &mid[ 0 ], quad[ 2 ], quad[ 3 ] }, plane, stv, ttv );
	}
}



/*
   MapQuad()
   seed function for MapQuad_r()
   requires a cw ordered triangle quad
 */

#define QUAD_PLANAR_EPSILON     0.5f

static bool MapQuad( rawLightmap_t *lm, const surfaceInfo_t *info, const QuadRef& quad ){
	Plane3f plane;
	/* get plane if possible */
	if ( lm->plane != nullptr ) {
		plane = *lm->plane;
	}
	/* otherwise make one from the points */
	else if ( !PlaneFromPoints( plane, quad[ 0 ]->xyz, quad[ 1 ]->xyz, quad[ 2 ]->xyz ) ) {
		return false;
	}

	/* 4th point must fall on the plane */
	if ( std::fabs( plane3_distance_to_point( plane, quad[ 3 ]->xyz ) ) > QUAD_PLANAR_EPSILON ) {
		return false;
	}

	Vector3          *stv, *ttv, stvStatic[ 4 ], ttvStatic[ 4 ];
	/* check to see if we need to calculate texture->world tangent vectors */
	if ( info->si->normalImage != nullptr && CalcTangentVectors( quad, stvStatic, ttvStatic ) ) {
		stv = stvStatic;
		ttv = ttvStatic;
	}
	else
	{
		stv = nullptr;
		ttv = nullptr;
	}

	/* map the vertexes */
	MapSingleLuxel( lm, info, *quad[ 0 ], &plane, 1, stv, ttv, nullptr );
	MapSingleLuxel( lm, info, *quad[ 1 ], &plane, 1, stv, ttv, nullptr );
	MapSingleLuxel( lm, info, *quad[ 2 ], &plane, 1, stv, ttv, nullptr );
	MapSingleLuxel( lm, info, *quad[ 3 ], &plane, 1, stv, ttv, nullptr );

	/* subdivide the quad */
	MapQuad_r( lm, info, quad, &plane, stv, ttv );
	return true;
}



/*
   MapRawLightmap()
   maps the locations, normals, and pvs clusters for a raw lightmap
 */

void MapRawLightmap( int rawLightmapNum ){
	int n, i, x, y, sx, sy;
	float               samples, radius, pass;
	rawLightmap_t       *lm;
	bspDrawVert_t fake;


	/* bail if this number exceeds the number of raw lightmaps */
	if ( rawLightmapNum >= numRawLightmaps ) {
		return;
	}

	/* get lightmap */
	lm = &rawLightmaps[ rawLightmapNum ];

	/* -----------------------------------------------------------------
	   map referenced surfaces onto the raw lightmap
	   ----------------------------------------------------------------- */

	/* walk the list of surfaces on this raw lightmap */
	for ( n = 0; n < lm->numLightSurfaces; ++n )
	{
		/* with > 1 surface per raw lightmap, clear occluded */
		if ( n > 0 ) {
			for ( y = 0; y < lm->sh; ++y )
			{
				for ( x = 0; x < lm->sw; ++x )
				{
					/* get cluster */
					int& cluster = lm->getSuperCluster( x, y );
					if ( cluster < CLUSTER_NORMAL ) {
						cluster = CLUSTER_UNMAPPED;
					}
				}
			}
		}

		/* get surface */
		const int num = lightSurfaces[ lm->firstLightSurface + n ];
		const bspDrawSurface_t& ds = bspDrawSurfaces[ num ];
		const surfaceInfo_t *info = &surfaceInfos[ num ];

		/* bail if no lightmap to calculate */
		if ( info->lm != lm ) {
			Sys_Printf( "!" );
			continue;
		}

		/* map the surface onto the lightmap origin/cluster/normal buffers */
		switch ( ds.surfaceType )
		{
		case MST_PLANAR:
		{
			/* get verts */
			const bspDrawVert_t *verts = &yDrawVerts[ ds.firstVert ];

			/* map the triangles */
			for ( int mapNonAxial = 0; mapNonAxial < 2; ++mapNonAxial )
				for ( i = 0; i < ds.numIndexes; i += 3 )
					MapTriangle( lm, info, TriRef{
						&verts[ bspDrawIndexes[ ds.firstIndex + i ] ],
						&verts[ bspDrawIndexes[ ds.firstIndex + i + 1 ] ],
						&verts[ bspDrawIndexes[ ds.firstIndex + i + 2 ] ] }, mapNonAxial );
			break;
		}
		case MST_PATCH:
		{
			/* make a mesh from the drawsurf */
			const mesh_t mesh = TessellatedMesh( mesh_view_t( ds.patchWidth, ds.patchHeight, &yDrawVerts[ ds.firstVert ] ), info->patchIterations );

			/* debug code */
#if 0
			if ( lm->plane ) {
				Sys_Printf( "Planar patch: [%1.3f %1.3f %1.3f] [%1.3f %1.3f %1.3f] [%1.3f %1.3f %1.3f]\n",
				            lm->plane[ 0 ], lm->plane[ 1 ], lm->plane[ 2 ],
				            lm->vecs[ 0 ][ 0 ], lm->vecs[ 0 ][ 1 ], lm->vecs[ 0 ][ 2 ],
				            lm->vecs[ 1 ][ 0 ], lm->vecs[ 1 ][ 1 ], lm->vecs[ 1 ][ 2 ] );
			}
#endif

			/* map the mesh quads */
#if 0
			for ( int mapNonAxial = 0; mapNonAxial < 2; ++mapNonAxial )
			{
				for( MeshQuadIterator it( mesh ); it; ++it ){
					for( const TriRef& tri : it.tris() )
						MapTriangle( lm, info, tri, mapNonAxial );
				}
			}
#else
			for( MeshQuadIterator it( mesh ); it; ++it ){
				/* attempt to map quad first */
				if ( !MapQuad( lm, info, it.quad() ) ) {
					for ( int mapNonAxial = 0; mapNonAxial < 2; ++mapNonAxial )
					{
						for( const TriRef& tri : it.tris() )
							MapTriangle( lm, info, tri, mapNonAxial );
					}
				}
			}
#endif
			break;
		}
		default:
			break;
		}
	}

	/* -----------------------------------------------------------------
	   average and clean up luxel normals
	   ----------------------------------------------------------------- */

	/* walk the luxels */
	for ( y = 0; y < lm->sh; ++y )
	{
		for ( x = 0; x < lm->sw; ++x )
		{
			/* get luxel */
			SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );

			/* only look at mapped luxels */
			if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
				continue;
			}

			/* the normal data could be the sum of multiple samples */
			if ( luxel.count > 1 ) {
				VectorNormalize( lm->getSuperNormal( x, y ) );
			}

			/* mark this luxel as having only one normal */
			luxel.count = 1;
		}
	}

	/* non-planar surfaces stop here */
	if ( lm->plane == nullptr ) {
		return;
	}

	/* -----------------------------------------------------------------
	   map occluded or unuxed luxels
	   ----------------------------------------------------------------- */

	/* walk the luxels */
	radius = std::max( 1, superSample / 2 ) + 1;
	for ( pass = 2.0f; pass <= radius; pass += 1 )
	{
		for ( y = 0; y < lm->sh; ++y )
		{
			for ( x = 0; x < lm->sw; ++x )
			{
				/* only look at unmapped luxels */
				if ( lm->getSuperCluster( x, y ) != CLUSTER_UNMAPPED ) {
					continue;
				}

				/* divine a normal and origin from neighboring luxels */
				fake.xyz.set( 0 );
				fake.normal.set( 0 );
				fake.lightmap[ 0 ] = Vector2( x, y );    //% 0.0001 + x; //% 0.0001 + y;
				samples = 0;
				for ( sy = ( y - 1 ); sy <= ( y + 1 ); ++sy )
				{
					if ( sy < 0 || sy >= lm->sh ) {
						continue;
					}

					for ( sx = ( x - 1 ); sx <= ( x + 1 ); ++sx )
					{
						if ( sx < 0 || sx >= lm->sw || ( sx == x && sy == y ) ) {
							continue;
						}

						/* get neighboring luxel */
						const SuperLuxel& luxel = lm->getSuperLuxel( 0, sx, sy );

						/* only consider luxels mapped in previous passes */
						if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL || luxel.value[ 0 ] >= pass ) {
							continue;
						}

						/* add its distinctiveness to our own */
						fake.xyz += lm->getSuperOrigin( sx, sy );
						fake.normal += lm->getSuperNormal( sx, sy );
						samples += luxel.count;
					}
				}

				/* any samples? */
				if ( samples == 0 ) {
					continue;
				}

				/* average */
				fake.xyz *= ( 1.f / samples );
				//%	fake.normal *= ( 1.f / samples );
				if ( VectorNormalize( fake.normal ) == 0 ) {
					continue;
				}

				/* map the fake vert */
				MapSingleLuxel( lm, nullptr, fake, lm->plane, pass, nullptr, nullptr, nullptr );
			}
		}
	}

	/* -----------------------------------------------------------------
	   average and clean up luxel normals
	   ----------------------------------------------------------------- */

	/* walk the luxels */
	for ( y = 0; y < lm->sh; ++y )
	{
		for ( x = 0; x < lm->sw; ++x )
		{
			/* get luxel */
			SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );

			/* only look at mapped luxels */
			if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
				continue;
			}

			/* the normal data could be the sum of multiple samples */
			if ( luxel.count > 1 ) {
				VectorNormalize( lm->getSuperNormal( x, y ) );
			}

			/* mark this luxel as having only one normal */
			luxel.count = 1;
		}
	}

	/* debug code */
	#if 0
	Sys_Printf( "\n" );
	for ( y = 0; y < lm->sh; ++y )
	{
		for ( x = 0; x < lm->sw; ++x )
		{
			const int cluster = lm->getSuperCluster( x, y );
			const Vector3& origin = lm->getSuperOrigin( x, y );
			const SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );

			if ( cluster < CLUSTER_NORMAL ) {
				continue;
			}

			/* check if within the bounding boxes of all surfaces referenced */
			MinMax minmax;
			for ( n = 0; n < lm->numLightSurfaces; ++n )
			{
				info = &surfaceInfos[ lightSurfaces[ lm->firstLightSurface + n ] ];
				minmax.extend( info->minmax );
				if( info->minmax.test( origin, info->sampleSize + 2 ) ){
					break;
				}
			}

			/* inside? */
			if ( n < lm->numLightSurfaces ) {
				continue;
			}

			/* report bogus origin */
			Sys_Printf( "%6d [%2d,%2d] (%4d): XYZ(%+4.1f %+4.1f %+4.1f) LO(%+4.1f %+4.1f %+4.1f) HI(%+4.1f %+4.1f %+4.1f) <%3.0f>\n",
			            rawLightmapNum, x, y, cluster,
			            origin[ 0 ], origin[ 1 ], origin[ 2 ],
			            minmax.mins[ 0 ], minmax.mins[ 1 ], minmax.mins[ 2 ],
			            minmax.maxs[ 0 ], minmax.maxs[ 1 ], minmax.maxs[ 2 ],
			            luxel.count );
		}
	}
	#endif
}



/*
   SetupDirt()
   sets up dirtmap (ambient occlusion)
 */

#define DIRT_CONE_ANGLE             88  /* degrees */
#define DIRT_NUM_ANGLE_STEPS        16
#define DIRT_NUM_ELEVATION_STEPS    3
#define DIRT_NUM_VECTORS            ( DIRT_NUM_ANGLE_STEPS * DIRT_NUM_ELEVATION_STEPS )

static Vector3 dirtVectors[ DIRT_NUM_VECTORS ];
static int numDirtVectors = 0;

void SetupDirt(){
	int i, j;
	float angle, elevation;


	/* note it */
	Sys_FPrintf( SYS_VRB, "--- SetupDirt ---\n" );

	/* calculate angular steps */
	const float angleStep = degrees_to_radians( 360.0f / DIRT_NUM_ANGLE_STEPS );
	const float elevationStep = degrees_to_radians( DIRT_CONE_ANGLE / DIRT_NUM_ELEVATION_STEPS );

	/* iterate angle */
	for ( i = 0, angle = 0; i < DIRT_NUM_ANGLE_STEPS; ++i, angle += angleStep )
	{
		/* iterate elevation */
		for ( j = 0, elevation = elevationStep * 0.5f; j < DIRT_NUM_ELEVATION_STEPS; ++j, elevation += elevationStep )
		{
			dirtVectors[ numDirtVectors ][ 0 ] = sin( elevation ) * cos( angle );
			dirtVectors[ numDirtVectors ][ 1 ] = sin( elevation ) * sin( angle );
			dirtVectors[ numDirtVectors ][ 2 ] = cos( elevation );
			numDirtVectors++;
		}
	}

	/* emit some statistics */
	Sys_FPrintf( SYS_VRB, "%9d dirtmap vectors\n", numDirtVectors );
}


/*
   DirtForSample()
   calculates dirt value for a given sample
 */

/* BehavEd stage 6 (NRC#101, "dark near sky"): dirt rays trace with testAll = false (divVerent
   aa00101d), so a ray into a sky brush stops in its opaque leaf (TraceLine_r TRACE_LEAF_SOLID,
   passSolid) before any surface is tested - compileFlags never gets C_SKY, the existing
   "&& !( trace->compileFlags & C_SKY )" never fires, and surfaces next to the sky got as much
   dirt as under a solid ceiling (measured: wall top/middle 0.926 under both). When a ray stops
   in solid, trace it once more with testAll up to just behind the entry point: did it cross a
   sky surface there, the ray left the map through the sky and is no occluder. */
static bool BhedDirtRayHitsSky( const trace_t *trace ){
	if ( trace->compileFlags & C_SKY ) {
		return true;
	}
	if ( !trace->opaque || !trace->passSolid ) {
		return false;
	}
	trace_t sky = *trace;
	sky.testAll = true;
	sky.end = trace->hit + trace->direction;
	SetupTrace( &sky );
	sky.color.set( 1 );
	TraceLine( &sky );
	return ( sky.compileFlags & C_SKY ) != 0;
}

static float DirtForSample( trace_t *trace ){
	int i;
	float gatherDirt, outDirt, angle, elevation, ooDepth;
	Vector3 myUp, myRt;


	/* dummy check */
	if ( !dirty ) {
		return 1;
	}
	if ( trace == nullptr || trace->cluster < CLUSTER_NORMAL ) {
		return 0;
	}

	/* setup */
	gatherDirt = 0;
	ooDepth = 1.0f / dirtDepth;
	const Vector3 normal( trace->normal );

	/* check if the normal is aligned to the world-up */
	if ( normal[ 0 ] == 0 && normal[ 1 ] == 0 && ( normal[ 2 ] == 1 || normal[ 2 ] == -1 ) ) {
		if ( normal[ 2 ] == 1 ) {
			myRt = g_vector3_axis_x;
			myUp = g_vector3_axis_y;
		}
		else if ( normal[ 2 ] == -1 ) {
			myRt = -g_vector3_axis_x;
			myUp = g_vector3_axis_y;
		}
	}
	else
	{
		myRt = VectorNormalized( vector3_cross( normal, g_vector3_axis_z ) );
		myUp = VectorNormalized( vector3_cross( myRt, normal ) );
	}

	/* 1 = random mode, 0 (well everything else) = non-random mode */
	if ( dirtMode == 1 ) {
		/* iterate */
		for ( i = 0; i < numDirtVectors; ++i )
		{
			/* get random vector */
			angle = Random() * degrees_to_radians( 360.0f );
			elevation = Random() * degrees_to_radians( DIRT_CONE_ANGLE );
			const Vector3 temp( cos( angle ) * sin( elevation ),
			                    sin( angle ) * sin( elevation ),
			                    cos( elevation ) );

			/* transform into tangent space */
			const Vector3 direction = myRt * temp[ 0 ] + myUp * temp[ 1 ] + normal * temp[ 2 ];

			/* set endpoint */
			trace->end = trace->origin + direction * dirtDepth;
			SetupTrace( trace );
			trace->color.set( 1 );

			/* trace */
			TraceLine( trace );
			if ( trace->opaque && !BhedDirtRayHitsSky( trace ) ) { // BehavEd stage 6: was !( trace->compileFlags & C_SKY )
				gatherDirt += 1.0f - ooDepth * vector3_length( trace->hit - trace->origin );
			}
		}
	}
	else
	{
		/* iterate through ordered vectors */
		for ( i = 0; i < numDirtVectors; ++i )
		{
			/* transform vector into tangent space */
			const Vector3 direction = myRt * dirtVectors[ i ][ 0 ] + myUp * dirtVectors[ i ][ 1 ] + normal * dirtVectors[ i ][ 2 ];

			/* set endpoint */
			trace->end = trace->origin + direction * dirtDepth;
			SetupTrace( trace );
			trace->color.set( 1 );

			/* trace */
			TraceLine( trace );
			if ( trace->opaque && !BhedDirtRayHitsSky( trace ) ) { // BehavEd stage 6 (NRC#101)
				gatherDirt += 1.0f - ooDepth * vector3_length( trace->hit - trace->origin );
			}
		}
	}

	/* direct ray */
	trace->end = trace->origin + normal * dirtDepth;
	SetupTrace( trace );
	trace->color.set( 1 );

	/* trace */
	TraceLine( trace );
	if ( trace->opaque && !BhedDirtRayHitsSky( trace ) ) { // BehavEd stage 6 (NRC#101)
		gatherDirt += 1.0f - ooDepth * vector3_length( trace->hit - trace->origin );
	}

	/* early out */
	if ( gatherDirt <= 0 ) {
		return 1;
	}

	/* apply gain (does this even do much? heh) */
	outDirt = std::min( 1.f, std::pow( gatherDirt / ( numDirtVectors + 1 ), dirtGain ) );

	/* apply scale */
	outDirt *= dirtScale;
	value_minimize( outDirt, 1.0f );

	/* return to sender */
	return 1.0f - outDirt;
}



/*
   DirtyRawLightmap()
   calculates dirty fraction for each luxel
 */

void DirtyRawLightmap( int rawLightmapNum ){
	float               average, samples;
	trace_t trace;
	bool noDirty;


	/* bail if this number exceeds the number of raw lightmaps */
	if ( rawLightmapNum >= numRawLightmaps ) {
		return;
	}

	/* get lightmap */
	rawLightmap_t& lm = rawLightmaps[ rawLightmapNum ];

	/* setup trace */
	trace.testOcclusion = true;
	trace.forceSunlight = false;
	trace.recvShadows = lm.recvShadows;
	trace.numSurfaces = lm.numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm.firstLightSurface ];
	trace.inhibitRadius = 0;
	trace.testAll = false;

	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	trace.twoSided = false;
	for ( int i : Span( trace.surfaces, trace.numSurfaces ) )
	{
		/* get surface */
		const surfaceInfo_t& info = surfaceInfos[ i ];

		/* check twosidedness */
		if ( info.si->twoSided ) {
			trace.twoSided = true;
			break;
		}
	}

	noDirty = false;
	for ( int i : Span( trace.surfaces, trace.numSurfaces ) )
	{
		/* get surface */
		const surfaceInfo_t& info = surfaceInfos[ i ];

		/* check twosidedness */
		if ( info.si->noDirty ) {
			noDirty = true;
			break;
		}
	}

	/* gather dirt */
	for ( int y = 0; y < lm.sh; ++y )
	{
		for ( int x = 0; x < lm.sw; ++x )
		{
			/* get luxel */
			const int cluster = lm.getSuperCluster( x, y );
			float& dirt = lm.getSuperDirt( x, y );

			/* set default dirt */
			dirt = 0;

			/* only look at mapped luxels */
			if ( cluster < CLUSTER_NORMAL ) {
				continue;
			}

			/* don't apply dirty on this surface */
			if ( noDirty ) {
				dirt = 1;
				continue;
			}

			/* copy to trace */
			trace.cluster = cluster;
			trace.origin = lm.getSuperOrigin( x, y );
			trace.normal = lm.getSuperNormal( x, y );

			/* get dirt */
			dirt = DirtForSample( &trace );
		}
	}

	/* testing no filtering */
	//%	return;

	/* filter dirt */
	for ( int y = 0; y < lm.sh; ++y )
	{
		for ( int x = 0; x < lm.sw; ++x )
		{
			/* get luxel */
			float& dirt = lm.getSuperDirt( x, y );

			/* filter dirt by adjacency to unmapped luxels */
			average = dirt;
			samples = 1;
			for ( int sy = ( y - 1 ); sy <= ( y + 1 ); ++sy )
			{
				if ( sy < 0 || sy >= lm.sh ) {
					continue;
				}

				for ( int sx = ( x - 1 ); sx <= ( x + 1 ); ++sx )
				{
					if ( sx < 0 || sx >= lm.sw || ( sx == x && sy == y ) ) {
						continue;
					}

					/* get neighboring luxel */
					const float dirt2 = lm.getSuperDirt( sx, sy );
					if ( lm.getSuperCluster( sx, sy ) < CLUSTER_NORMAL || dirt2 <= 0 ) {
						continue;
					}

					/* add it */
					average += dirt2;
					samples += 1;
				}

				/* bail */
				if ( samples <= 0 ) {
					break;
				}
			}

			/* bail */
			if ( samples <= 0 ) {
				continue;
			}

			/* scale dirt */
			dirt = average / samples;
		}
	}
}



/*
   SubmapRawLuxel()
   calculates the pvs cluster, origin, normal of a sub-luxel
 */

static bool SubmapRawLuxel( const rawLightmap_t *lm, int x, int y, float bx, float by, int& sampleCluster, Vector3& sampleOrigin, Vector3& sampleNormal ){
	const Vector3       *origin, *origin2;
	Vector3 originVecs[ 2 ];


	/* calculate x vector */
	if ( ( x < ( lm->sw - 1 ) && bx >= 0 ) || ( x == 0 && bx <= 0 ) ) {
		origin = &lm->getSuperOrigin( x, y );
		//%	normal = SUPER_NORMAL( x, y );
		origin2 = lm->getSuperCluster( x + 1, y ) < CLUSTER_NORMAL ? &lm->getSuperOrigin( x, y ) : &lm->getSuperOrigin( x + 1, y );
		//%	normal2 = *cluster2 < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x + 1, y );
	}
	else if ( ( x > 0 && bx <= 0 ) || ( x == ( lm->sw - 1 ) && bx >= 0 ) ) {
		origin = lm->getSuperCluster( x - 1, y ) < CLUSTER_NORMAL ? &lm->getSuperOrigin( x, y ) : &lm->getSuperOrigin( x - 1, y );
		//%	normal = *cluster < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x - 1, y );
		origin2 = &lm->getSuperOrigin( x, y );
		//%	normal2 = SUPER_NORMAL( x, y );
	}
	else
	{
		Error( "Spurious lightmap S vector\n" );
	}

	originVecs[ 0 ] = *origin2 - *origin;
	//%	VectorSubtract( normal2, normal, normalVecs[ 0 ] );

	/* calculate y vector */
	if ( ( y < ( lm->sh - 1 ) && bx >= 0 ) || ( y == 0 && bx <= 0 ) ) {
		origin = &lm->getSuperOrigin( x, y );
		//%	normal = SUPER_NORMAL( x, y );
		origin2 = lm->getSuperCluster( x, y + 1 ) < CLUSTER_NORMAL ? &lm->getSuperOrigin( x, y ) : &lm->getSuperOrigin( x, y + 1 );
		//%	normal2 = *cluster2 < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x, y + 1 );
	}
	else if ( ( y > 0 && bx <= 0 ) || ( y == ( lm->sh - 1 ) && bx >= 0 ) ) {
		origin = lm->getSuperCluster( x, y - 1 ) < CLUSTER_NORMAL ? &lm->getSuperOrigin( x, y ) : &lm->getSuperOrigin( x, y - 1 );
		//%	normal = *cluster < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x, y - 1 );
		origin2 = &lm->getSuperOrigin( x, y );
		//%	normal2 = SUPER_NORMAL( x, y );
	}
	else{
		Sys_Warning( "Spurious lightmap T vector\n" );
	}

	originVecs[ 1 ] = *origin2 - *origin;
	//%	VectorSubtract( normal2, normal, normalVecs[ 1 ] );

	/* calculate new origin */
	//%	VectorMA( origin, bx, originVecs[ 0 ], sampleOrigin );
	//%	VectorMA( sampleOrigin, by, originVecs[ 1 ], sampleOrigin );
	sampleOrigin += ( originVecs[ 0 ] * bx ) + ( originVecs[ 1 ] * by );

	/* get cluster */
	sampleCluster = ClusterForPointExtFilter( sampleOrigin, ( LUXEL_EPSILON * 2 ), lm->numLightClusters, lm->lightClusters );
	if ( sampleCluster <= CLUSTER_OPAQUE ) {
		return false;
	}

	/* calculate new normal */
	//%	VectorMA( normal, bx, normalVecs[ 0 ], sampleNormal );
	//%	VectorMA( sampleNormal, by, normalVecs[ 1 ], sampleNormal );
	//%	if( VectorNormalize( sampleNormal, sampleNormal ) == 0 )
	//%		return false;
	sampleNormal = lm->getSuperNormal( x, y );

	/* return ok */
	return true;
}


/*
   SubsampleRawLuxel_r()
   recursively subsamples a luxel until its color gradient is low enough or subsampling limit is reached
 */

static void SubsampleRawLuxel_r( rawLightmap_t *lm, trace_t *trace, const Vector3& sampleOrigin, int x, int y, float bias, SuperLuxel& lightLuxel, Vector3 *lightDeluxel ){
	int b, samples, mapped, lighted;
	int cluster[ 4 ];
	SuperLuxel luxel[ 4 ];
	Vector3 deluxel[ 4 ];
	Vector3 origin[ 4 ], normal[ 4 ];
	float biasDirs[ 4 ][ 2 ] = { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
	Vector3 color, direction( 0 ), total( 0 );


	/* limit check */
	if ( lightLuxel.count >= lightSamples ) {
		return;
	}

	/* setup */
	mapped = 0;
	lighted = 0;

	/* make 2x2 subsample stamp */
	for ( b = 0; b < 4; ++b )
	{
		/* set origin */
		origin[ b ] = sampleOrigin;

		/* calculate position */
		if ( !SubmapRawLuxel( lm, x, y, ( bias * biasDirs[ b ][ 0 ] ), ( bias * biasDirs[ b ][ 1 ] ), cluster[ b ], origin[ b ], normal[ b ] ) ) {
			cluster[ b ] = CLUSTER_UNMAPPED;
			continue;
		}
		mapped++;

		/* increment sample count */
		luxel[ b ].count = lightLuxel.count + 1;

		/* setup trace */
		trace->cluster = *cluster;
		trace->origin = origin[ b ];
		trace->normal = normal[ b ];

		/* sample light */

		LightContributionToSample( trace );
		if ( trace->forceSubsampling > 1 ) {
			/* alphashadow: we subsample as deep as we can */
			++lighted;
			++mapped;
			++mapped;
		}

		/* add to totals (fixme: make contrast function) */
		luxel[ b ].value = trace->color;
		if ( lightDeluxel ) {
			deluxel[ b ] = trace->directionContribution;
		}
		total += trace->color;
		if ( ( luxel[ b ].value[ 0 ] + luxel[ b ].value[ 1 ] + luxel[ b ].value[ 2 ] ) > 0 ) {
			lighted++;
		}
	}

	/* subsample further? */
	if ( ( lightLuxel.count + 1 ) < lightSamples &&
	     ( total[ 0 ] > 4.0f || total[ 1 ] > 4.0f || total[ 2 ] > 4.0f ) &&
	     lighted != 0 && lighted != mapped ) {
		for ( b = 0; b < 4; ++b )
		{
			if ( cluster[ b ] < CLUSTER_NORMAL ) {
				continue;
			}
			SubsampleRawLuxel_r( lm, trace, origin[ b ], x, y, ( bias * 0.5f ), luxel[ b ], lightDeluxel ? &deluxel[ b ] : nullptr );
		}
	}

	/* average */
	//%	color.set( 0 );
	//%	samples = 0;
	color = lightLuxel.value;
	if ( lightDeluxel ) {
		direction = *lightDeluxel;
	}
	samples = 1;
	for ( b = 0; b < 4; ++b )
	{
		if ( cluster[ b ] < CLUSTER_NORMAL ) {
			continue;
		}
		color += luxel[ b ].value;
		if ( lightDeluxel ) {
			direction += deluxel[ b ];
		}
		samples++;
	}

	/* add to luxel */
	if ( samples > 0 ) {
		/* average */
		color /= samples;

		/* add to color */
		lightLuxel.value = color;
		lightLuxel.count += 1;

		if ( lightDeluxel ) {
			direction /= samples;
			*lightDeluxel = direction;
		}
	}
}

/* A mostly Gaussian-like bounded random distribution (sigma is expected standard deviation) */
static void GaussLikeRandom( float sigma, float *x, float *y ){
	float r;
	r = Random() * 2 * c_pi;
	*x = sigma * 2.73861278752581783822 * cos( r );
	*y = sigma * 2.73861278752581783822 * sin( r );
	r = Random();
	r = 1 - sqrt( r );
	r = 1 - sqrt( r );
	*x *= r;
	*y *= r;
}
static void RandomSubsampleRawLuxel( rawLightmap_t *lm, trace_t *trace, const Vector3& sampleOrigin, int x, int y, float bias, SuperLuxel& lightLuxel, Vector3 *lightDeluxel ){
	int b, mapped = 0;
	int cluster;
	Vector3 origin, normal;
	Vector3 total( 0 ), totaldirection( 0 );
	float dx, dy;

	for ( b = 0; b < lightSamples; ++b )
	{
		/* set origin */
		origin = sampleOrigin;
		GaussLikeRandom( bias, &dx, &dy );

		/* calculate position */
		if ( !SubmapRawLuxel( lm, x, y, dx, dy, cluster, origin, normal ) ) {
			cluster = CLUSTER_UNMAPPED;
			continue;
		}
		mapped++;

		trace->cluster = cluster;
		trace->origin = origin;
		trace->normal = normal;

		LightContributionToSample( trace );
		total += trace->color;
		if ( lightDeluxel ) {
			totaldirection += trace->directionContribution;
		}
	}

	/* add to luxel */
	if ( mapped > 0 ) {
		/* average */
		lightLuxel.value = total / mapped;

		if ( lightDeluxel ) {
			*lightDeluxel = totaldirection / mapped;
		}
	}
}



/*
   CreateTraceLightsForBounds()
   creates a list of lights that affect the given bounding box and pvs clusters (bsp leaves)
 */

static void CreateTraceLightsForBounds( const MinMax& minmax, const Vector3 *normal, int numClusters, int *clusters, LightFlags flags, trace_t *trace ){
	int i;
	float length;


	/* debug code */
	//% Sys_Printf( "CTWLFB: (%4.1f %4.1f %4.1f) (%4.1f %4.1f %4.1f)\n", minmax.mins[ 0 ], minmax.mins[ 1 ], minmax.mins[ 2 ], minmax.maxs[ 0 ], minmax.maxs[ 1 ], minmax.maxs[ 2 ] );

	/* allocate the light list */
	trace->lights = safe_malloc( sizeof( light_t* ) * ( lights.size() + 1 ) );
	trace->numLights = 0;

	/* calculate spherical bounds */
	const Vector3 origin = minmax.origin();
	const float radius = vector3_length( minmax.maxs - origin );

	/* get length of normal vector */
	if ( normal != nullptr ) {
		length = vector3_length( *normal );
	}
	else
	{
		normal = &g_vector3_identity;
		length = 0;
	}

	/* test each light and see if it reaches the sphere */
	/* note: the attenuation code MUST match LightingAtSample() */
	for ( const light_t& light : lights )
	{
		/* check zero sized envelope */
		if ( light.envelope <= 0 ) {
			lightsEnvelopeCulled++;
			continue;
		}

		/* check flags */
		if ( !( light.flags & flags ) ) {
			continue;
		}

		/* sunlight skips all this nonsense */
		if ( light.type != ELightType::Sun ) {
			/* sun only? */
			if ( sunOnly ) {
				continue;
			}

			/* check against pvs cluster */
			if ( numClusters > 0 && clusters != nullptr ) {
				for ( i = 0; i < numClusters; ++i )
				{
					if ( ClusterVisible( light.cluster, clusters[ i ] ) ) {
						break;
					}
				}

				/* fixme! */
				if ( i == numClusters ) {
					lightsClusterCulled++;
					continue;
				}
			}

			/* if the light's bounding sphere intersects with the bounding sphere then this light needs to be tested */
			if ( vector3_length( light.origin - origin ) - light.envelope - radius > 0 ) {
				lightsEnvelopeCulled++;
				continue;
			}

			/* check bounding box against light's pvs envelope (note: this code never eliminated any lights, so disabling it) */
			#if 0
			if( !minmax.test( light.minmax ) ){
				lightsBoundsCulled++;
				continue;
			}
			#endif
		}

		/* planar surfaces (except twosided surfaces) have a couple more checks */
		if ( length > 0 && !trace->twoSided ) {
			/* lights coplanar with a surface won't light it */
			if ( !( light.flags & LightFlags::Twosided ) && vector3_dot( light.normal, *normal ) > 0.999f ) {
				lightsPlaneCulled++;
				continue;
			}

			/* check to see if light is behind the plane */
			if ( vector3_dot( light.origin, *normal ) - vector3_dot( origin, *normal ) < -1 ) {
				lightsPlaneCulled++;
				continue;
			}
		}

		/* add this light */
		trace->lights[ trace->numLights++ ] = &light;
	}

	/* make last night null */
	trace->lights[ trace->numLights ] = nullptr;
}



/*
   CreateTraceLightsForSurface()
   creates a list of lights that can potentially affect a drawsurface
 */

static void CreateTraceLightsForSurface( int num, trace_t *trace ){
	/* dummy check */
	if ( num < 0 ) {
		return;
	}

	/* get drawsurface and info */
	const bspDrawSurface_t& ds = bspDrawSurfaces[ num ];
	const surfaceInfo_t& info = surfaceInfos[ num ];

	/* get the mins/maxs for the dsurf */
	MinMax minmax;
	Vector3 normal = bspDrawVerts[ ds.firstVert ].normal;
	for ( const bspDrawVert_t& dv : Span( &yDrawVerts[ ds.firstVert ], ds.numVerts ) )
	{
		minmax.extend( dv.xyz );
		if ( !VectorCompare( dv.normal, normal ) ) {
			normal.set( 0 );
		}
	}

	/* create the lights for the bounding box */
	CreateTraceLightsForBounds( minmax, &normal, info.numSurfaceClusters, &surfaceClusters[ info.firstSurfaceCluster ], LightFlags::Surfaces, trace );
}

inline void FreeTraceLights( trace_t *trace ){
	free( trace->lights );
}



/*
   IlluminateRawLightmap()
   illuminates the luxels
 */
static void FloodlightIlluminateLightmap( rawLightmap_t *lm );

/* BehavEd stage 7 (speed, same result): the light loop of IlluminateRawLightmap for one big raw
   lightmap on all threads. On mp__duel1 one lightmap took 21 % of the radiosity time, so more than
   4.8 times faster was impossible however many threads (measured 3.9 with 32). Here the luxel rows
   and blocks of lights are spread over the threads; every luxel gets exactly the same float
   operations in the same order as in the loop below:
   - pass 1 (one sample per luxel) for a block of lights at once, each light into its own buffer
     (the loop below reuses one buffer and lm->superFlags per light; nothing else is written);
   - in light order, serially: the style check (it depends on the styles set by earlier lights),
     the "nothing lit" skip, allocation of the style lightmap and setting the style;
   - pass 2 (adaptive supersampling) per light as a wavefront: stamp (x,y) reads and writes the
     luxels (x..x+1, y..y+1); the earlier stamps sharing a luxel are (x-1..x+1, y-1) and (x-1, y),
     so row y starts stamp x only when row y-1 has finished stamps 0..x+1 - every stamp sees the
     luxels and flags of the serial row-by-row order;
   - pass 3 (filter, add to the lightmap) per row, the lights of the block in their order.
   Not used with -randomsamples (random numbers per work item) or the debug fills. */
static void BhedLichterParallel( rawLightmap_t *lm, trace_t& trace, bhed_par::Pool& pool ){
	const int sw = lm->sw;
	const int sh = lm->sh;
	const size_t n = size_t( sw ) * sh;
	const bool proben = lightSamples > 1;
	const float tests[ 4 ][ 2 ] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	/* at most about 64 MB of temporary luxels per block; the buffers are kept between calls
	   (only the main thread comes here) - fresh memory costs page faults every time */
	const size_t jeLicht = n * ( sizeof( SuperLuxel ) + ( deluxemap ? sizeof( Vector3 ) : 0 ) + 1 );
	const int block = int( std::clamp<size_t>( ( size_t( 64 ) << 20 ) / std::max<size_t>( jeLicht, 1 ), 1, 128 ) );
	static std::vector<SuperLuxel> luxelPuffer;
	static std::vector<Vector3> deluxelPuffer;
	static std::vector<byte> flagPuffer;
	static std::vector<int> licht;
	static std::vector<int> kandidatSlot;
	static std::vector<std::vector<int>> kandidaten;
	static std::vector<std::vector<SuperLuxel>> ergebnisse;
	static std::vector<std::vector<Vector3>> ergebnisseDir;
	static std::vector<std::pair<int, int>> arbeit;
	if ( luxelPuffer.size() < n * block ) {
		luxelPuffer.resize( n * block );
	}
	if ( deluxemap && deluxelPuffer.size() < n * block ) {
		deluxelPuffer.resize( n * block );
	}
	if ( proben && flagPuffer.size() < n * block ) {
		flagPuffer.resize( n * block );
	}
	if ( licht.size() < size_t( sh ) * block ) {
		licht.resize( size_t( sh ) * block );
	}
	struct Aktiv
	{
		int j;
		int lightmapNum;
		int luxelFilterRadius;
		const light_t *light;
	};
	std::vector<Aktiv> aktiv;
	for ( int basis = 0; basis < trace.numLights; basis += block )
	{
		const int k = std::min( block, trace.numLights - basis );
		memset( static_cast<void*>( luxelPuffer.data() ), 0, n * k * sizeof( SuperLuxel ) );
		if ( deluxemap ) {
			memset( static_cast<void*>( deluxelPuffer.data() ), 0, n * k * sizeof( Vector3 ) );
		}
		if ( proben ) {
			memset( flagPuffer.data(), 0, n * k );
		}

		/* pass 1 */
		pool.fuerAlle( k * sh, [&]( int aufgabe ){
			const int j = aufgabe / sh;
			const int y = aufgabe % sh;
			trace_t tr = trace;
			tr.light = trace.lights[ basis + j ];
			SuperLuxel *luxel = luxelPuffer.data() + n * j;
			Vector3 *deluxel = deluxemap ? deluxelPuffer.data() + n * j : nullptr;
			byte *flag = proben ? flagPuffer.data() + n * j : nullptr;
			int lit = 0;
			for ( int x = 0; x < sw; ++x )
			{
				const int cluster = lm->getSuperCluster( x, y );
				if ( cluster < CLUSTER_NORMAL ) {
					continue;
				}
				SuperLuxel& lightLuxel = luxel[ y * sw + x ];
				lightLuxel.count = 1;
				tr.cluster = cluster;
				tr.origin = lm->getSuperOrigin( x, y );
				tr.normal = lm->getSuperNormal( x, y );
				LightContributionToSample( &tr );
				lightLuxel.value = tr.color;
				if ( deluxemap ) {
					deluxel[ y * sw + x ] = tr.directionContribution;
				}
				if ( tr.forceSubsampling > 1 && proben ) {
					lit++;
					flag[ y * sw + x ] |= FLAG_FORCE_SUBSAMPLING;
				}
				else if ( tr.color != g_vector3_identity ) {
					lit++;
				}
			}
			licht[ size_t( j ) * sh + y ] = lit;
		} );

		/* in light order */
		aktiv.clear();
		for ( int j = 0; j < k; ++j )
		{
			const light_t *light = trace.lights[ basis + j ];
			int lightmapNum;
			for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
			{
				if ( lm->styles[ lightmapNum ] == light->style ||
				     lm->styles[ lightmapNum ] == LS_NONE ) {
					break;
				}
			}
			if ( lightmapNum >= MAX_LIGHTMAPS ) {
				Sys_Warning( "Hit per-surface style limit (%d)\n", MAX_LIGHTMAPS );
				continue;
			}
			int totalLighted = 0;
			for ( int y = 0; y < sh; ++y )
				totalLighted += licht[ size_t( j ) * sh + y ];
			const float filterRadius = std::max( { 0.f, lm->filterRadius, light->filterRadius } );
			int luxelFilterRadius = lm->sampleSize != 0 ? superSample * filterRadius / lm->sampleSize : 0;
			if ( luxelFilterRadius == 0 && ( filterRadius > 0 || filter ) ) {
				luxelFilterRadius = 1;
			}
			if ( totalLighted == 0 ) {
				continue;
			}
			if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
				lm->superLuxels[ lightmapNum ] = safe_calloc( n * sizeof( *lm->superLuxels[0] ) );
			}
			if ( lightmapNum > 0 ) {
				lm->styles[ lightmapNum ] = light->style;
			}
			aktiv.push_back( { j, lightmapNum, luxelFilterRadius, light } );
		}
		if ( aktiv.empty() ) {
			continue;
		}

		/* pass 2: adaptive supersampling. The serial stamp scan is cheap; the work is in
		   SubsampleRawLuxel_r, and its result for a luxel depends only on the luxel's value from
		   pass 1 (each luxel is subsampled at most once, FLAG_ALREADY_SUBSAMPLED, and nothing else
		   changes luxels in this pass). So: predict the luxels with the pass-1 values, subsample
		   them in parallel into a side buffer, then run the exact serial scan per light (lights in
		   parallel) and take the precomputed result where there is one - a luxel the prediction
		   missed is subsampled right there, exactly as in the loop below. */
		if ( proben && sh > 1 && sw > 1 ) {
			const int na = int( aktiv.size() );
			if ( kandidatSlot.size() < n * block ) {
				kandidatSlot.resize( n * block );
			}
			kandidaten.resize( na );
			ergebnisse.resize( na );
			ergebnisseDir.resize( na );
			/* the stamp test of the loop below */
			const auto stempel = [&]( const SuperLuxel *luxel, const byte *flags, int x, int y ){
				int mapped = 0;
				int lighted = 0;
				Vector3 total( 0 );
				for ( int t = 0; t < 4; ++t )
				{
					const int sx = x + tests[ t ][ 0 ];
					const int sy = y + tests[ t ][ 1 ];
					if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
						continue;
					}
					mapped++;
					if ( flags[ sy * sw + sx ] & FLAG_FORCE_SUBSAMPLING ) {
						++lighted;
						++mapped;
						++mapped;
					}
					const SuperLuxel& lightLuxel = luxel[ sy * sw + sx ];
					total += lightLuxel.value;
					if ( ( lightLuxel.value[ 0 ] + lightLuxel.value[ 1 ] + lightLuxel.value[ 2 ] ) > 0 ) {
						lighted++;
					}
				}
				return !( total[ 0 ] <= 4.0f && total[ 1 ] <= 4.0f && total[ 2 ] <= 4.0f ) && lighted != 0 && lighted != mapped;
			};
			/* prediction */
			pool.fuerAlle( na, [&]( int ai ){
				const int j = aktiv[ ai ].j;
				const SuperLuxel *luxel = luxelPuffer.data() + n * j;
				const byte *flags = flagPuffer.data() + n * j;
				int *slot = kandidatSlot.data() + n * j;
				std::fill( slot, slot + n, -1 );
				std::vector<int>& liste = kandidaten[ ai ];
				liste.clear();
				for ( int y = 0; y < sh - 1; ++y )
				{
					for ( int x = 0; x < sw - 1; ++x )
					{
						if ( !stempel( luxel, flags, x, y ) ) {
							continue;
						}
						for ( int t = 0; t < 4; ++t )
						{
							const int sx = x + tests[ t ][ 0 ];
							const int sy = y + tests[ t ][ 1 ];
							if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL || slot[ sy * sw + sx ] >= 0 ) {
								continue;
							}
							slot[ sy * sw + sx ] = int( liste.size() );
							liste.push_back( sy * sw + sx );
						}
					}
				}
				ergebnisse[ ai ].resize( liste.size() );
				ergebnisseDir[ ai ].resize( deluxemap ? liste.size() : 0 );
			} );
			/* subsample the predicted luxels, all lights, in chunks */
			arbeit.clear();
			for ( int ai = 0; ai < na; ++ai )
				for ( size_t k = 0; k < kandidaten[ ai ].size(); k += 8 )
					arbeit.push_back( { ai, int( k ) } );
			pool.fuerAlle( int( arbeit.size() ), [&]( int w ){
				const int ai = arbeit[ w ].first;
				const int j = aktiv[ ai ].j;
				trace_t tr = trace;
				tr.light = aktiv[ ai ].light;
				const SuperLuxel *luxel = luxelPuffer.data() + n * j;
				const Vector3 *deluxel = deluxemap ? deluxelPuffer.data() + n * j : nullptr;
				const std::vector<int>& liste = kandidaten[ ai ];
				const size_t ende = std::min( liste.size(), size_t( arbeit[ w ].second ) + 8 );
				for ( size_t k = arbeit[ w ].second; k < ende; ++k )
				{
					const int i = liste[ k ];
					const int sx = i % sw;
					const int sy = i / sw;
					SuperLuxel& ziel = ergebnisse[ ai ][ k ];
					ziel = luxel[ i ];
					Vector3 *zielDir = nullptr;
					if ( deluxemap ) {
						ergebnisseDir[ ai ][ k ] = deluxel[ i ];
						zielDir = &ergebnisseDir[ ai ][ k ];
					}
					SubsampleRawLuxel_r( lm, &tr, lm->getSuperOrigin( sx, sy ), sx, sy, 0.25f * lightSamplesSearchBoxSize, ziel, zielDir );
				}
			} );
			/* the exact serial scan, one light per task */
			pool.fuerAlle( na, [&]( int ai ){
				const int j = aktiv[ ai ].j;
				trace_t tr = trace;
				tr.light = aktiv[ ai ].light;
				SuperLuxel *luxel = luxelPuffer.data() + n * j;
				Vector3 *deluxel = deluxemap ? deluxelPuffer.data() + n * j : nullptr;
				byte *flags = flagPuffer.data() + n * j;
				const int *slot = kandidatSlot.data() + n * j;
				for ( int y = 0; y < sh - 1; ++y )
				{
					for ( int x = 0; x < sw - 1; ++x )
					{
						if ( !stempel( luxel, flags, x, y ) ) {
							continue;
						}
						for ( int t = 0; t < 4; ++t )
						{
							const int sx = x + tests[ t ][ 0 ];
							const int sy = y + tests[ t ][ 1 ];
							if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
								continue;
							}
							byte& flag = flags[ sy * sw + sx ];
							if ( flag & FLAG_ALREADY_SUBSAMPLED ) {
								continue;
							}
							const int i = sy * sw + sx;
							if ( slot[ i ] >= 0 ) {
								luxel[ i ] = ergebnisse[ ai ][ slot[ i ] ];
								if ( deluxemap ) {
									deluxel[ i ] = ergebnisseDir[ ai ][ slot[ i ] ];
								}
							}
							else
							{
								Vector3 *lightDeluxel = deluxemap ? &deluxel[ i ] : nullptr;
								SubsampleRawLuxel_r( lm, &tr, lm->getSuperOrigin( sx, sy ), sx, sy, 0.25f * lightSamplesSearchBoxSize, luxel[ i ], lightDeluxel );
							}
							flag |= FLAG_ALREADY_SUBSAMPLED;
						}
					}
				}
			} );
		}

		/* pass 3 */
		pool.fuerAlle( sh, [&]( int y ){
			for ( const Aktiv& a : aktiv )
			{
				const SuperLuxel *luxel = luxelPuffer.data() + n * a.j;
				const Vector3 *deluxel = deluxemap ? deluxelPuffer.data() + n * a.j : nullptr;
				const int r = a.luxelFilterRadius;
				for ( int x = 0; x < sw; ++x )
				{
					if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
						continue;
					}
					if ( r ) {
						Vector3 averageColor( 0 ), averageDir( 0 );
						float samples = 0;
						for ( int sy = ( y - r ); sy <= ( y + r ); ++sy )
						{
							if ( sy < 0 || sy >= sh ) {
								continue;
							}
							for ( int sx = ( x - r ); sx <= ( x + r ); ++sx )
							{
								if ( sx < 0 || sx >= sw ) {
									continue;
								}
								if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
									continue;
								}
								float weight = ( abs( sx - x ) == r ? 0.5f : 1.0f );
								weight *= ( abs( sy - y ) == r ? 0.5f : 1.0f );
								averageColor += luxel[ sy * sw + sx ].value * weight;
								if ( deluxemap ) {
									averageDir += deluxel[ sy * sw + sx ] * weight;
								}
								samples += weight;
							}
						}
						if ( samples <= 0 ) {
							continue;
						}
						SuperLuxel& ziel = lm->getSuperLuxel( a.lightmapNum, x, y );
						ziel.count = 1;
						if ( a.light->flags & LightFlags::Negative ) {
							ziel.value -= averageColor / samples;
						}
						else
						{
							ziel.value += averageColor / samples;
						}
						if ( deluxemap ) {
							lm->getSuperDeluxel( x, y ) += averageDir / samples;
						}
					}
					else
					{
						const SuperLuxel& lightLuxel = luxel[ y * sw + x ];
						SuperLuxel& ziel = lm->getSuperLuxel( a.lightmapNum, x, y );
						ziel.count = 1;
						if ( a.light->flags & LightFlags::Negative ) {
							ziel.value -= lightLuxel.value;
						}
						else{
							ziel.value += lightLuxel.value;
						}
						if ( deluxemap ) {
							lm->getSuperDeluxel( x, y ) += deluxel[ y * sw + x ];
						}
					}
				}
			}
		} );
	}
}

/* BehavEd stage 7 (speed, same result): the big raw lightmaps are lit in advance with
   BhedLichterParallel (BhedIlluminateRawLightmaps below); the normal threaded pass then skips
   them. g_bhedVorab[ n ] != 0: lightmap n is already done. */
static std::vector<char> g_bhedVorab;
static void IlluminateRawLightmap_( int rawLightmapNum, bhed_par::Pool *pool );

/* BehavEd stage 8g: -bhedgpubake - the finished luxels of one raw lightmap from the GPU (the whole light
   loop of IlluminateRawLightmap ran there): 4 floats per mapped luxel (row order) and used style slot */
struct BhedBakeFertig
{
	bool da = false;
	const float *luxel = nullptr;
	int aus[ MAX_LIGHTMAPS ] = { -1, -1, -1, -1 };
	int numLights = 0;
	int warnungen = 0;              /* "Hit per-surface style limit" of the style check */
	Array4<byte> styles{};
	size_t paare = 0;               /* lights * mapped luxels: the default check takes only small lightmaps */
	bool pruefen = false;           /* the first lightmap of a round is always checked (small maps get a report too) */
};
static std::vector<BhedBakeFertig> g_bhedBakeFertig;
/* the deviation report: every N-th lightmap (-bhedgpubakecheck) is computed a second time on the CPU */
static std::atomic<long long> g_bbProbenLm{ 0 }, g_bbProbenLuxel{ 0 }, g_bbGleichLuxel{ 0 }, g_bbLmAnders{ 0 }, g_bbStileAnders{ 0 }, g_bbRunden{ 0 };
static std::atomic<long long> g_bbGpuLm{ 0 }, g_bbCpuLm{ 0 }, g_bbAlphaLm{ 0 }, g_bbNichtEndlichLm{ 0 }, g_bbZuGrossLm{ 0 }, g_bbLeerLm{ 0 }, g_bbGeteiltLm{ 0 };
static std::atomic<long long> g_bbSummeMikro{ 0 };   /* sum of |difference| in 1e-6 units */
static std::mutex g_bbMaxMutex;
static float g_bbMaxAbw = 0;
static std::vector<std::pair<int, long long>> g_bbAndersListe;   /* lightmap, different luxels (for <map>.bhedgpu.txt) */
/* BHED_GPUBAKE_ZEITEN: where the time of BhedIlluminateGpu goes (seconds): lists, blocks, initial pass,
   style check, rest on the GPU, finishing on the CPU, lightmaps on the CPU */
static double g_bbZeit[ 8 ] = {};
static double BhedJetzt(){
	return std::chrono::duration<double>( std::chrono::steady_clock::now().time_since_epoch() ).count();
}

/* the GPU result into the lightmap - instead of the light loop (the luxels were ambient/zero before) */
static void BhedBakeEinsetzen( rawLightmap_t *lm, const BhedBakeFertig& f ){
	for ( int w = 0; w < f.warnungen; ++w ) {
		Sys_Warning( "Hit per-surface style limit (%d)\n", MAX_LIGHTMAPS );
	}
	const size_t size = lm->sw * lm->sh * sizeof( *lm->superLuxels[0] );
	for ( int s = 0; s < MAX_LIGHTMAPS; ++s )
	{
		if ( f.aus[ s ] < 0 ) {
			continue;
		}
		if ( lm->superLuxels[ s ] == nullptr ) {
			lm->superLuxels[ s ] = safe_calloc( size );
		}
		if ( s > 0 ) {
			lm->styles[ s ] = f.styles[ s ];
		}
		const float *q = f.luxel + size_t( f.aus[ s ] ) * 4;
		for ( int y = 0; y < lm->sh; ++y )
		{
			for ( int x = 0; x < lm->sw; ++x )
			{
				if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
					continue;
				}
				SuperLuxel& l = lm->getSuperLuxel( s, x, y );
				l.value = Vector3( q[ 0 ], q[ 1 ], q[ 2 ] );
				l.count = 1;
				q += 4;
			}
		}
	}
}
static void BhedIlluminateGpu();

void IlluminateRawLightmap( int rawLightmapNum ){
	if ( rawLightmapNum < int( g_bhedVorab.size() ) && g_bhedVorab[ rawLightmapNum ] ) {
		return;
	}
	IlluminateRawLightmap_( rawLightmapNum, nullptr );
}

static bhed_par::Pool& BhedPool(){
	return bhed_par::pool( numthreads );
}

void BhedIlluminateRawLightmaps(){
	/* BehavEd stage 8c/8g: -bhedgpubake (not with deluxemaps, random samples or the debug fills) */
	if ( g_bhedBakeAn && BhedBakeBereit() && !deluxemap && !lightRandomSamples &&
	     !( debugSurfaces || debugAxis || debugCluster || debugOrigin || dirtDebug || normalmap ) ) {
		BhedIlluminateGpu();
		return;
	}
	g_bhedVorab.assign( numRawLightmaps, 0 );
	if ( numthreads > 1 && !lightRandomSamples &&
	     !( debugSurfaces || debugAxis || debugCluster || debugOrigin || dirtDebug || normalmap ) ) {
		double total = 0;
		for ( int i = 0; i < numRawLightmaps; ++i )
			total += double( rawLightmaps[ i ].sw ) * rawLightmaps[ i ].sh;
		/* big: more luxels than the share of one thread - such a lightmap would keep one
		   thread busy while the others run out of work */
		const double grenze = total / numthreads;
		int gross = 0;
		for ( int i = 0; i < numRawLightmaps; ++i )
		{
			if ( double( rawLightmaps[ i ].sw ) * rawLightmaps[ i ].sh > grenze ) {
				IlluminateRawLightmap_( i, &BhedPool() );
				g_bhedVorab[ i ] = 1;
				++gross;
			}
		}
		if ( getenv( "BHED_Q3_ZEITEN" ) != nullptr ) { /* for the determinism ctest: the path really ran */
			Sys_Printf( "BHED-Parallel: %d\n", gross );
		}
	}
	RunThreadsOnIndividual( numRawLightmaps, true, IlluminateRawLightmap );
	g_bhedVorab.clear();
}

/* the state IlluminateRawLightmap changes: styles, the style lightmaps, the clusters (the filter pass
   floods unmapped luxels) - for the deviation report */
struct BhedLmStand
{
	Array4<byte> styles{};
	std::vector<SuperLuxel> lux[ MAX_LIGHTMAPS ];
	bool da[ MAX_LIGHTMAPS ] = {};
	std::vector<int> cluster;
};
static void BhedStandMerken( const rawLightmap_t *lm, BhedLmStand& s ){
	const size_t n = size_t( lm->sw ) * lm->sh;
	s.styles = lm->styles;
	for ( int i = 0; i < MAX_LIGHTMAPS; ++i )
	{
		s.da[ i ] = lm->superLuxels[ i ] != nullptr;
		s.lux[ i ].assign( s.da[ i ] ? lm->superLuxels[ i ] : nullptr, s.da[ i ] ? lm->superLuxels[ i ] + n : nullptr );
	}
	s.cluster.assign( lm->superClusters, lm->superClusters + n );
}
static void BhedStandSetzen( rawLightmap_t *lm, const BhedLmStand& s ){
	const size_t n = size_t( lm->sw ) * lm->sh;
	lm->styles = s.styles;
	for ( int i = 0; i < MAX_LIGHTMAPS; ++i )
	{
		if ( !s.da[ i ] ) {
			free( lm->superLuxels[ i ] );
			lm->superLuxels[ i ] = nullptr;
			continue;
		}
		if ( lm->superLuxels[ i ] == nullptr ) {
			lm->superLuxels[ i ] = safe_malloc( n * sizeof( SuperLuxel ) );
		}
		memcpy( static_cast<void*>( lm->superLuxels[ i ] ), s.lux[ i ].data(), n * sizeof( SuperLuxel ) );
	}
	memcpy( lm->superClusters, s.cluster.data(), n * sizeof( int ) );
}

/* finish one raw lightmap with the GPU result; every N-th one is computed on the CPU too and compared
   luxel by luxel (the GPU result stays) - with N > 1 only lightmaps up to 1 M pairs of light and luxel,
   so that the check costs a few percent (one thread per lightmap), and the first small one of every round;
   -bhedgpubakecheck 1 checks all, 0 none */
static void BhedBakeFertigstellen( int r ){
	const int n = g_bhedBakePruefen;
	if ( n <= 0 || ( !g_bhedBakeFertig[ r ].pruefen && ( ( r + int( g_bbRunden.load() ) ) % n != 0 || ( n > 1 && g_bhedBakeFertig[ r ].paare > ( size_t( 1 ) << 20 ) ) ) ) ) {
		IlluminateRawLightmap_( r, nullptr );
		return;
	}
	rawLightmap_t *lm = &rawLightmaps[ r ];
	BhedLmStand vorher, gpu, cpu;
	BhedStandMerken( lm, vorher );
	IlluminateRawLightmap_( r, nullptr );
	BhedStandMerken( lm, gpu );
	BhedStandSetzen( lm, vorher );
	const BhedBakeFertig f = g_bhedBakeFertig[ r ];
	g_bhedBakeFertig[ r ].da = false;
	IlluminateRawLightmap_( r, nullptr );
	g_bhedBakeFertig[ r ] = f;
	BhedStandMerken( lm, cpu );
	BhedStandSetzen( lm, gpu );
	long long luxel = 0, gleich = 0;
	float maxAbw = 0;
	double summe = 0;
	const size_t m = size_t( lm->sw ) * lm->sh;
	for ( int i = 0; i < MAX_LIGHTMAPS; ++i )
	{
		if ( !gpu.da[ i ] && !cpu.da[ i ] ) {
			continue;
		}
		for ( size_t k = 0; k < m; ++k )
		{
			const SuperLuxel a = gpu.da[ i ] ? gpu.lux[ i ][ k ] : SuperLuxel{};
			const SuperLuxel b = cpu.da[ i ] ? cpu.lux[ i ][ k ] : SuperLuxel{};
			++luxel;
			if ( gpu.da[ i ] == cpu.da[ i ] && memcmp( &a, &b, sizeof( SuperLuxel ) ) == 0 ) {
				++gleich;
				continue;
			}
			for ( int c = 0; c < 3; ++c ) {
				const float d = std::fabs( a.value[ c ] - b.value[ c ] );
				if ( std::isfinite( d ) ) {
					maxAbw = std::max( maxAbw, d );
					summe += d;
				}
			}
		}
	}
	const bool stile = gpu.styles != cpu.styles || gpu.cluster != cpu.cluster;
	++g_bbProbenLm;
	g_bbProbenLuxel += luxel;
	g_bbGleichLuxel += gleich;
	g_bbSummeMikro += static_cast<long long>( summe * 1e6 );
	if ( stile ) {
		++g_bbStileAnders;
	}
	if ( gleich != luxel || stile ) {
		++g_bbLmAnders;
		std::lock_guard<std::mutex> sperre( g_bbMaxMutex );
		g_bbMaxAbw = std::max( g_bbMaxAbw, maxAbw );
		g_bbAndersListe.emplace_back( r, luxel - gleich );
	}
}

/* BehavEd stage 8g: the light table of this round for the GPU (lightmaps and light grid): all lights in
   list order, with the switches; nr/zeiger (optional): number of every light and back */
bool BhedBakeLichtTafel( void *nrZeiger, void *zeigerZeiger ){
	auto *nr = static_cast<std::unordered_map<const light_t*, int>*>( nrZeiger );
	auto *zeiger = static_cast<std::vector<const light_t*>*>( zeigerZeiger );
	std::vector<BhedBakeLicht> tafel;
	std::vector<float> windungen;
	for ( const light_t& l : lights )
	{
		BhedBakeLicht b{};
		b.typ = int( l.type );
		b.flags = int( static_cast<std::uint32_t>( l.flags ) );
		b.cluster = l.cluster;
		b.windErste = int( windungen.size() / 3 );
		b.windAnzahl = int( l.w.size() );
		for ( const Vector3& p : l.w ) {
			windungen.push_back( p[ 0 ] );
			windungen.push_back( p[ 1 ] );
			windungen.push_back( p[ 2 ] );
		}
		for ( int j = 0; j < 3; ++j ) {
			b.o[ j ] = l.origin[ j ];
			b.n[ j ] = l.normal[ j ];
			b.c[ j ] = l.color[ j ];
			b.mins[ j ] = l.minmax.mins[ j ];
			b.maxs[ j ] = l.minmax.maxs[ j ];
		}
		b.dist = l.dist;
		b.photons = l.photons;
		b.radiusByDist = l.radiusByDist;
		b.fade = l.fade;
		b.angleScale = l.angleScale;
		b.extraDist = l.extraDist;
		b.add = l.add;
		b.envelope = l.envelope;
		b.falloffTol = l.falloffTolerance;
		b.skyIndex = l.skyIndex;
		b.style = l.style;
		if ( nr != nullptr ) {
			nr->emplace( &l, int( tafel.size() ) );
		}
		if ( zeiger != nullptr ) {
			zeiger->push_back( &l );
		}
		tafel.push_back( b );
	}
	const BhedBakeSchalter schalter{ faster ? 1 : 0, lightAngleHL ? 1 : 0, bouncing ? 1 : 0, noTrace ? 0 : 1, g_oneSky ? 1 : 0, noSurfaces ? 1 : 0, linearScale,
	                                 lightSamples, 0.25f * lightSamplesSearchBoxSize };
	return BhedBakeLichter( tafel, windungen, schalter, BhedBakeInfos() );
}

static long long g_bbGitterGpu = 0, g_bbGitterCpu = 0;
void BhedBakeGitterZaehlen( long long gpu, long long cpu ){
	g_bbGitterGpu += gpu;
	g_bbGitterCpu += cpu;
}

/* BehavEd stage 8g: -bhedgpubake - the culled light list of CreateTraceLightsForBounds for every raw
   lightmap, faster: a bounding volume tree over the light spheres finds the lights whose sphere can reach
   the lightmap's sphere (with a margin, so never fewer), then exactly the tests of CreateTraceLightsForBounds
   run on these candidates in list order - the same list. On md_am_sith (up to 63000 lights, 127000
   lightmaps per round) the plain loop took 9 of 29 seconds. */
struct BhedLichtBaum
{
	struct Knoten
	{
		float mins[ 3 ], maxs[ 3 ];
		int links = -1, rechts = -1;   /* inner node: children; leaf: links = -1, erstes/anzahl */
		int erstes = 0, anzahl = 0;
	};
	std::vector<Knoten> knoten;
	std::vector<int> nr;               /* light numbers in leaf order */
	std::vector<int> immer;            /* suns: no sphere test */
	std::vector<std::array<float, 6>> kasten;   /* per light: sphere box with margin */

	void bauen( const std::vector<const light_t*>& zeiger ){
		knoten.clear();
		nr.clear();
		immer.clear();
		kasten.assign( zeiger.size(), {} );
		for ( size_t i = 0; i < zeiger.size(); ++i )
		{
			const light_t& l = *zeiger[ i ];
			if ( l.envelope <= 0 || !( l.flags & LightFlags::Surfaces ) ) {
				continue;   /* CreateTraceLightsForBounds never takes these */
			}
			if ( l.type == ELightType::Sun ) {
				immer.push_back( int( i ) );
				continue;
			}
			const float r = l.envelope * 1.001f + 1.0f;   /* margin: the test runs exactly in double later */
			kasten[ i ] = { l.origin[ 0 ] - r, l.origin[ 1 ] - r, l.origin[ 2 ] - r, l.origin[ 0 ] + r, l.origin[ 1 ] + r, l.origin[ 2 ] + r };
			nr.push_back( int( i ) );
		}
		if ( !nr.empty() ) {
			knoten.reserve( nr.size() * 2 / 4 + 2 );
			teilen( 0, int( nr.size() ) );
		}
	}
	int teilen( int von, int bis ){
		const int k = int( knoten.size() );
		knoten.emplace_back();
		Knoten kn;
		for ( int j = 0; j < 3; ++j ) {
			kn.mins[ j ] = std::numeric_limits<float>::max();
			kn.maxs[ j ] = -std::numeric_limits<float>::max();
		}
		for ( int i = von; i < bis; ++i )
		{
			const auto& b = kasten[ nr[ i ] ];
			for ( int j = 0; j < 3; ++j ) {
				kn.mins[ j ] = std::min( kn.mins[ j ], b[ j ] );
				kn.maxs[ j ] = std::max( kn.maxs[ j ], b[ 3 + j ] );
			}
		}
		if ( bis - von <= 4 ) {
			kn.erstes = von;
			kn.anzahl = bis - von;
			knoten[ k ] = kn;
			return k;
		}
		int achse = 0;
		float weite = -1;
		for ( int j = 0; j < 3; ++j ) {
			if ( kn.maxs[ j ] - kn.mins[ j ] > weite ) {
				weite = kn.maxs[ j ] - kn.mins[ j ];
				achse = j;
			}
		}
		const int mitte = ( von + bis ) / 2;
		std::nth_element( nr.begin() + von, nr.begin() + mitte, nr.begin() + bis, [&]( int a, int b ){
			const float ma = kasten[ a ][ achse ] + kasten[ a ][ 3 + achse ];
			const float mb = kasten[ b ][ achse ] + kasten[ b ][ 3 + achse ];
			return ma < mb || ( ma == mb && a < b );
		} );
		kn.links = teilen( von, mitte );
		kn.rechts = teilen( mitte, bis );
		knoten[ k ] = kn;
		return k;
	}
	/* the lights whose box overlaps mins..maxs, plus the suns; ascending */
	void suchen( const float mins[ 3 ], const float maxs[ 3 ], std::vector<int>& aus ) const {
		aus = immer;
		if ( knoten.empty() ) {
			return;
		}
		int stapel[ 128 ];
		int sp = 0;
		stapel[ sp++ ] = 0;
		while ( sp > 0 )
		{
			const Knoten& kn = knoten[ stapel[ --sp ] ];
			if ( kn.mins[ 0 ] > maxs[ 0 ] || kn.mins[ 1 ] > maxs[ 1 ] || kn.mins[ 2 ] > maxs[ 2 ] ||
			     kn.maxs[ 0 ] < mins[ 0 ] || kn.maxs[ 1 ] < mins[ 1 ] || kn.maxs[ 2 ] < mins[ 2 ] ) {
				continue;
			}
			if ( kn.links < 0 ) {
				for ( int i = kn.erstes; i < kn.erstes + kn.anzahl; ++i )
				{
					const auto& b = kasten[ nr[ i ] ];
					if ( b[ 0 ] <= maxs[ 0 ] && b[ 1 ] <= maxs[ 1 ] && b[ 2 ] <= maxs[ 2 ] && b[ 3 ] >= mins[ 0 ] && b[ 4 ] >= mins[ 1 ] && b[ 5 ] >= mins[ 2 ] ) {
						aus.push_back( nr[ i ] );
					}
				}
				continue;
			}
			if ( sp + 2 > 128 ) {   /* deeper than ever needed: take everything (still exact) */
				aus.assign( immer.begin(), immer.end() );
				aus.insert( aus.end(), nr.begin(), nr.end() );
				break;
			}
			stapel[ sp++ ] = kn.links;
			stapel[ sp++ ] = kn.rechts;
		}
		std::sort( aus.begin(), aus.end() );
	}
};

/* CreateTraceLightsForBounds( lm->minmax, plane normal, lm clusters, LightFlags::Surfaces ) on the candidates */
static void BhedLichtListe( const rawLightmap_t *lm, bool twoSided, const BhedLichtBaum& baum, const std::vector<const light_t*>& zeiger,
                            std::vector<int>& kandidaten, std::vector<int>& liste ){
	const MinMax& minmax = lm->minmax;
	const Vector3 *normal = lm->plane == nullptr ? nullptr : &lm->plane->normal();
	const int numClusters = lm->numLightClusters;
	const int *clusters = lm->lightClusters;
	const Vector3 origin = minmax.origin();
	const float radius = vector3_length( minmax.maxs - origin );
	float length;
	if ( normal != nullptr ) {
		length = vector3_length( *normal );
	}
	else{
		normal = &g_vector3_identity;
		length = 0;
	}
	const float r = radius * 1.001f + 1.0f;
	const float mins[ 3 ] = { origin[ 0 ] - r, origin[ 1 ] - r, origin[ 2 ] - r };
	const float maxs[ 3 ] = { origin[ 0 ] + r, origin[ 1 ] + r, origin[ 2 ] + r };
	baum.suchen( mins, maxs, kandidaten );
	liste.clear();
	for ( const int k : kandidaten )
	{
		const light_t& light = *zeiger[ k ];
		if ( light.envelope <= 0 ) {
			continue;
		}
		if ( !( light.flags & LightFlags::Surfaces ) ) {
			continue;
		}
		if ( light.type != ELightType::Sun ) {
			if ( sunOnly ) {
				continue;
			}
			if ( numClusters > 0 && clusters != nullptr ) {
				int i;
				for ( i = 0; i < numClusters; ++i )
				{
					if ( ClusterVisible( light.cluster, clusters[ i ] ) ) {
						break;
					}
				}
				if ( i == numClusters ) {
					continue;
				}
			}
			if ( vector3_length( light.origin - origin ) - light.envelope - radius > 0 ) {
				continue;
			}
		}
		if ( length > 0 && !twoSided ) {
			if ( !( light.flags & LightFlags::Twosided ) && vector3_dot( light.normal, *normal ) > 0.999f ) {
				continue;
			}
			if ( vector3_dot( light.origin, *normal ) - vector3_dot( origin, *normal ) < -1 ) {
				continue;
			}
		}
		liste.push_back( k );
	}
}

/* BehavEd stage 8g: one raw lightmap (or a part of its light list) into a GPU block */
static void BhedBlockLm( BhedBakeBlock& b, int r, const BhedBakeLm& info, int m, const int *lichter, int nLichter, size_t paarBase ){
	const rawLightmap_t *lm = &rawLightmaps[ r ];
	BhedBakeLm L = info;
	L.sw = lm->sw;
	L.sh = lm->sh;
	L.m = m;
	L.luxelBase = int( b.luxel.size() );
	L.idxBase = int( b.idx.size() );
	L.lichtBase = int( b.lichtListe.size() );
	L.nLichter = nLichter;
	L.paarBase = int( paarBase );
	L.clusterBase = int( b.lmCluster.size() );
	L.nClusters = lm->lightClusters != nullptr ? lm->numLightClusters : 0;
	for ( int j = 0; j < 3; ++j ) {
		L.ambient[ j ] = lm->ambientColor[ j ];
	}
	for ( int& a : L.aus ) {
		a = -1;
	}
	L.start = 0;
	const int li = int( b.lms.size() );
	int k = 0;
	for ( int y = 0; y < lm->sh; ++y )
	{
		for ( int x = 0; x < lm->sw; ++x )
		{
			const int cluster = lm->getSuperCluster( x, y );
			if ( cluster < CLUSTER_NORMAL ) {
				b.idx.push_back( -1 );
				continue;
			}
			b.idx.push_back( k++ );
			BhedBakeLuxel l{};
			const Vector3& o = lm->getSuperOrigin( x, y );
			const Vector3& n = lm->getSuperNormal( x, y );
			for ( int j = 0; j < 3; ++j ) {
				l.o[ j ] = o[ j ];
				l.n[ j ] = n[ j ];
			}
			l.cluster = cluster;
			l.lm = li;
			l.x = x;
			l.y = y;
			b.luxel.push_back( l );
		}
	}
	b.lichtListe.insert( b.lichtListe.end(), lichter, lichter + nLichter );
	if ( L.nClusters > 0 ) {
		b.lmCluster.insert( b.lmCluster.end(), lm->lightClusters, lm->lightClusters + L.nClusters );
	}
	b.lms.push_back( L );
}

/* the style check of the light loop in light order (style slot, "nothing lit", set style) for the lights of
   lightmap li of the block, starting with the given styles */
struct BhedStil
{
	std::vector<BhedBakeAktiv> aktiv;
	Array4<byte> styles{};
	int warnungen = 0;
};
static void BhedStilPruefen( const rawLightmap_t *lm, const BhedBakeBlock& b, int li, const std::vector<std::uint32_t>& zaehler,
                             const std::vector<const light_t*>& zeiger, BhedStil& s ){
	const BhedBakeLm& L = b.lms[ li ];
	for ( int j = 0; j < L.nLichter; ++j )
	{
		const light_t *light = zeiger[ b.lichtListe[ L.lichtBase + j ] ];
		int lightmapNum;
		for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			if ( s.styles[ lightmapNum ] == light->style ||
			     s.styles[ lightmapNum ] == LS_NONE ) {
				break;
			}
		}
		if ( lightmapNum >= MAX_LIGHTMAPS ) {
			++s.warnungen;
			continue;
		}
		const float filterRadius = std::max( { 0.f, lm->filterRadius, light->filterRadius } );
		int luxelFilterRadius = lm->sampleSize != 0 ? superSample * filterRadius / lm->sampleSize : 0;
		if ( luxelFilterRadius == 0 && ( filterRadius > 0 || filter ) ) {
			luxelFilterRadius = 1;
		}
		if ( zaehler[ L.lichtBase + j ] == 0 ) {
			continue;
		}
		if ( lightmapNum > 0 ) {
			s.styles[ lightmapNum ] = light->style;
		}
		s.aktiv.push_back( { j, lightmapNum, luxelFilterRadius, ( light->flags & LightFlags::Negative ) ? 1 : 0, 0, {} } );
	}
}

/* the active lights of every lightmap of the block into the block (aktiv, rows of the stamp scan, output slots);
   flags[ li ] != 0: the CPU does that lightmap, nothing on the GPU */
static void BhedAktivSetzen( BhedBakeBlock& b, const std::vector<BhedStil>& stil, const std::vector<std::uint32_t>& flags ){
	b.aktivNr.assign( b.lichtListe.size(), -1 );
	b.aktiv.clear();
	std::uint32_t zeilen = 0, ausAnzahl = 0, maske = 0;
	for ( size_t li = 0; li < b.lms.size(); ++li )
	{
		BhedBakeLm& L = b.lms[ li ];
		L.aktivBase = int( b.aktiv.size() );
		L.zeilenBase = int( zeilen );
		L.nAktiv = 0;
		if ( flags[ li ] != 0 ) {
			continue;
		}
		bool benutzt[ MAX_LIGHTMAPS ] = {};
		const bool proben = lightSamples > 1 && L.sw > 1 && L.sh > 1;
		for ( BhedBakeAktiv a : stil[ li ].aktiv )
		{
			b.aktivNr[ L.lichtBase + a.j ] = int( b.aktiv.size() );
			a.maske = int( maske );
			if ( proben ) {
				maske += std::uint32_t( L.sw - 1 ) * std::uint32_t( L.sh - 1 );
			}
			b.aktiv.push_back( a );
			benutzt[ a.slot ] = true;
		}
		L.nAktiv = int( stil[ li ].aktiv.size() );
		if ( proben ) {
			zeilen += std::uint32_t( L.nAktiv ) * std::uint32_t( L.sh - 1 );
		}
		for ( int s = 0; s < MAX_LIGHTMAPS; ++s )
		{
			if ( benutzt[ s ] ) {
				L.aus[ s ] = int( ausAnzahl );
				ausAnzahl += std::uint32_t( L.m );
			}
		}
	}
	b.zeilen = zeilen;
	b.aus = ausAnzahl;
	b.maskeBits = maske;
}

/* are all output luxels of lightmap li finite? */
static bool BhedEndlich( const BhedBakeLm& L, const std::vector<float>& aus ){
	for ( int s = 0; s < MAX_LIGHTMAPS; ++s )
	{
		if ( L.aus[ s ] < 0 ) {
			continue;
		}
		for ( size_t q = size_t( L.aus[ s ] ) * 4; q < ( size_t( L.aus[ s ] ) + L.m ) * 4; ++q ) {
			if ( !std::isfinite( aus[ q ] ) ) {
				return false;
			}
		}
	}
	return true;
}

/* BehavEd stage 8g: -bhedgpubake - the whole light loop of IlluminateRawLightmap on the GPU, in blocks of
   raw lightmaps (as many pairs of light and luxel as the graphics memory takes, see BhedBakeBudgetPaare):
   initial pass, the style check in light order here on the CPU (with the lit counts from the GPU),
   adaptive supersampling and filtering/adding on the GPU; the finished luxels come back and the rest of
   IlluminateRawLightmap runs on all threads as always. A lightmap the GPU cannot do exactly (alpha shadow,
   light filter, a division it cannot round, not finite) or that does not fit, and everything after a lost
   device, is computed on the CPU as always. */
static void BhedIlluminateGpu(){
	bhed_par::Pool& pool = BhedPool();
	++g_bbRunden;
	double t = BhedJetzt();
	const auto zeit = [&]( int i ){
		const double jetzt = BhedJetzt();
		g_bbZeit[ i ] += jetzt - t;
		t = jetzt;
	};
	/* the lights of this round */
	std::unordered_map<const light_t*, int> nr;
	std::vector<const light_t*> zeiger;
	/* -samples deeper than the subsampling stack of the GPU (MAXTIEFE + 1 in bhed_gpubacken.hlsl): the CPU */
	const bool gpu = lightSamples <= 7 && BhedBakeLichtTafel( &nr, &zeiger );
	/* the culled light list of every raw lightmap (as IlluminateRawLightmap builds it) */
	std::vector<std::vector<int>> liste( numRawLightmaps );
	std::vector<BhedBakeLm> lmInfo( numRawLightmaps );
	std::vector<int> abgebildet( numRawLightmaps, 0 );
	BhedLichtBaum baum;
	std::atomic<long long> listenAnders{ 0 };
	const bool listenPruefen = getenv( "BHED_GPUBAKE_LISTENPRUEFEN" ) != nullptr;   /* test: the list against CreateTraceLightsForBounds */
	if ( gpu ) {
		baum.bauen( zeiger );
		pool.fuerAlle( numRawLightmaps, [&]( int r ){
			rawLightmap_t *lm = &rawLightmaps[ r ];
			trace_t trace{};
			trace.recvShadows = lm->recvShadows;
			trace.numSurfaces = lm->numLightSurfaces;
			trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
			trace.twoSided = false;
			for ( int i : Span( trace.surfaces, trace.numSurfaces ) )
			{
				if ( surfaceInfos[ i ].si->twoSided ) {
					trace.twoSided = true;
					break;
				}
			}
			std::vector<int> kandidaten;
			BhedLichtListe( lm, trace.twoSided, baum, zeiger, kandidaten, liste[ r ] );
			if ( listenPruefen ) {
				CreateTraceLightsForBounds( lm->minmax, ( lm->plane == nullptr? nullptr : &lm->plane->normal() ), lm->numLightClusters, lm->lightClusters, LightFlags::Surfaces, &trace );
				std::vector<int> alt;
				for ( int i = 0; i < trace.numLights; ++i ) {
					alt.push_back( nr.at( trace.lights[ i ] ) );
				}
				FreeTraceLights( &trace );
				if ( alt != liste[ r ] ) {
					++listenAnders;
				}
			}
			BhedBakeLm& L = lmInfo[ r ];
			L = BhedBakeLm{};
			L.recv = lm->recvShadows;
			L.sErste = lm->firstLightSurface;
			L.sAnzahl = lm->numLightSurfaces;
			L.twoSided = trace.twoSided ? 1 : 0;
			int m = 0;
			for ( int y = 0; y < lm->sh; ++y )
				for ( int x = 0; x < lm->sw; ++x )
					m += lm->getSuperCluster( x, y ) >= CLUSTER_NORMAL ? 1 : 0;
			abgebildet[ r ] = m;
		} );
	}
	if ( listenPruefen ) {
		Sys_Printf( "BHED-GPU-Listen: %lld of %d lists different\n", listenAnders.load(), numRawLightmaps );
	}
	zeit( 0 );
	g_bhedBakeFertig.assign( numRawLightmaps, BhedBakeFertig{} );
	std::vector<char> cpu( numRawLightmaps, 0 );
	std::vector<int> gpuListe, leer, gross;
	const size_t budget = BhedBakeBudgetPaare();
	for ( int r = 0; r < numRawLightmaps; ++r )
	{
		const size_t bedarf = size_t( abgebildet[ r ] ) * liste[ r ].size();
		if ( !gpu ) {
			cpu[ r ] = 1;
		}
		else if ( bedarf == 0 ) {
			leer.push_back( r );   /* no light or no mapped luxel: nothing to light, see below */
		}
		else if ( bedarf > budget && size_t( abgebildet[ r ] ) > budget ) {
			cpu[ r ] = 1;   /* not even one light fits into a block */
			++g_bbZuGrossLm;
		}
		else if ( bedarf > budget ) {
			gross.push_back( r );   /* the light list in parts, see below */
		}
		else{
			gpuListe.push_back( r );
		}
	}
	/* nothing to light: the light loop only checks the styles (warnings), the list is known - no second
	   CreateTraceLightsForBounds; the rest of IlluminateRawLightmap as always */
	pool.fuerAlle( int( leer.size() ), [&]( int i ){
		const int r = leer[ i ];
		const rawLightmap_t *lm = &rawLightmaps[ r ];
		BhedBakeFertig& f = g_bhedBakeFertig[ r ];
		f.da = true;
		f.numLights = int( liste[ r ].size() );
		f.styles = lm->styles;
		for ( int t : liste[ r ] )
		{
			int lightmapNum;
			for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
			{
				if ( lm->styles[ lightmapNum ] == zeiger[ t ]->style || lm->styles[ lightmapNum ] == LS_NONE ) {
					break;
				}
			}
			f.warnungen += lightmapNum >= MAX_LIGHTMAPS ? 1 : 0;
		}
		BhedBakeFertigstellen( r );
		g_bhedBakeFertig[ r ] = BhedBakeFertig{};
	} );
	g_bbLeerLm += static_cast<long long>( leer.size() );
	size_t pos = 0;
	bool ersteGeprueft = false;
	std::vector<std::uint32_t> zaehler, flags, flags2;
	std::vector<float> aus;
	while ( pos < gpuListe.size() && BhedBakeBereit() )
	{
		/* the next block */
		BhedBakeBlock b;
		std::vector<int> nrs;
		size_t paare = 0;
		while ( pos < gpuListe.size() )
		{
			const int r = gpuListe[ pos ];
			const size_t bedarf = size_t( abgebildet[ r ] ) * liste[ r ].size();
			if ( !nrs.empty() && paare + bedarf > budget ) {
				break;
			}
			BhedBlockLm( b, r, lmInfo[ r ], abgebildet[ r ], liste[ r ].data(), int( liste[ r ].size() ), paare );
			nrs.push_back( r );
			paare += bedarf;
			++pos;
		}
		b.paare = std::uint32_t( paare );
		zeit( 1 );
		/* 1. initial pass on the GPU */
		if ( !BhedBakeErst( b, zaehler, flags ) ) {
			pos -= nrs.size();   /* lost: this block and the rest on the CPU */
			break;
		}
		zeit( 2 );
		/* 2. the style check in light order (IlluminateRawLightmap: style slot, "nothing lit", set style) */
		std::vector<BhedStil> stil( nrs.size() );
		pool.fuerAlle( int( nrs.size() ), [&]( int li ){
			if ( flags[ li ] != 0 ) {
				return;
			}
			const rawLightmap_t *lm = &rawLightmaps[ nrs[ li ] ];
			stil[ li ].styles = lm->styles;
			BhedStilPruefen( lm, b, li, zaehler, zeiger, stil[ li ] );
		} );
		BhedAktivSetzen( b, stil, flags );
		zeit( 3 );
		/* 3. and 4. supersampling, filtering and adding on the GPU; the finished luxels back */
		if ( !BhedBakeRest( b, aus, flags2 ) ) {
			pos -= nrs.size();
			break;
		}
		zeit( 4 );
		for ( size_t li = 0; li < nrs.size(); ++li )
		{
			const int r = nrs[ li ];
			const BhedBakeLm& L = b.lms[ li ];
			if ( flags[ li ] != 0 || flags2[ li ] != 0 ) {
				cpu[ r ] = 1;
				++g_bbAlphaLm;
				continue;
			}
			if ( !BhedEndlich( L, aus ) ) {
				cpu[ r ] = 1;
				++g_bbNichtEndlichLm;
				continue;
			}
			BhedBakeFertig& f = g_bhedBakeFertig[ r ];
			f.da = true;
			f.luxel = aus.data();
			for ( int s = 0; s < MAX_LIGHTMAPS; ++s ) {
				f.aus[ s ] = L.aus[ s ];
			}
			f.numLights = L.nLichter;
			f.paare = size_t( L.m ) * size_t( L.nLichter );
			f.warnungen = stil[ li ].warnungen;
			f.styles = stil[ li ].styles;
			f.pruefen = !ersteGeprueft && f.paare <= ( size_t( 1 ) << 20 );
			ersteGeprueft = ersteGeprueft || f.pruefen;
			++g_bbGpuLm;
		}
		/* the rest of IlluminateRawLightmap on all threads */
		pool.fuerAlle( int( nrs.size() ), [&]( int li ){
			if ( !cpu[ nrs[ li ] ] ) {
				BhedBakeFertigstellen( nrs[ li ] );
			}
		} );
		for ( int r : nrs ) {
			g_bhedBakeFertig[ r ] = BhedBakeFertig{};
		}
		zeit( 5 );
	}
	for ( ; pos < gpuListe.size(); ++pos ) {
		cpu[ gpuListe[ pos ] ] = 1;
	}
	/* lightmaps above the block budget: the light list in parts, each part a block of its own - the style
	   check goes on from part to part, and every part adds onto the luxels of the part before (start values),
	   so every luxel gets the same additions in the same order */
	for ( const int r : gross )
	{
		const rawLightmap_t *lm = &rawLightmaps[ r ];
		const int m = abgebildet[ r ];
		const int n = int( liste[ r ].size() );
		const int teil = int( std::max<size_t>( 1, budget / size_t( m ) ) );
		BhedStil stand;
		stand.styles = lm->styles;
		std::vector<float> summe( size_t( MAX_LIGHTMAPS ) * size_t( m ) * 4, 0.0f );
		bool slotDa[ MAX_LIGHTMAPS ] = {};
		bool ok = BhedBakeBereit();
		bool alpha = false;
		for ( int j0 = 0; j0 < n && ok; j0 += teil )
		{
			const int j1 = std::min( n, j0 + teil );
			BhedBakeBlock b;
			BhedBlockLm( b, r, lmInfo[ r ], m, liste[ r ].data() + j0, j1 - j0, 0 );
			b.paare = std::uint32_t( size_t( m ) * size_t( j1 - j0 ) );
			if ( !BhedBakeErst( b, zaehler, flags ) || flags[ 0 ] != 0 ) {
				alpha = BhedBakeBereit();
				ok = false;
				break;
			}
			std::vector<BhedStil> stil( 1 );
			stil[ 0 ].styles = stand.styles;
			BhedStilPruefen( lm, b, 0, zaehler, zeiger, stil[ 0 ] );
			stand.warnungen += stil[ 0 ].warnungen;
			stand.styles = stil[ 0 ].styles;
			BhedAktivSetzen( b, stil, flags );
			BhedBakeLm& L = b.lms[ 0 ];
			L.start = 1;
			b.startWert.assign( size_t( b.aus ) * 4, 0.0f );
			for ( int s = 0; s < MAX_LIGHTMAPS; ++s )
			{
				if ( L.aus[ s ] < 0 ) {
					continue;
				}
				for ( int k = 0; k < m; ++k ) {
					for ( int c = 0; c < 3; ++c ) {
						b.startWert[ ( size_t( L.aus[ s ] ) + k ) * 4 + c ] = slotDa[ s ] ? summe[ ( size_t( s ) * m + k ) * 4 + c ] : ( s == 0 ? lm->ambientColor[ c ] : 0.0f );
					}
				}
			}
			if ( !BhedBakeRest( b, aus, flags2 ) || flags2[ 0 ] != 0 || !BhedEndlich( L, aus ) ) {
				alpha = BhedBakeBereit();
				ok = false;
				break;
			}
			for ( int s = 0; s < MAX_LIGHTMAPS; ++s )
			{
				if ( L.aus[ s ] >= 0 ) {
					std::copy( aus.begin() + size_t( L.aus[ s ] ) * 4, aus.begin() + ( size_t( L.aus[ s ] ) + m ) * 4, summe.begin() + size_t( s ) * m * 4 );
					slotDa[ s ] = true;
				}
			}
		}
		if ( !ok ) {
			cpu[ r ] = 1;
			g_bbAlphaLm += alpha ? 1 : 0;
			continue;
		}
		BhedBakeFertig& f = g_bhedBakeFertig[ r ];
		f.da = true;
		f.luxel = summe.data();
		for ( int s = 0; s < MAX_LIGHTMAPS; ++s ) {
			f.aus[ s ] = slotDa[ s ] ? s * m : -1;
		}
		f.numLights = n;
		f.paare = size_t( m ) * size_t( n );
		f.warnungen = stand.warnungen;
		f.styles = stand.styles;
		++g_bbGpuLm;
		++g_bbGeteiltLm;
		BhedBakeFertigstellen( r );
		g_bhedBakeFertig[ r ] = BhedBakeFertig{};
	}
	zeit( 7 );
	g_bhedBakeFertig.clear();
	/* the CPU as always: big lightmaps with all threads (stage 7), the others in parallel */
	std::vector<int> rest;
	double total = 0;
	for ( int r = 0; r < numRawLightmaps; ++r )
	{
		if ( cpu[ r ] ) {
			rest.push_back( r );
			total += double( rawLightmaps[ r ].sw ) * rawLightmaps[ r ].sh;
		}
	}
	g_bbCpuLm += static_cast<long long>( rest.size() );
	const double grenze = total / std::max( 1, numthreads );
	std::vector<int> klein;
	for ( int r : rest )
	{
		if ( numthreads > 1 && double( rawLightmaps[ r ].sw ) * rawLightmaps[ r ].sh > grenze ) {
			IlluminateRawLightmap_( r, &pool );
		}
		else{
			klein.push_back( r );
		}
	}
	pool.fuerAlle( int( klein.size() ), [&]( int i ){
		IlluminateRawLightmap_( klein[ i ], nullptr );
	} );
	zeit( 6 );
}

/* BehavEd stage 8c/8g: the deviation report after the bake (stdout and <map>.bhedgpu.txt) */
void BhedBakeBericht( const char *quelle ){
	const BhedBakeZahlen z = BhedBakeStatistik();
	const long long proben = g_bbProbenLuxel.load();
	const long long gleich = g_bbGleichLuxel.load();
	const long long anders = proben - gleich;
	const double mittel = anders > 0 ? double( g_bbSummeMikro.load() ) / 1e6 / double( proben ) : 0.0;
	Sys_Printf( "BehavEd GPU bake: %s, %s, %lld rounds, %lld lightmaps on the GPU (%lld without light, %lld in parts), %lld on the CPU (%lld alpha shadow/unsafe, %lld not finite, %lld too big), "
	            "%lld pairs on the GPU, %lld blocks, %lld dispatches, GPU %.2f s%s%s\n",
	            BhedBakeAdapter().c_str(), BhedBakeDoppelt() ? "double" : "float only", g_bbRunden.load(), g_bbGpuLm.load() + g_bbLeerLm.load(), g_bbLeerLm.load(), g_bbGeteiltLm.load(), g_bbCpuLm.load(),
	            g_bbAlphaLm.load(), g_bbNichtEndlichLm.load(), g_bbZuGrossLm.load(), z.paare, z.bloecke, z.dispatches, double( z.gpuMikros ) / 1e6,
	            z.ausgefallen ? " - failed: " : "", z.ausgefallen ? z.grund.c_str() : "" );
	Sys_Printf( "BehavEd GPU bake report: %lld luxels in %lld lightmaps checked on the CPU, %.4f %% identical; %lld lightmaps with a difference "
	            "(%lld in styles), mean |diff| %.6f, max |diff| %.6f; light grid: %lld points on the GPU, %lld on the CPU\n",
	            proben, g_bbProbenLm.load(), proben > 0 ? 100.0 * double( gleich ) / double( proben ) : 100.0, g_bbLmAnders.load(), g_bbStileAnders.load(),
	            mittel, g_bbMaxAbw, g_bbGitterGpu, g_bbGitterCpu );
	if ( getenv( "BHED_GPUBAKE_ZEITEN" ) != nullptr ) {
		Sys_Printf( "BHED-GPU-Zeiten: Listen %.3f, Bloecke %.3f, Erst %.3f, Stil %.3f, Rest %.3f, Fertig %.3f, CPU %.3f, Teile %.3f; Kerne erst %.3f kandidaten %.3f unter %.3f stempellauf %.3f filtern %.3f gitter %.3f\n",
		            g_bbZeit[ 0 ], g_bbZeit[ 1 ], g_bbZeit[ 2 ], g_bbZeit[ 3 ], g_bbZeit[ 4 ], g_bbZeit[ 5 ], g_bbZeit[ 6 ], g_bbZeit[ 7 ], double( z.kernMikros[ 0 ] ) / 1e6,
		            double( z.kernMikros[ 1 ] ) / 1e6, double( z.kernMikros[ 2 ] ) / 1e6, double( z.kernMikros[ 3 ] ) / 1e6, double( z.kernMikros[ 4 ] ) / 1e6,
		            double( z.kernMikros[ 5 ] ) / 1e6 );
		Sys_Printf( "BHED-GPU-Kandidaten: %lld %lld %lld %lld\n", z.kandidaten[ 0 ], z.kandidaten[ 1 ], z.kandidaten[ 2 ], z.kandidaten[ 3 ] );
	}
	if ( quelle == nullptr ) {
		return;
	}
	const auto pfad = StringStream( PathExtensionless( quelle ), ".bhedgpu.txt" );
	if ( FILE *f = fopen( pfad, "wb" ) ) {
		fprintf( f, "adapter\t%s\ndouble\t%d\nrounds\t%lld\ngpu_lightmaps\t%lld\ncpu_lightmaps\t%lld\nalpha\t%lld\nnot_finite\t%lld\ntoo_big\t%lld\n"
		            "pairs\t%lld\nblocks\t%lld\ndispatches\t%lld\ngpu_seconds\t%.3f\nchecked_lightmaps\t%lld\nchecked_luxels\t%lld\nidentical\t%lld\n"
		            "different_lightmaps\t%lld\nmean\t%.9g\nmax\t%.9g\ngrid_gpu\t%lld\ngrid_cpu\t%lld\nfailed\t%s\n",
		         BhedBakeAdapter().c_str(), BhedBakeDoppelt() ? 1 : 0, g_bbRunden.load(), g_bbGpuLm.load(), g_bbCpuLm.load(), g_bbAlphaLm.load(),
		         g_bbNichtEndlichLm.load(), g_bbZuGrossLm.load(), z.paare, z.bloecke, z.dispatches, double( z.gpuMikros ) / 1e6, g_bbProbenLm.load(), proben,
		         gleich, g_bbLmAnders.load(), mittel, g_bbMaxAbw, g_bbGitterGpu, g_bbGitterCpu, z.ausgefallen ? z.grund.c_str() : "-" );
		std::sort( g_bbAndersListe.begin(), g_bbAndersListe.end() );
		for ( const auto& [ lm, n ] : g_bbAndersListe ) {
			fprintf( f, "lm\t%d\t%lld\n", lm, n );
		}
		fclose( f );
	}
}

/* BehavEd stage 8b: -bhedgpu - record the rays of the initial passes of the lights i0.. (the same
   style check and luxel walk as the light loop below, TraceLine only notes its inputs) until the
   block is full, trace them on the GPU; returns the first light not in the block */
static int BhedGpuAufnehmen( rawLightmap_t *lm, const trace_t& vorlage, int i0, BhedGpuSitzung& s ){
	constexpr size_t block = size_t( 1 ) << 16;
	s.strahlen.clear();
	s.geeignet.clear();
	s.erg.clear();
	s.pos = 0;
	s.kaputt = false;
	trace_t trace = vorlage;
	s.modus = 1;
	g_bhedGpuSitzung = &s;
	int i = i0;
	for ( ; i < trace.numLights && ( i == i0 || s.strahlen.size() < block ); ++i )
	{
		trace.light = trace.lights[ i ];
		int lightmapNum;
		for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			if ( lm->styles[ lightmapNum ] == trace.light->style ||
			     lm->styles[ lightmapNum ] == LS_NONE ) {
				break;
			}
		}
		if ( lightmapNum >= MAX_LIGHTMAPS ) {
			continue;
		}
		for ( int y = 0; y < lm->sh; ++y )
		{
			for ( int x = 0; x < lm->sw; ++x )
			{
				const int cluster = lm->getSuperCluster( x, y );
				if ( cluster < CLUSTER_NORMAL ) {
					continue;
				}
				trace.cluster = cluster;
				trace.origin = lm->getSuperOrigin( x, y );
				trace.normal = lm->getSuperNormal( x, y );
				LightContributionToSample( &trace );
			}
		}
	}
	g_bhedGpuSitzung = nullptr;
	s.modus = 0;
	BhedGpuRechnen( s );
	return i;
}

static void IlluminateRawLightmap_( int rawLightmapNum, bhed_par::Pool *pool ){
	int t, x, y, sx, sy, size, luxelFilterRadius, lightmapNum;
	int                 mapped, lighted, totalLighted;
	rawLightmap_t       *lm;
	bool filterColor, filterDir;
	float               samples, filterRadius, weight;
	Vector3 averageColor, averageDir;
	float tests[ 4 ][ 2 ] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	trace_t trace;
	SuperLuxel stackLightLuxels[ 64 * 64 ];


	/* bail if this number exceeds the number of raw lightmaps */
	if ( rawLightmapNum >= numRawLightmaps ) {
		return;
	}

	/* get lightmap */
	lm = &rawLightmaps[ rawLightmapNum ];

	/* setup trace */
	trace.testOcclusion = !noTrace;
	trace.forceSunlight = false;
	trace.recvShadows = lm->recvShadows;
	trace.numSurfaces = lm->numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
	trace.inhibitRadius = DEFAULT_INHIBIT_RADIUS;

	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	trace.twoSided = false;
	for ( int i : Span( trace.surfaces, trace.numSurfaces ) )
	{
		/* get surface */
		const surfaceInfo_t& info = surfaceInfos[ i ];

		/* check twosidedness */
		if ( info.si->twoSided ) {
			trace.twoSided = true;
			break;
		}
	}

	/* create a culled light list for this raw lightmap */
	/* BehavEd stage 8g: -bhedgpubake - the list was made for the GPU already, and the light loop ran there:
	   only its length is needed (not with -bhedlightcache, which hashes the list) */
	if ( pool == nullptr && rawLightmapNum < int( g_bhedBakeFertig.size() ) && g_bhedBakeFertig[ rawLightmapNum ].da && !BhedLichtCacheAn() ) {
		trace.lights = nullptr;
		trace.numLights = g_bhedBakeFertig[ rawLightmapNum ].numLights;
	}
	else
	CreateTraceLightsForBounds( lm->minmax, ( lm->plane == nullptr? nullptr : &lm->plane->normal() ), lm->numLightClusters, lm->lightClusters, LightFlags::Surfaces, &trace );

	/* BehavEd stage 8a: the same culled lights as in the cached run - the raw luxels are the same */
	if ( BhedLmAusCache( rawLightmapNum, trace ) ) {
		FreeTraceLights( &trace );
		return;
	}

	/* BehavEd stage 8g: -bhedgpubake - the finished luxels of the light loop from the GPU, if this list was computed */
	const BhedBakeFertig *bhedFertig = ( pool == nullptr && rawLightmapNum < int( g_bhedBakeFertig.size() ) && g_bhedBakeFertig[ rawLightmapNum ].da &&
	                                     g_bhedBakeFertig[ rawLightmapNum ].numLights == trace.numLights ) ? &g_bhedBakeFertig[ rawLightmapNum ] : nullptr;

	/* -----------------------------------------------------------------
	   fill pass
	   ----------------------------------------------------------------- */

	/* set counts */
	numLuxelsIlluminated += ( lm->sw * lm->sh );

	/* test debugging state */
	if ( debugSurfaces || debugAxis || debugCluster || debugOrigin || dirtDebug || normalmap ) {
		/* debug fill the luxels */
		for ( y = 0; y < lm->sh; ++y )
		{
			for ( x = 0; x < lm->sw; ++x )
			{
				/* get cluster */
				const int cluster = lm->getSuperCluster( x, y );

				/* only fill mapped luxels */
				if ( cluster < CLUSTER_NORMAL ) {
					continue;
				}

				/* get particulars */
				SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );

				/* color the luxel with raw lightmap num? */
				if ( debugSurfaces ) {
					luxel.value = debugColors[ rawLightmapNum % 12 ];
				}

				/* color the luxel with lightmap axis? */
				else if ( debugAxis ) {
					luxel.value = ( lm->axis + Vector3( 1 ) ) * 127.5f;
				}

				/* color the luxel with luxel cluster? */
				else if ( debugCluster ) {
					luxel.value = debugColors[ cluster % 12 ];
				}

				/* color the luxel with luxel origin? */
				else if ( debugOrigin ) {
					const Vector3 temp = ( lm->minmax.maxs - lm->minmax.mins ) * ( 1.0f / 255.0f );
					const Vector3 temp2 = lm->getSuperOrigin( x, y ) - lm->minmax.mins;
					luxel.value = lm->minmax.mins + ( temp * temp2 );
				}

				/* color the luxel with the normal */
				else if ( normalmap ) {
					luxel.value = ( lm->getSuperNormal( x, y ) + Vector3( 1 ) ) * 127.5f;
				}

				/* otherwise clear it */
				else{
					luxel.value.set( 0 );
				}

				/* add to counts */
				luxel.count = 1;
			}
		}
	}
	else
	{
		/* allocate temporary per-light luxel storage */
		rawLightmap_t tmplm = *lm;
		const size_t llSize = lm->sw * lm->sh * sizeof( *lm->superLuxels[0] );
		const size_t ldSize = lm->sw * lm->sh * sizeof( *lm->superDeluxels );
		if ( llSize <= sizeof( stackLightLuxels ) ) {
			tmplm.superLuxels[0] = stackLightLuxels;
		}
		else{
			tmplm.superLuxels[0] = safe_malloc( llSize );
		}
		if ( deluxemap ) {
			tmplm.superDeluxels = safe_malloc( ldSize );
		}
		else{
			tmplm.superDeluxels = nullptr;
		}

		/* clear luxels */
		//%	memset( lm->superLuxels[ 0 ], 0, llSize );

		/* set ambient color */
		for ( y = 0; y < lm->sh; ++y )
		{
			for ( x = 0; x < lm->sw; ++x )
			{
				/* get cluster */
				SuperLuxel& luxel = lm->getSuperLuxel( 0, x, y );

				/* blacken unmapped clusters */
				if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
					luxel.value.set( 0 );
				}
				/* set ambient */
				else
				{
					luxel.value = lm->ambientColor;
					if ( deluxemap ) {
						// use AT LEAST this amount of contribution from ambient for the deluxemap, fixes points that receive ZERO light
						const float brightness = std::max( 0.00390625f, RGBTOGRAY( lm->ambientColor ) * ( 1.0f / 255.0f ) );

						lm->getSuperDeluxel( x, y ) = lm->getSuperNormal( x, y ) * brightness;
					}
					luxel.count = 1;
				}
			}
		}

		/* clear styled lightmaps */
		size = lm->sw * lm->sh * sizeof( *lm->superLuxels[0] );
		for ( lightmapNum = 1; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			if ( lm->superLuxels[ lightmapNum ] != nullptr ) {
				memset( lm->superLuxels[ lightmapNum ], 0, size );
			}
		}

		/* debugging code */
		//%	if( trace.numLights <= 0 )
		//%		Sys_Printf( "Lightmap %9d: 0 lights, axis: %.2f, %.2f, %.2f\n", rawLightmapNum, lm->axis[ 0 ], lm->axis[ 1 ], lm->axis[ 2 ] );

		/* BehavEd stage 8b: -bhedgpu - the rays of the initial pass are recorded in blocks of lights,
		   traced on the GPU and taken by TraceLine during the normal pass (inputs compared) */
		BhedGpuSitzung bhedGpu;
		int bhedGpuBis = 0;

		/* walk light list */
		if ( bhedFertig != nullptr ) { /* BehavEd stage 8g: -bhedgpubake, the light loop ran on the GPU */
			BhedBakeEinsetzen( lm, *bhedFertig );
		}
		else if ( pool != nullptr ) { /* BehavEd stage 7: the same light loop, rows and lights in blocks on all threads */
			BhedLichterParallel( lm, trace, *pool );
		}
		else
		for ( int i = 0; i < trace.numLights; ++i )
		{
			/* setup trace */
			trace.light = trace.lights[ i ];

			/* style check */
			for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
			{
				if ( lm->styles[ lightmapNum ] == trace.light->style ||
				     lm->styles[ lightmapNum ] == LS_NONE ) {
					break;
				}
			}

			/* max of MAX_LIGHTMAPS (4) styles allowed to hit a surface/lightmap */
			if ( lightmapNum >= MAX_LIGHTMAPS ) {
				Sys_Warning( "Hit per-surface style limit (%d)\n", MAX_LIGHTMAPS );
				continue;
			}

			/* setup */
			memset( tmplm.superLuxels[0], 0, llSize );
			if ( deluxemap ) {
				memset( tmplm.superDeluxels, 0, ldSize );
			}
			totalLighted = 0;

			/* determine filter radius */
			filterRadius = std::max( { 0.f, lm->filterRadius, trace.light->filterRadius } );

			/* set luxel filter radius */
			luxelFilterRadius = lm->sampleSize != 0 ? superSample * filterRadius / lm->sampleSize : 0;
			if ( luxelFilterRadius == 0 && ( filterRadius > 0 || filter ) ) {
				luxelFilterRadius = 1;
			}

			/* allocate sampling flags storage */
			if ( lightSamples > 1 || lightRandomSamples ) {
				size = lm->sw * lm->sh * sizeof( *lm->superFlags );
				if ( lm->superFlags == nullptr ) {
					lm->superFlags = safe_malloc( size );
				}
				memset( lm->superFlags, 0, size );
			}

			/* BehavEd stage 8b: -bhedgpu */
			if ( g_bhedGpu ) {
				if ( i >= bhedGpuBis ) {
					if ( BhedGpuFrei() ) {
						bhedGpuBis = BhedGpuAufnehmen( lm, trace, i, bhedGpu );
					}
					else{ /* the GPU is busy: this light on the CPU */
						BhedGpuBelegt();
						bhedGpu.strahlen.clear();
						bhedGpu.geeignet.clear();
						bhedGpu.erg.clear();
						bhedGpu.pos = 0;
						bhedGpu.kaputt = true;
						bhedGpuBis = i + 1;
					}
				}
				bhedGpu.modus = 2;
				g_bhedGpuSitzung = &bhedGpu;
			}

			/* initial pass, one sample per luxel */
			for ( y = 0; y < lm->sh; ++y )
			{
				for ( x = 0; x < lm->sw; ++x )
				{
					/* get cluster */
					const int cluster = lm->getSuperCluster( x, y );
					if ( cluster < CLUSTER_NORMAL ) {
						continue;
					}

					/* get particulars */
					SuperLuxel& lightLuxel = tmplm.getSuperLuxel( 0, x, y );

#if 0
					////////// 27's temp hack for testing edge clipping ////
					if ( lm->getSuperOrigin( x, y ) == g_vector3_identity ) {
						lightLuxel.value[ 1 ] = 255;
						lightLuxel.count = 1;
						totalLighted++;
					}
					else
#endif
					{
						/* set contribution count */
						lightLuxel.count = 1;

						/* setup trace */
						trace.cluster = cluster;
						trace.origin = lm->getSuperOrigin( x, y );
						trace.normal = lm->getSuperNormal( x, y );

						/* get light for this sample */
						LightContributionToSample( &trace );
						lightLuxel.value = trace.color;

						/* add the contribution to the deluxemap */
						if ( deluxemap ) {
							tmplm.getSuperDeluxel( x, y ) = trace.directionContribution;
						}

						/* check for evilness */
						if ( trace.forceSubsampling > 1 && ( lightSamples > 1 || lightRandomSamples ) ) {
							totalLighted++;
							lm->getSuperFlag( x, y ) |= FLAG_FORCE_SUBSAMPLING; /* force */
						}
						/* add to count */
						else if ( trace.color != g_vector3_identity ) {
							totalLighted++;
						}
					}
				}
			}

			g_bhedGpuSitzung = nullptr; /* BehavEd stage 8b: only the initial pass */

			/* don't even bother with everything else if nothing was lit */
			if ( totalLighted == 0 ) {
				continue;
			}

			/* secondary pass, adaptive supersampling (fixme: use a contrast function to determine if subsampling is necessary) */
			/* 2003-09-27: changed it so filtering disamples supersampling, as it would waste time */
			if ( lightSamples > 1 || lightRandomSamples ) {
				/* walk luxels */
				for ( y = 0; y < ( lm->sh - 1 ); ++y )
				{
					for ( x = 0; x < ( lm->sw - 1 ); ++x )
					{
						/* setup */
						mapped = 0;
						lighted = 0;
						Vector3 total( 0 );

						/* test 2x2 stamp */
						for ( t = 0; t < 4; ++t )
						{
							/* set sample coords */
							sx = x + tests[ t ][ 0 ];
							sy = y + tests[ t ][ 1 ];

							/* get cluster */
							if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
								continue;
							}
							mapped++;

							/* get luxel */
							if ( lm->getSuperFlag( sx, sy ) & FLAG_FORCE_SUBSAMPLING ) {
								/* force a lighted/mapped discrepancy so we subsample */
								++lighted;
								++mapped;
								++mapped;
							}
							const SuperLuxel& lightLuxel = tmplm.getSuperLuxel( 0, sx, sy );
							total += lightLuxel.value;
							if ( ( lightLuxel.value[ 0 ] + lightLuxel.value[ 1 ] + lightLuxel.value[ 2 ] ) > 0 ) {
								lighted++;
							}
						}

						/* if total color is under a certain amount, then don't bother subsampling */
						if ( total[ 0 ] <= 4.0f && total[ 1 ] <= 4.0f && total[ 2 ] <= 4.0f ) {
							continue;
						}

						/* if all 4 pixels are either in shadow or light, then don't subsample */
						if ( lighted != 0 && lighted != mapped ) {
							for ( t = 0; t < 4; ++t )
							{
								/* set sample coords */
								sx = x + tests[ t ][ 0 ];
								sy = y + tests[ t ][ 1 ];

								/* get luxel */
								if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
									continue;
								}
								byte& flag = lm->getSuperFlag( sx, sy );
								if ( flag & FLAG_ALREADY_SUBSAMPLED ) { // already subsampled
									continue;
								}
								SuperLuxel& lightLuxel = tmplm.getSuperLuxel( 0, sx, sy );
								/* no deluxels without a deluxemap: superDeluxels is null then, and binding a reference to its
								   element is undefined behaviour (UBSan, S12.passage) even if it is never read */
								/* BehavEd stage 6: from Garux/netradiant-custom PR #316, commit ca2e281df (auto-pigeon) */
								Vector3* lightDeluxel = deluxemap ? &tmplm.getSuperDeluxel( sx, sy ) : nullptr;
								const Vector3& origin = lm->getSuperOrigin( sx, sy );

								/* only subsample shadowed luxels */
								//%	if( ( lightLuxel[ 0 ] + lightLuxel[ 1 ] + lightLuxel[ 2 ] ) <= 0 )
								//%		continue;

								/* subsample it */
								if ( lightRandomSamples ) {
									RandomSubsampleRawLuxel( lm, &trace, origin, sx, sy, 0.5f * lightSamplesSearchBoxSize, lightLuxel, lightDeluxel );
								}
								else{
									SubsampleRawLuxel_r( lm, &trace, origin, sx, sy, 0.25f * lightSamplesSearchBoxSize, lightLuxel, lightDeluxel );
								}

								flag |= FLAG_ALREADY_SUBSAMPLED;

								/* debug code to colorize subsampled areas to yellow */
								//%	lm->getSuperLuxel( lightmapNum, sx, sy ).value = { 255, 204, 0 };
							}
						}
					}
				}
			}

			/* tertiary pass, apply dirt map (ambient occlusion) */
			if ( 0 && dirty ) {
				/* walk luxels */
				for ( y = 0; y < lm->sh; ++y )
				{
					for ( x = 0; x < lm->sw; ++x )
					{
						/* get cluster  */
						if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
							continue;
						}

						/* scale light value */
						tmplm.getSuperLuxel( 0, x, y ).value *= lm->getSuperDirt( x, y );
					}
				}
			}

			/* allocate sampling lightmap storage */
			if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
				/* allocate sampling lightmap storage */
				size = lm->sw * lm->sh * sizeof( *lm->superLuxels[0] );
				lm->superLuxels[ lightmapNum ] = safe_calloc( size );
			}

			/* set style */
			if ( lightmapNum > 0 ) {
				lm->styles[ lightmapNum ] = trace.light->style;
				//%	Sys_Printf( "Surface %6d has lightstyle %d\n", rawLightmapNum, trace.light->style );
			}

			/* copy to permanent luxels */
			for ( y = 0; y < lm->sh; ++y )
			{
				for ( x = 0; x < lm->sw; ++x )
				{
					/* get cluster and origin */
					if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
						continue;
					}

					/* filter? */
					if ( luxelFilterRadius ) {
						/* setup */
						averageColor.set( 0 );
						averageDir.set( 0 );
						samples = 0;

						/* cheaper distance-based filtering */
						for ( sy = ( y - luxelFilterRadius ); sy <= ( y + luxelFilterRadius ); ++sy )
						{
							if ( sy < 0 || sy >= lm->sh ) {
								continue;
							}

							for ( sx = ( x - luxelFilterRadius ); sx <= ( x + luxelFilterRadius ); ++sx )
							{
								if ( sx < 0 || sx >= lm->sw ) {
									continue;
								}

								/* get particulars */
								if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
									continue;
								}

								/* create weight */
								weight = ( abs( sx - x ) == luxelFilterRadius ? 0.5f : 1.0f );
								weight *= ( abs( sy - y ) == luxelFilterRadius ? 0.5f : 1.0f );

								/* scale luxel by filter weight */
								averageColor += tmplm.getSuperLuxel( 0, sx, sy ).value * weight;
								if ( deluxemap ) {
									averageDir += tmplm.getSuperDeluxel( sx, sy ) * weight;
								}
								samples += weight;
							}
						}

						/* any samples? */
						if ( samples <= 0 ) {
							continue;
						}

						/* scale into luxel */
						SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, x, y );
						luxel.count = 1;

						/* handle negative light */
						if ( trace.light->flags & LightFlags::Negative ) {
							luxel.value -= averageColor / samples;
						}
						/* handle normal light */
						else
						{
							luxel.value += averageColor / samples;
						}

						if ( deluxemap ) {
							/* scale into luxel */
							lm->getSuperDeluxel( x, y ) += averageDir / samples;
						}
					}

					/* single sample */
					else
					{
						/* get particulars */
						const SuperLuxel& lightLuxel = tmplm.getSuperLuxel( 0, x, y );
						SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, x, y );

						/* handle negative light */
						if ( trace.light->flags & LightFlags::Negative ) {
							vector3_negate( averageColor );
						}

						/* add color */
						luxel.count = 1;

						/* handle negative light */
						if ( trace.light->flags & LightFlags::Negative ) {
							luxel.value -= lightLuxel.value;
						}
						/* handle normal light */
						else{
							luxel.value += lightLuxel.value;
						}

						if ( deluxemap ) {
							lm->getSuperDeluxel( x, y ) += tmplm.getSuperDeluxel( x, y );
						}
					}
				}
			}
		}

		/* free temporary luxels */
		if ( tmplm.superLuxels[0] != stackLightLuxels ) {
			free( tmplm.superLuxels[0] );
		}

		if ( deluxemap ) {
			free( tmplm.superDeluxels );
		}
	}

	/* free light list */
	FreeTraceLights( &trace );

	/* floodlight pass */
	if ( floodlighty ) {
		FloodlightIlluminateLightmap( lm );
	}

	if ( debugnormals ) {
		for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			/* early out */
			if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
				continue;
			}

			for ( y = 0; y < lm->sh; ++y )
			{
				for ( x = 0; x < lm->sw; ++x )
				{
					/* get cluster */
					//%	if( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL )
					//%		continue;

					lm->getSuperLuxel( lightmapNum, x, y ).value = lm->getSuperNormal( x, y ) * 127 + Vector3( 127 );
				}
			}
		}
	}

	/*	-----------------------------------------------------------------
	    dirt pass
	    ----------------------------------------------------------------- */

	if ( dirty ) {
		/* walk lightmaps */
		for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			/* early out */
			if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
				continue;
			}

			/* apply dirt to each luxel */
			for ( y = 0; y < lm->sh; ++y )
			{
				for ( x = 0; x < lm->sw; ++x )
				{
					/* get cluster */
					//%	if( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) // TODO why not do this check? These pixels should be zero anyway
					//%		continue;

					/* get particulars */
					SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, x, y );
					const float dirt = lm->getSuperDirt( x, y );

					/* apply dirt */
					luxel.value *= dirt;

					/* debugging */
					if ( dirtDebug ) {
						luxel.value.set( dirt * 255.0f );
					}
				}
			}
		}
	}

	/* -----------------------------------------------------------------
	   filter pass
	   ----------------------------------------------------------------- */

	/* walk lightmaps */
	for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
	{
		/* early out */
		if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
			continue;
		}

		/* average occluded luxels from neighbors */
		for ( y = 0; y < lm->sh; ++y )
		{
			for ( x = 0; x < lm->sw; ++x )
			{
				/* get particulars */
				int& cluster = lm->getSuperCluster( x, y );
				SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, x, y );

				/* determine if filtering is necessary */
				filterColor = false;
				filterDir = false;
				if ( cluster < CLUSTER_NORMAL ||
				     ( lm->splotchFix && ( luxel.value[ 0 ] <= lm->ambientColor[ 0 ]
				                        || luxel.value[ 1 ] <= lm->ambientColor[ 1 ]
				                        || luxel.value[ 2 ] <= lm->ambientColor[ 2 ] ) ) ) {
					filterColor = true;
				}

				if ( deluxemap && lightmapNum == 0 && ( cluster < CLUSTER_NORMAL || filter ) ) {
					filterDir = true;
				}

				if ( !filterColor && !filterDir ) {
					continue;
				}

				/* choose seed amount */
				averageColor.set( 0 );
				averageDir.set( 0 );
				samples = 0;

				/* walk 3x3 matrix */
				for ( sy = ( y - 1 ); sy <= ( y + 1 ); ++sy )
				{
					if ( sy < 0 || sy >= lm->sh ) {
						continue;
					}

					for ( sx = ( x - 1 ); sx <= ( x + 1 ); ++sx )
					{
						if ( sx < 0 || sx >= lm->sw || ( sx == x && sy == y ) ) {
							continue;
						}

						/* get neighbor's particulars */
						const SuperLuxel& luxel2 = lm->getSuperLuxel( lightmapNum, sx, sy );

						/* ignore unmapped/unlit luxels */
						if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL || luxel2.count == 0 ||
						     ( lm->splotchFix && VectorCompare( luxel2.value, lm->ambientColor ) ) ) {
							continue;
						}

						/* add its distinctiveness to our own */
						averageColor += luxel2.value;
						samples += luxel2.count;
						if ( filterDir ) {
							averageDir += lm->getSuperDeluxel( sx, sy );
						}
					}
				}

				/* fall through */
				if ( samples <= 0 ) {
					continue;
				}

				/* dark lightmap seams */
				if ( dark ) {
					if ( lightmapNum == 0 ) {
						averageColor += lm->ambientColor * 2;
					}
					samples += 2.0f;
				}

				/* average it */
				if ( filterColor ) {
					luxel.value = averageColor * ( 1.f / samples );
					luxel.count = 1;
				}
				if ( filterDir ) {
					lm->getSuperDeluxel( x, y ) = averageDir * ( 1.f / samples );
				}

				/* set cluster to -3 */
				if ( cluster < CLUSTER_NORMAL ) {
					cluster = CLUSTER_FLOODED;
				}
			}
		}
	}


#if 0
	// audit pass
	for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
	{
		/* early out */
		if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
			continue;
		}
		for ( y = 0; y < lm->sh; ++y )
			for ( x = 0; x < lm->sw; ++x )
			{
				/* get cluster */
				cluster = SUPER_CLUSTER( x, y );
				luxel = SUPER_LUXEL( lightmapNum, x, y );
				deluxel = SUPER_DELUXEL( x, y );
				if ( !luxel || !deluxel || !cluster ) {
					Sys_FPrintf( SYS_WRN | SYS_VRBflag, "WARNING: I got NULL'd.\n" );
					continue;
				}
				else if ( *cluster < 0 ) {
					// unmapped pixel
					// should have neither deluxemap nor lightmap
					if ( deluxel[3] ) {
						Sys_FPrintf( SYS_WRN | SYS_VRBflag, "WARNING: I have written deluxe to an unmapped luxel. Sorry.\n" );
					}
				}
				else
				{
					// mapped pixel
					// should have both deluxemap and lightmap
					if ( deluxel[3] ) {
						Sys_FPrintf( SYS_WRN | SYS_VRBflag, "WARNING: I forgot to write deluxe to a mapped luxel. Sorry.\n" );
					}
				}
			}
	}
#endif
}



/*
   IlluminateVertexes()
   light the surface vertexes
 */

#define VERTEX_NUDGE    4.0f

void IlluminateVertexes( int num ){
	int i, x, y, z, x1, y1, z1, sx, sy;
	float samples, dirt;
	trace_t trace;
	float floodLightAmount;
	Vector3 floodColor;


	/* get surface, info, and raw lightmap */
	bspDrawSurface_t& ds = bspDrawSurfaces[ num ];
	const surfaceInfo_t& info = surfaceInfos[ num ];
	rawLightmap_t *lm = info.lm;

	/* -----------------------------------------------------------------
	   illuminate the vertexes
	   ----------------------------------------------------------------- */

	/* calculate vertex lighting for surfaces without lightmaps */
	if ( lm == nullptr || cpmaHack ) {
		/* setup trace */
		trace.testOcclusion = ( cpmaHack && lm != nullptr ) ? false : !noTrace;
		trace.forceSunlight = info.si->forceSunlight;
		trace.recvShadows = info.recvShadows;
		trace.numSurfaces = 1;
		trace.surfaces = &num;
		trace.inhibitRadius = DEFAULT_INHIBIT_RADIUS;

		/* twosided lighting */
		trace.twoSided = info.si->twoSided;

		/* make light list for this surface */
		CreateTraceLightsForSurface( num, &trace );

		/* BehavEd stage 8a: the same lights as in the cached run - vertex light and colours from the cache */
		if ( BhedFlaecheAusCache( num, trace ) ) {
			FreeTraceLights( &trace );
			return;
		}

		/* setup */
		bspDrawVert_t *verts = &yDrawVerts[ ds.firstVert ];
		int numAvg = 0;
		Array4<Vector3> colors;
		Array4<Vector3> avgColors = makeArray4( Vector3( 0 ) );

		/* walk the surface verts */
		for ( i = 0; i < ds.numVerts; ++i )
		{
			/* get vertex luxel */
			Vector3& radVertLuxel = getRadVertexLuxel( 0, ds.firstVert + i );

			/* color the luxel with raw lightmap num? */
			if ( debugSurfaces ) {
				radVertLuxel = debugColors[ num % 12 ];
			}

			/* color the luxel with luxel origin? */
			else if ( debugOrigin ) {
				const Vector3 temp = ( info.minmax.maxs - info.minmax.mins ) * ( 1.0f / 255.0f );
				const Vector3 temp2 = verts[ i ].xyz - info.minmax.mins;
				radVertLuxel = info.minmax.mins + ( temp * temp2 );
			}

			/* color the luxel with the normal */
			else if ( normalmap ) {
				radVertLuxel = ( verts[ i ].normal + Vector3( 1 ) ) * 127.5f;
			}

			else if ( info.si->noVertexLight ) {
				radVertLuxel.set( 127.5f );
			}

			else if ( noVertexLighting > 0 ) {
				radVertLuxel.set( 127.5f * noVertexLighting );
			}

			/* illuminate the vertex */
			else
			{
				/* clear vertex luxel */
				radVertLuxel.set( -1 );
				/* setup trace */
				trace.normal = verts[ i ].normal;
				/* try at initial origin */
				trace.cluster = ClusterForPointExtFilter( verts[ i ].xyz, VERTEX_EPSILON, info.numSurfaceClusters, &surfaceClusters[ info.firstSurfaceCluster ] );
				if ( trace.cluster >= CLUSTER_NORMAL ) {
					/* setup trace */
					trace.origin = verts[ i ].xyz;

					/* r7 dirt */
					if ( dirty && !bouncing ) {
						dirt = DirtForSample( &trace );
					}
					else{
						dirt = 1;
					}

					/* jal: floodlight */
					floodLightAmount = 0;
					floodColor.set( 0 );
					if ( floodlighty && !bouncing ) {
						floodLightAmount = floodlightIntensity * FloodLightForSample( &trace, floodlightDistance, floodlight_lowquality );
						floodColor = floodlightRGB * floodLightAmount;
					}

					/* trace */
					LightingAtSample( &trace, ds.vertexStyles, colors, info.ambientColor );

					/* store */
					for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
					{
						/* r7 dirt */
						colors[ lightmapNum ] *= dirt;

						/* jal: floodlight */
						colors[ lightmapNum ] += floodColor;

						/* store */
						getRadVertexLuxel( lightmapNum, ds.firstVert + i ) = colors[ lightmapNum ];
						colors[ lightmapNum ] += avgColors[ lightmapNum ];
					}
				}

				/* is this sample bright enough? */
				const auto vector3_component_greater = []( const Vector3& greater, const Vector3& lesser ){
					return greater[0] > lesser[0] || greater[1] > lesser[1] || greater[2] > lesser[2];
				};
				if ( !vector3_component_greater( getRadVertexLuxel( 0, ds.firstVert + i ), info.ambientColor ) ) {
					/* nudge the sample point around a bit */
					for ( x = 0; x < 5; ++x )
					{
						/* two's complement 0, 1, -1, 2, -2, etc */
						x1 = ( ( x >> 1 ) ^ ( x & 1 ? -1 : 0 ) ) + ( x & 1 );

						for ( y = 0; y < 5; ++y )
						{
							y1 = ( ( y >> 1 ) ^ ( y & 1 ? -1 : 0 ) ) + ( y & 1 );

							for ( z = 0; z < 5; ++z )
							{
								z1 = ( ( z >> 1 ) ^ ( z & 1 ? -1 : 0 ) ) + ( z & 1 );

								/* nudge origin */
								trace.origin = verts[ i ].xyz + Vector3( x1, y1, z1 ) * VERTEX_NUDGE;

								/* try at nudged origin */
								trace.cluster = ClusterForPointExtFilter( trace.origin, VERTEX_EPSILON, info.numSurfaceClusters, &surfaceClusters[ info.firstSurfaceCluster ] );
								if ( trace.cluster < CLUSTER_NORMAL ) {
									continue;
								}

								/* r7 dirt */
								if ( dirty && !bouncing ) {
									dirt = DirtForSample( &trace );
								}
								else{
									dirt = 1;
								}

								/* jal: floodlight */
								floodLightAmount = 0;
								floodColor.set( 0 );
								if ( floodlighty && !bouncing ) {
									floodLightAmount = floodlightIntensity * FloodLightForSample( &trace, floodlightDistance, floodlight_lowquality );
									floodColor = floodlightRGB * floodLightAmount;
								}

								/* trace */
								LightingAtSample( &trace, ds.vertexStyles, colors, info.ambientColor );

								/* store */
								for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
								{
									/* r7 dirt */
									colors[ lightmapNum ] *= dirt;

									/* jal: floodlight */
									colors[ lightmapNum ] += floodColor;

									/* store */
									getRadVertexLuxel( lightmapNum, ds.firstVert + i ) = colors[ lightmapNum ];
								}

								/* bright enough? */
								if ( vector3_component_greater( getRadVertexLuxel( 0, ds.firstVert + i ), info.ambientColor ) ) {
									x = y = z = 1000;
								}
							}
						}
					}
				}

				/* add to average? */
				if ( vector3_component_greater( getRadVertexLuxel( 0, ds.firstVert + i ), info.ambientColor ) ) {
					numAvg++;
					for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
					{
						avgColors[ lightmapNum ] += getRadVertexLuxel( lightmapNum, ds.firstVert + i );
					}
				}
			}

			/* another happy customer */
			numVertsIlluminated++;
		}

		/* set average color */
		if ( numAvg > 0 ) {
			for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
				avgColors[ lightmapNum ] *= ( 1.0f / numAvg );
		}
		else
		{
			avgColors[ 0 ] = info.ambientColor;
		}

		/* clean up and store vertex color */
		for ( i = 0; i < ds.numVerts; ++i )
		{
			/* store average in occluded vertexes */
			if ( getRadVertexLuxel( 0, ds.firstVert + i )[ 0 ] < 0 ) {
				for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
				{
					getRadVertexLuxel( lightmapNum, ds.firstVert + i ) = avgColors[ lightmapNum ];

					/* debug code */
					//%	getRadVertexLuxel( lightmapNum, ds.firstVert + i ) = { 255.0f, 0.0f, 0.0f };
				}
			}

			/* store it */
			for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
			{
				/* get luxels */
				Vector3& vertLuxel = getVertexLuxel( lightmapNum, ds.firstVert + i );
				const Vector3& radVertLuxel = getRadVertexLuxel( lightmapNum, ds.firstVert + i );

				/* store */
				if ( bouncing || bounce == 0 || !bounceOnly ) {
					vertLuxel += radVertLuxel;
				}
				if ( !info.si->noVertexLight ) {
					verts[ i ].color[ lightmapNum ].rgb() = ColorToBytes( vertLuxel, info.si->vertexScale );
				}
			}
		}

		/* free light list */
		FreeTraceLights( &trace );

		/* return to sender */
		return;
	}

	/* -----------------------------------------------------------------
	   reconstitute vertex lighting from the luxels
	   ----------------------------------------------------------------- */

	/* set styles from lightmap */
	ds.vertexStyles = lm->styles;

	/* get max search radius */
	const int maxRadius = std::max( lm->sw, lm->sh );

	/* walk the surface verts */
	bspDrawVert_t *verts = &yDrawVerts[ ds.firstVert ];
	for ( i = 0; i < ds.numVerts; ++i )
	{
		/* do each lightmap */
		for ( int lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
		{
			/* early out */
			if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
				continue;
			}

			/* get luxel coords */
			x = std::clamp( int( verts[ i ].lightmap[ 0 ][ 0 ] ), 0, lm->sw - 1 );
			y = std::clamp( int( verts[ i ].lightmap[ 0 ][ 1 ] ), 0, lm->sh - 1 );

			/* get vertex luxels */
			Vector3& vertLuxel = getVertexLuxel( lightmapNum, ds.firstVert + i );
			Vector3& radVertLuxel = getRadVertexLuxel( lightmapNum, ds.firstVert + i );

			/* color the luxel with the normal? */
			if ( normalmap ) {
				radVertLuxel = ( verts[ i ].normal + Vector3( 1 ) ) * 127.5f;
			}

			/* color the luxel with surface num? */
			else if ( debugSurfaces ) {
				radVertLuxel = debugColors[ num % 12 ];
			}

			else if ( info.si->noVertexLight ) {
				radVertLuxel.set( 127.5f );
			}

			else if ( noVertexLighting > 0 ) {
				radVertLuxel.set( 127.5f * noVertexLighting );
			}

			/* divine color from the superluxels */
			else
			{
				/* increasing radius */
				radVertLuxel.set( 0 );
				samples = 0;
				for ( int radius = 0; radius < maxRadius && samples <= 0; ++radius )
				{
					/* sample within radius */
					for ( sy = ( y - radius ); sy <= ( y + radius ); ++sy )
					{
						if ( sy < 0 || sy >= lm->sh ) {
							continue;
						}

						for ( sx = ( x - radius ); sx <= ( x + radius ); ++sx )
						{
							if ( sx < 0 || sx >= lm->sw ) {
								continue;
							}

							/* get luxel particulars */
							const SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, sx, sy );
							if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL ) {
								continue;
							}

							/* testing: must be brigher than ambient color */
							//%	if( luxel[ 0 ] <= info.ambientColor[ 0 ] || luxel[ 1 ] <= info.ambientColor[ 1 ] || luxel[ 2 ] <= info.ambientColor[ 2 ] )
							//%		continue;

							/* add its distinctiveness to our own */
							radVertLuxel += luxel.value;
							samples += luxel.count;
						}
					}
				}

				/* any color? */
				if ( samples > 0 ) {
					radVertLuxel *= ( 1.f / samples );
				}
				else{
					radVertLuxel = info.ambientColor;
				}
			}

			/* store into floating point storage */
			vertLuxel += radVertLuxel;
			numVertsIlluminated++;

			/* store into bytes (for vertex approximation) */
			if ( !info.si->noVertexLight ) {
				verts[ i ].color[ lightmapNum ].rgb() = ColorToBytes( vertLuxel );
			}
		}
	}
}



/* -------------------------------------------------------------------------------

   light optimization (-fast)

   creates a list of lights that will affect a surface and stores it in tw
   this is to optimize surface lighting by culling out as many of the
   lights in the world as possible from further calculation

   ------------------------------------------------------------------------------- */

/*
   SetupBrushes()
   determines opaque brushes in the world and find sky shaders for sunlight calculations
 */

void SetupBrushesFlags( int mask_any, int test_any, int mask_all, int test_all ){
	int compileFlags, allCompileFlags;


	/* note it */
	Sys_FPrintf( SYS_VRB, "--- SetupBrushes ---\n" );

	/* clear */
	opaqueBrushes = decltype( opaqueBrushes )( bspBrushes.size(), false );
	int numOpaqueBrushes = 0;

	/* walk the list of worldspawn brushes */
	for ( int i = 0; i < bspModels[ 0 ].numBSPBrushes; ++i )
	{
		/* get brush */
		const int b = bspModels[ 0 ].firstBSPBrush + i;
		const bspBrush_t& brush = bspBrushes[ b ];

		/* check all sides */
		compileFlags = 0;
		allCompileFlags = ~( 0 );
		for ( const bspBrushSide_t& side : Span( &bspBrushSides[ brush.firstSide ], brush.numSides ) )
		{
			/* get shader info */
			const shaderInfo_t *si = ShaderInfoForShaderNull( bspShaders[ side.shaderNum ].shader );
			if ( si == nullptr ) {
				continue;
			}

			/* or together compile flags */
			compileFlags |= si->compileFlags;
			allCompileFlags &= si->compileFlags;
		}

		/* determine if this brush is opaque to light */
		if ( ( compileFlags & mask_any ) == test_any && ( allCompileFlags & mask_all ) == test_all ) {
			opaqueBrushes[ b ] = true;
			numOpaqueBrushes++;
			maxOpaqueBrush = i;
		}
	}

	/* emit some statistics */
	Sys_FPrintf( SYS_VRB, "%9d opaque brushes\n", numOpaqueBrushes );
}
void SetupBrushes(){
	SetupBrushesFlags( C_TRANSLUCENT, 0, 0, 0 );
}



/*
   ChopBounds()
   chops a bounding box by the plane defined by origin and normal
   returns false if the bounds is entirely clipped away

   this is not exactly the fastest way to do this...
 */

inline bool ChopBounds( MinMax& minmax, const Vector3& origin, const Vector3& normal ){
	/* FIXME: rewrite this so it doesn't use bloody brushes */
	return true;
}



/*
   SetupEnvelopes()
   calculates each light's effective envelope,
   taking into account brightness, type, and pvs.
 */

#define LIGHT_EPSILON   0.125f
#define LIGHT_NUDGE     2.0f

void SetupEnvelopes( bool forGrid, bool fastFlag ){
	float radius, intensity;


	/* early out for weird cases where there are no lights */
	if ( lights.empty() ) {
		return;
	}

	/* note it */
	Sys_FPrintf( SYS_VRB, "--- SetupEnvelopes%s ---\n", fastFlag ? " (fast)" : "" );

	/* count lights */
	int numCulledLights = 0;
	for( auto light = lights.begin(); light != lights.end(); )
	{
		/* handle negative lights */
		if ( light->photons < 0 || light->add < 0 ) {
			light->photons *= -1;
			light->add *= -1;
			light->flags |= LightFlags::Negative;
		}

		/* sunlight? */
		if ( light->type == ELightType::Sun ) {
			/* special cased */
			light->cluster = 0;
			light->envelope = MAX_WORLD_COORD * 8.0f;
			light->minmax.mins.set( MIN_WORLD_COORD * 8.0f );
			light->minmax.maxs.set( MAX_WORLD_COORD * 8.0f );
		}

		/* everything else */
		else
		{
			/* get pvs cluster for light */
			light->cluster = ClusterForPointExt( light->origin, LIGHT_EPSILON );

			/* invalid cluster? */
			if ( light->cluster <= CLUSTER_OPAQUE ) {
				/* nudge the sample point around a bit */
				for ( int x = 0; x < 4; ++x )
				{
					/* two's complement 0, 1, -1, 2, -2, etc */
					const int x1 = ( ( x >> 1 ) ^ ( x & 1 ? -1 : 0 ) ) + ( x & 1 );

					for ( int y = 0; y < 4; ++y )
					{
						const int y1 = ( ( y >> 1 ) ^ ( y & 1 ? -1 : 0 ) ) + ( y & 1 );

						for ( int z = 0; z < 4; ++z )
						{
							const int z1 = ( ( z >> 1 ) ^ ( z & 1 ? -1 : 0 ) ) + ( z & 1 );

							/* nudge origin */
							const Vector3 origin = light->origin + Vector3( x1, y1, z1 ) * LIGHT_NUDGE;

							/* try at nudged origin */
							light->cluster = ClusterForPointExt( origin, LIGHT_EPSILON );
							if ( light->cluster <= CLUSTER_OPAQUE ) {
								continue;
							}

							/* set origin */
							light->origin = origin;
						}
					}
				}
			}

			/* only calculate for lights in pvs and outside of opaque brushes */
			if ( light->cluster > CLUSTER_OPAQUE ) {
				/* set light fast flag */
				if ( fastFlag ) {
					light->flags |= LightFlags::FastTemp;
				}
				else{
					light->flags &= ~LightFlags::FastTemp;
				}
				if ( fastpoint && ( light->type != ELightType::Area ) ) {
					light->flags |= LightFlags::FastTemp;
				}
				if ( light->si && light->si->noFast ) {
					light->flags &= ~( LightFlags::FastActual );
				}

				/* clear light envelope */
				light->envelope = 0;

				/* handle area lights */
				if ( exactPointToPolygon && light->type == ELightType::Area && !light->w.empty() ) {
					light->envelope = MAX_WORLD_COORD * 8.0f;

					/* check for fast mode */
					if ( light->flags & LightFlags::FastActual ) {
						/* ugly hack to calculate extent for area lights, but only done once */
						const Vector3 dir = -light->normal;
						for ( radius = 100.0f; radius < MAX_WORLD_COORD * 8.0f; radius += 10.0f )
						{
							const Vector3 origin = light->origin + light->normal * radius;
							const float factor = std::abs( PointToPolygonFormFactor( origin, dir, light->w ) );
							if ( ( factor * light->add ) <= light->falloffTolerance ) {
								light->envelope = radius;
								break;
							}
						}
					}

					intensity = light->photons; /* hopefully not used */
				}
				else
				{
					radius = 0;
					intensity = light->photons;
				}

				/* other calcs */
				if ( light->envelope <= 0 ) {
					/* solve distance for non-distance lights */
					if ( !( light->flags & LightFlags::AttenDistance ) ) {
						light->envelope = MAX_WORLD_COORD * 8.0f;
					}

					else if ( light->flags & LightFlags::FastActual ) {
						/* solve distance for linear lights */
						if ( ( light->flags & LightFlags::AttenLinear ) ) {
							light->envelope = ( ( intensity * linearScale ) - light->falloffTolerance ) / light->fade;
						}

						/*
						   add = angle * light->photons * linearScale - (dist * light->fade);
						   T = (light->photons * linearScale) - (dist * light->fade);
						   T + (dist * light->fade) = (light->photons * linearScale);
						   dist * light->fade = (light->photons * linearScale) - T;
						   dist = ((light->photons * linearScale) - T) / light->fade;
						 */

						/* solve for inverse square falloff */
						else{
							light->envelope = sqrt( intensity / light->falloffTolerance ) + radius;
						}

						/*
						   add = light->photons / (dist * dist);
						   T = light->photons / (dist * dist);
						   T * (dist * dist) = light->photons;
						   dist = sqrt( light->photons / T );
						 */
					}
					else
					{
						/* solve distance for linear lights */
						if ( ( light->flags & LightFlags::AttenLinear ) ) {
							light->envelope = ( intensity * linearScale ) / light->fade;
						}

						/* can't cull these */
						else{
							light->envelope = MAX_WORLD_COORD * 8.0f;
						}
					}
				}

				/* chop radius against pvs */
				{
					/* clear bounds */
					MinMax minmax;

					/* check all leaves */
					for ( const bspLeaf_t& leaf : bspLeafs )
					{
						/* in pvs? */
						if ( leaf.cluster <= CLUSTER_OPAQUE ) {
							continue;
						}
						if ( !ClusterVisible( light->cluster, leaf.cluster ) ) { /* ydnar: thanks Arnout for exposing my stupid error (this never failed before) */
							continue;
						}

						/* add this leafs bbox to the bounds */
						minmax.extend( leaf.minmax );
					}

					/* test to see if bounds encompass light */
					if ( !minmax.test( light->origin ) ) {
						//% Sys_Warning( "Light PVS bounds (%.0f, %.0f, %.0f) -> (%.0f, %.0f, %.0f)\ndo not encompass light %d (%f, %f, %f)\n",
						//%     minmax.mins[ 0 ], minmax.mins[ 1 ], minmax.mins[ 2 ],
						//%     minmax.maxs[ 0 ], minmax.maxs[ 1 ], minmax.maxs[ 2 ],
						//%     numLights, light->origin[ 0 ], light->origin[ 1 ], light->origin[ 2 ] );
						minmax.extend( light->origin );
					}

					/* chop the bounds by a plane for area lights and spotlights */
					if ( light->type == ELightType::Area || light->type == ELightType::Spot ) {
						ChopBounds( minmax, light->origin, light->normal );
					}

					/* copy bounds */
					light->minmax = minmax;

					/* reflect bounds around light origin */
					//%	VectorMA( light->origin, -1.0f, origin, origin );
					minmax.extend( light->origin * 2 - minmax.maxs );
					//%	VectorMA( light->origin, -1.0f, mins, origin );
					minmax.extend( light->origin * 2 - minmax.mins );

					/* calculate spherical bounds */
					radius = vector3_length( minmax.maxs - light->origin );

					/* if this radius is smaller than the envelope, then set the envelope to it */
					//% if ( radius < light->envelope ) Sys_FPrintf( SYS_VRB, "PVS Cull (%d): culled\n", numLights );
					//%	else Sys_FPrintf( SYS_VRB, "PVS Cull (%d): failed (%8.0f > %8.0f)\n", numLights, radius, light->envelope );
					value_minimize( light->envelope, radius );
				}

				/* add grid/surface only check */
				if ( forGrid ) {
					if ( !( light->flags & LightFlags::Grid ) ) {
						light->envelope = 0;
					}
				}
				else
				{
					if ( !( light->flags & LightFlags::Surfaces ) ) {
						light->envelope = 0;
					}
				}
			}

			/* culled? */
			if ( light->cluster <= CLUSTER_OPAQUE || light->envelope <= 0 ) {
				/* debug code */
				//%	Sys_Printf( "Culling light: Cluster: %d Envelope: %f\n", light->cluster, light->envelope );

				/* delete the light */
				numCulledLights++;
				light = lights.erase( light );
				continue;
			}
		}

		/* square envelope */
		light->envelope2 = ( light->envelope * light->envelope );

		/* set next light */
		++light;
	}

	/* sort lights by style */
	lights.sort( []( const light_t& a, const light_t& b ){ return a.style < b.style; } );
	/* if any styled light is present, automatically set nocollapse */
	if ( !lights.empty() && lights.back().style != LS_NORMAL ) {
		noCollapse = true;
	}

	/* emit some statistics */
	Sys_Printf( "%9zu total lights\n", lights.size() );
	Sys_Printf( "%9d culled lights\n", numCulledLights );
}



/////////////////////////////////////////////////////////////

#define FLOODLIGHT_CONE_ANGLE           88  /* degrees */
#define FLOODLIGHT_NUM_ANGLE_STEPS      16
#define FLOODLIGHT_NUM_ELEVATION_STEPS  4
#define FLOODLIGHT_NUM_VECTORS          ( FLOODLIGHT_NUM_ANGLE_STEPS * FLOODLIGHT_NUM_ELEVATION_STEPS )

static Vector3 floodVectors[ FLOODLIGHT_NUM_VECTORS ];
static int numFloodVectors = 0;

void SetupFloodLight(){
	/* note it */
	Sys_FPrintf( SYS_VRB, "--- SetupFloodLight ---\n" );

	/* calculate angular steps */
	const float angleStep = degrees_to_radians( 360.0f / FLOODLIGHT_NUM_ANGLE_STEPS );
	const float elevationStep = degrees_to_radians( FLOODLIGHT_CONE_ANGLE / FLOODLIGHT_NUM_ELEVATION_STEPS );

	/* iterate angle */
	float angle = 0;
	for ( int i = 0; i < FLOODLIGHT_NUM_ANGLE_STEPS; ++i, angle += angleStep )
	{
		/* iterate elevation */
		float elevation = elevationStep * 0.5f;
		for ( int j = 0; j < FLOODLIGHT_NUM_ELEVATION_STEPS; ++j, elevation += elevationStep )
		{
			floodVectors[ numFloodVectors ][ 0 ] = sin( elevation ) * cos( angle );
			floodVectors[ numFloodVectors ][ 1 ] = sin( elevation ) * sin( angle );
			floodVectors[ numFloodVectors ][ 2 ] = cos( elevation );
			numFloodVectors++;
		}
	}

	/* emit some statistics */
	Sys_FPrintf( SYS_VRB, "%9d numFloodVectors\n", numFloodVectors );

	/* floodlight */
	if ( const char  *value; entities[ 0 ].read_keyvalue( value, "_floodlight" ) ) {
		double v1, v2, v3, v4, v5, v6;
		v1 = v2 = v3 = 0;
		v4 = floodlightDistance;
		v5 = floodlightIntensity;
		v6 = floodlightDirectionScale;

		sscanf( value, "%lf %lf %lf %lf %lf %lf", &v1, &v2, &v3, &v4, &v5, &v6 );

		floodlightRGB = Vector3( v1, v2, v3 );

		if ( floodlightRGB == g_vector3_identity ) {
			floodlightRGB = { 0.94, 0.94, 1.0 };
		}

		floodlightDistance = v4 < 1? 1024 : v4;
		floodlightIntensity = v5 < 1? 128 : v5;
		floodlightDirectionScale = v6 < 0? 1 : v6;

		floodlighty = true;
		Sys_Printf( "FloodLighting enabled via worldspawn _floodlight key.\n" );
	}
	else
	{
		floodlightRGB = { 0.94, 0.94, 1.0 };
	}
	ColorFromSRGB( floodlightRGB );
	ColorNormalize( floodlightRGB );
}

/*
   FloodLightForSample()
   calculates floodlight value for a given sample
   once again, kudos to the dirtmapping coder
 */

float FloodLightForSample( trace_t *trace, float floodLightDistance, bool floodLightLowQuality ){
	int i;
	float contribution;
	float gatherLight, outLight;
	Vector3 myUp, myRt;
	int vecs = 0;

	gatherLight = 0;
	/* dummy check */
	//if( !dirty )
	//	return 1.0f;
	if ( trace == nullptr || trace->cluster < CLUSTER_NORMAL ) {
		return 0;
	}


	/* setup */
	const float dd = floodLightDistance;
	const Vector3 normal( trace->normal );

	/* check if the normal is aligned to the world-up */
	if ( normal[ 0 ] == 0 && normal[ 1 ] == 0 && ( normal[ 2 ] == 1 || normal[ 2 ] == -1 ) ) {
		if ( normal[ 2 ] == 1 ) {
			myRt = g_vector3_axis_x;
			myUp = g_vector3_axis_y;
		}
		else if ( normal[ 2 ] == -1 ) {
			myRt = -g_vector3_axis_x;
			myUp = g_vector3_axis_y;
		}
	}
	else
	{
		myRt = VectorNormalized( vector3_cross( normal, g_vector3_axis_z ) );
		myUp = VectorNormalized( vector3_cross( myRt, normal ) );
	}

	/* vortex: optimise floodLightLowQuality a bit */
	if ( floodLightLowQuality ) {
		/* iterate through ordered vectors */
		for ( i = 0; i < numFloodVectors; ++i )
			if ( rand() % 10 != 0 ) {
				continue;
			}
	}
	else
	{
		/* iterate through ordered vectors */
		for ( i = 0; i < numFloodVectors; ++i )
		{
			vecs++;

			/* transform vector into tangent space */
			const Vector3 direction = myRt * floodVectors[ i ][ 0 ] + myUp * floodVectors[ i ][ 1 ] + normal * floodVectors[ i ][ 2 ];

			/* set endpoint */
			trace->end = trace->origin + direction * dd;

			// trace->origin += direction;

			SetupTrace( trace );
			trace->color.set( 1 );
			/* trace */
			TraceLine( trace );
			contribution = 1;

			if ( trace->compileFlags & C_SKY || trace->compileFlags & C_TRANSLUCENT ) {
				contribution = 1;
			}
			else if ( trace->opaque ) {
				const float d = vector3_length( trace->hit - trace->origin );

				// d = trace->distance;
				//if ( d > 256 ) gatherDirt += 1;
				contribution = std::min( 1.f, d / dd );

				//gatherDirt += 1.0f - ooDepth * VectorLength( displacement );
			}

			gatherLight += contribution;
		}
	}

	/* early out */
	if ( gatherLight <= 0 ) {
		return 0;
	}

	gatherLight /= std::max( 1, vecs );

	outLight = std::min( 1.f, gatherLight );

	/* return to sender */
	return outLight;
}

/*
   FloodLightRawLightmap
   lighttracer style ambient occlusion light hack.
   Kudos to the dirtmapping author for most of this source.
   VorteX: modified to floodlight up custom surfaces (q3map_floodLight)
   VorteX: fixed problems with deluxemapping
 */

// floodlight pass on a lightmap
static void FloodLightRawLightmapPass( rawLightmap_t *lm, Vector3& lmFloodLightRGB, float lmFloodLightIntensity, float lmFloodLightDistance, bool lmFloodLightLowQuality, float floodlightDirectionScale ){
	trace_t trace;
	// int sx, sy;
	// float samples, average, *floodlight2;

	memset( &trace, 0, sizeof( trace_t ) );

	/* setup trace */
	trace.testOcclusion = true;
	trace.forceSunlight = false;
	trace.twoSided = true;
	trace.recvShadows = lm->recvShadows;
	trace.numSurfaces = lm->numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
	trace.inhibitRadius = DEFAULT_INHIBIT_RADIUS;
	trace.testAll = false;
	trace.distance = 1024;

	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	//trace.twoSided = false;
	for ( int i : Span( trace.surfaces, trace.numSurfaces ) )
	{
		/* get surface */
		const surfaceInfo_t& info = surfaceInfos[ i ];

		/* check twosidedness */
		if ( info.si->twoSided ) {
			trace.twoSided = true;
			break;
		}
	}

	/* gather floodlight */
	for ( int y = 0; y < lm->sh; ++y )
	{
		for ( int x = 0; x < lm->sw; ++x )
		{
			/* get luxel */
			const int cluster = lm->getSuperCluster( x, y );
			SuperFloodLight& floodlight = lm->getSuperFloodLight( x, y );

			/* set default dirt */
			floodlight.value[0] = 0;

			/* only look at mapped luxels */
			if ( cluster < CLUSTER_NORMAL ) {
				continue;
			}

			/* copy to trace */
			trace.cluster = cluster;
			trace.origin = lm->getSuperOrigin( x, y );
			trace.normal = lm->getSuperNormal( x, y );

			/* get floodlight */
			const float floodLightAmount = FloodLightForSample( &trace, lmFloodLightDistance, lmFloodLightLowQuality ) * lmFloodLightIntensity;

			/* add floodlight */
			floodlight.value += lmFloodLightRGB * floodLightAmount;
			floodlight.scale += floodlightDirectionScale;
		}
	}

	/* testing no filtering */
	return;

#if 0

	/* filter "dirt" */
	for ( int y = 0; y < lm->sh; ++y )
	{
		for ( int x = 0; x < lm->sw; ++x )
		{
			/* get luxel */
			SuperFloodLight& floodlight = lm->getSuperFloodLight( x, y );

			/* filter dirt by adjacency to unmapped luxels */
			average = floodlight.value[0];
			samples = 1.0f;
			for ( int sy = ( y - 1 ); sy <= ( y + 1 ); ++sy )
			{
				if ( sy < 0 || sy >= lm->sh ) {
					continue;
				}

				for ( int sx = ( x - 1 ); sx <= ( x + 1 ); ++sx )
				{
					if ( sx < 0 || sx >= lm->sw || ( sx == x && sy == y ) ) {
						continue;
					}

					/* get neighboring luxel */
					const SuperFloodLight& floodlight2 = lm->getSuperFloodLight( sx, sy );
					if ( lm->getSuperCluster( sx, sy ) < CLUSTER_NORMAL || floodlight2.value[0] <= 0 ) {
						continue;
					}

					/* add it */
					average += floodlight2.value[0];
					samples += 1.0f;
				}

				/* bail */
				if ( samples <= 0 ) {
					break;
				}
			}

			/* bail */
			if ( samples <= 0 ) {
				continue;
			}

			/* scale dirt */
			floodlight.value[0] = average / samples;
		}
	}
#endif
}

static int numSurfacesFloodlighten;

static void FloodLightRawLightmap( int rawLightmapNum ){
	rawLightmap_t       *lm;

	/* bail if this number exceeds the number of raw lightmaps */
	if ( rawLightmapNum >= numRawLightmaps ) {
		return;
	}
	/* get lightmap */
	lm = &rawLightmaps[ rawLightmapNum ];

	/* global pass */
	if ( floodlighty && floodlightIntensity ) {
		FloodLightRawLightmapPass( lm, floodlightRGB, floodlightIntensity, floodlightDistance, floodlight_lowquality, floodlightDirectionScale );
	}

	/* custom pass */
	if ( lm->floodlightIntensity ) {
		FloodLightRawLightmapPass( lm, lm->floodlightRGB, lm->floodlightIntensity, lm->floodlightDistance, false, lm->floodlightDirectionScale );
		numSurfacesFloodlighten += 1;
	}
}

void FloodlightRawLightmaps(){
	Sys_Printf( "--- FloodlightRawLightmap ---\n" );
	numSurfacesFloodlighten = 0;
	RunThreadsOnIndividual( numRawLightmaps, true, FloodLightRawLightmap );
	Sys_Printf( "%9d custom lightmaps floodlighted\n", numSurfacesFloodlighten );
}

/*
   FloodLightIlluminate()
   illuminate floodlight into lightmap luxels
 */

static void FloodlightIlluminateLightmap( rawLightmap_t *lm ){
	int x, y, lightmapNum;

	/* walk lightmaps */
	for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; ++lightmapNum )
	{
		/* early out */
		if ( lm->superLuxels[ lightmapNum ] == nullptr ) {
			continue;
		}

		if( lm->styles[lightmapNum] != LS_NORMAL && lm->styles[lightmapNum] != LS_NONE ) // isStyleLight
			continue;

		/* apply floodlight to each luxel */
		for ( y = 0; y < lm->sh; ++y )
		{
			for ( x = 0; x < lm->sw; ++x )
			{
				/* get floodlight */
				const SuperFloodLight& floodlight = lm->getSuperFloodLight( x, y );
				if ( floodlight.value == g_vector3_identity ) {
					continue;
				}

				/* only process mapped luxels */
				if ( lm->getSuperCluster( x, y ) < CLUSTER_NORMAL ) {
					continue;
				}

				/* get particulars */
				SuperLuxel& luxel = lm->getSuperLuxel( lightmapNum, x, y );

				/* add to lightmap */
				luxel.value += floodlight.value;

				if ( luxel.count == 0 ) {
					luxel.count = 1;
				}

				/* add to deluxemap */
				if ( deluxemap && floodlight.scale > 0 ) {
					// use AT LEAST this amount of contribution from ambient for the deluxemap, fixes points that receive ZERO light
					const float brightness = std::max( 0.00390625f, RGBTOGRAY( floodlight.value ) * ( 1.0f / 255.0f ) * floodlight.scale );

					const Vector3 lightvector = lm->getSuperNormal( x, y ) * brightness;
					lm->getSuperDeluxel( x, y ) += lightvector;
				}
			}
		}
	}
}
