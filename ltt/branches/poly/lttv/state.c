
#include <lttv/state.h>
#include <ltt/facility.h>
#include <ltt/trace.h>

LttvExecutionMode
  LTTV_STATE_MODE_UNKNOWN,
  LTTV_STATE_USER_MODE,
  LTTV_STATE_SYSCALL,
  LTTV_STATE_TRAP,
  LTTV_STATE_IRQ;

LttvExecutionSubmode
  LTTV_STATE_SUBMODE_UNKNOWN,
  LTTV_STATE_SUBMODE_NONE;

LttvProcessStatus
  LTTV_STATE_UNNAMED,
  LTTV_STATE_WAIT_FORK,
  LTTV_STATE_WAIT_CPU,
  LTTV_STATE_EXIT,
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN;

static GQuark
  LTTV_STATE_HOOKS;


static void fill_name_tables(LttvTraceState *tcs);

static void free_name_tables(LttvTraceState *tcs);

static void remove_all_processes(GHashTable *processes);

LttvProcessState *create_process(LttvTracefileState *tfs, 
				 LttvProcessState *parent, guint pid);

static void
init(LttvTracesetState *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  LttvTracefileState *tfcs;
  
  LttTime timestamp = {0,0};

  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      init((LttvTracesetContext *)self, ts);

  nb_trace = lttv_traceset_number(ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tc = self->parent.traces[i];
    tcs = (LttvTraceState *)tc;
    tcs->processes = g_hash_table_new(g_direct_hash, g_direct_equal);
    fill_name_tables(tcs);

    nb_control = ltt_trace_control_tracefile_number(tc->t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(tc->t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfcs = LTTV_TRACEFILE_STATE(tc->control_tracefiles[j]);
      }
      else {
        tfcs = LTTV_TRACEFILE_STATE(tc->per_cpu_tracefiles[j - nb_control]);
      }

      tfcs->parent.timestamp = timestamp;
      tfcs->process = create_process(tfcs, NULL,0);
      tfcs->cpu_name= g_quark_from_string(ltt_tracefile_name(tfcs->parent.tf));
    }
  }
}


static void
fini(LttvTracesetState *self)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceState *tcs;

  LttvTracefileState *tfcs;

  nb_trace = lttv_traceset_number(LTTV_TRACESET_CONTEXT(self)->ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceState *)(LTTV_TRACESET_CONTEXT(self)->traces[i]);
    remove_all_processes(tcs->processes);
    g_hash_table_destroy(tcs->processes);
    free_name_tables(tcs);
  }
  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      fini((LttvTracesetContext *)self);
}


static LttvTracesetContext *
new_traceset_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT(g_object_new(LTTV_TRACESET_STATE_TYPE, NULL));
}


static LttvTraceContext * 
new_trace_context(LttvTracesetContext *self)
{
  return LTTV_TRACE_CONTEXT(g_object_new(LTTV_TRACE_STATE_TYPE, NULL));
}


static LttvTracefileContext *
new_tracefile_context(LttvTracesetContext *self)
{
  return LTTV_TRACEFILE_CONTEXT(g_object_new(LTTV_TRACEFILE_STATE_TYPE, NULL));
}


static void
traceset_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
traceset_state_finalize (LttvTracesetState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
traceset_state_class_init (LttvTracesetContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) traceset_state_finalize;
  klass->init = (void (*)(LttvTracesetContext *self, LttvTraceset *ts))init;
  klass->fini = (void (*)(LttvTracesetContext *self))fini;
  klass->new_traceset_context = new_traceset_context;
  klass->new_trace_context = new_trace_context;
  klass->new_tracefile_context = new_tracefile_context;
}


GType 
lttv_traceset_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracesetStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) traceset_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracesetContext),
      0,      /* n_preallocs */
      (GInstanceInitFunc) traceset_state_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACESET_CONTEXT_TYPE, "LttvTracesetStateType", 
        &info, 0);
  }
  return type;
}


static void
trace_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
trace_state_finalize (LttvTraceState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACE_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
trace_state_class_init (LttvTraceContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) trace_state_finalize;
}


GType 
lttv_trace_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTraceStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) trace_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTraceState),
      0,      /* n_preallocs */
      (GInstanceInitFunc) trace_state_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACE_CONTEXT_TYPE, 
        "LttvTraceStateType", &info, 0);
  }
  return type;
}


static void
tracefile_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
tracefile_state_finalize (LttvTracefileState *self)
{
  G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACEFILE_CONTEXT_TYPE))->
      finalize(G_OBJECT(self));
}


