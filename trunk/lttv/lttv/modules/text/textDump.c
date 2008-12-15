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
#include <inttypes.h>

static gboolean
  a_noevent,
  a_no_field_names,
  a_state,
  a_cpu_stats,
  a_process_stats,
  a_path_output;

static char
  *a_file_name = NULL;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *event_hook;


static void 
print_path_tree(FILE *fp, GString *indent, LttvAttribute *tree)
{
  int i, nb, saved_length;

  LttvAttribute *subtree;

  LttvAttributeName name;

  LttvAttributeValue value;

  LttvAttributeType type;

  gboolean is_named;

  saved_length = indent->len;
  nb = lttv_attribute_get_number(tree);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(tree, i, &name, &value, &is_named);
    if(is_named) {
      g_string_sprintfa(indent, "/%s", g_quark_to_string(name));
    } else {
	    g_string_sprintfa(indent, "/%" PRIu32, (guint32) name);
    }

    switch(type) {
      case LTTV_INT:
       fprintf(fp, "%s: %d\n", indent->str, *value.v_int);
        break;
      case LTTV_UINT:
       fprintf(fp, "%s: %d\n", indent->str, *value.v_int);
        break;
      case LTTV_LONG:
       fprintf(fp, "%s: %ld\n", indent->str, *value.v_ulong);
        break;
      case LTTV_ULONG:
       fprintf(fp, "%s: %lu\n", indent->str, *value.v_ulong);
        break;
      case LTTV_FLOAT:
       fprintf(fp, "%s: %f\n", indent->str, (double) *value.v_float);
        break;
      case LTTV_DOUBLE:
        fprintf(fp, "%s: %f\n", indent->str, *value.v_double);
        break;
      case LTTV_TIME:
       fprintf(fp, "%s: %lu.%09lu\n", indent->str, value.v_time->tv_sec, value.v_time->tv_nsec);
        break;
      case LTTV_POINTER:
       fprintf(fp, "%s: POINTER\n", indent->str);
        break;
      case LTTV_STRING:
       fprintf(fp, "%s: %s\n", indent->str, *value.v_string);
        break;
      case LTTV_GOBJECT:
        if(LTTV_IS_ATTRIBUTE(*(value.v_gobject))) {
          subtree = (LttvAttribute*) *(value.v_gobject);
          print_path_tree(fp, indent, subtree);
        } else {
         fprintf(fp, "%s: GOBJECT\n", indent->str);
       }
        break;
      case LTTV_NONE:
        break;
    }
    g_string_truncate(indent, saved_length);
  }
}

static void
print_tree(FILE *fp, GString *indent, LttvAttribute *tree)
{
  int i, nb, saved_length;

  LttvAttribute *subtree;

  LttvAttributeName name;

  LttvAttributeValue value;

  LttvAttributeType type;

	gboolean is_named;

  nb = lttv_attribute_get_number(tree);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(tree, i, &name, &value, &is_named);
		if(is_named)
	    fprintf(fp, "%s%s: ", indent->str, g_quark_to_string(name));
		else
			fprintf(fp, "%s%" PRIu32 ": ", indent->str,
				(guint32) name);

    switch(type) {
      case LTTV_INT:
        fprintf(fp, "%d\n", *value.v_int);
        break;
      case LTTV_UINT:
        fprintf(fp, "%u\n", *value.v_uint);
        break;
      case LTTV_LONG:
        fprintf(fp, "%ld\n", *value.v_long);
        break;
      case LTTV_ULONG:
        fprintf(fp, "%lu\n", *value.v_ulong);
        break;
      case LTTV_FLOAT:
        fprintf(fp, "%f\n", (double)*value.v_float);
        break;
      case LTTV_DOUBLE:
        fprintf(fp, "%f\n", *value.v_double);
        break;
      case LTTV_TIME:
        fprintf(fp, "%10lu.%09lu\n", value.v_time->tv_sec, 
            value.v_time->tv_nsec);
        break;
      case LTTV_POINTER:
        fprintf(fp, "POINTER\n");
        break;
      case LTTV_STRING:
        fprintf(fp, "%s\n", *value.v_string);
        break;
      case LTTV_GOBJECT:
        if(LTTV_IS_ATTRIBUTE(*(value.v_gobject))) {
          fprintf(fp, "\n");
          subtree = (LttvAttribute *)*(value.v_gobject);
          saved_length = indent->len; 
          indent = g_string_append(indent, "  ");
          print_tree(fp, indent, subtree);
          g_string_truncate(indent, saved_length);
        }
        else fprintf(fp, "GOBJECT\n");
        break;
      case LTTV_NONE:
        break;
    }
  }
}

static void
print_stats(FILE *fp, LttvTracesetStats *tscs)
{
  int i, nb, saved_length;

  LttvTraceset *ts;

  LttvTraceStats *tcs;

  GString *indent;

  if(tscs->stats == NULL) return;
  indent = g_string_new("");
  fprintf(fp, "Traceset statistics:\n\n");
  if(a_path_output) {
    print_path_tree(fp, indent, tscs->stats);
  } else {
    print_tree(fp, indent, tscs->stats);
  }

  ts = tscs->parent.parent.ts;
  nb = lttv_traceset_number(ts);

  for(i = 0 ; i < nb ; i++) {
    tcs = (LttvTraceStats *)(LTTV_TRACESET_CONTEXT(tscs)->traces[i]);
#if 0 //FIXME
    LttSystemDescription *desc;
    desc = ltt_trace_system_description(tcs->parent.parent.t);
    LttTime start_time = ltt_trace_system_description_trace_start_time(desc);
    fprintf(fp, "Trace on system %s at time %lu.%09lu :\n", 
	    ltt_trace_system_description_node_name(desc), 
	    start_time.tv_sec,
      start_time.tv_nsec);
#endif //FIXME
    saved_length = indent->len;
    if(a_path_output) {
      g_string_sprintfa(indent, "/trace%i", i);
      print_path_tree(fp, indent, tcs->stats);
    } else {
      g_string_append(indent, "  ");
      print_tree(fp, indent, tcs->stats);
    }
    g_string_truncate(indent, saved_length);
  }
  g_string_free(indent, TRUE);
}

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
    lttv_stats_sum_traceset((LttvTracesetStats *)tc, ltt_time_infinite);
    print_stats(a_file, (LttvTracesetStats *)tc);
  }

  if(a_file_name != NULL) fclose(a_file);

  return FALSE;
}


