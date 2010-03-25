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
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lttv/module.h>
#include <lttv/option.h>


#include "event_processing_lttng_standard.h"
#include "event_processing_lttng_null.h"
#include "event_matching_tcp.h"
#include "event_matching_broadcast.h"
#include "event_matching_distributor.h"
#include "event_analysis_chull.h"
#include "event_analysis_linreg.h"
#include "event_analysis_eval.h"
#include "factor_reduction_accuracy.h"
#include "sync_chain.h"
#include "sync_chain_lttv.h"


static void init();
static void destroy();

static void gfAddModuleOption(gpointer data, gpointer user_data);
static void gfRemoveModuleOption(gpointer data, gpointer user_data);

static ModuleOption optionSync= {
	.longName= "sync",
	.hasArg= NO_ARG,
	.optionHelp= "synchronize the time between the traces",
};
static ModuleOption optionSyncStats= {
	.longName= "sync-stats",
	.hasArg= NO_ARG,
	.optionHelp= "print statistics about the time synchronization",
};
static ModuleOption optionSyncNull= {
	.longName= "sync-null",
	.hasArg= NO_ARG,
	.optionHelp= "read the events but do not perform any processing",
};
static GString* analysisModulesNames;
static ModuleOption optionSyncAnalysis= {
	.longName= "sync-analysis",
	.hasArg= REQUIRED_ARG,
	.optionHelp= "specify the algorithm to use for event analysis",
};
static GString* reductionModulesNames;
static ModuleOption optionSyncReduction= {
	.longName= "sync-reduction",
	.hasArg= REQUIRED_ARG,
	.optionHelp= "specify the algorithm to use for factor reduction",
};
static ModuleOption optionSyncGraphs= {
	.longName= "sync-graphs",
	.hasArg= NO_ARG,
	.optionHelp= "output gnuplot graph showing synchronization points",
};
static char graphsDir[20];
static ModuleOption optionSyncGraphsDir= {
	.longName= "sync-graphs-dir",
	.hasArg= REQUIRED_ARG,
	.optionHelp= "specify the directory where to store the graphs",
};


/*
 * Module init function
 *
 * This function is declared to be the module initialization function.
 */
static void init()
{
	int retval;
	unsigned int i;
	const struct
	{
		GQueue* modules;
		ModuleOption* option;
		size_t nameOffset;
		GString** names;
		void (*gfAppendName)(gpointer data, gpointer user_data);
	} loopValues[]= {
		{&analysisModules, &optionSyncAnalysis, offsetof(AnalysisModule,
				name), &analysisModulesNames, &gfAppendAnalysisName},
		{&reductionModules, &optionSyncReduction, offsetof(ReductionModule,
				name), &reductionModulesNames, &gfAppendReductionName},
	};

	g_debug("Sync init");

	/*
	 * Initialize event modules
	 * Call the "constructor" or initialization function of each event module
	 * so it can register itself. This must be done before elements in
	 * processingModules, matchingModules, analysisModules or moduleOptions
	 * are accessed.
	 */
	registerProcessingLTTVStandard();
	registerProcessingLTTVNull();

	registerMatchingTCP();
	registerMatchingBroadcast();
	registerMatchingDistributor();

	registerAnalysisCHull();
	registerAnalysisLinReg();
	registerAnalysisEval();

	registerReductionAccuracy();

	// Build module names lists for option and help string
	for (i= 0; i < ARRAY_SIZE(loopValues); i++)
	{
		g_assert(g_queue_get_length(loopValues[i].modules) > 0);
		loopValues[i].option->arg= (char*)(*(void**)
			g_queue_peek_head(loopValues[i].modules) +
			loopValues[i].nameOffset);
		*loopValues[i].names= g_string_new("");
		g_queue_foreach(loopValues[i].modules, loopValues[i].gfAppendName,
			*loopValues[i].names);
		// remove the last ", "
		g_string_truncate(*loopValues[i].names, (*loopValues[i].names)->len -
			2);
		loopValues[i].option->argHelp= (*loopValues[i].names)->str;
	}

	retval= snprintf(graphsDir, sizeof(graphsDir), "graphs-%d", getpid());
	if (retval > sizeof(graphsDir) - 1)
	{
		graphsDir[sizeof(graphsDir) - 1]= '\0';
	}
	optionSyncGraphsDir.arg= graphsDir;
	optionSyncGraphsDir.argHelp= graphsDir;

	g_queue_push_head(&moduleOptions, &optionSyncGraphsDir);
	g_queue_push_head(&moduleOptions, &optionSyncGraphs);
	g_queue_push_head(&moduleOptions, &optionSyncReduction);
	g_queue_push_head(&moduleOptions, &optionSyncAnalysis);
	g_queue_push_head(&moduleOptions, &optionSyncNull);
	g_queue_push_head(&moduleOptions, &optionSyncStats);
	g_queue_push_head(&moduleOptions, &optionSync);

	g_queue_foreach(&moduleOptions, &gfAddModuleOption, NULL);
}


/*
 * Module unload function
 */
