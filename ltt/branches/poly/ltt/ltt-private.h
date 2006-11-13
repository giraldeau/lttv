/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
 *               2006 Mathieu Desnoyers
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

#ifndef LTT_PRIVATE_H
#define LTT_PRIVATE_H

#include <glib.h>
#include <sys/types.h>
#include <ltt/ltt.h>
#include <endian.h>


#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif



#define LTT_MAGIC_NUMBER 0x00D6B7ED
#define LTT_REV_MAGIC_NUMBER 0xEDB7D600

#define NSEC_PER_USEC 1000

#define LTT_PACKED_STRUCT __attribute__ ((packed))

/* Hardcoded facilities */
#define LTT_FACILITY_CORE 0
 
/* Byte ordering */
#define LTT_GET_BO(t) ((t)->reverse_bo)

#define LTT_HAS_FLOAT(t) ((t)->float_word_order!=0)
#define LTT_GET_FLOAT_BO(t) \
  (((t)->float_word_order==__BYTE_ORDER)?0:1)

#define SEQUENCE_AVG_ELEMENTS 1000
                               
/* Hardcoded core events */
enum ltt_core_events {
    LTT_EVENT_FACILITY_LOAD,
    LTT_EVENT_FACILITY_UNLOAD,
    LTT_EVENT_HEARTBEAT,
    LTT_EVENT_STATE_DUMP_FACILITY_LOAD
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
  guint32 int_size;
  guint32 long_size;
  guint32 pointer_size;
  guint32 size_t_size;
  guint32 has_alignment;
} LTT_PACKED_STRUCT;

struct LttFacilityUnload {
  guint32 id;
} LTT_PACKED_STRUCT;

struct LttStateDumpFacilityLoad {
  guint32 checksum;
  guint32 id;
  guint32 int_size;
  guint32 long_size;
  guint32 pointer_size;
  guint32 size_t_size;
  guint32  has_alignment;
} LTT_PACKED_STRUCT;

