// bhed_vischeck.h - BehavEd vis3, vis4 (not in NetRadiant-Custom): -vis -bhedvischeck
//
// Checks the portal visibility the flows have computed, against what must hold whatever rule
// the flows read each other's vectors by (NRC's rule, -bhedvisexact, -bhedvisfixed):
//
//   * upper bound: a flow starts from portalflood and only takes bits away, so portalvis of a
//     portal is a subset of its portalflood;
//   * lower bound: a straight line that leaves a portal p into its leaf and passes from leaf
//     to leaf through the inside of portals - never through a wall - is a line of sight, so
//     every portal on it has to be in portalvis of p. The check draws such lines between
//     random points of p and of the portals in portalflood of p - every fourth to a portal
//     nearby instead, whatever the flood says - and walks them through the
//     portals of the leafs (which are convex): a portal on such a line that portalvis lacks
//     would be something visible that the game does not draw - if no other portal of its leaf sees
//     it either: the check counts both, the portals and those that lead into a cluster the leaf vis
//     lacks, and against a reference how many of them the reference has;
//   * against another run (BHED_VIS_CHECK_REF, written by that run with BHED_VIS_CHECK_DUMP):
//     the pairs only one of the two has are counted - two valid results may differ in both
//     directions, the separator cache of a frame is filled by the first portal that gets
//     there - and for every pair only the other run has, lines are drawn between exactly
//     these two portals: none may be found.
//
// The check judges by a shape of its own for every portal (the plane from all points of the winding,
// corners written twice taken as one), not by the planes of the flows - it counts those of them that
// miss their own winding.
// The lines keep a distance from the portal edges and from the planes, and their way through each
// leaf is longer than that distance (ON_EPSILON of the flows is 0.1: in a leaf thinner than that
// they take two portals for one plane) - so a line that is found is one the flows have to find as
// well.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

/* after the flows: checks portalvis of all portals and prints the counts (lines "BehavEd: vis check ...") */
void BhedVisCheck();