static void
tracefile_state_class_init (LttvTracefileStateClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->finalize = (void (*)(GObject *self)) tracefile_state_finalize;
}


GType 
lttv_tracefile_state_get_type(void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvTracefileStateClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) tracefile_state_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvTracefileState),
      0,      /* n_preallocs */
      (GInstanceInitFunc) tracefile_state_instance_init    /* instance_init */
    };

    type = g_type_register_static (LTTV_TRACEFILE_CONTEXT_TYPE, 
        "LttvTracefileStateType", &info, 0);
  }
  return type;
}


static void 
fill_name_tables(LttvTraceState *tcs) 
{
  int i, nb;

  char *f_name, *e_name;

  LttvTraceHook h;

  LttEventType *et;

  LttType *t;

  GString *fe_name = g_string_new("");

  nb = ltt_trace_eventtype_number(tcs->parent.t);
  tcs->eventtype_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    et = ltt_trace_eventtype_get(tcs->parent.t, i);
    e_name = ltt_eventtype_name(et);
    f_name = ltt_facility_name(ltt_eventtype_facility(et));
    g_string_printf(fe_name, "%s.%s", f_name, e_name);
    tcs->eventtype_names[i] = g_quark_from_string(fe_name->str);    
  }

  lttv_trace_find_hook(tcs->parent.t, "core", "syscall_entry",
      "syscall_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

  /* CHECK syscalls should be an emun but currently are not!  
  tcs->syscall_names = g_new(GQuark, nb);

  for(i = 0 ; i < nb ; i++) {
    tcs->syscall_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
  }
  */

  tcs->syscall_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "syscall %d", i);
    tcs->syscall_names[i] = g_quark_from_string(fe_name->str);
  }

  lttv_trace_find_hook(tcs->parent.t, "core", "trap_entry",
      "trap_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

  /*
  tcs->trap_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    tcs->trap_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
  }
  */

  tcs->trap_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "trap %d", i);
    tcs->trap_names[i] = g_quark_from_string(fe_name->str);
  }

  lttv_trace_find_hook(tcs->parent.t, "core", "irq_entry",
      "irq_id", NULL, NULL, NULL, &h);
  t = ltt_field_type(h.f1);
  nb = ltt_type_element_number(t);

  /*
  tcs->irq_names = g_new(GQuark, nb);
  for(i = 0 ; i < nb ; i++) {
    tcs->irq_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
  }
  */

  tcs->irq_names = g_new(GQuark, 256);
  for(i = 0 ; i < 256 ; i++) {
    g_string_printf(fe_name, "irq %d", i);
    tcs->irq_names[i] = g_quark_from_string(fe_name->str);
  }

  g_string_free(fe_name, TRUE);
}


static void 
free_name_tables(LttvTraceState *tcs) 
{
  g_free(tcs->eventtype_names);
  g_free(tcs->syscall_names);
  g_free(tcs->trap_names);
  g_free(tcs->irq_names);
} 


static void push_state(LttvTracefileState *tfs, LttvExecutionMode t, 
    guint state_id)
{
  LttvExecutionState *es;

  LttvProcessState *process = tfs->process;

  guint depth = process->execution_stack->len;

  g_array_set_size(process->execution_stack, depth + 1);
  es = &g_array_index(process->execution_stack, LttvExecutionState, depth);
  es->t = t;
  es->n = state_id;
  es->entry = es->change = tfs->parent.timestamp;
  es->s = process->state->s;
  process->state = es;
}


