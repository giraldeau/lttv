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

// for INFINITY in math.h
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sync_chain.h"

#include "event_analysis_linreg.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


// Functions common to all analysis modules
static void initAnalysisLinReg(SyncState* const syncState);
static void destroyAnalysisLinReg(SyncState* const syncState);

static void analyzeExchangeLinReg(SyncState* const syncState, Exchange* const exchange);
static GArray* finalizeAnalysisLinReg(SyncState* const syncState);
static void printAnalysisStatsLinReg(SyncState* const syncState);
static void writeAnalysisGraphsPlotsLinReg(SyncState* const syncState, const
	unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerAnalysisLinReg() __attribute__((constructor (102)));

static void finalizeLSA(SyncState* const syncState);
static void doGraphProcessing(SyncState* const syncState);
static GArray* calculateFactors(SyncState* const syncState);
static void shortestPath(Fit* const* const fitArray, const unsigned int
	traceNum, const unsigned int traceNb, double* const distances,
	unsigned int* const previousVertex);
static double sumDistances(const double* const distances, const unsigned int
	traceNb);
static void reduceFactors(Fit* const* const fitArray, const unsigned int* const
	previousVertex, const unsigned int traceNum, double* const drift,
	double* const offset, double* const stDev);

// Graph-related Glib functions
static void gfGraphDestroy(gpointer data, gpointer user_data);
static gint gcfGraphTraceCompare(gconstpointer a, gconstpointer b);


static AnalysisModule analysisModuleLinReg= {
	.name= "linreg",
	.initAnalysis= &initAnalysisLinReg,
	.destroyAnalysis= &destroyAnalysisLinReg,
	.analyzeExchange= &analyzeExchangeLinReg,
	.finalizeAnalysis= &finalizeAnalysisLinReg,
	.printAnalysisStats= &printAnalysisStatsLinReg,
	.graphFunctions= {
		.writeTraceTracePlots= &writeAnalysisGraphsPlotsLinReg,
	}
};


/*
 * Analysis module registering function
 */
static void registerAnalysisLinReg()
{
	g_queue_push_tail(&analysisModules, &analysisModuleLinReg);
}


/*
 * Analysis init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Allocate some of the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function allocates these analysisData members:
 *                 fitArray
 *                 stDev
 */
static void initAnalysisLinReg(SyncState* const syncState)
{
	unsigned int i;
	AnalysisDataLinReg* analysisData;

	analysisData= malloc(sizeof(AnalysisDataLinReg));
	syncState->analysisData= analysisData;

	analysisData->fitArray= malloc(syncState->traceNb * sizeof(Fit*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->fitArray[i]= calloc(syncState->traceNb, sizeof(Fit));
	}

	if (syncState->stats)
	{
		analysisData->stDev= malloc(sizeof(double) * syncState->traceNb);
	}
}


/*
 * Analysis destroy function
 *
 * Free the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function deallocates these analysisData members:
 *                 fitArray
 *                 graphList
 *                 stDev
 */
static void destroyAnalysisLinReg(SyncState* const syncState)
{
	unsigned int i;
	AnalysisDataLinReg* analysisData;

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;

	if (analysisData == NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		free(analysisData->fitArray[i]);
	}
	free(analysisData->fitArray);

	g_queue_foreach(analysisData->graphList, &gfGraphDestroy, NULL);
	g_queue_free(analysisData->graphList);

	if (syncState->stats)
	{
		free(analysisData->stDev);
	}

	free(syncState->analysisData);
	syncState->analysisData= NULL;
}


/*
 * Perform analysis on a series of event pairs.
 *
 * Args:
 *   syncState     container for synchronization data
 *   exchange      structure containing the many events
 */
static void analyzeExchangeLinReg(SyncState* const syncState, Exchange* const exchange)
{
	unsigned int ni, nj;
	double dji, eji;
	double timoy;
	Fit* fit;
	Message* ackedMessage;
	AnalysisDataLinReg* analysisData;

	g_debug("Synchronization calculation, ");
	g_debug("%d acked packets - using last one, ",
		g_queue_get_length(exchange->acks));

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;
	ackedMessage= g_queue_peek_tail(exchange->acks);

	// Calculate the intermediate values for the
	// least-squares analysis
	dji= ((double) ackedMessage->inE->cpuTime - (double) ackedMessage->outE->cpuTime
		+ (double) exchange->message->outE->cpuTime - (double)
		exchange->message->inE->cpuTime) / 2;
	eji= fabs((double) ackedMessage->inE->cpuTime - (double)
		ackedMessage->outE->cpuTime - (double) exchange->message->outE->cpuTime +
		(double) exchange->message->inE->cpuTime) / 2;
	timoy= ((double) ackedMessage->outE->cpuTime + (double)
		exchange->message->inE->cpuTime) / 2;
	ni= ackedMessage->outE->traceNum;
	nj= ackedMessage->inE->traceNum;
	fit= &analysisData->fitArray[nj][ni];

	fit->n++;
	fit->st+= timoy;
	fit->st2+= pow(timoy, 2);
	fit->sd+= dji;
	fit->sd2+= pow(dji, 2);
	fit->std+= timoy * dji;

	g_debug("intermediate values: dji= %f ti moy= %f "
		"ni= %u nj= %u fit: n= %u st= %f st2= %f sd= %f "
		"sd2= %f std= %f, ", dji, timoy, ni, nj, fit->n,
		fit->st, fit->st2, fit->sd, fit->sd2, fit->std);
}


/*
 * Finalize the factor calculations
 *
 * The least squares analysis is finalized and finds the factors directly
 * between each pair of traces that had events together. The traces are
 * aranged in a graph, a reference trace is chosen and the factors between
 * this reference and every other trace is calculated. Sometimes it is
 * necessary to use many graphs when there are "islands" of independent
 * traces.
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* finalizeAnalysisLinReg(SyncState* const syncState)
{
	GArray* factors;

	// Finalize the processing
	finalizeLSA(syncState);

	// Find a reference node and structure nodes in a graph
	doGraphProcessing(syncState);

	/* Calculate the resulting offset and drift between each trace and its
	 * reference
	 */
	factors= calculateFactors(syncState);

	return factors;
}


/*
 * Print statistics related to analysis. Must be called after
 * finalizeAnalysis.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printAnalysisStatsLinReg(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataLinReg* analysisData;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;

	printf("Linear regression analysis stats:\n");

	printf("\tIndividual synchronization factors:\n");

	for (j= 0; j < syncState->traceNb; j++)
	{
		for (i= 0; i < j; i++)
		{
			Fit* fit;

			fit= &analysisData->fitArray[j][i];
			printf("\t\t%3d - %-3d: ", i, j);
			printf("a0= % 7g a1= 1 %c %7g accuracy %7g\n", fit->d0, fit->x <
				0.  ? '-' : '+', fabs(fit->x), fit->e);

			fit= &analysisData->fitArray[i][j];
			printf("\t\t%3d - %-3d: ", j, i);
			printf("a0= % 7g a1= 1 %c %7g accuracy %7g\n", fit->d0, fit->x <
				0.  ? '-' : '+', fabs(fit->x), fit->e);
		}
	}

	printf("\tTree:\n");
	for (i= 0; i < syncState->traceNb; i++)
	{
		GList* result;

		result= g_queue_find_custom(analysisData->graphList, &i,
			&gcfGraphTraceCompare);
		if (result != NULL)
		{
			Graph* graph;

			graph= (Graph*) result->data;

			printf("\t\ttrace %u reference %u previous vertex ", i,
				graph->reference);

			if (i == graph->reference)
			{
				printf("- ");
			}
			else
			{
				printf("%u ", graph->previousVertex[i]);
			}

			printf("stdev= %g\n", analysisData->stDev[i]);
		}
		else
		{
			g_error("Trace %d not part of a graph\n", i);
		}
	}
}


/*
 * Finalize the least-squares analysis. The intermediate values in the fit
 * array are used to calculate the drift and the offset between each pair of
 * nodes based on their exchanges.
 *
 * Args:
 *   syncState:    container for synchronization data.
 */
static void finalizeLSA(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataLinReg* analysisData;

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				Fit* fit;
				double delta;

				fit= &analysisData->fitArray[i][j];

				delta= fit->n * fit->st2 - pow(fit->st, 2);
				fit->x= (fit->n * fit->std - fit->st * fit->sd) / delta;
				fit->d0= (fit->st2 * fit->sd - fit->st * fit->std) / delta;
				fit->e= sqrt((fit->sd2 - (fit->n * pow(fit->std, 2) +
							pow(fit->sd, 2) * fit->st2 - 2 * fit->st * fit->sd
							* fit->std) / delta) / (fit->n - 2));

				g_debug("[i= %u j= %u]\n", i, j);
				g_debug("n= %d st= %g st2= %g sd= %g sd2= %g std= %g\n",
					fit->n, fit->st, fit->st2, fit->sd, fit->sd2, fit->std);
				g_debug("xij= %g d0ij= %g e= %g\n", fit->x, fit->d0, fit->e);
				g_debug("(xji= %g d0ji= %g)\n", -fit->x / (1 + fit->x),
					-fit->d0 / (1 + fit->x));
			}
		}
	}
}


