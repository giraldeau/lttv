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
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lookup3.h"
#include "sync_chain.h"

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
static void writeAnalysisGraphsPlotsEval(SyncState* const syncState, const
	unsigned int i, const unsigned int j);
static void writeAnalysisGraphsOptionsEval(SyncState* const syncState, const
	unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerAnalysisEval() __attribute__((constructor (102)));
static guint ghfRttKeyHash(gconstpointer key);
static gboolean gefRttKeyEqual(gconstpointer a, gconstpointer b);
static void gdnDestroyRttKey(gpointer data);
static void gdnDestroyDouble(gpointer data);
static void readRttInfo(GHashTable* rttInfo, FILE* rttFile);
static void positionStream(FILE* stream);

static void gfSum(gpointer data, gpointer userData);
static void gfSumSquares(gpointer data, gpointer userData);
static void ghfPrintExchangeRtt(gpointer key, gpointer value, gpointer user_data);

static void initGraphs(SyncState* const syncState);
static void writeGraphFiles(SyncState* const syncState);
static void dumpBinToFile(const uint32_t* const bin, const uint32_t total,
	FILE* const file);
static void destroyGraphs(SyncState* const syncState);
static unsigned int binNum(const double value) __attribute__((pure));
static double binStart(const unsigned int binNum) __attribute__((pure));
static double binEnd(const unsigned int binNum) __attribute__((pure));


const unsigned int binNb= 10000;
double binBase;

static AnalysisModule analysisModuleEval= {
	.name= "eval",
	.initAnalysis= &initAnalysisEval,
	.destroyAnalysis= &destroyAnalysisEval,
	.analyzeMessage= &analyzeMessageEval,
	.analyzeExchange= &analyzeExchangeEval,
	.analyzeBroadcast= &analyzeBroadcastEval,
	.finalizeAnalysis= &finalizeAnalysisEval,
	.printAnalysisStats= &printAnalysisStatsEval,
	.writeAnalysisGraphsPlots= &writeAnalysisGraphsPlotsEval,
	.writeAnalysisGraphsOptions= &writeAnalysisGraphsOptionsEval,
};

static ModuleOption optionEvalRttFile= {
	.longName= "eval-rtt-file",
	.hasArg= REQUIRED_ARG,
	{.arg= NULL},
	.optionHelp= "specify the file containing RTT information",
	.argHelp= "FILE",
};


/*
 * Analysis module registering function
 */
static void registerAnalysisEval()
{
	g_queue_push_tail(&analysisModules, &analysisModuleEval);
	g_queue_push_tail(&moduleOptions, &optionEvalRttFile);
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

	analysisData->rttInfo= g_hash_table_new_full(&ghfRttKeyHash,
		&gefRttKeyEqual, &gdnDestroyRttKey, &gdnDestroyDouble);
	if (optionEvalRttFile.arg)
	{
		FILE* rttStream;
		int retval;

		rttStream= fopen(optionEvalRttFile.arg, "r");
		if (rttStream == NULL)
		{
			g_error(strerror(errno));
		}

		readRttInfo(analysisData->rttInfo, rttStream);

		retval= fclose(rttStream);
		if (retval == EOF)
		{
			g_error(strerror(errno));
		}
	}

	if (syncState->stats)
	{
		analysisData->stats= calloc(1, sizeof(AnalysisStatsEval));
		analysisData->stats->broadcastDiffSum= 0.;

		analysisData->stats->messageStats= malloc(syncState->traceNb *
			sizeof(MessageStats*));
		for (i= 0; i < syncState->traceNb; i++)
		{
			analysisData->stats->messageStats[i]= calloc(syncState->traceNb,
				sizeof(MessageStats));
		}

		analysisData->stats->exchangeRtt=
			g_hash_table_new_full(&ghfRttKeyHash, &gefRttKeyEqual,
				&gdnDestroyRttKey, &gdnDestroyDouble);
	}

	if (syncState->graphsStream)
	{
		binBase= exp10(6. / (binNb - 2));
		analysisData->graphs= malloc(sizeof(AnalysisGraphsEval));
		initGraphs(syncState);
	}
}


/*
 * Create and open files used to store histogram points to genereate
 * graphs. Allocate and populate array to store file pointers.
 *
 * Also create data structures to store histogram points during analysis.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void initGraphs(SyncState* const syncState)
{
	unsigned int i, j;
	int retval;
	char* cwd;
	char name[36];
	AnalysisDataEval* analysisData= syncState->analysisData;

	cwd= changeToGraphDir(syncState->graphsDir);

	analysisData->graphs->ttPoints= malloc(syncState->traceNb *
		sizeof(FILE**));
	analysisData->graphs->ttBinsArray= malloc(syncState->traceNb *
		sizeof(uint32_t**));
	analysisData->graphs->ttBinsTotal= malloc(syncState->traceNb *
		sizeof(uint32_t*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->graphs->ttPoints[i]= malloc(syncState->traceNb *
			sizeof(FILE*));
		analysisData->graphs->ttBinsArray[i]= malloc(syncState->traceNb *
			sizeof(uint32_t*));
		analysisData->graphs->ttBinsTotal[i]= calloc(syncState->traceNb,
			sizeof(uint32_t));
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= snprintf(name, sizeof(name),
					"analysis_eval_tt-%03u_to_%03u.data", i, j);
				if (retval > sizeof(name) - 1)
				{
					name[sizeof(name) - 1]= '\0';
				}
				if ((analysisData->graphs->ttPoints[i][j]= fopen(name, "w")) ==
					NULL)
				{
					g_error(strerror(errno));
				}

				analysisData->graphs->ttBinsArray[i][j]= calloc(binNb,
					sizeof(uint32_t));
			}
		}
	}

	analysisData->graphs->hrttPoints= malloc(syncState->traceNb *
		sizeof(FILE**));
	analysisData->graphs->hrttBinsArray= malloc(syncState->traceNb *
		sizeof(uint32_t**));
	analysisData->graphs->hrttBinsTotal= malloc(syncState->traceNb *
		sizeof(uint32_t*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->graphs->hrttPoints[i]= malloc(i * sizeof(FILE*));
		analysisData->graphs->hrttBinsArray[i]= malloc(i * sizeof(uint32_t*));
		analysisData->graphs->hrttBinsTotal[i]= calloc(i, sizeof(uint32_t));
		for (j= 0; j < i; j++)
		{
			retval= snprintf(name, sizeof(name),
				"analysis_eval_hrtt-%03u_and_%03u.data", i, j);
			if (retval > sizeof(name) - 1)
			{
				name[sizeof(name) - 1]= '\0';
			}
			if ((analysisData->graphs->hrttPoints[i][j]= fopen(name, "w")) ==
				NULL)
			{
				g_error(strerror(errno));
			}

			analysisData->graphs->hrttBinsArray[i][j]= calloc(binNb,
				sizeof(uint32_t));
		}
	}

	retval= chdir(cwd);
	if (retval == -1)
	{
		g_error(strerror(errno));
	}
	free(cwd);
}


/*
 * Write histogram points to all files to generate graphs.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void writeGraphFiles(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataEval* analysisData= syncState->analysisData;

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				dumpBinToFile(analysisData->graphs->ttBinsArray[i][j],
					analysisData->graphs->ttBinsTotal[i][j] -
					analysisData->graphs->ttBinsArray[i][j][binNb - 1],
					analysisData->graphs->ttPoints[i][j]);
			}

			if (i > j)
			{
				dumpBinToFile(analysisData->graphs->hrttBinsArray[i][j],
					analysisData->graphs->hrttBinsTotal[i][j] -
					analysisData->graphs->hrttBinsArray[i][j][binNb - 1],
					analysisData->graphs->hrttPoints[i][j]);
			}
		}
	}
}


/*
 * Write the content of one bin in a histogram point file
 *
 * Args:
 *   bin:          array of values that make up a histogram
 *   total:        total number of messages in bins 0 to binNb - 2
 *   file:         FILE*
 */
static void dumpBinToFile(const uint32_t* const bin, const uint32_t total,
	FILE* const file)
{
	unsigned int i;

	// Last bin is skipped because is continues till infinity
	for (i= 0; i < binNb - 1; i++)
	{
		if (bin[i] > 0)
		{
			fprintf(file, "%20.9f %20.9f %20.9f\n", (binStart(i) + binEnd(i)) / 2, (double) bin[i]
				/ ((binEnd(i) - binStart(i)) * total), binEnd(i) - binStart(i));
		}
	}
}


/*
 * Close files used to store histogram points to generate graphs. Deallocate
 * arrays of file pointers and arrays used to store histogram points during
 * analysis.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void destroyGraphs(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataEval* analysisData= syncState->analysisData;
	int retval;

	if (analysisData->graphs == NULL || analysisData->graphs->ttPoints ==
		NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= fclose(analysisData->graphs->ttPoints[i][j]);
				if (retval != 0)
				{
					g_error(strerror(errno));
				}

				free(analysisData->graphs->ttBinsArray[i][j]);
			}
		}
		free(analysisData->graphs->ttPoints[i]);
		free(analysisData->graphs->ttBinsArray[i]);
		free(analysisData->graphs->ttBinsTotal[i]);
	}
	free(analysisData->graphs->ttPoints);
	free(analysisData->graphs->ttBinsArray);
	free(analysisData->graphs->ttBinsTotal);

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			retval= fclose(analysisData->graphs->hrttPoints[i][j]);
			if (retval != 0)
			{
				g_error(strerror(errno));
			}

			free(analysisData->graphs->hrttBinsArray[i][j]);
		}
		free(analysisData->graphs->hrttPoints[i]);
		free(analysisData->graphs->hrttBinsArray[i]);
		free(analysisData->graphs->hrttBinsTotal[i]);
	}
	free(analysisData->graphs->hrttPoints);
	free(analysisData->graphs->hrttBinsArray);
	free(analysisData->graphs->hrttBinsTotal);

	analysisData->graphs->ttPoints= NULL;
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

	g_hash_table_destroy(analysisData->rttInfo);
	analysisData->rttInfo= NULL;

	if (syncState->stats)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			free(analysisData->stats->messageStats[i]);
		}
		free(analysisData->stats->messageStats);

		g_hash_table_destroy(analysisData->stats->exchangeRtt);

		free(analysisData->stats);
	}

	if (syncState->graphsStream && analysisData->graphs)
	{
		destroyGraphs(syncState);
		free(analysisData->graphs);
	}

	free(syncState->analysisData);
	syncState->analysisData= NULL;
}


/*
 * Perform analysis on an event pair.
 *
 * Check if there is message inversion or messages that are too fast.
 *
 * Args:
 *   syncState     container for synchronization data
 *   message       structure containing the events
 */
static void analyzeMessageEval(SyncState* const syncState, Message* const message)
{
	AnalysisDataEval* analysisData;
	MessageStats* messageStats;
	double* rtt;
	double tt;
	struct RttKey rttKey;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataEval*) syncState->analysisData;
	messageStats=
		&analysisData->stats->messageStats[message->outE->traceNum][message->inE->traceNum];

	messageStats->total++;

	tt= wallTimeSub(&message->inE->wallTime, &message->outE->wallTime);
	if (tt <= 0)
	{
		messageStats->inversionNb++;
	}
	else if (syncState->graphsStream)
	{
		analysisData->graphs->ttBinsArray[message->outE->traceNum][message->inE->traceNum][binNum(tt)]++;
		analysisData->graphs->ttBinsTotal[message->outE->traceNum][message->inE->traceNum]++;
	}

	g_assert(message->inE->type == TCP);
	rttKey.saddr=
		message->inE->event.tcpEvent->segmentKey->connectionKey.saddr;
	rttKey.daddr=
		message->inE->event.tcpEvent->segmentKey->connectionKey.daddr;
	rtt= g_hash_table_lookup(analysisData->rttInfo, &rttKey);
	g_debug("rttInfo, looking up (%u, %u)->(%f)", rttKey.saddr,
		rttKey.daddr, rtt ? *rtt : NAN);

	if (rtt)
	{
		g_debug("rttInfo, tt: %f rtt / 2: %f", tt, *rtt / 2.);
		if (tt < *rtt / 2.)
		{
			messageStats->tooFastNb++;
		}
	}
	else
	{
		messageStats->noRTTInfoNb++;
	}
}


