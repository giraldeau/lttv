
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
#include <ltt/type.h>
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <stdio.h>


void lttv_print_field(LttEvent *e, LttField *f, GString *s,
                      gboolean field_names) {

  LttType *type;

  LttField *element;

  GQuark name;

  int nb, i;

  type = ltt_field_type(f);
  switch(ltt_type_class(type)) {
    case LTT_INT:
    case LTT_LONG:
    case LTT_SSIZE_T:
      g_string_append_printf(s, " %lld", ltt_event_get_long_int(e,f));
      break;

    case LTT_UINT:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_OFF_T:
      g_string_append_printf(s, " %llu", ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_FLOAT:
      g_string_append_printf(s, " %g", ltt_event_get_double(e,f));
      break;

    case LTT_POINTER:
      g_string_append_printf(s, " 0x%llx", ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_STRING:
      g_string_append_printf(s, " \"%s\"", ltt_event_get_string(e,f));
      break;

    case LTT_ENUM:
      g_string_append_printf(s, " %s", 
          g_quark_to_string(ltt_enum_string_get(type,
          ltt_event_get_unsigned(e,f)-1)));
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      g_string_append_printf(s, " {");
      nb = ltt_event_field_element_number(e,f);
      element = ltt_field_element(f);
      for(i = 0 ; i < nb ; i++) {
        ltt_event_field_element_select(e,f,i);
        lttv_print_field(e, element, s, field_names);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_STRUCT:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        if(field_names) {
          ltt_type_member_type(type, i, &name);
          g_string_append_printf(s, " %s = ", g_quark_to_string(name));
        }
        lttv_print_field(e, element, s, field_names);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_UNION:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        if(field_names) {
          ltt_type_member_type(type, i, &name);
          g_string_append_printf(s, " %s = ", g_quark_to_string(name));
        }
        lttv_print_field(e, element, s, field_names);
      }
      g_string_append_printf(s, " }");
      break;

  }
}


void lttv_event_to_string(LttEvent *e, GString *s,
    gboolean mandatory_fields, gboolean field_names, LttvTracefileState *tfs)
{ 
  LttFacility *facility;

  LttEventType *event_type;

  LttField *field;

  LttTime time;

  guint cpu = ltt_tracefile_num(tfs->parent.tf);
  LttvTraceState *ts = (LttvTraceState*)tfs->parent.t_context;
  LttvProcessState *process = ts->running_process[cpu];

  g_string_set_size(s,0);

  facility = ltt_event_facility(e);
  event_type = ltt_event_eventtype(e);
  field = ltt_event_field(e);

  if(mandatory_fields) {
    time = ltt_event_time(e);
    g_string_append_printf(s,"%s.%s: %ld.%09ld (%s_%u)",
        g_quark_to_string(ltt_facility_name(facility)),
        g_quark_to_string(ltt_eventtype_name(event_type)),
        (long)time.tv_sec, time.tv_nsec,
        g_quark_to_string(ltt_tracefile_name(tfs->parent.tf)),
        cpu);
    /* Print the process id and the state/interrupt type of the process */
    g_string_append_printf(s,", %u, %u,  %s", process->pid,
		    process->ppid,
		    g_quark_to_string(process->state->t));
  }

  if(field)
    lttv_print_field(e, field, s, field_names);
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

