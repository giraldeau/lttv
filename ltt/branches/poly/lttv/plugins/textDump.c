
typedef struct _text_hook_data {
  FILE *fp;
  lttv_string *s;
} text_hook_data;

void *lttv_textDump_before(lttv_trace_set *s, FILE *fp)
{
  int i, j, nb_trace, nb_tracefile;
  lttv_attributes *a;
  lttv_hooks *h;
  lttv_trace *t;
  lttv_tracefile *tf;
  text_hook_data *hook_data;

  hook_data = g_new(ltt_hook_data,1);
  nb_trace = lttv_trace_set_number(s);
  hook_data->fp = fp;
  hook_data->s = lttv_string_new;

  for(i = 0 ; i < nb_trace ; i++) {
    t = lttv_trace_set_get(s,i);
    a = lttv_trace_attributes(t);
    h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/before");
    lttv_hooks_add(h, print_trace_title, hook_data);
    nb_tracefile = lttv_trace_number(t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/before");
      lttv_hooks_add(h, print_tracefile_title, hook_data);
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/event");
      lttv_hooks_add(h, print_event, hook_data);
    }
  }
}

void lttv_textDump_after(lttv_trace_set *ts, void *hook_data)
{
  int i, j, nb_trace, nb_tracefile;
  lttv_attributes *a;
  lttv_hooks *h;
  lttv_trace *t;
  lttv_tracefile *tf;

  nb_trace = lttv_trace_set_number(s);

  for(i = 0 ; i < nb_trace ; i++) {
    t = lttv_trace_set_get(s,i);
    a = lttv_trace_attributes(t);
    h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/before");
    lttv_hooks_remove(h, print_trace_title, hook_data);
    nb_tracefile = lttv_trace_number(t);

    for(j = 0 ; j < nb_tracefile ; j++) {
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/before");
      lttv_hooks_remove(h, print_tracefile_title, hook_data);
      h = (lttv_hooks *)lttv_attributes_get_pointer_pathname(a,"hooks/event");
      lttv_hooks_remove(h, print_event, hook_data);
    }
  }
  lttv_string_destroy(hook_data->s);
  g_free(hook_data);
}

static void print_trace_title(void *hook_data, void *call_data)
{
  lttv_trace *t;
  FILE *fp;

  fp = ((text_hook_data *)hook_data)->fp;  
  t = (lttv_trace *)call_data;
  fprintf(fp,"\n\nTrace %s:\n\n" lttv_trace_name(t));
}

static void print_trace(void *hook_data, void *call_data)
{
  lttv_tracefile *tf;
  FILE *fp;

  fp = ((text_hook_data *)hook_data)->fp;  
  tf = (lttv_tracefile *)call_data;
  fprintf(fp,"\n\nTracefile %s:\n\n" lttv_tracefile_name(tf));
}

static void print_event(void *hook_data, void *call_data)
{
  ltt_event *e;
  FILE *fp;
  text_hook_data *d;

  d = ((text_hook_data *)hook_data;
  e = (lttv_event *)call_data;
  lttv_event_to_string(e,d->s,TRUE);
  fprintf(fp,"%s\n" d->s->str);
}



