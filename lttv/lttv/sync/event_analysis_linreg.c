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


// Functions common to all analysis modules
static void initAnalysisLinReg(SyncState* const syncState);
static void destroyAnalysisLinReg(SyncState* const syncState);

static void analyzeExchangeLinReg(SyncState* const syncState, Exchange* const exchange);
static AllFactors* finalizeAnalysisLinReg(SyncState* const syncState);
static void printAnalysisStatsLinReg(SyncState* const syncState);
static void writeAnalysisGraphsPlotsLinReg(SyncState* const syncState, const
	unsigned int i, const unsigned int j);

// Functions specific to this module
static void finalizeLSA(SyncState* const syncState);


static AnalysisModule analysisModuleLinReg= {
	.name= "linreg",
	.initAnalysis= &initAnalysisLinReg,
	.destroyAnalysis= &destroyAnalysisLinReg,
	.analyzeExchange= &analyzeExchangeLinReg,
	.finalizeAnalysis= &finalizeAnalysisLinReg,
	.printAnalysisStats= &printAnalysisStatsLinReg,
	.graphFunctions= {
		.writeTraceTraceForePlots= &writeAnalysisGraphsPlotsLinReg,
	}
};


/*
 * Analysis module registering function
 */
void registerAnalysisLinReg()
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

	g_debug("Synchronization calculation, "
		"%d acked packets - using last one,",
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
 *   AllFactors*   synchronization factors for each trace pair
 */
static AllFactors* finalizeAnalysisLinReg(SyncState* const syncState)
{
	AllFactors* result;
	unsigned int i, j;
	AnalysisDataLinReg* analysisData= (AnalysisDataLinReg*)
		syncState->analysisData;

	finalizeLSA(syncState);

	result= createAllFactors(syncState->traceNb);

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				Fit* fit;

				fit= &analysisData->fitArray[i][j];
				result->pairFactors[i][j].type= APPROXIMATE;
				result->pairFactors[i][j].approx= malloc(sizeof(Factors));
				result->pairFactors[i][j].approx->drift= 1. + fit->x;
				result->pairFactors[i][j].approx->offset= fit->d0;
				result->pairFactors[i][j].accuracy= fit->e;
			}
		}
	}

	return result;
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

				g_debug("[i= %u j= %u]", i, j);
				g_debug("n= %d st= %g st2= %g sd= %g sd2= %g std= %g",
					fit->n, fit->st, fit->st2, fit->sd, fit->sd2, fit->std);
				g_debug("xij= %g d0ij= %g e= %g", fit->x, fit->d0, fit->e);
				g_debug("(xji= %g d0ji= %g)", -fit->x / (1 + fit->x),
					-fit->d0 / (1 + fit->x));
			}
		}
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
