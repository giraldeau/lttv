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


struct WriteHistogramInfo
{
	GHashTable* rttInfo;
	FILE* graphsStream;
};

// Functions common to all analysis modules
static void initAnalysisEval(SyncState* const syncState);
static void destroyAnalysisEval(SyncState* const syncState);

static void analyzeMessageEval(SyncState* const syncState, Message* const
	message);
static void analyzeExchangeEval(SyncState* const syncState, Exchange* const
	exchange);
static void analyzeBroadcastEval(SyncState* const syncState, Broadcast* const
	broadcast);
static AllFactors* finalizeAnalysisEval(SyncState* const syncState);
static void printAnalysisStatsEval(SyncState* const syncState);
static void writeAnalysisTraceTimeBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTimeForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j);

// Functions specific to this module
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
void registerAnalysisEval()
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
			g_error("%s", strerror(errno));
		}

		readRttInfo(analysisData->rttInfo, rttStream);

		retval= fclose(rttStream);
		if (retval == EOF)
		{
			g_error("%s", strerror(errno));
		}
	}

	if (syncState->stats)
	{
		analysisData->stats= calloc(1, sizeof(AnalysisStatsEval));
		analysisData->stats->broadcastRangeMin= INFINITY;
		analysisData->stats->broadcastRangeMax= -INFINITY;

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
		AnalysisGraphsEval* graphs= malloc(sizeof(AnalysisGraphsEval));
		GList* result;

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

		graphs->chullSS= (SyncState*) malloc(sizeof(SyncState));
		memcpy(graphs->chullSS, syncState, sizeof(SyncState));
		graphs->chullSS->analysisData= NULL;
		result= g_queue_find_custom(&analysisModules, "chull", &gcfCompareAnalysis);
		g_assert(result != NULL);
		graphs->chullSS->analysisModule= (AnalysisModule*) result->data;
		graphs->chullSS->analysisModule->initAnalysis(graphs->chullSS);
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
			g_error("%s", strerror(errno));
		}
	}

	retval= chdir(cwd);
	if (retval == -1)
	{
		g_error("%s", strerror(errno));
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
			g_error("%s", strerror(errno));
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
			g_error("%s", strerror(errno));
		}
		if (fseek(graphsStream, 0, SEEK_END) == -1)
		{
			g_error("%s", strerror(errno));
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
	unsigned int i;
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

		graphs->chullSS->analysisModule->destroyAnalysis(graphs->chullSS);
		free(graphs->chullSS);

		free(graphs);
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
	MessageStats* messageStats = NULL;	/* for gcc */
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

		analysisData->graphs->chullSS->analysisModule->analyzeMessage(analysisData->graphs->chullSS,
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
			analysisData->stats->broadcastStdevSum+= sqrt(y);
		}

		if (syncState->traceNb == 2 && g_queue_get_length(broadcast->events)
			== 2)
		{
			Event* e0, * e1;
			double dd;

			e0= g_queue_peek_head(broadcast->events);
			e1= g_queue_peek_tail(broadcast->events);
			if (e0->traceNum > e1->traceNum)
			{
				Event* tmp;

				tmp= e0;
				e0= e1;
				e1= tmp;
			}

			dd= wallTimeSub(&e1->wallTime, &e0->wallTime);

			analysisData->stats->broadcastPairNb++;
			if (dd < analysisData->stats->broadcastRangeMin)
			{
				analysisData->stats->broadcastRangeMin= dd;
			}
			if (dd > analysisData->stats->broadcastRangeMax)
			{
				analysisData->stats->broadcastRangeMax= dd;
			}

			analysisData->stats->broadcastSum+= dd;
			analysisData->stats->broadcastSumSquares+= pow(dd, 2);
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
 * calculate factors, absent factors are returned. Instead, histograms are
 * written out and histogram structures are freed.
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   AllFactors*   synchronization factors for each trace pair
 */
static AllFactors* finalizeAnalysisEval(SyncState* const syncState)
{
	AnalysisDataEval* analysisData= syncState->analysisData;

	/* This function may be run twice because of matching_distributor. This
	 * check is there to make sure the next block is run only once.
	 */
	if (syncState->graphsStream && analysisData->graphs->histograms)
	{
		g_hash_table_foreach(analysisData->graphs->histograms,
			&ghfWriteHistogram, &(struct WriteHistogramInfo) {.rttInfo=
			analysisData->rttInfo, .graphsStream= syncState->graphsStream});
		g_hash_table_destroy(analysisData->graphs->histograms);
		analysisData->graphs->histograms= NULL;

		if (syncState->graphsStream)
		{
			SyncState* chullSS= analysisData->graphs->chullSS;

			freeAllFactors(chullSS->analysisModule->finalizeAnalysis(chullSS),
				chullSS->traceNb);
		}
	}

	return createAllFactors(syncState->traceNb);
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
		printf("\tBroadcast differential delay:\n");
		printf("\t\tsum of standard deviations: %g\n",
			analysisData->stats->broadcastStdevSum);
		printf("\t\taverage standard deviation: %g\n",
			analysisData->stats->broadcastStdevSum /
			analysisData->stats->broadcastNb);

		if (syncState->traceNb == 2)
		{
			printf("\t\tdifferential delay range: [ %g .. %g ]\n",
				analysisData->stats->broadcastRangeMin,
				analysisData->stats->broadcastRangeMax);
			printf("\t\tdifferential delay average: %g\n",
				analysisData->stats->broadcastSum /
				analysisData->stats->broadcastPairNb);
			printf("\t\tdifferential delay standard deviation: %g\n",
				sqrt(analysisData->stats->broadcastSumSquares /
					analysisData->stats->broadcastPairNb -
					pow(analysisData->stats->broadcastSum /
						analysisData->stats->broadcastPairNb, 2)));
		}
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
				printf("%u (%.2f%%)%n", messageStats->inversionNb, (double)
					messageStats->inversionNb / messageStats->total * 100,
					&charNb);
				printf("%*s", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ");
				printf("%u (%.2f%%)%n", messageStats->tooFastNb, (double)
					messageStats->tooFastNb / messageStats->total * 100,
					&charNb);
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
	printf("%u (%.2f%%)%n", totInversion, (double) totInversion / totTotal *
		100, &charNb);
	printf("%*s", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ");
	printf("%u (%.2f%%)%n", totTooFast, (double) totTooFast / totTotal * 100,
		&charNb);
	printf("%*s%-10u   %u\n", 17 - charNb > 0 ? 17 - charNb + 1: 1, " ",
		totNoInfo, totTotal);

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
			g_error("%s", strerror(errno));
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
			g_error("%s", strerror(errno));
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
					g_error("%s", strerror(errno));
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


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeAnalysisTraceTimeBackPlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	SyncState* chullSS= ((AnalysisDataEval*)
				syncState->analysisData)->graphs->chullSS;
	const GraphFunctions* graphFunctions=
		&chullSS->analysisModule->graphFunctions;

	if (graphFunctions->writeTraceTimeBackPlots != NULL)
	{
			graphFunctions->writeTraceTimeBackPlots(chullSS, i, j);
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
static void writeAnalysisTraceTimeForePlotsEval(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	SyncState* chullSS= ((AnalysisDataEval*)
		syncState->analysisData)->graphs->chullSS;
	const GraphFunctions* graphFunctions=
		&chullSS->analysisModule->graphFunctions;

	if (graphFunctions->writeTraceTimeForePlots != NULL)
	{
		graphFunctions->writeTraceTimeForePlots(chullSS, i, j);
	}
}
