#ifndef STATE_H
#define STATE_H

#include <lttv/processTrace.h>

/* The operating system state kept during the trace analysis
   contains a subset of the real operating system state, 
   sufficient for the analysis, and possibly organized quite differently.

   The state information is added to LttvTraceSetContext, LttvTraceContext 
   and LttvTracefileContext objects, used by processTrace, through
   subtyping. The context objects already reflect the multiple tracefiles
   (one per cpu) per trace and multiple traces per trace set. The state
   objects defined here simply add fields to the relevant context objects. */


/* The LttvProcessState structure defines the current state for each process.
   A process can make system calls (in some rare cases nested) and receive
   interrupts/faults. For instance, a process may issue a system call,
   generate a page fault while reading an argument from user space, and
   get caught by an interrupt. To represent these nested states, an
   interrupt stack is maintained. The stack bottom is normal user mode and
   the top of stack is the current interrupt state.

   The interrupt state tells about the process status, interrupt type and
   interrupt number. All these could be defined as enumerations but may 
   need extensions (e.g. new process state). GQuark are thus used being 
   as easy to manipulate as integers but having a string associated, just
   like enumerations. */


gboolean lttv_state_add_event_hooks(LttvTracesetState *self);

gboolean lttv_state_remove_event_hooks(LttvTracesetState *self);


/* The interrupt type is one of "user mode", "kernel thread", "system call",
   "interrupt request", "fault". */

typedef GQuark LttvInterruptType;


/* The interrupt number depends on the interrupt type. For user mode or kernel
   thread, which are the normal mode (interrupt stack bottom), it is set to
   "none". For interrupt requests, faults and system calls, it is set 
   respectively to the interrupt name (e.g. "timer"), fault name 
   (e.g. "page fault"), and system call name (e.g. "select"). */
 
typedef GQuark LttvInterruptNumber;


/* The process status is one of "running", "wait-cpu" (runnable), or "wait-*"
   where "*" describes the resource waited for (e.g. timer, process, 
   disk...). */

typedef GQuark LttvProcessStatus;


typedef _LttvInterruptState {
  LttvInterruptType t;
  LttvInterruptNumber n;
  LttvTime entry;
  LttvTime last_change;
  LttvProcessStatus s;
} LttvInterruptState;


typedef struct _LttvProcessState {
  guint pid;
  LttvTime birth;
  GQuark name;
  GArray *interrupt_stack;         /* Array of LttvInterruptState */
  LttvInterruptState *state;       /* Top of interrupt stack */
  /* opened file descriptors, address map... */
} LttvProcessState;


/* The LttvTraceSetState, LttvTraceState and LttvTracefileState types
   inherit from the corresponding Context objects defined in processTrace. */

#define LTTV_TRACESET_STATE_TYPE  (lttv_traceset_state_get_type ())
#define LTTV_TRACESET_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACESET_STATE_TYPE, LttvTraceSetState))
#define LTTV_TRACESET_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACESET_STATE_TYPE, LttvTraceSetStateClass))
#define LTTV_IS_TRACESET_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACESET_STATE_TYPE))
#define LTTV_IS_TRACESET_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACESET_STATE_TYPE))
#define LTTV_TRACESET_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACESET_STATE_TYPE, LttvTraceSetStateClass))

typedef struct _LttvTraceSetState LttvTraceSetState;
typedef struct _LttvTraceSetStateClass LttvTraceSetStateClass;

struct _LttvTraceSetState {
  LttvTraceSetContext parent;
};

struct _LttvTracesetStateClass {
  LttvTraceSetClass parent;
};

GType lttv_traceset_state_get_type (void);


#define LTTV_TRACE_STATE_TYPE  (lttv_trace_state_get_type ())
#define LTTV_TRACE_STATE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACE_STATE_TYPE, LttvTraceState))
#define LTTV_TRACE_STATE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACE_STATE_TYPE, LttvTraceStateClass))
#define LTTV_IS_TRACE_STATE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACE_STATE_TYPE))
#define LTTV_IS_TRACE_STATE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACE_STATE_TYPE))
#define LTTV_TRACE_STATE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACE_STATE_TYPE, LttvTraceStateClass))

typedef struct _LttvTraceState LttvTraceState;
typedef struct _LttvTraceStateClass LttvTraceStateClass;

struct _LttvTraceState {
  LttvTraceContext parent;

  GHashTable *processes;  /* LttvProcessState objects indexed by pid */
  /* Block/char devices, locks, memory pages... */
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

typedef struct _LttvTracefileState LttvTracefileState;
typedef struct _LttvTracefileStateClass LttvTracefileStateClass;

struct _LttvTracefileState {
  LttvTracefileContext parent;

  LttvProcessState *process;
};

struct _LttvTracefileStateClass {
  LttvTracefileContextClass parent;
};

GType lttv_tracefile_state_get_type (void);


#endif // PROCESSTRACE_H
