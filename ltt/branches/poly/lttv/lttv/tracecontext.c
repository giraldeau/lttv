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


#include <lttv/tracecontext.h>
#include <ltt/event.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/type.h>




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

struct _LttvTraceContextPosition {
  LttEventPosition **tf_pos;         /* Position in each tracefile       */
  guint nb_tracefile;                /* Number of tracefiles (check)     */
};

struct _LttvTracesetContextPosition {
  LttvTraceContextPosition *t_pos;    /* Position in each trace          */
  guint nb_trace;                    /* Number of traces (check)         */
  LttTime timestamp;                 /* Current time at the saved position */
};

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
 * Keep the time span is sync with on the fly addition and removal of traces
 * in a trace set. It must be called each time a trace is added/removed from
 * the traceset. It could be more efficient to call it only once a bunch
 * of traces are loaded, but the calculation is not long, so it's not
 * critical.
 *
 * Author : Xang Xiu Yang
 ***************************************************************************/
static void lttv_traceset_context_compute_time_span(
                                          LttvTracesetContext *self,
	                              				  TimeInterval *time_span)
{
  LttvTraceset * traceset = self->ts;
  int numTraces = lttv_traceset_number(traceset);
  int i;
  LttTime s, e;
  LttvTraceContext *tc;
  LttTrace * trace;

  time_span->start_time.tv_sec = 0;
  time_span->start_time.tv_nsec = 0;
  time_span->end_time.tv_sec = 0;
  time_span->end_time.tv_nsec = 0;
  
  for(i=0; i<numTraces;i++){
    tc = self->traces[i];
    trace = tc->t;

    ltt_trace_time_span_get(trace, &s, &e);

    if(i==0){
      time_span->start_time = s;
      time_span->end_time   = e;
    }else{
      if(s.tv_sec < time_span->start_time.tv_sec 
          || (s.tv_sec == time_span->start_time.tv_sec 
               && s.tv_nsec < time_span->start_time.tv_nsec))
	      time_span->start_time = s;
      if(e.tv_sec > time_span->end_time.tv_sec
          || (e.tv_sec == time_span->end_time.tv_sec 
               && e.tv_nsec > time_span->end_time.tv_nsec))
        time_span->end_time = e;      
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
  self->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  self->ts_a = lttv_traceset_attribute(ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_trace_context(self);
    self->traces[i] = tc;

    tc->ts_context = self;
    tc->index = i;
    tc->vt = lttv_traceset_get(ts, i);
    tc->t = lttv_trace(tc->vt);
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
      tfc->event = lttv_hooks_new();
      tfc->event_by_id = lttv_hooks_by_id_new();
      tfc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    }
  }
  self->pqueue = g_tree_new(compare_tracefile);
  lttv_process_traceset_seek_time(self, null_time);
  lttv_traceset_context_compute_time_span(self, &self->time_span);

}


void fini(LttvTracesetContext *self)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  LttvTraceset *ts = self->ts;

  //FIXME : segfault

  g_tree_destroy(self->pqueue);
  g_object_unref(self->a);

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];

    g_object_unref(tc->a);

    nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
        ltt_trace_per_cpu_tracefile_number(tc->t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = tc->tracefiles[j];
      lttv_hooks_destroy(tfc->event);
      lttv_hooks_by_id_destroy(tfc->event_by_id);
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
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event,
    LttvHooksById *event_by_id)
{
  LttvTraceset *ts = self->ts;

  guint i, nb_trace;

  LttvTraceContext *tc;

  lttv_hooks_call(before_traceset, self);

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_trace_context_add_hooks(tc,
                                 before_trace,
                                 before_tracefile,
                                 event,
                                 event_by_id);
  }
}


void lttv_traceset_context_remove_hooks(LttvTracesetContext *self,
    LttvHooks *after_traceset,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id)
{

  LttvTraceset *ts = self->ts;

  guint i, nb_trace;

  LttvTraceContext *tc;

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_trace_context_remove_hooks(tc,
                                    after_trace,
                                    after_tracefile,
                                    event,
                                    event_by_id);
  }

  lttv_hooks_call(after_traceset, self);


}

void lttv_trace_context_add_hooks(LttvTraceContext *self,
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id)
{
  guint i, nb_tracefile;

  LttvTracefileContext *tfc;

  lttv_hooks_call(before_trace, self);
  nb_tracefile = ltt_trace_control_tracefile_number(self->t) +
      ltt_trace_per_cpu_tracefile_number(self->t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = self->tracefiles[i];
    lttv_tracefile_context_add_hooks(tfc,
                                     before_tracefile,
                                     event,
                                     event_by_id);
  }
}



