
#include <lttv/traceset.h>

/* A trace is a sequence of events gathered in the same tracing session. The
   events may be stored in several tracefiles in the same directory. 
   A trace set is defined when several traces are to be analyzed together,
   possibly to study the interactions between events in the different traces. 
*/

struct _LttvTraceset {
  GPtrArray *traces;
  GPtrArray *attributes;
  LttvAttribute *a;
};


LttvTraceset *lttv_traceset_new() 
{
  LttvTraceset *s;

  s = g_new(LttvTraceset, 1);
  s->traces = g_ptr_array_new();
  s->attributes = g_ptr_array_new();
  s->a = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  return s;
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