static void pop_state(LttvTracefileState *tfs, LttvExecutionMode t)
{
  LttvProcessState *process = tfs->process;

  guint depth = process->execution_stack->len - 1;

  if(process->state->t != t){
    g_warning("Different execution mode type (%d.%09d): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    g_warning("process state has %s when pop_int is %s\n",
		    g_quark_to_string(process->state->t),
		    g_quark_to_string(t));
    g_warning("{ %u, %u, %s, %s }\n",
		    process->pid,
		    process->ppid,
		    g_quark_to_string(process->name),
		    g_quark_to_string(process->state->s));
    return;
  }

  if(depth == 0){
    g_warning("Trying to pop last state on stack (%d.%09d): ignore it\n",
        tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
    return;
  }

  g_array_remove_index(process->execution_stack, depth);
  depth--;
  process->state = &g_array_index(process->execution_stack, LttvExecutionState,
      depth);
  process->state->change = tfs->parent.timestamp;
}


LttvProcessState *create_process(LttvTracefileState *tfs, 
    LttvProcessState *parent, guint pid)
{
  LttvProcessState *process = g_new(LttvProcessState, 1);

  LttvExecutionState *es;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  char buffer[128];

  tcs = (LttvTraceState *)tc = tfs->parent.t_context;

  g_hash_table_insert(tcs->processes, GUINT_TO_POINTER(pid), process);
  process->pid = pid;

  if(parent) {
    process->ppid = parent->pid;
    process->name = parent->name;
  }
  else {
    process->ppid = 0;
    process->name = LTTV_STATE_UNNAMED;
  }

  process->creation_time = tfs->parent.timestamp;
  sprintf(buffer,"%d-%lu.%lu",pid, process->creation_time.tv_sec, 
	  process->creation_time.tv_nsec);
  process->pid_time = g_quark_from_string(buffer);
  process->execution_stack = g_array_new(FALSE, FALSE, 
      sizeof(LttvExecutionState));
  g_array_set_size(process->execution_stack, 1);
  es = process->state = &g_array_index(process->execution_stack, 
      LttvExecutionState, 0);
  es->t = LTTV_STATE_USER_MODE;
  es->n = LTTV_STATE_SUBMODE_NONE;
  es->entry = tfs->parent.timestamp;
  es->change = tfs->parent.timestamp;
  es->s = LTTV_STATE_WAIT_FORK;

  return process;
}


LttvProcessState *lttv_state_find_process(LttvTracefileState *tfs, guint pid)
{
  LttvTraceState *ts =(LttvTraceState *)LTTV_TRACEFILE_CONTEXT(tfs)->t_context;
  LttvProcessState *process = g_hash_table_lookup(ts->processes, 
      GUINT_TO_POINTER(pid));
  if(process == NULL) process = create_process(tfs, NULL, pid);
  return process;
}


static void exit_process(LttvTracefileState *tfs, LttvProcessState *process) 
{
  LttvTraceState *ts = LTTV_TRACE_STATE(tfs->parent.t_context);

  g_hash_table_remove(ts->processes, GUINT_TO_POINTER(process->pid));
  g_array_free(process->execution_stack, TRUE);
  g_free(process);
}


static void free_process_state(gpointer key, gpointer value,gpointer user_data)
{
  g_array_free(((LttvProcessState *)value)->execution_stack, TRUE);
  g_free(value);
}


static void remove_all_processes(GHashTable *processes)
{
  g_hash_table_foreach(processes, free_process_state, NULL);
}


static gboolean syscall_entry(void *hook_data, void *call_data)
{
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->syscall_names[
      ltt_event_get_unsigned(s->parent.e, f)];
  push_state(s, LTTV_STATE_SYSCALL, submode);
  return FALSE;
}


static gboolean syscall_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_SYSCALL);
  return FALSE;
}


static gboolean trap_entry(void *hook_data, void *call_data)
{
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->trap_names[
      ltt_event_get_unsigned(s->parent.e, f)];
  push_state(s, LTTV_STATE_TRAP, submode);
  return FALSE;
}


static gboolean trap_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_TRAP);
  return FALSE;
}


static gboolean irq_entry(void *hook_data, void *call_data)
{
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  LttvExecutionSubmode submode;

  submode = ((LttvTraceState *)(s->parent.t_context))->irq_names[
      ltt_event_get_unsigned(s->parent.e, f)];

  /* Do something with the info about being in user or system mode when int? */
  push_state(s, LTTV_STATE_IRQ, submode);
  return FALSE;
}


static gboolean irq_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_IRQ);
  return FALSE;
}


static gboolean schedchange(void *hook_data, void *call_data)
{
  LttvTraceHook *h = (LttvTraceHook *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint pid_in, pid_out, state_out;

  pid_in = ltt_event_get_unsigned(s->parent.e, h->f1);
  pid_out = ltt_event_get_unsigned(s->parent.e, h->f2);
  state_out = ltt_event_get_unsigned(s->parent.e, h->f3);

  if(s->process != NULL) {

    if(state_out == 0) s->process->state->s = LTTV_STATE_WAIT_CPU;
    else if(s->process->state->s == LTTV_STATE_EXIT) 
        exit_process(s, s->process);
    else s->process->state->s = LTTV_STATE_WAIT;

    if(s->process->pid == 0)
      s->process->pid == pid_out;

    s->process->state->change = s->parent.timestamp;
  }
  s->process = lttv_state_find_process(s, pid_in);
  s->process->state->s = LTTV_STATE_RUN;
  s->process->state->change = s->parent.timestamp;
  return FALSE;
}


static gboolean process_fork(void *hook_data, void *call_data)
{
  LttField *f = ((LttvTraceHook *)hook_data)->f1;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint child_pid;

  child_pid = ltt_event_get_unsigned(s->parent.e, f);
  create_process(s, s->process, child_pid);
  return FALSE;
}


static gboolean process_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  if(s->process != NULL) {
    s->process->state->s = LTTV_STATE_EXIT;
  }
  return FALSE;
}


