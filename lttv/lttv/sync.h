/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2008 Benjamin Poirier <benjamin.poirier@polymtl.ca>
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

#ifndef SYNC_H
#define SYNC_H

#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#include <lttv/tracecontext.h>
#include <lttv/traceset.h>


#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


struct _Packet;

typedef struct
{
	struct _Packet* packet;
	LttTrace* trace;
	LttCycleCount tsc;
	void* skb;
} NetEvent;

typedef struct
{
	uint32_t saddr, daddr;
	uint16_t source, dest;
} ConnectionKey;

typedef struct _Packet
{
	ConnectionKey connKey;
	unsigned int tot_len, ihl, seq, ack_seq, doff, ack, rst, syn, fin;
	NetEvent* inE, * outE;
	GQueue* acks;
} Packet;

typedef struct
{
	unsigned int n;
	// notation: s__: sum of __; __2: __ squared; example sd2: sum of d squared
	double st, st2, sd, sd2, std, x, d0, e;
} Fit;

typedef struct
{
	double errorSum;
	unsigned int* previousVertex;
	unsigned int reference;
} Graph;

typedef struct
{
	int totRecv, totRecvIp, totInE, totOutE, totPacket, totExchangeEffective,
		totExchangeReal;
	int totPacketNeedAck, totPacketCummAcked;
} Stats;

typedef struct
{
	LttvTracesetContext* tsc;

	unsigned int traceNb;
	// unsigned int traceNumTable[trace*]
	GHashTable* traceNumTable;

	// hookListList conceptually is a two dimensionnal array of LttvTraceHook
	// elements. It uses GArrays to interface with other lttv functions that
	// do.
	GArray* hookListList;

	// inE* pendingRecv[trace]
	GHashTable* pendingRecv;
	// inE* unMatchedInE[packet]
	GHashTable* unMatchedInE;
	// outE* unMatchedOutE[packet]
	GHashTable* unMatchedOutE;
	// packet* unAcked[connKey]
	GHashTable* unAcked;
	Fit** fitArray;

	GQueue* graphList;

	FILE* dataFd;
	Stats* stats;
} SyncState;

typedef struct
{
	LttvTraceHook* traceHook;
	SyncState* syncState;
} HookData;


static void init();
static void destroy();

void sync_traceset(LttvTracesetContext* const tsc);

void registerHooks(SyncState* const syncState);
void unregisterHooks(SyncState* const syncState);
void finalizeLSA(SyncState* const syncState);
void doGraphProcessing(SyncState* const syncState);
void calculateFactors(SyncState* const syncState);

static gboolean process_event_by_id(void* hook_data, void* call_data);

static void matchEvents(NetEvent* const netEvent, GHashTable* const unMatchedList,
	GHashTable* const unMatchedOppositeList, LttEvent* const event,
	LttvTraceHook* const trace_hook, const size_t fieldOffset);

static bool isAck(const Packet* const packet);
static bool isAcking(const Packet* const ackPacket, const Packet* const
	ackedPacket);
static bool needsAck(const Packet* const packet);

static bool connectionKeyEqual(const ConnectionKey* const a, const
	ConnectionKey* const b);

static void netEventListDestroy(gpointer data);
static void netEventRemove(gpointer data, gpointer user_data);

static guint netEventPacketHash(gconstpointer key);
static gboolean netEventPacketEqual(gconstpointer a, gconstpointer b);
static void ghtDestroyNetEvent(gpointer data);

static void packetListDestroy(gpointer data);
static void packetRemove(gpointer data, gpointer user_data);

static void destroyPacket(Packet* const packet);
static void destroyNetEvent(NetEvent* const event);

static void graphRemove(gpointer data, gpointer user_data);

static gint netEventSkbCompare(gconstpointer a, gconstpointer b);
static gint netEventPacketCompare(gconstpointer a, gconstpointer b);
static gint packetAckCompare(gconstpointer a, gconstpointer b);
static gint graphTraceCompare(gconstpointer a, gconstpointer b);

static guint connectionHash(gconstpointer key);
static gboolean connectionEqual(gconstpointer a, gconstpointer b);
static void connectionDestroy(gpointer data);

static void convertIP(char* const str, const uint32_t addr);
static void printPacket(const Packet* const packet);

static void shortestPath(Fit* const* const fitArray, const unsigned int
	traceNum, const unsigned int traceNb, double* const distances, unsigned
	int* const previousVertex);
static double sumDistances(const double* const distances, const unsigned int traceNb);
static void factors(Fit* const* const fitArray, const unsigned int* const
	previousVertex, const unsigned int traceNum, double* const drift, double*
	const offset, double* const stDev);

static void timeDiff(struct timeval* const end, const struct timeval* const start);

#endif // SYNC_H
