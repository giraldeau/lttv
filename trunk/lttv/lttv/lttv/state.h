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

#ifndef STATE_H
#define STATE_H

#include <glib.h>
#include <lttv/tracecontext.h>
#include <stdio.h>

/* The operating system state, kept during the trace analysis,
   contains a subset of the real operating system state, 
   sufficient for the analysis, and possibly organized quite differently.

   The state information is added to LttvTracesetContext, LttvTraceContext 
   and LttvTracefileContext objects, used by process_traceset, through
   subtyping. The context objects already reflect the multiple tracefiles
   (one per cpu) per trace and multiple traces per trace set. The state
   objects defined here simply add fields to the relevant context objects. 

   There is no traceset specific state yet. It may eventually contains such
   things as clock differences over time.

   The trace state currently consists in a process table. 

   The tracefile level state relates to the associated cpu. It contains the
   position of the current event in the tracefile (since the state depends on
   which events have been processed) and a pointer to the current process,
   in the process table, being run on that cpu.

   For each process in the process table, various informations such as exec 
   file name, pid, ppid and creation time are stored. Each process state also
   contains an execution mode stack (e.g. irq within system call, called
   from user mode). */

/* Priority of state hooks */
#define LTTV_PRIO_STATE 25

#define LTTV_STATE_SAVE_INTERVAL 50000

/* Channel Quarks */

extern GQuark
    LTT_CHANNEL_FD_STATE,
    LTT_CHANNEL_GLOBAL_STATE,
    LTT_CHANNEL_IRQ_STATE,
    LTT_CHANNEL_MODULE_STATE,
    LTT_CHANNEL_NETIF_STATE,
    LTT_CHANNEL_SOFTIRQ_STATE,
    LTT_CHANNEL_SWAP_STATE,
    LTT_CHANNEL_SYSCALL_STATE,
    LTT_CHANNEL_TASK_STATE,
    LTT_CHANNEL_VM_STATE,
    LTT_CHANNEL_KPROBE_STATE,
    LTT_CHANNEL_FS,
    LTT_CHANNEL_KERNEL,
    LTT_CHANNEL_MM,
    LTT_CHANNEL_USERSPACE,
    LTT_CHANNEL_BLOCK;

/* Events Quarks */

extern GQuark 
    LTT_EVENT_SYSCALL_ENTRY,
    LTT_EVENT_SYSCALL_EXIT,
    LTT_EVENT_TRAP_ENTRY,
    LTT_EVENT_TRAP_EXIT,
    LTT_EVENT_PAGE_FAULT_ENTRY,
    LTT_EVENT_PAGE_FAULT_EXIT,
    LTT_EVENT_PAGE_FAULT_NOSEM_ENTRY,
    LTT_EVENT_PAGE_FAULT_NOSEM_EXIT,
    LTT_EVENT_IRQ_ENTRY,
    LTT_EVENT_IRQ_EXIT,
    LTT_EVENT_SOFT_IRQ_RAISE,
    LTT_EVENT_SOFT_IRQ_ENTRY,
    LTT_EVENT_SOFT_IRQ_EXIT,
    LTT_EVENT_SCHED_SCHEDULE,
    LTT_EVENT_PROCESS_FORK,
    LTT_EVENT_KTHREAD_CREATE,
    LTT_EVENT_PROCESS_EXIT,
    LTT_EVENT_PROCESS_FREE,
    LTT_EVENT_EXEC,
    LTT_EVENT_PROCESS_STATE,
    LTT_EVENT_STATEDUMP_END,
    LTT_EVENT_FUNCTION_ENTRY,
    LTT_EVENT_FUNCTION_EXIT,
    LTT_EVENT_THREAD_BRAND,
    LTT_EVENT_REQUEST_ISSUE,
    LTT_EVENT_REQUEST_COMPLETE,
    LTT_EVENT_LIST_INTERRUPT,
    LTT_EVENT_SYS_CALL_TABLE,
    LTT_EVENT_SOFTIRQ_VEC,
    LTT_EVENT_KPROBE_TABLE,
    LTT_EVENT_KPROBE;

/* Fields Quarks */

