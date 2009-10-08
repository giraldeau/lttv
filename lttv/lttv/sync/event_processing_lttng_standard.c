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

#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <linux/if_ether.h>
#include <math.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>

#include "sync_chain_lttv.h"
#include "event_processing_lttng_common.h"

#include "event_processing_lttng_standard.h"


#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif


// Functions common to all processing modules
static void initProcessingLTTVStandard(SyncState* const syncState,
	LttvTracesetContext* const traceSetContext);
static void destroyProcessingLTTVStandard(SyncState* const syncState);

static void finalizeProcessingLTTVStandard(SyncState* const syncState);
static void printProcessingStatsLTTVStandard(SyncState* const syncState);
static void writeProcessingGraphsPlotsLTTVStandard(FILE* stream, SyncState*
	const syncState, const unsigned int i, const unsigned int j);
static void writeProcessingGraphsOptionsLTTVStandard(FILE* stream, SyncState*
	const syncState, const unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerProcessingLTTVStandard() __attribute__((constructor (102)));
static gboolean processEventLTTVStandard(void* hookData, void* callData);
static void partialDestroyProcessingLTTVStandard(SyncState* const syncState);


static ProcessingModule processingModuleLTTVStandard = {
	.name= "LTTV-standard",
	.initProcessing= &initProcessingLTTVStandard,
	.destroyProcessing= &destroyProcessingLTTVStandard,
	.finalizeProcessing= &finalizeProcessingLTTVStandard,
	.printProcessingStats= &printProcessingStatsLTTVStandard,
	.writeProcessingGraphsPlots= &writeProcessingGraphsPlotsLTTVStandard,
	.writeProcessingGraphsOptions= &writeProcessingGraphsOptionsLTTVStandard,
};



/*
 * Processing Module registering function
 */
static void registerProcessingLTTVStandard()
{
	g_queue_push_tail(&processingModules, &processingModuleLTTVStandard);

	createQuarks();
}


/*
 * Allocate and initialize data structures for synchronizing a traceset.
 * Register event hooks.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function allocates these processingData members:
 *                 traceNumTable
 *                 pendingRecv
 *                 hookListList
 *                 stats
 *   traceSetContext: set of LTTV traces
 */
static void initProcessingLTTVStandard(SyncState* const syncState, LttvTracesetContext*
	const traceSetContext)
{
	unsigned int i;
	ProcessingDataLTTVStandard* processingData;

	processingData= malloc(sizeof(ProcessingDataLTTVStandard));
	syncState->processingData= processingData;
	processingData->traceSetContext= traceSetContext;

	if (syncState->stats)
	{
		processingData->stats= calloc(1, sizeof(ProcessingStatsLTTVStandard));
	}
	else
	{
		processingData->stats= NULL;
	}

	processingData->traceNumTable= g_hash_table_new(&g_direct_hash, NULL);
	processingData->hookListList= g_array_sized_new(FALSE, FALSE,
		sizeof(GArray*), syncState->traceNb);
	processingData->pendingRecv= malloc(sizeof(GHashTable*) *
		syncState->traceNb);

	for(i= 0; i < syncState->traceNb; i++)
	{
		g_hash_table_insert(processingData->traceNumTable,
			processingData->traceSetContext->traces[i]->t, (gpointer) i);
	}

	for(i= 0; i < syncState->traceNb; i++)
	{
		processingData->pendingRecv[i]= g_hash_table_new_full(&g_direct_hash,
			NULL, NULL, &gdnDestroyEvent);
	}

	registerHooks(processingData->hookListList, traceSetContext,
		&processEventLTTVStandard, syncState);
}


/*
 * Call the partial processing destroyer, obtain and adjust the factors from
 * downstream
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void finalizeProcessingLTTVStandard(SyncState* const syncState)
{
	unsigned int i;
	GArray* factors;
	double minOffset, minDrift;
	unsigned int refFreqTrace;
	ProcessingDataLTTVStandard* processingData;

	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;

	partialDestroyProcessingLTTVStandard(syncState);

	factors= syncState->matchingModule->finalizeMatching(syncState);

	/* The offsets are adjusted so the lowest one is 0. This is done because
	 * of a Lttv specific limitation: events cannot have negative times. By
	 * having non-negative offsets, events cannot be moved backwards to
	 * negative times.
	 */
	minOffset= 0;
	for (i= 0; i < syncState->traceNb; i++)
	{
		minOffset= MIN(g_array_index(factors, Factors, i).offset, minOffset);
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		g_array_index(factors, Factors, i).offset-= minOffset;
	}

	/* Because the timestamps are corrected at the TSC level (not at the
	 * LttTime level) all trace frequencies must be made equal. We choose to
	 * use the frequency of the system with the lowest drift
	 */
	minDrift= INFINITY;
	refFreqTrace= 0;
	for (i= 0; i < syncState->traceNb; i++)
	{
		if (g_array_index(factors, Factors, i).drift < minDrift)
		{
			minDrift= g_array_index(factors, Factors, i).drift;
			refFreqTrace= i;
		}
	}
	g_assert(syncState->traceNb == 0 || minDrift != INFINITY);

	// Write the factors to the LttTrace structures
	for (i= 0; i < syncState->traceNb; i++)
	{
		LttTrace* t;
		Factors* traceFactors;

		t= processingData->traceSetContext->traces[i]->t;
		traceFactors= &g_array_index(factors, Factors, i);

		t->drift= traceFactors->drift;
		t->offset= traceFactors->offset;
		t->start_freq=
			processingData->traceSetContext->traces[refFreqTrace]->t->start_freq;
		t->freq_scale=
			processingData->traceSetContext->traces[refFreqTrace]->t->freq_scale;
		t->start_time_from_tsc =
			ltt_time_from_uint64(tsc_to_uint64(t->freq_scale, t->start_freq,
					t->drift * t->start_tsc + t->offset));
	}

	g_array_free(factors, TRUE);

	lttv_traceset_context_compute_time_span(processingData->traceSetContext,
		&processingData->traceSetContext->time_span);

	g_debug("traceset start %ld.%09ld end %ld.%09ld\n",
		processingData->traceSetContext->time_span.start_time.tv_sec,
		processingData->traceSetContext->time_span.start_time.tv_nsec,
		processingData->traceSetContext->time_span.end_time.tv_sec,
		processingData->traceSetContext->time_span.end_time.tv_nsec);
}


