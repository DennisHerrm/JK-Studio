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
#include "vis.h"
#include "timer.h"

#include <immintrin.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <condition_variable>
#include <mutex>
#include <vector>




/*

   each portal will have a list of all possible to see from first portal

   if ( !thread->portalmightsee[portalnum] )

   portal mightsee

   for p2 = all other portals in leaf
    get sperating planes
    for all portals that might be seen by p2
        mark as unseen if not present in separating plane
    flood fill a new mightsee
    save as passagemightsee


   void CalcMightSee( leaf_t *leaf,
 */

int CountBits( const byte *bits, int numbits ){
	int c = 0;
	for ( int i = 0; i < numbits; ++i )
		if ( bit_is_enabled( bits, i ) ) {
			c++;
		}

	return c;
}


/* BehavEd vis1: the might-see vector of each recursion depth lives in a buffer per thread and depth,
   zeroed once and kept zero outside the words a frame lists (pstack_t::mightwords). A flow step then
   only touches the nonzero words: on duel_mos_espa a frame has 11 of 212 words nonzero on average,
   NRC copied and combined all of them for every portal of every leaf. The bits stay the same. */
namespace
{
struct BhedVisFrames
{
	std::vector<uint64_t*> bits;
	std::vector<uint16_t*> words;
	~BhedVisFrames(){
		for ( uint64_t *b : bits )
			free( b );
		for ( uint16_t *w : words )
			free( w );
	}
};
thread_local BhedVisFrames t_visFrames;
}

static void BhedFrameBuffers( pstack_t& stack ){
	BhedVisFrames& f = t_visFrames;
	while ( int( f.bits.size() ) <= stack.depth ) {
		f.bits.push_back( safe_calloc( portalwords * sizeof( uint64_t ) ) );
		f.words.push_back( safe_malloc( portalwords * sizeof( uint16_t ) ) );
	}
	stack.mightsee = f.bits[stack.depth];
	stack.mightwords = f.words[stack.depth];
	stack.nummightwords = 0;
}

static void BhedClearMight( pstack_t& stack ){
	for ( int k = 0; k < stack.nummightwords; ++k )
		stack.mightsee[stack.mightwords[k]] = 0;
	stack.nummightwords = 0;
}

/* the head of a flow: the portal's flood vector itself (read only), with the list of its nonzero words */
static void BhedHeadFrame( pstack_t& head, const vportal_t *p ){
	head.depth = 0;
	BhedFrameBuffers( head );
	head.mightsee = reinterpret_cast<uint64_t*>( p->portalflood );
	for ( int w = 0; w < portalwords; ++w )
		if ( head.mightsee[w] != 0 )
			head.mightwords[head.nummightwords++] = uint16_t( w );
}


/* BehavEd vis1: the same visibility with any number of threads.
   A flow reads the vectors of the portals it passes: portalvis of a finished portal, portalflood
   of one that is not finished yet. With one thread the portals run in the sorted order, so a
   portal sees exactly the portals before it as finished. With several threads NRC took whatever
   happened to be finished at that moment - the result changed from run to run.
   With g_bhedRankOrder a flow reads portalvis exactly for the portals before it in the sorted
   order - waiting for one that is still running - and portalflood for all after it, finished
   or not: the result of one thread. While a thread waits it gives its turn to another
   (BhedTurnTake/BhedTurnGive: at most numthreads compute at a time, more threads are started).
   Slower than NRC's way where flows wait for long ones (duel_mos_espa), hence a switch. */
namespace
{
std::vector<int> g_bhedRank;            /* position of each portal in sorted_portals */
bool g_bhedRankOrder = false;

/* waiting for a portal: its stripe of 64 (only the threads waiting on these portals wake up) */
struct alignas( 64 ) BhedDoneStripe
{
	std::mutex mutex;
	std::condition_variable cv;
	std::atomic<int> waiters{ 0 };
};
BhedDoneStripe g_bhedDone[64];

/* the turns: at most numthreads flows compute at a time; a free turn goes to the waiting flow that
   comes first in the sorted order (the later ones may wait for it) */
struct BhedTurnWaiter
{
	int rank;
	bool granted;
	std::condition_variable cv;
};
std::mutex g_bhedTurnMutex;
std::vector<BhedTurnWaiter*> g_bhedTurnWaiters;     /* heap, smallest rank on top */
int g_bhedTurnsFree = 0;
thread_local int t_bhedRank = 0;                     /* the flow this thread computes */

bool BhedTurnLater( const BhedTurnWaiter *a, const BhedTurnWaiter *b ){
	return a->rank > b->rank;
}
}

static void BhedTurnTake(){
	std::unique_lock<std::mutex> lock( g_bhedTurnMutex );
	if ( g_bhedTurnsFree > 0 ) {
		--g_bhedTurnsFree;
		return;
	}
	BhedTurnWaiter me{ t_bhedRank, false, {} };
	g_bhedTurnWaiters.push_back( &me );
	std::push_heap( g_bhedTurnWaiters.begin(), g_bhedTurnWaiters.end(), BhedTurnLater );
	me.cv.wait( lock, [&me]{ return me.granted; } );
}

static void BhedTurnGive(){
	std::lock_guard<std::mutex> lock( g_bhedTurnMutex );
	if ( g_bhedTurnWaiters.empty() ) {
		++g_bhedTurnsFree;
		return;
	}
	std::pop_heap( g_bhedTurnWaiters.begin(), g_bhedTurnWaiters.end(), BhedTurnLater );
	BhedTurnWaiter *next = g_bhedTurnWaiters.back();
	g_bhedTurnWaiters.pop_back();
	next->granted = true;
	next->cv.notify_one();
}

static EVStatus BhedStatus( const vportal_t *p ){
	return std::atomic_ref<const EVStatus>( p->status ).load( std::memory_order_acquire );
}

static void BhedSetStatus( vportal_t *p, EVStatus status ){
	std::atomic_ref<EVStatus>( p->status ).store( status, std::memory_order_seq_cst );
	BhedDoneStripe& stripe = g_bhedDone[( p - portals ) & 63];
	if ( status == EVStatus::Done && stripe.waiters.load( std::memory_order_seq_cst ) != 0 ) {
		{
			std::lock_guard<std::mutex> lock( stripe.mutex );
		}
		stripe.cv.notify_all();
	}
}

static void BhedWaitDone( const vportal_t *p ){
	for ( int i = 0; i < 1000; ++i )
	{
		if ( BhedStatus( p ) == EVStatus::Done ) {
			return;
		}
		_mm_pause();
	}
	BhedTurnGive();
	{
		BhedDoneStripe& stripe = g_bhedDone[( p - portals ) & 63];
		std::unique_lock<std::mutex> lock( stripe.mutex );
		stripe.waiters.fetch_add( 1, std::memory_order_seq_cst );
		stripe.cv.wait( lock, [p]{ return std::atomic_ref<const EVStatus>( p->status ).load( std::memory_order_seq_cst ) == EVStatus::Done; } );
		stripe.waiters.fetch_sub( 1, std::memory_order_seq_cst );
	}
	BhedTurnTake();
}

