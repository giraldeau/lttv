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

#ifndef PROCESSTRACE_H
#define PROCESSTRACE_H

#include <lttv/traceset.h>
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


typedef struct _LttvTracesetContext LttvTracesetContext;
typedef struct _LttvTracesetContextClass LttvTracesetContextClass;

typedef struct _LttvTraceContext LttvTraceContext;
typedef struct _LttvTraceContextClass LttvTraceContextClass;

typedef struct _LttvTracefileContext LttvTracefileContext;
typedef struct _LttvTracefileContextClass LttvTracefileContextClass;

typedef struct _LttvTracesetContextPosition LttvTracesetContextPosition;
typedef struct _LttvTraceContextPosition LttvTraceContextPosition;

#define LTTV_TRACESET_CONTEXT_TYPE  (lttv_traceset_context_get_type ())
#define LTTV_TRACESET_CONTEXT(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContext))
#define LTTV_TRACESET_CONTEXT_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContextClass))
#define LTTV_IS_TRACESET_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACESET_CONTEXT_TYPE))
#define LTTV_IS_TRACESET_CONTEXT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACESET_CONTEXT_TYPE))
#define LTTV_TRACESET_CONTEXT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACESET_CONTEXT_TYPE, LttvTracesetContextClass))

struct _LttvTracesetContext {
  GObject parent;

  LttvTraceset *ts;
  LttvTraceContext **traces;
  LttvAttribute *a;
  LttvAttribute *ts_a;
  TimeInterval time_span;
  GTree *pqueue;
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

struct _LttvTraceContext {
  GObject parent;

  LttvTracesetContext *ts_context;
  guint index;                /* in ts_context->traces */
  LttTrace *t;
  LttvTrace *vt;
  LttvTracefileContext **tracefiles;
  LttvAttribute *a;
  LttvAttribute *t_a;
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

struct _LttvTracefileContext {
  GObject parent;

  LttvTraceContext *t_context;
  gboolean control;
  guint index;                /* in ts_context->tracefiles */
  LttTracefile *tf;
  LttEvent *e;
  LttvHooks *event;
  LttvHooksById *event_by_id;
  LttTime timestamp;
  LttvAttribute *a;
};

struct _LttvTracefileContextClass {
  GObjectClass parent;
};

GType lttv_tracefile_context_get_type (void);

/* Run through the events in a traceset in sorted order calling all the
   hooks appropriately. It starts at the current time and runs until end or
   nb_events are processed. */

void lttv_process_traceset(LttvTracesetContext *self, LttTime end, 
    unsigned nb_events);

/* Save the complete tracefile information in the context */
//void lttv_process_traceset_get_sync_data(LttvTracesetContext *self);

/* Put all the tracefiles at the tracefile context position */
void lttv_process_traceset_synchronize_tracefiles(LttvTracesetContext *self);

/* Process traceset can also be done in smaller pieces calling begin,
 * then seek and middle repeatedly, and end. The middle function return the
 * number of events processed. It will be smaller than nb_events if the end time
 * or end position is reached. */


void lttv_process_traceset_begin(LttvTracesetContext *self,
                                 LttvHooks       *before_traceset,
                                 LttvHooks       *before_trace,
                                 LttvHooks       *before_tracefile,
                                 LttvHooks       *event,
                                 LttvHooksById   *event_by_id);


guint lttv_process_traceset_middle(LttvTracesetContext *self,
                              LttTime end, 
                              guint nb_events,
                              const LttvTracesetContextPosition *end_position);

void lttv_process_traceset_end(LttvTracesetContext *self,
                               LttvHooks           *after_traceset,
                               LttvHooks           *after_trace,
                               LttvHooks           *after_tracefile,
                               LttvHooks           *event,
                               LttvHooksById       *event_by_id);


void lttv_process_traceset_seek_time(LttvTracesetContext *self, LttTime start);

gboolean lttv_process_traceset_seek_position(LttvTracesetContext *self, 
                                        const LttvTracesetContextPosition *pos);

void lttv_process_trace_seek_time(LttvTraceContext *self, LttTime start);

void lttv_traceset_context_add_hooks(LttvTracesetContext *self,
    LttvHooks *before_traceset,
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event,
    LttvHooksById *event_by_id);

void lttv_traceset_context_remove_hooks(LttvTracesetContext *self,
    LttvHooks *after_traceset,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id);

void lttv_trace_context_add_hooks(LttvTraceContext *self,
    LttvHooks *before_trace, 
    LttvHooks *before_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id);

void lttv_trace_context_remove_hooks(LttvTraceContext *self,
    LttvHooks *after_trace, 
    LttvHooks *after_tracefile,
    LttvHooks *event, 
    LttvHooksById *event_by_id);

void lttv_tracefile_context_add_hooks(LttvTracefileContext *self,
          LttvHooks *before_tracefile,
          LttvHooks *event, 
          LttvHooksById *event_by_id);


void lttv_tracefile_context_remove_hooks(LttvTracefileContext *self,
           LttvHooks *after_tracefile,
           LttvHooks *event, 
           LttvHooksById *event_by_id);


void lttv_tracefile_context_add_hooks_by_id(LttvTracefileContext *self,
					    unsigned i,
					    LttvHooks *event_by_id);

void lttv_tracefile_context_remove_hooks_by_id(LttvTracefileContext *self,
					       unsigned i);

typedef struct _LttvTraceHook {
  LttvHook h;
  guint id;
  LttField *f1;
  LttField *f2;
  LttField *f3;
} LttvTraceHook;


/* Search in the trace for the id of the named event type within the named
   facility. Then, find the three (if non null) named fields. All that
   information is then used to fill the LttvTraceHook structure. This
   is useful to find the specific id for an event within a trace, for
   registering a hook using this structure as event data;
   it already contains the (up to three) needed fields handles. */
 
void lttv_trace_find_hook(LttTrace *t, char *facility, char *event_type,
    char *field1, char *field2, char *field3, LttvHook h, LttvTraceHook *th);

LttvTracefileContext *lttv_traceset_context_get_current_tfc(
                             LttvTracesetContext *self);


LttvTracesetContextPosition *lttv_traceset_context_position_new();

void lttv_traceset_context_position_save(const LttvTracesetContext *self,
                                    LttvTracesetContextPosition *pos);

void lttv_traceset_context_position_destroy(LttvTracesetContextPosition *pos);

void lttv_traceset_context_position_copy(LttvTracesetContextPosition *dest,
                                   const LttvTracesetContextPosition *src);

gint lttv_traceset_context_pos_pos_compare(
                          const LttvTracesetContextPosition *pos1,
                          const LttvTracesetContextPosition *pos2);

gint lttv_traceset_context_ctx_pos_compare(const LttvTracesetContext *self,
                                    const LttvTracesetContextPosition *pos2);

LttTime lttv_traceset_context_position_get_time(
                                      const LttvTracesetContextPosition *pos);


#endif // PROCESSTRACE_H
