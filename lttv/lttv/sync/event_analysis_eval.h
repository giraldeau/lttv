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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#ifdef HAVE_LIBGLPK
#include <glpk.h>
#endif

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

#ifdef HAVE_LIBGLPK
	/* FactorsCHull** chFactorsArray[traceNum][traceNum]
	 * FactorsCHull** lpFactorsArray[traceNum][traceNum]
	 *
	 * As usual, only the lower triangular part of theses matrixes is
	 * allocated */
	FactorsCHull** chFactorsArray;
	FactorsCHull** lpFactorsArray;
#endif
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
	  * values are output. Each host-pair has two files, one for each message
	  * direction. As for traces, the host with the smallest address is
	  * considered to be the reference for the direction of messages (ie.
	  * messages from the host with the lowest address to the host with the
	  * largest address are "sent"). */
	FILE* ttSendPoints;
	FILE* ttRecvPoints;

	struct Bins ttSendBins;
	struct Bins ttRecvBins;

	/* File pointers to files where half round trip times (evaluated from
	 * exchanges) histogram values are output. */
	FILE* hrttPoints;

	struct Bins hrttBins;
} AnalysisHistogramEval;

typedef struct
{
	// These are the cpu times of the first and last interactions (message or
	// broadcast) between two traces. The times are from the trace with the
	// lowest traceNum.
	uint64_t min, max;
} Bounds;

typedef struct
{
	/* AnalysisHistogramEval* graphs[RttKey];
	 * For this table, saddr and daddr are swapped as necessary such that
	 * saddr < daddr */
	GHashTable* histograms;

	/* Bounds bounds[traceNum][traceNum]
	 *
	 * Only the lower triangular part of the matrix is allocated, that is
	 * bounds[i][j] where i > j */
	Bounds** bounds;

#ifdef HAVE_LIBGLPK
	/* glp_prob* lps[traceNum][traceNum]
	 *
	 * Only the lower triangular part of the matrix is allocated, that is
	 * lps[i][j] where i > j */
	glp_prob*** lps;

	/* Factors lpFactors[traceNum][traceNum]
	 *
	 * Only the lower triangular part of the matrix is allocated, that is
	 * lpFactorsArray[i][j] where i > j */
	FactorsCHull** lpFactorsArray;
#endif
} AnalysisGraphsEval;

typedef struct
{
	// double* rttInfo[RttKey]
	GHashTable* rttInfo;

	/* The convex hull analysis is encapsulated and messages are passed to it
	 * so that it builds the convex hulls. These are reused in the linear
	 * program. */
	struct _SyncState* chullSS;

	AnalysisStatsEval* stats;
	AnalysisGraphsEval* graphs;
} AnalysisDataEval;

void registerAnalysisEval();

#endif
