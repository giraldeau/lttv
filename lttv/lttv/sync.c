/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2008 Benjamin Poirier <benjamin.poirier@polymtl.ca>
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

// for INFINITY in math.h
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arpa/inet.h>
#include <fcntl.h>
#include <glib.h>
#include <limits.h>
#include <linux/if_ether.h>
#include <math.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <lttv/module.h>
#include <lttv/option.h>

#include "lookup3.h"

#include "sync.h"


GQuark
	LTT_CHANNEL_NET,
	LTT_CHANNEL_NETIF_STATE;

GQuark
	LTT_EVENT_DEV_HARD_START_XMIT_TCP,
	LTT_EVENT_DEV_RECEIVE,
	LTT_EVENT_PKFREE_SKB,
	LTT_EVENT_TCPV4_RCV,
	LTT_EVENT_NETWORK_IPV4_INTERFACE;

GQuark
	LTT_FIELD_SKB,
	LTT_FIELD_PROTOCOL,
	LTT_FIELD_SADDR,
	LTT_FIELD_DADDR,
	LTT_FIELD_TOT_LEN,
	LTT_FIELD_IHL,
	LTT_FIELD_SOURCE,
	LTT_FIELD_DEST,
	LTT_FIELD_SEQ,
	LTT_FIELD_ACK_SEQ,
	LTT_FIELD_DOFF,
	LTT_FIELD_ACK,
	LTT_FIELD_RST,
	LTT_FIELD_SYN,
	LTT_FIELD_FIN,
	LTT_FIELD_NAME,
    LTT_FIELD_ADDRESS,
    LTT_FIELD_UP;

static gboolean optionSync;
static gboolean optionSyncStats;
static char* optionSyncData;

/*
 * Module init function
 */
static void init()
{
	g_debug("\t\t\tXXXX sync init\n");

	LTT_CHANNEL_NET= g_quark_from_string("net");
	LTT_CHANNEL_NETIF_STATE= g_quark_from_string("netif_state");

	LTT_EVENT_DEV_HARD_START_XMIT_TCP=
		g_quark_from_string("dev_hard_start_xmit_tcp");
	LTT_EVENT_DEV_RECEIVE= g_quark_from_string("dev_receive");
	LTT_EVENT_PKFREE_SKB= g_quark_from_string("pkfree_skb");
	LTT_EVENT_TCPV4_RCV= g_quark_from_string("tcpv4_rcv");
	LTT_EVENT_NETWORK_IPV4_INTERFACE=
		g_quark_from_string("network_ipv4_interface");

	LTT_FIELD_SKB= g_quark_from_string("skb");
	LTT_FIELD_PROTOCOL= g_quark_from_string("protocol");
	LTT_FIELD_SADDR= g_quark_from_string("saddr");
	LTT_FIELD_DADDR= g_quark_from_string("daddr");
	LTT_FIELD_TOT_LEN= g_quark_from_string("tot_len");
	LTT_FIELD_IHL= g_quark_from_string("ihl");
	LTT_FIELD_SOURCE= g_quark_from_string("source");
	LTT_FIELD_DEST= g_quark_from_string("dest");
	LTT_FIELD_SEQ= g_quark_from_string("seq");
	LTT_FIELD_ACK_SEQ= g_quark_from_string("ack_seq");
	LTT_FIELD_DOFF= g_quark_from_string("doff");
	LTT_FIELD_ACK= g_quark_from_string("ack");
	LTT_FIELD_RST= g_quark_from_string("rst");
	LTT_FIELD_SYN= g_quark_from_string("syn");
	LTT_FIELD_FIN= g_quark_from_string("fin");
	LTT_FIELD_NAME= g_quark_from_string("name");
	LTT_FIELD_ADDRESS= g_quark_from_string("address");
	LTT_FIELD_UP= g_quark_from_string("up");

	optionSync= FALSE;
	lttv_option_add("sync", '\0', "synchronize the time between tracefiles "
		"based on network communications", "none", LTTV_OPT_NONE, &optionSync,
		NULL, NULL);

	optionSyncStats= FALSE;
	lttv_option_add("sync-stats", '\0', "print statistics about the time "
		"synchronization", "none", LTTV_OPT_NONE, &optionSyncStats, NULL, NULL);

	optionSyncData= NULL;
	lttv_option_add("sync-data", '\0', "save information about every offset "
		"identified", "pathname of the file where to save the offsets",
		LTTV_OPT_STRING, &optionSyncData, NULL, NULL);
}


/*
 * Module unload function
 */
static void destroy()
{
	g_debug("\t\t\tXXXX sync destroy\n");

	lttv_option_remove("sync");
	lttv_option_remove("sync-stats");
	lttv_option_remove("sync-data");
}


/*
 * Calculate a traceset's drift and offset values based on network events
 *
 * Args:
 *   tsc:          traceset
 */
void sync_traceset(LttvTracesetContext* const tsc)
{
	SyncState* syncState;
	struct timeval startTime, endTime;
	struct rusage startUsage, endUsage;
	int retval;

	if (optionSync == FALSE)
	{
		g_debug("Not synchronizing traceset because option is disabled");
		return;
	}

	if (optionSyncStats)
	{
		gettimeofday(&startTime, 0);
		getrusage(RUSAGE_SELF, &startUsage);
	}

	// Initialize data structures
	syncState= malloc(sizeof(SyncState));
	syncState->tsc= tsc;
	syncState->traceNb= lttv_traceset_number(tsc->ts);

	if (optionSyncStats)
	{
		syncState->stats= calloc(1, sizeof(Stats));
	}

	if (optionSyncData)
	{
		syncState->dataFd= fopen(optionSyncData, "w");
		if (syncState->dataFd == NULL)
		{
			perror(0);
			goto out_free;
		}

		fprintf(syncState->dataFd, "%10s %10s %21s %21s %21s\n", "ni", "nj",
			"timoy", "dji", "eji");
	}

	// Process traceset
	registerHooks(syncState);

	lttv_process_traceset_seek_time(tsc, ltt_time_zero);
	lttv_process_traceset_middle(tsc, ltt_time_infinite, G_MAXULONG, NULL);
	lttv_process_traceset_seek_time(tsc, ltt_time_zero);

	unregisterHooks(syncState);

	// Finalize the least-squares analysis
	finalizeLSA(syncState);

	// Find a reference node and structure nodes in a graph
	doGraphProcessing(syncState);

	// Calculate the resulting offset and drift between each trace and its
	// reference and write those values to the LttTrace structures
	calculateFactors(syncState);

	if (optionSyncData)
	{
		retval= fclose(syncState->dataFd);
		if (retval != 0)
		{
			perror(0);
		}
	}

out_free:
	if (optionSyncStats)
	{
		printf("Stats:\n");
		// Received frames
		printf("\ttotal received packets: %d\n", syncState->stats->totRecv);
		// Received frames that are ip
		printf("\ttotal received IP packets: %d\n",
			syncState->stats->totRecvIp);
		// Processed packets that are tcp
		printf("\ttotal input events: %d\n", syncState->stats->totInE);
		// Sent packets that are tcp
		printf("\ttotal output events: %d\n", syncState->stats->totOutE);
		// Input and output events were matched together
		printf("\ttotal packets identified: %d\n",
			syncState->stats->totPacket);
		printf("\ttotal packets identified needing an acknowledge: %d\n",
			syncState->stats->totPacketNeedAck);
		// Four events are matched
		printf("\ttotal packets fully acknowledged: %d\n",
			syncState->stats->totExchangeEffective);
		// Many packets were acknowledged at once
		printf("\ttotal packets cummulatively acknowledged (excluding the "
			"first in each series): %d\n",
			syncState->stats->totPacketCummAcked);
		// Offset calculations that could be done, some effective exchanges are
		// not used when there is cummulative acknowledge
		printf("\ttotal exchanges identified: %d\n",
			syncState->stats->totExchangeReal);

		free(syncState->stats);
	}
	free(syncState);

	if (optionSyncStats)
	{
		gettimeofday(&endTime, 0);
		retval= getrusage(RUSAGE_SELF, &endUsage);

		timeDiff(&endTime, &startTime);
		timeDiff(&endUsage.ru_utime, &startUsage.ru_utime);
		timeDiff(&endUsage.ru_stime, &startUsage.ru_stime);

		printf("Synchronization time:\n");
		printf("\treal time: %ld.%06ld\n", endTime.tv_sec, endTime.tv_usec);
		printf("\tuser time: %ld.%06ld\n", endUsage.ru_utime.tv_sec, endUsage.ru_utime.tv_usec);
		printf("\tsystem time: %ld.%06ld\n", endUsage.ru_stime.tv_sec, endUsage.ru_stime.tv_usec);
	}
}


