
#include <lttv/lttvfilter.h>
#include <stdio.h>
#include <ltt/trace.h>

struct _LttvTracesetSelector {
  char      * traceset_name;
  GPtrArray *traces;
};


struct _LttvTraceSelector {
  char      * trace_name;
  GPtrArray * tracefiles;
  gboolean    selected;
};

struct _LttvTracefileSelector {
  char      * tracefile_name;
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
  trace->selected = TRUE;
  return trace;
}

LttvTracefileSelector *lttv_tracefile_selector_new(LttTracefile *t) 
{
  LttvTracefileSelector * tracefile;

  tracefile = g_new(LttvTracefileSelector, 1);
  tracefile->tracefile_name = g_strdup(ltt_tracefile_name(t));
  tracefile->selected = TRUE;
  return tracefile;
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

  for(i=0;i<s->tracefiles->len;i++){
    t = (LttvTracefileSelector*)s->tracefiles->pdata[i];
    lttv_tracefile_selector_destroy(t);
  }
  if(s->trace_name) g_free(s->trace_name);
  g_free(s);
}

void lttv_tracefile_selector_destroy(LttvTracefileSelector *t) 
{
  if(t->tracefile_name) g_free(t->tracefile_name);
  g_free(t);
}

void lttv_traceset_selector_add(LttvTracesetSelector *s, LttvTraceSelector *t) 
{
  g_ptr_array_add(s->traces, t);
}

void lttv_trace_selector_add(LttvTraceSelector *s, LttvTracefileSelector *t) 
{
  g_ptr_array_add(s->tracefiles, t);
}


unsigned lttv_traceset_selector_number(LttvTracesetSelector *s) 
{
  return s->traces->len;
}

unsigned lttv_trace_selector_number(LttvTraceSelector *s) 
{
  return s->tracefiles->len;
}

LttvTraceSelector *lttv_traceset_selector_get(LttvTracesetSelector *s, unsigned i) 
{
  g_assert(s->traces->len > i);
  return ((LttvTraceSelector *)s->traces->pdata[i]);
}

LttvTracefileSelector *lttv_trace_selector_get(LttvTraceSelector *s, unsigned i) 
{
  g_assert(s->tracefiles->len > i);
  return ((LttvTracefileSelector *)s->tracefiles->pdata[i]);
}

void lttv_traceset_selector_remove(LttvTracesetSelector *s, unsigned i) 
{
  g_assert(s->traces->len > i);
  g_ptr_array_remove_index(s->traces, i);
}

void lttv_trace_selector_remove(LttvTraceSelector *s, unsigned i) 
{
  g_assert(s->tracefiles->len > i);
  g_ptr_array_remove_index(s->tracefiles, i);
}


void lttv_trace_selector_set_selected(LttvTraceSelector *s, gboolean g)
{
  s->selected = g;
}

void lttv_tracefile_selector_set_selected(LttvTracefileSelector *s, gboolean g)
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

char * lttv_trace_selector_get_name(LttvTraceSelector *s)
{
  return s->trace_name;
}

char * lttv_tracefile_selector_get_name(LttvTracefileSelector *s)
{
  return s->tracefile_name;
}
