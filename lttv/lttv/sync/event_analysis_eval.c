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
#include "event_analysis_chull.h"

#include "event_analysis_eval.h"


struct WriteHistogramInfo
{
	GHashTable* rttInfo;
	FILE* graphsStream;
};

#ifdef HAVE_LIBGLPK
struct LPAddRowInfo
{
	glp_prob* lp;
	int boundType;
	GArray* iArray, * jArray, * aArray;
};
#endif

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
static void writeAnalysisTraceTimeBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTimeForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTraceBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTraceForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);

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
static void ghfPrintExchangeRtt(gpointer key, gpointer value, gpointer
	user_data);

static void hitBin(struct Bins* const bins, const double value);
static unsigned int binNum(const double value) __attribute__((pure));
static double binStart(const unsigned int binNum) __attribute__((pure));
static double binEnd(const unsigned int binNum) __attribute__((pure));
static uint32_t normalTotal(struct Bins* const bins) __attribute__((const));

static AnalysisHistogramEval* constructAnalysisHistogramEval(const char* const
	graphsDir, const struct RttKey* const rttKey);
static void destroyAnalysisHistogramEval(AnalysisHistogramEval* const
	histogram);
static void gdnDestroyAnalysisHistogramEval(gpointer data);
static void ghfWriteHistogram(gpointer key, gpointer value, gpointer
	user_data);
static void dumpBinToFile(const struct Bins* const bins, FILE* const file);
static void writeHistogram(FILE* graphsStream, const struct RttKey* rttKey,
	double* minRtt, AnalysisHistogramEval* const histogram);

static void updateBounds(Bounds** const bounds, Event* const e1, Event* const
	e2);

// The next group of functions is only needed when computing synchronization
// accuracy.
#ifdef HAVE_LIBGLPK
static glp_prob* lpCreateProblem(GQueue* const lowerHull, GQueue* const
	upperHull);
static void gfLPAddRow(gpointer data, gpointer user_data);
static Factors* calculateFactors(glp_prob* const lp, const int direction);
static void calculateCompleteFactors(glp_prob* const lp, FactorsCHull*
	factors);
static FactorsCHull** createAllFactors(const unsigned int traceNb);
static inline void finalizeAnalysisEvalLP(SyncState* const syncState);
#else
static void finalizeAnalysisEvalLP(SyncState* const syncState);
#endif


// initialized in registerAnalysisEval()
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
	.graphFunctions= {
		.writeTraceTimeBackPlots= &writeAnalysisTraceTimeBackPlotsEval,
		.writeTraceTimeForePlots= &writeAnalysisTraceTimeForePlotsEval,
		.writeTraceTraceBackPlots= &writeAnalysisTraceTraceBackPlotsEval,
		.writeTraceTraceForePlots= &writeAnalysisTraceTraceForePlotsEval,
	}
};

static ModuleOption optionEvalRttFile= {
	.longName= "eval-rtt-file",
	.hasArg= REQUIRED_ARG,
	.optionHelp= "specify the file containing RTT information",
	.argHelp= "FILE",
};


/*
 * Analysis module registering function
 */
static void registerAnalysisEval()
{
	binBase= exp10(6. / (BIN_NB - 3));

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
	unsigned int i, j;

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

#ifdef HAVE_LIBGLPK
		analysisData->stats->chFactorsArray= NULL;
		analysisData->stats->lpFactorsArray= NULL;
#endif
	}

	if (syncState->graphsStream)
	{
		AnalysisGraphsEval* graphs= malloc(sizeof(AnalysisGraphsEval));

		analysisData->graphs= graphs;

		graphs->histograms= g_hash_table_new_full(&ghfRttKeyHash,
			&gefRttKeyEqual, &gdnDestroyRttKey,
			&gdnDestroyAnalysisHistogramEval);

		graphs->bounds= malloc(syncState->traceNb * sizeof(Bounds*));
		for (i= 0; i < syncState->traceNb; i++)
		{
			graphs->bounds[i]= malloc(i * sizeof(Bounds));
			for (j= 0; j < i; j++)
			{
				graphs->bounds[i][j].min= UINT64_MAX;
				graphs->bounds[i][j].max= 0;
			}
		}

#ifdef HAVE_LIBGLPK
		graphs->lps= NULL;
		graphs->lpFactorsArray= NULL;
#endif
	}

	if (syncState->stats || syncState->graphsStream)
	{
		GList* result;

		analysisData->chullSS= malloc(sizeof(SyncState));
		memcpy(analysisData->chullSS, syncState, sizeof(SyncState));
		analysisData->chullSS->stats= false;
		analysisData->chullSS->analysisData= NULL;
		result= g_queue_find_custom(&analysisModules, "chull",
			&gcfCompareAnalysis);
		analysisData->chullSS->analysisModule= (AnalysisModule*) result->data;
		analysisData->chullSS->analysisModule->initAnalysis(analysisData->chullSS);
	}
}


/*
 * Create and open files used to store histogram points to generate graphs.
 * Create data structures to store histogram points during analysis.
 *
 * Args:
 *   graphsDir:    folder where to write files
 *   rttKey:       host pair, make sure saddr < daddr
 */
