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

#include <stdlib.h>

#include "sync_chain_lttv.h"

#include "event_analysis_eval.h"


// Functions common to all analysis modules
static void initAnalysisEval(SyncState* const syncState);
static void destroyAnalysisEval(SyncState* const syncState);

static void analyzeMessageEval(SyncState* const syncState, Message* const
	message);
static void analyzeExchangeEval(SyncState* const syncState, Exchange* const
	exchange);
static void analyzeBroadcastEval(SyncState* const syncState, Broadcast* const
	broadcast);
static GArray* finalizeAnalysisEval(SyncState* const syncState);
static void printAnalysisStatsEval(SyncState* const syncState);

// Functions specific to this module
static void registerAnalysisEval() __attribute__((constructor (102)));


static AnalysisModule analysisModuleEval= {
	.name= "eval",
	.initAnalysis= &initAnalysisEval,
	.destroyAnalysis= &destroyAnalysisEval,
	.analyzeMessage= &analyzeMessageEval,
	.analyzeExchange= &analyzeExchangeEval,
	.analyzeBroadcast= &analyzeBroadcastEval,
	.finalizeAnalysis= &finalizeAnalysisEval,
	.printAnalysisStats= &printAnalysisStatsEval,
	.writeAnalysisGraphsPlots= NULL,
	.writeAnalysisGraphsOptions= NULL,
};


/*
 * Analysis module registering function
 */
static void registerAnalysisEval()
{
	g_queue_push_tail(&analysisModules, &analysisModuleEval);
}


/*
 * Analysis init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void initAnalysisEval(SyncState* const syncState)
{
	AnalysisDataEval* analysisData;
	unsigned int i;

	analysisData= malloc(sizeof(AnalysisDataEval));
	syncState->analysisData= analysisData;

	//readRttInfo(&analysisData->rttInfo, optionEvalRttFile);

	if (syncState->stats)
	{
		analysisData->stats= malloc(sizeof(AnalysisStatsEval));
		analysisData->stats->broadcastDiffSum= 0.;

		analysisData->stats->allStats= malloc(syncState->traceNb *
			sizeof(TracePairStats*));
		for (i= 0; i < syncState->traceNb; i++)
		{
			analysisData->stats->allStats[i]= calloc(syncState->traceNb,
				sizeof(TracePairStats));
		}
	}
}


/*
 * Analysis destroy function
 *
 * Free the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void destroyAnalysisEval(SyncState* const syncState)
{
	unsigned int i;
	AnalysisDataEval* analysisData;

	analysisData= (AnalysisDataEval*) syncState->analysisData;

	if (analysisData == NULL || analysisData->rttInfo == NULL)
	{
		return;
	}

	//g_hash_table_destroy(analysisData->rttInfo);
	analysisData->rttInfo= NULL;

	if (syncState->stats)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			free(analysisData->stats->allStats[i]);
		}
		free(analysisData->stats->allStats);
		free(analysisData->stats);
	}

	free(syncState->analysisData);
	syncState->analysisData= NULL;
}


/*
 * Perform analysis on an event pair.
 *
 * Args:
 *   syncState     container for synchronization data
 *   message       structure containing the events
 */
static void analyzeMessageEval(SyncState* const syncState, Message* const message)
{
	AnalysisDataEval* analysisData;

	analysisData= (AnalysisDataEval*) syncState->analysisData;
}


/*
 * Perform analysis on multiple messages
 *
 * Args:
 *   syncState     container for synchronization data
 *   exchange      structure containing the messages
 */
static void analyzeExchangeEval(SyncState* const syncState, Exchange* const exchange)
{
	AnalysisDataEval* analysisData;

	analysisData= (AnalysisDataEval*) syncState->analysisData;
}


/*
 * Perform analysis on muliple events
 *
 * Args:
 *   syncState     container for synchronization data
 *   broadcast     structure containing the events
 */
static void analyzeBroadcastEval(SyncState* const syncState, Broadcast* const broadcast)
{
	AnalysisDataEval* analysisData;

	analysisData= (AnalysisDataEval*) syncState->analysisData;
}


/*
 * Finalize the factor calculations
 *
 * Since this module does not really calculate factors, identity factors are
 * returned.
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* finalizeAnalysisEval(SyncState* const syncState)
{
	GArray* factors;
	unsigned int i;

	factors= g_array_sized_new(FALSE, FALSE, sizeof(Factors),
		syncState->traceNb);
	g_array_set_size(factors, syncState->traceNb);
	for (i= 0; i < syncState->traceNb; i++)
	{
		Factors* e;

		e= &g_array_index(factors, Factors, i);
		e->drift= 1.;
		e->offset= 0.;
	}

	return factors;
}


/*
 * Print statistics related to analysis. Must be called after
 * finalizeAnalysis.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printAnalysisStatsEval(SyncState* const syncState)
{
	AnalysisDataEval* analysisData;
	unsigned int i, j;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataEval*) syncState->analysisData;

	printf("Synchronization evaluation analysis stats:\n");
	printf("\tsum of broadcast differential delays: %g\n",
		analysisData->stats->broadcastDiffSum);

	printf("\tIndividual evaluation:\n"
		"\t\tTrace pair  Inversions  Too fast    (No RTT info)\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			TracePairStats* tpStats;
			const char* format= "\t\t%3d - %-3d   %-10u  %-10u  %u\n";

			tpStats= &analysisData->stats->allStats[i][j];

			printf(format, i, j, tpStats->inversionNb, tpStats->tooFastNb,
				tpStats->noRTTInfoNb);

			tpStats= &analysisData->stats->allStats[j][i];

			printf(format, j, i, tpStats->inversionNb, tpStats->tooFastNb,
				tpStats->noRTTInfoNb);
		}
	}
}
