#ifndef STATE_H
#define STATE_H

#include <glib.h>
#include <lttv/processTrace.h>

/* The operating system state kept during the trace analysis
   contains a subset of the real operating system state, 
   sufficient for the analysis, and possibly organized quite differently.

   The state information is added to LttvTracesetContext, LttvTraceContext 
   and LttvTracefileContext objects, used by processTrace, through
   subtyping. The context objects already reflect the multiple tracefiles
   (one per cpu) per trace and multiple traces per trace set. The state
   objects defined here simply add fields to the relevant context objects. */

typedef struct _LttvTracesetState LttvTracesetState;
typedef struct _LttvTracesetStateClass LttvTracesetStateClass;

typedef struct _LttvTraceState LttvTraceState;
typedef struct _LttvTraceStateClass LttvTraceStateClass;

typedef struct _LttvTracefileState LttvTracefileState;
typedef struct _LttvTracefileStateClass LttvTracefileStateClass;

gboolean lttv_state_add_event_hooks(LttvTracesetState *self);

gboolean lttv_state_remove_event_hooks(LttvTracesetState *self);

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
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN;


typedef struct _LttvExecutionState {
  LttvExecutionMode t;
  LttvExecutionSubmode n;
  LttTime entry;
  LttTime change;
  LttvProcessStatus s;
} LttvExecutionState;


typedef struct _LttvProcessState {
  guint pid;
  guint ppid;
  LttTime creation_time;
  GQuark name;
  GQuark pid_time;
  GArray *execution_stack;         /* Array of LttvExecutionState */
  LttvExecutionState *state;       /* Top of interrupt stack */
  /* opened file descriptors, address map?... */
} LttvProcessState;


LttvProcessState *lttv_state_find_process(LttvTracefileState *tfs, guint pid);


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

struct _LttvTraceState {
  LttvTraceContext parent;

  GHashTable *processes;  /* LttvProcessState objects indexed by pid */
  /* Block/char devices, locks, memory pages... */
  GQuark *eventtype_names;
  GQuark *syscall_names;
  GQuark *trap_names;
  GQuark *irq_names;
};

struct _LttvTraceStateClass {
  LttvTraceContextClass parent;
};

GType lttv_trace_state_get_type (void);


#define LTTV_TRACEFILE_STATE_TYPE  (lttv_tracefile_state_get_type ())
#define LTTV_TRACEFILE_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileState))
#define LTTV_TRACEFILE_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileStateClass))
#define LTTV_IS_TRACEFILE_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACEFILE_STATE_TYPE))
#define LTTV_IS_TRACEFILE_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACEFILE_STATE_TYPE))
#define LTTV_TRACEFILE_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACEFILE_STATE_TYPE, LttvTracefileStateClass))

struct _LttvTracefileState {
  LttvTracefileContext parent;

  LttvProcessState *process;
  GQuark cpu_name;
};

struct _LttvTracefileStateClass {
  LttvTracefileContextClass parent;
};

GType lttv_tracefile_state_get_type (void);


#endif // STATE_H
