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
   all the events in the main tracefile. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <ltt/trace.h>

static LttvTraceset *traceset;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *before_tracefile,
  *after_tracefile,
  *event_hook,
  *main_hooks;

static char *a_trace;

static gboolean a_stats;

void lttv_trace_option(void *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  lttv_traceset_add(traceset, lttv_trace_new(trace));
}


static gboolean process_traceset(void *hook_data, void *call_data)
{
  LttvAttributeValue value_expression, value_filter;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  LttvTracesetStats *tscs;

  LttvTracesetState *tss;

  LttvTracesetContext *tc;

  LttTime start, end;

  g_info("BatchAnalysis begin process traceset");

  if (a_stats) {
    tscs = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    tss = &tscs->parent;
  } else {
    tss = g_object_new(LTTV_TRACESET_STATE_TYPE, NULL);
  }
  tc = &tss->parent;

  g_info("BatchAnalysis initialize context");

  lttv_context_init(tc, traceset);
  lttv_state_add_event_hooks(tc);
  if(a_stats) lttv_stats_add_event_hooks(tscs);

  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value_expression));

  g_assert(lttv_iattribute_find_by_path(attributes, "filter/lttv_filter",
      LTTV_POINTER, &value_filter));

  *(value_filter.v_pointer) = lttv_filter_new();
  //g_debug("Filter string: %s",((GString*)*(value_expression.v_pointer))->str);

  lttv_filter_append_expression(*(value_filter.v_pointer),((GString*)*(value_expression.v_pointer))->str);
  
  //lttv_traceset_context_add_hooks(tc,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_process_traceset_begin(tc,
                              before_traceset,
                              before_trace,
                              before_tracefile,
                              event_hook,
                              NULL);

  start.tv_sec = 0;
  start.tv_nsec = 0;
  end.tv_sec = G_MAXULONG;
  end.tv_nsec = G_MAXULONG;

  g_info("BatchAnalysis process traceset");

  lttv_process_traceset_seek_time(tc, start);
  lttv_process_traceset_middle(tc,
                               end,
                               G_MAXULONG,
                               NULL);


  //lttv_traceset_context_remove_hooks(tc,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_process_traceset_end(tc,
                            after_traceset,
                            after_trace,
                            after_tracefile,
                            event_hook,
                            NULL);

  g_info("BatchAnalysis destroy context");

  lttv_filter_destroy(*(value_filter.v_pointer));
  lttv_state_remove_event_hooks(tss);
  if(a_stats) lttv_stats_remove_event_hooks(tscs);
  lttv_context_fini(tc);
  if (a_stats)
    g_object_unref(tscs);
  else
    g_object_unref(tss);

  g_info("BatchAnalysis end process traceset");
  return FALSE;
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init batchAnalysis.c");

  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  a_stats = FALSE;
  lttv_option_add("stats", 's', 
      "write the traceset and trace statistics", 
      "", 
      LTTV_OPT_NONE, &a_stats, NULL, NULL);


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
  lttv_option_remove("stats");

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
    ltt_trace_close(lttv_trace(trace));
    /* This will be done by lttv_traceset_destroy */
    //lttv_trace_destroy(trace);
  }

  lttv_traceset_destroy(traceset); 
}

LTTV_MODULE("batchAnalysis", "Batch processing of a trace", \
    "Run through a trace calling all the registered hooks", \
    init, destroy, "state", "stats", "option","textFilter")
