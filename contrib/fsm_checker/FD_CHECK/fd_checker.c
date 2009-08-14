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
#include "fd.h"

GArray *_fsm_list;
GArray * _fsms_to_remove;
struct timeval *tv1, *tv2;
int max_fsms = 0;
int total_forks = 0;
int count_events = 0;
int skip=0;

static gboolean fs_close(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 fd = ltt_event_get_long_unsigned(e, f);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	//initialize a new finite state machine	
	struct fd *fdstruct = fd_Init();
	total_forks++;
	//add new fsm to list
	g_array_append_val(_fsm_list, fdstruct);
	//call corresponding transition
	fdContext_fs_close(&(fdstruct->_fsm), pid, fd);

	if(max_fsms<_fsm_list->len)
		max_fsms=_fsm_list->len;	
	return FALSE;

}

static gboolean fs_open(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 fd = ltt_event_get_long_unsigned(e, f);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	//for every fsm in the list, call appropriate transition
	//index i is sent as an argument in case fsm is to be freed
	struct fd *fdstruct;
	int i;
	for(i=_fsm_list->len-1; i>=0; i--){
		if (skip)
			break;
		fdstruct = (struct fd *) g_array_index(_fsm_list, struct fdstruct *, i);	
		fdContext_fs_open(&(fdstruct->_fsm), pid, fd, i);
	}
	skip=0;
	return FALSE;
}

static gboolean fs_read(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 fd = ltt_event_get_long_unsigned(e, f);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	LttTime time = ltt_event_time(e);
	long ts_nsec, ts_sec;
	ts_sec = (long) time.tv_sec;
	ts_nsec=(long) time.tv_nsec;

	//for every fsm in the list, call appropriate transition
	struct fd *fdstruct;
	int i;
	for(i=0; i<_fsm_list->len; i++){
		if (skip)
			break;
		fdstruct = (struct fd *)g_array_index(_fsm_list, struct fd *, i);	
		fdContext_fs_read(&(fdstruct->_fsm), pid, fd, ts_sec, ts_nsec);
	}

	skip=0;
	return FALSE;
}
static gboolean fs_write(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	guint32 fd = ltt_event_get_long_unsigned(e, f);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	//for every fsm in the list, call appropriate transition
	struct fd *fdstruct;
	int i;
	for(i=0; i<_fsm_list->len; i++){
		if (skip)
			break;
		fdstruct = g_array_index(_fsm_list, struct fd *, i);	
		fdContext_fs_write(&(fdstruct->_fsm), pid, fd);
	}
	skip=0;
	return FALSE;
}
static gboolean process_exit(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;


	//for every fsm in the list, call appropriate transition
	struct fd *fdstruct;
	int i;
	for(i=_fsm_list->len-1; i>=0 ; i--){
		fdstruct =(struct fd *) g_array_index(_fsm_list, struct fd *, i);	
		fdContext_process_exit(&(fdstruct->_fsm), pid, i);
	}
	skip=0;
	return FALSE;
}
static gboolean fs_dup3(void *hook_data, void *call_data){
	count_events++;
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	
	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	struct marker_field *f = lttv_trace_get_hook_field(th, 1);
	guint32 newfd = ltt_event_get_long_unsigned(e, f);

	//for every fsm in the list, call appropriate transition
	struct fd *fdstruct;
	int i;
	for(i=_fsm_list->len-1; i>=0; i--){
		if (skip)
			break;
		fdstruct =(struct fd *) g_array_index(_fsm_list, struct fd *, i);	
		fdContext_fs_dup3(&(fdstruct->_fsm), pid, newfd, i);
	}
	skip=0;
	return FALSE;

}

void skip_FSM(){
	skip=1;
}	

int removefsm(int i){
	g_array_remove_index(_fsm_list, i);
	}
static int add_events_by_id_hooks(void *hook_data, void *call_data){
	LttvTraceContext *tc = (LttvTraceContext *) call_data;
	LttTrace *t = tc->t;

	//EVENT CHROOT
	GQuark LTT_FACILITY_FS = g_quark_from_string("fs");
	GQuark LTT_FACILITY_KERNEL = g_quark_from_string("kernel");	

	GQuark LTT_EVENT_CLOSE = g_quark_from_string("close");
		//EVENT FIELDS
		GQuark LTT_FIELD_FD = g_quark_from_string("fd");
		GQuark LTT_FIELD_OLD_FD = g_quark_from_string("oldfd");
		GQuark LTT_FIELD_NEW_FD = g_quark_from_string("newfd");

	GQuark LTT_EVENT_OPEN = g_quark_from_string("open");

	GQuark LTT_EVENT_READ = g_quark_from_string("read");	

	GQuark LTT_EVENT_WRITE = g_quark_from_string("write");	
	
	GQuark LTT_EVENT_DUP3 = g_quark_from_string("dup3");

	GQuark LTT_EVENT_PROCESS_EXIT = g_quark_from_string("process_exit");

	GArray *hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 6);	

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_CLOSE,
			FIELD_ARRAY(LTT_FIELD_FD),
			fs_close,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_OPEN,
			FIELD_ARRAY(LTT_FIELD_FD),
			fs_open,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_READ,
			FIELD_ARRAY(LTT_FIELD_FD),
			fs_read,
			NULL,
			&hooks);


	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_WRITE,
			FIELD_ARRAY(LTT_FIELD_FD),
			fs_write,
			NULL,
			&hooks);
	
	lttv_trace_find_hook(t, LTT_FACILITY_KERNEL, LTT_EVENT_PROCESS_EXIT,
			NULL,
			process_exit,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_DUP3,
			FIELD_ARRAY(LTT_FIELD_OLD_FD, LTT_FIELD_NEW_FD),
			fs_dup3,
			NULL,
			&hooks);

	int nb_tracefiles = tc->tracefiles->len;
	LttvTracefileContext **tfc;
	LttvHooks *needed_hooks;
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	int i, j;
	for(i=0; i<nb_tracefiles; i++){
		tfc = &g_array_index(tc->tracefiles, LttvTracefileContext*, i);
		for(j=0; j<hooks->len; j++){
			th=&g_array_index(hooks, LttvTraceHook, j);
			needed_hooks = lttv_hooks_by_id_find((*tfc)->event_by_id, th->id);
			lttv_hooks_add(needed_hooks, th->h, th, LTTV_PRIO_DEFAULT);
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
	//This function will be overwritten between checkers
	lttv_hooks_add(before_trace, add_events_by_id_hooks, NULL, LTTV_PRIO_DEFAULT);

	//Initialize empty GArray for FSMs
	_fsm_list = g_array_new(FALSE, FALSE, sizeof(struct fd *));

	tv1 = (struct timeval *)malloc(sizeof(struct timeval));
	gettimeofday(tv1, NULL);
}
static void destroy(){
	tv2 = (struct timeval *)malloc(sizeof(struct timeval));  
	gettimeofday(tv2, NULL);
	int seconds = tv2->tv_sec - tv1->tv_sec;
	printf("analysis took: %d seconds \n", seconds);
	printf("total number of coexisting fsms is: %d\n",max_fsms);
	printf("total number of forked fsms is: %d\n", total_forks);
	printf("size of one fsm (in bytes) is: %d\n", sizeof(struct fd));
	printf("total number of pertinent events is %d\n", count_events);

}

LTTV_MODULE("fd_checker", "Detects improper fd usage",
	       "finds read/write access to a closed fd in a trace file",
       init, destroy, "stats", "batchAnalysis", "option")	       
