/* This module inserts a hook in the program main loop. This hook processes 
   all the events in the main tracefile. */


#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>

static void process_trace_set(void *hook_data, void *call_data)
{
  int i, nb;
  lttv_attributes *a, *sa;
  lttv_trace_set *s;
  lttv_hooks *global_before, *before, *global_after, *after;
  ltt_time start, end;
  lttv_key *key;
  lttv_hook f;
  void *hook_data;

  a = lttv_global_attributes();
  global_before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/before");
  global_after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,
      "hooks/trace_set/after");
  s = (lttv_trace_set *)lttv_attributes_get_pointer_pathname(a,
      "traceSet/main");

  key = lttv_key_new_pathname("time/start");
  start = lttv_attributes_get_time(a,key);
  lttv_key_destroy(key);

  key = lttv_key_new_pathname("time/end");
  end = lttv_attributes_get_time(a,key);
  lttv_key_destroy(key);

  sa = lttv_trace_set_attributes(s);

  before = (lttv_hooks *)lttv_attributes_get_pointer_pathname(sa,
      "hooks/before");
  if(before == NULL) {
    before = lttv_hooks_new();
    lttv_attributes_set_pointer_pathname(sa, "hooks/before", before);
  }

  after = (lttv_hooks *)lttv_attributes_get_pointer_pathname(sa,
      "hooks/after");
  if(after == NULL) {
    after = lttv_hooks_new();
    lttv_attributes_set_pointer_pathname(sa, "hooks/after", after);
  }

  nb = lttv_hooks_number(global_before);
  for(i = 0 ; i < nb ; i++) {
    lttv_hooks_get(global_before, i, &f, &hook_data);
    lttv_hooks_add(before, f, hook_data);
  }

  nb = lttv_hooks_number(global_after);
  for(i = 0 ; i < nb ; i++) {
    lttv_hooks_get(global_after, i, &f, &hook_data);
    lttv_hooks_add(after, f, hook_data);
  }

  lttv_trace_set_process(s, before_trace_set, after_trace_set, filter, start,
      end);

  nb = lttv_hooks_number(global_before);
  for(i = 0 ; i < nb ; i++) {
    lttv_hooks_get(global_before, i, &f, &hook_data);
    lttv_hooks_remove(before, f, hook_data);
  }

  nb = lttv_hooks_number(global_after);
  for(i = 0 ; i < nb ; i++) {
    lttv_hooks_get(global_after, i, &f, &hook_data);
    lttv_hooks_remove(after, f, hook_data);
  }
}


void init(int argc, char **argv)
{
  lttv_attributes *a;
  lttv_hooks *h;

  a = lttv_global_attributes();
  h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/main");
  lttv_hooks_add(h, process_trace_set, NULL);
}


void destroy()
{
  lttv_attributes *a;
  lttv_hooks *h;

  a = lttv_global_attributes();
  h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/main");
  lttv_hooks_remove(h, process_trace_set, NULL);
}




