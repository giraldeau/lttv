
#include <lttv/processTrace.h>
#include <ltt/event.h>

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


static void
init(LttvTracesetContext *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  nb_trace = lttv_traceset_number(ts);
  self->ts = ts;
  self->traces = g_new(LttvTraceContext *, nb_trace);
  self->before = lttv_hooks_new();
  self->after = lttv_hooks_new();
  self->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_trace_context(self);
    self->traces[i] = tc;

    tc->ts_context = self;
    tc->index = i;
    tc->t = lttv_traceset_get(ts, i);
    tc->check = lttv_hooks_new();
    tc->before = lttv_hooks_new();
    tc->after = lttv_hooks_new();
    tc->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;
    tc->control_tracefiles = g_new(LttvTracefileContext *, nb_control);
    tc->per_cpu_tracefiles = g_new(LttvTracefileContext *, nb_per_cpu);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = LTTV_TRACESET_CONTEXT_GET_CLASS(self)->new_tracefile_context(self);
      if(j < nb_control) {
        tc->control_tracefiles[j] = tfc;
        tfc->control = TRUE;
        tfc->index = j;
        tfc->tf = ltt_trace_control_tracefile_get(tc->t, j);
      }
      else {
        tc->per_cpu_tracefiles[j - nb_control] = tfc;
        tfc->control = FALSE;
        tfc->index = j - nb_control;
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
}


void fini(LttvTracesetContext *self)
{
  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceContext *tc;

  LttvTracefileContext *tfc;

  LttvTraceset *ts = self->ts;

  lttv_hooks_destroy(self->before);
  lttv_hooks_destroy(self->after);
  g_object_unref(self->a);

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->traces[i];

    lttv_hooks_destroy(tc->check);
    lttv_hooks_destroy(tc->before);
    lttv_hooks_destroy(tc->after);
    g_object_unref(tc->a);

    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;

    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) tfc = tc->control_tracefiles[j];
      else tfc = tc->per_cpu_tracefiles[j - nb_control];

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
    g_free(tc->control_tracefiles);
    g_free(tc->per_cpu_tracefiles);
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

  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

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
    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;

    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfc = tc->control_tracefiles[j];
      }
      else {
        tfc = tc->per_cpu_tracefiles[j-nb_control];
      }
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

  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

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
    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;

    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfc = tc->control_tracefiles[j];
      }
      else {
        tfc = tc->per_cpu_tracefiles[j-nb_control];
      }
      lttv_hooks_remove_list(tfc->check, check_tracefile);
      lttv_hooks_remove_list(tfc->before, before_tracefile);
      lttv_hooks_remove_list(tfc->after, after_tracefile);
      lttv_hooks_remove_list(tfc->check_event, check_event);
      lttv_hooks_remove_list(tfc->before_event, before_event);
      lttv_hooks_remove_list(tfc->after_event, after_event);
    }
  }
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
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACESET_CONTEXT_GET_CLASS(self)))->finalize(G_OBJECT(self));
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
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACE_CONTEXT_GET_CLASS(self)))->finalize(G_OBJECT(self));
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
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACEFILE_CONTEXT_GET_CLASS(self)))->finalize(G_OBJECT(self));
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
  if(((LttvTime *)a)->tv_sec > ((LttvTime *)b)->tv_sec) return 1;
  if(((LttvTime *)a)->tv_sec < ((LttvTime *)b)->tv_sec) return -1;
  if(((LttvTime *)a)->tv_nsec > ((LttvTime *)b)->tv_nsec) return 1;
  if(((LttvTime *)a)->tv_nsec < ((LttvTime *)b)->tv_nsec) return -1;
  return 0;
}


gboolean get_first(gpointer key, gpointer value, gpointer user_data) {
  *((LttvTracefileContext **)user_data) = (LttvTracefileContext *)value;
  return TRUE;
}