/*
 * Allocate and initialize data structures for synchronizing a traceset.
 * Register event hooks.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function allocates theses members:
 *                 traceNumTable
 *                 hookListList
 *                 pendingRecv
 *                 unMatchedInE
 *                 unMatchedOutE
 *                 unAcked
 *                 fitArray
 */
void registerHooks(SyncState* const syncState)
{
	unsigned int i, j, k;

	// Allocate lists
	syncState->pendingRecv= g_hash_table_new_full(NULL, NULL, NULL,
		&netEventListDestroy);
	syncState->unMatchedInE= g_hash_table_new_full(&netEventPacketHash,
		&netEventPacketEqual, NULL, &ghtDestroyNetEvent);
	syncState->unMatchedOutE= g_hash_table_new_full(&netEventPacketHash,
		&netEventPacketEqual, NULL, &ghtDestroyNetEvent);
	syncState->unAcked= g_hash_table_new_full(&connectionHash,
		&connectionEqual, &connectionDestroy, &packetListDestroy);

	syncState->fitArray= malloc(syncState->traceNb * sizeof(Fit*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		syncState->fitArray[i]= calloc(syncState->traceNb, sizeof(Fit));
	}

	syncState->traceNumTable= g_hash_table_new(&g_direct_hash,
		&g_direct_equal);

	syncState->hookListList= g_array_sized_new(FALSE, FALSE, sizeof(GArray*),
		syncState->traceNb);

	// Add event hooks and initialize traceNumTable
	// note: possibilité de remettre le code avec lttv_trace_find_marker_ids (voir r328)
	for(i= 0; i < syncState->traceNb; i++)
	{
		guint old_len;
		LttvTraceContext* tc;
		GArray* hookList;
		const int hookNb= 5;
		int retval;

		hookList= g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), hookNb);
		g_array_append_val(syncState->hookListList, hookList);

		tc= syncState->tsc->traces[i];

		g_hash_table_insert(syncState->traceNumTable, tc->t, (gpointer) i);

		// Find the hooks
		old_len= hookList->len;
		retval= lttv_trace_find_hook(tc->t, LTT_CHANNEL_NET,
			LTT_EVENT_DEV_HARD_START_XMIT_TCP,
			FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_SADDR,
				LTT_FIELD_DADDR, LTT_FIELD_TOT_LEN,
				LTT_FIELD_IHL, LTT_FIELD_SOURCE,
				LTT_FIELD_DEST, LTT_FIELD_SEQ,
				LTT_FIELD_ACK_SEQ, LTT_FIELD_DOFF,
				LTT_FIELD_ACK, LTT_FIELD_RST, LTT_FIELD_SYN,
				LTT_FIELD_FIN), process_event_by_id,
			syncState, &hookList);
		if (retval != 0)
		{
			g_warning("Trace %d contains no %s.%s marker\n", i,
				g_quark_to_string(LTT_CHANNEL_NET),
				g_quark_to_string(LTT_EVENT_DEV_HARD_START_XMIT_TCP));
		}
		else
		{
			g_assert(hookList->len - old_len == 1);
		}

		old_len= hookList->len;
		retval= lttv_trace_find_hook(tc->t, LTT_CHANNEL_NET,
			LTT_EVENT_DEV_RECEIVE, FIELD_ARRAY(LTT_FIELD_SKB,
				LTT_FIELD_PROTOCOL), process_event_by_id, syncState, &hookList);
		if (retval != 0)
		{
			g_warning("Trace %d contains no %s.%s marker\n", i,
				g_quark_to_string(LTT_CHANNEL_NET),
				g_quark_to_string(LTT_EVENT_DEV_RECEIVE));
		}
		else
		{
			g_assert(hookList->len - old_len == 1);
		}

		old_len= hookList->len;
		retval= lttv_trace_find_hook(tc->t, LTT_CHANNEL_NET,
			LTT_EVENT_PKFREE_SKB, FIELD_ARRAY(LTT_FIELD_SKB),
			process_event_by_id, syncState, &hookList);
		if (retval != 0)
		{
			g_warning("Trace %d contains no %s.%s marker\n", i,
				g_quark_to_string(LTT_CHANNEL_NET),
				g_quark_to_string(LTT_EVENT_PKFREE_SKB));
		}
		else
		{
			g_assert(hookList->len - old_len == 1);
		}

		old_len= hookList->len;
		retval= lttv_trace_find_hook(tc->t, LTT_CHANNEL_NET, LTT_EVENT_TCPV4_RCV,
			FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_SADDR, LTT_FIELD_DADDR,
				LTT_FIELD_TOT_LEN, LTT_FIELD_IHL, LTT_FIELD_SOURCE,
				LTT_FIELD_DEST, LTT_FIELD_SEQ, LTT_FIELD_ACK_SEQ,
				LTT_FIELD_DOFF, LTT_FIELD_ACK, LTT_FIELD_RST, LTT_FIELD_SYN,
				LTT_FIELD_FIN), process_event_by_id, syncState, &hookList);
		if (retval != 0)
		{
			g_warning("Trace %d contains no %s.%s marker\n", i,
				g_quark_to_string(LTT_CHANNEL_NET),
				g_quark_to_string(LTT_EVENT_TCPV4_RCV));
		}
		else
		{
			g_assert(hookList->len - old_len == 1);
		}

		old_len= hookList->len;
		retval= lttv_trace_find_hook(tc->t, LTT_CHANNEL_NETIF_STATE,
			LTT_EVENT_NETWORK_IPV4_INTERFACE, FIELD_ARRAY(LTT_FIELD_NAME,
				LTT_FIELD_ADDRESS, LTT_FIELD_UP), process_event_by_id, syncState,
			&hookList);
		if (retval != 0)
		{
			g_warning("Trace %d contains no %s.%s marker\n", i,
				g_quark_to_string(LTT_CHANNEL_NETIF_STATE),
				g_quark_to_string(LTT_EVENT_NETWORK_IPV4_INTERFACE));
		}
		else
		{
			g_assert(hookList->len - old_len == 1);
		}

		// Add the hooks to each tracefile's event_by_id hook list
		for(j= 0; j < tc->tracefiles->len; j++)
		{
			LttvTracefileContext* tfc;

			tfc= g_array_index(tc->tracefiles, LttvTracefileContext*, j);

			for(k= 0; k < hookList->len; k++)
			{
				LttvTraceHook* trace_hook;

				trace_hook= &g_array_index(hookList, LttvTraceHook, k);
				if (trace_hook->hook_data != syncState)
				{
					g_assert_not_reached();
				}
				if (trace_hook->mdata == tfc->tf->mdata)
				{
					lttv_hooks_add(lttv_hooks_by_id_find(tfc->event_by_id,
							trace_hook->id),
						trace_hook->h, trace_hook,
						LTTV_PRIO_DEFAULT);
				}
			}
		}
	}
}


