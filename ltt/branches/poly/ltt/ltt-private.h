/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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

#ifndef LTT_PRIVATE_H
#define LTT_PRIVATE_H

#include <glib.h>
#include <sys/types.h>
#include <ltt/ltt.h>


#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


#define LTT_MAGIC_NUMBER 0x00D6B7ED
#define LTT_REV_MAGIC_NUMBER 0xEDB7D600

#define NSEC_PER_USEC 1000

#define LTT_PACKED_STRUCT __attribute__ ((packed))

/* Hardcoded facilities */
#define LTT_FACILITY_CORE 0

/* Hardcoded core events */
enum ltt_core_events {
    LTT_EVENT_FACILITY_LOAD,
    LTT_EVENT_FACILITY_UNLOAD,
    LTT_EVENT_STATE_DUMP_FACILITY_LOAD,
    LTT_EVENT_HEARTBEAT
};


#if 0
/* enumeration definition */

typedef enum _BuildinEvent{
  TRACE_FACILITY_LOAD = 0,
  TRACE_BLOCK_START   = 17,
  TRACE_BLOCK_END     = 18,
  TRACE_TIME_HEARTBEAT= 19
} BuildinEvent;


/* structure definition */

typedef struct _FacilityLoad{
  gchar * name;
  LttChecksum checksum;
  guint32     base_code;
} LTT_PACKED_STRUCT FacilityLoad;

typedef struct _BlockStart {
  LttTime       time;       //Time stamp of this block
  LttCycleCount cycle_count; //cycle count of the event
  guint32       block_id;    //block id 
} LTT_PACKED_STRUCT BlockStart;

typedef struct _BlockEnd {
  LttTime       time;       //Time stamp of this block
  LttCycleCount cycle_count; //cycle count of the event
  guint32       block_id;    //block id 
} LTT_PACKED_STRUCT BlockEnd;
#endif //0


typedef guint8 uint8_t;
typedef guint16 uint16_t;
typedef guint32 uint32_t;
typedef guint64 uint64_t;

/* Hardcoded facility load event : this plus an preceding "name" string */
struct LttFacilityLoad {
  guint32 checksum;
  guint32 id;
  guint32 long_size;
  guint32 pointer_size;
  guint32 size_t_size;
  guint32 alignment;
} LTT_PACKED_STRUCT;

struct LttFacilityUnload {
  guint32 id;
} LTT_PACKED_STRUCT;

struct LttStateDumpFacilityLoad {
  guint32 checksum;
  guint32 id;
  guint32 long_size;
  guint32 pointer_size;
  guint32 size_t_size;
  guint32 alignment;
} LTT_PACKED_STRUCT;

typedef struct _TimeHeartbeat {
  LttTime       time;       //Time stamp of this block
  uint64_t cycle_count; //cycle count of the event
} LTT_PACKED_STRUCT TimeHeartbeat;

struct ltt_event_header_hb {
  uint32_t      timestamp;
  unsigned char  facility_id;
  unsigned char event_id;
  uint16_t      event_size;
} LTT_PACKED_STRUCT;

struct ltt_event_header_nohb {
  uint64_t      timestamp;
  unsigned char  facility_id;
  unsigned char event_id;
  uint16_t      event_size;
} LTT_PACKED_STRUCT;

struct ltt_trace_header {
  uint32_t        magic_number;
  uint32_t        arch_type;
  uint32_t        arch_variant;
  uint8_t         arch_size;
  //uint32_t        system_type;
  uint8_t          major_version;
  uint8_t          minor_version;
  uint8_t          flight_recorder;
  uint8_t          has_heartbeat;
  uint8_t          has_alignment;  /* Event header alignment */
	uint8_t					 has_tsc;
} LTT_PACKED_STRUCT;


struct ltt_block_start_header {
  struct { 
    struct timeval          timestamp;
    uint64_t                cycle_count;
  } begin;
  struct {
    struct timeval          timestamp;
    uint64_t                cycle_count;
  } end;
  uint32_t                lost_size;  /* Size unused at the end of the buffer */
  uint32_t                buf_size;   /* The size of this sub-buffer */
  struct ltt_trace_header trace;
} LTT_PACKED_STRUCT;


struct _LttType{
  GQuark type_name;                //type name if it is a named type
  GQuark element_name;             //elements name of the struct
  gchar * fmt;
  unsigned int size;
  LttTypeEnum type_class;          //which type
  GQuark * enum_strings;            //for enum labels
  struct _LttType ** element_type; //for array, sequence and struct
  unsigned element_number;         //the number of elements 
                                   //for enum, array, sequence and structure
};

struct _LttEventType{
  GQuark name;
  gchar * description;
  guint index;              //id of the event type within the facility
  LttFacility * facility; //the facility that contains the event type
  LttField * root_field;  //root field
};

/* Structure LttEvent and LttEventPosition must begin with the _exact_ same
 * fields in the exact same order. LttEventPosition is a parent of LttEvent. */
struct _LttEvent{
  
  /* Begin of LttEventPosition fields */
  LttTracefile  *tracefile;
  unsigned int  block;
  unsigned int  offset;

  /* Timekeeping */
  uint64_t                tsc;       /* Current timestamp counter */
  
  /* End of LttEventPosition fields */

	union {											/* choice by trace has_tsc */
	  guint32  timestamp;				/* truncated timestamp */
  	LttTime  delta;
	} time;

  unsigned char facility_id;	/* facility ID are never reused. */
  unsigned char event_id;

  LttTime event_time;

  void * data;               //event data
  guint  data_size;

  int      count;                    //the number of overflow of cycle count
  gint64 overflow_nsec;              //precalculated nsec for overflows
};

