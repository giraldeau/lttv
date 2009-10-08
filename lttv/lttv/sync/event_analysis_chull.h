/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef EVENT_ANALYSIS_CHULL_H
#define EVENT_ANALYSIS_CHULL_H

#include <glib.h>

#include "data_structures.h"


typedef struct
{
	LttCycleCount x, y;
} Point;


typedef enum
{
	/* Used for identity factors (a0= 0, a1= 1) that map a trace to itself. In
	 * this case, min, max and accuracy are not initialized.
	 */
	EXACT,
	/* The approximation is the middle of the min and max limits, all fields
	 * are initialized.
	 */
	MIDDLE,
	/* min and max are not available because the hulls do not respect
	 * assumptions (hulls should not intersect and the upper half-hull should
	 * be below the lower half-hull). The approximation is a "best effort".
	 * All fields are initialized but min and max are NULL.
	 */
	FALLBACK,
	/* min or max is available but not both. The hulls respected assumptions
	 * but all receives took place after all sends or vice versa. approx and
	 * accuracy are not initialized.
	 */
	INCOMPLETE,
	/* The pair of trace did not have communications in both directions (maybe
	 * even no communication at all). approx and accuracy are not initialized.
	 */
	ABSENT,
	/* min and max are not available because the algorithms are screwed. One
	 * of min or max (but not both) is NULL. The other is initialized. Approx
	 * is not initialized.
	 */
	SCREWED,
} ApproxType;


typedef struct
{
	Factors* min, * max, * approx;
	ApproxType type;
	double accuracy;
} FactorsCHull;


typedef struct
{
	unsigned int dropped;

	/* FactorsCHull allFactors[traceNb][traceNb]
	 *
	 * allFactors is divided into three parts depending on the position of an
	 * element allFactors[i][j]:
	 *   Lower triangular part of the matrix
	 *     i > j
	 *     This contains the factors between nodes i and j. These factors
	 *     convert the time values of j to time values of i.
	 *   Diagonal part of the matrix
	 *     i = j
	 *     This contains identity factors (a0= 0, a1= 1).
	 *   Upper triangular part of the matrix
	 *     i < j
	 *     This area is not allocated.
	 */
	FactorsCHull** allFactors;
} AnalysisStatsCHull;


typedef struct
{
	/* This array contains file pointers to files where hull points x-y data
	 * is outputed. Each trace-pair has two files, one for each message
	 * direction. The structure of the array is the same as for hullArray,
	 * hullPoints[row][col] where:
	 *   row= inE->traceNum
	 *   col= outE->traceNum
	 *
	 * The elements on the diagonal are not initialized.
	 */
	FILE*** hullPoints;

	/* FactorsCHull allFactors[traceNb][traceNb]
	 * This is the same array as AnalysisStatsCHull.allFactors.
	 */
	FactorsCHull** allFactors;
} AnalysisGraphsDataCHull;


typedef struct
{
	/* Point hullArray[traceNb][traceNb][]
	 *
	 * A message comes from two traces. The lowest numbered trace is
	 * considered to be the reference clock, CA. The other is CB. The
	 * direction of messages (sent or received) is relative to CA. Points are
	 * formed such that their abscissa comes from CA and their ordinate from
	 * CB.
	 *
	 * hullArray is divided into three parts depending on the position of an
	 * element hullArray[i][j]:
	 *   Lower triangular part of the matrix
	 *     i > j
	 *     This contains the points that form lower hulls, therefore that
	 *     represent "sent" messages.
	 *   Upper triangular part of the matrix
	 *     i < j
	 *     This contains the points that form upper hulls, therefore that
	 *     represent "received" messages.
	 *   Diagonal part of the matrix
	 *     i = j
	 *     This contains empty lists
	 *
	 * When a message is such that:
	 *   inE->traceNum < outE->traceNum
	 *     CA is inE->traceNum, therefore the message was received and may
	 *     generate a point in the upper hull. Upper hulls are such that i <
	 *     j, therefore, i= inE->traceNum and j= outE->traceNum.
	 *   inE->traceNum > outE->traceNum
	 *     CA is outE->traceNum, therefore the message was sent and may
	 *     generate a point in the lower hull. Lower hulls are such that i >
	 *     j, therefore, i= inE->traceNum and j= outE->traceNum. Hence, we
	 *     always have:
	 *       i= inE->traceNum
	 *       j= outE->traceNum
	 *
	 * Do not let yourself get confused! Always remember that the "lower hull"
	 * is in fact the "lower half" of a hull. When assumptions are respected,
	 * the lower half is above the upper half.
	 */
	GQueue*** hullArray;

	AnalysisStatsCHull* stats;
	AnalysisGraphsDataCHull* graphsData;
} AnalysisDataCHull;

#endif
