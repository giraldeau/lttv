/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LTT_H
#define LTT_H

#include <glib.h>
#include <ltt/time.h>
#include <ltt/compiler.h>

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
   
#define PREALLOC_EVENTS	28

typedef struct LttTrace LttTrace;

typedef struct LttTracefile LttTracefile;

typedef struct LttSystemDescription LttSystemDescription;

typedef struct LttEvent LttEvent;

/* Checksums are used to differentiate facilities which have the same name
   but differ. */

//typedef guint32 LttChecksum;


/* Events are usually stored with the easily obtained CPU clock cycle count,
   ltt_cycle_count. This can be converted to the real time value, LttTime,
   using linear interpolation between regularly sampled values (e.g. a few 
   times per second) of the real time clock with their corresponding 
   cycle count values. */


typedef struct _TimeInterval{
  LttTime start_time;
  LttTime end_time;  
} TimeInterval;


typedef guint64 LttCycleCount;

/* Event positions are used to seek within a tracefile based on
   the block number and event position within the block. */

typedef struct LttEventPosition LttEventPosition;


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

typedef enum _LttTypeEnum 
{ LTT_INT_FIXED,
  LTT_UINT_FIXED,
  LTT_POINTER,
  LTT_CHAR,
  LTT_UCHAR,
  LTT_SHORT,
  LTT_USHORT,
  LTT_INT,
  LTT_UINT,
  LTT_LONG,
  LTT_ULONG,
  LTT_SIZE_T,
  LTT_SSIZE_T,
  LTT_OFF_T,
  LTT_FLOAT,
  LTT_STRING,
  LTT_ENUM,
  LTT_ARRAY,
  LTT_SEQUENCE,
  LTT_STRUCT,
  LTT_UNION,
  LTT_NONE
} LttTypeEnum;
 

/* Architecture types */
#define LTT_ARCH_TYPE_I386          1
#define LTT_ARCH_TYPE_PPC           2
#define LTT_ARCH_TYPE_SH            3
#define LTT_ARCH_TYPE_S390          4
#define LTT_ARCH_TYPE_MIPS          5
#define LTT_ARCH_TYPE_ARM           6
#define LTT_ARCH_TYPE_PPC64         7
#define LTT_ARCH_TYPE_X86_64        8
#define LTT_ARCH_TYPE_C2            9
#define LTT_ARCH_TYPE_POWERPC       10
#define LTT_ARCH_TYPE_X86           11

/* Standard definitions for variants */
#define LTT_ARCH_VARIANT_NONE       0  /* Main architecture implementation */

#endif // LTT_H
