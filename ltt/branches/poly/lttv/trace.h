#ifndef TRACE_H
#define TRACE_H

/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

typedef struct _lttv_trace_set lttv_trace_set;

typedef struct _lttv_trace lttv_trace;

typedef struct _lttv_tracefile lttv_tracefile;


/* Trace sets may be added to, removed from and their content listed. */

lttv_trace_set *lttv_trace_set_new();

lttv_trace_set *lttv_trace_set_destroy(lttv_trace_set *s);

void lttv_trace_set_add(lttv_trace_set *s, lttv_trace *t);

unsigned lttv_trace_set_number(lttv_trace_set *s);

lttv_trace *lttv_trace_set_get(lttv_trace_set *s, unsigned i);

lttv_trace *lttv_trace_set_remove(lttv_trace_set *s, unsigned i);


/* A trace is identified by the pathname of its containing directory */

lttv_trace *lttv_trace_open(char *pathname);

int lttv_trace_close(lttv_trace *t);

char *lttv_trace_name(lttv_trace *t);


/* A trace typically contains one tracefile with important events
   (for all CPUs), and one tracefile with ordinary events per cpu.
   The tracefiles in a trace may be enumerated for each category
   (all cpu and per cpu). The total number of tracefiles and of CPUs
   may also be obtained. */

unsigned int lttv_trace_tracefile_number(lttv_trace *t);

unsigned int lttv_trace_cpu_number(lttv_trace *t);

unsigned int lttv_trace_tracefile_number_per_cpu(lttv_trace *t);

unsigned int lttv_trace_tracefile_number_all_cpu(lttv_trace *t);

lttv_tracefile *lttv_trace_tracefile_get_per_cpu(lttv_trace *t, unsigned i);

lttv_tracefile *lttv_trace_tracefile_get_all_cpu(lttv_trace *t, unsigned i);


/* A set of attributes is attached to each trace set, trace and tracefile
   to store user defined data as needed. */

lttv_attributes *lttv_trace_set_attributes(lttv_trace_set *s);

lttv_attributes *lttv_trace_attributes(lttv_trace *t);

lttv_attributes *lttv_tracefile_attributes(lttv_tracefile *tf);


/* The underlying ltt_tracefile, from which events may be read, is accessible.
   The tracefile name is also available. */

lttv_tracefile *lttv_tracefile_ltt_tracefile(lttv_tracefile *tf);

char *lttv_tracefile_name(lttv_tracefile *tf);

#endif // TRACE_H

