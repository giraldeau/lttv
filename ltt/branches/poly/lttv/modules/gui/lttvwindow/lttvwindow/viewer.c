/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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

/*! \file lttvviewer.c
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

#include <lttvwindow/common.h>
#include <ltt/ltt.h>
#include <lttv/lttv.h>
#include <lttvwindow/mainwindow.h>   
#include <lttvwindow/viewer.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/toolbar.h>
#include <lttvwindow/menu.h>
#include <lttv/state.h>
#include <lttv/stats.h>


/**
 * Internal function parts
 */

/**
 * Function to set/update traceset for the viewers
 * @param main_win main window 
 * @param traceset traceset of the main window.
 */

void SetTraceset(MainWindow * main_win, gpointer traceset)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
     "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL)return;
  lttv_hooks_call(tmp,traceset);
}


/**
 * Function to set/update filter for the viewers
 * @param main_win main window 
 * @param filter filter of the main window.
 */

void SetFilter(MainWindow * main_win, gpointer filter)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
     "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp,filter);
}



/**
 * API parts
 */

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param ButtonPixmap image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void toolbar_item_reg(char ** pixmap, char *tooltip, lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar == NULL){    
    toolbar = lttv_toolbars_new();
    *(value.v_pointer) = toolbar;
  }
  lttv_toolbars_add(toolbar, view_constructor, tooltip, pixmap);
}


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void toolbar_item_unreg(lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);
  
  main_window_remove_toolbar_item(view_constructor);

  lttv_toolbars_remove(toolbar, view_constructor);
}


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void menu_item_reg(char *menu_path, char *menu_text, lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);
  
  if(menu == NULL){    
    menu = lttv_menus_new();
    *(value.v_pointer) = menu;
  }
  lttv_menus_add(menu, view_constructor, menu_path, menu_text);
}

/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void menu_item_unreg(lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
                              "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  main_window_remove_menu_item(view_constructor);

  lttv_menus_remove(menu, view_constructor);
}


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void update_status(MainWindow *main_win, char *info)
{
}


/**
 * Function to get the current time interval shown on the current tab.
 * It will be called by a viewer's hook function to update the 
 * shown time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

void get_time_window(MainWindow *main_win, TimeWindow *time_window)
{
  //time_window->start_time = main_win->current_tab->time_window.start_time;
  //time_window->time_width = main_win->current_tab->time_window.time_width;
  *time_window = main_win->current_tab->time_window;
}

/**
 * Function to get the current time interval of the current traceset.
 * It will be called by a viewer's hook function to update the 
 * time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

void get_traceset_time_span(MainWindow *main_win, TimeInterval *time_interval)
{
  //time_window->start_time = main_win->current_tab->time_window.start_time;
  //time_window->time_width = main_win->current_tab->time_window.time_width;
  *time_interval = *(LTTV_TRACESET_CONTEXT(main_win->current_tab->traceset_info->
             traceset_context)->Time_Span);
}



/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void set_time_window(MainWindow *main_win, TimeWindow *time_window)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  main_win->current_tab->time_window = *time_window;
  gtk_multi_vpaned_set_scroll_value(main_win->current_tab->multi_vpaned,
            ltt_time_to_double(time_window->start_time)
            * NANOSECONDS_PER_SECOND );
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, time_window);
}


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

void get_current_time(MainWindow *main_win, LttTime *time)
{
  time = &main_win->current_tab->current_time;
}


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void set_current_time(MainWindow *main_win, LttTime *time)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  main_win->current_tab->current_time = *time;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp, time);
}


/**
 * Function to get the traceset from the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its traceset.
 * @param main_win the main window the viewer belongs to.
 * @param traceset a pointer to a traceset.
 */
/*
void get_traceset(MainWindow *main_win, Traceset *traceset)
{
}
*/

/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */
/*
void get_filter(MainWindow *main_win, Filter *filter)
{
}
*/

/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_time_window(LttvHook hook, gpointer hook_data,
         MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook,hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_time_window(LttvHook hook, gpointer hook_data,
           MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_traceset(LttvHook hook, gpointer hook_data,
           MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_traceset(LttvHook hook, gpointer hook_data,
       MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to redraw each viewer belonging to the current tab 
 * @param main_win the main window the viewer belongs to.
 */

void update_traceset(MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, NULL);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_filter(LttvHook hook, gpointer hook_data,
         MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_filter(LttvHook hook, gpointer hook_data,
           MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_current_time(LttvHook hook, gpointer hook_data, 
        MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_current_time(LttvHook hook, gpointer hook_data,
          MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to show 
 *the content of the viewer.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_show_viewer(LttvHook hook, gpointer hook_data, 
         MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * show the content of the viewer..
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_show_viewer(LttvHook hook, gpointer hook_data,
           MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to show each viewer in the current tab.
 * It will be called by main window after it called process_traceset 
 * @param main_win the main window the viewer belongs to.
 */

void show_viewer(MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, NULL);
}


/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */

void set_focused_pane(MainWindow *main_win, gpointer paned)
{
  gtk_multi_vpaned_set_focus((GtkWidget*)main_win->current_tab->multi_vpaned,paned);  
}


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void reg_update_dividor(LttvHook hook, gpointer hook_data, 
          MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void unreg_update_dividor(LttvHook hook, gpointer hook_data, 
      MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void set_hpane_dividor(MainWindow *main_win, gint position)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &position);
}

/**
 * Function to get the stats of the traceset 
 * @param main_win the main window the viewer belongs to.
 */

LttvTracesetStats* get_traceset_stats(MainWindow *main_win)
{
  return main_win->current_tab->traceset_info->traceset_context;
}


LttvTracesetContext* get_traceset_context(MainWindow *main_win)
{
  return (LttvTracesetContext*)main_win->current_tab->traceset_info->traceset_context;
}
