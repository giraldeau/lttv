
#include <lttv/stats.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/event.h>

GQuark
  LTTV_STATS_PROCESS_UNKNOWN,
  LTTV_STATS_PROCESSES,
  LTTV_STATS_CPU,
  LTTV_STATS_MODE_TYPES,
  LTTV_STATS_MODES,
  LTTV_STATS_SUBMODES,
  LTTV_STATS_EVENT_TYPES,
  LTTV_STATS_CPU_TIME,
  LTTV_STATS_ELAPSED_TIME,
  LTTV_STATS_EVENTS,
  LTTV_STATS_EVENTS_COUNT;

static GQuark
  LTTV_STATS_BEFORE_HOOKS,
  LTTV_STATS_AFTER_HOOKS;

static void remove_all_processes(GHashTable *processes);

static void
find_event_tree(LttvTracefileStats *tfcs, GQuark process, GQuark cpu,
    GQuark mode, GQuark sub_mode, LttvAttribute **events_tree, 
    LttvAttribute **event_types_tree);

static void
init(LttvTracesetStats *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceContext *tc;

  LttvTraceStats *tcs;

  LttvTracefileContext *tfc;

  LttvTracefileStats *tfcs;
  
  LttTime timestamp = {0,0};

  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_STATE_TYPE))->
      init((LttvTracesetContext *)self, ts);

  self->stats = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceStats *)tc = (LTTV_TRACESET_CONTEXT(self)->traces[i]);
    tcs->stats = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);

    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfcs = LTTV_TRACEFILE_STATS(tc->control_tracefiles[j]);
      }
      else {
        tfcs = LTTV_TRACEFILE_STATS(tc->per_cpu_tracefiles[j - nb_control]);
      }

      tfcs->stats = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
      find_event_tree(tfcs, LTTV_STATS_PROCESS_UNKNOWN,
          tfcs->parent.cpu_name, LTTV_STATE_MODE_UNKNOWN, 
          LTTV_STATE_SUBMODE_UNKNOWN, &tfcs->current_events_tree,
          &tfcs->current_event_types_tree);
    }
  }
}


static void
fini(LttvTracesetStats *self)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceset *ts;

  LttvTraceContext *tc;

  LttvTraceStats *tcs;

  LttvTracefileContext *tfc;

  LttvTracefileStats *tfcs;
  
  LttTime timestamp = {0,0};

  lttv_attribute_recursive_free(self->stats);
  ts = self->parent.parent.ts;
  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceStats *)tc = (LTTV_TRACESET_CONTEXT(self)->traces[i]);
    lttv_attribute_recursive_free(tcs->stats);

    nb_tracefile = ltt_trace_control_tracefile_number(tc->t);
    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = (LttvTracefileStats *)tfc = tc->control_tracefiles[j];
      lttv_attribute_recursive_free(tfcs->stats);
      tfcs->current_events_tree = NULL;
      tfcs->current_event_types_tree = NULL;
    }

    nb_tracefile = ltt_trace_per_cpu_tracefile_number(tc->t);
    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = (LttvTracefileStats *)tfc = tc->per_cpu_tracefiles[j];
      lttv_attribute_recursive_free(tfcs->stats);
      tfcs->current_events_tree = NULL;
      tfcs->current_event_types_tree = NULL;
    }
  }
  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_STATE_TYPE))->
      fini((LttvTracesetContext *)self);
}


static LttvTracesetContext *
new_traceset_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT(g_object_new(LTTV_TRACESET_STATS_TYPE, NULL));
}


static LttvTraceContext * 
new_trace_context(LttvTracesetContext *self)
{
  return LTTV_TRACE_CONTEXT(g_object_new(LTTV_TRACE_STATS_TYPE, NULL));
}


static LttvTracefileContext *
new_tracefile_context(LttvTracesetContext *self)
{
  return LTTV_TRACEFILE_CONTEXT(g_object_new(LTTV_TRACEFILE_STATS_TYPE, NULL));
}


static void
traceset_stats_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
traceset_stats_finalize (LttvTracesetStats *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACESET_STATE_TYPE))->
      finalize(G_OBJECT(self));
}


static void
traceset_stats_class_init (LttvTracesetContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) traceset_stats_finalize;
  klass->init = (void (*)(LttvTracesetContext *self, LttvTraceset *ts))init;
  klass->fini = (void (*)(LttvTracesetContext *self))fini;
  klass->new_traceset_context = new_traceset_context;
  klass->new_trace_context = new_trace_context;
  klass->new_tracefile_context = new_tracefile_context;
}


