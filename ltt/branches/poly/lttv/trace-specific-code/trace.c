/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

struct _lttv_trace_set {
  GPtrArray *traces;
  lttv_attributes *a;
};

struct _lttv_trace {
  GPtrArray *all_cpu;
  GPtrArray *per_cpu;
  char *name;
  lttv_attributes *a;
};


struct _lttv_tracefile {
  ltt_tracefile *t;
  lttv_attributes *a;
};


lttv_trace_set *lttv_trace_set_new() {
  lttv_trace_set s;

  s = g_new(lttv_trace_set, 1);
  s->traces = g_ptr_array_new();
  s->a = lttv_attributes_new();
}

lttv_trace_set *lttv_trace_set_destroy(lttv_trace_set *s) {
  g_ptr_array_free(s->traces);
  lttv_attributes_destroy(s->a);
  return g_free(s);
}

void lttv_trace_set_add(lttv_trace_set *s, lttv_trace *t) {
  g_ptr_array_add(s,t);
}

unsigned lttv_trace_set_number(lttv_trace_set *s) {
  return s->traces.len;
}


lttv_trace *lttv_trace_set_get(lttv_trace_set *s, unsigned i) {
  g_assert(s->traces->len <= i);
  return s->traces.pdata[i];
}


lttv_trace *lttv_trace_set_remove(lttv_trace_set *s, unsigned i) {
  return g_ptr_array_remove_index(s->traces,i);
}


/* Look at all files in the directory. Open all those with ltt as extension
   and sort these as per cpu or all cpu. */

lttv_trace *lttv_trace_open(char *pathname) {
  lttv_trace *t;

  t = g_new(lttv_trace, 1);
  t->per_cpu = g_ptr_array_new();
  t->all_cpu = g_ptr_array_new();
  t->a = lttv_attributes_new();
  return t;
}

int lttv_trace_close(lttv_trace *t) {

  g_ptr_array_free(t->per_cpu);
  g_ptr_array_free(t->all_cpu);
  lttv_attributes_destroy(t->a);
  g_free(t);
  return 0;
}

char *lttv_trace_name(lttv_trace *t) {
  return t->name;
}


unsigned int lttv_trace_tracefile_number(lttv_trace *t) {
  return t->per_cpu->len + t->all_cpu->len;
}

unsigned int lttv_trace_cpu_number(lttv_trace *t) {
  /* */
}

unsigned int lttv_trace_tracefile_number_per_cpu(lttv_trace *t) {
  return t->per_cpu->len;
}

unsigned int lttv_trace_tracefile_number_all_cpu(lttv_trace *t) {
  return t->all_cpu_len;
}

lttv_tracefile *lttv_trace_tracefile_get_per_cpu(lttv_trace *t, unsigned i) {
  return t->per_cpu->pdata[i];
}

lttv_tracefile *lttv_trace_tracefile_get_all_cpu(lttv_trace *t, unsigned i) {
  return t->all_cpu->pdata[i];
}


/* A set of attributes is attached to each trace set, trace and tracefile
   to store user defined data as needed. */

lttv_attributes *lttv_trace_set_attributes(lttv_trace_set *s) {
  return s->a;
}

lttv_attributes *lttv_trace_attributes(lttv_trace *t) {
  return t->a;
}

lttv_attributes *lttv_tracefile_attributes(lttv_tracefile *tf) {
  return tf->a;
}


ltt_tracefile *lttv_tracefile_ltt_tracefile(lttv_tracefile *tf) {
  return tf->t;
}

char *lttv_tracefile_name(lttv_tracefile *tf) {
  return tf->name;
}


