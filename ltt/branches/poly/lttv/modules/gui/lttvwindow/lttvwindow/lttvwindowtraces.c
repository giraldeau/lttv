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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ltt/time.h>
#include <ltt/trace.h>
#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/traceset.h>
#include <lttv/attribute.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/lttvwindow.h> // for CHUNK_NUM_EVENTS


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



/* Prototypes */
gboolean lttvwindowtraces_process_pending_requests(LttvTrace *trace);

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
  struct stat buf;
  gchar attribute_path[PATH_MAX];

  if(stat(ltt_trace_name(lttv_trace(trace)), &buf)) {
    g_warning("lttvwindowtraces_add_trace: Trace %s not found",
        ltt_trace_name(lttv_trace(trace)));
    return;
  }
  g_assert(
      snprintf(attribute_path, PATH_MAX, "%lu:%lu", buf.st_dev, buf.st_ino) >= 0);
  
  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_TRACES)));
    
  value = lttv_attribute_add(attribute,
                     g_quark_from_string(attribute_path),
                     LTTV_POINTER);

  *(value.v_pointer) = (gpointer)trace;
  
  /* create new traceset and tracesetcontext */
  LttvTraceset *ts;
  LttvTracesetStats *tss;
  
  attribute = lttv_trace_attribute(trace);
  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_COMPUTATION_TRACESET,
                                LTTV_POINTER,
                                &value));
  ts = lttv_traceset_new();
  *(value.v_pointer) = ts;
 
  lttv_traceset_add(ts,trace);

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_COMPUTATION_TRACESET_CONTEXT,
                                LTTV_POINTER,
                                &value));
  tss = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  *(value.v_pointer) = tss;
  
  lttv_context_init(LTTV_TRACESET_CONTEXT(tss), ts);

  value = lttv_attribute_add(attribute,
                     LTTV_REQUESTS_QUEUE,
                     LTTV_POINTER);

  value = lttv_attribute_add(attribute,
                     LTTV_REQUESTS_CURRENT,
                     LTTV_POINTER);
 
  value = lttv_attribute_add(attribute,
                     LTTV_NOTIFY_QUEUE,
                     LTTV_POINTER);
  
  value = lttv_attribute_add(attribute,
                     LTTV_NOTIFY_CURRENT,
                     LTTV_POINTER);
 
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

    /* Remove and background computation that could be in progress */
    g_idle_remove_by_data(trace_v);
    
    if(trace_v == trace) {
      /* Found */
      LttvAttribute *l_attribute;

      /* create new traceset and tracesetcontext */
      LttvTraceset *ts;
      LttvTracesetStats *tss;
      
      l_attribute = lttv_trace_attribute(trace);


      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_REQUESTS_QUEUE);

      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_REQUESTS_CURRENT);

      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_NOTIFY_QUEUE);

      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_NOTIFY_CURRENT);

      g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(l_attribute),
                                    LTTV_COMPUTATION_TRACESET,
                                    LTTV_POINTER,
                                    &value));
      ts = (LttvTraceset*)*(value.v_pointer);
     
      g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(l_attribute),
                                    LTTV_COMPUTATION_TRACESET_CONTEXT,
                                    LTTV_POINTER,
                                    &value));
      tss = (LttvTracesetStats*)*(value.v_pointer);
      
      lttv_context_fini(LTTV_TRACESET_CONTEXT(tss));
      g_object_unref(tss);
      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_COMPUTATION_TRACESET_CONTEXT);
      lttv_traceset_destroy(ts);
      lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(l_attribute),
                                     LTTV_COMPUTATION_TRACESET);

      /* finally, remove the global attribute */
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

  /* Priority lower than live servicing */
  g_idle_remove_by_data(trace);
  g_idle_add_full((G_PRIORITY_HIGH_IDLE + 23),
                  (GSourceFunc)lttvwindowtraces_process_pending_requests,
                  trace,
                  NULL);
  /* FIXME : show message in status bar, need context and message id */
  g_info("Background computation started for trace %p", trace);
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
  if(notify_position != NULL) {
    bg_notify->notify_position = lttv_traceset_context_position_new();
    lttv_traceset_context_position_copy(bg_notify->notify_position,
                                        notify_position);
  } else {
    bg_notify->notify_position = NULL;
  }

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
  if(notify_position!= NULL) {
    bg_notify->notify_position = lttv_traceset_context_position_new();
    lttv_traceset_context_position_copy(bg_notify->notify_position,
                                        notify_position);
  } else {
    bg_notify->notify_position = NULL;
  }
  bg_notify->notify = lttv_hooks_new();
  lttv_hooks_add_list(bg_notify->notify, notify);

  *slist = g_slist_append(*slist, bg_notify);
}


