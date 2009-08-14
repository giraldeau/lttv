#include <glib.h>
#include <string.h>
struct rootjail * chrootjail_Init(){
	struct rootjail *this = (struct rootjail *) g_malloc(sizeof(struct rootjail)); 
	rootjailContext_Init(&this->_fsm, this);
	this->pid=-1;
	this->newroot = g_string_new("");
	return this;
}
void rootjail_savepid(struct rootjail *this, int pid){
	this->pid=pid;
}
void rootjail_savenewroot(struct rootjail *this, char *newroot){
	g_string_printf(this->newroot, newroot);
}
void rootjail_destroyfsm(struct rootjail *this){
	//remove fsm from fsm_list
	removefsm(this);
	g_string_free(this->newroot,TRUE);
	g_free(this);
}
void rootjail_warning(struct rootjail *this){
	printf("WARNING: pid %d attempted to open a file before calling chdir()\n", this->pid);
}
int checknewdir(char * newdir){
	if(!strcmp(newdir, "/"))//returns 0 when strings are equal
		return 1;
	return 0;
}
int thisprocess(struct rootjail *this, int pid){
	if(this->pid==pid)
		return 1;
	return 0;	
}
