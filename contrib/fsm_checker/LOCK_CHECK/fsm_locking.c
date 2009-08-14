#include <lttv/lttv.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/trace.h>
#include <stdio.h>
#include "fsm_locking_sm.h"
#include "lockclass.h"


int NUM_OF_CPUS;
//one hashing table for all locks
GHashTable *locksTable;
GArray *fsm_array;
int total_acquire;
//Global timestamps very useful for debugging
long ts_sec;
long ts_ns;

void lockclass_clearlock(struct lockclass *fsm, struct lockstruct *lock){
	struct lockstruct *temp;
		
	temp = g_hash_table_lookup(locksTable,(gconstpointer)lock->lock_add);	
	if(temp!=NULL){
		temp->taken_irqs_on=0;
		temp->taken_irqs_off=0;
		temp->hardirq_context =0;
	}
}

static gboolean lockdep_init_map(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	int cpu = s->cpu;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;

	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 lock_add = ltt_event_get_long_unsigned(e, f);

	//TODO:the lock_add being initialized by lockdep should not be present in stack
	struct lockclass *fsm = g_array_index(fsm_array,struct lockclass *, cpu);

	struct lockstruct *lock = g_hash_table_lookup(locksTable,(gconstpointer)lock_add);	
	if(lock==NULL)
		return FALSE;

	LttTime time = ltt_event_time(e);
	ts_sec=(long)time.tv_sec;
	ts_ns=(long)time.tv_nsec;

	lockclassContext_free_lock(&fsm->_fsm, lock);
	return FALSE;
}
static gboolean kernel_sched_schedule(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	//printf("event sched_schedule encountered on cpu: %d\n", s->cpu);
	//work should be done per processor
	int cpu = s->cpu;
	//parse event here
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;	
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 prev_pid = ltt_event_get_long_unsigned(e, f);

	f = lttv_trace_get_hook_field(th, 1);
	guint32 next_pid = ltt_event_get_long_unsigned(e, f);

	LttTime time = ltt_event_time(e);
	ts_sec=(long)time.tv_sec;
	ts_ns=(long)time.tv_nsec;

	struct lockclass *fsm = g_array_index(fsm_array,struct lockclass *,  cpu);

	lockclassContext_schedule_out(&fsm->_fsm, prev_pid);	
	return FALSE; 
}


static gboolean lockdep_lock_acquire(void *hook_data, void *call_data){
	total_acquire++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	int cpu = s->cpu;
	
	//parse event here
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;	
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 2);
	guint32 lock_add = ltt_event_get_long_unsigned(e, f);
	
	f= lttv_trace_get_hook_field(th, 5);
	int hardirqs_off = ltt_event_get_long_unsigned(e, f);
	
	f=lttv_trace_get_hook_field(th, 0);
	guint32 ret_add = ltt_event_get_long_unsigned(e, f);
	
	f=lttv_trace_get_hook_field(th,4);
	int read = ltt_event_get_long_unsigned(e, f);
	
	f=lttv_trace_get_hook_field(th, 6);
	int hardirq_context = ltt_event_get_long_unsigned(e, f);

	LttvTraceState *ts = (LttvTraceState*) s->parent.t_context;
	LttvProcessState *process = ts->running_process[cpu];
	int pid = process->pid;

	LttTime time = ltt_event_time(e);
	ts_sec=(long)time.tv_sec;
	ts_ns=(long)time.tv_nsec;


	//filter rwlock_acquire_read and rwsem_acquire_read
	if(read==2 || read==1)
		return FALSE;
	//read needs to be 0: spin_acquire, rwlock_acquire, mutex_acquire, rwsem_acquire & lock_map_acquire
	struct lockclass *fsm = g_array_index(fsm_array,struct lockclass *, cpu);
	
	struct lockstruct *lock = g_hash_table_lookup(locksTable,(gconstpointer)lock_add);
	if(lock==NULL){
		lock = lockstruct_Init(lock_add);
		//add lock to table
		g_hash_table_insert(locksTable, (gpointer)lock_add,lock);
	}
	//lock->pid = pid;//update pid
	lockclassContext_acquire_lock(&fsm->_fsm,lock, lock->lock_add, hardirqs_off, hardirq_context, pid);

	return FALSE;	
}
static gboolean lockdep_lock_release(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;	
	int cpu = s->cpu;

	//parse event here
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;	
	struct marker_field *f = lttv_trace_get_hook_field(th, 1);
	guint32 lock_add = ltt_event_get_long_unsigned(e, f);

	LttvTraceState *ts = (LttvTraceState*) s->parent.t_context;
	LttvProcessState *process = ts->running_process[cpu];
	int pid = process->pid;

	LttTime time = ltt_event_time(e);
	ts_sec=(long)time.tv_sec;
	ts_ns=(long)time.tv_nsec;

	struct lockstruct *lock = g_hash_table_lookup(locksTable,(gconstpointer)lock_add);
	
      	if(lock==NULL)	
		return FALSE;

	struct lockclass *fsm = g_array_index(fsm_array,struct lockclass *,  cpu);
	
	lockclassContext_release_lock(&fsm->_fsm, lock);
	return FALSE;
}
void printlock(struct lockstruct * lock){
	printf("Lock 0x%x: held_irqs_on: %d held_irqs_off %d irqcontext: %d pid: %u\n",
			lock->lock_add, lock->taken_irqs_on, 
			lock->taken_irqs_off, lock->hardirq_context, 
			lock->pid);
}

