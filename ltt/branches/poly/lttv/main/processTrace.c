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


#include <lttv/processTrace.h>
#include <ltt/event.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/type.h>

void lttv_context_init(LttvTracesetContext *self, LttvTraceset *ts)
{
  LTTV_TRACESET_CONTEXT_GET_CLASS(self)->init(self, ts);
}


void lttv_context_fini(LttvTracesetContext *self)
{
  LTTV_TRACESET_CONTEXT_GET_CLASS(self)->fini(self);
}


LttvTracesetContext *
lttv_context_new_traceset_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_traceset_context(self);
}




LttvTraceContext * 
lttv_context_new_trace_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_trace_context(self);
}


LttvTracefileContext *
lttv_context_new_tracefile_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_tracefile_context(self);
}

/****************************************************************************
 * lttv_traceset_context_compute_time_span
 *
 * Keep the Time_Span is sync with on the fly addition and removal of traces
 * in a trace set. It must be called each time a trace is added/removed from
 * the traceset. It could be more efficient to call it only once a bunch
 * of traces are loaded, but the calculation is not long, so it's not
 * critical.
 *
 * Author : Xang Xiu Yang
 * Imported from gtkTraceSet.c by Mathieu Desnoyers
 ***************************************************************************/
static void lttv_traceset_context_compute_time_span(
                                          LttvTracesetContext *self,
					  TimeInterval *Time_Span)
{
  LttvTraceset * traceset = self->ts;
  int numTraces = lttv_traceset_number(traceset);
  int i;
  LttTime s, e;
  LttvTraceContext *tc;
  LttTrace * trace;

  Time_Span->startTime.tv_sec = 0;
  Time_Span->startTime.tv_nsec = 0;
  Time_Span->endTime.tv_sec = 0;
  Time_Span->endTime.tv_nsec = 0;
  
  for(i=0; i<numTraces;i++){
    tc = self->traces[i];
    trace = tc->t;

    ltt_trace_time_span_get(trace, &s, &e);

    if(i==0){
      Time_Span->startTime = s;
      Time_Span->endTime   = e;
    }else{
      if(s.tv_sec < Time_Span->startTime.tv_sec ||
	 (s.tv_sec == Time_Span->startTime.tv_sec 
          && s.tv_nsec < Time_Span->startTime.tv_nsec))
	Time_Span->startTime = s;
      if(e.tv_sec > Time_Span->endTime.tv_sec ||
	 (e.tv_sec == Time_Span->endTime.tv_sec &&
          e.tv_nsec > Time_Span->endTime.tv_nsec))
	Time_Span->endTime = e;      
    }
  }
}


static void
init(LttvTracesetContext *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  LttTime null_time = {0, 0};

  nb_trace = lttv_traceset_number(ts);
  self->ts = ts;
  self->traces = g_new(LttvTraceContext *, nb_trace);
  self->before = lttv_hooks_new();
  self->after = lttv_hooks_new();
  self->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  self->ts_a = lttv_traceset_attribute(ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_trace_context(self);
    self->traces[i] = tc;

    tc->ts_context = self;
    tc->index = i;
    tc->vt = lttv_traceset_get(ts, i);
    tc->t = lttv_trace(tc->vt);
    tc->check = lttv_hooks_new();
    tc->before = lttv_hooks_new();
    tc->after = lttv_hooks_new();
    tc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    tc->t_a = lttv_trace_attribute(tc->vt);
    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;
    tc->tracefiles = g_new(LttvTracefileContext *, nb_tracefile);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_tracefile_context(self);
      tc->tracefiles[j] = tfc;
      tfc->index = j;

      if(j < nb_control) {
        tfc->control = TRUE;
        tfc->tf = ltt_trace_control_tracefile_get(tc->t, j);
      }
      else {
        tfc->control = FALSE;
        tfc->tf = ltt_trace_per_cpu_tracefile_get(tc->t, j - nb_control);
      }
      tfc->t_context = tc;
      tfc->check = lttv_hooks_new();
      tfc->before = lttv_hooks_new();
      tfc->after = lttv_hooks_new();
      tfc->check_event = lttv_hooks_new();
      tfc->before_event = lttv_hooks_new();
      tfc->before_event_by_id = lttv_hooks_by_id_new();
      tfc->after_event = lttv_hooks_new();
      tfc->after_event_by_id = lttv_hooks_by_id_new();
      tfc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    }
  }
  lttv_process_traceset_seek_time(self, null_time);
  /*CHECK why dynamically allocate the time span... and the casing is wroNg*/
  self->Time_Span = g_new(TimeInterval,1);
  lttv_traceset_context_compute_time_span(self, self->Time_Span);
}


