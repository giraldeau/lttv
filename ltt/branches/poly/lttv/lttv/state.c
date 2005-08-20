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

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/state.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <stdio.h>

#define PREALLOCATED_EXECUTION_STACK 10

/* Facilities Quarks */

GQuark
    LTT_FACILITY_KERNEL,
    LTT_FACILITY_PROCESS;

/* Events Quarks */

GQuark 
    LTT_EVENT_SYSCALL_ENTRY,
    LTT_EVENT_SYSCALL_EXIT,
    LTT_EVENT_TRAP_ENTRY,
    LTT_EVENT_TRAP_EXIT,
    LTT_EVENT_IRQ_ENTRY,
    LTT_EVENT_IRQ_EXIT,
    LTT_EVENT_SCHEDCHANGE,
    LTT_EVENT_FORK,
    LTT_EVENT_EXIT,
    LTT_EVENT_FREE;

/* Fields Quarks */

GQuark 
    LTT_FIELD_SYSCALL_ID,
    LTT_FIELD_TRAP_ID,
    LTT_FIELD_IRQ_ID,
    LTT_FIELD_OUT,
    LTT_FIELD_IN,
    LTT_FIELD_OUT_STATE,
    LTT_FIELD_PARENT_PID,
    LTT_FIELD_CHILD_PID,
    LTT_FIELD_PID;

LttvExecutionMode
  LTTV_STATE_MODE_UNKNOWN,
  LTTV_STATE_USER_MODE,
  LTTV_STATE_SYSCALL,
  LTTV_STATE_TRAP,
  LTTV_STATE_IRQ;

LttvExecutionSubmode
  LTTV_STATE_SUBMODE_UNKNOWN,
  LTTV_STATE_SUBMODE_NONE;

LttvProcessStatus
  LTTV_STATE_UNNAMED,
  LTTV_STATE_WAIT_FORK,
  LTTV_STATE_WAIT_CPU,
  LTTV_STATE_EXIT,
  LTTV_STATE_ZOMBIE,
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN;

static GQuark
  LTTV_STATE_TRACEFILES,
  LTTV_STATE_PROCESSES,
  LTTV_STATE_PROCESS,
  LTTV_STATE_EVENT,
  LTTV_STATE_SAVED_STATES,
  LTTV_STATE_SAVED_STATES_TIME,
  LTTV_STATE_TIME,
  LTTV_STATE_HOOKS,
  LTTV_STATE_NAME_TABLES,
  LTTV_STATE_TRACE_STATE_USE_COUNT;

static void create_max_time(LttvTraceState *tcs);

static void get_max_time(LttvTraceState *tcs);

static void free_max_time(LttvTraceState *tcs);

static void create_name_tables(LttvTraceState *tcs);

static void get_name_tables(LttvTraceState *tcs);

static void free_name_tables(LttvTraceState *tcs);

static void free_saved_state(LttvTraceState *tcs);

static void lttv_state_free_process_table(GHashTable *processes);


void lttv_state_save(LttvTraceState *self, LttvAttribute *container)
{
  LTTV_TRACE_STATE_GET_CLASS(self)->state_save(self, container);
}


void lttv_state_restore(LttvTraceState *self, LttvAttribute *container)
{
  LTTV_TRACE_STATE_GET_CLASS(self)->state_restore(self, container);
}


void lttv_state_state_saved_free(LttvTraceState *self, 
    LttvAttribute *container)
{
  LTTV_TRACE_STATE_GET_CLASS(self)->state_saved_free(self, container);
}


guint process_hash(gconstpointer key) 
{
  guint pid = ((const LttvProcessState *)key)->pid;
  return (pid>>8 ^ pid>>4 ^ pid>>2 ^ pid) ;
}


/* If the hash table hash function is well distributed,
 * the process_equal should compare different pid */
gboolean process_equal(gconstpointer a, gconstpointer b)
{
  const LttvProcessState *process_a, *process_b;
  gboolean ret = TRUE;
  
  process_a = (const LttvProcessState *)a;
  process_b = (const LttvProcessState *)b;
  
  if(likely(process_a->pid != process_b->pid)) ret = FALSE;
  else if(likely(process_a->pid == 0 && 
                 process_a->last_cpu != process_b->last_cpu)) ret = FALSE;

  return ret;
}


static void
restore_init_state(LttvTraceState *self)
{
  guint i, nb_tracefile;

  LttvTracefileState *tfcs;
  
  if(self->processes != NULL) lttv_state_free_process_table(self->processes);
  self->processes = g_hash_table_new(process_hash, process_equal);
  self->nb_event = 0;

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs =
      LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    ltt_trace_time_span_get(self->parent.t, &tfcs->parent.timestamp, NULL);
//    tfcs->saved_position = 0;
    tfcs->process = lttv_state_create_process(tfcs, NULL,0);
    tfcs->process->state->s = LTTV_STATE_RUN;
    tfcs->process->last_cpu = tfcs->cpu_name;
    tfcs->process->last_cpu_index = ((LttvTracefileContext*)tfcs)->index;
  }
}

static LttTime time_zero = {0,0};

static void
init(LttvTracesetState *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  LttvTracefileState *tfcs;
  
  LttvAttributeValue v;

  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      init((LttvTracesetContext *)self, ts);

  nb_trace = lttv_traceset_number(ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->parent.traces[i];
    tcs = (LttvTraceState *)tc;
    tcs->save_interval = LTTV_STATE_SAVE_INTERVAL;
    lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_TRACE_STATE_USE_COUNT, 
        LTTV_UINT, &v);
    (*v.v_uint)++;

    if(*(v.v_uint) == 1) {
      create_name_tables(tcs);
      create_max_time(tcs);
    }
    get_name_tables(tcs);
    get_max_time(tcs);

    nb_tracefile = tc->tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(tc->tracefiles,
                                          LttvTracefileContext*, j));
      tfcs->cpu_name = ltt_tracefile_name(tfcs->parent.tf);
    }
    tcs->processes = NULL;
    restore_init_state(tcs);
  }
}