/*
 * Perform analysis on multiple messages
 *
 * Measure the RTT
 *
 * Args:
 *   syncState     container for synchronization data
 *   exchange      structure containing the messages
 */
static void analyzeExchangeEval(SyncState* const syncState, Exchange* const exchange)
{
	AnalysisDataEval* analysisData= syncState->analysisData;
	Message* m1= g_queue_peek_tail(exchange->acks);
	Message* m2= exchange->message;
	struct RttKey* rttKey;
	double* rtt, * exchangeRtt;

	if (!syncState->stats)
	{
		return;
	}

	// (T2 - T1) - (T3 - T4)
	rtt= malloc(sizeof(double));
	*rtt= wallTimeSub(&m1->inE->wallTime, &m1->outE->wallTime) -
		wallTimeSub(&m2->outE->wallTime, &m2->inE->wallTime);

	if (syncState->graphsStream)
	{
		unsigned int row= MAX(m1->inE->traceNum, m1->outE->traceNum);
		unsigned int col= MIN(m1->inE->traceNum, m1->outE->traceNum);

		analysisData->graphs->hrttBinsArray[row][col][binNum(*rtt / 2.)]++;
		analysisData->graphs->hrttBinsTotal[row][col]++;
	}

	g_assert(m1->inE->type == TCP);
	rttKey= malloc(sizeof(struct RttKey));
	rttKey->saddr=
		MIN(m1->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
			m1->inE->event.tcpEvent->segmentKey->connectionKey.daddr);
	rttKey->daddr=
		MAX(m1->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
			m1->inE->event.tcpEvent->segmentKey->connectionKey.daddr);
	exchangeRtt= g_hash_table_lookup(analysisData->stats->exchangeRtt,
		rttKey);

	if (exchangeRtt)
	{
		if (*rtt < *exchangeRtt)
		{
			g_hash_table_replace(analysisData->stats->exchangeRtt, rttKey, rtt);
		}
	}
	else
	{
		g_hash_table_insert(analysisData->stats->exchangeRtt, rttKey, rtt);
	}
}