void fini(LttvTracesetContext *self)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  LttvTraceset *ts = self->ts;

  g_free(self->Time_Span);

  lttv_hooks_destroy(self->before);
  lttv_hooks_destroy(self->after);
  //FIXME : segfault
  g_object_unref(self->a);

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];

    lttv_hooks_destroy(tc->check);
    lttv_hooks_destroy(tc->before);
    lttv_hooks_destroy(tc->after);
    g_object_unref(tc->a);

    nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
        ltt_trace_per_cpu_tracefile_number(tc->t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = tc->tracefiles[j];
      lttv_hooks_destroy(tfc->check);
      lttv_hooks_destroy(tfc->before);
      lttv_hooks_destroy(tfc->after);
      lttv_hooks_destroy(tfc->check_event);
      lttv_hooks_destroy(tfc->before_event);
      lttv_hooks_by_id_destroy(tfc->before_event_by_id);
      lttv_hooks_destroy(tfc->after_event);
      lttv_hooks_by_id_destroy(tfc->after_event_by_id);
      g_object_unref(tfc->a);
      g_object_unref(tfc);
    }
    g_free(tc->tracefiles);
    g_object_unref(tc);
  }
  g_free(self->traces);
}


void lttv_traceset_context_add_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset, 
    LttvHooks *after_traceset,
    LttvHooks *check_trace, 
    LttvHooks *before_trace, 
    LttvHooks *after_trace, 
    LttvHooks *check_tracefile,
    LttvHooks *before_tracefile,
    LttvHooks *after_tracefile,
    LttvHooks *check_event, 
    LttvHooks *before_event, 
    LttvHooks *after_event)
{
  LttvTraceset *ts = self->ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  void *hook_data;

  lttv_hooks_add_list(self->before, before_traceset);
  lttv_hooks_add_list(self->after, after_traceset);
  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_hooks_add_list(tc->check, check_trace);
    lttv_hooks_add_list(tc->before, before_trace);
    lttv_hooks_add_list(tc->after, after_trace);
    nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
        ltt_trace_per_cpu_tracefile_number(tc->t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = tc->tracefiles[j];
      lttv_hooks_add_list(tfc->check, check_tracefile);
      lttv_hooks_add_list(tfc->before, before_tracefile);
      lttv_hooks_add_list(tfc->after, after_tracefile);
      lttv_hooks_add_list(tfc->check_event, check_event);
      lttv_hooks_add_list(tfc->before_event, before_event);
      lttv_hooks_add_list(tfc->after_event, after_event);
    }
  }
}


void lttv_traceset_context_remove_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset, 
    LttvHooks *after_traceset,
    LttvHooks *check_trace, 
    LttvHooks *before_trace, 
    LttvHooks *after_trace, 
    LttvHooks *check_tracefile,
    LttvHooks *before_tracefile,
    LttvHooks *after_tracefile,
    LttvHooks *check_event, 
    LttvHooks *before_event, 
    LttvHooks *after_event)
{
  LttvTraceset *ts = self->ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  void *hook_data;

  lttv_hooks_remove_list(self->before, before_traceset);
  lttv_hooks_remove_list(self->after, after_traceset);
  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_hooks_remove_list(tc->check, check_trace);
    lttv_hooks_remove_list(tc->before, before_trace);
    lttv_hooks_remove_list(tc->after, after_trace);
    nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
        ltt_trace_per_cpu_tracefile_number(tc->t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = tc->tracefiles[j];
      lttv_hooks_remove_list(tfc->check, check_tracefile);
      lttv_hooks_remove_list(tfc->before, before_tracefile);
      lttv_hooks_remove_list(tfc->after, after_tracefile);
      lttv_hooks_remove_list(tfc->check_event, check_event);
      lttv_hooks_remove_list(tfc->before_event, before_event);
      lttv_hooks_remove_list(tfc->after_event, after_event);
    }
  }
}

void lttv_trace_context_add_hooks(LttvTraceContext *tc,
				  LttvHooks *check_trace, 
				  LttvHooks *before_trace, 
				  LttvHooks *after_trace)
{
  lttv_hooks_add_list(tc->check, check_trace);
  lttv_hooks_add_list(tc->before, before_trace);
  lttv_hooks_add_list(tc->after, after_trace);
}

void lttv_trace_context_remove_hooks(LttvTraceContext *tc,
				     LttvHooks *check_trace, 
				     LttvHooks *before_trace, 
				     LttvHooks *after_trace)
{
  lttv_hooks_remove_list(tc->check, check_trace);
  lttv_hooks_remove_list(tc->before, before_trace);
  lttv_hooks_remove_list(tc->after, after_trace);
}

