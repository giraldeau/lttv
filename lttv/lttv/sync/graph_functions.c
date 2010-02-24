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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sync_chain.h"
#include "graph_functions.h"


/*
 * Create the directory used to hold graphs and the header of the gnuplot
 * script.
 *
 * Args:
 *   graphsDir:    name of directory
 *
 * Returns:
 *   The current working directory before the execution of the function. The
 *   string must be free'd by the caller.
 */
FILE* createGraphsDir(const char* const graphsDir)
{
	char* cwd;
	int graphsFp;
	FILE* result;
	int retval;

	cwd= changeToGraphsDir(graphsDir);

	if ((graphsFp= open("graphs.gnu", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR |
				S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH
				| S_IWOTH | S_IXOTH)) == -1)
	{
		g_error(strerror(errno));
	}
	if ((result= fdopen(graphsFp, "w")) == NULL)
	{
		g_error(strerror(errno));
	}

	fprintf(result,
		"#!/usr/bin/gnuplot\n\n"
		"set terminal postscript eps color size 8in,6in\n");

	retval= chdir(cwd);
	if (retval == -1)
	{
		g_error(strerror(errno));
	}
	free(cwd);

	return result;
}


/*
 * Change to the directory used to hold graphs. Create it if necessary.
 *
 * Args:
 *   graphsDir:    name of directory
 *
 * Returns:
 *   The current working directory before the execution of the function. The
 *   string must be free'd by the caller.
 */
char* changeToGraphsDir(const char* const graphsDir)
{
	int retval;
	char* cwd;

	cwd= getcwd(NULL, 0);
	if (cwd == NULL)
	{
		g_error(strerror(errno));
	}
	while ((retval= chdir(graphsDir)) != 0)
	{
		if (errno == ENOENT)
		{
			retval= mkdir(graphsDir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
				S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
			if (retval != 0)
			{
				g_error(strerror(errno));
			}
		}
		else
		{
			g_error(strerror(errno));
		}
	}

	return cwd;
}


/*
 * Call each graph variable, option and plot line function of each module to
 * produce a gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
void writeGraphsScript(SyncState* const syncState)
{
	unsigned int i, j, k, l, m;
	long pos1, pos2;
	const GraphFunctions* moduleGraphFunctions[]= {
		&syncState->processingModule->graphFunctions,
		&syncState->matchingModule->graphFunctions,
		&syncState->analysisModule->graphFunctions,
	};
	const struct {
		char* name;
		size_t plotsOffsets[2];
		size_t optionsOffset;
	} graphTypes[]= {
		{
			"TraceTrace",
			{
				offsetof(GraphFunctions, writeTraceTraceBackPlots),
				offsetof(GraphFunctions, writeTraceTraceForePlots),
			},
			offsetof(GraphFunctions, writeTraceTraceOptions),
		},
		{
			"TraceTime",
			{
				offsetof(GraphFunctions, writeTraceTimeBackPlots),
				offsetof(GraphFunctions, writeTraceTimeForePlots),
			},
			offsetof(GraphFunctions, writeTraceTimeOptions),
		},
	};

	fprintf(syncState->graphsStream, "\n");

	// Write variables
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

	// Write plots and options
	for (l= 0; l < sizeof(graphTypes) / sizeof(*graphTypes); l++)
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
					"plot \\\n", i, j, graphTypes[l].name);

				pos1= ftell(syncState->graphsStream);

				for (m= 0; m < sizeof(graphTypes[l].plotsOffsets) /
					sizeof(*graphTypes[l].plotsOffsets); m++)
				{
					for (k= 0; k < sizeof(moduleGraphFunctions) /
						sizeof(*moduleGraphFunctions); k++)
					{
						GraphFunction** writePlots= (void*)
							moduleGraphFunctions[k] +
							graphTypes[l].plotsOffsets[m];

						if (*writePlots)
						{
							(*writePlots)(syncState, i, j);
						}
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
					"set title \"\"\n", i, j, graphTypes[l].name);

				for (k= 0; k < sizeof(moduleGraphFunctions) /
					sizeof(*moduleGraphFunctions); k++)
				{
					GraphFunction** writeOptions= (void*)
						moduleGraphFunctions[k] + graphTypes[l].optionsOffset;

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
