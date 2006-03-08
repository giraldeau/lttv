
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


struct ltt_buf {
	atomic_t	offset;
	atomic_t	consumed;
	atomic_t	reserve_count[LTT_N_SUBBUFS];
	atomic_t	commit_count[LTT_N_SUBBUFS];

	atomic_t	events_lost;
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

#endif //_LTT_USERTRACE_FAST_H