static void notify_request_free(BackgroundNotify *notify_req)
{
  if(notify_req == NULL) return;

  if(notify_req->notify_position != NULL)
    lttv_traceset_context_position_destroy(notify_req->notify_position);
  if(notify_req->notify != NULL)
    lttv_hooks_destroy(notify_req->notify);
  g_free(notify_req);
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

    attribute = lttv_trace_attribute(trace_v);

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
        notify_request_free(bg_notify);
        *slist = g_slist_remove_link(*slist, rem_iter);
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
        notify_request_free(bg_notify);
        *slist = g_slist_remove_link(*slist, rem_iter);
      } else {
        iter=g_slist_next(iter);
      }
    }
  }
}


/* Background processing helper functions */

void lttvwindowtraces_add_computation_hooks(LttvAttributeName module_name,
                                            LttvTracesetContext *tsc)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *module_attribute;
  LttvAttributeType type;
  LttvAttributeValue value;
  LttvHooks *before_chunk_traceset=NULL;
  LttvHooks *before_chunk_trace=NULL;
  LttvHooks *before_chunk_tracefile=NULL;
  LttvHooks *event_hook=NULL;
  LttvHooksById *event_hook_by_id=NULL;
  LttvTracesetStats *tss = LTTV_TRACESET_STATS(tsc);

 
  g_assert(module_attribute =
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_COMPUTATION)));

  g_assert(module_attribute =
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
                                LTTV_IATTRIBUTE(module_attribute),
                                module_name)));

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_BEFORE_CHUNK_TRACESET,
                                     &value);
  if(type == LTTV_POINTER) {
    before_chunk_traceset = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_BEFORE_CHUNK_TRACE,
                                     &value);
  if(type == LTTV_POINTER) {
    before_chunk_trace = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_BEFORE_CHUNK_TRACEFILE,
                                     &value);
  if(type == LTTV_POINTER) {
    before_chunk_tracefile = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_EVENT_HOOK,
                                     &value);
  if(type == LTTV_POINTER) {
    event_hook = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_EVENT_HOOK_BY_ID,
                                     &value);
  if(type == LTTV_POINTER) {
    event_hook_by_id = (LttvHooksById*)*(value.v_pointer);
  }

  /* Call the module's hook adder */
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_HOOK_ADDER,
                                     &value);
  if(type == LTTV_POINTER) {
    lttv_hooks_call((LttvHooks*)*(value.v_pointer), (gpointer)tss);
  }



  lttv_process_traceset_begin(tsc,
                              before_chunk_traceset,
                              before_chunk_trace,
                              before_chunk_tracefile,
                              event_hook,
                              event_hook_by_id);

}
                                            
