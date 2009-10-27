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

#ifndef SYNC_CHAIN_LTTV_H
#define SYNC_CHAIN_LTTV_H

#include <glib.h>
#include <sys/time.h>

#include "event_processing.h"
#include "event_matching.h"
#include "event_analysis.h"

typedef struct _SyncState
{
	unsigned int traceNb;
	bool stats;
	char* graphs;

	const ProcessingModule* processingModule;
	void* processingData;
	const MatchingModule* matchingModule;
	void* matchingData;
	const AnalysisModule* analysisModule;
	void* analysisData;
} SyncState;

extern GQueue processingModules;
extern GQueue matchingModules;
extern GQueue analysisModules;


void syncTraceset(LttvTracesetContext* const traceSetContext);

char* changeToGraphDir(char* const graphs);
void timeDiff(struct timeval* const end, const struct timeval* const start);

gint gcfCompareProcessing(gconstpointer a, gconstpointer b);
gint gcfCompareMatching(gconstpointer a, gconstpointer b);
gint gcfCompareAnalysis(gconstpointer a, gconstpointer b);

#endif
