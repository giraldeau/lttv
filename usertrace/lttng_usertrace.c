
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

#include "lttng_usertrace.h"

#define MAX_TRACES 16


/*
 * Notes :
 *
 * ltt_update :
 *
 * it should send the information on the traces that has their status MODIFIED.
 * It's a necessary assumption to have a correct lttng_free_trace_info, which
 * would not be reentrant otherwise.
 */


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
	[ 0 ... MAX_TRACES-1 ].destroy = 0,
	[ 0 ... MAX_TRACES-1 ].nesting = ATOMIC_INIT(0),
	[ 0 ... MAX_TRACES-1 ].channel = 
		{ NULL,
			0,
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0),
			ATOMIC_INIT(0)
		}
};


/* Must be called we sure nobody else is using the info.
 * It implies that the trace should have been previously stopped
 * and that every writer has finished.
 *
 * Writers should always check if the trace must be destroyed when they
 * finish writing and the nesting level is 0.
 */
void lttng_free_trace_info(struct lttng_trace_info *info)
{
	int ret;
	
	if(info->active) {
		printf(
		"LTTng ERROR : lttng_free_trace_info should be called on inactive trace\n");
		exit(1);
	}
	if(!info->destroy) {
		printf(
		"LTTng ERROR : lttng_free_trace_info should be called on destroyed trace\n");
		exit(1);
	}
	if(atomic_read(&info->nesting) > 0) {
		printf(
		"LTTng ERROR : lttng_free_trace_info should not be nested on tracing\n");
		exit(1);
	}
	
	/* Remove the maps */
	ret = munmap(info->channel.cpu.start, info->channel.cpu.length);
	if(ret) {
		perror("LTTNG : error in munmap");
	}
	ret = munmap(info->channel.facilities.start, info->channel.facilities.length);
	if(ret) {
		perror("LTTNG : error in munmap");
	}
	
	/* Zero the structure */
	memset(info, 0, sizeof(struct lttng_trace_info));
}


static struct lttng_trace_info* find_info(unsigned long cpu_addr,
		unsigned long fac_addr, unsigned int *first_empty)
{
	struct lttng_trace_info *found = NULL;
	unsigned int i;

	*first_empty = MAX_TRACES;

	/* Try to find the trace */
	for(i=0;i<MAX_TRACES;i++) {
		if(i<*first_empty && !lttng_trace_info[i].channel.cpu.start)
			*first_empty = i;
		if(cpu_addr == 
				(unsigned long)lttng_trace_info[i].channel.cpu.start &&
			 fac_addr == 
				(unsigned long)lttng_trace_info[i].channel.facilities.start) {
			/* Found */
			found = &lttng_trace_info[i];
			break;
		}
	}
	return found;
}


static void lttng_get_new_info(void)
{
	unsigned long cpu_addr, fac_addr;
	unsigned int i, first_empty;
	int active, filter, destroy;
	int ret;
	struct lttng_trace_info *info;
	sigset_t set, oldset;

	/* Disable signals */
	ret = sigfillset(&set);
	if(ret) {
		printf("Error in sigfillset\n");
		exit(1);
	}
	
	ret = pthread_sigmask(SIG_BLOCK, &set, &oldset);
	if(ret) {
		printf("Error in sigprocmask\n");
		exit(1);
	}

	/* Get all the new traces */
	while(1) {
		cpu_addr = fac_addr = 0;
		active = filter = destroy = 0;
		ret = ltt_update(&cpu_addr, &fac_addr, &active, &filter, &destroy);
		if(ret) {
			printf("LTTng : error in ltt_update\n");
			exit(1);
		}
		
		if(!cpu_addr || !fac_addr) break;
		
		info = find_info(cpu_addr, fac_addr, &first_empty);
		if(info) {
			info->filter = filter;
			info->active = active;
			info->destroy = destroy;
			if(destroy && !atomic_read(&info->nesting))
				lttng_free_trace_info(info);
		} else {
			/* Not found. Must take an empty slot */
			if(first_empty == MAX_TRACES) {
				printf(
				"LTTng WARNING : too many traces requested for pid %d by the kernel.\n"
				"                Compilation defined maximum is %u\n",
					getpid(), MAX_TRACES);

			} else {
				info = &lttng_trace_info[first_empty];
				info->channel.cpu.start = (void*)cpu_addr;
				info->channel.cpu.length = PAGE_SIZE;
				info->channel.facilities.start = (void*)fac_addr;
				info->channel.facilities.length = PAGE_SIZE;
				info->filter = filter;
				info->active = active;
				info->destroy = destroy;
				if(destroy && !atomic_read(&info->nesting))
					lttng_free_trace_info(info);
			}
		}
	}

	/* Enable signals */
	ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
	if(ret) {
		printf("Error in sigprocmask\n");
		exit(1);
	}
}


/* signal handler */
void __lttng_sig_trace_handler(int signo)
{
  printf("LTTng signal handler : thread id : %lu, pid %lu\n", pthread_self(), getpid());
	lttng_get_new_info();
}


static void thread_init(void)
{
	int err;
	lttng_get_new_info();

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


