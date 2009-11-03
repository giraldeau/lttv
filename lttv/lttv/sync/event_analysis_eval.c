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


static AnalysisModule analysisModuleEval= {
	.name= "eval",
	.initAnalysis= &initAnalysisEval,
	.destroyAnalysis= &destroyAnalysisEval,
	.analyzeMessage= &analyzeMessageEval,
	.analyzeExchange= &analyzeExchangeEval,
	.analyzeBroadcast= &analyzeBroadcastEval,
	.finalizeAnalysis= &finalizeAnalysisEval,
	.printAnalysisStats= &printAnalysisStatsEval,
	.writeAnalysisGraphsPlots= NULL,
	.writeAnalysisGraphsOptions= NULL,
};

static ModuleOption optionEvalRttFile= {
	.longName= "eval-rtt-file",
	.hasArg= REQUIRED_ARG,
	{.arg= NULL},
	.optionHelp= "specify the file containing rtt information",
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
