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

	MessageStats** messageStats;
	/* double* exchangeRtt[RttKey]
	 * For this table, saddr and daddr are swapped as necessary such that
	 * saddr < daddr */
	GHashTable* exchangeRtt;
} AnalysisStatsEval;

typedef struct
{
	/* FILE* ttPoints[row][col] where
	 *   row= outE->traceNum
	 *   col= inE->traceNum
	 *
	 * This array contains file pointers to files where "trip times" (message
	 * latency) histogram values are outputted. Each trace-pair has two files,
	 * one for each message direction. The elements on the diagonal are not
	 * initialized.
	 */
	FILE*** ttPoints;

	// uint32_t ttBinsArray[row][col][binNum];
	// Row and col have the same structure as ttPoints
	uint32_t*** ttBinsArray;
	// uint32_t ttBinsTotal[row][col];
	// Row and col have the same structure as ttPoints
	uint32_t** ttBinsTotal;

	/* FILE* hrttPoints[traceNum][traceNum] where
	 *   row > col, other elements are not initialized
	 *
	 * This array contains file pointers to files where half round trip times
	 * (evaluated from exchanges) histogram values are outputted.
	 */
	FILE*** hrttPoints;

	// uint32_t hrttBinsArray[row][col][binNum];
	// Row and col have the same structure as hrttPoints
	uint32_t*** hrttBinsArray;
	// uint32_t hrttBinsTotal[row][col];
	// Row and col have the same structure as hrttPoints
	uint32_t** hrttBinsTotal;
} AnalysisGraphsEval;

typedef struct
{
	// double* rttInfo[RttKey]
	GHashTable* rttInfo;

	AnalysisStatsEval* stats;
	AnalysisGraphsEval* graphs;
} AnalysisDataEval;

#endif