/* the vector a flow from thread->base reads for p (see above); nullptr: p comes before the flow and
   is still running - BhedResolve decides */
static const uint64_t *BhedVector( const threaddata_t *thread, const vportal_t *p ){
	if ( g_bhedRankOrder ) {
		if ( g_bhedRank[p - portals] > g_bhedRank[thread->base - portals] ) {
			return reinterpret_cast<const uint64_t*>( p->portalflood );
		}
		return BhedStatus( p ) == EVStatus::Done ? reinterpret_cast<const uint64_t*>( p->portalvis ) : nullptr;
	}
	return reinterpret_cast<const uint64_t*>( BhedStatus( p ) == EVStatus::Done ? p->portalvis : p->portalflood );
}

/* p is still running: when every bit the flow would take from portalflood (query & portalflood) is
   already in the part of portalvis that p has found so far, portalvis gives the same - no waiting.
   Otherwise wait for p. The query words come as (index, bits) pairs from `next`. */
template<typename Next>
static const uint64_t *BhedResolve( const vportal_t *p, Next next ){
	const uint64_t *flood = reinterpret_cast<const uint64_t*>( p->portalflood );
	uint64_t *found = reinterpret_cast<uint64_t*>( p->portalvis );
	int w;
	uint64_t query;
	while ( next( w, query ) )
	{
		if ( ( query & flood[w] & ~std::atomic_ref<uint64_t>( found[w] ).load( std::memory_order_relaxed ) ) != 0 ) {
			BhedWaitDone( p );
			return found;
		}
	}
	return flood;
}

/* marks a portal visible for the flow of base - other flows may read base->portalvis meanwhile */
static void BhedMark( vportal_t *base, int pnum ){
	uint64_t& word = reinterpret_cast<uint64_t*>( base->portalvis )[pnum >> 6];
	const uint64_t bit = uint64_t( 1 ) << ( pnum & 63 );
	if ( ( std::atomic_ref<uint64_t>( word ).load( std::memory_order_relaxed ) & bit ) == 0 ) {
		std::atomic_ref<uint64_t>( word ).fetch_or( bit, std::memory_order_relaxed );
	}
}

void BhedSortedRanks( bool rankOrder ){
	g_bhedRank.assign( numportals * 2, 0 );
	for ( int i = 0; i < numportals * 2; ++i )
		g_bhedRank[sorted_portals[i] - portals] = i;
	g_bhedRankOrder = rankOrder;
}

/* BehavEd vis1: the pacifier of the flow counts work, not portals. The portals come sorted by
   nummightsee and the last ones cost by far the most (duel_mos_espa: the last tenth of the portals
   took 29 % of the time, 47 % of the time had gone when the pacifier showed 90 %). The cost of a
   flow grows about with nummightsee squared (measured on md_ch_ben, mb2_manaan, duel_mos_espa: the
   pacifier then stays within 5-9 % of the real share of the work, by portals 42-50 %). The text is
   NRC's (0...1...2 ... 9... (seconds)), each sign stands for a 40th of the work of the finished
   flows - the remaining time from it (Build progress) is no longer a lower bound. */
namespace
{
std::mutex g_bhedPacifierMutex;
uint64_t g_bhedWorkTotal = 0;
std::atomic<uint64_t> g_bhedWorkDone{ 0 };
int g_bhedPacifierShown = -1;
}

static uint64_t BhedFlowWork( const vportal_t *p ){
	return p->removed ? 0 : uint64_t( p->nummightsee ) * uint64_t( p->nummightsee ) + 1;
}

static void BhedPacifier( uint64_t done ){
	const int f = int( std::min<uint64_t>( 39, 40 * done / std::max<uint64_t>( g_bhedWorkTotal, 1 ) ) );
	std::lock_guard<std::mutex> lock( g_bhedPacifierMutex );
	while ( f > g_bhedPacifierShown )
	{
		++g_bhedPacifierShown;
		if ( g_bhedPacifierShown % 4 == 0 ) {
			Sys_Printf( "%i", g_bhedPacifierShown / 4 );
		}
		else{
			Sys_Printf( "." );
		}
		fflush( stdout );
	}
}

void BhedRunFlow( void ( *flow )( int ) ){
	static void ( *s_flow )( int );
	s_flow = flow;
	g_bhedWorkTotal = 0;
	for ( const vportal_t *p : Span( sorted_portals, numportals * 2 ) )
		g_bhedWorkTotal += BhedFlowWork( p );
	g_bhedWorkDone = 0;
	g_bhedPacifierShown = -1;
	BhedPacifier( 0 );
	Timer timer;
	if ( !g_bhedRankOrder || numthreads <= 1 ) {
		RunThreadsOnIndividual( numportals * 2, false, []( int portalnum ){
			s_flow( portalnum );
			BhedPacifier( g_bhedWorkDone += BhedFlowWork( sorted_portals[portalnum] ) );
		} );
	}
	else{
		g_bhedTurnsFree = numthreads;
		BhedRunThreadsOnIndividual( numportals * 2, false, []( int portalnum ){
			t_bhedRank = portalnum;
			BhedTurnTake();
			s_flow( portalnum );
			BhedTurnGive();
			BhedPacifier( g_bhedWorkDone += BhedFlowWork( sorted_portals[portalnum] ) );
		}, std::min( numthreads * 4, 1024 ) ); /* mb2_manaan, 32 threads: 2 / 4 / 8 / 16 times as many threads 21 / 19 / 19 / 30 s */
	}
	Sys_Printf( " (%i)\n", int( timer.elapsed_sec() ) );
}


static void CheckStack( leaf_t *leaf, threaddata_t *thread ){
	for ( pstack_t *p = thread->pstack_head.next; p; p = p->next )
	{
//		Sys_Printf( "=" );
		if ( p->leaf == leaf ) {
			Error( "CheckStack: leaf recursion" );
		}
		for ( pstack_t *p2 = thread->pstack_head.next; p2 != p; p2 = p2->next )
			if ( p2->leaf == p->leaf ) {
				Error( "CheckStack: late leaf recursion" );
			}
	}
//	Sys_Printf( "\n" );
}


static fixedWinding_t *AllocStackWinding( pstack_t *stack ){
	for ( int i = 0; i < 3; ++i )
	{
		if ( stack->freewindings[i] ) {
			stack->freewindings[i] = 0;
			return &stack->windings[i];
		}
	}

	Error( "AllocStackWinding: failed" );

	return nullptr;
}

static void FreeStackWinding( fixedWinding_t *w, pstack_t *stack ){
	const int i = w - stack->windings;

	if ( i < 0 || i > 2 ) {
		return;     // not from local
	}
	if ( stack->freewindings[i] ) {
		Error( "FreeStackWinding: already free" );
	}
	stack->freewindings[i] = 1;
}