extern GQuark 
    LTT_FIELD_SYSCALL_ID,
    LTT_FIELD_TRAP_ID,
    LTT_FIELD_IRQ_ID,
    LTT_FIELD_SOFT_IRQ_ID,
    LTT_FIELD_PREV_PID,
    LTT_FIELD_NEXT_PID,
    LTT_FIELD_PREV_STATE,
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
    LTT_FIELD_CALL_SITE,
    LTT_FIELD_MINOR,
    LTT_FIELD_MAJOR,
    LTT_FIELD_OPERATION,
    LTT_FIELD_ACTION,
    LTT_FIELD_ID,
    LTT_FIELD_ADDRESS,
    LTT_FIELD_SYMBOL,
    LTT_FIELD_IP;

typedef struct _LttvTracesetState LttvTracesetState;
typedef struct _LttvTracesetStateClass LttvTracesetStateClass;

typedef struct _LttvTraceState LttvTraceState;
typedef struct _LttvTraceStateClass LttvTraceStateClass;

typedef struct _LttvTracefileState LttvTracefileState;
typedef struct _LttvTracefileStateClass LttvTracefileStateClass;

gint lttv_state_hook_add_event_hooks(void *hook_data, void *call_data);
void lttv_state_add_event_hooks(LttvTracesetState *self);

gint lttv_state_hook_remove_event_hooks(void *hook_data, void *call_data);
void lttv_state_remove_event_hooks(LttvTracesetState *self);

void lttv_state_save_add_event_hooks(LttvTracesetState *self);
// Hook wrapper. call_data is a trace context.
gint lttv_state_save_hook_add_event_hooks(void *hook_data, void *call_data);

void lttv_state_save_remove_event_hooks(LttvTracesetState *self);
// Hook wrapper. call_data is a trace context.
gint lttv_state_save_hook_remove_event_hooks(void *hook_data, void *call_data);

void lttv_state_traceset_seek_time_closest(LttvTracesetState *self, LttTime t);

/* The LttvProcessState structure defines the current state for each process.
   A process can make system calls (in some rare cases nested) and receive
   interrupts/faults. For instance, a process may issue a system call,
   generate a page fault while reading an argument from user space, and
   get caught by an interrupt. To represent these nested states, an
   execution mode stack is maintained. The stack bottom is normal user mode 
   and the top of stack is the current execution mode.

   The execution mode stack tells about the process status, execution mode and
   submode (interrupt, system call or IRQ number). All these could be 
   defined as enumerations but may need extensions (e.g. new process state). 
   GQuark are thus used. They are as easy to manipulate as integers but have
   a string associated, just like enumerations.

   The execution mode is one of "user mode", "kernel thread", "system call",
   "interrupt request", "fault". */

typedef GQuark LttvExecutionMode;

extern LttvExecutionMode
  LTTV_STATE_USER_MODE,
  LTTV_STATE_SYSCALL,
  LTTV_STATE_TRAP,
  LTTV_STATE_IRQ,
  LTTV_STATE_SOFT_IRQ,
  LTTV_STATE_MODE_UNKNOWN;


/* The submode number depends on the execution mode. For user mode or kernel
   thread, which are the normal mode (execution mode stack bottom), 
   it is set to "none". For interrupt requests, faults and system calls, 
   it is set respectively to the interrupt name (e.g. "timer"), fault name 
   (e.g. "page fault"), and system call name (e.g. "select"). */
 
typedef GQuark LttvExecutionSubmode;

extern LttvExecutionSubmode
  LTTV_STATE_SUBMODE_NONE,
  LTTV_STATE_SUBMODE_UNKNOWN;

/* The process status is one of "running", "wait-cpu" (runnable), or "wait-*"
   where "*" describes the resource waited for (e.g. timer, process, 
   disk...). */

typedef GQuark LttvProcessStatus;

extern LttvProcessStatus
  LTTV_STATE_UNNAMED,
  LTTV_STATE_WAIT_FORK,
  LTTV_STATE_WAIT_CPU,
  LTTV_STATE_EXIT,
  LTTV_STATE_ZOMBIE,
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN,
  LTTV_STATE_DEAD;

extern GQuark
  LTTV_STATE_UNBRANDED;

typedef GQuark LttvProcessType;

extern LttvProcessType
  LTTV_STATE_USER_THREAD,
  LTTV_STATE_KERNEL_THREAD;