void lttv_trace_context_remove_hooks(LttvTraceContext *self,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id)
{
  guint i, nb_tracefile;

  LttvTracefileContext *tfc;

  nb_tracefile = ltt_trace_control_tracefile_number(self->t) +
      ltt_trace_per_cpu_tracefile_number(self->t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = self->tracefiles[i];
    lttv_tracefile_context_remove_hooks(tfc,
                                        after_tracefile,
                                        event,
                                        event_by_id);
  }

  lttv_hooks_call(after_trace, self);
}

void lttv_tracefile_context_add_hooks(LttvTracefileContext *self,
          LttvHooks *before_tracefile,
          LttvHooks *event, 
          LttvHooksById *event_by_id)
{
  guint i;

  LttvHooks *hook;
  
  lttv_hooks_call(before_tracefile, self);
  lttv_hooks_add_list(self->event, event);
  if(event_by_id != NULL)
    for(i = 0; i < lttv_hooks_by_id_max_id(event_by_id); i++) {
      hook = lttv_hooks_by_id_get(self->event_by_id, i);
      if(hook != NULL)
        lttv_hooks_remove_list(hook, lttv_hooks_by_id_get(event_by_id, i));
    }

}

void lttv_tracefile_context_remove_hooks(LttvTracefileContext *self,
           LttvHooks *after_tracefile,
           LttvHooks *event, 
           LttvHooksById *event_by_id)
{
  guint i;

  LttvHooks *hook;
  

  lttv_hooks_remove_list(self->event, event);
  if(event_by_id != NULL)
    for(i = 0; i < lttv_hooks_by_id_max_id(event_by_id); i++) {
      hook = lttv_hooks_by_id_find(self->event_by_id, i);
      lttv_hooks_add_list(hook, lttv_hooks_by_id_get(event_by_id, i));
    }

  lttv_hooks_call(after_tracefile, self);
}



void lttv_tracefile_context_add_hooks_by_id(LttvTracefileContext *tfc,
					    unsigned i,
					    LttvHooks *event_by_id)
{
  LttvHooks * h;
  h = lttv_hooks_by_id_find(tfc->event_by_id, i);
  lttv_hooks_add_list(h, event_by_id);
}