/*
 * Unregister event hooks. Deallocate some data structures that are not needed
 * anymore after running the hooks.
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function deallocates theses members:
 *                 hookListList
 *                 pendingRecv
 *                 unMatchedInE
 *                 unMatchedOutE
 *                 unAcked
 */
void unregisterHooks(SyncState* const syncState)
{
	unsigned int i, j, k;

	// Remove event hooks
	for(i= 0; i < syncState->traceNb; i++)
	{
		LttvTraceContext* tc;
		GArray* hookList;

		tc= syncState->tsc->traces[i];
		hookList= g_array_index(syncState->hookListList, GArray*, i);

		// Remove the hooks from each tracefile's event_by_id hook list
		for(j= 0; j < tc->tracefiles->len; j++)
		{
			LttvTracefileContext* tfc;

			tfc= g_array_index(tc->tracefiles, LttvTracefileContext*, j);

			for(k= 0; k < hookList->len; k++)
			{
				LttvTraceHook* trace_hook;

				trace_hook= &g_array_index(hookList, LttvTraceHook, k);
				if (trace_hook->mdata == tfc->tf->mdata)
				{
					lttv_hooks_remove_data(lttv_hooks_by_id_find(tfc->event_by_id,
							trace_hook->id), trace_hook->h, trace_hook);
				}
			}
		}

		g_array_free(hookList, TRUE);
	}

	g_array_free(syncState->hookListList, TRUE);

	// Free lists
	g_debug("Cleaning up pendingRecv list\n");
	g_hash_table_destroy(syncState->pendingRecv);
	g_debug("Cleaning up unMatchedInE list\n");
	g_hash_table_destroy(syncState->unMatchedInE);
	g_debug("Cleaning up unMatchedOutE list\n");
	g_hash_table_destroy(syncState->unMatchedOutE);
	g_debug("Cleaning up unAcked list\n");
	g_hash_table_destroy(syncState->unAcked);
}


/*
 * Finalize the least-squares analysis. The intermediate values in the fit
 * array are used to calculate the drift and the offset between each pair of
 * nodes based on their exchanges.
 *
 * Args:
 *   syncState:    container for synchronization data.
 */
void finalizeLSA(SyncState* const syncState)
{
	unsigned int i, j;

	if (optionSyncStats)
	{
		printf("Individual synchronization factors:\n");
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				Fit* fit;
				double delta;

				fit= &syncState->fitArray[i][j];

				delta= fit->n * fit->st2 - pow(fit->st, 2);
				fit->x= (fit->n * fit->std - fit->st * fit->sd) / delta;
				fit->d0= (fit->st2 * fit->sd - fit->st * fit->std) / delta;
				fit->e= sqrt((fit->sd2 - (fit->n * pow(fit->std, 2) +
							pow(fit->sd, 2) * fit->st2 - 2 * fit->st * fit->sd
							* fit->std) / delta) / (fit->n - 2));

				g_debug("[i= %u j= %u]\n", i, j);
				g_debug("n= %d st= %g st2= %g sd= %g sd2= %g std= %g\n",
					fit->n, fit->st, fit->st2, fit->sd, fit->sd2, fit->std);
				g_debug("xij= %g d0ij= %g e= %g\n", fit->x, fit->d0, fit->e);
				g_debug("(xji= %g d0ji= %g)\n", -fit->x / (1 + fit->x),
					-fit->d0 / (1 + fit->x));
			}
		}
	}

	if (optionSyncStats)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= 0; j < syncState->traceNb; j++)
			{
				if (i < j)
				{
					Fit* fit;

					fit= &syncState->fitArray[i][j];
					printf("\tbetween trace i= %u and j= %u, xij= %g d0ij= %g "
						"e= %g\n", i, j, fit->x, fit->d0, fit->e);

					fit= &syncState->fitArray[j][i];
					printf("\tbetween trace i= %u and j= %u, xij= %g d0ij= %g "
						"e= %g\n", j, i, fit->x, fit->d0, fit->e);
				}
			}
		}
	}
}


/*
 * Structure nodes in graphs of nodes that had exchanges. Each graph has a
 * reference node, the one that can reach the others with the smallest
 * cummulative error.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function allocates these members:
 *                 graphList
 */