static gboolean write_trace_header(void *hook_data, void *call_data)
{
#if 0 //FIXME
  LttvTraceContext *tc = (LttvTraceContext *)call_data;
  LttSystemDescription *system = ltt_trace_system_description(tc->t);

  fprintf(a_file,"  Trace from %s in %s\n%s\n\n", 
	  ltt_trace_system_description_node_name(system), 
	  ltt_trace_system_description_domain_name(system), 
	  ltt_trace_system_description_description(system));
#endif //0
  return FALSE;
}


static int write_event_content(void *hook_data, void *call_data)
{
  gboolean result;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttEvent *e;

  LttvAttributeValue value_filter;

  LttvFilter *filter;

  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfc->t_context;
  LttvProcessState *process = ts->running_process[cpu];

  if (a_noevent)
    return FALSE;

  e = ltt_tracefile_get_event(tfc->tf);

  result = lttv_iattribute_find_by_path(attributes, "filter/lttv_filter",
      LTTV_POINTER, &value_filter);
  g_assert(result);
  filter = (LttvFilter*)*(value_filter.v_pointer);

  /*
   * call to the filter if available
   */
  if(filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
                               tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;
  
  lttv_event_to_string(e, a_string, TRUE, !a_no_field_names, tfs);

  if(a_state) {
    g_string_append_printf(a_string, " %s ",
        g_quark_to_string(process->state->s));
  }

  g_string_append_printf(a_string,"\n");

  fputs(a_string->str, a_file);
  return FALSE;
}


static void init()
{
  gboolean result;

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init textDump.c");

  a_string = g_string_new("");

  a_file_name = NULL;
  lttv_option_add("output", 'o', 
      "output file where the text is written", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, NULL, NULL);

  a_noevent = FALSE;
  lttv_option_add("noevent", 'n', 
      "disable event printout", 
      "", 
      LTTV_OPT_NONE, &a_noevent, NULL, NULL);

  a_no_field_names = FALSE;
  lttv_option_add("field_names", 's', 
      "do not write the field names for each event", 
      "", 
      LTTV_OPT_NONE, &a_no_field_names, NULL, NULL);

  a_state = FALSE;
  lttv_option_add("process_state", 'r', 
      "write the pid and state for each event", 
      "", 
      LTTV_OPT_NONE, &a_state, NULL, NULL);

  a_cpu_stats = FALSE;
  lttv_option_add("cpu_stats", 'c', 
      "write the per cpu statistics", 
      "", 
      LTTV_OPT_NONE, &a_cpu_stats, NULL, NULL);

  a_process_stats = FALSE;
  lttv_option_add("process_stats", 'p', 
      "write the per process statistics", 
      "", 
      LTTV_OPT_NONE, &a_process_stats, NULL, NULL);

  a_path_output = FALSE;
  lttv_option_add("path_output", 'a',
      "print the process stats in path format, for easy grepping",
      "",
      LTTV_OPT_NONE, &a_path_output, NULL, NULL);

  result = lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value);
  g_assert(result);
  event_hook = *(value.v_pointer);
  g_assert(event_hook);
  lttv_hooks_add(event_hook, write_event_content, NULL, LTTV_PRIO_DEFAULT);

  result = lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value);
  g_assert(result);
  before_trace = *(value.v_pointer);
  g_assert(before_trace);
  lttv_hooks_add(before_trace, write_trace_header, NULL, LTTV_PRIO_DEFAULT);

  result = lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value);
  g_assert(result);
  before_traceset = *(value.v_pointer);
  g_assert(before_traceset);
  lttv_hooks_add(before_traceset, write_traceset_header, NULL,
      LTTV_PRIO_DEFAULT);

  result = lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value);
  g_assert(result);
  after_traceset = *(value.v_pointer);
  g_assert(after_traceset);
  lttv_hooks_add(after_traceset, write_traceset_footer, NULL,
      LTTV_PRIO_DEFAULT);
}

static void destroy()
{
  g_info("Destroy textDump");

  lttv_option_remove("noevent");

  lttv_option_remove("output");

  lttv_option_remove("field_names");

  lttv_option_remove("process_state");

  lttv_option_remove("cpu_stats");

  lttv_option_remove("process_stats");

  lttv_option_remove("path_output");

  g_string_free(a_string, TRUE);

  lttv_hooks_remove_data(event_hook, write_event_content, NULL);

  lttv_hooks_remove_data(before_trace, write_trace_header, NULL);

  lttv_hooks_remove_data(before_traceset, write_traceset_header, NULL);

  lttv_hooks_remove_data(after_traceset, write_traceset_footer, NULL);
}


LTTV_MODULE("textDump", "Print events in a file", \
	    "Produce a detailed text printout of a trace", \
	    init, destroy, "stats", "batchAnalysis", "option", "print")