/*
 * Perform analysis on muliple events
 *
 * Sum the broadcast differential delays
 *
 * Args:
 *   syncState     container for synchronization data
 *   broadcast     structure containing the events
 */
static void analyzeBroadcastEval(SyncState* const syncState, Broadcast* const broadcast)
{
	AnalysisDataEval* analysisData;
	double sum= 0, squaresSum= 0;
	double y;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataEval*) syncState->analysisData;

	g_queue_foreach(broadcast->events, &gfSum, &sum);
	g_queue_foreach(broadcast->events, &gfSumSquares, &squaresSum);

	analysisData->stats->broadcastNb++;
	// Because of numerical errors, this can at times be < 0
	y= squaresSum / g_queue_get_length(broadcast->events) - pow(sum /
		g_queue_get_length(broadcast->events), 2.);
	if (y > 0)
	{
		analysisData->stats->broadcastDiffSum+= sqrt(y);
	}
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
 *   Factors[traceNb] identity factors for each trace
 */
static GArray* finalizeAnalysisEval(SyncState* const syncState)
{
	GArray* factors;
	unsigned int i;
	AnalysisDataEval* analysisData= syncState->analysisData;

	if (syncState->graphsStream && analysisData->graphs)
	{
		writeGraphFiles(syncState);
		destroyGraphs(syncState);
		analysisData->graphs= NULL;
	}

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
	printf("\taverage broadcast differential delays: %g\n",
		analysisData->stats->broadcastDiffSum /
		analysisData->stats->broadcastNb);

	printf("\tIndividual evaluation:\n"
		"\t\tTrace pair  Inversions Too fast   (No RTT info) Total\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			MessageStats* messageStats;
			const char* format= "\t\t%3d - %-3d   %-10u %-10u %-10u    %u\n";

			messageStats= &analysisData->stats->messageStats[i][j];

			printf(format, i, j, messageStats->inversionNb, messageStats->tooFastNb,
				messageStats->noRTTInfoNb, messageStats->total);

			messageStats= &analysisData->stats->messageStats[j][i];

			printf(format, j, i, messageStats->inversionNb, messageStats->tooFastNb,
				messageStats->noRTTInfoNb, messageStats->total);
		}
	}

	printf("\tRound-trip times:\n"
		"\t\tHost pair                          RTT from exchanges  RTTs from file (ms)\n");
	g_hash_table_foreach(analysisData->stats->exchangeRtt,
		&ghfPrintExchangeRtt, analysisData->rttInfo);
}