/* Empty event */
typedef struct _TimeHeartbeat {
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


/* Block and trace headers */

struct ltt_trace_header_any {
  uint32_t        magic_number;
  uint32_t        arch_type;
  uint32_t        arch_variant;
  uint32_t        float_word_order;
  uint8_t         arch_size;
  uint8_t         major_version;
  uint8_t         minor_version;
  uint8_t         flight_recorder;
  uint8_t         has_heartbeat;
  uint8_t         has_alignment;  /* Event header alignment */
  uint32_t        freq_scale;
} LTT_PACKED_STRUCT;


/* For version 0.3 */

struct ltt_trace_header_0_3 {
  uint32_t        magic_number;
  uint32_t        arch_type;
  uint32_t        arch_variant;
  uint32_t        float_word_order;
  uint8_t         arch_size;
  uint8_t         major_version;
  uint8_t         minor_version;
  uint8_t         flight_recorder;
  uint8_t         has_heartbeat;
  uint8_t         has_alignment;  /* Event header alignment */
  uint32_t        freq_scale;
} LTT_PACKED_STRUCT;

/* For version 0.7 */

struct ltt_trace_header_0_7 {
  uint32_t        magic_number;
  uint32_t        arch_type;
  uint32_t        arch_variant;
  uint32_t        float_word_order;
  uint8_t         arch_size;
  uint8_t         major_version;
  uint8_t         minor_version;
  uint8_t         flight_recorder;
  uint8_t         has_heartbeat;
  uint8_t         has_alignment;  /* Event header alignment */
  uint32_t        freq_scale;
  uint64_t        start_freq;
  uint64_t        start_tsc;
  uint64_t        start_monotonic;
  uint64_t        start_time_sec;
  uint64_t        start_time_usec;
} LTT_PACKED_STRUCT;


struct ltt_block_start_header {
  struct { 
    uint64_t                cycle_count;
    uint64_t                freq;
  } begin;
  struct {
    uint64_t                cycle_count;
    uint64_t                freq;
  } end;
  uint32_t                lost_size;  /* Size unused at the end of the buffer */
  uint32_t                buf_size;   /* The size of this sub-buffer */
  struct ltt_trace_header_any trace[0];
} LTT_PACKED_STRUCT;


struct _LttType{
// LTTV does not care about type names. Everything is a field.
// GQuark type_name;                //type name if it is a named type
  gchar * fmt;
  guint size;
  LttTypeEnum type_class;          //which type
  GHashTable *enum_map;                 //maps enum labels to numbers.
  GArray *fields;     // Array of LttFields, for array, sequence, union, struct.
  GData *fields_by_name;
  guint network;  // Is the type in network byte order ?
};

struct _LttEventType{
  GQuark name;
  gchar * description;
  guint index;            //id of the event type within the facility
  LttFacility * facility; //the facility that contains the event type
  GArray * fields;        //event's fields (LttField)
  GData *fields_by_name;
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

  guint32  timestamp;        /* truncated timestamp */

  unsigned char facility_id;  /* facility ID are never reused. */
  unsigned char event_id;

  LttTime event_time;

  void * data;               //event data
  guint  data_size;
  guint  event_size;         //event_size field of the header : 
                             //used to verify data_size from facility.

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
  GQuark name;
  gchar *description;
  LttType field_type;      //field type

  off_t offset_root;            //offset from the root
  enum field_status fixed_root; //offset fixed according to the root

  guint field_size;       // size of the field
                          // Only if field type size is set to 0
                          // (it's variable), then the field_size should be
                          // dynamically calculated while reading the trace
                          // and put here. Otherwise, the field_size always
                          // equels the type size.
  off_t array_offset;     // offset of the beginning of the array (for array
                          // and sequences)
  GArray * dynamic_offsets; // array of offsets calculated dynamically at
                            // each event for sequences and arrays that
                            // contain variable length fields.
};

struct _LttFacility{
  LttTrace  *trace;
  GQuark name;
  guint32 checksum;      //checksum of the facility 
  guint32  id;          //id of the facility
 
  guint32 int_size;
  guint32 long_size;
  guint32 pointer_size;
  guint32 size_t_size;
  guint32 alignment;

  GArray *events;
  GData *events_by_name;
 // not necessary in LTTV GData *named_types;
  
  unsigned char exists; /* 0 does not exist, 1 exists */
};

typedef struct _LttBuffer {
  void * head;
  unsigned int index;

  struct {
    LttTime                 timestamp;
    uint64_t                cycle_count;
    uint64_t                freq; /* Frequency in khz */
  } begin;
  struct {
    LttTime                 timestamp;
    uint64_t                cycle_count;
    uint64_t                freq; /* Frequency in khz */
  } end;
  uint32_t                lost_size; /* Size unused at the end of the buffer */

  /* Timekeeping */
  uint64_t                tsc;       /* Current timestamp counter */
  uint64_t                freq; /* Frequency in khz */
  //double                  nsecs_per_cycle;  /* Precalculated from freq */
  guint32                 cyc2ns_scale;
} LttBuffer;

struct _LttTracefile{
  gboolean cpu_online;               //is the cpu online ?
  GQuark long_name;                  //tracefile complete filename
  GQuark name;                       //tracefile name
  guint cpu_num;                     //cpu number of the tracefile
  guint  tid;                         //Usertrace tid, else 0
  guint pgid;                         //Usertrace pgid, else 0
  guint64 creation;                   //Usertrace creation, else 0
  LttTrace * trace;                  //trace containing the tracefile
  int fd;                            //file descriptor 
  off_t file_size;                   //file size
  //unsigned block_size;               //block_size
  guint num_blocks;           //number of blocks in the file
  gboolean  reverse_bo;              //must we reverse byte order ?
  gboolean  float_word_order;        //what is the byte order of floats ?
  size_t    has_alignment;           //alignment of events in the tracefile.
                                     // 0 or the architecture size in bytes.

  size_t    buffer_header_size;

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
  guint     num_cpu;

  guint32   arch_type;
  guint32   arch_variant;
  guint8    arch_size;
  guint8    ltt_major_version;
  guint8    ltt_minor_version;
  guint8    flight_recorder;
  guint8    has_heartbeat;
  guint32    freq_scale;
  uint64_t  start_freq;
  uint64_t  start_tsc;
  uint64_t  start_monotonic;
  LttTime   start_time;
  LttTime   start_time_from_tsc;

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


off_t get_alignment(LttField *field);

/* Calculate the offset needed to align the type.
 * If has_alignment is 0, alignment is disactivated.
 * else, the function returns the offset needed to
 * align align_drift on the has_alignment value (should be
 * the size of the architecture). */
static inline unsigned int ltt_align(size_t align_drift,
          size_t size_of_type,
          size_t has_alignment)
{
  size_t alignment = min(has_alignment, size_of_type);
  
  if(!has_alignment) return 0;
  
  g_assert(size_of_type != 0);
  return ((alignment - align_drift) & (alignment-1));
}


#endif /* LTT_PRIVATE_H */
