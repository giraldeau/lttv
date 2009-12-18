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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "event_analysis.h"
#include "sync_chain.h"

#include "event_matching_tcp.h"


// Functions common to all matching modules
static void initMatchingTCP(SyncState* const syncState);
static void destroyMatchingTCP(SyncState* const syncState);

static void matchEventTCP(SyncState* const syncState, Event* const event);
static GArray* finalizeMatchingTCP(SyncState* const syncState);
static void printMatchingStatsTCP(SyncState* const syncState);
static void writeMatchingGraphsPlotsTCPMessages(SyncState* const syncState,
	const unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerMatchingTCP() __attribute__((constructor (101)));

static void matchEvents(SyncState* const syncState, Event* const event,
	GHashTable* const unMatchedList, GHashTable* const
	unMatchedOppositeList, const size_t fieldOffset, const size_t
	oppositeFieldOffset);
static void partialDestroyMatchingTCP(SyncState* const syncState);

static bool isAck(const Message* const message);
static bool needsAck(const Message* const message);
static void buildReversedConnectionKey(ConnectionKey* const
	reversedConnectionKey, const ConnectionKey* const connectionKey);

static void openGraphDataFiles(SyncState* const syncState);
static void closeGraphDataFiles(SyncState* const syncState);
static void writeMessagePoint(FILE* stream, const Message* const message);


static MatchingModule matchingModuleTCP = {
	.name= "TCP",
	.canMatch[TCP]= true,
	.canMatch[UDP]= false,
	.initMatching= &initMatchingTCP,
	.destroyMatching= &destroyMatchingTCP,
	.matchEvent= &matchEventTCP,
	.finalizeMatching= &finalizeMatchingTCP,
	.printMatchingStats= &printMatchingStatsTCP,
	.graphFunctions= {
		.writeTraceTraceForePlots= &writeMatchingGraphsPlotsTCPMessages,
	}
};


/*
 * Matching module registering function
 */
static void registerMatchingTCP()
{
	g_queue_push_tail(&matchingModules, &matchingModuleTCP);
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
 *                 unMatchedInE
 *                 unMatchedOutE
 *                 unAcked
 *                 stats
 */
static void initMatchingTCP(SyncState* const syncState)
{
	MatchingDataTCP* matchingData;

	matchingData= malloc(sizeof(MatchingDataTCP));
	syncState->matchingData= matchingData;

	matchingData->unMatchedInE= g_hash_table_new_full(&ghfSegmentKeyHash,
		&gefSegmentKeyEqual, NULL, &gdnDestroyEvent);
	matchingData->unMatchedOutE= g_hash_table_new_full(&ghfSegmentKeyHash,
		&gefSegmentKeyEqual, NULL, &gdnDestroyEvent);
	matchingData->unAcked= g_hash_table_new_full(&ghfConnectionKeyHash,
		&gefConnectionKeyEqual, &gdnConnectionKeyDestroy,
		&gdnTCPSegmentListDestroy);

	if (syncState->stats)
	{
		unsigned int i;

		matchingData->stats= calloc(1, sizeof(MatchingStatsTCP));
		matchingData->stats->totMessageArray= malloc(syncState->traceNb *
			sizeof(unsigned int*));
		for (i= 0; i < syncState->traceNb; i++)
		{
			matchingData->stats->totMessageArray[i]=
				calloc(syncState->traceNb, sizeof(unsigned int));
		}
	}
	else
	{
		matchingData->stats= NULL;
	}

	if (syncState->graphsStream)
	{
		openGraphDataFiles(syncState);
	}
	else
	{
		matchingData->messagePoints= NULL;
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
static void destroyMatchingTCP(SyncState* const syncState)
{
	MatchingDataTCP* matchingData;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	if (matchingData == NULL)
	{
		return;
	}

	partialDestroyMatchingTCP(syncState);

	if (syncState->stats)
	{
		unsigned int i;

		for (i= 0; i < syncState->traceNb; i++)
		{
			free(matchingData->stats->totMessageArray[i]);
		}
		free(matchingData->stats->totMessageArray);
		free(matchingData->stats);
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
 *                 unMatchedInE
 *                 unMatchedOut
 *                 unAcked
 */
static void partialDestroyMatchingTCP(SyncState* const syncState)
{
	MatchingDataTCP* matchingData;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	if (matchingData == NULL || matchingData->unMatchedInE == NULL)
	{
		return;
	}

	g_hash_table_destroy(matchingData->unMatchedInE);
	matchingData->unMatchedInE= NULL;
	g_hash_table_destroy(matchingData->unMatchedOutE);
	g_hash_table_destroy(matchingData->unAcked);

	if (syncState->graphsStream && matchingData->messagePoints)
	{
		closeGraphDataFiles(syncState);
	}
}


/*
 * Try to match one event from a trace with the corresponding event from
 * another trace.
 *
 * Args:
 *   syncState     container for synchronization data.
 *   event         new event to match
 */
static void matchEventTCP(SyncState* const syncState, Event* const event)
{
	MatchingDataTCP* matchingData;

	g_assert(event->type == TCP);

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	if (event->event.tcpEvent->direction == IN)
	{
		matchEvents(syncState, event, matchingData->unMatchedInE,
			matchingData->unMatchedOutE, offsetof(Message, inE),
			offsetof(Message, outE));
	}
	else
	{
		matchEvents(syncState, event, matchingData->unMatchedOutE,
			matchingData->unMatchedInE, offsetof(Message, outE),
			offsetof(Message, inE));
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
static GArray* finalizeMatchingTCP(SyncState* const syncState)
{
	partialDestroyMatchingTCP(syncState);

	return syncState->analysisModule->finalizeAnalysis(syncState);
}


/*
 * Print statistics related to matching. Must be called after
 * finalizeMatching.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printMatchingStatsTCP(SyncState* const syncState)
{
	unsigned int i, j;
	MatchingDataTCP* matchingData;

	if (!syncState->stats)
	{
		return;
	}

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	printf("TCP matching stats:\n");
	printf("\ttotal input and output events matched together to form a packet: %u\n",
		matchingData->stats->totPacket);

	printf("\tMessage traffic:\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			printf("\t\t%3d - %-3d: sent %-10u received %-10u\n", i, j,
				matchingData->stats->totMessageArray[j][i],
				matchingData->stats->totMessageArray[i][j]);
		}
	}

	if (syncState->analysisModule->analyzeExchange != NULL)
	{
		printf("\ttotal packets identified needing an acknowledge: %u\n",
			matchingData->stats->totPacketNeedAck);
		printf("\ttotal exchanges (four events matched together): %u\n",
			matchingData->stats->totExchangeEffective);
		printf("\ttotal synchronization exchanges: %u\n",
			matchingData->stats->totExchangeSync);
	}
}


/*
 * Implementation of a packet matching algorithm for TCP
 *
 * Args:
 *   event:        new event to match
 *   unMatchedList: list of unmatched events of the same type (send or
 *                 receive) as event
 *   unMatchedOppositeList: list of unmatched events of the opposite type of
 *                 event
 *   fieldOffset:  offset of the Event field in the Message struct for the
 *                 field of the type of event
 *   oppositeFieldOffset: offset of the Event field in the Message struct
 *                 for the field of the opposite type of event
 */
static void matchEvents(SyncState* const syncState, Event* const event,
	GHashTable* const unMatchedList, GHashTable* const unMatchedOppositeList,
	const size_t fieldOffset, const size_t oppositeFieldOffset)
{
	Event* companionEvent;
	Message* packet;
	MatchingDataTCP* matchingData;
	GQueue* conUnAcked;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	companionEvent= g_hash_table_lookup(unMatchedOppositeList, event->event.tcpEvent->segmentKey);
	if (companionEvent != NULL)
	{
		g_debug("Found matching companion event, ");

		// If it's there, remove it and create a Message
		g_hash_table_steal(unMatchedOppositeList, event->event.tcpEvent->segmentKey);
		packet= malloc(sizeof(Message));
		*((Event**) ((void*) packet + fieldOffset))= event;
		*((Event**) ((void*) packet + oppositeFieldOffset))= companionEvent;
		packet->print= &printTCPSegment;
		// Both events can now share the same segmentKey
		free(packet->outE->event.tcpEvent->segmentKey);
		packet->outE->event.tcpEvent->segmentKey= packet->inE->event.tcpEvent->segmentKey;

		if (syncState->stats)
		{
			matchingData->stats->totPacket++;
			matchingData->stats->totMessageArray[packet->inE->traceNum][packet->outE->traceNum]++;
		}

		// Discard loopback traffic
		if (packet->inE->traceNum == packet->outE->traceNum)
		{
			destroyTCPSegment(packet);
			return;
		}

		if (syncState->graphsStream)
		{
			writeMessagePoint(matchingData->messagePoints[packet->inE->traceNum][packet->outE->traceNum],
				packet);
		}

		if (syncState->analysisModule->analyzeMessage != NULL)
		{
			syncState->analysisModule->analyzeMessage(syncState, packet);
		}

		// We can skip the rest of the algorithm if the analysis module is not
		// interested in exchanges
		if (syncState->analysisModule->analyzeExchange == NULL)
		{
			destroyTCPSegment(packet);
			return;
		}

		// If this packet acknowleges some data ...
		if (isAck(packet))
		{
			ConnectionKey oppositeConnectionKey;

			buildReversedConnectionKey(&oppositeConnectionKey,
				&event->event.tcpEvent->segmentKey->connectionKey);
			conUnAcked= g_hash_table_lookup(matchingData->unAcked,
				&oppositeConnectionKey);
			if (conUnAcked != NULL)
			{
				Message* ackedPacket;
				GList* result;
				Exchange* exchange;

				exchange= NULL;

				result= g_queue_find_custom(conUnAcked, packet, &gcfTCPSegmentAckCompare);

				while (result != NULL)
				{
					// Remove the acknowledged packet from the unAcked list
					// and keep it for later offset calculations
					g_debug("Found matching unAcked packet, ");

					ackedPacket= (Message*) result->data;
					g_queue_delete_link(conUnAcked, result);

					if (syncState->stats)
					{
						matchingData->stats->totExchangeEffective++;
					}

					if (exchange == NULL)
					{
						exchange= malloc(sizeof(Exchange));
						exchange->message= packet;
						exchange->acks= g_queue_new();
					}

					g_queue_push_tail(exchange->acks, ackedPacket);

					result= g_queue_find_custom(conUnAcked, packet,
						&gcfTCPSegmentAckCompare);
				}

				// It might be possible to do an offset calculation
				if (exchange != NULL)
				{
					ackedPacket= g_queue_peek_tail(exchange->acks);
					if (ackedPacket->outE->traceNum != packet->inE->traceNum
						|| ackedPacket->inE->traceNum !=
						packet->outE->traceNum || packet->inE->traceNum ==
						packet->outE->traceNum)
					{
						ackedPacket->print(ackedPacket);
						packet->print(packet);
						g_error("Disorganized exchange encountered during "
							"synchronization");
					}
					else
					{
						if (syncState->stats)
						{
							matchingData->stats->totExchangeSync++;
						}

						syncState->analysisModule->analyzeExchange(syncState,
							exchange);
					}

					exchange->message= NULL;
					destroyTCPExchange(exchange);
				}
			}
		}

		if (needsAck(packet))
		{
			if (syncState->stats)
			{
				matchingData->stats->totPacketNeedAck++;
			}

			// If this packet will generate an ack, add it to the unAcked list
			g_debug("Adding to unAcked, ");
			conUnAcked= g_hash_table_lookup(matchingData->unAcked,
				&event->event.tcpEvent->segmentKey->connectionKey);
			if (conUnAcked == NULL)
			{
				ConnectionKey* connectionKey;

				connectionKey= malloc(sizeof(ConnectionKey));
				memcpy(connectionKey, &event->event.tcpEvent->segmentKey->connectionKey,
					sizeof(ConnectionKey));
				g_hash_table_insert(matchingData->unAcked, connectionKey,
					conUnAcked= g_queue_new());
			}
			g_queue_push_tail(conUnAcked, packet);
		}
		else
		{
			destroyTCPSegment(packet);
		}
	}
	else
	{
		// If there's no corresponding event, add the event to the unmatched
		// list for this type of event
		g_debug("Adding to unmatched event list, ");
		g_hash_table_replace(unMatchedList, event->event.tcpEvent->segmentKey, event);
	}
}


/*
 * Check if a packet is an acknowledge
 *
 * Args:
 *   packet        TCP Message
 *
 * Returns:
 *   true if it is,
 *   false otherwise
 */
static bool isAck(const Message* const packet)
{
	if (packet->inE->event.tcpEvent->segmentKey->ack == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Check if a packet will increment the sequence number, thus needing an
 * acknowledge
 *
 * Args:
 *   packet        TCP Message
 *
 * Returns:
 *   true if the packet will need an acknowledge
 *   false otherwise
 */
static bool needsAck(const Message* const packet)
{
	if (packet->inE->event.tcpEvent->segmentKey->syn || packet->inE->event.tcpEvent->segmentKey->fin ||
		packet->inE->event.tcpEvent->segmentKey->tot_len - packet->inE->event.tcpEvent->segmentKey->ihl * 4 -
		packet->inE->event.tcpEvent->segmentKey->doff * 4 > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Populate a connection key structure for the opposite direction of a
 * connection
 *
 * Args:
 *   reversedConnectionKey the result, must be pre-allocated
 *   connectionKey the connection key to reverse
 */
static void buildReversedConnectionKey(ConnectionKey* const
	reversedConnectionKey, const ConnectionKey* const connectionKey)
{
	reversedConnectionKey->saddr= connectionKey->daddr;
	reversedConnectionKey->daddr= connectionKey->saddr;
	reversedConnectionKey->source= connectionKey->dest;
	reversedConnectionKey->dest= connectionKey->source;
}


/*
 * Create and open files used to store message points to genereate
 * graphs. Allocate and populate array to store file pointers.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void openGraphDataFiles(SyncState* const syncState)
{
	unsigned int i, j;
	int retval;
	char* cwd;
	char name[29];
	MatchingDataTCP* matchingData;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	cwd= changeToGraphsDir(syncState->graphsDir);

	matchingData->messagePoints= malloc(syncState->traceNb * sizeof(FILE**));
	for (i= 0; i < syncState->traceNb; i++)
	{
		matchingData->messagePoints[i]= malloc(syncState->traceNb *
			sizeof(FILE*));
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= snprintf(name, sizeof(name),
					"matching_tcp-%03u_to_%03u.data", j, i);
				if (retval > sizeof(name) - 1)
				{
					name[sizeof(name) - 1]= '\0';
				}
				if ((matchingData->messagePoints[i][j]= fopen(name, "w")) ==
					NULL)
				{
					g_error(strerror(errno));
				}
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
 * Write a message point to a file used to generate graphs
 *
 * Args:
 *   stream:       FILE*, file pointer where to write the point
 *   message:      message for which to write the point
 */
static void writeMessagePoint(FILE* stream, const Message* const message)
{
	uint64_t x, y;

	if (message->inE->traceNum < message->outE->traceNum)
	{
		// CA is inE->traceNum
		x= message->inE->cpuTime;
		y= message->outE->cpuTime;
	}
	else
	{
		// CA is outE->traceNum
		x= message->outE->cpuTime;
		y= message->inE->cpuTime;
	}

	fprintf(stream, "%20" PRIu64 " %20" PRIu64 "\n", x, y);
}


/*
 * Close files used to store convex hull points to genereate graphs.
 * Deallocate array to store file pointers.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void closeGraphDataFiles(SyncState* const syncState)
{
	unsigned int i, j;
	MatchingDataTCP* matchingData;
	int retval;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	if (matchingData->messagePoints == NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= fclose(matchingData->messagePoints[i][j]);
				if (retval != 0)
				{
					g_error(strerror(errno));
				}
			}
		}
		free(matchingData->messagePoints[i]);
	}
	free(matchingData->messagePoints);

	matchingData->messagePoints= NULL;
}


/*
 * Write the matching-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingGraphsPlotsTCPMessages(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
		"\t\"matching_tcp-%1$03d_to_%2$03d.data\" "
			"title \"Sent messages\" with points linetype 4 "
			"linecolor rgb \"#98fc66\" pointtype 9 pointsize 2, \\\n"
		"\t\"matching_tcp-%2$03d_to_%1$03d.data\" "
			"title \"Received messages\" with points linetype 4 "
			"linecolor rgb \"#6699cc\" pointtype 11 pointsize 2, \\\n", i, j);
}