static void
fini(LttvTracesetState *self)
{
  guint i, nb_trace;

  LttvTraceState *tcs;

  LttvTracefileState *tfcs;

  LttvAttributeValue v;

  nb_trace = lttv_traceset_number(LTTV_TRACESET_CONTEXT(self)->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceState *)(LTTV_TRACESET_CONTEXT(self)->traces[i]);
    lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_TRACE_STATE_USE_COUNT, 
        LTTV_UINT, &v);

    g_assert(*(v.v_uint) != 0);
    (*v.v_uint)--;

    if(*(v.v_uint) == 0) {
      free_name_tables(tcs);
      free_max_time(tcs);
      free_saved_state(tcs);
    }
    lttv_state_free_process_table(tcs->processes);
    tcs->processes = NULL;
  }
  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      fini((LttvTracesetContext *)self);
}


static LttvTracesetContext *
new_traceset_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT(g_object_new(LTTV_TRACESET_STATE_TYPE, NULL));
}


static LttvTraceContext * 
new_trace_context(LttvTracesetContext *self)
{
  return LTTV_TRACE_CONTEXT(g_object_new(LTTV_TRACE_STATE_TYPE, NULL));
}


static LttvTracefileContext *
new_tracefile_context(LttvTracesetContext *self)
{
  return LTTV_TRACEFILE_CONTEXT(g_object_new(LTTV_TRACEFILE_STATE_TYPE, NULL));
}


/* Write the process state of the trace */

static void write_process_state(gpointer key, gpointer value,
    gpointer user_data)
{
  LttvProcessState *process;

  LttvExecutionState *es;

  FILE *fp = (FILE *)user_data;

  guint i;

  process = (LttvProcessState *)value;
  fprintf(fp,
"  <PROCESS CORE=%p PID=%u PPID=%u CTIME_S=%lu CTIME_NS=%lu NAME=\"%s\" CPU=\"%s\">\n",
      process, process->pid, process->ppid, process->creation_time.tv_sec,
      process->creation_time.tv_nsec, g_quark_to_string(process->name),
      g_quark_to_string(process->last_cpu));

  for(i = 0 ; i < process->execution_stack->len; i++) {
    es = &g_array_index(process->execution_stack, LttvExecutionState, i);
    fprintf(fp, "    <ES MODE=\"%s\" SUBMODE=\"%s\" ENTRY_S=%lu ENTRY_NS=%lu",
	    g_quark_to_string(es->t), g_quark_to_string(es->n),
            es->entry.tv_sec, es->entry.tv_nsec);
    fprintf(fp, " CHANGE_S=%lu CHANGE_NS=%lu STATUS=\"%s\"/>\n",
            es->change.tv_sec, es->change.tv_nsec, g_quark_to_string(es->s)); 
  }
  fprintf(fp, "  </PROCESS>\n");
}


void lttv_state_write(LttvTraceState *self, LttTime t, FILE *fp)
{
  guint i, nb_tracefile, nb_block, offset;
  guint64 tsc;

  LttvTracefileState *tfcs;

  LttTracefile *tf;

  LttEventPosition *ep;

  ep = ltt_event_position_new();

  fprintf(fp,"<PROCESS_STATE TIME_S=%lu TIME_NS=%lu>\n", t.tv_sec, t.tv_nsec);

  g_hash_table_foreach(self->processes, write_process_state, fp);

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    fprintf(fp, "  <TRACEFILE PID=%u TIMESTAMP_S=%lu TIMESTAMP_NS=%lu", 
        tfcs->process->pid, tfcs->parent.timestamp.tv_sec, 
        tfcs->parent.timestamp.tv_nsec);
    LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
    if(e == NULL) fprintf(fp,"/>\n");
    else {
      ltt_event_position(e, ep);
      ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
      fprintf(fp, " BLOCK=%u OFFSET=%u TSC=%llu/>\n", nb_block, offset,
          tsc);
    }
  }
  g_free(ep);
  fprintf(fp,"</PROCESS_STATE>");
}


/* Copy each process from an existing hash table to a new one */

static void copy_process_state(gpointer key, gpointer value,gpointer user_data)
{
  LttvProcessState *process, *new_process;

  GHashTable *new_processes = (GHashTable *)user_data;

  guint i;

  process = (LttvProcessState *)value;
  new_process = g_new(LttvProcessState, 1);
  *new_process = *process;
  new_process->execution_stack = g_array_sized_new(FALSE, FALSE, 
      sizeof(LttvExecutionState), PREALLOCATED_EXECUTION_STACK);
  g_array_set_size(new_process->execution_stack,process->execution_stack->len);
  for(i = 0 ; i < process->execution_stack->len; i++) {
    g_array_index(new_process->execution_stack, LttvExecutionState, i) =
        g_array_index(process->execution_stack, LttvExecutionState, i);
  }
  new_process->state = &g_array_index(new_process->execution_stack, 
      LttvExecutionState, new_process->execution_stack->len - 1);
  g_hash_table_insert(new_processes, new_process, new_process);
}


static GHashTable *lttv_state_copy_process_table(GHashTable *processes)
{
  GHashTable *new_processes = g_hash_table_new(process_hash, process_equal);

  g_hash_table_foreach(processes, copy_process_state, new_processes);
  return new_processes;
}


/* The saved state for each trace contains a member "processes", which
   stores a copy of the process table, and a member "tracefiles" with
   one entry per tracefile. Each tracefile has a "process" member pointing
   to the current process and a "position" member storing the tracefile
   position (needed to seek to the current "next" event. */

static void state_save(LttvTraceState *self, LttvAttribute *container)
{
  guint i, nb_tracefile;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  LttEventPosition *ep;

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);

  value = lttv_attribute_add(container, LTTV_STATE_PROCESSES,
      LTTV_POINTER);
  *(value.v_pointer) = lttv_state_copy_process_table(self->processes);

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    tracefile_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    value = lttv_attribute_add(tracefiles_tree, i, 
        LTTV_GOBJECT);
    *(value.v_gobject) = (GObject *)tracefile_tree;
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_PROCESS, 
        LTTV_UINT);
    *(value.v_uint) = tfcs->process->pid;
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_EVENT, 
        LTTV_POINTER);
    LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
    if(e == NULL) *(value.v_pointer) = NULL;
    else {
      ep = ltt_event_position_new();
      ltt_event_position(e, ep);
      *(value.v_pointer) = ep;

      guint nb_block, offset;
      guint64 tsc;
      LttTracefile *tf;
      ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
      g_debug("Block %u offset %u tsc %llu time %lu.%lu", nb_block, offset,
          tsc,
          tfcs->parent.timestamp.tv_sec, tfcs->parent.timestamp.tv_nsec);
    }
  }
}


