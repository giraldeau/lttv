/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/traceset.h>
#include <lttv/iattribute.h>
#include <stdio.h>

/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

struct _LttvTraceset {
  char * filename;
  GPtrArray *traces;
  LttvAttribute *a;
};


struct _LttvTrace {
  LttTrace *t;
  LttvAttribute *a;
  guint ref_count;
};


LttvTraceset *lttv_traceset_new() 
{
  LttvTraceset *s;

  s = g_new(LttvTraceset, 1);
  s->filename = NULL;
  s->traces = g_ptr_array_new();
  s->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  return s;
}

char * lttv_traceset_name(LttvTraceset * s)
{
  return s->filename;
}

LttvTrace *lttv_trace_new(LttTrace *t) 
{
  LttvTrace *new_trace;

  new_trace = g_new(LttvTrace, 1);
  new_trace->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  new_trace->t = t;
  new_trace->ref_count = 0;
  return new_trace;
}


LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig) 
{
  guint i;
  LttvTraceset *s;
  LttvTrace * trace;

  s = g_new(LttvTraceset, 1);
  s->filename = NULL;
  s->traces = g_ptr_array_new();
  for(i=0;i<s_orig->traces->len;i++)
  {
    trace = g_ptr_array_index(s_orig->traces, i);
    trace->ref_count++;

    g_ptr_array_add(s->traces,
	                  trace);
  }
  s->a = LTTV_ATTRIBUTE(lttv_iattribute_deep_copy(LTTV_IATTRIBUTE(s_orig->a)));
  return s;
}


LttvTraceset *lttv_traceset_load(const gchar *filename)
{
  LttvTraceset *s = g_new(LttvTraceset,1);
  FILE *tf;
  
  s->filename = g_strdup(filename);
  tf = fopen(filename,"r");

  g_critical("NOT IMPLEMENTED : load traceset data from a XML file");
  
  fclose(tf);
  return s;
}

gint lttv_traceset_save(LttvTraceset *s)
{
  FILE *tf;

  tf = fopen(s->filename, "w");
  
  g_critical("NOT IMPLEMENTED : save traceset data in a XML file");

  fclose(tf);
  return 0;
}

void lttv_traceset_destroy(LttvTraceset *s) 
{
  guint i;

  for(i=0;i<s->traces->len;i++) {
    LttvTrace *trace = g_ptr_array_index(s->traces, i);
    lttv_trace_unref(trace);
    if(lttv_trace_get_ref_number(trace) == 0)
      lttv_trace_destroy(trace);
  }
  g_ptr_array_free(s->traces, TRUE);
  g_object_unref(s->a);
  g_free(s);
}

void lttv_trace_destroy(LttvTrace *t) 
{
  g_object_unref(t->a);
  g_free(t);
}


void lttv_traceset_add(LttvTraceset *s, LttvTrace *t) 
{
  t->ref_count++;
  g_ptr_array_add(s->traces, t);
}


unsigned lttv_traceset_number(LttvTraceset *s) 
{
  return s->traces->len;
}


LttvTrace *lttv_traceset_get(LttvTraceset *s, unsigned i) 
{
  g_assert(s->traces->len > i);
  return ((LttvTrace *)s->traces->pdata[i]);
}


void lttv_traceset_remove(LttvTraceset *s, unsigned i) 
{
  LttvTrace * t;
  g_assert(s->traces->len > i);
  t = (LttvTrace *)s->traces->pdata[i];
  t->ref_count--;
  g_ptr_array_remove_index(s->traces, i);
}


/* A set of attributes is attached to each trace set, trace and tracefile
   to store user defined data as needed. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s) 
{
  return s->a;
}


LttvAttribute *lttv_trace_attribute(LttvTrace *t)
{
  return t->a;
}


LttTrace *lttv_trace(LttvTrace *t)
{
  return t->t;
}

guint lttv_trace_get_ref_number(LttvTrace * t)
{
  return t->ref_count;
}

guint lttv_trace_ref(LttvTrace * t)
{
  t->ref_count++;
  
  return t->ref_count;
}

guint lttv_trace_unref(LttvTrace * t)
{
  if(likely(t->ref_count > 0))
    t->ref_count--;

  return t->ref_count;
}

