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

#include <arpa/inet.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "lookup3.h"

#include "data_structures.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif

// TCP sequence numbers use clock arithmetic, these comparison functions take
// that into account
#define SEQ_LT(a,b)     ((int32_t)((a)-(b)) < 0)
#define SEQ_LEQ(a,b)    ((int32_t)((a)-(b)) <= 0)
#define SEQ_GT(a,b)     ((int32_t)((a)-(b)) > 0)
#define SEQ_GEQ(a,b)    ((int32_t)((a)-(b)) >= 0)


/*
 * Compare two ConnectionKey structures
 *
 * Returns:
 *   true if each field of the structure is equal
 *   false otherwise
 */
bool connectionKeyEqual(const ConnectionKey* const a, const
	ConnectionKey* const b)
{
	if (a->saddr == b->saddr && a->daddr == b->daddr && a->source == b->source
		&& a->dest == b->dest)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Check if a packet is an acknowledge of another packet.
 *
 * Args:
 *   ackSegment    packet that is the confirmation
 *   ackedSegment  packet that contains the original data, both packets have to
 *                 come from the same direction of the same connection. Both
 *                 messages have to contain TCP events.
 */
bool isAcking(const Message* const ackSegment, const Message* const
	ackedSegment)
{
	g_assert(ackSegment->inE->type == TCP);
	g_assert(ackSegment->outE->type == TCP);

	if (SEQ_GT(ackSegment->inE->event.tcpEvent->segmentKey->ack_seq,
			ackedSegment->inE->event.tcpEvent->segmentKey->seq))
    {
        return true;
    }
    else
    {
        return false;
    }
}


/*
 * Convert an IP address from 32 bit form to dotted quad
 *
 * Args:
 *   str:          A preallocated string of length >= 16
 *   addr:         Address
 */
void convertIP(char* const str, const uint32_t addr)
{
	strcpy(str, inet_ntoa((struct in_addr) {.s_addr= addr}));
}


/*
 * Print the content of a TCP Message structure
 */
void printTCPSegment(const Message* const segment)
{
	char saddr[16], daddr[16];
	SegmentKey* segmentKey;

	g_assert(segment->inE->type == TCP);
	g_assert(segment->inE->event.tcpEvent->segmentKey ==
		segment->outE->event.tcpEvent->segmentKey);

	segmentKey= segment->inE->event.tcpEvent->segmentKey;

	convertIP(saddr, segmentKey->connectionKey.saddr);
	convertIP(daddr, segmentKey->connectionKey.daddr);
	g_debug("%s:%u to %s:%u tot_len: %u ihl: %u seq: %u ack_seq: %u doff: %u "
		"ack: %u rst: %u syn: %u fin: %u", saddr,
		segmentKey->connectionKey.source, daddr, segmentKey->connectionKey.dest,
		segmentKey->tot_len, segmentKey->ihl, segmentKey->seq,
		segmentKey->ack_seq, segmentKey->doff, segmentKey->ack, segmentKey->rst,
		segmentKey->syn, segmentKey->fin);
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * This function is for indexing TCPEvents in unMatched lists. All fields of
 * the corresponding SegmentKey must match for two keys to be equal.
 *
 * Args:
 *   key           SegmentKey*
 *
 * Returns:
 *   A hash of all fields in the SegmentKey
 */
guint ghfSegmentKeyHash(gconstpointer key)
{
	const SegmentKey* p;
	uint32_t a, b, c;

	p= (SegmentKey*) key;

	a= p->connectionKey.source + (p->connectionKey.dest << 16);
	b= p->connectionKey.saddr;
	c= p->connectionKey.daddr;
	mix(a, b, c);

	a+= p->ihl + (p->tot_len << 8) + (p->doff << 24);
	b+= p->seq;
	c+= p->ack_seq;
	mix(a, b, c);

	a+= p->ack + (p->rst << 8) + (p->syn << 16) + (p->fin << 24);
	final(a, b, c);

	g_debug("segment key hash %p: %u", p, c);

	return c;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * This function is for indexing TCPEvents in unMatched lists. All fields of
 * the corresponding SegmentKey must match for two keys to be equal.
 *
 * Args:
 *   a, b          SegmentKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
gboolean gefSegmentKeyEqual(gconstpointer a, gconstpointer b)
{
	const SegmentKey* sA, * sB;

	sA= (SegmentKey*) a;
	sB= (SegmentKey*) b;

	if (connectionKeyEqual(&sA->connectionKey, &sB->connectionKey) &&
		sA->ihl == sB->ihl &&
		sA->tot_len == sB->tot_len &&
		sA->seq == sB->seq &&
		sA->ack_seq == sB->ack_seq &&
		sA->doff == sB->doff &&
		sA->ack == sB->ack &&
		sA->rst == sB->rst &&
		sA->syn == sB->syn &&
		sA->fin == sB->fin)
	{
		g_debug("segment key equal %p %p: TRUE", sA, sB);
		return TRUE;
	}
	else
	{
		g_debug("segment key equal %p %p: FALSE", sA, sB);
		return FALSE;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         Event*
 */
void gdnDestroyEvent(gpointer data)
{
	Event* event= data;

	event->destroy(event);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         GQueue* list[Packet]
 */
void gdnTCPSegmentListDestroy(gpointer data)
{
	GQueue* list;

	list= (GQueue*) data;

	g_debug("XXXX gdnTCPSegmentListDestroy\n");

	g_queue_foreach(list, &gfTCPSegmentDestroy, NULL);
	g_queue_free(list);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Message*, TCP message to destroy
 *   user_data     NULL
 */
void gfTCPSegmentDestroy(gpointer data, gpointer user_data)
{
	g_debug("XXXX gfTCPSegmentDestroy\n");
	destroyTCPSegment((Message*) data);
}


/*
 * Free the memory used by a TCP Message and the memory of all its associated
 * resources
 *
 * Args:
 *   segment       TCP Message to destroy
 */
void destroyTCPSegment(Message* const segment)
{
	TCPEvent* inE, *outE;

	g_debug("XXXX destroyTCPSegment");
	segment->print(segment);

	g_assert(segment->inE != NULL && segment->outE != NULL);
	g_assert(segment->inE->type == TCP && segment->outE->type == TCP);
	inE= segment->inE->event.tcpEvent;
	outE= segment->outE->event.tcpEvent;
	g_assert(inE->segmentKey == outE->segmentKey);

	outE->segmentKey= NULL;

	destroyTCPEvent(segment->inE);
	destroyTCPEvent(segment->outE);

	free(segment);
}


/*
 * Free the memory used by a TCP Exchange and the memory of SOME of its
 * associated resources. The message is not destroyed. Use destroyTCPSegment()
 * to free it.
 *
 * Args:
 *   exchange      TCP Exchange to destroy. The .message must be NULL
 */
void destroyTCPExchange(Exchange* const exchange)
{
	g_assert(exchange->message == NULL);

	if (exchange->acks != NULL)
	{
		g_queue_foreach(exchange->acks, &gfTCPSegmentDestroy, NULL);
		g_queue_free(exchange->acks);
	}

	free(exchange);
}


/*
 * Free the memory used by a TCP Event and its associated resources
 */
void destroyTCPEvent(Event* const event)
{
	g_assert(event->type == TCP);

	if (event->event.tcpEvent->segmentKey != NULL)
	{
		free(event->event.tcpEvent->segmentKey);
	}
	free(event->event.tcpEvent);
	event->event.tcpEvent= NULL;
	destroyEvent(event);
}


/*
 * Free the memory used by a base Event
 */
void destroyEvent(Event* const event)
{
	g_assert(event->event.tcpEvent == NULL);

	free(event);
}


/*
 * Free the memory used by a UDP Event and its associated resources
 */
void destroyUDPEvent(Event* const event)
{
	g_assert(event->type == UDP);

	if (event->event.udpEvent->datagramKey != NULL)
	{
		free(event->event.udpEvent->datagramKey);
	}
	free(event->event.udpEvent);
	event->event.udpEvent= NULL;
	destroyEvent(event);
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a          Message* acked packet
 *   b          Message* ack packet
 *
 * Returns:
 *   0 if b acks a
 */
gint gcfTCPSegmentAckCompare(gconstpointer a, gconstpointer b)
{
	if (isAcking((const Message*) b, (const Message*) a))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * Hash TCP connection keys. Here are a few possible implementations:
 *
 * 2.4 kernels used tcp_hashfn()
 *
 * I've seen something about an XOR hash:
 * http://tservice.net.ru/~s0mbre/blog/2006/05/14#2006_05_14:
 * unsigned int h = (laddr ^ lport) ^ (faddr ^ fport);
 * h ^= h >> 16;
 * h ^= h >> 8;
 * return h;
 *
 * In 2.6 kernels, inet_ehashfn() handles connection hashing with the help of
 * Jenkins hashing, jhash.h
 *
 * This function uses jenkins hashing. The hash is not the same for packets in
 * opposite directions of the same connection. (Hence the name
 * connection*key*hash)
 *
 * Args:
 *    key        ConnectionKey*
 */
guint ghfConnectionKeyHash(gconstpointer key)
{
	ConnectionKey* connectionKey;
	uint32_t a, b, c;

	connectionKey= (ConnectionKey*) key;

	a= connectionKey->source + (connectionKey->dest << 16);
	b= connectionKey->saddr;
	c= connectionKey->daddr;
	final(a, b, c);

	return c;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * Args:
 *   a, b          ConnectionKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
gboolean gefConnectionKeyEqual(gconstpointer a, gconstpointer b)
{
	// Two packets in the same direction
	if (connectionKeyEqual((const ConnectionKey*) a, (const ConnectionKey*) b))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         ConnectionKey*
 */
void gdnConnectionKeyDestroy(gpointer data)
{
	free((ConnectionKey*) data);
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * Args:
 *    key        DatagramKey*
 */
guint ghfDatagramKeyHash(gconstpointer key)
{
	DatagramKey* datagramKey;
	uint32_t a, b, c;

	datagramKey= (DatagramKey*) key;

	a= datagramKey->saddr;
	b= datagramKey->daddr;
	c= datagramKey->source + (datagramKey->dest << 16);
	mix(a, b, c);

	a+= datagramKey->ulen; // 16 bits left here
	b+= *((uint32_t*) datagramKey->dataKey);
	c+= *((uint32_t*) ((void*) datagramKey->dataKey + 4));
	final(a, b, c);

	return c;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * Args:
 *   a, b          DatagramKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
gboolean gefDatagramKeyEqual(gconstpointer a, gconstpointer b)
{
	const DatagramKey* dA, * dB;

	dA= (DatagramKey*) a;
	dB= (DatagramKey*) b;

	if (dA->saddr == dB->saddr && dA->daddr == dB->daddr &&
		dA->source == dB->source && dA->dest == dB->dest &&
		dA->ulen == dB->ulen &&
		memcmp(dA->dataKey, dB->dataKey, sizeof(dA->dataKey)) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         DatagramKey*
 */
void gdnDestroyDatagramKey(gpointer data)
{
	free((DatagramKey*) data);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         Broadcast*
 */
void gdnDestroyBroadcast(gpointer data)
{
	destroyBroadcast((Broadcast*) data);
}


/*
 * Free a Broadcast struct and its associated ressources
 *
 * Args:
 *   broadcast:    Broadcast*
 */
void destroyBroadcast(Broadcast* const broadcast)
{
	g_queue_foreach(broadcast->events, &gfDestroyEvent, NULL);
	g_queue_free(broadcast->events);
	free(broadcast);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Event*
 *   user_data     NULL
 */
void gfDestroyEvent(gpointer data, gpointer user_data)
{
	Event* event= data;

	event->destroy(event);
}


/* Subtract two WallTime structs
 *
 * Args:
 *   tA, tB:       WallTime
 *
 * Returns:
 *   The result of tA - tB, as a double. This may incur a loss of
 *   precision.
 */
double wallTimeSub(const WallTime const* tA, const WallTime const* tB)
{
	return (double) tA->seconds - tB->seconds + ((double) tA->nanosec - tB->nanosec) / 1e9;
}


/*
 * Allocate and copy a base event
 *
 * Args:
 *   newEvent:     new event, pointer will be updated
 *   event:        event to copy
 */
void copyEvent(const Event* const event, Event** const newEvent)
{
	g_assert(event->event.tcpEvent == NULL);

	*newEvent= malloc(sizeof(Event));
	memcpy(*newEvent, event, sizeof(Event));
}


/*
 * Allocate and copy a TCP event
 *
 * Args:
 *   newEvent:     new event, pointer will be updated
 *   event:        event to copy
 */
void copyTCPEvent(const Event* const event, Event** const newEvent)
{
	g_assert(event->type == TCP);

	*newEvent= malloc(sizeof(Event));
	memcpy(*newEvent, event, sizeof(Event));

	(*newEvent)->event.tcpEvent= malloc(sizeof(TCPEvent));
	memcpy((*newEvent)->event.tcpEvent, event->event.tcpEvent,
		sizeof(TCPEvent));

	(*newEvent)->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
	memcpy((*newEvent)->event.tcpEvent->segmentKey,
		event->event.tcpEvent->segmentKey, sizeof(SegmentKey));
}


/*
 * Allocate and copy a UDP event
 *
 * Args:
 *   newEvent:     new event, pointer will be updated
 *   event:        event to copy
 */
void copyUDPEvent(const Event* const event, Event** const newEvent)
{
	g_assert(event->type == UDP);

	*newEvent= malloc(sizeof(Event));
	memcpy(*newEvent, event, sizeof(Event));

	(*newEvent)->event.udpEvent= malloc(sizeof(UDPEvent));
	memcpy((*newEvent)->event.udpEvent, event->event.udpEvent,
		sizeof(UDPEvent));

	(*newEvent)->event.udpEvent->datagramKey= malloc(sizeof(DatagramKey));
	memcpy((*newEvent)->event.udpEvent->datagramKey,
		event->event.udpEvent->datagramKey, sizeof(DatagramKey));
}