void lttv_tracefile_context_add_hooks(LttvTracefileContext *tfc,
				      LttvHooks *check_tracefile,
				      LttvHooks *before_tracefile,
				      LttvHooks *after_tracefile,
				      LttvHooks *check_event, 
				      LttvHooks *before_event, 
				      LttvHooks *after_event)
{
  lttv_hooks_add_list(tfc->check, check_tracefile);
  lttv_hooks_add_list(tfc->before, before_tracefile);
  lttv_hooks_add_list(tfc->after, after_tracefile);
  lttv_hooks_add_list(tfc->check_event, check_event);
  lttv_hooks_add_list(tfc->before_event, before_event);
  lttv_hooks_add_list(tfc->after_event, after_event);
}

void lttv_tracefile_context_remove_hooks(LttvTracefileContext *tfc,
					 LttvHooks *check_tracefile,
					 LttvHooks *before_tracefile,
					 LttvHooks *after_tracefile,
					 LttvHooks *check_event, 
					 LttvHooks *before_event, 
					 LttvHooks *after_event)
{
  lttv_hooks_remove_list(tfc->check, check_tracefile);
  lttv_hooks_remove_list(tfc->before, before_tracefile);
  lttv_hooks_remove_list(tfc->after, after_tracefile);
  lttv_hooks_remove_list(tfc->check_event, check_event);
  lttv_hooks_remove_list(tfc->before_event, before_event);
  lttv_hooks_remove_list(tfc->after_event, after_event);
}

void lttv_tracefile_context_add_hooks_by_id(LttvTracefileContext *tfc,
					    unsigned i,
					    LttvHooks *before_event_by_id, 
					    LttvHooks *after_event_by_id)
{
  LttvHooks * h;
  h = lttv_hooks_by_id_find(tfc->before_event_by_id, i);
  lttv_hooks_add_list(h, before_event_by_id);
  h = lttv_hooks_by_id_find(tfc->after_event_by_id, i);
  lttv_hooks_add_list(h, after_event_by_id);
}

void lttv_tracefile_context_remove_hooks_by_id(LttvTracefileContext *tfc,
					       unsigned i)
{
  lttv_hooks_by_id_remove(tfc->before_event_by_id, i);
  lttv_hooks_by_id_remove(tfc->after_event_by_id, i);
}

static LttvTracesetContext *
new_traceset_context(LttvTracesetContext *self)
{
  return g_object_new(LTTV_TRACESET_CONTEXT_TYPE, NULL);
}


static LttvTraceContext * 
new_trace_context(LttvTracesetContext *self)
{
  return g_object_new(LTTV_TRACE_CONTEXT_TYPE, NULL);
}


static LttvTracefileContext *
new_tracefile_context(LttvTracesetContext *self)
{
  return g_object_new(LTTV_TRACEFILE_CONTEXT_TYPE, NULL);
}


static void
traceset_context_instance_init (GTypeInstance *instance, gpointer g_class)
{
  /* Be careful of anything which would not work well with shallow copies */
}


static void
traceset_context_finalize (LttvTracesetContext *self)
{
  G_OBJECT_CLASS(g_type_class_peek(g_type_parent(LTTV_TRACESET_CONTEXT_TYPE)))
      ->finalize(G_OBJECT(self));
}


static void
traceset_context_class_init (LttvTracesetContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self))traceset_context_finalize;
  klass->init = init;
  klass->fini = fini;
  klass->new_traceset_context = new_traceset_context;
  klass->new_trace_context = new_trace_context;
  klass->new_tracefile_context = new_tracefile_context;
}


GType 
lttv_traceset_context_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracesetContextClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) traceset_context_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracesetContext),
      0,      /* n_preallocs */
      (GInstanceInitFunc) traceset_context_instance_init /* instance_init */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "LttvTracesetContextType", 
        &info, 0);
  }
  return type;
}


static void
trace_context_instance_init (GTypeInstance *instance, gpointer g_class)
{
  /* Be careful of anything which would not work well with shallow copies */
}


static void
trace_context_finalize (LttvTraceContext *self)
{
  G_OBJECT_CLASS(g_type_class_peek(g_type_parent(LTTV_TRACE_CONTEXT_TYPE)))->
      finalize(G_OBJECT(self));
}


static void
trace_context_class_init (LttvTraceContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) trace_context_finalize;
}