static void state_restore(LttvTraceState *self, LttvAttribute *container)
{
  guint i, nb_tracefile, pid;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  LttEventPosition *ep;

  LttvTracesetContext *tsc = self->parent.ts_context;

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);

  type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES, 
      &value);
  g_assert(type == LTTV_POINTER);
  lttv_state_free_process_table(self->processes);
  self->processes = lttv_state_copy_process_table(*(value.v_pointer));

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    type = lttv_attribute_get(tracefiles_tree, i, &name, &value);
    g_assert(type == LTTV_GOBJECT);
    tracefile_tree = *((LttvAttribute **)(value.v_gobject));

    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_PROCESS, 
        &value);
    g_assert(type == LTTV_UINT);
    pid = *(value.v_uint);
    tfcs->process = lttv_state_find_process_or_create(tfcs, pid);

    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT, 
        &value);
    g_assert(type == LTTV_POINTER);
    g_assert(*(value.v_pointer) != NULL);
    ep = *(value.v_pointer);
    g_assert(tfcs->parent.t_context != NULL);
    
    g_tree_destroy(tsc->pqueue);
    tsc->pqueue = g_tree_new(compare_tracefile);
    
    LttvTracefileContext *tfc = LTTV_TRACEFILE_CONTEXT(tfcs);
    
    g_assert(ltt_tracefile_seek_position(tfc->tf, ep) == 0);
    tfc->timestamp = ltt_event_time(ltt_tracefile_get_event(tfc->tf));
    g_tree_insert(tsc->pqueue, tfc, tfc);
  }
}


static void state_saved_free(LttvTraceState *self, LttvAttribute *container)
{
  guint i, nb_tracefile;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  LttEventPosition *ep;

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);
  g_object_ref(G_OBJECT(tracefiles_tree));
  lttv_attribute_remove_by_name(container, LTTV_STATE_TRACEFILES);

  type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES, 
      &value);
  g_assert(type == LTTV_POINTER);
  lttv_state_free_process_table(*(value.v_pointer));
  *(value.v_pointer) = NULL;
  lttv_attribute_remove_by_name(container, LTTV_STATE_PROCESSES);

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    type = lttv_attribute_get(tracefiles_tree, i, &name, &value);
    g_assert(type == LTTV_GOBJECT);
    tracefile_tree = *((LttvAttribute **)(value.v_gobject));

    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT, 
        &value);
    g_assert(type == LTTV_POINTER);
    if(*(value.v_pointer) != NULL) g_free(*(value.v_pointer));
  }
  g_object_unref(G_OBJECT(tracefiles_tree));
}


static void free_saved_state(LttvTraceState *self)
{
  guint i, nb;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  LttvAttribute *saved_states;

  saved_states = lttv_attribute_find_subdir(self->parent.t_a,
      LTTV_STATE_SAVED_STATES);

  nb = lttv_attribute_get_number(saved_states);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(saved_states, i, &name, &value);
    g_assert(type == LTTV_GOBJECT);
    state_saved_free(self, *((LttvAttribute **)value.v_gobject));
  }

  lttv_attribute_remove_by_name(self->parent.t_a, LTTV_STATE_SAVED_STATES);
}


static void 
create_max_time(LttvTraceState *tcs) 
{
  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_SAVED_STATES_TIME, 
        LTTV_POINTER, &v);
  g_assert(*(v.v_pointer) == NULL);
  *(v.v_pointer) = g_new(LttTime,1);
  *((LttTime *)*(v.v_pointer)) = time_zero;
}


static void 
get_max_time(LttvTraceState *tcs) 
{
  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_SAVED_STATES_TIME, 
        LTTV_POINTER, &v);
  g_assert(*(v.v_pointer) != NULL);
  tcs->max_time_state_recomputed_in_seek = (LttTime *)*(v.v_pointer);
}


static void 
free_max_time(LttvTraceState *tcs) 
{
  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_SAVED_STATES_TIME, 
        LTTV_POINTER, &v);
  g_free(*(v.v_pointer));
  *(v.v_pointer) = NULL;
}


typedef struct _LttvNameTables {
 // FIXME  GQuark *eventtype_names;
  GQuark *syscall_names;
  GQuark *trap_names;
  GQuark *irq_names;
} LttvNameTables;


static void 
create_name_tables(LttvTraceState *tcs) 
{
  int i, nb;

  GQuark f_name, e_name;

  LttvTraceHook *h;

  LttvTraceHookByFacility *thf;

  LttEventType *et;

  LttType *t;

  GString *fe_name = g_string_new("");

  LttvNameTables *name_tables = g_new(LttvNameTables, 1);

  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_NAME_TABLES, 
      LTTV_POINTER, &v);
  g_assert(*(v.v_pointer) == NULL);
  *(v.v_pointer) = name_tables;
#if 0 // Use iteration over the facilities_by_name and then list all event
      // types of each facility
  nb = ltt_trace_eventtype_number(tcs->parent.t);
  name_tables->eventtype_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    et = ltt_trace_eventtype_get(tcs->parent.t, i);
    e_name = ltt_eventtype_name(et);
    f_name = ltt_facility_name(ltt_eventtype_facility(et));
    g_string_printf(fe_name, "%s.%s", f_name, e_name);
    name_tables->eventtype_names[i] = g_quark_from_string(fe_name->str);    
  }