/*
 * Structure nodes in graphs of nodes that had exchanges. Each graph has a
 * reference node, the one that can reach the others with the smallest
 * cummulative error.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function allocates these analysisData members:
 *                 graphList
 */
static void doGraphProcessing(SyncState* const syncState)
{
	unsigned int i, j;
	double* distances;
	unsigned int* previousVertex;
	AnalysisDataLinReg* analysisData;

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;

	distances= malloc(syncState->traceNb * sizeof(double));
	previousVertex= malloc(syncState->traceNb * sizeof(unsigned int));
	analysisData->graphList= g_queue_new();

	for (i= 0; i < syncState->traceNb; i++)
	{
		double errorSum;
		GList* result;

		// Perform shortest path search
		g_debug("shortest path trace %d\ndistances: ", i);
		shortestPath(analysisData->fitArray, i, syncState->traceNb, distances,
			previousVertex);

		for (j= 0; j < syncState->traceNb; j++)
		{
			g_debug("%g, ", distances[j]);
		}
		g_debug("\npreviousVertex: ");
		for (j= 0; j < syncState->traceNb; j++)
		{
			g_debug("%u, ", previousVertex[j]);
		}
		g_debug("\n");

		// Group in graphs nodes that have exchanges
		errorSum= sumDistances(distances, syncState->traceNb);
		result= g_queue_find_custom(analysisData->graphList, &i,
			&gcfGraphTraceCompare);
		if (result != NULL)
		{
			Graph* graph;

			g_debug("found graph\n");
			graph= (Graph*) result->data;
			if (errorSum < graph->errorSum)
			{
				g_debug("adding to graph\n");
				graph->errorSum= errorSum;
				free(graph->previousVertex);
				graph->previousVertex= previousVertex;
				graph->reference= i;
				previousVertex= malloc(syncState->traceNb * sizeof(unsigned
						int));
			}
		}
		else
		{
			Graph* newGraph;

			g_debug("creating new graph\n");
			newGraph= malloc(sizeof(Graph));
			newGraph->errorSum= errorSum;
			newGraph->previousVertex= previousVertex;
			newGraph->reference= i;
			previousVertex= malloc(syncState->traceNb * sizeof(unsigned int));

			g_queue_push_tail(analysisData->graphList, newGraph);
		}
	}

	free(previousVertex);
	free(distances);
}


