/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009 Benjamin Poirier
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
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lttv/attribute.h>
#include <lttv/filter.h>
#include <lttv/hook.h>
#include <lttv/iattribute.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/option.h>
#include <lttv/print.h>
#include <lttv/sync/sync_chain_lttv.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/trace.h>


struct TracesetChainState {
	// uint64_t* eventNbs[LttvTraceContext*]
	GHashTable* eventNbs;

	SyncState* syncState;
	struct timeval startTime;
	struct rusage startUsage;
	FILE* graphsStream;
};

static LttvHooks* before_traceset, * before_trace, * event_hook, * after_traceset;


static void init();
static void destroy();

static gboolean tracesetStart(void *hook_data, void *call_data);
static gboolean traceStart(void *hook_data, void *call_data);
static int processEveryEvent(void *hook_data, void *call_data);
static gboolean tracesetEnd(void *hook_data, void *call_data);

static void setupSyncChain(LttvTracesetContext* const traceSetContext);
static void teardownSyncChain(LttvTracesetContext* const traceSetContext);

void ghfPrintEventCount(gpointer key, gpointer value, gpointer user_data);
void gdnDestroyUint64(gpointer data);

// struct TracesetChainState* tracesetChainStates[LttvTracesetContext*]
static GHashTable* tracesetChainStates;

static LttvHooks* before_traceset, * before_trace, * event_hook, * after_traceset;
static const struct {
	const char const *path;
	LttvHooks **hook;
	LttvHook function;
} batchAnalysisHooks[] = {
	{"hooks/traceset/before", &before_traceset, &tracesetStart},
	{"hooks/trace/before", &before_trace, &traceStart},
	{"hooks/event", &event_hook, &processEveryEvent},
	{"hooks/traceset/after", &after_traceset, &tracesetEnd},
};

static gboolean optionEvalGraphs;
static char* optionEvalGraphsDir;
static char graphsDir[20];


/*
 * Module init function
 */
static void init()
{
	gboolean result;
	unsigned int i;
	LttvAttributeValue value;
	int retval;
	LttvIAttribute* attributes= LTTV_IATTRIBUTE(lttv_global_attributes());

	tracesetChainStates= g_hash_table_new(NULL, NULL);

	for (i= 0; i < sizeof(batchAnalysisHooks) / sizeof(*batchAnalysisHooks);
		i++)
	{
		result= lttv_iattribute_find_by_path(attributes,
			batchAnalysisHooks[i].path, LTTV_POINTER, &value);
		g_assert(result);
		*batchAnalysisHooks[i].hook= *(value.v_pointer);
		g_assert(*batchAnalysisHooks[i].hook);
		lttv_hooks_add(*batchAnalysisHooks[i].hook,
			batchAnalysisHooks[i].function, NULL, LTTV_PRIO_DEFAULT);
	}

	optionEvalGraphs= FALSE;
	lttv_option_add("eval-graphs", '\0', "output gnuplot graph showing "
		"synchronization points", "none", LTTV_OPT_NONE, &optionEvalGraphs,
		NULL, NULL);

	retval= snprintf(graphsDir, sizeof(graphsDir), "graphs-%d", getpid());
	if (retval > sizeof(graphsDir) - 1)
	{
		graphsDir[sizeof(graphsDir) - 1]= '\0';
	}
	optionEvalGraphsDir= graphsDir;
	lttv_option_add("eval-graphs-dir", '\0', "specify the directory where to"
		" store the graphs", graphsDir, LTTV_OPT_STRING, &optionEvalGraphsDir,
		NULL, NULL);
}


/*
 * Module destroy function
 */
static void destroy()
{
	unsigned int i;

	g_assert_cmpuint(g_hash_table_size(tracesetChainStates), ==, 0);
	g_hash_table_destroy(tracesetChainStates);

	for (i= 0; i < sizeof(batchAnalysisHooks) / sizeof(*batchAnalysisHooks);
		i++)
	{
		lttv_hooks_remove_data(*batchAnalysisHooks[i].hook,
			batchAnalysisHooks[i].function, NULL);
	}

	lttv_option_remove("eval-graphs");
	lttv_option_remove("eval-graphs-dir");
}


/*
 * Lttv hook function that will be called before a traceset is processed
 *
 * Args:
 *   hookData:     NULL
 *   callData:     LttvTracesetContext* at that moment
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks
 */
static gboolean tracesetStart(void *hook_data, void *call_data)
{
	struct TracesetChainState* tracesetChainState;
	LttvTracesetContext *tsc= (LttvTracesetContext *) call_data;

	tracesetChainState= malloc(sizeof(struct TracesetChainState));
	g_hash_table_insert(tracesetChainStates, tsc, tracesetChainState);
	tracesetChainState->eventNbs= g_hash_table_new_full(&g_direct_hash,
		&g_direct_equal, NULL, &gdnDestroyUint64);

	gettimeofday(&tracesetChainState->startTime, 0);
	getrusage(RUSAGE_SELF, &tracesetChainState->startUsage);

	setupSyncChain(tsc);

	return FALSE;
}