static void destroy()
{
	g_debug("Sync destroy");

	g_queue_foreach(&moduleOptions, &gfRemoveModuleOption, NULL);
	g_string_free(analysisModulesNames, TRUE);
	g_string_free(reductionModulesNames, TRUE);

	g_queue_clear(&processingModules);
	g_queue_clear(&matchingModules);
	g_queue_clear(&analysisModules);
	g_queue_clear(&reductionModules);
	g_queue_clear(&moduleOptions);
}


/*
 * Calculate a traceset's drift and offset values based on network events
 *
 * The individual correction factors are written out to each trace.
 *
 * Args:
 *   traceSetContext: traceset
 *
 * Returns:
 *   false if synchronization was not performed, true otherwise
 */
bool syncTraceset(LttvTracesetContext* const traceSetContext)
{
	SyncState* syncState;
	struct timeval startTime, endTime;
	struct rusage startUsage, endUsage;
	GList* result;
	unsigned int i;
	AllFactors* allFactors;
	GArray* factors;
	double minOffset, minDrift;
	unsigned int refFreqTrace;
	int retval;

	if (!optionSync.present)
	{
		g_debug("Not synchronizing traceset because option is disabled");
		return false;
	}

	if (optionSyncStats.present)
	{
		gettimeofday(&startTime, 0);
		getrusage(RUSAGE_SELF, &startUsage);
	}

	// Initialize data structures
	syncState= malloc(sizeof(SyncState));

	if (optionSyncStats.present)
	{
		syncState->stats= true;
	}
	else
	{
		syncState->stats= false;
	}

	if (!optionSyncNull.present && optionSyncGraphs.present)
	{
		// Create the graph directory right away in case the module initialization
		// functions have something to write in it.
		syncState->graphsDir= optionSyncGraphsDir.arg;
		syncState->graphsStream= createGraphsDir(syncState->graphsDir);
	}
	else
	{
		syncState->graphsStream= NULL;
		syncState->graphsDir= NULL;
	}

	// Identify and initialize modules
	syncState->processingData= NULL;
	if (optionSyncNull.present)
	{
		result= g_queue_find_custom(&processingModules, "LTTV-null",
			&gcfCompareProcessing);
	}
	else
	{
		result= g_queue_find_custom(&processingModules, "LTTV-standard",
			&gcfCompareProcessing);
	}
	g_assert(result != NULL);
	syncState->processingModule= (ProcessingModule*) result->data;

	syncState->matchingData= NULL;
	result= g_queue_find_custom(&matchingModules, "TCP", &gcfCompareMatching);
	g_assert(result != NULL);
	syncState->matchingModule= (MatchingModule*) result->data;

	syncState->analysisData= NULL;
	result= g_queue_find_custom(&analysisModules, optionSyncAnalysis.arg,
		&gcfCompareAnalysis);
	if (result != NULL)
	{
		syncState->analysisModule= (AnalysisModule*) result->data;
	}
	else
	{
		g_error("Analysis module '%s' not found", optionSyncAnalysis.arg);
	}

	syncState->reductionData= NULL;
	result= g_queue_find_custom(&reductionModules, optionSyncReduction.arg,
		&gcfCompareReduction);
	if (result != NULL)
	{
		syncState->reductionModule= (ReductionModule*) result->data;
	}
	else
	{
		g_error("Reduction module '%s' not found", optionSyncReduction.arg);
	}

	syncState->processingModule->initProcessing(syncState, traceSetContext);
	if (!optionSyncNull.present)
	{
		syncState->matchingModule->initMatching(syncState);
		syncState->analysisModule->initAnalysis(syncState);
		syncState->reductionModule->initReduction(syncState);
	}

	// Process traceset
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);
	lttv_process_traceset_middle(traceSetContext, ltt_time_infinite,
		G_MAXULONG, NULL);
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);

	// Obtain, reduce, adjust and set correction factors
	allFactors= syncState->processingModule->finalizeProcessing(syncState);
	factors= syncState->reductionModule->finalizeReduction(syncState,
		allFactors);
	freeAllFactors(allFactors, syncState->traceNb);

	/* The offsets are adjusted so the lowest one is 0. This is done because
	 * of a Lttv specific limitation: events cannot have negative times. By
	 * having non-negative offsets, events cannot be moved backwards to
	 * negative times.
	 */
	minOffset= 0;
	for (i= 0; i < syncState->traceNb; i++)
	{
		minOffset= MIN(g_array_index(factors, Factors, i).offset, minOffset);
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		g_array_index(factors, Factors, i).offset-= minOffset;
	}

	/* Because the timestamps are corrected at the TSC level (not at the
	 * LttTime level) all trace frequencies must be made equal. We use the
	 * frequency of the system with the lowest drift
	 */
	minDrift= INFINITY;
	refFreqTrace= 0;
	for (i= 0; i < syncState->traceNb; i++)
	{
		if (g_array_index(factors, Factors, i).drift < minDrift)
		{
			minDrift= g_array_index(factors, Factors, i).drift;
			refFreqTrace= i;
		}
	}
	g_assert(syncState->traceNb == 0 || minDrift != INFINITY);

	// Write the factors to the LttTrace structures
	for (i= 0; i < syncState->traceNb; i++)
	{
		LttTrace* t;
		Factors* traceFactors;

		t= traceSetContext->traces[i]->t;
		traceFactors= &g_array_index(factors, Factors, i);

		t->drift= traceFactors->drift;
		t->offset= traceFactors->offset;
		t->start_freq= traceSetContext->traces[refFreqTrace]->t->start_freq;
		t->freq_scale= traceSetContext->traces[refFreqTrace]->t->freq_scale;
		t->start_time_from_tsc =
			ltt_time_from_uint64(tsc_to_uint64(t->freq_scale, t->start_freq,
					t->drift * t->start_tsc + t->offset));
	}

	g_array_free(factors, TRUE);

	lttv_traceset_context_compute_time_span(traceSetContext,
		&traceSetContext->time_span);

	g_debug("traceset start %ld.%09ld end %ld.%09ld",
		traceSetContext->time_span.start_time.tv_sec,
		traceSetContext->time_span.start_time.tv_nsec,
		traceSetContext->time_span.end_time.tv_sec,
		traceSetContext->time_span.end_time.tv_nsec);

	// Write graphs file
	if (!optionSyncNull.present && optionSyncGraphs.present)
	{
		writeGraphsScript(syncState);

		if (fclose(syncState->graphsStream) != 0)
		{
			g_error(strerror(errno));
		}
	}

	if (!optionSyncNull.present && optionSyncStats.present)
	{
		printStats(syncState);

		printf("Resulting synchronization factors:\n");
		for (i= 0; i < syncState->traceNb; i++)
		{
			LttTrace* t;

			t= traceSetContext->traces[i]->t;

			printf("\ttrace %u drift= %g offset= %g (%f) start time= %ld.%09ld\n",
				i, t->drift, t->offset, (double) tsc_to_uint64(t->freq_scale,
					t->start_freq, t->offset) / NANOSECONDS_PER_SECOND,
				t->start_time_from_tsc.tv_sec,
				t->start_time_from_tsc.tv_nsec);
		}
	}

	syncState->processingModule->destroyProcessing(syncState);
	if (syncState->matchingModule != NULL)
	{
		syncState->matchingModule->destroyMatching(syncState);
	}
	if (syncState->analysisModule != NULL)
	{
		syncState->analysisModule->destroyAnalysis(syncState);
	}
	if (syncState->reductionModule != NULL)
	{
		syncState->reductionModule->destroyReduction(syncState);
	}

	free(syncState);

	if (optionSyncStats.present)
	{
		gettimeofday(&endTime, 0);
		retval= getrusage(RUSAGE_SELF, &endUsage);

		timeDiff(&endTime, &startTime);
		timeDiff(&endUsage.ru_utime, &startUsage.ru_utime);
		timeDiff(&endUsage.ru_stime, &startUsage.ru_stime);

		printf("Synchronization time:\n");
		printf("\treal time: %ld.%06ld\n", endTime.tv_sec, endTime.tv_usec);
		printf("\tuser time: %ld.%06ld\n", endUsage.ru_utime.tv_sec,
			endUsage.ru_utime.tv_usec);
		printf("\tsystem time: %ld.%06ld\n", endUsage.ru_stime.tv_sec,
			endUsage.ru_stime.tv_usec);
	}

	return true;
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data:         ModuleOption*
 *   user_data:    NULL
 */
