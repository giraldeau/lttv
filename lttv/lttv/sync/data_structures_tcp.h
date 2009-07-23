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

#ifndef DATA_STRUCTURES_TCP_H
#define DATA_STRUCTURES_TCP_H

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#include <ltt/ltt.h>

typedef struct
{
	uint32_t saddr, daddr;
	uint16_t source, dest;
} ConnectionKey;

typedef struct
{
	ConnectionKey connectionKey;
	uint8_t ihl;
	uint16_t tot_len;
	uint32_t seq, ack_seq;
	uint8_t doff;
	uint8_t ack, rst, syn, fin;
} PacketKey;

typedef struct
{
	// lttng metainformation
	unsigned long traceNum;
	LttCycleCount tsc;

	// kernel metainformation
	void* skb;

	// packet header fields
	PacketKey* packetKey;
} NetEvent;

typedef struct
{
	NetEvent* inE, * outE;
	GQueue* acks;
} Packet;

typedef struct
{
	double drift, offset;
} Factors;

typedef enum
{
	OUT,
	IN
} EventType;


void convertIP(char* const str, const uint32_t addr);
void printPacket(const Packet* const packet);

// ConnectionKey-related functions
bool connectionKeyEqual(const ConnectionKey* const a, const ConnectionKey*
	const b);

// NetEvent-related functions
void destroyNetEvent(NetEvent* const event);

// Packet-related functions
bool isAcking(const Packet* const ackPacket, const Packet* const ackedPacket);
void destroyPacket(Packet* const packet);

// ConnectionKey-related Glib functions
guint ghfConnectionKeyHash(gconstpointer key);
gboolean gefConnectionKeyEqual(gconstpointer a, gconstpointer b);
void gdnConnectionKeyDestroy(gpointer data);

// PacketKey-related Glib functions
guint ghfPacketKeyHash(gconstpointer key);
gboolean gefPacketKeyEqual(gconstpointer a, gconstpointer b);

// NetEvent-related Glib functions
void gdnDestroyNetEvent(gpointer data);

// Packet-related Glib functions
void gdnPacketListDestroy(gpointer data);
void gfPacketDestroy(gpointer data, gpointer user_data);
gint gcfPacketAckCompare(gconstpointer a, gconstpointer b);

#endif
