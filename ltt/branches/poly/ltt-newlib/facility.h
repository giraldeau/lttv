/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef FACILITY_H
#define FACILITY_H

#include <ltt/ltt.h>

/* Facilities are obtained from an opened trace. The structures associated 
   with a facility are released when the trace is closed. Each facility
   is characterized by its name and checksum. */

GQuark ltt_facility_name(LttFacility *f);

LttChecksum ltt_facility_checksum(LttFacility *f);

/* open facility */
void ltt_facility_open(LttTrace * t, GQuark facility_name);

/* Discover the event types within the facility. The event type integer id
   relative to the trace is from 0 to nb_event_types - 1. The event
   type id within the trace is the relative id + the facility base event
   id. */

unsigned ltt_facility_base_id(LttFacility *f);

unsigned ltt_facility_eventtype_number(LttFacility *f);

LttEventType *ltt_facility_eventtype_get(LttFacility *f, unsigned i);

LttEventType *ltt_facility_eventtype_get_by_name(LttFacility *f, GQuark name);

int ltt_facility_close(LttFacility *f);

/* Reserved facility names */

static const char *ltt_facility_name_core = "core";

#endif // FACILITY_H

