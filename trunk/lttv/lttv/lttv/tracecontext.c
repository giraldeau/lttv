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

#include <string.h>
#include <lttv/lttv.h>
#include <lttv/tracecontext.h>
#include <ltt/event.h>
#include <ltt/trace.h>
#include <lttv/filter.h>
#include <errno.h>

gint compare_tracefile(gconstpointer a, gconstpointer b)
{
  gint comparison = 0;

  const LttvTracefileContext *trace_a = (const LttvTracefileContext *)a;
  const LttvTracefileContext *trace_b = (const LttvTracefileContext *)b;

  if(likely(trace_a != trace_b)) {
    comparison = ltt_time_compare(trace_a->timestamp, trace_b->timestamp);
    if(unlikely(comparison == 0)) {
      if(trace_a->index < trace_b->index) comparison = -1;
      else if(trace_a->index > trace_b->index) comparison = 1;
      else if(trace_a->t_context->index < trace_b->t_context->index) 
        comparison = -1;
      else if(trace_a->t_context->index > trace_b->t_context->index)
        comparison = 1;
    }
  }
  return comparison;
}

typedef struct _LttvTracefileContextPosition {
  LttEventPosition *event;
  LttvTracefileContext *tfc;
  gboolean used; /* Tells if the tfc is at end of traceset position */
} LttvTracefileContextPosition;


struct _LttvTracesetContextPosition {
  GArray *tfcp;                      /* Array of LttvTracefileContextPosition */
  LttTime timestamp;                 /* Current time at the saved position */ 
                                     /* If ltt_time_infinite : no position is
                                      * set, else, a position is set (may be end
                                      * of trace, with ep->len == 0) */
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
    tc->time_span.start_time = s;
    tc->time_span.end_time = e;

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

static void init_tracefile_context(LttTracefile *tracefile,
                                    LttvTraceContext *tc)
{
  LttvTracefileContext *tfc;
  LttvTracesetContext *tsc = tc->ts_context;
  
  tfc = LTTV_TRACESET_CONTEXT_GET_CLASS(tsc)->new_tracefile_context(tsc);

  tfc->index = tc->tracefiles->len;
  tc->tracefiles = g_array_append_val(tc->tracefiles, tfc);

  tfc->tf = tracefile;

  tfc->t_context = tc;
  tfc->event = lttv_hooks_new();
  tfc->event_by_id = lttv_hooks_by_id_new();
  tfc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  tfc->target_pid = -1;
}


static void
init(LttvTracesetContext *self, LttvTraceset *ts)
{
  guint i, nb_trace;

  LttvTraceContext *tc;

  GData **tracefiles_groups;

  struct compute_tracefile_group_args args;

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
    tc->tracefiles = g_array_sized_new(FALSE, TRUE,
                        sizeof(LttvTracefileContext*), 10);

    tracefiles_groups = ltt_trace_get_tracefiles_groups(tc->t);
    if(tracefiles_groups != NULL) {
      args.func = (ForEachTraceFileFunc)init_tracefile_context;
      args.func_args = tc;

      g_datalist_foreach(tracefiles_groups, 
                            (GDataForeachFunc)compute_tracefile_group,
                            &args);
    }
      
#if 0
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
      tfc->e = ltt_event_new();
      tfc->event = lttv_hooks_new();
      tfc->event_by_id = lttv_hooks_by_id_new();
      tfc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    }
#endif //0

  }
  self->sync_position = lttv_traceset_context_position_new(self);
  self->pqueue = g_tree_new(compare_tracefile);
  lttv_process_traceset_seek_time(self, ltt_time_zero);
  lttv_traceset_context_compute_time_span(self, &self->time_span);

}


void fini(LttvTracesetContext *self)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext **tfc;

  LttvTraceset *ts = self->ts;

  g_tree_destroy(self->pqueue);
  g_object_unref(self->a);
  lttv_traceset_context_position_destroy(self->sync_position);

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];

    g_object_unref(tc->a);

    nb_tracefile = tc->tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = &g_array_index(tc->tracefiles, LttvTracefileContext*, j);
      lttv_hooks_destroy((*tfc)->event);
      lttv_hooks_by_id_destroy((*tfc)->event_by_id);
      g_object_unref((*tfc)->a);
      g_object_unref(*tfc);
    }
    g_array_free(tc->tracefiles, TRUE);
    g_object_unref(tc);
  }
  g_free(self->traces);
}


void lttv_traceset_context_add_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset,
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event,
    LttvHooksByIdChannelArray *event_by_id_channel)
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
                                 event_by_id_channel);
  }
}


void lttv_traceset_context_remove_hooks(LttvTracesetContext *self,
    LttvHooks *after_traceset,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksByIdChannelArray *event_by_id_channel)
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
                                    event_by_id_channel);
  }

  lttv_hooks_call(after_traceset, self);


}

