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
	GraphFunction* writeTraceTracePlots;
	GraphFunction* writeTraceTraceOptions;
	/* This is for graphs where the data on the abscissa is in the range of
	 * timestamps and the ordinates is in the range of timestamp deltas */
	GraphFunction* writeTraceTimePlots;
	GraphFunction* writeTraceTimeOptions;
} GraphFunctions;


void writeGraphsScript(struct _SyncState* const syncState);

#endif
