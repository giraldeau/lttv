#include <glib.h>
#include "fsm_locking_sm.h"
struct lockclass{
	struct lockclassContext _fsm;
	int cpu;		//one class per CPU
	GArray *local_stack;	//stack containing held (per CPU) locks	
};

struct lockstruct{
	guint32 lock_add;
	guint32 ret_add;

	int taken_irqs_off;
	long taken_irqs_off_ts_sec,taken_irqs_off_ts_ns;

	int taken_irqs_on;
	long taken_irqs_on_ts_sec, taken_irqs_on_ts_ns;

	int hardirq_context;	//1 if taken inside an irq handler
	long hardirq_context_ts_sec, hardirq_context_ts_ns;
	
	int pid;		//pid of the process currently holding the lock
				//only valid if lock is present in stack;
};


struct lockclass * lockclass_Init(int CPU);

struct lockstruct * lockstruct_Init();
int irq_check(struct lockclass *fsm, void *lock,  int hardirqs_off, int hardirq_context);

int empty_stack(struct lockclass *fsm);
int lock_held(struct lockclass *fsm, struct lockstruct *lock);
int lock_held_on_behalf(struct lockclass *fsm, guint32 pid);
void lockclass_warning(struct lockclass *fsm, char *msg, struct lockstruct *);
void lockclass_printstack(struct lockclass *fsm);
void poplock(struct lockclass *fsm, struct lockstruct *lock);
void pushlock(struct lockclass *fsm, struct lockstruct *lock);
void lockclass_clearlock(struct lockclass *fsm, struct lockstruct *lock);
void lockclass_updatelock(struct lockclass *fsm,  struct lockstruct *lock, guint32 lock_add, int pid, int hardirqs_off, int hardirq_context);
void lockclass_test();