/*
 * Print statistics related to processing and downstream modules. Must be
 * called after finalizeProcessing.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printProcessingStatsLTTVStandard(SyncState* const syncState)
{
	unsigned int i;
	ProcessingDataLTTVStandard* processingData;

	if (!syncState->stats)
	{
		return;
	}

	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;

	printf("LTTV processing stats:\n");
	printf("\treceived frames: %d\n", processingData->stats->totRecv);
	printf("\treceived frames that are IP: %d\n",
		processingData->stats->totRecvIp);
	printf("\treceived and processed packets that are TCP: %d\n",
		processingData->stats->totInE);
	printf("\tsent packets that are TCP: %d\n",
		processingData->stats->totOutE);

	if (syncState->matchingModule->printMatchingStats != NULL)
	{
		syncState->matchingModule->printMatchingStats(syncState);
	}

	printf("Resulting synchronization factors:\n");
	for (i= 0; i < syncState->traceNb; i++)
	{
		LttTrace* t;

		t= processingData->traceSetContext->traces[i]->t;

		printf("\ttrace %u drift= %g offset= %g (%f) start time= %ld.%09ld\n",
			i, t->drift, t->offset, (double) tsc_to_uint64(t->freq_scale,
				t->start_freq, t->offset) / NANOSECONDS_PER_SECOND,
			t->start_time_from_tsc.tv_sec, t->start_time_from_tsc.tv_nsec);
	}
}


/*
 * Unregister event hooks. Deallocate processingData.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function deallocates these processingData members:
 *                 stats
 */
static void destroyProcessingLTTVStandard(SyncState* const syncState)
{
	ProcessingDataLTTVStandard* processingData;

	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;

	if (processingData == NULL)
	{
		return;
	}

	partialDestroyProcessingLTTVStandard(syncState);

	if (syncState->stats)
	{
		free(processingData->stats);
	}

	free(syncState->processingData);
	syncState->processingData= NULL;
}


/*
 * Unregister event hooks. Deallocate some of processingData.
 *
 * This function can be called right after the events have been processed to
 * free some data structures that are not needed for finalization.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function deallocates these members:
 *                 traceNumTable
 *                 hookListList
 *                 pendingRecv
 */
