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

#ifndef SYNC_CHAIN_H
#define SYNC_CHAIN_H

#include <glib.h>
#include <sys/time.h>

#include "event_processing.h"
#include "event_matching.h"
#include "event_analysis.h"

typedef struct _SyncState
{
	unsigned int traceNb;
	bool stats;
	FILE* graphsStream;
	const char* graphsDir;

	const ProcessingModule* processingModule;
	void* processingData;
	const MatchingModule* matchingModule;
	void* matchingData;
	const AnalysisModule* analysisModule;
	void* analysisData;
} SyncState;

typedef struct
{
	const char shortName;
	const char* longName;
	enum {
		NO_ARG,
		REQUIRED_ARG,
		OPTIONAL_ARG,
		HAS_ARG_COUNT // This must be the last field
	} hasArg;
	bool present;
	// in the case of OPTIONAL_ARG, arg can be initialized to a default value.
	// If an argument is present, arg will be modified
	const char* arg;
	const char* optionHelp;
	const char* argHelp;
} ModuleOption;


extern GQueue processingModules;
extern GQueue matchingModules;
extern GQueue analysisModules;
extern GQueue moduleOptions;

void printStats(SyncState* const syncState);

void timeDiff(struct timeval* const end, const struct timeval* const start);

gint gcfCompareProcessing(gconstpointer a, gconstpointer b);
gint gcfCompareMatching(gconstpointer a, gconstpointer b);
gint gcfCompareAnalysis(gconstpointer a, gconstpointer b);
void gfAppendAnalysisName(gpointer data, gpointer user_data);

#endif
