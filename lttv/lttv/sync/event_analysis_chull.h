/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009, 2010 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENT_ANALYSIS_CHULL_H
#define EVENT_ANALYSIS_CHULL_H

#include <glib.h>

#include "data_structures.h"

#ifdef HAVE_LIBGLPK
#include <glpk.h>
#endif

typedef struct
{
	uint64_t x, y;
} Point;


typedef struct
{
	unsigned int dropped;

	/* geoFactors is divided into three parts depending on the position of an
	 * element geoFactors->pairFactors[i][j]:
	 *   Lower triangular part of the matrix
	 *     i > j
	 *     This contains the factors between nodes i and j. These factors
	 *     convert the time values of j to time values of i.
	 *   Diagonal part of the matrix
	 *     i = j
	 *     This contains identity factors (a0= 0, a1= 1).
	 *   Upper triangular part of the matrix
	 *     i < j
	 *     These factors are absent
	 *
	 * Factor types are used as follows:
	 * EXACT,
	 * Used for identity factors (a0= 0, a1= 1) that map a trace to itself. In
	 * this case, min, max and accuracy are not initialized.
	 *
	 * ACCURATE,
	 * The approximation is the middle of the min and max limits.
	 *
	 * APPROXIMATE,
	 * min and max are not available because the hulls do not respect
	 * assumptions (hulls should not intersect and the upper half-hull should
	 * be below the lower half-hull). The approximation is a "best effort".
	 * All fields are initialized but min and max are NULL.
	 *
	 * INCOMPLETE,
	 * min or max is available but not both. The hulls respected assumptions
	 * but all receives took place after all sends or vice versa.
	 *
	 * ABSENT,
	 * The pair of trace did not have communications in both directions (maybe
	 * even no communication at all). Also used for factors in the upper
	 * triangular matrix.
	 *
	 * FAIL,
	 * min and max are not available because the algorithms are defective. One
	 * of min or max (but not both) is NULL. The other is initialized.
	 */
	AllFactors* geoFactors;

#ifdef HAVE_LIBGLPK
	/* Synchronization factors, as calculated via LP, for comparison. Same
	 * structure as geoFactors.
	 *
	 * Factor types are used as follows:
	 * EXACT,
	 * Used for identity factors (a0= 0, a1= 1) that map a trace to itself. In
	 * this case, min, max and accuracy are not initialized.
	 *
	 * ACCURATE,
	 * The approximation is the middle of the min and max limits.
	 *
	 * INCOMPLETE,
	 * min or max is available but not both. The hulls respected assumptions
	 * but all receives took place after all sends or vice versa.
	 *
	 * ABSENT,
	 * The pair of trace did not have communications in both directions (maybe
	 * even no communication at all). Also used when the hulls do not respect
	 * assumptions. Also used for factors in the upper triangular matrix.
	 */
	AllFactors* lpFactors;
#endif

	/* This is AnalysisStatsCHull.lpFactors if it is available or else
	 * AnalysisStatsCHull.geoFactors.
	 */
	AllFactors* allFactors;
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

	/* This is AnalysisStatsCHull.lpFactors if it is available or else
	 * AnalysisStatsCHull.geoFactors.
	 */
	AllFactors* allFactors;

#ifdef HAVE_LIBGLPK
	/* This is the same array as AnalysisStatsCHull.lpFactors.
	 */
	AllFactors* lpFactors;
#endif
} AnalysisGraphsDataCHull;


typedef struct
{
	/* Point* hullArray[traceNb][traceNb][]
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

#ifdef HAVE_LIBGLPK
	/* glp_prob* lps[traceNum][traceNum]
	 *
	 * Only the lower triangular part of the matrix is allocated, that is
	 * lps[i][j] where i > j */
	glp_prob*** lps;
#endif

	AnalysisStatsCHull* stats;
	AnalysisGraphsDataCHull* graphsData;
} AnalysisDataCHull;

void registerAnalysisCHull();

#endif
