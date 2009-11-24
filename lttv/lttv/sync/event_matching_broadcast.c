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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "event_analysis.h"
#include "sync_chain.h"

#include "event_matching_broadcast.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


// Functions common to all matching modules
static void initMatchingBroadcast(SyncState* const syncState);
static void destroyMatchingBroadcast(SyncState* const syncState);

static void matchEventBroadcast(SyncState* const syncState, Event* const event);
static GArray* finalizeMatchingBroadcast(SyncState* const syncState);
static void printMatchingStatsBroadcast(SyncState* const syncState);
static void writeMatchingGraphsPlotsBroadcast(SyncState* const syncState, const
	unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerMatchingBroadcast() __attribute__((constructor (101)));

static void partialDestroyMatchingBroadcast(SyncState* const syncState);

static void openGraphDataFiles(SyncState* const syncState);
static void writeAccuracyPoints(MatchingGraphsBroadcast* graphs, const
	Broadcast* const broadcast);
static void closeGraphDataFiles(SyncState* const syncState);


static MatchingModule matchingModuleBroadcast = {
	.name= "broadcast",
	.canMatch[TCP]= false,
	.canMatch[UDP]= true,
	.initMatching= &initMatchingBroadcast,
	.destroyMatching= &destroyMatchingBroadcast,
	.matchEvent= &matchEventBroadcast,
	.finalizeMatching= &finalizeMatchingBroadcast,
	.printMatchingStats= &printMatchingStatsBroadcast,
	.graphFunctions= {
		.writeTraceTimeForePlots= &writeMatchingGraphsPlotsBroadcast,
	}
};


/*
 * Matching module registering function
 */
static void registerMatchingBroadcast()
{
	g_queue_push_tail(&matchingModules, &matchingModuleBroadcast);
}


/*
 * Matching init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Allocate the matching specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function allocates these matchingData members:
 *                 pendingBroadcasts
 *                 stats
 */
static void initMatchingBroadcast(SyncState* const syncState)
{
	MatchingDataBroadcast* matchingData;

	matchingData= malloc(sizeof(MatchingDataBroadcast));
	syncState->matchingData= matchingData;

	matchingData->pendingBroadcasts= g_hash_table_new_full(&ghfDatagramKeyHash,
		&gefDatagramKeyEqual, &gdnDestroyDatagramKey, &gdnDestroyBroadcast);

	if (syncState->stats)
	{
		matchingData->stats= calloc(1, sizeof(MatchingStatsBroadcast));
	}
	else
	{
		matchingData->stats= NULL;
	}

	if (syncState->graphsStream)
	{
		matchingData->graphs= malloc(sizeof(MatchingGraphsBroadcast));
		openGraphDataFiles(syncState);
	}
	else
	{
		matchingData->graphs= NULL;
	}
}


/*
 * Matching destroy function
 *
 * Free the matching specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function deallocates these matchingData members:
 *                 stats
 */
static void destroyMatchingBroadcast(SyncState* const syncState)
{
	MatchingDataBroadcast* matchingData= syncState->matchingData;
	unsigned int i;

	if (matchingData == NULL)
	{
		return;
	}

	partialDestroyMatchingBroadcast(syncState);

	if (syncState->stats)
	{
		free(matchingData->stats);
	}

	if (syncState->graphsStream)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			free(matchingData->graphs->pointsNb[i]);
		}
		free(matchingData->graphs->pointsNb);
		free(matchingData->graphs);
	}

	free(syncState->matchingData);
	syncState->matchingData= NULL;
}


/*
 * Free some of the matching specific data structures
 *
 * This function can be called right after the events have been processed to
 * free some data structures that are not needed for finalization.
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function deallocates these matchingData members:
 *                 pendingBroadcasts
 */
static void partialDestroyMatchingBroadcast(SyncState* const syncState)
{
	MatchingDataBroadcast* matchingData;

	matchingData= (MatchingDataBroadcast*) syncState->matchingData;

	if (matchingData == NULL || matchingData->pendingBroadcasts == NULL)
	{
		return;
	}

	g_hash_table_destroy(matchingData->pendingBroadcasts);
	matchingData->pendingBroadcasts= NULL;

	if (syncState->graphsStream && matchingData->graphs->accuracyPoints)
	{
		closeGraphDataFiles(syncState);
	}
}


/*
 * Try to match one broadcast with previously received broadcasts (based on
 * the addresses and the fist bytes of data they contain). Deliver them to the
 * analysis module once traceNb events have been accumulated for a broadcast.
 *
 * Args:
 *   syncState     container for synchronization data.
 *   event         new event to match
 */
static void matchEventBroadcast(SyncState* const syncState, Event* const event)
{
	MatchingDataBroadcast* matchingData;

	g_assert(event->type == UDP);

	matchingData= (MatchingDataBroadcast*) syncState->matchingData;

	if (!event->event.udpEvent->unicast)
	{
		if (event->event.udpEvent->direction == IN)
		{
			Broadcast* broadcast;
			DatagramKey* datagramKey;
			gboolean result;

			if (matchingData->stats)
			{
				matchingData->stats->totReceive++;
			}

			/* if event in pendingBroadcasts:
			 *   add it to its broadcast
			 *   if this broadcast has traceNb events:
			 *     remove it from pending and deliver it to analysis
			 *     destroy the broadcast (and its elements)
			 * else:
			 *   create a broadcast and add it to pending
			 */

			result=
				g_hash_table_lookup_extended(matchingData->pendingBroadcasts,
					event->event.udpEvent->datagramKey, (gpointer)
					&datagramKey, (gpointer) &broadcast);
			if (result)
			{
				g_queue_push_tail(broadcast->events, event);
				if (broadcast->events->length == syncState->traceNb)
				{
					if (matchingData->stats)
					{
						matchingData->stats->totComplete++;
					}

					g_hash_table_steal(matchingData->pendingBroadcasts, datagramKey);
					free(datagramKey);
					syncState->analysisModule->analyzeBroadcast(syncState, broadcast);

					if (syncState->graphsStream)
					{
						writeAccuracyPoints(matchingData->graphs, broadcast);
					}
					destroyBroadcast(broadcast);
				}
			}
			else
			{
				broadcast= malloc(sizeof(Broadcast));
				broadcast->events= g_queue_new();
				g_queue_push_tail(broadcast->events, event);

				datagramKey= malloc(sizeof(DatagramKey));
				*datagramKey= *event->event.udpEvent->datagramKey;

				g_hash_table_insert(matchingData->pendingBroadcasts,
					datagramKey, broadcast);
			}
		}
		else
		{
			if (matchingData->stats)
			{
				matchingData->stats->totTransmit++;
			}

			event->destroy(event);
		}
	}
	else
	{
		event->destroy(event);
	}

}