void lttv_process_trace(LttTime start, LttTime end, LttvTraceset *traceset, 
    LttvTracesetContext *context)
{
  GPtrArray *traces = g_ptr_array_new();

  GPtrArray *tracefiles = g_ptr_array_new();

  GTree *pqueue = g_tree_new(compare_tracefile);

  guint i, j, nbi, nbj, id, nb_control, nb_cpu;

  LttTrace *trace;

  LttvTraceContext *tc;

  LttTracefile *tracefile;

  LttvTracefileContext *tfc;

  LttEvent *event;

  /* Call all before_traceset, before_trace, and before_tracefile hooks.
     For all qualifying tracefiles, seek to the start time, create a context,
     read one event and insert in the pqueue based on the event time. */

  lttv_hooks_call(context->before, context);
  nbi = lttv_traceset_number(traceset);
  //  nbi = ltt_trace_set_number(traceset);

  for(i = 0 ; i < nbi ; i++) {
    tc = context->traces[i];
    trace = tc->t;

    if(!lttv_hooks_call_check(tc->check, tc)) {
      g_ptr_array_add(traces, tc);
      lttv_hooks_call(tc->before, tc);
      nb_control = ltt_trace_control_tracefile_number(trace);
      nb_cpu = ltt_trace_per_cpu_tracefile_number(trace);
      nbj = nb_control + nb_cpu;

      for(j = 0 ; j < nbj ; j++) {
        if(j < nb_control) {
          tfc = tc->control_tracefiles[j];
        }
        else {
          tfc = tc->per_cpu_tracefiles[j - nb_control];
        }

        tracefile = tfc->tf;

        if(!lttv_hooks_call_check(tfc->check, tfc)) {
          g_ptr_array_add(tracefiles, tfc);
          lttv_hooks_call(tfc->before, tfc);

          ltt_tracefile_seek_time(tracefile, start);
          event = ltt_tracefile_read(tracefile);
          tfc->e = event;

          if(event != NULL) {
            tfc->timestamp = ltt_event_time(event);
            g_tree_insert(pqueue, &(tfc->timestamp), tfc);
          }
        }
      }
    }
  }

  /* Get the next event from the pqueue, call its hooks, 
     reinsert in the pqueue the following event from the same tracefile 
     unless the tracefile is finished or the event is later than the 
     start time. */

  while(TRUE) {
    tfc = NULL;
    g_tree_foreach(pqueue, get_first, &tfc);
    if(tfc == NULL) break;

    /* Get the tracefile with an event for the smallest time found. If two
       or more tracefiles have events for the same time, hope that lookup
       and remove are consistent. */

    tfc = g_tree_lookup(pqueue, &(tfc->timestamp));
    g_tree_remove(pqueue, &(tfc->timestamp));

    if(!lttv_hooks_call(tfc->check_event, context)) {
      id = ltt_event_eventtype_id(tfc->e);
      lttv_hooks_call(tfc->before_event, tfc);
      lttv_hooks_call(lttv_hooks_by_id_get(tfc->before_event_by_id, id), tfc);
      lttv_hooks_call(tfc->after_event, context);
      lttv_hooks_call(lttv_hooks_by_id_get(tfc->after_event_by_id, id), tfc);
    }

    event = ltt_tracefile_read(tfc->tf);
    if(event != NULL) {
      tfc->e = event;
      tfc->timestamp = ltt_event_time(event);
      g_tree_insert(pqueue, &(tfc->timestamp), tfc);
    }
  }

  /* Call all the after_tracefile, after_trace and after_traceset hooks. */

  for(i = 0, j = 0 ; i < traces->len ; i++) {
    tc = traces->pdata[i];
    while(j < tracefiles->len) {
      tfc = tracefiles->pdata[j];

      if(tfc->t_context == tc) {
        lttv_hooks_call(tfc->after, tfc);
        j++;
      }
      else break;
    }
    lttv_hooks_call(tc->after, tc);
  }

  g_assert(j == tracefiles->len);
  lttv_hooks_call(context->after, context);

  /* Free the traces, tracefiles and pqueue */

  g_ptr_array_free(tracefiles, TRUE);
  g_ptr_array_free(traces, TRUE);
  g_tree_destroy(pqueue);
}
