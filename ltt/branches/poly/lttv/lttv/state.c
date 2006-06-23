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
#include <string.h>

#define PREALLOCATED_EXECUTION_STACK 10

/* Facilities Quarks */

GQuark
    LTT_FACILITY_KERNEL,
    LTT_FACILITY_KERNEL_ARCH,
    LTT_FACILITY_PROCESS,
    LTT_FACILITY_FS,
    LTT_FACILITY_STATEDUMP,
    LTT_FACILITY_USER_GENERIC;

/* Events Quarks */

GQuark 
    LTT_EVENT_SYSCALL_ENTRY,
    LTT_EVENT_SYSCALL_EXIT,
    LTT_EVENT_TRAP_ENTRY,
    LTT_EVENT_TRAP_EXIT,
    LTT_EVENT_IRQ_ENTRY,
    LTT_EVENT_IRQ_EXIT,
    LTT_EVENT_SOFT_IRQ_ENTRY,
    LTT_EVENT_SOFT_IRQ_EXIT,
    LTT_EVENT_SCHEDCHANGE,
    LTT_EVENT_FORK,
    LTT_EVENT_KERNEL_THREAD,
    LTT_EVENT_EXIT,
    LTT_EVENT_FREE,
    LTT_EVENT_EXEC,
    LTT_EVENT_ENUM_PROCESS_STATE,
    LTT_EVENT_FUNCTION_ENTRY,
    LTT_EVENT_FUNCTION_EXIT,
    LTT_EVENT_THREAD_BRAND;

/* Fields Quarks */

GQuark 
    LTT_FIELD_SYSCALL_ID,
    LTT_FIELD_TRAP_ID,
    LTT_FIELD_IRQ_ID,
    LTT_FIELD_SOFT_IRQ_ID,
    LTT_FIELD_OUT,
    LTT_FIELD_IN,
    LTT_FIELD_OUT_STATE,
    LTT_FIELD_PARENT_PID,
    LTT_FIELD_CHILD_PID,
    LTT_FIELD_PID,
    LTT_FIELD_TGID,
    LTT_FIELD_FILENAME,
    LTT_FIELD_NAME,
    LTT_FIELD_TYPE,
    LTT_FIELD_MODE,
    LTT_FIELD_SUBMODE,
    LTT_FIELD_STATUS,
    LTT_FIELD_THIS_FN,
    LTT_FIELD_CALL_SITE;

LttvExecutionMode
  LTTV_STATE_MODE_UNKNOWN,
  LTTV_STATE_USER_MODE,
  LTTV_STATE_SYSCALL,
  LTTV_STATE_TRAP,
  LTTV_STATE_IRQ,
  LTTV_STATE_SOFT_IRQ;

LttvExecutionSubmode
  LTTV_STATE_SUBMODE_UNKNOWN,
  LTTV_STATE_SUBMODE_NONE;

LttvProcessStatus
  LTTV_STATE_UNNAMED,
  LTTV_STATE_UNBRANDED,
  LTTV_STATE_WAIT_FORK,
  LTTV_STATE_WAIT_CPU,
  LTTV_STATE_EXIT,
  LTTV_STATE_ZOMBIE,
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN,
  LTTV_STATE_DEAD;

LttvProcessType
  LTTV_STATE_USER_THREAD,
  LTTV_STATE_KERNEL_THREAD;

static GQuark
  LTTV_STATE_TRACEFILES,
  LTTV_STATE_PROCESSES,
  LTTV_STATE_PROCESS,
  LTTV_STATE_RUNNING_PROCESS,
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
                 process_a->cpu != process_b->cpu)) ret = FALSE;

  return ret;
}

static void delete_usertrace(gpointer key, gpointer value, gpointer user_data)
{
  g_tree_destroy((GTree*)value);
}

static void lttv_state_free_usertraces(GHashTable *usertraces)
{
  g_hash_table_foreach(usertraces, delete_usertrace, NULL);
  g_hash_table_destroy(usertraces);
}



static void
restore_init_state(LttvTraceState *self)
{
  guint i, nb_cpus;

  LttvTracefileState *tfcs;

  LttTime start_time, end_time;
  
  /* Free the process tables */
  if(self->processes != NULL) lttv_state_free_process_table(self->processes);
  if(self->usertraces != NULL) lttv_state_free_usertraces(self->usertraces);
  self->processes = g_hash_table_new(process_hash, process_equal);
  self->usertraces = g_hash_table_new(g_direct_hash, g_direct_equal);
  self->nb_event = 0;

  /* Seek time to beginning */
  // Mathieu : fix : don't seek traceset here : causes inconsistency in seek
  // closest. It's the tracecontext job to seek the trace to the beginning
  // anyway : the init state might be used at the middle of the trace as well...
  //g_tree_destroy(self->parent.ts_context->pqueue);
  //self->parent.ts_context->pqueue = g_tree_new(compare_tracefile);
  
  ltt_trace_time_span_get(self->parent.t, &start_time, &end_time);
  
  //lttv_process_trace_seek_time(&self->parent, ltt_time_zero);

  nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
  
  /* Put the per cpu running_process to beginning state : process 0. */
  for(i=0; i< nb_cpus; i++) {
    self->running_process[i] = lttv_state_create_process(self, NULL, i, 0, 0,
        LTTV_STATE_UNNAMED, &start_time);
    self->running_process[i]->state->s = LTTV_STATE_RUN;
    self->running_process[i]->cpu = i;
  }
  
#if 0
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
    tfcs->process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfcs)->tf);
  }
#endif //0
}

//static LttTime time_zero = {0,0};

static gint compare_usertraces(gconstpointer a, gconstpointer b, 
    gpointer user_data)
{
  const LttTime *t1 = (const LttTime *)a;
  const LttTime *t2 = (const LttTime *)b;

  return ltt_time_compare(*t1, *t2);
}

static void free_usertrace_key(gpointer data)
{
  g_free(data);
}

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
    tcs = LTTV_TRACE_STATE(tc);
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
    tcs->processes = NULL;
    tcs->usertraces = NULL;
    tcs->running_process = g_new(LttvProcessState*, 
                                 ltt_trace_get_num_cpu(tc->t));
    restore_init_state(tcs);
    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(tc->tracefiles,
                                          LttvTracefileContext*, j));
      tfcs->tracefile_name = ltt_tracefile_name(tfcs->parent.tf);
      tfcs->cpu = ltt_tracefile_cpu(tfcs->parent.tf);
#if 0    
      if(ltt_tracefile_tid(tfcs->parent.tf) != 0) {
        /* It's a Usertrace */
        LttvProcessState *process;
        LttTime timestamp = 
          ltt_interpolate_time_from_tsc(tfcs->parent.tf,
              ltt_tracefile_creation(tfcs->parent.tf));
        process = lttv_state_find_process_or_create(
                        tcs,
                        0, ltt_tracefile_tid(tfcs->parent.tf),
                        &timestamp);
        process->usertrace = tfcs;
      }
    }
