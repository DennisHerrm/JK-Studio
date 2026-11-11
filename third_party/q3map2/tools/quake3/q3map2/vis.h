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
*/


#pragma once

#include <atomic>

/* vis structures */

using visPlane_t = Plane3f;


struct fixedWinding_t
{
	int numpoints;
	Vector3 points[ MAX_POINTS_ON_FIXED_WINDING ];                   /* variable sized */
};


/* BehavEd vis1: the bit vectors are read as 64-bit words (NRC: long, 32 bits on Windows) - the same
   bits. A passage keeps only the nonzero words of cansee (all portals that can be seen through it):
   md_rev_TarisUpper2 needed 9 GB for the full vectors. */
struct passage_t
{
	const uint64_t      *mask;          /* [portalmaskwords]: bit w set = word w of cansee is not zero */
	const uint16_t      *rank;          /* [portalmaskwords]: nonzero words before mask word i */
	const uint64_t      *bits;          /* the nonzero words of cansee, ascending */
};


enum class EVStatus
{
	None,
	Working,
	Done
};


struct vportal_t
{
	int num;
	bool hint;                          /* true if this portal was created from a hint splitter */
	bool sky;                           /* true if this portal belongs to a sky leaf */
	bool removed;
	visPlane_t plane;                   /* normal pointing into neighbor */
	int leaf;                           /* neighbor */

	Vector3 origin;                     /* for fast clip testing */
	float radius;

	fixedWinding_t      *winding;
	EVStatus status;
	byte                *portalfront;   /* [portals], preliminary */
	byte                *portalflood;   /* [portals], intermediate */
	byte                *portalvis;     /* [portals], final */

	int nummightsee;                    /* bit count on portalflood for sort */
	passage_t           *passages;      /* there are just as many passages as there */
	                                    /* are portals in the leaf this portal leads */
	                                    /* BehavEd vis1: an array, one per portal there that is not removed, in leaf order */
};


struct leaf_t
{
	int numportals;
	int merged;
	vportal_t           *portals[MAX_PORTALS_ON_LEAF];
};


struct pstack_t
{
	/* BehavEd vis1: [portalwords] from a buffer per depth (NRC: byte mightsee[ MAX_PORTALS / 8 ] here),
	   zero outside the words listed in mightwords - the flows go through these words only */
	uint64_t            *mightsee;
	uint16_t            *mightwords;    /* indices of the nonzero words of mightsee, ascending */
	int nummightwords;
	uint64_t            *mightmask;     /* [portalmaskwords]: bit w set = word w is in mightwords */
	pstack_t            *next;
	leaf_t              *leaf;
	vportal_t           *portal;        /* portal exiting */
	fixedWinding_t      *source;
	fixedWinding_t      *pass;

	fixedWinding_t windings[ 3 ];       /* source, pass, temp in any order */
	int freewindings[ 3 ];

	visPlane_t portalplane;
	int depth;
#ifdef SEPERATORCACHE
	visPlane_t seperators[ 2 ][ MAX_SEPERATORS ];
	int numseperators[ 2 ];
	/* BehavEd vis2s (geometry memo, visflow.cpp): the memo can tell the result of a portal without the
	   planes - numseperators counts the planes the frame has by NRC's rule, sephave those that are
	   really in seperators, sepedge the source edge the search for the next one starts at */
	int sephave[ 2 ];
	int sepedge[ 2 ];
#endif
	/* BehavEd vis2s: the windings in the geometry memo */
	uint32_t geo;                       /* memo id of ( source, pass ), 0: none */
	uint32_t geoslot;                   /* where the memo keeps their points */
	bool geofresh;                      /* the id is new: the memo knows nothing about it yet */
	bool lazy;                          /* source and pass are not here, only their id (BhedGeoWindings) */
	uint8_t geobefore[2];               /* lazy: planes in the two separator caches before this portal */
	pstack_t            *prev;          /* the frame above */
};


struct threaddata_t
{
	vportal_t           *base;
	int c_chains;
	struct BhedGeoMemo  *geo;           /* BehavEd vis2s: geometry memo of the thread */
	/* BehavEd vis3 (fixed rule, visflow.cpp g_bhedFixed): the flow counts the frames of its recursion (work);
	   its start plus this count is its clock, shown to the other threads in *now. It reads portalvis of the
	   flows that ended by its view. */
	uint64_t start;
	uint64_t work;
	uint64_t view;
	std::atomic<uint64_t> *now;
	std::atomic<uint64_t> *limit;
	struct BhedChain    *chain;
	pstack_t pstack_head;
};


/* commandline arguments */
inline bool fastvis;
inline bool noPassageVis;
inline bool passageVisOnly;
inline bool mergevis;
inline bool mergevisportals;
inline bool nosort;
inline bool saveprt;
inline bool hint;             /* ydnar */
/* BehavEd vis4: -bhedvisnrcplanes - the separating planes and the portal planes as NetRadiant-Custom
   makes them (visflow.cpp BhedSeperates, vis.cpp PlaneFromWinding): the .bsp of NRC's q3map2, with
   its gaps in the visibility where portal corners are nearly doubled */
inline bool bhedNrcPlanes;
/* BehavEd vis4: -bhedvisstrictplanes - a separating plane also has to hold the three points it is made from
   on their side within ON_EPSILON (visflow.cpp BhedSeperates) */
inline bool bhedStrictPlanes;

inline float farPlaneDist;                /* rr2do2, rf, mre, ydnar all contributed to this one... */
inline int farPlaneDistMode;


/* global variables */
inline int numportals;
inline int portalclusters;

inline vportal_t          *portals;
inline leaf_t             *leafs;

inline vportal_t          *faces;
inline leaf_t             *faceleafs;

inline int numfaces;

inline int leafbytes;
inline int portalbytes, portallongs;
inline int portalwords;               /* BehavEd vis1: portalbytes / 8 */
inline int portalmaskwords;           /* BehavEd vis1: ( portalwords + 63 ) / 64 */

extern vportal_t          *sorted_portals[ MAX_MAP_PORTALS * 2 ];
