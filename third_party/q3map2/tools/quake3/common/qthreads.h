/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
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
 */

#pragma once

#include <atomic>  /* BehavEd stage 6: shaderInfo_t::s_bhedNumbers (q3map2.h) */

extern int numthreads;

void ThreadSetDefault();
void RunThreadsOnIndividual( int workcnt, bool showpacifier, void ( *func )( int ) );
/* BehavEd vis1: the same on `threads` threads instead of numthreads (the deterministic vis flow
   keeps more threads than cores, of which numthreads compute at a time) */
void BhedRunThreadsOnIndividual( int workcnt, bool showpacifier, void ( *func )( int ), int threads );
void ThreadLock();
void ThreadUnlock();

/* BehavEd stage 6 (determinism): Random() is rand(), and MSVC keeps the rand() state per
   thread (each worker starts at seed 1) - so in threaded passes (TraceGrid nudging grid
   points out of solid, dirt random mode, -randomsamples, minimap) the numbers a work item
   got depended on which items its thread had done before. Each work item now gets its own
   generator, seeded from (pass, item); outside the threaded passes Random() stays rand()
   as in NRC. Same result for any thread count. */
extern thread_local bool g_bhedWorkRandom;
extern thread_local unsigned int g_bhedRandomState;
inline float BhedWorkRandom(){ /* MSVC rand() LCG, 15 bits like RAND_MAX 0x7fff */
	g_bhedRandomState = g_bhedRandomState * 214013u + 2531011u;
	return (float)( ( g_bhedRandomState >> 16 ) & 0x7fff ) / 0x7fff;
}

/* BehavEd vis1: the same generator with its own state (BhedWorkRandom on a copy), and the state
   after n draws without drawing them (x -> a x + c, n times: a^n x + c ( a^(n-1) + ... + 1 ),
   by squaring). A work item split over several threads gives each part the state the serial
   order would have reached - the same numbers in the same places. */
struct BhedRandom
{
	unsigned int state;
	float operator()(){
		state = state * 214013u + 2531011u;
		return (float)( ( state >> 16 ) & 0x7fff ) / 0x7fff;
	}
};
inline unsigned int BhedRandomSkip( unsigned int state, unsigned long long n ){
	unsigned int mul = 1, add = 0, a = 214013u, c = 2531011u;
	for ( ; n != 0; n >>= 1 )
	{
		if ( n & 1 ) {
			mul *= a;
			add = add * a + c;
		}
		c = c * a + c;
		a *= a;
	}
	return state * mul + add;
}
/* the seed work item `work` gets in the next RunThreadsOnIndividual, and in the running one */
unsigned int BhedNextPassSeed( int work );
unsigned int BhedPassSeed( int work );