#endif //0
      if(ltt_tracefile_tid(tfcs->parent.tf) != 0) {
        /* It's a Usertrace */
        guint tid = ltt_tracefile_tid(tfcs->parent.tf);
        GTree *usertrace_tree = (GTree*)g_hash_table_lookup(tcs->usertraces,
            (gconstpointer)tid);
        if(!usertrace_tree) {
          usertrace_tree = g_tree_new_full(compare_usertraces,
              NULL, free_usertrace_key, NULL);
          g_hash_table_insert(tcs->usertraces,
              (gpointer)tid, usertrace_tree);
        }
        LttTime *timestamp = g_new(LttTime, 1);
        *timestamp = ltt_interpolate_time_from_tsc(tfcs->parent.tf,
              ltt_tracefile_creation(tfcs->parent.tf));
        g_tree_insert(usertrace_tree, timestamp, tfcs);
      }
    }

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
    g_free(tcs->running_process);
    tcs->running_process = NULL;
    lttv_state_free_process_table(tcs->processes);
    lttv_state_free_usertraces(tcs->usertraces);
    tcs->processes = NULL;
    tcs->usertraces = NULL;
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
"  <PROCESS CORE=%p PID=%u PPID=%u TYPE=\"%s\"CTIME_S=%lu CTIME_NS=%lu NAME=\"%s\" BRAND=\"%s\" CPU=\"%u\">\n",
      process, process->pid, process->ppid, g_quark_to_string(process->type),
      process->creation_time.tv_sec,
      process->creation_time.tv_nsec, g_quark_to_string(process->name),
      g_quark_to_string(process->brand),
      process->cpu);

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

  guint nb_cpus;

  ep = ltt_event_position_new();

  fprintf(fp,"<PROCESS_STATE TIME_S=%lu TIME_NS=%lu>\n", t.tv_sec, t.tv_nsec);

  g_hash_table_foreach(self->processes, write_process_state, fp);
  
  nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
  for(i=0;i<nb_cpus;i++) {
    fprintf(fp,"<CPU NUM=%u RUNNING_PROCESS=%u>\n",
        i, self->running_process[i]->pid);
  }

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    fprintf(fp, "  <TRACEFILE TIMESTAMP_S=%lu TIMESTAMP_NS=%lu", 
        tfcs->parent.timestamp.tv_sec, 
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
  new_process->execution_stack = 
              g_array_set_size(new_process->execution_stack,
                  process->execution_stack->len);
  for(i = 0 ; i < process->execution_stack->len; i++) {
    g_array_index(new_process->execution_stack, LttvExecutionState, i) =
        g_array_index(process->execution_stack, LttvExecutionState, i);
  }
  new_process->state = &g_array_index(new_process->execution_stack, 
      LttvExecutionState, new_process->execution_stack->len - 1);
  new_process->user_stack = g_array_sized_new(FALSE, FALSE, 
      sizeof(guint64), 0);
  new_process->user_stack = 
              g_array_set_size(new_process->user_stack,
                  process->user_stack->len);
  for(i = 0 ; i < process->user_stack->len; i++) {
    g_array_index(new_process->user_stack, guint64, i) =
        g_array_index(process->user_stack, guint64, i);
  }
  new_process->current_function = process->current_function;
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
  guint i, nb_tracefile, nb_cpus;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;
  
  guint *running_process;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  LttEventPosition *ep;

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);

  value = lttv_attribute_add(container, LTTV_STATE_PROCESSES,
      LTTV_POINTER);
  *(value.v_pointer) = lttv_state_copy_process_table(self->processes);

  /* Add the currently running processes array */
  nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
  running_process = g_new(guint, nb_cpus);
  for(i=0;i<nb_cpus;i++) {
    running_process[i] = self->running_process[i]->pid;
  }
  value = lttv_attribute_add(container, LTTV_STATE_RUNNING_PROCESS, 
                             LTTV_POINTER);
  *(value.v_pointer) = running_process;
  
  g_info("State save");
  
  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    tracefile_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
    value = lttv_attribute_add(tracefiles_tree, i, 
        LTTV_GOBJECT);
    *(value.v_gobject) = (GObject *)tracefile_tree;
#if 0
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_PROCESS, 
        LTTV_UINT);
    *(value.v_uint) = tfcs->process->pid;
#endif //0
    value = lttv_attribute_add(tracefile_tree, LTTV_STATE_EVENT, 
        LTTV_POINTER);
    /* Only save the position if the tfs has not infinite time. */
    //if(!g_tree_lookup(self->parent.ts_context->pqueue, &tfcs->parent)
    //    && current_tfcs != tfcs) {
    if(ltt_time_compare(tfcs->parent.timestamp, ltt_time_infinite) == 0) {
      *(value.v_pointer) = NULL;
    } else {
      LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
      ep = ltt_event_position_new();
      ltt_event_position(e, ep);
      *(value.v_pointer) = ep;

      guint nb_block, offset;
      guint64 tsc;
      LttTracefile *tf;
      ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
      g_info("Block %u offset %u tsc %llu time %lu.%lu", nb_block, offset,
          tsc,
          tfcs->parent.timestamp.tv_sec, tfcs->parent.timestamp.tv_nsec);
    }
  }
}


static void state_restore(LttvTraceState *self, LttvAttribute *container)
{
  guint i, nb_tracefile, pid, nb_cpus;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;

  guint *running_process;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  gboolean is_named;

  LttEventPosition *ep;

  LttvTracesetContext *tsc = self->parent.ts_context;

  tracefiles_tree = lttv_attribute_find_subdir(container, 
      LTTV_STATE_TRACEFILES);

  type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES, 
      &value);
  g_assert(type == LTTV_POINTER);
  lttv_state_free_process_table(self->processes);
  self->processes = lttv_state_copy_process_table(*(value.v_pointer));

  /* Add the currently running processes array */
  nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
  type = lttv_attribute_get_by_name(container, LTTV_STATE_RUNNING_PROCESS, 
        &value);
  g_assert(type == LTTV_POINTER);
  running_process = *(value.v_pointer);
  for(i=0;i<nb_cpus;i++) {
    pid = running_process[i];
    self->running_process[i] = lttv_state_find_process(self, i, pid);
    g_assert(self->running_process[i] != NULL);
  }

 
  nb_tracefile = self->parent.tracefiles->len;

  //g_tree_destroy(tsc->pqueue);
  //tsc->pqueue = g_tree_new(compare_tracefile);
 
  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    type = lttv_attribute_get(tracefiles_tree, i, &name, &value, &is_named);
    g_assert(type == LTTV_GOBJECT);
    tracefile_tree = *((LttvAttribute **)(value.v_gobject));
#if 0
    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_PROCESS, 
        &value);
    g_assert(type == LTTV_UINT);
    pid = *(value.v_uint);
    tfcs->process = lttv_state_find_process_or_create(tfcs, pid);
