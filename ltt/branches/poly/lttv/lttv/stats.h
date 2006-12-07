/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifndef STATS_H
#define STATS_H

#include <glib.h>
#include <lttv/state.h>

/* The statistics are for a complete time interval. These structures differ
   from the system state since they relate to static components of the 
   system (all processes which existed instead of just the currently 
   existing processes). 

   The basic attributes tree to gather for several different execution modes 
   (e.g., user mode, syscall, irq), thereafter called the "events tree", 
   contains the following attributes: the number of events of each type, 
   the total number of events, the number of bytes written, the time spent 
   executing, waiting for a resource, waiting for a cpu, and possibly many 
   others. The name "facility-event_type" below is to be replaced
   by specific event types (e.g., core-schedchange, code-syscall_entry...).

   event_types/
     "facility-event_type"
   events_count
   cpu_time
	 cumulative_cpu_time
   elapsed_time
   wait_time
   bytes_written
   packets_sent
   ...

   The events for several different execution modes are joined together to 
   form the "execution modes tree". The name "execution mode" is to be replaced
   by "system call", "trap", "irq", "user mode" or "kernel thread".
   The name "submode" is to be replaced by the specific system call, trap or
   irq name. The "submode" is an empty string if none is applicable, which is
   the case for "user mode" and "kernel thread".

   An "events tree" for each "execution mode" contains the sum for all its 
   different submodes. An "events tree" in the "execution modes tree" contains
   the sum for all its different execution modes.

   mode_types/
     "execution mode"/
       submodes/
         "submode"/
           Events Tree
       events/
         Event Tree
   events/
     Events Tree

   Each trace set contains an "execution modes tree". While the traces
   come from possibly different systems, which may differ in their system
   calls..., most of the system calls will have the same name, even if their
   actual internal numeric id differs. Categories such as cpu id and process
   id are not kept since these are specific to each system. When several
   traces are taken from the same system, these categories may make sense and
   could eventually be considered.

   Each trace contains a global "execution modes tree", one for each
   cpu and process, and one for each process/cpu combination. The name
   "cpu number" stands for the cpu identifier, and "process_id-start_time"
   is a unique process identifier composed of the process id
   (unique at any given time but which may be reused over time) concatenated
   with the process start time. Each process has a "functions" tree which
	 contains each process'function address (when the information is available).
	 If not, only the 0x0 function will appear.

   modes/
     Execution Modes Tree
   cpu/
     "cpu number"/
       Execution Modes Tree
   processes/
     "process_id-start_time"/
       exec_file_name
       parent
       start_time
       end_time
       modes/
         Execution Modes Tree
       cpu/
         "cpu number"/
           Execution Modes Tree
	       functions/
			     "function address"/
					    Execution Modes Tree
	     functions/
			   "function address"/
					  Execution Modes Tree

   All the events and derived values (cpu, elapsed and wait time) are
   added during the trace analysis in the relevant 
   trace/processes/ * /cpu/ * /functions/ * /mode_types/ * /submodes/ * 
   "events tree". To achieve this efficiently, each tracefile context 
   contains a pointer to the current relevant "events tree" and "event_types" 
   tree within it.

   Once all the events are processed, the total number of events is computed
   within each
   trace/processes/ * /cpu/ * /functions/ * /mode_types/ * /submodes/ *.
   Then, the "events tree" are summed for all submodes within each mode type 
   and for all mode types within a processes/ * /cpu/ * /functions/ *
   "execution modes tree".
   
	 Then, the "execution modes trees" for all functions within a
	 trace/processes/ * /cpu for all cpu within a process, for all processes,
	 and for all traces are computed. Separately, the "execution modes tree" for
	 each function (over all cpus) for all processes, and for all traces are
	 summed in the trace/processes/ * /functions/ * subtree.
	 
   Finally, the "execution modes trees" for all cpu within a process,
   for all processes, and for all traces are computed. Separately,
   the "execution modes tree" for each cpu but for all processes within a
   trace are summed in the trace / cpu / * subtrees.

 */


/* The various statistics branch names are GQuarks. They are pre-computed for
   easy and efficient access */

#define LTTV_PRIO_STATS_BEFORE_STATE LTTV_PRIO_STATE-5
#define LTTV_PRIO_STATS_AFTER_STATE LTTV_PRIO_STATE+5

 
extern GQuark
  LTTV_STATS_PROCESS_UNKNOWN,
  LTTV_STATS_PROCESSES,
  LTTV_STATS_CPU,
  LTTV_STATS_MODE_TYPES,
  LTTV_STATS_SUBMODES,
  LTTV_STATS_FUNCTIONS,
  LTTV_STATS_EVENT_TYPES,
  LTTV_STATS_CPU_TIME,
  LTTV_STATS_CUMULATIVE_CPU_TIME,
  LTTV_STATS_ELAPSED_TIME,
  LTTV_STATS_EVENTS,
  LTTV_STATS_EVENTS_COUNT,
  LTTV_STATS_BEFORE_HOOKS,
  LTTV_STATS_AFTER_HOOKS;


