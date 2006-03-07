
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "ltt-usertrace-fast.h"



void *thr1(void *arg)
{
	int i;
	ltt_thread_init();
  printf("thread 1, thread id : %lu, pid %lu\n", pthread_self(), getpid());

  //while(1) {}
	for(i=0; i<5; i++) {
		ltt_usertrace_fast_buffer_switch();
		sleep(1);
	}

  //return ((void*)1);
	pthread_exit((void*)1);
}

void *thr2(void *arg)
{
	int i;
	ltt_thread_init();
  //while(1) {
    printf("thread 2, thread id : %lu, pid %lu\n", pthread_self(), getpid());
    sleep(2);
  //}
	for(i=0; i<2; i++) {
		ltt_usertrace_fast_buffer_switch();
		sleep(3);
	}


  return ((void*)2);	/* testing "die" */
	//pthread_exit((void*)2);
}


int main()
{
	int i;
	int err;
	pthread_t tid1, tid2;
	void *tret;

  printf("thread main, thread id : %lu, pid %lu\n", pthread_self(), getpid());
  err = pthread_create(&tid1, NULL, thr1, NULL);
  if(err!=0) exit(1);

  err = pthread_create(&tid2, NULL, thr2, NULL);
  if(err!=0) exit(1);

	for(i=0; i<2; i++) {
		ltt_usertrace_fast_buffer_switch();
		sleep(3);
	}

  err = pthread_join(tid1, &tret);
  if(err!= 0) exit(1);

  err = pthread_join(tid2, &tret);
  if(err!= 0) exit(1);
  
  return 0;
}