void lttv_trace_context_add_hooks(LttvTraceContext *self,
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event, 
    LttvHooksByIdChannelArray *event_by_id_channel)
{
  guint i, j, nb_tracefile;
  LttvTracefileContext **tfc;
  LttTracefile *tf;

  lttv_hooks_call(before_trace, self);

  nb_tracefile = self->tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = &g_array_index(self->tracefiles, LttvTracefileContext*, i);
    tf = (*tfc)->tf;
    lttv_tracefile_context_add_hooks(*tfc,
                                     before_tracefile,
                                     event,
                                     NULL);
    if (event_by_id_channel) {
      for(j = 0; j < event_by_id_channel->array->len; j++) {
        LttvHooksByIdChannel *hooks = &g_array_index(event_by_id_channel->array,
                                                    LttvHooksByIdChannel, j);
        if (tf->name == hooks->channel)
          lttv_tracefile_context_add_hooks(*tfc,
                                       NULL,
                                       NULL,
                                       hooks->hooks_by_id);
      }
    }
  }
}



void lttv_trace_context_remove_hooks(LttvTraceContext *self,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksByIdChannelArray *event_by_id_channel)
{
  guint i, j, nb_tracefile;
  LttvTracefileContext **tfc;
  LttTracefile *tf;

  nb_tracefile = self->tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = &g_array_index(self->tracefiles, LttvTracefileContext*, i);
    tf = (*tfc)->tf;
    if (event_by_id_channel) {
      for(j = 0; j < event_by_id_channel->array->len; j++) {
        LttvHooksByIdChannel *hooks = &g_array_index(event_by_id_channel->array,
                                                    LttvHooksByIdChannel, j);
        if (tf->name == hooks->channel)
          lttv_tracefile_context_remove_hooks(*tfc,
                                       NULL,
                                       NULL,
                                       hooks->hooks_by_id);
      }
    }
    lttv_tracefile_context_remove_hooks(*tfc,
                                     after_tracefile,
                                     event,
                                     NULL);

  }

  lttv_hooks_call(after_trace, self);
}

void lttv_tracefile_context_add_hooks(LttvTracefileContext *self,
          LttvHooks *before_tracefile,
          LttvHooks *event, 
          LttvHooksById *event_by_id)
{
  guint i, index;
  LttvHooks *hook;
  
  lttv_hooks_call(before_tracefile, self);
  lttv_hooks_add_list(self->event, event);
  if(event_by_id != NULL) {
    for(i = 0; i < event_by_id->array->len; i++) {
      index = g_array_index(event_by_id->array, guint, i);
      hook = lttv_hooks_by_id_find(self->event_by_id, index);
      lttv_hooks_add_list(hook, lttv_hooks_by_id_get(event_by_id, index));
    }
  }
}

void lttv_tracefile_context_remove_hooks(LttvTracefileContext *self,
           LttvHooks *after_tracefile,
           LttvHooks *event, 
           LttvHooksById *event_by_id)
{
  guint i, index;

  LttvHooks *hook;
  
  lttv_hooks_remove_list(self->event, event);
  if(event_by_id != NULL) {
    for(i = 0; i < event_by_id->array->len; i++) {
      index = g_array_index(event_by_id->array, guint, i);
      hook = lttv_hooks_by_id_get(self->event_by_id, index);
      if(hook != NULL)
        lttv_hooks_remove_list(hook, lttv_hooks_by_id_get(event_by_id, index));
    }
  }

  lttv_hooks_call(after_tracefile, self);
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
      (GInstanceInitFunc) traceset_context_instance_init, /* instance_init */
      NULL    /* Value handling */
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
      (GInstanceInitFunc) trace_context_instance_init,    /* instance_init */
      NULL    /* Value handling */
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
      (GInstanceInitFunc) tracefile_context_instance_init,    /* instance_init */
      NULL    /* Value handling */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "LttvTracefileContextType", 
        &info, 0);
  }
  return type;
}



static gboolean get_first(gpointer key, gpointer value, gpointer user_data) {
  g_assert(key == value);
  *((LttvTracefileContext **)user_data) = (LttvTracefileContext *)value;
  return TRUE;
}

#ifdef DEBUG
// Test to see if pqueue is traversed in the right order.
static LttTime test_time;

static gboolean test_tree(gpointer key, gpointer value, gpointer user_data) {

  LttvTracefileContext *tfc = (LttvTracefileContext *)key;

  g_debug("Tracefile name %s, time %lu.%lu, tfi %u, ti %u",
      g_quark_to_string(ltt_tracefile_name(tfc->tf)),
      tfc->timestamp.tv_sec, tfc->timestamp.tv_nsec,
      tfc->index, tfc->t_context->index);
  
  if(user_data != NULL) {
    if(((LttvTracefileContext *)user_data) == (LttvTracefileContext *)value) {
      g_assert(compare_tracefile(user_data, value) == 0);
    } else
      g_assert(compare_tracefile(user_data, value) != 0);
  }
  g_assert(ltt_time_compare(test_time, tfc->timestamp) <= 0);
  test_time.tv_sec = tfc->timestamp.tv_sec;
  test_time.tv_nsec = tfc->timestamp.tv_nsec;

  
  //g_assert(((LttvTracefileContext *)user_data) != (LttvTracefileContext *)value);
  return FALSE;
}
#endif //DEBUG



