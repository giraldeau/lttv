/* This file is part of the Linux Trace Toolkit Graphic User Interface
 * Copyright (C) 2003-2004 Xiangxiu Yang, Mathieu Desnoyers
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

/*
This file is what every viewer plugin writer should refer to.


Module Related API

A viewer plugin is, before anything, a plugin. As a dynamically loadable
module, it thus has an init and a destroy function called whenever it is
loaded/initialized and unloaded/destroyed. A graphical module depends on
lttvwindow for construction of its viewer instances. In order to achieve
this, it must register its constructor function to the main window along
with button description or text menu entry description. A module keeps
a list of every viewer that currently sits in memory so it can destroy
them before the module gets unloaded/destroyed.

The contructor registration to the main windows adds button and menu
entry to each main window, thus allowing instanciation of viewers.


Main Window

The main window is a container that offers menus, buttons and a
notebook. Some of those menus and buttons are part of the core of the
main window, others are dynamically added and removed when modules are
loaded/unloaded.

The notebook contains as much tabs as wanted. Each tab is linked with
a set of traces (traceset). Each trace contains many tracefiles (one
per cpu).  A trace corresponds to a kernel being traced. A traceset
corresponds to many traces read together. The time span of a traceset
goes from the earliest start of all the traces to the latest end of all
the traces.

Inside each tab are added the viewers. When they interact with the main
window through the lttvwindow API, they affect the other viewers located
in the same tab as they are.

The insertion of many viewers in a tab permits a quick look at all the
information wanted in a glance. The main window does merge the read
requests from all the viewers in the same tab in a way that every viewer
will get exactly the events it asked for, while the event reading loop
and state update are shared. It improves performance of events delivery
to the viewers.



Viewer Instance Related API

The lifetime of a viewer is as follows. The viewer constructor function
is called each time an instance view is created (one subwindow of this
viewer type is created by the user either by clicking on the menu item
or the button corresponding to the viewer). Thereafter, the viewer gets
hooks called for different purposes by the window containing it. These
hooks are detailed below. It also has to deal with GTK Events. Finally,
it can be destructed by having its top level widget unreferenced by the
main window or by any GTK Event causing a "destroy-event" signal on the
its top widget. Another possible way for it do be destroyed is if the
module gets unloaded. The module unload function will have to emit a
"destroy" signal on each top level widget of all instances of its viewers.


Notices from Main Window

time_window : This is the time interval visible on the viewer's tab. Every
              viewer that cares about being synchronised by respect to the
              time with other viewers should register to this notification.
              They should redraw all or part of their display when this occurs.

traceset :    This notification is called whenever a trace is added/removed
              from the traceset. As it affects all the data displayed by the
              viewer, it sould redraw itself totally.

filter :      FIXME : describe..

current_time: Being able to zoom nearer a specific time or highlight a specific
              time on every viewer in synchronicity implies that the viewer
              has to shown a visual sign over the drawing or select an event
              when it receives this notice. It should also inform the main
              window with the appropriate report API function when a user
              selects a specific time as being the current time.

dividor :     This notice links the positions of the horizontal dividors
              between the graphic display zone of every viewer and their Y axis,
              typically showing processes, cpus, ...
              

Reporting Changes to the Main Window

In most cases, the enclosing window knows about updates such as described
in the Notification section higher. There are a few cases, however, where
updates are caused by actions known by a view instance. For example,
clicking in a view may update the current time; all viewers within
the same window must be told about the new current time to change the
currently highlighted time point. A viewer reports such events by calling
lttvwindow_report_current_time on its lttvwindow.  The lttvwindow will
consequently call current_time_notify for each of its contained viewers.


Available report methods are :

lttvwindow_report_time_window : reports the new time window.
lttvwindow_report_current_time : reports the new current time.
lttvwindow_report_dividor : reports the new horizontal dividor's position.
lttvwindow_report_filter : reports the new filter object



Requesting Events to Main Window

Events can be requested by passing a EventsRequest structure to the main
window.  They will be delivered later when the next g_idle functions
will be called.  Event delivery is done by calling the event hook for
this event ID, or the main event hooks. A pointer to the EventsRequest
structure is passed as hook_data to the event hooks of the viewers.

EventsRequest consists in 
- a pointer to the viewer specific data structure
- a start timestamp or position
- a stop_flag, ending the read process when set to TRUE
- a end timestamp and/or position and/or number of events to read
- hook lists to call for traceset/trace/tracefile begin and end, and for each
  event (event hooks and event_by_id_channel hooks).
  
The main window will deliver events for every EventRequests it has
pending through an algorithm that guarantee that all events requested,
and only them, will be delivered to the viewer between the call of the
tracefile_begin hooks and the call of the tracefile_end hooks.

If a viewer wants to stop the event request at a certain point inside the
event hooks, it has to set the stop_flag to TRUE and return TRUE from the
hook function. Then return value will stop the process traceset. Then,
the main window will look for the stop_flag and remove the EventRequests
from its lists, calling the process_traceset_end for this request (it
removes hooks from the context and calls the after hooks).

It no stop_flag is risen, the end timestamp, end position or number
of events to read has to be reached to determine the end of the
request. Otherwise, the end of traceset does determine it.


GTK Events

Events and Signals

GTK is quite different from the other graphical toolkits around
there. The main difference resides in that there are many X Windows
inside one GtkWindow, instead of just one. That means that X events are
delivered by the glib main loop directly to the widget corresponding to
the GdkWindow affected by the X event.

Event delivery to a widget emits a signal on that widget. Then, if a
handler is connected to this widget's signal, it will be executed. There
are default handlers for signals, connected at class instantiation
time. There is also the possibility to connect other handlers to these
signals, which is what should be done in most cases when a viewer needs
to interact with X in any way.



Signal emission and propagation is described there : 

http://www.gtk.org/tutorial/sec-signalemissionandpropagation.html

For further information on the GTK main loop (now a wrapper over glib main loop)
see :

http://developer.gnome.org/doc/API/2.0/gtk/gtk-General.html
http://developer.gnome.org/doc/API/2.0/glib/glib-The-Main-Event-Loop.html


For documentation on event handling in GTK/GDK, see :

http://developer.gnome.org/doc/API/2.0/gdk/gdk-Events.html
http://developer.gnome.org/doc/API/2.0/gdk/gdk-Event-Structures.html


Signals can be connected to handlers, emitted, propagated, blocked, 
stopped. See :

http://developer.gnome.org/doc/API/2.0/gobject/gobject-Signals.html




The "expose_event"

Provides the exposed region in the GdkEventExpose structure. 

There are two ways of dealing with exposures. The first one is to directly
draw on the screen and the second one is to draw in a pixmap buffer,
and then to update the screen when necessary.

In the first case, the expose event will be responsible for registering
hooks to process_traceset and require time intervals to the main
window. So, in this scenario, if a part of the screen is damaged, the
trace has to be read to redraw the screen.

In the second case, with a pixmap buffer, the expose handler is only
responsible of showing the pixmap buffer on the screen. If the pixmap
buffer has never been filled with a drawing, the expose handler may ask
for it to be filled.

The interest of using events request to the main window instead of reading
the events directly from the trace comes from the fact that the main
window does merge requests from the different viewers in the same tab so
that the read loop and the state update is shared. As viewers will, in
the common scenario, request the same events, only one pass through the
trace that will call the right hooks for the right intervals will be done.

When the traceset read is over for a events request, the traceset_end
hook is called. It has the responsibility of finishing the drawing if
some parts still need to be drawn and to show it on the screen (if the
viewer uses a pixmap buffer).

It can add dotted lines and such visual effects to enhance the user's
experience.


FIXME : explain other important events

*/