#endif //0
  if(lttv_trace_find_hook(tcs->parent.t,
      LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_ENTRY,
      LTT_FIELD_SYSCALL_ID, 0, 0,
      NULL, h))
    return;
  
  thf = lttv_trace_hook_get_first(h);
  
  t = ltt_field_type(thf->f1);
  nb = ltt_type_element_number(t);
  
  lttv_trace_hook_destroy(h);

  /* CHECK syscalls should be an enum but currently are not!  
  name_tables->syscall_names = g_new(GQuark, nb);

  for(i = 0 ; i < nb ; i++) {
    name_tables->syscall_names[i] = g_quark_from_string(
        ltt_enum_string_get(t, i));
  }
  */

  name_tables->syscall_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "syscall %d", i);
    name_tables->syscall_names[i] = g_quark_from_string(fe_name->str);
  }

  if(lttv_trace_find_hook(tcs->parent.t, LTT_FACILITY_KERNEL,
        LTT_EVENT_TRAP_ENTRY,
        LTT_FIELD_TRAP_ID, 0, 0,
        NULL, h))
    return;

  thf = lttv_trace_hook_get_first(h);

  t = ltt_field_type(thf->f1);
  nb = ltt_type_element_number(t);

  lttv_trace_hook_destroy(h);

  /*
  name_tables->trap_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    name_tables->trap_names[i] = g_quark_from_string(
        ltt_enum_string_get(t, i));
  }
  */

  name_tables->trap_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "trap %d", i);
    name_tables->trap_names[i] = g_quark_from_string(fe_name->str);
  }

  if(lttv_trace_find_hook(tcs->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
        LTT_FIELD_IRQ_ID, 0, 0,
        NULL, h))
    return;
  
  thf = lttv_trace_hook_get_first(h);
  
  t = ltt_field_type(thf->f1);
  nb = ltt_type_element_number(t);

  lttv_trace_hook_destroy(h);

  /*
  name_tables->irq_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    name_tables->irq_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
  }
  */

  name_tables->irq_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "irq %d", i);
    name_tables->irq_names[i] = g_quark_from_string(fe_name->str);
  }

  g_string_free(fe_name, TRUE);
}


static void 
get_name_tables(LttvTraceState *tcs) 
{
  LttvNameTables *name_tables;

  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_NAME_TABLES, 
        LTTV_POINTER, &v);
  g_assert(*(v.v_pointer) != NULL);
  name_tables = (LttvNameTables *)*(v.v_pointer);
  //tcs->eventtype_names = name_tables->eventtype_names;
  tcs->syscall_names = name_tables->syscall_names;
  tcs->trap_names = name_tables->trap_names;
  tcs->irq_names = name_tables->irq_names;
}


static void 
free_name_tables(LttvTraceState *tcs) 
{
  LttvNameTables *name_tables;

  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_NAME_TABLES, 
        LTTV_POINTER, &v);
  name_tables = (LttvNameTables *)*(v.v_pointer);
  *(v.v_pointer) = NULL;

 // g_free(name_tables->eventtype_names);
  g_free(name_tables->syscall_names);
  g_free(name_tables->trap_names);
  g_free(name_tables->irq_names);
  g_free(name_tables);
} 


static void push_state(LttvTracefileState *tfs, LttvExecutionMode t, 
    guint state_id)
{
  LttvExecutionState *es;

  LttvProcessState *process = tfs->process;

  guint depth = process->execution_stack->len;

  g_array_set_size(process->execution_stack, depth + 1);
  es = &g_array_index(process->execution_stack, LttvExecutionState, depth);
  es->t = t;
  es->n = state_id;
  es->entry = es->change = tfs->parent.timestamp;
  es->s = process->state->s;
  process->state = es;
}


static void pop_state(LttvTracefileState *tfs, LttvExecutionMode t)
{
  LttvProcessState *process = tfs->process;

  guint depth = process->execution_stack->len;

  if(process->state->t != t){
    g_info("Different execution mode type (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    g_info("process state has %s when pop_int is %s\n",
		    g_quark_to_string(process->state->t),
		    g_quark_to_string(t));
    g_info("{ %u, %u, %s, %s }\n",
		    process->pid,
		    process->ppid,
		    g_quark_to_string(process->name),
		    g_quark_to_string(process->state->s));
    return;
  }

  if(depth == 1){
    g_info("Trying to pop last state on stack (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    return;
  }

  g_array_set_size(process->execution_stack, depth - 1);
  process->state = &g_array_index(process->execution_stack, LttvExecutionState,
      depth - 2);
  process->state->change = tfs->parent.timestamp;
}


LttvProcessState *
lttv_state_create_process(LttvTracefileState *tfs, LttvProcessState *parent, 
    guint pid)
{
  LttvProcessState *process = g_new(LttvProcessState, 1);

  LttvExecutionState *es;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  char buffer[128];

  tc = tfs->parent.t_context;
  tcs = (LttvTraceState *)tc;
	
  process->pid = pid;
  process->last_cpu = tfs->cpu_name;
  process->last_cpu_index = ((LttvTracefileContext*)tfs)->index;
  g_info("Process %u, core %p", process->pid, process);
  g_hash_table_insert(tcs->processes, process, process);

  if(parent) {
    process->ppid = parent->pid;
    process->name = parent->name;
    process->creation_time = tfs->parent.timestamp;
  }

  /* No parent. This process exists but we are missing all information about
     its creation. The birth time is set to zero but we remember the time of
     insertion */

  else {
    process->ppid = 0;
    process->name = LTTV_STATE_UNNAMED;
    process->creation_time = ltt_time_zero;
  }

  process->insertion_time = tfs->parent.timestamp;
  sprintf(buffer,"%d-%lu.%lu",pid, process->creation_time.tv_sec, 
	  process->creation_time.tv_nsec);
  process->pid_time = g_quark_from_string(buffer);
  process->last_cpu = tfs->cpu_name;
  process->last_cpu_index = ((LttvTracefileContext*)tfs)->index;
  process->execution_stack = g_array_sized_new(FALSE, FALSE, 
      sizeof(LttvExecutionState), PREALLOCATED_EXECUTION_STACK);
  g_array_set_size(process->execution_stack, 1);
  es = process->state = &g_array_index(process->execution_stack, 
      LttvExecutionState, 0);
  es->t = LTTV_STATE_USER_MODE;
  es->n = LTTV_STATE_SUBMODE_NONE;
  es->entry = tfs->parent.timestamp;
  g_assert(tfs->parent.timestamp.tv_sec != 0);
  es->change = tfs->parent.timestamp;
  es->s = LTTV_STATE_WAIT_FORK;

  return process;
}

LttvProcessState *lttv_state_find_process(LttvTracefileState *tfs, 
    guint pid)
{
  LttvProcessState key;
  LttvProcessState *process;

  LttvTraceState* ts = (LttvTraceState*)tfs->parent.t_context;

  key.pid = pid;
  key.last_cpu = tfs->cpu_name;
  process = g_hash_table_lookup(ts->processes, &key);
  return process;
}

LttvProcessState *
lttv_state_find_process_or_create(LttvTracefileState *tfs, guint pid)
{
  LttvProcessState *process = lttv_state_find_process(tfs, pid);

  if(unlikely(process == NULL)) process = lttv_state_create_process(tfs, NULL, pid);
  return process;
}

/* FIXME : this function should be called when we receive an event telling that
 * release_task has been called in the kernel. In happens generally when
 * the parent waits for its child terminaison, but may also happen in special
 * cases in the child's exit : when the parent ignores its children SIGCCHLD or
 * has the flag SA_NOCLDWAIT. It can also happen when the child is part
 * of a killed thread ground, but isn't the leader.
 */
static void exit_process(LttvTracefileState *tfs, LttvProcessState *process) 
{
  LttvTraceState *ts = LTTV_TRACE_STATE(tfs->parent.t_context);
  LttvProcessState key;

  key.pid = process->pid;
  key.last_cpu = process->last_cpu;
  g_hash_table_remove(ts->processes, &key);
  g_array_free(process->execution_stack, TRUE);
  g_free(process);
}


static void free_process_state(gpointer key, gpointer value,gpointer user_data)
{
  g_array_free(((LttvProcessState *)value)->execution_stack, TRUE);
  g_free(value);
}


static void lttv_state_free_process_table(GHashTable *processes)
{
  g_hash_table_foreach(processes, free_process_state, NULL);
  g_hash_table_destroy(processes);
}


static gboolean syscall_entry(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->syscall_names[
      ltt_event_get_unsigned(e, f)];
  push_state(s, LTTV_STATE_SYSCALL, submode);
  return FALSE;
}


static gboolean syscall_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_SYSCALL);
  return FALSE;
}


