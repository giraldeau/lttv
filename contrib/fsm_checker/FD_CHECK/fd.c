#include "fd.h"
#include <glib.h>

struct fd * fd_Init(){
	struct fd *this = (struct fd *) g_malloc(sizeof(struct fd)); 
	fdContext_Init(&this->_fsm, this);
	this->pid=-1;
	this->fd=-1;
	return this;
}

int test_args(struct fd *this, int pid, int fd){
	if(this->pid==pid && this->fd==fd)
		return 1;
	return 0;
}

void fd_save_args(struct fd *this, int pid, int fd){
	this->pid=pid;
	this->fd=fd;	
}

int my_process_exit(struct fd *this, int pid){
	if(this->pid==pid)
		return 1;
	return 0;
}

void fd_destroy_scenario(struct fd *this, int i){
	//remove fsm from fsm_list... not yet implemented
	
	removefsm(i);
	g_free(this);
}
void fd_skip_FSM(){
	skip_FSM();
}
void fd_warning(struct fd *this, char *msg){
	printf("%s\n",msg);	
}
void fd_print_ts(struct fd *this, long ts_sec, long ts_nsec){
	printf("ts=%ld.%09ld\n", ts_sec, ts_nsec);

}
