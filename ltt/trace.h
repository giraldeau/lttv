/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
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
#include <ltt/ltt-private.h>
#include <stdint.h>
#include <glib.h>

struct LttTrace {
  GQuark pathname;                          //the pathname of the trace
  //LttSystemDescription * system_description;//system description 

  guint     num_cpu;

  guint32   arch_type;
  guint32   arch_variant;
  guint8    arch_size;
  guint8    ltt_major_version;
  guint8    ltt_minor_version;
  guint8    flight_recorder;
  guint32   freq_scale;
  uint64_t  start_freq;
  uint64_t  start_tsc;
  uint64_t  start_monotonic;
  LttTime   start_time;
  LttTime   start_time_from_tsc;

  GData     *tracefiles;                    //tracefiles groups
};

static inline guint ltt_trace_get_num_cpu(LttTrace *t)
{
  return t->num_cpu;
}

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

static inline GQuark ltt_trace_name(const LttTrace *t)
{
  return t->pathname;
}


void ltt_trace_close(LttTrace *t); 

LttSystemDescription *ltt_trace_system_description(LttTrace *t);


/* Get the start time and end time of the trace */

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end);


/* Get the name of a tracefile */

static inline GQuark ltt_tracefile_name(const LttTracefile *tf)
{
  return tf->name;
}

static inline GQuark ltt_tracefile_long_name(const LttTracefile *tf)
{
  return tf->long_name;
}

/* get the cpu number of the tracefile */

static inline guint ltt_tracefile_cpu(LttTracefile *tf)
{
  return tf->cpu_num;
}

/* For usertrace */
static inline guint ltt_tracefile_tid(LttTracefile *tf)
{
  return tf->tid;
}

static inline guint ltt_tracefile_pgid(LttTracefile *tf)
{
  return tf->pgid;
}

static inline guint64 ltt_tracefile_creation(LttTracefile *tf)
{
  return tf->creation;
}

static inline LttTrace *ltt_tracefile_get_trace(LttTracefile *tf)
{
  return tf->trace;
}

/* Get the number of blocks in the tracefile */

static inline guint ltt_tracefile_block_number(LttTracefile *tf)
{
  return tf->num_blocks; 
}


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


/* get the NTP start time of the trace */

LttTime ltt_trace_start_time(LttTrace *t);

/* get the monotonic start time of the trace */

LttTime ltt_trace_start_time_monotonic(LttTrace *t);

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


gint64 ltt_get_int(gboolean reverse_byte_order, gint size, void *data);

guint64 ltt_get_uint(gboolean reverse_byte_order, gint size, void *data);

LttTime ltt_interpolate_time_from_tsc(LttTracefile *tf, guint64 tsc);

/* Set to enable event debugging output */
void ltt_event_debug(int state);

#endif // TRACE_H