static void gfAddModuleOption(gpointer data, gpointer user_data)
{
	ModuleOption* option= data;
	LttvOptionType conversion[]= {
		[NO_ARG]= LTTV_OPT_NONE,
		[OPTIONAL_ARG]= LTTV_OPT_NONE,
		[REQUIRED_ARG]= LTTV_OPT_STRING,
	};
	size_t fieldOffset[]= {
		[NO_ARG]= offsetof(ModuleOption, present),
		[REQUIRED_ARG]= offsetof(ModuleOption, arg),
	};
	static const char* argHelpNone= "none";

	g_assert_cmpuint(sizeof(conversion) / sizeof(*conversion), ==,
		HAS_ARG_COUNT);
	if (option->hasArg == OPTIONAL_ARG)
	{
		g_warning("Parameters with optional arguments not supported by the "
			"lttv option scheme, parameter '%s' will not be available",
			option->longName);
	}
	else
	{
		lttv_option_add(option->longName, '\0', option->optionHelp,
			option->argHelp ? option->argHelp : argHelpNone,
			conversion[option->hasArg], (void*) option + fieldOffset[option->hasArg],
			NULL, NULL);
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data:         ModuleOption*
 *   user_data:    NULL
 */
static void gfRemoveModuleOption(gpointer data, gpointer user_data)
{
	lttv_option_remove(((ModuleOption*) data)->longName);
}


LTTV_MODULE("sync", "Synchronize traces", \
	"Synchronizes a traceset based on the correspondance of network events", \
	init, destroy, "option")
