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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <inttypes.h>
#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <ltt/trace.h>
#include <ltt/event.h>

#define __UNUSED__ __attribute__((__unused__))

static LttvTraceset *traceset;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *before_tracefile,
  *after_tracefile,
  //*before_event,
  //*after_event,
  *event_hook,
  *main_hooks;

static char *a_trace;

static char *a_dump_tracefiles;

static char *a_save_sample;

static int
  a_sample_interval,
  a_sample_number,
  a_seek_number,
  a_save_interval;

static gboolean
  a_trace_event,
  a_save_state_copy,
  a_test1,
  a_test2,
  a_test3,
  a_test4,
  a_test5,
  a_test6,
  a_test7,
  a_test8,
  a_test9,
  a_test10,
  a_test_all;

static GQuark QUARK_BLOCK_START,
              QUARK_BLOCK_END;

LttEventPosition *a_event_position;

typedef struct _save_state {
  guint count;
  FILE *fp;
  guint interval;
  guint position;
  guint size;
  LttTime *write_time;
  guint version;
} SaveState;


static void lttv_trace_option(void __UNUSED__ *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) {
    g_critical("cannot open trace %s", a_trace);
  } else {
    lttv_traceset_add(traceset, lttv_trace_new(trace));
  }
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

  unsigned int i;

  //lttv_traceset_context_add_hooks(&ts->parent,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_process_traceset_begin(&ts->parent,
                              before_traceset,
                              before_trace,
                              before_tracefile,
                              event_hook,
                              NULL);

  for(i = 0 ; i < lttv_traceset_number(traceset) ; i++) {
    ((LttvTraceState *)(ts->parent.traces[i]))->save_interval =a_save_interval;
  }

  t0 = get_time();
  lttv_state_traceset_seek_time_closest(ts, start);
  //lttv_process_traceset(&ts->parent, end, G_MAXULONG);
  lttv_process_traceset_middle(&ts->parent,
                               end,
                               G_MAXULONG,
                               NULL);
  t1 = get_time();

  //lttv_traceset_context_remove_hooks(&ts->parent,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_process_traceset_end(&ts->parent,
                            after_traceset,
                            after_trace,
                            after_tracefile,
                            event_hook,
                            NULL);

  return t1 - t0;
}


gboolean trace_event(void __UNUSED__ *hook_data, void *call_data)
{
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  guint nb_block, offset;

  guint64 tsc;

  LttTracefile *tf;
  LttEvent *e = ltt_tracefile_get_event(tfs->parent.tf);
  ltt_event_position(e, a_event_position);
  ltt_event_position_get(a_event_position, &tf, &nb_block, &offset, &tsc);
  fprintf(stderr, "Event %s %lu.%09lu [%u 0x%x tsc %" PRIu64 "]\n",
      g_quark_to_string(marker_get_info_from_id(tf->mdata,
      			ltt_event_id(e))->name),
      tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec,
      nb_block, offset, tsc);
  return FALSE;
}

static LttTime count_previous_time = { 0, 0 };

gboolean count_event(void *hook_data, void __UNUSED__ *call_data)
{
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttTracefile *tracefile = tfs->parent.tf;
  guint nb_block, offset;
  LttTracefile *tf_pos;
  guint64 tsc;
  LttEvent * event = ltt_tracefile_get_event(tracefile);
  LttTime time;
  guint *pcount = (guint *)hook_data;

  (*pcount)++;
  
  time = ltt_event_time(event);
  ltt_event_position(event, a_event_position);
  ltt_event_position_get(a_event_position, &tf_pos, &nb_block, &offset, &tsc);
  
  if(ltt_time_compare(time, count_previous_time) < 0) {
    g_warning("Time decreasing trace %s tracefile %s cpu %u position %u/0x%x",
  g_quark_to_string(ltt_trace_name(ltt_tracefile_get_trace(tracefile))),
  g_quark_to_string(ltt_tracefile_name(tracefile)), 
  tfs->cpu, nb_block, offset);
    g_warning("last time %lu.%lu vs current %lu.%lu",
  count_previous_time.tv_sec, count_previous_time.tv_nsec,
  time.tv_sec, time.tv_nsec);
  } 
  count_previous_time = time;



  return FALSE;
}