/*
 * Call the partial matching destroyer and Obtain the factors from downstream
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* finalizeMatchingBroadcast(SyncState* const syncState)
{
	MatchingDataBroadcast* matchingData;

	matchingData= (MatchingDataBroadcast*) syncState->matchingData;

	if (matchingData->stats)
	{
		matchingData->stats->totIncomplete=
			g_hash_table_size(matchingData->pendingBroadcasts);
	}

	partialDestroyMatchingBroadcast(syncState);

	return syncState->analysisModule->finalizeAnalysis(syncState);
}


/*
 * Print statistics related to matching. Must be called after
 * finalizeMatching.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printMatchingStatsBroadcast(SyncState* const syncState)
{
	MatchingDataBroadcast* matchingData;

	if (!syncState->stats)
	{
		return;
	}
	matchingData= (MatchingDataBroadcast*) syncState->matchingData;

	printf("Broadcast matching stats:\n");
	printf("\ttotal broadcasts datagrams emitted: %u\n",
		matchingData->stats->totTransmit);
	printf("\ttotal broadcasts datagrams received: %u\n",
		matchingData->stats->totReceive);
	printf("\ttotal broadcast groups for which all receptions were identified: %u\n",
		matchingData->stats->totComplete);
	printf("\ttotal broadcast groups missing some receptions: %u\n",
		matchingData->stats->totIncomplete);
	if (matchingData->stats->totIncomplete > 0)
	{
		printf("\taverage number of broadcast datagrams received in incomplete groups: %f\n",
			(double) (matchingData->stats->totReceive -
				matchingData->stats->totComplete * syncState->traceNb) /
			matchingData->stats->totIncomplete);
	}
}


/*
 * Create and open files used to store accuracy points to genereate graphs.
 * Allocate and populate array to store file pointers and counters.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void openGraphDataFiles(SyncState* const syncState)
{
	unsigned int i, j;
	int retval;
	char* cwd;
	char name[36];
	MatchingGraphsBroadcast* graphs= ((MatchingDataBroadcast*)
		syncState->matchingData)->graphs;

	cwd= changeToGraphDir(syncState->graphsDir);

	graphs->accuracyPoints= malloc(syncState->traceNb * sizeof(FILE**));
	graphs->pointsNb= malloc(syncState->traceNb * sizeof(unsigned int*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		graphs->accuracyPoints[i]= malloc(i * sizeof(FILE*));
		graphs->pointsNb[i]= calloc(i, sizeof(unsigned int));
		for (j= 0; j < i; j++)
		{
			retval= snprintf(name, sizeof(name),
				"matching_broadcast-%03u_and_%03u.data", j, i);
			g_assert_cmpint(retval, <=, sizeof(name) - 1);
			if ((graphs->accuracyPoints[i][j]= fopen(name, "w")) == NULL)
			{
				g_error(strerror(errno));
			}
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
 * Calculate and write points used to generate graphs
 *
 * Args:
 *   graphs:       structure containing array of file pointers and counters
 *   broadcast:    broadcast for which to write the points
 */
static void writeAccuracyPoints(MatchingGraphsBroadcast* graphs, const
	Broadcast* const broadcast)
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
				fprintf(graphs->accuracyPoints[eventJ->traceNum][eventI->traceNum],
					"%20llu %20.9f\n", eventI->cpuTime,
					wallTimeSub(&eventJ->wallTime, &eventI->wallTime));
				graphs->pointsNb[eventJ->traceNum][eventI->traceNum]++;
			}
		}
	}

	g_array_free(events, TRUE);
}


/*
 * Close files used to store accuracy points to genereate graphs. Deallocate
 * array to store file pointers (but not array for counters).
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void closeGraphDataFiles(SyncState* const syncState)
{
	unsigned int i, j;
	MatchingGraphsBroadcast* graphs= ((MatchingDataBroadcast*)
		syncState->matchingData)->graphs;
	int retval;

	if (graphs->accuracyPoints == NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			retval= fclose(graphs->accuracyPoints[i][j]);
			if (retval != 0)
			{
				g_error(strerror(errno));
			}
		}
		free(graphs->accuracyPoints[i]);
	}
	free(graphs->accuracyPoints);

	graphs->accuracyPoints= NULL;
}


/*
 * Write the matching-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingGraphsPlotsBroadcast(SyncState* const syncState, const
	unsigned int i, const unsigned int j)
{
	if (((MatchingDataBroadcast*)
			syncState->matchingData)->graphs->pointsNb[j][i])
	{
		fprintf(syncState->graphsStream,
			"\t\"matching_broadcast-%03d_and_%03d.data\" "
				"title \"Broadcast differential delays\" with points "
				"linecolor rgb \"black\" pointtype 6 pointsize 2, \\\n", i,
				j);
	}
}
