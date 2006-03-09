
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LTT_TRACE
#define LTT_TRACE_FAST
#include <ltt/ltt-facility-user_generic.h>


void *thr1(void *arg)
{
	ltt_thread_init();	/* This init is not required : it will be done
												 automatically anyways at the first tracing call site */
	printf("thread 1, thread id : %lu, pid %lu\n", pthread_self(), getpid());

	while(1) {
		trace_user_generic_string("Hello world! Have a nice day.");
		sleep(2);
	}
	pthread_exit((void*)1);
}


/* Example of a _bad_ thread, which still works with the tracing */
void *thr2(void *arg)
{
	/* See ? no init */
	printf("thread 2, thread id : %lu, pid %lu\n", pthread_self(), getpid());
	sleep(1);
	while(1) {
		trace_user_generic_string("Hello world! Have a nice day.");
		sleep(2);
	}
	/* This thread is a bad citizen : returning like this will cause its cancel
	 * routines not to be executed. This is still detected by the tracer, but only
	 * when the complete process dies. This is not recommended if you create a
	 * huge amount of threads */
	return ((void*)2);
}


int main()
{
	int err;
	pthread_t tid1, tid2;
	void *tret;

	printf("Will trace the following string : Hello world! Have a nice day.\n");
	printf("Press CTRL-C to stop.\n");
	printf("No file is created with this example : it logs through a kernel\n");
	printf("system call. See the LTTng lttctl command to start tracing.\n\n");

	printf("thread main, thread id : %lu, pid %lu\n", pthread_self(), getpid());
	err = pthread_create(&tid1, NULL, thr1, NULL);
	if(err!=0) exit(1);

	err = pthread_create(&tid2, NULL, thr2, NULL);
	if(err!=0) exit(1);

	err = pthread_join(tid1, &tret);
	if(err!= 0) exit(1);

	err = pthread_join(tid2, &tret);
	if(err!= 0) exit(1);
	
	return 0;
}
