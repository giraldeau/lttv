/* This file is part of the Linux Trace Toolkit Graphic User Interface
 * Copyright (C) 2003-2004 Mathieu Desnoyers
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

/* This file is the API used to launch any background computation on a trace */

/* Here is the implementation of the API */

#include <ltt/time.h>
#include <ltt/trace.h>
#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/traceset.h>
#include <lttv/attribute.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/lttvwindowtraces.h>


typedef struct _BackgroundRequest {
  LttvAttributeName module_name; /* Hook path in global attributes,
                                    where all standard hooks under computation/.
                                    i.e. modulename */
  LttvTrace *trace; /* trace concerned */
} BackgroundRequest;

typedef struct _BackgroundNotify {
  gpointer                     owner;
  LttvTrace                   *trace; /* trace */
  LttTime                      notify_time;
  LttvTracesetContextPosition *notify_position;
  LttvHooks                   *notify; /* Hook to call when the notify is
                                          passed, or at the end of trace */
} BackgroundNotify;



/* Get a trace by its path name. 
 *
 * @param path path of the trace on the virtual file system.
 * @return Pointer to trace if found
 *         NULL is returned if the trace is not present
 */

LttvTrace *lttvwindowtraces_get_trace_by_name(gchar *path)
{
  LttvAttribute *attribute = lttv_global_attributes();
  guint i;

  for(i=0;i<lttvwindowtraces_get_number();i++) {
    LttvTrace *trace_v = lttvwindowtraces_get_trace(i);
    LttTrace *trace;
    gchar *name;
    
    g_assert(trace_v != NULL);

    trace = lttv_trace(trace_v);
    g_assert(trace != NULL);
    name = ltt_trace_name(trace);

    if(strcmp(name, path) == 0) {
      /* Found */
      return trace_v;
    }
  }
  
  return NULL;
}

/* Get a trace by its number identifier */

LttvTrace *lttvwindowtraces_get_trace(guint num)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;
  LttvAttributeType type;
  LttvAttributeName name;
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_TRACES)));
  
  type = lttv_iattribute_get(LTTV_IATTRIBUTE(attribute), num, &name, &value);

  if(type == LTTV_POINTER) {
    return (LttvTrace *)*(value.v_pointer);
  }

  return NULL;
}

/* Total number of traces */

guint lttvwindowtraces_get_number()
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_TRACES)));
 
  return ( lttv_iattribute_get_number(LTTV_IATTRIBUTE(attribute)) );
}

/* Add a trace to the global attributes */

void lttvwindowtraces_add_trace(LttvTrace *trace)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;
  LttvAttributeValue value;
  guint num;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_TRACES)));
  num = lttv_attribute_get_number(attribute);
 
  value = lttv_attribute_add(attribute,
                     num,
                     LTTV_POINTER);

  *(value.v_pointer) = (gpointer)trace;
}

/* Remove a trace from the global attributes */

void lttvwindowtraces_remove_trace(LttvTrace *trace)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;
  LttvAttributeValue value;
  guint i;

  g_assert(attribute =
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_TRACES)));

  for(i=0;i<lttvwindowtraces_get_number();i++) {
    LttvTrace *trace_v = lttvwindowtraces_get_trace(i);
    
    g_assert(trace_v != NULL);

    if(trace_v == trace) {
      /* Found */
      lttv_attribute_remove(attribute, i);
      return;
    }
  }
}


/**
 * Function to request data from a specific trace
 *
 * The memory allocated for the request will be managed by the API.
 * 
 * @param trace the trace to compute
 * @param module_name the name of the module which registered global computation
 *                    hooks.
 */

void lttvwindowtraces_background_request_queue
                     (LttvTrace *trace, gchar *module_name)
{
  BackgroundRequest *bg_req;
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;
  GSList **slist;
  guint num;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_REQUESTS_QUEUE,
                                LTTV_POINTER,
                                &value));
  slist = (GSList**)(value.v_pointer);
  
  bg_req = g_new(BackgroundRequest,1);
  bg_req->module_name = g_quark_from_string(module_name);
  bg_req->trace = trace;

  *slist = g_slist_append(*slist, bg_req);
}

/**
 * Remove a background request from a trace.
 *
 * This should ONLY be used by the modules which registered the global hooks
 * (module_name). If this is called by the viewers, it may lead to incomplete
 * and incoherent background processing information.
 *
 * Even if the module which deals with the hooks removes the background
 * requests, it may cause a problem if the module gets loaded again in the
 * session : the data will be partially calculated. The calculation function
 * must deal with this case correctly.
 * 
 * @param trace the trace to compute
 * @param module_name the name of the module which registered global computation
 *                    hooks.
 */

