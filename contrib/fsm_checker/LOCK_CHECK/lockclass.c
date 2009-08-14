#include "lockclass.h"



struct lockclass * lockclass_Init(int CPU){
	struct lockclass *this = (struct lockclass *) g_malloc(sizeof(struct lockclass));
	lockclassContext_Init(&this->_fsm, this);
	this->cpu = CPU;
	this->local_stack = g_array_new(FALSE, FALSE, sizeof(struct lockstruct *));
	return this;
}

struct lockstruct * lockstruct_Init(guint32 address){
	struct lockstruct *this = (struct lockstruct *) g_malloc(sizeof(struct lockstruct));
	this->lock_add = address;
	this->taken_irqs_off=0;
	this->taken_irqs_on=0;
		this->hardirq_context=0;
	return this;
}
int irq_check(struct lockclass *fsm, void *lock,  int hardirqs_off, int hardirq_context){
	struct lockstruct *lock_check = (struct lockstruct *)lock;
	if(hardirq_context)
		lock_check->hardirq_context=hardirq_context;
	if(hardirq_context && lock_check->taken_irqs_on)
		return 1;
	else if(lock_check->hardirq_context && !hardirqs_off)
		return 1;	

	return 0;
}
int empty_stack(struct lockclass *fsm){
	return fsm->local_stack->len;
}
int lock_held(struct lockclass *fsm, struct lockstruct *lock){
	//loop through stack & return 1 if found, 0 otherwise
	int i, len=fsm->local_stack->len;
	for(i=0; i<len; i++)
	{
		struct lockstruct *temp = g_array_index(fsm->local_stack, struct lockstruct *, i);
		if(temp==lock)
			return 1;
	}
	return 0;
}
int lock_held_on_behalf(struct lockclass *fsm, guint32 pid){
	//loop through stack & return 1 if a lock is being held on behalf of pid
	int i, len=fsm->local_stack->len;
	for(i=0; i<len; i++)
	{
		struct lockstruct *temp=g_array_index(fsm->local_stack, struct lockstruct *, i);
		if(temp->pid==pid){
			return 1;

		}	
	}
	return 0;
}
void lockclass_test(){
		//the smc compiler didn't generate the right code since no action was specified. (only a guard)
	return;
}
void lockclass_warning(struct lockclass *fsm, char *msg, struct lockstruct *lock){
	printf("WARNING: %s\n", msg);
	if(lock!=NULL)
		printf("Lock 0x%x: taken_hard_irqs_on: %d, taken_hard_irqs_off %d, hardirq_context %d\n ",
			lock->lock_add, lock->taken_irqs_on, lock->taken_irqs_off, lock->hardirq_context);
}
void lockclass_pushlock(struct lockclass *fsm, struct lockstruct *lock){
	g_array_append_val(fsm->local_stack, lock);
}

void lockclass_poplock(struct lockclass *fsm, struct lockstruct *lock){
	int i;
	struct lockstruct *temp;
	for(i=fsm->local_stack->len-1; i>=0; i--){
		temp=g_array_index(fsm->local_stack, struct lockstruct *, i);	
		if(temp==lock){
			g_array_remove_index(fsm->local_stack, i);
			break;
		}
	}	
}

