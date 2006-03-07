
/* LTTng user-space "fast" tracing code
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */


#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syscall.h>
#include <features.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>

#include "ltt-usertrace-fast.h"

/* TLS for the trace buffer
 * http://www.dis.com/gnu/gcc/C--98-Thread-Local-Edits.html
 *
 * Add after paragraph 4
 *
 *     The storage for an object of thread storage duration shall be statically
 *     initialized before the first statement of the thread startup function. An
 *     object of thread storage duration shall not require dynamic
 *     initialization.
 */

__thread struct lttng_trace_info lttng_trace_info =
{
	.init = 0,
	.filter = 0,
	.nesting = ATOMIC_INIT(0),
	.channel.facilities = 
		{	ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0)
		},
	.channel.cpu = 
		{ ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0)
		},
};


static void ltt_cleanup_thread(void *arg)
{
	/* Flush the data in the lttng_trace_info */

}


void ltt_thread_init(void)
{
	_pthread_cleanup_push(&lttng_trace_info.cleanup,
			ltt_cleanup_thread, NULL);
}


void __attribute__((constructor)) __ltt_usertrace_fast_init(void)
{
	int err;

  printf("LTTng usertrace-fast init\n");

	ltt_thread_init();

}

