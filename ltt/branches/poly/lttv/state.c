
#include <lttv/state.h>
#include <ltt/facility.h>
#include <ltt/trace.h>

LttvInterruptType
  LTTV_STATE_USER_MODE,
  LTTV_STATE_SYSCALL,
  LTTV_STATE_TRAP,
  LTTV_STATE_IRQ;


LttvProcessStatus
  LTTV_STATE_UNNAMED,
  LTTV_STATE_WAIT_FORK,
  LTTV_STATE_WAIT_CPU,
  LTTV_STATE_EXIT,
  LTTV_STATE_WAIT,
  LTTV_STATE_RUN;

static GQuark
  LTTV_STATE_HOOKS;

void remove_all_processes(GHashTable *processes);


static void
init(LttvTracesetState *self, LttvTraceset *ts)
{
  guint i, j, nb_trace, nb_tracefile;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  LttvTracefileContext *tfc;

  LttvTracefileState *tfcs;

  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek_parent(LTTV_TRACESET_STATE_GET_CLASS(self)))->init((LttvTracesetContext *)self, ts);

  nb_trace = lttv_traceset_number(ts);
  for(i = 0 ; i < nb_trace ; i++) {
    tcs = (LttvTraceState *)tc = (LTTV_TRACESET_CONTEXT(self)->traces[i]);
    tcs->processes = g_hash_table_new(g_direct_hash, g_direct_equal);

    nb_tracefile = ltt_trace_control_tracefile_number(tc->t);
    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = (LttvTracefileState *)tfc = tc->control_tracefiles[j];
      tfcs->process = NULL;
    }

    nb_tracefile = ltt_trace_per_cpu_tracefile_number(tc->t);
    for(j = 0 ; j < nb_tracefile ; j++) {
      tfcs = (LttvTracefileState *)tfc = tc->per_cpu_tracefiles[j];
      tfcs->process = NULL;
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
  }
  LTTV_TRACESET_CONTEXT_CLASS(g_type_class_peek_parent(LTTV_TRACESET_STATE_GET_CLASS(self)))->fini((LttvTracesetContext *)self);
}


LttvTracesetContext *
new_traceset_context(LttvTracesetContext *self)
{
  return LTTV_TRACESET_CONTEXT(g_object_new(LTTV_TRACESET_STATE_TYPE, NULL));
}


LttvTraceContext * 
new_trace_context(LttvTracesetContext *self)
{
  return LTTV_TRACE_CONTEXT(g_object_new(LTTV_TRACE_STATE_TYPE, NULL));
}


LttvTracefileContext *
new_tracefile_context(LttvTracesetContext *self)
{
  return LTTV_TRACEFILE_CONTEXT(g_object_new(LTTV_TRACEFILE_STATE_TYPE, NULL));
}


static void
traceset_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void
traceset_state_finalize (LttvTracesetContext *self)
{
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACESET_STATE_GET_CLASS(self)))->finalize(G_OBJECT(self));
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
trace_state_finalize (LttvTraceContext *self)
{
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACE_STATE_GET_CLASS(self)))->finalize(G_OBJECT(self));
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
      sizeof (LttvTracesetState),
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
  G_OBJECT_CLASS(g_type_class_peek_parent(LTTV_TRACEFILE_STATE_GET_CLASS(self)))->finalize(G_OBJECT(self));
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


struct HookData {
  LttField *f1;
  LttField *f2;
  LttField *f3;
};


struct HookId {
  LttvHook h;
  guint id;
  void *hook_data;
  gboolean free_hook_data;
};


static void push_state(LttvTracefileState *tfs, LttvInterruptType t, 
    guint state_id)
{
  LttvInterruptState *intr;

  LttvProcessState *process = tfs->process;

  guint depth = process->interrupt_stack->len;

  g_array_set_size(process->interrupt_stack, depth + 1);
  intr = &g_array_index(process->interrupt_stack, LttvInterruptState, depth);
  intr->t = t;
  intr->n = state_id;
  intr->entry = intr->last_change = LTTV_TRACEFILE_CONTEXT(tfs)->timestamp;
  intr->s = process->state->s;
}


static void pop_state(LttvTracefileState *tfs, LttvInterruptType t)
{
  LttvProcessState *process = tfs->process;

  guint depth = process->interrupt_stack->len - 1;

  g_assert(process->state->t == t);
  g_array_remove_index(process->interrupt_stack, depth);
  depth--;
  process->state = &g_array_index(process->interrupt_stack, LttvInterruptState,
      depth);
}


