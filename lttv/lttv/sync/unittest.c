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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sync_chain.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


static void timeDiff(struct timeval* const end, const struct timeval* const start);
static void usage(const char* const programName);
static gint gcfCompareAnalysis(gconstpointer a, gconstpointer b);
static void gfAppendAnalysisName(gpointer data, gpointer user_data);
static unsigned int readTraceNb(FILE* testCase);
static void skipCommentLines(FILE* testCase);
static void processEvents(SyncState* const syncState, FILE* testCase);
static void nullLog(const gchar *log_domain, GLogLevelFlags log_level, const
	gchar *message, gpointer user_data);

GQueue processingModules= G_QUEUE_INIT;
GQueue matchingModules= G_QUEUE_INIT;
GQueue analysisModules= G_QUEUE_INIT;

// time values in test case files will be scaled by this factor
const double freq= 1e9;


/*
 * Create matching and analysis modules and feed them events read from a text
 * file.
 *
 * Idealy, this would've been a processing module but sync_chain.c and
 * ProcessingModule use some LTTV-specific types and functions. Unfortunately,
 * there is some code duplication from sync_chain.c
 *
 */
int main(int argc, char* argv[])
{
	int c;
	extern char* optarg;
	extern int optind, opterr, optopt;
	bool optionSyncStats= false;
	char* optionGraphsDir= NULL;
	FILE* testCase= NULL;

	SyncState* syncState;
	struct timeval startTime, endTime;
	struct rusage startUsage, endUsage;
	GList* result;
	char* cwd;
	FILE* graphsStream;
	int graphsFp;
	GArray* factors;

	int retval;

	syncState= malloc(sizeof(SyncState));

	g_assert(g_queue_get_length(&analysisModules) > 0);
	syncState->analysisModule= g_queue_peek_head(&analysisModules);

	do
	{
		int optionIndex= 0;

		static struct option longOptions[]=
		{
			{"sync-stats", no_argument, 0, 's'},
			{"sync-graphs", optional_argument, 0, 'g'},
			{"sync-analysis", required_argument, 0, 'a'},
			{0, 0, 0, 0}
		};

		c= getopt_long(argc, argv, "sg::a:", longOptions, &optionIndex);

		switch (c)
		{
			case -1:
			case 0:
				break;

			case 's':
				if (!optionSyncStats)
				{
					gettimeofday(&startTime, 0);
					getrusage(RUSAGE_SELF, &startUsage);
				}
				optionSyncStats= true;
				break;

			case 'g':
				if (optarg)
				{
					printf("xxx:%s\n", optarg);
					optionGraphsDir= malloc(strlen(optarg));
					strcpy(optionGraphsDir, optarg);
				}
				else
				{
					optionGraphsDir= malloc(20);
					retval= snprintf(optionGraphsDir, 20, "graphs-%d",
						getpid());
					if (retval > 20 - 1)
					{
						optionGraphsDir[20 - 1]= '\0';
					}
				}
				break;

			case 'a':
				printf("xxx:%s\n", optarg);
				result= g_queue_find_custom(&analysisModules, optarg,
					&gcfCompareAnalysis);
				if (result != NULL)
				{
					syncState->analysisModule= (AnalysisModule*) result->data;
				}
				else
				{
					g_error("Analysis module '%s' not found", optarg);
				}
				break;

			case '?':
				usage(argv[0]);
				abort();

			default:
				g_error("Option parse error");
		}
	} while (c != -1);

	if (argc <= optind)
	{
		fprintf(stderr, "Test file unspecified\n");
		usage(argv[0]);
		abort();
	}

	testCase= fopen(argv[optind], "r");
	if (testCase == NULL)
	{
		g_error(strerror(errno));
	}

	// Initialize data structures
	syncState->traceNb= readTraceNb(testCase);

	if (optionSyncStats)
	{
		syncState->stats= true;
	}
	else
	{
		syncState->stats= false;
	}

	syncState->graphs= optionGraphsDir;

	if (!optionSyncStats)
	{
		g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, nullLog, NULL);
	}

	// Identify and initialize matching and analysis modules
	syncState->matchingData= NULL;
	syncState->analysisData= NULL;

	g_assert(g_queue_get_length(&matchingModules) == 1);
	syncState->matchingModule= (MatchingModule*)
		g_queue_peek_head(&matchingModules);

	graphsStream= NULL;
	if (syncState->graphs)
	{
		// Create the graph directory right away in case the module initialization
		// functions have something to write in it.
		cwd= changeToGraphDir(syncState->graphs);

		if (syncState->matchingModule->writeMatchingGraphsPlots != NULL)
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

	syncState->matchingModule->initMatching(syncState);
	syncState->analysisModule->initAnalysis(syncState);

	// Process traceset
	processEvents(syncState, testCase);

	factors= syncState->matchingModule->finalizeMatching(syncState);

	// Write graphs file
	if (graphsStream != NULL)
	{
		unsigned int i, j;

		fprintf(graphsStream,
			"#!/usr/bin/gnuplot\n\n"
			"#set terminal postscript eps color size 8in,6in\n");

		// Cover the upper triangular matrix, i is the reference node.
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= i + 1; j < syncState->traceNb; j++)
			{
				long pos;

				fprintf(graphsStream,
					"\n#set output \"%03d-%03d.eps\"\n"
					"plot \\\n", i, j);

				syncState->matchingModule->writeMatchingGraphsPlots(graphsStream,
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
					"set title \"\"\n"
					"set xlabel \"Clock %1$u\"\n"
					"set xtics nomirror\n"
					"set ylabel \"Clock %2$u\"\n"
					"set ytics nomirror\n"
					"set x2label \"Clock %1$u (s)\"\n"
					"set x2range [GPVAL_X_MIN / %3$.1f : GPVAL_X_MAX / %3$.1f]\n"
					"set x2tics\n"
					"set y2label \"Clock %2$u (s)\"\n"
					"set y2range [GPVAL_Y_MIN / %3$.1f: GPVAL_Y_MAX / %3$.1f]\n"
					"set y2tics\n"
					"set key inside right bottom\n", i, j, freq);

				syncState->matchingModule->writeMatchingGraphsOptions(graphsStream,
					syncState, i, j);

				fprintf(graphsStream, "replot\n\n"
					"pause -1\n");
			}
		}

		if (fclose(graphsStream) != 0)
		{
			g_error(strerror(errno));
		}
	}
	if (optionGraphsDir)
	{
		free(optionGraphsDir);
	}

	if (optionSyncStats && syncState->matchingModule->printMatchingStats !=
		NULL)
	{
		unsigned int i;

		syncState->matchingModule->printMatchingStats(syncState);

		printf("Resulting synchronization factors:\n");
		for (i= 0; i < syncState->traceNb; i++)
		{
			Factors* traceFactors;

			traceFactors= &g_array_index(factors, Factors, i);
			printf("\ttrace %u drift= %g offset= %g\n", i,
				traceFactors->drift, traceFactors->offset);
		}
	}

	syncState->matchingModule->destroyMatching(syncState);
	syncState->analysisModule->destroyAnalysis(syncState);

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

	return EXIT_SUCCESS;
}


