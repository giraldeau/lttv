/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009, 2010 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "data_structures.h"
#include "event_processing_lttng_common.h"


/*
 * Initialize the GQuarks needed to register the event hooks for
 * synchronization
 */
void createQuarks()
{
	LTT_CHANNEL_NET= g_quark_from_static_string("net");

	LTT_EVENT_DEV_XMIT_EXTENDED= g_quark_from_static_string("dev_xmit_extended");
	LTT_EVENT_DEV_RECEIVE= g_quark_from_static_string("dev_receive");
	LTT_EVENT_TCPV4_RCV_EXTENDED= g_quark_from_static_string("tcpv4_rcv_extended");
	LTT_EVENT_UDPV4_RCV_EXTENDED= g_quark_from_static_string("udpv4_rcv_extended");

	LTT_FIELD_SKB= g_quark_from_static_string("skb");
	LTT_FIELD_PROTOCOL= g_quark_from_static_string("protocol");
	LTT_FIELD_NETWORK_PROTOCOL=
		g_quark_from_static_string("network_protocol");
	LTT_FIELD_TRANSPORT_PROTOCOL=
		g_quark_from_static_string("transport_protocol");
	LTT_FIELD_SADDR= g_quark_from_static_string("saddr");
	LTT_FIELD_DADDR= g_quark_from_static_string("daddr");
	LTT_FIELD_TOT_LEN= g_quark_from_static_string("tot_len");
	LTT_FIELD_IHL= g_quark_from_static_string("ihl");
	LTT_FIELD_SOURCE= g_quark_from_static_string("source");
	LTT_FIELD_DEST= g_quark_from_static_string("dest");
	LTT_FIELD_SEQ= g_quark_from_static_string("seq");
	LTT_FIELD_ACK_SEQ= g_quark_from_static_string("ack_seq");
	LTT_FIELD_DOFF= g_quark_from_static_string("doff");
	LTT_FIELD_ACK= g_quark_from_static_string("ack");
	LTT_FIELD_RST= g_quark_from_static_string("rst");
	LTT_FIELD_SYN= g_quark_from_static_string("syn");
	LTT_FIELD_FIN= g_quark_from_static_string("fin");
	LTT_FIELD_UNICAST= g_quark_from_static_string("unicast");
	LTT_FIELD_ULEN= g_quark_from_static_string("ulen");
	LTT_FIELD_DATA_START= g_quark_from_static_string("data_start");
}


/* Fill hookListList and add event hooks
 *
 * Note: possibilité de remettre le code avec lttv_trace_find_marker_ids (voir
 * r328)
 *
 * Args:
 *   hookListList: LttvTraceHook hookListList[traceNum][hookNum]
 *   traceSetContext: LTTV traceset
 *   hookFunction: call back function when event is encountered
 *   hookData:     data that will be made accessible to hookFunction in
 *                 arg0->hook_data
 *   eventTypes:   types of events for which to register hooks
 */
void registerHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext, LttvHook hookFunction, gpointer hookData, const bool
	const* eventTypes)
{
	unsigned int i, j, k;
	unsigned int traceNb= lttv_traceset_number(traceSetContext->ts);
	struct {
		GQuark channelName;
		GQuark eventName;
		GQuark* fields;
		bool eventTypes[TYPE_COUNT];
	} eventHookInfoList[] = {
		{
			.channelName= LTT_CHANNEL_NET,
			.eventName= LTT_EVENT_DEV_XMIT_EXTENDED,
			.fields= FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_NETWORK_PROTOCOL,
				LTT_FIELD_TRANSPORT_PROTOCOL, LTT_FIELD_SADDR,
				LTT_FIELD_DADDR, LTT_FIELD_TOT_LEN, LTT_FIELD_IHL,
				LTT_FIELD_SOURCE, LTT_FIELD_DEST, LTT_FIELD_SEQ,
				LTT_FIELD_ACK_SEQ, LTT_FIELD_DOFF, LTT_FIELD_ACK,
				LTT_FIELD_RST, LTT_FIELD_SYN, LTT_FIELD_FIN),
			.eventTypes[TCP]= true,
			.eventTypes[UDP]= true,
		}, {
			.channelName= LTT_CHANNEL_NET,
			.eventName= LTT_EVENT_DEV_RECEIVE,
			.fields= FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_PROTOCOL),
			.eventTypes[TCP]= true,
			.eventTypes[UDP]= true,
		}, {
			.channelName= LTT_CHANNEL_NET,
			.eventName= LTT_EVENT_TCPV4_RCV_EXTENDED,
			.fields= FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_SADDR,
				LTT_FIELD_DADDR, LTT_FIELD_TOT_LEN, LTT_FIELD_IHL,
				LTT_FIELD_SOURCE, LTT_FIELD_DEST, LTT_FIELD_SEQ,
				LTT_FIELD_ACK_SEQ, LTT_FIELD_DOFF, LTT_FIELD_ACK,
				LTT_FIELD_RST, LTT_FIELD_SYN, LTT_FIELD_FIN),
			.eventTypes[TCP]= true,
			.eventTypes[UDP]= false,
		}, {
			.channelName= LTT_CHANNEL_NET,
			.eventName= LTT_EVENT_UDPV4_RCV_EXTENDED,
			.fields= FIELD_ARRAY(LTT_FIELD_SKB, LTT_FIELD_SADDR,
				LTT_FIELD_DADDR, LTT_FIELD_UNICAST, LTT_FIELD_ULEN,
				LTT_FIELD_SOURCE, LTT_FIELD_DEST, LTT_FIELD_DATA_START),
			.eventTypes[TCP]= false,
			.eventTypes[UDP]= true,
		}
	}; // This is called a compound literal
	unsigned int hookNb= sizeof(eventHookInfoList) / sizeof(*eventHookInfoList);

	for(i= 0; i < traceNb; i++)
	{
		LttvTraceContext* tc;
		GArray* hookList;
		int retval;

		tc= traceSetContext->traces[i];
		hookList= g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
		g_array_append_val(hookListList, hookList);

		// Find the hooks
		for (j= 0; j < hookNb; j++)
		{
			guint old_len;
			bool registerHook;

			registerHook= false;
			for (k= 0; k < TYPE_COUNT; k++)
			{
				if (eventTypes[k] && eventHookInfoList[j].eventTypes[k])
				{
					registerHook= true;
					break;
				}
			}
			if (!registerHook)
			{
				continue;
			}

			old_len= hookList->len;
			retval= lttv_trace_find_hook(tc->t,
				eventHookInfoList[j].channelName,
				eventHookInfoList[j].eventName, eventHookInfoList[j].fields,
				hookFunction, hookData, &hookList);
			if (retval != 0)
			{
				g_warning("Trace %d contains no %s.%s marker\n", i,
					g_quark_to_string(eventHookInfoList[j].channelName),
					g_quark_to_string(eventHookInfoList[j].eventName));
			}
			else
			{
				g_assert(hookList->len - old_len == 1);
			}
		}

		// Add the hooks to each tracefile's event_by_id hook list
		for(j= 0; j < tc->tracefiles->len; j++)
		{
			LttvTracefileContext* tfc;

			tfc= g_array_index(tc->tracefiles, LttvTracefileContext*, j);

			for(k= 0; k < hookList->len; k++)
			{
				LttvTraceHook* traceHook;

				traceHook= &g_array_index(hookList, LttvTraceHook, k);
				if (traceHook->hook_data != hookData)
				{
					g_assert_not_reached();
				}
				if (traceHook->mdata == tfc->tf->mdata)
				{
					lttv_hooks_add(lttv_hooks_by_id_find(tfc->event_by_id,
							traceHook->id), traceHook->h, traceHook,
						LTTV_PRIO_DEFAULT);
				}
			}
		}
	}
}


/* Remove event hooks and free hookListList
 *
 * Args:
 *   hookListList: LttvTraceHook hookListList[traceNum][hookNum]
 *   traceSetContext: LTTV traceset
 */
void unregisterHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext)
{
	unsigned int i, j, k;
	unsigned int traceNb= lttv_traceset_number(traceSetContext->ts);

	for(i= 0; i < traceNb; i++)
	{
		LttvTraceContext* tc;
		GArray* hookList;

		tc= traceSetContext->traces[i];
		hookList= g_array_index(hookListList, GArray*, i);

		// Remove the hooks from each tracefile's event_by_id hook list
		for(j= 0; j < tc->tracefiles->len; j++)
		{
			LttvTracefileContext* tfc;

			tfc= g_array_index(tc->tracefiles, LttvTracefileContext*, j);

			for(k= 0; k < hookList->len; k++)
			{
				LttvTraceHook* traceHook;

				traceHook= &g_array_index(hookList, LttvTraceHook, k);
				if (traceHook->mdata == tfc->tf->mdata)
				{
					lttv_hooks_remove_data(lttv_hooks_by_id_find(tfc->event_by_id,
							traceHook->id), traceHook->h, traceHook);
				}
			}
		}

		g_array_free(hookList, TRUE);
	}
	g_array_free(hookListList, TRUE);
}
