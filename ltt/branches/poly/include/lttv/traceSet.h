#ifndef TRACESET_H
#define TRACESET_H

#include <lttv/attribute.h>
#include <lttv/hook.h>

/* A traceSet is a set of traces to be analyzed together. */

typedef struct _lttv_trace_set lttv_trace_set;


/* Trace sets may be added to, removed from and their content listed. */

lttv_trace_set *lttv_trace_set_new();

lttv_trace_set *lttv_trace_set_destroy(lttv_trace_set *s);

void lttv_trace_set_add(lttv_trace_set *s, ltt_trace *t);

unsigned lttv_trace_set_number(lttv_trace_set *s);

ltt_trace *lttv_trace_set_get(lttv_trace_set *s, unsigned i);

ltt_trace *lttv_trace_set_remove(lttv_trace_set *s, unsigned i);


/* An attributes table is attached to the set and to each trace in the set. */

lttv_attributes *lttv_trace_set_attributes(lttv_trace_set *s);

lttv_attributes *lttv_trace_set_trace_attributes(lttv_trace_set *s, 
    unsigned i);


/* Process the events in a trace set. Lists of hooks are provided to be
   called before and after the trace set and each trace and tracefile.
   For each event, a trace set filter function is called to verify if the
   event is of interest (if it returns TRUE). If this is the case, hooks
   are called for the event, as well as type specific hooks if applicable.
   Any of the hooks lists and the filter may be null if not to be used. */

lttv_trace_set_process(lttv_trace_set *s, 
    lttv_hooks *before_trace_set, lttv_hooks *after_trace_set, 
    char *filter, ltt_time start, ltt_time end);

#endif // TRACESET_H