#endif //0
    type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT, 
        &value);
    g_assert(type == LTTV_POINTER);
    //g_assert(*(value.v_pointer) != NULL);
    ep = *(value.v_pointer);
    g_assert(tfcs->parent.t_context != NULL);
    
    LttvTracefileContext *tfc = LTTV_TRACEFILE_CONTEXT(tfcs);
    g_tree_remove(tsc->pqueue, tfc);
    
    if(ep != NULL) {
      g_assert(ltt_tracefile_seek_position(tfc->tf, ep) == 0);
      tfc->timestamp = ltt_event_time(ltt_tracefile_get_event(tfc->tf));
      g_assert(ltt_time_compare(tfc->timestamp, ltt_time_infinite) != 0);
      g_tree_insert(tsc->pqueue, tfc, tfc);
      g_info("Restoring state for a tf at time %lu.%lu", tfc->timestamp.tv_sec, tfc->timestamp.tv_nsec);
    } else {
      tfc->timestamp = ltt_time_infinite;
    }
  }
}


static void state_saved_free(LttvTraceState *self, LttvAttribute *container)
{
  guint i, nb_tracefile, nb_cpus;

  LttvTracefileState *tfcs;

  LttvAttribute *tracefiles_tree, *tracefile_tree;

  guint *running_process;

  LttvAttributeType type;

  LttvAttributeValue value;

  LttvAttributeName name;

  gboolean is_named;

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

  /* Free running processes array */
  nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
  type = lttv_attribute_get_by_name(container, LTTV_STATE_RUNNING_PROCESS, 
        &value);
  g_assert(type == LTTV_POINTER);
  running_process = *(value.v_pointer);
  g_free(running_process);

  nb_tracefile = self->parent.tracefiles->len;

  for(i = 0 ; i < nb_tracefile ; i++) {
    tfcs = 
          LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
                                          LttvTracefileContext*, i));
    type = lttv_attribute_get(tracefiles_tree, i, &name, &value, &is_named);
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

  gboolean is_named;

  LttvAttribute *saved_states;

  saved_states = lttv_attribute_find_subdir(self->parent.t_a,
      LTTV_STATE_SAVED_STATES);

  nb = lttv_attribute_get_number(saved_states);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(saved_states, i, &name, &value, &is_named);
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
  *((LttTime *)*(v.v_pointer)) = ltt_time_zero;
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
  guint nb_syscalls;
  GQuark *trap_names;
  guint nb_traps;
  GQuark *irq_names;
  GQuark *soft_irq_names;
} LttvNameTables;


static void 
create_name_tables(LttvTraceState *tcs) 
{
  int i, nb;

  GQuark f_name, e_name;

  LttvTraceHook h;

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
  if(!lttv_trace_find_hook(tcs->parent.t,
      LTT_FACILITY_KERNEL_ARCH, LTT_EVENT_SYSCALL_ENTRY,
      LTT_FIELD_SYSCALL_ID, 0, 0,
      NULL, NULL, &h)) {
    
    thf = lttv_trace_hook_get_first(&h);
    
    t = ltt_field_type(thf->f1);
    nb = ltt_type_element_number(t);
    
    lttv_trace_hook_destroy(&h);

    name_tables->syscall_names = g_new(GQuark, nb);
    name_tables->nb_syscalls = nb;

    for(i = 0 ; i < nb ; i++) {
      name_tables->syscall_names[i] = ltt_enum_string_get(t, i);
    }

    //name_tables->syscall_names = g_new(GQuark, 256);
    //for(i = 0 ; i < 256 ; i++) {
    //  g_string_printf(fe_name, "syscall %d", i);
    //  name_tables->syscall_names[i] = g_quark_from_string(fe_name->str);
    //}
  } else {
    name_tables->syscall_names = NULL;
    name_tables->nb_syscalls = 0;
  }

  if(!lttv_trace_find_hook(tcs->parent.t, LTT_FACILITY_KERNEL,
        LTT_EVENT_TRAP_ENTRY,
        LTT_FIELD_TRAP_ID, 0, 0,
        NULL, NULL, &h)) {

    thf = lttv_trace_hook_get_first(&h);

    t = ltt_field_type(thf->f1);
    //nb = ltt_type_element_number(t);

    lttv_trace_hook_destroy(&h);

    /*
    name_tables->trap_names = g_new(GQuark, nb);
    for(i = 0 ; i < nb ; i++) {
      name_tables->trap_names[i] = g_quark_from_string(
          ltt_enum_string_get(t, i));
    }
    */
    name_tables->nb_traps = 256;
    name_tables->trap_names = g_new(GQuark, 256);
    for(i = 0 ; i < 256 ; i++) {
      g_string_printf(fe_name, "trap %d", i);
      name_tables->trap_names[i] = g_quark_from_string(fe_name->str);
    }
  } else {
    name_tables->trap_names = NULL;
    name_tables->nb_traps = 0;
  }

  if(!lttv_trace_find_hook(tcs->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
        LTT_FIELD_IRQ_ID, 0, 0,
        NULL, NULL, &h)) {
    
    thf = lttv_trace_hook_get_first(&h);
    
    t = ltt_field_type(thf->f1);
    //nb = ltt_type_element_number(t);

    lttv_trace_hook_destroy(&h);

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
  } else {
    name_tables->irq_names = NULL;
  }
  /*
  name_tables->soft_irq_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    name_tables->soft_irq_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
  }
  */

  name_tables->soft_irq_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "softirq %d", i);
    name_tables->soft_irq_names[i] = g_quark_from_string(fe_name->str);
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
  tcs->nb_syscalls = name_tables->nb_syscalls;
  tcs->trap_names = name_tables->trap_names;
  tcs->nb_traps = name_tables->nb_traps;
  tcs->irq_names = name_tables->irq_names;
  tcs->soft_irq_names = name_tables->soft_irq_names;
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
  if(name_tables->syscall_names) g_free(name_tables->syscall_names);
  if(name_tables->trap_names) g_free(name_tables->trap_names);
  if(name_tables->irq_names) g_free(name_tables->irq_names);
  if(name_tables->soft_irq_names) g_free(name_tables->soft_irq_names);
  if(name_tables) g_free(name_tables);
} 

#ifdef HASH_TABLE_DEBUG

static void test_process(gpointer key, gpointer value, gpointer user_data)
{
  LttvProcessState *process = (LttvProcessState *)value;
  
  /* Test for process corruption */
  guint stack_len = process->execution_stack->len;
}

static void hash_table_check(GHashTable *table)
{
  g_hash_table_foreach(table, test_process, NULL);
}


#endif


static void push_state(LttvTracefileState *tfs, LttvExecutionMode t, 
    guint state_id)
{
  LttvExecutionState *es;
  
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  guint cpu = tfs->cpu;

#ifdef HASH_TABLE_DEBUG
  hash_table_check(ts->processes);
#endif
  LttvProcessState *process = ts->running_process[cpu];

  guint depth = process->execution_stack->len;

  process->execution_stack = 
    g_array_set_size(process->execution_stack, depth + 1);
  /* Keep in sync */
  process->state =
    &g_array_index(process->execution_stack, LttvExecutionState, depth - 1);
    
  es = &g_array_index(process->execution_stack, LttvExecutionState, depth);
  es->t = t;
  es->n = state_id;
  es->entry = es->change = tfs->parent.timestamp;
  es->cum_cpu_time = ltt_time_zero;
  es->s = process->state->s;
  process->state = es;
}

/* pop state
 * return 1 when empty, else 0 */
