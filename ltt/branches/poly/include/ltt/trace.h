#ifndef TRACE_H
#define TRACE_H

#include <ltt/ltt.h>

/* A trace is specified as a pathname to the directory containing all the
   associated data (control tracefiles, per cpu tracefiles, event 
   descriptions...).

   When a trace is closed, all the associated facilities, types and fields
   are released as well. */

LttTrace *ltt_trace_open(char *pathname);

void ltt_trace_close(LttTrace *t); 


/* The characteristics of the system on which the trace was obtained
   is described in a LttSystemDescription structure. */

struct _LttSystemDescription {
  char *description;
  char *node_name;
  char *domain_name;
  unsigned nb_cpu;
  LttArchSize size;
  LttArchEndian endian;
  char *kernel_name;
  char *kernel_release;
  char *kernel_version;
  char *machine;
  char *processor;
  char *hardware_platform;
  char *operating_system;
  unsigned ltt_major_version;
  unsigned ltt_minor_version;
  unsigned ltt_block_size;
  LttTime trace_start;
  LttTime trace_end;
};

LttSystemDescription *ltt_trace_system_description(LttTrace *t);


/* Functions to discover the facilities in the trace. Once the number
   of facilities is known, they may be accessed by position. Multiple
   versions of a facility (same name, different checksum) have consecutive
   positions. */

unsigned ltt_trace_facility_number(LttTrace *t);

LttFacility *ltt_trace_facility_get(LttTrace *t, unsigned i);


/* Look for a facility by name. It returns the number of facilities found
   and sets the position argument to the first found. Returning 0, the named
   facility is unknown, returning 1, the named facility is at the specified
   position, returning n, the facilities are from position to 
   position + n - 1. */

unsigned ltt_trace_facility_find(LttTrace *t, char *name, unsigned *position);


/* Functions to discover all the event types in the trace */

unsigned ltt_trace_eventtype_number(LttTrace *t);

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned i);


/* There is one "per cpu" tracefile for each CPU, numbered from 0 to
   the maximum number of CPU in the system. When the number of CPU installed
   is less than the maximum, some positions are unused. There are also a
   number of "control" tracefiles (facilities, interrupts...). */

unsigned ltt_trace_control_tracefile_number(LttTrace *t);

unsigned ltt_trace_per_cpu_tracefile_number(LttTrace *t);


/* It is possible to search for the tracefiles by name or by CPU position.
   The index within the tracefiles of the same type is returned if found
   and a negative value otherwise. */

int ltt_trace_control_tracefile_find(LttTrace *t, char *name);

int ltt_trace_per_cpu_tracefile_find(LttTrace *t, unsigned i);


/* Get a specific tracefile */

LttTracefile *ltt_trace_control_tracefile_get(LttTrace *t, unsigned i);

LttTracefile *ltt_trace_per_cpu_tracefile_get(LttTrace *t, unsigned i);


/* Get the name of a tracefile */

char *ltt_tracefile_name(LttTracefile *tf);


/* Seek to the first event of the trace with time larger or equal to time */

void ltt_tracefile_seek_time(LttTracefile *t, LttTime time);


/* Read the next event */

LttEvent *ltt_tracefile_read(LttTracefile *t);

#endif // TRACE_H