GType 
lttv_traceset_stats_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracesetStatsClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) traceset_stats_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracesetContext),
      0,      /* n_preallocs */
      (GInstanceInitFunc) traceset_stats_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACESET_STATE_TYPE, "LttvTracesetStatsType", 
        &info, 0);
  }
  return type;
}


static void
trace_stats_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
trace_stats_finalize (LttvTraceStats *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACE_STATE_TYPE))->
      finalize(G_OBJECT(self));
}


static void
trace_stats_class_init (LttvTraceContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) trace_stats_finalize;
}


GType 
lttv_trace_stats_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTraceStatsClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) trace_stats_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTraceStats),
      0,      /* n_preallocs */
      (GInstanceInitFunc) trace_stats_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACE_STATE_TYPE, 
        "LttvTraceStatsType", &info, 0);
  }
  return type;
}


static void
tracefile_stats_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
tracefile_stats_finalize (LttvTracefileStats *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACEFILE_STATE_TYPE))->
      finalize(G_OBJECT(self));
}


static void
tracefile_stats_class_init (LttvTracefileStatsClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) tracefile_stats_finalize;
}


GType 
lttv_tracefile_stats_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracefileStatsClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) tracefile_stats_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracefileStats),
      0,      /* n_preallocs */
      (GInstanceInitFunc) tracefile_stats_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACEFILE_STATE_TYPE, 
        "LttvTracefileStatsType", &info, 0);
  }
  return type;
}


static void
find_event_tree(LttvTracefileStats *tfcs, GQuark process, GQuark cpu,
    GQuark mode, GQuark sub_mode, LttvAttribute **events_tree, 
    LttvAttribute **event_types_tree)
{
  LttvAttribute *a;

  LttvTraceStats *tcs = LTTV_TRACE_STATS(tfcs->parent.parent.t_context);
  a = lttv_attribute_find_subdir(tcs->stats, LTTV_STATS_PROCESSES);
  a = lttv_attribute_find_subdir(a, tfcs->parent.process->pid_time);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_CPU);
  a = lttv_attribute_find_subdir(a, tfcs->parent.cpu_name);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_MODE_TYPES);
  a = lttv_attribute_find_subdir(a, tfcs->parent.process->state->t);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_SUBMODES);
  a = lttv_attribute_find_subdir(a, tfcs->parent.process->state->n);
  *events_tree = a;
  a = lttv_attribute_find_subdir(a, LTTV_STATS_EVENT_TYPES);
  *event_types_tree = a;
}


static void update_event_tree(LttvTracefileStats *tfcs) 
{
  LttvExecutionState *es = tfcs->parent.process->state;

  find_event_tree(tfcs, tfcs->parent.process->pid_time, tfcs->parent.cpu_name, 
      es->t, es->n, &(tfcs->current_events_tree), 
      &(tfcs->current_event_types_tree));
}


static void mode_change(LttvTracefileStats *tfcs)
{
  LttvAttributeValue cpu_time; 

  LttTime delta;

  lttv_attribute_find(tfcs->current_events_tree, LTTV_STATS_CPU_TIME, 
      LTTV_TIME, &cpu_time);
  delta = ltt_time_sub(tfcs->parent.parent.timestamp, 
      tfcs->parent.process->state->change);
  *(cpu_time.v_time) = ltt_time_add(*(cpu_time.v_time), delta);
}


static void mode_end(LttvTracefileStats *tfcs)
{
  LttvAttributeValue elapsed_time, cpu_time; 

  LttTime delta;

  lttv_attribute_find(tfcs->current_events_tree, LTTV_STATS_ELAPSED_TIME, 
      LTTV_TIME, &elapsed_time);
  delta = ltt_time_sub(tfcs->parent.parent.timestamp, 
      tfcs->parent.process->state->entry);
  *(elapsed_time.v_time) = ltt_time_add(*(elapsed_time.v_time), delta);

  lttv_attribute_find(tfcs->current_events_tree, LTTV_STATS_CPU_TIME, 
      LTTV_TIME, &cpu_time);
  delta = ltt_time_sub(tfcs->parent.parent.timestamp, 
      tfcs->parent.process->state->change);
  *(cpu_time.v_time) = ltt_time_add(*(cpu_time.v_time), delta);
}


static gboolean before_syscall_entry(void *hook_data, void *call_data)
{
  mode_change((LttvTracefileStats *)call_data);
  return FALSE;
}


