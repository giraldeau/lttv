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

	void (*initAnalysis)(struct _SyncState* const syncState);
	void (*destroyAnalysis)(struct _SyncState* const syncState);

	void (*analyzeMessage)(struct _SyncState* const syncState, Message* const
		message);
	void (*analyzeExchange)(struct _SyncState* const syncState, Exchange* const
		exchange);
	void (*analyzeBroadcast)(struct _SyncState* const syncState, Broadcast* const
		broadcast);
	AllFactors* (*finalizeAnalysis)(struct _SyncState* const syncState);

	void (*printAnalysisStats)(struct _SyncState* const syncState);
	GraphFunctions graphFunctions;
} AnalysisModule;

#endif