int lttv_state_pop_state_cleanup(LttvProcessState *process, 
    LttvTracefileState *tfs)
{ 
  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;

  guint depth = process->execution_stack->len;

  if(depth == 1){
    return 1;
  }

  process->execution_stack = 
    g_array_set_size(process->execution_stack, depth - 1);
  process->state = &g_array_index(process->execution_stack, LttvExecutionState,
      depth - 2);
  process->state->change = tfs->parent.timestamp;
  
  return 0;
}

static void pop_state(LttvTracefileState *tfs, LttvExecutionMode t)
{
  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];

  guint depth = process->execution_stack->len;

  if(process->state->t != t){
    g_info("Different execution mode type (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    g_info("process state has %s when pop_int is %s\n",
        g_quark_to_string(process->state->t),
        g_quark_to_string(t));
    g_info("{ %u, %u, %s, %s, %s }\n",
        process->pid,
        process->ppid,
        g_quark_to_string(process->name),
        g_quark_to_string(process->brand),
        g_quark_to_string(process->state->s));
    return;
  }

  if(depth == 1){
    g_info("Trying to pop last state on stack (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    return;
  }

  process->execution_stack = 
    g_array_set_size(process->execution_stack, depth - 1);
  process->state = &g_array_index(process->execution_stack, LttvExecutionState,
      depth - 2);
  process->state->change = tfs->parent.timestamp;
}

struct search_result {
  const LttTime *time;  /* Requested time */
  LttTime *best;  /* Best result */
};

static gint search_usertrace(gconstpointer a, gconstpointer b)
{
  const LttTime *elem_time = (const LttTime*)a;
  /* Explicit non const cast */
  struct search_result *res = (struct search_result *)b;

  if(ltt_time_compare(*elem_time, *(res->time)) < 0) {
    /* The usertrace was created before the schedchange */
    /* Get larger keys */
    return 1;
  } else if(ltt_time_compare(*elem_time, *(res->time)) >= 0) {
    /* The usertrace was created after the schedchange time */
    /* Get smaller keys */
    if(res->best) {
      if(ltt_time_compare(*elem_time, *res->best) < 0) {
        res->best = elem_time;
      }
    } else {
      res->best = elem_time;
    }
    return -1;
  }
  return 0;
}

static LttvTracefileState *ltt_state_usertrace_find(LttvTraceState *tcs,
    guint pid, const LttTime *timestamp)
{
  LttvTracefileState *tfs = NULL;
  struct search_result res;
  /* Find the usertrace associated with a pid and time interval.
   * Search in the usertraces by PID (within a hash) and then, for each
   * corresponding element of the array, find the first one with creation
   * timestamp the lowest, but higher or equal to "timestamp". */
  res.time = timestamp;
  res.best = NULL;
  GTree *usertrace_tree = g_hash_table_lookup(tcs->usertraces, (gpointer)pid);
  if(usertrace_tree) {
    g_tree_search(usertrace_tree, search_usertrace, &res);
    if(res.best)
      tfs = g_tree_lookup(usertrace_tree, res.best);
  }

  return tfs;
}


LttvProcessState *
lttv_state_create_process(LttvTraceState *tcs, LttvProcessState *parent, 
    guint cpu, guint pid, guint tgid, GQuark name, const LttTime *timestamp)
{
  LttvProcessState *process = g_new(LttvProcessState, 1);

  LttvExecutionState *es;

  LttvTraceContext *tc = (LttvTraceContext*)tcs;

  char buffer[128];

  process->pid = pid;
  process->tgid = tgid;
  process->cpu = cpu;
  process->name = name;
  process->brand = LTTV_STATE_UNBRANDED;
  //process->last_cpu = tfs->cpu_name;
  //process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfs)->tf);
  process->type = LTTV_STATE_USER_THREAD;
  process->usertrace = ltt_state_usertrace_find(tcs, pid, timestamp);
  process->current_function = 0; //function 0x0 by default.

  g_info("Process %u, core %p", process->pid, process);
  g_hash_table_insert(tcs->processes, process, process);

  if(parent) {
    process->ppid = parent->pid;
    process->creation_time = *timestamp;
  }

  /* No parent. This process exists but we are missing all information about
     its creation. The birth time is set to zero but we remember the time of
     insertion */

  else {
    process->ppid = 0;
    process->creation_time = ltt_time_zero;
  }

  process->insertion_time = *timestamp;
  sprintf(buffer,"%d-%lu.%lu",pid, process->creation_time.tv_sec, 
    process->creation_time.tv_nsec);
  process->pid_time = g_quark_from_string(buffer);
  process->cpu = cpu;
  //process->last_cpu = tfs->cpu_name;
  //process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfs)->tf);
  process->execution_stack = g_array_sized_new(FALSE, FALSE, 
      sizeof(LttvExecutionState), PREALLOCATED_EXECUTION_STACK);
  process->execution_stack = g_array_set_size(process->execution_stack, 2);
  es = process->state = &g_array_index(process->execution_stack, 
      LttvExecutionState, 0);
  es->t = LTTV_STATE_USER_MODE;
  es->n = LTTV_STATE_SUBMODE_NONE;
  es->entry = *timestamp;
  //g_assert(timestamp->tv_sec != 0);
  es->change = *timestamp;
  es->cum_cpu_time = ltt_time_zero;
  es->s = LTTV_STATE_RUN;

  es = process->state = &g_array_index(process->execution_stack, 
      LttvExecutionState, 1);
  es->t = LTTV_STATE_SYSCALL;
  es->n = LTTV_STATE_SUBMODE_NONE;
  es->entry = *timestamp;
  //g_assert(timestamp->tv_sec != 0);
  es->change = *timestamp;
  es->cum_cpu_time = ltt_time_zero;
  es->s = LTTV_STATE_WAIT_FORK;
  
  /* Allocate an empty function call stack. If it's empty, use 0x0. */
  process->user_stack = g_array_sized_new(FALSE, FALSE,
      sizeof(guint64), 0);
  
  return process;
}

LttvProcessState *lttv_state_find_process(LttvTraceState *ts, guint cpu,
    guint pid)
{
  LttvProcessState key;
  LttvProcessState *process;

  key.pid = pid;
  key.cpu = cpu;
  process = g_hash_table_lookup(ts->processes, &key);
  return process;
}

LttvProcessState *
lttv_state_find_process_or_create(LttvTraceState *ts, guint cpu, guint pid,
    LttTime *timestamp)
{
  LttvProcessState *process = lttv_state_find_process(ts, cpu, pid);
  LttvExecutionState *es;
  
  /* Put ltt_time_zero creation time for unexisting processes */
  if(unlikely(process == NULL)) {
    process = lttv_state_create_process(ts,
                NULL, cpu, pid, 0, LTTV_STATE_UNNAMED, timestamp);
    /* We are not sure is it's a kernel thread or normal thread, put the
      * bottom stack state to unknown */
    es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
    es->t = LTTV_STATE_MODE_UNKNOWN;
  }
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
  key.cpu = process->cpu;
  g_hash_table_remove(ts->processes, &key);
  g_array_free(process->execution_stack, TRUE);
  g_array_free(process->user_stack, TRUE);
  g_free(process);
}


