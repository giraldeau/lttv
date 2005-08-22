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

#include <stdio.h>
#include <lttv/module.h>
#include <lttv/stats.h>
#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/event.h>

#define BUF_SIZE 256

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
  LTTV_STATS_EVENTS_COUNT,
  LTTV_STATS_USE_COUNT,
  LTTV_STATS,
  LTTV_STATS_TRACEFILES,
  LTTV_STATS_SUMMED;

static GQuark
  LTTV_STATS_BEFORE_HOOKS,
  LTTV_STATS_AFTER_HOOKS;

static void
find_event_tree(LttvTracefileStats *tfcs, GQuark pid_time, GQuark cpu,
    GQuark mode, GQuark sub_mode, LttvAttribute **events_tree, 
    LttvAttribute **event_types_tree);

static void
init(LttvTracesetStats *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTraceStats *tcs;

  LttvTracefileContext *tfc;

  LttvTracefileContext **tfs;
  LttvTracefileStats *tfcs;
  
  LttTime timestamp = {0,0};

  LttvAttributeValue v;

  LttvAttribute
    *stats_tree,
    *tracefiles_stats;

  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_STATE_TYPE))->
      init((LttvTracesetContext *)self, ts);

  self->stats = lttv_attribute_find_subdir(
                      lttv_traceset_attribute(self->parent.parent.ts),
                      LTTV_STATS);
  lttv_attribute_find(lttv_traceset_attribute(self->parent.parent.ts),
                      LTTV_STATS_USE_COUNT, 
                      LTTV_UINT, &v);

  *(v.v_uint)++;
  if(*(v.v_uint) == 1) { 
    g_assert(lttv_attribute_get_number(self->stats) == 0);
  }

  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->parent.parent.traces[i];
    tcs = LTTV_TRACE_STATS(tc);

    tcs->stats = lttv_attribute_find_subdir(tcs->parent.parent.t_a,LTTV_STATS);
    tracefiles_stats = lttv_attribute_find_subdir(tcs->parent.parent.t_a, 
          LTTV_STATS_TRACEFILES);
    lttv_attribute_find(tcs->parent.parent.t_a, LTTV_STATS_USE_COUNT, 
        LTTV_UINT, &v);

    *(v.v_uint)++;
    if(*(v.v_uint) == 1) { 
      g_assert(lttv_attribute_get_number(tcs->stats) == 0);
    }

    nb_tracefile = tc->tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = &g_array_index(tc->tracefiles,
                      LttvTracefileContext*, j);
      tfcs = LTTV_TRACEFILE_STATS(*tfs);
      tfcs->stats = lttv_attribute_find_subdir(tracefiles_stats, 
          tfcs->parent.cpu_name);
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

  LttvAttributeValue v;

  LttvAttribute *tracefiles_stats;

  lttv_attribute_find(self->parent.parent.ts_a, LTTV_STATS_USE_COUNT, 
        LTTV_UINT, &v);
  *(v.v_uint)--;

  if(*(v.v_uint) == 0) {
    lttv_attribute_remove_by_name(self->parent.parent.ts_a, LTTV_STATS);
  }
  self->stats = NULL;

  ts = self->parent.parent.ts;
  nb_trace = lttv_traceset_number(ts);

  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceStats *)(tc = (LTTV_TRACESET_CONTEXT(self)->traces[i]));

    lttv_attribute_find(tcs->parent.parent.t_a, LTTV_STATS_USE_COUNT, 
        LTTV_UINT, &v);
    *(v.v_uint)--;

    if(*(v.v_uint) == 0) { 
      lttv_attribute_remove_by_name(tcs->parent.parent.t_a,LTTV_STATS);
      tracefiles_stats = lttv_attribute_find_subdir(tcs->parent.parent.t_a, 
          LTTV_STATS_TRACEFILES);
      lttv_attribute_remove_by_name(tcs->parent.parent.t_a,
          LTTV_STATS_TRACEFILES);
    }
    tcs->stats = NULL;

    nb_tracefile = tc->tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfc = g_array_index(tc->tracefiles,
                                  LttvTracefileContext*, j);
      tfcs = (LttvTracefileStats *)tfc;
      tfcs->stats = NULL;
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
      sizeof (LttvTracesetStats),
      0,      /* n_preallocs */
      (GInstanceInitFunc) traceset_stats_instance_init,    /* instance_init */
      NULL    /* Value handling */
    };

    type = g_type_register_static (LTTV_TRACESET_STATE_TYPE,
                                   "LttvTracesetStatsType", 
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
      (GInstanceInitFunc) trace_stats_instance_init,    /* instance_init */
      NULL    /* Value handling */
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
      (GInstanceInitFunc) tracefile_stats_instance_init,    /* instance_init */
      NULL    /* Value handling */
    };

    type = g_type_register_static (LTTV_TRACEFILE_STATE_TYPE, 
        "LttvTracefileStatsType", &info, 0);
  }
  return type;
}


