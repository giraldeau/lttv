/* This file is part of the Linux Trace Toolkit viewer
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

A viewer plugin is, before anything, a plugin. It thus has an init and 
a destroy function called whenever it is loaded/initialized and 
unloaded/destroyed. A viewer depends on lttvwindow and thus uses its init and
destroy functions to register module related hooks defined in this file.


Viewer Instance Related API

The lifetime of a viewer is as follows. The viewer constructor function is
called each time an instance view is created (one subwindow of this viewer
type is created by the user either by clicking on the menu item or the button
corresponding to the viewer). Thereafter, the viewer gets hooks called for
different purposes by the window containing it. These hooks are detailed
below. It also has to deal with GTK Events. Finally, it can be destructed by
having its top level widget unreferenced by the main window or by any
GTK Event causing a "destroy-event" signal on the its top widget. Another
possible way for it do be destroyed is if the module gets unloaded. The module
unload function will have to emit a "destroy" signal on each top level widget
of all instances of its viewers.


Notices from Main Window :

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
              

FIXME : Add background computation explanation here
background_init: prepare for background computation (comes after show_end).
process_trace for background: done in small chunks in gtk_idle, hooks called.
background_end: remove the hooks and perhaps update the window.


Reporting Changes to the Main Window

In most cases, the enclosing window knows about updates such as described in the
Notification section higher. There are a few cases, however, where updates are
caused by actions known by a view instance. For example, clicking in a view may
update the current time; all viewers within the same window must be told about
the new current time to change the currently highlighted time point. A viewer
reports such events by calling lttvwindow_report_current_time on its lttvwindow.
The lttvwindow will thereafter call update for each of its contained viewers.


Available report methods are :

lttvwindow_report_status : reports the text of the status bar.
lttvwindow_report_time_window : reports the new time window.
lttvwindow_report_current_time : reports the new current time.
lttvwindow_report_dividor : reports the new horizontal dividor's position.
lttvwindow_report_focus : One on the widgets in the viewer has the keyboard's
                          focus from GTK.

Requiring Time Interval

FIXME : explain



GTK Events

FIXME: explain GTK Events distribution and signals propagation in details
(useful!)

The "expose_event"

Provides the exposed region in the GdkEventExpose structure. 

There are two ways of dealing with exposures. The first one is to directly draw
on the screen and the second one is to draw in a pixmap buffer, and then to 
update the screen when necessary.

In the first case, the expose event will be responsible for registering hooks to
process_traceset and require time intervals to the main window. So, in this
scenario, if a part of the screen is damaged, the trace has to be read to
redraw the screen.

In the second case, with a pixmap buffer, the expose handler is only responsible
of showing the pixmap buffer on the screen. If the pixmap buffer has never
been filled with a drawing, the expose handler may ask for it to be filled.

It can add dotted lines and such visual effects to enhance the user's
experience.


FIXME : explain other important events

*/




/*! \file viewer.h
 * \brief API used by the graphical viewers to interact with their top window.
 * 
 * Main window (lttvwindow module) is the place to contain and display viewers. 
 * Viewers (lttv plugins) interact with main window through this API.
 * This header file should be included in each graphic module.
 * 
 */

#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <lttv/hook.h>
#include <lttvwindow/common.h>
#include <lttv/stats.h>
//FIXME (not ready yet) #include <lttv/filter.h>


/* Module Related API */


/* constructor a the viewer */
//FIXME explain LttvTracesetSelector and key
typedef GtkWidget * (*lttvwindow_viewer_constructor)
                (MainWindow * main_window, LttvTracesetSelector * s, char *key);


