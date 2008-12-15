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
#include <ltt/trace.h>
#include <stdio.h>

static gboolean
  a_field_names,
  a_state,
  a_cpu_stats,
  a_process_stats,
  a_raw;

static char
  *a_file_name = NULL,
  *a_quark_file_name = NULL;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *event_hook;

static guint a_event_count = 0;

/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

static FILE *a_file;

static GString *a_string;

static gboolean write_traceset_header(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

  if(a_file_name == NULL) a_file = stdout;
  else a_file = fopen(a_file_name, "w");

  if(a_file == NULL) g_error("cannot open file %s", a_file_name);

  /* Print the trace set header */
  if(a_raw) {
  /* TODO : Write a header that will check for ILP size and endianness */
    //fputc(HDR_TRACESET, a_file);
    g_assert(lttv_traceset_number(tc->ts) == 1); /* Only one trace in traceset */
  } else  {
    fprintf(a_file,"<TRACESET NUM_TRACES=%d/>\n", 
        lttv_traceset_number(tc->ts));
  }

  return FALSE;
}


static gboolean write_traceset_footer(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;
  GQuark q;
  const gchar *string;

  if(a_raw) {

  } else {
    fprintf(a_file,"</TRACESET>\n");
  }

  if(a_file_name != NULL) fclose(a_file);

  if(a_raw) {
    if(a_quark_file_name == NULL) {
      if(a_file_name == NULL) a_file = stdout;
      else a_file = fopen(a_file_name, "a");
    } else {
      if(a_quark_file_name == NULL) a_file = stdout;
      else a_file = fopen(a_quark_file_name, "w");
    }

    if(a_file == NULL) g_error("cannot open file %s", a_quark_file_name);

    fputc(HDR_QUARKS, a_file);
    q = 1;
    do {
      string = g_quark_to_string(q);
      if(string == NULL) break;
      fputc(HDR_QUARK, a_file);
      // increment. fwrite(&q, sizeof(GQuark), 1, a_file);
      fwrite(string, sizeof(char), strlen(string)+1, a_file);
      q++;
    } while(1);
   
    if(a_quark_file_name != NULL || a_file_name != NULL) fclose(a_file);

  }

  return FALSE;
}


static gboolean write_trace_header(void *hook_data, void *call_data)
{
  LttvTraceContext *tc = (LttvTraceContext *)call_data;

  if(a_raw) {
    fputc(HDR_TRACE, a_file);
  } else {
    fprintf(a_file,"<TRACE TRACE_NUMBER=%d/>\n", 
        tc->index);
  }
  
  return FALSE;
}

static gboolean write_trace_footer(void *hook_data, void *call_data)
{
  LttvTraceContext *tc = (LttvTraceContext *)call_data;

  if(a_raw) {

  } else {
    fprintf(a_file,"</TRACE>\n");
  }

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

  if(a_raw) {
    lttv_state_write_raw(ts, tfs->parent.timestamp, a_file);
  } else {
    lttv_state_write(ts, tfs->parent.timestamp, a_file);
  }
  
  return FALSE;
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init precomputeState.c");

  a_string = g_string_new("");

  a_file_name = NULL;
  lttv_option_add("output", 'o', 
      "output file where the saved states are to be written", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, NULL, NULL);

  a_quark_file_name = NULL;
  lttv_option_add("qoutput", 'q', 
      "output file where the quarks (tuples integer, string) are to be written", 
      "file name", 
      LTTV_OPT_STRING, &a_quark_file_name, NULL, NULL);

  lttv_option_add("raw", 'r', 
      "Output in raw binary",
      "Raw binary", 
      LTTV_OPT_NONE, &a_raw, NULL, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value));
  g_assert((event_hook = *(value.v_pointer)) != NULL);
  lttv_hooks_add(event_hook, for_each_event, &a_event_count, LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  g_assert((before_trace = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_trace, write_trace_header, NULL, LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
      LTTV_POINTER, &value));
  g_assert((after_trace = *(value.v_pointer)) != NULL);
  lttv_hooks_add(after_trace, write_trace_footer, NULL, LTTV_PRIO_DEFAULT);

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
  g_info("Destroy precomputeState");

  lttv_option_remove("output");

  lttv_option_remove("qoutput");

  lttv_option_remove("raw");

  g_string_free(a_string, TRUE);

  lttv_hooks_remove_data(event_hook, for_each_event, NULL);

  lttv_hooks_remove_data(before_trace, write_trace_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_footer, NULL);
}


LTTV_MODULE("precomputeState", "Precompute states", \
	    "Precompute states in a trace, XML or binary output.", \
	    init, destroy, "stats", "batchAnalysis", "option", "print")

