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
#if defined( _MSC_VER )
#include <intrin.h>     /* _umul128 */
#endif

#include <algorithm>
#include <atomic>
#include <bit>
#include <climits>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
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
   zeroed once and kept zero outside the words a frame lists (pstack_t::mightwords, and as a bit mask
   mightmask). A flow step then only touches the words that are nonzero in both the previous vector
   and the passage (the masks ANDed): on duel_mos_espa a frame has 11 of 212 words nonzero on average,
   a passage 30; NRC copied and combined all words for every portal of every leaf. Same bits. */
namespace
{
struct BhedVisFrames
{
	std::vector<uint64_t*> bits;
	std::vector<uint16_t*> words;
	std::vector<uint64_t*> masks;
	~BhedVisFrames(){
		for ( uint64_t *b : bits )
			free( b );
		for ( uint16_t *w : words )
			free( w );
		for ( uint64_t *m : masks )
			free( m );
	}
};
thread_local BhedVisFrames t_visFrames;
}

static void BhedFrameBuffers( pstack_t& stack ){
	BhedVisFrames& f = t_visFrames;
	while ( int( f.bits.size() ) <= stack.depth ) {
		f.bits.push_back( safe_calloc( portalwords * sizeof( uint64_t ) ) );
		f.words.push_back( safe_malloc( portalwords * sizeof( uint16_t ) ) );
		f.masks.push_back( safe_calloc( portalmaskwords * sizeof( uint64_t ) ) );
	}
	stack.mightsee = f.bits[stack.depth];
	stack.mightwords = f.words[stack.depth];
	stack.mightmask = f.masks[stack.depth];
	stack.nummightwords = 0;
}

static void BhedClearMight( pstack_t& stack ){
	for ( int k = 0; k < stack.nummightwords; ++k )
	{
		const int w = stack.mightwords[k];
		stack.mightsee[w] = 0;
		stack.mightmask[w >> 6] = 0;
	}
	stack.nummightwords = 0;
}

/* a nonzero word of the new vector */
static void BhedSetMight( pstack_t& stack, int w, uint64_t m ){
	stack.mightsee[w] = m;
	stack.mightwords[stack.nummightwords++] = uint16_t( w );
	stack.mightmask[w >> 6] |= uint64_t( 1 ) << ( w & 63 );
}

/* f( w, cansee word w ) for the words that are nonzero in both prev and the passage, ascending */
template<typename F>
static void BhedCommonWords( const pstack_t *prev, const passage_t *ps, F f ){
	for ( int mw = 0; mw < portalmaskwords; ++mw )
	{
		const uint64_t pm = ps->mask[mw];
		uint64_t both = prev->mightmask[mw] & pm;
		while ( both != 0 )
		{
			const int b = std::countr_zero( both );
			both &= both - 1;
			f( mw * 64 + b, ps->bits[ps->rank[mw] + std::popcount( pm & ( ( uint64_t( 1 ) << b ) - 1 ) )] );
		}
	}
}

/* the head of a flow: the portal's flood vector itself (read only), with the list of its nonzero words */
static void BhedHeadFrame( pstack_t& head, const vportal_t *p ){
	head.depth = 0;
	BhedFrameBuffers( head );
	head.mightsee = reinterpret_cast<uint64_t*>( p->portalflood );
	for ( int w = 0; w < portalwords; ++w )
		if ( head.mightsee[w] != 0 ) {
			head.mightwords[head.nummightwords++] = uint16_t( w );
			head.mightmask[w >> 6] |= uint64_t( 1 ) << ( w & 63 );
		}
}

