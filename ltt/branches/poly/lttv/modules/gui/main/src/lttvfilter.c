/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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


#include <lttv/lttvfilter.h>
#include <stdio.h>
#include <ltt/trace.h>
#include <ltt/type.h>

struct _LttvTracesetSelector {
  char      * traceset_name;
  GPtrArray * traces;
};


struct _LttvTraceSelector {
  char      * trace_name;
  GPtrArray * tracefiles;
  GPtrArray * eventtypes;
  gboolean    selected;
};

struct _LttvTracefileSelector {
  char      * tracefile_name;
  GPtrArray * eventtypes;
  gboolean    selected;
};

struct _LttvEventtypeSelector {
  char      * eventtype_name;
  gboolean    selected;
};


LttvTracesetSelector *lttv_traceset_selector_new(char * name) 
{
  LttvTracesetSelector *s;

  s = g_new(LttvTracesetSelector, 1);
  if(name)
    s->traceset_name = g_strdup(name);
  else
    s->traceset_name = NULL;    
  s->traces = g_ptr_array_new();
  return s;
}

LttvTraceSelector *lttv_trace_selector_new(LttTrace *t) 
{
  LttvTraceSelector * trace;

  trace = g_new(LttvTraceSelector, 1);
  trace->trace_name = g_strdup(ltt_trace_name(t));
  trace->tracefiles = g_ptr_array_new();
  trace->eventtypes = g_ptr_array_new();
  trace->selected = TRUE;
  return trace;
}

LttvTracefileSelector *lttv_tracefile_selector_new(LttTracefile *t) 
{
  LttvTracefileSelector * tracefile;

  tracefile = g_new(LttvTracefileSelector, 1);
  tracefile->tracefile_name = g_strdup(ltt_tracefile_name(t));
  tracefile->eventtypes = g_ptr_array_new();
  tracefile->selected = TRUE;
  return tracefile;
}

LttvEventtypeSelector *lttv_eventtype_selector_new(LttEventType * et)
{
  LttvEventtypeSelector * ev;
  ev = g_new(LttvEventtypeSelector, 1);
  ev->eventtype_name = g_strdup(ltt_eventtype_name(et));
  ev->selected = TRUE;
  return ev;
}

void lttv_traceset_selector_destroy(LttvTracesetSelector *s) 
{
  int i;
  LttvTraceSelector * t;

  for(i=0;i<s->traces->len;i++){
    t = (LttvTraceSelector*)s->traces->pdata[i];
    lttv_trace_selector_destroy(t);
  }
  g_ptr_array_free(s->traces, TRUE);
  if(s->traceset_name) g_free(s->traceset_name);
  g_free(s);
}

void lttv_trace_selector_destroy(LttvTraceSelector *s) 
{
  int i;
  LttvTracefileSelector * t;
  LttvEventtypeSelector * e;

  for(i=0;i<s->tracefiles->len;i++){
    t = (LttvTracefileSelector*)s->tracefiles->pdata[i];
    lttv_tracefile_selector_destroy(t);
  }
  for(i=0;i<s->eventtypes->len;i++){
    e = (LttvEventtypeSelector*)s->eventtypes->pdata[i];
    lttv_eventtype_selector_destroy(e);
  }
  if(s->trace_name) g_free(s->trace_name);
  g_free(s);
}

void lttv_tracefile_selector_destroy(LttvTracefileSelector *t) 
{
  int i;
  LttvEventtypeSelector * e;

  for(i=0;i<t->eventtypes->len;i++){
    e = (LttvEventtypeSelector*)t->eventtypes->pdata[i];
    lttv_eventtype_selector_destroy(e);
  }

  if(t->tracefile_name) g_free(t->tracefile_name);
  g_free(t);
}

void lttv_eventtype_selector_destroy(LttvEventtypeSelector *e)
{
  if(e->eventtype_name) g_free(e->eventtype_name);
  free(e);
}

void lttv_traceset_selector_trace_add(LttvTracesetSelector *s, 
				      LttvTraceSelector *t) 
{
  g_ptr_array_add(s->traces, t);
}

void lttv_trace_selector_tracefile_add(LttvTraceSelector *s, 
				       LttvTracefileSelector *t) 
{
  g_ptr_array_add(s->tracefiles, t);
}

void lttv_trace_selector_eventtype_add(LttvTraceSelector *s, 
				       LttvEventtypeSelector *et)
{  
  g_ptr_array_add(s->eventtypes, et);
}