void doGraphProcessing(SyncState* const syncState)
{
	unsigned int i, j;
	double* distances;
	unsigned int* previousVertex;

	distances= malloc(syncState->traceNb * sizeof(double));
	previousVertex= malloc(syncState->traceNb * sizeof(unsigned int));
	syncState->graphList= g_queue_new();

	for (i= 0; i < syncState->traceNb; i++)
	{
		double errorSum;
		GList* result;

		// Perform shortest path search
		g_debug("shortest path trace %d\ndistances: ", i);
		shortestPath(syncState->fitArray, i, syncState->traceNb, distances,
			previousVertex);

		for (j= 0; j < syncState->traceNb; j++)
		{
			g_debug("%g, ", distances[j]);
		}
		g_debug("\npreviousVertex: ");
		for (j= 0; j < syncState->traceNb; j++)
		{
			g_debug("%u, ", previousVertex[j]);
		}
		g_debug("\n");

		// Group in graphs nodes that have exchanges
		errorSum= sumDistances(distances, syncState->traceNb);
		result= g_queue_find_custom(syncState->graphList, &i,
			&graphTraceCompare);
		if (result != NULL)
		{
			Graph* graph;

			g_debug("found graph\n");
			graph= (Graph*) result->data;
			if (errorSum < graph->errorSum)
			{
				g_debug("adding to graph\n");
				graph->errorSum= errorSum;
				free(graph->previousVertex);
				graph->previousVertex= previousVertex;
				graph->reference= i;
				previousVertex= malloc(syncState->traceNb * sizeof(unsigned
						int));
			}
		}
		else
		{
			Graph* newGraph;

			g_debug("creating new graph\n");
			newGraph= malloc(sizeof(Graph));
			newGraph->errorSum= errorSum;
			newGraph->previousVertex= previousVertex;
			newGraph->reference= i;
			previousVertex= malloc(syncState->traceNb * sizeof(unsigned int));

			g_queue_push_tail(syncState->graphList, newGraph);
		}
	}

	free(previousVertex);
	free(distances);
}


/*
 * Calculate the resulting offset and drift between each trace and its
 * reference and write those values to the LttTrace structures. Also free
 * structures that are not needed anymore.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *                 This function deallocates:
 *                 traceNumTable
 *                 fitArray
 *                 graphList
 */
void calculateFactors(SyncState* const syncState)
{
	unsigned int i, j;
	double minOffset;

	// Calculate the resulting offset and drift between each trace and its
	// reference
	g_info("Factors:\n");
	if (optionSyncStats)
	{
		printf("Resulting synchronization factors:\n");
	}

	minOffset= 0;
	for (i= 0; i < syncState->traceNb; i++)
	{
		GList* result;

		result= g_queue_find_custom(syncState->graphList, &i,
			&graphTraceCompare);
		if (result != NULL)
		{
			Graph* graph;
			double drift, offset, stDev;
			LttTrace* t;

			t= syncState->tsc->traces[i]->t;
			graph= (Graph*) result->data;

			g_info("trace %u (%p) graph: reference %u\n", i, t, graph->reference);

			for (j= 0; j < syncState->traceNb; j++)
			{
				g_info("%u, ", graph->previousVertex[j]);
			}
			g_info("\n");

			factors(syncState->fitArray, graph->previousVertex, i, &drift,
				&offset, &stDev);
			t->drift= drift;
			t->offset= offset;
			t->start_time_from_tsc =
				ltt_time_from_uint64(tsc_to_uint64(t->freq_scale,
						t->start_freq, drift * t->start_tsc + offset));

			if (optionSyncStats)
			{
				if (i == graph->reference)
				{
					printf("\ttrace %u reference %u previous vertex - "
						"stdev= %g\n", i,
						graph->reference, stDev);
				}
				else
				{
					printf("\ttrace %u reference %u previous vertex %u "
						"stdev= %g\n", i,
						graph->reference,
						graph->previousVertex[i],
						stDev);
				}
			}

			if (offset < minOffset)
			{
				minOffset= offset;
			}
		}
		else
		{
			fprintf(stderr, "trace: %d\n", i);
			g_assert_not_reached();
		}
	}

	// Adjust all offsets so the lowest one is 0 (no negative offsets)
	for (i= 0; i < syncState->traceNb; i++)
	{
		LttTrace* t;

		t= syncState->tsc->traces[i]->t;
		t->offset-= minOffset;
		t->start_time_from_tsc =
			ltt_time_from_uint64(tsc_to_uint64(t->freq_scale,
					t->start_freq, t->drift * t->start_tsc
					+ t->offset));

		g_info("trace %u drift: %f offset: %g start_time: %ld.%09ld\n",
			i, t->drift, t->offset, t->start_time_from_tsc.tv_sec,
			t->start_time_from_tsc.tv_nsec);

		if (optionSyncStats)
		{
			printf("\ttrace %u drift= %g offset= %g (%f)\n", i,
				t->drift, t->offset,
				tsc_to_uint64(t->freq_scale, t->start_freq,
					t->offset) / 1e9);
		}
	}

	lttv_traceset_context_compute_time_span(syncState->tsc,
		&syncState->tsc->time_span);

	g_debug("traceset start %ld.%09ld end %ld.%09ld\n",
		syncState->tsc->time_span.start_time.tv_sec,
		syncState->tsc->time_span.start_time.tv_nsec,
		syncState->tsc->time_span.end_time.tv_sec,
		syncState->tsc->time_span.end_time.tv_nsec);

	g_queue_foreach(syncState->graphList, &graphRemove, NULL);
	g_queue_free(syncState->graphList);

	for (i= 0; i < syncState->traceNb; i++)
	{
		free(syncState->fitArray[i]);
	}
	free(syncState->fitArray);

	g_hash_table_destroy(syncState->traceNumTable);
}


/*
 * Lttv hook function that will be called for network events
 *
 * Args:
 *   hook_data:    LttvTraceHook* for the type of event that generated the call
 *   call_data:    LttvTracefileContext* at the moment of the event
 *
 * Returns:
 *   FALSE         Always returns FALSE, meaning to keep processing hooks for
 *                 this event
 */