void lttv_process_traceset_begin(LttvTracesetContext *self,
                                 LttvHooks       *before_traceset,
                                 LttvHooks       *before_trace,
                                 LttvHooks       *before_tracefile,
                                 LttvHooks       *event,
                                 LttvHooksByIdChannelArray *event_by_id_channel)
{

  /* simply add hooks in context. _before hooks are called by add_hooks. */
  /* It calls all before_traceset, before_trace, and before_tracefile hooks. */
  lttv_traceset_context_add_hooks(self,
                                  before_traceset,
                                  before_trace,
                                  before_tracefile,
                                  event,
                                  event_by_id_channel);
  
}

//enum read_state { LAST_NONE, LAST_OK, LAST_EMPTY };

/* Note : a _middle must be preceded from a _seek or another middle */
guint lttv_process_traceset_middle(LttvTracesetContext *self,
                              LttTime end,
                              gulong nb_events,
                              const LttvTracesetContextPosition *end_position)
{
  GTree *pqueue = self->pqueue;

  LttvTracefileContext *tfc;

  LttEvent *e;
  
  unsigned count = 0;

  guint read_ret;

  //enum read_state last_read_state = LAST_NONE;

  gint last_ret = 0; /* return value of the last hook list called */

  /* Get the next event from the pqueue, call its hooks, 
     reinsert in the pqueue the following event from the same tracefile 
     unless the tracefile is finished or the event is later than the 
     end time. */

  while(TRUE) {
    tfc = NULL;
    g_tree_foreach(pqueue, get_first, &tfc);
    /* End of traceset : tfc is NULL */
    if(unlikely(tfc == NULL))
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

    if(unlikely(last_ret == TRUE ||
                ((count >= nb_events) && (nb_events != G_MAXULONG)) ||
     (end_position!=NULL&&lttv_traceset_context_ctx_pos_compare(self,
                                                          end_position) == 0)||
       ltt_time_compare(end, tfc->timestamp) <= 0))
    {
      return count;
    }
    
    /* Get the tracefile with an event for the smallest time found. If two
       or more tracefiles have events for the same time, hope that lookup
       and remove are consistent. */
 
#ifdef DEBUG
    test_time.tv_sec = 0;
    test_time.tv_nsec = 0;
    g_debug("test tree before remove");
    g_tree_foreach(pqueue, test_tree, tfc);
#endif //DEBUG
    g_tree_remove(pqueue, tfc);

#ifdef DEBUG
    test_time.tv_sec = 0;
    test_time.tv_nsec = 0;
    g_debug("test tree after remove");
    g_tree_foreach(pqueue, test_tree, tfc);
#endif //DEBUG


    e = ltt_tracefile_get_event(tfc->tf);

    //if(last_read_state != LAST_EMPTY) {
    /* Only call hooks if the last read has given an event or if we are at the
     * first pass (not if last read returned end of tracefile) */
    count++;
    
    tfc->target_pid = -1; /* unset target PID */
    /* Hooks : 
     * return values : 0 : continue read, 1 : go to next position and stop read,
     * 2 : stay at the current position and stop read */
    last_ret = lttv_hooks_call_merge(tfc->event, tfc,
                        lttv_hooks_by_id_get(tfc->event_by_id, e->event_id), tfc);

#if 0
    /* This is buggy : it won't work well with state computation */
   if(unlikely(last_ret == 2)) {
      /* This is a case where we want to stay at this position and stop read. */
	    g_tree_insert(pqueue, tfc, tfc);
      return count - 1;
    }
#endif //0
    read_ret = ltt_tracefile_read(tfc->tf);
    
   
    if(likely(!read_ret)) {
      //g_debug("An event is ready");
      tfc->timestamp = ltt_event_time(e);
      g_assert(ltt_time_compare(tfc->timestamp, ltt_time_infinite) != 0);
	    g_tree_insert(pqueue, tfc, tfc);
#ifdef DEBUG
      test_time.tv_sec = 0;
      test_time.tv_nsec = 0;
      g_debug("test tree after event ready");
      g_tree_foreach(pqueue, test_tree, NULL);
#endif //DEBUG

      //last_read_state = LAST_OK;
    } else {
      tfc->timestamp = ltt_time_infinite;

      if(read_ret == ERANGE) {
      //  last_read_state = LAST_EMPTY;
        g_debug("End of trace");
      } else
        g_error("Error happened in lttv_process_traceset_middle");
    }
  }
}


void lttv_process_traceset_end(LttvTracesetContext *self,
                               LttvHooks           *after_traceset,
                               LttvHooks           *after_trace,
                               LttvHooks           *after_tracefile,
                               LttvHooks           *event,
                               LttvHooksByIdChannelArray *event_by_id_channel)
{
  /* Remove hooks from context. _after hooks are called by remove_hooks. */
  /* It calls all after_traceset, after_trace, and after_tracefile hooks. */
  lttv_traceset_context_remove_hooks(self,
                                     after_traceset,
                                     after_trace,
                                     after_tracefile,
                                     event,
                                     event_by_id_channel);
}

/* Subtile modification : 
 * if tracefile has no event at or after the time requested, it is not put in
 * the queue, as the next read would fail.
 *
 * Don't forget to empty the traceset pqueue before calling this.
 */
