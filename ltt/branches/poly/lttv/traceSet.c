/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

struct _lttv_trace_set {
  GPtrArray *traces;
  GPtrArray *attributes;
  lttv_attributes *a;
};


lttv_trace_set *lttv_trace_set_new() 
{
  lttv_trace_set s;

  s = g_new(lttv_trace_set, 1);
  s->traces = g_ptr_array_new();
  s->attributes = g_ptr_array_new();
  s->a = lttv_attributes_new();
}

lttv_trace_set *lttv_trace_set_destroy(lttv_trace_set *s) 
{
  int i, nb;

  for(i = 0 ; i < s->attributes->len ; i++) {
    lttv_attributes_destroy((lttv_attributes *)s->attributes->pdata[i]);
  }
  g_ptr_array_free(s->attributes);
  g_ptr_array_free(s->traces);
  lttv_attributes_destroy(s->a);
  return g_free(s);
}

void lttv_trace_set_add(lttv_trace_set *s, lttv_trace *t) 
{
  g_ptr_array_add(s,t);
  g_ptr_array_add(s,lttv_attributes_new());
}

unsigned lttv_trace_set_number(lttv_trace_set *s) 
{
  return s->traces.len;
}


lttv_trace *lttv_trace_set_get(lttv_trace_set *s, unsigned i) 
{
  g_assert(s->traces->len <= i);
  return s->traces.pdata[i];
}


lttv_trace *lttv_trace_set_remove(lttv_trace_set *s, unsigned i) 
{
  return g_ptr_array_remove_index(s->traces,i);
  lttv_attributes_destroy(g_ptr_array_remove_index(s->attributes,i));
}


/* A set of attributes is attached to each trace set, trace and tracefile
   to store user defined data as needed. */

lttv_attributes *lttv_trace_set_attributes(lttv_trace_set *s) 
{
  return s->a;
}

lttv_attributes *lttv_trace_set_trace_attributes(lttv_trace_set *s, unsigned i)
{
  return t->a;
}

lttv_attributes *lttv_tracefile_attributes(lttv_tracefile *tf) {
  return (lttv_attributes *)s->attributes->pdata[i];
}


static void lttv_analyse_trace(lttv_trace *t);

static void lttv_analyse_tracefile(lttv_tracefile *t);

lttv_trace_set_process(lttv_trace_set *s, ltt_time start, ltt_time end)
{
  int i, nb;
  lttv_hooks *before, *after;
  lttv_attributes *a;
  ltt_trace *t;
  lttv_filter *filter_data;

  a = lttv_trace_set_attributes(s);
  before = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/before");
  after = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/after");
  nb = lttv_trace_set_number(s);

  lttv_hooks_call(before, s);

  for(i = 0; i < nb; i++) {
    t = lttv_trace_set_get(s,i);
    a = lttv_trace_set_trace_attributes(s,i);
    lttv_analyse_trace(t, a, start, end);
  }

  lttv_hooks_call(after, s);
}


static void lttv_analyse_trace(ltt_trace *t, lttv_attributes *a,
    ltt_time start, ltt_time end)
{
  int i, nb_all_cpu, nb_per_cpu;
  lttv_hooks *before, *after;

  before = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/before");
  after = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/after");

  nb_all_cpu = ltt_trace_tracefile_number_all_cpu(t);
  nb_per_cpu = ltt_trace_tracefile_number_per_cpu(t);

  lttv_hooks_call(before, t);

  for(i = 0; i < nb_all_cpu; i++) {
    lttv_analyse_tracefile(ltt_trace_get_all_cpu(t,i), a, start, end);
  }

  for(i = 0; i < nb_per_cpu; i++) {
    lttv_analyse_tracefile(ltt_trace_get_per_cpu(t,i), a, start, end);
  }

  lttv_hooks_call(after, t);
}


static void lttv_analyse_tracefile(ltt_tracefile *t, lttv_attributes *a,
    ltt_time start, ltt_time end)
{
  ltt_event *event;
  unsigned id;
  lttv_hooks *before, *after, *event_hooks, *tracefile_check, *event_check;
  lttv_hooks_by_id *event_hooks_by_id;
  lttv_attributes *a;

  before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/tracefile/before");
  after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/tracefile/after");
  event_hooks = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,
      "hooks/event/selected");
  event_hooks_by_id = (lttv_hooks_by_id*)
      lttv_attributes_get_pointer_pathname(a, "hooks/event/byid");
  tracefile_check = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/tracefile/check");
  event_check = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/event/check");
  
  lttv_hooks_call(before, t);

  if(lttv_hooks_call_check(tracefile_check,t) &&
      ( lttv_hooks_number(event_hooks) != 0 || 
        lttv_hooks_by_id_number(event_hooks_by_id) != 0) {

    ltt_tracefile_seek_time(t, start);
    while((event = ltt_tracefile_read(t)) != NULL && 
          ltt_event_time(event) < end) {
      if(lttv_hooks_call_check(event_check)) {
        lttv_hooks_call(event_hooks,event);
        lttv_hooks_by_id_call(event_hooks_by_id,event,
            ltt_event_type_id(event));
      }
    }
  }
  lttv_hooks_call(after, t);
}