void lockclass_printstack(struct lockclass *fsm){
	GArray *localstack = fsm->local_stack;	
	int len = localstack->len;
	int i;
	struct lockstruct *lock;
	for(i=0; i<len; i++){
		lock = g_array_index(localstack, struct lockstruct *, i);	
		printlock(lock);
	}
}
void lockclass_schedule_err(struct lockclass *fsm, guint32 pid){
	printf("process %u was scheduled out\n",pid);
}


static int add_events_by_id_hooks(void *hook_data, void *call_data){
	LttvTraceContext *tc = (LttvTraceContext *) call_data; 
	LttTrace *t = tc->t;

	//FIND NUMBER OF CPUS
	NUM_OF_CPUS = ltt_trace_get_num_cpu(t);

	//	EVENT ***LOCKDEP_LOCK_ACQUIRE***
	GQuark LTT_FACILITY_LOCKDEP = g_quark_from_string("lockdep");
	GQuark LTT_EVENT_LOCK_ACQUIRE = g_quark_from_string("lock_acquire");

	GQuark LTT_FIELD_RET_ADDRESS = g_quark_from_string("retaddr");
	GQuark LTT_FIELD_SUBCLASS = g_quark_from_string("subclass");
	GQuark LTT_FIELD_LOCK = g_quark_from_string("lock");
	GQuark LTT_FIELD_TRYLOCK = g_quark_from_string("trylock");
	GQuark LTT_FIELD_READ = g_quark_from_string("read");
	GQuark LTT_FIELD_HARD_IRQS_OFF = g_quark_from_string("hardirqs_off");
	GQuark LTT_FIELD_HARDIRQ_CONTEXT = g_quark_from_string("hardirq_context");
	//*******************************************************************

	//	EVENT ***KERNEL_SCHED_SCHEDULE
	GQuark LTT_FACILITY_KERNEL = g_quark_from_string("kernel");
	GQuark LTT_EVENT_SCHED_SCHEDULE = g_quark_from_string("sched_schedule");
	
	GQuark LTT_FIELD_PREV_PID = g_quark_from_string("prev_pid");
	GQuark LTT_FIELD_NEXT_PID = g_quark_from_string("next_pid");
	GQuark LTT_FIELD_PREV_STATE = g_quark_from_string("prev_state");
	//*******************************************************************
	//	EVENT ***LOCKDEP_LOCK_RELEASE***
	GQuark LTT_EVENT_LOCK_RELEASE = g_quark_from_string("lock_release");
	
	GQuark LTT_FIELD_NESTED = g_quark_from_string("nested");
	
	//******************************************************************* 
	//	EVENT ***LOCKDEP_INIT_MAP***
	GQuark LTT_EVENT_INIT_MAP = g_quark_from_string("init_map"); 

	//*******************************************************************
	
	//	EVENT ***...
	
	//...
	
	//*******************************************************************

	//# of hooks to register = # of desired events
	GArray *hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 3 );
	
	lttv_trace_find_hook(t, LTT_FACILITY_LOCKDEP, LTT_EVENT_LOCK_ACQUIRE,
			FIELD_ARRAY(LTT_FIELD_RET_ADDRESS, LTT_FIELD_SUBCLASS, LTT_FIELD_LOCK, LTT_FIELD_TRYLOCK,
				LTT_FIELD_READ, LTT_FIELD_HARD_IRQS_OFF, LTT_FIELD_HARDIRQ_CONTEXT),
			lockdep_lock_acquire,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_KERNEL, LTT_EVENT_SCHED_SCHEDULE, 
			FIELD_ARRAY(LTT_FIELD_PREV_PID, LTT_FIELD_NEXT_PID, LTT_FIELD_PREV_STATE),
			kernel_sched_schedule,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_LOCKDEP, LTT_EVENT_LOCK_RELEASE,
			FIELD_ARRAY(LTT_FIELD_RET_ADDRESS, LTT_FIELD_LOCK, LTT_FIELD_NESTED),
			lockdep_lock_release,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_LOCKDEP, LTT_EVENT_INIT_MAP,
			FIELD_ARRAY(LTT_FIELD_LOCK),
			lockdep_init_map,
			NULL,
			&hooks);
	//find remaining hooks here to fill the hooks array
	//...
	
	//LttvTraceHook *th = &g_array_index(hooks, LttvTraceHook, 0);


	//Determine id of needed events
	GQuark evQuark_lock_acquire = g_quark_try_string("lockdep_lock_acquire");
	struct marker_info *info_lock_acquire = marker_get_info_from_name(t, evQuark_lock_acquire);
	int ev_id_lock_acquire = marker_get_id_from_info(t, info_lock_acquire);

	//printf("id of event of interest: %d\n", ev_id_lock_acquire);
	
	//when multiple tracefiles exists:	
	int nb_tracefiles = tc->tracefiles->len;	
	int i, j;
	LttvTracefileContext **tfc;
	LttvHooks *needed_hooks;
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	for(i=0; i<nb_tracefiles; i++){
		tfc = &g_array_index(tc->tracefiles, LttvTracefileContext*, i);
		//printf("adding hooks for tracefile #%d\n",i);
		//add all needed hooks
		for(j=0; j<hooks->len; j++)
		{
			th=&g_array_index(hooks, LttvTraceHook, j);
			needed_hooks = lttv_hooks_by_id_find((*tfc)->event_by_id, th->id);
			lttv_hooks_add(needed_hooks, th->h, th, LTTV_PRIO_DEFAULT);
			//printf("hooked with event id: %d\n", th->id);
		}
	}

	fsm_array = g_array_sized_new(FALSE, FALSE, sizeof(struct lockclass *), NUM_OF_CPUS);
	
	struct lockclass *class;
	for(i=0; i<NUM_OF_CPUS; i++){
		class = lockclass_Init(i);
		g_array_append_val(fsm_array, class);
	}
	locksTable = g_hash_table_new(g_direct_hash, NULL);
	total_acquire=0;
}
//function to be moved to lockclass when testing is done:
void lockclass_updatelock(struct lockclass *fsm, struct lockstruct *lock, guint32 lock_add, int pid, int hardirqs_off, int hardirq_context){
	struct lockstruct *temp = g_hash_table_lookup(locksTable,(gconstpointer)lock_add);
	
	if(temp==NULL)
		printf("Attemping to update an uninitialized lock");
	if(temp!=NULL){
		temp->pid = pid;
		if(hardirq_context==1){
			temp->hardirq_context=1;
			temp->hardirq_context_ts_sec=ts_sec;
			temp->hardirq_context_ts_ns=ts_ns;
		}
		if(hardirqs_off==1){
			temp->taken_irqs_off=1;
			temp->taken_irqs_off_ts_sec=ts_sec;
			temp->taken_irqs_off_ts_ns=ts_ns;
		}
		else if(hardirqs_off==0){
			temp->taken_irqs_on=1;
			temp->taken_irqs_on_ts_sec=ts_sec;
			temp->taken_irqs_on_ts_ns=ts_ns;
		}

	}
}
static void init(){
	
	gboolean result;

	LttvAttributeValue value;

	LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

	static LttvHooks *before_trace;

	result = lttv_iattribute_find_by_path(attributes, "hooks/trace/before", LTTV_POINTER, &value);
	g_assert(result);
	before_trace = *(value.v_pointer);
	g_assert(before_trace);
	//Register add_events_by_id_hook to be called before starting to read the trace
	lttv_hooks_add(before_trace, add_events_by_id_hooks, NULL, LTTV_PRIO_DEFAULT);

	//******************************************************************
	
		
}
static void destroy(){
	printf("total lock_acquire %d\n", total_acquire);	
	printf("\nEnd of locks analysis.\n");	
}
LTTV_MODULE("fsm_locking", "Detects improper use of kernel locks", \
		"4 scenarios of problematic use of spinlocks are searched for",\
		init, destroy, "stats", "batchAnalysis", "option")