gboolean save_state_copy_event(void *hook_data, void *call_data)
{
  SaveState __UNUSED__ *save_state = (SaveState *)hook_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfs->parent.t_context;

  LttEvent *e = ltt_tracefile_get_event(tfs->parent.tf);

  GString *filename;

  FILE *fp;

  if(ts->nb_event == 0 && 
      marker_get_info_from_id(tfs->parent.tf->mdata, e->event_id)->name
                            == QUARK_BLOCK_START) {
    if(a_save_sample != NULL) {
      filename = g_string_new("");
      g_string_printf(filename, "%s.copy.%lu.%09lu.xml", a_save_sample, 
          tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
      fp = fopen(filename->str, "w");
      if(fp == NULL) g_error("Cannot open %s", filename->str);
      g_string_free(filename, TRUE);
      lttv_state_write(ts, tfs->parent.timestamp, fp);
      fclose(fp);
    } //else lttv_state_write(ts, tfs->parent.timestamp, save_state->fp);
  }
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
    } //else lttv_state_write(ts, tfs->parent.timestamp, save_state->fp);

    save_state->write_time[save_state->position] = tfs->parent.timestamp;
    save_state->position++;
  }
  return FALSE;
}


static void sanitize_name(gchar *name)
{
  while(*name != '\0') {
    if(*name == '/') *name = '_';
    name++;
  }
  
}