void lttvwindowtraces_remove_computation_hooks(LttvAttributeName module_name,
                                               LttvTracesetContext *tsc)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *module_attribute;
  LttvAttributeType type;
  LttvAttributeValue value;
  LttvHooks *after_chunk_traceset=NULL;
  LttvHooks *after_chunk_trace=NULL;
  LttvHooks *after_chunk_tracefile=NULL;
  LttvHooks *event_hook=NULL;
  LttvHooksById *event_hook_by_id=NULL;
  LttvTracesetStats *tss = LTTV_TRACESET_STATS(tsc);
 
  g_assert(module_attribute =
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_COMPUTATION)));

  g_assert(module_attribute =
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
                                LTTV_IATTRIBUTE(module_attribute),
                                module_name)));

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_AFTER_CHUNK_TRACESET,
                                     &value);
  if(type == LTTV_POINTER) {
    after_chunk_traceset = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_AFTER_CHUNK_TRACE,
                                     &value);
  if(type == LTTV_POINTER) {
    after_chunk_trace = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_AFTER_CHUNK_TRACEFILE,
                                     &value);
  if(type == LTTV_POINTER) {
    after_chunk_tracefile = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_EVENT_HOOK,
                                     &value);
  if(type == LTTV_POINTER) {
    event_hook = (LttvHooks*)*(value.v_pointer);
  }

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_EVENT_HOOK_BY_ID,
                                     &value);
  if(type == LTTV_POINTER) {
    event_hook_by_id = (LttvHooksById*)*(value.v_pointer);
  }
  
  lttv_process_traceset_end(tsc,
                            after_chunk_traceset,
                            after_chunk_trace,
                            after_chunk_tracefile,
                            event_hook,
                            event_hook_by_id);

  /* Call the module's hook remover */
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(module_attribute),
                                     LTTV_HOOK_REMOVER,
                                     &value);
  if(type == LTTV_POINTER) {
    lttv_hooks_call((LttvHooks*)*(value.v_pointer), (gpointer)tss);
  }
}


void lttvwindowtraces_set_in_progress(LttvAttributeName module_name,
                                      LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  value = lttv_iattribute_add(LTTV_IATTRIBUTE(attribute),
                              LTTV_IN_PROGRESS,
                              LTTV_INT);
  /* the value is left unset. The only presence of the attribute is necessary.
   */
}

void lttvwindowtraces_unset_in_progress(LttvAttributeName module_name,
                                        LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                         LTTV_IN_PROGRESS);
}

gboolean lttvwindowtraces_get_in_progress(LttvAttributeName module_name,
                                          LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeType type;
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_IN_PROGRESS,
                                     &value);
  /* The only presence of the attribute is necessary. */
  if(type == LTTV_NONE)
    return FALSE;
  else
    return TRUE;
}

void lttvwindowtraces_set_ready(LttvAttributeName module_name,
                                LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  value = lttv_iattribute_add(LTTV_IATTRIBUTE(attribute),
                              LTTV_READY,
                              LTTV_INT);
  /* the value is left unset. The only presence of the attribute is necessary.
   */
}

void lttvwindowtraces_unset_ready(LttvAttributeName module_name,
                                  LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                         LTTV_READY);
}

gboolean lttvwindowtraces_get_ready(LttvAttributeName module_name,
                                    LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeType type;
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));
 
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_READY,
                                     &value);
  /* The only presence of the attribute is necessary. */
  if(type == LTTV_NONE)
    return FALSE;
  else
    return TRUE;
}


/* lttvwindowtraces_process_pending_requests
 * 
 * This internal function gets called by g_idle, taking care of the pending
 * requests.
 *
 */