/*
   ==============
   VisChopWinding

   ==============
 */
static fixedWinding_t  *VisChopWinding( fixedWinding_t *in, pstack_t *stack, const visPlane_t& split ){
	float dists[128];
	EPlaneSide sides[128];
	int counts[3];
	float dot;
	int i, j;
	Vector3 mid;
	fixedWinding_t  *neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0; i < in->numpoints; ++i )
	{
		dists[i] = plane3_distance_to_point( split, in->points[i] );
		if ( dists[i] > ON_EPSILON ) {
			sides[i] = eSideFront;
		}
		else if ( dists[i] < -ON_EPSILON ) {
			sides[i] = eSideBack;
		}
		else
		{
			sides[i] = eSideOn;
		}
		counts[sides[i]]++;
	}

	if ( !counts[1] ) {
		return in;      // completely on front side
	}
	if ( !counts[0] ) {
		FreeStackWinding( in, stack );
		return nullptr;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];

	neww = AllocStackWinding( stack );

	neww->numpoints = 0;

	for ( i = 0; i < in->numpoints; ++i )
	{
		const Vector3& p1 = in->points[i];

		if ( neww->numpoints == MAX_POINTS_ON_FIXED_WINDING ) {
			FreeStackWinding( neww, stack );
			return in;      // can't chop -- fall back to original
		}

		if ( sides[i] == eSideOn ) {
			neww->points[neww->numpoints] = p1;
			neww->numpoints++;
			continue;
		}

		if ( sides[i] == eSideFront ) {
			neww->points[neww->numpoints] = p1;
			neww->numpoints++;
		}

		if ( sides[i + 1] == eSideOn || sides[i + 1] == sides[i] ) {
			continue;
		}

		if ( neww->numpoints == MAX_POINTS_ON_FIXED_WINDING ) {
			FreeStackWinding( neww, stack );
			return in;      // can't chop -- fall back to original
		}

		// generate a split point
		const Vector3& p2 = in->points[( i + 1 ) % in->numpoints];

		dot = dists[i] / ( dists[i] - dists[i + 1] );
		for ( j = 0; j < 3; ++j )
		{	// avoid round off error when possible
			if ( split.normal()[j] == 1 ) {
				mid[j] = split.dist();
			}
			else if ( split.normal()[j] == -1 ) {
				mid[j] = -split.dist();
			}
			else{
				mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
			}
		}

		neww->points[neww->numpoints] = mid;
		neww->numpoints++;
	}

	// free the original winding
	FreeStackWinding( in, stack );

	return neww;
}

/*
   ==============
   ClipToSeperators

   Source, pass, and target are an ordering of portals.

   Generates separating planes canidates by taking two points from source and one
   point from pass, and clips target by them.

   If target is totally clipped away, that portal can not be seen through.

   Normal clip keeps target on the same side as pass, which is correct if the
   order goes source, pass, target.  If the order goes pass, source, target then
   flipclip should be set.
   ==============
 */
static fixedWinding_t  *ClipToSeperators( fixedWinding_t *source, fixedWinding_t *pass, fixedWinding_t *target, bool flipclip, pstack_t *stack ){
	int i, j, k, l;
	float d;
	int counts[3];
	bool fliptest;

	// check all combinations
	for ( i = 0; i < source->numpoints; ++i )
	{
		l = ( i + 1 ) % source->numpoints;

		// find a vertex of pass that makes a plane that puts all of the
		// vertexes of pass on the front side and all of the vertexes of
		// source on the back side
		for ( j = 0; j < pass->numpoints; ++j )
		{
			visPlane_t plane;
			// if points don't make a valid plane, skip it
			if ( !PlaneFromPoints( plane, source->points[i], pass->points[j], source->points[l] ) ) {
				continue;
			}

			//
			// find out which side of the generated separating plane has the
			// source portal
			//
#if 1
			fliptest = false;
			for ( k = 0; k < source->numpoints; ++k )
			{
				if ( k == i || k == l ) {
					continue;
				}
				d = plane3_distance_to_point( plane, source->points[k] );
				if ( d < -ON_EPSILON ) { // source is on the negative side, so we want all
				                        // pass and target on the positive side
					fliptest = false;
					break;
				}
				else if ( d > ON_EPSILON ) { // source is on the positive side, so we want all
				                            // pass and target on the negative side
					fliptest = true;
					break;
				}
			}
			if ( k == source->numpoints ) {
				continue;       // planar with source portal
			}
#else
			fliptest = flipclip;
#endif
			//
			// flip the normal if the source portal is backwards
			//
			if ( fliptest ) {
				plane = plane3_flipped( plane );
			}
#if 1
			//
			// if all of the pass portal points are now on the positive side,
			// this is the separating plane
			//
			counts[0] = counts[1] = counts[2] = 0;
			for ( k = 0; k < pass->numpoints; ++k )
			{
				if ( k == j ) {
					continue;
				}
				d = plane3_distance_to_point( plane, pass->points[k] );
				if ( d < -ON_EPSILON ) {
					break;
				}
				else if ( d > ON_EPSILON ) {
					counts[0]++;
				}
				else{
					counts[2]++;
				}
			}
			if ( k != pass->numpoints ) {
				continue;   // points on negative side, not a separating plane
			}
			if ( !counts[0] ) {
				continue;   // planar with separating plane
			}
#else
			k = ( j + 1 ) % pass->numpoints;
			d = plane3_distance_to_point( plane, pass->points[k] );
			if ( d < -ON_EPSILON ) {
				continue;
			}
			k = ( j + pass->numpoints - 1 ) % pass->numpoints;
			d = plane3_distance_to_point( plane, pass->points[k] );
			if ( d < -ON_EPSILON ) {
				continue;
			}
#endif
			//
			// flip the normal if we want the back side
			//
			if ( flipclip ) {
				plane = plane3_flipped( plane );
			}

#ifdef SEPERATORCACHE
			stack->seperators[flipclip][stack->numseperators[flipclip]] = plane;
			if ( ++stack->numseperators[flipclip] >= MAX_SEPERATORS ) {
				Error( "MAX_SEPERATORS" );
			}
#endif
			//MrE: fast check first
			d = plane3_distance_to_point( plane, stack->portal->origin );
			//if completely at the back of the separator plane
			if ( d < -stack->portal->radius ) {
				return nullptr;
			}
			//if completely on the front of the separator plane
			if ( d > stack->portal->radius ) {
				break;
			}

			//
			// clip target by the separating plane
			//
			target = VisChopWinding( target, stack, plane );
			if ( !target ) {
				return nullptr;        // target is not visible
			}
			break;      // optimization by Antony Suter
		}
	}

	return target;
}

/*
   ==================
   RecursiveLeafFlow

   Flood fill through the leafs
   If src_portal is NULL, this is the originating leaf
   ==================
 */