static gboolean process_event_by_id(void* hook_data, void* call_data)
{
	LttvTraceHook* trace_hook;
	LttvTracefileContext* tfc;
	LttEvent* event;
	LttTime time;
	LttCycleCount tsc;
	LttTrace* trace;
	struct marker_info* info;
	SyncState* syncState;

	trace_hook= (LttvTraceHook*) hook_data;
	tfc= (LttvTracefileContext*) call_data;
	syncState= (SyncState*) trace_hook->hook_data;
	event= ltt_tracefile_get_event(tfc->tf);
	time= ltt_event_time(event);
	tsc= ltt_event_cycle_count(event);
	trace= tfc->t_context->t;
	info= marker_get_info_from_id(tfc->tf->mdata, event->event_id);

	g_debug("XXXX process event: time: %ld.%09ld trace: %p name: %s ",
		(long) time.tv_sec, time.tv_nsec, trace,
		g_quark_to_string(info->name));

	if (info->name == LTT_EVENT_DEV_HARD_START_XMIT_TCP)
	{
		NetEvent* outE;

		if (optionSyncStats)
		{
			syncState->stats->totOutE++;
		}

		outE= malloc(sizeof(NetEvent));

		outE->packet= NULL;
		outE->trace= trace;
		outE->tsc= tsc;

		matchEvents(outE, syncState->unMatchedOutE, syncState->unMatchedInE,
			event, trace_hook, offsetof(Packet, outE));

		g_debug("Output event done\n");
	}
	else if (info->name == LTT_EVENT_DEV_RECEIVE)
	{
		NetEvent* inE;
		guint64 protocol;

		if (optionSyncStats)
		{
			syncState->stats->totRecv++;
		}

		protocol= ltt_event_get_long_unsigned(event,
			lttv_trace_get_hook_field(trace_hook, 1));

		if (protocol == ETH_P_IP)
		{
			GQueue* list;

			if (optionSyncStats)
			{
				syncState->stats->totRecvIp++;
			}

			inE= malloc(sizeof(NetEvent));

			inE->packet= NULL;
			inE->trace= trace;
			inE->tsc= tsc;
			inE->skb= (void*) (unsigned long)
				ltt_event_get_long_unsigned(event,
					lttv_trace_get_hook_field(trace_hook, 0));

			list= g_hash_table_lookup(syncState->pendingRecv, trace);
			if (unlikely(list == NULL))
			{
				g_hash_table_insert(syncState->pendingRecv, trace, list=
					g_queue_new());
			}
			g_queue_push_head(list, inE);

			g_debug("Adding inE to pendingRecv\n");
		}
		else
		{
			g_debug("\n");
		}
	}
	else if (info->name == LTT_EVENT_TCPV4_RCV)
	{
		GQueue* prList;

		// Search pendingRecv for an event with the same skb
		prList= g_hash_table_lookup(syncState->pendingRecv, trace);
		if (unlikely(prList == NULL))
		{
			g_debug("No pending receive event list for this trace\n");
		}
		else
		{
			NetEvent tempInE;
			GList* result;

			tempInE.skb= (void*) (unsigned long)
				ltt_event_get_long_unsigned(event,
					lttv_trace_get_hook_field(trace_hook, 0));
			result= g_queue_find_custom(prList, &tempInE, &netEventSkbCompare);

			if (result == NULL)
			{
				g_debug("No matching pending receive event found\n");
			}
			else
			{
				NetEvent* inE;

				if (optionSyncStats)
				{
					syncState->stats->totInE++;
				}

				// If it's there, remove it and proceed with a receive event
				inE= (NetEvent*) result->data;
				g_queue_delete_link(prList, result);

				matchEvents(inE, syncState->unMatchedInE,
					syncState->unMatchedOutE, event, trace_hook,
					offsetof(Packet, inE));

				g_debug("Input event done\n");
			}
		}
	}
	else if (info->name == LTT_EVENT_PKFREE_SKB)
	{
		GQueue* list;

		list= g_hash_table_lookup(syncState->pendingRecv, trace);
		if (unlikely(list == NULL))
		{
			g_debug("No pending receive event list for this trace\n");
		}
		else
		{
			NetEvent tempInE;
			GList* result;

			tempInE.skb= (void*) (unsigned long)
				ltt_event_get_long_unsigned(event,
					lttv_trace_get_hook_field(trace_hook, 0));
			result= g_queue_find_custom(list, &tempInE, &netEventSkbCompare);

			if (result == NULL)
			{
				g_debug("No matching pending receive event found, \"shaddow"
					"skb\"\n");
			}
			else
			{
				NetEvent* inE;

				inE= (NetEvent*) result->data;
				g_queue_delete_link(list, result);
				destroyNetEvent(inE);

				g_debug("Non-TCP skb\n");
			}
		}
	}
	else if (info->name == LTT_EVENT_NETWORK_IPV4_INTERFACE)
	{
		char* name;
		guint64 address;
		gint64 up;
		char addressString[17];

		name= ltt_event_get_string(event, lttv_trace_get_hook_field(trace_hook,
				0));
		address= ltt_event_get_long_unsigned(event,
			lttv_trace_get_hook_field(trace_hook, 1));
		up= ltt_event_get_long_int(event, lttv_trace_get_hook_field(trace_hook,
				2));

		convertIP(addressString, address);

		g_debug("name \"%s\" address %s up %lld\n", name, addressString, up);
	}
	else
	{
		g_debug("<default>\n");
	}

	return FALSE;
}


/*
 * Implementation of a packet matching algorithm for TCP
 *
 * Args:
 *   netEvent      new event to match
 *   unMatchedList list of unmatched events of the same type (send or receive)
 *                 as netEvent
 *   unMatchedOppositeList list of unmatched events of the opposite type of
 *                 netEvent
 *   event         event corresponding to netEvent
 *   trace_hook    trace_hook corresponding to netEvent
 *   fieldOffset   offset of the NetEvent field in the Packet struct for the
 *                 field of the type of netEvent
 */