void lttvwindowtraces_background_request_remove
                     (LttvTrace *trace, gchar *module_name)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;
  GSList *iter = NULL;
  GSList **slist;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_REQUESTS_QUEUE,
                                LTTV_POINTER,
                                &value));
  slist = (GSList**)(value.v_pointer);

  for(iter=*slist;iter!=NULL;) {
    BackgroundRequest *bg_req = 
              (BackgroundRequest *)iter->data;

    if(bg_req->module_name == g_quark_from_string(module_name)) {
      GSList *rem_iter = iter;
      iter=g_slist_next(iter);
      g_free(bg_req); 
      *slist = g_slist_delete_link(*slist, rem_iter);
    } else {
      iter=g_slist_next(iter);
    }
  }
}
 
 
/**
 * Register a callback to be called when requested data is passed in the next
 * queued background processing.
 * 
 * @param owner owner of the background notification
 * @param trace the trace computed
 * @param notify_time time when notification hooks must be called
 * @param notify_position position when notification hooks must be called
 * @param notify  Hook to call when the notify position is passed
 */

void lttvwindowtraces_background_notify_queue
 (gpointer                     owner,
  LttvTrace                   *trace,
  LttTime                      notify_time,
  const LttvTracesetContextPosition *notify_position,
  const LttvHooks                   *notify)
{
  BackgroundNotify *bg_notify;
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;
  GSList **slist;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_NOTIFY_QUEUE,
                                LTTV_POINTER,
                                &value));
  slist = (GSList**)(value.v_pointer);
 

  bg_notify = g_new(BackgroundNotify,1);

  bg_notify->owner = owner;
  bg_notify->trace = trace;
  bg_notify->notify_time = notify_time;
  bg_notify->notify_position = ltt_traceset_context_position_new();
  lttv_traceset_context_position_copy(bg_notify->notify_position,
                                      notify_position);
  bg_notify->notify = lttv_hooks_new();
  lttv_hooks_add_list(bg_notify->notify, notify);

  *slist = g_slist_append(*slist, bg_notify);
}

/**
 * Register a callback to be called when requested data is passed in the current
 * background processing.
 * 
 * @param owner owner of the background notification
 * @param trace the trace computed
 * @param notify_time time when notification hooks must be called
 * @param notify_position position when notification hooks must be called
 * @param notify  Hook to call when the notify position is passed
 */

void lttvwindowtraces_background_notify_current
 (gpointer                     owner,
  LttvTrace                   *trace,
  LttTime                      notify_time,
  const LttvTracesetContextPosition *notify_position,
  const LttvHooks                   *notify)
{
  BackgroundNotify *bg_notify;
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;
  GSList **slist;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_NOTIFY_CURRENT,
                                LTTV_POINTER,
                                &value));
  slist = (GSList**)(value.v_pointer);

  bg_notify = g_new(BackgroundNotify,1);

  bg_notify->owner = owner;
  bg_notify->trace = trace;
  bg_notify->notify_time = notify_time;
  bg_notify->notify_position = ltt_traceset_context_position_new();
  lttv_traceset_context_position_copy(bg_notify->notify_position,
                                      notify_position);
  bg_notify->notify = lttv_hooks_new();
  lttv_hooks_add_list(bg_notify->notify, notify);

  *slist = g_slist_append(*slist, bg_notify);
}

/**
 * Removes all the notifications requests from a specific viewer.
 * 
 * @param owner owner of the background notification
 */

void lttvwindowtraces_background_notify_remove(gpointer owner)
{
  guint i;

  for(i=0;i<lttvwindowtraces_get_number();i++) {
    LttvAttribute *attribute;
    LttvAttributeValue value;
    LttvTrace *trace_v = lttvwindowtraces_get_trace(i);
    GSList **slist;
    GSList *iter = NULL;
    
    g_assert(trace_v != NULL);

    LttvAttribute *t_a = lttv_trace_attribute(trace_v);

    g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                  LTTV_NOTIFY_QUEUE,
                                  LTTV_POINTER,
                                  &value));
    slist = (GSList**)(value.v_pointer);
 
    for(iter=*slist;iter!=NULL;) {
    
      BackgroundNotify *bg_notify = (BackgroundNotify*)iter->data;

      if(bg_notify->owner == owner) {
        GSList *rem_iter = iter;
        iter=g_slist_next(iter);
        lttv_traceset_context_position_destroy(
                      bg_notify->notify_position);
        lttv_hooks_destroy(bg_notify->notify);
        g_free(bg_notify); 
        g_slist_remove_link(*slist, rem_iter);
      } else {
        iter=g_slist_next(iter);
      }
    }

    g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                  LTTV_NOTIFY_CURRENT,
                                  LTTV_POINTER,
                                  &value));
    slist = (GSList**)(value.v_pointer);
 
    for(iter=*slist;iter!=NULL;) {
    
      BackgroundNotify *bg_notify = (BackgroundNotify*)iter->data;

      if(bg_notify->owner == owner) {
        GSList *rem_iter = iter;
        iter=g_slist_next(iter);
        lttv_traceset_context_position_destroy(
                      bg_notify->notify_position);
        lttv_hooks_destroy(bg_notify->notify);
        g_free(bg_notify); 
        g_slist_remove_link(*slist, rem_iter);
      } else {
        iter=g_slist_next(iter);
      }
    }
  }
}