gboolean lttvwindowtraces_process_pending_requests(LttvTrace *trace)
{
  LttvTracesetContext *tsc;
  LttvTracesetStats *tss;
  LttvTraceset *ts;
  LttvAttribute *attribute;
  GSList **list_out, **list_in, **notify_in, **notify_out;
  LttvAttributeValue value;
  LttvAttributeType type;
  gboolean ret_val;

  if(trace == NULL)
    return FALSE;
   
  attribute = lttv_trace_attribute(trace);
  
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_REQUESTS_QUEUE,
                                     &value);
  g_assert(type == LTTV_POINTER);
  list_out = (GSList**)(value.v_pointer);

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_REQUESTS_CURRENT,
                                     &value);
  g_assert(type == LTTV_POINTER);
  list_in = (GSList**)(value.v_pointer);
 
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_NOTIFY_QUEUE,
                                     &value);
  g_assert(type == LTTV_POINTER);
  notify_out = (GSList**)(value.v_pointer);
  
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_NOTIFY_CURRENT,
                                     &value);
  g_assert(type == LTTV_POINTER);
  notify_in = (GSList**)(value.v_pointer);
 
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_COMPUTATION_TRACESET,
                                     &value);
  g_assert(type == LTTV_POINTER);
  ts = (LttvTraceset*)*(value.v_pointer);
 
  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_COMPUTATION_TRACESET_CONTEXT,
                                     &value);
  g_assert(type == LTTV_POINTER);
  tsc = (LttvTracesetContext*)*(value.v_pointer);
  tss = (LttvTracesetStats*)*(value.v_pointer);
  g_assert(LTTV_IS_TRACESET_CONTEXT(tsc));
  g_assert(LTTV_IS_TRACESET_STATS(tss));
 
  /* There is no events requests pending : we should never have been called! */
  g_assert(g_slist_length(*list_out) != 0 || g_slist_length(*list_in) != 0);

  /* 0.1 Lock traces */
  {
    guint iter_trace=0;
    
    for(iter_trace=0; 
        iter_trace<lttv_traceset_number(tsc->ts);
        iter_trace++) {
      LttvTrace *trace_v = lttv_traceset_get(tsc->ts,iter_trace);

      if(lttvwindowtraces_lock(trace_v) != 0)
        return TRUE; /* Cannot get trace lock, try later */

    }
  }
  /* 0.2 Sync tracefiles */
  lttv_process_traceset_synchronize_tracefiles(tsc);

  /* 1. Before processing */
  {
    /* if list_in is empty */
    if(g_slist_length(*list_in) == 0) {

      {
        /* - Add all requests in list_out to list_in, empty list_out */
        GSList *iter = *list_out;

        while(iter != NULL) {
          gboolean remove = FALSE;
          gboolean free_data = FALSE;

          BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;

          remove = TRUE;
          free_data = FALSE;
          *list_in = g_slist_append(*list_in, bg_req);

          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) g_free(remove_iter->data);
            *list_out = g_slist_remove_link(*list_out, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
        }
      }

      {
        GSList *iter = *list_in;
        /* - for each request in list_in */
        while(iter != NULL) {
          
          BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;
          /*- add hooks to context*/
          lttvwindowtraces_set_in_progress(bg_req->module_name,
                                           bg_req->trace);
          
          iter = g_slist_next(iter);
        }
      }

      /* - seek trace to start */
      {
        LttTime start = { 0, 0};
        lttv_process_traceset_seek_time(tsc, start);
      }

      /* - Move all notifications from notify_out to notify_in. */
      {
        GSList *iter = *notify_out;
        g_assert(g_slist_length(*notify_in) == 0);

        while(iter != NULL) {
          gboolean remove = FALSE;
          gboolean free_data = FALSE;

          BackgroundNotify *notify_req = (BackgroundNotify*)iter->data;

          remove = TRUE;
          free_data = FALSE;
          *notify_in = g_slist_append(*notify_in, notify_req);

          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) 
                 notify_request_free((BackgroundNotify*)remove_iter->data);
            *notify_out = g_slist_remove_link(*notify_out, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
        }
      }
    }

    {
      GSList *iter = *list_in;
      /* - for each request in list_in */
      while(iter != NULL) {
        
        BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;
        /*- Call before chunk hooks for list_in*/
        /*- add hooks to context*/
        lttvwindowtraces_add_computation_hooks(bg_req->module_name,
                                               tsc);
        iter = g_slist_next(iter);
      }
    }
  }

  /* 2. call process traceset middle for a chunk */
  {
    /*(assert list_in is not empty! : should not even be called in that case)*/
    LttTime end = { G_MAXUINT, G_MAXUINT };
    g_assert(g_slist_length(*list_in) != 0);
    
    lttv_process_traceset_middle(tsc, end, CHUNK_NUM_EVENTS, NULL);
  }

  /* 3. After the chunk */
  {
    /*  3.1 call after_chunk hooks for list_in */
    {
      GSList *iter = *list_in;
      /* - for each request in list_in */
      while(iter != NULL) {
        
        BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;
        /* - Call after chunk hooks for list_in */
        /* - remove hooks from context */
        lttvwindowtraces_remove_computation_hooks(bg_req->module_name,
                                                  tsc);
        iter = g_slist_next(iter);
      }
    }

    /* 3.2 for each notify_in */
    {
      GSList *iter = *notify_in;
      LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
        
      while(iter != NULL) {
        gboolean remove = FALSE;
        gboolean free_data = FALSE;

        BackgroundNotify *notify_req = (BackgroundNotify*)iter->data;

        /* - if current time >= notify time, call notify and remove from
         * notify_in.
         * - if current position >= notify position, call notify and remove
         * from notify_in.
         */
        if( (tfc != NULL &&
              ltt_time_compare(notify_req->notify_time, tfc->timestamp) <= 0)
           ||
            (notify_req->notify_position != NULL && 
                     lttv_traceset_context_ctx_pos_compare(tsc,
                              notify_req->notify_position) >= 0)
           ) {

          lttv_hooks_call(notify_req->notify, notify_req);

          remove = TRUE;
          free_data = TRUE;
        }

        /* Go to next */
        if(remove)
        {
          GSList *remove_iter = iter;

          iter = g_slist_next(iter);
          if(free_data) 
               notify_request_free((BackgroundNotify*)remove_iter->data);
          *notify_in = g_slist_remove_link(*notify_in, remove_iter);
        } else { // not remove
          iter = g_slist_next(iter);
        }
      }
    }

    {
      LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
      /* 3.3 if end of trace reached */
      if(tfc != NULL)
        g_debug("Current time : %lu sec, %lu nsec",
            tfc->timestamp.tv_sec, tfc->timestamp.tv_nsec);
      if(tfc == NULL || ltt_time_compare(tfc->timestamp,
                         tsc->time_span.end_time) > 0) {
        
        /* - for each request in list_in */
        {
          GSList *iter = *list_in;
          
          while(iter != NULL) {
            gboolean remove = FALSE;
            gboolean free_data = FALSE;

            BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;

            /* - set hooks'in_progress flag to FALSE */
            lttvwindowtraces_unset_in_progress(bg_req->module_name,
                                               bg_req->trace);
            /* - set hooks'ready flag to TRUE */
            lttvwindowtraces_set_ready(bg_req->module_name,
                                       bg_req->trace);
            /* - remove request */
            remove = TRUE;
            free_data = TRUE;

            /* Go to next */
            if(remove)
            {
              GSList *remove_iter = iter;

              iter = g_slist_next(iter);
              if(free_data) g_free(remove_iter->data);
              *list_in = g_slist_remove_link(*list_in, remove_iter);
            } else { // not remove
              iter = g_slist_next(iter);
            }
          }
        }

        /* - for each notifications in notify_in */
        {
          GSList *iter = *notify_in;
          
          while(iter != NULL) {
            gboolean remove = FALSE;
            gboolean free_data = FALSE;

            BackgroundNotify *notify_req = (BackgroundNotify*)iter->data;

            /* - call notify and remove from notify_in */
            lttv_hooks_call(notify_req->notify, notify_req);
            remove = TRUE;
            free_data = TRUE;

            /* Go to next */
            if(remove)
            {
              GSList *remove_iter = iter;

              iter = g_slist_next(iter);
              if(free_data) 
                    notify_request_free((BackgroundNotify*)remove_iter->data);
              *notify_in = g_slist_remove_link(*notify_in, remove_iter);
            } else { // not remove
              iter = g_slist_next(iter);
            }
          }
        }
        
        /* - return FALSE (scheduler stopped) */
        g_debug("Background computation scheduler stopped");
        g_info("Background computation finished for trace %p", trace);
        /* FIXME : remove status bar info, need context id and message id */
        ret_val = FALSE;
      } else {
        /* 3.4 else, end of trace not reached */
        /* - return TRUE (scheduler still registered) */
        g_debug("Background computation left");
        ret_val = TRUE;
      }
    }
  }
  /* 4. Unlock traces */
  {
    //lttv_process_traceset_get_sync_data(tsc);
    guint iter_trace;
    
    for(iter_trace=0; 
        iter_trace<lttv_traceset_number(tsc->ts);
        iter_trace++) {
      LttvTrace *trace_v = lttv_traceset_get(tsc->ts, iter_trace);

      lttvwindowtraces_unlock(trace_v);
    }
  }
  return ret_val;
}



