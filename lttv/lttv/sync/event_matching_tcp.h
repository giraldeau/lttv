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

#ifndef EVENT_MATCHING_TCP_H
#define EVENT_MATCHING_TCP_H

#include <glib.h>

#include "data_structures.h"


typedef struct
{
	unsigned int totPacket,
		totPacketNeedAck,
		totExchangeEffective,
		totExchangeSync;
	/* The structure of the array is the same as for hullArray in
	 * analysis_chull, messagePoints[row][col] where:
	 *   row= inE->traceNum
	 *   col= outE->traceNum
	 */
	unsigned int** totMessageArray;
} MatchingStatsTCP;

typedef struct
{
	// NetEvent* unMatchedInE[packetKey]
	GHashTable* unMatchedInE;
	// NetEvent* unMatchedOutE[packetKey]
	GHashTable* unMatchedOutE;
	// Packet* unAcked[connectionKey]
	GHashTable* unAcked;

	MatchingStatsTCP* stats;
	/* This array is used for graphs. It contains file pointers to files where
	 * messages x-y points are outputed. Each trace-pair has two files, one
	 * for each message direction. The structure of the array is the same as
	 * for hullArray in analysis_chull, messagePoints[row][col] where:
	 *   row= inE->traceNum
	 *   col= outE->traceNum
	 *
	 * The elements on the diagonal are not initialized.
	 */
	FILE*** messagePoints;
} MatchingDataTCP;

#endif
