/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/hook.h>
#include <ltt/compiler.h>
#include <ltt/ltt.h>

typedef struct _LttvHookClosure {
  LttvHook      hook;
  void         *hook_data;
  LttvHookPrio  prio;
  guint         ref_count;
} LttvHookClosure;

gint lttv_hooks_prio_compare(LttvHookClosure *a, LttvHookClosure *b)
{
  gint ret=0;
  if(a->prio < b->prio) ret = -1;
  else if(a->prio > b->prio) ret = 1;
  return ret;
}


LttvHooks *lttv_hooks_new() 
{
  return g_array_new(FALSE, FALSE, sizeof(LttvHookClosure));
}


void lttv_hooks_destroy(LttvHooks *h) 
{
  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "lttv_hooks_destroy()");
  g_array_free(h, TRUE);
}


void lttv_hooks_add(LttvHooks *h, LttvHook f, void *hook_data, LttvHookPrio p) 
{
  LttvHookClosure *c, new_c;
  guint i;
  
  if(unlikely(h == NULL))g_error("Null hook added");

  new_c.hook = f;
  new_c.hook_data = hook_data;
  new_c.prio = p;
  new_c.ref_count = 1;

  /* Preliminary check for duplication */
  /* only hook and hook data is checked */
  for(i = 0; i < h->len; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(new_c.hook == c->hook && new_c.hook_data == c->hook_data) {
      g_assert(new_c.prio == c->prio);
      c->ref_count++;
      return;
    }
  }


  for(i = 0; i < h->len; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(new_c.prio < c->prio) {
      g_array_insert_val(h,i,new_c);
      return;
    }
  }
  if(i == h->len)
    g_array_append_val(h,new_c);
}

/* lttv_hooks_add_list
 *
 * Adds a sorted list into another sorted list.
 *
 * Note : h->len is modified, but only incremented. This assures
 * its coherence through the function.
 *
 * j is an index to the element following the last one added in the
 * destination array.
 */
void lttv_hooks_add_list(LttvHooks *h, const LttvHooks *list) 
{
  guint i,j,k;
  LttvHookClosure *c;
  const LttvHookClosure *new_c;

  if(unlikely(list == NULL)) return;

  for(i = 0, j = 0 ; i < list->len; i++) {
    new_c = &g_array_index(list, LttvHookClosure, i);
    gboolean found=FALSE;

    /* Preliminary check for duplication */
    /* only hook and hook data is checked, not priority */
    for(k = 0; k < h->len; k++) {
      c = &g_array_index(h, LttvHookClosure, k);
      if(new_c->hook == c->hook && new_c->hook_data == c->hook_data) {
        /* Found another identical entry : increment its ref_count and
         * jump over the source index */
        g_assert(new_c->prio == c->prio);
        found=TRUE;
        c->ref_count++;
        break;
      }
    }

    if(!found) {
      /* If not found, add it to the destination array */
      while(j < h->len) {
        c = &g_array_index(h, LttvHookClosure, j);
        if(new_c->prio < c->prio) {
          g_array_insert_val(h,j,*new_c);
          j++;
          break;
        }
        else j++;
      }
      if(j == h->len) {
        g_array_append_val(h,*new_c);
        j++;
      }
    }
  }
}


void *lttv_hooks_remove(LttvHooks *h, LttvHook f)
{
  unsigned i;

  void *hook_data;

  LttvHookClosure *c;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook == f) {
      if(c->ref_count == 1) {
        hook_data = c->hook_data;
        lttv_hooks_remove_by_position(h, i);
        return hook_data;
      } else {
        g_assert(c->ref_count != 0);
        c->ref_count--;
        return NULL;  /* We do not want anyone to free a hook_data 
                         still referenced */
      }
    }
  }
  return NULL;
}


void lttv_hooks_remove_data(LttvHooks *h, LttvHook f, void *hook_data)
{
  unsigned i;

  LttvHookClosure *c;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook == f && c->hook_data == hook_data) {
      if(c->ref_count == 1) {
        lttv_hooks_remove_by_position(h, i);
        return;
      } else {
        g_assert(c->ref_count != 0);
        c->ref_count--;
        return;
      }
    }
  }
}


void lttv_hooks_remove_list(LttvHooks *h, LttvHooks *list)
{
  guint i, j;

  LttvHookClosure *c, *c_list;

  if(list == NULL) return;
  for(i = 0, j = 0 ; i < h->len && j < list->len ;) {
    c = &g_array_index(h, LttvHookClosure, i);
    c_list = &g_array_index(list, LttvHookClosure, j);
    if(c->hook == c_list->hook && c->hook_data == c_list->hook_data) {
      if(c->ref_count == 1) {
        lttv_hooks_remove_by_position(h, i);
      } else {
        g_assert(c->ref_count != 0);
        c->ref_count--;
      }
      j++;
    }
    else i++;
  }

  /* Normally the hooks in h are ordered as in list. If this is not the case,
     try harder here. */

  if(unlikely(j < list->len)) {
    for(; j < list->len ; j++) {
      c_list = &g_array_index(list, LttvHookClosure, j);
      lttv_hooks_remove_data(h, c_list->hook, c_list->hook_data);
    }
  }
}