void lttv_process_trace_seek_time(LttvTraceContext *self, LttTime start)
{
  guint i, nb_tracefile;

  gint ret;
  
  LttvTracefileContext **tfc;

  nb_tracefile = self->tracefiles->len;

  GTree *pqueue = self->ts_context->pqueue;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfc = &g_array_index(self->tracefiles, LttvTracefileContext*, i);

    g_tree_remove(pqueue, *tfc);
    
    ret = ltt_tracefile_seek_time((*tfc)->tf, start);
    if(ret == EPERM) g_error("error in lttv_process_trace_seek_time seek");

    if(ret == 0) { /* not ERANGE especially */
      (*tfc)->timestamp = ltt_event_time(ltt_tracefile_get_event((*tfc)->tf));
      g_assert(ltt_time_compare((*tfc)->timestamp, ltt_time_infinite) != 0);
      g_tree_insert(pqueue, (*tfc), (*tfc));
    } else {
      (*tfc)->timestamp = ltt_time_infinite;
    }
  }
#ifdef DEBUG
  test_time.tv_sec = 0;
  test_time.tv_nsec = 0;
  g_debug("test tree after seek_time");
  g_tree_foreach(pqueue, test_tree, NULL);
#endif //DEBUG
}


void lttv_process_traceset_seek_time(LttvTracesetContext *self, LttTime start)
{
  guint i, nb_trace;

  LttvTraceContext *tc;

  //g_tree_destroy(self->pqueue);
  //self->pqueue = g_tree_new(compare_tracefile);

  nb_trace = lttv_traceset_number(self->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];
    lttv_process_trace_seek_time(tc, start);
  }
}


gboolean lttv_process_traceset_seek_position(LttvTracesetContext *self, 
                                        const LttvTracesetContextPosition *pos)
{
  guint i;
   /* If a position is set, seek the traceset to this position */
  if(ltt_time_compare(pos->timestamp, ltt_time_infinite) != 0) {

    /* Test to see if the traces has been added to the trace set :
     * It should NEVER happen. Clear all positions if a new trace comes in. */
    /* FIXME I know this test is not optimal : should keep a number of
     * tracefiles variable in the traceset.. eventually */
    guint num_traces = lttv_traceset_number(self->ts);
    guint tf_count = 0;
    for(i=0; i<num_traces;i++) {
      GArray * tracefiles = self->traces[i]->tracefiles;
      guint j;
      guint num_tracefiles = tracefiles->len;
      for(j=0;j<num_tracefiles;j++)
        tf_count++;
    }
    g_assert(tf_count == pos->tfcp->len);
     

    //g_tree_destroy(self->pqueue);
    //self->pqueue = g_tree_new(compare_tracefile);
    
    for(i=0;i<pos->tfcp->len; i++) {
      LttvTracefileContextPosition *tfcp = 
        &g_array_index(pos->tfcp, LttvTracefileContextPosition, i);

      g_tree_remove(self->pqueue, tfcp->tfc);
      
      if(tfcp->used == TRUE) {
        if(ltt_tracefile_seek_position(tfcp->tfc->tf, tfcp->event) != 0)
          return 1;
        tfcp->tfc->timestamp =
          ltt_event_time(ltt_tracefile_get_event(tfcp->tfc->tf));
        g_assert(ltt_time_compare(tfcp->tfc->timestamp,
                                  ltt_time_infinite) != 0);
        g_tree_insert(self->pqueue, tfcp->tfc, tfcp->tfc);

      } else {
        tfcp->tfc->timestamp = ltt_time_infinite;
      }
    }
  }
#ifdef DEBUG
  test_time.tv_sec = 0;
  test_time.tv_nsec = 0;
  g_debug("test tree after seek_position");
  g_tree_foreach(self->pqueue, test_tree, NULL);
#endif //DEBUG



  return 0;
}


#if 0 // pmf: temporary disable
static LttField *
find_field(LttEventType *et, const GQuark field)
{
  LttField *f;

  if(field == 0) return NULL;
  
  f = ltt_eventtype_field_by_name(et, field);
  if (!f) {
    g_warning("Cannot find field %s in event %s.%s", g_quark_to_string(field),
    	g_quark_to_string(ltt_facility_name(ltt_eventtype_facility(et))),
    	g_quark_to_string(ltt_eventtype_name(et)));
  }

  return f;
}
#endif

struct marker_info *lttv_trace_hook_get_marker(LttTrace *t, LttvTraceHook *th)
{
  return marker_get_info_from_id(th->mdata, th->id);
}