static void matchEvents(NetEvent* const netEvent, GHashTable* const
	unMatchedList, GHashTable* const unMatchedOppositeList, LttEvent* const
	event, LttvTraceHook* const trace_hook, const size_t fieldOffset)
{
	Packet* packet;
	GQueue* uaList;
	GList* result;
	SyncState* syncState;
	NetEvent* companionEvent;

	syncState= (SyncState*) trace_hook->hook_data;

	// Search unmatched list of opposite type for a matching event
	packet= malloc(sizeof(Packet));
	packet->connKey.saddr= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 1));
	packet->connKey.daddr= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 2));
	packet->tot_len= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 3));
	packet->ihl= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 4));
	packet->connKey.source= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 5));
	packet->connKey.dest= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 6));
	packet->seq= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 7));
	packet->ack_seq= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 8));
	packet->doff= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 9));
	packet->ack= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 10));
	packet->rst= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 11));
	packet->syn= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 12));
	packet->fin= ltt_event_get_long_unsigned(event,
		lttv_trace_get_hook_field(trace_hook, 13));
	packet->inE= packet->outE= NULL;
	packet->acks= NULL;

	companionEvent= g_hash_table_lookup(unMatchedOppositeList, packet);
	if (companionEvent != NULL)
	{
		if (optionSyncStats)
		{
			syncState->stats->totPacket++;
		}

		g_debug("Found matching companion event, ");
		// If it's there, remove it and update the structures
		g_hash_table_steal(unMatchedOppositeList, packet);
		free(packet);
		packet= companionEvent->packet;
		*((NetEvent**) ((void*) packet + fieldOffset))= netEvent;

		// If this packet acknowleges some data ...
		if (isAck(packet))
		{
			uaList= g_hash_table_lookup(syncState->unAcked, &packet->connKey);
			if (uaList != NULL)
			{
				Packet* ackedPacket;

				result= g_queue_find_custom(uaList, packet, &packetAckCompare);

				while (result != NULL)
				{
					// Remove the acknowledged packet from the unAcked list
					// and keep this packet for later offset calculations
					g_debug("Found matching unAcked packet, ");

					ackedPacket= (Packet*) result->data;
					g_queue_delete_link(uaList, result);

					// If the acked packet doesn't have both of its events,
					// remove the orphaned event from the corresponding
					// unmatched list and destroy the acked packet (an event
					// was not in the trace)
					if (ackedPacket->inE == NULL)
					{
						g_hash_table_steal(syncState->unMatchedOutE, packet);
						destroyPacket(ackedPacket);
					}
					else if (ackedPacket->outE == NULL)
					{
						g_hash_table_steal(syncState->unMatchedInE, packet);
						destroyPacket(ackedPacket);
					}
					else
					{
						if (optionSyncStats)
						{
							syncState->stats->totExchangeEffective++;
						}

						if (packet->acks == NULL)
						{
							packet->acks= g_queue_new();
						}
						else if (optionSyncStats)
						{
							syncState->stats->totPacketCummAcked++;
						}

						g_queue_push_tail(packet->acks, ackedPacket);
					}

					result= g_queue_find_custom(uaList, packet,
						&packetAckCompare);
				}

				// It might be possible to do an offset calculation
				if (packet->acks != NULL)
				{
					if (optionSyncStats)
					{
						syncState->stats->totExchangeReal++;
					}

					g_debug("Synchronization calculation, ");
					g_debug("%d acked packets - using last one, ",
						g_queue_get_length(packet->acks));

					ackedPacket= g_queue_peek_tail(packet->acks);
					if (ackedPacket->outE->trace != packet->inE->trace ||
						ackedPacket->inE->trace != packet->outE->trace)
					{
						g_debug("disorganized exchange - discarding, ");
					}
					else if (ackedPacket->outE->trace ==
						ackedPacket->inE->trace)
					{
						g_debug("packets from the same trace - discarding, ");
					}
					else
					{
						double dji, eji;
						double timoy;
						unsigned int ni, nj;
						LttTrace* orig_key;
						Fit* fit;

						// Calculate the intermediate values for the
						// least-squares analysis
						dji= ((double) ackedPacket->inE->tsc - (double)
							ackedPacket->outE->tsc + (double) packet->outE->tsc
							- (double) packet->inE->tsc) / 2;
						eji= fabs((double) ackedPacket->inE->tsc - (double)
							ackedPacket->outE->tsc - (double) packet->outE->tsc
							+ (double) packet->inE->tsc) / 2;
						timoy= ((double) ackedPacket->outE->tsc + (double)
							packet->inE->tsc) / 2;
						g_assert(g_hash_table_lookup_extended(syncState->traceNumTable,
								ackedPacket->outE->trace, (gpointer*)
								&orig_key, (gpointer*) &ni));
						g_assert(g_hash_table_lookup_extended(syncState->traceNumTable,
								ackedPacket->inE->trace, (gpointer*)
								&orig_key, (gpointer*) &nj));
						fit= &syncState->fitArray[nj][ni];

						fit->n++;
						fit->st+= timoy;
						fit->st2+= pow(timoy, 2);
						fit->sd+= dji;
						fit->sd2+= pow(dji, 2);
						fit->std+= timoy * dji;

						g_debug("intermediate values: dji= %f ti moy= %f "
							"ni= %u nj= %u fit: n= %u st= %f st2= %f sd= %f "
							"sd2= %f std= %f, ", dji, timoy, ni, nj, fit->n,
							fit->st, fit->st2, fit->sd, fit->sd2, fit->std);

						if (optionSyncData)
						{
							double freq;

							freq= syncState->tsc->traces[ni]->t->start_freq *
								syncState->tsc->traces[ni]->t->freq_scale;

							fprintf(syncState->dataFd, "%10u %10u %21.10f %21.10f %21.10f\n", ni,
								nj, timoy / freq, dji / freq, eji / freq);
						}
					}
				}
			}
		}

		if (needsAck(packet))
		{
			if (optionSyncStats)
			{
				syncState->stats->totPacketNeedAck++;
			}

			// If this packet will generate an ack, add it to the unAcked list
			g_debug("Adding to unAcked, ");
			uaList= g_hash_table_lookup(syncState->unAcked, &packet->connKey);
			if (uaList == NULL)
			{
				ConnectionKey* connKey;

				connKey= malloc(sizeof(ConnectionKey));
				memcpy(connKey, &packet->connKey, sizeof(ConnectionKey));
				g_hash_table_insert(syncState->unAcked, connKey, uaList= g_queue_new());
			}
			g_queue_push_tail(uaList, packet);
		}
		else
		{
			destroyPacket(packet);
		}
	}
	else
	{
		// If there's no corresponding event, finish creating the data
		// structures and add an event to the unmatched list for this type of
		// event
		netEvent->packet= packet;
		*((NetEvent**) ((void*) packet + fieldOffset))= netEvent;

		g_debug("Adding to unmatched event list, ");
		g_hash_table_insert(unMatchedList, packet, netEvent);
	}
}


/*
 * Check if a packet is an acknowledge
 *
 * Returns:
 *   true if it is,
 *   false otherwise
 */
