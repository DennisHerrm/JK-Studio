// bhed_vischeck.cpp - BehavEd vis3: -vis -bhedvischeck (see bhed_vischeck.h)
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).

#include "q3map2.h"
#include "vis.h"
#include "bhed_vischeck.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
/* a line stays this far inside the portals it passes, and its ends this far off the planes of the
   portals it passes - ten times ON_EPSILON, the tolerance the flows clip with */
constexpr double c_clear = 1.0;
/* and it crosses the plane of a portal at least this steeply (sine of the angle) */
constexpr double c_steep = 0.05;
/* lines tried between the two portals of a pair that only the reference has */
constexpr int c_pairLines = 256;
constexpr char c_dumpMagic[] = "BHVPV1";

struct Vec
{
	double x, y, z;
};
Vec operator+( Vec a, Vec b ){
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}
Vec operator-( Vec a, Vec b ){
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}
Vec operator*( Vec a, double f ){
	return { a.x * f, a.y * f, a.z * f };
}
double Dot( Vec a, Vec b ){
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec Cross( Vec a, Vec b ){
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
double Length( Vec a ){
	return std::sqrt( Dot( a, a ) );
}
Vec Point( const Vector3& v ){
	return { v[0], v[1], v[2] };
}
/* BehavEd vis4: the check judges by a shape of its own for every portal, in double precision - not by the
   plane the flows work with (vis.cpp PlaneFromWinding, wrong where the first three points are nearly on one
   line), so that a wrong plane of theirs cannot make the check see a sight line where there is none or miss
   one that is there.
   The plane is made from all points of the winding. Its normal points into the leaf the portal leads to: it
   is the one the winding turns counterclockwise about (LoadPortals: the forward portal has NRC's plane
   flipped, the backward portal the points reversed), so the inside of the winding is on the left of every
   edge. Corners nearer to the one before than the flows' epsilon are left out - a corner written twice,
   the edge between the two has a direction that is nothing but rounding: what is left is convex and lies
   within the winding, so a point inside it is inside the portal. With less than three corners left the
   portal is no way for a line (a sliver) and no start or end of one. */
struct Shape
{
	Vec normal{ 0, 0, 0 };
	double dist = 0;
	std::vector<Vec> corners;       /* empty: not usable */
};
std::vector<Shape> g_shapes;

Shape ShapeOfWinding( const fixedWinding_t *w ){
	Shape shape;
	if ( w->numpoints < 3 ) {
		return shape;
	}
	const Vec origin = Point( w->points[0] );
	for ( int i = 1; i + 1 < w->numpoints; ++i )
		shape.normal = shape.normal + Cross( Point( w->points[i] ) - origin, Point( w->points[i + 1] ) - origin );
	const double length = Length( shape.normal );
	if ( length == 0 ) {
		return shape;
	}
	shape.normal = shape.normal * ( 1 / length );
	for ( int i = 0; i < w->numpoints; ++i )
	{
		const Vec point = Point( w->points[i] );
		shape.dist += Dot( shape.normal, point );
		if ( shape.corners.empty() || Length( point - shape.corners.back() ) >= ON_EPSILON ) {
			shape.corners.push_back( point );
		}
	}
	shape.dist /= w->numpoints;
	while ( shape.corners.size() > 1 && Length( shape.corners.back() - shape.corners.front() ) < ON_EPSILON )
		shape.corners.pop_back();
	if ( shape.corners.size() < 3 ) {
		shape.corners.clear();
	}
	return shape;
}

Vec Normal( const vportal_t *p ){
	return g_shapes[p - portals].normal;
}
double Distance( const vportal_t *p, Vec point ){
	return Dot( Normal( p ), point ) - g_shapes[p - portals].dist;
}
bool Usable( const vportal_t *p ){
	return !p->removed && !g_shapes[p - portals].corners.empty();
}

/* xorshift64: the lines of a portal are the same in every run */
struct Random
{
	uint64_t state;
	uint64_t next(){
		state ^= state << 13;
		state ^= state >> 7;
		state ^= state << 17;
		return state;
	}
	double unit(){
		return double( next() >> 11 ) * ( 1.0 / 9007199254740992.0 );
	}
};

/* a random point of the winding: a triangle of the fan from point 0 by its area, then a point in it */
Vec RandomPoint( const fixedWinding_t *w, Random& rnd ){
	const Vec origin = Point( w->points[0] );
	const auto area = [&]( int i ){
		return Length( Cross( Point( w->points[i] ) - origin, Point( w->points[i + 1] ) - origin ) );
	};
	double total = 0;
	for ( int i = 1; i + 1 < w->numpoints; ++i )
		total += area( i );
	double pick = rnd.unit() * total;
	int i = 1;
	while ( i + 2 < w->numpoints && pick >= area( i ) )
		pick -= area( i++ );
	double u = rnd.unit(), v = rnd.unit();
	if ( u + v > 1 ) {
		u = 1 - u;
		v = 1 - v;
	}
	return origin + ( Point( w->points[i] ) - origin ) * u + ( Point( w->points[i + 1] ) - origin ) * v;
}

/* the point lies in the plane of the portal: inside its shape, at least c_clear from every edge? */
bool Inside( const vportal_t *p, Vec point ){
	const Shape& shape = g_shapes[p - portals];
	const size_t count = shape.corners.size();
	for ( size_t i = 0; i < count; ++i )
	{
		const Vec a = shape.corners[i], b = shape.corners[i + 1 == count ? 0 : i + 1];
		const Vec in = Cross( shape.normal, b - a );    /* to the left of the edge: into the winding */
		if ( Dot( in, point - a ) < c_clear * Length( in ) ) {
			return false;
		}
	}
	return count != 0;
}

/* the line from the point a of the portal p to the point b of the portal q: the portals it passes on its way
   through the leafs, q last. false: it meets a wall, or it is too close to an edge or a plane to be sure.
   The leafs are convex and a portal lies in the surface of its leaf, so the line stays in the leaf from
   where it comes in to the portal it hits inside the winding - no other way out is nearer. */
bool SightLine( const vportal_t *p, Vec a, const vportal_t *q, Vec b, std::vector<int>& passed ){
	passed.clear();
	const Vec dir = b - a;
	const double length = Length( dir );
	const double forward = Dot( dir, Normal( p ) );
	if ( length < c_clear || forward < c_steep * length || !Inside( p, a ) || !Inside( q, b ) ) {
		return false;
	}
	const leaf_t *leaf = &leafs[p->leaf];
	double at = 0;
	while ( true )
	{
		const vportal_t *out = nullptr;
		double outAt = 0, outSlope = 0;
		for ( const vportal_t *x : Span( leaf->portals, leaf->numportals ) )
		{
			if ( !Usable( x ) ) {
				continue;
			}
			const double slope = Dot( dir, Normal( x ) );
			if ( slope < c_steep * length ) {
				continue;
			}
			const double t = -Distance( x, a ) / slope;
			if ( t <= at || t > 1 + 1e-6 || ( out != nullptr && t >= outAt ) || !Inside( x, a + dir * t ) ) {
				continue;
			}
			out = x;
			outAt = t;
			outSlope = slope;
		}
		/* the portal in front of the plane of p and p behind the plane of the portal, by more than the flows' epsilon -
		   and the way through the leaf longer than that: in a leaf thinner than the epsilon the flows take two
		   portals for one plane (duel_mos_espa has one that is 0.1 units thick) */
		if ( out == nullptr || outAt * forward < c_clear || outAt * outSlope < c_clear || ( outAt - at ) * length < c_clear ) {
			return false;
		}
		passed.push_back( int( out - portals ) );
		if ( out == q ) {
			return true;
		}
		leaf = &leafs[out->leaf];
		at = outAt;
	}
}

struct Check
{
	int lines = 256;                        /* lines tried for each portal (BHED_VIS_CHECK_LINES) */
	int hide = 0;                           /* BHED_VIS_CHECK_HIDE=<n>: the check sees one visible pair in n as not visible - shows that it notices */
	std::vector<uint64_t> ref;              /* portalvis of the reference run */
	std::atomic<uint64_t> pairs{ 0 }, outside{ 0 };
	std::atomic<uint64_t> found{ 0 }, onLines{ 0 }, missing{ 0 }, missingCluster{ 0 };
	std::atomic<uint64_t> onlyHere{ 0 }, onlyRef{ 0 }, refFound{ 0 }, missingRef{ 0 };
	std::atomic<uint64_t> wrongPlanes{ 0 };  /* planes of the flows that miss their own winding by more than ON_EPSILON */
	std::mutex mutex;
	std::string example;
};
Check g_check;

bool Visible( int from, int to ){
	if ( g_check.hide > 0 && ( uint64_t( from ) * 0x9E3779B97F4A7C15ull + uint64_t( to ) * 0xC2B2AE3D27D4EB4Full ) % uint64_t( g_check.hide ) == 0 ) {
		return false;
	}
	return bit_is_enabled( portals[from].portalvis, to );
}

void Example( const char *what, int from, int to ){
	std::lock_guard<std::mutex> lock( g_check.mutex );
	if ( g_check.example.empty() ) {
		g_check.example = StringStream( what, ": portal ", to, " from portal ", from ).c_str();
	}
}

/* the leaf a portal belongs to: the one its other side leads into */
int LeafOf( int num ){
	return portals[num ^ 1].leaf;
}

/* does the leaf vis of `from` get the leaf `to`? As ClusterMerge makes it: the leaf itself, the leafs behind
   its portals and behind every portal one of them sees */
bool LeafSees( int from, int to ){
	if ( from == to ) {
		return true;
	}
	for ( const vportal_t *q : Span( leafs[from].portals, leafs[from].numportals ) )
	{
		if ( q->removed ) {
			continue;
		}
		if ( q->leaf == to ) {
			return true;
		}
		const uint64_t *vis = reinterpret_cast<const uint64_t*>( q->portalvis );
		for ( int w = 0; w < portalwords; ++w )
			for ( uint64_t bits = vis[w]; bits != 0; bits &= bits - 1 )
				if ( portals[w * 64 + std::countr_zero( bits )].leaf == to ) {
					return true;
				}
	}
	return false;
}

void CheckPortal( int num ){
	const vportal_t *p = portals + num;
	if ( !Usable( p ) ) {
		return;
	}
	/* a plane of the flows that is none (the normal 0 0 0: the first three points on one line) or misses
	   its own winding by more than their epsilon */
	if ( vector3_length_squared( p->plane.normal() ) == 0 ) {
		++g_check.wrongPlanes;
	}
	else{
		for ( const Vector3& point : Span( p->winding->points, p->winding->numpoints ) )
			if ( std::fabs( plane3_distance_to_point( p->plane, point ) ) > ON_EPSILON ) {
				++g_check.wrongPlanes;
				break;
			}
	}
	const uint64_t *vis = reinterpret_cast<const uint64_t*>( p->portalvis );
	const uint64_t *flood = reinterpret_cast<const uint64_t*>( p->portalflood );
	uint64_t pairs = 0, outside = 0;
	std::vector<int> might;
	for ( int w = 0; w < portalwords; ++w )
	{
		pairs += std::popcount( vis[w] );
		outside += std::popcount( vis[w] & ~flood[w] );
		for ( uint64_t bits = flood[w]; bits != 0; bits &= bits - 1 )
			might.push_back( w * 64 + std::countr_zero( bits ) );
	}
	g_check.pairs += pairs;
	g_check.outside += outside;
	/* BehavEd vis4: every fourth line goes to a portal nearby whatever the flood says - one of the leaf behind
	   the portal or of the leafs behind those. A portal the flood has lost without reason (its plane wrong or
	   none) is in no portalflood and would never be the end of a line: in a corridor of three portals with
	   such a one in the middle the check found nothing to complain about. */
	std::vector<int> near;
	for ( const vportal_t *x : Span( leafs[p->leaf].portals, leafs[p->leaf].numportals ) )
	{
		near.push_back( int( x - portals ) );
		for ( const vportal_t *y : Span( leafs[x->leaf].portals, leafs[x->leaf].numportals ) )
			near.push_back( int( y - portals ) );
	}

	Random rnd{ ( uint64_t( num ) + 1 ) * 0x9E3779B97F4A7C15ull | 1 };
	std::vector<int> passed;
	uint64_t found = 0, onLines = 0, missing = 0, missingCluster = 0, missingRef = 0;
	for ( int k = 0; k < g_check.lines; ++k )
	{
		const std::vector<int>& ends = ( k & 3 ) == 3 || might.empty() ? near : might;
		if ( ends.empty() ) {
			break;
		}
		const vportal_t *q = portals + ends[rnd.next() % ends.size()];
		if ( !Usable( q ) ) {
			continue;
		}
		const Vec a = RandomPoint( p->winding, rnd ), b = RandomPoint( q->winding, rnd );
		if ( !SightLine( p, a, q, b, passed ) ) {
			continue;
		}
		++found;
		onLines += passed.size();
		for ( const int x : passed )
			if ( !Visible( num, x ) ) {
				++missing;
				/* what the game gets is the leaf vis: another portal of the leaf may see it, or the leaf behind it */
				if ( !LeafSees( LeafOf( num ), portals[x].leaf ) ) {
					++missingCluster;
					Example( "on a sight line into a cluster that is not visible", num, x );
				}
				if ( !g_check.ref.empty() && ( g_check.ref[size_t( num ) * portalwords + ( x >> 6 )] >> ( x & 63 ) & 1 ) != 0 ) {
					++missingRef;
					Example( "on a sight line, visible in the reference, but not visible here", num, x );
				}
			}
	}
	g_check.found += found;
	g_check.onLines += onLines;
	g_check.missing += missing;
	g_check.missingCluster += missingCluster;
	g_check.missingRef += missingRef;

	if ( !g_check.ref.empty() ) {
		const uint64_t *ref = g_check.ref.data() + size_t( num ) * portalwords;
		uint64_t onlyHere = 0, onlyRef = 0, refFound = 0;
		for ( int w = 0; w < portalwords; ++w )
		{
			onlyHere += std::popcount( vis[w] & ~ref[w] );
			onlyRef += std::popcount( ref[w] & ~vis[w] );
			for ( uint64_t bits = ref[w] & ~vis[w]; bits != 0; bits &= bits - 1 )
			{
				const int x = w * 64 + std::countr_zero( bits );
				const vportal_t *q = portals + x;
				if ( !Usable( q ) ) {
					continue;
				}
				for ( int k = 0; k < c_pairLines; ++k )
				{
					const Vec a = RandomPoint( p->winding, rnd ), b = RandomPoint( q->winding, rnd );
					if ( SightLine( p, a, q, b, passed ) ) {
						++refFound;
						Example( "visible in the reference, on a sight line, but not visible here", num, x );
						break;
					}
				}
			}
		}
		g_check.onlyHere += onlyHere;
		g_check.onlyRef += onlyRef;
		g_check.refFound += refFound;
	}
}

std::string ReadFile( const char *name ){
	std::string data;
	if ( FILE *f = fopen( name, "rb" ) ) {
		char buf[65536];
		size_t n;
		while ( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
			data.append( buf, n );
		fclose( f );
	}
	return data;
}
}

void BhedVisCheck(){
	if ( mergevis || mergevisportals || farPlaneDist != 0 ) {
		/* merged leafs are not convex, and a far plane hides on purpose */
		Sys_Printf( "BehavEd: vis check left out (merged leafs or a far plane)\n" );
		return;
	}
	if ( const char *lines = getenv( "BHED_VIS_CHECK_LINES" ) ) {
		g_check.lines = std::max( atoi( lines ), 0 );
	}
	if ( const char *hide = getenv( "BHED_VIS_CHECK_HIDE" ) ) {
		g_check.hide = std::max( atoi( hide ), 0 );
	}
	const uint32_t count = uint32_t( numportals * 2 ), words = uint32_t( portalwords );
	const char *refName = getenv( "BHED_VIS_CHECK_REF" );
	if ( refName != nullptr ) {
		const std::string data = ReadFile( refName );
		uint32_t head[2] = { 0, 0 };
		if ( data.size() >= 14 ) {
			memcpy( head, data.data() + 6, 8 );
		}
		if ( data.size() != 14 + size_t( count ) * words * 8 || data.compare( 0, 6, c_dumpMagic ) != 0 || head[0] != count || head[1] != words ) {
			Error( "BehavEd: vis check: %s is not the visibility of these portals", refName );
		}
		g_check.ref.resize( size_t( count ) * words );
		memcpy( g_check.ref.data(), data.data() + 14, g_check.ref.size() * 8 );
	}

	g_shapes.resize( count );
	for ( uint32_t i = 0; i < count; ++i )
		g_shapes[i] = ShapeOfWinding( portals[i].winding );

	Sys_Printf( "\n--- BehavEd: vis check (%i lines a portal) ---\n", g_check.lines );
	RunThreadsOnIndividual( numportals * 2, true, CheckPortal );
	if ( g_check.wrongPlanes != 0 ) {
		Sys_Printf( "BehavEd: vis check: %llu portal planes of the flows miss their own winding by more than %.1f or are none\n",
		            (unsigned long long)g_check.wrongPlanes.load(), ON_EPSILON );
	}
	Sys_Printf( "BehavEd: vis check: %llu visible portal pairs, %llu of them outside portalflood\n",
	            (unsigned long long)g_check.pairs.load(), (unsigned long long)g_check.outside.load() );
	Sys_Printf( "BehavEd: vis check: %llu sight lines with %llu portals on them, %llu of these portals not visible, %llu of them into a cluster that is not visible\n",
	            (unsigned long long)g_check.found.load(), (unsigned long long)g_check.onLines.load(), (unsigned long long)g_check.missing.load(),
	            (unsigned long long)g_check.missingCluster.load() );
	if ( refName != nullptr ) {
		Sys_Printf( "BehavEd: vis check against %s: %llu pairs only here, %llu pairs only there, a sight line found for %llu of those, %llu portals on sight lines visible there but not here\n",
		            refName, (unsigned long long)g_check.onlyHere.load(), (unsigned long long)g_check.onlyRef.load(), (unsigned long long)g_check.refFound.load(),
		            (unsigned long long)g_check.missingRef.load() );
	}
	if ( !g_check.example.empty() ) {
		Sys_Printf( "BehavEd: vis check: %s\n", g_check.example.c_str() );
	}

	if ( const char *dumpName = getenv( "BHED_VIS_CHECK_DUMP" ) ) {
		FILE *f = fopen( dumpName, "wb" );
		if ( f == nullptr ) {
			Error( "BehavEd: vis check: cannot write %s", dumpName );
		}
		const std::vector<uint64_t> none( words, 0 );
		bool ok = fwrite( c_dumpMagic, 1, 6, f ) == 6 && fwrite( &count, 4, 1, f ) == 1 && fwrite( &words, 4, 1, f ) == 1;
		for ( uint32_t i = 0; ok && i < count; ++i )
			ok = fwrite( portals[i].removed ? reinterpret_cast<const byte*>( none.data() ) : portals[i].portalvis, 8, words, f ) == words;
		if ( fclose( f ) != 0 || !ok ) {
			Error( "BehavEd: vis check: cannot write %s", dumpName );
		}
	}
}