typedef GQuark LttvCPUMode;
extern LttvCPUMode
  LTTV_CPU_UNKNOWN,
  LTTV_CPU_IDLE,
  LTTV_CPU_BUSY,
  LTTV_CPU_IRQ,
  LTTV_CPU_SOFT_IRQ,
  LTTV_CPU_TRAP;

typedef GQuark LttvIRQMode;
extern LttvIRQMode
  LTTV_IRQ_UNKNOWN,
  LTTV_IRQ_IDLE,
  LTTV_IRQ_BUSY;

typedef GQuark LttvBdevMode;
extern LttvBdevMode
  LTTV_BDEV_UNKNOWN,
  LTTV_BDEV_IDLE,
  LTTV_BDEV_BUSY_READING,
  LTTV_BDEV_BUSY_WRITING;

typedef struct _LttvExecutionState {
  LttvExecutionMode t;
  LttvExecutionSubmode n;
  LttTime entry;
  LttTime change;
  LttTime cum_cpu_time;
  LttvProcessStatus s;
} LttvExecutionState;

typedef struct _LttvProcessState {
  guint pid;
  guint tgid;
  guint ppid;
  LttTime creation_time;
  LttTime insertion_time;
  GQuark name;
  GQuark brand;
  GQuark pid_time;
  GArray *execution_stack;         /* Array of LttvExecutionState */
  LttvExecutionState *state;       /* Top of interrupt stack */
      /* WARNING : each time the execution_stack size is modified, the state
       * must be reget : g_array_set_size can have to move the array.
       * (Mathieu) */
  guint cpu;                /* CPU where process is scheduled (being either in
                               the active or inactive runqueue)*/
//  guint  last_tracefile_index;    /* index in the trace for cpu tracefile */
  LttvTracefileState  *usertrace;    /* Associated usertrace */
  /* opened file descriptors, address map?... */
  GArray *user_stack;          /* User space function call stack */
  guint64  current_function;
  LttvProcessType type;        /* kernel thread or user space ? */
  guint target_pid; /* target PID of the current event. */
  guint free_events; /* 0 : none, 1 : free or exit dead, 2 : should delete */
} LttvProcessState;

#define ANY_CPU 0 /* For clarity sake : a call to lttv_state_find_process for
                     a PID != 0 will search on any cpu automatically. */

LttvProcessState *
lttv_state_find_process(LttvTraceState *ts, guint cpu, guint pid);

LttvProcessState *
lttv_state_find_process_or_create(LttvTraceState *ts, guint cpu, guint pid,
    const LttTime *timestamp);

LttvProcessState *
lttv_state_create_process(LttvTraceState *tcs, LttvProcessState *parent, 
    guint cpu, guint pid, guint tgid, GQuark name, const LttTime *timestamp);

void lttv_state_write(LttvTraceState *self, LttTime t, FILE *fp);
void lttv_state_write_raw(LttvTraceState *self, LttTime t, FILE *fp);

/* The LttvTracesetState, LttvTraceState and LttvTracefileState types
   inherit from the corresponding Context objects defined in processTrace. */

#define LTTV_TRACESET_STATE_TYPE  (lttv_traceset_state_get_type ())
#define LTTV_TRACESET_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACESET_STATE_TYPE, LttvTracesetState))
#define LTTV_TRACESET_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACESET_STATE_TYPE, LttvTracesetStateClass))
#define LTTV_IS_TRACESET_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACESET_STATE_TYPE))
#define LTTV_IS_TRACESET_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACESET_STATE_TYPE))
#define LTTV_TRACESET_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACESET_STATE_TYPE, LttvTracesetStateClass))

struct _LttvTracesetState {
  LttvTracesetContext parent;
};

struct _LttvTracesetStateClass {
  LttvTracesetContextClass parent;
};

GType lttv_traceset_state_get_type (void);


#define LTTV_TRACE_STATE_TYPE  (lttv_trace_state_get_type ())
#define LTTV_TRACE_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACE_STATE_TYPE, LttvTraceState))
#define LTTV_TRACE_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACE_STATE_TYPE, LttvTraceStateClass))
#define LTTV_IS_TRACE_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACE_STATE_TYPE))
#define LTTV_IS_TRACE_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACE_STATE_TYPE))
#define LTTV_TRACE_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACE_STATE_TYPE, LttvTraceStateClass))

