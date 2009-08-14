#include <glib.h>
#include <string.h>
struct realtime * realtime_Init(int pid, long period_sec, long period_nsec, long running_time_sec, long running_time_nsec){
	struct realtime *this = (struct realtime *) g_malloc(sizeof(struct realtime)); 
	realtimeContext_Init(&this->_fsm, this);
	this->pid = pid;
	this->period_sec=period_sec;
	this->period_nsec=period_nsec;
	this->running_time_sec = running_time_sec;
	this->running_time_nsec = running_time_nsec;
	this->schedin_ts_sec=0;//needed for first event
	this->schedin_ts_nsec=0;
	return this;
}
void realtime_destroyfsm(struct realtime *this){
	//remove fsm from fsm_list
	removefsm(this);
	g_free(this);
}
void realtime_warning(struct realtime *this, long ts_sec, long ts_nsec){
	printf("WARNING: real-time process, pid %d was scheduled in after tolerable period @ %ld.%09ld.\n", this->pid, ts_sec, ts_nsec);
}
void realtime_report_insufficient_scheduling_time(struct realtime *this, long ts_sec, long ts_nsec){
	printf("WARNING: real-time process, pid %d was scheduled out early @%ld.%09ld.\n", this->pid, ts_sec, ts_nsec);
}
int latency(struct realtime *this, long ts_sec, long ts_nsec){
	//2 successive schedin are seperated by more than period
	long delta_sec = ts_sec - this->schedin_ts_sec;
	long delta_nsec = ts_nsec - this->schedin_ts_nsec;
	if(delta_sec < this->period_sec)
		return 0;//no latency
	else if(delta_sec == this->period_sec)
		if(delta_nsec < this->period_nsec)
			return 0;//no latency
	return 1;
}
int running_enough(struct realtime *this, long ts_sec, long ts_nsec){
	if(ts_sec - this->schedin_ts_sec > this->running_time_sec)
		return 1;
	else if(ts_sec - this->schedin_ts_sec == this->running_time_sec)
		if(ts_nsec - this->running_time_nsec > this->running_time_nsec)
			return 1;
	return 0;
}
void realtime_save_ts(struct realtime *this, long ts_sec, long ts_nsec){
	this->schedin_ts_sec = ts_sec;
	this->schedin_ts_nsec = ts_nsec;	
}