static gboolean trap_entry(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->trap_names[
      ltt_event_get_unsigned(e, f)];
  push_state(s, LTTV_STATE_TRAP, submode);
  return FALSE;
}


static gboolean trap_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_TRAP);
  return FALSE;
}


static gboolean irq_entry(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->irq_names[
      ltt_event_get_unsigned(e, f)];

  /* Do something with the info about being in user or system mode when int? */
  push_state(s, LTTV_STATE_IRQ, submode);
  return FALSE;
}


static gboolean irq_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_IRQ);
  return FALSE;
}


static gboolean schedchange(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  guint pid_in, pid_out, state_out;

  pid_out = ltt_event_get_unsigned(e, thf->f1);
  pid_in = ltt_event_get_unsigned(e, thf->f2);
  state_out = ltt_event_get_unsigned(e, thf->f3);

  if(likely(s->process != NULL)) {

    /* We could not know but it was not the idle process executing.
       This should only happen at the beginning, before the first schedule
       event, and when the initial information (current process for each CPU)
       is missing. It is not obvious how we could, after the fact, compensate
       the wrongly attributed statistics. */

    if(unlikely(s->process->pid != pid_out)) {
      g_assert(s->process->pid == 0);
    }

    if(unlikely(s->process->state->s == LTTV_STATE_EXIT)) {
      s->process->state->s = LTTV_STATE_ZOMBIE;
    } else {
      if(unlikely(state_out == 0)) s->process->state->s = LTTV_STATE_WAIT_CPU;
      else s->process->state->s = LTTV_STATE_WAIT;
    } /* FIXME : we do not remove process here, because the kernel
       * still has them : they may be zombies. We need to know
       * exactly when release_task is executed on the PID to 
       * know when the zombie is destroyed.
       */
    //else
    //  exit_process(s, s->process);

    s->process->state->change = s->parent.timestamp;
  }
  s->process = lttv_state_find_process_or_create(s, pid_in);
  s->process->state->s = LTTV_STATE_RUN;
  s->process->last_cpu = s->cpu_name;
  s->process->last_cpu_index = ((LttvTracefileContext*)s)->index;
  s->process->state->change = s->parent.timestamp;
  return FALSE;
}

static gboolean process_fork(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  LttField *f;
  guint parent_pid;
  guint child_pid;
  LttvProcessState *zombie_process;

  /* Parent PID */
  f = thf->f1;
  parent_pid = ltt_event_get_unsigned(e, f);

  /* Child PID */
  f = thf->f2;
  child_pid = ltt_event_get_unsigned(e, f);

  zombie_process = lttv_state_find_process(s, child_pid);

  if(unlikely(zombie_process != NULL)) {
    /* Reutilisation of PID. Only now we are sure that the old PID
     * has been released. FIXME : should know when release_task happens instead.
     */
    exit_process(s, zombie_process);
  }
  g_assert(s->process->pid != child_pid);
  // FIXME : Add this test in the "known state" section
  // g_assert(s->process->pid == parent_pid);
  lttv_state_create_process(s, s->process, child_pid);

  return FALSE;
}


static gboolean process_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  LttField *f;
  guint pid;

  pid = ltt_event_get_unsigned(e, thf->f1);

  // FIXME : Add this test in the "known state" section
  // g_assert(s->process->pid == pid);

  if(likely(s->process != NULL)) {
    s->process->state->s = LTTV_STATE_EXIT;
  }
  return FALSE;
}

static gboolean process_free(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf =
        lttv_trace_hook_get_fac((LttvTraceHook *)hook_data, 
                                 ltt_event_facility_id(e));
  guint release_pid;
  LttvProcessState *process;

  /* PID of the process to release */
  release_pid = ltt_event_get_unsigned(e, thf->f1);

  process = lttv_state_find_process(s, release_pid);

  if(likely(process != NULL)) {
    /* release_task is happening at kernel level : we can now safely release
     * the data structure of the process */
    exit_process(s, process);
  }

  return FALSE;
}

