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
} AnalysisStatsEval;

struct RttKey
{
	uint32_t saddr, daddr;
};

typedef struct
{
	// double* rttInfo[RttKey]
	GHashTable* rttInfo;

	AnalysisStatsEval* stats;
} AnalysisDataEval;

#endif