/*
 * Print information about program options and arguments.
 *
 * Args:
 *   programName:  name of the program, as contained in argv[0] for example
 */
static void usage(const char* const programName)
{
	GString* analysisModulesNames;

	analysisModulesNames= g_string_new("");
	g_queue_foreach(&analysisModules, &gfAppendAnalysisName,
		analysisModulesNames);
	// remove the last ", "
	g_string_truncate(analysisModulesNames, analysisModulesNames->len - 2);

	printf(
		"%s [options] <test file>\n"
		"Options:\n"
		"\t-s, --sync-stats                 Print statistics and debug messages\n"
		"\t-g, --sync-graphs[=OUPUT_DIR]    Generate graphs\n"
		"\t-a, --sync-analysis=MODULE_NAME  Specify which module to use for analysis\n"
		"\t                                 Available modules: %s\n",
		programName, analysisModulesNames->str);

	g_string_free(analysisModulesNames, TRUE);
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
 * Read trace number from the test case stream. The trace number should be the
 * first non-comment line and should be an unsigned int by itself on a line.
 *
 * Args:
 *   testCase:     test case stream
 *
 * Returns:
 *   The trace number
 */
static unsigned int readTraceNb(FILE* testCase)
{
	unsigned int result;
	int retval;
	char* line= NULL;
	size_t len;
	char tmp;

	skipCommentLines(testCase);
	retval= getline(&line, &len, testCase);
	if (retval == -1)
	{
		if (feof(testCase))
		{
			g_error("Unexpected end of file while looking for number of traces");
		}
		else
		{
			g_error(strerror(errno));
		}
	}
	if (line[retval - 1] == '\n')
	{
		line[retval - 1]= '\0';
	}

	retval= sscanf(line, " %u %c", &result, &tmp);
	if (retval == EOF || retval != 1)
	{
		g_error("Error parsing test file while looking for number of traces, line was '%s'", line);

		// Not really needed but avoids warning from gcc
		abort();
	}

	return result;
}


/*
 * Advance testCase stream over empty space, empty lines and lines that begin
 * with '#'
 *
 * Args:
 *   testCase:     test case stream
 */
static void skipCommentLines(FILE* testCase)
{
	int firstChar;
	ssize_t retval;
	char* line= NULL;
	size_t len;

	do
	{
		firstChar= fgetc(testCase);
		if (firstChar == (int) '#')
		{
			retval= getline(&line, &len, testCase);
			if (retval == -1)
			{
				if (feof(testCase))
				{
					goto outEof;
				}
				else
				{
					g_error(strerror(errno));
				}
			}
		}
		else if (firstChar == (int) '\n' || firstChar == (int) ' ')
		{}
		else if (firstChar == EOF)
		{
			goto outEof;
		}
		else
		{
			break;
		}
	} while (true);
	retval= ungetc(firstChar, testCase);
	if (retval == EOF)
	{
		g_error("Error: ungetc()");
	}

outEof:
	if (line)
	{
		free(line);
	}
}


/*
 * Make up events from the messages in the test case. Dispatch those events to
 * the matching module.
 */
static void processEvents(SyncState* const syncState, FILE* testCase)
{
	char* line= NULL;
	size_t len;
	int retval;
	unsigned int addressOffset;
	unsigned int* seq;

	// Trace numbers run from 0 to traceNb - 1. addressOffset is added to a
	// traceNum to convert it to an address.
	addressOffset= pow(10, floor(log(syncState->traceNb - 1) / log(10)) + 1);

	seq= calloc(syncState->traceNb, sizeof(unsigned int));

	skipCommentLines(testCase);
	retval= getline(&line, &len, testCase);
	while(!feof(testCase))
	{
		unsigned int sender, receiver;
		double sendTime, recvTime;
		char tmp;
		NetEvent* event;

		if (retval == -1 && !feof(testCase))
		{
			g_error(strerror(errno));
		}

		if (line[len - 1] == '\n')
		{
			line[len - 1]= '\0';
		}

		retval= sscanf(line, " %u %u %lf %lf %c", &sender, &receiver,
			&sendTime, &recvTime, &tmp);
		if (retval == EOF)
		{
			g_error(strerror(errno));
		}
		else if (retval != 4)
		{
			g_error("Error parsing test file while looking for data point, line was '%s'", line);
		}

		if (sender + 1 > syncState->traceNb)
		{
			g_error("Error parsing test file, sender is out of range, line was '%s'", line);
		}

		if (receiver + 1 > syncState->traceNb)
		{
			g_error("Error parsing test file, receiver is out of range, line was '%s'", line);
		}

		// Output event
		event= malloc(sizeof(NetEvent));
		event->traceNum= sender;
		event->tsc= round(sendTime * freq);
		event->skb= NULL;
		event->packetKey= malloc(sizeof(PacketKey));
		event->packetKey->ihl= 5;
		event->packetKey->tot_len= 40;
		event->packetKey->connectionKey.saddr= sender + addressOffset;
		event->packetKey->connectionKey.daddr= receiver + addressOffset;
		event->packetKey->connectionKey.source= 57645;
		event->packetKey->connectionKey.dest= 80;
		event->packetKey->seq= seq[sender];
		event->packetKey->ack_seq= 0;
		event->packetKey->doff= 5;
		event->packetKey->ack= 0;
		event->packetKey->rst= 0;
		event->packetKey->syn= 1;
		event->packetKey->fin= 0;

		syncState->matchingModule->matchEvent(syncState, event, OUT);

		// Input event
		event= malloc(sizeof(NetEvent));
		event->traceNum= receiver;
		event->tsc= round(recvTime * freq);
		event->skb= NULL;
		event->packetKey= malloc(sizeof(PacketKey));
		event->packetKey->ihl= 5;
		event->packetKey->tot_len= 40;
		event->packetKey->connectionKey.saddr= sender + addressOffset;
		event->packetKey->connectionKey.daddr= receiver + addressOffset;
		event->packetKey->connectionKey.source= 57645;
		event->packetKey->connectionKey.dest= 80;
		event->packetKey->seq= seq[sender];
		event->packetKey->ack_seq= 0;
		event->packetKey->doff= 5;
		event->packetKey->ack= 0;
		event->packetKey->rst= 0;
		event->packetKey->syn= 1;
		event->packetKey->fin= 0;

		syncState->matchingModule->matchEvent(syncState, event, IN);

		seq[sender]++;

		skipCommentLines(testCase);
		retval= getline(&line, &len, testCase);
	}

	free(seq);

	if (line)
	{
		free(line);
	}
}


/*
 * A Glib log function which does nothing.
 */
static void nullLog(const gchar *log_domain, GLogLevelFlags log_level, const
	gchar *message, gpointer user_data)
{}
