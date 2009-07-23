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
#include <sys/time.h>
#include <sys/resource.h>

#include <lttv/module.h>
#include <lttv/option.h>

#include "sync_chain.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


static void init();
static void destroy();

static void timeDiff(struct timeval* const end, const struct timeval* const start);
static gint gcfCompareAnalysis(gconstpointer a, gconstpointer b);
static gint gcfCompareProcessing(gconstpointer a, gconstpointer b);
static void gfAppendAnalysisName(gpointer data, gpointer user_data);

static gboolean optionSync;
static gboolean optionSyncStats;
static gboolean optionSyncNull;
static char* optionSyncAnalysis;

GQueue processingModules= G_QUEUE_INIT;
GQueue matchingModules= G_QUEUE_INIT;
GQueue analysisModules= G_QUEUE_INIT;


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
	GString* analysisModulesNames;

	g_debug("\t\t\tXXXX sync init\n");

	optionSync= FALSE;
	lttv_option_add("sync", '\0', "synchronize the time between the traces" ,
		"none", LTTV_OPT_NONE, &optionSync, NULL, NULL);

	optionSyncStats= FALSE;
	lttv_option_add("sync-stats", '\0', "print statistics about the time "
		"synchronization", "none", LTTV_OPT_NONE, &optionSyncStats, NULL,
		NULL);

	optionSyncNull= FALSE;
	lttv_option_add("sync-null", '\0', "read the events but do not perform "
		"any processing", "none", LTTV_OPT_NONE, &optionSyncNull, NULL, NULL);

	g_assert(g_queue_get_length(&analysisModules) > 0);
	optionSyncAnalysis= ((AnalysisModule*)
		g_queue_peek_head(&analysisModules))->name;
	analysisModulesNames= g_string_new("");
	g_queue_foreach(&analysisModules, &gfAppendAnalysisName,
		analysisModulesNames);
	// remove the last ", "
	g_string_truncate(analysisModulesNames, analysisModulesNames->len - 2);
	lttv_option_add("sync-analysis", '\0', "specify the algorithm to use for "
		"event analysis" , analysisModulesNames->str, LTTV_OPT_STRING,
		&optionSyncAnalysis, NULL, NULL);
	g_string_free(analysisModulesNames, TRUE);
}


/*
 * Module unload function
 */
static void destroy()
{
	g_debug("\t\t\tXXXX sync destroy\n");

	lttv_option_remove("sync");
	lttv_option_remove("sync-stats");
	lttv_option_remove("sync-null");
	lttv_option_remove("sync-analysis");
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
	int retval;

	if (optionSync == FALSE)
	{
		g_debug("Not synchronizing traceset because option is disabled");
		return;
	}

	if (optionSyncStats)
	{
		gettimeofday(&startTime, 0);
		getrusage(RUSAGE_SELF, &startUsage);
	}

	// Initialize data structures
	syncState= malloc(sizeof(SyncState));
	syncState->traceNb= lttv_traceset_number(traceSetContext->ts);

	if (optionSyncStats)
	{
		syncState->stats= true;
	}
	else
	{
		syncState->stats= false;
	}

	syncState->processingData= NULL;
	if (optionSyncNull)
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
	syncState->processingModule->initProcessing(syncState, traceSetContext);

	syncState->matchingData= NULL;
	syncState->analysisData= NULL;
	if (optionSyncNull)
	{
		syncState->matchingModule= NULL;
		syncState->analysisModule= NULL;
	}
	else
	{
		g_assert(g_queue_get_length(&matchingModules) == 1);
		syncState->matchingModule= (MatchingModule*)
			g_queue_peek_head(&matchingModules);
		syncState->matchingModule->initMatching(syncState);

		result= g_queue_find_custom(&analysisModules, optionSyncAnalysis,
			&gcfCompareAnalysis);
		if (result != NULL)
		{
			syncState->analysisModule= (AnalysisModule*) result->data;
			syncState->analysisModule->initAnalysis(syncState);
		}
		else
		{
			g_error("Analysis module '%s' not found", optionSyncAnalysis);
		}
	}

	// Process traceset
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);
	lttv_process_traceset_middle(traceSetContext, ltt_time_infinite,
		G_MAXULONG, NULL);
	lttv_process_traceset_seek_time(traceSetContext, ltt_time_zero);

	syncState->processingModule->finalizeProcessing(syncState);

	if (syncState->processingModule->printProcessingStats != NULL)
	{
		syncState->processingModule->printProcessingStats(syncState);
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

	if (optionSyncStats)
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
 * Calculate the elapsed time between two timeval values
 *
 * Args:
 *   end:          end time, result is also stored in this structure
 *   start:        start time
 */
static void timeDiff(struct timeval* const end, const struct timeval* const start)
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
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            AnalysisModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the analysis module a's name is b
 */
static gint gcfCompareAnalysis(gconstpointer a, gconstpointer b)
{
	const AnalysisModule* analysisModule;
	const char* name;

	analysisModule= (const AnalysisModule*)a;
	name= (const char*)b;

	return strncmp(analysisModule->name, name, strlen(analysisModule->name) +
		1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            ProcessingModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the analysis module a's name is b
 */
static gint gcfCompareProcessing(gconstpointer a, gconstpointer b)
{
	const ProcessingModule* processingModule;
	const char* name;

	processingModule= (const ProcessingModule*)a;
	name= (const char*)b;

	return strncmp(processingModule->name, name,
		strlen(processingModule->name) + 1);
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
static void gfAppendAnalysisName(gpointer data, gpointer user_data)
{
	g_string_append((GString*) user_data, ((AnalysisModule*) data)->name);
	g_string_append((GString*) user_data, ", ");
}


LTTV_MODULE("sync", "Synchronize traces", \
	"Synchronizes a traceset based on the correspondance of network events", \
	init, destroy, "option")

