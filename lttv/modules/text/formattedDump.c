/*
 * This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
 *               2011 Vincent Attard <vinc.attard@gmail.com>
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

/*
 * Formatted dump plugin prints a formatted output of each events in a trace.
 * The output format is defined as a parameter. It provides a default format
 * easy to read, a "strace-like" format and the original textDump format for
 * backward compatibility.
 */

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
#include <string.h>
#include <stdlib.h>

static gboolean a_no_field_names;
static gboolean a_state;
static gboolean a_text;
static gboolean a_strace;

static char *a_file_name;
static char *a_format;

static LttvHooks *before_traceset;
static LttvHooks *event_hook;

static const char default_format[] =
		"channel:%c event:%e timestamp:%t elapsed:%l cpu:%u pid:%d ppid:%i "
		"tgpid:%g process:%p brand:%b state:%a payload:{ %m }";
static const char textDump_format[] =
		"%c.%e: %s.%n (%r/%c_%u), %d, %g, %p, %b, %i, %y, %a { %m }";
static const char strace_format[] = "%e(%m) %s.%n";

static FILE *a_file;

static GString *a_string;

static gboolean open_output_file(void *hook_data, void *call_data)
{
	g_info("Open the output file");
	if (a_file_name == NULL) {
		a_file = stdout;
	} else {
		a_file = fopen(a_file_name, "w");
	}
	if (a_file == NULL) {
		g_error("cannot open file %s", a_file_name);
	}
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
	LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
	LttvProcessState *process = ts->running_process[cpu];

	e = ltt_tracefile_get_event(tfc->tf);

	result = lttv_iattribute_find_by_path(attributes, "filter/lttv_filter",
			LTTV_POINTER, &value_filter);
	g_assert(result);
	filter = (LttvFilter *)*(value_filter.v_pointer);

	/* call to the filter if available */
	if (filter->head != NULL) {
		if (!lttv_filter_tree_parse(filter->head, e, tfc->tf,
				tfc->t_context->t, tfc, NULL, NULL)) {
			return FALSE;
		}
	}

	lttv_event_to_string(e, a_string, TRUE, !a_no_field_names, tfs);

	if (a_state) {
		g_string_append_printf(a_string, "%s ",
				g_quark_to_string(process->state->s));
	}

	g_string_append_printf(a_string, "\n");

	fputs(a_string->str, a_file);
	return FALSE;
}

void lttv_event_to_string(LttEvent *e, GString *string_buffer, gboolean mandatory_fields,
		gboolean field_names, LttvTracefileState *tfs)
{
	struct marker_field *field;
	struct marker_info *info;
	LttTime time;

	static LttTime time_prev = {0, 0};
	/*
	 * TODO:
	 * Added this static value into state.c and reset each time you do a
	 * seek for using it in the GUI.
	 */
	LttTime elapse;
	const char *fmt;
	int i;
	int len;
	guint cpu = tfs->cpu;
	LttvTraceState *ts = (LttvTraceState *)tfs->parent.t_context;
	LttvProcessState *process = ts->running_process[cpu];

	info = marker_get_info_from_id(tfs->parent.tf->mdata, e->event_id);
	if (mandatory_fields) {
		time = ltt_event_time(e);
		/* Calculate elapsed time between current and previous event */
		if (time_prev.tv_sec == 0 && time_prev.tv_nsec == 0) {
			time_prev = ltt_event_time(e);
			elapse.tv_sec = 0;
			elapse.tv_nsec = 0;
			/*
			 * TODO:
			 * Keep in mind that you should add the ability to
			 * restore the previous event time to state.c if you
			 * want to reuse this code into the GUI.
			 */
		} else {
			elapse = ltt_time_sub(time, time_prev);
			time_prev = time;
		}
	}
	if (a_text) {
		/* textDump format (used with -T command option) */
		fmt = textDump_format;
	} else if (a_strace) {
		/* strace-like format (used with -S command option) */
		fmt = strace_format;
	} else if (!a_format) {
		/* Default format (used if no option) */
		fmt = default_format;
	} else {
		/*
		 * formattedDump format
		 * (used with -F command option following by the desired format)
		 */
		fmt = a_format;
	}

	g_string_set_size(string_buffer, 0);
	/*
	 * Switch case:
	 * all '%-' are replaced by the desired value in 'string_buffer'
	 */
	len = strlen(fmt);
	for (i = 0; i < len; i++) {
		if (fmt[i] == '%') {
			switch (fmt[++i]) {
			case 't':
				g_string_append_printf(string_buffer,
						"%ld:%02ld:%02ld.%09ld",
						time.tv_sec/3600,
						(time.tv_sec%3600)/60,
						time.tv_sec%60,
						time.tv_nsec);
				break;
			case 'c':
				g_string_append(string_buffer,
						g_quark_to_string(ltt_tracefile_name(tfs->parent.tf)));
				break;
			case 'e':
				g_string_append(string_buffer,
						g_quark_to_string(info->name));
				break;
			case 'd':
				g_string_append_printf(string_buffer, "%u",
						process->pid);
				break;
			case 's':
				g_string_append_printf(string_buffer, "%ld",
						time.tv_sec);
				break;
			case 'n':
				g_string_append_printf(string_buffer, "%ld",
						time.tv_nsec);
				break;
			case 'i':
				g_string_append_printf(string_buffer, "%u",
						process->ppid);
				break;
			case 'g':
				g_string_append_printf(string_buffer, "%u",
						process->tgid);
				break;
			case 'p':
				g_string_append(string_buffer,
						g_quark_to_string(process->name));
				break;
			case 'b':
				g_string_append_printf(string_buffer, "%u",
						process->brand);
				break;
			case 'u':
				g_string_append_printf(string_buffer, "%u", cpu);
				break;
			case 'l':
				g_string_append_printf(string_buffer,
						"%ld.%09ld",
						elapse.tv_sec, elapse.tv_nsec);
				break;
			case 'a':
				g_string_append(string_buffer,
						g_quark_to_string(process->state->t));
				break;
			case 'm':
				{
				/*
				 * Get and print markers and tracepoints fields
				 * into 'string_buffer'
				 */
				if (marker_get_num_fields(info) == 0)
					break;
					for (field = marker_get_field(info, 0);
							field != marker_get_field(info, marker_get_num_fields(info));
							field++) {
						if (field != marker_get_field(info, 0)) {
							g_string_append(string_buffer, ", ");
						}

						lttv_print_field(e, field, string_buffer, field_names, tfs);
					}
				}
				break;
			case 'r':
				g_string_append(string_buffer, g_quark_to_string(
						ltt_trace_name(ltt_tracefile_get_trace(tfs->parent.tf))));
				break;
			case '%':
				g_string_append_c(string_buffer, '%');
				break;
			case 'y':
				g_string_append_printf(string_buffer,
						"0x%" PRIx64,
						process->current_function);
				break;
			}
		} else {
			/* Copy every character if different of '%' */
			g_string_append_c(string_buffer, fmt[i]);
		}
	}
}

