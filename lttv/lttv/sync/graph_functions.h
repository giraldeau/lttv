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

#ifndef GRAPH_FUNCTIONS_H
#define GRAPH_FUNCTIONS_H

struct _SyncState;

typedef void (GraphVariableFunction)(struct _SyncState* const syncState, const
	unsigned int i);
typedef void (GraphFunction)(struct _SyncState* const syncState, const
	unsigned int i, const unsigned int j);

typedef struct
{
	GraphVariableFunction* writeVariables;
	/* This is for graphs where the data on both axis is in the range of
	 * timestamps */
	GraphFunction* writeTraceTraceForePlots;
	GraphFunction* writeTraceTraceBackPlots;
	GraphFunction* writeTraceTraceOptions;
	/* This is for graphs where the data on the abscissa is in the range of
	 * timestamps and the ordinates is in the range of timestamp deltas */
	GraphFunction* writeTraceTimeForePlots;
	GraphFunction* writeTraceTimeBackPlots;
	GraphFunction* writeTraceTimeOptions;
} GraphFunctions;


FILE* createGraphsDir(const char* const graphsDir);
char* changeToGraphsDir(const char* const graphsDir);
void writeGraphsScript(struct _SyncState* const syncState);

#endif
