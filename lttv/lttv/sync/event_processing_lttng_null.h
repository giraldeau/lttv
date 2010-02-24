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

#ifndef EVENT_PROCESSING_LTTNG_NULL_H
#define EVENT_PROCESSING_LTTNG_NULL_H

#include <glib.h>

#include <lttv/tracecontext.h>


typedef struct
{
	LttvTracesetContext* traceSetContext;

	// hookListList conceptually is a two dimensionnal array of LttvTraceHook
	// elements. It uses GArrays to interface with other lttv functions that
	// do.
	// LttvTraceHook hookListList[traceNum][hookNum]
	GArray* hookListList;
} ProcessingDataLTTVNull;

void registerProcessingLTTVNull();

#endif
