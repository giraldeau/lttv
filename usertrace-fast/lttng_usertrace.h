
/* LTTng user-space tracing header
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */

#ifndef _LTTNG_USERTRACE_H
#define _LTTNG_USERTRACE_H

#include <errno.h>
#include <asm/atomic.h>

#ifndef	LTT_BUF_SIZE_CPU
#define LTT_BUF_SIZE_CPU 1048576
#endif //LTT_BUF_SIZE_CPU

#ifndef	LTT_BUF_SIZE_FACILITIES
#define LTT_BUF_SIZE_FACILITIES 4096
#endif //LTT_BUF_SIZE_FACILITIES

struct ltt_buf {
	atomic_t	offset;
	atomic_t	reserve_count;
	atomic_t	commit_count;

	atomic_t	events_lost;
};

struct lttng_trace_info {
	struct _pthread_cleanup_buffer cleanup;
	int filter;
	atomic_t nesting;
	struct {
		struct ltt_buf facilities;
		char facilities_buf[LTT_BUF_SIZE_FACILITIES] __attribute__ ((aligned (8)));
		struct ltt_buf cpu;
		char cpu_buf[LTT_BUF_SIZE_CPU] __attribute__ ((aligned (8)));
	} channel;
};

extern __thread struct lttng_trace_info lttng_trace_info;

void ltt_thread_init(void);

#endif //_LTTNG_USERTRACE_H
