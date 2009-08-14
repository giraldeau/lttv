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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
static gboolean optionSyncGraphs;
static char* optionSyncGraphsDir;
static char graphsDir[20];

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
	int retval;

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

	optionSyncGraphs= FALSE;
	lttv_option_add("sync-graphs", '\0', "output gnuplot graph showing "
		"synchronization points", "none", LTTV_OPT_NONE, &optionSyncGraphs,
		NULL, NULL);

	retval= snprintf(graphsDir, sizeof(graphsDir), "graphs-%d", getpid());
	if (retval > sizeof(graphsDir) - 1)
	{
		graphsDir[sizeof(graphsDir) - 1]= '\0';
	}
	optionSyncGraphsDir= graphsDir;
	lttv_option_add("sync-graphs-dir", '\0', "specify the directory where to"
		" store the graphs", graphsDir, LTTV_OPT_STRING, &optionSyncGraphsDir,
		NULL, NULL);
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
	lttv_option_remove("sync-graphs");
	lttv_option_remove("sync-graphs-dir");
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
	char* cwd;
	FILE* graphsStream;
	int graphsFp;
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

	if (optionSyncGraphs)
	{
		syncState->graphs= optionSyncGraphsDir;
	}
	else
	{
		syncState->graphs= NULL;
	}

	// Identify and initialize processing module
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

	graphsStream= NULL;
	if (syncState->graphs)
	{
		// Create the graph directory right away in case the module initialization
		// functions have something to write in it.
		cwd= changeToGraphDir(syncState->graphs);

		if (syncState->processingModule->writeProcessingGraphsPlots != NULL)
		{
			if ((graphsFp= open("graphs.gnu", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR |
					S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH
					| S_IWOTH | S_IXOTH)) == -1)
			{
				g_error(strerror(errno));
			}
			if ((graphsStream= fdopen(graphsFp, "w")) == NULL)
			{
				g_error(strerror(errno));
			}
		}

		retval= chdir(cwd);
		if (retval == -1)
		{
			g_error(strerror(errno));
		}
		free(cwd);
	}

	syncState->processingModule->initProcessing(syncState, traceSetContext);

	// Identify and initialize matching and analysis modules
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

	// Write graphs file
	if (graphsStream != NULL)
	{
		unsigned int i, j;

		fprintf(graphsStream,
			"#!/usr/bin/gnuplot\n\n"
			"set terminal postscript eps color size 8in,6in\n");

		// Cover the upper triangular matrix, i is the reference node.
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= i + 1; j < syncState->traceNb; j++)
			{
				long pos;

				fprintf(graphsStream,
					"\nset output \"%03d-%03d.eps\"\n"
					"plot \\\n", i, j);

				syncState->processingModule->writeProcessingGraphsPlots(graphsStream,
					syncState, i, j);

				// Remove the ", \\\n" from the last graph plot line
				fflush(graphsStream);
				pos= ftell(graphsStream);
				if (ftruncate(fileno(graphsStream), pos - 4) == -1)
				{
					g_error(strerror(errno));
				}
				if (fseek(graphsStream, 0, SEEK_END) == -1)
				{
					g_error(strerror(errno));
				}

				fprintf(graphsStream,
					"\nset output \"%1$03d-%2$03d.eps\"\n"
					"set key inside right bottom\n"
					"set title \"\"\n"
					"set xlabel \"Clock %1$u\"\n"
					"set xtics nomirror\n"
					"set ylabel \"Clock %2$u\"\n"
					"set ytics nomirror\n", i, j);

				syncState->processingModule->writeProcessingGraphsOptions(graphsStream,
					syncState, i, j);

				fprintf(graphsStream,
					"replot\n");
			}
		}

		if (fclose(graphsStream) != 0)
		{
			g_error(strerror(errno));
		}
	}

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


/*
 * Change to the directory used to hold graphs. Create it if necessary.
 *
 * Args:
 *   graph:        name of directory
 *
 * Returns:
 *   The current working directory before the execution of the function. The
 *   string must be free'd by the caller.
 */
char* changeToGraphDir(char* const graphs)
{
	int retval;
	char* cwd;

	cwd= getcwd(NULL, 0);
	if (cwd == NULL)
	{
		g_error(strerror(errno));
	}
	while ((retval= chdir(graphs)) != 0)
	{
		if (errno == ENOENT)
		{
			retval= mkdir(graphs, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
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


LTTV_MODULE("sync", "Synchronize traces", \
	"Synchronizes a traceset based on the correspondance of network events", \
	init, destroy, "option")
