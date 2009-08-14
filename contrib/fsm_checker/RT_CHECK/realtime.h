#include <glib.h>
#define DEFAULT_PERIOD_SEC		0	
#define DEFAULT_PERIOD_NSEC		500000000 //500ms

#define DEFAULT_RUNNING_TIME_SEC	0
#define DEFAULT_RUNNING_TIME_NSEC	500000000 //500ms



struct realtime{
	int pid;			//set by checker
	long period_sec;		//tolerable period
	long period_nsec;	
	long running_time_sec; 		//needed time to execute per scheduling cycle
	long running_time_nsec;
	long schedin_ts_sec;
	long schedin_ts_nsec;
	struct realtimeContext _fsm;

};
struct realtime * realtime_Init(int, long, long, long, long);

void realtime_warning(struct realtime *, long, long);

void realtime_report_insufficient_scheduling_time(struct realtime *, long, long);

void realtime_save_ts(struct realtime *, long, long);

int latency(struct realtime *, long, long);

int running_enough(struct realtime *, long, long);

void removefsm(struct realtime *rtstruct);

