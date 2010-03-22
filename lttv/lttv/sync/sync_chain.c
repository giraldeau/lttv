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

#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sync_chain.h"


GQueue processingModules= G_QUEUE_INIT;
GQueue matchingModules= G_QUEUE_INIT;
GQueue analysisModules= G_QUEUE_INIT;
GQueue moduleOptions= G_QUEUE_INIT;


static void floydWarshall(AllFactors* const allFactors, double*** const
	distances, unsigned int*** const predecessors);
static void getFactors(AllFactors* const allFactors, unsigned int** const
	predecessors, unsigned int* const references, const unsigned int traceNum,
	Factors* const factors);


/*
 * Call the statistics function of each module of a sync chain
 *
 * Args:
 *   syncState:    Container for synchronization data
 */
void printStats(SyncState* const syncState)
{
	if (syncState->processingModule->printProcessingStats != NULL)
	{
		syncState->processingModule->printProcessingStats(syncState);
	}
	if (syncState->matchingModule->printMatchingStats != NULL)
	{
		syncState->matchingModule->printMatchingStats(syncState);
	}
	if (syncState->analysisModule->printAnalysisStats != NULL)
	{
		syncState->analysisModule->printAnalysisStats(syncState);
	}
}


/*
 * Calculate the elapsed time between two timeval values
 *
 * Args:
 *   end:          end time, result is also stored in this structure
 *   start:        start time
 */
void timeDiff(struct timeval* const end, const struct timeval* const start)
{
		if (end->tv_usec >= start->tv_usec)
		{
			end->tv_sec-= start->tv_sec;
			end->tv_usec-= start->tv_usec;
		}
		else
		{
			end->tv_sec= end->tv_sec - start->tv_sec - 1;
			end->tv_usec= end->tv_usec - start->tv_usec + 1e6;
		}
}


/*
 * Calculate a resulting offset and drift for each trace.
 *
 * Traces are assembled in groups. A group is an "island" of nodes/traces that
 * exchanged messages. A reference is determined for each group by using a
 * shortest path search based on the accuracy of the approximation. This also
 * forms a tree of the best way to relate each node's clock to the reference's
 * based on the accuracy. Sometimes it may be necessary or advantageous to
 * propagate the factors through intermediary clocks. Resulting factors for
 * each trace are determined based on this tree.
 *
 * This part was not the focus of my research. The algorithm used here is
 * inexact in some ways:
 * 1) The reference used may not actually be the best one to use. This is
 *    because the accuracy is not corrected based on the drift during the
 *    shortest path search.
 * 2) The min and max factors are not propagated and are no longer valid.
 * 3) Approximations of different types (ACCURATE and APPROXIMATE) are compared
 *    together. The "accuracy" parameters of these have different meanings and
 *    are not readily comparable.
 *
 * Nevertheless, the result is satisfactory. You just can't tell "how much" it
 * is.
 *
 * Two alternative (and subtly different) ways of propagating factors to
 * preserve min and max boundaries have been proposed, see:
 * [Duda, A., Harrus, G., Haddad, Y., and Bernard, G.: Estimating global time
 * in distributed systems, Proc. 7th Int. Conf. on Distributed Computing
 * Systems, Berlin, volume 18, 1987] p.304
 *
 * [Jezequel, J.M., and Jard, C.: Building a global clock for observing
 * computations in distributed memory parallel computers, Concurrency:
 * Practice and Experience 8(1), volume 8, John Wiley & Sons, Ltd Chichester,
 * 1996, 32] Section 5; which is mostly the same as
 * [Jezequel, J.M.: Building a global time on parallel machines, Proceedings
 * of the 3rd International Workshop on Distributed Algorithms, LNCS, volume
 * 392, 136–147, 1989] Section 5
 *
 * Args:
 *   allFactors:   offset and drift between each pair of traces
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
GArray* reduceFactors(AllFactors* const allFactors)
{
	GArray* factors;
	double** distances;
	unsigned int** predecessors;
	double* distanceSums;
	unsigned int* references;
	unsigned int i, j;
	const unsigned int traceNb= allFactors->traceNb;

	// Solve the all-pairs shortest path problem using the Floyd-Warshall
	// algorithm
	floydWarshall(allFactors, &distances, &predecessors);

	/* Find the reference for each node
	 *
	 * First calculate, for each node, the sum of the distances to each other
	 * node it can reach.
	 *
	 * Then, go through each "island" of traces to find the trace that has the
	 * lowest distance sum. Assign this trace as the reference to each trace
	 * of the island.
	 */
	distanceSums= malloc(traceNb * sizeof(double));
	for (i= 0; i < traceNb; i++)
	{
		distanceSums[i]= 0.;
		for (j= 0; j < traceNb; j++)
		{
			distanceSums[i]+= distances[i][j];
		}
	}

	references= malloc(traceNb * sizeof(unsigned int));
	for (i= 0; i < traceNb; i++)
	{
		references[i]= UINT_MAX;
	}
	for (i= 0; i < traceNb; i++)
	{
		if (references[i] == UINT_MAX)
		{
			unsigned int reference;
			double distanceSumMin;

			// A node is its own reference by default
			reference= i;
			distanceSumMin= INFINITY;
			for (j= 0; j < traceNb; j++)
			{
				if (distances[i][j] != INFINITY && distanceSums[j] <
					distanceSumMin)
				{
					reference= j;
					distanceSumMin= distanceSums[j];
				}
			}
			for (j= 0; j < traceNb; j++)
			{
				if (distances[i][j] != INFINITY)
				{
					references[j]= reference;
				}
			}
		}
	}

	for (i= 0; i < traceNb; i++)
	{
		free(distances[i]);
	}
	free(distances);
	free(distanceSums);

	/* For each trace, calculate the factors based on their corresponding
	 * tree. The tree is rooted at the reference and the shortest path to each
	 * other nodes are the branches.
	 */
	factors= g_array_sized_new(FALSE, FALSE, sizeof(Factors),
		traceNb);
	g_array_set_size(factors, traceNb);
	for (i= 0; i < traceNb; i++)
	{
		getFactors(allFactors, predecessors, references, i, &g_array_index(factors,
				Factors, i));
	}

	for (i= 0; i < traceNb; i++)
	{
		free(predecessors[i]);
	}
	free(predecessors);
	free(references);

	return factors;
}


