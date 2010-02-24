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

#ifndef EVENT_PROCESSING_LTTNG_COMMON_H
#define EVENT_PROCESSING_LTTNG_COMMON_H

#include <glib.h>
#include <stdbool.h>

#include <lttv/tracecontext.h>


GQuark
	LTT_CHANNEL_NET;

GQuark
	LTT_EVENT_DEV_XMIT_EXTENDED,
	LTT_EVENT_DEV_RECEIVE,
	LTT_EVENT_TCPV4_RCV_EXTENDED,
	LTT_EVENT_UDPV4_RCV_EXTENDED;

GQuark
	LTT_FIELD_SKB,
	LTT_FIELD_PROTOCOL,
	LTT_FIELD_NETWORK_PROTOCOL,
	LTT_FIELD_TRANSPORT_PROTOCOL,
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
	LTT_FIELD_UNICAST,
	LTT_FIELD_ULEN,
	LTT_FIELD_DATA_START;


void createQuarks();
void registerHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext, LttvHook hookFunction, gpointer hookData, const bool
	const* eventTypes);
void unregisterHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext);

#endif