static void partialDestroyProcessingLTTVStandard(SyncState* const syncState)
{
	unsigned int i;
	ProcessingDataLTTVStandard* processingData;

	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;

	if (processingData == NULL || processingData->traceNumTable == NULL)
	{
		return;
	}

	g_hash_table_destroy(processingData->traceNumTable);
	processingData->traceNumTable= NULL;

	for(i= 0; i < syncState->traceNb; i++)
	{

		g_debug("Cleaning up pendingRecv list\n");
		g_hash_table_destroy(processingData->pendingRecv[i]);
	}
	free(processingData->pendingRecv);

	unregisterHooks(processingData->hookListList,
		processingData->traceSetContext);
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
static gboolean processEventLTTVStandard(void* hookData, void* callData)
{
	LttvTraceHook* traceHook;
	LttvTracefileContext* tfc;
	LttEvent* event;
	LttTime time;
	LttCycleCount tsc;
	LttTrace* trace;
	unsigned long traceNum;
	struct marker_info* info;
	SyncState* syncState;
	ProcessingDataLTTVStandard* processingData;

	traceHook= (LttvTraceHook*) hookData;
	tfc= (LttvTracefileContext*) callData;
	syncState= (SyncState*) traceHook->hook_data;
	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;
	event= ltt_tracefile_get_event(tfc->tf);
	time= ltt_event_time(event);
	tsc= ltt_event_cycle_count(event);
	trace= tfc->t_context->t;
	info= marker_get_info_from_id(tfc->tf->mdata, event->event_id);

	g_assert(g_hash_table_lookup_extended(processingData->traceNumTable,
			trace, NULL, (gpointer*) &traceNum));

	g_debug("XXXX process event: time: %ld.%09ld trace: %ld (%p) name: %s ",
		(long) time.tv_sec, time.tv_nsec, traceNum, trace,
		g_quark_to_string(info->name));

	if (info->name == LTT_EVENT_DEV_XMIT)
	{
		Event* outE;

		if (!ltt_event_get_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 1)) == ETH_P_IP ||
			!ltt_event_get_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 2)) == IPPROTO_TCP)
		{
			return FALSE;
		}

		if (syncState->stats)
		{
			processingData->stats->totOutE++;
		}

		outE= malloc(sizeof(Event));
		outE->traceNum= traceNum;
		outE->time= tsc;
		outE->type= TCP;
		outE->destroy= &destroyTCPEvent;
		outE->event.tcpEvent= malloc(sizeof(TCPEvent));
		outE->event.tcpEvent->direction= OUT;
		outE->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
		outE->event.tcpEvent->segmentKey->connectionKey.saddr=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					3));
		outE->event.tcpEvent->segmentKey->connectionKey.daddr=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					4));
		outE->event.tcpEvent->segmentKey->tot_len=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					5));
		outE->event.tcpEvent->segmentKey->ihl= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 6));
		outE->event.tcpEvent->segmentKey->connectionKey.source=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					7));
		outE->event.tcpEvent->segmentKey->connectionKey.dest=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					8));
		outE->event.tcpEvent->segmentKey->seq= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 9));
		outE->event.tcpEvent->segmentKey->ack_seq=
			ltt_event_get_unsigned(event, lttv_trace_get_hook_field(traceHook,
					10));
		outE->event.tcpEvent->segmentKey->doff= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 11));
		outE->event.tcpEvent->segmentKey->ack= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 12));
		outE->event.tcpEvent->segmentKey->rst= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 13));
		outE->event.tcpEvent->segmentKey->syn= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 14));
		outE->event.tcpEvent->segmentKey->fin= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 15));

		syncState->matchingModule->matchEvent(syncState, outE);

		g_debug("Output event done\n");
	}
	else if (info->name == LTT_EVENT_DEV_RECEIVE)
	{
		guint32 protocol;

		if (syncState->stats)
		{
			processingData->stats->totRecv++;
		}

		protocol= ltt_event_get_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 1));

		if (protocol == ETH_P_IP)
		{
			Event* inE;
			void* skb;

			if (syncState->stats)
			{
				processingData->stats->totRecvIp++;
			}

			inE= malloc(sizeof(Event));
			inE->traceNum= traceNum;
			inE->time= tsc;
			inE->event.tcpEvent= NULL;
			inE->destroy= &destroyEvent;

			skb= (void*) (long) ltt_event_get_long_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 0));
			g_hash_table_replace(processingData->pendingRecv[traceNum], skb,
				inE);

			g_debug("Adding inE %p for skb %p to pendingRecv\n", inE, skb);
		}
		else
		{
			g_debug("\n");
		}
	}
	else if (info->name == LTT_EVENT_TCPV4_RCV)
	{
		Event* inE;
		void* skb;

		// Search pendingRecv for an event with the same skb
		skb= (void*) (long) ltt_event_get_long_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 0));

		inE= (Event*)
			g_hash_table_lookup(processingData->pendingRecv[traceNum], skb);
		if (inE == NULL)
		{
			// This should only happen in case of lost events
			g_debug("No matching pending receive event found\n");
		}
		else
		{
			if (syncState->stats)
			{
				processingData->stats->totInE++;
			}

			// If it's there, remove it and proceed with a receive event
			g_hash_table_steal(processingData->pendingRecv[traceNum], skb);

			inE->type= TCP;
			inE->event.tcpEvent= malloc(sizeof(TCPEvent));
			inE->destroy= &destroyTCPEvent;
			inE->event.tcpEvent->direction= IN;
			inE->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
			inE->event.tcpEvent->segmentKey->connectionKey.saddr=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 1));
			inE->event.tcpEvent->segmentKey->connectionKey.daddr=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 2));
			inE->event.tcpEvent->segmentKey->tot_len=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 3));
			inE->event.tcpEvent->segmentKey->ihl=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 4));
			inE->event.tcpEvent->segmentKey->connectionKey.source=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 5));
			inE->event.tcpEvent->segmentKey->connectionKey.dest=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 6));
			inE->event.tcpEvent->segmentKey->seq=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 7));
			inE->event.tcpEvent->segmentKey->ack_seq=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 8));
			inE->event.tcpEvent->segmentKey->doff=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 9));
			inE->event.tcpEvent->segmentKey->ack=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 10));
			inE->event.tcpEvent->segmentKey->rst=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 11));
			inE->event.tcpEvent->segmentKey->syn=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 12));
			inE->event.tcpEvent->segmentKey->fin=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 13));

			syncState->matchingModule->matchEvent(syncState, inE);

			g_debug("Input event %p for skb %p done\n", inE, skb);
		}
	}
	else if (info->name == LTT_EVENT_NETWORK_IPV4_INTERFACE)
	{
		char* name;
		guint64 address;
		gint64 up;
		char addressString[17];

		address= ltt_event_get_long_unsigned(event,
			lttv_trace_get_hook_field(traceHook, 1));
		up= ltt_event_get_long_int(event, lttv_trace_get_hook_field(traceHook,
				2));
		/* name must be the last field to get or else copy the string, see the
		 * doc for ltt_event_get_string()
		 */
		name= ltt_event_get_string(event, lttv_trace_get_hook_field(traceHook,
				0));

		convertIP(addressString, address);

		g_debug("name \"%s\" address %s up %lld\n", name, addressString, up);
	}
	else
	{
		g_assert_not_reached();
	}

	return FALSE;
}


