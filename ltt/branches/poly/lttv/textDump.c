/* The text dump facility needs to print headers before the trace set and
   before each trace, to print each event, and to print statistics
   after each trace. */

#include <lttv/lttv.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <lttv/state.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>
#include <stdio.h>

static gboolean
  a_field_names,
  a_state;

static char
  *a_file_name;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *before_event;


void print_field(LttEvent *e, LttField *f, GString *s, gboolean field_names) {

  LttType *type;

  LttField *element;

  char *name;

  int nb, i;

  type = ltt_field_type(f);
  switch(ltt_type_class(type)) {
    case LTT_INT:
      g_string_append_printf(s, " %ld", ltt_event_get_long_int(e,f));
      break;

    case LTT_UINT:
      g_string_append_printf(s, " %lu", ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_FLOAT:
      g_string_append_printf(s, " %g", ltt_event_get_double(e,f));
      break;

    case LTT_STRING:
      g_string_append_printf(s, " \"%s\"", ltt_event_get_string(e,f));
      break;

    case LTT_ENUM:
      g_string_append_printf(s, " %s", ltt_enum_string_get(type,
          event_get_unsigned(e,f)));
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      g_string_append_printf(s, " {");
      nb = ltt_event_field_element_number(e,f);
      element = ltt_field_element(f);
      for(i = 0 ; i < nb ; i++) {
        ltt_event_field_element_select(e,f,i);
        print_field(e, element, s, field_names);
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
          g_string_append_printf(s, " %s = ", name);
        }
        print_field(e, element, s, field_names);
      }
      g_string_append_printf(s, " }");
      break;
  }
}


void lttv_event_to_string(LttEvent *e, LttTracefile *tf, GString *s,
    gboolean mandatory_fields, gboolean field_names)
{
  LttFacility *facility;

  LttEventType *event_type;

  LttType *type;

  LttField *field;

  LttTime time;

  g_string_set_size(s,0);

  facility = ltt_event_facility(e);
  event_type = ltt_event_eventtype(e);
  field = ltt_event_field(e);

  if(mandatory_fields) {
    time = ltt_event_time(e);
    g_string_append_printf(s,"%s.%s: %ld.%ld (%s)",ltt_facility_name(facility),
        ltt_eventtype_name(event_type), (long)time.tv_sec, time.tv_nsec,
        ltt_tracefile_name(tf));
    /* Print the process id and the state/interrupt type of the process */
  }

  print_field(e, field, s, field_names);
} 


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
  fprintf(a_file,"Trace set contains %d traces\n\n", 
      lttv_traceset_number(tc->ts));

  return FALSE;
}


static gboolean write_traceset_footer(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

  fprintf(a_file,"End trace set\n\n");

  if(a_file_name != NULL) fclose(a_file);

  return FALSE;
}


static gboolean write_trace_header(void *hook_data, void *call_data)
{
  LttvTraceContext *tc = (LttvTraceContext *)call_data;

  LttSystemDescription *system = ltt_trace_system_description(tc->t);

  fprintf(a_file,"  Trace from %s in %s\n%s\n\n", system->node_name, 
      system->domain_name, system->description);
  return FALSE;
}


static int write_event_content(void *hook_data, void *call_data)
{
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttEvent *e;

  e = tfc->e;

  lttv_event_to_string(e, tfc->tf, a_string, TRUE, a_field_names);

  if(a_state) {
    g_string_append_printf(a_string, " %s",
        g_quark_to_string(tfs->process->state->s));
  }

  fputs(a_string->str, a_file);
  return FALSE;
}


void init(int argc, char **argv)
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  a_file_name = NULL;
  lttv_option_add("output", 'o', 
      "output file where the text is written", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, NULL, NULL);

  a_field_names = FALSE;
  lttv_option_add("field_names", 'l', 
      "write the field names for each event", 
      "", 
      LTTV_OPT_NONE, &a_field_names, NULL, NULL);

  a_state = FALSE;
  lttv_option_add("process_state", 's', 
      "write the pid and state for each event", 
      "", 
      LTTV_OPT_NONE, &a_state, NULL, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
      LTTV_POINTER, &value));
  g_assert((before_event = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_event, write_event_content, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  g_assert((before_trace = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_trace, write_trace_header, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  g_assert((before_traceset = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_traceset, write_traceset_header, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  g_assert((after_traceset = *(value.v_pointer)) != NULL);
  lttv_hooks_add(after_traceset, write_traceset_footer, NULL);
}


void destroy()
{
  lttv_option_remove("output");

  lttv_option_remove("field_names");

  lttv_option_remove("process_state");

  lttv_hooks_remove_data(before_event, write_event_content, NULL);

  lttv_hooks_remove_data(before_trace, write_trace_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_header, NULL);

  lttv_hooks_remove_data(before_trace, write_traceset_footer, NULL);
}




