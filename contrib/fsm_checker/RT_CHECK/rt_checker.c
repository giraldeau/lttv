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
#include <math.h>
#include "realtime.h"

int application_pid;
long period_sec, period_nsec, running_time_sec, running_time_nsec;
long t2_sec, t2_nsec;
GArray *fsm_list;
struct timeval *tv1, *tv2;

static gboolean schedule(void *hook_data, void *call_data){
	LttvTracefileState *s = (LttvTracefileState *) call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;

	struct marker_field *f = lttv_trace_get_hook_field(th,0);
	guint32 prev_pid = ltt_event_get_long_unsigned(e, f);

	f = lttv_trace_get_hook_field(th, 1);
	guint32 next_pid = ltt_event_get_long_unsigned(e, f);

	LttTime time = ltt_event_time(e);
	t2_sec=(long)time.tv_sec;
	t2_nsec=(long)time.tv_nsec;
	struct realtime *rtstruct;
	if(application_pid==-1){
		//iterate over all fsms
		int i, found=0;

		for(i=0; i<fsm_list->len; i++)
		{
			rtstruct = g_array_index(fsm_list, struct realtime *, i);
			if(rtstruct->pid==next_pid){
				found=1;
				realtimeContext_schedule_in(&rtstruct->_fsm, t2_sec, t2_nsec);
				break;
			}
			else if(rtstruct->pid==prev_pid){
				found=1;
				realtimeContext_schedule_out(&rtstruct->_fsm, t2_sec, t2_nsec);
				break;
			}

		}	
		if(!found){
			rtstruct = realtime_Init(next_pid, DEFAULT_PERIOD_SEC, 
					DEFAULT_PERIOD_NSEC,
				       	DEFAULT_RUNNING_TIME_SEC,
					DEFAULT_RUNNING_TIME_NSEC);
			g_array_append_val(fsm_list, rtstruct);
			//call transition
			realtimeContext_schedule_in(&rtstruct->_fsm, t2_sec, t2_nsec);
			
		}
	}

	else//we might have already created the fsm so check @ first
	{
		if(fsm_list->len==0){
			rtstruct = realtime_Init(application_pid, period_sec, period_nsec, running_time_sec, running_time_nsec);
			g_array_append_val(fsm_list, rtstruct);

		}		
		else
			rtstruct = g_array_index(fsm_list, struct realtime *, 0);
	

	if(rtstruct->pid==next_pid)
		realtimeContext_schedule_in(&rtstruct->_fsm, t2_sec, t2_nsec);
	else if(rtstruct->pid==prev_pid)
		realtimeContext_schedule_out(&rtstruct->_fsm, t2_sec, t2_nsec);
	}		
	return FALSE;
}
void removefsm(struct realtime *rtstruct){
	int i;
	for(i=0; i<fsm_list->len; i++){
		struct realtime *tmp = g_array_index(fsm_list,struct realtime *, i);
		if(tmp==rtstruct){
			g_array_remove_index(fsm_list,i);
			break;
		}
	}

}
static int add_events_by_id_hooks(void *hook_data, void *call_data){
	LttvTraceContext *tc = (LttvTraceContext *) call_data;
	LttTrace *t = tc->t;

	//EVENT CHROOT
	GQuark LTT_FACILITY_KERNEL = g_quark_from_string("kernel");
	GQuark LTT_EVENT_SCHED_SCHEDULE = g_quark_from_string("sched_schedule");
	//EVENT FIELDS
	GQuark LTT_FIELD_PREV_PID = g_quark_from_string("prev_pid");
	GQuark LTT_FIELD_NEXT_PID = g_quark_from_string("next_pid");
	GQuark LTT_FIELD_PREV_STATE = g_quark_from_string("prev_state");


	GArray *hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 1);	

	lttv_trace_find_hook(t, LTT_FACILITY_KERNEL, LTT_EVENT_SCHED_SCHEDULE,
			FIELD_ARRAY(LTT_FIELD_PREV_PID, LTT_FIELD_NEXT_PID, LTT_FIELD_PREV_STATE),
			schedule,
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

     printf("Enter your real-time application ip (-1 for all)\n");
     scanf("%d", &application_pid);
     if(application_pid!=-1)
     {
             printf("Enter period_sec, period_nsec, running_time_sec, running_time_nsec:\n");
             //scanf("%ld%ld%ld%ld", &period_sec, &period_nsec, &running_time_sec, &running_time_nsec);
             period_sec=1;
             period_nsec=0;
             running_time_sec=0;
             running_time_nsec=500000;

     }
     tv1 = (struct timeval *)malloc(sizeof(struct timeval));
     gettimeofday(tv1, NULL);

     fsm_list = g_array_new(FALSE, FALSE, sizeof(struct realtime *));
}
static void destroy(){
     tv2 = (struct timeval *)malloc(sizeof(struct timeval));
     gettimeofday(tv2, NULL);
     int seconds = tv2->tv_sec - tv1->tv_sec;
     printf("analysis took: %d seconds \n", seconds);


}

LTTV_MODULE("rt_checker", "Detects scheduling latencies",
             "detects latencies",
             init, destroy, "stats", "batchAnalysis", "option")

