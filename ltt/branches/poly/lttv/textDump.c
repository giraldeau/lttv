/* The text dump facility needs to print headers before the trace set and
   before each trace, to print each event, and to print statistics
   after each trace. */

#include <ltt/type.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>

void init(int argc, char **argv)
{
  lttv_attributes *a;
  lttv_hooks *before, *after;

  a = lttv_global_attributes();
  before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/before");
  after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/after");
  lttv_hooks_add(before, textDump_trace_set_before, NULL);
  lttv_hooks_add(after, textDump_trace_set_after, NULL);
}


void destroy()
{
  lttv_attributes *a;
  lttv_hooks *before, *after;

  a = lttv_global_attributes();
  before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/before");
  after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/after");
  lttv_hooks_remove(before, textDump_trace_set_before, NULL);
  lttv_hooks_remove(after, textDump_trace_set_after, NULL);
}

/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

typedef struct _trace_context {
  g_string s;
  FILE *fp;
  bool mandatory_fields;
  lttv_attributes *a;  
} trace_context;

static bool textDump_trace_set_before(void *hook_data, void *call_data)
{
  FILE *fp;
  int i, j, nb, nbtf;
  lttv_trace_set *s;
  lttv_attributes *a;
  trace_context *c;

  a = lttv_global_attributes();
  s = (lttv_trace_set *)call_data

  /* Get the file pointer */

  fp = (FILE *)lttv_attributes_get_pointer_pathname(a, "textDump/file");

  /* For each trace prepare the contexts and insert the hooks */

  nb = lttv_trace_set_number(s);
  for(i = 0 ; i < nb ; i++) {
    c = g_new(trace_context);
    a = lttv_trace_set_trace_attributes(s, i);

    if(lttv_attributes_get_pointer_pathname(a, "textDump/context") != NULL) {
      g_error("Recursive call to TextDump");
    }

    c->fp = fp;
    c->mandatory_fields = TRUE;
    c->s = g_string_new();
    c->a = a;

    lttv_attributes_set_pointer_pathname(a, "textDump/context", c);

    h = lttv_attributes_get_hooks(a, "hooks/before");
    lttv_hooks_add(h, textDump_trace_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/after");
    lttv_hooks_add(h, textDump_trace_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/tacefile/before");
    lttv_hooks_add(h, textDump_tracefile_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/tracefile/after");
    lttv_hooks_add(h, textDump_tracefile_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/event/selected");
    lttv_hooks_add(h, textDump_event, c);
  }

  /* Print the trace set header */
  fprintf(fp,"Trace set contains %d traces\n\n", nb);

  return TRUE;
}


/* Remove the hooks before and after each trace and tracefile, and for each
   event. Print trace set level statistics. */

static bool textDump_trace_set_after(void *hook_data, void *call_data)
{
  FILE *fp;
  int i, j, nb, nbtf;
  lttv_trace_set *s;
  lttv_attributes *ga, *a;
  trace_context *c;

  ga = lttv_global_attributes();
  s = (lttv_trace_set *)lttv_attributes_get_pointer_pathname(ga, 
      "trace_set/main");

  /* Get the file pointer */

  fp = (FILE *)lttv_attributes_get_pointer_pathname(ga, "textDump/file");

  /* For each trace remove the hooks */

  nb = lttv_trace_set_number(s);
  for(i = 0 ; i < nb ; i++) {
    a = lttv_trace_set_trace_attributes(s, i);
    c = (trace_context *)lttv_attributes_get_pointer_pathname(a, 
        "textDump/context");
    lttv_attributes_set_pointer_pathname(a, "textDump/context", NULL);
    g_string_free(c->s);

    h = lttv_attributes_get_hooks(a, "hooks/before");
    lttv_hooks_remove(h, textDump_trace_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/after");
    lttv_hooks_remove(h, textDump_trace_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/tacefile/before");
    lttv_hooks_remove(h, textDump_tracefile_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/tracefile/after");
    lttv_hooks_remove(h, textDump_tracefile_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/event/selected");
    lttv_hooks_remove(h, textDump_event, c);
    g_free(c);
  }

  /* Print the trace set statistics */

  fprintf(fp,"Trace set contains %d traces\n\n", nb);

  print_stats(fp, ga);

  return TRUE;
}


/* Print a trace level header */

static bool textDump_trace_before(void *hook_data, void *call_data)
{
  ltt_trace *t;
  trace_context *c;

  c = (trace_context *)hook_data;
  t = (ltt_trace *)call_data;
  fprintf(c->fp,"Start trace\n");
  return TRUE;
}


/* Print trace level statistics */

static bool textDump_trace_after(void *hook_data, void *call_data)
{
  ltt_trace *t;
  trace_context *c;

  c = (trace_context *)hook_data;
  t = (ltt_trace *)call_data;
  fprintf(c->fp,"End trace\n");
  print_stats(c->fp,c->a);
  return TRUE;
}


static bool textDump_tracefile_before(void *hook_data, void *call_data)
{
  ltt_tracefile *tf;
  trace_context *c;

  c = (trace_context *)hook_data;
  tf = (ltt_tracefile *)call_data;
  fprintf(c->fp,"Start tracefile\n");
  return TRUE;
}


static bool textDump_tracefile_after(void *hook_data, void *call_data)
{
  ltt_tracefile *tf;
  trace_context *c;

  c = (trace_context *)hook_data;
  tf = (ltt_tracefile *)call_data;
  fprintf(c->fp,"End tracefile\n");
  return TRUE;
}


/* Print the event content */

static bool textDump_event(void *hook_data, void *call_data)
{
  ltt_event *e;
  trace_context *c;

  e = (ltt_event *)call_data;
  c = (event_context *)hook_data;
  lttv_event_to_string(e,c->s,c->mandatory_fields);
  fputs(s, c->fd);
  return TRUE;
}


static void print_stats(FILE *fp, lttv_attributes *a)
{
  int i, j, k, nb, nbc;

  lttv_attributes *sa, *ra;
  lttv_attribute *reports, *content;
  lttv_key *key, *previous_key, null_key;

  null_key = lttv_key_new_pathname("");
  sa = (lttv_attributes *)lttv_attributes_get_pointer_pathname(a,"stats");
  reports = lttv_attributes_array_get(sa);
  nb= lttv_attributes_number(sa);

  for(i = 0 ; i < nb ; i++) {
    ra = (lttv_attributes *)reports[i].v.p;
    key = reports[i].key;
    g_assert(reports[i].t == LTTV_POINTER);

    /* CHECK maybe have custom handlers registered for some specific reports */

    print_key(fp,key);

    content = lttv_attributes_array_get(ra);
    nbc = lttv_attributes_number(ra);
    lttv_attribute_array_sort_lexicographic(content, nbc, NULL, 0);
    previous_key = nullKey;
    for(j = 0 ; j < nbc ; j++) {
      key = content[j].key;
      for(k = 0 ; lttv_key_index(previous_key,k) == lttv_index(key,k) ; k++)
      for(; k < lttv_key_number(key) ; k++) {
        for(l = 0 ; l < k ; l++) fprintf(fp,"  ");
        fprintf(fp, "%s", lttv_string_id_to_string(lttv_index(key,k)));
        if(k == lttv_key_number(key)) {
          switch(content[j].t) {
            case LTTV_INTEGER:
              fprintf(fp," %d\n", content[j].v.i);
              break;
            case LTTV_TIME:
              fprintf(fp," %d.%09d\n", content[j].v.t.tv_sec, 
                  content[j].v.t.tv_nsec);
              break;
            case LTTV_DOUBLE:
              fprintf(fp," %g\n", content[j].v.d);
              break;
	    case LTTV_POINTER:
              fprintf(fp," pointer\n");
              break;
          }
        }
        else fprintf(fp,"\n");
      }
    }
    lttv_attribute_array_destroy(content);
  }
  lttv_attribute_array_destroy(reports);
  lttv_key_destroy(null_key);
}


void lttv_event_to_string(ltt_event *e, lttv_string *s, bool mandatory_fields)
{
  ltt_facility *facility;
  ltt_eventtype *eventtype;
  ltt_type *type;
  ltt_field *field;
  ltt_time time;

  g_string_set_size(s,0);

  facility = lttv_event_facility(e);
  eventtype = ltt_event_eventtype(e);
  field = ltt_event_field(e);

  if(mandatory_fields) {
    time = ltt_event_time(e);
    g_string_append_printf(s,"%s.%s: %ld.%ld",ltt_facility_name(facility),
        ltt_eventtype_name(eventtype), (long)time.tv_sec, time.tv_nsec);
  }

  print_field(e,f,s);
} 

void print_field(ltt_event *e, ltt_field *f, lttv_string *s) {
  ltt_type *type;
  ltt_field *element;

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
          event_get_unsigned(e,f));
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      g_string_append_printf(s, " {");
      nb = ltt_event_field_element_number(e,f);
      element = ltt_field_element(f);
      for(i = 0 ; i < nb ; i++) {
        ltt_event_field_element_select(e,f,i);
        print_field(e,element,s);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_STRUCT:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        print_field(e,element,s);
      }
      g_string_append_printf(s, " }");
      break;
  }
}