/*
 * Lttv hook function that will be called before a trace is processed
 *
 * Args:
 *   hookData:     NULL
 *   callData:     LttvTraceContext* at that moment
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks
 */
static gboolean traceStart(void *hook_data, void *call_data)
{
	struct TracesetChainState* tracesetChainState;
	uint64_t* eventNb;
	LttvTraceContext* tc= (LttvTraceContext*) call_data;
	LttvTracesetContext* tsc= tc->ts_context;

	tracesetChainState= g_hash_table_lookup(tracesetChainStates, tsc);
	eventNb= malloc(sizeof(uint64_t));
	*eventNb= 0;
	g_hash_table_insert(tracesetChainState->eventNbs, tc, eventNb);

	return FALSE;
}


/*
 * Lttv hook function that is called for every event
 *
 * Args:
 *   hookData:     NULL
 *   callData:     LttvTracefileContext* at the moment of the event
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks for
 *                 this event
 */
static int processEveryEvent(void *hook_data, void *call_data)
{
	LttvTracefileContext* tfc= (LttvTracefileContext*) call_data;
	LttvTraceContext* tc= tfc->t_context;
	LttvTracesetContext* tsc= tc->ts_context;
	struct TracesetChainState* tracesetChainState;
	uint64_t* eventNb;

	tracesetChainState= g_hash_table_lookup(tracesetChainStates, tsc);
	eventNb= g_hash_table_lookup(tracesetChainState->eventNbs, tc);

	(*eventNb)++;

	return FALSE;
}


/*
 * Lttv hook function that is called after a traceset has been processed
 *
 * Args:
 *   hookData:     NULL
 *   callData:     LttvTracefileContext* at that moment
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks
 */
static gboolean tracesetEnd(void *hook_data, void *call_data)
{
	struct TracesetChainState* tracesetChainState;
	LttvTracesetContext* tsc= (LttvTracesetContext*) call_data;
	uint64_t sum= 0;

	tracesetChainState= g_hash_table_lookup(tracesetChainStates, tsc);
	printf("Event count (%u traces):\n",
		g_hash_table_size(tracesetChainState->eventNbs));
	g_hash_table_foreach(tracesetChainState->eventNbs, &ghfPrintEventCount,
		&sum);
	printf("\ttotal events: %" PRIu64 "\n", sum);
	g_hash_table_destroy(tracesetChainState->eventNbs);

	teardownSyncChain(tsc);

	g_hash_table_remove(tracesetChainStates, tsc);

	return FALSE;
}


/*
 * Initialize modules in a sync chain. Use modules that will check
 * the precision of time synchronization between a group of traces.
 *
 * Args:
 *   traceSetContext: traceset
 */
void setupSyncChain(LttvTracesetContext* const traceSetContext)
{
	struct TracesetChainState* tracesetChainState;
	SyncState* syncState;
	GList* result;
	int retval;

	tracesetChainState= g_hash_table_lookup(tracesetChainStates, traceSetContext);
	syncState= malloc(sizeof(SyncState));
	tracesetChainState->syncState= syncState;
	syncState->traceNb= lttv_traceset_number(traceSetContext->ts);

	syncState->stats= true;

	if (optionEvalGraphs)
	{
		syncState->graphs= optionEvalGraphsDir;
	}
	else
	{
		syncState->graphs= NULL;
	}

	syncState->processingData= NULL;
	result= g_queue_find_custom(&processingModules, "LTTV-standard",
		&gcfCompareProcessing);
	syncState->processingModule= (ProcessingModule*) result->data;

	tracesetChainState->graphsStream= NULL;
	if (syncState->graphs &&
		syncState->processingModule->writeProcessingGraphsPlots != NULL)
	{
		char* cwd;
		int graphsFp;

		// Create the graph directory right away in case the module initialization
		// functions have something to write in it.
		cwd= changeToGraphDir(syncState->graphs);

		if ((graphsFp= open("graphs.gnu", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR |
				S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH
				| S_IWOTH | S_IXOTH)) == -1)
		{
			g_error(strerror(errno));
		}
		if ((tracesetChainState->graphsStream= fdopen(graphsFp, "w")) == NULL)
		{
			g_error(strerror(errno));
		}

		retval= chdir(cwd);
		if (retval == -1)
		{
			g_error(strerror(errno));
		}
		free(cwd);
	}

	syncState->matchingData= NULL;
	result= g_queue_find_custom(&matchingModules, "broadcast", &gcfCompareMatching);
	syncState->matchingModule= (MatchingModule*) result->data;

	syncState->analysisData= NULL;
	result= g_queue_find_custom(&analysisModules, "chull", &gcfCompareAnalysis);
	syncState->analysisModule= (AnalysisModule*) result->data;

	syncState->processingModule->initProcessing(syncState, traceSetContext);
	syncState->matchingModule->initMatching(syncState);
	syncState->analysisModule->initAnalysis(syncState);
}


