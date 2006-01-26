
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
	struct lttng_trace_info *next;
	int	active:1;
	struct {
		struct ltt_buf facilities;
		struct ltt_buf cpu;
	} channel;
};


/* TLS for the trace info */
static __thread struct lttng_trace_info *test;
static __thread struct lttng_trace_info lttng_trace_info[MAX_TRACES];


/* signal handler */
void __lttng_sig_trace_handler(int signo)
{
	int ret;
	sigset_t set, oldset;
	
  printf("LTTng Sig handler : pid : %lu\n", getpid());

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
	
	/* Get all the new traces */
#if 0
	do {
		/* FIXME : allocate the trace structure somewhere : thread private */
		ret = ltt_update(addr, &active, &filter);
		
		if(ret) {
			printf("Error in ltt_update system call\n");
			exit(1);
		}
	} while(addr);
		
#endif //0
	/* Enable signals */
	ret = sigprocmask(SIG_SETMASK, &oldset, NULL);
	if(ret) {
		printf("Error in sigprocmask\n");
		exit(1);
	}
	
}


void __lttng_init_trace_info(void)
{
	memset(&lttng_trace_info, 0, MAX_TRACES*sizeof(struct lttng_trace_info));
}

void __attribute__((constructor)) __lttng_user_init(void)
{
	static struct sigaction act;
	int err;

  printf("LTTng user init\n");

	/* Init trace info */
	__lttng_init_trace_info();
	
	/* Activate the signal */
	act.sa_handler = __lttng_sig_trace_handler;
	err = sigemptyset(&(act.sa_mask));
	if(err) perror("Error with sigemptyset");
	err = sigaddset(&(act.sa_mask), SIGRTMIN+3);
	if(err) perror("Error with sigaddset");
	err = sigaction(SIGRTMIN+3, &act, NULL);
	if(err) perror("Error with sigaction");

	/* TEST */
	err = ltt_switch((unsigned long)NULL);
	if(err) {
		printf("Error in ltt_switch system call\n");
		exit(1);
	}

	/* Make the first ltt_update system call */
	err = ltt_update(1, NULL, NULL);
	if(err) {
		printf("Error in ltt_update system call\n");
		exit(1);
	}

	/* Make some ltt_switch syscalls */
	err = ltt_switch((unsigned long)NULL);
	if(err) {
		printf("Error in ltt_switch system call\n");
		exit(1);
	}



}
