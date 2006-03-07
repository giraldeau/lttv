
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

#ifndef	LTT_BUF_SIZE_CPU
#define LTT_BUF_SIZE_CPU 1048576
#endif //LTT_BUF_SIZE_CPU

#ifndef	LTT_BUF_SIZE_FACILITIES
#define LTT_BUF_SIZE_FACILITIES 4096
#endif //LTT_BUF_SIZE_FACILITIES

#ifndef LTT_USERTRACE_ROOT
#define LTT_USERTRACE_ROOT "/tmp/ltt-usertrace"
#endif //LTT_USERTRACE_ROOT

struct ltt_buf {
	atomic_t	offset;
	atomic_t	reserve_count;
	atomic_t	commit_count;

	atomic_t	events_lost;
};

struct ltt_trace_info {
	int init;
	int filter;
#ifndef LTT_USE_THREADS
	pid_t daemon_id;
#else
	pthread_t daemon_id;
#endif //LTT_USE_THREADS
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
