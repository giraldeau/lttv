#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <ltt/ltt.h>

/* A tracefile is specified as a pathname. Facilities must be added to the
   tracefile to declare the type of the contained events. 

   The ltt_tracefile_facility_add call increases the facility 
   usage count and also specifies the base of the numeric range 
   assigned to the event types in the facility for this tracefile. 
   This information is normally obtained through "facility used" events 
   stored in the tracefile.

   When a tracefile is closed, all the associated facilities may be 
   automatically closed as well, if their usage count is 0, when the 
   close_facilities argument is true. */

ltt_tracefile *ltt_tracefile_open(char *pathname);

int ltt_tracefile_close(ltt_tracefile *t, int close_facilities); 

int ltt_tracefile_facility_add(ltt_tracefile *t, ltt_facility *f, int base_id);


/* A tracefile may be queried for its architecture type (e.g., "i386", 
   "powerpc", "powerpcle", "s390", "s390x"), its architecture variant 
   (e.g., "att" versus "sun" for m68k), its operating system (e.g., "linux",
   "bsd"), its generic architecture, and the machine identity (e.g., system
   host name). All character strings belong to the associated tracefile 
   and are freed when it is closed. */


uint32_t ltt_tracefile_arch_type(ltt_tracefile *t); 

uint32_t ltt_tracefile_arch_variant(ltt_tracefile *t);

ltt_arch_size ltt_tracefile_arch_size(ltt_tracefile *t);

ltt_arch_endian ltt_tracefile_arch_endian(ltt_tracefile *t); 

uint32_t ltt_tracefile_system_type(ltt_tracefile *t);

char *ltt_tracefile_system_name(ltt_tracefile *t);


/* SMP multi-processors have 2 or more CPUs */

unsigned ltt_tracefile_cpu_number(ltt_tracefile *t);


/* Does the tracefile contain events only for a single CPU? */
 
int ltt_tracefile_cpu_single(ltt_tracefile *t); 


/* It this is the case, which CPU? */

unsigned ltt_tracefile_cpu_id(ltt_tracefile *t);


/* Start and end time of the trace and its duration */

ltt_time ltt_tracefile_time_start(ltt_tracefile *t);

ltt_time ltt_tracefile_time_end(ltt_tracefile *t);

ltt_time ltt_tracefile_duration(ltt_tracefile *t);


/* Functions to discover the facilities added to the tracefile */

unsigned ltt_tracefile_facility_number(ltt_tracefile *t);

ltt_facility *ltt_tracefile_facility_get(ltt_tracefile *t, unsigned i);

ltt_facility *ltt_tracefile_facility_get_by_name(ltt_tracefile *t, char *name);


/* Functions to discover all the event types in the facilities added to the
   tracefile. The event type integer id, unique for the trace, is used. */

unsigned ltt_tracefile_eventtype_number(ltt_tracefile *t);

ltt_eventtype *ltt_tracefile_eventtype_get(ltt_tracefile *t, unsigned i);


/* Given an event type, find its unique id within the tracefile */

unsigned ltt_tracefile_eventtype_id(ltt_tracefile *t, ltt_eventtype *et);


/* Get the root field associated with an event type for the tracefile */

ltt_field *ltt_tracefile_eventtype_root_field(ltt_tracefile *t, unsigned id);


/* Seek to the first event of the trace with time larger or equal to time */

int ltt_tracefile_seek_time(ltt_tracefile *t, ltt_time time);


/* Read the next event */

ltt_event *ltt_tracefile_read(ltt_tracefile *t);

#endif // TRACEFILE_H
