
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LTT_TRACE
//this one is a non blocking sample (not #define LTT_BLOCKING 1)
#include <ltt/ltt-facility-user_generic.h>


void *thr1(void *arg)
{
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
	printf("thread 2, thread id : %lu, pid %lu\n", pthread_self(), getpid());
	sleep(1);
	while(1) {
		trace_user_generic_string("Hello world! Have a nice day.");
		sleep(2);
	}
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
