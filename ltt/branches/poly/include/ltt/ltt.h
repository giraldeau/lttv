
#include <ltt/ltt-private.h>

/* A trace is associated with a tracing session run on a single, possibly
   multi-cpu, system. It is defined as a pathname to a directory containing
   all the relevant trace files. All the tracefiles for a trace were 
   generated in a single system for the same time period by the same 
   trace daemon. They simply contain different events. Typically one file
   contains the important events (process creations and registering tracing
   facilities) for all CPUs, and one file for each CPU contains all the 
   events for that CPU. All the tracefiles within the same trace directory
   then use the exact same id numbers for event types.

   A tracefile (ltt_tracefile) contains a list of events (ltt_event) sorted
   by time for each CPU; events from different CPUs may be slightly out of
   order, especially using the (possibly drifting) cycle counters as 
   time unit.

   A facility is a list of event types (ltt_eventtype), declared in a special 
   .event file. An associated checksum differentiates different facilities 
   which would have the same name but a different content (e.g., different 
   versions).

   The list of facilities (and associated checksum) used in a tracefile 
   must be known in order to properly decode the contained events. An event
   is usually stored in the trace to denote each different "facility used". 
   While many facilities may be present when the trace starts, new 
   facilities may be introduced later as kernel modules are loaded. 
   This is fine as long as the "facility used" event precedes any event 
   described in that facility.

   Event types (ltt_eventtype) refer to data types (ltt_type) describing
   their content. The data types supported are integer and unsigned integer 
   (of various length), enumerations (a special form of unsigned integer), 
   floating point (of various length), fixed size arrays, sequence 
   (variable sized arrays), structures and null terminated strings. 
   The elements of arrays and sequences, and the data members for 
   structures, may be of any nested data type (ltt_type).

   An ltt_field is a special object to denote a specific, possibly nested,
   field within an event type. Suppose an event type socket_connect is a 
   structure containing two data membes, source and destination, of type 
   socket_address. Type socket_address contains two unsigned integer 
   data members, ip and port. An ltt_field is different from a data type 
   structure member since it can denote a specific nested field, like the 
   source port, and store associated access information (byte offset within 
   the event data). The ltt_field objects are tracefile specific since the
   contained information (byte offsets) may vary with the architecture
   associated to the tracefile. */
   
typedef struct _ltt_tracefile ltt_tracefile;

typedef struct _ltt_facility ltt_facility;

typedef struct _ltt_eventtype ltt_eventtype;

typedef struct _ltt_type ltt_type;

typedef struct _ltt_field ltt_field;

typedef struct _ltt_event ltt_event;


/* Different types allowed */

typedef enum _ltt_type_enum 
{ LTT_INT, LTT_UINT, LTT_FLOAT, LTT_STRING, LTT_ENUM, LTT_ARRAY, 
  LTT_SEQUENCE, LTT_STRUCT
} ltt_type_enum;


/* Checksums are used to differentiate facilities which have the same name
   but differ. */

typedef unsigned long ltt_checksum;


/* Events are usually stored with the easily obtained CPU clock cycle count,
   ltt_cycle_count. This can be converted to the real time value, ltt_time,
   using linear interpolation between regularly sampled values (e.g. a few 
   times per second) of the real time clock with their corresponding 
   cycle count values. */

typedef struct timespec ltt_time;

typedef uint64_t ltt_cycle_count;




