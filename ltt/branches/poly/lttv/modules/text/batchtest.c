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

/* This module inserts a hook in the program main loop. This hook processes 
   all the events in the main tracefile while testing the speed and
   functionality of the state and stats computations. */


#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/processTrace.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <ltt/trace.h>
#include <ltt/event.h>

static LttvTraceset *traceset;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *before_tracefile,
  *after_tracefile,
  *before_event,
  *after_event,
  *main_hooks;

static char *a_trace;

static char *a_dump_tracefiles;

static char *a_save_sample;

static int
  a_sample_interval,
  a_sample_number,
  a_save_interval;

static gboolean
  a_test1,
  a_test2,
  a_test3,
  a_test4,
  a_test5,
  a_test6,
  a_test7,
  a_test_all;

typedef struct _save_state {
  guint count;
  FILE *fp;
  guint interval;
  guint position;
  guint size;
  LttTime *write_time;
  guint version;
} SaveState;


static void lttv_trace_option(void *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  lttv_traceset_add(traceset, lttv_trace_new(trace));
}

static double get_time() 
{
  GTimeVal gt;

  g_get_current_time(&gt);
  return gt.tv_sec + (double)gt.tv_usec / (double)1000000.0;
}

static double run_one_test(LttvTracesetState *ts, LttTime start, LttTime end)
{
  double t0, t1;

  lttv_traceset_context_add_hooks(&ts->parent,
  before_traceset, after_traceset, NULL, before_trace, after_trace,
  NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);

  t0 = get_time();
  lttv_state_traceset_seek_time_closest(ts, start);
  lttv_process_traceset(&ts->parent, end, G_MAXULONG);
  t1 = get_time();

  lttv_traceset_context_remove_hooks(&ts->parent,
  before_traceset, after_traceset, NULL, before_trace, after_trace,
  NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);

  return t1 - t0;
}


gboolean count_event(void *hook_data, void *call_data)
{
  guint *pcount = (guint *)hook_data;

  (*pcount)++;
  return FALSE;
}


gboolean save_state_event(void *hook_data, void *call_data)
{
  SaveState *save_state = (SaveState *)hook_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfs->parent.t_context;

  GString *filename;

  FILE *fp;

  (save_state->count)++;
  if(save_state->count % save_state->interval == 0 && 
     save_state->position < save_state->size) {
    if(a_save_sample != NULL) {
      filename = g_string_new("");
      g_string_printf(filename, "%s.%u.xml.%u", a_save_sample, 
          save_state->position, save_state->version);
      fp = fopen(filename->str, "w");
      if(fp == NULL) g_error("Cannot open %s", filename->str);
      g_string_free(filename, TRUE);
      lttv_state_write(ts, tfs->parent.timestamp, fp);
      fclose(fp);
    } else lttv_state_write(ts, tfs->parent.timestamp, save_state->fp);

    save_state->write_time[save_state->position] = tfs->parent.timestamp;
    save_state->position++;
  }
  return FALSE;
}