/* after the flow: the head frame's list and mask back to zero (its bits are portalflood) */
static void BhedHeadDone( pstack_t& head ){
	for ( int k = 0; k < head.nummightwords; ++k )
		head.mightmask[head.mightwords[k] >> 6] = 0;
	head.nummightwords = 0;
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
   BehavEd vis2: the wait ends as soon as the running flow has found every bit that is asked for
   (BhedResolve, BhedWaitFound) - on t3_byss three of five waits were for portals whose result for
   the question was portalflood anyway. What remains are the flows that really need the end of an
   earlier one; chains of them keep the deterministic VIS behind NRC's rule on open maps
   (docs/intern/NOTIZEN_vis2_1011.md), hence a switch. */
namespace
{
std::vector<int> g_bhedRank;            /* position of each portal in sorted_portals */
bool g_bhedRankOrder = false;

/* waiting for a portal: the flows waiting for it sleep on its epoch (std::atomic wait). A waiting flow
   misses m bits, so the portal's flow has to find at least m more: it counts the bits it finds (marks)
   and wakes the sleepers when the count reaches the smallest such number (wakeAt), and when it is done.
   (t3_byss: waking on every new bit woke a waiting flow 250 times on average.) */
struct BhedWaitSlot
{
	std::atomic<uint32_t> epoch{ 0 };
	std::atomic<int> marks{ 0 };
	std::atomic<int> wakeAt{ INT_MAX };
};
std::unique_ptr<BhedWaitSlot[]> g_bhedWait;

/* the turns: at most numthreads flows compute at a time. A flow that waits for a portal keeps its
   thread and gives its turn away: to a flow whose wait is over (the one that comes first in the
   sorted order - the later ones may wait for it), else to a new flow on a parked or a new thread.
   A thread that has no turn holds no flow: in vis1 every thread took a flow from NRC's thread loop
   first and then waited for a turn, so most of the threads held flows that could not start anyway
   (t3_byss, 128 threads under the same load: 50 s, now 33 s; NRC's rule 27 to 29 s). */
struct BhedTurnWaiter
{
	int rank;
	bool granted;
	std::condition_variable cv;
};
struct BhedFlowPool
{
	std::mutex mutex;
	std::vector<BhedTurnWaiter*> waiters;   /* flows whose wait is over: heap, smallest rank on top */
	int turnsFree = 0;
	int next = 0, count = 0;                /* the next flow to start */
	int parked = 0, parkedTurns = 0;        /* workers without a flow, turns handed to them */
	int alive = 0, maxAlive = 0;            /* worker threads */
	std::condition_variable parkCv, doneCv;
	std::vector<std::thread> threads;
	void ( *work )( int ) = nullptr;
};
BhedFlowPool g_bhedPool;
thread_local int t_bhedRank = 0;        /* the flow this thread computes */

bool BhedTurnLater( const BhedTurnWaiter *a, const BhedTurnWaiter *b ){
	return a->rank > b->rank;
}

/* a worker thread that went deeper than this ends after its flow and a new one takes over: its stack
   and its frame buffers stay as large as the deepest flow needed them */
constexpr size_t c_bhedDeepFlow = 48;

void BhedPoolWorker();

/* the mutex is held: the turn to the waiting flow that comes first */
bool BhedTurnToWaiter(){
	BhedFlowPool& pool = g_bhedPool;
	if ( pool.waiters.empty() ) {
		return false;
	}
	std::pop_heap( pool.waiters.begin(), pool.waiters.end(), BhedTurnLater );
	BhedTurnWaiter *next = pool.waiters.back();
	pool.waiters.pop_back();
	next->granted = true;
	next->cv.notify_one();
	return true;
}

/* the mutex is held, a turn is free */
void BhedTurnFree(){
	BhedFlowPool& pool = g_bhedPool;
	if ( BhedTurnToWaiter() ) {
		return;
	}
	if ( pool.next < pool.count ) {
		if ( pool.parked > pool.parkedTurns ) {
			++pool.parkedTurns;
			pool.parkCv.notify_one();
			return;
		}
		if ( pool.alive < pool.maxAlive ) {
			++pool.alive;
			pool.threads.emplace_back( BhedPoolWorker );
			return;
		}
	}
	++pool.turnsFree;
}

/* a worker: starts with a turn */
void BhedPoolWorker(){
	BhedFlowPool& pool = g_bhedPool;
	std::unique_lock<std::mutex> lock( pool.mutex );
	while ( true )
	{
		if ( BhedTurnToWaiter() ) {
			/* flows whose wait is over come first: park until there is a turn for a new flow */
			++pool.parked;
			pool.parkCv.wait( lock, [&pool]{ return pool.parkedTurns > 0 || pool.next >= pool.count; } );
			--pool.parked;
			if ( pool.parkedTurns == 0 ) {
				break;
			}
			--pool.parkedTurns;
			continue;
		}
		if ( pool.next >= pool.count ) {
			++pool.turnsFree;
			break;
		}
		const int flow = pool.next++;
		if ( pool.next >= pool.count ) {
			pool.parkCv.notify_all();   /* the last flow is taken: the parked workers end */
		}
		lock.unlock();
		t_bhedRank = flow;
		pool.work( flow );
		lock.lock();
		if ( t_visFrames.bits.size() > c_bhedDeepFlow && pool.next < pool.count ) {
			pool.threads.emplace_back( BhedPoolWorker );    /* takes over the turn */
			return;
		}
	}
	if ( --pool.alive == 0 ) {
		pool.doneCv.notify_all();
	}
}

void BhedPoolRun( int count, int maxThreads, void ( *work )( int ) ){
	BhedFlowPool& pool = g_bhedPool;
	{
		std::unique_lock<std::mutex> lock( pool.mutex );
		pool.next = 0;
		pool.count = count;
		pool.turnsFree = pool.parked = pool.parkedTurns = 0;
		pool.work = work;
		pool.maxAlive = maxThreads;
		pool.alive = numthreads;
		for ( int i = 0; i < numthreads; ++i )
			pool.threads.emplace_back( BhedPoolWorker );
		pool.doneCv.wait( lock, [&pool]{ return pool.alive == 0; } );
	}
	for ( std::thread& t : pool.threads )
		t.join();
	pool.threads.clear();
}
}

static void BhedTurnTake(){
	BhedFlowPool& pool = g_bhedPool;
	std::unique_lock<std::mutex> lock( pool.mutex );
	if ( pool.turnsFree > 0 ) {
		--pool.turnsFree;
		return;
	}
	BhedTurnWaiter me{ t_bhedRank, false, {} };
	pool.waiters.push_back( &me );
	std::push_heap( pool.waiters.begin(), pool.waiters.end(), BhedTurnLater );
	me.cv.wait( lock, [&me]{ return me.granted; } );
}

static void BhedTurnGive(){
	std::lock_guard<std::mutex> lock( g_bhedPool.mutex );
	BhedTurnFree();
}

static EVStatus BhedStatus( const vportal_t *p ){
	return std::atomic_ref<const EVStatus>( p->status ).load( std::memory_order_acquire );
}

static void BhedSetStatus( vportal_t *p, EVStatus status ){
	std::atomic_ref<EVStatus>( p->status ).store( status, std::memory_order_seq_cst );
	if ( status == EVStatus::Done && g_bhedWait ) {
		BhedWaitSlot& slot = g_bhedWait[p - portals];
		slot.epoch.fetch_add( 1, std::memory_order_seq_cst );
		slot.epoch.notify_all();
	}
}

/* BehavEd vis2: set when the VIS is paused (stop file, see BhedVisState below) - the running flows
   end without a result, the finished ones go into the state file */
namespace
{
std::atomic<bool> g_bhedVisStop{ false };
}

static bool BhedStopped(){
	return g_bhedVisStop.load( std::memory_order_relaxed );
}

/* BehavEd vis3: -bhedvisfixed - a fixed rule for several threads: the same bytes in every run and
   with every number of threads, without the chains of waiting flows of -bhedvisexact.
   NRC's rule lets a flow read portalvis of whatever happens to be finished at that moment - the
   result depends on how fast each thread got on. The fixed rule is one run of NRC's rule, on a
   machine that is the same everywhere: a fixed number of chains (its threads) that all work at
   the same pace, one frame of the recursion of a flow in one unit of time.
     - The flows start in the sorted order at the boundaries of a grid of times
       (BhedFixedAdvance): at a boundary each chain that is free takes the next portal - NRC's
       thread loop, with threads that look for work only then. A step of the grid is half as
       long as the flows that ended two steps before (their median).
     - A flow that starts at s and has n frames ends at e = s + n + 1; then its chain is free.
       The frames of a flow depend only on what the flow reads.
     - A flow that starts at s and has done w frames reads portalvis of exactly the flows with
       e <= view( s, w ) and portalflood of all others. The view starts at s and follows the
       clock s + w of the flow at three quarters of its pace (BhedFlowFrame): a long flow gets to
       see what ends while it runs, as with NRC's rule - where a thread is that much slower with
       its portals than with the report of its end.
   Every vector a flow reads is thus final (or portalflood) and the same in every run, so its
   result and its frames are the same in every run, and so are all starts and ends.
   The real threads work this machine off, each one a chain (BhedFixedChain). They keep to one
   thing: nothing is decided before it is certain.
     - A boundary is passed, and the next one open, when every chain with a flow has passed it
       (BhedFixedAdvance): then it is certain which chains were free by it. A chain that gets
       free by the open boundary takes its next flow at once.
     - A flow may take portalflood for a portal without an end only when that end cannot come
       before its view (BhedFixedUncertain): the flow of the portal runs on a chain whose clock
       has reached the view, or it has no chain yet and cannot start before the view (BhedHorizon).
       Else it waits, and gives its turn away meanwhile.
   -threads sets how many chains compute at a time; the result does not depend on it. A chain
   may be ahead of the others by a step of the grid and by what its view lags behind its clock
   before it has to wait, and the chain with the lowest clock never waits.
   docs/intern/NOTIZEN_vis3_1111.md has the measurements and the checks. */
namespace
{
constexpr uint64_t c_bhedNever = ~uint64_t( 0 );
constexpr uint16_t c_bhedNoChain = 0xFFFF;
}

struct alignas( 64 ) BhedChain
{
	/* with a flow: its start + its frames; free: the end of its last flow; c_bhedNever: the chain has ended */
	std::atomic<uint64_t> clock{ 0 };
	/* the flow calls BhedFixedReached when its clock gets here: a boundary or a chain waits for that */
	std::atomic<uint64_t> limit{ c_bhedNever };
	std::vector<uint64_t> seen;             /* the clocks of all chains as this one saw them last: lower bounds */
	uint64_t seenOpen = 0, seenAfter = 0;   /* and the horizon (BhedHorizon) */
	int seenContested = 0;
	/* all below: under the mutex */
	std::condition_variable cv;
	int flow = -1;                          /* its flow (sorted position), -1: free */
	bool ended = false;                     /* no flow left for it */
	BhedChain *waitChain = nullptr;         /* its flow waits until the clock of that chain has reached waitFor ... */
	bool waitHorizon = false;               /* ... or until the horizon has ... */
	uint64_t waitFor = 0;
	int waitPortal = -1;                    /* ... or this portal has its end, or its chain */
	bool wake = false;                      /* the wait is over */
	bool wantsTurn = false, granted = false;
};

namespace
{
int g_bhedFixed = 0;                                    /* chains of the fixed rule, 0: the rule is off */
/* The view goes at 1 - 2^-pace of the pace of the clock, three quarters, and the grid of the starts is 2^-grid
   of the median flow wide, a half. Measured on t3_byss and ctf3 (docs/intern/NOTIZEN_vis3_1111.md): a faster view
   saves frames - the flows see more - and makes them wait more often, a finer grid likewise. Both are part of the
   rule: other values, other bytes. */
constexpr int c_bhedFixedPace = 2;
constexpr int c_bhedFixedGrid = 1;
std::unique_ptr<std::atomic<uint64_t>[]> g_bhedEnd;    /* by portal: the end of its flow, c_bhedNever while it has none */
std::unique_ptr<std::atomic<uint16_t>[]> g_bhedChainOf; /* by portal: the chain of its flow, c_bhedNoChain before it starts */
/* The horizon: what is certain about the flows that have no chain yet. They start at the open boundary - the one
   that not every chain with a flow has passed - or later. At the open boundary only the chains that are still
   behind it can take one more flow each, in the sorted order: a flow at or after the sorted position `contested`
   starts at the boundary after the open one or later. */
struct BhedHorizon
{
	uint64_t open, after;
	int contested;
};
std::atomic<uint32_t> g_bhedHorizonChange{ 0 };         /* odd while the three below are written */
std::atomic<uint64_t> g_bhedHorizonOpen{ 0 }, g_bhedHorizonAfter{ 0 };
std::atomic<int> g_bhedHorizonContested{ 0 };

struct BhedFixedSched
{
	std::mutex mutex;
	std::unique_ptr<BhedChain[]> chains;
	std::vector<int> order;                 /* the flows: the sorted positions of the portals that are not removed */
	size_t next = 0;
	uint64_t open = 0;                      /* the open boundary, c_bhedNever: no flow left */
	uint64_t step = 1;                      /* from the open boundary to the one after it */
	std::vector<std::pair<uint64_t, uint64_t>> ended;   /* end and frames of the flows that ended behind the last boundary that is passed */
	int turnsFree = 0;
	void ( *work )( int ) = nullptr;
	uint64_t waits = 0;
	double waited = 0;                      /* seconds the flows waited for another chain or the horizon */
};
BhedFixedSched g_bhedFixedSched;

/* between a flow and the thread that runs it */
struct BhedFlowClock
{
	uint64_t start = 0;                     /* in: start of the flow (fixed rule) */
	BhedChain *chain = nullptr;             /* in: its chain */
	uint64_t work = 0;                      /* out: frames of the flow */
	bool done = false;                      /* out: the flow has its result */
};
thread_local BhedFlowClock t_bhedFlow;
/* NRC's rule and -bhedvisexact: the flows count their frames all the same */
thread_local std::atomic<uint64_t> t_bhedNoClock{ 0 }, t_bhedNoLimit{ c_bhedNever };

std::atomic<uint64_t> g_bhedFlowFrames{ 0 }, g_bhedFlowLargest{ 0 };
}

static void BhedFixedReached();
static void BhedFixedStop();
static int BhedGeoMemos();
static const uint64_t *BhedFixedUncertain( threaddata_t *thread, const vportal_t *p );

static void BhedFlowBegin( threaddata_t& data ){
	BhedChain *chain = t_bhedFlow.chain;
	data.start = data.view = t_bhedFlow.start;
	data.now = chain != nullptr ? &chain->clock : &t_bhedNoClock;
	data.limit = chain != nullptr ? &chain->limit : &t_bhedNoLimit;
	data.chain = chain;
}

/* a frame of the recursion: the clock of the flow, and its view */
static inline void BhedFlowFrame( threaddata_t *thread ){
	const uint64_t work = ++thread->work;
	const uint64_t now = thread->start + work;
	thread->now->store( now, std::memory_order_relaxed );
	thread->view = now - ( work >> c_bhedFixedPace );
	if ( now >= thread->limit->load( std::memory_order_relaxed ) ) [[unlikely]] {
		BhedFixedReached();
	}
}

static void BhedFlowDone( const threaddata_t& data ){
	t_bhedFlow.work = data.work;
	t_bhedFlow.done = true;
}

/* waits until p is done (false) or its flow has found every bit the caller misses (true);
   missing() is the number of bits still missing */
template<typename Missing>
static bool BhedWaitFound( const vportal_t *p, Missing missing ){
	for ( int i = 0; i < 1000; ++i )
	{
		if ( BhedStatus( p ) == EVStatus::Done || BhedStopped() ) {
			return false;
		}
		_mm_pause();
	}
	BhedTurnGive();
	bool foundAll = false;
	BhedWaitSlot& slot = g_bhedWait[p - portals];
	while ( true )
	{
		const uint32_t epoch = slot.epoch.load( std::memory_order_seq_cst );
		if ( BhedStatus( p ) == EVStatus::Done || g_bhedVisStop.load( std::memory_order_seq_cst ) ) {
			break;
		}
		/* the count first: the bits read after it are at least that many */
		const int marks = slot.marks.load( std::memory_order_seq_cst );
		const int m = missing();
		if ( m == 0 ) {
			foundAll = true;
			break;
		}
		const int target = marks + m;
		int at = slot.wakeAt.load( std::memory_order_seq_cst );
		while ( target < at && !slot.wakeAt.compare_exchange_weak( at, target, std::memory_order_seq_cst ) )
		{
		}
		/* the flow of p counts first and then looks at wakeAt, this one the other way round: one of them sees the other */
		if ( slot.marks.load( std::memory_order_seq_cst ) >= target ) {
			continue;
		}
		slot.epoch.wait( epoch, std::memory_order_seq_cst );
	}
	BhedTurnTake();
	return foundAll;
}

/* the vector a flow from thread->base reads for p (see above); nullptr: p comes before the flow and
   is still running - BhedResolve decides */
static const uint64_t *BhedVector( threaddata_t *thread, const vportal_t *p ){
	if ( g_bhedFixed ) {
		/* BehavEd vis3: the flows that have ended by the view of this flow */
		const int pnum = int( p - portals );
		const uint64_t end = g_bhedEnd[pnum].load( std::memory_order_acquire );
		if ( end <= thread->view ) {
			return reinterpret_cast<const uint64_t*>( p->portalvis );
		}
		if ( end != c_bhedNever ) {
			return reinterpret_cast<const uint64_t*>( p->portalflood );
		}
		/* no end: none can come before the clock of its chain, or before the horizon while it has no chain -
		   as far as this thread has seen them (read before the end above: lower bounds) */
		const uint16_t chain = g_bhedChainOf[pnum].load( std::memory_order_acquire );
		const BhedChain& me = *thread->chain;
		if ( ( chain != c_bhedNoChain ? me.seen[chain] : g_bhedRank[pnum] >= me.seenContested ? me.seenAfter : me.seenOpen ) >= thread->view ) {
			return reinterpret_cast<const uint64_t*>( p->portalflood );
		}
		return BhedFixedUncertain( thread, p );
	}
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
   Otherwise wait for p. forEach( g ) calls g( w, query word w ) for the query. */
template<typename ForEach>
static const uint64_t *BhedResolve( const vportal_t *p, ForEach forEach ){
	const uint64_t *flood = reinterpret_cast<const uint64_t*>( p->portalflood );
	uint64_t *found = reinterpret_cast<uint64_t*>( p->portalvis );
	bool any = false;
	forEach( [&]( int w, uint64_t query ){
		if ( !any && ( query & flood[w] & ~std::atomic_ref<uint64_t>( found[w] ).load( std::memory_order_relaxed ) ) != 0 ) {
			any = true;
		}
	} );
	const auto missing = [&]{
		int m = 0;
		forEach( [&]( int w, uint64_t query ){
			m += std::popcount( query & flood[w] & ~std::atomic_ref<uint64_t>( found[w] ).load( std::memory_order_seq_cst ) );
		} );
		return m;
	};
	if ( !any || BhedWaitFound( p, missing ) ) {
		return flood;
	}
	return found;
}

/* marks a portal visible for the flow of base - other flows may read base->portalvis meanwhile */
static void BhedMark( vportal_t *base, int pnum ){
	uint64_t& word = reinterpret_cast<uint64_t*>( base->portalvis )[pnum >> 6];
	const uint64_t bit = uint64_t( 1 ) << ( pnum & 63 );
	if ( ( std::atomic_ref<uint64_t>( word ).load( std::memory_order_relaxed ) & bit ) == 0 ) {
		std::atomic_ref<uint64_t>( word ).fetch_or( bit, std::memory_order_seq_cst );
		/* a flow may wait for exactly this bit (BhedWaitFound) */
		if ( g_bhedRankOrder ) {
			BhedWaitSlot& slot = g_bhedWait[base - portals];
			if ( slot.marks.fetch_add( 1, std::memory_order_seq_cst ) + 1 >= slot.wakeAt.load( std::memory_order_seq_cst ) ) {
				slot.wakeAt.store( INT_MAX, std::memory_order_seq_cst );
				slot.epoch.fetch_add( 1, std::memory_order_seq_cst );
				slot.epoch.notify_all();
			}
		}
	}
}

void BhedSortedRanks( bool rankOrder, int fixedChains ){
	g_bhedRank.assign( numportals * 2, 0 );
	for ( int i = 0; i < numportals * 2; ++i )
		g_bhedRank[sorted_portals[i] - portals] = i;
	g_bhedRankOrder = rankOrder;
	g_bhedWait = std::make_unique<BhedWaitSlot[]>( numportals * 2 );
	g_bhedFixed = fixedChains;
	g_bhedEnd = std::make_unique<std::atomic<uint64_t>[]>( numportals * 2 );
	for ( int i = 0; i < numportals * 2; ++i )
		g_bhedEnd[i].store( c_bhedNever, std::memory_order_relaxed );
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

/* BehavEd vis2: VIS state - pause and resume (-bhedvisstate, vis.cpp).
   Every few minutes (and when paused) the portalvis vectors of all finished flows go into
   <map>.bhedvisstate, together with the portal file and the switches. A stop file <map>.bhedvisstop
   pauses the VIS: no new flows, the running ones end without a result, the state is written and
   -vis ends without writing the .bsp. The next -vis -bhedvisstate with the same portal file and
   switches takes the finished flows from the state and computes only the others.
   With -bhedvisexact byte for byte the result of a VIS without pause: the flows run in the order
   of one thread (g_bhedRankOrder), a finished flow is final whatever runs beside it, and the others
   read exactly what they read without the pause (portalvis of the flows before them in the sorted
   order - restored or computed - and portalflood of all after them).
   With NRC's rule (several threads without -bhedvisexact) no two runs give the same bytes anyway;
   the restored flows are finished flows of that rule, and the rest is computed by it. The switch
   is part of the key: a state of one rule is not taken by the other. */
namespace
{
struct BhedVisState
{
	bool on = false;
	std::string file, stop, key, prt;
	double every = 120;
	int stopAfter = 0;              /* BHED_VIS_STOPAFTER (ctest q3map2_vispause): pause after that many flows */
	std::atomic<int> computed{ 0 };
	int resumed = 0;
	bool stopped = false;
	std::vector<char> restored;     /* by sorted position */
};
BhedVisState g_bhedState;

constexpr char c_bhedStateMagic[] = "BHVST2";

bool BhedFileExists( const char *name ){
	if ( FILE *f = fopen( name, "rb" ) ) {
		fclose( f );
		return true;
	}
	return false;
}

/* the finished flows (status Done after the acquire load: their portalvis is complete), written straight
   into a new file beside the old one that then replaces it: a crash while writing keeps the old state */
int BhedStateWrite(){
	const std::string tmp = g_bhedState.file + ".tmp";
	FILE *f = fopen( tmp.c_str(), "wb" );
	if ( !f ) {
		return -1;
	}
	bool ok = fwrite( c_bhedStateMagic, 1, 6, f ) == 6;
	for ( const std::string *block : { &g_bhedState.key, &g_bhedState.prt } )
	{
		const uint32_t len = uint32_t( block->size() );
		ok = ok && fwrite( &len, 4, 1, f ) == 1 && fwrite( block->data(), 1, block->size(), f ) == block->size();
	}
	const uint32_t words = uint32_t( portalwords );
	uint32_t count = 0;
	/* BehavEd vis3, fixed rule: a flow is finished when it has its end, and the end goes into the state -
	   any part of the finished flows will do, the others take their place in the schedule again */
	std::vector<uint64_t> ends( numportals * 2, 0 );
	for ( int i = 0; g_bhedFixed && i < numportals * 2; ++i )
		ends[i] = g_bhedEnd[i].load( std::memory_order_acquire );
	ok = ok && fwrite( &words, 4, 1, f ) == 1;
	const long countAt = ftell( f );
	ok = ok && fwrite( &count, 4, 1, f ) == 1;
	std::vector<uint64_t> vis( portalwords );
	for ( int i = 0; ok && i < numportals * 2; ++i )
	{
		const vportal_t& p = portals[i];
		if ( p.removed || ( g_bhedFixed ? ends[i] == c_bhedNever : BhedStatus( &p ) != EVStatus::Done ) ) {
			continue;
		}
		const uint32_t index = uint32_t( i );
		for ( int w = 0; w < portalwords; ++w )
			vis[w] = std::atomic_ref<uint64_t>( reinterpret_cast<uint64_t*>( p.portalvis )[w] ).load( std::memory_order_relaxed );
		ok = fwrite( &index, 4, 1, f ) == 1 && fwrite( &ends[i], 8, 1, f ) == 1 && fwrite( vis.data(), 8, vis.size(), f ) == vis.size();
		++count;
	}
	ok = ok && fseek( f, countAt, SEEK_SET ) == 0 && fwrite( &count, 4, 1, f ) == 1;
	std::error_code ec;
	if ( fclose( f ) != 0 || !ok ) {
		remove( tmp.c_str() );
		return -1;
	}
	std::filesystem::rename( tmp, g_bhedState.file, ec );
	if ( ec ) {
		remove( tmp.c_str() );
		return -1;
	}
	return int( count );
}

/* restores the finished flows of a matching state; the reason if there is none */
const char *BhedStateRead(){
	std::string d;
	if ( FILE *f = fopen( g_bhedState.file.c_str(), "rb" ) ) {
		char buf[65536];
		size_t n;
		while ( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
			d.append( buf, n );
		fclose( f );
	}
	else{
		return "no state";
	}
	size_t pos = 6;
	const auto block = [&]( std::string& out ){
		uint32_t n = 0;
		if ( pos + 4 > d.size() ) {
			return false;
		}
		memcpy( &n, d.data() + pos, 4 );
		pos += 4;
		if ( n > d.size() - pos ) {
			return false;
		}
		out.assign( d.data() + pos, n );
		pos += n;
		return true;
	};
	std::string key, prt;
	if ( d.size() >= 6 && d.compare( 0, 5, c_bhedStateMagic, 5 ) == 0 && d[5] != c_bhedStateMagic[5] ) {
		return "state of another version";
	}
	if ( d.size() < 6 || d.compare( 0, 6, c_bhedStateMagic ) != 0 || !block( key ) || !block( prt ) || pos + 8 > d.size() ) {
		return "state damaged";
	}
	/* BehavEd lcfix (P4-3): the key begins with the version of bhed_q3map2 up to the first line end (a state of rc611 has
	   none: its whole key is that part) */
	if ( key.substr( 0, key.find( '\n' ) ) != g_bhedState.key.substr( 0, g_bhedState.key.find( '\n' ) ) ) {
		return "state of another version";
	}
	if ( key != g_bhedState.key ) {
		return "other vis switches";
	}
	if ( prt != g_bhedState.prt ) {
		return "other portals";
	}
	uint32_t words, count;
	memcpy( &words, d.data() + pos, 4 );
	memcpy( &count, d.data() + pos + 4, 4 );
	pos += 8;
	const size_t entry = 12 + size_t( portalwords ) * 8;
	if ( words != uint32_t( portalwords ) || count > uint32_t( numportals * 2 ) || d.size() - pos != count * entry ) {
		return "state damaged";
	}
	/* check all entries before taking one */
	std::vector<uint64_t> ends( numportals * 2, c_bhedNever );     /* by sorted position */
	for ( uint32_t k = 0; k < count; ++k )
	{
		uint32_t index;
		memcpy( &index, d.data() + pos + k * entry, 4 );
		if ( index >= uint32_t( numportals * 2 ) || portals[index].removed || portals[index].portalvis == nullptr ) {
			return "state damaged";
		}
		memcpy( &ends[g_bhedRank[index]], d.data() + pos + k * entry + 4, 8 );
		if ( g_bhedFixed && ends[g_bhedRank[index]] == c_bhedNever ) {
			return "state damaged";
		}
	}
	for ( uint32_t k = 0; k < count; ++k )
	{
		uint32_t index;
		memcpy( &index, d.data() + pos + k * entry, 4 );
		vportal_t& p = portals[index];
		memcpy( p.portalvis, d.data() + pos + k * entry + 12, size_t( portalwords ) * 8 );
		p.status = EVStatus::Done;
		g_bhedState.restored[g_bhedRank[index]] = 1;
		if ( g_bhedFixed ) {
			g_bhedEnd[index].store( ends[g_bhedRank[index]], std::memory_order_relaxed );
		}
	}
	g_bhedState.resumed = int( count );
	return nullptr;
}
}

void BhedVisStateSetup( const char *file, const char *stop, const std::string& key, const std::string& prt, double every ){
	g_bhedState.on = true;
	g_bhedState.file = file;
	g_bhedState.stop = stop;
	g_bhedState.key = key;
	g_bhedState.prt = prt;
	g_bhedState.every = every;
	if ( const char *after = getenv( "BHED_VIS_STOPAFTER" ) ) {
		g_bhedState.stopAfter = atoi( after );
	}
}

bool BhedVisPaused(){
	return g_bhedState.stopped;
}

void BhedVisStateDone(){
	if ( g_bhedState.on && !g_bhedState.stopped ) {
		remove( g_bhedState.file.c_str() );
		remove( g_bhedState.stop.c_str() );     /* a stop file that came after the last flow */
	}
}

/* pauses the VIS: no new flows, the running and the waiting ones end without a result */
static void BhedStopFlows(){
	g_bhedVisStop.store( true, std::memory_order_seq_cst );
	if ( g_bhedFixed ) {
		BhedFixedStop();
	}
	for ( int i = 0; i < numportals * 2; ++i )
	{
		g_bhedWait[i].epoch.fetch_add( 1, std::memory_order_seq_cst );
		g_bhedWait[i].epoch.notify_all();
	}
}

/* BehavEd vis2s: the geometry memo of the flows (BhedGeoMemo below) - its size and what it did */
namespace
{
std::atomic<uint64_t> g_bhedGeoKnown{ 0 }, g_bhedGeoMade{ 0 }, g_bhedGeoAgain{ 0 }, g_bhedGeoPlanes{ 0 }, g_bhedGeoEmptied{ 0 }, g_bhedGeoRefused{ 0 };

int BhedGeoBits(){
	static const int bits = []{
		const char *value = getenv( "BHED_VIS_GEOMEMO" );
		return value != nullptr ? std::clamp( atoi( value ), 0, 20 ) : 14;
	}();
	return bits;
}

/* the last id a memo gives out before it has to start again empty (BhedGeoMemo::begin);
   BHED_VIS_GEOMEMO_IDS: a small number, for the ctest */
uint32_t BhedGeoLastId(){
	static const uint32_t last = []{
		const char *value = getenv( "BHED_VIS_GEOMEMO_IDS" );
		return value != nullptr ? uint32_t( std::clamp( atoi( value ), 2, INT_MAX ) ) : UINT32_MAX;
	}();
	return last;
}
}

/* BehavEd vis3: the chains of the fixed rule (see g_bhedFixed above) */
namespace
{
/* the mutex is held: the flows that wait for this chain and need not any more */
void BhedFixedWakeup( BhedChain& its ){
	const uint64_t clock = its.clock.load( std::memory_order_seq_cst );
	for ( int i = 0; i < g_bhedFixed; ++i )
	{
		BhedChain& chain = g_bhedFixedSched.chains[i];
		if ( chain.waitChain == &its && !chain.wake
		  && ( clock >= chain.waitFor || g_bhedEnd[chain.waitPortal].load( std::memory_order_relaxed ) != c_bhedNever ) ) {
			chain.wake = true;
			chain.cv.notify_one();
		}
	}
}

/* the mutex is held, a turn is free: to the chain with the lowest clock that wants one */
void BhedFixedTurnFree(){
	BhedFixedSched& sched = g_bhedFixedSched;
	BhedChain *first = nullptr;
	for ( int i = 0; i < g_bhedFixed; ++i )
	{
		BhedChain& chain = sched.chains[i];
		if ( chain.wantsTurn && !chain.granted
		  && ( first == nullptr || chain.clock.load( std::memory_order_relaxed ) < first->clock.load( std::memory_order_relaxed ) ) ) {
			first = &chain;
		}
	}
	if ( first != nullptr ) {
		first->granted = true;
		first->cv.notify_one();
	}
	else{
		++sched.turnsFree;
	}
}

void BhedFixedTurnTake( BhedChain& me, std::unique_lock<std::mutex>& lock ){
	BhedFixedSched& sched = g_bhedFixedSched;
	if ( sched.turnsFree > 0 ) {
		--sched.turnsFree;
		return;
	}
	me.granted = false;
	me.wantsTurn = true;
	me.cv.wait( lock, [&me]{ return me.granted; } );
	me.wantsTurn = false;
}

/* the horizon as one piece (the three values belong to one open boundary) */
BhedHorizon BhedHorizonGet(){
	while ( true )
	{
		const uint32_t change = g_bhedHorizonChange.load( std::memory_order_seq_cst );
		const BhedHorizon horizon{ g_bhedHorizonOpen.load( std::memory_order_seq_cst ), g_bhedHorizonAfter.load( std::memory_order_seq_cst ),
		                           g_bhedHorizonContested.load( std::memory_order_seq_cst ) };
		if ( ( change & 1 ) == 0 && g_bhedHorizonChange.load( std::memory_order_seq_cst ) == change ) {
			return horizon;
		}
		_mm_pause();
	}
}

/* the mutex is held; true: it is another one than before */
bool BhedHorizonSet( uint64_t open, uint64_t after, int contested ){
	if ( g_bhedHorizonOpen.load( std::memory_order_relaxed ) == open && g_bhedHorizonAfter.load( std::memory_order_relaxed ) == after
	  && g_bhedHorizonContested.load( std::memory_order_relaxed ) == contested ) {
		return false;
	}
	g_bhedHorizonChange.fetch_add( 1, std::memory_order_seq_cst );
	g_bhedHorizonOpen.store( open, std::memory_order_seq_cst );
	g_bhedHorizonAfter.store( after, std::memory_order_seq_cst );
	g_bhedHorizonContested.store( contested, std::memory_order_seq_cst );
	g_bhedHorizonChange.fetch_add( 1, std::memory_order_seq_cst );
	return true;
}

/* what the horizon tells about the end of the portal as long as it has no chain */
uint64_t BhedHorizonBound( const BhedHorizon& horizon, int pnum ){
	return g_bhedRank[pnum] >= horizon.contested ? horizon.after : horizon.open;
}

/* the mutex is held: hands out flows and moves the open boundary on as far as that is certain.
   The chains that are free by the open boundary take the next flows of the sorted order at once: they start
   there whatever the other chains do (which chain takes which flow does not matter, a flow is its place in
   the order and its start). The boundary is passed when every chain with a flow has passed it - then no
   chain can get free by it any more, and the one after it is the open one. */
void BhedFixedAdvance(){
	BhedFixedSched& sched = g_bhedFixedSched;
	if ( BhedStopped() ) {
		return;     /* paused: no flow starts any more */
	}
	bool moved = false;
	while ( sched.open != c_bhedNever )
	{
		const uint64_t boundary = sched.open;
		for ( int i = 0; i < g_bhedFixed && sched.next < sched.order.size(); ++i )
		{
			BhedChain& chain = sched.chains[i];
			if ( chain.flow < 0 && !chain.ended && chain.clock.load( std::memory_order_relaxed ) <= boundary ) {
				chain.flow = sched.order[sched.next++];
				/* first the clock, then the chain of the portal: who finds the chain finds the clock of this flow */
				chain.clock.store( boundary, std::memory_order_seq_cst );
				g_bhedChainOf[sorted_portals[chain.flow] - portals].store( uint16_t( i ), std::memory_order_seq_cst );
				chain.cv.notify_one();
				moved = true;
			}
		}
		if ( sched.next == sched.order.size() ) {
			sched.open = c_bhedNever;
			moved |= BhedHorizonSet( c_bhedNever, c_bhedNever, INT_MAX );
			break;
		}
		int behind = 0;
		for ( int i = 0; i < g_bhedFixed; ++i )
		{
			BhedChain& chain = sched.chains[i];
			if ( chain.flow >= 0 && chain.clock.load( std::memory_order_seq_cst ) < boundary ) {
				++behind;
				/* its flow tells when it gets there (BhedFlowFrame) */
				if ( chain.limit.load( std::memory_order_relaxed ) > boundary ) {
					chain.limit.store( boundary, std::memory_order_seq_cst );
				}
			}
		}
		if ( behind > 0 ) {
			moved |= BhedHorizonSet( boundary, boundary + sched.step,
			                         sched.next + behind < sched.order.size() ? sched.order[sched.next + behind] : INT_MAX );
			break;
		}
		/* passed. The grid goes on with the step that was fixed when this boundary became the open one; the
		   step behind the next boundary is half the median length of the flows that ended in the step before
		   this boundary (all of them have ended now) - where none ended the grid gets wider. */
		std::vector<uint64_t> frames;
		for ( size_t k = 0; k < sched.ended.size(); )
		{
			if ( sched.ended[k].first <= boundary ) {
				frames.push_back( sched.ended[k].second );
				sched.ended[k] = sched.ended.back();
				sched.ended.pop_back();
			}
			else{
				++k;
			}
		}
		sched.open = boundary + sched.step;
		if ( frames.empty() ) {
			sched.step += std::max<uint64_t>( sched.step >> 3, 1 );
		}
		else{
			std::nth_element( frames.begin(), frames.begin() + ( frames.size() - 1 ) / 2, frames.end() );
			sched.step = std::max<uint64_t>( frames[( frames.size() - 1 ) / 2] >> c_bhedFixedGrid, 1 );
		}
	}
	if ( sched.open == c_bhedNever ) {
		/* no flow left: the free chains end */
		for ( int i = 0; i < g_bhedFixed; ++i )
		{
			BhedChain& chain = sched.chains[i];
			if ( chain.flow < 0 && !chain.ended ) {
				chain.ended = true;
				chain.clock.store( c_bhedNever, std::memory_order_seq_cst );
				chain.cv.notify_one();
			}
		}
	}
	if ( moved ) {
		/* the flows that wait for the horizon look again */
		for ( int i = 0; i < g_bhedFixed; ++i )
		{
			BhedChain& chain = sched.chains[i];
			if ( chain.waitHorizon && !chain.wake ) {
				chain.wake = true;
				chain.cv.notify_one();
			}
		}
	}
}

/* the flow of this thread waits until the clock of the chain (nullptr: the horizon) has reached `until`
   or the portal has got its end (its chain), and gives its turn away meanwhile */
void BhedFixedWait( BhedChain *its, uint64_t until, int portal ){
	BhedFixedSched& sched = g_bhedFixedSched;
	BhedChain& me = *t_bhedFlow.chain;
	std::unique_lock<std::mutex> lock( sched.mutex );
	/* ends, chains of portals and the horizon change under the mutex; a clock goes on without it - the
	   flow of that chain looks at the limit in every frame (BhedFlowFrame) */
	bool wait;
	if ( its != nullptr ) {
		if ( its->limit.load( std::memory_order_relaxed ) > until ) {
			its->limit.store( until, std::memory_order_seq_cst );
		}
		wait = g_bhedEnd[portal].load( std::memory_order_relaxed ) == c_bhedNever && its->clock.load( std::memory_order_seq_cst ) < until;
	}
	else{
		wait = g_bhedChainOf[portal].load( std::memory_order_relaxed ) == c_bhedNoChain && BhedHorizonBound( BhedHorizonGet(), portal ) < until;
	}
	if ( wait && !BhedStopped() ) {
		me.waitChain = its;
		me.waitHorizon = its == nullptr;
		me.waitFor = until;
		me.waitPortal = portal;
		me.wake = false;
		++sched.waits;
		BhedFixedTurnFree();
		Timer timer;
		me.cv.wait( lock, [&me]{ return me.wake; } );
		sched.waited += timer.elapsed_sec();
		me.waitChain = nullptr;
		me.waitHorizon = false;
		BhedFixedTurnTake( me, lock );
	}
}

/* one chain: the thread runs the flows that are handed to it */
void BhedFixedChain( int index, const std::vector<char> *restored ){
	BhedFixedSched& sched = g_bhedFixedSched;
	BhedChain& me = sched.chains[index];
	std::unique_lock<std::mutex> lock( sched.mutex );
	bool turn = false;
	while ( true )
	{
		if ( turn && me.flow < 0 ) {
			BhedFixedTurnFree();    /* nothing to run at the moment */
			turn = false;
		}
		me.cv.wait( lock, [&me]{ return me.flow >= 0 || me.ended || BhedStopped(); } );
		if ( me.flow < 0 || BhedStopped() ) {
			break;
		}
		const int flow = me.flow;
		const int pnum = int( sorted_portals[flow] - portals );
		const uint64_t start = me.clock.load( std::memory_order_relaxed );
		uint64_t end;
		if ( ( *restored )[flow] ) {
			/* resumed: the flow has its vector and its end from the state - it takes its place in the schedule again */
			end = g_bhedEnd[pnum].load( std::memory_order_relaxed );
			if ( end <= start ) {
				Error( "BehavEd: the vis state does not fit the fixed rule (portal %i)", pnum );
			}
		}
		else{
			if ( !turn ) {
				BhedFixedTurnTake( me, lock );
				turn = true;
			}
			lock.unlock();
			t_bhedFlow.start = start;
			t_bhedFlow.chain = &me;
			sched.work( flow );
			lock.lock();
			if ( !t_bhedFlow.done ) {
				break;      /* paused */
			}
			/* the vector is complete: first its end, then the clock of the chain - a flow that reads the clock
			   and then finds no end (BhedFixedUncertain) is right that the end comes after that clock */
			end = start + t_bhedFlow.work + 1;
			g_bhedEnd[pnum].store( end, std::memory_order_seq_cst );
		}
		me.clock.store( end, std::memory_order_seq_cst );
		me.limit.store( c_bhedNever, std::memory_order_seq_cst );
		me.flow = -1;
		sched.ended.emplace_back( end, end - start - 1 );
		BhedFixedWakeup( me );
		BhedFixedAdvance();
	}
	if ( turn ) {
		BhedFixedTurnFree();
	}
	/* paused: all flows end without a result */
	me.ended = true;
	me.flow = -1;
	me.clock.store( c_bhedNever, std::memory_order_seq_cst );
}

/* before the flows (and before the thread that writes the state) */
void BhedFixedSetup(){
	BhedFixedSched& sched = g_bhedFixedSched;
	sched.chains = std::make_unique<BhedChain[]>( g_bhedFixed );
	for ( int i = 0; i < g_bhedFixed; ++i )
	{
		/* a flow never waits for its own chain */
		sched.chains[i].seen.assign( g_bhedFixed, 0 );
		sched.chains[i].seen[i] = c_bhedNever;
	}
	sched.order.clear();
	for ( int flow = 0; flow < numportals * 2; ++flow )
		if ( !sorted_portals[flow]->removed ) {
			sched.order.push_back( flow );
		}
	sched.next = 0;
	sched.open = 0;
	sched.step = 1;
	sched.ended.clear();
	sched.turnsFree = std::clamp( numthreads, 1, g_bhedFixed );
	sched.waits = 0;
	sched.waited = 0;
	g_bhedChainOf = std::make_unique<std::atomic<uint16_t>[]>( numportals * 2 );
	for ( int i = 0; i < numportals * 2; ++i )
		g_bhedChainOf[i].store( c_bhedNoChain, std::memory_order_relaxed );
	BhedHorizonSet( 0, 1, 0 );
}

void BhedFixedRun( void ( *work )( int ), const std::vector<char>& restored ){
	BhedFixedSched& sched = g_bhedFixedSched;
	sched.work = work;
	for ( int flow = 0; flow < numportals * 2; ++flow )
		if ( sorted_portals[flow]->removed ) {
			work( flow );       /* no flow: it takes no time and no chain */
		}
	{
		std::lock_guard<std::mutex> lock( sched.mutex );
		BhedFixedAdvance();
	}
	BhedThreaded( true );
	std::vector<std::thread> pool;
	for ( int i = 0; i < g_bhedFixed; ++i )
		pool.emplace_back( BhedFixedChain, i, &restored );
	for ( std::thread& t : pool )
		t.join();
	BhedThreaded( false );
}
}

/* the flow of this thread has come to the limit of its chain: a boundary or a flow waits for that */
static void BhedFixedReached(){
	BhedChain& me = *t_bhedFlow.chain;
	std::lock_guard<std::mutex> lock( g_bhedFixedSched.mutex );
	me.limit.store( c_bhedNever, std::memory_order_seq_cst );
	BhedFixedWakeup( me );
	BhedFixedAdvance();     /* sets the limit again when the chain is behind the next boundary */
	/* and for the flows that still wait for this chain */
	const uint64_t now = me.clock.load( std::memory_order_relaxed );
	uint64_t limit = me.limit.load( std::memory_order_relaxed );
	for ( int i = 0; i < g_bhedFixed; ++i )
	{
		const BhedChain& chain = g_bhedFixedSched.chains[i];
		if ( chain.waitChain == &me && !chain.wake && chain.waitFor > now ) {
			limit = std::min( limit, chain.waitFor );
		}
	}
	me.limit.store( limit, std::memory_order_seq_cst );
}

/* the portal has no end, and what this flow knows does not tell that none can come before its view.
   It cannot: when the flow of the portal runs on a chain whose clock has reached the view, or when the
   portal has no chain yet and the horizon has reached the view - its flow starts at a boundary that is
   not handed out yet. Until then the flow waits; when the portal gets its end meanwhile, the end decides.
   The order of reading matters: the horizon before the chain of the portal, the clock of the chain
   before the end. */
static const uint64_t *BhedFixedUncertain( threaddata_t *thread, const vportal_t *p ){
	const int pnum = int( p - portals );
	BhedChain& me = *thread->chain;
	const uint64_t *flood = reinterpret_cast<const uint64_t*>( p->portalflood );
	while ( true )
	{
		const BhedHorizon horizon = BhedHorizonGet();
		const uint16_t chain = g_bhedChainOf[pnum].load( std::memory_order_seq_cst );
		BhedChain *its = chain == c_bhedNoChain ? nullptr : &g_bhedFixedSched.chains[chain];
		const uint64_t bound = its != nullptr ? its->clock.load( std::memory_order_seq_cst ) : BhedHorizonBound( horizon, pnum );
		const uint64_t end = g_bhedEnd[pnum].load( std::memory_order_seq_cst );
		if ( end != c_bhedNever ) {
			return end <= thread->view ? reinterpret_cast<const uint64_t*>( p->portalvis ) : flood;
		}
		me.seenOpen = horizon.open;
		me.seenAfter = horizon.after;
		me.seenContested = horizon.contested;
		if ( its != nullptr ) {
			me.seen[chain] = bound;
		}
		if ( bound >= thread->view || BhedStopped() ) {     /* paused: the flow has no result */
			return flood;
		}
		BhedFixedWait( its, thread->view, pnum );
	}
}

/* paused: the chains that wait or sleep end */
static void BhedFixedStop(){
	BhedFixedSched& sched = g_bhedFixedSched;
	if ( !sched.chains ) {
		return;
	}
	std::lock_guard<std::mutex> lock( sched.mutex );
	for ( int i = 0; i < g_bhedFixed; ++i )
	{
		BhedChain& chain = sched.chains[i];
		chain.wake = true;
		chain.granted = true;
		chain.cv.notify_all();
	}
}

void BhedRunFlow( void ( *flow )( int ) ){
	static void ( *s_flow )( int );
	s_flow = flow;
	g_bhedWorkTotal = 0;
	for ( const vportal_t *p : Span( sorted_portals, numportals * 2 ) )
		g_bhedWorkTotal += BhedFlowWork( p );
	g_bhedWorkDone = 0;
	g_bhedState.restored.assign( numportals * 2, 0 );
	if ( g_bhedState.on ) {
		if ( const char *why = BhedStateRead() ) {
			Sys_Printf( "BehavEd: vis state not used (%s): %s\n", why, g_bhedState.file.c_str() );
		}
		else{
			uint64_t work = 0;
			for ( int i = 0; i < numportals * 2; ++i )
				if ( g_bhedState.restored[i] ) {
					work += BhedFlowWork( sorted_portals[i] );
				}
			g_bhedWorkDone = work;
			Sys_Printf( "BehavEd: vis resumed from %s: %i of %i portals done (%.1f %% of the work)\n", g_bhedState.file.c_str(), g_bhedState.resumed,
			            numportals * 2, 100.0 * double( work ) / double( std::max<uint64_t>( g_bhedWorkTotal, 1 ) ) );
		}
	}
	if ( g_bhedFixed ) {
		BhedFixedSetup();
	}
	g_bhedPacifierShown = -1;
	BhedPacifier( g_bhedWorkDone );
	Timer timer;

	/* a stop file that is already there (the pause came while the portals were loaded): no flow at all */
	if ( g_bhedState.on && BhedFileExists( g_bhedState.stop.c_str() ) ) {
		BhedStopFlows();
	}

	/* the state: a thread that writes it every few minutes and watches for the stop file */
	std::mutex writerMutex;
	std::condition_variable writerCv;
	bool flowDone = false;
	std::thread writer;
	if ( g_bhedState.on ) {
		writer = std::thread( [&]{
			Timer since;
			double pause = g_bhedState.every;
			std::unique_lock<std::mutex> lock( writerMutex );
			while ( !flowDone )
			{
				writerCv.wait_for( lock, std::chrono::milliseconds( std::clamp( int( g_bhedState.every * 500 ), 10, 250 ) ) );
				if ( flowDone ) {
					break;
				}
				if ( BhedFileExists( g_bhedState.stop.c_str() ) ) {
					BhedStopFlows();
					break;
				}
				if ( since.elapsed_sec() >= pause ) {
					/* a large state takes a while to write (md_rev_TarisUpper2: 152168 vectors of 9.5 KB): at most 5 % of the time */
					Timer writing;
					BhedStateWrite();
					pause = std::max( g_bhedState.every, 20 * writing.elapsed_sec() );
					since.start();
				}
			}
		} );
	}

	g_bhedFlowFrames = 0;
	g_bhedFlowLargest = 0;
	const auto one = []( int portalnum ){
		t_bhedFlow.done = false;
		s_flow( portalnum );
		if ( t_bhedFlow.done ) {
			g_bhedFlowFrames.fetch_add( t_bhedFlow.work, std::memory_order_relaxed );
			uint64_t largest = g_bhedFlowLargest.load( std::memory_order_relaxed );
			while ( t_bhedFlow.work > largest && !g_bhedFlowLargest.compare_exchange_weak( largest, t_bhedFlow.work, std::memory_order_relaxed ) )
			{
			}
		}
		if ( !g_bhedState.restored[portalnum] && !BhedStopped() ) {    /* paused: the flow has no result */
			BhedPacifier( g_bhedWorkDone += BhedFlowWork( sorted_portals[portalnum] ) );
			if ( g_bhedState.stopAfter > 0 && ++g_bhedState.computed == g_bhedState.stopAfter ) {
				BhedStopFlows();
			}
		}
	};
	if ( g_bhedFixed ) {
		BhedFixedRun( one, g_bhedState.restored );
	}
	else if ( !g_bhedRankOrder || numthreads <= 1 ) {
		RunThreadsOnIndividual( numportals * 2, false, one );
	}
	else{
		BhedThreaded( true );
		/* t3_byss, 32 turns: 128 and 512 threads 33 s (2048: 41 s), with 250, 520 and 1320 MB at the peak */
		BhedPoolRun( numportals * 2, std::max( numthreads * 4, 64 ), one );
		BhedThreaded( false ); /* mb2_manaan, 32 threads: 2 / 4 / 8 / 16 times as many threads 21 / 19 / 19 / 30 s */
	}
	if ( writer.joinable() ) {
		{
			std::lock_guard<std::mutex> lock( writerMutex );
			flowDone = true;
		}
		writerCv.notify_all();
		writer.join();
	}
	if ( BhedStopped() ) {
		g_bhedState.stopped = true;
		const int n = BhedStateWrite();
		Sys_Printf( "\nBehavEd: vis paused, %i of %i portals done, state saved in %s\n", n, numportals * 2, g_bhedState.file.c_str() );
		remove( g_bhedState.stop.c_str() );
		return;
	}
	Sys_Printf( " (%i)\n", int( timer.elapsed_sec() ) );
	if ( g_bhedFixed ) {
		uint64_t last = 0;
		for ( int i = 0; i < numportals * 2; ++i )
			if ( !portals[i].removed ) {
				last = std::max( last, g_bhedEnd[i].load( std::memory_order_relaxed ) );
			}
		Sys_FPrintf( SYS_VRB, "BehavEd: fixed vis rule: %i chains, %i at a time, the last flow ends at %llu, %llu waits for a chain or the horizon (%.2f s), %i geometry memos\n",
		             g_bhedFixed, std::clamp( numthreads, 1, g_bhedFixed ), (unsigned long long)last, (unsigned long long)g_bhedFixedSched.waits,
		             g_bhedFixedSched.waited, BhedGeoMemos() );
	}
	/* BehavEd vis3: the work of the flows in frames - the same in every run with one thread, -bhedvisexact and -bhedvisfixed */
	Sys_FPrintf( SYS_VRB, "BehavEd: flows: %llu frames, the largest %llu\n", (unsigned long long)g_bhedFlowFrames.load(), (unsigned long long)g_bhedFlowLargest.load() );
	if ( BhedGeoBits() > 0 && g_bhedGeoKnown + g_bhedGeoMade != 0 ) {
		Sys_FPrintf( SYS_VRB, "BehavEd: geometry memo (2^%i states a thread): %llu portals known, %llu computed, %llu windings made again, %llu planes made again, emptied %llu times, %llu states without id\n",
		             BhedGeoBits(), (unsigned long long)g_bhedGeoKnown.load(), (unsigned long long)g_bhedGeoMade.load(),
		             (unsigned long long)g_bhedGeoAgain.load(), (unsigned long long)g_bhedGeoPlanes.load(),
		             (unsigned long long)g_bhedGeoEmptied.load(), (unsigned long long)g_bhedGeoRefused.load() );
	}
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
	/* BehavEd vis2s: four points at a time. The same numbers as NRC's loop below: the products and sums of
	   plane3_distance_to_point in single precision and in its order, the distance of the plane taken off in
	   double precision, the result rounded to float (dists[i]). A float is above the double ON_EPSILON
	   exactly if it is at least float( ON_EPSILON ) - 0.1f is the smallest float above 0.1 - and below
	   -ON_EPSILON exactly if it is at most -0.1f. The last four points may overlap the four before (the
	   same numbers once more); the sides are written out only when the winding is really cut.
	   (md_ch_ben: 487 million calls with four points on average, two of three end right after the distances.) */
	static_assert( sizeof( Vector3 ) == 3 * sizeof( float ) );
	static_assert( MAX_POINTS_ON_FIXED_WINDING <= 32 );
	unsigned frontbits = 0, backbits = 0;
	const bool four = in->numpoints >= 4;
	i = 0;
	if ( four ) {
		const __m128 nx = _mm_set1_ps( split.normal()[0] ), ny = _mm_set1_ps( split.normal()[1] ), nz = _mm_set1_ps( split.normal()[2] );
		const __m128d dist = _mm_set1_pd( split.dist() );
		const __m128 front = _mm_set1_ps( float( ON_EPSILON ) ), back = _mm_set1_ps( -float( ON_EPSILON ) );
		while ( true )
		{
			const float *f = &in->points[i][0];
			const __m128 a = _mm_loadu_ps( f );                                     // x0 y0 z0 x1
			const __m128 b = _mm_loadu_ps( f + 4 );                                 // y1 z1 x2 y2
			const __m128 c = _mm_loadu_ps( f + 8 );                                 // z2 x3 y3 z3
			const __m128 xy23 = _mm_shuffle_ps( b, c, _MM_SHUFFLE( 2, 1, 3, 2 ) );  // x2 y2 x3 y3
			const __m128 yz01 = _mm_shuffle_ps( a, b, _MM_SHUFFLE( 1, 0, 2, 1 ) );  // y0 z0 y1 z1
			const __m128 x = _mm_shuffle_ps( a, xy23, _MM_SHUFFLE( 2, 0, 3, 0 ) );
			const __m128 y = _mm_shuffle_ps( yz01, xy23, _MM_SHUFFLE( 3, 1, 2, 0 ) );
			const __m128 z = _mm_shuffle_ps( yz01, c, _MM_SHUFFLE( 3, 0, 3, 1 ) );
			const __m128 sum = _mm_add_ps( _mm_add_ps( _mm_mul_ps( x, nx ), _mm_mul_ps( y, ny ) ), _mm_mul_ps( z, nz ) );
			const __m128 lo = _mm_cvtpd_ps( _mm_sub_pd( _mm_cvtps_pd( sum ), dist ) );
			const __m128 hi = _mm_cvtpd_ps( _mm_sub_pd( _mm_cvtps_pd( _mm_movehl_ps( sum, sum ) ), dist ) );
			const __m128 d = _mm_movelh_ps( lo, hi );
			_mm_storeu_ps( dists + i, d );
			frontbits |= unsigned( _mm_movemask_ps( _mm_cmpge_ps( d, front ) ) ) << i;
			backbits |= unsigned( _mm_movemask_ps( _mm_cmple_ps( d, back ) ) ) << i;
			if ( i + 4 >= in->numpoints ) {
				break;
			}
			i = std::min( i + 4, in->numpoints - 4 );
		}
		counts[0] = frontbits != 0;
		counts[1] = backbits != 0;
		i = in->numpoints;
	}
	for ( ; i < in->numpoints; ++i )
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

	if ( four ) {
		for ( j = 0; j < in->numpoints; ++j )
			sides[j] = ( frontbits >> j & 1 ) ? eSideFront : ( backbits >> j & 1 ) ? eSideBack : eSideOn;
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
		const Vector3& p2 = in->points[i + 1 == in->numpoints ? 0 : i + 1];

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

/* BehavEd vis4 (not in NetRadiant-Custom): has the plane that NRC's code is about to take as a separating
   plane - made from the source edge i-l and the point j of pass, pass in front of it - the whole of source
   behind it? NRC's code, id's since q3map, takes the side of source from its first point farther than
   ON_EPSILON from the plane and never looks at the others. That is right for a plane through an edge of a
   convex winding. But the three points may lie nearly on one line: two portals that share an edge have its
   corners with other digits, so the point of pass is the end of the source edge a hundredth of a unit away.
   Such a plane turns about the edge as the rounding has it, and the cross product in single precision loses
   its digits - the plane misses the points it is made from by more than ON_EPSILON (kejim_post: 0.11 to 0.18
   units, the sine of the angle at the points is 1e-5). A nearly doubled corner of source next to the edge then
   is "in front" by that error and is taken for the side of the whole winding: the plane is accepted with
   source and pass on the same side, and everything behind pass is cut away (kejim_post: the 131 portals on
   sight lines the check of vis3 found missing, at three portals; in a chain of four leafs with such portals
   the last leaf is missing in the visibility of the first - with NRC's q3map2.exe and any number of threads,
   docs/intern/NOTIZEN_vis4_1111.md).
   So the side of source is taken from all the points NRC's loop looks at, not from the first: none of them
   may be farther than ON_EPSILON in front of the plane. A plane through an edge of a convex winding passes
   this always (the first point says where the others are), so nothing changes where NRC is right; a plane
   that fails is no separating plane and is left out, as NRC leaves out the others that fail.
   -bhedvisstrictplanes asks more: the two points of the edge and the point of pass, which NRC's loops skip
   because the plane is made from them, have to lie on their side within ON_EPSILON as well - a plane that
   misses its own points by more is the work of rounding. No gap is known that needs it. */
static std::atomic<uint64_t> g_bhedFalsePlanes{ 0 };

/* the rare cases of BhedSeperates, out of the way of the flows: a plane that failed, or a switch is set */
__declspec( noinline ) static bool BhedSeperatesRare( const visPlane_t& plane, const fixedWinding_t *source, const fixedWinding_t *pass, int i, int l, int j, bool behind ){
	if ( bhedNrcPlanes ) {
		return true;
	}
	if ( behind ) {     /* -bhedvisstrictplanes */
		behind = plane3_distance_to_point( plane, pass->points[j] ) >= -ON_EPSILON
		      && plane3_distance_to_point( plane, source->points[i] ) <= ON_EPSILON
		      && plane3_distance_to_point( plane, source->points[l] ) <= ON_EPSILON;
	}
	if ( !behind ) {
		g_bhedFalsePlanes.fetch_add( 1, std::memory_order_relaxed );
	}
	return behind;
}

static inline bool BhedSeperates( const visPlane_t& plane, const fixedWinding_t *source, const fixedWinding_t *pass, int i, int l, int j, int side ){
	/* `side` is the point NRC's loop took the side from: it is behind the plane, the points before it are
	   within ON_EPSILON of it - the ones after it are left. With four points that is at most one: the sum
	   of the indices 0 to 3 less the edge and `side`. */
	bool behind = true;
	if ( source->numpoints == 4 ) {
		const int k = 6 - i - l - side;
		behind = k < side || plane3_distance_to_point( plane, source->points[k] ) <= ON_EPSILON;
	}
	else{
		for ( int k = side + 1; behind && k < source->numpoints; ++k )
			behind = k == i || k == l || plane3_distance_to_point( plane, source->points[k] ) <= ON_EPSILON;
	}
	if ( behind && !bhedStrictPlanes ) {
		return true;
	}
	return BhedSeperatesRare( plane, source, pass, i, l, j, behind );
}

uint64_t BhedFalsePlanes(){
	return g_bhedFalsePlanes.load();
}

/* BehavEd vis2s: the inner loop of ClipToSeperators - the separating plane through the source edge i
   and a vertex of pass, false if there is none. NRC's code, moved into a function of its own: the planes
   of a frame do not depend on the target, so they can be made again later (BhedSeperatorsHave). */
static bool BhedEdgeSeperator( const fixedWinding_t *source, const fixedWinding_t *pass, int i, bool flipclip, visPlane_t& plane ){
	int j, k, l;
	float d;
	int counts[3];
	bool fliptest;

	l = i + 1 == source->numpoints ? 0 : i + 1;     /* BehavEd vis2s: without the division of ( i + 1 ) % numpoints */

	// find a vertex of pass that makes a plane that puts all of the
	// vertexes of pass on the front side and all of the vertexes of
	// source on the back side
	for ( j = 0; j < pass->numpoints; ++j )
	{
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
		const int side = k;     /* BehavEd vis4: for BhedSeperates */
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
		if ( !BhedSeperates( plane, source, pass, i, l, j, side ) ) {
			continue;   /* BehavEd vis4: source on both sides, no separating plane */
		}
		//
		// flip the normal if we want the back side
		//
		if ( flipclip ) {
			plane = plane3_flipped( plane );
		}
		return true;
	}
	return false;
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
	float d;

	// check all combinations
	for ( int i = 0; i < source->numpoints; ++i )
	{
		visPlane_t plane;
		if ( !BhedEdgeSeperator( source, pass, i, flipclip, plane ) ) {
			continue;
		}

#ifdef SEPERATORCACHE
		stack->seperators[flipclip][stack->numseperators[flipclip]] = plane;
		if ( ++stack->numseperators[flipclip] >= MAX_SEPERATORS ) {
			Error( "MAX_SEPERATORS" );
		}
		stack->sephave[flipclip] = stack->numseperators[flipclip];
		stack->sepedge[flipclip] = i + 1;
#endif
		//MrE: fast check first
		d = plane3_distance_to_point( plane, stack->portal->origin );
		//if completely at the back of the separator plane
		if ( d < -stack->portal->radius ) {
			return nullptr;
		}
		//if completely on the front of the separator plane
		if ( d > stack->portal->radius ) {
			continue;
		}

		//
		// clip target by the separating plane
		//
		target = VisChopWinding( target, stack, plane );
		if ( !target ) {
			return nullptr;        // target is not visible
		}
		// optimization by Antony Suter: one plane per source edge
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
	BhedFlowFrame( thread );

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
		if ( BhedStopped() ) {
			break;      /* BehavEd vis2: paused */
		}
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
			test = BhedResolve( p, [&]( auto&& g ){
				for ( int k = 0; k < prevstack->nummightwords; ++k )
					g( prevstack->mightwords[k], prevstack->mightsee[prevstack->mightwords[k]] );
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
				BhedSetMight( stack, w, m );
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
	/* BehavEd vis2: restored from the vis state, or the VIS is paused */
	if ( BhedStatus( p ) == EVStatus::Done || BhedStopped() ) {
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;
	BhedFlowBegin( data );

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );

	RecursiveLeafFlow( p->leaf, &data, &data.pstack_head );
	BhedHeadDone( data.pstack_head );
	if ( g_bhedVisStop.load( std::memory_order_seq_cst ) ) {
		return;     /* BehavEd vis2: paused - no result, it may have read unfinished vectors */
	}

	BhedFlowDone( data );
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

	BhedFlowFrame( thread );

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
		if ( BhedStopped() ) {
			break;      /* BehavEd vis2: paused */
		}
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

		/* BehavEd vis1: only the words nonzero in the previous vector and cansee (the rest is zero) */
		const uint64_t *portalvis = BhedVector( thread, p );
		if ( portalvis == nullptr ) {
			portalvis = BhedResolve( p, [&]( auto&& g ){
				BhedCommonWords( prevstack, ps, [&]( int w, uint64_t cansee ){ g( w, prevstack->mightsee[w] & cansee ); } );
			} );
		}
		BhedClearMight( stack );
		uint64_t more = 0;
		BhedCommonWords( prevstack, ps, [&]( int w, uint64_t cansee ){
			const uint64_t m = prevstack->mightsee[w] & cansee & portalvis[w];
			if ( m != 0 ) {
				BhedSetMight( stack, w, m );
				more |= m & ~vis[w];
			}
		} );

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
	/* BehavEd vis2: restored from the vis state, or the VIS is paused */
	if ( BhedStatus( p ) == EVStatus::Done || BhedStopped() ) {
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

//	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;
	BhedFlowBegin( data );

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );

	RecursivePassageFlow( p, &data, &data.pstack_head );
	BhedHeadDone( data.pstack_head );
	if ( g_bhedVisStop.load( std::memory_order_seq_cst ) ) {
		return;     /* BehavEd vis2: paused - no result, it may have read unfinished vectors */
	}

	BhedFlowDone( data );
	BhedSetStatus( p, EVStatus::Done );

	/*
	   c_can = CountBits( p->portalvis, numportals * 2 );

	   Sys_FPrintf( SYS_VRB, "portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n",
	    (int)( p - portals ), c_might, c_can, data.c_chains );
	 */
}

/* BehavEd vis2s: geometry memo of the flow.
   Profile (md_ch_ben, one thread): 51 % of the flow makes separating planes (ClipToSeperators), 21 %
   clips windings, 25 % are the bit vectors. A flow walks through the same portals again and again -
   56 million frames for 1750 portals, nearly all of them chasing bits that are in mightsee and never
   become visible - and half of the time the frame has exactly the windings of an earlier one.
   What the geometry does with a portal p of the leaf is a function of the flow (plane, origin and
   radius of the base portal), of the source and pass windings the frame was entered with, of p and of
   the number of planes each of the two separator caches holds at that moment - not of the bit
   vectors and not of anything found so far. The memo keeps this function:
     state:   ( flow, points of source, points of pass ) -> id, compared point by point
     outcome: ( id, p, planes cached before ) -> planes cached after, clipped away or the id of the
              windings behind p
   A frame that finds its portal in the memo makes no plane and no cut, and it does not fetch the
   windings behind p either: the frame below gets their id only (pstack_t::lazy) and goes on through
   the outcomes of that id. Only when a portal down there is not in the memo the points are taken
   from the state - or, if its slot has been overwritten meanwhile, made again from the windings of
   the frame above (BhedGeoWindings). The caches then hold fewer planes than NRC's code counts
   (pstack_t::sephave); a later portal of the frame that is not in the memo makes the missing ones
   again (BhedSeperatorsHave) - the same planes, they depend on the windings of the frame only.
   Same bits as without the memo, for any size of the tables: an id stands for one ( flow, source,
   pass ) as long as the memo is not emptied, whatever is overwritten, and nothing is taken from a
   slot without comparing the id or the points.
   The outcomes lie in buckets of four (one cache line; a new one takes an empty place or that of the
   oldest state), and the bucket is fetched while the frame still makes its bit vectors. A state that
   was just put into the memo cannot have outcomes yet: its frame asks for none.
   BHED_VIS_GEOMEMO=<n> sets the size (2^n states and eight times as many outcomes for each thread,
   14 if not set: 5.5 MB), 0 switches the memo off - for measurements and for the ctest, which runs
   a small memo (16 states) to get at the overwritten slots.
   What it brings and why this size (md_ch_ben, ctf3, t3_byss, duel_mos_espa): docs/intern/NOTIZEN_vis2s_1011.md. */

/* 64 x 64 -> 128 bits, the two halves folded (hash of the points) */
static uint64_t BhedFoldMul( uint64_t a, uint64_t b ){
#if defined( _MSC_VER )
	uint64_t high;
	const uint64_t low = _umul128( a, b, &high );
	return low ^ high;
#else
	const unsigned __int128 product = static_cast<unsigned __int128>( a ) * b;
	return uint64_t( product ) ^ uint64_t( product >> 64 );
#endif
}

struct BhedGeoMemo
{
	static constexpr int c_points = 16;         /* points of source and pass together, more: not in the memo */
	struct State
	{
		uint32_t id;                            /* 0: empty */
		uint32_t tag;                           /* upper half of the hash */
		int32_t flow;
		uint8_t numsource, numpass;
		Vector3 points[c_points];               /* source, then pass */
	};
	struct Outcome                              /* four make a bucket: one cache line */
	{
		uint32_t state;                         /* id of the frame's windings, 0: empty */
		uint32_t key;                           /* BhedGeoMemo::key */
		uint32_t id;                            /* the windings behind the portal, 0: clipped away */
		uint32_t where;                         /* their slot | planes in the two separator caches after: << 20, << 26 */
	};
	static constexpr int c_outcomes = 3;        /* 2^c_outcomes outcomes for each state */
	const int bits;                             /* 2^bits states, 0: no memo */
	const uint32_t lastId;
	State *states;
	void *block;                                /* the memory of outcomes */
	Outcome *outcomes;                          /* on a cache line boundary */
	uint32_t ids = 0;
	int flows = 0;                              /* flows that use the memo at this moment */
	/* portals taken from the memo, computed, windings and planes made again, memo emptied, states that got no id */
	uint64_t known = 0, made = 0, again = 0, planes = 0, emptied = 0, refused = 0;

	explicit BhedGeoMemo( int size = BhedGeoBits() ) : bits( size ), lastId( BhedGeoLastId() ),
		states( bits > 0 ? static_cast<State*>( safe_calloc( ( size_t( 1 ) << bits ) * sizeof( State ) ) ) : nullptr ),
		block( bits > 0 ? safe_calloc( outcomeBytes() + 64 ) : nullptr ),
		outcomes( reinterpret_cast<Outcome*>( ( reinterpret_cast<uintptr_t>( block ) + 63 ) & ~uintptr_t( 63 ) ) ){
		static_assert( sizeof( Outcome ) == 16 );
	}
	~BhedGeoMemo(){
		free( states );
		free( block );
	}
	size_t outcomeBytes() const {
		return ( size_t( 1 ) << ( bits + c_outcomes ) ) * sizeof( Outcome );
	}
	BhedGeoMemo( const BhedGeoMemo& ) = delete;
	BhedGeoMemo& operator=( const BhedGeoMemo& ) = delete;

	/* A flow begins. The ids have 32 bits: when half of them are used up, the memo starts again empty -
	   only between two flows, a running flow holds ids in its frames. A single flow that needs more
	   than the other half goes on without new states (intern). */
	void begin(){
		if ( flows++ == 0 && ids >= lastId / 2 && bits > 0 ) {
			memset( states, 0, ( size_t( 1 ) << bits ) * sizeof( State ) );
			memset( outcomes, 0, outcomeBytes() );
			ids = 0;
			++emptied;
		}
	}
	void end(){
		--flows;
	}

	/* a portal number has 18 bits; a frame with an id has at most c_points points and so at most
	   c_points planes in a cache (one for each edge of source) - the caller checks the 6 bits */
	static uint32_t key( int pnum, int before0, int before1 ){
		static_assert( MAX_MAP_PORTALS * 2 <= 1 << 18 );
		return uint32_t( pnum ) | uint32_t( before0 ) << 18 | uint32_t( before1 ) << 24;
	}
	/* the bucket of ( state, key ): four outcomes */
	Outcome *bucket( uint32_t state, uint32_t key ){
		return outcomes + ( ( ( uint64_t( state ) << 32 | key ) * 0x9E3779B97F4A7C15ull ) >> ( 66 - bits - c_outcomes ) << 2 );
	}
	static const Outcome *find( const Outcome *bucket, uint32_t state, uint32_t key ){
		for ( int i = 0; i < 4; ++i )
			if ( bucket[i].state == state && bucket[i].key == key ) {
				return bucket + i;
			}
		return nullptr;
	}
	/* where a new outcome goes: an empty place, else the one of the oldest state */
	static Outcome& place( Outcome *bucket ){
		Outcome *oldest = bucket;
		for ( int i = 1; i < 4; ++i )
			if ( bucket[i].state < oldest->state ) {
				oldest = bucket + i;
			}
		return *oldest;
	}

	uint64_t hash( int flow, const fixedWinding_t *source, const fixedWinding_t *pass ) const {
		uint64_t h = uint64_t( uint32_t( flow ) ) << 16 | uint64_t( source->numpoints ) << 8 | uint64_t( pass->numpoints );
		uint64_t n = 0;
		for ( const fixedWinding_t *w : { source, pass } )
			for ( int i = 0; i < w->numpoints; ++i )
			{
				/* a product for each point, none waits for another */
				uint64_t a;
				uint32_t b;
				memcpy( &a, &w->points[i], 8 );
				memcpy( &b, reinterpret_cast<const char*>( &w->points[i] ) + 8, 4 );
				n += 0x9E3779B97F4A7C15ull;
				h += BhedFoldMul( a ^ n, b ^ 0xC2B2AE3D27D4EB4Full );
			}
		return BhedFoldMul( h, 0xFF51AFD7ED558CCDull );
	}
	static bool same( const State& s, int flow, const fixedWinding_t *source, const fixedWinding_t *pass ){
		return s.flow == flow && s.numsource == source->numpoints && s.numpass == pass->numpoints
		    && memcmp( s.points, source->points, source->numpoints * sizeof( Vector3 ) ) == 0
		    && memcmp( s.points + source->numpoints, pass->points, pass->numpoints * sizeof( Vector3 ) ) == 0;
	}
	static void store( State& s, uint32_t id, uint32_t tag, int flow, const fixedWinding_t *source, const fixedWinding_t *pass ){
		s.id = id;
		s.tag = tag;
		s.flow = flow;
		s.numsource = uint8_t( source->numpoints );
		s.numpass = uint8_t( pass->numpoints );
		memcpy( s.points, source->points, source->numpoints * sizeof( Vector3 ) );
		memcpy( s.points + source->numpoints, pass->points, pass->numpoints * sizeof( Vector3 ) );
	}

	/* the id of ( flow, source, pass ) and its slot, 0: none (too many points, or the ids ran out);
	   fresh: the id is new, no outcome can have it yet */
	uint32_t intern( int flow, const fixedWinding_t *source, const fixedWinding_t *pass, uint32_t& slot, bool& fresh ){
		if ( bits == 0 || source->numpoints + pass->numpoints > c_points ) {
			return 0;
		}
		const uint64_t h = hash( flow, source, pass );
		slot = uint32_t( h ) & ( ( uint32_t( 1 ) << bits ) - 1 );
		State& s = states[slot];
		if ( s.id != 0 && s.tag == uint32_t( h >> 32 ) && same( s, flow, source, pass ) ) {
			return s.id;
		}
		if ( ids >= lastId ) {
			++refused;
			return 0;
		}
		fresh = true;
		store( s, ++ids, uint32_t( h >> 32 ), flow, source, pass );
		return ids;
	}

	/* the points of an id whose slot was overwritten were made again: back into the slot. False if
	   they do not belong there - then they are not the points the id was given for. */
	bool restore( uint32_t id, uint32_t slot, int flow, const fixedWinding_t *source, const fixedWinding_t *pass ){
		const uint64_t h = hash( flow, source, pass );
		if ( source->numpoints + pass->numpoints > c_points || ( uint32_t( h ) & ( ( uint32_t( 1 ) << bits ) - 1 ) ) != slot ) {
			return false;
		}
		State& s = states[slot];
		if ( s.id == 0 || s.tag != uint32_t( h >> 32 ) || !same( s, flow, source, pass ) ) {
			store( s, id, uint32_t( h >> 32 ), flow, source, pass );
		}
		return true;
	}
};

static BhedGeoMemo *BhedGeoThreadMemo(){
	static thread_local std::unique_ptr<BhedGeoMemo> memo;
	if ( !memo ) {
		memo = std::make_unique<BhedGeoMemo>();
	}
	return memo.get();
}

/* BehavEd vis3: the memo of a flow. With NRC's rule and -bhedvisexact that is the memo of its thread. The fixed
   rule has a thread for each chain, more than compute at a time - a memo for each would be 5.5 MB times the
   chains. There a flow takes a memo from a store when it begins and gives it back when it ends (it holds ids in
   its frames meanwhile, also while it waits). The store has as many memos of the full size as chains compute
   at a time (-threads); the flows that are under way beyond that - as many as are waiting - get small ones,
   a quarter of the size. The result does not depend on the size of a memo. */
namespace
{
std::mutex g_bhedGeoStoreMutex;
std::vector<std::unique_ptr<BhedGeoMemo>> g_bhedGeoStore[2];   /* the memos no flow has at the moment: full size, small */
int g_bhedGeoMemos[2] = { 0, 0 };                              /* all there are */
}

static BhedGeoMemo *BhedGeoTake(){
	if ( !g_bhedFixed ) {
		return BhedGeoThreadMemo();
	}
	int small;
	{
		std::lock_guard<std::mutex> lock( g_bhedGeoStoreMutex );
		for ( auto& store : g_bhedGeoStore )
			if ( !store.empty() ) {
				BhedGeoMemo *memo = store.back().release();     /* the one that was given back last */
				store.pop_back();
				return memo;
			}
		small = g_bhedGeoMemos[0] >= std::clamp( numthreads, 1, g_bhedFixed );
		++g_bhedGeoMemos[small];
	}
	return new BhedGeoMemo( small && BhedGeoBits() > 2 ? BhedGeoBits() - 2 : BhedGeoBits() );
}

static int BhedGeoMemos(){
	std::lock_guard<std::mutex> lock( g_bhedGeoStoreMutex );
	return g_bhedGeoMemos[0] + g_bhedGeoMemos[1];
}

static void BhedGeoGive( BhedGeoMemo *memo ){
	if ( g_bhedFixed ) {
		std::lock_guard<std::mutex> lock( g_bhedGeoStoreMutex );
		g_bhedGeoStore[memo->bits < BhedGeoBits()].emplace_back( memo );
	}
}

/* at the end of a flow: the counts of the thread to the sum of the run */
static void BhedGeoCount( BhedGeoMemo& memo ){
	g_bhedGeoKnown += memo.known;
	g_bhedGeoMade += memo.made;
	g_bhedGeoAgain += memo.again;
	g_bhedGeoPlanes += memo.planes;
	g_bhedGeoEmptied += memo.emptied;
	g_bhedGeoRefused += memo.refused;
	memo.known = memo.made = memo.again = memo.planes = memo.emptied = memo.refused = 0;
}

static bool BhedPortalGeometry( threaddata_t *thread, pstack_t *prevstack, pstack_t& stack, vportal_t *p );

/* The windings of a frame that got only their id from the memo (pstack_t::lazy) are needed after all:
   from the state of the id, or - its slot has been overwritten - by the geometry of the frame's
   portal once more, with the separator caches as they were when the memo answered. */
static void BhedGeoWindings( threaddata_t *thread, pstack_t& stack ){
	BhedGeoMemo& memo = *thread->geo;
	stack.lazy = false;
	const BhedGeoMemo::State& behind = memo.states[stack.geoslot];
	if ( behind.id == stack.geo ) {
		stack.windings[0].numpoints = behind.numsource;
		memcpy( stack.windings[0].points, behind.points, behind.numsource * sizeof( Vector3 ) );
		stack.windings[1].numpoints = behind.numpass;
		memcpy( stack.windings[1].points, behind.points + behind.numsource, behind.numpass * sizeof( Vector3 ) );
		stack.source = &stack.windings[0];
		stack.pass = &stack.windings[1];
		return;
	}
	const int after0 = stack.numseperators[0], after1 = stack.numseperators[1];
	stack.numseperators[0] = stack.geobefore[0];
	stack.numseperators[1] = stack.geobefore[1];
	stack.freewindings[0] = 1;
	stack.freewindings[1] = 1;
	stack.freewindings[2] = 1;
	if ( !BhedPortalGeometry( thread, stack.prev, stack, stack.portal ) || stack.numseperators[0] != after0 || stack.numseperators[1] != after1
	  || !memo.restore( stack.geo, stack.geoslot, int( thread->base - portals ), stack.source, stack.pass ) ) {
		Error( "BhedGeoWindings: the geometry memo does not match the geometry" );
	}
	++memo.again;
}

/* the separator cache `flip` of the frame really holds the planes NRC's code counts for it (see BhedGeoMemo) */
static void BhedSeperatorsHave( BhedGeoMemo& memo, const pstack_t *prevstack, pstack_t& stack, int flip ){
	const fixedWinding_t *source = flip ? prevstack->pass : prevstack->source;
	const fixedWinding_t *pass = flip ? prevstack->source : prevstack->pass;
	while ( stack.sephave[flip] < stack.numseperators[flip] )
	{
		visPlane_t plane;
		bool found = false;
		while ( !found && stack.sepedge[flip] < source->numpoints )
			found = BhedEdgeSeperator( source, pass, stack.sepedge[flip]++, flip != 0, plane );
		if ( !found ) {
			Error( "BhedSeperatorsHave: plane %i of %i not found", stack.sephave[flip], stack.numseperators[flip] );
		}
		stack.seperators[flip][stack.sephave[flip]++] = plane;
		++memo.planes;
	}
}

/* the geometry of RecursivePassagePortalFlow for the portal p of the leaf (NRC's code, moved): the
   windings behind p in stack.source and stack.pass, false: p cannot be seen */
static bool BhedPortalGeometry( threaddata_t *thread, pstack_t *prevstack, pstack_t& stack, vportal_t *p ){
	int n;

	if ( prevstack->lazy ) {
		BhedGeoWindings( thread, *prevstack );
	}

#if 1
	{
		const float d = plane3_distance_to_point( thread->pstack_head.portalplane, p->origin );
		if ( d < -p->radius ) {
			return false;
		}
		else if ( d > p->radius ) {
			stack.pass = p->winding;
		}
		else
		{
			stack.pass = VisChopWinding( p->winding, &stack, thread->pstack_head.portalplane );
			if ( !stack.pass ) {
				return false;
			}
		}
	}
#else
	stack.pass = VisChopWinding( p->winding, &stack, thread->pstack_head.portalplane );
	if ( !stack.pass ) {
		return false;
	}
#endif


#if 1
	{
		const float d = plane3_distance_to_point( p->plane, thread->base->origin );
		//MrE: vis-bug fix
		//if ( d > p->radius )
		if ( d > thread->base->radius ) {
			return false;
		}
		//MrE: vis-bug fix
		//if ( d < -p->radius )
		else if ( d < -thread->base->radius ) {
			stack.source = prevstack->source;
		}
		else
		{
			// get plane of portal, point normal into the neighbor leaf
			stack.source = VisChopWinding( prevstack->source, &stack, plane3_flipped( p->plane ) );
			//FIXME: shouldn't we create a new source origin and radius for fast checks?
			if ( !stack.source ) {
				return false;
			}
		}
	}
#else
	stack.source = VisChopWinding( prevstack->source, &stack, plane3_flipped( p->plane ) );
	if ( !stack.source ) {
		return false;
	}
#endif

	if ( !prevstack->pass ) { // the second leaf can only be blocked if coplanar
		return true;
	}

#ifdef SEPERATORCACHE
	if ( stack.numseperators[0] ) {
		BhedSeperatorsHave( *thread->geo, prevstack, stack, 0 );
		for ( n = 0; n < stack.numseperators[0]; ++n )
		{
			stack.pass = VisChopWinding( stack.pass, &stack, stack.seperators[0][n] );
			if ( !stack.pass ) {
				break;      // target is not visible
			}
		}
		if ( n < stack.numseperators[0] ) {
			return false;
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
		return false;
	}

#ifdef SEPERATORCACHE
	if ( stack.numseperators[1] ) {
		BhedSeperatorsHave( *thread->geo, prevstack, stack, 1 );
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
	return stack.pass != nullptr;
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
	int i;

//	thread->c_chains++;
	BhedFlowFrame( thread );

	leaf = &leafs[portal->leaf];
//	CheckStack( leaf, thread );

	prevstack->next = &stack;

	stack.next = nullptr;
	stack.leaf = leaf;
	stack.portal = nullptr;
	stack.depth = prevstack->depth + 1;
	stack.prev = prevstack;
	stack.lazy = false;
	BhedFrameBuffers( stack );

#ifdef SEPERATORCACHE
	stack.numseperators[0] = 0;
	stack.numseperators[1] = 0;
	stack.sephave[0] = 0;
	stack.sephave[1] = 0;
	stack.sepedge[0] = 0;
	stack.sepedge[1] = 0;
#endif

	const uint64_t *vis = reinterpret_cast<const uint64_t*>( thread->base->portalvis );
	BhedGeoMemo& memo = *thread->geo;
	const uint32_t geo = prevstack->geo;    /* the windings of this frame in the memo, 0: not there */
	const bool fresh = prevstack->geofresh; /* just put there: none of its portals can be known */
	const int flow = int( thread->base - portals );

	const passage_t *passage = portal->passages;
	// check all portals for flowing into other leafs
	for ( i = 0; i < leaf->numportals; ++i )
	{
		if ( BhedStopped() ) {
			break;      /* BehavEd vis2: paused */
		}
		p = leaf->portals[i];
		if ( p->removed ) {
			continue;
		}
		const passage_t *ps = passage++;
		const int pnum = p - portals;

		if ( !bit_is_enabled( reinterpret_cast<const byte*>( prevstack->mightsee ), pnum ) ) {
			continue;   // can't possibly see it
		}
		/* BehavEd vis2s: where the geometry memo has this portal - into the cache while the bit vectors are made */
		const int before0 = stack.numseperators[0], before1 = stack.numseperators[1];
		const uint32_t key = BhedGeoMemo::key( pnum, before0, before1 );
		BhedGeoMemo::Outcome *bucket = nullptr;
		if ( geo != 0 && ( before0 | before1 ) < 64 ) {
			bucket = memo.bucket( geo, key );
			if ( !fresh ) {
				_mm_prefetch( reinterpret_cast<const char*>( bucket ), _MM_HINT_T0 );
			}
		}
		/* BehavEd vis1: only the words nonzero in the previous vector and cansee (the rest is zero) */
		const uint64_t *portalvis = BhedVector( thread, p );
		if ( portalvis == nullptr ) {
			portalvis = BhedResolve( p, [&]( auto&& g ){
				BhedCommonWords( prevstack, ps, [&]( int w, uint64_t cansee ){ g( w, prevstack->mightsee[w] & cansee ); } );
			} );
		}
		BhedClearMight( stack );
		uint64_t more = 0;
		BhedCommonWords( prevstack, ps, [&]( int w, uint64_t cansee ){
			const uint64_t m = prevstack->mightsee[w] & cansee & portalvis[w];
			if ( m != 0 ) {
				BhedSetMight( stack, w, m );
				more |= m & ~vis[w];
			}
		} );

		if ( !more && bit_is_enabled( thread->base->portalvis, pnum ) ) { // can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;

		stack.portal = p;
		stack.next = nullptr;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;

		/* BehavEd vis2s: the geometry, from the memo where it has been done before */
		bool known = false;
		if ( bucket != nullptr && !fresh ) {
			if ( const BhedGeoMemo::Outcome *outcome = BhedGeoMemo::find( bucket, geo, key ) ) {
				stack.numseperators[0] = int( outcome->where >> 20 & 63 );
				stack.numseperators[1] = int( outcome->where >> 26 );
				++memo.known;
				if ( outcome->id == 0 ) {
					continue;
				}
				/* only the id of the windings behind p - the frame below may never need the points */
				stack.geo = outcome->id;
				stack.geoslot = outcome->where & 0xFFFFF;
				stack.geofresh = false;
				stack.geobefore[0] = uint8_t( before0 );
				stack.geobefore[1] = uint8_t( before1 );
				stack.lazy = true;
				stack.source = nullptr;
				stack.pass = nullptr;
				known = true;
			}
		}
		if ( !known ) {
			stack.lazy = false;
			const bool visible = BhedPortalGeometry( thread, prevstack, stack, p );
			uint32_t slot = 0;
			bool isnew = false;
			stack.geo = visible ? memo.intern( flow, stack.source, stack.pass, slot, isnew ) : 0;
			stack.geofresh = isnew;
			++memo.made;
			if ( bucket != nullptr && ( !visible || stack.geo != 0 ) && ( stack.numseperators[0] | stack.numseperators[1] ) < 64 ) {
				BhedGeoMemo::Outcome& outcome = BhedGeoMemo::place( bucket );
				outcome.state = geo;
				outcome.key = key;
				outcome.id = stack.geo;
				outcome.where = slot | uint32_t( stack.numseperators[0] ) << 20 | uint32_t( stack.numseperators[1] ) << 26;
			}
			if ( !visible ) {
				continue;
			}
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
	/* BehavEd vis2: restored from the vis state, or the VIS is paused */
	if ( BhedStatus( p ) == EVStatus::Done || BhedStopped() ) {
		return;
	}

	BhedSetStatus( p, EVStatus::Working );

//	c_might = CountBits( p->portalflood, numportals * 2 );

	memset( &data, 0, sizeof( data ) );
	data.base = p;
	BhedFlowBegin( data );

	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	BhedHeadFrame( data.pstack_head, p );
	data.geo = BhedGeoTake();
	data.geo->begin();

	RecursivePassagePortalFlow( p, &data, &data.pstack_head );
	data.geo->end();
	BhedGeoCount( *data.geo );
	BhedGeoGive( data.geo );
	BhedHeadDone( data.pstack_head );
	if ( g_bhedVisStop.load( std::memory_order_seq_cst ) ) {
		return;     /* BehavEd vis2: paused - no result, it may have read unfinished vectors */
	}

	BhedFlowDone( data );
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
			const int side = k;     /* BehavEd vis4: for BhedSeperates */
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
			if ( !BhedSeperates( plane, source, pass, i, l, j, side ) ) {
				continue;   /* BehavEd vis4: source on both sides, no separating plane */
			}
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
	static thread_local std::vector<uint64_t> allmasks;
	static thread_local std::vector<uint16_t> allranks;
	static thread_local std::vector<uint64_t> allbits;
	static thread_local std::vector<int> counts;
	cansee.assign( portalwords, 0 );
	allmasks.clear();
	allranks.clear();
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
			if ( wi % 64 == 0 ) {
				allmasks.push_back( 0 );
				allranks.push_back( uint16_t( count ) );
			}
			if ( cansee[wi] != 0 ) {
				allmasks.back() |= uint64_t( 1 ) << ( wi & 63 );
				allbits.push_back( cansee[wi] );
				cansee[wi] = 0;
				++count;
			}
		}
		counts.push_back( count );
	}

	const size_t bytes = counts.size() * sizeof( passage_t ) + ( allmasks.size() + allbits.size() ) * sizeof( uint64_t ) + allranks.size() * sizeof( uint16_t );
	byte *block = safe_malloc( std::max<size_t>( bytes, 1 ) );
	passage_t *passages = reinterpret_cast<passage_t*>( block );
	uint64_t *masks = reinterpret_cast<uint64_t*>( block + counts.size() * sizeof( passage_t ) );
	uint64_t *bits = masks + allmasks.size();
	uint16_t *ranks = reinterpret_cast<uint16_t*>( bits + allbits.size() );
	std::copy( allmasks.begin(), allmasks.end(), masks );
	std::copy( allbits.begin(), allbits.end(), bits );
	std::copy( allranks.begin(), allranks.end(), ranks );
	for ( const int count : counts )
	{
		passages->mask = masks;
		passages->rank = ranks;
		passages->bits = bits;
		masks += portalmaskwords;
		ranks += portalmaskwords;
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