static gboolean after_syscall_entry(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_syscall_exit(void *hook_data, void *call_data)
{
  mode_end((LttvTracefileStats *)call_data);
  return FALSE;
}


static gboolean after_syscall_exit(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_trap_entry(void *hook_data, void *call_data)
{
  mode_change((LttvTracefileStats *)call_data);
  return FALSE;
}


static gboolean after_trap_entry(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_trap_exit(void *hook_data, void *call_data)
{
  mode_end((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean after_trap_exit(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_irq_entry(void *hook_data, void *call_data)
{
  mode_change((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean after_irq_entry(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_irq_exit(void *hook_data, void *call_data)
{
  mode_end((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean after_irq_exit(void *hook_data, void *call_data)
{
  update_event_tree((LttvTracefileStats *)call_data);
  return FALSE;
}


gboolean before_schedchange(void *hook_data, void *call_data)
{
  LttvTraceHook *h = (LttvTraceHook *)hook_data;

  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;

  guint pid_in, pid_out, state_out;

  LttvProcessState *process;

  pid_in = ltt_event_get_unsigned(tfcs->parent.parent.e, h->f1);
  pid_out = ltt_event_get_unsigned(tfcs->parent.parent.e, h->f2);
  state_out = ltt_event_get_unsigned(tfcs->parent.parent.e, h->f3);

  /* compute the time for the process to schedule out */

  mode_change(tfcs);

  /* get the information for the process scheduled in */

  process = lttv_state_find_process(&(tfcs->parent), pid_in);

  find_event_tree(tfcs, process->pid_time, tfcs->parent.cpu_name, 
      process->state->t, process->state->n, &(tfcs->current_events_tree), 
      &(tfcs->current_event_types_tree));

  /* compute the time waiting for the process to schedule in */

  mode_change(tfcs);
  return FALSE;
}


gboolean process_fork(void *hook_data, void *call_data)
{
  /* nothing to do for now */
  return FALSE;
}


gboolean process_exit(void *hook_data, void *call_data)
{
  /* We should probably exit all modes here or we could do that at 
     schedule out. */
  return FALSE;
}


gboolean every_event(void *hook_data, void *call_data)
{
  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;

  LttvAttributeValue v;

  /* The current branch corresponds to the tracefile/process/interrupt state.
     Statistics are added within it, to count the number of events of this
     type occuring in this context. A quark has been pre-allocated for each
     event type and is used as name. */

  lttv_attribute_find(tfcs->current_event_types_tree, 
      ((LttvTraceState *)(tfcs->parent.parent.t_context))->
      eventtype_names[ltt_event_eventtype_id(tfcs->parent.parent.e)], 
      LTTV_UINT, &v);
  (*(v.v_uint))++;
  return FALSE;
}


static gboolean 
sum_stats(void *hook_data, void *call_data)
{
  LttvTracesetStats *tscs = (LttvTracesetStats *)call_data;

  LttvTraceStats *tcs;

  LttvTraceset *traceset = tscs->parent.parent.ts;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  unsigned sum;

  int i, j, k, l, m, n, nb_trace, nb_process, nb_cpu, nb_mode_type, nb_submode,
      nb_event_type;

  LttvAttribute *main_tree, *processes_tree, *process_tree, *cpus_tree,
      *cpu_tree, *mode_tree, *mode_types_tree, *submodes_tree,
      *submode_tree, *event_types_tree, *mode_events_tree,
      *cpu_events_tree, *process_modes_tree, *trace_cpu_tree, 
      *trace_modes_tree, *traceset_modes_tree;

  traceset_modes_tree = lttv_attribute_find_subdir(tscs->stats, 
      LTTV_STATS_MODES);
  nb_trace = lttv_traceset_number(traceset);

  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceStats *)(tscs->parent.parent.traces[i]);
    main_tree = tcs->stats;
    processes_tree = lttv_attribute_find_subdir(main_tree, 
        LTTV_STATS_PROCESSES);
    trace_modes_tree = lttv_attribute_find_subdir(main_tree, LTTV_STATS_MODES);
    nb_process = lttv_attribute_get_number(processes_tree);

    for(j = 0 ; j < nb_process ; j++) {
      type = lttv_attribute_get(processes_tree, j, &name, &value);
      process_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

      cpus_tree = lttv_attribute_find_subdir(process_tree, LTTV_STATS_CPU);
      process_modes_tree = lttv_attribute_find_subdir(process_tree,
          LTTV_STATS_MODES);
      nb_cpu = lttv_attribute_get_number(cpus_tree);

      for(k = 0 ; k < nb_cpu ; k++) {
        type = lttv_attribute_get(cpus_tree, k, &name, &value);
        cpu_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

        mode_types_tree = lttv_attribute_find_subdir(cpu_tree, 
            LTTV_STATS_MODE_TYPES);
        cpu_events_tree = lttv_attribute_find_subdir(cpu_tree,
            LTTV_STATS_EVENTS);
        trace_cpu_tree = lttv_attribute_find_subdir(main_tree, LTTV_STATS_CPU);
        trace_cpu_tree = lttv_attribute_find_subdir(trace_cpu_tree, name);
        nb_mode_type = lttv_attribute_get_number(mode_types_tree);

        for(l = 0 ; l < nb_mode_type ; l++) {
          type = lttv_attribute_get(mode_types_tree, l, &name, &value);
          mode_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

          submodes_tree = lttv_attribute_find_subdir(mode_tree, 
              LTTV_STATS_SUBMODES);
          mode_events_tree = lttv_attribute_find_subdir(mode_tree,
	      LTTV_STATS_EVENTS);
          nb_submode = lttv_attribute_get_number(submodes_tree);

          for(m = 0 ; m < nb_submode ; m++) {
            type = lttv_attribute_get(submodes_tree, m, &name, &value);
            submode_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

            event_types_tree = lttv_attribute_find_subdir(submode_tree, 
              LTTV_STATS_EVENT_TYPES);
            nb_event_type = lttv_attribute_get_number(event_types_tree);

            sum = 0;
            for(n = 0 ; n < nb_event_type ; n++) {
              type = lttv_attribute_get(event_types_tree, n, &name, &value);
              sum += *(value.v_uint);
            }
            lttv_attribute_find(submode_tree, LTTV_STATS_EVENTS_COUNT, 
                LTTV_UINT, &value);
            *(value.v_uint) = sum;
            lttv_attribute_recursive_add(mode_events_tree, submode_tree);
          }
	  lttv_attribute_recursive_add(cpu_events_tree, mode_events_tree);
        }
        lttv_attribute_recursive_add(process_modes_tree, cpu_tree);
        lttv_attribute_recursive_add(trace_cpu_tree, cpu_tree);
      }
      lttv_attribute_recursive_add(trace_modes_tree, process_modes_tree);
    }
    lttv_attribute_recursive_add(traceset_modes_tree, trace_modes_tree);
  }
  return FALSE;
}


lttv_stats_add_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttFacility *f;

  LttEventType *et;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  void *hook_data;

  GArray *hooks, *before_hooks, *after_hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceStats *)self->parent.parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
    g_array_set_size(hooks, 7);

    lttv_trace_find_hook(ts->parent.parent.t, "core","syscall_entry",
        "syscall_id", NULL, NULL, before_syscall_entry, 
        &g_array_index(hooks, LttvTraceHook, 0));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "syscall_exit", NULL, 
        NULL, NULL, before_syscall_exit, 
        &g_array_index(hooks, LttvTraceHook, 1));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "trap_entry", "trap_id",
	NULL, NULL, before_trap_entry, 
        &g_array_index(hooks, LttvTraceHook, 2));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "trap_exit", NULL, NULL,
	NULL, before_trap_exit, &g_array_index(hooks, LttvTraceHook, 3));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "irq_entry", "irq_id",
	NULL, NULL, before_irq_entry, &g_array_index(hooks, LttvTraceHook, 4));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "irq_exit", NULL, NULL,
	NULL, before_irq_exit, &g_array_index(hooks, LttvTraceHook, 5));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "schedchange", "in", 
        "out", "out_state", before_schedchange, 
         &g_array_index(hooks, LttvTraceHook, 6));

    before_hooks = hooks;

    hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
    g_array_set_size(hooks, 8);

    lttv_trace_find_hook(ts->parent.parent.t, "core","syscall_entry",
        "syscall_id", NULL, NULL, after_syscall_entry, 
        &g_array_index(hooks, LttvTraceHook, 0));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "syscall_exit", NULL, 
        NULL, NULL, after_syscall_exit, 
        &g_array_index(hooks, LttvTraceHook, 1));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "trap_entry", "trap_id",
	NULL, NULL, after_trap_entry, &g_array_index(hooks, LttvTraceHook, 2));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "trap_exit", NULL, NULL,
	NULL, after_trap_exit, &g_array_index(hooks, LttvTraceHook, 3));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "irq_entry", "irq_id",
	NULL, NULL, after_irq_entry, &g_array_index(hooks, LttvTraceHook, 4));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "irq_exit", NULL, NULL,
	NULL, after_irq_exit, &g_array_index(hooks, LttvTraceHook, 5));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "process_fork", 
        "child_pid", NULL, NULL, process_fork, 
        &g_array_index(hooks, LttvTraceHook, 6));

    lttv_trace_find_hook(ts->parent.parent.t, "core", "process_exit", NULL, 
        NULL, NULL, process_exit, &g_array_index(hooks, LttvTraceHook, 7));

    after_hooks = hooks;

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.parent.t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(ts->parent.parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATS(ts->parent.parent.control_tracefiles[j]);
      }
      else {
        tfs = LTTV_TRACEFILE_STATS(ts->parent.parent.per_cpu_tracefiles[
            j-nb_control]);
      }

      lttv_hooks_add(tfs->parent.parent.after_event, every_event, NULL);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = g_array_index(before_hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(
            tfs->parent.parent.before_event_by_id, 
	    hook.id), hook.h, &g_array_index(before_hooks, LttvTraceHook, k));
      }
      for(k = 0 ; k < after_hooks->len ; k++) {
        hook = g_array_index(after_hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(
            tfs->parent.parent.after_event_by_id, 
	    hook.id), hook.h, &g_array_index(after_hooks, LttvTraceHook, k));
      }
    }
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    *(val.v_pointer) = before_hooks;
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_AFTER_HOOKS, 
        LTTV_POINTER, &val);
    *(val.v_pointer) = after_hooks;
  }
  lttv_hooks_add(self->parent.parent.after, sum_stats, NULL);
}


