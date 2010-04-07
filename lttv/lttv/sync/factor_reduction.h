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

#ifndef FACTOR_REDUCTION_H
#define FACTOR_REDUCTION_H

#include "data_structures.h"
#include "graph_functions.h"


struct _SyncState;

typedef struct
{
	char* name;

	/*
	 * This function is called at the beginning of a synchronization run for a
	 * set of traces. Allocate some reduction specific data structures.
	 */
	void (*initReduction)(struct _SyncState* const syncState);

	/*
	 * Free the reduction specific data structures
	 */
	void (*destroyReduction)(struct _SyncState* const syncState);

	/*
	 * Convert trace pair synchronization factors to a resulting offset and
	 * drift for each trace.
	 */
	GArray* (*finalizeReduction)(struct _SyncState* const syncState,
		AllFactors* allFactors);

	/*
	 * Print statistics related to reduction. Is always called after
	 * finalizeReduction.
	 */
	void (*printReductionStats)(struct _SyncState* const syncState);

	/*
	 * Write the reduction-specific options and graph commands in the gnuplot
	 * script. Is always called after finalizeReduction.
	 */
	GraphFunctions graphFunctions;
} ReductionModule;

#endif
