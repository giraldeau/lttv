/**
 * Main window (main module) is the place to contain and display viewers. 
 * Viewers (lttv modules) interacter with main window though API of the 
 * main window and hooks of itself.
 * This header file should be included in each graphic module.
 */

#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <lttv/hook.h>

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param pixmap, pixmap shown on the toolbar item.
 * @param tooltip, tooltip of the toolbar item.
 * @view_constructor, constructor of the viewer. 
 */

void ToolbarItemReg(GdkPixmap *pixmap, char *tooltip, void *view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor, constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void ToolbarItemUnreg(void *view_constructor);


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path, path of the menu item.
 * @param menu_text, text of the menu item.
 * @view_constructor, constructor of the viewer. 
 */

void MenuItemReg(char *menu_path, char *menu_text, void *view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor, constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void MenuItemUnreg(void *view_constructor);


/**
 * Attach a viewer to the current tab.
 * It will be called in the constructor of the viewer.
 * @param main_win, the main window the viewer belongs to.
 * @param viewer, viewer to be attached to the current tab
 */

void AttachViewer(MainWindow *main_win, GtkWidget *viewer);


/* ?? Maybe we do not need this function, when a widget is destoried, 
 *    it will be removed automatically from its container             
 */

/**
 * Detach a viewer from the current tab.
 * It will be called in the destructor of the viewer.
 * @param main_win, the main window the viewer belongs to.
 * @param viewer, viewer to be detached from the current tab.
 */

void DetachViewer(MainWindow *main_win, GtkWidget *viewer);


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win, the main window the viewer belongs to.
 * @param info, the message which will be shown in the status bar.
 */

void UpdateStatus(MainWindow *main_win, char *info);


/**
 * Function to get the current time interval of the current tab.
 * It will be called by a viewer's hook function to update the 
 * time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win, the main window the viewer belongs to.
 * @param time_interval, a pointer where time interval will be stored.
 */

void GetTimeInterval(MainWindow *main_win, TimeInterval *time_interval);


/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win, the main window the viewer belongs to.
 * @param time_interval, a pointer where time interval is stored.
 */

void SetTimeInterval(MainWindow *main_win, TimeInterval *time_interval);


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win, the main window the viewer belongs to.
 * @param ltt_time, a pointer where time will be stored.
 */

void GetCurrentTime(MainWindow *main_win, ltt_time *time);


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win, the main window the viewer belongs to.
 * @param ltt_time, a pointer where time is stored.
 */

void SetCurrentTime(MainWindow *main_win, ltt_time *time);


/**
 * Function to get the traceset from the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its traceset.
 * @param main_win, the main window the viewer belongs to.
 * @param traceset, a pointer to a traceset.
 */

void GetTraceset(MainWindow *main_win, Traceset *traceset);


/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

void GetFilter(MainWindow *main_win, Filter *filter);


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * It will be called by the constructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void RegUpdateTimeInterval(lttv_hook *hook, TimeInterval *hook_data,
			   MainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void UnregUpdateTimeInterval(lttv_hook *hook, TimeInterval *hook_data,
			     MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * It will be called by the constructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void RegUpdateTraceset(lttv_hook *hook, Traceset *hook_data,
		       MainWindow * main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void UnregUpdateTraceset(lttv_hook *hook, Traceset *hook_data,
			 MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * It will be called by the constructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void RegUpdateFilter(lttv_hook *hook, Filter *hook_data, 
		     MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void UnregUpdateFilter(lttv_hook *hook, Filter *hook_data,
		       MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * It will be called by the constructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void RegUpdateCurrentTime(lttv_hook *hook, ltt_time *hook_data, 
			  MainWindow *main_win);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook, hook function of the viewer.
 * @param hook_data, hook data associated with the hook function.
 * @param main_win, the main window the viewer belongs to.
 */

void UnregUpdateCurrentTime(lttv_hook *hook, ltt_time *hook_data,
			    MainWindow * main_win);