/*
 * A GHFunc for g_hash_table_foreach()
 *
 * Args:
 *   key:          RttKey* where saddr < daddr
 *   value:        double*, RTT estimated from exchanges
 *   user_data     GHashTable* rttInfo
 */
static void ghfPrintExchangeRtt(gpointer key, gpointer value, gpointer user_data)
{
	char addr1[16], addr2[16];
	struct RttKey* rttKey1= key;
	struct RttKey rttKey2= {rttKey1->daddr, rttKey1->saddr};
	double* fileRtt1, *fileRtt2;
	GHashTable* rttInfo= user_data;

	convertIP(addr1, rttKey1->saddr);
	convertIP(addr2, rttKey1->daddr);

	fileRtt1= g_hash_table_lookup(rttInfo, rttKey1);
	fileRtt2= g_hash_table_lookup(rttInfo, &rttKey2);

	printf("\t\t(%15s, %-15s) %-18.3f  ", addr1, addr2, *(double*) value * 1e3);

	if (fileRtt1 || fileRtt2)
	{
		if (fileRtt1)
		{
			printf("%.3f", *fileRtt1 * 1e3);
		}
		if (fileRtt1 && fileRtt2)
		{
			printf(", ");
		}
		if (fileRtt2)
		{
			printf("%.3f", *fileRtt2 * 1e3);
		}
	}
	else
	{
		printf("-");
	}
	printf("\n");
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * Args:
 *    key        struct RttKey*
 */
static guint ghfRttKeyHash(gconstpointer key)
{
	struct RttKey* rttKey;
	uint32_t a, b, c;

	rttKey= (struct RttKey*) key;

	a= rttKey->saddr;
	b= rttKey->daddr;
	c= 0;
	final(a, b, c);

	return c;
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         struct RttKey*
 */
static void gdnDestroyRttKey(gpointer data)
{
	free(data);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         double*
 */
static void gdnDestroyDouble(gpointer data)
{
	free(data);
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * Args:
 *   a, b          RttKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
static gboolean gefRttKeyEqual(gconstpointer a, gconstpointer b)
{
	const struct RttKey* rkA, * rkB;

	rkA= (struct RttKey*) a;
	rkB= (struct RttKey*) b;

	if (rkA->saddr == rkB->saddr && rkA->daddr == rkB->daddr)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
 * Read a file contain minimum round trip time values and fill an array with
 * them. The file is formatted as such:
 * <host1 IP> <host2 IP> <RTT in milliseconds>
 * ip's should be in dotted quad format
 *
 * Args:
 *   rttInfo:      double* rttInfo[RttKey], empty table, will be filled
 *   rttStream:      stream from which to read
 */
static void readRttInfo(GHashTable* rttInfo, FILE* rttStream)
{
	char* line= NULL;
	size_t len;
	int retval;

	positionStream(rttStream);
	retval= getline(&line, &len, rttStream);
	while(!feof(rttStream))
	{
		struct RttKey* rttKey;
		char saddrDQ[20], daddrDQ[20];
		double* rtt;
		char tmp;
		struct in_addr addr;
		unsigned int i;
		struct {
			char* dq;
			size_t offset;
		} loopValues[] = {
			{saddrDQ, offsetof(struct RttKey, saddr)},
			{daddrDQ, offsetof(struct RttKey, daddr)}
		};

		if (retval == -1 && !feof(rttStream))
		{
			g_error(strerror(errno));
		}

		if (line[retval - 1] == '\n')
		{
			line[retval - 1]= '\0';
		}

		rtt= malloc(sizeof(double));
		retval= sscanf(line, " %19s %19s %lf %c", saddrDQ, daddrDQ, rtt,
			&tmp);
		if (retval == EOF)
		{
			g_error(strerror(errno));
		}
		else if (retval != 3)
		{
			g_error("Error parsing RTT file, line was '%s'", line);
		}

		rttKey= malloc(sizeof(struct RttKey));
		for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
		{
			retval= inet_aton(loopValues[i].dq, &addr);
			if (retval == 0)
			{
				g_error("Error converting address '%s'", loopValues[i].dq);
			}
			*(uint32_t*) ((void*) rttKey + loopValues[i].offset)=
				addr.s_addr;
		}

		*rtt/= 1e3;
		g_debug("rttInfo, Inserting (%u, %u)->(%f)", rttKey->saddr,
			rttKey->daddr, *rtt);
		g_hash_table_insert(rttInfo, rttKey, rtt);

		positionStream(rttStream);
		retval= getline(&line, &len, rttStream);
	}

	if (line)
	{
		free(line);
	}
}


/*
 * Advance stream over empty space, empty lines and lines that begin with '#'
 *
 * Args:
 *   stream:     stream, at exit, will be over the first non-empty character
 *               of a line of be at EOF
 */
static void positionStream(FILE* stream)
{
	int firstChar;
	ssize_t retval;
	char* line= NULL;
	size_t len;

	do
	{
		firstChar= fgetc(stream);
		if (firstChar == (int) '#')
		{
			retval= getline(&line, &len, stream);
			if (retval == -1)
			{
				if (feof(stream))
				{
					goto outEof;
				}
				else
				{
					g_error(strerror(errno));
				}
			}
		}
		else if (firstChar == (int) '\n' || firstChar == (int) ' ' ||
			firstChar == (int) '\t')
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
	retval= ungetc(firstChar, stream);
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
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Event*, a UDP broadcast event
 *   user_data     double*, the running sum
 *
 * Returns:
 *   Adds the time of the event to the sum
 */
static void gfSum(gpointer data, gpointer userData)
{
	Event* event= (Event*) data;

	*(double*) userData+= event->wallTime.seconds + event->wallTime.nanosec /
		1e9;
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Event*, a UDP broadcast event
 *   user_data     double*, the running sum
 *
 * Returns:
 *   Adds the square of the time of the event to the sum
 */
static void gfSumSquares(gpointer data, gpointer userData)
{
	Event* event= (Event*) data;

	*(double*) userData+= pow(event->wallTime.seconds + event->wallTime.nanosec
		/ 1e9, 2.);
}


/*
 * Figure out the bin in a histogram to which a value belongs.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   value:        the value, must be >=0, or else expect the unexpected
 *                 (floating point exception)
 */
static unsigned int binNum(const double value)
{
	if (value == 0)
	{
		return 0;
	}
	else
	{
		double result= floor(log(value) / log(binBase)) + (binNb - 1);

		if (result < 0.)
		{
			return 0.;
		}
		else
		{
			return result;
		}
	}
}


/*
 * Figure out the start of the interval of a bin in a histogram. The starting
 * value is included in the interval.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   binNum:       bin number
 */
static double binStart(const unsigned int binNum)
{
	g_assert(binNum < binNb);

	if (binNum == 0)
	{
		return 0;
	}
	else
	{
		return pow(binBase, (double) binNum - (binNb - 1));
	}
}


/*
 * Figure out the end of the interval of a bin in a histogram. The end value
 * is not included in the interval.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   binNum:       bin number
 */
static double binEnd(const unsigned int binNum)
{
	g_assert(binNum < binNb);

	if (binNum < binNb)
	{
		return pow(binBase, (double) binNum - (binNb - 2));
	}
	else
	{
		return INFINITY;
	}
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisGraphsPlotsEval(SyncState* const syncState, const
	unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
		"\t\"analysis_eval_hrtt-%2$03d_and_%1$03d.data\" "
			"title \"RTT/2\" with boxes linetype 1 linewidth 3 "
			"linecolor rgb \"black\" fill transparent solid 0.75, \\\n"
		/*"\t\"analysis_eval_tt-%1$03d_to_%2$03d.data\" "
			"title \"%1$u to %2$u\" with boxes linetype 1 linewidth 3 "
			"linecolor rgb \"black\" fill transparent solid 0.5, \\\n"
		"\t\"analysis_eval_tt-%2$03d_to_%1$03d.data\" "
			"title \"%2$u to %1$u\" with boxes linetype 1 linewidth 3 "
			"linecolor rgb \"black\" fill transparent solid 0.25, \\\n"*/
			, i, j);
	/*
	fprintf(syncState->graphsStream,
		"\t\"analysis_eval_hrtt-%2$03d_and_%1$03d.data\" "
			"title \"RTT/2\" with linespoints linetype 1 linewidth 3 "
			"linecolor rgb \"black\", \\\n"
		"\t\"analysis_eval_tt-%1$03d_to_%2$03d.data\" "
			"title \"%1$u to %2$u\" with linespoints linetype 1 linewidth 3 "
			"linecolor rgb \"gray70\", \\\n"
		"\t\"analysis_eval_tt-%2$03d_to_%1$03d.data\" "
			"title \"%2$u to %1$u\" with linespoints linetype 1 linewidth 3 "
			"linecolor rgb \"gray40\", \\\n", i, j);
*/
}


/*
 * Write the analysis-specific options in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisGraphsOptionsEval(SyncState* const syncState, const
	unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
		"set xlabel \"Message Latency (s)\"\n"
		"set ylabel \"Proportion of messages per second\"\n");
}