lttv_stats_remove_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  void *hook_data;

  GArray *before_hooks, *after_hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATS(self->parent.parent.traces[i]);
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    before_hooks = *(val.v_pointer);
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_AFTER_HOOKS, 
        LTTV_POINTER, &val);
    after_hooks = *(val.v_pointer);

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.parent.t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(ts->parent.parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATS(ts->parent.parent.control_tracefiles[j]);
      }
      else {
        tfs =LTTV_TRACEFILE_STATS(ts->parent.parent.per_cpu_tracefiles[
            j-nb_control]);
      }

      lttv_hooks_remove_data(tfs->parent.parent.after_event, every_event, 
          NULL);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = g_array_index(before_hooks, LttvTraceHook, k);
        lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.parent.before_event_by_id, 
	    hook.id), hook.h, &g_array_index(before_hooks, LttvTraceHook, k));
      }
      for(k = 0 ; k < after_hooks->len ; k++) {
        hook = g_array_index(after_hooks, LttvTraceHook, k);
        lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.parent.after_event_by_id, 
	    hook.id), hook.h, &g_array_index(after_hooks, LttvTraceHook, k));
      }
    }
    g_critical("lttv_stats_remove_event_hooks()");
    g_array_free(before_hooks, TRUE);
    g_array_free(after_hooks, TRUE);
  }
  lttv_hooks_remove_data(self->parent.parent.after, sum_stats, NULL);
}


void lttv_stats_init(int argc, char **argv)
{
  LTTV_STATS_PROCESS_UNKNOWN = g_quark_from_string("unknown process");
  LTTV_STATS_PROCESSES = g_quark_from_string("processes");
  LTTV_STATS_CPU = g_quark_from_string("cpu");
  LTTV_STATS_MODE_TYPES = g_quark_from_string("mode_types");
  LTTV_STATS_MODES = g_quark_from_string("modes");
  LTTV_STATS_SUBMODES = g_quark_from_string("submodes");
  LTTV_STATS_EVENT_TYPES = g_quark_from_string("event_types");
  LTTV_STATS_CPU_TIME = g_quark_from_string("cpu time");
  LTTV_STATS_ELAPSED_TIME = g_quark_from_string("elapsed time");
  LTTV_STATS_EVENTS = g_quark_from_string("events");
  LTTV_STATS_EVENTS_COUNT = g_quark_from_string("events count");
  LTTV_STATS_BEFORE_HOOKS = g_quark_from_string("saved stats before hooks");
  LTTV_STATS_AFTER_HOOKS = g_quark_from_string("saved stats after hooks");
}

void lttv_stats_destroy() 
{
}

