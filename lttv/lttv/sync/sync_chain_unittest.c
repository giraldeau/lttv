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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "event_processing_text.h"
#include "event_matching_tcp.h"
#include "event_matching_broadcast.h"
#include "event_matching_distributor.h"
#include "event_analysis_chull.h"
#include "event_analysis_linreg.h"
#include "event_analysis_eval.h"
#include "sync_chain.h"


struct OptionsInfo
{
	GArray* longOptions;
	GString* optionString;
	GQueue* longIndex;
	GHashTable* shortIndex;
};


const char* processOptions(const int argc, char* const argv[]);
static void usage(const char* const programName);
static void gfPrintModuleOption(gpointer data, gpointer user_data);
static void nullLog(const gchar *log_domain, GLogLevelFlags log_level, const
	gchar *message, gpointer user_data);
static void gfAddModuleOption(gpointer data, gpointer user_data);
static guint ghfCharHash(gconstpointer key);
static gboolean gefCharEqual(gconstpointer a, gconstpointer b);


static ModuleOption optionSyncStats= {
	.shortName= 's',
	.longName= "sync-stats",
	.hasArg= NO_ARG,
	.optionHelp= "Print statistics and debug messages",
};
static char graphsDir[20];
static ModuleOption optionSyncGraphs= {
	.shortName= 'g',
	.longName= "sync-graphs",
	.hasArg= OPTIONAL_ARG,
	.optionHelp= "Output gnuplot graph showing synchronization points",
};
static ModuleOption optionSyncAnalysis= {
	.shortName= 'a',
	.longName= "sync-analysis",
	.hasArg= REQUIRED_ARG,
	.optionHelp= "Specify which algorithm to use for event analysis",
};


/*
 * Implement a sync chain, it is mostly for unittest and it does not depend on
 * lttv
 *
 * Args:
 *   argc, argv:   standard argument arrays
 *
 * Returns:
 *   exit status from main() is always EXIT_SUCCESS
 */
int main(const int argc, char* const argv[])
{
	SyncState* syncState;
	struct timeval startTime, endTime;
	struct rusage startUsage, endUsage;
	GList* result;
	GArray* factors;
	int retval;
	bool stats;
	const char* testCaseName;
	GString* analysisModulesNames;
	unsigned int id;
	AllFactors* allFactors;

	/*
	 * Initialize event modules
	 * Call the "constructor" or initialization function of each event module
	 * so it can register itself. This must be done before elements in
	 * processingModules, matchingModules, analysisModules or moduleOptions
	 * are accessed.
	 */
	registerProcessingText();

	registerMatchingTCP();
	registerMatchingBroadcast();
	registerMatchingDistributor();

	registerAnalysisCHull();
	registerAnalysisLinReg();
	registerAnalysisEval();

	// Initialize data structures
	syncState= malloc(sizeof(SyncState));

	// Process command line arguments
	g_assert(g_queue_get_length(&analysisModules) > 0);
	optionSyncAnalysis.arg= ((AnalysisModule*)
		g_queue_peek_head(&analysisModules))->name;
	analysisModulesNames= g_string_new("Available modules: ");
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
	optionSyncGraphs.arg= graphsDir;

	g_queue_push_head(&moduleOptions, &optionSyncAnalysis);
	g_queue_push_head(&moduleOptions, &optionSyncGraphs);
	g_queue_push_head(&moduleOptions, &optionSyncStats);

	testCaseName= processOptions(argc, argv);

	g_string_free(analysisModulesNames, TRUE);

	if (optionSyncStats.present)
	{
		syncState->stats= true;
		gettimeofday(&startTime, 0);
		getrusage(RUSAGE_SELF, &startUsage);
	}
	else
	{
		syncState->stats= false;
		id= g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, nullLog, NULL);
	}

	if (optionSyncGraphs.present)
	{
        // Create the graph directory right away in case the module initialization
        // functions have something to write in it.
        syncState->graphsDir= optionSyncGraphs.arg;
        syncState->graphsStream= createGraphsDir(syncState->graphsDir);
    }
    else
    {
        syncState->graphsStream= NULL;
        syncState->graphsDir= NULL;
    }

	// Identify modules
	syncState->processingData= NULL;
	result= g_queue_find_custom(&processingModules, "text",
		&gcfCompareProcessing);
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

	// Initialize modules
	syncState->processingModule->initProcessing(syncState, testCaseName);
	syncState->matchingModule->initMatching(syncState);
	syncState->analysisModule->initAnalysis(syncState);

	// Process traceset
	allFactors= syncState->processingModule->finalizeProcessing(syncState);
	factors= reduceFactors(allFactors);
	freeAllFactors(allFactors);

	// Write graphs file
	if (syncState->graphsStream)
	{
		writeGraphsScript(syncState);

		if (fclose(syncState->graphsStream) != 0)
		{
			g_error(strerror(errno));
		}
	}

	// Print statistics
	if (optionSyncStats.present)
	{
		unsigned int i;

		printStats(syncState);

		printf("Resulting synchronization factors:\n");
		for (i= 0; i < factors->len; i++)
		{
			Factors* traceFactors= &g_array_index(factors, Factors, i);
			printf("\ttrace %u drift= %g offset= %g\n", i,
				traceFactors->drift, traceFactors->offset);
		}
	}

	// Destroy modules and clean up
	syncState->processingModule->destroyProcessing(syncState);
	syncState->matchingModule->destroyMatching(syncState);
	syncState->analysisModule->destroyAnalysis(syncState);

	stats= syncState->stats;
	free(syncState);

	if (stats)
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

	if (!optionSyncStats.present)
	{
		g_log_remove_handler(NULL, id);
	}

	return EXIT_SUCCESS;
}