int lttv_trace_find_hook(LttTrace *t, GQuark channel_name, GQuark event_name,
    GQuark fields[], LttvHook h, gpointer hook_data, GArray **trace_hooks)
{
  struct marker_info *info;
  guint16 marker_id;
  int init_array_size;
  GArray *group;
  struct marker_data *mdata;

  group = g_datalist_id_get_data(&t->tracefiles, channel_name);
  if (unlikely(!group || group->len == 0)) {
    g_warning("No channel for marker named %s.%s found",
    g_quark_to_string(channel_name), g_quark_to_string(event_name));
    return 1;
  }

  mdata = g_array_index (group, LttTracefile, 0).mdata;
  info = marker_get_info_from_name(mdata, event_name);
  if(unlikely(info == NULL)) {
    g_warning("No marker named %s.%s found",
    g_quark_to_string(channel_name), g_quark_to_string(event_name));
    return 1;
  }

  init_array_size = (*trace_hooks)->len;

  /* for each marker with the requested name */
  do {
    LttvTraceHook tmpth;
    int found;
    GQuark *f;
    struct marker_field *marker_field;

    marker_id = marker_get_id_from_info(mdata, info);

    tmpth.h = h;
    tmpth.mdata = mdata;
    tmpth.channel = channel_name;
    tmpth.id = marker_id;
    tmpth.hook_data = hook_data;
    tmpth.fields = g_ptr_array_new();

    /* for each field requested */
    for(f = fields; f && *f != 0; f++) {
      found = 0;
      for_each_marker_field(marker_field, info) {
        if(marker_field->name == *f) {
          found = 1;
          g_ptr_array_add(tmpth.fields, marker_field);
          break;
        }
      }
      if(!found) {
        /* Did not find the one of the fields in this instance of the
           marker. Print a warning and skip this marker completely.
	   Still iterate on other markers with same name. */
        g_ptr_array_free(tmpth.fields, TRUE);
        g_warning("Field %s cannot be found in marker %s.%s",
                g_quark_to_string(*f), g_quark_to_string(channel_name),
                                       g_quark_to_string(event_name));
        goto skip_marker;
      }
    }
    /* all fields were found: add the tracehook to the array */
    *trace_hooks = g_array_append_val(*trace_hooks, tmpth);
skip_marker:
    info = info->next;
  } while(info != NULL);

  /* Error if no new trace hook has been added */
  if (init_array_size == (*trace_hooks)->len) {
        g_warning("No marker of name %s.%s has all requested fields",
                g_quark_to_string(channel_name), g_quark_to_string(event_name));
        return 1;
  }
  return 0;
}

void lttv_trace_hook_remove_all(GArray **th)
{
  int i;
  for(i=0; i<(*th)->len; i++) {
    g_ptr_array_free(g_array_index(*th, LttvTraceHook, i).fields, TRUE);
  }
  if((*th)->len)
    *th = g_array_remove_range(*th, 0, (*th)->len);
}

LttvTracesetContextPosition *lttv_traceset_context_position_new(
                                        const LttvTracesetContext *self)
{
  guint num_traces = lttv_traceset_number(self->ts);
  guint tf_count = 0;
  guint i;
  
  for(i=0; i<num_traces;i++) {
    GArray * tracefiles = self->traces[i]->tracefiles;
    guint j;
    guint num_tracefiles = tracefiles->len;
    for(j=0;j<num_tracefiles;j++)
      tf_count++;
  }
  LttvTracesetContextPosition *pos =
          g_new(LttvTracesetContextPosition, 1);
  pos->tfcp = g_array_sized_new(FALSE, TRUE,
                                sizeof(LttvTracefileContextPosition),
                                tf_count);
  g_array_set_size(pos->tfcp, tf_count);
  for(i=0;i<pos->tfcp->len;i++) {
    LttvTracefileContextPosition *tfcp = 
      &g_array_index(pos->tfcp, LttvTracefileContextPosition, i);
    tfcp->event = ltt_event_position_new();
  }

  pos->timestamp = ltt_time_infinite;
  return pos;
}

/* Save all positions, the ones with infinite time will have NULL
 * ep. */
/* note : a position must be destroyed when a trace is added/removed from a
 * traceset */
void lttv_traceset_context_position_save(const LttvTracesetContext *self,
                                    LttvTracesetContextPosition *pos)
{
  guint i;
  guint num_traces = lttv_traceset_number(self->ts);
  guint tf_count = 0;
  
  pos->timestamp = ltt_time_infinite;
  
  for(i=0; i<num_traces;i++) {
    GArray * tracefiles = self->traces[i]->tracefiles;
    guint j;
    guint num_tracefiles = tracefiles->len;

    for(j=0;j<num_tracefiles;j++) {
      g_assert(tf_count < pos->tfcp->len);
      LttvTracefileContext **tfc = &g_array_index(tracefiles,
          LttvTracefileContext*, j);
      LttvTracefileContextPosition *tfcp = 
        &g_array_index(pos->tfcp, LttvTracefileContextPosition, tf_count);

      tfcp->tfc = *tfc;

      if(ltt_time_compare((*tfc)->timestamp, ltt_time_infinite) != 0) {
        LttEvent *event = ltt_tracefile_get_event((*tfc)->tf);
        ltt_event_position(event, tfcp->event);
        if(ltt_time_compare((*tfc)->timestamp, pos->timestamp) < 0)
          pos->timestamp = (*tfc)->timestamp;
        tfcp->used = TRUE;
      } else {
        tfcp->used = FALSE;
      }
      
      //g_array_append_val(pos->tfc, *tfc);
      //g_array_append_val(pos->ep, ep);
      tf_count++;
    }

  }
}

void lttv_traceset_context_position_destroy(LttvTracesetContextPosition *pos)
{
  int i;
  
  for(i=0;i<pos->tfcp->len;i++) {
    LttvTracefileContextPosition *tfcp = 
      &g_array_index(pos->tfcp, LttvTracefileContextPosition, i);
    g_free(tfcp->event);
    tfcp->event = NULL;
    tfcp->used = FALSE;
  }
  g_array_free(pos->tfcp, TRUE);
  g_free(pos);
}

