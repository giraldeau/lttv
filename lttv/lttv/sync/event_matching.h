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

#ifndef EVENT_MATCHING_H
#define EVENT_MATCHING_H

#include <glib.h>

#include "data_structures.h"
#include "graph_functions.h"

struct _SyncState;

typedef struct
{
	char* name;
	bool canMatch[TYPE_COUNT];

	/*
	 * This function is called at the beginning of a synchronization run for a set
	 * of traces. Allocate the matching specific data structures.
	 */
	void (*initMatching)(struct _SyncState* const syncState);

	/*
	 * Free the matching specific data structures.
	 */
	void (*destroyMatching)(struct _SyncState* const syncState);

	/*
	 * Try to match one event from a trace with the corresponding event from
	 * another trace. If it is possible, create a new structure and call the
	 * analyse{message,exchange,broadcast} function of the analysis module.
	 */
	void (*matchEvent)(struct _SyncState* const syncState, Event* const
		event);

	/*
	 * Obtain the factors from downstream.
	 */
	AllFactors* (*finalizeMatching)(struct _SyncState* const syncState);

	/*
     * Print statistics related to matching. Is always called after
     * finalizeMatching.
     */
	void (*printMatchingStats)(struct _SyncState* const syncState);

	/*
	 * Write the matching-specific options and graph commands in the gnuplot
	 * script. Is always called after finalizeMatching.
	 */
	GraphFunctions graphFunctions;
} MatchingModule;

#endif