static gboolean process_traceset(void *hook_data, void *call_data)
{
  LttvTracesetStats *tscs;

  LttvTracesetState *ts;

  LttvTracesetContext *tc;

  GString *filename;

  FILE *fp;

  double t;

  guint i, j, count, nb_control, nb_tracefile, nb_block, nb_event, nb_equal;

  LttTrace *trace;

  LttTracefile *tracefile, *tf;

  LttEvent *event;

  LttFacility *facility;

  LttType *type;

  LttEventType *event_type;

  LttTime time, previous_time;

  LttEventPosition *event_position;

  LttTime zero_time = ltt_time_zero;

  LttTime max_time = { G_MAXULONG, G_MAXULONG };

  if(a_dump_tracefiles != NULL) {
    event_position = ltt_event_position_new();
    for(i = 0 ; i < lttv_traceset_number(traceset) ; i++) {
      trace = lttv_trace(lttv_traceset_get(traceset, i));
      nb_control = ltt_trace_control_tracefile_number(trace);
      nb_tracefile = nb_control + ltt_trace_per_cpu_tracefile_number(trace);
      for(j = 0 ; j < nb_tracefile ; j++) {
        if(j < nb_control) {
          tracefile = ltt_trace_control_tracefile_get(trace,j);
        }
        else {
          tracefile = ltt_trace_per_cpu_tracefile_get(trace,j - nb_control);
        }

        filename = g_string_new("");
        g_string_printf(filename, "%s.%u.%u.trace", a_dump_tracefiles, i, j);
        fp = fopen(filename->str, "w");
        if(fp == NULL) g_error("Cannot open %s", filename->str);
        g_string_free(filename, TRUE);
        ltt_tracefile_seek_time(tracefile, zero_time);
        previous_time = zero_time;
        nb_equal = 0;
        while((event = ltt_tracefile_read(tracefile)) != NULL) {
          facility = ltt_event_facility(event);
          event_type = ltt_event_eventtype(event);
          time = ltt_event_time(event);
          ltt_event_position(event, event_position);
          ltt_event_position_get(event_position, &nb_block, &nb_event, &tf);
          fprintf(fp,"%s.%s: %lu.%09lu position %u/%u\n", 
              ltt_facility_name(facility), ltt_eventtype_name(event_type), 
		  (long)time.tv_sec, time.tv_nsec, nb_block, nb_event);

          if(ltt_time_compare(time, previous_time) == 0) nb_equal++;
	  else if(nb_equal > 0) {
            g_warning("Consecutive %d events with time %lu.%lu",
	        nb_equal + 1, previous_time.tv_sec, previous_time.tv_nsec);
            nb_equal = 0;
	  }

          if(ltt_time_compare(time, previous_time) < 0) {
            g_warning("Time decreasing trace %d tracefile %d position %u/%u",
		      i, j, nb_block, nb_event);
          }
          previous_time = time;
        }
        fclose(fp);
      }
    }
    g_free(event_position);
  }

  tscs = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  ts = &tscs->parent;
  tc = &tscs->parent.parent;

  lttv_context_init(tc, traceset);
  for(i = 0 ; i < lttv_traceset_number(traceset) ; i++) {
    ((LttvTraceState *)(tc->traces[i]))->save_interval = a_save_interval;
  }

  /* For each case compute and print the elapsed time.
     The first case is simply to run through all events with a
     simple counter. */

  if(a_test1 || a_test_all) {
    count = 0;
    lttv_hooks_add(after_event, count_event, &count);
    t = run_one_test(ts, zero_time, max_time);
    lttv_hooks_remove_data(after_event, count_event, &count);
    g_warning(
        "Processing trace while counting events (%u events in %g seconds)",
	count, t);
  }

  /* Run through all events computing the state. */

  if(a_test2 || a_test_all) {
    lttv_state_add_event_hooks(ts);
    t = run_one_test(ts, zero_time, max_time);
    lttv_state_remove_event_hooks(ts);
    g_warning("Processing trace while updating state (%g seconds)", t);
  }

  /* Run through all events computing the state and writing it out 
     periodically. */

  SaveState save_state;

  save_state.interval = a_sample_interval;
  save_state.size = a_sample_number;
  save_state.fp = stderr;
  save_state.write_time = g_new(LttTime, a_sample_number);


  if(a_test3 || a_test_all) {
    for(i = 0 ; i < 2 ; i++) {
      save_state.count = 0;
      save_state.position = 0;
      save_state.version = i;
      lttv_state_add_event_hooks(ts);
      lttv_hooks_add(after_event, save_state_event, &save_state);
      t = run_one_test(ts, zero_time, max_time);
      lttv_state_remove_event_hooks(ts);
      lttv_hooks_remove_data(after_event, save_state_event, &save_state);
      g_warning("Processing while updating/writing state (%g seconds)", t);
    }
  }

  /* Run through all events computing the stats. */

  if(a_test4 || a_test_all) {
    if(lttv_profile_memory) {
      g_message("Memory summary before computing stats");
      g_mem_profile();
    }

    lttv_stats_add_event_hooks(tscs);
    t = run_one_test(ts, zero_time, max_time);
    lttv_stats_remove_event_hooks(tscs);
    g_warning("Processing trace while counting stats (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing stats");
      g_mem_profile();
    }
  }

  /* Run through all events computing the state and stats. */

  if(a_test5 || a_test_all) {
    if(lttv_profile_memory) {
      g_message("Memory summary before computing state and stats");
      g_mem_profile();
    }

    lttv_state_add_event_hooks(ts);
    lttv_stats_add_event_hooks(tscs);
    t = run_one_test(ts, zero_time, max_time);
    lttv_state_remove_event_hooks(ts);
    lttv_stats_remove_event_hooks(tscs);
    g_warning(
        "Processing trace while counting state and stats (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing and state and stats");
      g_mem_profile();
    }
  }

  /* Run through all events computing and saving the state. */

  if(a_test6 || a_test_all) {
    if(lttv_profile_memory) {
      g_message("Memory summary before computing and saving state");
      g_mem_profile();
    }

    lttv_state_add_event_hooks(ts);
    lttv_state_save_add_event_hooks(ts);
    t = run_one_test(ts, zero_time, max_time);
    lttv_state_remove_event_hooks(ts);
    lttv_state_save_remove_event_hooks(ts);
    g_warning("Processing trace while updating/saving state (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing/saving state");
      g_mem_profile();
    }
  }

  /* Seek a few times to each saved position */

  if((a_test7 && a_test3) || a_test_all) {
    int i, j;

    for(i = 0 ; i < 2 ; i++) {
      for(j = save_state.position - 1 ; j >= 0 ; j--) {
        lttv_state_add_event_hooks(ts);
        t = run_one_test(ts, save_state.write_time[j], 
            save_state.write_time[j]);
        lttv_state_remove_event_hooks(ts);
        g_warning("Seeking to %lu.%lu (%g seconds)", 
            save_state.write_time[j].tv_sec, save_state.write_time[j].tv_nsec,
            t);

        if(a_save_sample != NULL) {
          filename = g_string_new("");
          g_string_printf(filename, "%s.%d.xml.bak%d", a_save_sample, j, i);
          fp = fopen(filename->str, "w");
          if(fp == NULL) g_error("Cannot open %s", filename->str);
          g_string_free(filename, TRUE);
          lttv_state_write((LttvTraceState *)tc->traces[0], 
              save_state.write_time[j], fp);
          fclose(fp);
        }
        else lttv_state_write((LttvTraceState *)tc->traces[0], 
            save_state.write_time[j], save_state.fp);
      }
    }
  }

  g_free(save_state.write_time);
  lttv_context_fini(tc);
  g_object_unref(tscs);

  g_info("BatchTest end process traceset");
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init batchtest.c");

  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  a_dump_tracefiles = NULL;
  lttv_option_add("dump-tracefiles", 'D', 
      "Write event by event the content of tracefiles", 
      "basename for the files where to dump events", 
      LTTV_OPT_STRING, &a_dump_tracefiles, NULL, NULL);

  a_save_sample = NULL;
  lttv_option_add("save-sample", 's', 
      "Save state samples to multiple files", 
      "basename for the files containing the state samples", 
      LTTV_OPT_STRING, &a_save_sample, NULL, NULL);

  a_save_interval = 100000;
  lttv_option_add("save-interval", 'i', 
      "Interval between saving state", 
      "number of events before a block start triggers saving state", 
      LTTV_OPT_INT, &a_save_interval, NULL, NULL);

  a_sample_interval = 100000;
  lttv_option_add("sample-interval", 'S', 
      "Interval between sampling state", 
      "number of events before sampling and writing state", 
      LTTV_OPT_INT, &a_sample_interval, NULL, NULL);

  a_sample_number = 20;
  lttv_option_add("sample-number", 'N', 
      "Number of state samples", 
      "maximum number", 
      LTTV_OPT_INT, &a_sample_number, NULL, NULL);

  a_test1 = FALSE;
  lttv_option_add("test1", '1', "Test just counting events", "", 
      LTTV_OPT_NONE, &a_test1, NULL, NULL);

  a_test2 = FALSE;
  lttv_option_add("test2", '2', "Test computing the state", "", 
      LTTV_OPT_NONE, &a_test2, NULL, NULL);

  a_test3 = FALSE;
  lttv_option_add("test3", '3', "Test computing the state, writing out a few",
      "", LTTV_OPT_NONE, &a_test3, NULL, NULL);

  a_test4 = FALSE;
  lttv_option_add("test4", '4', "Test computing the stats", "", 
      LTTV_OPT_NONE, &a_test4, NULL, NULL);

  a_test5 = FALSE;
  lttv_option_add("test5", '5', "Test computing the state and stats", "", 
      LTTV_OPT_NONE, &a_test5, NULL, NULL);

  a_test6 = FALSE;
  lttv_option_add("test6", '6', "Test computing and saving the state", "", 
      LTTV_OPT_NONE, &a_test6, NULL, NULL);

  a_test7 = FALSE;
  lttv_option_add("test7", '7', "Test seeking to positions written out in 3", 
      "", LTTV_OPT_NONE, &a_test7, NULL, NULL);

  a_test_all = FALSE;
  lttv_option_add("testall", 'a', "Run all tests ", "", 
      LTTV_OPT_NONE, &a_test_all, NULL, NULL);

  traceset = lttv_traceset_new();

  before_traceset = lttv_hooks_new();
  after_traceset = lttv_hooks_new();
  before_trace = lttv_hooks_new();
  after_trace = lttv_hooks_new();
  before_tracefile = lttv_hooks_new();
  after_tracefile = lttv_hooks_new();
  before_event = lttv_hooks_new();
  after_event = lttv_hooks_new();

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_event;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_event;

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  g_assert((main_hooks = *(value.v_pointer)) != NULL);
  lttv_hooks_add(main_hooks, process_traceset, NULL);
}