void lttv_traceset_context_position_copy(LttvTracesetContextPosition *dest,
                                   const LttvTracesetContextPosition *src)
{
  int i;
  LttvTracefileContextPosition *src_tfcp, *dest_tfcp;
  
  g_assert(src->tfcp->len == src->tfcp->len);
  
  for(i=0;i<src->tfcp->len;i++) {
    src_tfcp = 
      &g_array_index(src->tfcp, LttvTracefileContextPosition, i);
    dest_tfcp = 
      &g_array_index(dest->tfcp, LttvTracefileContextPosition, i);
    
    dest_tfcp->used = src_tfcp->used;
    dest_tfcp->tfc = src_tfcp->tfc;

    if(src_tfcp->used) {
      ltt_event_position_copy(
          dest_tfcp->event,
          src_tfcp->event);
    }
  }
  dest->timestamp = src->timestamp;
}

gint lttv_traceset_context_ctx_pos_compare(const LttvTracesetContext *self,
                                        const LttvTracesetContextPosition *pos)
{
  int i;
  int ret = 0;
  
  if(pos->tfcp->len == 0) {
    if(lttv_traceset_number(self->ts) == 0) return 0;
    else return 1;
  }
  if(lttv_traceset_number(self->ts) == 0)
    return -1;
  
  for(i=0;i<pos->tfcp->len;i++) {
    LttvTracefileContextPosition *tfcp = 
      &g_array_index(pos->tfcp, LttvTracefileContextPosition, i);
    
    if(tfcp->used == FALSE) {
      if(ltt_time_compare(tfcp->tfc->timestamp, ltt_time_infinite) < 0) {
        ret = -1;
      }
    } else {
      if(ltt_time_compare(tfcp->tfc->timestamp, ltt_time_infinite) == 0) {
        ret = 1;
      } else {
        LttEvent *event = ltt_tracefile_get_event(tfcp->tfc->tf);

        ret = ltt_event_position_compare((LttEventPosition*)event, 
                                          tfcp->event);
      }
    }
    if(ret != 0) return ret;

  }
  return 0;
}