/*
 * Calculate the resulting offset and drift between each trace and its
 * reference.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* calculateFactors(SyncState* const syncState)
{
	unsigned int i;
	AnalysisDataLinReg* analysisData;
	GArray* factors;

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;
	factors= g_array_sized_new(FALSE, FALSE, sizeof(Factors),
		syncState->traceNb);
	g_array_set_size(factors, syncState->traceNb);

	// Calculate the resulting offset and drift between each trace and its
	// reference
	for (i= 0; i < syncState->traceNb; i++)
	{
		GList* result;

		result= g_queue_find_custom(analysisData->graphList, &i,
			&gcfGraphTraceCompare);
		if (result != NULL)
		{
			Graph* graph;
			double stDev;
			Factors* traceFactors;

			graph= (Graph*) result->data;
			traceFactors= &g_array_index(factors, Factors, i);

			reduceFactors(analysisData->fitArray, graph->previousVertex, i,
				&traceFactors->drift, &traceFactors->offset, &stDev);

			if (syncState->stats)
			{
				analysisData->stDev[i]= stDev;
			}
		}
		else
		{
			g_error("Trace %d not part of a graph\n", i);
		}
	}

	return factors;
}


/*
 * Single-source shortest path search to find the path with the lowest error to
 * convert one node's time to another.
 * Uses Dijkstra's algorithm
 *
 * Args:
 *   fitArray:     array with the regression parameters
 *   traceNum:     reference node
 *   traceNb:      number of traces = number of nodes
 *   distances:    array of computed distance from source node to node i,
 *                 INFINITY if i is unreachable, preallocated to the number of
 *                 nodes
 *   previousVertex: previous vertex from a node i on the way to the source,
 *                 UINT_MAX if i is not on the way or is the source,
 *                 preallocated to the number of nodes
 */