/*
 * Perform an all-source shortest path search using the Floyd-Warshall
 * algorithm.
 *
 * The algorithm is implemented accoding to the description here:
 * http://web.mit.edu/urban_or_book/www/book/chapter6/6.2.2.html
 *
 * Args:
 *   allFactors:   offset and drift between each pair of traces
 *   distances:    resulting matrix of the length of the shortest path between
 *                 two nodes. If there is no path between two nodes, the
 *                 length is INFINITY
 *   predecessors: resulting matrix of each node's predecessor on the shortest
 *                 path between two nodes
 */
static void floydWarshall(AllFactors* const allFactors, double*** const
	distances, unsigned int*** const predecessors)
{
	unsigned int i, j, k;
	const unsigned int traceNb= allFactors->traceNb;
	PairFactors** const pairFactors= allFactors->pairFactors;

	// Setup initial conditions
	*distances= malloc(traceNb * sizeof(double*));
	*predecessors= malloc(traceNb * sizeof(unsigned int*));
	for (i= 0; i < traceNb; i++)
	{
		(*distances)[i]= malloc(traceNb * sizeof(double));
		for (j= 0; j < traceNb; j++)
		{
			if (i == j)
			{
				g_assert(pairFactors[i][j].type == EXACT);

				(*distances)[i][j]= 0.;
			}
			else
			{
				if (pairFactors[i][j].type == ACCURATE ||
					pairFactors[i][j].type == APPROXIMATE)
				{
					(*distances)[i][j]= pairFactors[i][j].accuracy;
				}
				else if (pairFactors[j][i].type == ACCURATE ||
					pairFactors[j][i].type == APPROXIMATE)
				{
					(*distances)[i][j]= pairFactors[j][i].accuracy;
				}
				else
				{
					(*distances)[i][j]= INFINITY;
				}
			}
		}

		(*predecessors)[i]= malloc(traceNb * sizeof(unsigned int));
		for (j= 0; j < traceNb; j++)
		{
			if (i != j)
			{
				(*predecessors)[i][j]= i;
			}
			else
			{
				(*predecessors)[i][j]= UINT_MAX;
			}
		}
	}

	// Run the iterations
	for (k= 0; k < traceNb; k++)
	{
		for (i= 0; i < traceNb; i++)
		{
			for (j= 0; j < traceNb; j++)
			{
				double distanceMin;

				distanceMin= MIN((*distances)[i][j], (*distances)[i][k] +
					(*distances)[k][j]);

				if (distanceMin != (*distances)[i][j])
				{
					(*predecessors)[i][j]= (*predecessors)[k][j];
				}

				(*distances)[i][j]= distanceMin;
			}
		}
	}
}