unsigned lttv_hooks_number(LttvHooks *h)
{
  return h->len;
}


void lttv_hooks_get(LttvHooks *h, unsigned i, LttvHook *f, void **hook_data,
                                              LttvHookPrio *p)
{
  LttvHookClosure *c;

  if(unlikely(i >= h->len))
  {
    *f = NULL;
    *hook_data = NULL;
    *p = 0;
    return;
  }
  
  c = &g_array_index(h, LttvHookClosure, i);
  *f = c->hook;
  *hook_data = c->hook_data;
  *p = c->prio;
}


void lttv_hooks_remove_by_position(LttvHooks *h, unsigned i)
{
  g_array_remove_index(h, i);
}

gboolean lttv_hooks_call(LttvHooks *h, void *call_data)
{
  gboolean ret, sum_ret = FALSE;

  LttvHookClosure *c;

  guint i;

  if(likely(h != NULL)) {
    for(i = 0 ; i < h->len ; i++) {
      c = &g_array_index(h, LttvHookClosure, i);
      ret = c->hook(c->hook_data,call_data);
      sum_ret = sum_ret || ret;
    }
  }
  return sum_ret;
}


gboolean lttv_hooks_call_check(LttvHooks *h, void *call_data)
{
  LttvHookClosure *c;

  guint i;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(unlikely(c->hook(c->hook_data,call_data))) return TRUE;
  }
  return FALSE;
}

/* Optimised for h1 == NULL, h2 != NULL. This is the case
 * for optimised computation (with specific by id hooks, but
 * no main hooks).
 *
 * The second case that should occur the most often is
 * h1 != NULL , h2 == NULL.
 */
gint lttv_hooks_call_merge(LttvHooks *h1, void *call_data1,
                               LttvHooks *h2, void *call_data2)
{
  gint ret, sum_ret = 0;

  LttvHookClosure *c1, *c2;

  guint i, j;

  if(unlikely(h1 != NULL)) {
    if(unlikely(h2 != NULL)) {
      for(i = 0, j = 0 ; i < h1->len && j < h2->len ;) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        c2 = &g_array_index(h2, LttvHookClosure, j);
        if(c1->prio <= c2->prio) {
          ret = c1->hook(c1->hook_data,call_data1);
          sum_ret = sum_ret | ret;
          i++;
        }
        else {
          ret = c2->hook(c2->hook_data,call_data2);
          sum_ret = sum_ret | ret;
          j++;
        }
      }
      /* Finish the last list with hooks left */
      for(;i < h1->len; i++) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        ret = c1->hook(c1->hook_data,call_data1);
        sum_ret = sum_ret | ret;
      }
      for(;j < h2->len; j++) {
        c2 = &g_array_index(h2, LttvHookClosure, j);
        ret = c2->hook(c2->hook_data,call_data2);
        sum_ret = sum_ret | ret;
      }
    } else {  /* h1 != NULL && h2 == NULL */
      for(i = 0 ; i < h1->len ; i++) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        ret = c1->hook(c1->hook_data,call_data1);
        sum_ret = sum_ret | ret;
      }
    }
  } else if(likely(h2 != NULL)) { /* h1 == NULL && h2 != NULL */
     for(j = 0 ; j < h2->len ; j++) {
      c2 = &g_array_index(h2, LttvHookClosure, j);
      ret = c2->hook(c2->hook_data,call_data2);
      sum_ret = sum_ret | ret;
    }
  }

  return sum_ret;
}

gboolean lttv_hooks_call_check_merge(LttvHooks *h1, void *call_data1,
                                     LttvHooks *h2, void *call_data2)
{
  LttvHookClosure *c1, *c2;

  guint i, j;

  if(unlikely(h1 != NULL)) {
    if(unlikely(h2 != NULL)) {
      for(i = 0, j = 0 ; i < h1->len && j < h2->len ;) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        c2 = &g_array_index(h2, LttvHookClosure, j);
        if(c1->prio <= c2->prio) {
          if(c1->hook(c1->hook_data,call_data1)) return TRUE;
            i++;
        }
        else {
          if(c2->hook(c2->hook_data,call_data2)) return TRUE;
          j++;
        }
      }
      /* Finish the last list with hooks left */
      for(;i < h1->len; i++) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        if(c1->hook(c1->hook_data,call_data1)) return TRUE;
      }
      for(;j < h2->len; j++) {
        c2 = &g_array_index(h2, LttvHookClosure, j);
        if(c2->hook(c2->hook_data,call_data2)) return TRUE;
      }
    } else { /* h2 == NULL && h1 != NULL */
      for(i = 0 ; i < h1->len ; i++) {
        c1 = &g_array_index(h1, LttvHookClosure, i);
        if(c1->hook(c1->hook_data,call_data1)) return TRUE;
      }
    }
  } else if(likely(h2 != NULL)) { /* h1 == NULL && h2 != NULL */
    for(j = 0 ; j < h2->len ; j++) {
      c2 = &g_array_index(h2, LttvHookClosure, j);
      if(c2->hook(c2->hook_data,call_data2)) return TRUE;
    }
  }
   
  return FALSE;

}

