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


#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/state.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <stdio.h>

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


void lttv_state__state_saved_free(LttvTraceState *self, 
    LttvAttribute *container)
{
  LTTV_TRACE_STATE_GET_CLASS(self)->state_saved_free(self, container);
}


guint process_hash(gconstpointer key) 
{
  return ((LttvProcessState *)key)->pid;
}


gboolean process_equal(gconstpointer a, gconstpointer b)
{
  LttvProcessState *process_a, *process_b;

  process_a = (LttvProcessState *)a;
  process_b = (LttvProcessState *)b;

  if(process_a->pid != process_b->pid) return FALSE;
  if(process_a->pid == 0 && 
      process_a->last_cpu != process_b->last_cpu) return FALSE;
  return TRUE;
}


static void
restore_init_state(LttvTraceState *self)
{
  guint i, nb_tracefile;

  LttvTracefileState *tfcs;
  
  LttTime null_time = {0,0};

  if(self->processes != NULL) lttv_state_free_process_table(self->processes);
  self->processes = g_hash_table_new(process_hash, process_equal);
  self->nb_event = 0;

  nb_tracefile = ltt_trace_control_tracefile_number(self->parent.t) +
      ltt_trace_per_cpu_tracefile_number(self->parent.t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = LTTV_TRACEFILE_STATE(self->parent.tracefiles[i]);
    tfcs->parent.timestamp = null_time;
    tfcs->saved_position = 0;
    tfcs->process = lttv_state_create_process(tfcs, NULL,0);
    tfcs->process->state->s = LTTV_STATE_RUN;
    tfcs->process->last_cpu = tfcs->cpu_name;
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
    tcs->save_interval = 50000;
    lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_TRACE_STATE_USE_COUNT, 
        LTTV_UINT, &v);
    (*v.v_uint)++;

    if(*(v.v_uint) == 1) {
      create_name_tables(tcs);
      create_max_time(tcs);
    }
    get_name_tables(tcs);
    get_max_time(tcs);

    nb_tracefile = ltt_trace_control_tracefile_number(tc->t) +
        ltt_trace_per_cpu_tracefile_number(tc->t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = LTTV_TRACEFILE_STATE(tc->tracefiles[j]);
      tfcs->cpu_name= g_quark_from_string(ltt_tracefile_name(tfcs->parent.tf));
    }
    tcs->processes = NULL;
    restore_init_state(tcs);
  }
}


