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
#include <ltt/event.h>

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

/* Byte ordering */
#define LTT_GET_BO(t) ((t)->reverse_bo)

#define LTT_HAS_FLOAT(t) ((t)->float_word_order!=0)
#define LTT_GET_FLOAT_BO(t) \
  (((t)->float_word_order==__BYTE_ORDER)?0:1)

#define SEQUENCE_AVG_ELEMENTS 1000
                               
typedef guint8 uint8_t;
typedef guint16 uint16_t;
typedef guint32 uint32_t;
typedef guint64 uint64_t;

struct ltt_event_header_hb {
  uint32_t      timestamp;
  uint16_t      event_id;
  uint16_t      event_size;
} LTT_PACKED_STRUCT;

struct ltt_event_header_nohb {
  uint64_t      timestamp;
  uint16_t      event_id;
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
  uint8_t         alignment;    /* Architecture alignment */
} LTT_PACKED_STRUCT;

struct ltt_trace_header_2_0 {
  uint32_t        magic_number;
  uint32_t        arch_type;
  uint32_t        arch_variant;
  uint32_t        float_word_order;
  uint8_t         arch_size;
  uint8_t         major_version;
  uint8_t         minor_version;
  uint8_t         flight_recorder;
  uint8_t         alignment;    /* Architecture alignment */
  uint8_t         tscbits;
  uint8_t         eventbits;
  uint8_t         unused1;
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


enum field_status { FIELD_UNKNOWN, FIELD_VARIABLE, FIELD_FIXED };

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

struct LttTracefile {
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
  size_t    alignment;               //alignment of events in the tracefile.
                                     // 0 or the architecture size in bytes.

  size_t    buffer_header_size;
  uint8_t   tscbits;
  uint8_t   eventbits;
  uint64_t  tsc_mask;
  uint64_t  tsc_mask_next_bit;       //next MSB after the mask

  /* Current event */
  LttEvent event;                    //Event currently accessible in the trace

  /* Current block */
  LttBuffer buffer;                  //current buffer
  guint32 buf_size;                  /* The size of blocks */
};

/* The characteristics of the system on which the trace was obtained
   is described in a LttSystemDescription structure. */

struct LttSystemDescription {
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

/* Calculate the offset needed to align the type.
 * If alignment is 0, alignment is disactivated.
 * else, the function returns the offset needed to
 * align align_drift on the alignment value (should be
 * the size of the architecture). */
static inline unsigned int ltt_align(size_t align_drift,
          size_t size_of_type,
          size_t alignment)
{
  size_t align_offset = min(alignment, size_of_type);
  
  if(!alignment)
    return 0;
  
  g_assert(size_of_type != 0);
  return ((align_offset - align_drift) & (align_offset-1));
}


#endif /* LTT_PRIVATE_H */
