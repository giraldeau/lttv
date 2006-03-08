
/* LTTng user-space "fast" tracing header
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */

#ifndef _LTT_USERTRACE_FAST_H
#define _LTT_USERTRACE_FAST_H

#include <errno.h>
#include <asm/atomic.h>
#include <pthread.h>
#include <stdint.h>
#include <syscall.h>
#include <linux/futex.h>

#ifndef futex
static inline _syscall6(long, futex, unsigned long, uaddr, int, op, int, val,
		unsigned long, timeout, unsigned long, uaddr2, int, val2)
#endif //futex


#ifndef	LTT_N_SUBBUFS
#define LTT_N_SUBBUFS 2
#endif //LTT_N_SUBBUFS

#ifndef	LTT_SUBBUF_SIZE_CPU
#define LTT_SUBBUF_SIZE_CPU 1048576
#endif //LTT_BUF_SIZE_CPU

#define LTT_BUF_SIZE_CPU (LTT_SUBBUF_SIZE_CPU * LTT_N_SUBBUFS)

#ifndef	LTT_SUBBUF_SIZE_FACILITIES
#define LTT_SUBBUF_SIZE_FACILITIES 4096
#endif //LTT_BUF_SIZE_FACILITIES

#define LTT_BUF_SIZE_FACILITIES  (LTT_SUBBUF_SIZE_FACILITIES * LTT_N_SUBBUFS)

#ifndef LTT_USERTRACE_ROOT
#define LTT_USERTRACE_ROOT "/tmp/ltt-usertrace"
#endif //LTT_USERTRACE_ROOT


/* Buffer offset macros */

#define BUFFER_OFFSET(offset, buf) (offset & (buf->alloc_size-1))
#define SUBBUF_OFFSET(offset, buf) (offset & (buf->subbuf_size-1))
#define SUBBUF_ALIGN(offset, buf) \
  (((offset) + buf->subbuf_size) & (~(buf->subbuf_size-1)))
#define SUBBUF_TRUNC(offset, buf) \
  ((offset) & (~(buf->subbuf_size-1)))
#define SUBBUF_INDEX(offset, buf) \
  (BUFFER_OFFSET(offset,buf)/buf->subbuf_size)


#define LTT_TRACER_MAGIC_NUMBER		 0x00D6B7ED
#define LTT_TRACER_VERSION_MAJOR		0
#define LTT_TRACER_VERSION_MINOR		7

#ifndef atomic_cmpxchg
#define atomic_cmpxchg(v, old, new) ((int)cmpxchg(&((v)->counter), old, new))
#endif //atomic_cmpxchg
struct ltt_trace_header {
	uint32_t				magic_number;
	uint32_t				arch_type;
	uint32_t				arch_variant;
	uint32_t				float_word_order;	 /* Only useful for user space traces */
	uint8_t 				arch_size;
	//uint32_t				system_type;
	uint8_t					major_version;
	uint8_t					minor_version;
	uint8_t					flight_recorder;
	uint8_t					has_heartbeat;
	uint8_t					has_alignment;	/* Event header alignment */
	uint32_t				freq_scale;
	uint64_t				start_freq;
	uint64_t				start_tsc;
	uint64_t				start_monotonic;
  uint64_t        start_time_sec;
  uint64_t        start_time_usec;
} __attribute((packed));


struct ltt_block_start_header {
	struct { 
		uint64_t								cycle_count;
		uint64_t								freq; /* khz */
	} begin;
	struct { 
		uint64_t								cycle_count;
		uint64_t								freq; /* khz */
	} end;
	uint32_t								lost_size;	/* Size unused at the end of the buffer */
	uint32_t								buf_size;		/* The size of this sub-buffer */
	struct ltt_trace_header	trace;
} __attribute((packed));



struct ltt_buf {
	void 			*start;
	atomic_t	offset;
	atomic_t	consumed;
	atomic_t	reserve_count[LTT_N_SUBBUFS];
	atomic_t	commit_count[LTT_N_SUBBUFS];

	atomic_t	events_lost;
	atomic_t	corrupted_subbuffers;
	atomic_t	full;	/* futex on which the writer waits : 1 : full */
	unsigned int	alloc_size;
	unsigned int	subbuf_size;
};

struct ltt_trace_info {
	int init;
	int filter;
	pid_t daemon_id;
	atomic_t nesting;
	struct {
		struct ltt_buf facilities;
		struct ltt_buf cpu;
		char facilities_buf[LTT_BUF_SIZE_FACILITIES] __attribute__ ((aligned (8)));
		char cpu_buf[LTT_BUF_SIZE_CPU] __attribute__ ((aligned (8)));
	} channel;
};



extern __thread struct ltt_trace_info *thread_trace_info;

void ltt_thread_init(void);

void ltt_usertrace_fast_buffer_switch(void);



static inline uint64_t ltt_get_timestamp()
{
	return get_cycles();
}

static inline unsigned int ltt_subbuf_header_len(struct ltt_buf *buf)
{
	return sizeof(struct ltt_block_start_header);
}



static inline void ltt_write_trace_header(struct ltt_trace_header *header)
{
	header->magic_number = LTT_TRACER_MAGIC_NUMBER;
	header->major_version = LTT_TRACER_VERSION_MAJOR;
	header->minor_version = LTT_TRACER_VERSION_MINOR;
	header->float_word_order = 0;	//FIXME
	header->arch_type = 0; //FIXME LTT_ARCH_TYPE;
	header->arch_size = sizeof(void*);
	header->arch_variant = 0; //FIXME LTT_ARCH_VARIANT;
	header->flight_recorder = 0;
	header->has_heartbeat = 0;

#ifdef CONFIG_LTT_ALIGNMENT
	header->has_alignment = sizeof(void*);
#else
	header->has_alignment = 0;
#endif
	
	//FIXME
	header->freq_scale = 0;
	header->start_freq = 0;
	header->start_tsc = 0;
	header->start_monotonic = 0;
	header->start_time_sec = 0;
	header->start_time_usec = 0;
}


static inline void ltt_buffer_begin_callback(struct ltt_buf *buf,
		      uint64_t tsc, unsigned int subbuf_idx)
{
	struct ltt_block_start_header *header = 
					(struct ltt_block_start_header*)
						(buf->start + (subbuf_idx*buf->subbuf_size));
	
	header->begin.cycle_count = tsc;
	header->begin.freq = 0; //ltt_frequency();

	header->lost_size = 0xFFFFFFFF; // for debugging...
	
	header->buf_size = buf->subbuf_size;
	
	ltt_write_trace_header(&header->trace);

}



static inline void ltt_buffer_end_callback(struct ltt_buf *buf,
		      uint64_t tsc, unsigned int offset, unsigned int subbuf_idx)
{
	struct ltt_block_start_header *header = 
						(struct ltt_block_start_header*)
								(buf->start + (subbuf_idx*buf->subbuf_size));
  /* offset is assumed to never be 0 here : never deliver a completely
   * empty subbuffer. */
  /* The lost size is between 0 and subbuf_size-1 */
	header->lost_size = SUBBUF_OFFSET((buf->subbuf_size - offset),
																		buf);
	header->end.cycle_count = tsc;
	header->end.freq = 0; //ltt_frequency();
}


static inline void ltt_deliver_callback(struct ltt_buf *buf,
    unsigned subbuf_idx,
    void *subbuf)
{
	ltt_usertrace_fast_buffer_switch();
}
#endif //_LTT_USERTRACE_FAST_H