static void compute_tracefile(LttTracefile *tracefile, void *hook_data)
{
  GString *filename;
  guint nb_equal, nb_block, offset;
  guint64 tsc;
  FILE *fp;
  LttTime time, previous_time;
  LttEvent *event = ltt_tracefile_get_event(tracefile);
  //LttEventType *event_type;
  struct marker_info *minfo;
  int err;
  gchar mod_name[PATH_MAX];

  /* start_count is always initialized in this function _if_ there is always
   * a block_start before a block_end.
   */
  //long long unsigned cycle_count, start_count=0, delta_cycle;


  filename = g_string_new("");
  strcpy(mod_name, g_quark_to_string(ltt_tracefile_name(tracefile)));
  
  sanitize_name(mod_name);
  
  g_warning("test %s test", g_quark_to_string(ltt_tracefile_name(tracefile)));
  g_string_printf(filename, "%s.%s.%u.trace", a_dump_tracefiles,
      mod_name, ltt_tracefile_cpu(tracefile));
  fp = fopen(filename->str, "w");
  if(fp == NULL) g_error("Cannot open %s", filename->str);
  g_string_free(filename, TRUE);
  err = ltt_tracefile_seek_time(tracefile, ltt_time_zero);
  if(err) goto close;
  
  previous_time = ltt_time_zero;
  nb_equal = 0;

  do {
    LttTracefile *tf_pos;
    //event_type = ltt_event_eventtype(event);
    minfo = marker_get_info_from_id(tracefile->mdata,
    					ltt_event_id(event));
    time = ltt_event_time(event);
    ltt_event_position(event, a_event_position);
    ltt_event_position_get(a_event_position, &tf_pos, &nb_block, &offset, &tsc);
    //fprintf(fp,"%s.%s: %llu %lu.%09lu position %u/%u\n", 
    fprintf(fp, "%s: %" PRIu64 " %lu.%09lu position %u/%u, tracefile %s\n",
    	g_quark_to_string(minfo->name),
        tsc, (unsigned long)time.tv_sec, 
        (unsigned long)time.tv_nsec, 
        nb_block, offset,
        g_quark_to_string(ltt_tracefile_name(tracefile)));

    if(ltt_time_compare(time, previous_time) < 0) {
      g_warning("Time decreasing trace %s tracefile %s cpu %u position %u/0x%x",
    g_quark_to_string(ltt_trace_name(ltt_tracefile_get_trace(tracefile))),
    g_quark_to_string(ltt_tracefile_name(tracefile)), 
    ltt_tracefile_cpu(tracefile), nb_block, offset);
      g_warning("last time %lu.%lu vs current %lu.%lu",
    previous_time.tv_sec, previous_time.tv_nsec,
    time.tv_sec, time.tv_nsec);
    }

#if 0 //FIXME
    if(ltt_eventtype_name(event_type) == QUARK_BLOCK_START) {
      start_count = cycle_count;
      start_time = time;
    }
    else if(ltt_eventtype_name(event_type) == QUARK_BLOCK_END) {
      delta_cycle = cycle_count - start_count;
      end_nsec_sec = (long long unsigned)time.tv_sec * (long long unsigned)1000000000;
      end_nsec_nsec = time.tv_nsec;
      end_nsec = end_nsec_sec + end_nsec_nsec;
      start_nsec = (long long unsigned)start_time.tv_sec * (long long unsigned)1000000000 + (long long unsigned)start_time.tv_nsec;
      delta_nsec = end_nsec - start_nsec;
      cycle_per_nsec = (double)delta_cycle / (double)delta_nsec;
      nsec_per_cycle = (double)delta_nsec / (double)delta_cycle;
      added_nsec = (double)delta_cycle * nsec_per_cycle;
      interpolated_nsec = start_nsec + added_nsec;
      added_nsec2 = (double)delta_cycle / cycle_per_nsec;
      interpolated_nsec2 = start_nsec + added_nsec2;

      fprintf(fp,"Time: start_count %llu, end_count %llu, delta_cycle %llu, start_nsec %llu, end_nsec_sec %llu, end_nsec_nsec %llu, end_nsec %llu, delta_nsec %llu, cycle_per_nsec %.25f, nsec_per_cycle %.25f, added_nsec %llu, added_nsec2 %llu, interpolated_nsec %llu, interpolated_nsec2 %llu\n", start_count, cycle_count, delta_cycle, start_nsec, end_nsec_sec, end_nsec_nsec, end_nsec, delta_nsec, cycle_per_nsec, nsec_per_cycle, added_nsec, added_nsec2, interpolated_nsec, interpolated_nsec2);
    }
    else {
#endif //0
      if(ltt_time_compare(time, previous_time) == 0)
      	nb_equal++;
      else if(nb_equal > 0) {
        g_warning("Consecutive %d events with time %lu.%09lu",
                   nb_equal + 1, previous_time.tv_sec, previous_time.tv_nsec);
        nb_equal = 0;
      }
      previous_time = time;
    //}
  } while((!ltt_tracefile_read(tracefile)));

close:
  fclose(fp);
}

static gboolean process_traceset(void __UNUSED__ *hook_data, 
                                 void __UNUSED__ *call_data)
{
  GString *filename;
  LttvTracesetStats *tscs;

  LttvTracesetState *ts;

  LttvTracesetContext *tc;

  FILE *fp;

  double t;

  //guint count, nb_control, nb_tracefile, nb_block, nb_event;
  //guint i, j, count, nb_control, nb_tracefile, nb_block, nb_event, nb_equal;
  guint i, j, count;

  LttTrace *trace;

  LttTime max_time = { G_MAXULONG, G_MAXULONG };

  a_event_position = ltt_event_position_new();

  GData **tracefiles_groups;

  struct compute_tracefile_group_args args;

  args.func = compute_tracefile;
  args.func_args = NULL;
  
  if(a_dump_tracefiles != NULL) {
    for(i = 0 ; i < lttv_traceset_number(traceset) ; i++) {
      trace = lttv_trace(lttv_traceset_get(traceset, i));
      tracefiles_groups = ltt_trace_get_tracefiles_groups(trace);

      g_datalist_foreach(tracefiles_groups, 
                            (GDataForeachFunc)compute_tracefile_group,
                            &args);
      
    }
  }

  tscs = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  ts = &tscs->parent;
  tc = &tscs->parent.parent;

  lttv_context_init(tc, traceset);

  /* For each case compute and print the elapsed time.
     The first case is simply to run through all events with a
     simple counter. */

  if(a_test1 || a_test_all) {
    count = 0;
    lttv_hooks_add(event_hook, count_event, &count, LTTV_PRIO_DEFAULT);
    t = run_one_test(ts, ltt_time_zero, max_time);
    lttv_hooks_remove_data(event_hook, count_event, &count);
    g_message(
        "Processing trace while counting events (%u events in %g seconds)",
	count, t);
  }

  /* Run through all events computing the state. */

  if(a_test2 || a_test_all) {
    lttv_state_add_event_hooks(ts);
    t = run_one_test(ts, ltt_time_zero, max_time);
    lttv_state_remove_event_hooks(ts);
    g_message("Processing trace while updating state (%g seconds)", t);
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
      lttv_hooks_add(event_hook, save_state_event, &save_state,
                        LTTV_PRIO_DEFAULT);
      t = run_one_test(ts, ltt_time_zero, max_time);
      lttv_state_remove_event_hooks(ts);
      lttv_hooks_remove_data(event_hook, save_state_event, &save_state);
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
    t = run_one_test(ts, ltt_time_zero, max_time);
    lttv_stats_remove_event_hooks(tscs);
    g_message("Processing trace while counting stats (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing stats");
      g_mem_profile();
    }

    lttv_stats_sum_traceset(tscs, ltt_time_infinite);

    if(lttv_profile_memory) {
      g_message("Memory summary after summing stats");
      g_mem_profile();
    }

    lttv_context_fini(tc);
    lttv_context_init(tc, traceset);

    if(lttv_profile_memory) {
      g_message("Memory summary after cleaning up the stats");
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
    t = run_one_test(ts, ltt_time_zero, max_time);
    lttv_state_remove_event_hooks(ts);
    lttv_stats_remove_event_hooks(tscs);
    g_message(
        "Processing trace while counting state and stats (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing and state and stats");
      g_mem_profile();
    }

    lttv_context_fini(tc);
    lttv_context_init(tc, traceset);

    if(lttv_profile_memory) {
      g_message("Memory summary after cleaning up the stats");
      g_mem_profile();
    }
  }

  /* Run through all events computing and saving the state. */

  if(a_trace_event) lttv_hooks_add(event_hook, trace_event, NULL,
                                      LTTV_PRIO_DEFAULT);

  if(a_test6 || a_test_all) {
    if(lttv_profile_memory) {
      g_message("Memory summary before computing and saving state");
      g_mem_profile();
    }

    lttv_state_add_event_hooks(ts);
    lttv_state_save_add_event_hooks(ts);
    if(a_save_state_copy)
        lttv_hooks_add(event_hook, save_state_copy_event, &save_state,
                          LTTV_PRIO_DEFAULT);
    t = run_one_test(ts, ltt_time_zero, max_time);
    lttv_state_remove_event_hooks(ts);
    lttv_state_save_remove_event_hooks(ts);
    if(a_save_state_copy)
        lttv_hooks_remove_data(event_hook,save_state_copy_event, &save_state);

    g_message("Processing trace while updating/saving state (%g seconds)", t);

    if(lttv_profile_memory) {
      g_message("Memory summary after computing/saving state");
      g_mem_profile();
    }
  }

  /* Seek a few times to each saved position */

  if((a_test7 && a_test3) || a_test_all) {
    g_assert(a_seek_number >= 0);
    for(i = 0 ; i < (guint)a_seek_number ; i++) {
      gint reverse_j; /* just to make sure j is unsigned */
      for(reverse_j = save_state.position - 1 ; reverse_j >= 0 ; reverse_j--) {
        j = (guint)reverse_j;
        lttv_state_add_event_hooks(ts);
        t = run_one_test(ts, save_state.write_time[j], 
            save_state.write_time[j]);
        lttv_state_remove_event_hooks(ts);
        g_message("Seeking to %lu.%lu (%g seconds)", 
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
        //else lttv_state_write((LttvTraceState *)tc->traces[0], 
        //    save_state.write_time[j], save_state.fp);
      }
    }
  }

  /* Seek at specified interval, using states computed in 6, making
   * sure that there is no more than the number of events between
   * state save interval to read before getting there.
   */

  if((a_test8 && a_test6) || a_test_all) {
    g_message("Running test 8 : check save interval");
    LttTime time = tc->time_span.start_time;
    LttTime interval;
    interval.tv_sec = 0;
    interval.tv_nsec = 175674987;
    guint count;

    while(ltt_time_compare(time, tc->time_span.end_time) < 0) {
      //g_message("Seeking at time %u.%u", time.tv_sec, time.tv_nsec);
      lttv_process_traceset_seek_time(&ts->parent, ltt_time_zero);
      lttv_state_traceset_seek_time_closest(ts, time);
      /* We add no hook to the traceset, not necessary */
      count = lttv_process_traceset_middle(&ts->parent,
          time, G_MAXUINT, NULL);
      g_info("Number of events to jump over : %u", count);
      
      if(count > LTTV_STATE_SAVE_INTERVAL)
        g_warning("Oops! Save interval is %u and it took %u events to seek to a time %lu.%lu supposed to be closer from the last saved state.",
            LTTV_STATE_SAVE_INTERVAL, count, time.tv_sec, time.tv_nsec);
      time = ltt_time_add(time, interval);
    }

  }

  if(a_test9 || a_test_all) {
    double t0, t1;
    /* Run seek_forward and seek_backward test */
    guint count;
    LttvTracesetContext *tsc = &ts->parent;
    LttvTracesetContextPosition *saved_pos = 
      lttv_traceset_context_position_new(tsc);
    g_message("Running test 9 : seek_forward and seek_backward");
    lttv_process_traceset_seek_time(tsc, ltt_time_zero);

    count = lttv_process_traceset_seek_n_forward(tsc, 500, NULL, NULL, NULL, NULL, NULL, NULL);
    g_assert(count == 500);
    lttv_traceset_context_position_save(tsc, saved_pos);
    t0 = get_time();
    count = lttv_process_traceset_seek_n_forward(tsc, 150000, NULL, NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek forward 150000 events in %g seconds", t1 - t0);
    g_assert(count == 150000);
    t0 = get_time();
    count = lttv_process_traceset_seek_n_backward(tsc, 150000,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek backward 150000 events in %g seconds", t1 - t0);
    g_assert(count == 150000);
    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos)) {
      g_warning("Problem with seek_n ! Positions differ. (1)");
    }
    
    lttv_process_traceset_seek_n_forward(tsc, 500, NULL, NULL, NULL, NULL, NULL, NULL);
    lttv_traceset_context_position_save(tsc, saved_pos);
    lttv_process_traceset_seek_n_forward(tsc, 15000, NULL, NULL, NULL, NULL, NULL, NULL);
    lttv_process_traceset_seek_n_backward(tsc, 15005,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    lttv_process_traceset_seek_n_forward(tsc, 5, NULL, NULL, NULL, NULL, NULL, NULL);
    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos)) {
      g_warning("Problem with seek_n ! Positions differ. (2)");
    }
    
    lttv_process_traceset_seek_time(tsc, ltt_time_infinite);
    
    count = lttv_process_traceset_seek_n_forward(tsc, 15000, NULL, NULL, NULL, NULL, NULL, NULL);
    if(count > 0)
      g_warning("Problem with seek_n ! Forward at end of traceset.");
    
    lttv_process_traceset_seek_time(tsc, ltt_time_infinite);

    lttv_traceset_context_position_save(tsc, saved_pos);
    t0 = get_time();
    lttv_process_traceset_seek_n_backward(tsc, 300,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek backward 300 events in %g seconds", t1 - t0);
    count = lttv_process_traceset_seek_n_forward(tsc, 299, NULL, NULL, NULL, NULL, NULL, NULL);
    count = lttv_process_traceset_seek_n_forward(tsc, 1, NULL, NULL, NULL, NULL, NULL, NULL);

    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos)) {
      g_warning("Problem with seek_n ! Positions differ. (4)");
    }
    
    lttv_traceset_context_position_save(tsc, saved_pos);
    t0 = get_time();
    lttv_process_traceset_seek_n_backward(tsc, 10,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek backward 10 events in %g seconds", t1 - t0);
    t0 = get_time();
    count = lttv_process_traceset_seek_n_forward(tsc, 10, NULL, NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek forward 10 events in %g seconds", t1 - t0);

   
    /* try a volountary error */
    lttv_process_traceset_seek_time(tsc, ltt_time_infinite);

    lttv_traceset_context_position_save(tsc, saved_pos);
    lttv_process_traceset_seek_n_backward(tsc, 301,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    count = lttv_process_traceset_seek_n_forward(tsc, 299, NULL, NULL, NULL, NULL, NULL, NULL);
    count = lttv_process_traceset_seek_n_forward(tsc, 1, NULL, NULL, NULL, NULL, NULL, NULL);

    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos) == 0) {
      g_warning("Problem with seek_n ! Positions _should_ differ. (5)");
    }

    /* Try a seek by closest time : Hint : try this one with and without states
     * computed. */
    lttv_process_traceset_seek_time(tsc, ltt_time_zero);
    count = lttv_process_traceset_seek_n_forward(tsc, 200000, NULL, NULL, NULL, NULL, NULL, NULL);
    lttv_traceset_context_position_save(tsc, saved_pos);
    t0 = get_time();
    lttv_process_traceset_seek_n_backward(tsc, 100301,
        seek_back_default_offset,
        (seek_time_fct)lttv_state_traceset_seek_time_closest, NULL,
        NULL, NULL, NULL, NULL, NULL);
    t1 = get_time();
    g_message("Seek backward 100301 events (with seek closest) in %g seconds",
                t1 - t0);
    count = lttv_process_traceset_seek_n_forward(tsc, 100301, NULL, NULL, NULL, NULL, NULL, NULL);
    
    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos)) {
      g_warning("Problem with seek_n with state seek time! Positions differ. (6)");
    }

    lttv_traceset_context_position_destroy(saved_pos);
  }
  
  if(a_test10 || a_test_all) {
    g_message("Running test 10 : check seek traceset context position");
    LttvTracesetContext *tsc = &ts->parent;
    LttvTracesetContextPosition *saved_pos = 
      lttv_traceset_context_position_new(tsc);

    lttv_process_traceset_seek_time(tsc, ltt_time_zero);
    lttv_process_traceset_seek_n_forward(tsc, 200000, NULL, NULL, NULL, NULL, NULL, NULL);
    lttv_traceset_context_position_save(tsc, saved_pos);
    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos) != 0)
      g_critical("Error in seek position. (1)");

    lttv_process_traceset_seek_time(tsc, ltt_time_infinite);
    lttv_process_traceset_seek_n_backward(tsc, 500,
        seek_back_default_offset, lttv_process_traceset_seek_time, NULL,
        NULL, NULL, NULL, NULL, NULL);
    lttv_traceset_context_position_save(tsc, saved_pos);

    if(lttv_traceset_context_ctx_pos_compare(tsc, saved_pos) != 0)
      g_critical("Error in seek position. (2)");
    
    lttv_traceset_context_position_destroy(saved_pos);
  }

  if(a_trace_event) lttv_hooks_remove_data(event_hook, trace_event, NULL);

  g_free(save_state.write_time);
  g_free(a_event_position);
  lttv_context_fini(tc);
  g_object_unref(tscs);

  if(lttv_profile_memory) {
    g_message("Memory summary at the end of batchtest");
    g_mem_profile();
  }

  g_info("BatchTest end process traceset");
  return 0;
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init batchtest.c");

  /* Init GQuarks */
  QUARK_BLOCK_START = g_quark_from_string("block_start");
  QUARK_BLOCK_END = g_quark_from_string("block_end");

  
  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  a_trace_event = FALSE;

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

  a_save_state_copy = FALSE;
  lttv_option_add("save-state-copy", 'S', "Write the state saved for seeking", 
      "", LTTV_OPT_NONE, &a_save_state_copy, NULL, NULL);

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

  a_seek_number = 200;
  lttv_option_add("seek-number", 'K', 
      "Number of seek", 
      "number", 
      LTTV_OPT_INT, &a_seek_number, NULL, NULL);

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

  a_test8 = FALSE;
  lttv_option_add("test8", '8', "Test seeking to positions using saved states computed at 6 : check if number of events fits", 
      "", LTTV_OPT_NONE, &a_test8, NULL, NULL);
  
  a_test9 = FALSE;
  lttv_option_add("test9", '9', "Test seeking backward/forward positions", 
      "", LTTV_OPT_NONE, &a_test9, NULL, NULL);

  a_test10 = FALSE;
  lttv_option_add("test10", ' ', "Test seeking traceset by position", 
      "", LTTV_OPT_NONE, &a_test10, NULL, NULL);



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
  //before_event = lttv_hooks_new();
  //after_event = lttv_hooks_new();
  event_hook = lttv_hooks_new();
  

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
  //g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
  //    LTTV_POINTER, &value));
  //*(value.v_pointer) = before_event;
  //g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
  //    LTTV_POINTER, &value));
  //*(value.v_pointer) = after_event;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value));
  *(value.v_pointer) = event_hook;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  g_assert((main_hooks = *(value.v_pointer)) != NULL);
  lttv_hooks_add(main_hooks, process_traceset, NULL, LTTV_PRIO_DEFAULT);
}