gint lttv_state_hook_add_event_hooks(void *hook_data, void *call_data)
{
  LttvTracesetState *tss = (LttvTracesetState*)(call_data);

  lttv_state_add_event_hooks(tss);

  return 0;
}

void lttv_state_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  GArray *hooks;

  LttvTraceHookByFacility *thf;
  
  LttvTraceHook *hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 10);
    g_array_set_size(hooks, 10);

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_ENTRY,
        LTT_FIELD_SYSCALL_ID, 0, 0,
        syscall_entry, &g_array_index(hooks, LttvTraceHook, 0));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SYSCALL_EXIT,
        0, 0, 0,
        syscall_exit, &g_array_index(hooks, LttvTraceHook, 1));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_ENTRY,
        LTT_FIELD_TRAP_ID, 0, 0,
        trap_entry, &g_array_index(hooks, LttvTraceHook, 2));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_EXIT,
        0, 0, 0, 
        trap_exit, &g_array_index(hooks, LttvTraceHook, 3));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
        LTT_FIELD_IRQ_ID, 0, 0,
        irq_entry, &g_array_index(hooks, LttvTraceHook, 4));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
        0, 0, 0, 
        irq_exit, &g_array_index(hooks, LttvTraceHook, 5));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_SCHEDCHANGE,
        LTT_FIELD_OUT, LTT_FIELD_IN, LTT_FIELD_OUT_STATE,
        schedchange, &g_array_index(hooks, LttvTraceHook, 6));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FORK,
        LTT_FIELD_PARENT_PID, LTT_FIELD_CHILD_PID, 0,
        process_fork, &g_array_index(hooks, LttvTraceHook, 7));

    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_EXIT,
        LTT_FIELD_PID, 0, 0,
        process_exit, &g_array_index(hooks, LttvTraceHook, 8));
    
    lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FREE,
        LTT_FIELD_PID, 0, 0,
        process_free, &g_array_index(hooks, LttvTraceHook, 9));


    /* Add these hooks to each event_by_id hooks list */

    nb_tracefile = ts->parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(&g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext, j));

      for(k = 0 ; k < hooks->len ; k++) {
        hook = &g_array_index(hooks, LttvTraceHook, k);
        for(l=0;l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_add(
            lttv_hooks_by_id_find(tfs->parent.event_by_id, thf->id),
            thf->h,
            &g_array_index(hooks, LttvTraceHook, k),
            LTTV_PRIO_STATE);
        }
      }
    }
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    *(val.v_pointer) = hooks;
  }
}

gint lttv_state_hook_remove_event_hooks(void *hook_data, void *call_data)
{
  LttvTracesetState *tss = (LttvTracesetState*)(call_data);

  lttv_state_remove_event_hooks(tss);

  return 0;
}

void lttv_state_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  GArray *hooks;

  LttvTraceHook *hook;
  
  LttvTraceHookByFacility *thf;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATE(self->parent.traces[i]);
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    hooks = *(val.v_pointer);

    /* Remove these hooks from each event_by_id hooks list */

    nb_tracefile = ts->parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext*, j));

      for(k = 0 ; k < hooks->len ; k++) {
        hook = &g_array_index(hooks, LttvTraceHook, k);
        for(l=0;l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          
          lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.event_by_id, thf->id),
                    thf->h,
                    &g_array_index(hooks, LttvTraceHook, k));
        }
        lttv_trace_hook_destroy(&g_array_index(hooks, LttvTraceHook, k));
      }
    }
    g_array_free(hooks, TRUE);
  }
}


static gboolean state_save_event_hook(void *hook_data, void *call_data)
{
  guint *event_count = (guint*)hook_data;

  /* Only save at LTTV_STATE_SAVE_INTERVAL */
  if(likely((*event_count)++ < LTTV_STATE_SAVE_INTERVAL))
    return FALSE;
  else
    event_count = 0;
  
  LttvTracefileState *self = (LttvTracefileState *)call_data;

  LttvTracefileState *tfcs;

  LttvTraceState *tcs = (LttvTraceState *)(self->parent.t_context);

  LttEventPosition *ep;

  guint i;

  LttTracefile *tf;

  LttvAttribute *saved_states_tree, *saved_state_tree;

  LttvAttributeValue value;

  saved_states_tree = lttv_attribute_find_subdir(tcs->parent.t_a, 
      LTTV_STATE_SAVED_STATES);
  saved_state_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  value = lttv_attribute_add(saved_states_tree, 
      lttv_attribute_get_number(saved_states_tree), LTTV_GOBJECT);
  *(value.v_gobject) = (GObject *)saved_state_tree;
  value = lttv_attribute_add(saved_state_tree, LTTV_STATE_TIME, LTTV_TIME);
  *(value.v_time) = self->parent.timestamp;
  lttv_state_save(tcs, saved_state_tree);
  g_debug("Saving state at time %lu.%lu", self->parent.timestamp.tv_sec,
    self->parent.timestamp.tv_nsec);

  *(tcs->max_time_state_recomputed_in_seek) = self->parent.timestamp;

  return FALSE;
}

#if 0
static gboolean block_start(void *hook_data, void *call_data)
{
  LttvTracefileState *self = (LttvTracefileState *)call_data;

  LttvTracefileState *tfcs;

  LttvTraceState *tcs = (LttvTraceState *)(self->parent.t_context);

  LttEventPosition *ep;

  guint i, nb_block, nb_event, nb_tracefile;

  LttTracefile *tf;

  LttvAttribute *saved_states_tree, *saved_state_tree;

  LttvAttributeValue value;

  ep = ltt_event_position_new();

  nb_tracefile = tcs->parent.tracefiles->len;

  /* Count the number of events added since the last block end in any
     tracefile. */

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(&g_array_index(tcs->parent.tracefiles,
                                          LttvTracefileContext, i));
    ltt_event_position(tfcs->parent.e, ep);
    ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
    tcs->nb_event += nb_event - tfcs->saved_position;
    tfcs->saved_position = nb_event;
  }
  g_free(ep);

  if(tcs->nb_event >= tcs->save_interval) {
    saved_states_tree = lttv_attribute_find_subdir(tcs->parent.t_a, 
        LTTV_STATE_SAVED_STATES);
    saved_state_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    value = lttv_attribute_add(saved_states_tree, 
        lttv_attribute_get_number(saved_states_tree), LTTV_GOBJECT);
    *(value.v_gobject) = (GObject *)saved_state_tree;
    value = lttv_attribute_add(saved_state_tree, LTTV_STATE_TIME, LTTV_TIME);
    *(value.v_time) = self->parent.timestamp;
    lttv_state_save(tcs, saved_state_tree);
    tcs->nb_event = 0;
    g_debug("Saving state at time %lu.%lu", self->parent.timestamp.tv_sec,
	    self->parent.timestamp.tv_nsec);
  }
  *(tcs->max_time_state_recomputed_in_seek) = self->parent.timestamp;
  return FALSE;
}
#endif //0