static void
find_event_tree(LttvTracefileStats *tfcs,
                GQuark pid_time,
                GQuark cpu,
                GQuark mode,
                GQuark sub_mode,
                LttvAttribute **events_tree, 
                LttvAttribute **event_types_tree)
{
  LttvAttribute *a;

  LttvTraceStats *tcs = (LttvTraceStats*)tfcs->parent.parent.t_context;
  a = lttv_attribute_find_subdir(tcs->stats, LTTV_STATS_PROCESSES);
  a = lttv_attribute_find_subdir(a, pid_time);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_CPU);
  a = lttv_attribute_find_subdir(a, cpu);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_MODE_TYPES);
  a = lttv_attribute_find_subdir(a, mode);
  a = lttv_attribute_find_subdir(a, LTTV_STATS_SUBMODES);
  a = lttv_attribute_find_subdir(a, sub_mode);
  *events_tree = a;
  a = lttv_attribute_find_subdir(a, LTTV_STATS_EVENT_TYPES);
  *event_types_tree = a;
}


static void update_event_tree(LttvTracefileStats *tfcs) 
{
  LttvExecutionState *es = tfcs->parent.process->state;

  find_event_tree(tfcs, tfcs->parent.process->pid_time,
      tfcs->parent.cpu_name, 
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
  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;

  LttEvent *e = ltt_tracefile_get_event(tfcs->parent.parent.tf);

  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;

  guint pid_in, pid_out;
    
  gint state_out;

  LttvProcessState *process;

  pid_out = ltt_event_get_unsigned(e, thf->f1);
  pid_in = ltt_event_get_unsigned(e, thf->f2);
  state_out = ltt_event_get_int(e, thf->f3);

  /* compute the time for the process to schedule out */

  mode_change(tfcs);

  /* get the information for the process scheduled in */

  process = lttv_state_find_process_or_create(&(tfcs->parent), pid_in);

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

gboolean process_free(void *hook_data, void *call_data)
{
  return FALSE;
}

gboolean every_event(void *hook_data, void *call_data)
{
  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;

  LttEvent *e = ltt_tracefile_get_event(tfcs->parent.parent.tf);

  LttvAttributeValue v;

  /* The current branch corresponds to the tracefile/process/interrupt state.
     Statistics are added within it, to count the number of events of this
     type occuring in this context. A quark has been pre-allocated for each
     event type and is used as name. */

  lttv_attribute_find(tfcs->current_event_types_tree, 
      ltt_eventtype_name(ltt_event_eventtype(e)), 
      LTTV_UINT, &v);
  (*(v.v_uint))++;
  return FALSE;
}


void
lttv_stats_sum_trace(LttvTraceStats *self)
{
  LttvAttribute *sum_container = self->stats;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  unsigned sum;

  int i, j, k, l, m, nb_process, nb_cpu, nb_mode_type, nb_submode,
      nb_event_type;

  LttvAttribute *main_tree, *processes_tree, *process_tree, *cpus_tree,
      *cpu_tree, *mode_tree, *mode_types_tree, *submodes_tree,
      *submode_tree, *event_types_tree, *mode_events_tree,
      *cpu_events_tree, *process_modes_tree, *trace_cpu_tree, 
      *trace_modes_tree;

  main_tree = sum_container;

  lttv_attribute_find(sum_container,
                      LTTV_STATS_SUMMED, 
                      LTTV_UINT, &value);
  if(*(value.v_uint) != 0) return;
  *(value.v_uint) = 1;

  processes_tree = lttv_attribute_find_subdir(main_tree, 
                                              LTTV_STATS_PROCESSES);
  trace_modes_tree = lttv_attribute_find_subdir(main_tree,
                                                LTTV_STATS_MODES);
  nb_process = lttv_attribute_get_number(processes_tree);

  for(i = 0 ; i < nb_process ; i++) {
    type = lttv_attribute_get(processes_tree, i, &name, &value);
    process_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

    cpus_tree = lttv_attribute_find_subdir(process_tree, LTTV_STATS_CPU);
    process_modes_tree = lttv_attribute_find_subdir(process_tree,
        LTTV_STATS_MODES);
    nb_cpu = lttv_attribute_get_number(cpus_tree);

    for(j = 0 ; j < nb_cpu ; j++) {
      type = lttv_attribute_get(cpus_tree, j, &name, &value);
      cpu_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

      mode_types_tree = lttv_attribute_find_subdir(cpu_tree, 
          LTTV_STATS_MODE_TYPES);
      cpu_events_tree = lttv_attribute_find_subdir(cpu_tree,
          LTTV_STATS_EVENTS);
      trace_cpu_tree = lttv_attribute_find_subdir(main_tree, LTTV_STATS_CPU);
      trace_cpu_tree = lttv_attribute_find_subdir(trace_cpu_tree, name);
      nb_mode_type = lttv_attribute_get_number(mode_types_tree);

      for(k = 0 ; k < nb_mode_type ; k++) {
        type = lttv_attribute_get(mode_types_tree, k, &name, &value);
        mode_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

        submodes_tree = lttv_attribute_find_subdir(mode_tree, 
            LTTV_STATS_SUBMODES);
        mode_events_tree = lttv_attribute_find_subdir(mode_tree,
            LTTV_STATS_EVENTS);
        nb_submode = lttv_attribute_get_number(submodes_tree);

        for(l = 0 ; l < nb_submode ; l++) {
          type = lttv_attribute_get(submodes_tree, l, &name, &value);
          submode_tree = LTTV_ATTRIBUTE(*(value.v_gobject));

          event_types_tree = lttv_attribute_find_subdir(submode_tree, 
            LTTV_STATS_EVENT_TYPES);
          nb_event_type = lttv_attribute_get_number(event_types_tree);

          sum = 0;
          for(m = 0 ; m < nb_event_type ; m++) {
            type = lttv_attribute_get(event_types_tree, m, &name, &value);
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
}


gboolean lttv_stats_sum_traceset_hook(void *hook_data, void *call_data)
{
  lttv_stats_sum_traceset((LttvTracesetStats *)call_data);
  return 0;
}

void
lttv_stats_sum_traceset(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;
  LttvAttribute *sum_container = self->stats;

  LttvTraceStats *tcs;

  int i, nb_trace;

  LttvAttribute *main_tree, *trace_modes_tree, *traceset_modes_tree;

  LttvAttributeValue value;

  lttv_attribute_find(sum_container, LTTV_STATS_SUMMED, 
      LTTV_UINT, &value);
  if(*(value.v_uint) != 0) return;
  *(value.v_uint) = 1;

  traceset_modes_tree = lttv_attribute_find_subdir(sum_container, 
      LTTV_STATS_MODES);
  nb_trace = lttv_traceset_number(traceset);

  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceStats *)(self->parent.parent.traces[i]);
    lttv_stats_sum_trace(tcs);
    main_tree = tcs->stats;
    trace_modes_tree = lttv_attribute_find_subdir(main_tree, LTTV_STATS_MODES);
    lttv_attribute_recursive_add(traceset_modes_tree, trace_modes_tree);
  }
}


// Hook wrapper. call_data is a traceset context.
gboolean lttv_stats_hook_add_event_hooks(void *hook_data, void *call_data)
{
   LttvTracesetStats *tss = (LttvTracesetStats*)call_data;

   lttv_stats_add_event_hooks(tss);

   return 0;
}

void lttv_stats_add_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttFacility *f;

  LttEventType *et;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  void *hook_data;

  GArray *hooks, *before_hooks, *after_hooks;

  LttvTraceHook *hook;

  LttvTraceHookByFacility *thf;

  LttvAttributeValue val;

  gint ret;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceStats *)self->parent.parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 7);
    g_array_set_size(hooks, 7);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_ENTRY,
        LTT_FIELD_SYSCALL_ID, 0, 0,
        before_syscall_entry, 
        &g_array_index(hooks, LttvTraceHook, 0));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_EXIT,
        0, 0, 0,
        before_syscall_exit, 
        &g_array_index(hooks, LttvTraceHook, 1));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_ENTRY,
        LTT_FIELD_TRAP_ID, 0, 0,
        before_trap_entry, 
        &g_array_index(hooks, LttvTraceHook, 2));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_EXIT,
        0, 0, 0,
        before_trap_exit, &g_array_index(hooks, LttvTraceHook, 3));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
        LTT_FIELD_IRQ_ID, 0, 0,
        before_irq_entry, &g_array_index(hooks, LttvTraceHook, 4));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
        0, 0, 0,
        before_irq_exit, &g_array_index(hooks, LttvTraceHook, 5));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_SCHEDCHANGE,
        LTT_FIELD_OUT, LTT_FIELD_IN, LTT_FIELD_OUT_STATE,
        before_schedchange, 
        &g_array_index(hooks, LttvTraceHook, 6));
    g_assert(!ret);

    before_hooks = hooks;

    hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 9);
    g_array_set_size(hooks, 9);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_ENTRY,
        LTT_FIELD_SYSCALL_ID, 0, 0,
        after_syscall_entry, 
        &g_array_index(hooks, LttvTraceHook, 0));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_EXIT,
        0, 0, 0,
        after_syscall_exit, 
        &g_array_index(hooks, LttvTraceHook, 1));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_ENTRY, 
        LTT_FIELD_TRAP_ID, 0, 0,
        after_trap_entry, &g_array_index(hooks, LttvTraceHook, 2));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_EXIT,
        0, 0, 0,
        after_trap_exit, &g_array_index(hooks, LttvTraceHook, 3));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY, 
        LTT_FIELD_IRQ_ID, 0, 0,
        after_irq_entry, &g_array_index(hooks, LttvTraceHook, 4));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
        0, 0, 0,
        after_irq_exit, &g_array_index(hooks, LttvTraceHook, 5));
    g_assert(!ret);


    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FORK, 
        LTT_FIELD_PARENT_PID, LTT_FIELD_CHILD_PID, 0,
        process_fork, 
        &g_array_index(hooks, LttvTraceHook, 6));
    g_assert(!ret);

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_EXIT,
        LTT_FIELD_PID, 0, 0,
        process_exit, &g_array_index(hooks, LttvTraceHook, 7));
    g_assert(!ret);
    
    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FREE,
        LTT_FIELD_PID, 0, 0,
        process_free, &g_array_index(hooks, LttvTraceHook, 8));
    g_assert(!ret);


    after_hooks = hooks;

    /* Add these hooks to each event_by_id hooks list */

    nb_tracefile = ts->parent.parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATS(g_array_index(ts->parent.parent.tracefiles,
                                  LttvTracefileContext*, j));
      lttv_hooks_add(tfs->parent.parent.event, every_event, NULL, 
                     LTTV_PRIO_DEFAULT);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = &g_array_index(before_hooks, LttvTraceHook, k);
        for(l = 0; l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_add(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf,
              LTTV_PRIO_STATS_BEFORE_STATE);
        }
      }
      for(k = 0 ; k < after_hooks->len ; k++) {
        hook = &g_array_index(after_hooks, LttvTraceHook, k);
        for(l = 0; l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_add(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf,
              LTTV_PRIO_STATS_AFTER_STATE);
        }
      }
    }
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    *(val.v_pointer) = before_hooks;
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_AFTER_HOOKS, 
        LTTV_POINTER, &val);
    *(val.v_pointer) = after_hooks;
  }
}