typedef struct _LttvCPUState {
  GArray *mode_stack;
  gint last_irq;
  gint last_soft_irq;
  gint last_trap;
} LttvCPUState;

typedef struct _LttvIRQState {
  GArray *mode_stack;
} LttvIRQState;

typedef struct _LttvSoftIRQState {
  guint pending; /* number of times it is pending */
  guint running; /* number of times it is currently running (on different processors) */
} LttvSoftIRQState;

typedef struct _LttvTrapState {
  guint running; /* number of times it is currently running (on different processors) */
} LttvTrapState;

typedef struct _LttvBdevState {
  GArray *mode_stack;
} LttvBdevState;

struct _LttvTraceState {
  LttvTraceContext parent;

  GHashTable *processes;  /* LttvProcessState objects indexed by pid and
                             last_cpu */
  GHashTable *usertraces;  /* GPtrArray objects indexed by pid, containing
                             pointers to LttvTracefileState objects. */
  guint nb_event, save_interval;
  /* Block/char devices, locks, memory pages... */
  GQuark *eventtype_names;
  GQuark *syscall_names;
  guint  nb_syscalls;
  GQuark *trap_names;
  guint  nb_traps;
  guint  nb_irqs;
  guint  nb_soft_irqs;
  GQuark *irq_names;
  GQuark *soft_irq_names;
  LttTime *max_time_state_recomputed_in_seek;
  GHashTable *kprobe_hash;

  /* Array of per cpu running process */
  LttvProcessState **running_process;
  gboolean has_precomputed_states;
  LttvCPUState *cpu_states; /* state of each cpu */
  LttvIRQState *irq_states; /* state of each irq handler */
  LttvSoftIRQState *soft_irq_states; /* state of each softirq */
  LttvTrapState *trap_states; /* state of each trap */
  GHashTable *bdev_states; /* state of the block devices */
};

struct _LttvTraceStateClass {
  LttvTraceContextClass parent;

  void (*state_save) (LttvTraceState *self, LttvAttribute *container);
  void (*state_restore) (LttvTraceState *self, LttvAttribute *container);
  void (*state_saved_free) (LttvTraceState *self, LttvAttribute *container);
};

GType lttv_trace_state_get_type (void);

void lttv_state_save(LttvTraceState *self, LttvAttribute *container);

void lttv_state_restore(LttvTraceState *self, LttvAttribute *container);

void lttv_state_state_saved_free(LttvTraceState *self, 
    LttvAttribute *container);

int lttv_state_pop_state_cleanup(LttvProcessState *process,
    LttvTracefileState *tfs);

#define LTTV_TRACEFILE_STATE_TYPE  (lttv_tracefile_state_get_type ())
#define LTTV_TRACEFILE_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileState))
#define LTTV_TRACEFILE_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileStateClass))
#define LTTV_IS_TRACEFILE_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACEFILE_STATE_TYPE))
#define LTTV_IS_TRACEFILE_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACEFILE_STATE_TYPE))
#define LTTV_TRACEFILE_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileStateClass))

struct _LttvTracefileState {
  LttvTracefileContext parent;

  GQuark tracefile_name;
  guint cpu;  /* Current cpu of the tracefile */ /* perhaps merge in cpu_state */
  LttvCPUState *cpu_state; /* cpu resource state */
};

struct _LttvTracefileStateClass {
  LttvTracefileContextClass parent;
};

GType lttv_tracefile_state_get_type (void);

static inline guint lttv_state_get_target_pid(LttvTracefileState *tfs)
{
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  guint cpu = tfs->cpu;
  LttvProcessState *process = ts->running_process[cpu];

  if(tfs->parent.target_pid >= 0) return tfs->parent.target_pid;
  else return process->pid;
}


#define HDR_PROCESS 0
#define HDR_ES 1
#define HDR_USER_STACK 2
#define HDR_USERTRACE 3
#define HDR_PROCESS_STATE 4
#define HDR_CPU 5
#define HDR_TRACEFILE 6
#define HDR_TRACESET 7
#define HDR_TRACE 8
#define HDR_QUARKS 9
#define HDR_QUARK 10

/* Device number manipulation macros from kernel source */
#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)
#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))

#endif // STATE_H