#if 0
static gboolean block_end(void *hook_data, void *call_data)
{
  LttvTracefileState *self = (LttvTracefileState *)call_data;

  LttvTraceState *tcs = (LttvTraceState *)(self->parent.t_context);

  LttTracefile *tf;

  LttEventPosition *ep;

  guint nb_block, nb_event;

  ep = ltt_event_position_new();
  ltt_event_position(self->parent.e, ep);
  ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
  tcs->nb_event += nb_event - self->saved_position + 1;
  self->saved_position = 0;
  *(tcs->max_time_state_recomputed_in_seek) = self->parent.timestamp;
  g_free(ep);

  return FALSE;
}
#endif //0
#if 0
void lttv_state_save_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  LttvTraceHook hook_start, hook_end;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    lttv_trace_find_hook(ts->parent.t, "core","block_start",NULL, 
	NULL, NULL, block_start, &hook_start);
    lttv_trace_find_hook(ts->parent.t, "core","block_end",NULL, 
	NULL, NULL, block_end, &hook_end);

    nb_tracefile = ts->parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(&g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext, j));
      lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id, 
      	        hook_start.id), hook_start.h, NULL, LTTV_PRIO_STATE);
      lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id, 
	              hook_end.id), hook_end.h, NULL, LTTV_PRIO_STATE);
    }
  }
}
#endif //0

void lttv_state_save_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

 
  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {

    ts = (LttvTraceState *)self->parent.traces[i];
    nb_tracefile = ts->parent.tracefiles->len;

    guint *event_count = g_new(guint, 1);
    *event_count = 0;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext*, j));
      lttv_hooks_add(tfs->parent.event,
                     state_save_event_hook,
                     event_count,
                     LTTV_PRIO_STATE);

    }
  }
}

gint lttv_state_save_hook_add_event_hooks(void *hook_data, void *call_data)
{
  LttvTracesetState *tss = (LttvTracesetState*)(call_data);

  lttv_state_save_add_event_hooks(tss);

  return 0;
}


#if 0
void lttv_state_save_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  LttvTraceHook hook_start, hook_end;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATE(self->parent.traces[i]);

    lttv_trace_find_hook(ts->parent.t, "core","block_start",NULL, 
	NULL, NULL, block_start, &hook_start);

    lttv_trace_find_hook(ts->parent.t, "core","block_end",NULL, 
	NULL, NULL, block_end, &hook_end);

    nb_tracefile = ts->parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(&g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext, j));
      lttv_hooks_remove_data(lttv_hooks_by_id_find(
          tfs->parent.event_by_id, hook_start.id), hook_start.h, NULL);
      lttv_hooks_remove_data(lttv_hooks_by_id_find(
          tfs->parent.event_by_id, hook_end.id), hook_end.h, NULL);
    }
  }
}
#endif //0

void lttv_state_save_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

 
  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {

    ts = (LttvTraceState *)self->parent.traces[i];
    nb_tracefile = ts->parent.tracefiles->len;

    guint *event_count;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext*, j));
      event_count = lttv_hooks_remove(tfs->parent.event,
                        state_save_event_hook);
      g_free(event_count);

    }
  }
}

gint lttv_state_save_hook_remove_event_hooks(void *hook_data, void *call_data)
{
  LttvTracesetState *tss = (LttvTracesetState*)(call_data);

  lttv_state_save_remove_event_hooks(tss);

  return 0;
}

void lttv_state_traceset_seek_time_closest(LttvTracesetState *self, LttTime t)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, nb_trace;

  int min_pos, mid_pos, max_pos;

  LttvTraceState *tcs;

  LttvAttributeValue value;

  LttvAttributeType type;

  LttvAttributeName name;

  LttvAttribute *saved_states_tree, *saved_state_tree, *closest_tree;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceState *)self->parent.traces[i];

    if(ltt_time_compare(t, *(tcs->max_time_state_recomputed_in_seek)) < 0) {
      saved_states_tree = lttv_attribute_find_subdir(tcs->parent.t_a,
          LTTV_STATE_SAVED_STATES);
      min_pos = -1;

      if(saved_states_tree) {
        max_pos = lttv_attribute_get_number(saved_states_tree) - 1;
        mid_pos = max_pos / 2;
        while(min_pos < max_pos) {
          type = lttv_attribute_get(saved_states_tree, mid_pos, &name, &value);
          g_assert(type == LTTV_GOBJECT);
          saved_state_tree = *((LttvAttribute **)(value.v_gobject));
          type = lttv_attribute_get_by_name(saved_state_tree, LTTV_STATE_TIME, 
              &value);
          g_assert(type == LTTV_TIME);
          if(ltt_time_compare(*(value.v_time), t) < 0) {
            min_pos = mid_pos;
            closest_tree = saved_state_tree;
          }
          else max_pos = mid_pos - 1;

          mid_pos = (min_pos + max_pos + 1) / 2;
        }
      }

      /* restore the closest earlier saved state */
      if(min_pos != -1) {
        lttv_state_restore(tcs, closest_tree);
      }

      /* There is no saved state, yet we want to have it. Restart at T0 */
      else {
        restore_init_state(tcs);
        lttv_process_trace_seek_time(&(tcs->parent), ltt_time_zero);
      }
    }
    /* We want to seek quickly without restoring/updating the state */
    else {
      restore_init_state(tcs);
      lttv_process_trace_seek_time(&(tcs->parent), t);
    }
  }
}