static void free_process_state(gpointer key, gpointer value,gpointer user_data)
{
  g_array_free(((LttvProcessState *)value)->execution_stack, TRUE);
  g_array_free(((LttvProcessState *)value)->user_stack, TRUE);
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
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  guint nb_syscalls = ((LttvTraceState *)(s->parent.t_context))->nb_syscalls;
  guint syscall = ltt_event_get_unsigned(e, f);
  
  if(syscall < nb_syscalls) {
    submode = ((LttvTraceState *)(s->parent.t_context))->syscall_names[
        syscall];
  } else {
    /* Fixup an incomplete syscall table */
    GString *string = g_string_new("");
    g_string_printf(string, "syscall %u", syscall);
    submode = g_quark_from_string(string->str);
    g_string_free(string, TRUE);
  }
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
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  guint64 nb_traps = ((LttvTraceState *)(s->parent.t_context))->nb_traps;
  guint64 trap = ltt_event_get_long_unsigned(e, f);

  if(trap < nb_traps) {
    submode = ((LttvTraceState *)(s->parent.t_context))->trap_names[trap];
  } else {
    /* Fixup an incomplete trap table */
    GString *string = g_string_new("");
    g_string_printf(string, "trap %llu", trap);
    submode = g_quark_from_string(string->str);
    g_string_free(string, TRUE);
  }

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
  guint8 fac_id = ltt_event_facility_id(e);
  guint8 ev_id = ltt_event_eventtype_id(e);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
 // g_assert(lttv_trace_hook_get_first((LttvTraceHook *)hook_data)->f1 != NULL);
  g_assert(thf->f1 != NULL);
 // g_assert(thf == lttv_trace_hook_get_first((LttvTraceHook *)hook_data));
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->irq_names[
      ltt_event_get_unsigned(e, f)];

  /* Do something with the info about being in user or system mode when int? */
  push_state(s, LTTV_STATE_IRQ, submode);
  return FALSE;
}

static gboolean soft_irq_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_SOFT_IRQ);
  return FALSE;
}



static gboolean irq_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_IRQ);
  return FALSE;
}

static gboolean soft_irq_entry(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  guint8 fac_id = ltt_event_facility_id(e);
  guint8 ev_id = ltt_event_eventtype_id(e);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
 // g_assert(lttv_trace_hook_get_first((LttvTraceHook *)hook_data)->f1 != NULL);
  g_assert(thf->f1 != NULL);
 // g_assert(thf == lttv_trace_hook_get_first((LttvTraceHook *)hook_data));
  LttField *f = thf->f1;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->soft_irq_names[
      ltt_event_get_long_unsigned(e, f)];

  /* Do something with the info about being in user or system mode when int? */
  push_state(s, LTTV_STATE_SOFT_IRQ, submode);
  return FALSE;
}

static void push_function(LttvTracefileState *tfs, guint64 funcptr)
{
  guint64 *new_func;
  
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  guint cpu = tfs->cpu;
  LttvProcessState *process = ts->running_process[cpu];

  guint depth = process->user_stack->len;

  process->user_stack = 
    g_array_set_size(process->user_stack, depth + 1);
    
  new_func = &g_array_index(process->user_stack, guint64, depth);
  *new_func = funcptr;
  process->current_function = funcptr;
}

static void pop_function(LttvTracefileState *tfs, guint64 funcptr)
{
  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];

  if(process->current_function != funcptr){
    g_info("Different functions (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    g_info("process state has %llu when pop_function is %llu\n",
        process->current_function, funcptr);
    g_info("{ %u, %u, %s, %s, %s }\n",
        process->pid,
        process->ppid,
        g_quark_to_string(process->name),
        g_quark_to_string(process->brand),
        g_quark_to_string(process->state->s));
    return;
  }
  guint depth = process->user_stack->len;

  if(depth == 0){
    g_info("Trying to pop last function on stack (%lu.%09lu): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    return;
  }

  process->user_stack = 
    g_array_set_size(process->user_stack, depth - 1);
  process->current_function =
    g_array_index(process->user_stack, guint64, depth - 2);
}


static gboolean function_entry(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  guint8 fac_id = ltt_event_facility_id(e);
  guint8 ev_id = ltt_event_eventtype_id(e);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  g_assert(thf->f1 != NULL);
  LttField *f = thf->f1;
  guint64 funcptr = ltt_event_get_long_unsigned(e, f);

  push_function(s, funcptr);
  return FALSE;
}

static gboolean function_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  guint8 fac_id = ltt_event_facility_id(e);
  guint8 ev_id = ltt_event_eventtype_id(e);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  g_assert(thf->f1 != NULL);
  LttField *f = thf->f1;
  guint64 funcptr = ltt_event_get_long_unsigned(e, f);

  LttvExecutionSubmode submode;

  pop_function(s, funcptr);
  return FALSE;
}

static gboolean schedchange(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  guint cpu = s->cpu;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];
  LttvProcessState *old_process = ts->running_process[cpu];
  
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  guint pid_in, pid_out;
  gint state_out;

  pid_out = ltt_event_get_unsigned(e, thf->f1);
  pid_in = ltt_event_get_unsigned(e, thf->f2);
  state_out = ltt_event_get_int(e, thf->f3);
  
  if(likely(process != NULL)) {

    /* We could not know but it was not the idle process executing.
       This should only happen at the beginning, before the first schedule
       event, and when the initial information (current process for each CPU)
       is missing. It is not obvious how we could, after the fact, compensate
       the wrongly attributed statistics. */

    //This test only makes sense once the state is known and if there is no
    //missing events. We need to silently ignore schedchange coming after a
    //process_free, or it causes glitches. (FIXME)
    //if(unlikely(process->pid != pid_out)) {
    //  g_assert(process->pid == 0);
    //}

    if(unlikely(process->state->s == LTTV_STATE_EXIT)) {
      process->state->s = LTTV_STATE_ZOMBIE;
      process->state->change = s->parent.timestamp;
    } else {
      if(unlikely(state_out == 0)) process->state->s = LTTV_STATE_WAIT_CPU;
      else process->state->s = LTTV_STATE_WAIT;
      process->state->change = s->parent.timestamp;
    }
    
    if(state_out == 32)
       exit_process(s, process); /* EXIT_DEAD */
          /* see sched.h for states */
  }
  process = ts->running_process[cpu] =
              lttv_state_find_process_or_create(
                  (LttvTraceState*)s->parent.t_context,
                  cpu, pid_in,
                  &s->parent.timestamp);
  process->state->s = LTTV_STATE_RUN;
  process->cpu = cpu;
  if(process->usertrace)
    process->usertrace->cpu = cpu;
 // process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)s)->tf);
  process->state->change = s->parent.timestamp;
  return FALSE;
}

static gboolean process_fork(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  guint parent_pid;
  guint child_pid;  /* In the Linux Kernel, there is one PID per thread. */
  guint child_tgid;  /* tgid in the Linux kernel is the "real" POSIX PID. */
  LttvProcessState *zombie_process;
  guint cpu = s->cpu;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];
  LttvProcessState *child_process;

  /* Parent PID */
  parent_pid = ltt_event_get_unsigned(e, thf->f1);

  /* Child PID */
  child_pid = ltt_event_get_unsigned(e, thf->f2);
  s->parent.target_pid = child_pid;

  /* Child TGID */
  if(thf->f3) child_tgid = ltt_event_get_unsigned(e, thf->f3);
  else child_tgid = 0;

  /* Mathieu : it seems like the process might have been scheduled in before the
   * fork, and, in a rare case, might be the current process. This might happen
   * in a SMP case where we don't have enough precision on the clocks.
   *
   * Test reenabled after precision fixes on time. (Mathieu) */
