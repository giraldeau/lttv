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
	/*
	 * These functions are called at the beginning of the gnuplot script and
	 * may writes variables that can be reused in the plot or options lines
	 */
	GraphVariableFunction* writeVariables;

	/*
	 * All "Back" functions are called, then all "Fore" functions. They add
	 * graphs to a gnuplot "plot" command. All "Options" functions are called.
	 * They can set options via the gnuplot "set" command. Finaly, a replot is
	 * performed. This is done so that options may be set using dynamic
	 * gnuplot variables like GPVAL_X_MIN
	 */
	/*
	 * These next three functions ("writeTraceTrace...") are for graphs where
	 * both axes are in the scale of timestamps.
	 */
	GraphFunction* writeTraceTraceForePlots;
	GraphFunction* writeTraceTraceBackPlots;
	GraphFunction* writeTraceTraceOptions;

	/*
	 * These next three functions ("writeTraceTime...") are for graphs where
	 * the abscissa are in the scale of timestamps and the ordinate in the
	 * scale of seconds.
	 */
	GraphFunction* writeTraceTimeForePlots;
	GraphFunction* writeTraceTimeBackPlots;
	GraphFunction* writeTraceTimeOptions;
} GraphFunctions;


FILE* createGraphsDir(const char* const graphsDir);
char* changeToGraphsDir(const char* const graphsDir);
void writeGraphsScript(struct _SyncState* const syncState);

#endif