static void destroy()
{
  guint i, nb;

  LttvTrace *trace;

  g_info("Destroy batchAnalysis.c");

  lttv_option_remove("trace");
  lttv_option_remove("dump-tracefiles");
  lttv_option_remove("save-sample");
  lttv_option_remove("save-state-copy");
  lttv_option_remove("sample-interval");
  lttv_option_remove("sample-number");
  lttv_option_remove("seek-number");
  lttv_option_remove("save-interval");
  lttv_option_remove("test1");
  lttv_option_remove("test2");
  lttv_option_remove("test3");
  lttv_option_remove("test4");
  lttv_option_remove("test5");
  lttv_option_remove("test6");
  lttv_option_remove("test7");
  lttv_option_remove("test8");
  lttv_option_remove("test9");
  lttv_option_remove("test10");
  lttv_option_remove("testall");

  lttv_hooks_destroy(before_traceset);
  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(before_trace);
  lttv_hooks_destroy(after_trace);
  lttv_hooks_destroy(before_tracefile);
  lttv_hooks_destroy(after_tracefile);
  //lttv_hooks_destroy(before_event);
  //lttv_hooks_destroy(after_event);
  lttv_hooks_destroy(event_hook);
  lttv_hooks_remove_data(main_hooks, process_traceset, NULL);

  nb = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb ; i++) {
    trace = lttv_traceset_get(traceset, i);
    lttv_traceset_remove(traceset,i);
    ltt_trace_close(lttv_trace(trace));
    lttv_trace_destroy(trace);
  }

  lttv_traceset_destroy(traceset); 
}


LTTV_MODULE("batchtest", "Batch processing of a trace for tests", \
    "Run through a trace calling all the registered hooks for tests", \
    init, destroy, "state", "stats", "option" )
