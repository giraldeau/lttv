#include <glib.h>


struct rootjail
{
	int pid;
	GString *newroot;
	struct rootjailContext _fsm;
};

struct rootjail * chrootjail_Init();

void rootjail_savepid(struct rootjail *, int);

void rootjail_savenewroot(struct rootjail *, char *);

void rootjail_destroyfsm(struct rootjail *);

void rootjail_warning(struct rootjail *);

int checknewdir(char *);

int removefsm(struct rootjail *);

