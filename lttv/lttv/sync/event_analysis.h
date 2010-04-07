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

#ifndef EVENT_ANALYSIS_H
#define EVENT_ANALYSIS_H

#include <glib.h>
#include <stdio.h>

#include "data_structures.h"
#include "graph_functions.h"


struct _SyncState;

typedef struct
{
	char* name;

	/*
	 * This function is called at the beginning of a synchronization run for a set
	 * of traces. Allocate analysis specific data structures.
	 */
	void (*initAnalysis)(struct _SyncState* const syncState);

	/*
	 * Free the analysis specific data structures.
	 */
	void (*destroyAnalysis)(struct _SyncState* const syncState);

	/*
	 * Perform analysis on an event pair.
	 */
	void (*analyzeMessage)(struct _SyncState* const syncState, Message* const
		message);

	/*
	 * Perform analysis on multiple messages.
	 */
	void (*analyzeExchange)(struct _SyncState* const syncState, Exchange* const
		exchange);

	/*
	 * Perform analysis on muliple events.
	 */
	void (*analyzeBroadcast)(struct _SyncState* const syncState, Broadcast* const
		broadcast);

	/*
	 * Finalize the factor calculations. Return synchronization factors
	 * between trace pairs.
	 */
	AllFactors* (*finalizeAnalysis)(struct _SyncState* const syncState);

	/*
	 * Print statistics related to analysis. Is always called after
	 * finalizeAnalysis.
	 */
	void (*printAnalysisStats)(struct _SyncState* const syncState);

	/*
	 * Write the analysis-specific options and graph commands in the gnuplot
	 * script. Is always called after finalizeAnalysis.
	 */
	GraphFunctions graphFunctions;
} AnalysisModule;

#endif
