#ifndef LTT_H
#define LTT_H

#include <ltt/LTTTypes.h>

/* A trace is associated with a tracing session run on a single, possibly
   multi-cpu, system. It is defined as a pathname to a directory containing
   all the relevant trace files. All the tracefiles for a trace were 
   generated in a single system for the same time period by the same 
   trace daemon. They simply contain different events. Typically control
   tracefiles contain the important events (process creations and registering 
   tracing facilities) for all CPUs, and one file for each CPU contains all 
   the events for that CPU. All the tracefiles within the same trace directory
   then use the exact same id numbers for event types.

   A tracefile (LttTracefile) contains a list of events (LttEvent) sorted
   by time for each CPU; events from different CPUs may be slightly out of
   order, especially using the (possibly drifting) cycle counters as 
   time unit.

   A facility is a list of event types (LttEventType), declared in a special 
   eventdefs file. A corresponding checksum differentiates different 
   facilities which would have the same name but a different content 
   (e.g., different versions). The files are stored within the trace 
   directory and are accessed automatically upon opening a trace.
   The list of facilities (and associated checksum) used in a trace 
   must be known in order to properly decode the contained events. An event
   is stored in the "facilities" control tracefile to denote each different 
   facility used. 

   Event types (LttEventType) refer to data types (LttType) describing
   their content. The data types supported are integer and unsigned integer 
   (of various length), enumerations (a special form of unsigned integer), 
   floating point (of various length), fixed size arrays, sequence 
   (variable sized arrays), structures and null terminated strings. 
   The elements of arrays and sequences, and the data members for 
   structures, may be of any nested data type (LttType).

   An LttField is a special object to denote a specific, possibly nested,
   field within an event type. Suppose an event type socket_connect is a 
   structure containing two data members, source and destination, of type 
   socket_address. Type socket_address contains two unsigned integer 
   data members, ip and port. An LttField is different from a data type 
   structure member since it can denote a specific nested field, like the 
   source port, and store associated access information (byte offset within 
   the event data). The LttField objects are trace specific since the
   contained information (byte offsets) may vary with the architecture
   associated to the trace. */
   
typedef struct _LttTrace LttTrace;

typedef struct _LttTracefile LttTracefile;

typedef struct _LttFacility LttFacility;

typedef struct _LttEventType LttEventType;

typedef struct _LttType LttType;

typedef struct _LttField LttField;

typedef struct _LttEvent LttEvent;

typedef struct _LttSystemDescription LttSystemDescription;

/* Checksums are used to differentiate facilities which have the same name
   but differ. */

typedef unsigned long LttChecksum;


/* Events are usually stored with the easily obtained CPU clock cycle count,
   ltt_cycle_count. This can be converted to the real time value, ltt_time,
   using linear interpolation between regularly sampled values (e.g. a few 
   times per second) of the real time clock with their corresponding 
   cycle count values. */

typedef struct timespec LttTime;

typedef uint64_t LttCycleCount;


/* Differences between architectures include word sizes, endianess,
   alignment, floating point format and calling conventions. For a
   packed binary trace, endianess and size matter, assuming that the
   floating point format is standard (and is seldom used anyway). */

typedef enum _LttArchSize 
{ LTT_LP32, LTT_ILP32, LTT_LP64, LTT_ILP64, LTT_UNKNOWN 
} LttArchSize;


typedef enum _LttArchEndian
{ LTT_LITTLE_ENDIAN, LTT_BIG_ENDIAN
} LttArchEndian;



#include <ltt/ltt-private.h>

 
#endif // LTT_H