/*
 * Cummulate the time correction factors to convert a node's time to its
 * reference's time.
 * This function recursively calls itself until it reaches the reference node.
 *
 * Args:
 *   allFactors:   offset and drift between each pair of traces
 *   predecessors: matrix of each node's predecessor on the shortest
 *                 path between two nodes
 *   references:   reference node for each node
 *   traceNum:     node for which to find the factors
 *   factors:      resulting factors
 */
static void getFactors(AllFactors* const allFactors, unsigned int** const
	predecessors, unsigned int* const references, const unsigned int traceNum,
	Factors* const factors)
{
	unsigned int reference;
	PairFactors** const pairFactors= allFactors->pairFactors;

	reference= references[traceNum];

	if (reference == traceNum)
	{
		factors->offset= 0.;
		factors->drift= 1.;
	}
	else
	{
		Factors previousVertexFactors;

		getFactors(allFactors, predecessors, references,
			predecessors[reference][traceNum], &previousVertexFactors);

		/* Convert the time from traceNum to reference;
		 * pairFactors[row][col] converts the time from col to row, invert the
		 * factors as necessary */

		if (pairFactors[reference][traceNum].approx != NULL)
		{
			factors->offset= previousVertexFactors.drift *
				pairFactors[reference][traceNum].approx->offset +
				previousVertexFactors.offset;
			factors->drift= previousVertexFactors.drift *
				pairFactors[reference][traceNum].approx->drift;
		}
		else if (pairFactors[traceNum][reference].approx != NULL)
		{
			factors->offset= previousVertexFactors.drift * (-1. *
				pairFactors[traceNum][reference].approx->offset /
				pairFactors[traceNum][reference].approx->drift) +
				previousVertexFactors.offset;
			factors->drift= previousVertexFactors.drift * (1. /
				pairFactors[traceNum][reference].approx->drift);
		}
		else
		{
			g_assert_not_reached();
		}
	}
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            ProcessingModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the processing module a's name is b
 */
gint gcfCompareProcessing(gconstpointer a, gconstpointer b)
{
	const ProcessingModule* processingModule;
	const char* name;

	processingModule= (const ProcessingModule*) a;
	name= (const char*) b;

	return strncmp(processingModule->name, name,
		strlen(processingModule->name) + 1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            MatchingModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the matching module a's name is b
 */
gint gcfCompareMatching(gconstpointer a, gconstpointer b)
{
	const MatchingModule* matchingModule;
	const char* name;

	matchingModule= (const MatchingModule*) a;
	name= (const char*) b;

	return strncmp(matchingModule->name, name, strlen(matchingModule->name) +
		1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            AnalysisModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the analysis module a's name is b
 */
gint gcfCompareAnalysis(gconstpointer a, gconstpointer b)
{
	const AnalysisModule* analysisModule;
	const char* name;

	analysisModule= (const AnalysisModule*) a;
	name= (const char*) b;

	return strncmp(analysisModule->name, name, strlen(analysisModule->name) +
		1);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Concatenate analysis module names.
 *
 * Args:
 *   data:         AnalysisModule*
 *   user_data:    GString*, concatenated names
 */
void gfAppendAnalysisName(gpointer data, gpointer user_data)
{
	g_string_append((GString*) user_data, ((AnalysisModule*) data)->name);
	g_string_append((GString*) user_data, ", ");
}
