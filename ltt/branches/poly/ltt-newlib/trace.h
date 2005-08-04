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

#ifndef TRACE_H
#define TRACE_H

#include <ltt/ltt.h>


extern GQuark LTT_FACILITY_NAME_HEARTBEAT,
              LTT_EVENT_NAME_HEARTBEAT;

/* A trace is specified as a pathname to the directory containing all the
   associated data (control tracefiles, per cpu tracefiles, event 
   descriptions...).

   When a trace is closed, all the associated facilities, types and fields
   are released as well. 
   
   return value is NULL if there is an error when opening the trace.
   
   */

LttTrace *ltt_trace_open(const gchar *pathname);

/* copy reopens a trace 
 *
 * return value NULL if error while opening the trace 
 */
LttTrace *ltt_trace_copy(LttTrace *self);

gchar * ltt_trace_name(LttTrace *t);

void ltt_trace_close(LttTrace *t); 


LttSystemDescription *ltt_trace_system_description(LttTrace *t);


/* Functions to discover the facilities in the trace. Once the number
   of facilities is known, they may be accessed by position. Multiple
   versions of a facility (same name, different checksum) have consecutive
   positions. */

unsigned ltt_trace_facility_number(LttTrace *t);

LttFacility *ltt_trace_facility_get(LttTrace *t, unsigned i);

LttFacility * ltt_trace_facility_by_id(LttTrace * trace, unsigned id);

/* Look for a facility by name. It returns the number of facilities found
   and sets the position argument to the first found. Returning 0, the named
   facility is unknown, returning 1, the named facility is at the specified
   position, returning n, the facilities are from position to 
   position + n - 1. */

unsigned ltt_trace_facility_find(LttTrace *t, gchar *name, unsigned *position);


/* Functions to discover all the event types in the trace */

unsigned ltt_trace_eventtype_number(LttTrace *t);

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned i);


/* There is one "per cpu" tracefile for each CPU, numbered from 0 to
   the maximum number of CPU in the system. When the number of CPU installed
   is less than the maximum, some positions are unused. There are also a
   number of "control" tracefiles (facilities, interrupts...). */

unsigned ltt_trace_control_tracefile_number(LttTrace *t);

unsigned ltt_trace_per_cpu_tracefile_number(LttTrace *t);


/* It is possible to search for the tracefiles by name or by CPU tracefile
 * name.
 * The index within the tracefiles of the same type is returned if found
 * and a negative value otherwise.
 */

int ltt_trace_control_tracefile_find(LttTrace *t, const gchar *name);

int ltt_trace_per_cpu_tracefile_find(LttTrace *t, const gchar *name);


/* Get a specific tracefile */

LttTracefile *ltt_trace_control_tracefile_get(LttTrace *t, unsigned i);

LttTracefile *ltt_trace_per_cpu_tracefile_get(LttTrace *t, unsigned i);


/* Get the start time and end time of the trace */

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end);


/* Get the name of a tracefile */

char *ltt_tracefile_name(LttTracefile *tf);


/* Get the number of blocks in the tracefile */

unsigned ltt_tracefile_block_number(LttTracefile *tf);


/* Seek to the first event of the trace with time larger or equal to time */

void ltt_tracefile_seek_time(LttTracefile *t, LttTime time);

/* Seek to the first event with position equal or larger to ep */

void ltt_tracefile_seek_position(LttTracefile *t,
    const LttEventPosition *ep);

/* Read the next event */

LttEvent *ltt_tracefile_read(LttTracefile *t, LttEvent *event);

/* open tracefile */

LttTracefile * ltt_tracefile_open(LttTrace *t, gchar * tracefile_name);

void ltt_tracefile_open_cpu(LttTrace *t, gchar * tracefile_name);

gint ltt_tracefile_open_control(LttTrace *t, gchar * control_name);


/* get the data type size and endian type of the local machine */

void getDataEndianType(LttArchSize * size, LttArchEndian * endian);

/* get an integer number */

gint64 getIntNumber(gboolean reverse_byte_order, int size1, void *evD);


/* get the node name of the system */

gchar * ltt_trace_system_description_node_name (LttSystemDescription * s);


/* get the domain name of the system */

gchar * ltt_trace_system_description_domain_name (LttSystemDescription * s);


/* get the description of the system */

gchar * ltt_trace_system_description_description (LttSystemDescription * s);


/* get the start time of the trace */

LttTime ltt_trace_system_description_trace_start_time(LttSystemDescription *s);

/* copy tracefile info over another. Used for sync. */
LttTracefile *ltt_tracefile_new();
void ltt_tracefile_destroy(LttTracefile *tf);
void ltt_tracefile_copy(LttTracefile *dest, const LttTracefile *src);

void get_absolute_pathname(const gchar *pathname, gchar * abs_pathname);



#endif // TRACE_H