#if 0 
  zombie_process = lttv_state_find_process(ts, ANY_CPU, child_pid);

  if(unlikely(zombie_process != NULL)) {
    /* Reutilisation of PID. Only now we are sure that the old PID
     * has been released. FIXME : should know when release_task happens instead.
     */
    guint num_cpus = ltt_trace_get_num_cpu(ts->parent.t);
    guint i;
    for(i=0; i< num_cpus; i++) {
      g_assert(zombie_process != ts->running_process[i]);
    }

    exit_process(s, zombie_process);
  }
#endif //0
  g_assert(process->pid != child_pid);
  // FIXME : Add this test in the "known state" section
  // g_assert(process->pid == parent_pid);
  child_process = lttv_state_find_process(ts, ANY_CPU, child_pid);
  if(child_process == NULL) {
    child_process = lttv_state_create_process(ts, process, cpu,
                              child_pid, child_tgid, 
                              LTTV_STATE_UNNAMED, &s->parent.timestamp);
  } else {
    /* The process has already been created :  due to time imprecision between
     * multiple CPUs : it has been scheduled in before creation. Note that we
     * shouldn't have this kind of imprecision.
     *
     * Simply put a correct parent.
     */
    g_assert(0); /* This is a problematic case : the process has been created
                    before the fork event */
    child_process->ppid = process->pid;
    child_process->tgid = child_tgid;
  }
  g_assert(child_process->name == LTTV_STATE_UNNAMED);
  child_process->name = process->name;
  child_process->brand = process->brand;

  return FALSE;
}

/* We stamp a newly created process as kernel_thread */
static gboolean process_kernel_thread(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  guint pid;
  guint cpu = s->cpu;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttvProcessState *process;
  LttvExecutionState *es;

  /* PID */
  pid = ltt_event_get_unsigned(e, thf->f1);
  s->parent.target_pid = pid;

  process = lttv_state_find_process(ts, ANY_CPU, pid);
  es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
  es->t = LTTV_STATE_SYSCALL;
  process->type = LTTV_STATE_KERNEL_THREAD;

  return FALSE;
}

static gboolean process_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  LttField *f;
  guint pid;
  guint cpu = s->cpu;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttvProcessState *process; // = ts->running_process[cpu];

  pid = ltt_event_get_unsigned(e, thf->f1);
  s->parent.target_pid = pid;

  // FIXME : Add this test in the "known state" section
  // g_assert(process->pid == pid);

  process = lttv_state_find_process(ts, ANY_CPU, pid);
  if(likely(process != NULL)) {
    process->state->s = LTTV_STATE_EXIT;
  }
  return FALSE;
}

static gboolean process_free(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  guint release_pid;
  LttvProcessState *process;

  /* PID of the process to release */
  release_pid = ltt_event_get_unsigned(e, thf->f1);
  s->parent.target_pid = release_pid;
  
  g_assert(release_pid != 0);

  process = lttv_state_find_process(ts, ANY_CPU, release_pid);

  if(likely(process != NULL)) {
    /* release_task is happening at kernel level : we can now safely release
     * the data structure of the process */
    //This test is fun, though, as it may happen that 
    //at time t : CPU 0 : process_free
    //at time t+150ns : CPU 1 : schedule out
    //Clearly due to time imprecision, we disable it. (Mathieu)
    //If this weird case happen, we have no choice but to put the 
    //Currently running process on the cpu to 0.
    //I re-enable it following time precision fixes. (Mathieu)
    //Well, in the case where an process is freed by a process on another CPU
    //and still scheduled, it happens that this is the schedchange that will
    //drop the last reference count. Do not free it here!
    guint num_cpus = ltt_trace_get_num_cpu(ts->parent.t);
    guint i;
    for(i=0; i< num_cpus; i++) {
      //g_assert(process != ts->running_process[i]);
      if(process == ts->running_process[i]) {
        //ts->running_process[i] = lttv_state_find_process(ts, i, 0);
        break;
      }
    }
    if(i == num_cpus) /* process is not scheduled */
      exit_process(s, process);
  }

  return FALSE;
}


static gboolean process_exec(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  //gchar *name;
  guint cpu = s->cpu;
  LttvProcessState *process = ts->running_process[cpu];

  /* PID of the process to release */
  guint64 name_len = ltt_event_field_element_number(e, thf->f1);
  //name = ltt_event_get_string(e, thf->f1);
  LttField *child = ltt_event_field_element_select(e, thf->f1, 0);
  gchar *name_begin = 
    (gchar*)(ltt_event_data(e)+ltt_event_field_offset(e, child));
  gchar *null_term_name = g_new(gchar, name_len+1);
  memcpy(null_term_name, name_begin, name_len);
  null_term_name[name_len] = '\0';

  process->name = g_quark_from_string(null_term_name);
  process->brand = LTTV_STATE_UNBRANDED;
  g_free(null_term_name);
  return FALSE;
}

static gboolean thread_brand(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  gchar *name;
  guint cpu = s->cpu;
  LttvProcessState *process = ts->running_process[cpu];

  name = ltt_event_get_string(e, thf->f1);
  process->brand = g_quark_from_string(name);

  return FALSE;
}

