#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <ltt/ltt.h>

/* A trace is specified as a pathname to the directory containing all the
   associated data (control tracefile, per cpu tracefiles, event 
   descriptions...).

   When a trace is closed, all the associated facilities, types and fields
   are released as well. */

ltt_trace *ltt_trace_open(char *pathname);

void ltt_trace_close(ltt_trace *t); 


/* A trace may be queried for its architecture type (e.g., "i386", 
   "powerpc", "powerpcle", "s390", "s390x"), its architecture variant 
   (e.g., "att" versus "sun" for m68k), its operating system (e.g., "linux",
   "bsd"), its generic architecture, and the machine identity (e.g., system
   host name). All character strings belong to the associated tracefile 
   and are freed when it is closed. */


char *ltt_tracefile_arch_type(ltt_trace *t); 

char *ltt_tracefile_arch_variant(ltt_trace *t);

char *ltt_tracefile_system_type(ltt_trace *t);

ltt_arch_size ltt_tracefile_arch_size(ltt_trace *t);

ltt_arch_endian ltt_tracefile_arch_endian(ltt_trace *t); 


/* Hostname of the system where the trace was recorded */

char *ltt_trace_system_name(ltt_tracefile *t);


/* SMP multi-processors have 2 or more CPUs */

unsigned ltt_trace_cpu_number(ltt_trace *t);


/* Start and end time of the trace and its duration */

ltt_time ltt_tracefile_time_start(ltt_trace *t);

ltt_time ltt_tracefile_time_end(ltt_trace *t);

ltt_time ltt_tracefile_duration(ltt_tracefile *t);


/* Functions to discover the facilities in the trace */

unsigned ltt_trace_facility_number(ltt_trace *t);

ltt_facility *ltt_trace_facility_get(ltt_trace *t, unsigned i);

ltt_facility *ltt_trace_facility_get_by_name(ltt_trace *t, char *name);


/* Functions to discover all the event types in the trace */

unsigned ltt_trace_eventtype_number(ltt_tracefile *t);

ltt_eventtype *ltt_trace_eventtype_get(ltt_tracefile *t, unsigned i);


/* A trace typically contains one "control" tracefile with important events
   (for all CPUs), and one tracefile with ordinary events per cpu.
   The tracefiles in a trace may be enumerated for each category
   (all cpu and per cpu). The total number of tracefiles and of CPUs
   may also be obtained. */

unsigned int ltt_trace_tracefile_number(ltt_trace *t);

unsigned int ltt_trace_tracefile_number_per_cpu(ltt_trace *t);

unsigned int ltt_trace_tracefile_number_all_cpu(ltt_trace *t);

ltt_tracefile *ltt_trace_tracefile_get_per_cpu(ltt_trace *t, unsigned i);

ltt_tracefile *ltt_trace_tracefile_get_all_cpu(ltt_trace *t, unsigned i);

char *ltt_tracefile_name(ltt_tracefile *tf);


/* Seek to the first event of the trace with time larger or equal to time */

int ltt_tracefile_seek_time(ltt_tracefile *t, ltt_time time);


/* Read the next event */

ltt_event *ltt_tracefile_read(ltt_tracefile *t);

#endif // TRACE_H