static bool isAck(const Packet* const packet)
{
	if (packet->ack == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Check if a packet is an acknowledge of another packet.
 *
 * Args:
 *   ackPacket     packet that is the confirmation
 *   ackedPacket   packet that contains the original data, both packets have to
 *                 come from the same connection
 */
static bool isAcking(const Packet* const ackPacket, const Packet* const ackedPacket)
{
	if (ackedPacket->connKey.saddr == ackPacket->connKey.daddr &&
		ackedPacket->connKey.daddr == ackPacket->connKey.saddr &&
		ackedPacket->connKey.source == ackPacket->connKey.dest &&
		ackedPacket->connKey.dest == ackPacket->connKey.source &&
		ackPacket->ack_seq > ackedPacket->seq)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Check if a packet will increment the sequence number, thus needing an
 * acknowledge
 *
 * Returns:
 *   true if the packet will need an acknowledge
 *   false otherwise
 */
static bool needsAck(const Packet* const packet)
{
	if (packet->syn || packet->fin || packet->tot_len - packet->ihl * 4 -
		packet->doff * 4 > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Compare two ConnectionKey structures
 *
 * Returns:
 *   true if each field of the structure is equal
 *   false otherwise
 */
static bool connectionKeyEqual(const ConnectionKey* const a, const ConnectionKey* const b)
{
	if (a->saddr == b->saddr && a->daddr == b->daddr && a->source ==
		b->source && a->dest == b->dest)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         GQueue* list[NetEvent]
 */
static void netEventListDestroy(gpointer data)
{
	GQueue* list;

	list= (GQueue*) data;

	g_debug("XXXX netEventListDestroy\n");
	g_queue_foreach(list, &netEventRemove, NULL);
	g_queue_free(list);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data:         NetEvent* event
 *   user_data:    NULL
 */
static void netEventRemove(gpointer data, gpointer user_data)
{
	destroyNetEvent((NetEvent*) data);
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * This function is for indexing netEvents in unMatched lists. All fields of
 * the corresponding packet must match for two keys to be equal.
 *
 * Args:
 *    key        Packet*
 */
static guint netEventPacketHash(gconstpointer key)
{
	const Packet* p;
	uint32_t a, b, c;

	p= key;

	a= p->connKey.source + (p->connKey.dest << 16);
	b= p->connKey.saddr;
	c= p->connKey.daddr;
	mix(a, b, c);

	a+= p->tot_len;
	b+= p->ihl;
	c+= p->seq;
	mix(a, b, c);

	a+= p->ack_seq;
	b+= p->doff;
	c+= p->ack;
	mix(a, b, c);

	a+= p->rst;
	b+= p->syn;
	c+= p->fin;
	final(a, b, c);

	return c;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * This function is for indexing netEvents in unMatched lists. All fields of
 * the corresponding packet must match for two keys to be equal.
 *
 * Args:
 *   a, b          Packet*
 *
 * Returns:
 *   TRUE if both values are equal
 */
static gboolean netEventPacketEqual(gconstpointer a, gconstpointer b)
{
	const Packet* pA, * pB;

	pA= a;
	pB= b;

	if (connectionKeyEqual(&pA->connKey, &pB->connKey) &&
		pA->tot_len == pB->tot_len &&
		pA->ihl == pB->ihl &&
		pA->seq == pB->seq &&
		pA->ack_seq == pB->ack_seq &&
		pA->doff == pB->doff &&
		pA->ack == pB->ack &&
		pA->rst == pB->rst &&
		pA->syn == pB->syn &&
		pA->fin == pB->fin)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         NetEvent*
 */
static void ghtDestroyNetEvent(gpointer data)
{
	destroyNetEvent((NetEvent*) data);
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         GQueue* list[Packet]
 */
static void packetListDestroy(gpointer data)
{
	GQueue* list;

	list= (GQueue*) data;

	g_debug("XXXX packetListDestroy\n");

	g_queue_foreach(list, &packetRemove, NULL);
	g_queue_free(list);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Packet*, packet to destroy
 *   user_data     NULL
 */
static void packetRemove(gpointer data, gpointer user_data)
{
	Packet* packet;

	packet= (Packet*) data;

	g_debug("XXXX packetRemove\n");
	destroyPacket(packet);
}


/*
 * Free the memory used by a Packet and the memory of all its associated
 * resources
 */
static void destroyPacket(Packet* const packet)
{
	g_debug("XXXX destroyPacket ");
	printPacket(packet);
	g_debug("\n");

	if (packet->inE)
	{
		free(packet->inE);
	}

	if (packet->outE)
	{
		free(packet->outE);
	}

	if (packet->acks)
	{
		g_queue_foreach(packet->acks, &packetRemove, NULL);
		g_queue_free(packet->acks);
	}

	free(packet);
}


/*
 * Free the memory used by a NetEvent and the memory of all its associated
 * resources. If the netEvent is part of a packet that also contains the other
 * netEvent, that one will be freed also. Beware not to keep references to that
 * other one.
 */
static void destroyNetEvent(NetEvent* const event)
{
	g_debug("XXXX destroyNetEvent\n");
	if (event->packet)
	{
		destroyPacket(event->packet);
	}
	else
	{
		free(event);
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Graph*, graph to destroy
 *   user_data     NULL
 */
static void graphRemove(gpointer data, gpointer user_data)
{
	Graph* graph;

	graph= (Graph*) data;

	free(graph->previousVertex);
	free(graph);
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a:            NetEvent*
 *   b:            NetEvent*
 *
 * Returns
 *   0 if the two events have the same skb
 */
static gint netEventSkbCompare(gconstpointer a, gconstpointer b)
{
	if (((NetEvent*) a)->skb == ((NetEvent*) b)->skb)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * A GCompareFunc to be used with g_queue_find_custom()
 *
 * Args:
 *   a             NetEvent*
 *   b             NetEvent*
 *
 * Returns:
 *   0 if the two net events correspond to the send and receive events of the
 *   same packet
 */
static gint netEventPacketCompare(gconstpointer a, gconstpointer b)
{
	Packet* pA, * pB;

	pA= ((NetEvent*) a)->packet;
	pB= ((NetEvent*) b)->packet;

	if (netEventPacketEqual(a, b))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a          Packet* acked packet
 *   b          Packet* ack packet
 *
 * Returns:
 *   0 if b acks a
 */
static gint packetAckCompare(gconstpointer a, gconstpointer b)
{
	if (isAcking(((Packet*) b), ((Packet*) a)))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * A GCompareFunc for g_queue_find_custom()
 *
 * Args:
 *   a:            Graph* graph
 *   b:            unsigned int* traceNum
 *
 * Returns:
 *   0 if graph contains traceNum
 */
static gint graphTraceCompare(gconstpointer a, gconstpointer b)
{
	Graph* graph;
	unsigned int traceNum;

	graph= (Graph*) a;
	traceNum= *(unsigned int *) b;

	if (graph->previousVertex[traceNum] != UINT_MAX)
	{
		return 0;
	}
	else if (graph->reference == traceNum)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


/*
 * A GHashFunc for g_hash_table_new()
 *
 * 2.4 kernels used tcp_hashfn(),
 *
 * I've seen something about an XOR hash:
 * http://tservice.net.ru/~s0mbre/blog/2006/05/14#2006_05_14:
 * unsigned int h = (laddr ^ lport) ^ (faddr ^ fport);
 * h ^= h >> 16;
 * h ^= h >> 8;
 * return h;
 *
 * and in 2.6 kernels inet_ehashfn() handles connection hashing with the help of
 * Jenkins hashing, jhash.h
 *
 * This function uses the XOR method.
 *
 * Args:
 *    key        ConnectionKey*
 */
static guint connectionHash(gconstpointer key)
{
	ConnectionKey* connKey;
	guint result;

	connKey= (ConnectionKey*) key;

	result= (connKey->saddr ^ connKey->source) ^ (connKey->daddr ^ connKey->dest);
	result^= result >> 16;
	result^= result >> 8;

	return result;
}


/*
 * A GEqualFunc for g_hash_table_new()
 *
 * Args:
 *   a, b          ConnectionKey*
 *
 * Returns:
 *   TRUE if both values are equal
 */
static gboolean connectionEqual(gconstpointer a, gconstpointer b)
{
	ConnectionKey* ckA, * ckB;

	ckA= (ConnectionKey*) a;
	ckB= (ConnectionKey*) b;

	// Two packets in the same direction
	if (ckA->saddr == ckB->saddr && ckA->daddr == ckB->daddr && ckA->source ==
		ckB->source && ckA->dest == ckB->dest)
	{
		return TRUE;
	}
	// Two packets in opposite directions
	else if (ckA->saddr == ckB->daddr && ckA->daddr == ckB->saddr &&
		ckA->source == ckB->dest && ckA->dest == ckB->source)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
 * A GDestroyNotify function for g_hash_table_new_full()
 *
 * Args:
 *   data:         ConnectionKey*
 */
static void connectionDestroy(gpointer data)
{
	free((ConnectionKey*) data);
}


/*
 * Convert an IP address from 32 bit form to dotted quad
 *
 * Args:
 *   str:          A preallocated string of length >= 17
 *   addr:         Address
 */
static void convertIP(char* const str, const uint32_t addr)
{
	struct in_addr iaddr;

	iaddr.s_addr= htonl(addr);
	strcpy(str, inet_ntoa(iaddr));
}


/*
 * Print the content of a Packet structure
 */
static void printPacket(const Packet* const packet)
{
	char saddr[17], daddr[17];

	convertIP(saddr, packet->connKey.saddr);
	convertIP(daddr, packet->connKey.daddr);
	g_debug("%s:%u to %s:%u tot_len: %u ihl: %u seq: %u ack_seq: %u doff: %u "
		"ack: %u rst: %u syn: %u fin: %u", saddr, packet->connKey.source,
		daddr, packet->connKey.dest, packet->tot_len, packet->ihl, packet->seq,
		packet->ack_seq, packet->doff, packet->ack, packet->rst, packet->syn,
		packet->fin);
}


/*
 * Single-source shortest path search to find the path with the lowest error to
 * convert one node's time to another.
 * Uses Dijkstra's algorithm
 *
 * Args:
 *   fitArray:     array with the regression parameters
 *   traceNum:     reference node
 *   traceNb:      number of traces = number of nodes
 *   distances:    array of computed distance from source node to node i,
 *                 INFINITY if i is unreachable, preallocated to the number of
 *                 nodes
 *   previousVertex: previous vertex from a node i on the way to the source,
 *                 UINT_MAX if i is not on the way or is the source,
 *                 preallocated to the number of nodes
 */
static void shortestPath(Fit* const* const fitArray, const unsigned int
	traceNum, const unsigned int traceNb, double* const distances, unsigned
	int* const previousVertex)
{
	bool* visited;
	unsigned int i, j;

	visited= malloc(traceNb * sizeof(bool));

	for (i= 0; i < traceNb; i++)
	{
		const Fit* fit;

		visited[i]= false;

		fit= &fitArray[traceNum][i];
		g_debug("fitArray[traceNum= %u][i= %u]->n = %u\n", traceNum, i, fit->n);
		if (fit->n > 0)
		{
			distances[i]= fit->e;
			previousVertex[i]= traceNum;
		}
		else
		{
			distances[i]= INFINITY;
			previousVertex[i]= UINT_MAX;
		}
	}
	visited[traceNum]= true;

	for (j= 0; j < traceNb; j++)
	{
		g_debug("(%d, %u, %g), ", visited[j], previousVertex[j], distances[j]);
	}
	g_debug("\n");

	for (i= 0; i < traceNb - 2; i++)
	{
		unsigned int v;
		double dvMin;

		dvMin= INFINITY;
		for (j= 0; j < traceNb; j++)
		{
			if (visited[j] == false && distances[j] < dvMin)
			{
				v= j;
				dvMin= distances[j];
			}
		}

		g_debug("v= %u dvMin= %g\n", v, dvMin);

		if (dvMin != INFINITY)
		{
			visited[v]= true;

			for (j= 0; j < traceNb; j++)
			{
				const Fit* fit;

				fit= &fitArray[v][j];
				if (visited[j] == false && fit->n > 0 && distances[v] + fit->e
					< distances[j])
				{
					distances[j]= distances[v] + fit->e;
					previousVertex[j]= v;
				}
			}
		}
		else
		{
			break;
		}

		for (j= 0; j < traceNb; j++)
		{
			g_debug("(%d, %u, %g), ", visited[j], previousVertex[j], distances[j]);
		}
		g_debug("\n");
	}

	free(visited);
}


/*
 * Cummulate the distances between a reference node and the other nodes
 * reachable from it in a graph.
 *
 * Args:
 *   distances:    array of shortest path distances, with UINT_MAX for
 *                 unreachable nodes
 *   traceNb:      number of nodes = number of traces
 */
static double sumDistances(const double* const distances, const unsigned int traceNb)
{
	unsigned int i;
	double result;

	result= 0;
	for (i= 0; i < traceNb; i++)
	{
		if (distances[i] != INFINITY)
		{
			result+= distances[i];
		}
	}

	return result;
}


/*
 * Cummulate the time correction factors between two nodes accross a graph
 *
 * With traceNum i, reference node r:
 * tr= (1 + Xri) * ti + D0ri
 *   = drift * ti + offset
 *
 * Args:
 *   fitArray:     array with the regression parameters
 *   previousVertex: previous vertex from a node i on the way to the source,
 *                 UINT_MAX if i is not on the way or is the source,
 *                 preallocated to the number of nodes
 *   traceNum:     end node, the reference depends on previousVertex
 *   drift:        drift factor
 *   offset:       offset factor
 */
static void factors(Fit* const* const fitArray, const unsigned int* const
	previousVertex, const unsigned int traceNum, double* const drift, double*
	const offset, double* const stDev)
{
	if (previousVertex[traceNum] == UINT_MAX)
	{
		*drift= 1.;
		*offset= 0.;
		*stDev= 0.;
	}
	else
	{
		const Fit* fit;
		double cummDrift, cummOffset, cummStDev;
		unsigned int pv;

		pv= previousVertex[traceNum];

		fit= &fitArray[pv][traceNum];
		factors(fitArray, previousVertex, pv, &cummDrift, &cummOffset, &cummStDev);

		*drift= cummDrift * (1 + fit->x);
		*offset= cummDrift * fit->d0 + cummOffset;
		*stDev= fit->x * cummStDev + fit->e;
	}
}


/*
 * Calculate the elapsed time between two timeval values
 *
 * Args:
 *   end:          end time, result is also stored in this structure
 *   start:        start time
 */
static void timeDiff(struct timeval* const end, const struct timeval* const start)
{
		if (end->tv_usec >= start->tv_usec)
		{
			end->tv_sec-= start->tv_sec;
			end->tv_usec-= start->tv_usec;
		}
		else
		{
			end->tv_sec= end->tv_sec - start->tv_sec - 1;
			end->tv_usec= end->tv_usec - start->tv_usec + 1e6;
		}
}


LTTV_MODULE("sync", "Synchronize traces", \
	"Synchronizes a traceset based on the correspondance of network events", \
	init, destroy)
