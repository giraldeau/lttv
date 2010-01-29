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

#include <math.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sync_chain.h"
#include "event_processing_lttng_common.h"

#include "event_processing_lttng_standard.h"

/* IPv4 Ethertype, taken from <linux/if_ether.h>, unlikely to change as it's
 * defined by IANA: http://www.iana.org/assignments/ethernet-numbers
 */
#define ETH_P_IP    0x0800


// Functions common to all processing modules
static void initProcessingLTTVStandard(SyncState* const syncState, ...);
static void destroyProcessingLTTVStandard(SyncState* const syncState);

static void finalizeProcessingLTTVStandard(SyncState* const syncState);
static void printProcessingStatsLTTVStandard(SyncState* const syncState);
static void writeProcessingGraphVariablesLTTVStandard(SyncState* const
	syncState, const unsigned int i);
static void writeProcessingTraceTraceOptionsLTTVStandard(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeProcessingTraceTimeOptionsLTTVStandard(SyncState* const
	syncState, const unsigned int i, const unsigned int j);

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
	.graphFunctions= {
		.writeVariables= &writeProcessingGraphVariablesLTTVStandard,
		.writeTraceTraceOptions= &writeProcessingTraceTraceOptionsLTTVStandard,
		.writeTraceTimeOptions= &writeProcessingTraceTimeOptionsLTTVStandard,
	},
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
 *   traceSetContext: LttvTracesetContext*, set of LTTV traces
 */
static void initProcessingLTTVStandard(SyncState* const syncState, ...)
{
	unsigned int i;
	ProcessingDataLTTVStandard* processingData;
	va_list ap;

	processingData= malloc(sizeof(ProcessingDataLTTVStandard));
	syncState->processingData= processingData;
	va_start(ap, syncState);
	processingData->traceSetContext= va_arg(ap, LttvTracesetContext*);
	va_end(ap);
	syncState->traceNb=
		lttv_traceset_number(processingData->traceSetContext->ts);
	processingData->hookListList= g_array_sized_new(FALSE, FALSE,
		sizeof(GArray*), syncState->traceNb);

	processingData->traceNumTable= g_hash_table_new(&g_direct_hash, NULL);
	for(i= 0; i < syncState->traceNb; i++)
	{
		g_hash_table_insert(processingData->traceNumTable,
			processingData->traceSetContext->traces[i]->t,
			GUINT_TO_POINTER(i));
	}

	processingData->pendingRecv= malloc(sizeof(GHashTable*) *
		syncState->traceNb);
	for(i= 0; i < syncState->traceNb; i++)
	{
		processingData->pendingRecv[i]= g_hash_table_new_full(&g_direct_hash,
			NULL, NULL, &gdnDestroyEvent);
	}

	if (syncState->stats)
	{
		processingData->stats= calloc(1, sizeof(ProcessingStatsLTTVStandard));
	}
	else
	{
		processingData->stats= NULL;
	}

	if (syncState->graphsStream)
	{
		processingData->graphs= malloc(syncState->traceNb *
			sizeof(ProcessingGraphsLTTVStandard));

		for(i= 0; i < syncState->traceNb; i++)
		{
			LttTrace* traceI= processingData->traceSetContext->traces[i]->t;

			processingData->graphs[i].startFreq= traceI->start_freq;
			processingData->graphs[i].freqScale= traceI->freq_scale;
		}
	}
	else
	{
		processingData->graphs= NULL;
	}

	registerHooks(processingData->hookListList,
		processingData->traceSetContext, &processEventLTTVStandard, syncState,
		syncState->matchingModule->canMatch);
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

	g_debug("traceset start %ld.%09ld end %ld.%09ld",
		processingData->traceSetContext->time_span.start_time.tv_sec,
		processingData->traceSetContext->time_span.start_time.tv_nsec,
		processingData->traceSetContext->time_span.end_time.tv_sec,
		processingData->traceSetContext->time_span.end_time.tv_nsec);
}


/*
 * Print statistics related to processing Must be called after
 * finalizeProcessing.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printProcessingStatsLTTVStandard(SyncState* const syncState)
{
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
	if (syncState->matchingModule->canMatch[TCP])
	{
		printf("\treceived and processed packets that are TCP: %d\n",
			processingData->stats->totRecvTCP);
	}
	if (syncState->matchingModule->canMatch[UDP])
	{
		printf("\treceived and processed packets that are UDP: %d\n",
			processingData->stats->totRecvUDP);
	}
	if (syncState->matchingModule->canMatch[TCP])
	{
		printf("\tsent packets that are TCP: %d\n",
			processingData->stats->totOutE);
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

	if (syncState->graphsStream)
	{
		free(processingData->graphs);
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

		g_debug("Cleaning up pendingRecv list");
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
	LttCycleCount tsc;
	LttTime time;
	WallTime wTime;
	LttTrace* trace;
	unsigned long traceNum;
	struct marker_info* info;
	SyncState* syncState;
	ProcessingDataLTTVStandard* processingData;
	gpointer traceNumP;

	traceHook= (LttvTraceHook*) hookData;
	tfc= (LttvTracefileContext*) callData;
	trace= tfc->t_context->t;
	syncState= (SyncState*) traceHook->hook_data;
	processingData= (ProcessingDataLTTVStandard*) syncState->processingData;
	event= ltt_tracefile_get_event(tfc->tf);
	info= marker_get_info_from_id(tfc->tf->mdata, event->event_id);
	tsc= ltt_event_cycle_count(event);
	time= ltt_event_time(event);
	wTime.seconds= time.tv_sec;
	wTime.nanosec= time.tv_nsec;

	g_assert(g_hash_table_lookup_extended(processingData->traceNumTable,
			trace, NULL, &traceNumP));
	traceNum= GPOINTER_TO_INT(traceNumP);

	g_debug("Process event: time: %ld.%09ld trace: %ld (%p) name: %s ",
		time.tv_sec, time.tv_nsec, traceNum, trace,
		g_quark_to_string(info->name));

	if (info->name == LTT_EVENT_DEV_XMIT_EXTENDED)
	{
		Event* outE;

		if (!ltt_event_get_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 1)) == ETH_P_IP ||
			!ltt_event_get_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 2)) == IPPROTO_TCP)
		{
			return FALSE;
		}

		if (!syncState->matchingModule->canMatch[TCP])
		{
			return FALSE;
		}

		if (syncState->stats)
		{
			processingData->stats->totOutE++;
		}

		outE= malloc(sizeof(Event));
		outE->traceNum= traceNum;
		outE->cpuTime= tsc;
		outE->wallTime= wTime;
		outE->type= TCP;
		outE->copy= &copyTCPEvent;
		outE->destroy= &destroyTCPEvent;
		outE->event.tcpEvent= malloc(sizeof(TCPEvent));
		outE->event.tcpEvent->direction= OUT;
		outE->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
		outE->event.tcpEvent->segmentKey->connectionKey.saddr=
			htonl(ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 3)));
		outE->event.tcpEvent->segmentKey->connectionKey.daddr=
			htonl(ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 4)));
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

		g_debug("Output event done");
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
			inE->cpuTime= tsc;
			inE->wallTime= wTime;
			inE->event.tcpEvent= NULL;
			inE->copy= &copyEvent;
			inE->destroy= &destroyEvent;

			skb= (void*) (long) ltt_event_get_long_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 0));
			g_hash_table_replace(processingData->pendingRecv[traceNum], skb,
				inE);

			g_debug("Adding inE %p for skb %p to pendingRecv", inE, skb);
		}
	}
	else if (info->name == LTT_EVENT_TCPV4_RCV_EXTENDED)
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
			g_warning("No matching pending receive event found");
		}
		else
		{
			if (syncState->stats)
			{
				processingData->stats->totRecvTCP++;
			}

			// If it's there, remove it and proceed with a receive event
			g_hash_table_steal(processingData->pendingRecv[traceNum], skb);

			inE->type= TCP;
			inE->event.tcpEvent= malloc(sizeof(TCPEvent));
			inE->copy= &copyTCPEvent;
			inE->destroy= &destroyTCPEvent;
			inE->event.tcpEvent->direction= IN;
			inE->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
			inE->event.tcpEvent->segmentKey->connectionKey.saddr=
				htonl(ltt_event_get_unsigned(event,
						lttv_trace_get_hook_field(traceHook, 1)));
			inE->event.tcpEvent->segmentKey->connectionKey.daddr=
				htonl(ltt_event_get_unsigned(event,
						lttv_trace_get_hook_field(traceHook, 2)));
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

			g_debug("TCP input event %p for skb %p done", inE, skb);
		}
	}
	else if (info->name == LTT_EVENT_UDPV4_RCV_EXTENDED)
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
			g_warning("No matching pending receive event found");
		}
		else
		{
			guint64 dataStart;

			if (syncState->stats)
			{
				processingData->stats->totRecvUDP++;
			}

			// If it's there, remove it and proceed with a receive event
			g_hash_table_steal(processingData->pendingRecv[traceNum], skb);

			inE->type= UDP;
			inE->event.udpEvent= malloc(sizeof(UDPEvent));
			inE->copy= &copyUDPEvent;
			inE->destroy= &destroyUDPEvent;
			inE->event.udpEvent->direction= IN;
			inE->event.udpEvent->datagramKey= malloc(sizeof(DatagramKey));
			inE->event.udpEvent->datagramKey->saddr=
				htonl(ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 1)));
			inE->event.udpEvent->datagramKey->daddr=
				htonl(ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 2)));
			inE->event.udpEvent->unicast= ltt_event_get_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 3)) == 0 ? false : true;
			inE->event.udpEvent->datagramKey->ulen=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 4));
			inE->event.udpEvent->datagramKey->source=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 5));
			inE->event.udpEvent->datagramKey->dest=
				ltt_event_get_unsigned(event,
					lttv_trace_get_hook_field(traceHook, 6));
			dataStart= ltt_event_get_long_unsigned(event,
				lttv_trace_get_hook_field(traceHook, 7));
			g_assert_cmpuint(sizeof(inE->event.udpEvent->datagramKey->dataKey),
				==, sizeof(guint64));
			if (inE->event.udpEvent->datagramKey->ulen - 8 >=
				sizeof(inE->event.udpEvent->datagramKey->dataKey))
			{
				memcpy(inE->event.udpEvent->datagramKey->dataKey, &dataStart,
					sizeof(inE->event.udpEvent->datagramKey->dataKey));
			}
			else
			{
				memset(inE->event.udpEvent->datagramKey->dataKey, 0,
					sizeof(inE->event.udpEvent->datagramKey->dataKey));
				memcpy(inE->event.udpEvent->datagramKey->dataKey, &dataStart,
						inE->event.udpEvent->datagramKey->ulen - 8);
			}

			syncState->matchingModule->matchEvent(syncState, inE);

			g_debug("UDP input event %p for skb %p done", inE, skb);
		}
	}
	else
	{
		g_assert_not_reached();
	}

	return FALSE;
}


/*
 * Write the processing-specific variables in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            trace number
 */
static void writeProcessingGraphVariablesLTTVStandard(SyncState* const
	syncState, const unsigned int i)
{
	ProcessingDataLTTVStandard* processingData= syncState->processingData;
	ProcessingGraphsLTTVStandard* traceI= &processingData->graphs[i];

	fprintf(syncState->graphsStream, "clock_freq_%u= %.3f\n", i, (double)
		traceI->startFreq / traceI->freqScale);
}


/*
 * Write the processing-specific options in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingTraceTraceOptionsLTTVStandard(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
        "set key inside right bottom\n"
        "set xlabel \"Clock %1$u\"\n"
        "set xtics nomirror\n"
        "set ylabel \"Clock %2$u\"\n"
        "set ytics nomirror\n"
		"set x2label \"Clock %1$d (s)\"\n"
		"set x2range [GPVAL_X_MIN / clock_freq_%1$u : GPVAL_X_MAX / clock_freq_%1$u]\n"
		"set x2tics\n"
		"set y2label \"Clock %2$d (s)\"\n"
		"set y2range [GPVAL_Y_MIN / clock_freq_%2$u : GPVAL_Y_MAX / clock_freq_%2$u]\n"
		"set y2tics\n", i, j);
}


/*
 * Write the processing-specific options in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingTraceTimeOptionsLTTVStandard(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
        "set key inside right bottom\n"
        "set xlabel \"Clock %1$u\"\n"
        "set xtics nomirror\n"
        "set ylabel \"time (s)\"\n"
        "set ytics nomirror\n"
		"set x2label \"Clock %1$d (s)\"\n"
		"set x2range [GPVAL_X_MIN / clock_freq_%1$u : GPVAL_X_MAX / clock_freq_%1$u]\n"
		"set x2tics\n", i);
}