struct _LttEventPosition{
  LttTracefile  *tracefile;
  unsigned int  block;
  unsigned int  offset;
  
  /* Timekeeping */
  uint64_t                tsc;       /* Current timestamp counter */
};


enum field_status { FIELD_UNKNOWN, FIELD_VARIABLE, FIELD_FIXED };

struct _LttField{
  //guint field_pos;           //field position within its parent
  LttType * field_type;      //field type, if it is root field
                             //then it must be struct type

  off_t offset_root;         //offset from the root, -1:uninitialized 
  enum field_status fixed_root;          //offset fixed according to the root
                             //-1:uninitialized, 0:unfixed, 1:fixed
  off_t offset_parent;       //offset from the parent,-1:uninitialized
  enum field_status fixed_parent;        //offset fixed according to its parent
                             //-1:uninitialized, 0:unfixed, 1:fixed
  //  void * base_address;       //base address of the field  ????
  
  guint field_size;     //      //>0: size of the field, 
                          //   //0 : uncertain
                           //  //-1: uninitialize
  enum field_status fixed_size;

  /* for sequence */
  gint sequ_number_size;      //the size of unsigned used to save the
                             //number of elements in the sequence

  gint element_size;          //the element size of the sequence
  //int field_fixed;           //0: field has string or sequence
                             //1: field has no string or sequenc
                             //-1: uninitialize

  struct _LttField * parent;
  struct _LttField ** child; //for array, sequence, struct and union: 
                             //list of fields, it may have only one
                             //field if the element is not a struct or
                             //union
  unsigned current_element;  //which element is currently processed
                             // Used for sequences and arrays.
};


struct _LttFacility{
  LttTrace  *trace;
  //gchar * name;               //facility name 
  GQuark name;
  guint32 checksum;      //checksum of the facility 
  guint32  id;          //id of the facility
 
  guint32 pointer_size;
  guint32 size_t_size;
  guint32 alignment;


  //LttEventType ** events;    //array of event types 
  //unsigned int event_number;          //number of events in the facility 
  //LttType ** named_types;
  //unsigned int named_types_number;

  GArray *events;
  GData *events_by_name;
 // GArray *named_types;
  //GData *named_types_by_name;
  GData *named_types;
  
  unsigned char exists; /* 0 does not exist, 1 exists */
};

typedef struct _LttBuffer {
  void * head;
  unsigned int index;

  struct {
    LttTime                 timestamp;
    uint64_t                cycle_count;
  } begin;
  struct {
    LttTime                 timestamp;
    uint64_t                cycle_count;
  } end;
  uint32_t                lost_size; /* Size unused at the end of the buffer */

  /* Timekeeping */
  uint64_t                tsc;       /* Current timestamp counter */
  double                  nsecs_per_cycle;
} LttBuffer;

struct _LttTracefile{
  gboolean cpu_online;               //is the cpu online ?
  GQuark name;                       //tracefile name
  guint cpu_num;                     //cpu number of the tracefile
  LttTrace * trace;                  //trace containing the tracefile
  int fd;                            //file descriptor 
  off_t file_size;                   //file size
  unsigned block_size;               //block_size
  unsigned int num_blocks;           //number of blocks in the file
  gboolean  reverse_bo;              //must we reverse byte order ?

	/* Current event */
  LttEvent event;                    //Event currently accessible in the trace

	/* Current block */
  LttBuffer buffer;                  //current buffer
  guint32 buf_size;                  /* The size of blocks */

	/* Time flow */
  //unsigned int      count;           //the number of overflow of cycle count
  //double nsec_per_cycle;             //Nsec per cycle
  //TimeHeartbeat * last_heartbeat;    //last heartbeat

  //LttCycleCount cycles_per_nsec_reciprocal; // Optimisation for speed
  //void * last_event_pos;

  //LttTime prev_block_end_time;       //the end time of previous block
  //LttTime prev_event_time;           //the time of the previous event
  //LttCycleCount pre_cycle_count;     //previous cycle count of the event
};

struct _LttTrace{
  GQuark pathname;                          //the pathname of the trace
  //LttSystemDescription * system_description;//system description 

  GArray *facilities_by_num;            /* fac_id as index in array */
  GData *facilities_by_name;            /* fac name (GQuark) as index */
                                        /* Points to array of fac_id of all the
                                         * facilities that has this name. */

  guint32   arch_type;
  guint32   arch_variant;
  guint8    arch_size;
  guint8    ltt_major_version;
  guint8    ltt_minor_version;
  guint8    flight_recorder;
  guint8    has_heartbeat;
  guint8    has_alignment;
	guint8		has_tsc;

  GData     *tracefiles;                    //tracefiles groups
};

/* The characteristics of the system on which the trace was obtained
   is described in a LttSystemDescription structure. */

struct _LttSystemDescription {
  gchar *description;
  gchar *node_name;
  gchar *domain_name;
  unsigned nb_cpu;
  LttArchSize size;
  LttArchEndian endian;
  gchar *kernel_name;
  gchar *kernel_release;
  gchar *kernel_version;
  gchar *machine;
  gchar *processor;
  gchar *hardware_platform;
  gchar *operating_system;
  LttTime trace_start;
  LttTime trace_end;
};

/*****************************************************************************
 macro for size of some data types
 *****************************************************************************/
// alignment -> dynamic!

//#define TIMESTAMP_SIZE    sizeof(guint32)
//#define EVENT_ID_SIZE     sizeof(guint16)
//#define EVENT_HEADER_SIZE (TIMESTAMP_SIZE + EVENT_ID_SIZE)

#define LTT_GET_BO(t) ((t)->reverse_bo)


#endif /* LTT_PRIVATE_H */