static void RecursiveLeafFlow( int leafnum, threaddata_t *thread, pstack_t *prevstack ){
	pstack_t stack;
	visPlane_t backplane;
	leaf_t      *leaf;
	int n;

	thread->c_chains++;

	leaf = &leafs[leafnum];
//	CheckStack( leaf, thread );

	prevstack->next = &stack;

	stack.next = nullptr;
	stack.leaf = leaf;
	stack.portal = nullptr;
	stack.depth = prevstack->depth + 1;
	BhedFrameBuffers( stack );

#ifdef SEPERATORCACHE
	stack.numseperators[0] = 0;
	stack.numseperators[1] = 0;
#endif

	const uint64_t *vis = reinterpret_cast<const uint64_t*>( thread->base->portalvis );

	// check all portals for flowing into other leafs
	for ( vportal_t *p : Span( leaf->portals, leaf->numportals ) )
	{
		if ( p->removed ) {
			continue;
		}
		const int pnum = p - portals;

		/* MrE: portal trace debug code
		   {
		    int portaltrace[] = { 13, 16, 17, 37 };
		    pstack_t *s;

		    s = &thread->pstack_head;
		    for ( j = 0; s->next && j < sizeof( portaltrace ) / sizeof( int ) - 1; ++j, s = s->next )
		    {
		        if ( s->portal->num != portaltrace[j] )
		            break;
		    }
		    if ( j >= sizeof( portaltrace ) / sizeof( int ) - 1 )
		    {
		        if ( p->num == portaltrace[j] )
		            n = 0; //traced through all the portals
		    }
		   }
		 */

		if ( !bit_is_enabled( reinterpret_cast<const byte*>( prevstack->mightsee ), pnum ) ) {
			continue;   // can't possibly see it
		}

		// if the portal can't see anything we haven't already seen, skip it
		const uint64_t *test = BhedVector( thread, p );
		if ( test == nullptr ) {
			int k = 0;
			test = BhedResolve( p, [&]( int& w, uint64_t& query ){
				if ( k == prevstack->nummightwords ) {
					return false;
				}
				w = prevstack->mightwords[k++];
				query = prevstack->mightsee[w];
				return true;
			} );
		}

		/* BehavEd vis1: only the nonzero words of the previous vector (the others stay zero) */
		BhedClearMight( stack );
		uint64_t more = 0;
		for ( int k = 0; k < prevstack->nummightwords; ++k )
		{
			const int w = prevstack->mightwords[k];
			const uint64_t m = prevstack->mightsee[w] & test[w];
			if ( m != 0 ) {
				stack.mightsee[w] = m;
				stack.mightwords[stack.nummightwords++] = uint16_t( w );
				more |= m & ~vis[w];
			}
		}

		if ( !more &&
		     bit_is_enabled( thread->base->portalvis, pnum ) ) { // can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;
		backplane = plane3_flipped( p->plane );

		stack.portal = p;
		stack.next = nullptr;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;

#if 1
		{
			const float d = plane3_distance_to_point( thread->pstack_head.portalplane, p->origin );
			if ( d < -p->radius ) {
				continue;
			}
			else if ( d > p->radius ) {
				stack.pass = p->winding;
			}
			else
			{
				stack.pass = VisChopWinding( p->winding, &stack, thread->pstack_head.portalplane );
				if ( !stack.pass ) {
					continue;
				}
			}
		}
#else
		stack.pass = VisChopWinding( p->winding, &stack, &thread->pstack_head.portalplane );
		if ( !stack.pass ) {
			continue;
		}
#endif


#if 1
		{
			const float d = plane3_distance_to_point( p->plane, thread->base->origin );
			//MrE: vis-bug fix
			//if ( d > p->radius )
			if ( d > thread->base->radius ) {
				continue;
			}
			//MrE: vis-bug fix
			//if ( d < -p->radius )
			else if ( d < -thread->base->radius ) {
				stack.source = prevstack->source;
			}
			else
			{
				stack.source = VisChopWinding( prevstack->source, &stack, backplane );
				//FIXME: shouldn't we create a new source origin and radius for fast checks?
				if ( !stack.source ) {
					continue;
				}
			}
		}
#else
		stack.source = VisChopWinding( prevstack->source, &stack, backplane );
		if ( !stack.source ) {
			continue;
		}
#endif

		if ( !prevstack->pass ) { // the second leaf can only be blocked if coplanar

			// mark the portal as visible
			BhedMark( thread->base, pnum );

			RecursiveLeafFlow( p->leaf, thread, &stack );
			continue;
		}

#ifdef SEPERATORCACHE
		if ( stack.numseperators[0] ) {
			for ( n = 0; n < stack.numseperators[0]; ++n )
			{
				stack.pass = VisChopWinding( stack.pass, &stack, stack.seperators[0][n] );
				if ( !stack.pass ) {
					break;      // target is not visible
				}
			}
			if ( n < stack.numseperators[0] ) {
				continue;
			}
		}
		else
		{
			stack.pass = ClipToSeperators( prevstack->source, prevstack->pass, stack.pass, false, &stack );
		}
#else
		stack.pass = ClipToSeperators( stack.source, prevstack->pass, stack.pass, false, &stack );
#endif
		if ( !stack.pass ) {
			continue;
		}

#ifdef SEPERATORCACHE
		if ( stack.numseperators[1] ) {
			for ( n = 0; n < stack.numseperators[1]; ++n )
			{
				stack.pass = VisChopWinding( stack.pass, &stack, stack.seperators[1][n] );
				if ( !stack.pass ) {
					break;      // target is not visible
				}
			}
		}
		else
		{
			stack.pass = ClipToSeperators( prevstack->pass, prevstack->source, stack.pass, true, &stack );
		}
#else
		stack.pass = ClipToSeperators( prevstack->pass, stack.source, stack.pass, true, &stack );
#endif
		if ( !stack.pass ) {
			continue;
		}

		// mark the portal as visible
		BhedMark( thread->base, pnum );

		// flow through it for real
		RecursiveLeafFlow( p->leaf, thread, &stack );
		//
		stack.next = nullptr;
	}
	BhedClearMight( stack );
}

/*
   ===============
   PortalFlow

   generates the portalvis bit vector
   ===============
 */
void PortalFlow( int portalnum ){
	threaddata_t data;
	vportal_t       *p;
	int c_might, c_can;

#ifdef MREDEBUG
	Sys_Printf( "\r%6d", portalnum );
#endif

	p = sorted_portals[portalnum];

	if ( p->removed ) {
		BhedSetStatus( p, EVStatus::Done );
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );

	RecursiveLeafFlow( p->leaf, &data, &data.pstack_head );

	BhedSetStatus( p, EVStatus::Done );

	c_can = CountBits( p->portalvis, numportals * 2 );

	Sys_FPrintf( SYS_VRB, "portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n",
	             (int)( p - portals ), c_might, c_can, data.c_chains );
}

/*
   ==================
   RecursivePassageFlow
   ==================
 */
static void RecursivePassageFlow( vportal_t *portal, threaddata_t *thread, pstack_t *prevstack ){
	pstack_t stack;
	vportal_t   *p;
	leaf_t      *leaf;
	int i;

	leaf = &leafs[portal->leaf];

	prevstack->next = &stack;

	stack.next = nullptr;
	stack.depth = prevstack->depth + 1;
	BhedFrameBuffers( stack );

	const uint64_t *vis = reinterpret_cast<const uint64_t*>( thread->base->portalvis );

	const passage_t *passage = portal->passages;
	// check all portals for flowing into other leafs
	for ( i = 0; i < leaf->numportals; ++i )
	{
		p = leaf->portals[i];
		if ( p->removed ) {
			continue;
		}
		const passage_t *ps = passage++;
		const int pnum = p - portals;

		if ( !bit_is_enabled( reinterpret_cast<const byte*>( prevstack->mightsee ), pnum ) ) {
			continue;   // can't possibly see it
		}

		// mark the portal as visible
		BhedMark( thread->base, pnum );

		/* BehavEd vis1: only the nonzero words of cansee (the rest of the new vector is zero) */
		const uint64_t *portalvis = BhedVector( thread, p );
		if ( portalvis == nullptr ) {
			int k = 0;
			portalvis = BhedResolve( p, [&]( int& w, uint64_t& query ){
				if ( k == ps->numwords ) {
					return false;
				}
				w = ps->words[k];
				query = prevstack->mightsee[w] & ps->bits[k];
				++k;
				return true;
			} );
		}
		BhedClearMight( stack );
		uint64_t more = 0;
		for ( int k = 0; k < ps->numwords; ++k )
		{
			const int w = ps->words[k];
			const uint64_t m = prevstack->mightsee[w] & ps->bits[k] & portalvis[w];
			if ( m != 0 ) {
				stack.mightsee[w] = m;
				stack.mightwords[stack.nummightwords++] = uint16_t( w );
				more |= m & ~vis[w];
			}
		}

		if ( !more ) {
			// can't see anything new
			continue;
		}

		// flow through it for real
		RecursivePassageFlow( p, thread, &stack );

		stack.next = nullptr;
	}
	BhedClearMight( stack );
}

/*
   ===============
   PassageFlow
   ===============
 */
void PassageFlow( int portalnum ){
	threaddata_t data;
	vportal_t       *p;
//	int             c_might, c_can;

#ifdef MREDEBUG
	Sys_Printf( "\r%6d", portalnum );
#endif

	p = sorted_portals[portalnum];

	if ( p->removed ) {
		BhedSetStatus( p, EVStatus::Done );
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

//	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );

	RecursivePassageFlow( p, &data, &data.pstack_head );

	BhedSetStatus( p, EVStatus::Done );

	/*
	   c_can = CountBits( p->portalvis, numportals * 2 );

	   Sys_FPrintf( SYS_VRB, "portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n",
	    (int)( p - portals ), c_might, c_can, data.c_chains );
	 */
}

/*
   ==================
   RecursivePassagePortalFlow
   ==================
 */
static void RecursivePassagePortalFlow( vportal_t *portal, threaddata_t *thread, pstack_t *prevstack ){
	pstack_t stack;
	vportal_t   *p;
	leaf_t      *leaf;
	visPlane_t backplane;
	int i, n;

//	thread->c_chains++;

	leaf = &leafs[portal->leaf];
//	CheckStack( leaf, thread );

	prevstack->next = &stack;

	stack.next = nullptr;
	stack.leaf = leaf;
	stack.portal = nullptr;
	stack.depth = prevstack->depth + 1;
	BhedFrameBuffers( stack );

#ifdef SEPERATORCACHE
	stack.numseperators[0] = 0;
	stack.numseperators[1] = 0;
#endif

	const uint64_t *vis = reinterpret_cast<const uint64_t*>( thread->base->portalvis );

	const passage_t *passage = portal->passages;
	// check all portals for flowing into other leafs
	for ( i = 0; i < leaf->numportals; ++i )
	{
		p = leaf->portals[i];
		if ( p->removed ) {
			continue;
		}
		const passage_t *ps = passage++;
		const int pnum = p - portals;

		if ( !bit_is_enabled( reinterpret_cast<const byte*>( prevstack->mightsee ), pnum ) ) {
			continue;   // can't possibly see it
		}
		/* BehavEd vis1: only the nonzero words of cansee (the rest of the new vector is zero) */
		const uint64_t *portalvis = BhedVector( thread, p );
		if ( portalvis == nullptr ) {
			int k = 0;
			portalvis = BhedResolve( p, [&]( int& w, uint64_t& query ){
				if ( k == ps->numwords ) {
					return false;
				}
				w = ps->words[k];
				query = prevstack->mightsee[w] & ps->bits[k];
				++k;
				return true;
			} );
		}
		BhedClearMight( stack );
		uint64_t more = 0;
		for ( int k = 0; k < ps->numwords; ++k )
		{
			const int w = ps->words[k];
			const uint64_t m = prevstack->mightsee[w] & ps->bits[k] & portalvis[w];
			if ( m != 0 ) {
				stack.mightsee[w] = m;
				stack.mightwords[stack.nummightwords++] = uint16_t( w );
				more |= m & ~vis[w];
			}
		}

		if ( !more && bit_is_enabled( thread->base->portalvis, pnum ) ) { // can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;
		backplane = plane3_flipped( p->plane );

		stack.portal = p;
		stack.next = nullptr;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;

#if 1
		{
			const float d = plane3_distance_to_point( thread->pstack_head.portalplane, p->origin );
			if ( d < -p->radius ) {
				continue;
			}
			else if ( d > p->radius ) {
				stack.pass = p->winding;
			}
			else
			{
				stack.pass = VisChopWinding( p->winding, &stack, thread->pstack_head.portalplane );
				if ( !stack.pass ) {
					continue;
				}
			}
		}
#else
		stack.pass = VisChopWinding( p->winding, &stack, thread->pstack_head.portalplane );
		if ( !stack.pass ) {
			continue;
		}
#endif


#if 1
		{
			const float d = plane3_distance_to_point( p->plane, thread->base->origin );
			//MrE: vis-bug fix
			//if ( d > p->radius )
			if ( d > thread->base->radius ) {
				continue;
			}
			//MrE: vis-bug fix
			//if ( d < -p->radius )
			else if ( d < -thread->base->radius ) {
				stack.source = prevstack->source;
			}
			else
			{
				stack.source = VisChopWinding( prevstack->source, &stack, backplane );
				//FIXME: shouldn't we create a new source origin and radius for fast checks?
				if ( !stack.source ) {
					continue;
				}
			}
		}
#else
		stack.source = VisChopWinding( prevstack->source, &stack, backplane );
		if ( !stack.source ) {
			continue;
		}
#endif

		if ( !prevstack->pass ) { // the second leaf can only be blocked if coplanar

			// mark the portal as visible
			BhedMark( thread->base, pnum );

			RecursivePassagePortalFlow( p, thread, &stack );
			continue;
		}

#ifdef SEPERATORCACHE
		if ( stack.numseperators[0] ) {
			for ( n = 0; n < stack.numseperators[0]; ++n )
			{
				stack.pass = VisChopWinding( stack.pass, &stack, stack.seperators[0][n] );
				if ( !stack.pass ) {
					break;      // target is not visible
				}
			}
			if ( n < stack.numseperators[0] ) {
				continue;
			}
		}
		else
		{
			stack.pass = ClipToSeperators( prevstack->source, prevstack->pass, stack.pass, false, &stack );
		}
#else
		stack.pass = ClipToSeperators( stack.source, prevstack->pass, stack.pass, false, &stack );
#endif
		if ( !stack.pass ) {
			continue;
		}

#ifdef SEPERATORCACHE
		if ( stack.numseperators[1] ) {
			for ( n = 0; n < stack.numseperators[1]; ++n )
			{
				stack.pass = VisChopWinding( stack.pass, &stack, stack.seperators[1][n] );
				if ( !stack.pass ) {
					break;      // target is not visible
				}
			}
		}
		else
		{
			stack.pass = ClipToSeperators( prevstack->pass, prevstack->source, stack.pass, true, &stack );
		}
#else
		stack.pass = ClipToSeperators( prevstack->pass, stack.source, stack.pass, true, &stack );
#endif
		if ( !stack.pass ) {
			continue;
		}

		// mark the portal as visible
		BhedMark( thread->base, pnum );

		// flow through it for real
		RecursivePassagePortalFlow( p, thread, &stack );
		//
		stack.next = nullptr;
	}
	BhedClearMight( stack );
}

/*
   ===============
   PassagePortalFlow
   ===============
 */
void PassagePortalFlow( int portalnum ){
	threaddata_t data;
	vportal_t       *p;
//	int				c_might, c_can;

#ifdef MREDEBUG
	Sys_Printf( "\r%6d", portalnum );
#endif

	p = sorted_portals[portalnum];

	if ( p->removed ) {
		BhedSetStatus( p, EVStatus::Done );
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

//	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );

	RecursivePassagePortalFlow( p, &data, &data.pstack_head );

	BhedSetStatus( p, EVStatus::Done );

	/*
	   c_can = CountBits( p->portalvis, numportals * 2 );

	   Sys_FPrintf( SYS_VRB, "portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n",
	    (int)( p - portals ), c_might, c_can, data.c_chains );
	 */
}

static fixedWinding_t *PassageChopWinding( fixedWinding_t *in, fixedWinding_t *out, const visPlane_t& split ){
	float dists[128];
	EPlaneSide sides[128];
	int counts[3];
	float dot;
	int i, j;
	Vector3 mid;
	fixedWinding_t  *neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0; i < in->numpoints; ++i )
	{
		dists[i] = plane3_distance_to_point( split, in->points[i] );
		if ( dists[i] > ON_EPSILON ) {
			sides[i] = eSideFront;
		}
		else if ( dists[i] < -ON_EPSILON ) {
			sides[i] = eSideBack;
		}
		else
		{
			sides[i] = eSideOn;
		}
		counts[sides[i]]++;
	}

	if ( !counts[1] ) {
		return in;      // completely on front side
	}
	if ( !counts[0] ) {
		return nullptr;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];

	neww = out;

	neww->numpoints = 0;

	for ( i = 0; i < in->numpoints; ++i )
	{
		const Vector3& p1 = in->points[i];

		if ( neww->numpoints == MAX_POINTS_ON_FIXED_WINDING ) {
			return in;      // can't chop -- fall back to original
		}

		if ( sides[i] == eSideOn ) {
			neww->points[neww->numpoints] = p1;
			neww->numpoints++;
			continue;
		}

		if ( sides[i] == eSideFront ) {
			neww->points[neww->numpoints] = p1;
			neww->numpoints++;
		}

		if ( sides[i + 1] == eSideOn || sides[i + 1] == sides[i] ) {
			continue;
		}

		if ( neww->numpoints == MAX_POINTS_ON_FIXED_WINDING ) {
			return in;      // can't chop -- fall back to original
		}

		// generate a split point
		const Vector3& p2 = in->points[( i + 1 ) % in->numpoints];

		dot = dists[i] / ( dists[i] - dists[i + 1] );
		for ( j = 0; j < 3; ++j )
		{	// avoid round off error when possible
			if ( split.normal()[j] == 1 ) {
				mid[j] = split.dist();
			}
			else if ( split.normal()[j] == -1 ) {
				mid[j] = -split.dist();
			}
			else{
				mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
			}
		}

		neww->points[neww->numpoints] = mid;
		neww->numpoints++;
	}

	return neww;
}

/*
   ===============
   AddSeperators
   ===============
 */
static int AddSeperators( const fixedWinding_t *source, const fixedWinding_t *pass, bool flipclip, visPlane_t *seperators, int maxseperators ){
	int i, j, k, l;
	int counts[3], numseperators;
	bool fliptest;

	numseperators = 0;
	// check all combinations
	for ( i = 0; i < source->numpoints; ++i )
	{
		l = ( i + 1 ) % source->numpoints;

		// find a vertex of pass that makes a plane that puts all of the
		// vertexes of pass on the front side and all of the vertexes of
		// source on the back side
		for ( j = 0; j < pass->numpoints; ++j )
		{
			visPlane_t plane;
			// if points don't make a valid plane, skip it
			if ( !PlaneFromPoints( plane, source->points[i], pass->points[j], source->points[l] ) ) {
				continue;
			}

			//
			// find out which side of the generated separating plane has the
			// source portal
			//
#if 1
			fliptest = false;
			for ( k = 0; k < source->numpoints; ++k )
			{
				if ( k == i || k == l ) {
					continue;
				}
				const double d = plane3_distance_to_point( plane, source->points[k] );
				if ( d < -ON_EPSILON ) { // source is on the negative side, so we want all
				                        // pass and target on the positive side
					fliptest = false;
					break;
				}
				else if ( d > ON_EPSILON ) { // source is on the positive side, so we want all
				                            // pass and target on the negative side
					fliptest = true;
					break;
				}
			}
			if ( k == source->numpoints ) {
				continue;       // planar with source portal
			}
#else
			fliptest = flipclip;
#endif
			//
			// flip the normal if the source portal is backwards
			//
			if ( fliptest ) {
				plane = plane3_flipped( plane );
			}
#if 1
			//
			// if all of the pass portal points are now on the positive side,
			// this is the separating plane
			//
			counts[0] = counts[1] = counts[2] = 0;
			for ( k = 0; k < pass->numpoints; ++k )
			{
				if ( k == j ) {
					continue;
				}
				const double d = plane3_distance_to_point( plane, pass->points[k] );
				if ( d < -ON_EPSILON ) {
					break;
				}
				else if ( d > ON_EPSILON ) {
					counts[0]++;
				}
				else{
					counts[2]++;
				}
			}
			if ( k != pass->numpoints ) {
				continue;   // points on negative side, not a separating plane
			}
			if ( !counts[0] ) {
				continue;   // planar with separating plane
			}
#else
			k = ( j + 1 ) % pass->numpoints;
			d = vector3_dot( pass->points[k], plane.normal ) - plane.dist;
			if ( d < -ON_EPSILON ) {
				continue;
			}
			k = ( j + pass->numpoints - 1 ) % pass->numpoints;
			d = vector3_dot( pass->points[k], plane.normal ) - plane.dist;
			if ( d < -ON_EPSILON ) {
				continue;
			}
#endif
			//
			// flip the normal if we want the back side
			//
			if ( flipclip ) {
				plane = plane3_flipped( plane );
			}

			if ( numseperators >= maxseperators ) {
				Error( "max seperators" );
			}
			seperators[numseperators] = plane;
			numseperators++;
			break;
		}
	}
	return numseperators;
}

/*
   ===============
   CreatePassages

   MrE: create passages from one portal to all the portals in the leaf the portal leads to
     every passage has a cansee bit string with all the portals that can be
     seen through the passage
   ===============
 */
/* BehavEd vis1: memory the passages really take (CreatePassages), for the line after the phase */
static std::atomic<size_t> g_bhedPassageBytes{ 0 };

void CreatePassages( int portalnum ){
	int k, n, numseperators;
	vportal_t       *portal, *p;
	visPlane_t seperators[MAX_SEPERATORS * 2];
	fixedWinding_t  *w;
	fixedWinding_t in, out, *res;


#ifdef MREDEBUG
	Sys_Printf( "\r%6d", portalnum );
#endif

	portal = sorted_portals[portalnum];

	if ( portal->removed ) {
		portal->status = EVStatus::Done;
		return;
	}

	/* BehavEd vis1: cansee of each passage in a full vector per thread, kept as its nonzero words;
	   all passages of the portal in one block */
	static thread_local std::vector<uint64_t> cansee;
	static thread_local std::vector<uint16_t> allwords;
	static thread_local std::vector<uint64_t> allbits;
	static thread_local std::vector<int> counts;
	cansee.assign( portalwords, 0 );
	allwords.clear();
	allbits.clear();
	counts.clear();
	const uint64_t *portalflood = reinterpret_cast<const uint64_t*>( portal->portalflood );

	for ( const vportal_t *target : Span( leafs[portal->leaf].portals, leafs[portal->leaf].numportals ) )
	{
		if ( target->removed ) {
			continue;
		}

		numseperators = AddSeperators( portal->winding, target->winding, false, seperators, MAX_SEPERATORS * 2 );
		numseperators += AddSeperators( target->winding, portal->winding, true, &seperators[numseperators], MAX_SEPERATORS * 2 - numseperators );

		//create the passage->cansee
		/* BehavEd vis1: the portals in both flood vectors in ascending order, word by word (NRC tested
		   every portal of the map) - the same portals in the same order */
		const uint64_t *targetflood = reinterpret_cast<const uint64_t*>( target->portalflood );
		for ( int wi = 0; wi < portalwords; ++wi )
		{
			for ( uint64_t both = portalflood[wi] & targetflood[wi]; both != 0; both &= both - 1 )
			{
				const int j = wi * 64 + std::countr_zero( both );
				p = &portals[j];
				if ( p->removed ) {
					continue;
				}
				for ( k = 0; k < numseperators; ++k )
				{
					//if completely at the back of the separator plane
					if ( plane3_distance_to_point( seperators[k], p->origin ) < -p->radius + ON_EPSILON ) {
						break;
					}
					w = p->winding;
					for ( n = 0; n < w->numpoints; ++n )
					{
						//if at the front of the separator
						if ( plane3_distance_to_point( seperators[k], w->points[n] ) > ON_EPSILON ) {
							break;
						}
					}
					//if no points are at the front of the separator
					if ( n >= w->numpoints ) {
						break;
					}
				}
				if ( k < numseperators ) {
					continue;
				}

				/* explitive deleted */


				/* ydnar: prefer correctness to stack overflow  */
				//% memcpy( &in, p->winding, (int)((fixedWinding_t *)0)->points[p->winding->numpoints] );
				if ( p->winding->numpoints <= MAX_POINTS_ON_FIXED_WINDING ) {
					memcpy( &in, p->winding, offsetof_array( fixedWinding_t, points, p->winding->numpoints ) );
				}
				else{
					memcpy( &in, p->winding, sizeof( fixedWinding_t ) );
				}


				for ( k = 0; k < numseperators; ++k )
				{
					/* ydnar: this is a shitty crutch */
					//% if ( in.numpoints > MAX_POINTS_ON_FIXED_WINDING ) Sys_Printf( "[%d]", p->winding->numpoints );
					value_minimize( in.numpoints, MAX_POINTS_ON_FIXED_WINDING );

					res = PassageChopWinding( &in, &out, seperators[ k ] );
					if ( res == &out ) {
						memcpy( &in, &out, sizeof( fixedWinding_t ) );
					}


					if ( res == nullptr ) {
						break;
					}
				}
				if ( k < numseperators ) {
					continue;
				}
				cansee[wi] |= uint64_t( 1 ) << ( j & 63 );
			}
		}
		int count = 0;
		for ( int wi = 0; wi < portalwords; ++wi )
		{
			if ( cansee[wi] != 0 ) {
				allwords.push_back( uint16_t( wi ) );
				allbits.push_back( cansee[wi] );
				cansee[wi] = 0;
				++count;
			}
		}
		counts.push_back( count );
	}

	const size_t bytes = counts.size() * sizeof( passage_t ) + allbits.size() * sizeof( uint64_t ) + allwords.size() * sizeof( uint16_t );
	byte *block = safe_malloc( std::max<size_t>( bytes, 1 ) );
	passage_t *passages = reinterpret_cast<passage_t*>( block );
	uint64_t *bits = reinterpret_cast<uint64_t*>( block + counts.size() * sizeof( passage_t ) );
	uint16_t *words = reinterpret_cast<uint16_t*>( bits + allbits.size() );
	std::copy( allbits.begin(), allbits.end(), bits );
	std::copy( allwords.begin(), allwords.end(), words );
	for ( const int count : counts )
	{
		passages->numwords = count;
		passages->words = words;
		passages->bits = bits;
		words += count;
		bits += count;
		++passages;
	}
	portal->passages = reinterpret_cast<passage_t*>( block );
	g_bhedPassageBytes += bytes;
}

void PassageMemory(){
	/* BehavEd stage 9d: NRC counted only the first numportals of the numportals * 2 sorted
	   portals (CreatePassages runs for all of them) and summed into an int - for
	   md_rev_TarisUpper2 (76084 portals) it printed 268 MB, CreatePassages took about 9 GB.
	   All portals and size_t. BehavEd vis1: that is the size with full bit vectors - the passages
	   keep only their nonzero words, CreatePassages prints what they really take. */
	size_t totalmem = 0, totalportals = 0;

	for ( const vportal_t *portal : Span( sorted_portals, numportals * 2 ) )
	{
		if ( portal->removed ) {
			continue;
		}
		for ( const vportal_t *target : Span( leafs[portal->leaf].portals, leafs[portal->leaf].numportals ) )
		{
			if ( target->removed ) {
				continue;
			}
			totalmem += sizeof( passage_t ) + portalbytes;
			totalportals++;
		}
	}
	Sys_Printf( "%7zu average number of passages per leaf\n", totalportals / ( numportals * 2 ) );
	Sys_Printf( "%7zu MB passage memory at most (full bit vectors)\n", totalmem >> 10 >> 10 );
}

void BhedPassageMemoryUsed(){
	const size_t vectors = size_t( numportals ) * 2 * 2 * portalbytes;   // portalflood, portalvis
	Sys_Printf( "BehavEd: passages take %zu MB, vis memory %zu MB\n", g_bhedPassageBytes >> 10 >> 10,
	            ( g_bhedPassageBytes + vectors ) >> 10 >> 10 );
}

/*
   ===============================================================================

   This is a rough first-order aproximation that is used to trivially reject some
   of the final calculations.


   Calculates portalfront and portalflood bit vectors

   thinking about:

   typedef struct passage_s
   {
    struct passage_s	*next;
    struct portal_s		*to;
    stryct sep_s		*seperators;
    byte				*mightsee;
   } passage_t;

   typedef struct portal_s
   {
    struct passage_s	*passages;
    int					leaf;		// leaf portal faces into
   } portal_s;

   leaf = portal->leaf
   clear
   for all portals


   calc portal visibility
    clear bit vector
    for all passages
        passage visibility


   for a portal to be visible to a passage, it must be on the front of
   all separating planes, and both portals must be behind the new portal

   ===============================================================================
 */



/*
   ==================
   SimpleFlood

   ==================
 */
static void SimpleFlood( vportal_t *srcportal, int leafnum ){
	for ( const vportal_t *p : Span( leafs[leafnum].portals, leafs[leafnum].numportals ) )
	{
		if ( p->removed ) {
			continue;
		}
		const int pnum = p - portals;
		if ( !bit_is_enabled( srcportal->portalfront, pnum ) ) {
			continue;
		}

		if ( bit_is_enabled( srcportal->portalflood, pnum ) ) {
			continue;
		}

		bit_enable( srcportal->portalflood, pnum );

		SimpleFlood( srcportal, p->leaf );
	}
}

/*
   ==============
   BasePortalVis
   ==============
 */
void BasePortalVis( int portalnum ){
	int j, k;
	vportal_t   *tp, *p;
	fixedWinding_t  *w;


	p = portals + portalnum;

	if ( p->removed ) {
		return;
	}

	p->portalfront = safe_calloc( portalbytes );
	p->portalflood = safe_calloc( portalbytes );
	p->portalvis = safe_calloc( portalbytes );

	for ( j = 0, tp = portals; j < numportals * 2; ++j, ++tp )
	{
		if ( j == portalnum ) {
			continue;
		}
		if ( tp->removed ) {
			continue;
		}

		/* ydnar: this is old farplane vis code from mre */
		/*
		   if ( farplanedist >= 0 )
		   {
		    vec3_t dir;
		    VectorSubtract( p->origin, tp->origin, dir );
		    if ( VectorLength( dir ) > farplanedist - p->radius - tp->radius )
		        continue;
		   }
		 */

		if ( !p->sky && !tp->sky && farPlaneDist != 0 ) {
			if( farPlaneDistMode == 'o' ){
				if( vector3_length( p->origin - tp->origin ) > farPlaneDist )
					continue;
			}
			else if( farPlaneDistMode == 'e' ){
				if( vector3_length( p->origin - tp->origin ) + p->radius + tp->radius > 2.0f * farPlaneDist )
					continue;
			}
			else if( farPlaneDistMode == 'r' ){
				if( p->radius + tp->radius > farPlaneDist )
					continue;
			}
			else{ /* ydnar: this is known-to-be-working farplane code */
				if ( vector3_length( p->origin - tp->origin ) - p->radius - tp->radius > farPlaneDist )
					continue;
			}
		}


		w = tp->winding;
		for ( k = 0; k < w->numpoints; ++k )
		{
			if ( plane3_distance_to_point( p->plane, w->points[k] ) > ON_EPSILON ) {
				break;
			}
		}
		if ( k == w->numpoints ) {
			continue;   // no points on front
		}
		w = p->winding;
		for ( k = 0; k < w->numpoints; ++k )
		{
			if ( plane3_distance_to_point( tp->plane, w->points[k] ) < -ON_EPSILON ) {
				break;
			}
		}
		if ( k == w->numpoints ) {
			continue;   // no points on front
		}
		bit_enable( p->portalfront, j );
	}

	SimpleFlood( p, p->leaf );
	/* BehavEd vis1: portalfront is only needed here */
	free( p->portalfront );
	p->portalfront = nullptr;

	p->nummightsee = CountBits( p->portalflood, numportals * 2 );
//	Sys_Printf( "portal %i: %i mightsee\n", portalnum, p->nummightsee );
}





/*
   ===============================================================================

   This is a second order aproximation

   Calculates portalvis bit vector

   WAAAAAAY too slow.

   ===============================================================================
 */

/*
   ==================
   RecursiveLeafBitFlow

   ==================
 */
static void RecursiveLeafBitFlow( int leafnum, byte *mightsee, byte *cansee ){
	byte newmight[MAX_PORTALS / 8];


	// check all portals for flowing into other leafs
	for ( const vportal_t *p : Span( leafs[leafnum].portals, leafs[leafnum].numportals ) )
	{
		if ( p->removed ) {
			continue;
		}
		const int pnum = p - portals;

		// if some previous portal can't see it, skip
		if ( !bit_is_enabled( mightsee, pnum ) ) {
			continue;
		}

		// if this portal can see some portals we mightsee, recurse
		long more = 0;
		for ( int i = 0; i < portallongs; ++i )
		{
			( (long *)newmight )[i] = ( (long *)mightsee )[i]
			                          & ( (long *)p->portalflood )[i];
			more |= ( (long *)newmight )[i] & ~( (long *)cansee )[i];
		}

		if ( !more ) {
			continue;   // can't see anything new
		}
		bit_enable( cansee, pnum );

		RecursiveLeafBitFlow( p->leaf, newmight, cansee );
	}
}

/*
   ==============
   BetterPortalVis
   ==============
 */
void BetterPortalVis( int portalnum ){
	vportal_t   *p;

	p = portals + portalnum;

	if ( p->removed ) {
		return;
	}

	RecursiveLeafBitFlow( p->leaf, p->portalflood, p->portalvis );

	// build leaf vis information
	p->nummightsee = CountBits( p->portalvis, numportals * 2 );
}
