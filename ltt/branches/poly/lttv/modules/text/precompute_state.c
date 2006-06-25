/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
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

/* The text dump facility needs to print headers before the trace set and
   before each trace, to print each event, and to print statistics
   after each trace. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/lttv.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <lttv/print.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <stdio.h>

static gboolean
  a_field_names,
  a_state,
  a_cpu_stats,
  a_process_stats;

static char
  *a_file_name = NULL;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *event_hook;


/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

static FILE *a_file;

static GString *a_string;

static gboolean write_traceset_header(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

  g_info("TextDump traceset header");

  if(a_file_name == NULL) a_file = stdout;
  else a_file = fopen(a_file_name, "w");

  if(a_file == NULL) g_error("cannot open file %s", a_file_name);

  /* Print the trace set header */
  fprintf(a_file,"Trace set contains %d traces\n\n", 
      lttv_traceset_number(tc->ts));

  return FALSE;
}


static gboolean write_traceset_footer(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

  g_info("TextDump traceset footer");

  fprintf(a_file,"End trace set\n\n");

  if(LTTV_IS_TRACESET_STATS(tc)) {
    lttv_stats_sum_traceset((LttvTracesetStats *)tc);
    print_stats(a_file, (LttvTracesetStats *)tc);
  }

  if(a_file_name != NULL) fclose(a_file);

  return FALSE;
}


static gboolean write_trace_header(void *hook_data, void *call_data)
{
  LttvTraceContext *tc = (LttvTraceContext *)call_data;
#if 0 //FIXME
  LttSystemDescription *system = ltt_trace_system_description(tc->t);

  fprintf(a_file,"  Trace from %s in %s\n%s\n\n", 
	  ltt_trace_system_description_node_name(system), 
	  ltt_trace_system_description_domain_name(system), 
	  ltt_trace_system_description_description(system));
#endif //0
  return FALSE;
}


static int for_each_event(void *hook_data, void *call_data)
{
  guint *event_count = (guint*)hook_data;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttEvent *e;

  LttvAttributeValue value_filter;

  /* Only save at LTTV_STATE_SAVE_INTERVAL */
  if(likely((*event_count)++ < LTTV_STATE_SAVE_INTERVAL))
    return FALSE;
  else
    *event_count = 0;

  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfc->t_context;
  LttvProcessState *process = ts->running_process[cpu];

  e = ltt_tracefile_get_event(tfc->tf);

  lttv_state_write(ts, tfs->parent.timestamp, a_file);
  
  return FALSE;
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init textDump.c");

  a_string = g_string_new("");

  a_file_name = NULL;
  lttv_option_add("output", 'o', 
      "output file where the saved states are to be written", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, NULL, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value));
  g_assert((event_hook = *(value.v_pointer)) != NULL);
  lttv_hooks_add(event_hook, write_event_content, NULL, LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  g_assert((before_trace = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_trace, write_trace_header, NULL, LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  g_assert((before_traceset = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_traceset, write_traceset_header, NULL,
      LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  g_assert((after_traceset = *(value.v_pointer)) != NULL);
  lttv_hooks_add(after_traceset, write_traceset_footer, NULL,
      LTTV_PRIO_DEFAULT);
}

static void destroy()
{
  g_info("Destroy textDump");

  lttv_option_remove("output");

  g_string_free(a_string, TRUE);

  lttv_hooks_remove_data(event_hook, for_each_event, NULL);

  lttv_hooks_remove_data(before_trace, write_trace_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_footer, NULL);
}


LTTV_MODULE("textDump", "Print events in a file", \
	    "Produce a detailed text printout of a trace", \
	    init, destroy, "stats", "batchAnalysis", "option", "print")

