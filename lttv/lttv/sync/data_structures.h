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

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


enum Direction
{
	OUT,
	IN,
};

enum EventType
{
	TCP,
	UDP,
	TYPE_COUNT // This must be the last field
};

// Stage 1 to 2: These structures are passed from processing to matching modules
// TCP events
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
} SegmentKey;

typedef struct
{
	enum Direction direction;
	SegmentKey* segmentKey;
} TCPEvent;

// UDP events
typedef struct
{
	uint32_t saddr, daddr;
	uint16_t source, dest;
	uint16_t ulen;
	uint8_t dataKey[8];
} DatagramKey;

typedef struct
{
	enum Direction direction;
	DatagramKey* datagramKey;
	bool unicast;
} UDPEvent;

typedef struct
{
	uint32_t seconds;
	uint32_t nanosec;
} WallTime;

typedef struct _Event
{
	unsigned long traceNum;
	// wallTime is corrected according to factors in trace struct, cpuTime
	// is not
	uint64_t cpuTime;
	WallTime wallTime;

	// specific event structures and functions could be in separate files and
	// type could be an int
	enum EventType type;
	// event could be a void*, this union is to avoid having to cast
	union {
		TCPEvent* tcpEvent;
		UDPEvent* udpEvent;
	} event;

	void (*copy)(const struct _Event* const event, struct _Event** const newEvent);
	void (*destroy)(struct _Event* const event);
} Event;

// Stage 2 to 3: These structures are passed from matching to analysis modules
typedef struct _Message
{
	Event* inE, * outE;

	void (*print)(const struct _Message* const message);
} Message;

typedef struct
{
	Message* message;
	// Event* acks[]
	GQueue* acks;
} Exchange;

typedef struct
{
	// Event* events[]
	GQueue* events;
} Broadcast;

// Stage 4: These structures contain correction factors
/* Correction factors for each trace, this is the final result of
 * synchronization */
typedef struct
{
	double drift, offset;
} Factors;

// Correction factors between trace pairs, to be used for reduction
typedef enum
{
	EXACT,
	/* Used for identity factors (a0= 0, a1= 1) that map a trace to itself. In
	 * this case, min, max and accuracy are NULL.
	 */

	ACCURATE,
	/* The approximation is the middle of the min and max limits. All fields
	 * are initialized.
	 */

	APPROXIMATE,
	/* min and max are not available. The approximation is a "best effort".
	 * min and max are NULL.
	 */

	INCOMPLETE,
	/* min or max is available but not both. approx and accuracy are not
	 * NULL.
	 */

	ABSENT,
	/* The pair of trace did not have communications in both directions (maybe
	 * even no communication at all). approx and accuracy are NULL.
	 */

	SCREWED,
	/* The algorithms are screwed. All fields may be NULL.
	 */

	APPROX_NB, // This must be the last member
} ApproxType;

extern const char* const approxNames[APPROX_NB];

typedef struct
{
	Factors* min, * max, * approx;
	ApproxType type;
	double accuracy;
} PairFactors;

typedef struct
{
	unsigned int refCount;
	PairFactors** pairFactors;
} AllFactors;


// ConnectionKey-related functions
guint ghfConnectionKeyHash(gconstpointer key);

gboolean gefConnectionKeyEqual(gconstpointer a, gconstpointer b);
bool connectionKeyEqual(const ConnectionKey* const a, const ConnectionKey*
	const b);

void gdnConnectionKeyDestroy(gpointer data);

// SegmentKey-related functions
guint ghfSegmentKeyHash(gconstpointer key);
gboolean gefSegmentKeyEqual(gconstpointer a, gconstpointer b);

// DatagramKey-related functions
guint ghfDatagramKeyHash(gconstpointer key);
gboolean gefDatagramKeyEqual(gconstpointer a, gconstpointer b);
void gdnDestroyDatagramKey(gpointer data);

// Event-related functions
void gdnDestroyEvent(gpointer data);
void copyEvent(const Event* const event, Event** const newEvent);
void copyTCPEvent(const Event* const event, Event** const newEvent);
void copyUDPEvent(const Event* const event, Event** const newEvent);
void destroyEvent(Event* const event);
void destroyTCPEvent(Event* const event);
void destroyUDPEvent(Event* const event);
void gfDestroyEvent(gpointer data, gpointer user_data);
double wallTimeSub(const WallTime const* tA, const WallTime const* tB);
void gfAddEventToArray(gpointer data, gpointer user_data);

// Message-related functions
void printTCPSegment(const Message* const segment);
void convertIP(char* const str, const uint32_t addr);

gint gcfTCPSegmentAckCompare(gconstpointer a, gconstpointer b);
bool isAcking(const Message* const ackSegment, const Message* const ackedSegment);

void gdnTCPSegmentListDestroy(gpointer data);
void gfTCPSegmentDestroy(gpointer data, gpointer user_data);
void destroyTCPSegment(Message* const segment);

// Exchange-related functions
void destroyTCPExchange(Exchange* const exchange);

// Broadcast-related functions
void gdnDestroyBroadcast(gpointer data);
void destroyBroadcast(Broadcast* const broadcast);

// Factor-related functions
void destroyPairFactors(PairFactors* factorsCHull);

AllFactors* createAllFactors(const unsigned int traceNb);
void freeAllFactors(AllFactors* const allFactors, const unsigned int traceNb);

#endif