static void
fini(LttvTracesetState *self)
{
  guint i, j, nb_trace;

  LttvTraceState *tcs;

  LttvTracefileState *tfcs;

  LttvAttributeValue v;

  nb_trace = lttv_traceset_number(LTTV_TRACESET_CONTEXT(self)->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceState *)(LTTV_TRACESET_CONTEXT(self)->traces[i]);
    lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_TRACE_STATE_USE_COUNT, 
        LTTV_UINT, &v);
    (*v.v_uint)--;

    g_assert(*(v.v_uint) >= 0);
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
  guint i, nb_tracefile, nb_block, nb_event;

  LttvTracefileState *tfcs;

  LttTracefile *tf;

  LttEventPosition *ep;

  ep = ltt_event_position_new();

  fprintf(fp,"<PROCESS_STATE TIME_S=%lu TIME_NS=%lu>\n", t.tv_sec, t.tv_nsec);

  g_hash_table_foreach(self->processes, write_process_state, fp);

  nb_tracefile = ltt_trace_control_tracefile_number(self->parent.t) +
      ltt_trace_per_cpu_tracefile_number(self->parent.t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = (LttvTracefileState *)self->parent.tracefiles[i];
    fprintf(fp, "  <TRACEFILE PID=%u TIMESTAMP_S=%lu TIMESTAMP_NS=%lu", 
        tfcs->process->pid, tfcs->parent.timestamp.tv_sec, 
        tfcs->parent.timestamp.tv_nsec);
    if(tfcs->parent.e == NULL) fprintf(fp,"/>\n");
    else {
      ltt_event_position(tfcs->parent.e, ep);
      ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
      fprintf(fp, " BLOCK=%u EVENT=%u/>\n", nb_block, nb_event);
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
  new_process->execution_stack = g_array_new(FALSE, FALSE, 
      sizeof(LttvExecutionState));
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

  nb_tracefile = ltt_trace_control_tracefile_number(self->parent.t) +
      ltt_trace_per_cpu_tracefile_number(self->parent.t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = (LttvTracefileState *)self->parent.tracefiles[i];
    tracefile_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    value = lttv_attribute_add(tracefiles_tree, i, 
        LTTV_GOBJECT);
    *(value.v_gobject) = (GObject *)tracefile_tree;
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_PROCESS, 
        LTTV_UINT);
    *(value.v_uint) = tfcs->process->pid;
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_EVENT, 
        LTTV_POINTER);
    if(tfcs->parent.e == NULL) *(value.v_pointer) = NULL;
    else {
      ep = ltt_event_position_new();
      ltt_event_position(tfcs->parent.e, ep);
      *(value.v_pointer) = ep;

      guint nb_block, nb_event;
      LttTracefile *tf;
      ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
      g_debug("Block %u event %u time %lu.%lu", nb_block, nb_event,
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

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);

  type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES, 
      &value);
  g_assert(type == LTTV_POINTER);
  lttv_state_free_process_table(self->processes);
  self->processes = lttv_state_copy_process_table(*(value.v_pointer));

  nb_tracefile = ltt_trace_control_tracefile_number(self->parent.t) +
      ltt_trace_per_cpu_tracefile_number(self->parent.t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = (LttvTracefileState *)self->parent.tracefiles[i];
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
    if(*(value.v_pointer) == NULL) tfcs->parent.e = NULL;
    else {
      ep = *(value.v_pointer);
      lttv_process_tracefile_seek_position(tfcs->parent, ep);
    }
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
  lttv_attribute_remove_by_name(container, LTTV_STATE_TRACEFILES);

  type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES, 
      &value);
  g_assert(type == LTTV_POINTER);
  lttv_state_free_process_table(*(value.v_pointer));
  *(value.v_pointer) = NULL;
  lttv_attribute_remove_by_name(container, LTTV_STATE_PROCESSES);

  nb_tracefile = ltt_trace_control_tracefile_number(self->parent.t) +
      ltt_trace_per_cpu_tracefile_number(self->parent.t);

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = (LttvTracefileState *)self->parent.tracefiles[i];
    type = lttv_attribute_get(tracefiles_tree, i, &name, &value);
    g_assert(type == LTTV_GOBJECT);
    tracefile_tree = *((LttvAttribute **)(value.v_gobject));

    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT, 
        &value);
    g_assert(type == LTTV_POINTER);
    if(*(value.v_pointer) != NULL) g_free(*(value.v_pointer));
  }
  lttv_attribute_recursive_free(tracefiles_tree);
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
  lttv_attribute_recursive_free(saved_states);
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
  GQuark *eventtype_names;
  GQuark *syscall_names;
  GQuark *trap_names;
  GQuark *irq_names;
} LttvNameTables;