#ifndef LTTVWINDOW_H
#define LTTVWINDOW_H

/*! \file lttvwindow.h
 * \brief API used by the graphical viewers to interact with their top window.
 * 
 * Main window (lttvwindow module) is the place to contain and display viewers. 
 * Viewers (lttv plugins) interact with main window through this API.
 * This header file should be included in each graphic module.
 * 
 */

#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <ltt/time.h>
#include <lttv/hook.h>
#include <lttv/tracecontext.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <lttvwindow/mainwindow.h>
#include <lttvwindow/lttv_plugin.h>

/* Module Related API */

/* GQuark containing constructors of viewers in global attributes */
extern GQuark LTTV_VIEWER_CONSTRUCTORS;

/* constructor a the viewer */
typedef GtkWidget* (*lttvwindow_viewer_constructor)(LttvPlugin *plugin);

extern gint lttvwindow_preempt_count;

#define CHECK_GDK_INTERVAL 50000

/**
 * Function to register a view constructor so that main window can generate
 * a menu item and a toolbar item for the viewer in order to generate a new
 * instance easily. A menu entry and toolbar item will be added to each main
 * window.
 * 
 * It should be called by init function of the module.
 *
 * @param name name of the viewer : mainly used as tag for constructor
 * @param menu_path path of the menu item. NULL : no menu entry.
 * @param menu_text text of the menu item.
 * @param pixmap Image shown on the toolbar item. NULL : no button.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_constructor
                            (char * name,
                             char *  menu_path, 
                             char *  menu_text,
                             char ** pixmap,
                             char *  tooltip,
                             lttvwindow_viewer_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text, pixmap, tooltip and constructor of the
 * viewer.
 * 
 * It will be called when a module is unloaded.
 * 
 * @param view_constructor constructor of the viewer.
 */