/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * 
 * It should be called by init function of the module.
 * 
 * @param pixmap Image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_toolbar
                            (char ** pixmap,
                             char *  tooltip,
                             lttvwindow_viewer_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * 
 * It will be called when a module is unloaded.
 * 
 * @param view_constructor constructor of the viewer.
 */

void lttvwindow_unregister_toolbar
                            (lttvwindow_viewer_constructor view_constructor);


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * 
 * It will be called by init function of the module.
 * 
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_menu(char *menu_path, 
                              char *menu_text,
                              lttvwindow_viewer_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * 
 * It will be called when a module is unloaded.
 * 
 * @param view_constructor constructor of the viewer.
 */

void lttvwindow_unregister_menu(lttvwindow_viewer_constructor view_constructor);



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
 * @param main_win the main window the viewer belongs to.
 * @param hook hook that sould be called by the main window when the time
 *             interval changes. This hook function takes a
 *             TimeWindowNotifyData* as call_data.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_time_window_notify(MainWindow *main_win,
                                            LttvHook    hook,
                                            gpointer    hook_data);


/**
 * Function to unregister the time_window notification hook.
 * 
 * This unregister function is typically called by the destructor of the viewer.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param hook hook that sould be called by the main window when the time
 *             interval changes. This hook function takes a
 *             TimeWindowNotifyData* as call_data.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_time_window_notify(MainWindow *main_win,
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
 * @param main_win the main window the viewer belongs to.
 * @param hook hook that should be called whenever a change to the traceset
 *             occurs. The call_data of this hook is a NULL pointer.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_traceset_notify(MainWindow *main_win,
                                         LttvHook    hook,
                                         gpointer    hook_data);


/**
 * Function to unregister the traceset_notify hook.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param hook hook that should be called whenever a change to the traceset
 *             occurs. The call_data of this hook is a NULL pointer.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_traceset_notify(MainWindow *main_win,
                                           LttvHook    hook,
                                           gpointer    hook_data);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter. 
 *
 * FIXME : Add information about what a filter is as seen from a viewer and how
 * to use it.
 *
 * This register function is typically called by the constructor of the viewer.
 *
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function called by the main window when a filter change
 *             occurs.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_filter_notify(MainWindow *main_win,
                                       LttvHook    hook,
                                       gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * 
 * This unregistration is called by the destructor of the viewer.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function called by the main window when a filter change
 *             occurs.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_filter_notify(MainWindow * main_win,
                                         LttvHook     hook,
                                         gpointer     hook_data);


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_current_time_notify(MainWindow *main_win,
                                             LttvHook    hook,
                                             gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function of the viewer that updates the current time. The
 *             call_data is a LttTime* representing the new current time.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_current_time_notify(MainWindow *main_win,
                                               LttvHook    hook,
                                               gpointer    hook_data);


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane. It provides a way to make the horizontal
 * dividors of all the viewers linked together.
 *
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function of the viewer that will be called whenever a
 *             dividor changes in another viewer. The call_data of this hook
 *             is a gint*. The value of the integer is the new position of the
 *             hpane dividor.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_register_dividor(MainWindow *main_win,
                                 LttvHook    hook,
                                 gpointer    hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param hook hook function of the viewer that will be called whenever a
 *             dividor changes in another viewer. The call_data of this hook
 *             is a gint*. The value of the integer is the new position of the
 *             hpane dividor.
 * @param hook_data hook data associated with the hook function. It will
 *                  be typically a pointer to the viewer's data structure.
 */

void lttvwindow_unregister_dividor(MainWindow *main_win,
                                   LttvHook    hook,
                                   gpointer    hook_data);



/**
 * This method reports the information to show on the status bar in the
 * main window.
 * 
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void lttvwindow_report_status(MainWindow *main_win, const char *info);


/**
 * Function to set the time interval of the current tab.a
 *
 * @param main_win the main window the viewer belongs to.
 * @param time_interval pointer to the time interval value.
 */

void lttvwindow_report_time_window(MainWindow       *main_win,
                                   const TimeWindow *time_window);

/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void lttvwindow_report_current_time(MainWindow    *main_win, 
                                    const LttTime *time);


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will typically be called by a viewer's signal handle associated 
 * with the motion_notify_event event/signal.
 *
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void lttvwindow_report_dividor(MainWindow *main_win, gint position);

/**
 * Function to set the focused viewer of the tab.
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal of all widgets in the viewer.
 *
 * @param main_win the main window the viewer belongs to.
 * @param top_widget the top widget containing all the other widgets of the
 *                   viewer.
 */
void lttvwindow_report_focus(MainWindow *main_win, 
                             GtkWidget  *top_widget);



/* Structure sent to the time request hook */
typedef struct _TimeRequest {
  TimeWindow  time_window;
  guint num_events;
  LttvHook after_hook;
  gpointer after_hook_data;
} TimeRequest;

/**
 * Function to request data in a specific time interval to the main window. The
 * time request servicing is differed until the glib idle functions are
 * called.
 *
 * The viewer has to make sure that it has registered hooks in the main window's
 * traceset context before the glib idle's function gets called to ensure that
 * it will be called for the events it has asked for.
 *
 * @param main_win the main window the viewer belongs to.
 * @param time_requested the time requested by the viewer.
 * @param num_events the quantity of events to get (can be a little more if many
 *                   events have the same timestamp than the last one)
 * @param after_process_traceset hook called after the process traceset. It will
 *                               typically unregister the hooks in the context.
 *                               The call_data of this hook is a 
 *                               const TimeRequest*, corresponding to the 
 *                               original time request. It's there for
 *                               information purpose only and should not be
 *                               freed.
 * @param after_process_traceset_data hook data associated with the hook
 *                                    function. It will be typically a pointer
 *                                    to the viewer's data structure.
 */

void lttvwindow_time_interval_request(MainWindow *main_win,
                                      TimeWindow  time_requested,
                                      guint       num_events,
                                      LttvHook    after_process_traceset,
                                      gpointer    after_process_traceset_data);

/**
 * Function to get the life span of the traceset
 *
 * @param main_win the main window the viewer belongs to.
 * @return pointer to a time interval : the life span of the traceset.
 */

const TimeInterval *lttvwindow_get_time_span(MainWindow *main_win);

/**
 * Function to get the current time window of the current tab.
 * 
 * @param main_win the main window the viewer belongs to.
 * @return a pointer to the current tab's time interval.
 */

const TimeWindow *lttvwindow_get_time_window(MainWindow *main_win);


/**
 * Function to get the current time of the current tab.
 *
 * @param main_win the main window the viewer belongs to.
 * @return a pointer to the current tab's current time.
 */

const LttTime *lttvwindow_get_current_time(MainWindow *main_win);


/**
 * Function to get the filter of the current tab.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

//FIXME
typedef void lttv_filter;
//FIXME
const lttv_filter *lttvwindow_get_filter(MainWindow *main_win);


/**
 * Function to get the stats of the traceset 
 * It must be non const so the viewer can modify it.
 * FIXME : a set/get pair of functions would be more appropriate here.
 * @param main_win the main window the viewer belongs to.
 * @return A pointer to Traceset statistics.
 */

LttvTracesetStats* lttvwindow_get_traceset_stats(MainWindow *main_win);

/**
 * Function to get the context of the traceset 
 * It must be non const so the viewer can add and remove hooks from it.
 * @param main_win the main window the viewer belongs to.
 * @return Context of the current tab.
 */


LttvTracesetContext* lttvwindow_get_traceset_context(MainWindow *main_win);