void lttv_tracefile_context_remove_hooks_by_id(LttvTracefileContext *tfc,
					       unsigned i)
{
  lttv_hooks_by_id_remove(tfc->event_by_id, i);
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



gboolean get_first(gpointer key, gpointer value, gpointer user_data) {
  *((LttvTracefileContext **)user_data) = (LttvTracefileContext *)value;
  return TRUE;
}



void lttv_process_traceset_begin(LttvTracesetContext *self,
                                 LttvHooks       *before_traceset,
                                 LttvHooks       *before_trace,
                                 LttvHooks       *before_tracefile,
                                 LttvHooks       *event,
                                 LttvHooksById   *event_by_id)
{

  /* simply add hooks in context. _before hooks are called by add_hooks. */
  /* It calls all before_traceset, before_trace, and before_tracefile hooks. */
  lttv_traceset_context_add_hooks(self,
                                  before_traceset,
                                  before_trace,
                                  before_tracefile,
                                  event,
                                  event_by_id);
  
}

/* Note : a _middle must be preceded from a _seek or another middle */
guint lttv_process_traceset_middle(LttvTracesetContext *self,
                              LttTime end,
                              guint nb_events,
                              const LttvTracesetContextPosition *end_position)
{
  GTree *pqueue = self->pqueue;

  guint id;

  LttvTracefileContext *tfc;

  LttEvent *event;

  unsigned count = 0;

  gboolean last_ret = FALSE; /* return value of the last hook list called */

  /* Get the next event from the pqueue, call its hooks, 
     reinsert in the pqueue the following event from the same tracefile 
     unless the tracefile is finished or the event is later than the 
     end time. */

  while(TRUE) {
    tfc = NULL;
    g_tree_foreach(pqueue, get_first, &tfc);
    /* End of traceset : tfc is NULL */
    if(tfc == NULL)
    {
      return count;
    }

    /* Have we reached :
     * - the maximum number of events specified?
     * - the end position ?
     * - the end time ?
     * then the read is finished. We leave the queue in the same state and
     * break the loop.
     */

    if(last_ret == TRUE ||
       count >= nb_events ||
       (end_position!=NULL)?FALSE:lttv_traceset_context_ctx_pos_compare(self,
                                                           end_position) >= 0 ||
       ltt_time_compare(tfc->timestamp, end) >= 0)
    {
      return count;
    }

    /* Get the tracefile with an event for the smallest time found. If two
       or more tracefiles have events for the same time, hope that lookup
       and remove are consistent. */

    g_tree_remove(pqueue, tfc);
    count++;

    id = ltt_event_eventtype_id(tfc->e);
    last_ret = lttv_hooks_call_merge(tfc->event, tfc,
                        lttv_hooks_by_id_get(tfc->event_by_id, id), tfc);

    event = ltt_tracefile_read(tfc->tf);
    if(event != NULL) {
      tfc->e = event;
      tfc->timestamp = ltt_event_time(event);
	    g_tree_insert(pqueue, tfc, tfc);
    }
  }
}


void lttv_process_traceset_end(LttvTracesetContext *self,
                               LttvHooks           *after_traceset,
                               LttvHooks           *after_trace,
                               LttvHooks           *after_tracefile,
                               LttvHooks           *event,
                               LttvHooksById       *event_by_id)
{
  /* Remove hooks from context. _after hooks are called by remove_hooks. */
  /* It calls all after_traceset, after_trace, and after_tracefile hooks. */
  lttv_traceset_context_remove_hooks(self,
                                     after_traceset,
                                     after_trace,
                                     after_tracefile,
                                     event,
                                     event_by_id);
}

void lttv_process_trace_seek_time(LttvTraceContext *self, LttTime start)
{
  guint i, nb_tracefile;

  LttvTracefileContext *tfc;

  LttEvent *event;

  GTree *pqueue = self->ts_context->pqueue;

  nb_tracefile = ltt_trace_control_tracefile_number(self->t) +
      ltt_trace_per_cpu_tracefile_number(self->t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = self->tracefiles[i];
    ltt_tracefile_seek_time(tfc->tf, start);
    g_tree_remove(pqueue, tfc);
    event = ltt_tracefile_read(tfc->tf);
    tfc->e = event;
    if(event != NULL) {
      tfc->timestamp = ltt_event_time(event);
      g_tree_insert(pqueue, tfc, tfc);
    }
  }
}


void lttv_process_traceset_seek_time(LttvTracesetContext *self, LttTime start)
{
  guint i, nb_trace;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  nb_trace = lttv_traceset_number(self->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_process_trace_seek_time(tc, start);
  }
}


gboolean lttv_process_tracefile_seek_position(LttvTracefileContext *self, 
                                              const LttEventPosition *pos)
{
  LttvTracefileContext *tfc = self;

  LttEvent *event;

  GTree *pqueue = self->t_context->ts_context->pqueue;
  
  ltt_tracefile_seek_position(tfc->tf, pos);
  g_tree_remove(pqueue, tfc);
  event = ltt_tracefile_read(tfc->tf);
  tfc->e = event;
  if(event != NULL) {
    tfc->timestamp = ltt_event_time(event);
    g_tree_insert(pqueue, tfc, tfc);
  }


}

gboolean lttv_process_trace_seek_position(LttvTraceContext *self, 
                                        const LttvTraceContextPosition *pos)
{
  guint i, nb_tracefile;

  LttvTracefileContext *tfc;

  LttEvent *event;

  nb_tracefile = ltt_trace_control_tracefile_number(self->t) +
      ltt_trace_per_cpu_tracefile_number(self->t);

  if(nb_tracefile != pos->nb_tracefile)
    return FALSE; /* Error */

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = self->tracefiles[i];
    lttv_process_tracefile_seek_position(tfc, pos->tf_pos[i]);
  }

  return TRUE;
}



gboolean lttv_process_traceset_seek_position(LttvTracesetContext *self, 
                                        const LttvTracesetContextPosition *pos)
{
  guint i, nb_trace;
  gboolean sum_ret = TRUE;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  nb_trace = lttv_traceset_number(self->ts);
  
  if(nb_trace != pos->nb_trace)
    return FALSE; /* Error */

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    sum_ret = sum_ret && lttv_process_trace_seek_position(tc, &pos->t_pos[i]);
  }

  return sum_ret;
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


