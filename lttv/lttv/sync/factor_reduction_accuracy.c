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

#include <math.h>
#include <stdlib.h>

#include "sync_chain.h"

#include "factor_reduction_accuracy.h"


// Functions common to all reduction modules
static void initReductionAccuracy(SyncState* const syncState);
static void destroyReductionAccuracy(SyncState* const syncState);

static GArray* finalizeReductionAccuracy(SyncState* const syncState,
	AllFactors* allFactors);
static void printReductionStatsAccuracy(SyncState* const syncState);

// Functions specific to this module
static void floydWarshall(AllFactors* const allFactors, const unsigned int
	traceNb, double*** const distances, unsigned int*** const predecessors);
static void getFactors(AllFactors* const allFactors, unsigned int** const
	predecessors, unsigned int* const references, const unsigned int traceNum,
	Factors* const factors);


static ReductionModule reductionModuleAccuracy= {
	.name= "accuracy",
	.initReduction= &initReductionAccuracy,
	.destroyReduction= &destroyReductionAccuracy,
	.finalizeReduction= &finalizeReductionAccuracy,
	.printReductionStats= &printReductionStatsAccuracy,
	.graphFunctions= {},
};


/*
 * Reduction module registering function
 */
void registerReductionAccuracy()
{
	g_queue_push_tail(&reductionModules, &reductionModuleAccuracy);
}


/*
 * Reduction init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Allocate some reduction specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void initReductionAccuracy(SyncState* const syncState)
{
	if (syncState->stats)
	{
		syncState->reductionData= calloc(1, sizeof(ReductionStatsAccuracy));
	}
}


/*
 * Reduction destroy function
 *
 * Free the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void destroyReductionAccuracy(SyncState* const syncState)
{
	unsigned int i;

	if (syncState->stats)
	{
		ReductionStatsAccuracy* stats= syncState->reductionData;

		if (stats->predecessors)
		{
			for (i= 0; i < syncState->traceNb; i++)
			{
				free(stats->predecessors[i]);
			}
			free(stats->predecessors);
			free(stats->references);
		}
		free(stats);
	}
}


/*
 * Finalize the factor reduction
 *
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
 *   syncState     container for synchronization data.
 *   allFactors    offset and drift between each pair of traces
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace

 */
static GArray* finalizeReductionAccuracy(SyncState* const syncState,
	AllFactors* allFactors)
{
	GArray* factors;
	double** distances;
	unsigned int** predecessors;
	double* distanceSums;
	unsigned int* references;
	unsigned int i, j;

	// Solve the all-pairs shortest path problem using the Floyd-Warshall
	// algorithm
	floydWarshall(allFactors, syncState->traceNb, &distances, &predecessors);

	/* Find the reference for each node
	 *
	 * First calculate, for each node, the sum of the distances to each other
	 * node it can reach.
	 *
	 * Then, go through each "island" of traces to find the trace that has the
	 * lowest distance sum. Assign this trace as the reference to each trace
	 * of the island.
	 */
	distanceSums= malloc(syncState->traceNb * sizeof(double));
	for (i= 0; i < syncState->traceNb; i++)
	{
		distanceSums[i]= 0.;
		for (j= 0; j < syncState->traceNb; j++)
		{
			distanceSums[i]+= distances[i][j];
		}
	}

	references= malloc(syncState->traceNb * sizeof(unsigned int));
	for (i= 0; i < syncState->traceNb; i++)
	{
		references[i]= UINT_MAX;
	}
	for (i= 0; i < syncState->traceNb; i++)
	{
		if (references[i] == UINT_MAX)
		{
			unsigned int reference;
			double distanceSumMin;

			// A node is its own reference by default
			reference= i;
			distanceSumMin= INFINITY;
			for (j= 0; j < syncState->traceNb; j++)
			{
				if (distances[i][j] != INFINITY && distanceSums[j] <
					distanceSumMin)
				{
					reference= j;
					distanceSumMin= distanceSums[j];
				}
			}
			for (j= 0; j < syncState->traceNb; j++)
			{
				if (distances[i][j] != INFINITY)
				{
					references[j]= reference;
				}
			}
		}
	}

	for (i= 0; i < syncState->traceNb; i++)
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
		syncState->traceNb);
	g_array_set_size(factors, syncState->traceNb);
	for (i= 0; i < syncState->traceNb; i++)
	{
		getFactors(allFactors, predecessors, references, i, &g_array_index(factors,
				Factors, i));
	}

	if (syncState->stats)
	{
		ReductionStatsAccuracy* stats= syncState->reductionData;

		stats->predecessors= predecessors;
		stats->references= references;
	}
	else
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			free(predecessors[i]);
		}
		free(predecessors);
		free(references);
	}

	return factors;
}


/*
 * Print statistics related to reduction. Must be called after
 * finalizeReduction.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printReductionStatsAccuracy(SyncState* const syncState)
{
	unsigned int i;
	ReductionStatsAccuracy* stats= syncState->reductionData;

	printf("Accuracy-based factor reduction stats:\n");
	for (i= 0; i < syncState->traceNb; i++)
	{
		unsigned int reference= stats->references[i];

		if (i == reference)
		{
			printf("\ttrace %u is a reference\n", i);
		}
		else
		{
			printf("\ttrace %u: reference %u, predecessor %u\n", i,
				reference,
				stats->predecessors[reference][i]);
		}
	}
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
 *   traceNb:      number of traces
 *   distances:    resulting matrix of the length of the shortest path between
 *                 two nodes. If there is no path between two nodes, the
 *                 length is INFINITY. distances[i][j] is the length of the
 *                 path from i to j.
 *   predecessors: resulting matrix of each node's predecessor on the shortest
 *                 path between two nodes. predecessors[i][j] is the
 *                 predecessor to j on the path from i to j.
 */
static void floydWarshall(AllFactors* const allFactors, const unsigned int
	traceNb, double*** const distances, unsigned int*** const predecessors)
{
	unsigned int i, j, k;
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
