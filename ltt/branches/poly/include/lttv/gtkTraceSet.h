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

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param ButtonPixmap image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void ToolbarItemReg(GdkPixmap * pixmap, char *tooltip, void *view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void ToolbarItemUnreg(void *view_constructor);


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void MenuItemReg(char *menu_path, char *menu_text, void *view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void MenuItemUnreg(void *view_constructor);


/**
 * Attach a viewer to the current tab.
 * It will be called in the constructor of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param viewer viewer to be attached to the current tab
 */

// Not Needed : Main window add widget returned by constructor
//void AttachViewer(mainWindow *main_win, GtkWidget *viewer);


/* ?? Maybe we do not need this function, when a widget is destroyed, 
 *    it will be removed automatically from its container             
 */
// Not needed
/**
 * Detach a viewer from the current tab.
 * It will be called in the destructor of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param viewer viewer to be detached from the current tab.
 */

//void DetachViewer(mainWindow *main_win, GtkWidget *viewer);


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void UpdateStatus(mainWindow *main_win, char *info);


/**
 * Function to get the current time interval of the current tab.
 * It will be called by a viewer's hook function to update the 
 * time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

void GetTimeInterval(mainWindow *main_win, TimeInterval *time_interval);


/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void SetTimeInterval(mainWindow *main_win, TimeInterval *time_interval);


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

void GetCurrentTime(mainWindow *main_win, LttTime *time);


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void SetCurrentTime(mainWindow *main_win, LttTime *time);


/**
 * Function to get the traceset from the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its traceset.
 * @param main_win the main window the viewer belongs to.
 * @param traceset a pointer to a traceset.
 */

//void GetTraceset(mainWindow *main_win, Traceset *traceset);


/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

//void GetFilter(mainWindow *main_win, Filter *filter);


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateTimeInterval(LttvHook hook, gpointer hook_data,
			   mainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateTimeInterval(LttvHook hook, gpointer hook_data,
			     mainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateTraceset(LttvHook hook, gpointer hook_data,
		       mainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateTraceset(LttvHook hook, gpointer hook_data,
			 mainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateFilter(LttvHook hook, gpointer hook_data, 
		     mainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateFilter(LttvHook hook,  gpointer hook_data,
		       mainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateCurrentTime(LttvHook hook, gpointer hook_data, 
			  mainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateCurrentTime(LttvHook hook, gpointer hook_data,
			    mainWindow * main_win);


/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */

void SetFocusedPane(mainWindow *main_win, gpointer paned);


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateDividor(LttvHook hook, gpointer hook_data, 
		      mainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateDividor(LttvHook hook, gpointer hook_data, 
			mainWindow *main_win);


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void SetHPaneDividor(mainWindow *main_win, gint position);


/**
 * Function to process traceset. It will call lttv_process_trace, 
 * each view will call this api to get events.
 * @param main_win the main window the viewer belongs to.
 * @param start the start time of the first event to be processed.
 * @param end the end time of the last event to be processed.
 */

void processTraceset(mainWindow *main_win, LttTime start, LttTime end);


/**
 * Function to add hooks into the context of a traceset,
 * before reading events from traceset, viewer will call this api to
 * register hooks
 * @param main_win the main window the viewer belongs to.
 * @param LttvHooks hooks to be registered.
 */

void contextAddHooks(mainWindow *main_win ,
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

void contextRemoveHooks(mainWindow *main_win ,
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


