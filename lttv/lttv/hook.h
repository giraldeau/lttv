/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * 25/05/2004 Mathieu Desnoyers : Hook priorities
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifndef HOOK_H
#define HOOK_H

#include <glib.h>
#include <ltt/compiler.h>

/* A hook is a function to call with the supplied hook data, and with 
   call site specific data (e.g., hooks for events are called with a 
   pointer to the current event). */

typedef gboolean (*LttvHook)(void *hook_data, void *call_data);


/* A list of hooks allows registering hooks to be called later. */

typedef GArray LttvHooks;

/* A priority associated with each hook, from -19 (high prio) to 20 (low prio)
 * 0 being the default priority.
 *
 * Priority ordering is done in the lttv_hooks_add and lttv_hooks_add_list 
 * functions. Hook removal does not change list order.
 */

#define LTTV_PRIO_DEFAULT 50
#define LTTV_PRIO_HIGH 0
#define LTTV_PRIO_LOW 99

typedef gint LttvHookPrio;

/* Create and destroy a list of hooks */

LttvHooks *lttv_hooks_new();

void lttv_hooks_destroy(LttvHooks *h);


/* Add a hook and its hook data to the list */

void lttv_hooks_add(LttvHooks *h, LttvHook f, void *hook_data, LttvHookPrio p);


/* Add a list of hooks to the list h */

void lttv_hooks_add_list(LttvHooks *h, const LttvHooks *list);


/* Remove a hook from the list. Return the hook data. */

void *lttv_hooks_remove(LttvHooks *h, LttvHook f);


/* Remove a hook from the list checking that the hook data match. */

void lttv_hooks_remove_data(LttvHooks *h, LttvHook f, void *hook_data);


/* Remove a list of hooks from the hooks list in h. */

void lttv_hooks_remove_list(LttvHooks *h, LttvHooks *list);


/* Return the number of hooks in the list */

unsigned lttv_hooks_number(LttvHooks *h);


/* Return the hook at the specified position in the list.
 * *f and *hook_data are NULL if no hook exists at that position. */

void lttv_hooks_get(LttvHooks *h, unsigned i, LttvHook *f, void **hook_data,
                                              LttvHookPrio *p);


/* Remove the specified hook. The position of the following hooks may change */
/* The hook is removed from the list event if its ref_count is higher than 1 */

void lttv_hooks_remove_by_position(LttvHooks *h, unsigned i);


/* Call all the hooks in the list, each with its hook data, 
   with the specified call data, in priority order. Return TRUE if one hook
   returned TRUE. */

gboolean lttv_hooks_call(LttvHooks *h, void *call_data);


/* Call the hooks in the list in priority order until one returns true,
 * in which case TRUE is returned. */

gboolean lttv_hooks_call_check(LttvHooks *h, void *call_data);


/* Call hooks from two lists in priority order. If priority is the same,
 * hooks from h1 are called first. */

gboolean lttv_hooks_call_merge(LttvHooks *h1, void *call_data1,
                               LttvHooks *h2, void *call_data2);

gboolean lttv_hooks_call_check_merge(LttvHooks *h1, void *call_data1,
                                     LttvHooks *h2, void *call_data2);

/* Sometimes different hooks need to be called based on the case. The
   case is represented by an unsigned integer id */

typedef struct _LttvHooksById {
  GPtrArray *index;
  GArray *array;
} LttvHooksById;

/* Create and destroy a hooks by id list */

LttvHooksById *lttv_hooks_by_id_new(void);

void lttv_hooks_by_id_destroy(LttvHooksById *h);


/* Obtain the hooks for a given id, creating a list if needed */

LttvHooks *lttv_hooks_by_id_find(LttvHooksById *h, unsigned id);


/* Return an id larger than any for which a list exists. */

unsigned lttv_hooks_by_id_max_id(LttvHooksById *h);


/* Get the list of hooks for an id, NULL if none exists */

static inline LttvHooks *lttv_hooks_by_id_get(LttvHooksById *h, unsigned id)
{
  LttvHooks *ret;
  if(likely(id < h->index->len)) ret = h->index->pdata[id];
  else ret = NULL;

  return ret;
}

/* Remove the list of hooks associated with an id */

void lttv_hooks_by_id_remove(LttvHooksById *h, unsigned id);

void lttv_hooks_by_id_copy(LttvHooksById *dest, LttvHooksById *src);

/*
 * Hooks per channel per id. Useful for GUI to save/restore hooks
 * on a per trace basis (rather than per tracefile).
 */

/* Internal structure, contained in by the LttvHooksByIdChannelArray */
typedef struct _LttvHooksByIdChannel {
  LttvHooksById *hooks_by_id;
  GQuark channel;
} LttvHooksByIdChannel;

typedef struct _LttvHooksByIdChannelArray {
  GArray *array;	/* Array of LttvHooksByIdChannel */
} LttvHooksByIdChannelArray;

LttvHooksByIdChannelArray *lttv_hooks_by_id_channel_new(void);

void lttv_hooks_by_id_channel_destroy(LttvHooksByIdChannelArray *h);

LttvHooks *lttv_hooks_by_id_channel_find(LttvHooksByIdChannelArray *h,
    GQuark channel, guint16 id);

#endif // HOOK_H