LttvProcessState *create_process(LttvTracefileState *tfs, 
    LttvProcessState *parent, guint pid)
{
  LttvProcessState *process = g_new(LttvProcessState, 1);

  LttvInterruptState *intr;

  LttvTraceContext *tc;

  LttvTraceState *tcs;

  LttvTracefileContext *tfc = LTTV_TRACEFILE_CONTEXT(tfs);

  tcs = (LttvTraceState *)tc = tfc->t_context;

  g_hash_table_insert(tcs->processes, GUINT_TO_POINTER(pid), process);
  process->pid = pid;
  process->birth = tfc->timestamp;
  process->name = LTTV_STATE_UNNAMED;
  process->interrupt_stack = g_array_new(FALSE, FALSE, 
      sizeof(LttvInterruptState));
  g_array_set_size(process->interrupt_stack, 1);
  intr = process->state = &g_array_index(process->interrupt_stack, 
      LttvInterruptState, 0);
  intr->t = LTTV_STATE_USER_MODE;
  intr->n = 0;
  intr->entry = tfc->timestamp;
  intr->last_change = tfc->timestamp;
  intr->s = LTTV_STATE_WAIT_FORK;
}


LttvProcessState *find_process(LttvTracefileState *tfs, guint pid)
{
  LttvTraceState *ts =(LttvTraceState *)LTTV_TRACEFILE_CONTEXT(tfs)->t_context;
  LttvProcessState *process = g_hash_table_lookup(ts->processes, 
      GUINT_TO_POINTER(pid));
  if(process == NULL) process = create_process(tfs, NULL, pid);
  return process;
}


void exit_process(LttvTracefileState *tfs, LttvProcessState *process) 
{
  LttvTraceState *ts = LTTV_TRACE_STATE(tfs->parent.t_context);

  g_hash_table_remove(ts->processes, GUINT_TO_POINTER(process->pid));
  g_array_free(process->interrupt_stack, TRUE);
  g_free(process);
}


void free_process_state(gpointer key, gpointer value, gpointer user_data)
{
  g_array_free(((LttvProcessState *)value)->interrupt_stack, TRUE);
  g_free(value);
}


void remove_all_processes(GHashTable *processes)
{
  g_hash_table_foreach(processes, free_process_state, NULL);
}


gboolean syscall_entry(void *hook_data, void *call_data)
{
  LttField *f = (LttField *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  push_state(s, LTTV_STATE_SYSCALL, ltt_event_get_unsigned(
      LTTV_TRACEFILE_CONTEXT(s)->e, f));
  return FALSE;
}


gboolean syscall_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_SYSCALL);
  return FALSE;
}


gboolean trap_entry(void *hook_data, void *call_data)
{
  LttField *f = (LttField *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  push_state(s, LTTV_STATE_TRAP, ltt_event_get_unsigned(s->parent.e, f));
  return FALSE;
}


gboolean trap_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_TRAP);
  return FALSE;
}


gboolean irq_entry(void *hook_data, void *call_data)
{
  LttField *f = (LttField *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  /* Do something with the info about being in user or system mode when int? */
  push_state(s, LTTV_STATE_IRQ, ltt_event_get_unsigned(s->parent.e, f));
  return FALSE;
}


gboolean irq_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  pop_state(s, LTTV_STATE_IRQ);
  return FALSE;
}


