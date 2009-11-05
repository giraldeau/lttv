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

#ifndef EVENT_PROCESSING_H
#define EVENT_PROCESSING_H

#include <glib.h>
#include <stdio.h>

#include <lttv/tracecontext.h>

#include "data_structures.h"


struct _SyncState;

typedef struct
{
	char* name;

	void (*initProcessing)(struct _SyncState* const syncStateLttv,
		LttvTracesetContext* const traceSetContext);
	void (*destroyProcessing)(struct _SyncState* const syncState);

	void (*finalizeProcessing)(struct _SyncState* const syncState);

	void (*printProcessingStats)(struct _SyncState* const syncState);

	/* The processing module must provide the next function if it wishes
	 * graphs to be created at all. If it provides the next function, it must
	 * also provide the second next function.
	 */
	void (*writeProcessingGraphsPlots)(struct _SyncState* const syncState,
		const unsigned int i, const unsigned int j);
	void (*writeProcessingGraphsOptions)(struct _SyncState* const syncState,
		const unsigned int i, const unsigned int j);
} ProcessingModule;

#endif
