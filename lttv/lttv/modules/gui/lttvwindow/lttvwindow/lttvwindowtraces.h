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

/* lttvwindowtraces
 *
 * This API consists in two main parts. The first one is for the background
 * computation provider and the second is for the viewer which needs this
 * information.
 *
 * A computation provider, i.e. a statistics computation module or a state
 * computation module, have two things in common : they append data to a trace
 * in an extensible container (LttvAttributes). This extended information, once
 * computed, can be kept all along with the trace and does not need to be
 * recomputed : a computation done on a trace must result in a identical result
 * each time it is done.
 *
 * This API provides functions for computation provider to register their
 * computation functions (or computation functions insertion and removal
 * functions). Once the computation provider is registered with its module name,
 * extended computation for a trace can be requested by any viewer by specifying
 * the module name, as we will describe in a moment.
 *
 * A viewer which needs extended information about a trace must ask for it to be
 * computed by doing a background computation request. It may also ask to be
 * notified of the completion of its request by doing a notify request.
 *
 * Before asking for the computation, it must check for its readiness. If it is
 * ready, the information has already been computed, so it is ready to use. If
 * the information is not ready, in must check whether or not the processing of
 * this task is in progress. If it is, it must not do any background computation
 * request. It must only do a background notification request of the current
 * processing to be informed of its completion. If the information is not ready
 * and not being processed, then the viewer may do a background computation
 * request and add a notify request to the notify queue.
 *
 * When a context takes control of a trace, it must lock the trace. This is a
 * way of ensuring that not conflict will occur between two traceset contexts
 * and shared traces. It will generate an error if a context try to get a lock
 * on a trace what is not unlocked. Upon every trace locking,
 * lttv_process_traceset_synchronize_tracefiles should be used to resynchronize
 * the traces with the trace context information.
 *
 * The usefulness of the lock in this framework can be questionable in a
 * single threaded environment, but can be great in the eventuality of
 * multiple threads.
 * 
 */



#ifndef LTTVWINDOWTRACES_H
#define LTTVWINDOWTRACES_H

#include <ltt/time.h>
#include <glib.h>
#include <gtk/gtk.h>

typedef GQuark LttvTraceInfo;

extern LttvTraceInfo LTTV_TRACES,
              LTTV_COMPUTATION,
              LTTV_REQUESTS_QUEUE,
              LTTV_REQUESTS_CURRENT,
              LTTV_NOTIFY_QUEUE,
              LTTV_NOTIFY_CURRENT,
              LTTV_COMPUTATION_TRACESET,
              LTTV_COMPUTATION_TRACESET_CONTEXT,
              LTTV_COMPUTATION_SYNC_POSITION,
              LTTV_BEFORE_CHUNK_TRACESET,
              LTTV_BEFORE_CHUNK_TRACE,
              LTTV_BEFORE_CHUNK_TRACEFILE,
              LTTV_AFTER_CHUNK_TRACESET,
              LTTV_AFTER_CHUNK_TRACE,
              LTTV_AFTER_CHUNK_TRACEFILE,
              LTTV_BEFORE_REQUEST,
              LTTV_AFTER_REQUEST,
              LTTV_EVENT_HOOK,
              LTTV_EVENT_HOOK_BY_ID_CHANNEL,
              LTTV_HOOK_ADDER,
              LTTV_HOOK_REMOVER,
              LTTV_IN_PROGRESS,
              LTTV_READY,
              LTTV_LOCK;
              


/* Get a trace by its path name. 
 *
 * @param path path of the trace on the virtual file system.
 * @return Pointer to trace if found
 *        NULL is returned if the trace is not present
 */

LttvTrace *lttvwindowtraces_get_trace_by_name(gchar *path);

/* Get a trace by its number identifier */

LttvTrace *lttvwindowtraces_get_trace(guint num);

/* Total number of traces */

guint lttvwindowtraces_get_number();

/* Add a trace to the global attributes */

void lttvwindowtraces_add_trace(LttvTrace *trace);

/* Remove a trace from the global attributes */

void lttvwindowtraces_remove_trace(LttvTrace *trace);


/**
 * Function to request data from a specific trace
 *
 * The memory allocated for the request will be managed by the API.
 * 
 * @param tab parent Window
 * @param trace the trace to compute
 * @param module_name the name of the module which registered global computation
 *                    hooks.
 */

void lttvwindowtraces_background_request_queue
                     (GtkWidget *widget, LttvTrace *trace, gchar *module_name);

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
                     (LttvTrace *trace, gchar *module_name);
                     

                     
/**
 * Find a background request in a trace
 *
 */

gboolean lttvwindowtraces_background_request_find
                     (LttvTrace *trace, gchar *module_name);
                     
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
  const LttvHooks                   *notify);

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
  const LttvHooks                   *notify);

/**
 * Removes all the notifications requests from a specific viewer.
 * 
 * @param owner owner of the background notification
 */

void lttvwindowtraces_background_notify_remove(gpointer owner);


/**
 * Tells if the information computed by a module for a trace is ready.
 *
 * Must be checked before a background processing request.
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 * @param trace The trace for which the information is verified.
 */

gboolean lttvwindowtraces_get_ready(LttvAttributeName module_name,
                                    LttvTrace *trace);

/**
 * Tells if the information computed by a module for a trace is being processed.
 * 
 * Must be checked before a background processing request.
 *
 * If it is effectively processed, the typical thing to do is to call
 * lttvwindowtraces_background_notify_current to be notified when the current
 * processing will be over.
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 * @param trace The trace for which the information is verified.
 */

gboolean lttvwindowtraces_get_in_progress(LttvAttributeName module_name,
                                    LttvTrace *trace);

/**
 * Register the background computation hooks for a specific module. It adds the
 * computation hooks to the global attrubutes, under "computation/module name"
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
                                          LttvHooks *hook_remover);
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
                                     (LttvAttributeName module_name);


/**
 * It removes all the requests than can be currently processed by the
 * background computation algorithm for all the traces (list_in and list_out).
 *
 * Leaves the flag to in_progress or none.. depending if current or queue
 *
 * @param module_name A GQuark : the name of the module which computes the
 *                    information.
 */
void lttvwindowtraces_unregister_requests(LttvAttributeName module_name);


/**
 * Lock a trace so no other instance can use it.
 *
 * @param trace The trace to lock.
 * @return 0 on success, -1 if cannot get lock.
 */
gint lttvwindowtraces_lock(LttvTrace *trace);


/**
 * Unlock a trace.
 *
 * @param trace The trace to unlock.
 * @return 0 on success, -1 if cannot unlock (not locked ?).
 */
gint lttvwindowtraces_unlock(LttvTrace *trace);

/**
 * Verify if a trace is locked.
 *
 * @param trace The trace to verify.
 * @return TRUE if locked, FALSE is unlocked.
 */
gint lttvwindowtraces_get_lock_state(LttvTrace *trace);


#endif //LTTVWINDOWTRACES_H
