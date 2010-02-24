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

#ifndef EVENT_MATCHING_BROADCAST_H
#define EVENT_MATCHING_BROADCAST_H

#include <glib.h>

#include "data_structures.h"


typedef struct
{
	unsigned int totComplete,
		totReceive,
		totIncomplete,
		totTransmit;
} MatchingStatsBroadcast;

typedef struct
{
	/* This array is used for graphs. It contains file pointers to files where
	 * broadcast differential delay points are output.
	 *
	 * accuracyPoints is divided into three parts depending on the position of an
     * element accuracyPoints[i][j]:
     *   Lower triangular part of the matrix
     *     i > j
     *     This contains the difference t[i] - t[j] between the times when
	 *     a broadcast was received in trace i and trace j.
     *   Diagonal part of the matrix
     *     i = j
     *     This area is not allocated.
     *   Upper triangular part of the matrix
     *     i < j
     *     This area is not allocated.
	 */
	FILE*** accuracyPoints;

	// pointsNb[traceNum][traceNum] has the same structure as accuracyPoints
	unsigned int** pointsNb;
} MatchingGraphsBroadcast;

typedef struct
{
	// Broadcast* pendingBroadcasts[dataStart]
	GHashTable* pendingBroadcasts;

	MatchingStatsBroadcast* stats;
	MatchingGraphsBroadcast* graphs;
} MatchingDataBroadcast;

void registerMatchingBroadcast();

#endif