/**
 * Register the background computation hooks for a specific module. It adds the
 * computation hooks to the global attrubutes, under "computation/module name".
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 */
void lttvwindowtraces_register_computation_hooks(LttvAttributeName module_name,
                                          LttvHooks *before_chunk_traceset,
                                          LttvHooks *before_chunk_trace,
                                          LttvHooks *before_chunk_tracefile,
                                          LttvHooks *after_chunk_traceset,
                                          LttvHooks *after_chunk_trace,
                                          LttvHooks *after_chunk_tracefile,
                                          LttvHooks *before_request,
                                          LttvHooks *after_request,
                                          LttvHooks *event_hook,
                                          LttvHooksById *event_hook_by_id,
                                          LttvHooks *hook_adder,
                                          LttvHooks *hook_remover)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;
  LttvAttributeValue value;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_COMPUTATION)));

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_BEFORE_CHUNK_TRACESET,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = before_chunk_traceset;
  
  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_BEFORE_CHUNK_TRACE,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = before_chunk_trace;
  
  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_BEFORE_CHUNK_TRACEFILE,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = before_chunk_tracefile;
  
  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_AFTER_CHUNK_TRACESET,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = after_chunk_traceset;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_AFTER_CHUNK_TRACE,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = after_chunk_trace;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_AFTER_CHUNK_TRACEFILE,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = after_chunk_tracefile;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_BEFORE_REQUEST,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = before_request;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_AFTER_REQUEST,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = after_request;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_EVENT_HOOK,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = event_hook;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_EVENT_HOOK_BY_ID,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = event_hook_by_id;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_HOOK_ADDER,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = hook_adder;

  g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                LTTV_HOOK_REMOVER,
                                LTTV_POINTER,
                                &value));
  *(value.v_pointer) = hook_remover;

}