typedef struct _LttvTracesetStats LttvTracesetStats;
typedef struct _LttvTracesetStatsClass LttvTracesetStatsClass;

typedef struct _LttvTraceStats LttvTraceStats;
typedef struct _LttvTraceStatsClass LttvTraceStatsClass;

typedef struct _LttvTracefileStats LttvTracefileStats;
typedef struct _LttvTracefileStatsClass LttvTracefileStatsClass;



// Hook wrapper. call_data is a trace context.
gboolean lttv_stats_hook_add_event_hooks(void *hook_data, void *call_data);
void lttv_stats_add_event_hooks(LttvTracesetStats *self);

// Hook wrapper. call_data is a trace context.
gboolean lttv_stats_hook_remove_event_hooks(void *hook_data, void *call_data);
void lttv_stats_remove_event_hooks(LttvTracesetStats *self);

gboolean lttv_stats_sum_traceset_hook(void *hook_data, void *call_data);
void lttv_stats_sum_traceset(LttvTracesetStats *self, LttTime current_time);

void lttv_stats_sum_trace(LttvTraceStats *self, LttvAttribute *ts_stats,
  LttTime current_time);

/* Reset all statistics containers */
void lttv_stats_reset(LttvTracesetStats *self);


/* The LttvTracesetStats, LttvTraceStats and LttvTracefileStats types
   inherit from the corresponding State objects defined in state.h.. */

#define LTTV_TRACESET_STATS_TYPE  (lttv_traceset_stats_get_type ())
#define LTTV_TRACESET_STATS(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACESET_STATS_TYPE, LttvTracesetStats))
#define LTTV_TRACESET_STATS_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACESET_STATS_TYPE, LttvTracesetStatsClass))
#define LTTV_IS_TRACESET_STATS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACESET_STATS_TYPE))
#define LTTV_IS_TRACESET_STATS_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACESET_STATS_TYPE))
#define LTTV_TRACESET_STATS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACESET_STATS_TYPE, LttvTracesetStatsClass))

struct _LttvTracesetStats {
  LttvTracesetState parent;

  LttvAttribute *stats;
};

struct _LttvTracesetStatsClass {
  LttvTracesetStateClass parent;
};

GType lttv_traceset_stats_get_type (void);


#define LTTV_TRACE_STATS_TYPE  (lttv_trace_stats_get_type ())
#define LTTV_TRACE_STATS(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACE_STATS_TYPE, LttvTraceStats))
#define LTTV_TRACE_STATS_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACE_STATS_TYPE, LttvTraceStatsClass))
#define LTTV_IS_TRACE_STATS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACE_STATS_TYPE))
#define LTTV_IS_TRACE_STATS_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACE_STATS_TYPE))
#define LTTV_TRACE_STATS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACE_STATS_TYPE, LttvTraceStatsClass))

struct _LttvTraceStats {
  LttvTraceState parent;

  LttvAttribute *stats;
};

struct _LttvTraceStatsClass {
  LttvTraceStateClass parent;
};

GType lttv_trace_stats_get_type (void);


#define LTTV_TRACEFILE_STATS_TYPE  (lttv_tracefile_stats_get_type ())
#define LTTV_TRACEFILE_STATS(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TRACEFILE_STATS_TYPE, LttvTracefileStats))
#define LTTV_TRACEFILE_STATS_CLASS(vtable)  (G_TYPE_CHECK_CLASS_CAST ((vtable), LTTV_TRACEFILE_STATS_TYPE, LttvTracefileStatsClass))
#define LTTV_IS_TRACEFILE_STATS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TRACEFILE_STATS_TYPE))
#define LTTV_IS_TRACEFILE_STATS_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), LTTV_TRACEFILE_STATS_TYPE))
#define LTTV_TRACEFILE_STATS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), LTTV_TRACEFILE_STATS_TYPE, LttvTracefileStatsClass))

struct _LttvTracefileStats {
  LttvTracefileState parent;

  LttvAttribute *stats;
  LttvAttribute *current_events_tree;
  LttvAttribute *current_event_types_tree;
};

struct _LttvTracefileStatsClass {
  LttvTracefileStateClass parent;
};

GType lttv_tracefile_stats_get_type (void);

struct sum_traceset_closure {
  LttvTracesetStats *tss;
  LttTime current_time;
};


#endif // STATS_H