static AnalysisHistogramEval* constructAnalysisHistogramEval(const char* const
	graphsDir, const struct RttKey* const rttKey)
{
	int retval;
	unsigned int i;
	char* cwd;
	char name[60], saddr[16], daddr[16];
	AnalysisHistogramEval* histogram= calloc(1, sizeof(*histogram));
	const struct {
		size_t pointsOffset;
		const char* fileName;
		const char* host1, *host2;
	} loopValues[]= {
		{offsetof(AnalysisHistogramEval, ttSendPoints),
			"analysis_eval_tt-%s_to_%s.data", saddr, daddr},
		{offsetof(AnalysisHistogramEval, ttRecvPoints),
			"analysis_eval_tt-%s_to_%s.data", daddr, saddr},
		{offsetof(AnalysisHistogramEval, hrttPoints),
			"analysis_eval_hrtt-%s_and_%s.data", saddr, daddr},
	};

	histogram->ttSendBins.min= BIN_NB - 1;
	histogram->ttRecvBins.min= BIN_NB - 1;
	histogram->hrttBins.min= BIN_NB - 1;

	convertIP(saddr, rttKey->saddr);
	convertIP(daddr, rttKey->daddr);

	cwd= changeToGraphsDir(graphsDir);

	for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
	{
		retval= snprintf(name, sizeof(name), loopValues[i].fileName,
			loopValues[i].host1, loopValues[i].host2);
		if (retval > sizeof(name) - 1)
		{
			name[sizeof(name) - 1]= '\0';
		}
		if ((*(FILE**)((void*) histogram + loopValues[i].pointsOffset)=
				fopen(name, "w")) == NULL)
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

	return histogram;
}


/*
 * Close files used to store histogram points to generate graphs.
 *
 * Args:
 *   graphsDir:    folder where to write files
 *   rttKey:       host pair, make sure saddr < daddr
 */
static void destroyAnalysisHistogramEval(AnalysisHistogramEval* const
	histogram)
{
	unsigned int i;
	int retval;
	const struct {
		size_t pointsOffset;
	} loopValues[]= {
		{offsetof(AnalysisHistogramEval, ttSendPoints)},
		{offsetof(AnalysisHistogramEval, ttRecvPoints)},
		{offsetof(AnalysisHistogramEval, hrttPoints)},
	};

	for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
	{
		retval= fclose(*(FILE**)((void*) histogram + loopValues[i].pointsOffset));
		if (retval != 0)
		{
			g_error(strerror(errno));
		}
	}

	free(histogram);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         AnalysisHistogramEval*
 */
static void gdnDestroyAnalysisHistogramEval(gpointer data)
{
	destroyAnalysisHistogramEval(data);
}


/*
 * A GHFunc for g_hash_table_foreach()
 *
 * Args:
 *   key:          RttKey* where saddr < daddr
 *   value:        AnalysisHistogramEval*
 *   user_data     struct WriteHistogramInfo*
 */
static void ghfWriteHistogram(gpointer key, gpointer value, gpointer user_data)
{
	double* rtt1, * rtt2;
	struct RttKey* rttKey= key;
	struct RttKey oppositeRttKey= {.saddr= rttKey->daddr, .daddr=
		rttKey->saddr};
	AnalysisHistogramEval* histogram= value;
	struct WriteHistogramInfo* info= user_data;

	rtt1= g_hash_table_lookup(info->rttInfo, rttKey);
	rtt2= g_hash_table_lookup(info->rttInfo, &oppositeRttKey);

	if (rtt1 == NULL)
	{
		rtt1= rtt2;
	}
	else if (rtt2 != NULL)
	{
		rtt1= MIN(rtt1, rtt2);
	}

	dumpBinToFile(&histogram->ttSendBins, histogram->ttSendPoints);
	dumpBinToFile(&histogram->ttRecvBins, histogram->ttRecvPoints);
	dumpBinToFile(&histogram->hrttBins, histogram->hrttPoints);
	writeHistogram(info->graphsStream, rttKey, rtt1, histogram);
}


/*
 * Write the content of one bin in a histogram point file
 *
 * Args:
 *   bin:          array of values that make up a histogram
 *   file:         FILE*, write to this file
 */
static void dumpBinToFile(const struct Bins* const bins, FILE* const file)
{
	unsigned int i;

	// The first and last bins are skipped, see struct Bins
	for (i= 1; i < BIN_NB - 1; i++)
	{
		if (bins->bin[i] > 0)
		{
			fprintf(file, "%20.9f %20.9f %20.9f\n", (binStart(i) + binEnd(i))
				/ 2., (double) bins->bin[i] / ((binEnd(i) - binStart(i)) *
					bins->total), binEnd(i) - binStart(i));
		}
	}
}


/*
 * Write the analysis-specific plot in the gnuplot script.
 *
 * Args:
 *   graphsStream: write to this file
 *   rttKey:       must be sorted such that saddr < daddr
 *   minRtt:       if available, else NULL
 *   histogram:    struct that contains the bins for the pair of traces
 *                 identified by rttKey
 */
static void writeHistogram(FILE* graphsStream, const struct RttKey* rttKey,
	double* minRtt, AnalysisHistogramEval* const histogram)
{
	char saddr[16], daddr[16];

	convertIP(saddr, rttKey->saddr);
	convertIP(daddr, rttKey->daddr);

	fprintf(graphsStream,
		"\nreset\n"
		"set output \"histogram-%s-%s.eps\"\n"
		"set title \"\"\n"
		"set xlabel \"Message Latency (s)\"\n"
		"set ylabel \"Proportion of messages per second\"\n", saddr, daddr);

	if (minRtt != NULL)
	{
		fprintf(graphsStream,
			"set arrow from %.9f, 0 rto 0, graph 1 "
			"nohead linetype 3 linewidth 3 linecolor rgb \"black\"\n", *minRtt
			/ 2);
	}

	if (normalTotal(&histogram->ttSendBins) ||
		normalTotal(&histogram->ttRecvBins) ||
		normalTotal(&histogram->hrttBins))
	{
		fprintf(graphsStream, "plot \\\n");

		if (normalTotal(&histogram->hrttBins))
		{
			fprintf(graphsStream,
				"\t\"analysis_eval_hrtt-%s_and_%s.data\" "
					"title \"RTT/2\" with linespoints linetype 1 linewidth 2 "
					"linecolor rgb \"black\" pointtype 6 pointsize 1,\\\n",
					saddr, daddr);
		}

		if (normalTotal(&histogram->ttSendBins))
		{
			fprintf(graphsStream,
				"\t\"analysis_eval_tt-%1$s_to_%2$s.data\" "
					"title \"%1$s to %2$s\" with linespoints linetype 4 linewidth 2 "
					"linecolor rgb \"gray60\" pointtype 6 pointsize 1,\\\n",
					saddr, daddr);
		}

		if (normalTotal(&histogram->ttRecvBins))
		{
			fprintf(graphsStream,
				"\t\"analysis_eval_tt-%1$s_to_%2$s.data\" "
					"title \"%1$s to %2$s\" with linespoints linetype 4 linewidth 2 "
					"linecolor rgb \"gray30\" pointtype 6 pointsize 1,\\\n",
					daddr, saddr);
		}

		// Remove the ",\\\n" from the last graph plot line
		if (ftruncate(fileno(graphsStream), ftell(graphsStream) - 3) == -1)
		{
			g_error(strerror(errno));
		}
		if (fseek(graphsStream, 0, SEEK_END) == -1)
		{
			g_error(strerror(errno));
		}
		fprintf(graphsStream, "\n");
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
	unsigned int i, j;
	AnalysisDataEval* analysisData;

	analysisData= (AnalysisDataEval*) syncState->analysisData;

	if (analysisData == NULL)
	{
		return;
	}

	g_hash_table_destroy(analysisData->rttInfo);

	if (syncState->stats)
	{
		AnalysisStatsEval* stats= analysisData->stats;

		for (i= 0; i < syncState->traceNb; i++)
		{
			free(stats->messageStats[i]);
		}
		free(stats->messageStats);

		g_hash_table_destroy(stats->exchangeRtt);

#ifdef HAVE_LIBGLPK
		freeAllFactors(syncState->traceNb, stats->chFactorsArray);
		freeAllFactors(syncState->traceNb, stats->lpFactorsArray);
#endif

		free(stats);
	}

	if (syncState->graphsStream)
	{
		AnalysisGraphsEval* graphs= analysisData->graphs;

		if (graphs->histograms)
		{
			g_hash_table_destroy(graphs->histograms);
		}

		for (i= 0; i < syncState->traceNb; i++)
		{
			free(graphs->bounds[i]);
		}
		free(graphs->bounds);

#ifdef HAVE_LIBGLPK
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= 0; j < i; j++)
			{
				// There seems to be a memory leak in glpk, valgrind reports a
				// loss even if the problem is deleted
				glp_delete_prob(graphs->lps[i][j]);
			}
			free(graphs->lps[i]);
		}
		free(graphs->lps);

		if (!syncState->stats)
		{
			freeAllFactors(syncState->traceNb, graphs->lpFactorsArray);
		}
#endif

		free(graphs);
	}

	if (syncState->stats || syncState->graphsStream)
	{
		analysisData->chullSS->analysisModule->destroyAnalysis(analysisData->chullSS);
		free(analysisData->chullSS);
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
static void analyzeMessageEval(SyncState* const syncState, Message* const
	message)
{
	AnalysisDataEval* analysisData= syncState->analysisData;
	MessageStats* messageStats;
	double* rtt;
	double tt;
	struct RttKey rttKey;

	g_assert(message->inE->type == TCP);

	if (syncState->stats)
	{
		messageStats=
			&analysisData->stats->messageStats[message->outE->traceNum][message->inE->traceNum];
		messageStats->total++;
	}

	tt= wallTimeSub(&message->inE->wallTime, &message->outE->wallTime);
	if (tt <= 0)
	{
		if (syncState->stats)
		{
			messageStats->inversionNb++;
		}
	}
	else if (syncState->graphsStream)
	{
		struct RttKey rttKey= {
			.saddr=MIN(message->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
				message->inE->event.tcpEvent->segmentKey->connectionKey.daddr),
			.daddr=MAX(message->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
				message->inE->event.tcpEvent->segmentKey->connectionKey.daddr),
		};
		AnalysisHistogramEval* histogram=
			g_hash_table_lookup(analysisData->graphs->histograms, &rttKey);

		if (histogram == NULL)
		{
			struct RttKey* tableKey= malloc(sizeof(*tableKey));

			histogram= constructAnalysisHistogramEval(syncState->graphsDir, &rttKey);
			memcpy(tableKey, &rttKey, sizeof(*tableKey));
			g_hash_table_insert(analysisData->graphs->histograms, tableKey, histogram);
		}

		if (message->inE->event.udpEvent->datagramKey->saddr <
			message->inE->event.udpEvent->datagramKey->daddr)
		{
			hitBin(&histogram->ttSendBins, tt);
		}
		else
		{
			hitBin(&histogram->ttRecvBins, tt);
		}
	}

	if (syncState->stats)
	{
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

	if (syncState->graphsStream)
	{
		updateBounds(analysisData->graphs->bounds, message->inE,
			message->outE);
	}

	if (syncState->stats || syncState->graphsStream)
	{
		analysisData->chullSS->analysisModule->analyzeMessage(analysisData->chullSS,
			message);
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
static void analyzeExchangeEval(SyncState* const syncState, Exchange* const
	exchange)
{
	AnalysisDataEval* analysisData= syncState->analysisData;
	Message* m1= g_queue_peek_tail(exchange->acks);
	Message* m2= exchange->message;
	struct RttKey* rttKey;
	double* rtt, * exchangeRtt;

	g_assert(m1->inE->type == TCP);

	// (T2 - T1) - (T3 - T4)
	rtt= malloc(sizeof(double));
	*rtt= wallTimeSub(&m1->inE->wallTime, &m1->outE->wallTime) -
		wallTimeSub(&m2->outE->wallTime, &m2->inE->wallTime);

	rttKey= malloc(sizeof(struct RttKey));
	rttKey->saddr=
		MIN(m1->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
			m1->inE->event.tcpEvent->segmentKey->connectionKey.daddr);
	rttKey->daddr=
		MAX(m1->inE->event.tcpEvent->segmentKey->connectionKey.saddr,
			m1->inE->event.tcpEvent->segmentKey->connectionKey.daddr);

	if (syncState->graphsStream)
	{
		AnalysisHistogramEval* histogram=
			g_hash_table_lookup(analysisData->graphs->histograms, rttKey);

		if (histogram == NULL)
		{
			struct RttKey* tableKey= malloc(sizeof(*tableKey));

			histogram= constructAnalysisHistogramEval(syncState->graphsDir,
				rttKey);
			memcpy(tableKey, rttKey, sizeof(*tableKey));
			g_hash_table_insert(analysisData->graphs->histograms, tableKey,
				histogram);
		}

		hitBin(&histogram->hrttBins, *rtt / 2);
	}

	if (syncState->stats)
	{
		exchangeRtt= g_hash_table_lookup(analysisData->stats->exchangeRtt,
			rttKey);

		if (exchangeRtt)
		{
			if (*rtt < *exchangeRtt)
			{
				g_hash_table_replace(analysisData->stats->exchangeRtt, rttKey, rtt);
			}
			else
			{
				free(rttKey);
				free(rtt);
			}
		}
		else
		{
			g_hash_table_insert(analysisData->stats->exchangeRtt, rttKey, rtt);
		}
	}
	else
	{
		free(rttKey);
		free(rtt);
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
static void analyzeBroadcastEval(SyncState* const syncState, Broadcast* const
	broadcast)
{
	AnalysisDataEval* analysisData= syncState->analysisData;

	if (syncState->stats)
	{
		double sum= 0, squaresSum= 0;
		double y;

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

	if (syncState->graphsStream)
	{
		unsigned int i, j;
		GArray* events;
		unsigned int eventNb= broadcast->events->length;

		events= g_array_sized_new(FALSE, FALSE, sizeof(Event*), eventNb);
		g_queue_foreach(broadcast->events, &gfAddEventToArray, events);

		for (i= 0; i < eventNb; i++)
		{
			for (j= 0; j < eventNb; j++)
			{
				Event* eventI= g_array_index(events, Event*, i), * eventJ=
					g_array_index(events, Event*, j);

				if (eventI->traceNum < eventJ->traceNum)
				{
					updateBounds(analysisData->graphs->bounds, eventI, eventJ);
				}
			}
		}

		g_array_free(events, TRUE);
	}
}


/*
 * Finalize the factor calculations. Since this module does not really
 * calculate factors, identity factors are returned. Instead, histograms are
 * written out and histogram structures are freed.
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

	if (syncState->graphsStream && analysisData->graphs->histograms)
	{
		g_hash_table_foreach(analysisData->graphs->histograms,
			&ghfWriteHistogram, &(struct WriteHistogramInfo) {.rttInfo=
			analysisData->rttInfo, .graphsStream= syncState->graphsStream});
		g_hash_table_destroy(analysisData->graphs->histograms);
		analysisData->graphs->histograms= NULL;
	}

	finalizeAnalysisEvalLP(syncState);

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
	unsigned int i, j, k;
	unsigned int totInversion= 0, totTooFast= 0, totNoInfo= 0, totTotal= 0;
	int charNb;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataEval*) syncState->analysisData;

	printf("Synchronization evaluation analysis stats:\n");
	if (analysisData->stats->broadcastNb)
	{
		printf("\tsum of broadcast differential delays: %g\n",
			analysisData->stats->broadcastDiffSum);
		printf("\taverage broadcast differential delay: %g\n",
			analysisData->stats->broadcastDiffSum /
			analysisData->stats->broadcastNb);
	}

	printf("\tIndividual evaluation:\n"
		"\t\tTrace pair  Inversions        Too fast          No RTT info  Total\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			MessageStats* messageStats;
			struct {
				unsigned int t1, t2;
			} loopValues[]= {
				{i, j},
				{j, i}
			};

			for (k= 0; k < sizeof(loopValues) / sizeof(*loopValues); k++)
			{
				messageStats=
					&analysisData->stats->messageStats[loopValues[k].t1][loopValues[k].t2];

				printf("\t\t%3d - %-3d   ", loopValues[k].t1, loopValues[k].t2);
				printf("%u (%u%%)%n", messageStats->inversionNb, (unsigned
						int) ceil((double) messageStats->inversionNb /
						messageStats->total * 100), &charNb);
				printf("%*s", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ");
				printf("%u (%u%%)%n", messageStats->tooFastNb, (unsigned int)
					ceil((double) messageStats->tooFastNb /
						messageStats->total * 100), &charNb);
				printf("%*s%-10u   %u\n", 17 - charNb > 0 ? 17 - charNb + 1:
					1, " ", messageStats->noRTTInfoNb, messageStats->total);

				totInversion+= messageStats->inversionNb;
				totTooFast+= messageStats->tooFastNb;
				totNoInfo+= messageStats->noRTTInfoNb;
				totTotal+= messageStats->total;
			}
		}
	}

	printf("\t\t  total     ");
	printf("%u (%u%%)%n", totInversion, (unsigned int) ceil((double)
			totInversion / totTotal * 100), &charNb);
	printf("%*s", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ");
	printf("%u (%u%%)%n", totTooFast, (unsigned int) ceil((double) totTooFast
			/ totTotal * 100), &charNb);
	printf("%*s%-10u   %u\n", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ",
		totNoInfo, totTotal);

	printf("\tRound-trip times:\n"
		"\t\tHost pair                          RTT from exchanges  RTTs from file (ms)\n");
	g_hash_table_foreach(analysisData->stats->exchangeRtt,
		&ghfPrintExchangeRtt, analysisData->rttInfo);

	printf("\tConvex hull factors comparisons:\n"
		"\t\tTrace pair  Factors type  Differences (lp - chull)\n"
		"\t\t                          a0                    a1\n"
		"\t\t                          Min        Max        Min        Max\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			FactorsCHull* chFactors= &analysisData->stats->chFactorsArray[i][j];
			FactorsCHull* lpFactors= &analysisData->stats->lpFactorsArray[i][j];

			printf("\t\t%3d - %-3d   ", i, j);
			if (lpFactors->type == chFactors->type)
			{
				if (lpFactors->type == MIDDLE)
				{
					printf("%-13s %-10.4g %-10.4g %-10.4g %.4g\n",
						approxNames[lpFactors->type],
						lpFactors->min->offset - chFactors->min->offset,
						lpFactors->max->offset - chFactors->max->offset,
						lpFactors->min->drift - chFactors->min->drift,
						lpFactors->max->drift - chFactors->max->drift);
				}
				else if (lpFactors->type == ABSENT)
				{
					printf("%s\n", approxNames[lpFactors->type]);
				}
			}
			else
			{
				printf("Different! %s and %s\n", approxNames[lpFactors->type],
					approxNames[chFactors->type]);
			}
		}
	}
}


/*
 * A GHFunc for g_hash_table_foreach()
 *
 * Args:
 *   key:          RttKey* where saddr < daddr
 *   value:        double*, RTT estimated from exchanges
 *   user_data     GHashTable* rttInfo
 */
static void ghfPrintExchangeRtt(gpointer key, gpointer value, gpointer
	user_data)
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
 * Update a struct Bins according to a new value
 *
 * Args:
 *   bins:         the structure containing bins to build a histrogram
 *   value:        the new value
 */
static void hitBin(struct Bins* const bins, const double value)
{
	unsigned int binN= binNum(value);

	if (binN < bins->min)
	{
		bins->min= binN;
	}
	else if (binN > bins->max)
	{
		bins->max= binN;
	}

	bins->total++;

	bins->bin[binN]++;
}


/*
 * Figure out the bin in a histogram to which a value belongs.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   value:        in the range -INFINITY to INFINITY
 *
 * Returns:
 *   The number of the bin in a struct Bins.bin
 */
static unsigned int binNum(const double value)
{
	if (value <= 0)
	{
		return 0;
	}
	else if (value < binEnd(1))
	{
		return 1;
	}
	else if (value >= binStart(BIN_NB - 1))
	{
		return BIN_NB - 1;
	}
	else
	{
		return floor(log(value) / log(binBase)) + BIN_NB + 1;
	}
}


/*
 * Figure out the start of the interval of a bin in a histogram. See struct
 * Bins.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   binNum:       bin number
 *
 * Return:
 *   The start of the interval, this value is included in the interval (except
 *   for -INFINITY, naturally)
 */
static double binStart(const unsigned int binNum)
{
	g_assert_cmpuint(binNum, <, BIN_NB);

	if (binNum == 0)
	{
		return -INFINITY;
	}
	else if (binNum == 1)
	{
		return 0.;
	}
	else
	{
		return pow(binBase, (double) binNum - BIN_NB + 1);
	}
}


/*
 * Figure out the end of the interval of a bin in a histogram. See struct
 * Bins.
 *
 * This uses exponentially sized bins that go from 0 to infinity.
 *
 * Args:
 *   binNum:       bin number
 *
 * Return:
 *   The end of the interval, this value is not included in the interval
 */
static double binEnd(const unsigned int binNum)
{
	g_assert_cmpuint(binNum, <, BIN_NB);

	if (binNum == 0)
	{
		return 0.;
	}
	else if (binNum < BIN_NB - 1)
	{
		return pow(binBase, (double) binNum - BIN_NB + 2);
	}
	else
	{
		return INFINITY;
	}
}


/*
 * Return the total number of elements in the "normal" bins (not underflow or
 * overflow)
 *
 * Args:
 *   bins:         the structure containing bins to build a histrogram
 */
static uint32_t normalTotal(struct Bins* const bins)
{
	return bins->total - bins->bin[0] - bins->bin[BIN_NB - 1];
}


/* Update the bounds between two traces
 *
 * Args:
 *   bounds:       the array containing all the trace-pair bounds
 *   e1, e2:       the two related events
 */
static void updateBounds(Bounds** const bounds, Event* const e1, Event* const
	e2)
{
	unsigned int traceI, traceJ;
	uint64_t messageTime;
	Bounds* tpBounds;

	if (e1->traceNum < e2->traceNum)
	{
		traceI= e2->traceNum;
		traceJ= e1->traceNum;
		messageTime= e1->cpuTime;
	}
	else
	{
		traceI= e1->traceNum;
		traceJ= e2->traceNum;
		messageTime= e2->cpuTime;
	}
	tpBounds= &bounds[traceI][traceJ];

	if (messageTime < tpBounds->min)
	{
		tpBounds->min= messageTime;
	}
	if (messageTime > tpBounds->max)
	{
		tpBounds->max= messageTime;
	}
}


#ifdef HAVE_LIBGLPK
/*
 * Create the linear programming problem containing the constraints defined by
 * two half-hulls. The objective function and optimization directions are not
 * written.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 * Returns:
 *   A new glp_prob*, this problem must be freed by the caller with
 *   glp_delete_prob()
 */
static glp_prob* lpCreateProblem(GQueue* const lowerHull, GQueue* const
	upperHull)
{
	unsigned int it;
	const int zero= 0;
	const double zeroD= 0.;
	glp_prob* lp= glp_create_prob();
	unsigned int hullPointNb= g_queue_get_length(lowerHull) +
		g_queue_get_length(upperHull);
	GArray* iArray= g_array_sized_new(FALSE, FALSE, sizeof(int), hullPointNb +
		1);
	GArray* jArray= g_array_sized_new(FALSE, FALSE, sizeof(int), hullPointNb +
		1);
	GArray* aArray= g_array_sized_new(FALSE, FALSE, sizeof(double),
		hullPointNb + 1);
	struct {
		GQueue* hull;
		struct LPAddRowInfo rowInfo;
	} loopValues[2]= {
		{lowerHull, {lp, GLP_UP, iArray, jArray, aArray}},
		{upperHull, {lp, GLP_LO, iArray, jArray, aArray}},
	};

	// Create the LP problem
	glp_term_out(GLP_OFF);
	glp_add_rows(lp, hullPointNb);
	glp_add_cols(lp, 2);

	glp_set_col_name(lp, 1, "a0");
	glp_set_col_bnds(lp, 1, GLP_FR, 0., 0.);
	glp_set_col_name(lp, 2, "a1");
	glp_set_col_bnds(lp, 2, GLP_LO, 0., 0.);

	// Add row constraints
	g_array_append_val(iArray, zero);
	g_array_append_val(jArray, zero);
	g_array_append_val(aArray, zeroD);

	for (it= 0; it < sizeof(loopValues) / sizeof(*loopValues); it++)
	{
		g_queue_foreach(loopValues[it].hull, &gfLPAddRow,
			&loopValues[it].rowInfo);
	}

	g_assert_cmpuint(iArray->len, ==, jArray->len);
	g_assert_cmpuint(jArray->len, ==, aArray->len);
	g_assert_cmpuint(aArray->len - 1, ==, hullPointNb * 2);

	glp_load_matrix(lp, aArray->len - 1, &g_array_index(iArray, int, 0),
		&g_array_index(jArray, int, 0), &g_array_index(aArray, double, 0));

	glp_scale_prob(lp, GLP_SF_AUTO);

	g_array_free(iArray, TRUE);
	g_array_free(jArray, TRUE);
	g_array_free(aArray, TRUE);

	return lp;
}


/*
 * A GFunc for g_queue_foreach(). Add constraints and bounds for one row.
 *
 * Args:
 *   data          Point*, synchronization point for which to add an LP row
 *                 (a constraint)
 *   user_data     LPAddRowInfo*
 */
static void gfLPAddRow(gpointer data, gpointer user_data)
{
	Point* p= data;
	struct LPAddRowInfo* rowInfo= user_data;
	int indexes[2];
	double constraints[2];

	indexes[0]= g_array_index(rowInfo->iArray, int, rowInfo->iArray->len - 1) + 1;
	indexes[1]= indexes[0];

	if (rowInfo->boundType == GLP_UP)
	{
		glp_set_row_bnds(rowInfo->lp, indexes[0], GLP_UP, 0., p->y);
	}
	else if (rowInfo->boundType == GLP_LO)
	{
		glp_set_row_bnds(rowInfo->lp, indexes[0], GLP_LO, p->y, 0.);
	}
	else
	{
		g_assert_not_reached();
	}

	g_array_append_vals(rowInfo->iArray, indexes, 2);
	indexes[0]= 1;
	indexes[1]= 2;
	g_array_append_vals(rowInfo->jArray, indexes, 2);
	constraints[0]= 1.;
	constraints[1]= p->x;
	g_array_append_vals(rowInfo->aArray, constraints, 2);
}


/*
 * Calculate min or max correction factors (as possible) using an LP problem.
 *
 * Args:
 *   lp:           A linear programming problem with constraints and bounds
 *                 initialized.
 *   direction:    The type of factors desired. Use GLP_MAX for max
 *                 approximation factors (a1, the drift or slope is the
 *                 largest) and GLP_MIN in the other case.
 *
 * Returns:
 *   If the calculation was successful, a new Factors struct. Otherwise, NULL.
 *   The calculation will fail if the hull assumptions are not respected.
 */
static Factors* calculateFactors(glp_prob* const lp, const int direction)
{
	int retval, status;
	Factors* factors;

	glp_set_obj_coef(lp, 1, 0.);
	glp_set_obj_coef(lp, 2, 1.);

	glp_set_obj_dir(lp, direction);
	retval= glp_simplex(lp, NULL);
	status= glp_get_status(lp);

	if (retval == 0 && status == GLP_OPT)
	{
		factors= malloc(sizeof(Factors));
		factors->offset= glp_get_col_prim(lp, 1);
		factors->drift= glp_get_col_prim(lp, 2);
	}
	else
	{
		factors= NULL;
	}

	return factors;
}


/*
 * Calculate min, max and approx correction factors (as possible) using an LP
 * problem.
 *
 * Args:
 *   lp:           A linear programming problem with constraints and bounds
 *                 initialized.
 *
 * Returns:
 *   Please note that the approximation type may be MIDDLE, INCOMPLETE or
 *   ABSENT. Unlike in analysis_chull, ABSENT is also used when the hulls do
 *   not respect assumptions.
 */
static void calculateCompleteFactors(glp_prob* const lp, FactorsCHull* factors)
{
	factors->min= calculateFactors(lp, GLP_MIN);
	factors->max= calculateFactors(lp, GLP_MAX);

	if (factors->min && factors->max)
	{
		factors->type= MIDDLE;
		calculateFactorsMiddle(factors);
	}
	else if (factors->min || factors->max)
	{
		factors->type= INCOMPLETE;
		factors->approx= NULL;
	}
	else
	{
		factors->type= ABSENT;
		factors->approx= NULL;
	}
}


/*
 * Create and initialize an array like AnalysisStatsCHull.allFactors
 *
 * Args:
 *   traceNb:      number of traces
 *
 * Returns:
 *   A new array, which can be freed with freeAllFactors()
 */
static FactorsCHull** createAllFactors(const unsigned int traceNb)
{
	FactorsCHull** factorsArray;
	unsigned int i;

	factorsArray= malloc(traceNb * sizeof(FactorsCHull*));
	for (i= 0; i < traceNb; i++)
	{
		factorsArray[i]= calloc((i + 1), sizeof(FactorsCHull));

		factorsArray[i][i].type= EXACT;
		factorsArray[i][i].approx= malloc(sizeof(Factors));
		factorsArray[i][i].approx->drift= 1.;
		factorsArray[i][i].approx->offset= 0.;
	}

	return factorsArray;
}
#endif


/*
 * Compute synchronization factors using a linear programming approach.
 * Compute the factors using analysis_chull. Compare the two.
 *
 * There are two definitions of this function. The empty one is used when the
 * solver library, glpk, is not available at build time. In that case, nothing
 * is actually produced.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
#ifndef HAVE_LIBGLPK
static inline void finalizeAnalysisEvalLP(SyncState* const syncState)
{
}
#else
static void finalizeAnalysisEvalLP(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataEval* analysisData= syncState->analysisData;
	AnalysisDataCHull* chAnalysisData= analysisData->chullSS->analysisData;
	FactorsCHull** lpFactorsArray= createAllFactors(syncState->traceNb);
	FactorsCHull* lpFactors;

	if (!syncState->stats && !syncState->graphsStream)
	{
		return;
	}

	if ((syncState->graphsStream && analysisData->graphs->lps != NULL) ||
		(syncState->stats && analysisData->stats->chFactorsArray != NULL))
	{
		return;
	}

	if (syncState->stats)
	{
		analysisData->stats->chFactorsArray=
			calculateAllFactors(analysisData->chullSS);
		analysisData->stats->lpFactorsArray= lpFactorsArray;
	}

	if (syncState->graphsStream)
	{
		analysisData->graphs->lps= malloc(syncState->traceNb *
			sizeof(glp_prob**));
		for (i= 0; i < syncState->traceNb; i++)
		{
			analysisData->graphs->lps[i]= malloc(i * sizeof(glp_prob*));
		}
		analysisData->graphs->lpFactorsArray= lpFactorsArray;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			glp_prob* lp;

			// Create the LP problem
			lp= lpCreateProblem(chAnalysisData->hullArray[i][j],
				chAnalysisData->hullArray[j][i]);

			// Use the LP problem to find the correction factors for this pair of
			// traces
			calculateCompleteFactors(lp, &lpFactorsArray[i][j]);

			if (syncState->graphsStream)
			{
				analysisData->graphs->lps[i][j]= lp;
			}
			else
			{
				glp_delete_prob(lp);
				destroyFactorsCHull(lpFactors);
			}
		}
	}

	g_array_free(analysisData->chullSS->analysisModule->finalizeAnalysis(analysisData->chullSS),
		TRUE);
}
#endif


/*
 * Compute synchronization accuracy information using a linear programming
 * approach. Write the neccessary data files and plot lines in the gnuplot
 * script.
 *
 * When the solver library, glpk, is not available at build time nothing is
 * actually produced.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisTraceTimeBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
#ifdef HAVE_LIBGLPK
	unsigned int it;
	AnalysisDataEval* analysisData= syncState->analysisData;
	AnalysisGraphsEval* graphs= analysisData->graphs;
	GQueue*** hullArray= ((AnalysisDataCHull*)
		analysisData->chullSS->analysisData)->hullArray;
	FactorsCHull* lpFactors= &graphs->lpFactorsArray[j][i];
	glp_prob* lp= graphs->lps[j][i];

	if (lpFactors->type == MIDDLE)
	{
		int retval;
		char* cwd;
		char fileName[40];
		FILE* fp;
		double* xValues;
		unsigned int xBegin, xEnd;
		double interval;
		const unsigned int graphPointNb= 1000;

		// Open the data file
		snprintf(fileName, 40, "analysis_eval_accuracy-%03u_and_%03u.data", i, j);
		fileName[sizeof(fileName) - 1]= '\0';

		cwd= changeToGraphsDir(syncState->graphsDir);

		if ((fp= fopen(fileName, "w")) == NULL)
		{
			g_error(strerror(errno));
		}
		fprintf(fp, "#%-24s %-25s %-25s %-25s\n", "x", "middle", "min", "max");

		retval= chdir(cwd);
		if (retval == -1)
		{
			g_error(strerror(errno));
		}
		free(cwd);

		// Build the list of absisca values for the points in the accuracy graph
		g_assert_cmpuint(graphPointNb, >=, 4);
		xValues= malloc(graphPointNb * sizeof(double));
		xValues[0]= graphs->bounds[j][i].min;
		xValues[graphPointNb - 1]= graphs->bounds[j][i].max;
		xValues[1]= MIN(((Point*) g_queue_peek_head(hullArray[i][j]))->x,
			((Point*) g_queue_peek_head(hullArray[j][i]))->x);
		xValues[graphPointNb - 2]= MAX(((Point*)
				g_queue_peek_tail(hullArray[i][j]))->x, ((Point*)
				g_queue_peek_tail(hullArray[j][i]))->x);

		if (xValues[0] == xValues[1])
		{
			xBegin= 0;
		}
		else
		{
			xBegin= 1;
		}
		if (xValues[graphPointNb - 2] == xValues[graphPointNb - 1])
		{
			xEnd= graphPointNb - 1;
		}
		else
		{
			xEnd= graphPointNb - 2;
		}
		interval= (xValues[xEnd] - xValues[xBegin]) / (graphPointNb - 1);

		for (it= xBegin; it <= xEnd; it++)
		{
			xValues[it]= xValues[xBegin] + interval * (it - xBegin);
		}

		/* For each absisca value and each optimisation direction, solve the LP
		 * and write a line in the data file */
		for (it= 0; it < graphPointNb; it++)
		{
			unsigned int it2;
			int directions[]= {GLP_MIN, GLP_MAX};

			glp_set_obj_coef(lp, 1, 1.);
			glp_set_obj_coef(lp, 2, xValues[it]);

			fprintf(fp, "%25.9f %25.9f", xValues[it], lpFactors->approx->offset
				+ lpFactors->approx->drift * xValues[it]);
			for (it2= 0; it2 < sizeof(directions) / sizeof(*directions); it2++)
			{
				int status;

				glp_set_obj_dir(lp, directions[it2]);
				retval= glp_simplex(lp, NULL);
				status= glp_get_status(lp);

				g_assert(retval == 0 && status == GLP_OPT);
				fprintf(fp, " %25.9f", glp_get_obj_val(lp));
			}
			fprintf(fp, "\n");
		}

		free(xValues);
		fclose(fp);

		fprintf(syncState->graphsStream,
			"\t\"analysis_eval_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($3 - $2) / clock_freq_%2$u):(($4 - $2) / clock_freq_%2$u) "
				"title \"Synchronization accuracy\" "
				"with filledcurves linewidth 2 linetype 1 "
				"linecolor rgb \"black\" fill solid 0.25 noborder, \\\n", i,
				j);
	}
#endif
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * When the solver library, glpk, is not available at build time nothing is
 * actually produced.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisTraceTimeForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
#ifdef HAVE_LIBGLPK
	if (((AnalysisDataEval*)
			syncState->analysisData)->graphs->lpFactorsArray[j][i].type ==
		MIDDLE)
	{
		fprintf(syncState->graphsStream,
			"\t\"analysis_eval_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($3 - $2) / clock_freq_%2$u) notitle "
				"with lines linewidth 2 linetype 1 "
				"linecolor rgb \"gray60\", \\\n"
			"\t\"analysis_eval_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($4 - $2) / clock_freq_%2$u) notitle "
				"with lines linewidth 2 linetype 1 "
				"linecolor rgb \"gray60\", \\\n", i, j);
	}
#endif
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisTraceTraceBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
#ifdef HAVE_LIBGLPK
	fprintf(syncState->graphsStream,
		"\t\"analysis_eval_accuracy-%1$03u_and_%2$03u.data\" "
		"using 1:3:4 "
		"title \"Synchronization accuracy\" "
		"with filledcurves linewidth 2 linetype 1 "
		"linecolor rgb \"black\" fill solid 0.25 noborder, \\\n", i, j);
#endif
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisTraceTraceForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	AnalysisDataEval* analysisData= syncState->analysisData;

	analysisData->chullSS->analysisModule->graphFunctions.writeTraceTraceForePlots(analysisData->chullSS,
		i, j);
}