gint lttv_traceset_context_pos_pos_compare(
                                  const LttvTracesetContextPosition *pos1,
                                  const LttvTracesetContextPosition *pos2)
{
  int i, j;
  int ret = 0;
  
  if(ltt_time_compare(pos1->timestamp, ltt_time_infinite) == 0) {
    if(ltt_time_compare(pos2->timestamp, ltt_time_infinite) == 0)
      return 0;
    else 
      return 1;
  }
  if(ltt_time_compare(pos2->timestamp, ltt_time_infinite) == 0)
    return -1;
  
  for(i=0;i<pos1->tfcp->len;i++) {
    LttvTracefileContextPosition *tfcp1 = 
      &g_array_index(pos1->tfcp, LttvTracefileContextPosition, i);
    
    if(tfcp1->used == TRUE) {
      for(j=0;j<pos2->tfcp->len;j++) {
        LttvTracefileContextPosition *tfcp2 = 
          &g_array_index(pos2->tfcp, LttvTracefileContextPosition, j);

        if(tfcp1->tfc == tfcp2->tfc) {
          if(tfcp2->used == TRUE)
            ret = ltt_event_position_compare(tfcp1->event, tfcp2->event);
          else
            ret = -1;

          if(ret != 0) return ret;
        }
      }

    } else {
      for(j=0;j<pos2->tfcp->len;j++) {
        LttvTracefileContextPosition *tfcp2 = 
          &g_array_index(pos2->tfcp, LttvTracefileContextPosition, j);

        if(tfcp1->tfc == tfcp2->tfc)
          if(tfcp2->used == TRUE) ret = 1;
        if(ret != 0) return ret;
      }
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
  LttvTracefileContext *tfc = NULL;

  g_tree_foreach(pqueue, get_first, &tfc);

  return tfc;
}

/* lttv_process_traceset_synchronize_tracefiles
 *
 * Use the sync_position field of the trace set context to synchronize each
 * tracefile with the previously saved position.
 *
 * If no previous position has been saved, it simply does nothing.
 */
void lttv_process_traceset_synchronize_tracefiles(LttvTracesetContext *tsc)
{
  g_assert(lttv_process_traceset_seek_position(tsc, tsc->sync_position) == 0);
}




void lttv_process_traceset_get_sync_data(LttvTracesetContext *tsc)
{
  lttv_traceset_context_position_save(tsc, tsc->sync_position);
}

struct seek_back_data {
  guint first_event;   /* Index of the first event in the array : we will always
                         overwrite at this position : this is a circular array. 
                       */
  guint events_found;
  guint n;             /* number of events requested */
  GPtrArray *array; /* array of LttvTracesetContextPositions pointers */
  LttvFilter *filter1;
  LttvFilter *filter2;
  LttvFilter *filter3;
  gpointer data;
  check_handler *check;
  gboolean *stop_flag;
  guint raw_event_count;
};

static gint seek_back_event_hook(void *hook_data, void* call_data)
{
  struct seek_back_data *sd = (struct seek_back_data*)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext*)call_data;
  LttvTracesetContext *tsc = tfc->t_context->ts_context;
  LttvTracesetContextPosition *pos;

  if(sd->check && sd->check(sd->raw_event_count, sd->stop_flag, sd->data))
    return TRUE;
  sd->raw_event_count++;

  if(sd->filter1 != NULL && sd->filter1->head != NULL && 
    !lttv_filter_tree_parse(sd->filter1->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }
  if(sd->filter2 != NULL && sd->filter2->head != NULL && 
    !lttv_filter_tree_parse(sd->filter2->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }
  if(sd->filter3 != NULL && sd->filter3->head != NULL && 
    !lttv_filter_tree_parse(sd->filter3->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }

  pos = (LttvTracesetContextPosition*)g_ptr_array_index (sd->array,
                                                         sd->first_event);

  lttv_traceset_context_position_save(tsc, pos);

  if(sd->first_event >= sd->array->len - 1) sd->first_event = 0;
  else sd->first_event++;

  sd->events_found = min(sd->n, sd->events_found + 1);

  return FALSE;
}

/* Seek back n events back from the current position.
 *
 * Parameters :
 * @self          The trace set context
 * @n             number of events to jump over
 * @first_offset  The initial offset value used.
 *                never put first_offset at ltt_time_zero.
 * @time_seeker   Function pointer of the function to use to seek time :
 *                either lttv_process_traceset_seek_time
 *                    or lttv_state_traceset_seek_time_closest
 * @filter        The filter to call.
 *
 * Return value : the number of events found (might be lower than the number
 * requested if beginning of traceset is reached).
 *
 * The first search will go back first_offset and try to find the last n events
 * matching the filter. If there are not enough, it will try to go back from the
 * new trace point from first_offset*2, and so on, until beginning of trace or n
 * events are found.
 *
 * Note : this function does not take in account the LttvFilter : use the
 * similar function found in state.c instead.
 *
 * Note2 : the caller must make sure that the LttvTracesetContext does not
 * contain any hook, as process_traceset_middle is used in this routine.
 */
guint lttv_process_traceset_seek_n_backward(LttvTracesetContext *self,
                                            guint n, LttTime first_offset,
                                            seek_time_fct time_seeker,
                                            check_handler *check,
                                            gboolean *stop_flag,
					    LttvFilter *filter1,
					    LttvFilter *filter2,
					    LttvFilter *filter3,
					    gpointer data)
{
  if(lttv_traceset_number(self->ts) == 0) return 0;
  g_assert(ltt_time_compare(first_offset, ltt_time_zero) != 0);
  
  guint i;
  LttvTracesetContextPosition *next_iter_end_pos =
                                lttv_traceset_context_position_new(self);
  LttvTracesetContextPosition *end_pos =
    lttv_traceset_context_position_new(self);
  LttvTracesetContextPosition *saved_pos =
    lttv_traceset_context_position_new(self);
  LttTime time;
  LttTime asked_time;
  LttTime time_offset;
  struct seek_back_data sd;
  LttvHooks *hooks = lttv_hooks_new();
  
  sd.first_event = 0;
  sd.events_found = 0;
  sd.array = g_ptr_array_sized_new(n);
  sd.filter1 = filter1;
  sd.filter2 = filter2;
  sd.filter3 = filter3;
  sd.data = data;
  sd.n = n;
  sd.check = check;
  sd.stop_flag = stop_flag;
  sd.raw_event_count = 0;
  g_ptr_array_set_size(sd.array, n);
  for(i=0;i<n;i++) {
    g_ptr_array_index (sd.array, i) = lttv_traceset_context_position_new(self);
  }
 
  lttv_traceset_context_position_save(self, next_iter_end_pos);
  lttv_traceset_context_position_save(self, saved_pos);
  /* Get the current time from which we will offset */
  time = lttv_traceset_context_position_get_time(next_iter_end_pos);
  /* the position saved might be end of traceset... */
  if(ltt_time_compare(time, self->time_span.end_time) > 0) {
    time = self->time_span.end_time;
  }
  asked_time = time;
  time_offset = first_offset;
 
  lttv_hooks_add(hooks, seek_back_event_hook, &sd, LTTV_PRIO_DEFAULT);
  
  lttv_process_traceset_begin(self, NULL, NULL, NULL, hooks, NULL);

  while(1) {
    /* stop criteria : - n events found
     *                 - asked_time < beginning of trace */
    if(ltt_time_compare(asked_time, self->time_span.start_time) < 0) break;

    lttv_traceset_context_position_copy(end_pos, next_iter_end_pos);

    /* We must seek the traceset back to time - time_offset */
    /* this time becomes the new reference time */
    time = ltt_time_sub(time, time_offset);
    asked_time = time;
    
    time_seeker(self, time);
    lttv_traceset_context_position_save(self, next_iter_end_pos);
    /* Resync the time in case of a seek_closest */
    time = lttv_traceset_context_position_get_time(next_iter_end_pos);
    if(ltt_time_compare(time, self->time_span.end_time) > 0) {
      time = self->time_span.end_time;
    }

    /* Process the traceset, calling a hook which adds events 
     * to the array, overwriting the tail. It changes first_event and
     * events_found too. */
    /* We would like to have a clean context here : no other hook than our's */
    
    lttv_process_traceset_middle(self, ltt_time_infinite,
        G_MAXUINT, end_pos);

    if(sd.events_found < n) {
      if(sd.first_event > 0) {
        /* Save the first position */
        LttvTracesetContextPosition *pos =
          (LttvTracesetContextPosition*)g_ptr_array_index (sd.array, 0);
        lttv_traceset_context_position_copy(saved_pos, pos);
      }
      g_assert(n-sd.events_found <= sd.array->len);
      /* Change array size to n - events_found */
      for(i=n-sd.events_found;i<sd.array->len;i++) {
        LttvTracesetContextPosition *pos =
          (LttvTracesetContextPosition*)g_ptr_array_index (sd.array, i);
        lttv_traceset_context_position_destroy(pos);
      }
      g_ptr_array_set_size(sd.array, n-sd.events_found);
      sd.first_event = 0;
      
    } else break; /* Second end criterion : n events found */
    
    time_offset = ltt_time_mul(time_offset, BACKWARD_SEEK_MUL);
  }
  
  lttv_traceset_context_position_destroy(end_pos);
  lttv_traceset_context_position_destroy(next_iter_end_pos);
  
  lttv_process_traceset_end(self, NULL, NULL, NULL, hooks, NULL);

  if(sd.events_found >= n) {
    /* Seek the traceset to the first event in the circular array */
    LttvTracesetContextPosition *pos =
      (LttvTracesetContextPosition*)g_ptr_array_index (sd.array,
                                                       sd.first_event);
    g_assert(lttv_process_traceset_seek_position(self, pos) == 0);
  } else {
    /* Will seek to the last saved position : in the worst case, it will be the
     * original position (if events_found is 0) */
    g_assert(lttv_process_traceset_seek_position(self, saved_pos) == 0);
  }
  
  for(i=0;i<sd.array->len;i++) {
    LttvTracesetContextPosition *pos =
      (LttvTracesetContextPosition*)g_ptr_array_index (sd.array, i);
    lttv_traceset_context_position_destroy(pos);
  }
  g_ptr_array_free(sd.array, TRUE);

  lttv_hooks_destroy(hooks);

  lttv_traceset_context_position_destroy(saved_pos);

  return sd.events_found;
}


struct seek_forward_data {
  guint event_count;  /* event counter */
  guint n;            /* requested number of events to jump over */
  LttvFilter *filter1;
  LttvFilter *filter2;
  LttvFilter *filter3;
  gpointer data;
  check_handler *check;
  gboolean *stop_flag;
  guint raw_event_count;  /* event counter */
};

static gint seek_forward_event_hook(void *hook_data, void* call_data)
{
  struct seek_forward_data *sd = (struct seek_forward_data*)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext*)call_data;

  if(sd->check && sd->check(sd->raw_event_count, sd->stop_flag, sd->data))
    return TRUE;
  sd->raw_event_count++;

  if(sd->filter1 != NULL && sd->filter1->head != NULL && 
    !lttv_filter_tree_parse(sd->filter1->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }
  if(sd->filter2 != NULL && sd->filter2->head != NULL && 
    !lttv_filter_tree_parse(sd->filter2->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }
  if(sd->filter3 != NULL && sd->filter3->head != NULL && 
    !lttv_filter_tree_parse(sd->filter3->head,
          ltt_tracefile_get_event(tfc->tf),
          tfc->tf,
          tfc->t_context->t,
          tfc,NULL,NULL)) {
	  return FALSE;
  }

  sd->event_count++;
  if(sd->event_count >= sd->n)
      return TRUE;
  return FALSE;
}

/* Seek back n events forward from the current position (1 to n)
 * 0 is ok too, but it will actually do nothing.
 *
 * Parameters :
 * @self   the trace set context
 * @n      number of events to jump over
 * @filter filter to call.
 *
 * returns : the number of events jumped over (may be less than requested if end
 * of traceset reached) */
guint lttv_process_traceset_seek_n_forward(LttvTracesetContext *self,
                                          guint n,
                                          check_handler *check,
                                          gboolean *stop_flag,
					  LttvFilter *filter1,
					  LttvFilter *filter2,
					  LttvFilter *filter3,
					  gpointer data)
{
  struct seek_forward_data sd;
  sd.event_count = 0;
  sd.n = n;
  sd.filter1 = filter1;
  sd.filter2 = filter2;
  sd.filter3 = filter3;
  sd.data = data;
  sd.check = check;
  sd.stop_flag = stop_flag;
  sd.raw_event_count = 0;
  
  if(sd.event_count >= sd.n) return sd.event_count;
  
  LttvHooks *hooks = lttv_hooks_new();

  lttv_hooks_add(hooks, seek_forward_event_hook, &sd, LTTV_PRIO_DEFAULT);

  lttv_process_traceset_begin(self, NULL, NULL, NULL, hooks, NULL);
  
  /* it will end on the end of traceset, or the fact that the
   * hook returns TRUE.
   */
  lttv_process_traceset_middle(self, ltt_time_infinite,
        G_MAXUINT, NULL);

  /* Here, our position is either the end of traceset, or the exact position
   * after n events : leave it like this. This might be placed on an event that
   * will be filtered out, we don't care : all we know is that the following
   * event filtered in will be the right one. */

  lttv_process_traceset_end(self, NULL, NULL, NULL, hooks, NULL);

  lttv_hooks_destroy(hooks);

  return sd.event_count;
}