/**
 * It removes all the requests than can be currently processed by the
 * background computation algorithm for all the traces (list_in and list_out).
 *
 * Leaves the flag to in_progress or none.. depending if current or queue
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 */
void lttvwindowtraces_unregister_requests(LttvAttributeName module_name)
{
  guint i;

  for(i=0;i<lttvwindowtraces_get_number();i++) {
    LttvTrace *trace_v = lttvwindowtraces_get_trace(i);
    g_assert(trace_v != NULL);
    LttTrace *trace;
    LttvAttribute *attribute = lttv_trace_attribute(trace_v);
    LttvAttributeValue value;
    GSList **queue, **current;
    GSList *iter;
    
    g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                  LTTV_REQUESTS_QUEUE,
                                  LTTV_POINTER,
                                  &value));
    queue = (GSList**)(value.v_pointer);
    
    iter = *queue;
    while(iter != NULL) {
      gboolean remove = FALSE;
      gboolean free_data = FALSE;

      BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;

      if(bg_req->module_name == module_name) {
        remove = TRUE;
        free_data = TRUE;
      }

      /* Go to next */
      if(remove)
      {
        GSList *remove_iter = iter;

        iter = g_slist_next(iter);
        if(free_data) g_free(remove_iter->data);
        *queue = g_slist_remove_link(*queue, remove_iter);
      } else { // not remove
        iter = g_slist_next(iter);
      }
    }
    
        
    g_assert(lttv_iattribute_find(LTTV_IATTRIBUTE(attribute),
                                  LTTV_REQUESTS_CURRENT,
                                  LTTV_POINTER,
                                  &value));
    current = (GSList**)(value.v_pointer);
    
    iter = *current;
    while(iter != NULL) {
      gboolean remove = FALSE;
      gboolean free_data = FALSE;

      BackgroundRequest *bg_req = (BackgroundRequest*)iter->data;

      if(bg_req->module_name == module_name) {
        remove = TRUE;
        free_data = TRUE;
      }

      /* Go to next */
      if(remove)
      {
        GSList *remove_iter = iter;

        iter = g_slist_next(iter);
        if(free_data) g_free(remove_iter->data);
        *current = g_slist_remove_link(*current, remove_iter);
      } else { // not remove
        iter = g_slist_next(iter);
      }
    }
  }
}


