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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lttv/module.h>
#include <lttv/option.h>

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
 * This function is declared to be the module initialization function. Event
 * modules are registered with a "constructor (102)" attribute except one in
 * each class (processing, matching, analysis) which is chosen to be the
 * default and which is registered with a "constructor (101)" attribute.
 * Constructors with no priority are called after constructors with
 * priorities. The result is that the list of event modules is known when this
 * function is executed.
 */
static void init()
{
	int retval;

	g_debug("Sync init");

	g_assert(g_queue_get_length(&analysisModules) > 0);
	optionSyncAnalysis.arg= ((AnalysisModule*)
		g_queue_peek_head(&analysisModules))->name;
	analysisModulesNames= g_string_new("");
	g_queue_foreach(&analysisModules, &gfAppendAnalysisName,
		analysisModulesNames);
	// remove the last ", "
	g_string_truncate(analysisModulesNames, analysisModulesNames->len - 2);
	optionSyncAnalysis.argHelp= analysisModulesNames->str;

	retval= snprintf(graphsDir, sizeof(graphsDir), "graphs-%d", getpid());
	if (retval > sizeof(graphsDir) - 1)
	{
		graphsDir[sizeof(graphsDir) - 1]= '\0';
	}
	optionSyncGraphsDir.arg= graphsDir;
	optionSyncGraphsDir.argHelp= graphsDir;

	g_queue_push_head(&moduleOptions, &optionSyncGraphsDir);
	g_queue_push_head(&moduleOptions, &optionSyncGraphs);
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

	g_queue_clear(&processingModules);
	g_queue_clear(&matchingModules);
	g_queue_clear(&analysisModules);
	g_queue_clear(&moduleOptions);
}


/*
 * Calculate a traceset's drift and offset values based on network events
 *
 * The individual correction factors are written out to each trace.
 *
 * Args:
 *   traceSetContext: traceset
 */
void syncTraceset(LttvTracesetContext* const traceSetContext)
{
	SyncState* syncState;
	struct timeval startTime, endTime;
	struct rusage startUsage, endUsage;
	GList* result;
	unsigned int i;
	int retval;

	if (!optionSync.present)
	{
		g_debug("Not synchronizing traceset because option is disabled");
		return;
	}

	if (optionSyncStats.present)
	{
		gettimeofday(&startTime, 0);
		getrusage(RUSAGE_SELF, &startUsage);
	}

	// Initialize data structures
	syncState= malloc(sizeof(SyncState));
	syncState->traceNb= lttv_traceset_number(traceSetContext->ts);

	if (optionSyncStats.present)
	{
		syncState->stats= true;
	}
	else
	{
		syncState->stats= false;
	}

	if (optionSyncGraphs.present)
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

	if (!optionSyncNull.present)
	{
		syncState->analysisModule->initAnalysis(syncState);
		syncState->matchingModule->initMatching(syncState);
	}
	syncState->processingModule->initProcessing(syncState, traceSetContext);

	// Process traceset
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);
	lttv_process_traceset_middle(traceSetContext, ltt_time_infinite,
		G_MAXULONG, NULL);
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);

	syncState->processingModule->finalizeProcessing(syncState);

	// Write graphs file
	if (optionSyncGraphs.present)
	{
		writeGraphsScript(syncState);

		if (fclose(syncState->graphsStream) != 0)
		{
			g_error(strerror(errno));
		}
	}

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

	if (optionSyncStats.present)
	{
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
