
#include <lttv/lttv.h>

void lttv_analyse_init() {


}

void lttv_analyse_destroy() {
	
}


void lttv_analyse_trace_set(lttv_trace_set *s) {
  int i, nb;
  lttv_hooks *before, *after;
  lttv_attributes *a;

  a = lttv_trace_set_attributes(s);
  before = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/before");
  after = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/after");
  nb = lttv_trace_set_number(s);

  lttv_hooks_call(before, s);
  for(i = 0; i < nb; i++) {
    lttv_analyse_trace(lttv_trace_set_get(s,i));
  }
  lttv_hooks_call(after, s);
}


void lttv_analyse_trace(lttv_trace *t) {
  int i, nb_all_cpu, nb_per_cpu;
  lttv_hooks *before, *after;
  lttv_attributes *a;

  a = lttv_trace_attributes(t);
  before = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/before");
  after = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/after");

  nb_all_cpu = lttv_trace_tracefile_number_all_cpu(t);
  nb_per_cpu = lttv_trace_tracefile_number_per_cpu(t);

  lttv_hooks_call(before, t);

  for(i = 0; i < nb_all_cpu; i++) {
    lttv_analyse_tracefile(lttv_trace_get_all_cpu(t,i));
  }

  for(i = 0; i < nb_per_cpu; i++) {
    lttv_analyse_tracefile(lttv_trace_get_per_cpu(t,i));
  }

  lttv_hooks_call(after, t);
}


void lttv_analyse_tracefile(lttv_tracefile *t) {
  ltt_tracefile *tf;
  ltt_event *event;
  unsigned id;
  lttv_hooks *before, *after, *event_hooks;
  lttv_hooks_by_id *event_hooks_by_id;
  lttv_attributes *a;

  a = lttv_tracefile_attributes(t);
  before = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/before");
  after = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,"hooks/after");
  event_hooks = (lttv_hooks*)lttv_attributes_get_pointer_pathname(a,
      "hooks/event");
  event_hooks_by_id = (lttv_hooks_by_id*)
      lttv_attributes_get_pointer_pathname(a, "hooks/eventid");
  
  tf = lttv_tracefile_ltt_tracefile(t);

  lttv_hooks_call(before, t);

  if(lttv_hooks_number(hooks_event) != 0 || 
     lttv_hooks_by_id_number(event_hook_by_id) != 0){
    while(event = ltt_tracefile_read(tf) != NULL) {
      lttv_hooks_call(event_hooks,event);
      lttv_hooks_by_id_call(event_hooks_by_id,event,ltt_event_type_id(event));
    }
  }

  lttv_hooks_call(after, t);

}


