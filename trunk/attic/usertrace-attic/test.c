
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "lttng_usertrace.h"



void *thr1(void *arg)
{
	lttng_thread_init();
  printf("thread 1, thread id : %lu, pid %lu\n", pthread_self(), getpid());

  while(1) {}

  return ((void*)1);

}

void *thr2(void *arg)
{
	lttng_thread_init();

  while(1) {
    printf("thread 2, thread id : %lu, pid %lu\n", pthread_self(), getpid());
    sleep(2);
  }
  return ((void*)2);
}


int main()
{
	int err;
	pthread_t tid1, tid2;
	void *tret;

  printf("thread main, thread id : %lu, pid %lu\n", pthread_self(), getpid());
  err = pthread_create(&tid1, NULL, thr1, NULL);
  if(err!=0) exit(1);

  err = pthread_create(&tid2, NULL, thr2, NULL);
  if(err!=0) exit(1);

  while(1)
  {
    
  }

  err = pthread_join(tid1, &tret);
  if(err!= 0) exit(1);

  err = pthread_join(tid2, &tret);
  if(err!= 0) exit(1);
  
  return 0;
}
