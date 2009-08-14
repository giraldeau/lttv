#include "fd_sm.h"
#include <glib.h>


struct fd
{
	int pid;	
	int fd;
	struct fdContext _fsm;
};
struct fd * fd_Init();
void fd_save_args(struct fd *this, int pid, int fd);
void fd_destroy_scenario(struct fd *, int);
void fd_warning(struct fd *this, char *msg);

int my_process_exit(struct fd *this, int pid);
int test_args(struct fd *this, int pid, int fd);
void skip_FSM();
