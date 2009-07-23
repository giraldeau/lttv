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

#include <stdlib.h>
#include <string.h>

#include "event_analysis.h"
#include "sync_chain.h"

#include "event_matching_tcp.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


// Functions common to all matching modules
static void initMatchingTCP(SyncState* const syncState);
static void destroyMatchingTCP(SyncState* const syncState);

static void matchEventTCP(SyncState* const syncState, NetEvent* const event,
	EventType eventType);
static GArray* finalizeMatchingTCP(SyncState* const syncState);
static void printMatchingStatsTCP(SyncState* const syncState);

// Functions specific to this module
static void registerMatchingTCP() __attribute__((constructor (101)));

static void matchEvents(SyncState* const syncState, NetEvent* const event,
	GHashTable* const unMatchedList, GHashTable* const
	unMatchedOppositeList, const size_t fieldOffset, const size_t
	oppositeFieldOffset);
static void partialDestroyMatchingTCP(SyncState* const syncState);

static bool isAck(const Packet* const packet);
static bool needsAck(const Packet* const packet);
static void buildReversedConnectionKey(ConnectionKey* const
	reversedConnectionKey, const ConnectionKey* const connectionKey);


static MatchingModule matchingModuleTCP = {
	.name= "TCP",
	.initMatching= &initMatchingTCP,
	.destroyMatching= &destroyMatchingTCP,
	.matchEvent= &matchEventTCP,
	.finalizeMatching= &finalizeMatchingTCP,
	.printMatchingStats= &printMatchingStatsTCP,
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

	matchingData->unMatchedInE= g_hash_table_new_full(&ghfPacketKeyHash,
		&gefPacketKeyEqual, NULL, &gdnDestroyNetEvent);
	matchingData->unMatchedOutE= g_hash_table_new_full(&ghfPacketKeyHash,
		&gefPacketKeyEqual, NULL, &gdnDestroyNetEvent);
	matchingData->unAcked= g_hash_table_new_full(&ghfConnectionKeyHash,
		&gefConnectionKeyEqual, &gdnConnectionKeyDestroy,
		&gdnPacketListDestroy);

	if (syncState->stats)
	{
		matchingData->stats= calloc(1, sizeof(MatchingStatsTCP));
	}
	else
	{
		matchingData->stats= NULL;
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

	g_debug("Cleaning up unMatchedInE list\n");
	g_hash_table_destroy(matchingData->unMatchedInE);
	matchingData->unMatchedInE= NULL;
	g_debug("Cleaning up unMatchedOutE list\n");
	g_hash_table_destroy(matchingData->unMatchedOutE);
	g_debug("Cleaning up unAcked list\n");
	g_hash_table_destroy(matchingData->unAcked);
}


/*
 * Try to match one event from a trace with the corresponding event from
 * another trace.
 *
 * Args:
 *   syncState     container for synchronization data.
 *   event         new event to match
 *   eventType     type of event to match
 */
static void matchEventTCP(SyncState* const syncState, NetEvent* const event, EventType eventType)
{
	MatchingDataTCP* matchingData;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	if (eventType == IN)
	{
		matchEvents(syncState, event, matchingData->unMatchedInE,
			matchingData->unMatchedOutE, offsetof(Packet, inE),
			offsetof(Packet, outE));
	}
	else
	{
		matchEvents(syncState, event, matchingData->unMatchedOutE,
			matchingData->unMatchedInE, offsetof(Packet, outE),
			offsetof(Packet, inE));
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
 * Print statistics related to matching and downstream modules. Must be
 * called after finalizeMatching.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printMatchingStatsTCP(SyncState* const syncState)
{
	MatchingDataTCP* matchingData;

	if (!syncState->stats)
	{
		return;
	}

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	printf("TCP matching stats:\n");
	printf("\ttotal input and output events matched together to form a packet: %d\n",
		matchingData->stats->totPacket);
	printf("\ttotal packets identified needing an acknowledge: %d\n",
		matchingData->stats->totPacketNeedAck);
	printf("\ttotal exchanges (four events matched together): %d\n",
		matchingData->stats->totExchangeEffective);
	printf("\ttotal synchronization exchanges: %d\n",
		matchingData->stats->totExchangeSync);

	if (syncState->analysisModule->printAnalysisStats != NULL)
	{
		syncState->analysisModule->printAnalysisStats(syncState);
	}
}


/*
 * Implementation of a packet matching algorithm for TCP
 *
 * Args:
 *   netEvent:     new event to match
 *   unMatchedList: list of unmatched events of the same type (send or
 *                 receive) as netEvent
 *   unMatchedOppositeList: list of unmatched events of the opposite type of
 *                 netEvent
 *   fieldOffset:  offset of the NetEvent field in the Packet struct for the
 *                 field of the type of netEvent
 *   oppositeFieldOffset: offset of the NetEvent field in the Packet struct
 *                 for the field of the opposite type of netEvent
 */
static void matchEvents(SyncState* const syncState, NetEvent* const event,
	GHashTable* const unMatchedList, GHashTable* const unMatchedOppositeList,
	const size_t fieldOffset, const size_t oppositeFieldOffset)
{
	NetEvent* companionEvent;
	Packet* packet;
	MatchingDataTCP* matchingData;
	GQueue* conUnAcked;

	matchingData= (MatchingDataTCP*) syncState->matchingData;

	companionEvent= g_hash_table_lookup(unMatchedOppositeList, event->packetKey);
	if (companionEvent != NULL)
	{
		g_debug("Found matching companion event, ");

		if (syncState->stats)
		{
			matchingData->stats->totPacket++;
		}

		// If it's there, remove it and create a Packet
		g_hash_table_steal(unMatchedOppositeList, event->packetKey);
		packet= malloc(sizeof(Packet));
		*((NetEvent**) ((void*) packet + fieldOffset))= event;
		*((NetEvent**) ((void*) packet + oppositeFieldOffset))= companionEvent;
		// Both events can now share the same packetKey
		free(packet->outE->packetKey);
		packet->outE->packetKey= packet->inE->packetKey;
		packet->acks= NULL;

		// Discard loopback traffic
		if (packet->inE->traceNum == packet->outE->traceNum)
		{
			destroyPacket(packet);
			return;
		}

		if (syncState->analysisModule->analyzePacket)
		{
			syncState->analysisModule->analyzePacket(syncState, packet);
		}

		// We can skip the rest of the algorithm if the analysis module is not
		// interested in exchanges
		if (!syncState->analysisModule->analyzeExchange)
		{
			destroyPacket(packet);
			return;
		}

		// If this packet acknowleges some data ...
		if (isAck(packet))
		{
			ConnectionKey oppositeConnectionKey;

			buildReversedConnectionKey(&oppositeConnectionKey,
				&event->packetKey->connectionKey);
			conUnAcked= g_hash_table_lookup(matchingData->unAcked,
				&oppositeConnectionKey);
			if (conUnAcked != NULL)
			{
				Packet* ackedPacket;
				GList* result;

				result= g_queue_find_custom(conUnAcked, packet, &gcfPacketAckCompare);

				while (result != NULL)
				{
					// Remove the acknowledged packet from the unAcked list
					// and keep it for later offset calculations
					g_debug("Found matching unAcked packet, ");

					ackedPacket= (Packet*) result->data;
					g_queue_delete_link(conUnAcked, result);

					if (syncState->stats)
					{
						matchingData->stats->totExchangeEffective++;
					}

					if (packet->acks == NULL)
					{
						packet->acks= g_queue_new();
					}

					g_queue_push_tail(packet->acks, ackedPacket);

					result= g_queue_find_custom(conUnAcked, packet,
						&gcfPacketAckCompare);
				}

				// It might be possible to do an offset calculation
				if (packet->acks != NULL)
				{
					ackedPacket= g_queue_peek_tail(packet->acks);
					if (ackedPacket->outE->traceNum != packet->inE->traceNum
						|| ackedPacket->inE->traceNum !=
						packet->outE->traceNum || packet->inE->traceNum ==
						packet->outE->traceNum)
					{
						printPacket(ackedPacket);
						printPacket(packet);
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
							packet);
					}
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
				&event->packetKey->connectionKey);
			if (conUnAcked == NULL)
			{
				ConnectionKey* connectionKey;

				connectionKey= malloc(sizeof(ConnectionKey));
				memcpy(connectionKey, &event->packetKey->connectionKey,
					sizeof(ConnectionKey));
				g_hash_table_insert(matchingData->unAcked, connectionKey,
					conUnAcked= g_queue_new());
			}
			g_queue_push_tail(conUnAcked, packet);
		}
		else
		{
			destroyPacket(packet);
		}
	}
	else
	{
		// If there's no corresponding event, add the event to the unmatched
		// list for this type of event
		g_debug("Adding to unmatched event list, ");
		g_hash_table_replace(unMatchedList, event->packetKey, event);
	}
}


/*
 * Check if a packet is an acknowledge
 *
 * Returns:
 *   true if it is,
 *   false otherwise
 */
static bool isAck(const Packet* const packet)
{
	if (packet->inE->packetKey->ack == 1)
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
 * Returns:
 *   true if the packet will need an acknowledge
 *   false otherwise
 */
static bool needsAck(const Packet* const packet)
{
	if (packet->inE->packetKey->syn || packet->inE->packetKey->fin ||
		packet->inE->packetKey->tot_len - packet->inE->packetKey->ihl * 4 -
		packet->inE->packetKey->doff * 4 > 0)
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
