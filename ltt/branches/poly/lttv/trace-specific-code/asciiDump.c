
#include "ltt_module.h"

/* This module dumps all events in a simple ascii format */

static gboolean 
  ascii_dump = FALSE,
  syscall_stats = FALSE;

static gchar *dump_file = NULL;

static FILE *dump_fp = stdout;

struct poptOption 
  ascii_dump_option = { "ascii-dump", 'd', POPT_ARG_NONE, &ascii_dump, 0},
  ascii_dump_option = { "dump-file", 'f', POPT_ARG_STRING, &dump_file, 0},
  syscall_stats_option = { "syscall-stats", 's', POPT_ARG_NONE, 
      &syscall_stats, 0};

static void after_options_hook(gpointer hook_data, gpointer call_data); 

static void before_trace_hook(gpointer hook_data, gpointer call_data); 

static void after_trace_hook(gpointer hook_data, gpointer call_data); 

static void events_hook(gpointer hook_data, gpointer call_data); 

void init(int argc, char **argv)
{
  ltt_add_command_option(&ascii_dump_option);
  ltt_add_command_option(&syscall_stats_option);
  ltt_add_hook(ltt_after_options_hooks,after_options_hook,NULL)
}

/* Check the command line options and insert hooks to do the work */

static void after_options_hook(gpointer hook_data, gpointer call_data)
{
  if(ascii_dump_option || syscall_stats) { 
    ltt_add_hook(ltt_before_process_each_trace_hooks,before_trace_hook,NULL);
    if(dump_file != NULL) {
      dump_fp = fopen(dump_file,"w");
      if(dump_fp == NULL) g_critical("cannot open output file %s",dump_file);
    }
    ltt_add_hook(ltt_after_process_each_trace_hooks,after_trace_hook,NULL);
  }
}

/* Insert the hooks to print the events and compute and print the statistics */

static unsigned *eventsCounters;

struct CPUState {
  lttProcess *current_process;
  lttStatKey *key;
  lttTime lastTime;
} *CPUStates;

static void before_trace_hook(gpointer hook_data, gpointer call_data) {
  ltt_add_hook(ltt_trace_events_hooks,events_hooks,NULL);
  fprintf(dump_fp,"Trace %s\n",(struct trace *)call_data->name);

  if(ascii_dump) fprintf(dump_fp,"\nEvents\n");

  /* To gather stats, register a few hooks */

  if(syscall_stats) {
    eventsCounters = g_new0(unsigned,nbEventType);
    CPUStates = g_new0(struct CPUState, nbCPU);
    /* initialize the state of each CPU and associated process */
    CHECK
  }
}

/* Print the events */

static void events_hook(gpointer hook_data, gpointer call_data)
{
  event_struct event;

  int i;

  event = (struct_event *)call_data;

  if(ascii_dump) {
    fprintf(dump_fp,"\n%s.%s t=%d.%d CPU%d",event->facility_handle->name,
	    event->event_handle->name, event->time.tv_seconds, 
            event->time.tv_nanoseconds,event->CPU_id);

    for(i = 0 ; i < event->base_field->nb_elements ; i++) {
      field = event->base_field->fields + i;
      fprintf(dump_fp," %s=",field->name);
      switch(field->type) {
        case INT:
          fprintf(dump_fp,"%d",ltt_get_integer(field,event->data));
          break;
        case UINT:
          fprintf(dump_fp,"%u",ltt_get_uinteger(field,event->data));
          break;
        case FLOAT:
          fprintf(dump_fp,"%lg",ltt_get_float(field,event->data));
          break;
        case DOUBLE:
          fprintf(dump_fp,"%g",ltt_get_double(field,event->data));
          break;
        case STRING:
          fprintf(dump_fp,"%s",ltt_get_string(field,event->data));
          break;
        case ENUM:
          fprintf(dump_fp,"%d",ltt_get_integer(field,event->data));
          break;
        case ARRAY:
          fprintf(dump_fp,"<nested array>");
          break;
        case SEQUENCE:
          fprintf(dump_fp,"<nested sequence>");
          break;
        case STRUCT:
          fprintf(dump_fp,"<nested struct>");
          break;
      }
    }
  }

  /* Collect statistics about each event type */

  if(syscall_stats) {
    /* Get the key for the corresponding CPU. It already contains the
       path components for the ip, CPU, process, state, subState.
       We add the event id and increment the statistic with that key.  */

    key = (GQuark *)CPUStates[event->CPUid]->key1;
    path = key->data;
    path[5] = eventsQuark[event->id];
    pval = ltt_get_integer(currentStats,key);
    (*pval)++;

    /* Count the time spent in the current state. Could be done only
       at state changes to optimize. */

    key = (GQuark *)CPUStates[event->CPUid]->key2;
    path = key->data;
    ptime = ltt_get_time(currentStats,key);
    (*ptime) = ltt_add_time((*ptime),ltt_sub_time(lastTime,event->time));    
  }
} 

/* Specific hooks to note process and state changes, compute the following values: number of bytes read/written,
   time elapsed, user, system, waiting, time spent in each system call,
   name for each process. */

maintain the process table, process state, last time... what we are waiting for

syscall_entry_hook
syscall_exit_hook
trap_entry_hook
trap_exit_hook
irq_entry_hook
irq_exit_hook
sched_change_hook -> not waiting
fork_hook -> wait fork
wait_hook -> waiting
wakeup_hook -> not waiting add up waiting time
exit_hook
exec_hook -> note file name
open_hook -> keep track of fd/name
close_hook -> keep track of fd
read_hook -> bytes read, if server CPU for client...
write_hook -> bytes written
select_hook -> wait reason
poll_hook -> wait reason
mmap_hook -> keep track of fd
munmap_hook -> keep track of fd
setitimer_hook -> wait reason
settimeout_hook -> wait reason
sockcreate_hook -> client/server
sockbind_hook -> client/server
sockaccept_hook -> client/server
sockconnect_hook -> client/server
/* Close the output file and print the statistics, globally for all CPUs and
   processes, per CPU, per process. */

static void after_trace_hook(gpointer hook_data, gpointer call_data)
{
  lttTrace t;

  unsigned nbEvents = 0;

  t = (lttTrace *)call_data;

  fprintf(dump_fp,"\n");
  fclose(dump_fp);

  if(syscall_stats) {
    fprintf(dump_fp,"\n\nStatistics\n\n");

    /* Trace start, end and duration */

    fprintf(dump_fp,"Trace started %s, ended %s, duration %s",
	    ltt_format_time(t->startTime),ltt_format_time(t->endTime),
            ltt_format_time(ltt_sub_time(t->endTime,t->startTime)));

    /* Number of events of each type */

    for(i = 0 ; i < t->nbEventTypes ; i++) {
      nbEvents += eventsCounters[i];
      if(eventsCounters[i] > 0) 
	fprintf(dump_fp,"%s: %u\n",t->types[i]->name,eventsCounters[i]);
    }
    fprintf(dump_fp,"\n\nTotal number of events: %u\n",nbEvents);

    /* Print the details for each process */
  }
}



