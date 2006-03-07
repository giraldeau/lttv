/* LTTng user-space "fast" library
 *
 * This daemon is spawned by each traced thread (to share the mmap).
 *
 * Its job is to dump periodically this buffer to disk (when it receives a
 * SIGUSR1 from its parent).
 *
 * It uses the control information in the shared memory area (producer/consumer
 * count).
 *
 * When the parent thread dies (yes, those thing may happen) ;) , this daemon
 * will flush the last buffer and write it to disk.
 *
 * Supplement note for streaming : the daemon is responsible for flushing
 * periodically the buffer if it is streaming data.
 * 
 *
 * Notes :
 * shm memory is typically limited to 4096 units (system wide limit SHMMNI in
 * /proc/sys/kernel/shmmni). As it requires computation time upon creation, we
 * do not use it : we will use a shared mmap() instead which is passed through
 * the fork().
 * MAP_SHARED mmap segment. Updated when msync or munmap are called.
 * MAP_ANONYMOUS.
 * Memory  mapped  by  mmap()  is  preserved across fork(2), with the same
 *   attributes.
 * 
 * Eventually, there will be two mode :
 * * Slow thread spawn : a fork() is done for each new thread. If the process
 *   dies, the data is not lost.
 * * Fast thread spawn : a pthread_create() is done by the application for each
 *   new thread.
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
#include <sys/mman.h>
#include <signal.h>

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
#if 0
__thread struct ltt_trace_info ltt_trace_info =
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
#endif //0

__thread struct ltt_trace_info *thread_trace_info = NULL;


/* signal handling */

static void handler(int signo)
{
	printf("Signal %d received\n", signo);
}



void ltt_usertrace_fast_buffer_switch(void)
{
	kill(thread_trace_info->daemon_id, SIGUSR1);
}

static void ltt_usertrace_fast_daemon(struct ltt_trace_info *shared_trace_info,
		sigset_t oldset)
{
	struct sigaction act;
	int ret;

	printf("ltt_usertrace_fast_daemon : init is %d, pid is %lu\n",
			shared_trace_info->init, getpid());

	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGUSR1);
	sigaction(SIGUSR1, &act, NULL);
	/* Enable signals */
	ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
	if(ret) {
		printf("Error in pthread_sigmask\n");
	}

	while(1) {
		pause();
		printf("Doing a buffer switch read. pid is : %lu\n", getpid());
	}

}

void ltt_thread_init(void)
{
	pid_t pid;
	struct ltt_trace_info *shared_trace_info;
	int ret;
	sigset_t set, oldset;

	/* parent : create the shared memory map */
	shared_trace_info = thread_trace_info = mmap(0, sizeof(*thread_trace_info),
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	memset(shared_trace_info, 0, sizeof(*thread_trace_info));
	thread_trace_info->init = 1;

	/* Disable signals */
  ret = sigfillset(&set);
  if(ret) {
    printf("Error in sigfillset\n");
  } 
	
	
  ret = pthread_sigmask(SIG_BLOCK, &set, &oldset);
  if(ret) {
    printf("Error in pthread_sigmask\n");
  }
	
	pid = fork();
	if(pid > 0) {
		/* Parent */
		thread_trace_info->daemon_id = pid;

		/* Enable signals */
		ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
		if(ret) {
			printf("Error in pthread_sigmask\n");
		}
	} else if(pid == 0) {
		/* Child */
		ltt_usertrace_fast_daemon(shared_trace_info, oldset);
		/* Should never return */
		exit(-1);
	} else if(pid < 0) {
		/* fork error */
		perror("Error in forking ltt-usertrace-fast");
	}
}

void __attribute__((constructor)) __ltt_usertrace_fast_init(void)
{
  printf("LTT usertrace-fast init\n");

	ltt_thread_init();
}

void __attribute__((destructor)) __ltt_usertrace_fast_fini(void)
{
  printf("LTT usertrace-fast fini\n");

}

