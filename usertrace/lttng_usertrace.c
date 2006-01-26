
/* LTTng user-space tracing code
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

#include <asm/atomic.h>
#include "lttng_usertrace.h"

#define MAX_TRACES 16

struct ltt_buf {
	void *start;
	atomic_t	offset;
	atomic_t	reserve_count;
	atomic_t	commit_count;

	atomic_t	events_lost;
};

struct lttng_trace_info {
	int	active:1;
	int filter;
	struct {
		struct ltt_buf facilities;
		struct ltt_buf cpu;
	} channel;
};


/* TLS for the trace info
 * http://www.dis.com/gnu/gcc/C--98-Thread-Local-Edits.html
 *
 * Add after paragraph 4
 *
 *     The storage for an object of thread storage duration shall be statically
 *     initialized before the first statement of the thread startup function. An
 *     object of thread storage duration shall not require dynamic
 *     initialization.
 * GCC extention permits init of a range.
 */

static __thread struct lttng_trace_info lttng_trace_info[MAX_TRACES] =
{	[ 0 ... MAX_TRACES-1 ].active = 0,
	[ 0 ... MAX_TRACES-1 ].filter = 0,
	[ 0 ... MAX_TRACES-1 ].channel = 
		{ NULL,
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0)
		}
};


static void lttng_get_new_info(void)
{
	unsigned long cpu_addr, fac_addr;
	unsigned int i, first_empty;
	int active, filter;
	int ret;

	/* Get all the new traces */
	while(1) {
		cpu_addr = fac_addr = 0;
		active = filter = 0;
		ret = ltt_update(&cpu_addr, &fac_addr, &active, &filter);
		if(ret) {
			printf("LTTng : error in ltt_update\n");
			exit(1);
		}
		
		if(!cpu_addr || !fac_addr) break;
		
		first_empty = MAX_TRACES;
		/* Try to find the trace */
		for(i=0;i<MAX_TRACES;i++) {
			if(i<first_empty && !lttng_trace_info[i].channel.cpu.start)
				first_empty = i;
			if(cpu_addr == 
					(unsigned long)lttng_trace_info[i].channel.cpu.start &&
				 fac_addr == 
				 	(unsigned long)lttng_trace_info[i].channel.facilities.start) {

				lttng_trace_info[i].filter = filter;
				lttng_trace_info[i].active = active;
			}

		}
		if(i == MAX_TRACES) {
			/* Not found. Must take an empty slot */
			if(first_empty == MAX_TRACES) {
				printf(
				"LTTng WARNING : too many traces requested for pid %d by the kernel.\n"
				"                Compilation defined maximum is %u\n",
					getpid(), MAX_TRACES);

			} else {
				lttng_trace_info[first_empty].channel.cpu.start = (void*)cpu_addr;
				lttng_trace_info[first_empty].channel.facilities.start =
																													(void*)fac_addr;
				lttng_trace_info[first_empty].filter = filter;
				lttng_trace_info[first_empty].active = active;
			}
		}
	}
}


/* signal handler */
void __lttng_sig_trace_handler(int signo)
{
	int ret;
	sigset_t set, oldset;
	
  printf("LTTng signal handler : thread id : %lu, pid %lu\n", pthread_self(), getpid());
#if 0
	/* Disable signals */
	ret = sigfillset(&set);
	if(ret) {
		printf("Error in sigfillset\n");
		exit(1);
	}
	
	ret = sigprocmask(SIG_BLOCK, &set, &oldset);
	if(ret) {
		printf("Error in sigprocmask\n");
		exit(1);
	}
#endif //0
#if 0
	/* Enable signals */
	ret = sigprocmask(SIG_SETMASK, &oldset, NULL);
	if(ret) {
		printf("Error in sigprocmask\n");
		exit(1);
	}
#endif //0
}


static void thread_init(void)
{
	lttng_get_new_info();
	int err;

	/* Make some ltt_switch syscalls */
	err = ltt_switch((unsigned long)NULL);
	if(err) {
		printf("Error in ltt_switch system call\n");
		exit(1);
	}
}

void __attribute__((constructor)) __lttng_user_init(void)
{
	static struct sigaction act;
	int err;

  printf("LTTng user init\n");

	/* Activate the signal */
	act.sa_handler = __lttng_sig_trace_handler;
	err = sigemptyset(&(act.sa_mask));
	if(err) perror("Error with sigemptyset");
	err = sigaddset(&(act.sa_mask), SIGRTMIN+3);
	if(err) perror("Error with sigaddset");
	err = sigaction(SIGRTMIN+3, &act, NULL);
	if(err) perror("Error with sigaction");

	thread_init();
}


void lttng_thread_init(void)
{
	thread_init();
}