void lttvwindow_unregister_constructor
                            (lttvwindow_viewer_constructor view_constructor);




/* Viewer Instance Related API */

/**
 * Structure used as hook_data for the time_window_notify hook.
 */
typedef struct _TimeWindowNotifyData {
  TimeWindow *new_time_window;
  TimeWindow *old_time_window;
} TimeWindowNotifyData;


/**
 * Function to register a hook function that will be called by the main window
 * when the time interval needs to be updated.
 * 
 * This register function is typically called by the constructor of the viewer.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook that sould be called by the main window when the time
 *             interval changes. This hook function takes a
 *             TimeWindowNotifyData* as call_data.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_time_window_notify(Tab *tab,
                                            LttvHook    hook,
                                            gpointer    hook_data);


/**
 * Function to unregister the time_window notification hook.
 * 
 * This unregister function is typically called by the destructor of the viewer.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook that sould be called by the main window when the time
 *             interval changes. This hook function takes a
 *             TimeWindowNotifyData* as call_data.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_time_window_notify(Tab *tab,
                                              LttvHook    hook, 
                                              gpointer    hook_data);


/**
 * Function to register a hook function that will be called by the main window
 * when the traceset is changed. That means that the viewer must redraw
 * itself completely or check if it's affected by the particular change to the
 * traceset.
 *
 * This register function is typically called by the constructor of the viewer.
 *
 * @param tab the tab the viewer belongs to.
 * @param hook hook that should be called whenever a change to the traceset
 *             occurs. The call_data of this hook is a NULL pointer.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_traceset_notify(Tab *tab,
                                         LttvHook    hook,
                                         gpointer    hook_data);


/**
 * Function to unregister the traceset_notify hook.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook that should be called whenever a change to the traceset
 *             occurs. The call_data of this hook is a NULL pointer.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_traceset_notify(Tab *tab,
                                           LttvHook    hook,
                                           gpointer    hook_data);


/**
 * Function to register a hook function for a viewer be completely redrawn.
 * 
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_redraw_notify(Tab *tab,
    LttvHook hook, gpointer hook_data);

/**
 * Function to unregister a hook function for a viewer be completely redrawn.
 *
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_redraw_notify(Tab *tab,
              LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to re-do the events
 * requests for the needed interval.
 *
 * This action is typically done after a "stop".
 *
 * The typical hook will remove all current requests for the viewer
 * and make requests for missing information.
 * 
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_continue_notify(Tab *tab,
    LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a hook function for a viewer to re-do the events
 * requests for the needed interval.
 *
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_continue_notify(Tab *tab,
              LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter. 
 *
 * FIXME : Add information about what a filter is as seen from a viewer and how
 * to use it.
 *
 * This register function is typically called by the constructor of the viewer.
 *
 * @param tab the tab the viewer belongs to.
 * @param hook hook function called by the main window when a filter change
 *             occurs.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_filter_notify(Tab *tab,
                                       LttvHook    hook,
                                       gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * 
 * This unregistration is called by the destructor of the viewer.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook function called by the main window when a filter change
 *             occurs.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_filter_notify(Tab *tab,
                                         LttvHook     hook,
                                         gpointer     hook_data);


/**
 * Function to get the current filter of the main window : useful at viewer
 * instanciation.
 * 
 * @param tab the tab the viewer belongs to.
 *
 * returns : the current filter.
 */