/**
 * Unregister the background computation hooks for a specific module.
 *
 * It also removes all the requests than can be currently processed by the
 * background computation algorithm for all the traces (list_in and list_out).
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 */

void lttvwindowtraces_unregister_computation_hooks
                                     (LttvAttributeName module_name)
{
  LttvAttribute *g_attribute = lttv_global_attributes();
  LttvAttribute *attribute;

  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_COMPUTATION)));
  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(attribute),
                                module_name)));


  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_EVENT_HOOK_BY_ID);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_EVENT_HOOK);

  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_AFTER_REQUEST);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_BEFORE_REQUEST);

  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_AFTER_CHUNK_TRACEFILE);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_AFTER_CHUNK_TRACE);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_AFTER_CHUNK_TRACESET);

  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_BEFORE_CHUNK_TRACEFILE);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_BEFORE_CHUNK_TRACE);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_BEFORE_CHUNK_TRACESET);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_HOOK_ADDER);
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_HOOK_REMOVER);

  /* finally, remove module name */
  g_assert(attribute = 
      LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(LTTV_IATTRIBUTE(g_attribute),
                                LTTV_COMPUTATION)));
  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                                     module_name);

}

/**
 * Lock a trace so no other instance can use it.
 *
 * @param trace The trace to lock.
 * @return 0 on success, -1 if cannot get lock.
 */
gint lttvwindowtraces_lock(LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeValue value;
  LttvAttributeType type;

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_LOCK,
                                     &value);
  /* Verify the absence of the lock. */
  if(type != LTTV_NONE) {
    g_critical("Cannot take trace lock");
    return -1;
  }

  value = lttv_iattribute_add(LTTV_IATTRIBUTE(attribute),
                              LTTV_LOCK,
                              LTTV_INT);
  /* the value is left unset. The only presence of the attribute is necessary.
   */

  return 0;
}

/**
 * Unlock a trace.
 *
 * @param trace The trace to unlock.
 * @return 0 on success, -1 if cannot unlock (not locked ?).
 */
gint lttvwindowtraces_unlock(LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeType type;
  LttvAttributeValue value;

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_LOCK,
                                     &value);
  /* Verify the presence of the lock. */
  if(type == LTTV_NONE) {
    g_critical("Cannot release trace lock");
    return -1;
  }

  lttv_iattribute_remove_by_name(LTTV_IATTRIBUTE(attribute),
                         LTTV_LOCK);

  return 0;
}

/**
 * Verify if a trace is locked.
 *
 * @param trace The trace to verify.
 * @return TRUE if locked, FALSE is unlocked.
 */
gint lttvwindowtraces_get_lock_state(LttvTrace *trace)
{
  LttvAttribute *attribute = lttv_trace_attribute(trace);
  LttvAttributeType type;
  LttvAttributeValue value;

  type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                     LTTV_LOCK,
                                     &value);
  /* The only presence of the attribute is necessary. */
  if(type == LTTV_NONE)
    return FALSE;
  else
    return TRUE;
}

