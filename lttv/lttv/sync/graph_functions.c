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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "sync_chain.h"
#include "graph_functions.h"


void writeGraphsScript(SyncState* const syncState)
{
	unsigned int i, j, k, l;
	long pos1, pos2;
	const GraphFunctions* moduleGraphFunctions[]= {
		&syncState->processingModule->graphFunctions,
		&syncState->matchingModule->graphFunctions,
		&syncState->analysisModule->graphFunctions,
	};
	const struct {
		size_t plotsOffset,
			   optionsOffset;
		char* name;
	} funcTypes[]= {
		{offsetof(GraphFunctions, writeTraceTracePlots),
			offsetof(GraphFunctions, writeTraceTraceOptions), "TraceTrace"},
		{offsetof(GraphFunctions, writeTraceTimePlots),
			offsetof(GraphFunctions, writeTraceTimeOptions), "TraceTime"},
	};

	fprintf(syncState->graphsStream, "\n");
	pos1= ftell(syncState->graphsStream);
	for (i= 0; i < syncState->traceNb; i++)
	{
		for (k= 0; k < sizeof(moduleGraphFunctions) /
			sizeof(*moduleGraphFunctions); k++)
		{
			GraphVariableFunction** writeVariables= (void*)
				moduleGraphFunctions[k] + offsetof(GraphFunctions,
					writeVariables);

			if (*writeVariables)
			{
				(*writeVariables)(syncState, i);
			}
		}
	}
	fflush(syncState->graphsStream);
	pos2= ftell(syncState->graphsStream);
	if (pos1 != pos2)
	{
		fprintf(syncState->graphsStream, "\n");
	}

	for (l= 0; l < sizeof(funcTypes) / sizeof(*funcTypes); l++)
	{
		// Cover the upper triangular matrix, i is the reference node.
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= i + 1; j < syncState->traceNb; j++)
			{
				long trunc;

				fprintf(syncState->graphsStream,
					"reset\n"
					"set output \"%03d-%03d-%s.eps\"\n"
					"plot \\\n", i, j, funcTypes[l].name);

				pos1= ftell(syncState->graphsStream);

				for (k= 0; k < sizeof(moduleGraphFunctions) /
					sizeof(*moduleGraphFunctions); k++)
				{
					GraphFunction** writePlots= (void*)
						moduleGraphFunctions[k] + funcTypes[l].plotsOffset;

					if (*writePlots)
					{
						(*writePlots)(syncState, i, j);
					}
				}

				fflush(syncState->graphsStream);
				pos2= ftell(syncState->graphsStream);
				if (pos1 != pos2)
				{
					// Remove the ", \\\n" from the last graph plot line
					trunc= pos2 - 4;
				}
				else
				{
					// Remove the "plot \\\n" line to avoid creating an invalid
					// gnuplot script
					trunc= pos2 - 7;
				}

				if (ftruncate(fileno(syncState->graphsStream), trunc) == -1)
				{
					g_error(strerror(errno));
				}
				if (fseek(syncState->graphsStream, 0, SEEK_END) == -1)
				{
					g_error(strerror(errno));
				}

				fprintf(syncState->graphsStream,
					"\nset output \"%03d-%03d-%s.eps\"\n"
					"set title \"\"\n", i, j, funcTypes[l].name);

				for (k= 0; k < sizeof(moduleGraphFunctions) /
					sizeof(*moduleGraphFunctions); k++)
				{
					GraphFunction** writeOptions= (void*)
						moduleGraphFunctions[k] + funcTypes[l].optionsOffset;

					if (*writeOptions)
					{
						(*writeOptions)(syncState, i, j);
					}
				}

				if (pos1 != pos2)
				{
					fprintf(syncState->graphsStream, "replot\n\n");
				}
				else
				{
					fprintf(syncState->graphsStream, "\n");
				}
			}
		}
	}
}
