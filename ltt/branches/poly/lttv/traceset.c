
#include <lttv/traceset.h>
#include <stdio.h>

/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

struct _LttvTraceset {
  char * filename;
  GPtrArray *traces;
  GPtrArray *attributes;
  LttvAttribute *a;
};


LttvTraceset *lttv_traceset_new() 
{
  LttvTraceset *s;

  s = g_new(LttvTraceset, 1);
  s->filename = NULL;
  s->traces = g_ptr_array_new();
  s->attributes = g_ptr_array_new();
  s->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  return s;
}

LttvTraceset *lttv_traceset_copy(LttvTraceset *s_orig) 
{
  int i;
  LttvTraceset *s;

  s = g_new(LttvTraceset, 1);
  s->filename = NULL;
  s->traces = g_ptr_array_new();
  for(i=0;i<s_orig->traces->len;i++)
  {
    g_ptr_array_add(
        s->traces,
	ltt_trace_copy(g_ptr_array_index(s_orig->traces, i)));
  }
  s->attributes = g_ptr_array_new();
  for(i=0;i<s_orig->attributes->len;i++)
  {
    g_ptr_array_add(
        s->attributes,
        lttv_iattribute_deep_copy(g_ptr_array_index(s_orig->attributes, i)));
  }
 
  s->a = LTTV_ATTRIBUTE(lttv_iattribute_deep_copy(LTTV_IATTRIBUTE(s_orig->a)));
  return s;
}


LttvTraceset *lttv_traceset_load(const gchar *filename)
{
  LttvTraceset *s = g_new(LttvTraceset,1);
  FILE *tf;
  
  s->filename = g_strdup(filename);
  tf = fopen(filename,"r");

  g_critical("NOT IMPLEMENTED : load traceset data from a XML file");
  
  fclose(tf);
  return s;
}

gint lttv_traceset_save(LttvTraceset *s)
{
  FILE *tf;

  tf = fopen(s->filename, "w");
  
  g_critical("NOT IMPLEMENTED : save traceset data in a XML file");

  fclose(tf);
  return 0;
}
void lttv_traceset_destroy(LttvTraceset *s) 
{
  int i, nb;

  for(i = 0 ; i < s->attributes->len ; i++) {
    g_object_unref((LttvAttribute *)s->attributes->pdata[i]);
  }
  g_ptr_array_free(s->attributes, TRUE);
  g_ptr_array_free(s->traces, TRUE);
  g_object_unref(s->a);
  g_free(s);
}

void lttv_traceset_add(LttvTraceset *s, LttTrace *t) 
{
  g_ptr_array_add(s->traces, t);
  g_ptr_array_add(s->attributes, g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
}


unsigned lttv_traceset_number(LttvTraceset *s) 
{
  return s->traces->len;
}


LttTrace *lttv_traceset_get(LttvTraceset *s, unsigned i) 
{
  g_assert(s->traces->len > i);
  return ((LttTrace *)s->traces->pdata[i]);
}


void lttv_traceset_remove(LttvTraceset *s, unsigned i) 
{
  g_ptr_array_remove_index(s->traces, i);
  g_object_unref(s->attributes->pdata[i]);
  g_ptr_array_remove_index(s->attributes,i);
}


/* A set of attributes is attached to each trace set, trace and tracefile
   to store user defined data as needed. */

LttvAttribute *lttv_traceset_attribute(LttvTraceset *s) 
{
  return s->a;
}


LttvAttribute *lttv_traceset_trace_attribute(LttvTraceset *s, unsigned i)
{
  return (LttvAttribute *)s->attributes->pdata[i];
}