static void destroy()
{
  guint i, nb;

  LttvTrace *trace;

  g_info("Destroy batchAnalysis.c");

  lttv_option_remove("trace");
  lttv_option_remove("dump-tracefiles");
  lttv_option_remove("save-sample");
  lttv_option_remove("sample-interval");
  lttv_option_remove("sample-number");
  lttv_option_remove("save-interval");
  lttv_option_remove("test1");
  lttv_option_remove("test2");
  lttv_option_remove("test3");
  lttv_option_remove("test4");
  lttv_option_remove("test5");
  lttv_option_remove("test6");
  lttv_option_remove("test7");
  lttv_option_remove("testall");

  lttv_hooks_destroy(before_traceset);
  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(before_trace);
  lttv_hooks_destroy(after_trace);
  lttv_hooks_destroy(before_tracefile);
  lttv_hooks_destroy(after_tracefile);
  lttv_hooks_destroy(before_event);
  lttv_hooks_destroy(after_event);
  lttv_hooks_remove_data(main_hooks, process_traceset, NULL);

  nb = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb ; i++) {
    trace = lttv_traceset_get(traceset, i);
    ltt_trace_close(lttv_trace(trace));
    lttv_trace_destroy(trace);
  }

  lttv_traceset_destroy(traceset); 
}


LTTV_MODULE("batchtest", "Batch processing of a trace for tests", \
    "Run through a trace calling all the registered hooks for tests", \
    init, destroy, "state", "stats", "option" )
