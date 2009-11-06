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

#ifndef EVENT_ANALYSIS_EVAL_H
#define EVENT_ANALYSIS_EVAL_H

#include <glib.h>

#include "data_structures.h"


struct RttKey
{
	uint32_t saddr, daddr;
};

typedef struct
{
	unsigned int inversionNb,
		tooFastNb,
		noRTTInfoNb,
		total;
} MessageStats;

typedef struct
{
	double broadcastDiffSum;
	unsigned int broadcastNb;

	// MessageStats messageStats[traceNb][traceNb]
	MessageStats** messageStats;

	/* double* exchangeRtt[RttKey]
	 * For this table, saddr and daddr are swapped as necessary such that
	 * saddr < daddr */
	GHashTable* exchangeRtt;
} AnalysisStatsEval;


#define BIN_NB 1001
struct Bins
{
	// index of min and max bins that are != 0
	uint32_t min, max;
	// sum of all bins
	uint32_t total;
	/* bin[0]: underflow ]-INFINITY..0[
	 * bin[1]: [0..1e-6[
	 * rest defined exponentially, see binStart()
	 * bin[BIN_NB - 1]: overflow [1..INFINITY[ */
	uint32_t bin[BIN_NB];
};


typedef struct
{
	 /* File pointers to files where "trip times" (message latency) histogram
	  * values are outputted. Each host-pair has two files, one for each
	  * message direction. As for traces, the host with the smallest address
	  * is considered to be the reference for the direction of messages (ie.
	  * messages from the host with the lowest address to the host with the
	  * largest address are "sent"). */
	FILE* ttSendPoints;
	FILE* ttRecvPoints;

	struct Bins ttSendBins;
	struct Bins ttRecvBins;

	/* File pointers to files where half round trip times (evaluated from
	 * exchanges) histogram values are outputted. */
	FILE* hrttPoints;

	struct Bins hrttBins;
} AnalysisGraphEval;

typedef struct
{
	// double* rttInfo[RttKey]
	GHashTable* rttInfo;

	AnalysisStatsEval* stats;
	/* AnalysisGraphsEval* graphs[RttKey];
	 * For this table, saddr and daddr are swapped as necessary such that
	 * saddr < daddr */
	GHashTable* graphs;
} AnalysisDataEval;

#endif
