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

#ifndef LTTVWINDOWTRACES_H
#define LTTVWINDOWTRACES_H

#include <ltt/time.h>
#include <glib.h>

typedef GQuark LttvTraceInfo;

extern LttvTraceInfo LTTV_TRACES,
              LTTV_COMPUTATION,
              LTTV_REQUESTS_QUEUE,
              LTTV_REQUESTS_CURRENT,
              LTTV_NOTIFY_QUEUE,
              LTTV_NOTIFY_CURRENT;

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
 * @param trace the trace to compute
 * @param module_name the name of the module which registered global computation
 *                    hooks.
 */

void lttvwindowtraces_background_request_queue
                     (LttvTrace *trace, gchar *module_name);

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


#endif //LTTVWINDOWTRACES_H
