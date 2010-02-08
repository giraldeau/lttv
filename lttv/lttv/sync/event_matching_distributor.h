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

#ifndef EVENT_MATCHING_DISTRIBUTOR_H
#define EVENT_MATCHING_DISTRIBUTOR_H

#include <glib.h>

#include "data_structures.h"
#include "sync_chain.h"


typedef struct
{
	// SyncState* distributedModules[]
	GQueue* distributedModules;
} MatchingDataDistributor;

void registerMatchingDistributor();

#endif