/*
 * Read program arguments and update ModuleOptions structures
 *
 * Args:
 *   argc, argv:   standard argument arrays
 *
 * Returns:
 *   Name of the test case file (first parameter)
 */
const char* processOptions(const int argc, char* const argv[])
{
	int c;
	extern char* optarg;
	extern int optind, opterr, optopt;
	GArray* longOptions;
	GString* optionString;
	GQueue* longIndex;
	int longOption;
	GHashTable* shortIndex;

	longOptions= g_array_sized_new(TRUE, FALSE, sizeof(struct option),
		g_queue_get_length(&moduleOptions));
	optionString= g_string_new("");
	longIndex= g_queue_new();
	shortIndex= g_hash_table_new(&ghfCharHash, &gefCharEqual);

	g_queue_foreach(&moduleOptions, &gfAddModuleOption, &(struct OptionsInfo)
		{longOptions, optionString, longIndex, shortIndex});

	do
	{
		int optionIndex= 0;
		ModuleOption* moduleOption;

		longOption= -1;
		c= getopt_long(argc, argv, optionString->str, (struct option*)
			longOptions->data, &optionIndex);

		if (longOption >= 0 && longOption < g_queue_get_length(longIndex))
		{
			moduleOption= g_queue_peek_nth(longIndex, longOption);
		}
		else if ((moduleOption= g_hash_table_lookup(shortIndex, &c)) != NULL)
		{
		}
		else if (c == -1)
		{
			break;
		}
		else if (c == '?')
		{
			usage(argv[0]);
			abort();
		}
		else
		{
			g_error("Option parse error");
		}

		moduleOption->present= true;

		if (moduleOption->hasArg == REQUIRED_ARG)
		{
			moduleOption->arg= optarg;
		}
		if (moduleOption->hasArg == OPTIONAL_ARG && optarg)
		{
			moduleOption->arg= optarg;
		}
	} while (c != -1);

	g_array_free(longOptions, TRUE);
	g_string_free(optionString, TRUE);
	g_queue_free(longIndex);
	g_hash_table_destroy(shortIndex);

	if (argc <= optind)
	{
		fprintf(stderr, "Test file unspecified\n");
		usage(argv[0]);
		abort();
	}

	return argv[optind];
}


/*
 * Print information about program options and arguments.
 *
 * Args:
 *   programName:  name of the program, as contained in argv[0] for example
 */
static void usage(const char* const programName)
{
	printf(
		"%s [options] <test file>\n"
		"Options:\n", programName);

	g_queue_foreach(&moduleOptions, &gfPrintModuleOption, NULL);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Print analysis module names.
 *
 * Args:
 *   data:         ModuleOption*, option
 *   user_data:    NULL
 */
static void gfPrintModuleOption(gpointer data, gpointer user_data)
{
	ModuleOption* option= data;
	int width= 0, sum= 0;
	const int colWidth= 27;

	printf("\t");

	if (option->shortName)
	{
		printf("-%c, %n", option->shortName, &width);
		sum+= width;
	}

	printf("--%-s%n", option->longName, &width);
	sum+= width;

	if (option->hasArg == REQUIRED_ARG || option->hasArg == OPTIONAL_ARG)
	{
		printf("=[..]%n", &width);
		sum+= width;
	}

	if (option->optionHelp)
	{
		printf("%*s%s\n", colWidth - sum > 0 ? colWidth - sum : 0, "", option->optionHelp);
	}

	if (option->argHelp)
	{
		printf("\t%*s%s\n", colWidth, "", option->argHelp);
	}

	if ((option->hasArg == REQUIRED_ARG || option->hasArg == OPTIONAL_ARG) && option->arg)
	{
		printf("\t%*sDefault value: %s\n", colWidth, "", option->arg);
	}
}


/*
 * A Glib log function which does nothing.
 */
static void nullLog(const gchar *log_domain, GLogLevelFlags log_level, const
	gchar *message, gpointer user_data)
{}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data:         ModuleOption*, option
 *   user_data:    struct OptionsInfo*, add option to this array of struct option
 */
static void gfAddModuleOption(gpointer data, gpointer user_data)
{
	ModuleOption* option= data;
	struct OptionsInfo* optionsInfo= user_data;
	struct option newOption;
	// "[mixing enumerations] can still be considered bad style even though it
	// is not strictly illegal" c.faq 2.22
	const int conversion[]= {
        [NO_ARG]= no_argument,
        [OPTIONAL_ARG]= optional_argument,
        [REQUIRED_ARG]= required_argument,
    };
	const char* colons[]= {
        [NO_ARG]= "",
        [OPTIONAL_ARG]= "::",
        [REQUIRED_ARG]= ":",
    };

	newOption.name= option->longName;
	newOption.has_arg= conversion[option->hasArg];
	newOption.flag= NULL;
	newOption.val= g_queue_get_length(optionsInfo->longIndex);

	g_array_append_val(optionsInfo->longOptions, newOption);
	if (option->shortName)
	{
		g_string_append_c(optionsInfo->optionString, option->shortName);
		g_string_append(optionsInfo->optionString, colons[option->hasArg]);

		g_hash_table_insert(optionsInfo->shortIndex, &option->shortName,
			option);
	}
	g_queue_push_tail(optionsInfo->longIndex, option);
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * Args:
 *    key        char*, just one character
 */
static guint ghfCharHash(gconstpointer key)
{
    return *(char*) key;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * Args:
 *   a, b          char*, just one character each
 *
 * Returns:
 *   TRUE if both values are equal
 */
static gboolean gefCharEqual(gconstpointer a, gconstpointer b)
{
	if (*(char*) a == *(char*) b)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