// Hook wrapper. call_data is a traceset context.
gboolean lttv_stats_hook_remove_event_hooks(void *hook_data, void *call_data)
{
   LttvTracesetStats *tss = (LttvTracesetStats*)call_data;

   lttv_stats_remove_event_hooks(tss);

   return 0;
}

void lttv_stats_remove_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  void *hook_data;

  GArray *before_hooks, *after_hooks;

  LttvTraceHook *hook;
  
  LttvTraceHookByFacility *thf;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceStats*)self->parent.parent.traces[i];
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    before_hooks = *(val.v_pointer);
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_AFTER_HOOKS, 
        LTTV_POINTER, &val);
    after_hooks = *(val.v_pointer);

    /* Remove these hooks from each event_by_id hooks list */

    nb_tracefile = ts->parent.parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATS(g_array_index(ts->parent.parent.tracefiles,
                                  LttvTracefileContext*, j));
      lttv_hooks_remove_data(tfs->parent.parent.event, every_event, 
          NULL);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = &g_array_index(before_hooks, LttvTraceHook, k);
        for(l = 0 ; l < hook->fac_list->len ; l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_remove_data(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf);
        }
      }
      for(k = 0 ; k < after_hooks->len ; k++) {
        hook = &g_array_index(after_hooks, LttvTraceHook, k);
        for(l = 0 ; l < hook->fac_list->len ; l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_remove_data(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf);
        }
      }
    }
    g_debug("lttv_stats_remove_event_hooks()");
    g_array_free(before_hooks, TRUE);
    g_array_free(after_hooks, TRUE);
  }
}


static void module_init()
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
  LTTV_STATS_USE_COUNT = g_quark_from_string("stats_use_count");
  LTTV_STATS = g_quark_from_string("statistics");
  LTTV_STATS_TRACEFILES = g_quark_from_string("tracefiles statistics");
  LTTV_STATS_SUMMED = g_quark_from_string("statistics summed");
}

static void module_destroy() 
{
}


LTTV_MODULE("stats", "Compute processes statistics", \
    "Accumulate statistics for event types, processes and CPUs", \
    module_init, module_destroy, "state");

/* Change the places where stats are called (create/read/write stats)

   Check for options in batchtest.c to reduce writing and see what tests are
   best candidates for performance analysis. Once OK, commit, move to main
   and run tests. Update the gui for statistics. */
