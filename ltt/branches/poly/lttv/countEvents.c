/* The countEvents module accumulates data for various basic reports

   Not only does it count the events for each event type, it also tracks
   the current state (current process and user/system mode) in order to 
   categorize accordingly the event counts. */

void init(int argc, char **argv)
{
  lttv_attributes *a;
  lttv_hooks *before, *after;

  a = lttv_global_attributes();
  before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/before");
  after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/after");
  lttv_hooks_add(before, countEvents_trace_set_before, NULL);
  lttv_hooks_add(after, countEvents_trace_set_after, NULL);
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
  lttv_hooks_remove(before, countEvents_trace_set_before, NULL);
  lttv_hooks_remove(after, countEvents_trace_set_after, NULL);
}


/* Insert the hooks before and after each trace and tracefile */

typedef struct _trace_context {
  unsigned nb_cpu;
  struct cpu_context *cpus;
  lttv_attributes *processes;
  lttv_key *key;
  lttv_attributes *event_counts;
  lttv_attributes *trace_attributes;
} trace_context;

static bool countEvents_trace_set_before(void *hook_data, void *call_data)
{
  int i, j, nb, nbtf;
  lttv_trace_set *s;
  lttv_attributes *a;
  trace_context *c;

  s = (lttv_trace_set *)call_data;

  /* For each trace prepare the contexts and insert the hooks */

  nb = lttv_trace_set_number(s);
  for(i = 0 ; i < nb ; i++) {
    c = g_new(trace_context);
    a = lttv_trace_set_trace_attributes(s, i);

    if(lttv_attributes_get_pointer_pathname(a, "countEvents/context") != NULL){
      g_error("Recursive call to TextDump");
    }

    lttv_attributes_set_pointer_pathname(a, "countEvents/context", c);

    h = lttv_attributes_get_hooks(a, "hooks/before");
    lttv_hooks_add(h, countEvents_trace_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/after");
    lttv_hooks_add(h, countEvents_trace_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/tacefile/before");
    lttv_hooks_add(h, countEvents_tracefile_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/tracefile/after");
    lttv_hooks_add(h, countEvents_tracefile_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/event/selected");
    lttv_hooks_add(h, couneEvents_event, c);
  }

  return TRUE;
}


/* Remove the hooks before and after each trace and tracefile, and for each
   event. Print trace set level statistics. */

static bool countEvents_trace_set_after(void *hook_data, void *call_data)
{
  int i, j, nb, nbtf;
  lttv_trace_set *s;
  lttv_attributes *a;
  trace_context *c;

  s = (lttv_trace_set *)call_data;

  /* Get the file pointer */

  fp = (FILE *)lttv_attributes_get_pointer_pathname(ga, "textDump/file");

  /* For each trace remove the hooks */

  nb = lttv_trace_set_number(s);
  for(i = 0 ; i < nb ; i++) {
    a = lttv_trace_set_trace_attributes(s, i);
    c = (trace_context *)lttv_attributes_get_pointer_pathname(a, 
        "textDump/context");
    lttv_attributes_set_pointer_pathname(a, "textDump/context", NULL);

    h = lttv_attributes_get_hooks(a, "hooks/before");
    lttv_hooks_remove(h, countEvents_trace_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/after");
    lttv_hooks_remove(h, countEvents_trace_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/tacefile/before");
    lttv_hooks_remove(h, countEvents_tracefile_before, c);
    h = lttv_attributes_get_hooks(a, "hooks/tracefile/after");
    lttv_hooks_remove(h, countEvents_tracefile_after, c);
    h = lttv_attributes_get_hooks(a, "hooks/event/selected");
    lttv_hooks_remove(h, countEvents_event, c);
    g_free(c);
  }

  /* Compute statistics for the complete trace set */

  return TRUE;
}


static bool countEvents_trace_before(void *hook_data, void *call_data)
{
  ltt_trace *t;
  trace_context *c;

  c = (trace_context *)hook_data;
  t = (ltt_trace *)call_data;

  /* Initialize the context */

  return TRUE;
}


/* Print trace level statistics */

static bool countEvents_trace_after(void *hook_data, void *call_data)
{
  ltt_trace *t;
  trace_context *c;

  c = (trace_context *)hook_data;
  t = (ltt_trace *)call_data;

  /* Sum events in different ways for the whole trace */

  return TRUE;
}


static bool countEvents_tracefile_before(void *hook_data, void *call_data)
{
  ltt_tracefile *tf;
  trace_context *c;

  c = (trace_context *)hook_data;
  tf = (ltt_tracefile *)call_data;

  /* Nothing special to do for now */

  return TRUE;
}


static bool countEvents_tracefile_after(void *hook_data, void *call_data)
{
  ltt_tracefile *tf;
  trace_context *c;

  c = (trace_context *)hook_data;
  tf = (ltt_tracefile *)call_data;

  /* Nothing special to do for now */

  return TRUE;
}


static bool countEvents_event(void *hook_data, void *call_data)
{
  ltt_event *e;
  trace_context *c;
  unsigned cpu;
  unsigned eventtype;
  ltt_time t;

  e = (ltt_event *)call_data;
  c = (event_context *)hook_data;

  eventtype = ltt_event_eventtype_id(e);
  cpu = ltt_event_cpu_id(e);
  time = ltt_event_time(e);

  /* Accumulate the CPU time spent in that state */

  key = c->cpu[cpu].key;
  last_time = c->cpu[cpu].last_time;
  c->cpu[cpu].last_time; = time;
  lttv_key_index(key,LTTV_KEY_TYPE) = KEY_CPU;
  total_time = lttv_attributes_time_get(c->main_attributes, key);

  lttv_sub_time_value(delta_time, last_time, time);
  lttv_add_time_valie(*total_time, *total_time, delta_time);
  
  /* Some events indicate a state change to remember (syscall goes from user to
     system mode, open assigns a new file to a file descriptor, exec changes
     the memory map for the text section...) or have additional statistics 
     gathered. */

  switch(c->eventtype_class[eventtype]) {

  case LTTV_EVENT_SYSCALL_ENTRY:
    n = ltt_event_get_unsigned(e,c->syscall_field)
    push_state(c, cpu, KEY_SYSCALL, n, time);
    /* For page faults it may be interesting to note waiting on which file */
    break;

  case LTTV_EVENT_SYSCALL_EXIT:
    pop_state(c->cpu, cpu, KEY_SYSCALL, time);
    break;

  case LTTV_EVENT_TRAP_ENTRY:
    n = ltt_event_get_unsigned(e,c->trap_field)
    push_state(c, cpu, KEY_TRAP, n, time);
    break;

  case LTTV_EVENT_TRAP_EXIT:
    pop_state(c->cpu, cpu, KEY_TRAP, time);
    break;

  case LTTV_EVENT_IRQ_ENTRY:
    n = ltt_event_get_unsigned(e,c->irq_field)
    push_state(c, cpu, KEY_IRQ, n, time);
    break;

  case LTTV_EVENT_IRQ_EXIT:
    pop_state(c->cpu, cpu, KEY_IRQ, time);
    break;


  default:
  }

  /* The key already specifies the host, cpu, process and state, add the 
     event type and simply count one for the current event. */

  lttv_key_index(key,LTTV_KEY_TYPE) = c->eventtype_key[eventtype];
  count = lttv_attributes_get_integer(c->main_attributes, key);
  (*count)++;

  return TRUE;
}
