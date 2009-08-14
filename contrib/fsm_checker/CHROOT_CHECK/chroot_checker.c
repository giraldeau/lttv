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
#include "chroot_jail.h"

GArray *_fsm_list;
struct timeval *tv1, *tv2;
int max_fsms = 0;
static gboolean chroot(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	
	char *rootDirectory = ltt_event_get_string(e, f);
	
	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	//initialize a new finite state machine	
	struct rootjail *rjstruct = chrootjail_Init();
	//add new fsm to list
	g_array_append_val(_fsm_list, rjstruct);
	//call corresponding transition
	rootjailContext_chroot(&(rjstruct->_fsm), pid, rootDirectory);

	if(max_fsms<_fsm_list->len)
		max_fsms=_fsm_list->len;	
	return FALSE;
}
static gboolean chdir(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *) hook_data;

	struct marker_field *f = lttv_trace_get_hook_field(th,0);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	char *newDirectory = ltt_event_get_string(e, f);
	
	int i;
	for(i=0; i<_fsm_list->len; i++){
		struct rootjail *rjstruct = g_array_index(_fsm_list, struct rootjail *, i);
		rootjailContext_chdir(&(rjstruct->_fsm), pid, newDirectory);
	}	

	return FALSE;

}
static gboolean open(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *) hook_data;

	struct marker_field *f = lttv_trace_get_hook_field(th,0);

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;	
	LttvTraceState *ts = (LttvTraceState *) tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[tfs->cpu];	
	int pid=process->pid;

	int i;
	for(i=0; i<_fsm_list->len; i++){
		struct rootjail *rjstruct = g_array_index(_fsm_list, struct rootjail *, i);
		rootjailContext_open(&(rjstruct->_fsm), pid);
	}	

	return FALSE;

}
int removefsm(struct rootjail *this){
	int i;
	for(i=0; i<_fsm_list->len; i++)
	{
		struct rootjail *rj = g_array_index(_fsm_list, struct rootjail *, i);
		if(rj==this)
		{
			g_array_remove_index(_fsm_list, i);
			break;
		}
	}
}
static int add_events_by_id_hooks(void *hook_data, void *call_data){
	LttvTraceContext *tc = (LttvTraceContext *) call_data;
	LttTrace *t = tc->t;

	//EVENT CHROOT
	GQuark LTT_FACILITY_FS = g_quark_from_string("fs");
	GQuark LTT_EVENT_CHROOT = g_quark_from_string("chroot");
		//EVENT FIELDS
		GQuark LTT_FIELD_ROOT = g_quark_from_string("RootDirectory");


	GQuark LTT_EVENT_CHDIR = g_quark_from_string("chdir");
		GQuark LTT_FIELD_DIRECTORY = g_quark_from_string("Directory");

	GQuark LTT_EVENT_OPEN = g_quark_from_string("open");	
		GQuark LTT_FIELD_FD = g_quark_from_string("fd");
		GQuark LTT_FIELD_FILENAME = g_quark_from_string("filename");	

	GArray *hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 3);	

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_CHROOT,
			FIELD_ARRAY(LTT_FIELD_ROOT),
			chroot,
			NULL,
			&hooks);

	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_CHDIR,
			FIELD_ARRAY(LTT_FIELD_DIRECTORY),
			chdir,
			NULL,
			&hooks);


	lttv_trace_find_hook(t, LTT_FACILITY_FS, LTT_EVENT_OPEN,
			FIELD_ARRAY(LTT_FIELD_FD, LTT_FIELD_FILENAME),
			open,
			NULL,
			&hooks);

	int nb_tracefiles = tc->tracefiles->len;
//someting may be missing here... after recovery file got split into 2 parts.
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
	_fsm_list = g_array_new(FALSE, FALSE, sizeof(struct rootjail *));

	tv1 = (struct timeval *)malloc(sizeof(struct timeval));
	gettimeofday(tv1, NULL);

}
static void destroy(){
	tv2 = (struct timeval *)malloc(sizeof(struct timeval));  
	gettimeofday(tv2, NULL);
	int seconds = tv2->tv_sec - tv1->tv_sec;
	printf("analysis took: %d seconds \n", seconds);
	printf("total number of coexisting fsms is: %d\n",max_fsms);

}

LTTV_MODULE("chroot_checker", "Detects improper chroot jailing",
	       "Catches attempts to open files after calling chroot without calling chdir",
       init, destroy, "stats", "batchAnalysis", "option")