GType 
lttv_trace_context_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTraceContextClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) trace_context_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTraceContext),
      0,      /* n_preallocs */
      (GInstanceInitFunc) trace_context_instance_init    /* instance_init */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "LttvTraceContextType", 
        &info, 0);
  }
  return type;
}


static void
tracefile_context_instance_init (GTypeInstance *instance, gpointer g_class)
{
  /* Be careful of anything which would not work well with shallow copies */
}


static void
tracefile_context_finalize (LttvTracefileContext *self)
{
  G_OBJECT_CLASS(g_type_class_peek(g_type_parent(LTTV_TRACEFILE_CONTEXT_TYPE)))
      ->finalize(G_OBJECT(self));
}


static void
tracefile_context_class_init (LttvTracefileContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self))tracefile_context_finalize;
}


GType 
lttv_tracefile_context_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracefileContextClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) tracefile_context_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracefileContext),
      0,      /* n_preallocs */
      (GInstanceInitFunc) tracefile_context_instance_init    /* instance_init */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "LttvTracefileContextType", 
        &info, 0);
  }
  return type;
}


gint compare_tracefile(gconstpointer a, gconstpointer b)
{
  gint comparison;

  LttvTracefileContext *trace_a = (LttvTracefileContext *)a;

  LttvTracefileContext *trace_b = (LttvTracefileContext *)b;

  if(trace_a == trace_b) return 0;
  comparison = ltt_time_compare(trace_a->timestamp, trace_b->timestamp);
  if(comparison != 0) return comparison;
  if(trace_a->index < trace_b->index) return -1;
  else if(trace_a->index > trace_b->index) return 1;
  if(trace_a->t_context->index < trace_b->t_context->index) return -1;
  else if(trace_a->t_context->index > trace_b->t_context->index) return 1;
  g_assert(FALSE);
}


gboolean get_first(gpointer key, gpointer value, gpointer user_data) {
  *((LttvTracefileContext **)user_data) = (LttvTracefileContext *)value;
  return TRUE;
}


void lttv_process_traceset_begin(LttvTracesetContext *self, LttTime end)
{
  guint i, j, nbi, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  /* Call all before_traceset, before_trace, and before_tracefile hooks.
     For all qualifying tracefiles, seek to the start time, create a context,
     read one event and insert in the pqueue based on the event time. */

  lttv_hooks_call(self->before, self);
  nbi = lttv_traceset_number(self->ts);
  self->pqueue = g_tree_new(compare_tracefile);

  for(i = 0 ; i < nbi ; i++) {
    tc = self->traces[i];

    if(!lttv_hooks_call_check(tc->check, tc)) {
      lttv_hooks_call(tc->before, tc);
      nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
          ltt_trace_per_cpu_tracefile_number(tc->t);

      for(j = 0 ; j < nb_tracefile ; j++) {
        tfc = tc->tracefiles[j];

        if(!lttv_hooks_call_check(tfc->check, tfc)) {
          lttv_hooks_call(tfc->before, tfc);

          if(tfc->e != NULL) {
	    if(tfc->timestamp.tv_sec < end.tv_sec ||
	       (tfc->timestamp.tv_sec == end.tv_sec && 
               tfc->timestamp.tv_nsec <= end.tv_nsec)) {
	      g_tree_insert(self->pqueue, tfc, tfc);
	    }
          }
        }
      }
    }
  }
}


guint lttv_process_traceset_middle(LttvTracesetContext *self, LttTime end, 
    unsigned nb_events)
{
  GTree *pqueue = self->pqueue;

  guint id;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  LttEvent *event;

  unsigned count = 0;

  LttTime previous_timestamp = {0, 0};

  /* Get the next event from the pqueue, call its hooks, 
     reinsert in the pqueue the following event from the same tracefile 
     unless the tracefile is finished or the event is later than the 
     start time. */

  while(TRUE) {
    tfc = NULL;
    g_tree_foreach(pqueue, get_first, &tfc);
    if(tfc == NULL) return count;

    /* Have we reached the maximum number of events specified? However,
       continue for all the events with the same time stamp (CHECK?). Then,
       empty the queue and break from the loop. */

    if(count >= nb_events && 
        ltt_time_compare(tfc->timestamp, previous_timestamp) != 0) 
        return count;

    previous_timestamp = tfc->timestamp;


    /* Get the tracefile with an event for the smallest time found. If two
       or more tracefiles have events for the same time, hope that lookup
       and remove are consistent. */

    g_tree_remove(pqueue, tfc);
    count++;

    if(!lttv_hooks_call(tfc->check_event, tfc)) {
      id = ltt_event_eventtype_id(tfc->e);
      lttv_hooks_call(lttv_hooks_by_id_get(tfc->before_event_by_id, id), tfc);
      lttv_hooks_call(tfc->before_event, tfc);
      lttv_hooks_call(lttv_hooks_by_id_get(tfc->after_event_by_id, id), tfc);
      lttv_hooks_call(tfc->after_event, tfc);
    }

    event = ltt_tracefile_read(tfc->tf);
    if(event != NULL) {
      tfc->e = event;
      tfc->timestamp = ltt_event_time(event);
      if(tfc->timestamp.tv_sec < end.tv_sec ||
	 (tfc->timestamp.tv_sec == end.tv_sec && tfc->timestamp.tv_nsec <= end.tv_nsec))
	g_tree_insert(pqueue, tfc, tfc);
    }
  }
}