LttvFilter *lttvwindow_get_filter(Tab *tab);

/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_current_time_notify(Tab *tab,
                                             LttvHook    hook,
                                             gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_current_time_notify(Tab *tab,
                                               LttvHook    hook,
                                               gpointer    hook_data);

/**
 * Function to register a hook function for a viewer to set/update its 
 * current position.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_current_position_notify(Tab *tab,
                                             LttvHook    hook,
                                             gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current position of the viewer.
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_current_position_notify(Tab *tab,
                                               LttvHook    hook,
                                               gpointer    hook_data);



/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane. It provides a way to make the horizontal
 * dividors of all the viewers linked together.
 *
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that will be called whenever a
 *             dividor changes in another viewer. The call_data of this hook
 *             is a gint*. The value of the integer is the new position of the
 *             hpane dividor.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_dividor(Tab *tab,
                                 LttvHook    hook,
                                 gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * 
 * @param tab the tab the viewer belongs to.
 * @param hook hook function of the viewer that will be called whenever a
 *             dividor changes in another viewer. The call_data of this hook
 *             is a gint*. The value of the integer is the new position of the
 *             hpane dividor.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_dividor(Tab *tab,
                                   LttvHook    hook,
                                   gpointer    hook_data);



/**
 * Function to set the time interval of the current tab.a
 *
 * @param tab the tab the viewer belongs to.
 * @param time_interval new time window.
 */

void lttvwindow_report_time_window(Tab *tab,
                                   TimeWindow time_window);

/**
 * Function to set the current time of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param tab the tab the viewer belongs to.
 * @param time current time.
 */

void lttvwindow_report_current_time(Tab *tab, 
                                    LttTime time);


/**
 * Function to set the current event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param tab the tab the viewer belongs to.
 * @param pos the current position.
 */

void lttvwindow_report_current_position(Tab *tab,
                                        LttvTracesetContextPosition *pos);

/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will typically be called by a viewer's signal handle associated 
 * with the motion_notify_event event/signal.
 *
 * @param tab the tab the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void lttvwindow_report_dividor(Tab *tab, gint position);


/* Structure sent to the events request hook */
                                                /* Value considered as empty*/
typedef struct _EventsRequest {
  gpointer                     owner;           /* Owner of the request     */
  gpointer                     viewer_data;     /* Unset : NULL             */
  gboolean                     servicing;       /* service in progress: TRUE*/ 
  LttTime                      start_time;      /* Unset : ltt_time_infinite*/
  LttvTracesetContextPosition *start_position;  /* Unset : NULL             */
  gboolean                     stop_flag;       /* Continue:TRUE Stop:FALSE */
  LttTime                      end_time;        /* Unset : ltt_time_infinite*/
  guint                        num_events;      /* Unset : G_MAXUINT        */
  LttvTracesetContextPosition *end_position;    /* Unset : NULL             */
  gint                         trace;           /* unset : -1               */
  GArray                      *hooks;           /* Unset : NULL             */
  LttvHooks                   *before_chunk_traceset; /* Unset : NULL       */
  LttvHooks                   *before_chunk_trace;    /* Unset : NULL       */
  LttvHooks                   *before_chunk_tracefile;/* Unset : NULL       */
  LttvHooks                   *event;           /* Unset : NULL             */
  LttvHooksByIdChannelArray   *event_by_id_channel;/* Unset : NULL          */
  LttvHooks                   *after_chunk_tracefile; /* Unset : NULL       */
  LttvHooks                   *after_chunk_trace;     /* Unset : NULL       */
  LttvHooks                   *after_chunk_traceset;  /* Unset : NULL       */
  LttvHooks                   *before_request;  /* Unset : NULL             */
  LttvHooks                   *after_request;   /* Unset : NULL             */
} EventsRequest;

/* Maximum number of events to proceed at once in a chunk */
#define CHUNK_NUM_EVENTS 6000


/**
 * Function to request data in a specific time interval to the main window. The
 * event request servicing is differed until the glib idle functions are
 * called.
 *
 * The viewer has to provide hooks that should be associated with the event
 * request.
 *
 * Either start time or start position must be defined in a EventRequest
 * structure for it to be valid.
 *
 * end_time, end_position and num_events can all be defined. The first one
 * to occur will be used as end criterion.
 *
 * The events_request memory will be managed by the main window once its
 * pointer is passed by this function.
 * 
 * @param tab the tab the viewer belongs to.
 * @param events_requested Details about the event request.
 */

void lttvwindow_events_request(Tab                  *tab,
                               EventsRequest  *events_request);

/**
 * Function to remove data requests related to a viewer.
 *
 * The existing requests's viewer gpointer is compared to the pointer
 * given in argument to establish which data request should be removed.
 * 
 * @param tab the tab the viewer belongs to.
 * @param viewer a pointer to the viewer data structure
 */

void lttvwindow_events_request_remove_all(Tab            *tab,
                                          gconstpointer   viewer);


/**
 * Function to see if there are events request pending.
 *
 * It tells if events requests are pending. Useful for checks in some events,
 * i.e. detailed event list scrolling.
 * 
 * @param tab the tab the viewer belongs to.
 * @param viewer a pointer to the viewer data structure
 * @return : TRUE is events requests are pending, else FALSE.
 */

gboolean lttvwindow_events_request_pending(Tab            *tab);




/**
 * Function to get the current time interval shown on the current tab.
 * It will be called by a viewer's hook function to update the 
 * shown time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param tab viewer's tab 
 * @return time window.
 */

TimeWindow lttvwindow_get_time_window(Tab *tab);


/**
 * Function to get the current time of the current tab.
 *
 * @param tab the tab the viewer belongs to.
 * @return the current tab's current time.
 */

LttTime lttvwindow_get_current_time(Tab *tab);


/**
 * Function to get the filter of the current tab.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

//LttvFilter *lttvwindow_get_filter(Tab *tab);

/**
 * Function to set the filter of the current tab.
 * It should be called by the filter GUI to tell the
 * main window to update the filter tab's lttv_filter.
 *
 * Notice : the lttv_filter object will be owned by the
 *          main window after the return of this function.
 *          Do NOT desallocate it.
 * 
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

void lttvwindow_report_filter(Tab *tab, LttvFilter *filter);



/**
 * Function to get the stats of the traceset 
 * It must be non const so the viewer can modify it.
 * FIXME : a set/get pair of functions would be more appropriate here.
 * @param tab the tab the viewer belongs to.
 * @return A pointer to Traceset statistics.
 */

LttvTracesetStats* lttvwindow_get_traceset_stats(Tab *tab);

/**
 * Function to get the context of the traceset 
 * It must be non const so the viewer can add and remove hooks from it.
 * @param tab the tab the viewer belongs to.
 * @return Context of the current tab.
 */


LttvTracesetContext* lttvwindow_get_traceset_context(Tab *tab);


/* set_time_window 
 *
 * It updates the time window of the tab, then calls the updatetimewindow
 * hooks of each viewer.
 *
 * This is called whenever the scrollbar value changes.
 *
 * This is mostly an internal function.
 */

void set_time_window(Tab *tab, const TimeWindow *time_window);


/* set_current_time
 *
 * It updates the current time of the tab, then calls the updatetimewindow
 * hooks of each viewer.
 *
 * This is called whenever the current time value changes.
 *
 * This is mostly an internal function.
 */

void set_current_time(Tab *tab, const LttTime *current_time);

void events_request_free(EventsRequest *events_request);

GtkWidget *main_window_get_widget(Tab *tab);

void set_current_position(Tab *tab, const LttvTracesetContextPosition *pos);


/**
 * Function to disable the EventsRequests scheduler, nestable.
 *
 */
static inline void lttvwindow_events_request_disable(void)
{
  lttvwindow_preempt_count++;
}

/**
 * Function to restore the EventsRequests scheduler, nestable.
 *
 */
static inline void lttvwindow_events_request_enable(void)
{
  lttvwindow_preempt_count--;
}


void current_position_change_manager(Tab *tab,
                                     LttvTracesetContextPosition *pos);

#endif //LTTVWINDOW_H