gboolean schedchange(void *hook_data, void *call_data)
{
  struct HookData *h = (struct HookData *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint pid_in, pid_out, state_out;

  pid_in = ltt_event_get_int(s->parent.e, h->f1);
  pid_out = ltt_event_get_int(s->parent.e, h->f2);
  state_out = ltt_event_get_int(s->parent.e, h->f3);
  if(s->process != NULL) {
    if(state_out == 0) s->process->state->s = LTTV_STATE_WAIT_CPU;
    else if(s->process->state->s == LTTV_STATE_EXIT) 
        exit_process(s, s->process);
    else s->process->state->s = LTTV_STATE_WAIT;
  }
  s->process = find_process(s, pid_in);
  s->process->state->s = LTTV_STATE_RUN;
  return FALSE;
}


gboolean process_fork(void *hook_data, void *call_data)
{
  LttField *f = (LttField *)hook_data;

  LttvTracefileState *s = (LttvTracefileState *)call_data;

  guint child_pid;

  child_pid = ltt_event_get_int(s->parent.e, f);
  create_process(s, s->process, child_pid);
  return FALSE;
}


gboolean process_exit(void *hook_data, void *call_data)
{
  LttvTracefileState *s = (LttvTracefileState *)call_data;

  if(s->process != NULL) {
    s->process->state->s = LTTV_STATE_EXIT;
  }
  return FALSE;
}


static LttField *
find_field(LttEventType *et, const char *field)
{
  LttType *t;

  LttField *f;

  guint i, nb;

  char *name;

  if(field == NULL) return NULL;

  f = ltt_eventtype_field(et);
  t = ltt_eventtype_type(et);
  g_assert(ltt_type_class(t) == LTT_STRUCT);
  nb = ltt_type_member_number(t);
  for(i = 0 ; i < nb ; i++) {
    ltt_type_member_type(t, i, &name);
    if(strcmp(name, field) == 0) break;
  }
  g_assert(i < nb);
  return ltt_field_member(f, i);
}


static struct HookId
find_hook(LttTrace *t, char *facility, char *event, 
    char *field1, char *field2, char *field3, LttvHook h)
{
  LttFacility *f;

  LttEventType *et;

  guint nb, pos, i;

  struct HookId hook_id;

  struct HookData hook_data, *phook_data;

  char *name;

  nb = ltt_trace_facility_find(t, facility, &pos);
  if(nb < 1) g_error("No %s facility", facility);
  f = ltt_trace_facility_get(t, pos);
  et = ltt_facility_eventtype_get_by_name(f, event);
  if(et == NULL) g_error("Event %s does not exist", event);

  hook_id.id = *(ltt_eventtype_id(et)); /* CHECK */
  hook_id.h = h;
  hook_id.free_hook_data = FALSE;
  hook_data.f1 = find_field(et, field1);
  hook_data.f2 = find_field(et, field2);
  hook_data.f3 = find_field(et, field3);
  if(hook_data.f1 == NULL) hook_id.hook_data = NULL;
  else if(hook_data.f2 == NULL) hook_id.hook_data = hook_data.f1;
  else {
    phook_data = g_new(struct HookData, 1);
    *phook_data = hook_data;
    hook_id.hook_data = phook_data;
    hook_id.free_hook_data = TRUE;
  }
  return hook_id;
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

  struct HookId hook_id;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceState *)self->parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_new(FALSE, FALSE, sizeof(struct HookId));
    g_array_add(hooks, find_hook(ts->parent.t, "core","syscall_entry", 
        "syscall_id", NULL, NULL, syscall_entry));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "syscall_exit", 
        NULL, NULL, NULL, syscall_exit));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "trap_entry", "trap_id",
        NULL, NULL, trap_entry));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "trap_exit", NULL, NULL,
        NULL, trap_exit));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "irq_entry", "irq_id",
        NULL, NULL, irq_entry));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "irq_exit", NULL, NULL,
        NULL, irq_exit));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "schedchange", 
        "in", "out", "out_state", schedchange));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "process_fork", 
        "child_pid", NULL, NULL, process_fork));
    g_array_add(hooks, find_hook(ts->parent.t, "core", "process_exit", 
        NULL, NULL, NULL, process_exit));

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_per_cpu = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.control_tracefiles[j]);
      }
      else {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.per_cpu_tracefiles[j]);
      }

      for(k = 0 ; k < hooks->len ; k++) {
        hook_id = g_array_index(hooks, struct HookId, k);
        lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.before_event_by_id, 
	  hook_id.id), hook_id.h, hook_id.hook_data);
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

  struct HookId hook_id;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = LTTV_TRACE_STATE(self->parent.traces[i]);
    lttv_attribute_find(self->parent.a, LTTV_STATE_HOOKS, LTTV_POINTER, &val);
    hooks = *(val.v_pointer);

    /* Add these hooks to each before_event_by_id hooks list */

    nb_control = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_per_cpu = ltt_trace_control_tracefile_number(ts->parent.t);
    nb_tracefile = nb_control + nb_per_cpu;
    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control) {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.control_tracefiles[j]);
      }
      else {
        tfs = LTTV_TRACEFILE_STATE(ts->parent.per_cpu_tracefiles[j]);
      }

      for(k = 0 ; k < hooks->len ; k++) {
        hook_id = g_array_index(hooks, struct HookId, k);
        lttv_hooks_remove_data(
            lttv_hooks_by_id_find(tfs->parent.before_event_by_id, 
	    hook_id.id), hook_id.h, hook_id.hook_data);

      }
      for(k = 0 ; k < hooks->len ; k++) {
        hook_id = g_array_index(hooks, struct HookId, k);
        if(hook_id.free_hook_data) g_free(hook_id.hook_data);
      }
    }
    g_array_free(hooks, TRUE);
  }
}


void lttv_state_init(int argc, char **argv)
{
  LTTV_STATE_UNNAMED = g_quark_from_string("unnamed");
  LTTV_STATE_USER_MODE = g_quark_from_string("user mode");
  LTTV_STATE_WAIT_FORK = g_quark_from_string("wait fork");
  LTTV_STATE_SYSCALL = g_quark_from_string("system call");
  LTTV_STATE_TRAP = g_quark_from_string("trap");
  LTTV_STATE_IRQ = g_quark_from_string("irq");
  LTTV_STATE_WAIT_CPU = g_quark_from_string("wait for cpu");
  LTTV_STATE_EXIT = g_quark_from_string("exiting");
  LTTV_STATE_WAIT = g_quark_from_string("wait for I/O");
  LTTV_STATE_RUN = g_quark_from_string("running");
  LTTV_STATE_HOOKS = g_quark_from_string("saved state hooks");
}

void lttv_state_destroy() 
{
}