static void 
create_name_tables(LttvTraceState *tcs) 
{
  int i, nb;

  char *f_name, *e_name;

  LttvTraceHook h;

  LttEventType *et;

  LttType *t;

  GString *fe_name = g_string_new("");

  LttvNameTables *name_tables = g_new(LttvNameTables, 1);

  LttvAttributeValue v;

  lttv_attribute_find(tcs->parent.t_a, LTTV_STATE_NAME_TABLES, 
      LTTV_POINTER, &v);
  g_assert(*(v.v_pointer) == NULL);
  *(v.v_pointer) = name_tables;

  nb = ltt_trace_eventtype_number(tcs->parent.t);
  name_tables->eventtype_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    et = ltt_trace_eventtype_get(tcs->parent.t, i);
    e_name = ltt_eventtype_name(et);
    f_name = ltt_facility_name(ltt_eventtype_facility(et));
    g_string_printf(fe_name, "%s.%s", f_name, e_name);
    name_tables->eventtype_names[i] = g_quark_from_string(fe_name->str);    
  }

  lttv_trace_find_hook(tcs->parent.t, "core", "syscall_entry",
      "syscall_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

  /* CHECK syscalls should be an emun but currently are not!  
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

  lttv_trace_find_hook(tcs->parent.t, "core", "trap_entry",
      "trap_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

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

  lttv_trace_find_hook(tcs->parent.t, "core", "irq_entry",
      "irq_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

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
  tcs->eventtype_names = name_tables->eventtype_names;
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

  g_free(name_tables->eventtype_names);
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
    g_info("Different execution mode type (%d.%09d): ignore it\n",
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
    g_info("Trying to pop last state on stack (%d.%09d): ignore it\n",
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
  g_warning("Process %u, core %p", process->pid, process);
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
  process->execution_stack = g_array_new(FALSE, FALSE, 
      sizeof(LttvExecutionState));
  g_array_set_size(process->execution_stack, 1);
  es = process->state = &g_array_index(process->execution_stack, 
      LttvExecutionState, 0);
  es->t = LTTV_STATE_USER_MODE;
  es->n = LTTV_STATE_SUBMODE_NONE;
  es->entry = tfs->parent.timestamp;
  es->change = tfs->parent.timestamp;
  es->s = LTTV_STATE_WAIT_FORK;

  return process;
}


LttvProcessState *
lttv_state_find_process_from_trace(LttvTraceState *ts, GQuark cpu, guint pid)
{
  LttvProcessState key;
  LttvProcessState *process;

  key.pid = pid;
  key.last_cpu = cpu;
  process = g_hash_table_lookup(ts->processes, &key);
  return process;
}


LttvProcessState *lttv_state_find_process(LttvTracefileState *tfs, 
    guint pid)
{
  LttvTraceState *ts =(LttvTraceState *)tfs->parent.t_context;
  return lttv_state_find_process_from_trace(ts, tfs->cpu_name, pid);
}


LttvProcessState *
lttv_state_find_process_or_create(LttvTracefileState *tfs, guint pid)
{
  LttvProcessState *process = lttv_state_find_process(tfs, pid);

  if(process == NULL) process = lttv_state_create_process(tfs, NULL, pid);
  return process;
}


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
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->syscall_names[
      ltt_event_get_unsigned(s->parent.e, f)];
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
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->trap_names[
      ltt_event_get_unsigned(s->parent.e, f)];
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
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->irq_names[
      ltt_event_get_unsigned(s->parent.e, f)];

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
  LttvTraceHook *h = (LttvTraceHook *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint pid_in, pid_out, state_out;

  pid_in = ltt_event_get_unsigned(s->parent.e, h->f1);
  pid_out = ltt_event_get_unsigned(s->parent.e, h->f2);
  state_out = ltt_event_get_unsigned(s->parent.e, h->f3);

  if(s->process != NULL) {

    /* We could not know but it was not the idle process executing.
       This should only happen at the beginning, before the first schedule
       event, and when the initial information (current process for each CPU)
       is missing. It is not obvious how we could, after the fact, compensate
       the wrongly attributed statistics. */

    if(s->process->pid != pid_out) {
      g_assert(s->process->pid == 0);
    }

    if(state_out == 0) s->process->state->s = LTTV_STATE_WAIT_CPU;
    else if(s->process->state->s == LTTV_STATE_EXIT) 
        exit_process(s, s->process);
    else s->process->state->s = LTTV_STATE_WAIT;

    s->process->state->change = s->parent.timestamp;
  }
  s->process = lttv_state_find_process_or_create(s, pid_in);
  s->process->state->s = LTTV_STATE_RUN;
  s->process->last_cpu = s->cpu_name;
  s->process->state->change = s->parent.timestamp;
  return FALSE;
}


static gboolean process_fork(void *hook_data, void *call_data)
{
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint child_pid;

  child_pid = ltt_event_get_unsigned(s->parent.e, f);
  lttv_state_create_process(s, s->process, child_pid);
  return FALSE;
}


static gboolean process_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  if(s->process != NULL) {
    s->process->state->s = LTTV_STATE_EXIT;
  }
  return FALSE;
}