/*
 * Write the processing-specific graph lines in the gnuplot script (none at
 * the moment). Call the downstream module's graph function.
 *
 * Args:
 *   stream:       stream where to write the data
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingGraphsPlotsLTTVStandard(FILE* stream, SyncState*
	const syncState, const unsigned int i, const unsigned int j)
{
	if (syncState->matchingModule->writeMatchingGraphsPlots != NULL)
	{
		syncState->matchingModule->writeMatchingGraphsPlots(stream, syncState,
			i, j);
	}
}


/*
 * Write the processing-specific options in the gnuplot script. Call the
 * downstream module's options function.
 *
 * Args:
 *   stream:       stream where to write the data
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingGraphsOptionsLTTVStandard(FILE* stream, SyncState*
	const syncState, const unsigned int i, const unsigned int j)
{
	ProcessingDataLTTVStandard* processingData;
	LttTrace* traceI, * traceJ;

	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;

	traceI= processingData->traceSetContext->traces[i]->t;
	traceJ= processingData->traceSetContext->traces[j]->t;

	fprintf(stream,
		"set x2label \"Clock %1$d (s)\"\n"
		"set x2range [GPVAL_X_MIN / %2$.1f : GPVAL_X_MAX / %2$.1f]\n"
		"set x2tics\n"
		"set y2label \"Clock %3$d (s)\"\n"
		"set y2range [GPVAL_Y_MIN / %4$.1f : GPVAL_Y_MAX / %4$.1f]\n"
		"set y2tics\n", i, (double) traceI->start_freq / traceI->freq_scale,
		j, (double) traceJ->start_freq / traceJ->freq_scale);

	if (syncState->matchingModule->writeMatchingGraphsOptions != NULL)
	{
		syncState->matchingModule->writeMatchingGraphsOptions(stream,
			syncState, i, j);
	}
}