lttv_state_add_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttFacility *f;

  LttEventType *et;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  void *hook_data;

  GArray *hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
    g_array_set_size(hooks, 9);

    lttv_trace_find_hook(ts->parent.t, "core","syscall_entry","syscall_id", 
	NULL, NULL, syscall_entry, &g_array_index(hooks, LttvTraceHook, 0));

    lttv_trace_find_hook(ts->parent.t, "core", "syscall_exit", NULL, NULL, 
        NULL, syscall_exit, &g_array_index(hooks, LttvTraceHook, 1));

    lttv_trace_find_hook(ts->parent.t, "core", "trap_entry", "trap_id",
	NULL, NULL, trap_entry, &g_array_index(hooks, LttvTraceHook, 2));

    lttv_trace_find_hook(ts->parent.t, "core", "trap_exit", NULL, NULL, NULL, 
        trap_exit, &g_array_index(hooks, LttvTraceHook, 3));

    lttv_trace_find_hook(ts->parent.t, "core", "irq_entry", "irq_id", NULL, 
        NULL, irq_entry, &g_array_index(hooks, LttvTraceHook, 4));

    lttv_trace_find_hook(ts->parent.t, "core", "irq_exit", NULL, NULL, NULL, 
        irq_exit, &g_array_index(hooks, LttvTraceHook, 5));

    lttv_trace_find_hook(ts->parent.t, "core", "schedchange", "in", "out", 
        "out_state", schedchange, &g_array_index(hooks, LttvTraceHook, 6));

    lttv_trace_find_hook(ts->parent.t, "core", "process_fork", "child_pid", 
        NULL, NULL, process_fork, &g_array_index(hooks, LttvTraceHook, 7));

    lttv_trace_find_hook(ts->parent.t, "core", "process_exit", NULL, NULL, 
        NULL, process_exit, &g_array_index(hooks, LttvTraceHook, 8));

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(ts->parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.control_tracefiles[j]);
      }
      else {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.per_cpu_tracefiles[j-nb_control]);
      }

      for(k = 0 ; k < hooks->len ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.after_event_by_id, 
	  hook.id), hook.h, &g_array_index(hooks, LttvTraceHook, k));
      }
    }
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    *(val.v_pointer) = hooks;
  }
}


lttv_state_remove_event_hooks(LttvTracesetState *self)
{
  LttvTraceset *traceset = self->parent.ts;

  guint i, j, k, nb_trace, nb_control, nb_per_cpu, nb_tracefile;

  LttvTraceState *ts;

  LttvTracefileState *tfs;

  void *hook_data;

  GArray *hooks;

  LttvTraceHook hook;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATE(self->parent.traces[i]);
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    hooks = *(val.v_pointer);

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(ts->parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.control_tracefiles[j]);
      }
      else {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.per_cpu_tracefiles[j-nb_control]);
      }

      for(k = 0 ; k < hooks->len ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.after_event_by_id, 
	    hook.id), hook.h, &g_array_index(hooks, LttvTraceHook, k));
      }
    }
    g_array_free(hooks, TRUE);
  }
}


void lttv_state_init(int argc, char **argv)
{
  LTTV_STATE_UNNAMED = g_quark_from_string("unnamed");
  LTTV_STATE_MODE_UNKNOWN = g_quark_from_string("unknown execution mode");
  LTTV_STATE_USER_MODE = g_quark_from_string("user mode");
  LTTV_STATE_WAIT_FORK = g_quark_from_string("wait fork");
  LTTV_STATE_SYSCALL = g_quark_from_string("system call");
  LTTV_STATE_TRAP = g_quark_from_string("trap");
  LTTV_STATE_IRQ = g_quark_from_string("irq");
  LTTV_STATE_SUBMODE_UNKNOWN = g_quark_from_string("unknown submode");
  LTTV_STATE_SUBMODE_NONE = g_quark_from_string("(no submode)");
  LTTV_STATE_WAIT_CPU = g_quark_from_string("wait for cpu");
  LTTV_STATE_EXIT = g_quark_from_string("exiting");
  LTTV_STATE_WAIT = g_quark_from_string("wait for I/O");
  LTTV_STATE_RUN = g_quark_from_string("running");
  LTTV_STATE_HOOKS = g_quark_from_string("saved state hooks");
}

void lttv_state_destroy() 
{
}




