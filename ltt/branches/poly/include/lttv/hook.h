#ifndef HOOK_H
#define HOOK_H

#include <glib.h>

/* A hook is a function to call with the supplied hook data, and with 
   call site specific data (e.g., hooks for events are called with a 
   pointer to the current event). */

typedef gboolean (*LttvHook)(void *hook_data, void *call_data);


/* A list of hooks allows registering hooks to be called later. */

typedef GArray LttvHooks;


/* Create and destroy a list of hooks */

LttvHooks *lttv_hooks_new();

void lttv_hooks_destroy(LttvHooks *h);


/* Add a hook and its hook data to the list */

void lttv_hooks_add(LttvHooks *h, LttvHook f, void *hook_data);


/* Add a list of hooks to the list h */

void lttv_hooks_add(LttvHooks *h, LttvHooks *list);


/* Remove a hook from the list. Return the hook data. */

void *lttv_hooks_remove(LttvHooks *h, LttvHook f);


/* Remove a hook from the list checking that the hook data match. */

void lttv_hooks_remove_data(LttvHooks *h, LttvHook f, void *hook_data);


/* Remove a list of hooks from the hooks list in h. */

void lttv_hooks_remove_data(LttvHooks *h, LttvHook *list);


/* Return the number of hooks in the list */

unsigned lttv_hooks_number(LttvHooks *h);


/* Return the hook at the specified position in the list */

void lttv_hooks_get(LttvHooks *h, unsigned i, LttvHook *f, void **hook_data);


/* Remove the specified hook. The position of the following hooks may change */

void lttv_hooks_remove_by_position(LttvHooks *h, unsigned i);


/* Call all the hooks in the list, each with its hook data, 
   with the specified call data. Return TRUE is one hook returned TRUE. */

gboolean lttv_hooks_call(LttvHooks *h, void *call_data);


/* Call the hooks in the list until one returns true, in which case TRUE is
   returned. */

gboolean lttv_hooks_call_check(LttvHooks *h, void *call_data);


/* Sometimes different hooks need to be called based on the case. The
   case is represented by an unsigned integer id */

typedef struct _LttvHooksById LttvHooksById;


/* Create and destroy a hooks by id list */

LttvHooksById *lttv_hooks_by_id_new();

void lttv_hooks_by_id_destroy(LttvHooksById *h);


/* Obtain the hooks for a given id, creating a list if needed */

LttvHooks *lttv_hooks_by_id_find(LttvHooksById *h, unsigned id);


/* Return an id larger than any for which a list exists. */

unsigned lttv_hooks_by_id_max_id(LttvHooksById *h);


/* Get the list of hooks for an id, NULL if none exists */

LttvHooks *lttv_hooks_by_id_get(LttvHooksById *h, unsigned id);


/* Remove the list of hooks associated with an id */

void lttv_hooks_by_id_remove(LttvHooksById *h, unsigned id);

#endif // HOOK_H
