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

/* print.c
 *
 * Event printing routines.
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
#include <ctype.h>
#include <ltt/ltt-private.h>
#include <string.h>
#include <inttypes.h>

static inline void print_enum_events(LttEvent *e, struct marker_field *f,
                      guint64 value, GString *s, LttvTracefileState *tfs)
{
  LttTracefile *tf = tfs->parent.tf;
  struct marker_info *info = marker_get_info_from_id(tf->mdata, e->event_id);
  LttvTraceState *ts = (LttvTraceState*)(tfs->parent.t_context);
  
  if (tf->name == LTT_CHANNEL_KERNEL) {
    if (info->name == LTT_EVENT_SYSCALL_ENTRY
        && f->name == LTT_FIELD_SYSCALL_ID) {
      g_string_append_printf(s, " [%s]",
        g_quark_to_string(ts->syscall_names[value]));
    } else if ((info->name == LTT_EVENT_SOFT_IRQ_ENTRY
                || info->name == LTT_EVENT_SOFT_IRQ_EXIT
                || info->name == LTT_EVENT_SOFT_IRQ_RAISE)
               && f->name == LTT_FIELD_SOFT_IRQ_ID) {
      g_string_append_printf(s, " [%s]",
        g_quark_to_string(ts->soft_irq_names[value]));
    } else if (info->name == LTT_EVENT_KPROBE
               && f->name == LTT_FIELD_IP) {
      GQuark symbol = g_hash_table_lookup(ts->kprobe_hash,
                                          (gconstpointer)value);
      if (symbol)
        g_string_append_printf(s, " [%s]", g_quark_to_string(symbol));
    }
  }
}

void lttv_print_field(LttEvent *e, struct marker_field *f, GString *s,
                      gboolean field_names, LttvTracefileState *tfs)
{
  GQuark name;
  guint64 value;

  //int nb, i;

  switch(f->type) {
    case LTT_TYPE_SIGNED_INT:
      if(field_names) {
        name = f->name;
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      value = ltt_event_get_long_int(e,f);
      //g_string_append_printf(s, "%lld", value);
      g_string_append_printf(s, f->fmt->str, value);
      //g_string_append_printf(s, type->fmt, ltt_event_get_long_int(e,f));
      print_enum_events(e, f, value, s, tfs);
      break;

    case LTT_TYPE_UNSIGNED_INT:
      if(field_names) {
        name = f->name;
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      value = ltt_event_get_long_unsigned(e,f);
      //g_string_append_printf(s, "%llu", value);
      g_string_append_printf(s, f->fmt->str, value);
      print_enum_events(e, f, value, s, tfs);
      //g_string_append_printf(s, type->fmt, ltt_event_get_long_unsigned(e,f));
      break;
    
#if 0
    case LTT_CHAR:
    case LTT_UCHAR:
      {
        unsigned car = ltt_event_get_unsigned(e,f);
        if(field_names) {
          name = ltt_field_name(f);
          if(name)
            g_string_append_printf(s, "%s = ", g_quark_to_string(name));
        }
        if(isprint(car)) {
          if(field_names) {
            name = ltt_field_name(f);
            if(name)
              g_string_append_printf(s, "%s = ", g_quark_to_string(name));
          }
          //g_string_append_printf(s, "%c", car);
	  g_string_append_printf(s, type->fmt, car);
        } else {
          g_string_append_printf(s, "\\%x", car);
        }
      }
      break;
    case LTT_FLOAT:
     if(field_names) {
        name = ltt_field_name(f);
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
     //g_string_append_printf(s, "%g", ltt_event_get_double(e,f));
      g_string_append_printf(s, type->fmt, ltt_event_get_double(e,f));
      break;
#endif

    case LTT_TYPE_POINTER:
      if(field_names) {
        name = f->name;
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      g_string_append_printf(s, "0x%" PRIx64, ltt_event_get_long_unsigned(e,f));
      //g_string_append_printf(s, type->fmt, ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_TYPE_STRING:
      if(field_names) {
        name = f->name;
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      g_string_append_printf(s, "\"%s\"", ltt_event_get_string(e,f));
      break;

#if 0
    case LTT_ENUM:
      {
        GQuark value = ltt_enum_string_get(type, ltt_event_get_unsigned(e,f));
        if(field_names) {
          name = ltt_field_name(f);
          if(name)
            g_string_append_printf(s, "%s = ", g_quark_to_string(name));
        }
        if(value)
          g_string_append_printf(s, "%s", g_quark_to_string(value));
        else
          g_string_append_printf(s, "%lld", ltt_event_get_long_int(e,f));
      }
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      if(field_names) {
        name = ltt_field_name(f);
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      // g_string_append_printf(s, "{ ");
      //Insert header
      g_string_append_printf(s, type->header);//tested, works fine.


      nb = ltt_event_field_element_number(e,f);
      for(i = 0 ; i < nb ; i++) {
        LttField *child = ltt_event_field_element_select(e,f,i);
        lttv_print_field(e, child, s, field_names, i);
	if(i<nb-1)
	  g_string_append_printf(s,type->separator);
      }
      //g_string_append_printf(s, " }");
      //Insert footer
      g_string_append_printf(s, type->footer);//tested, works fine.
      break;

    case LTT_STRUCT:
      if(field_names) {
        name = ltt_field_name(f);
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      //      g_string_append_printf(s, "{ ");
      //Insert header
      g_string_append_printf(s, type->header);

      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        LttField *element;
        element = ltt_field_member(f,i);
        lttv_print_field(e, element, s, field_names, i);
	if(i < nb-1)
	  g_string_append_printf(s,type->separator);
      }
      //g_string_append_printf(s, " }");
      //Insert footer
      g_string_append_printf(s, type->footer);
      break;

    case LTT_UNION:
      if(field_names) {
        name = ltt_field_name(f);
        if(name)
          g_string_append_printf(s, "%s = ", g_quark_to_string(name));
      }
      //      g_string_append_printf(s, "{ ");
      g_string_append_printf(s, type->header);

      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        LttField *element;
        element = ltt_field_member(f,i);
        lttv_print_field(e, element, s, field_names, i);
	if(i<nb-1)
	  g_string_append_printf(s, type->separator);
      }
      //      g_string_append_printf(s, " }");
      g_string_append_printf(s, type->footer);
      break;
#endif
    case LTT_TYPE_COMPACT:
      g_error("compact type printing not implemented");
      break;
    case LTT_TYPE_NONE:
      break;
  }
}

void lttv_event_to_string(LttEvent *e, GString *s,
    gboolean mandatory_fields, gboolean field_names, LttvTracefileState *tfs)
{ 
  struct marker_field *field;
  struct marker_info *info;

  LttTime time;

  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];

  s = g_string_set_size(s,0);

  info = marker_get_info_from_id(tfs->parent.tf->mdata, e->event_id);

  if(mandatory_fields) {
    time = ltt_event_time(e);
    g_string_append_printf(s,"%s.%s: %ld.%09ld (%s/%s_%u)",
	g_quark_to_string(ltt_tracefile_name(tfs->parent.tf)),
        g_quark_to_string(info->name),
        (long)time.tv_sec, time.tv_nsec,
	g_quark_to_string(
		ltt_trace_name(ltt_tracefile_get_trace(tfs->parent.tf))),
        g_quark_to_string(ltt_tracefile_name(tfs->parent.tf)),
        cpu);
    /* Print the process id and the state/interrupt type of the process */
    g_string_append_printf(s,", %u, %u, %s, %s, %u, 0x%" PRIx64", %s",
			   process->pid,
			   process->tgid,
			   g_quark_to_string(process->name),
			   g_quark_to_string(process->brand),
			   process->ppid, process->current_function,
			   g_quark_to_string(process->state->t));
  }
  
  if(marker_get_num_fields(info) == 0) return;
  g_string_append_printf(s, " ");
  g_string_append_printf(s, "{ ");
  for (field = marker_get_field(info, 0); 
                field != marker_get_field(info, marker_get_num_fields(info));
                field++) {
    if(field != marker_get_field(info, 0))
      g_string_append_printf(s, ", ");
    lttv_print_field(e, field, s, field_names, tfs);
  }
  g_string_append_printf(s, " }");
} 

static void init()
{
}

static void destroy()
{
}

LTTV_MODULE("print", "Print events", \
      "Produce a detailed text printout of events", \
      init, destroy)

