#ifndef PROCESSTRACE_H
#define PROCESSTRACE_H

#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <ltt/ltt.h>

/* This is the generic part of trace processing. All events within a
   certain time interval are accessed and processing hooks are called for
   each. The events are examined in monotonically increasing time to more
   closely follow the traced system behavior. 

   Hooks are called at several different places during the processing:
   before traceset, after traceset, check trace, before trace, after trace, 
   check tracefile, before tracefile, after tracefile, 
   check_event, before_event, before_event_by_id, 
   after_event, after_event_by_id.

   In each case the "check" hooks are called first to determine if further
   processing of the trace, tracefile or event is wanted. Then, the before
   hooks and the after hooks are called. The before hooks for a traceset
   are called before those for the contained traces, which are called before
   those for the contained tracefiles. The after hooks are called in reverse
   order. The event hooks are called after all the before_tracefile hooks
   and before all the after_tracefile hooks.

   The hooks receive two arguments, the hook_data and call_data. The hook_data
   is specified when the hook is registered and typically links to the
   object registering the hook (e.g. a graphical events viewer). The call_data
   must contain all the context related to the call. The traceset hooks receive
   the LttvTracesetContext provided by the caller. The trace hooks receive
   the LttvTraceContext from the traces array in the LttvTracesetContext.
   The tracefile and event hooks receive the LttvTracefileContext from
   the tracefiles array in the LttvTraceContext. The LttEvent and LttTime
   fields in the tracefile context are set to the current event and current
   event time before calling the event hooks. No other context field is 
   modified.

   The contexts in the traces and tracefiles arrays must be allocated by 
   the caller, either before the call or during the before hooks of the 
   enclosing traceset or trace. The order in the traces array must
   correspond to the lttv_traceset_get function. The order in the tracefiles
   arrays must correspond to the ltt_trace_control_tracefile_get and
   ltt_trace_per_cpu_tracefile_get functions. The traceset, trace and
   tracefile contexts may be subtyped as needed. Indeed, both the contexts
   and the hooks are defined by the caller. */

#define LTTV_TRACESET_CONTEXT_TYPE  (lttv_traceset_context_get_type ())
#define LTTV_TRACESET_CONTEXT(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContext))
#define LTTV_TRACESET_CONTEXT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContextClass))
#define LTTV_IS_TRACESET_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACESET_CONTEXT_TYPE))
#define LTTV_IS_TRACESET_CONTEXT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACESET_CONTEXT_TYPE))
#define LTTV_TRACESET_CONTEXT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContextClass))

typedef struct _LttvTracesetContext LttvTracesetContext;
typedef struct _LttvTracesetContextClass LttvTracesetContextClass;

struct _LttvTracesetContext {
  GObject parent;

  LttvTraceset *ts;
  LttvHooks *before;
  LttvHooks *after;
  LttvTraceContext **traces;
  LttvAttribute *a;
};

struct _LttvTracesetContextClass {
  GObjectClass parent;

  void (*init) (LttvTracesetContext *self, LttvTraceset *ts);
  void (*fini) (LttvTracesetContext *self);
  LttvTracesetContext* (*new_traceset_context) (LttvTracesetContext *self);
  LttvTraceContext* (*new_trace_context) (LttvTracesetContext *self);
  LttvTracefileContext* (*new_tracefile_context) (LttvTracesetContext *self);
};

GType lttv_traceset_context_get_type (void);

void lttv_context_init(LttvTracesetContext *self, LttvTraceset *ts);

void lttv_context_fini(LttvTracesetContext *self);

LttvTracesetContext *
lttv_context_new_traceset_context(LttvTracesetContext *self);

LttvTraceContext * 
lttv_context_new_trace_context(LttvTracesetContext *self);

LttvTracefileContext *
lttv_context_new_tracefile_context(LttvTracesetContext *self);


#define LTTV_TRACE_CONTEXT_TYPE  (lttv_trace_context_get_type ())
#define LTTV_TRACE_CONTEXT(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACE_CONTEXT_TYPE, LttvTraceContext))
#define LTTV_TRACE_CONTEXT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACE_CONTEXT_TYPE, LttvTraceContextClass))
#define LTTV_IS_TRACE_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACE_CONTEXT_TYPE))
#define LTTV_IS_TRACE_CONTEXT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACE_CONTEXT_TYPE))
#define LTTV_TRACE_CONTEXT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACE_CONTEXT_TYPE, LttvTraceContextClass))

typedef struct _LttvTraceContext LttvTraceContext;
typedef struct _LttvTraceContextClass LttvTraceContextClass;

struct _LttvTraceContext {
  GObject parent;

  LttvTracesetContext *ts_context;
  guint index;                /* in ts_context->traces */
  LttvTrace *t;
  LttvHooks *check;
  LttvHooks *before;
  LttvHooks *after;
  LttvTracefileContext **control_tracefiles;
  LttvTracefileContext **per_cpu_tracefiles;
  LttvAttribute *a;
};

struct _LttvTraceContextClass {
  GObjectClass parent;
};

GType lttv_trace_context_get_type (void);

#define LTTV_TRACEFILE_CONTEXT_TYPE  (lttv_tracefile_context_get_type ())
#define LTTV_TRACEFILE_CONTEXT(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACEFILE_CONTEXT_TYPE, LttvTracefileContext))
#define LTTV_TRACEFILE_CONTEXT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACEFILE_CONTEXT_TYPE, LttvTracefileContextClass))
#define LTTV_IS_TRACEFILE_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACEFILE_CONTEXT_TYPE))
#define LTTV_IS_TRACEFILE_CONTEXT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACEFILE_CONTEXT_TYPE))
#define LTTV_TRACEFILE_CONTEXT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACEFILE_CONTEXT_TYPE, LttvTracefileContextClass))

typedef struct _LttvTracefileContext LttvTracefileContext;
typedef struct _LttvTracefileContextClass LttvTracefileContextClass;

struct _LttvTracefileContext {
  GObject parent;

  LttvTraceContext *t_context;
  gboolean control;
  guint index;                /* in ts_context->control/per_cpu_tracefiles */
  LttvTracefile *tf;
  LttvHooks *check;
  LttvHooks *before;
  LttvHooks *after;
  LttvEvent *e;
  LttvHooks *check_event;
  LttvHooks *before_event;
  LttvHooksById *before_event_by_id;
  LttvHooks *after_event;
  LttvHooksById *after_event_by_id;
  LttTime *time;
  LttvAttribute *a;
};

struct _LttvTracefileContextClass {
  GObjectClass parent;
};

GType lttv_tracefile_context_get_type (void);

void lttv_process_trace(LttvTime start, LttvTime end, LttvTraceset *traceset, 
    LttvTracesetContext *context);

void lttv_traceset_context_add_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset, 
    LttvHooks *after_traceset,
    LttvHooks *check_trace, 
    LttvHooks *before_trace, 
    LttvHooks *after_trace, 
    LttvHooks *check_event, 
    LttvHooks *before_event, 
    LttvHooks *after_event)

void lttv_traceset_context_remove_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset, 
    LttvHooks *after_traceset,
    LttvHooks *check_trace, 
    LttvHooks *before_trace, 
    LttvHooks *after_trace, 
    LttvHooks *check_event, 
    LttvHooks *before_event, 
    LttvHooks *after_event)

#endif // PROCESSTRACE_H
