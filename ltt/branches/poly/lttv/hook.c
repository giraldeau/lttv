#include <lttv/hook.h>


typedef struct _lttv_hook_closure {
  lttv_hook hook;
  void *hook_data;
} lttv_hook_closure;


inline lttv_hooks *lttv_hooks_new() {
  return g_array_new(FALSE, FALSE, sizeof(lttv_hook_closure));
}

/* MD: delete elements of the array also, but don't free pointed addresses
 * (functions).
 */
inline void lttv_hooks_destroy(lttv_hooks *h) {
  g_array_free(h, TRUE);
}

inline void lttv_hooks_add(lttv_hooks *h, lttv_hook f, void *hook_data) {
  lttv_hook_closure c;

  /* if h is null, do nothing, or popup a warning message */
  if(h == NULL)return;

  c.hook = f;
  c.hook_data = hook_data;
  g_array_append_val(h,c);
}


void lttv_hooks_call(lttv_hooks *h, void *call_data)
{
  int i;
  lttv_hook_closure * c;

  for(i = 0 ; i < h->len ; i++) {
    c = ((lttv_hook_closure *)h->data) + i;
    if(c != NULL)
	    c->hook(c->hook_data,call_data);
    else
	    g_critical("NULL hook called\n");
  }
}


/* Sometimes different hooks need to be called based on the case. The
   case is represented by an unsigned integer id and may represent different
   event types, for instance. */

inline lttv_hooks_by_id *lttv_hooks_by_id_new() {
  return g_ptr_array_new();
}


inline void lttv_hooks_by_id_destroy(lttv_hooks_by_id *h) {
  /* MD: delete elements of the array also */
  g_ptr_array_free(h, TRUE);
}


void lttv_hooks_by_id_add(lttv_hooks_by_id *h, lttv_hook f, void *hook_data, 
    unsigned int id)
{
  if(h->len < id) g_ptr_array_set_size(h, id);
  if(h->pdata[id] == NULL) h->pdata[id] = lttv_hooks_new();
  lttv_hooks_add(h->pdata[id], f, hook_data);
}

void lttv_hooks_by_id_call(lttv_hooks_by_id *h, void *call_data, unsigned int id)
{
  if(h->len <= id || h->pdata[id] == NULL) return;
  lttv_hooks_call(h->pdata[id],call_data);
}