void lttv_traceset_context_position_save(const LttvTracesetContext *self,
                                    LttvTracesetContextPosition *pos)
{
  guint nb_trace, nb_tracefile;
  guint iter_trace, iter_tracefile;
  
  LttvTraceContext *tc;
  
  LttvTracefileContext *tfc;

  LttEvent *event;

  LttTime timestamp = self->time_span.end_time;

  pos->nb_trace = nb_trace = lttv_traceset_number(self->ts);
  pos->t_pos = g_new(LttvTraceContextPosition, nb_trace);
  
  for(iter_trace = 0 ; iter_trace < nb_trace ; iter_trace++) {
    tc = self->traces[iter_trace];
    pos->t_pos[iter_trace].nb_tracefile = nb_tracefile =
                    ltt_trace_control_tracefile_number(tc->t) +
                    ltt_trace_per_cpu_tracefile_number(tc->t);

    pos->t_pos[iter_trace].tf_pos = g_new(LttEventPosition*, nb_tracefile);
    for(iter_tracefile = 0; iter_tracefile < nb_tracefile; iter_tracefile++) {
      pos->t_pos[iter_trace].tf_pos[iter_tracefile] 
                                                = ltt_event_position_new();
      tfc = tc->tracefiles[iter_tracefile];
      event = tfc->e;
      ltt_event_position(event, 
                         pos->t_pos[iter_trace].tf_pos[iter_tracefile]);
      if(ltt_time_compare(tfc->timestamp, timestamp) < 0)
        timestamp = tfc->timestamp;
    }
  }
  pos->timestamp = timestamp;
}

void lttv_traceset_context_position_destroy(LttvTracesetContextPosition *pos)
{
  guint nb_trace, nb_tracefile;
  guint iter_trace, iter_tracefile;
  
  nb_trace = pos->nb_trace;
  
  for(iter_trace = 0 ; iter_trace < nb_trace ; iter_trace++) {
    for(iter_tracefile = 0; iter_tracefile < 
                        pos->t_pos[iter_trace].nb_tracefile;
                        iter_tracefile++) {
      
      g_free(pos->t_pos[iter_trace].tf_pos[iter_tracefile]);
    }
    g_free(pos->t_pos[iter_trace].tf_pos);
  }
  g_free(pos->t_pos);

}

gint lttv_traceset_context_ctx_pos_compare(const LttvTracesetContext *self,
                                        const LttvTracesetContextPosition *pos)
{
  guint nb_trace, nb_tracefile;
  guint iter_trace, iter_tracefile;
  gint ret;

  LttvTraceContext *tc;
  
  LttvTracefileContext *tfc;

  LttEvent *event;

  nb_trace = lttv_traceset_number(self->ts);

  if(pos->nb_trace != nb_trace)
    g_error("lttv_traceset_context_ctx_pos_compare : nb_trace does not match.");
  
  for(iter_trace = 0 ; iter_trace < nb_trace ; iter_trace++) {
    tc = self->traces[iter_trace];
    nb_tracefile =  ltt_trace_control_tracefile_number(tc->t) +
                    ltt_trace_per_cpu_tracefile_number(tc->t);

    if(pos->t_pos[iter_trace].nb_tracefile != nb_tracefile)
      g_error("lttv_traceset_context_ctx_pos_compare : nb_tracefile does not match.");

    for(iter_tracefile = 0; iter_tracefile < nb_tracefile; iter_tracefile++) {
      tfc = tc->tracefiles[iter_tracefile];
      event = tfc->e;
      if(
          ret =
            ltt_event_event_position_compare(event, 
                            pos->t_pos[iter_trace].tf_pos[iter_tracefile])
          != 0)
        return ret;
    }
  }
  return 0;
}


gint lttv_traceset_context_pos_pos_compare(
                                  const LttvTracesetContextPosition *pos1,
                                  const LttvTracesetContextPosition *pos2)
{
  guint nb_trace, nb_tracefile;
  guint iter_trace, iter_tracefile;
  
  gint ret;

  nb_trace = pos1->nb_trace;
  if(nb_trace != pos2->nb_trace)
    g_error("lttv_traceset_context_pos_pos_compare : nb_trace does not match.");

  for(iter_trace = 0 ; iter_trace < nb_trace ; iter_trace++) {

    nb_tracefile = pos1->t_pos[iter_trace].nb_tracefile;
    if(nb_tracefile != pos2->t_pos[iter_trace].nb_tracefile)
      g_error("lttv_traceset_context_ctx_pos_compare : nb_tracefile does not match.");

    for(iter_tracefile = 0; iter_tracefile < nb_tracefile; iter_tracefile++) {
      if(ret = 
          ltt_event_position_compare(
                pos1->t_pos[iter_trace].tf_pos[iter_tracefile],
                pos2->t_pos[iter_trace].tf_pos[iter_tracefile])
          != 0)
        return ret;
    }
  }
  return 0;
}


LttTime lttv_traceset_context_position_get_time(
                                  const LttvTracesetContextPosition *pos)
{
  return pos->timestamp;
}


LttvTracefileContext *lttv_traceset_context_get_current_tfc(LttvTracesetContext *self)
{
  GTree *pqueue = self->pqueue;
  LttvTracefileContext *tfc;

  g_tree_foreach(pqueue, get_first, &tfc);

  return tfc;
}