static void init()
{
	gboolean result;

	LttvAttributeValue value;

	LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

	g_info("Init formattedDump.c");

	a_string = g_string_new("");

	a_file_name = NULL;
	lttv_option_add("output", 'o',
			"output file where the text is written",
			"file name",
			LTTV_OPT_STRING, &a_file_name, NULL, NULL);

	a_text = FALSE;
	lttv_option_add("text", 'T',
			"output the textDump format",
			"",
			LTTV_OPT_NONE, &a_text, NULL, NULL);

	a_strace = FALSE;
	lttv_option_add("strace", 'S',
			"output a \"strace-like\" format",
			"",
			LTTV_OPT_NONE, &a_strace, NULL, NULL);

	a_format = NULL;
	lttv_option_add("format", 'F',
			"output the desired format\n"
			"		FORMAT controls the output. "
			"Interpreted sequences are:\n"
			"\n"
			"		%c   channel name\n"
			"		%p   process name\n"
			"		%e   event name\n"
			"		%r   path to trace\n"
			"		%t   timestamp  (e.g., 2:08:54.025684145)\n"
			"		%s   seconds\n"
			"		%n   nanoseconds\n"
			"		%l   elapsed time with the previous event\n"
			"		%d   pid\n"
			"		%i   ppid\n"
			"		%g   tgid\n"
			"		%u   cpu\n"
			"		%b   brand\n"
			"		%a   state\n"
			"		%y   memory address\n"
			"		%m   markers and tracepoints fields\n",
			"format string (e.g., \"channel:%c event:%e process:%p\")",
			LTTV_OPT_STRING, &a_format, NULL, NULL);

	result = lttv_iattribute_find_by_path(attributes, "hooks/event",
			LTTV_POINTER, &value);
	g_assert(result);
	event_hook = *(value.v_pointer);
	g_assert(event_hook);
	lttv_hooks_add(event_hook, write_event_content, NULL, LTTV_PRIO_DEFAULT);

	result = lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
			LTTV_POINTER, &value);
	g_assert(result);
	before_traceset = *(value.v_pointer);
	g_assert(before_traceset);
	lttv_hooks_add(before_traceset, open_output_file, NULL,
			LTTV_PRIO_DEFAULT);

}

static void destroy()
{
	g_info("Destroy formattedDump");

	lttv_option_remove("format");

	lttv_option_remove("output");

	lttv_option_remove("text");

	lttv_option_remove("strace");

	g_string_free(a_string, TRUE);

	lttv_hooks_remove_data(event_hook, write_event_content, NULL);

	lttv_hooks_remove_data(before_traceset, open_output_file, NULL);

}


LTTV_MODULE("formattedDump", "Print events with desired format in a file",
		"Produce a detailed formatted text printout of a trace",
		init, destroy, "batchAnalysis", "option")