/* Two pointer arrays : 
 * * one indexed by id for quick search : 
 *  size : max id
 *  typically 4 bytes * 256 facilities * 10 events = 10kbytes
 * * another array that keeps a list of used numbers (for later deletion)
 *  size : number of ids used.
 */

LttvHooksById *lttv_hooks_by_id_new() 
{
  LttvHooksById *h = g_new(LttvHooksById, 1);
  h->index = g_ptr_array_sized_new(PREALLOC_EVENTS);
  h->array = g_array_sized_new(FALSE, FALSE, sizeof(guint), PREALLOC_EVENTS);
  return h;
}


void lttv_hooks_by_id_destroy(LttvHooksById *h) 
{
  guint i;

  for(i = 0 ; i < h->array->len ; i++) {
    guint index = g_array_index(h->array, guint, i);
    if(h->index->pdata[index] != NULL) { /* hook may have been removed */
      lttv_hooks_destroy(h->index->pdata[index]);
      h->index->pdata[index] = NULL;  /* Must be there in case of 
                                         multiple addition of the same index */
    }
  }
  g_ptr_array_free(h->index, TRUE);
  g_array_free(h->array, TRUE);
}

/* Optimised for searching an existing hook */
LttvHooks *lttv_hooks_by_id_find(LttvHooksById *h, unsigned id)
{
  if(unlikely(h->index->len <= id)) g_ptr_array_set_size(h->index, id + 1);
  if(unlikely(h->index->pdata[id] == NULL)) {
    h->index->pdata[id] = lttv_hooks_new();
    g_array_append_val(h->array, id);
  }
  return h->index->pdata[id];
}


unsigned lttv_hooks_by_id_max_id(LttvHooksById *h)
{
  return h->index->len;
}

/* We don't bother removing the used slot array id : lttv_hooks_by_id_destroy is
 * almost never called and is able to deal with used slot repetition. */
void lttv_hooks_by_id_remove(LttvHooksById *h, unsigned id)
{
  if(likely(id < h->index->len && h->index->pdata[id] != NULL)) {
    lttv_hooks_destroy((LttvHooks *)h->index->pdata[id]);
    h->index->pdata[id] = NULL;
  }
}

void lttv_hooks_by_id_copy(LttvHooksById *dest, LttvHooksById *src)
{
  guint i;

  for(i = 0 ; i < src->array->len ; i++) {
    guint index = g_array_index(src->array, guint, i);
    LttvHooks *srch = lttv_hooks_by_id_find(src, index);
    LttvHooks *desth = lttv_hooks_by_id_find(dest, index);
    lttv_hooks_add_list(desth, srch);
  }
}

LttvHooksByIdChannelArray *lttv_hooks_by_id_channel_new(void)
{
  LttvHooksByIdChannelArray *h = g_new(LttvHooksByIdChannelArray, 1);

  h->array = g_array_new(FALSE, FALSE, sizeof(LttvHooksByIdChannel));
  return h;
}

void lttv_hooks_by_id_channel_destroy(LttvHooksByIdChannelArray *h)
{
  LttvHooksByIdChannel *hid;
  int i;

  for (i = 0; i < h->array->len; i++) {
    hid = &g_array_index(h->array, LttvHooksByIdChannel, i);
    lttv_hooks_by_id_destroy(hid->hooks_by_id);
  }
  g_array_free(h->array, TRUE);
  g_free(h);
}

static LttvHooksByIdChannel *lttv_hooks_by_id_channel_find_channel(
    LttvHooksByIdChannelArray *h,
    GQuark channel)
{
  LttvHooksByIdChannel *hid;
  int i, found = 0;

  for (i = 0; i < h->array->len; i++) {
    hid = &g_array_index(h->array, LttvHooksByIdChannel, i);
    if (hid->channel == channel)
      found = 1;
      break;
  }
  if (!found) {
    g_array_set_size(h->array, h->array->len + 1);
    hid = &g_array_index(h->array, LttvHooksByIdChannel, h->array->len - 1);
    hid->channel = channel;
    hid->hooks_by_id = lttv_hooks_by_id_new();
  }
  return hid;
}

/* get, or create if not found */
LttvHooks *lttv_hooks_by_id_channel_find(LttvHooksByIdChannelArray *h,
    GQuark channel, guint16 id)
{
  LttvHooksByIdChannel *hid;
  hid = lttv_hooks_by_id_channel_find_channel(h, channel);
  return lttv_hooks_by_id_find(hid->hooks_by_id, id);
}
