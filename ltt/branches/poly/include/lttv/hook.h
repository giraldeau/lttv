#ifndef HOOK_H
#define HOOK_H

#include <glib.h>

/* A hook is a function to call with the supplied hook data, and with 
   call site specific data (e.g., hooks for events are called with a 
   pointer to the current event). */
// MD compile fix: int instead of bool as return value
typedef int (*lttv_hook)(void *hook_data, void *call_data);


/* A list of hooks allows registering hooks to be called later. */

typedef GArray _lttv_hooks;
typedef _lttv_hooks lttv_hooks;

lttv_hooks *lttv_hooks_new();

void lttv_hooks_destroy(lttv_hooks *h);

void lttv_hooks_add(lttv_hooks *h, lttv_hook f, void *hook_data);

void lttv_hooks_remove(lttv_hooks *h, lttv_hook f, void *hook_data);

unsigned lttv_hooks_number(lttv_hooks *h);

void lttv_hooks_get(lttv_hooks *h, unsigned i, lttv_hook *f, void **hook_data);

void lttv_hooks_remove_by_position(lttv_hooks *h, unsigned i);

int lttv_hooks_call(lttv_hooks *h, void *call_data);

int lttv_hooks_call_check(lttv_hooks *h, void *call_data);


/* Sometimes different hooks need to be called based on the case. The
   case is represented by an unsigned integer id and may represent different
   event types, for instance. */

typedef GPtrArray _lttv_hooks_by_id;
typedef _lttv_hooks_by_id lttv_hooks_by_id;

lttv_hooks_by_id *lttv_hooks_by_id_new();

void lttv_hooks_by_id_destroy(lttv_hooks_by_id *h);

void lttv_hooks_by_id_add(lttv_hooks_by_id *h, lttv_hook f, void *hook_data, 
    unsigned int id);

void lttv_hooks_by_id_call(lttv_hooks_by_id *h, void *call_data, unsigned int id);

#endif // HOOK_H
