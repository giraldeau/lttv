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

GQuark ltt_trace_name(LttTrace *t);

void ltt_trace_close(LttTrace *t); 


LttSystemDescription *ltt_trace_system_description(LttTrace *t);


/* Functions to discover the facilities in the trace. Once the number
   of facilities is known, they may be accessed by position. Multiple
   versions of a facility (same name, different checksum) have consecutive
   positions. */

unsigned ltt_trace_facility_number(LttTrace *t);

LttFacility *ltt_trace_facility_get(LttTrace *t, unsigned i);

LttFacility * ltt_trace_facility_by_id(LttTrace * trace, guint8 id);

/* Returns an array of indexes (guint) that matches the facility name */
GArray *ltt_trace_facility_get_by_name(LttTrace *t, GQuark name);

/* Functions to discover all the event types in the trace */

unsigned ltt_trace_eventtype_number(LttTrace *t);

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned i);


/* Get the start time and end time of the trace */

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end);


/* Get the name of a tracefile */

GQuark ltt_tracefile_name(LttTracefile *tf);

/* get the cpu number of the tracefile */

guint ltt_tracefile_num(LttTracefile *tf);

LttTrace *ltt_tracefile_get_trace(LttTracefile *tf);

/* Get the number of blocks in the tracefile */

unsigned ltt_tracefile_block_number(LttTracefile *tf);


/* Seek to the first event of the trace with time larger or equal to time */

int ltt_tracefile_seek_time(LttTracefile *t, LttTime time);

/* Seek to the first event with position equal or larger to ep */

int ltt_tracefile_seek_position(LttTracefile *t,
    const LttEventPosition *ep);

/* Read the next event */

int ltt_tracefile_read(LttTracefile *t);

/* ltt_tracefile_read cut down in pieces */
int ltt_tracefile_read_seek(LttTracefile *t);
int ltt_tracefile_read_update_event(LttTracefile *t);
int ltt_tracefile_read_op(LttTracefile *t);

/* Get the current event of the tracefile : valid until the next read */
LttEvent *ltt_tracefile_get_event(LttTracefile *tf);

/* open tracefile */

gint ltt_tracefile_open(LttTrace *t, gchar * fileName, LttTracefile *tf);

/* get the data type size and endian type of the local machine */

void getDataEndianType(LttArchSize * size, LttArchEndian * endian);

/* get an integer number */
gint64 get_int(gboolean reverse_byte_order, gint size, void *data);

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

/* May return a NULL tracefile group */
GData **ltt_trace_get_tracefiles_groups(LttTrace *trace);

typedef void (*ForEachTraceFileFunc)(LttTracefile *tf, gpointer func_args);

struct compute_tracefile_group_args {
  ForEachTraceFileFunc func;
  gpointer func_args;
};


void compute_tracefile_group(GQuark key_id,
                             GArray *group,
                             struct compute_tracefile_group_args *args);

LttFacility *ltt_trace_get_facility_by_num(LttTrace *t, guint num);


gint check_fields_compatibility(LttEventType *event_type1,
    LttEventType *event_type2,
    LttField *field1, LttField *field2);

#endif // TRACE_H