static void shortestPath(Fit* const* const fitArray, const unsigned int
	traceNum, const unsigned int traceNb, double* const distances, unsigned
	int* const previousVertex)
{
	bool* visited;
	unsigned int i, j;

	visited= malloc(traceNb * sizeof(bool));

	for (i= 0; i < traceNb; i++)
	{
		const Fit* fit;

		visited[i]= false;

		fit= &fitArray[traceNum][i];
		g_debug("fitArray[traceNum= %u][i= %u]->n = %u\n", traceNum, i, fit->n);
		if (fit->n > 0)
		{
			distances[i]= fit->e;
			previousVertex[i]= traceNum;
		}
		else
		{
			distances[i]= INFINITY;
			previousVertex[i]= UINT_MAX;
		}
	}
	visited[traceNum]= true;

	for (j= 0; j < traceNb; j++)
	{
		g_debug("(%d, %u, %g), ", visited[j], previousVertex[j], distances[j]);
	}
	g_debug("\n");

	for (i= 0; i < traceNb - 2; i++)
	{
		unsigned int v;
		double dvMin;

		dvMin= INFINITY;
		for (j= 0; j < traceNb; j++)
		{
			if (visited[j] == false && distances[j] < dvMin)
			{
				v= j;
				dvMin= distances[j];
			}
		}

		g_debug("v= %u dvMin= %g\n", v, dvMin);

		if (dvMin != INFINITY)
		{
			visited[v]= true;

			for (j= 0; j < traceNb; j++)
			{
				const Fit* fit;

				fit= &fitArray[v][j];
				if (visited[j] == false && fit->n > 0 && distances[v] + fit->e
					< distances[j])
				{
					distances[j]= distances[v] + fit->e;
					previousVertex[j]= v;
				}
			}
		}
		else
		{
			break;
		}

		for (j= 0; j < traceNb; j++)
		{
			g_debug("(%d, %u, %g), ", visited[j], previousVertex[j], distances[j]);
		}
		g_debug("\n");
	}

	free(visited);
}


/*
 * Cummulate the distances between a reference node and the other nodes
 * reachable from it in a graph.
 *
 * Args:
 *   distances:    array of shortest path distances, with UINT_MAX for
 *                 unreachable nodes
 *   traceNb:      number of nodes = number of traces
 */
static double sumDistances(const double* const distances, const unsigned int traceNb)
{
	unsigned int i;
	double result;

	result= 0;
	for (i= 0; i < traceNb; i++)
	{
		if (distances[i] != INFINITY)
		{
			result+= distances[i];
		}
	}

	return result;
}


/*
 * Cummulate the time correction factors between two nodes accross a graph
 *
 * With traceNum i, reference node r:
 * tr= (1 + Xri) * ti + D0ri
 *   = drift * ti + offset
 *
 * Args:
 *   fitArray:     array with the regression parameters
 *   previousVertex: previous vertex from a node i on the way to the source,
 *                 UINT_MAX if i is not on the way or is the source,
 *                 preallocated to the number of nodes
 *   traceNum:     end node, the reference depends on previousVertex
 *   drift:        drift factor
 *   offset:       offset factor
 */
static void reduceFactors(Fit* const* const fitArray, const unsigned int* const
	previousVertex, const unsigned int traceNum, double* const drift, double*
	const offset, double* const stDev)
{
	if (previousVertex[traceNum] == UINT_MAX)
	{
		*drift= 1.;
		*offset= 0.;
		*stDev= 0.;
	}
	else
	{
		const Fit* fit;
		double cummDrift, cummOffset, cummStDev;
		unsigned int pv;

		pv= previousVertex[traceNum];

		fit= &fitArray[pv][traceNum];
		reduceFactors(fitArray, previousVertex, pv, &cummDrift, &cummOffset,
			&cummStDev);

		*drift= cummDrift * (1 + fit->x);
		*offset= cummDrift * fit->d0 + cummOffset;
		*stDev= fit->x * cummStDev + fit->e;
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Graph*, graph to destroy
 *   user_data     NULL
 */
static void gfGraphDestroy(gpointer data, gpointer user_data)
{
	Graph* graph;

	graph= (Graph*) data;

	free(graph->previousVertex);
	free(graph);
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a:            Graph* graph
 *   b:            unsigned int* traceNum
 *
 * Returns:
 *   0 if graph contains traceNum
 */
static gint gcfGraphTraceCompare(gconstpointer a, gconstpointer b)
{
	Graph* graph;
	unsigned int traceNum;

	graph= (Graph*) a;
	traceNum= *(unsigned int *) b;

	if (graph->previousVertex[traceNum] != UINT_MAX)
	{
		return 0;
	}
	else if (graph->reference == traceNum)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number, on the x axis
 *   j:            second trace number, garanteed to be larger than i
 */
void writeAnalysisGraphsPlotsLinReg(SyncState* const syncState, const unsigned
	int i, const unsigned int j)
{
	AnalysisDataLinReg* analysisData;
	Fit* fit;

	analysisData= (AnalysisDataLinReg*) syncState->analysisData;
	fit= &analysisData->fitArray[j][i];

	fprintf(syncState->graphsStream,
		"\t%7g + %7g * x "
		"title \"Linreg conversion\" with lines "
		"linecolor rgb \"gray60\" linetype 1, \\\n",
		fit->d0, 1. + fit->x);
}
