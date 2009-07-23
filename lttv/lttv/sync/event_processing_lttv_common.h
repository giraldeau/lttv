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

#ifndef EVENT_PROCESSING_LTTV_COMMON_H
#define EVENT_PROCESSING_LTTV_COMMON_H

#include <glib.h>

#include <lttv/tracecontext.h>


GQuark
	LTT_CHANNEL_NET,
	LTT_CHANNEL_NETIF_STATE;

GQuark
	LTT_EVENT_DEV_XMIT,
	LTT_EVENT_DEV_RECEIVE,
	LTT_EVENT_PKFREE_SKB,
	LTT_EVENT_TCPV4_RCV,
	LTT_EVENT_NETWORK_IPV4_INTERFACE;

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
	LTT_FIELD_NAME,
    LTT_FIELD_ADDRESS,
    LTT_FIELD_UP;


void createQuarks();
void registerHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext, unsigned int traceNb, LttvHook hookFunction, gpointer
	hookData);
void unregisterHooks(GArray* hookListList, LttvTracesetContext* const
	traceSetContext, unsigned int traceNb);

#endif
