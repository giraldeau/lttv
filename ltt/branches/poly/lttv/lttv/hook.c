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


#include <lttv/hook.h>


typedef struct _LttvHookClosure {
  LttvHook      hook;
  void         *hook_data;
  LttvHookPrio  prio;
} LttvHookClosure;

gint lttv_hooks_prio_compare(LttvHookClosure *a, LttvHookClosure *b)
{
  if(a->prio < b->prio) return -1;
  if(a->prio > b->prio) return 1;
  return 0;
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
  LttvHookClosure c;

  if(h == NULL)g_error("Null hook added");

  c.hook = f;
  c.hook_data = hook_data;
  c.prio = p;
  g_array_append_val(h,c);
  g_array_sort(h, (GCompareFunc)lttv_hooks_prio_compare);
}


void lttv_hooks_add_list(LttvHooks *h, LttvHooks *list) 
{
  guint i;

  if(list == NULL) return;
  for(i = 0 ; i < list->len; i++) {
    g_array_append_val(h,g_array_index(list, LttvHookClosure, i));
  }
  g_array_sort(h, (GCompareFunc)lttv_hooks_prio_compare);
}


void *lttv_hooks_remove(LttvHooks *h, LttvHook f)
{
  unsigned i;

  void *hook_data;

  LttvHookClosure *c;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook == f) {
      hook_data = c->hook_data;
      lttv_hooks_remove_by_position(h, i);
      return hook_data;
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
      lttv_hooks_remove_by_position(h, i);
      return;
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
      lttv_hooks_remove_by_position(h, i);
      j++;
    }
    else i++;
  }

  /* Normally the hooks in h are ordered as in list. If this is not the case,
     try harder here. */

  if(j < list->len) {
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

  if(i >= h->len)
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

  if(h != NULL) {
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
    if(c->hook(c->hook_data,call_data)) return TRUE;
  }
  return FALSE;
}

gboolean lttv_hooks_call_merge(LttvHooks *h1, void *call_data1,
                               LttvHooks *h2, void *call_data2)
{
  gboolean ret, sum_ret = FALSE;

  LttvHookClosure *c1, *c2;

  guint i, j;

  if(h1 != NULL && h2 != NULL) {
    for(i = 0, j = 0 ; i < h1->len && j < h2->len ;) {
      c1 = &g_array_index(h1, LttvHookClosure, i);
      c2 = &g_array_index(h2, LttvHookClosure, j);
      if(c1->prio <= c2->prio) {
        ret = c1->hook(c1->hook_data,call_data1);
        sum_ret = sum_ret || ret;
        i++;
      }
      else {
        ret = c2->hook(c2->hook_data,call_data2);
        sum_ret = sum_ret || ret;
        j++;
      }
    }
    /* Finish the last list with hooks left */
    for(;i < h1->len; i++) {
      c1 = &g_array_index(h1, LttvHookClosure, i);
      ret = c1->hook(c1->hook_data,call_data1);
      sum_ret = sum_ret || ret;
    }
    for(;j < h2->len; j++) {
      c2 = &g_array_index(h2, LttvHookClosure, j);
      ret = c2->hook(c2->hook_data,call_data2);
      sum_ret = sum_ret || ret;
    }
  }
  else if(h1 != NULL && h2 == NULL) {
    for(i = 0 ; i < h1->len ; i++) {
      c1 = &g_array_index(h1, LttvHookClosure, i);
      ret = c1->hook(c1->hook_data,call_data1);
      sum_ret = sum_ret || ret;
    }
  } 
  else if(h1 == NULL && h2 != NULL) {
    for(j = 0 ; j < h2->len ; j++) {
      c2 = &g_array_index(h2, LttvHookClosure, j);
      ret = c2->hook(c2->hook_data,call_data2);
      sum_ret = sum_ret || ret;
    }
  }

  return sum_ret;
}

gboolean lttv_hooks_call_check_merge(LttvHooks *h1, void *call_data1,
                                     LttvHooks *h2, void *call_data2)
{
  LttvHookClosure *c1, *c2;

  guint i, j;

  if(h1 != NULL && h2 != NULL) {
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
  }
  else if(h1 != NULL && h2 == NULL) {
    for(i = 0 ; i < h1->len ; i++) {
      c1 = &g_array_index(h1, LttvHookClosure, i);
      if(c1->hook(c1->hook_data,call_data1)) return TRUE;
    }
  } 
  else if(h1 == NULL && h2 != NULL) {
    for(j = 0 ; j < h2->len ; j++) {
      c2 = &g_array_index(h2, LttvHookClosure, j);
      if(c2->hook(c2->hook_data,call_data2)) return TRUE;
    }
  }

  return FALSE;

}


LttvHooksById *lttv_hooks_by_id_new() 
{
  return g_ptr_array_new();
}


void lttv_hooks_by_id_destroy(LttvHooksById *h) 
{
  guint i;

  for(i = 0 ; i < h->len ; i++) {
    if(h->pdata[i] != NULL) lttv_hooks_destroy((LttvHooks *)(h->pdata[i]));
  }
  g_ptr_array_free(h, TRUE);
}


LttvHooks *lttv_hooks_by_id_find(LttvHooksById *h, unsigned id)
{
  if(h->len <= id) g_ptr_array_set_size(h, id + 1);
  if(h->pdata[id] == NULL) h->pdata[id] = lttv_hooks_new();
  return h->pdata[id];
}


unsigned lttv_hooks_by_id_max_id(LttvHooksById *h)
{
  return h->len;
}


LttvHooks *lttv_hooks_by_id_get(LttvHooksById *h, unsigned id)
{
  if(id < h->len) return h->pdata[id];
  return NULL;
}


void lttv_hooks_by_id_remove(LttvHooksById *h, unsigned id)
{
  if(id < h->len && h->pdata[id] != NULL) {
    lttv_hooks_destroy((LttvHooks *)h->pdata[id]);
    h->pdata[id] = NULL;
  }
}