void lttv_state_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  GArray *hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
    g_array_set_size(hooks, 9);

    lttv_trace_find_hook(ts->parent.t, "core","syscall_entry","syscall_id", 
	NULL, NULL, syscall_entry, &g_array_index(hooks, LttvTraceHook, 0));

    lttv_trace_find_hook(ts->parent.t, "core", "syscall_exit", NULL, NULL, 
        NULL, syscall_exit, &g_array_index(hooks, LttvTraceHook, 1));

    lttv_trace_find_hook(ts->parent.t, "core", "trap_entry", "trap_id",
	NULL, NULL, trap_entry, &g_array_index(hooks, LttvTraceHook, 2));

    lttv_trace_find_hook(ts->parent.t, "core", "trap_exit", NULL, NULL, NULL, 
        trap_exit, &g_array_index(hooks, LttvTraceHook, 3));

    lttv_trace_find_hook(ts->parent.t, "core", "irq_entry", "irq_id", NULL, 
        NULL, irq_entry, &g_array_index(hooks, LttvTraceHook, 4));

    lttv_trace_find_hook(ts->parent.t, "core", "irq_exit", NULL, NULL, NULL, 
        irq_exit, &g_array_index(hooks, LttvTraceHook, 5));

    lttv_trace_find_hook(ts->parent.t, "core", "schedchange", "in", "out", 
        "out_state", schedchange, &g_array_index(hooks, LttvTraceHook, 6));

    lttv_trace_find_hook(ts->parent.t, "core", "process_fork", "child_pid", 
        NULL, NULL, process_fork, &g_array_index(hooks, LttvTraceHook, 7));

    lttv_trace_find_hook(ts->parent.t, "core", "process_exit", NULL, NULL, 
        NULL, process_exit, &g_array_index(hooks, LttvTraceHook, 8));

    /* Add these hooks to each event_by_id hooks list */

    nb_tracefile = ltt_trace_control_tracefile_number(ts->parent.t) +
        ltt_trace_per_cpu_tracefile_number(ts->parent.t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATE(ts->parent.tracefiles[j]);

      for(k = 0 ; k < hooks->len ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id, 
	  hook.id), hook.h, &g_array_index(hooks, LttvTraceHook, k), LTTV_PRIO_STATE);
      }
    }
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    *(val.v_pointer) = hooks;
  }
}


void lttv_state_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  GArray *hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATE(self->parent.traces[i]);
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    hooks = *(val.v_pointer);

    /* Remove these hooks from each event_by_id hooks list */

    nb_tracefile = ltt_trace_control_tracefile_number(ts->parent.t) +
        ltt_trace_per_cpu_tracefile_number(ts->parent.t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATE(ts->parent.tracefiles[j]);

      for(k = 0 ; k < hooks->len ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.event_by_id, 
	    hook.id), hook.h, &g_array_index(hooks, LttvTraceHook, k));
      }
    }
    g_array_free(hooks, TRUE);
  }
}


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
  nb_tracefile = ltt_trace_control_tracefile_number(tcs->parent.t) +
      ltt_trace_per_cpu_tracefile_number(tcs->parent.t);

  /* Count the number of events added since the last block end in any
     tracefile. */

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = (LttvTracefileState *)tcs->parent.tracefiles[i];
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
}


void lttv_state_save_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_tracefile;

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

    nb_tracefile = ltt_trace_control_tracefile_number(ts->parent.t) +
        ltt_trace_per_cpu_tracefile_number(ts->parent.t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATE(ts->parent.tracefiles[j]);
      lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id, 
	  hook_start.id), hook_start.h, NULL, LTTV_PRIO_STATE);
      lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id, 
	  hook_end.id), hook_end.h, NULL, LTTV_PRIO_STATE);
    }
  }
}


void lttv_state_save_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_tracefile;

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

    nb_tracefile = ltt_trace_control_tracefile_number(ts->parent.t) +
        ltt_trace_per_cpu_tracefile_number(ts->parent.t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATE(ts->parent.tracefiles[j]);
      lttv_hooks_remove_data(lttv_hooks_by_id_find(
          tfs->parent.event_by_id, hook_start.id), hook_start.h, NULL);
      lttv_hooks_remove_data(lttv_hooks_by_id_find(
          tfs->parent.event_by_id, hook_end.id), hook_end.h, NULL);
    }
  }
}


void lttv_state_traceset_seek_time_closest(LttvTracesetState *self, LttTime t)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, nb_trace, nb_saved_state;

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
      (GInstanceInitFunc) traceset_state_instance_init    /* instance_init */
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
      (GInstanceInitFunc) trace_state_instance_init    /* instance_init */
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
      (GInstanceInitFunc) tracefile_state_instance_init    /* instance_init */
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
}

static void module_destroy() 
{
}


LTTV_MODULE("state", "State computation", \
    "Update the system state, possibly saving it at intervals", \
    module_init, module_destroy)



