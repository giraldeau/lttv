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

/* Byte ordering */
#define LTT_GET_BO(t) ((t)->reverse_bo)

#define LTT_HAS_FLOAT(t) ((t)->float_word_order ! =0)
#define LTT_GET_FLOAT_BO(t) \
  (((t)->float_word_order == __BYTE_ORDER) ? 0 : 1)

#define SEQUENCE_AVG_ELEMENTS 1000
 
/*
 * offsetof taken from Linux kernel.
 */
#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

typedef guint8 uint8_t;
typedef guint16 uint16_t;
typedef guint32 uint32_t;
typedef guint64 uint64_t;

/* Subbuffer header */
struct ltt_subbuffer_header_2_3 {
	uint64_t cycle_count_begin;	/* Cycle count at subbuffer start */
	uint64_t cycle_count_end;	/* Cycle count at subbuffer end */
	uint32_t magic_number;		/*
					 * Trace magic number.
					 * contains endianness information.
					 */
	uint8_t major_version;
	uint8_t minor_version;
	uint8_t arch_size;		/* Architecture pointer size */
	uint8_t alignment;		/* LTT data alignment */
	uint64_t start_time_sec;	/* NTP-corrected start time */
	uint64_t start_time_usec;
	uint64_t start_freq;		/*
					 * Frequency at trace start,
					 * used all along the trace.
					 */
	uint32_t freq_scale;		/* Frequency scaling (divide freq) */
	uint32_t lost_size;		/* Size unused at end of subbuffer */
	uint32_t buf_size;		/* Size of this subbuffer */
	uint32_t events_lost;		/*
					 * Events lost in this subbuffer since
					 * the beginning of the trace.
					 * (may overflow)
					 */
	uint32_t subbuf_corrupt;	/*
					 * Corrupted (lost) subbuffers since
					 * the begginig of the trace.
					 * (may overflow)
					 */
	char header_end[0];		/* End of header */
};

typedef struct ltt_subbuffer_header_2_3 ltt_subbuffer_header_t;

/*
 * Return header size without padding after the structure. Don't use packed
 * structure because gcc generates inefficient code on some architectures
 * (powerpc, mips..)
 */
static inline size_t ltt_subbuffer_header_size(void)
{
	return offsetof(ltt_subbuffer_header_t, header_end);
}

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
  struct marker_data *mdata;         // marker id/name/fields mapping
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
  uint64_t  tsc_mask_next_bit;       //next MSB after the mask<
  uint32_t  events_lost;
  uint32_t  subbuf_corrupt;

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