static gboolean enum_process_state(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;
  LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
  //It's slow : optimise later by doing this before reading trace.
  LttEventType *et = ltt_event_eventtype(e);
  //
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility *)hook_data;
  guint parent_pid;
  guint pid;
  guint tgid;
  gchar * command;
  guint cpu = s->cpu;
  LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];
  LttvProcessState *parent_process;
  LttField *f4, *f5, *f6, *f7, *f8;
  GQuark type, mode, submode, status;
  LttvExecutionState *es;

  /* PID */
  pid = ltt_event_get_unsigned(e, thf->f1);
  s->parent.target_pid = pid;
  
  /* Parent PID */
  parent_pid = ltt_event_get_unsigned(e, thf->f2);

  /* Command name */
  command = ltt_event_get_string(e, thf->f3);

  /* type */
  f4 = ltt_eventtype_field_by_name(et, LTT_FIELD_TYPE);
  type = ltt_enum_string_get(ltt_field_type(f4),
      ltt_event_get_unsigned(e, f4));

  /* mode */
  f5 = ltt_eventtype_field_by_name(et, LTT_FIELD_MODE);
  mode = ltt_enum_string_get(ltt_field_type(f5), 
      ltt_event_get_unsigned(e, f5));

  /* submode */
  f6 = ltt_eventtype_field_by_name(et, LTT_FIELD_SUBMODE);
  submode = ltt_enum_string_get(ltt_field_type(f6), 
      ltt_event_get_unsigned(e, f6));

  /* status */
  f7 = ltt_eventtype_field_by_name(et, LTT_FIELD_STATUS);
  status = ltt_enum_string_get(ltt_field_type(f7), 
      ltt_event_get_unsigned(e, f7));

  /* TGID */
  f8 = ltt_eventtype_field_by_name(et, LTT_FIELD_TGID);
  if(f8) tgid = ltt_event_get_unsigned(e, f8);
  else tgid = 0;

  /* The process might exist if a process was forked while performing the state 
   * dump. */
  process = lttv_state_find_process(ts, ANY_CPU, pid);
  if(process == NULL) {
    parent_process = lttv_state_find_process(ts, ANY_CPU, parent_pid);
    process = lttv_state_create_process(ts, parent_process, cpu,
                              pid, tgid, g_quark_from_string(command),
                              &s->parent.timestamp);
  
    /* Keep the stack bottom : a running user mode */
    /* Disabled because of inconsistencies in the current statedump states. */
    if(type == LTTV_STATE_KERNEL_THREAD) {
      /* Only keep the bottom */
      process->execution_stack = g_array_set_size(process->execution_stack, 1);
      es = process->state = &g_array_index(process->execution_stack, 
          LttvExecutionState, 0);
      es->t = LTTV_STATE_SYSCALL;
      es->s = status;
      es->n = submode;
    } else {
      /* On top of it : */
      es = process->state = &g_array_index(process->execution_stack, 
          LttvExecutionState, 1);
      es->t = LTTV_STATE_USER_MODE;
      es->s = status;
      es->n = submode;
    }
#if 0
    /* UNKNOWN STATE */
    {
      es = process->state = &g_array_index(process->execution_stack, 
          LttvExecutionState, 1);
      es->t = LTTV_STATE_MODE_UNKNOWN;
      es->s = LTTV_STATE_UNNAMED;
      es->n = LTTV_STATE_SUBMODE_UNKNOWN;
    }
#endif //0
  } else {
    /* The process has already been created :
     * Probably was forked while dumping the process state or
     * was simply scheduled in prior to get the state dump event.
     * We know for sure if it is a user space thread.
     */
    process->ppid = parent_pid;
    process->tgid = tgid;
    process->name = g_quark_from_string(command);
    es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
    if(type != LTTV_STATE_KERNEL_THREAD)
      es->t = LTTV_STATE_USER_MODE;
    /* Don't mess around with the stack, it will eventually become
     * ok after the end of state dump. */
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

  gint ret;
  gint hn;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 18);
    hooks = g_array_set_size(hooks, 18); // Max possible number of hooks.
    hn = 0;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL_ARCH, LTT_EVENT_SYSCALL_ENTRY,
        LTT_FIELD_SYSCALL_ID, 0, 0,
        syscall_entry, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL_ARCH, LTT_EVENT_SYSCALL_EXIT,
        0, 0, 0,
        syscall_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_ENTRY,
        LTT_FIELD_TRAP_ID, 0, 0,
        trap_entry, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_TRAP_EXIT,
        0, 0, 0, 
        trap_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
        LTT_FIELD_IRQ_ID, 0, 0,
        irq_entry, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
        0, 0, 0, 
        irq_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SOFT_IRQ_ENTRY,
        LTT_FIELD_SOFT_IRQ_ID, 0, 0,
        soft_irq_entry, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_KERNEL, LTT_EVENT_SOFT_IRQ_EXIT,
        0, 0, 0, 
        soft_irq_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_SCHEDCHANGE,
        LTT_FIELD_OUT, LTT_FIELD_IN, LTT_FIELD_OUT_STATE,
        schedchange, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FORK,
        LTT_FIELD_PARENT_PID, LTT_FIELD_CHILD_PID, LTT_FIELD_TGID,
        process_fork, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_KERNEL_THREAD,
        LTT_FIELD_PID, 0, 0,
        process_kernel_thread, NULL, &g_array_index(hooks, LttvTraceHook,
          hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_EXIT,
        LTT_FIELD_PID, 0, 0,
        process_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;
    
    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_PROCESS, LTT_EVENT_FREE,
        LTT_FIELD_PID, 0, 0,
        process_free, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_FS, LTT_EVENT_EXEC,
        LTT_FIELD_FILENAME, 0, 0,
        process_exec, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_USER_GENERIC, LTT_EVENT_THREAD_BRAND,
        LTT_FIELD_NAME, 0, 0,
        thread_brand, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

     /* statedump-related hooks */
    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_STATEDUMP, LTT_EVENT_ENUM_PROCESS_STATE,
        LTT_FIELD_PID, LTT_FIELD_PARENT_PID, LTT_FIELD_NAME,
        enum_process_state, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_USER_GENERIC, LTT_EVENT_FUNCTION_ENTRY,
        LTT_FIELD_THIS_FN, LTT_FIELD_CALL_SITE, 0,
        function_entry, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.t,
        LTT_FACILITY_USER_GENERIC, LTT_EVENT_FUNCTION_EXIT,
        LTT_FIELD_THIS_FN, LTT_FIELD_CALL_SITE, 0,
        function_exit, NULL, &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    hooks = g_array_set_size(hooks, hn);
  
    /* Add these hooks to each event_by_id hooks list */

    nb_tracefile = ts->parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext*, j));

      for(k = 0 ; k < hooks->len ; k++) {
        hook = &g_array_index(hooks, LttvTraceHook, k);
        for(l=0;l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_add(
            lttv_hooks_by_id_find(tfs->parent.event_by_id, thf->id),
            thf->h,
            thf,
            LTTV_PRIO_STATE);
        }
      }
    }
    lttv_attribute_find(ts->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
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
    lttv_attribute_find(ts->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
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
                    thf);
        }
      }
    }
    for(k = 0 ; k < hooks->len ; k++)
      lttv_trace_hook_destroy(&g_array_index(hooks, LttvTraceHook, k));
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
    *event_count = 0;
  
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

static gboolean state_save_after_trace_hook(void *hook_data, void *call_data)
{
  LttvTraceState *tcs = (LttvTraceState *)(call_data);
  
  *(tcs->max_time_state_recomputed_in_seek) = tcs->parent.time_span.end_time;

  return FALSE;
}

