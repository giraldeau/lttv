/*

Analyse: loop over events, either one tracefile after another or
         simultaneously by increasing time over all tracefiles.

Process: create the process_state structure and register for all state 
         changing events to update the process_state.

Stats: create an lttv_attributes to receive statistics. Offer functions
       to specify statistics gathering (event types, specific field as int,
       specific field as histogram...); this is used for syscalls and for
       bytes read and written. Eventually factor out the type of
       state and key positions (disk state, ethernet state...)

Operations on stats:
       select based on match, sort based on compare function/key order,
       sum based on equality of truncated key

Sort order:
  key to base the sort on, by decreasing order of preference

Match/combine:
  for each key component, accept as is, only accept x, combine with previous.

Print stats:
  print hierarchically


*/


typedef struct _stats_hook_data {
  lttv_attributes *a;
  lttv_key *key;
  GHashTable *processes;
  lttv_string_id current_process;
  GArray *state;
  lttv_string_id current_state;
  bool init_done;
} stats_hook_data;

/* Process state is wait, user, system, trap, irq */
 
/* before, after, print, free */

/* The accumulated statistics are:

for each trace:

  The hierarchical key contains:

  system/cpu/process/state/type/id 

  where state is one of user, system, irq, trap or wait, and type is one
  of eventtype, syscall, and id is specific to each category (event id,
  syscall number...).

  print per system/state/substate/eventid (sum over process/cpu)
  print per system/cpu/state/substate/eventid (sum over process)
  print per system/process/state/substate/eventid (sum over cpu)

  number of events of each type
*/

lttv_basicStats_before(lttv_trace_set *s)
{
  int i, j, nb_trace, nb_tracefile;
  lttv_trace *t;
  lttv_tracefile *tf;
  lttv_attributes *a;
  stats_hook_data *hook_data, *old;
  
  nb_trace = lttv_trace_set_number(s);

  for(i = 0 ; i < nb_trace ; i++) {
    t = lttv_trace_set_get(s,i);
    nb_tracefile = lttv_trace_number(t);

    hook_data = lttv_basicStats_new();
    a = lttv_trace_attributes(t);
    old = (stats_hook_data *)lttv_attributes_get_pointer_pathname(a,
        "stats/basic");
    lttv_basicStats_destroy(old);
    lttv_attributes_set_pointer_pathname(a,"stats/basic",hook_data);

    for(j = 0 ; j < nb_tracefile ; j++) {
      tf = lttv_trace_get(t,j);
      a = lttv_tracefile_attributes(tf);
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/event");
      lttv_hooks_add(h, compute_stats, hook_data);
    }
  }
}

lttv_basicStats_after(lttv_trace_set *s)
{
  int i, j, nb_trace, nb_tracefile;
  lttv_trace *t;
  lttv_tracefile *tf;
  lttv_attributes *a;
  stats_hook_data *hook_data;
  
  nb_trace = lttv_trace_set_number(s);

  for(i = 0 ; i < nb_trace ; i++) {
    t = lttv_trace_set_get(s,i);
    nb_tracefile = lttv_trace_number(t);

    hook_data = (stats_hook_data *)lttv_attributes_get_pointer_pathname(a,
        "stats/basic");

    for(j = 0 ; j < nb_tracefile ; j++) {
      tf = lttv_trace_get(t,j);
      a = lttv_tracefile_attributes(tf);
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/event");
      lttv_hooks_remove(h, compute_stats, hook_data);
    }

    lttv_basicStats_destroy(hook_data);
  }
}


update_state

compute time in that state...

For processes remember the command name...

Compute bytes read/written...

static void compute_eventtype_id_stats(void *hook_data, void *call_data)
{
  stats_hook_data *d;
  ltt_event *e;

  d = (stats_hook_data *)hook_data;
  e = (ltt_event *)call_data;

  lttv_key_index(d->key,4) = string_id_EventType;
  lttv_key_index(d->key,5) = string_id_unsigned(ltt_event_eventtype_id(e));
  (*lttv_attributes_get_integer(d->a,d->key))++;
}

/* The field for which a sum is required is expressed as eventtype/field */

typedef struct _field_sum_data {
  stats_hook_data *d;
  ltt_field *f;
  lttv_string_id type_name;
  lttv_string_id id_name;
} field_sum_data;

lttv_basicStats_sum_integer_field_before(lttv_trace_set *s, char *field_path,
    char *type_name, char *id_name)
{
  int i, j, nb_trace, nb_tracefile;
  lttv_trace *t;
  lttv_tracefile *tf;
  lttv_attributes *a;
  lttv_hooks_by_id h;
  stats_hook_data *stats_data;
  field_sum_data *hook_data;
  unsigned id;

  nb_trace = lttv_trace_set_number(s);

  for(i = 0 ; i < nb_trace ; i++) {
    t = lttv_trace_set_get(s,i);
    nb_tracefile = lttv_trace_number(t);

    a = lttv_trace_attributes(t);
    stats_data = (stats_hook_data *)lttv_attributes_get_pointer_pathname(a,
        "stats/basic");

    for(j = 0 ; j < nb_tracefile ; j++) {
      tf = lttv_trace_get(t,j);
      a = lttv_tracefile_attributes(tf);
      hook_data = g_new(field_sum_data);
      hook_data->d = stats_data;
      hook_data->f = lttv_tracefile_eventtype_field_pathname(
          lttv_tracefile_ltt_tracefile(tf), field_path, &id);
      hook_data->type_name = type_name;
      hook_data->id_name = id_name;
      h = (lttv_hooks_by_id *)lttv_attributes_get_pointer_pathname(a,
          "hooks/eventid");
      if(id_name != NULL) {
        lttv_hooks_add(h, compute_integer_field_sum, hook_data);
      }
      else {
        lttv_hooks_add(h, compute_integer_field_histogram, hook_data);
      }
    }
  }
}

static void compute_integer_field_sum(void *hook_data, void *call_data)
{
  field_sum_data *d;
  ltt_event *e;

  d = (field_sum_data *)hook_data;
  e = (ltt_event *)call_data;

  lttv_key_index(d->key,4) = d->type_name;
  lttv_key_index(d->key,5) = d->id_name;
  (*lttv_attributes_get_integer(d->a,d->key)) += 
      ltt_event_get_unsigned(e,d->f);
}

static void compute_integer_field_histogram(void *hook_data, void *call_data)
{
  field_sum_data *d;
  ltt_event *e;

  d = (field_sum_data *)hook_data;
  e = (ltt_event *)call_data;

  lttv_key_index(d->key,4) = d->type_name;
  lttv_key_index(d->key,5)= string_id_unsigned(ltt_event_get_unsigned(e,d->f));
  (*lttv_attributes_get_integer(d->a,d->key))++;
}


stats_hook_data *lttv_basicStats_new()
{
  g_new(stats_hook_data,1);
  hook_data->a = lttv_attributes_new();
  hook_data->key = lttv_key_new();
  id = lttv_string_id("");
  for j = 0 ; j < 6 ; j++) lttv_key_append(hook_data->key,id);
  hook_data->processes = g_hash_table_new(g_int_hash,g_int_equal);
  hook_data->init_done = FALSE;
}

stats_hook_data *lttv_basicStats_destroy(stats_hook_data *hook_data)
{
  lttv_attributes_destroy(hook_data->a);
  lttv_key_destroy(hook_data->key);
  lttv_process_state_destroy(hook_data->processes);
  g_free(hook_data);
  return NULL;
}