static void
traceset_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
traceset_state_finalize (LttvTracesetState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
traceset_state_class_init (LttvTracesetContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) traceset_state_finalize;
  klass->init = (void (*)(LttvTracesetContext *self, LttvTraceset *ts))init;
  klass->fini = (void (*)(LttvTracesetContext *self))fini;
  klass->new_traceset_context = new_traceset_context;
  klass->new_trace_context = new_trace_context;
  klass->new_tracefile_context = new_tracefile_context;
}


GType 
lttv_traceset_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracesetStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) traceset_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracesetState),
      0,      /* n_preallocs */
      (GInstanceInitFunc) traceset_state_instance_init,    /* instance_init */
      NULL    /* value handling */
    };

    type = g_type_register_static (LTTV_TRACESET_CONTEXT_TYPE, "LttvTracesetStateType", 
        &info, 0);
  }
  return type;
}


static void
trace_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
trace_state_finalize (LttvTraceState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACE_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
trace_state_class_init (LttvTraceStateClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) trace_state_finalize;
  klass->state_save = state_save;
  klass->state_restore = state_restore;
  klass->state_saved_free = state_saved_free;
}


GType 
lttv_trace_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTraceStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) trace_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTraceState),
      0,      /* n_preallocs */
      (GInstanceInitFunc) trace_state_instance_init,    /* instance_init */
      NULL    /* value handling */
    };

    type = g_type_register_static (LTTV_TRACE_CONTEXT_TYPE, 
        "LttvTraceStateType", &info, 0);
  }
  return type;
}


static void
tracefile_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
tracefile_state_finalize (LttvTracefileState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACEFILE_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
tracefile_state_class_init (LttvTracefileStateClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) tracefile_state_finalize;
}


GType 
lttv_tracefile_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracefileStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) tracefile_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracefileState),
      0,      /* n_preallocs */
      (GInstanceInitFunc) tracefile_state_instance_init,    /* instance_init */
      NULL    /* value handling */
    };

    type = g_type_register_static (LTTV_TRACEFILE_CONTEXT_TYPE, 
        "LttvTracefileStateType", &info, 0);
  }
  return type;
}


static void module_init()
{
  LTTV_STATE_UNNAMED = g_quark_from_string("unnamed");
  LTTV_STATE_MODE_UNKNOWN = g_quark_from_string("unknown execution mode");
  LTTV_STATE_USER_MODE = g_quark_from_string("user mode");
  LTTV_STATE_WAIT_FORK = g_quark_from_string("wait fork");
  LTTV_STATE_SYSCALL = g_quark_from_string("system call");
  LTTV_STATE_TRAP = g_quark_from_string("trap");
  LTTV_STATE_IRQ = g_quark_from_string("irq");
  LTTV_STATE_SUBMODE_UNKNOWN = g_quark_from_string("unknown submode");
  LTTV_STATE_SUBMODE_NONE = g_quark_from_string("(no submode)");
  LTTV_STATE_WAIT_CPU = g_quark_from_string("wait for cpu");
  LTTV_STATE_EXIT = g_quark_from_string("exiting");
  LTTV_STATE_ZOMBIE = g_quark_from_string("zombie");
  LTTV_STATE_WAIT = g_quark_from_string("wait for I/O");
  LTTV_STATE_RUN = g_quark_from_string("running");
  LTTV_STATE_TRACEFILES = g_quark_from_string("tracefiles");
  LTTV_STATE_PROCESSES = g_quark_from_string("processes");
  LTTV_STATE_PROCESS = g_quark_from_string("process");
  LTTV_STATE_EVENT = g_quark_from_string("event");
  LTTV_STATE_SAVED_STATES = g_quark_from_string("saved states");
  LTTV_STATE_SAVED_STATES_TIME = g_quark_from_string("saved states time");
  LTTV_STATE_TIME = g_quark_from_string("time");
  LTTV_STATE_HOOKS = g_quark_from_string("saved state hooks");
  LTTV_STATE_NAME_TABLES = g_quark_from_string("name tables");
  LTTV_STATE_TRACE_STATE_USE_COUNT = 
      g_quark_from_string("trace_state_use_count");

  
  LTT_FACILITY_KERNEL     = g_quark_from_string("kernel");
  LTT_FACILITY_PROCESS    = g_quark_from_string("process");

  
  LTT_EVENT_SYSCALL_ENTRY = g_quark_from_string("syscall_entry");
  LTT_EVENT_SYSCALL_EXIT  = g_quark_from_string("syscall_exit");
  LTT_EVENT_TRAP_ENTRY    = g_quark_from_string("trap_entry");
  LTT_EVENT_TRAP_EXIT     = g_quark_from_string("trap_exit");
  LTT_EVENT_IRQ_ENTRY     = g_quark_from_string("irq_entry");
  LTT_EVENT_IRQ_EXIT      = g_quark_from_string("irq_exit");
  LTT_EVENT_SCHEDCHANGE   = g_quark_from_string("schedchange");
  LTT_EVENT_FORK          = g_quark_from_string("fork");
  LTT_EVENT_EXIT          = g_quark_from_string("exit");
  LTT_EVENT_FREE          = g_quark_from_string("free");


  LTT_FIELD_SYSCALL_ID    = g_quark_from_string("syscall_id");
  LTT_FIELD_TRAP_ID       = g_quark_from_string("trap_id");
  LTT_FIELD_IRQ_ID        = g_quark_from_string("irq_id");
  LTT_FIELD_OUT           = g_quark_from_string("out");
  LTT_FIELD_IN            = g_quark_from_string("in");
  LTT_FIELD_OUT_STATE     = g_quark_from_string("out_state");
  LTT_FIELD_PARENT_PID    = g_quark_from_string("parent_pid");
  LTT_FIELD_CHILD_PID     = g_quark_from_string("child_pid");
  LTT_FIELD_PID           = g_quark_from_string("pid");
  
}

static void module_destroy() 
{
}


LTTV_MODULE("state", "State computation", \
    "Update the system state, possibly saving it at intervals", \
    module_init, module_destroy)



