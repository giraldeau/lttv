/*! \file gtkTraceSet.h
 * \brief API used by the graphical viewers to interact with their top window.
 * 
 * Main window (gui module) is the place to contain and display viewers. 
 * Viewers (lttv plugins) interacte with main window through this API and
 * events sent by gtk.
 * This header file should be included in each graphic module.
 * This library is used by graphical modules to interact with the
 * tracesetWindow.
 * 
 */

#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <lttv/hook.h>
#include <lttv/common.h>
#include <lttv/stats.h>

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param ButtonPixmap image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void toolbar_item_reg(char ** pixmap, char *tooltip, lttv_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void toolbar_item_unreg(lttv_constructor view_constructor);


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void menu_item_reg(char *menu_path, char *menu_text, lttv_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void menu_item_unreg(lttv_constructor view_constructor);


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void update_status(MainWindow *main_win, char *info);


/**
 * Function to get the current time window of the current tab.
 * It will be called by a viewer's hook function to update the 
 * time window of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

void get_time_window(MainWindow *main_win, TimeWindow *time_window);


/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void set_time_window(MainWindow *main_win, TimeWindow *time_window);

/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

void get_current_time(MainWindow *main_win, LttTime *time);


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void set_current_time(MainWindow *main_win, LttTime *time);


/**
 * Function to get the traceset from the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its traceset.
 * @param main_win the main window the viewer belongs to.
 * @param traceset a pointer to a traceset.
 */

//void get_traceset(MainWindow *main_win, Traceset *traceset);


/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

//void get_filter(MainWindow *main_win, Filter *filter);


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer. Takes a TimeInterval* as call_data.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_time_window(LttvHook hook, gpointer hook_data,
			    MainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer. Takes a TimeInterval as call_data.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_time_window(LttvHook hook, gpointer hook_data,
			      MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_traceset(LttvHook hook, gpointer hook_data,
		       MainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_traceset(LttvHook hook, gpointer hook_data,
			 MainWindow * main_win);


/**
 * Function to redraw each viewer belonging to the current tab 
 * @param main_win the main window the viewer belongs to.
 */

void update_traceset(MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_filter(LttvHook hook, gpointer hook_data, 
		     MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_filter(LttvHook hook,  gpointer hook_data,
		       MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_current_time(LttvHook hook, gpointer hook_data, 
			  MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_current_time(LttvHook hook, gpointer hook_data,
			    MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to show 
 *the content of the viewer.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_show_viewer(LttvHook hook, gpointer hook_data, 
		     MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * show the content of the viewer..
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_show_viewer(LttvHook hook, gpointer hook_data,
		       MainWindow * main_win);


/**
 * Function to show each viewer in the current tab.
 * It will be called by main window after it called process_traceset 
 * @param main_win the main window the viewer belongs to.
 */

void show_viewer(MainWindow *main_win);


/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */

void set_focused_pane(MainWindow *main_win, gpointer paned);


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_dividor(LttvHook hook, gpointer hook_data, 
		      MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_dividor(LttvHook hook, gpointer hook_data, 
			MainWindow *main_win);


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void set_hpane_dividor(MainWindow *main_win, gint position);


/**
 * Function to process traceset. It will call lttv_process_trace, 
 * each view will call this api to get events.
 * @param main_win the main window the viewer belongs to.
 * @param start the start time of the first event to be processed.
 * @param end the end time of the last event to be processed.
 */

void process_traceset_api(MainWindow *main_win, LttTime start, 
			  LttTime end, unsigned maxNumEvents);


/**
 * Function to add hooks into the context of a traceset,
 * before reading events from traceset, viewer will call this api to
 * register hooks
 * @param main_win the main window the viewer belongs to.
 * @param LttvHooks hooks to be registered.
 */

void context_add_hooks_api(MainWindow *main_win ,
			   LttvHooks *before_traceset, 
			   LttvHooks *after_traceset,
			   LttvHooks *check_trace, 
			   LttvHooks *before_trace, 
			   LttvHooks *after_trace, 
			   LttvHooks *check_tracefile,
			   LttvHooks *before_tracefile,
			   LttvHooks *after_tracefile,
			   LttvHooks *check_event, 
			   LttvHooks *before_event, 
			   LttvHooks *after_event);


/**
 * Function to remove hooks from the context of a traceset,
 * before reading events from traceset, viewer will call this api to
 * unregister hooks
 * @param main_win the main window the viewer belongs to.
 * @param LttvHooks hooks to be registered.
 */

void context_remove_hooks_api(MainWindow *main_win ,
			      LttvHooks *before_traceset, 
			      LttvHooks *after_traceset,
			      LttvHooks *check_trace, 
			      LttvHooks *before_trace, 
			      LttvHooks *after_trace, 
			      LttvHooks *check_tracefile,
			      LttvHooks *before_tracefile,
			      LttvHooks *after_tracefile,
			      LttvHooks *check_event, 
			      LttvHooks *before_event, 
			      LttvHooks *after_event);


/**
 * Function to get the life span of the traceset
 * @param main_win the main window the viewer belongs to.
 * @param start start time of the traceset.
 * @param end end time of the traceset.
 */

void get_traceset_time_span(MainWindow *main_win, TimeInterval *time_span);


/**
 * Function to add/remove event hooks for state 
 * @param main_win the main window the viewer belongs to.
 */

void state_add_event_hooks_api(MainWindow *main_win );
void state_remove_event_hooks_api(MainWindow *main_win );


/**
 * Function to add/remove event hooks for stats 
 * @param main_win the main window the viewer belongs to.
 */

void stats_add_event_hooks_api(MainWindow *main_win );
void stats_remove_event_hooks_api(MainWindow *main_win );


/**
 * Function to get the stats of the traceset 
 * @param main_win the main window the viewer belongs to.
 */

LttvTracesetStats* get_traceset_stats_api(MainWindow *main_win);

LttvTracesetContext* get_traceset_context(MainWindow *main_win);