void lttv_process_traceset_end(LttvTracesetContext *self)
{
  guint i, j, nbi, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  /* Call all after_traceset, after_trace, and after_tracefile hooks. */

  nbi = lttv_traceset_number(self->ts);

  for(i = 0 ; i < nbi ; i++) {
    tc = self->traces[i];

    /* The check hooks are called again to avoid memorizing the results
       obtained at the beginning. CHECK if it poses a problem */

    if(!lttv_hooks_call_check(tc->check, tc)) {
      nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
          ltt_trace_per_cpu_tracefile_number(tc->t);

      for(j = 0 ; j < nb_tracefile ; j++) {
        tfc = tc->tracefiles[j];

        if(!lttv_hooks_call_check(tfc->check, tfc)) {
          lttv_hooks_call(tfc->after, tfc);
        }
      }
      lttv_hooks_call(tc->after, tc);
    }
  }
  lttv_hooks_call(self->after, self);

  /* Empty and free the pqueue */

  while(TRUE){
    tfc = NULL;
    g_tree_foreach(self->pqueue, get_first, &tfc);
    if(tfc == NULL) break;
    g_tree_remove(self->pqueue, &(tfc->timestamp));
  }
  g_tree_destroy(self->pqueue);
}


void lttv_process_traceset(LttvTracesetContext *self, LttTime end, 
    unsigned nb_events)
{
  lttv_process_traceset_begin(self, end);
  lttv_process_traceset_middle(self, end, nb_events);
  lttv_process_traceset_end(self);
}


void lttv_process_trace_seek_time(LttvTraceContext *self, LttTime start)
{
  guint i, nb_tracefile;

  LttvTracefileContext *tfc;

  LttEvent *event;

  nb_tracefile = ltt_trace_control_tracefile_number(self->t) +
      ltt_trace_per_cpu_tracefile_number(self->t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = self->tracefiles[i];
    ltt_tracefile_seek_time(tfc->tf, start);
    event = ltt_tracefile_read(tfc->tf);
    tfc->e = event;
    if(event != NULL) tfc->timestamp = ltt_event_time(event);
  }
}


void lttv_process_traceset_seek_time(LttvTracesetContext *self, LttTime start)
{
  guint i, nb_trace;

  LttvTraceContext *tc;

  nb_trace = lttv_traceset_number(self->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_process_trace_seek_time(tc, start);
  }
}


static LttField *
find_field(LttEventType *et, const char *field)
{
  LttType *t;

  LttField *f;

  guint i, nb;

  char *name;

  if(field == NULL) return NULL;

  f = ltt_eventtype_field(et);
  t = ltt_eventtype_type(et);
  g_assert(ltt_type_class(t) == LTT_STRUCT);
  nb = ltt_type_member_number(t);
  for(i = 0 ; i < nb ; i++) {
    ltt_type_member_type(t, i, &name);
    if(strcmp(name, field) == 0) break;
  }
  g_assert(i < nb);
  return ltt_field_member(f, i);
}


void 
lttv_trace_find_hook(LttTrace *t, char *facility, char *event_type, 
    char *field1, char *field2, char *field3, LttvHook h, LttvTraceHook *th)
{
  LttFacility *f;

  LttEventType *et;

  guint nb, pos, i;

  char *name;

  nb = ltt_trace_facility_find(t, facility, &pos);
  if(nb < 1) g_error("No %s facility", facility);
  f = ltt_trace_facility_get(t, pos);
  et = ltt_facility_eventtype_get_by_name(f, event_type);
  if(et == NULL) g_error("Event %s does not exist", event_type);

  th->h = h;
  th->id = ltt_eventtype_id(et);
  th->f1 = find_field(et, field1);
  th->f2 = find_field(et, field2);
  th->f3 = find_field(et, field3);
}