guint lttv_state_current_cpu(LttvTracefileState *tfs)
{
  return tfs->cpu;
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
  
  lttv_process_traceset_begin(&self->parent,
                NULL, NULL, NULL, NULL, NULL);
  
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

  LttvHooks *after_trace = lttv_hooks_new();
  
  lttv_hooks_add(after_trace,
                 state_save_after_trace_hook,
                 NULL,
                 LTTV_PRIO_STATE);

  
  lttv_process_traceset_end(&self->parent,
                NULL, after_trace, NULL, NULL, NULL);
 
  lttv_hooks_destroy(after_trace);
  
  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {

    ts = (LttvTraceState *)self->parent.traces[i];
    nb_tracefile = ts->parent.tracefiles->len;

    guint *event_count = NULL;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = 
          LTTV_TRACEFILE_STATE(g_array_index(ts->parent.tracefiles,
                                          LttvTracefileContext*, j));
      event_count = lttv_hooks_remove(tfs->parent.event,
                        state_save_event_hook);
    }
    if(event_count) g_free(event_count);
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

  guint call_rest = 0;

  LttvTraceState *tcs;

  LttvAttributeValue value;

  LttvAttributeType type;

  LttvAttributeName name;

  gboolean is_named;

  LttvAttribute *saved_states_tree, *saved_state_tree, *closest_tree;

  //g_tree_destroy(self->parent.pqueue);
  //self->parent.pqueue = g_tree_new(compare_tracefile);
  
  g_info("Entering seek_time_closest for time %lu.%lu", t.tv_sec, t.tv_nsec);
  
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
          type = lttv_attribute_get(saved_states_tree, mid_pos, &name, &value,
              &is_named);
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
        call_rest = 1;
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
  if(!call_rest) g_info("NOT Calling restore");
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
  LTTV_STATE_UNNAMED = g_quark_from_string("UNNAMED");
  LTTV_STATE_UNBRANDED = g_quark_from_string("UNBRANDED");
  LTTV_STATE_MODE_UNKNOWN = g_quark_from_string("MODE_UNKNOWN");
  LTTV_STATE_USER_MODE = g_quark_from_string("USER_MODE");
  LTTV_STATE_SYSCALL = g_quark_from_string("SYSCALL");
  LTTV_STATE_TRAP = g_quark_from_string("TRAP");
  LTTV_STATE_IRQ = g_quark_from_string("IRQ");
  LTTV_STATE_SOFT_IRQ = g_quark_from_string("SOFTIRQ");
  LTTV_STATE_SUBMODE_UNKNOWN = g_quark_from_string("UNKNOWN");
  LTTV_STATE_SUBMODE_NONE = g_quark_from_string("NONE");
  LTTV_STATE_WAIT_FORK = g_quark_from_string("WAIT_FORK");
  LTTV_STATE_WAIT_CPU = g_quark_from_string("WAIT_CPU");
  LTTV_STATE_EXIT = g_quark_from_string("EXIT");
  LTTV_STATE_ZOMBIE = g_quark_from_string("ZOMBIE");
  LTTV_STATE_WAIT = g_quark_from_string("WAIT");
  LTTV_STATE_RUN = g_quark_from_string("RUN");
  LTTV_STATE_DEAD = g_quark_from_string("DEAD");
  LTTV_STATE_USER_THREAD = g_quark_from_string("USER_THREAD");
  LTTV_STATE_KERNEL_THREAD = g_quark_from_string("KERNEL_THREAD");
  LTTV_STATE_TRACEFILES = g_quark_from_string("tracefiles");
  LTTV_STATE_PROCESSES = g_quark_from_string("processes");
  LTTV_STATE_PROCESS = g_quark_from_string("process");
  LTTV_STATE_RUNNING_PROCESS = g_quark_from_string("running_process");
  LTTV_STATE_EVENT = g_quark_from_string("event");
  LTTV_STATE_SAVED_STATES = g_quark_from_string("saved states");
  LTTV_STATE_SAVED_STATES_TIME = g_quark_from_string("saved states time");
  LTTV_STATE_TIME = g_quark_from_string("time");
  LTTV_STATE_HOOKS = g_quark_from_string("saved state hooks");
  LTTV_STATE_NAME_TABLES = g_quark_from_string("name tables");
  LTTV_STATE_TRACE_STATE_USE_COUNT = 
      g_quark_from_string("trace_state_use_count");

  
  LTT_FACILITY_KERNEL     = g_quark_from_string("kernel");
  LTT_FACILITY_KERNEL_ARCH = g_quark_from_string("kernel_arch");
  LTT_FACILITY_PROCESS    = g_quark_from_string("process");
  LTT_FACILITY_FS    = g_quark_from_string("fs");
  LTT_FACILITY_STATEDUMP    = g_quark_from_string("statedump");
  LTT_FACILITY_USER_GENERIC    = g_quark_from_string("user_generic");

  
  LTT_EVENT_SYSCALL_ENTRY = g_quark_from_string("syscall_entry");
  LTT_EVENT_SYSCALL_EXIT  = g_quark_from_string("syscall_exit");
  LTT_EVENT_TRAP_ENTRY    = g_quark_from_string("trap_entry");
  LTT_EVENT_TRAP_EXIT     = g_quark_from_string("trap_exit");
  LTT_EVENT_IRQ_ENTRY     = g_quark_from_string("irq_entry");
  LTT_EVENT_IRQ_EXIT      = g_quark_from_string("irq_exit");
  LTT_EVENT_SOFT_IRQ_ENTRY     = g_quark_from_string("soft_irq_entry");
  LTT_EVENT_SOFT_IRQ_EXIT      = g_quark_from_string("soft_irq_exit");
  LTT_EVENT_SCHEDCHANGE   = g_quark_from_string("schedchange");
  LTT_EVENT_FORK          = g_quark_from_string("fork");
  LTT_EVENT_KERNEL_THREAD = g_quark_from_string("kernel_thread");
  LTT_EVENT_EXIT          = g_quark_from_string("exit");
  LTT_EVENT_FREE          = g_quark_from_string("free");
  LTT_EVENT_EXEC          = g_quark_from_string("exec");
  LTT_EVENT_ENUM_PROCESS_STATE  = g_quark_from_string("enumerate_process_state");
  LTT_EVENT_FUNCTION_ENTRY  = g_quark_from_string("function_entry");
  LTT_EVENT_FUNCTION_EXIT  = g_quark_from_string("function_exit");
  LTT_EVENT_THREAD_BRAND  = g_quark_from_string("thread_brand");


  LTT_FIELD_SYSCALL_ID    = g_quark_from_string("syscall_id");
  LTT_FIELD_TRAP_ID       = g_quark_from_string("trap_id");
  LTT_FIELD_IRQ_ID        = g_quark_from_string("irq_id");
  LTT_FIELD_SOFT_IRQ_ID        = g_quark_from_string("softirq_id");
  LTT_FIELD_OUT           = g_quark_from_string("out");
  LTT_FIELD_IN            = g_quark_from_string("in");
  LTT_FIELD_OUT_STATE     = g_quark_from_string("out_state");
  LTT_FIELD_PARENT_PID    = g_quark_from_string("parent_pid");
  LTT_FIELD_CHILD_PID     = g_quark_from_string("child_pid");
  LTT_FIELD_PID           = g_quark_from_string("pid");
  LTT_FIELD_TGID          = g_quark_from_string("tgid");
  LTT_FIELD_FILENAME      = g_quark_from_string("filename");
  LTT_FIELD_NAME          = g_quark_from_string("name");
  LTT_FIELD_TYPE          = g_quark_from_string("type");
  LTT_FIELD_MODE          = g_quark_from_string("mode");
  LTT_FIELD_SUBMODE       = g_quark_from_string("submode");
  LTT_FIELD_STATUS        = g_quark_from_string("status");
  LTT_FIELD_THIS_FN       = g_quark_from_string("this_fn");
  LTT_FIELD_CALL_SITE     = g_quark_from_string("call_site");
  
}

static void module_destroy() 
{
}


LTTV_MODULE("state", "State computation", \
    "Update the system state, possibly saving it at intervals", \
    module_init, module_destroy)