void lttv_tracefile_selector_eventtype_add(LttvTracefileSelector *s, 
					   LttvEventtypeSelector *et)
{
  g_ptr_array_add(s->eventtypes, et);
}

unsigned lttv_traceset_selector_trace_number(LttvTracesetSelector *s) 
{
  return s->traces->len;
}

unsigned lttv_trace_selector_tracefile_number(LttvTraceSelector *s) 
{
  return s->tracefiles->len;
}

unsigned lttv_trace_selector_eventtype_number(LttvTraceSelector *s)
{
  return s->eventtypes->len;
}

unsigned lttv_tracefile_selector_eventtype_number(LttvTracefileSelector *s)
{
  return s->eventtypes->len;
}

LttvTraceSelector *lttv_traceset_selector_trace_get(LttvTracesetSelector *s,
						    unsigned i) 
{
  g_assert(s->traces->len > i);
  return ((LttvTraceSelector *)s->traces->pdata[i]);
}

LttvTracefileSelector *lttv_trace_selector_tracefile_get(LttvTraceSelector *s,
							 unsigned i) 
{
  g_assert(s->tracefiles->len > i);
  return ((LttvTracefileSelector *)s->tracefiles->pdata[i]);
}

LttvEventtypeSelector *lttv_trace_selector_eventtype_get(LttvTraceSelector *s,
							 unsigned i)
{
  g_assert(s->eventtypes->len > i);
  return ((LttvEventtypeSelector *)s->eventtypes->pdata[i]);
}

LttvEventtypeSelector *lttv_tracefile_selector_eventtype_get(LttvTracefileSelector *s,
							     unsigned i)
{
  g_assert(s->eventtypes->len > i);
  return ((LttvEventtypeSelector *)s->eventtypes->pdata[i]);
}

void lttv_traceset_selector_trace_remove(LttvTracesetSelector *s, unsigned i) 
{
  g_assert(s->traces->len > i);
  g_ptr_array_remove_index(s->traces, i);
}

void lttv_trace_selector_tracefile_remove(LttvTraceSelector *s, unsigned i) 
{
  g_assert(s->tracefiles->len > i);
  g_ptr_array_remove_index(s->tracefiles, i);
}

void lttv_trace_selector_eventtype_remove(LttvTraceSelector *s, unsigned i)
{
  g_assert(s->eventtypes->len > i);
  g_ptr_array_remove_index(s->eventtypes, i);
}

void lttv_tracefile_selector_eventtype_remove(LttvTracefileSelector *s, unsigned i)
{
  g_assert(s->eventtypes->len > i);
  g_ptr_array_remove_index(s->eventtypes, i);
}

void lttv_trace_selector_set_selected(LttvTraceSelector *s, gboolean g)
{
  s->selected = g;
}

void lttv_tracefile_selector_set_selected(LttvTracefileSelector *s, gboolean g)
{
  s->selected = g;
}

void lttv_eventtype_selector_set_selected(LttvEventtypeSelector *s, gboolean g)
{
  s->selected = g;
}

gboolean lttv_trace_selector_get_selected(LttvTraceSelector *s)
{
  return s->selected;
}

gboolean lttv_tracefile_selector_get_selected(LttvTracefileSelector *s)
{
  return s->selected;
}

gboolean lttv_eventtype_selector_get_selected(LttvEventtypeSelector *s)
{
  return s->selected;
}

char * lttv_traceset_selector_get_name(LttvTracesetSelector *s)
{
  return s->traceset_name;
}

char * lttv_trace_selector_get_name(LttvTraceSelector *s)
{
  return s->trace_name;
}

char * lttv_tracefile_selector_get_name(LttvTracefileSelector *s)
{
  return s->tracefile_name;
}

char * lttv_eventtype_selector_get_name(LttvEventtypeSelector *s)
{
  return s->eventtype_name;
}

LttvEventtypeSelector * lttv_eventtype_selector_clone(LttvEventtypeSelector * s)
{
  LttvEventtypeSelector * ev = g_new(LttvEventtypeSelector, 1);
  ev->eventtype_name = g_strdup(s->eventtype_name);
  ev->selected = s->selected;
  return ev;
}

void lttv_eventtype_selector_copy(LttvTraceSelector * s, LttvTracefileSelector * d)
{
  int i, len;
  LttvEventtypeSelector * ev, *ev1;

  len = s->eventtypes->len;
  for(i=0;i<len;i++){
    ev = lttv_trace_selector_eventtype_get(s,i);
    ev1 = lttv_eventtype_selector_clone(ev);
    lttv_tracefile_selector_eventtype_add(d,ev1);
  }
}