/*
 * Destroy modules in a sync chain
 *
 * Args:
 *   traceSetContext: traceset
 */
void teardownSyncChain(LttvTracesetContext* const traceSetContext)
{
	struct TracesetChainState* tracesetChainState;
	SyncState* syncState;
	struct timeval endTime;
	struct rusage endUsage;
	int retval;

	tracesetChainState= g_hash_table_lookup(tracesetChainStates, traceSetContext);
	syncState= tracesetChainState->syncState;

	syncState->processingModule->finalizeProcessing(syncState);

	// Write graphs file
	if (tracesetChainState->graphsStream != NULL)
	{
		unsigned int i, j;

		fprintf(tracesetChainState->graphsStream,
			"#!/usr/bin/gnuplot\n\n"
			"set terminal postscript eps color size 8in,6in\n");

		// Cover the upper triangular matrix, i is the reference node.
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= i + 1; j < syncState->traceNb; j++)
			{
				long pos;

				fprintf(tracesetChainState->graphsStream,
					"\nset output \"%03d-%03d.eps\"\n"
					"plot \\\n", i, j);

				syncState->processingModule->writeProcessingGraphsPlots(tracesetChainState->graphsStream,
					syncState, i, j);

				// Remove the ", \\\n" from the last graph plot line
				fflush(tracesetChainState->graphsStream);
				pos= ftell(tracesetChainState->graphsStream);
				if (ftruncate(fileno(tracesetChainState->graphsStream), pos - 4) == -1)
				{
					g_error(strerror(errno));
				}
				if (fseek(tracesetChainState->graphsStream, 0, SEEK_END) == -1)
				{
					g_error(strerror(errno));
				}

				fprintf(tracesetChainState->graphsStream,
					"\nset output \"%1$03d-%2$03d.eps\"\n"
					"set key inside right bottom\n"
					"set title \"\"\n"
					"set xlabel \"Clock %1$u\"\n"
					"set xtics nomirror\n"
					"set ylabel \"Clock %2$u\"\n"
					"set ytics nomirror\n", i, j);

				syncState->processingModule->writeProcessingGraphsOptions(tracesetChainState->graphsStream,
					syncState, i, j);

				fprintf(tracesetChainState->graphsStream,
					"replot\n");
			}
		}

		if (fclose(tracesetChainState->graphsStream) != 0)
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

	gettimeofday(&endTime, 0);
	retval= getrusage(RUSAGE_SELF, &endUsage);

	timeDiff(&endTime, &tracesetChainState->startTime);
	timeDiff(&endUsage.ru_utime, &tracesetChainState->startUsage.ru_utime);
	timeDiff(&endUsage.ru_stime, &tracesetChainState->startUsage.ru_stime);

	printf("Evaluation time:\n");
	printf("\treal time: %ld.%06ld\n", endTime.tv_sec, endTime.tv_usec);
	printf("\tuser time: %ld.%06ld\n", endUsage.ru_utime.tv_sec,
		endUsage.ru_utime.tv_usec);
	printf("\tsystem time: %ld.%06ld\n", endUsage.ru_stime.tv_sec,
		endUsage.ru_stime.tv_usec);

	g_hash_table_remove(tracesetChainStates, traceSetContext);
	free(tracesetChainState);
}



/*
 * A GHFunc function for g_hash_table_foreach()
 *
 * Args:
 *   key:          LttvTraceContext *
 *   value:        uint64_t *, event count for this trace
 *   user_data:    uint64_t *, sum of the event counts
 *
 * Returns:
 *   Updates the sum in user_data
 */
void ghfPrintEventCount(gpointer key, gpointer value, gpointer user_data)
{
	LttvTraceContext *tc = (LttvTraceContext *) key;
	uint64_t *eventNb = (uint64_t *)value;
	uint64_t *sum = (uint64_t *)user_data;

	printf("\t%s: %" PRIu64 "\n", g_quark_to_string(ltt_trace_name(tc->t)),
		*eventNb);
	*sum += *eventNb;
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         TsetStats *
 */
void gdnDestroyUint64(gpointer data)
{
	free((uint64_t *) data);
}


LTTV_MODULE("sync_chain_batch", "Execute synchronization modules in a "\
	"post-processing step.", "This can be used to quantify the precision "\
	"with which a group of trace is synchronized.", init, destroy,\
	"batchAnalysis", "option", "sync")
