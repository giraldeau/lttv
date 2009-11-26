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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#include "sync_chain.h"
#include "event_processing_lttng_common.h"

#include "event_processing_lttng_null.h"


// Functions common to all processing modules
static void initProcessingLTTVNull(SyncState* const syncState, ...);
static void destroyProcessingLTTVNull(SyncState* const syncState);

static void finalizeProcessingLTTVNull(SyncState* const syncState);

// Functions specific to this module
static void registerProcessingLTTVNull() __attribute__((constructor (102)));
static gboolean processEventLTTVNull(void* hookData, void* callData);


static ProcessingModule processingModuleLTTVNull = {
	.name= "LTTV-null",
	.initProcessing= &initProcessingLTTVNull,
	.destroyProcessing= &destroyProcessingLTTVNull,
	.finalizeProcessing= &finalizeProcessingLTTVNull,
};



/*
 * Processing Module registering function
 */
static void registerProcessingLTTVNull()
{
	g_queue_push_tail(&processingModules, &processingModuleLTTVNull);

	createQuarks();
}


/*
 * Allocate and initialize data structures for synchronizing a traceset.
 * Register event hooks.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function allocates these processingData members:
 *                 hookListList
 *   traceSetContext: LttvTracesetContext*, set of LTTV traces
 */
static void initProcessingLTTVNull(SyncState* const syncState, ...)
{
	ProcessingDataLTTVNull* processingData;
	LttvTracesetContext* traceSetContext;
	va_list ap;

	processingData= malloc(sizeof(ProcessingDataLTTVNull));
	syncState->processingData= processingData;
	processingData->traceSetContext= traceSetContext;

	processingData->hookListList= g_array_sized_new(FALSE, FALSE,
		sizeof(GArray*), syncState->traceNb);

	va_start(ap, syncState);
	traceSetContext= va_arg(ap, LttvTracesetContext*);
	va_end(ap);
	registerHooks(processingData->hookListList, traceSetContext,
		&processEventLTTVNull, syncState,
		syncState->matchingModule->canMatch);
}


/*
 * Nothing to do
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void finalizeProcessingLTTVNull(SyncState* const syncState)
{
	return;
}


/*
 * Unregister event hooks. Deallocate processingData.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function deallocates these members:
 *                 hookListList
 */
static void destroyProcessingLTTVNull(SyncState* const syncState)
{
	ProcessingDataLTTVNull* processingData;

	processingData= (ProcessingDataLTTVNull*) syncState->processingData;

	if (processingData == NULL)
	{
		return;
	}

	unregisterHooks(processingData->hookListList,
		processingData->traceSetContext);

	free(syncState->processingData);
	syncState->processingData= NULL;
}


/*
 * Lttv hook function that will be called for network events
 *
 * Args:
 *   hookData:     LttvTraceHook* for the type of event that generated the call
 *   callData:     LttvTracefileContext* at the moment of the event
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks for
 *                 this event
 */
static gboolean processEventLTTVNull(void* hookData, void* callData)
{
	return FALSE;
}
