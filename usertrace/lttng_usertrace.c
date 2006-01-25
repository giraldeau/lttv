
/* LTTng user-space tracing code
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */


#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <syscall.h>

#include "lttng_usertrace.h"

/* signal handler */
void __lttng_sig_trace_handler(int signo)
{
  printf("LTTng Sig handler : pid : %lu\n", getpid());
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

	/* Make the first ltt_update system call */
	err = ltt_update(1, NULL, NULL);
	if(err) {
		printf("Error in ltt_update system call\n");
		exit(1);
	}
}
