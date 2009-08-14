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

#include "data_structures_tcp.h"


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
 *   ackPacket     packet that is the confirmation
 *   ackedPacket   packet that contains the original data, both packets have to
 *                 come from the same direction of the same connection
 */
bool isAcking(const Packet* const ackPacket, const Packet* const
	ackedPacket)
{
	if (SEQ_GT(ackPacket->inE->packetKey->ack_seq,
			ackedPacket->inE->packetKey->seq))
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
 *   str:          A preallocated string of length >= 17
 *   addr:         Address
 */
void convertIP(char* const str, const uint32_t addr)
{
	struct in_addr iaddr;

	iaddr.s_addr= htonl(addr);
	strcpy(str, inet_ntoa(iaddr));
}


/*
 * Print the content of a Packet structure
 */
void printPacket(const Packet* const packet)
{
	char saddr[17], daddr[17];
	PacketKey* packetKey;

	packetKey= packet->inE->packetKey;

	convertIP(saddr, packetKey->connectionKey.saddr);
	convertIP(daddr, packetKey->connectionKey.daddr);
	g_debug("%s:%u to %s:%u tot_len: %u ihl: %u seq: %u ack_seq: %u doff: %u "
		"ack: %u rst: %u syn: %u fin: %u", saddr,
		packetKey->connectionKey.source, daddr, packetKey->connectionKey.dest,
		packetKey->tot_len, packetKey->ihl, packetKey->seq,
		packetKey->ack_seq, packetKey->doff, packetKey->ack, packetKey->rst,
		packetKey->syn, packetKey->fin);
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * This function is for indexing netEvents in unMatched lists. All fields of
 * the corresponding packet must match for two keys to be equal.
 *
 * Args:
 *    key        PacketKey*
 */
guint ghfPacketKeyHash(gconstpointer key)
{
	const PacketKey* p;
	uint32_t a, b, c;

	p= (PacketKey*) key;

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

	return c;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * This function is for indexing netEvents in unMatched lists. All fields of
 * the corresponding packet must match for two keys to be equal.
 *
 * Args:
 *   a, b          PacketKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
gboolean gefPacketKeyEqual(gconstpointer a, gconstpointer b)
{
	const PacketKey* pA, * pB;

	pA= ((PacketKey*) a);
	pB= ((PacketKey*) b);

	if (connectionKeyEqual(&pA->connectionKey, &pB->connectionKey) &&
		pA->ihl == pB->ihl &&
		pA->tot_len == pB->tot_len &&
		pA->seq == pB->seq &&
		pA->ack_seq == pB->ack_seq &&
		pA->doff == pB->doff &&
		pA->ack == pB->ack &&
		pA->rst == pB->rst &&
		pA->syn == pB->syn &&
		pA->fin == pB->fin)
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
 *   data:         NetEvent*
 */
void gdnDestroyNetEvent(gpointer data)
{
	destroyNetEvent((NetEvent*) data);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         GQueue* list[Packet]
 */
void gdnPacketListDestroy(gpointer data)
{
	GQueue* list;

	list= (GQueue*) data;

	g_debug("XXXX gdnPacketListDestroy\n");

	g_queue_foreach(list, &gfPacketDestroy, NULL);
	g_queue_free(list);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Packet*, packet to destroy
 *   user_data     NULL
 */
void gfPacketDestroy(gpointer data, gpointer user_data)
{
	g_debug("XXXX gfPacketDestroy\n");
	destroyPacket((Packet*) data);
}


/*
 * Free the memory used by a Packet and the memory of all its associated
 * resources
 */
void destroyPacket(Packet* const packet)
{
	g_debug("XXXX destroyPacket ");
	printPacket(packet);
	g_debug("\n");

	g_assert(packet->inE != NULL && packet->outE != NULL &&
		packet->inE->packetKey == packet->outE->packetKey);

	packet->outE->packetKey= NULL;

	destroyNetEvent(packet->inE);
	destroyNetEvent(packet->outE);

	if (packet->acks != NULL)
	{
		g_queue_foreach(packet->acks, &gfPacketDestroy, NULL);
		g_queue_free(packet->acks);
	}

	free(packet);
}


/*
 * Free the memory used by a NetEvent
 */
void destroyNetEvent(NetEvent* const event)
{
	g_debug("XXXX destroyNetEvent\n");

	if (event->packetKey != NULL)
	{
		free(event->packetKey);
	}
	free(event);
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a          Packet* acked packet
 *   b          Packet* ack packet
 *
 * Returns:
 *   0 if b acks a
 */
gint gcfPacketAckCompare(gconstpointer a, gconstpointer b)
{
	if (isAcking((const Packet*) b, (const Packet*) a))
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
