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

/*! \file lttvwindow.c
 * \brief API used by the graphical viewers to interact with their tab.
 * 
 * Main window (gui module) is the place to contain and display viewers. 
 * Viewers (lttv plugins) interact with tab and main window through this API
 * and events sent by gtk.
 * This header file should be included in each graphic module.
 * This library is used by graphical modules to interact with their tab and
 * main window.
 * 
 */

#include <ltt/ltt.h>
#include <lttv/lttv.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/mainwindow.h>   
#include <lttvwindow/mainwindow-private.h>   
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/toolbar.h>
#include <lttvwindow/menu.h>
#include <lttvwindow/callbacks.h> // for execute_events_requests
#include <lttvwindow/support.h>


/**
 * Internal function parts
 */

extern GSList * g_main_window_list;

/**
 * Function to set/update traceset for the viewers
 * @param tab viewer's tab 
 * @param traceset traceset of the main window.
 * return value :
 * -1 : error
 *  0 : traceset updated
 *  1 : no traceset hooks to update; not an error.
 */

int SetTraceset(Tab * tab, LttvTraceset *traceset)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  if( lttv_iattribute_find_by_path(tab->attributes,
     "hooks/updatetraceset", LTTV_POINTER, &value) != 0)
    return -1;

  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return 1;
  

  lttv_hooks_call(tmp,traceset);
  
  return 0;
}


/**
 * Function to set/update filter for the viewers
 * @param tab viewer's tab 
 * @param filter filter of the main window.
 * return value :
 * -1 : error
 *  0 : filters updated
 *  1 : no filter hooks to update; not an error.
 */

int SetFilter(Tab * tab, gpointer filter)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  if(lttv_iattribute_find_by_path(tab->attributes,
     "hooks/updatefilter", LTTV_POINTER, &value) != 0)
    return -1;

  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL) return 1;
  lttv_hooks_call(tmp,filter);

  return 0;
}

/**
 * Function to redraw each viewer belonging to the current tab 
 * @param tab viewer's tab 
 */

void update_traceset(Tab *tab)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, NULL);
}

void set_time_window_adjustment(Tab *tab, const TimeWindow* new_time_window)
{
  gtk_multi_vpaned_set_adjust(tab->multi_vpaned, new_time_window, FALSE);
}


void set_time_window(Tab *tab, const TimeWindow *time_window)
{
  LttvAttributeValue value;
  LttvHooks * tmp;

  TimeWindowNotifyData time_window_notify_data;
  TimeWindow old_time_window = tab->time_window;
  time_window_notify_data.old_time_window = &old_time_window;
  tab->time_window = *time_window;
  time_window_notify_data.new_time_window = 
                          &(tab->time_window);

  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &time_window_notify_data);


}

void add_toolbar_constructor(MainWindow *mw, LttvToolbarClosure *toolbar_c)
{
  LttvIAttribute *attributes = mw->attributes;
  LttvAttributeValue value;
  LttvToolbars * instance_toolbar;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *new_widget, *pixmap;
  GdkPixbuf *pixbuf;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_toolbars_new();
  instance_toolbar = (LttvToolbars*)*(value.v_pointer);

  constructor = toolbar_c->con;
  tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar1");
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)toolbar_c->pixmap);
  pixmap = gtk_image_new_from_pixbuf(pixbuf);
  new_widget =
     gtk_toolbar_append_element (GTK_TOOLBAR (tool_menu_title_menu),
        GTK_TOOLBAR_CHILD_BUTTON,
        NULL,
        "",
        toolbar_c->tooltip, NULL,
        pixmap, NULL, NULL);
  gtk_label_set_use_underline(
      GTK_LABEL (((GtkToolbarChild*) (
                       g_list_last (GTK_TOOLBAR 
                          (tool_menu_title_menu)->children)->data))->label),
      TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (new_widget), 1);
  g_signal_connect ((gpointer) new_widget,
      "clicked",
      G_CALLBACK (insert_viewer_wrap),
      constructor);       
  gtk_widget_show (new_widget);

  lttv_toolbars_add(instance_toolbar, toolbar_c->con, 
                    toolbar_c->tooltip,
                    toolbar_c->pixmap,
                    new_widget);

}

void add_menu_constructor(MainWindow *mw, LttvMenuClosure *menu_c)
{
  LttvIAttribute *attributes = mw->attributes;
  LttvAttributeValue value;
  LttvToolbars * instance_menu;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *new_widget;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_menus_new();
  instance_menu = (LttvMenus*)*(value.v_pointer);


  constructor = menu_c->con;
  tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
  new_widget =
          gtk_menu_item_new_with_mnemonic (menu_c->menu_text);
  gtk_container_add (GTK_CONTAINER (tool_menu_title_menu),
          new_widget);
  g_signal_connect ((gpointer) new_widget, "activate",
                      G_CALLBACK (insert_viewer_wrap),
                      constructor);
  gtk_widget_show (new_widget);
  lttv_menus_add(instance_menu, menu_c->con, 
                    menu_c->menu_path,
                    menu_c->menu_text,
                    new_widget);
}

void remove_toolbar_constructor(MainWindow *mw, lttvwindow_viewer_constructor viewer_constructor)
{
  LttvIAttribute *attributes = mw->attributes;
  LttvAttributeValue value;
  LttvToolbars * instance_toolbar;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *widget;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_toolbars_new();
  instance_toolbar = (LttvToolbars*)*(value.v_pointer);

  tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar1");
  widget = lttv_menus_remove(instance_toolbar, viewer_constructor);
  gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), 
                        widget);
}


void remove_menu_constructor(MainWindow *mw, lttvwindow_viewer_constructor viewer_constructor)
{
  LttvIAttribute *attributes = mw->attributes;
  LttvAttributeValue value;
  LttvMenus * instance_menu;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *widget;
  LttvMenuClosure *menu_item_i;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_menus_new();
  instance_menu = (LttvMenus*)*(value.v_pointer);

  widget = lttv_menus_remove(instance_menu, viewer_constructor);
  tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
  gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), widget);
}


/**
 * API parts
 */


/**
 * Function to register a view constructor so that main window can generate
 * a menu item and a toolbar item for the viewer in order to generate a new
 * instance easily. A menu entry and toolbar item will be added to each main
 * window.
 * 
 * It should be called by init function of the module.
 * 
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param pixmap Image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_constructor
                            (char *  menu_path, 
                             char *  menu_text,
                             char ** pixmap,
                             char *  tooltip,
                             lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvMenus * menu;
  LttvToolbarClosure toolbar_c;
  LttvMenuClosure menu_c;
  LttvAttributeValue value;

  if(pixmap != NULL) {
    g_assert(lttv_iattribute_find_by_path(attributes_global,
       "viewers/toolbar", LTTV_POINTER, &value));
    toolbar = (LttvToolbars*)*(value.v_pointer);

    if(toolbar == NULL) {
      toolbar = lttv_toolbars_new();
      *(value.v_pointer) = toolbar;
    }
    toolbar_c = lttv_toolbars_add(toolbar, view_constructor, tooltip, pixmap,
                                  NULL);

    g_slist_foreach(g_main_window_list,
                    (gpointer)add_toolbar_constructor,
                    &toolbar_c);
  }

  if(menu_path != NULL) {
    g_assert(lttv_iattribute_find_by_path(attributes_global,
       "viewers/menu", LTTV_POINTER, &value));
    menu = (LttvMenus*)*(value.v_pointer);
    
    if(menu == NULL) {
      menu = lttv_menus_new();
      *(value.v_pointer) = menu;
    }
    menu_c = lttv_menus_add(menu, view_constructor, menu_path, menu_text,NULL);

    g_slist_foreach(g_main_window_list,
                    (gpointer)add_menu_constructor,
                    &menu_c);
  }
}


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
                  (lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);
  
  if(toolbar != NULL) {
    g_slist_foreach(g_main_window_list,
                    (gpointer)remove_toolbar_constructor,
                    view_constructor);
    lttv_toolbars_remove(toolbar, view_constructor);
  }

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);
  
  if(menu != NULL) {
    g_slist_foreach(g_main_window_list,
                    (gpointer)remove_menu_constructor,
                    view_constructor);
    lttv_menus_remove(menu, view_constructor);
  }
}


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */
void lttvwindow_register_time_window_notify(Tab *tab,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook,hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_time_window_notify(Tab *tab,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_traceset_notify(Tab *tab,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_traceset_notify(Tab *tab,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_filter_notify(Tab *tab,
      LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_filter_notify(Tab *tab,
                                         LttvHook hook,
                                         gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_current_time_notify(Tab *tab,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_current_time_notify(Tab *tab,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to show 
 * the content of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_show_notify(Tab *tab,
          LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * show the content of the viewer..
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_show_notify(Tab *tab,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_register_dividor(Tab *tab,
                    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * It will be called by the destructor of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

void lttvwindow_unregister_dividor(Tab *tab,
                    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Update the status bar whenever something changed in the viewer.
 * @param tab viewer's tab 
 * @param info the message which will be shown in the status bar.
 */

void lttvwindow_report_status(Tab *tab, const char *info)
{ 
  //FIXME
  g_warning("update_status not implemented in viewer.c");
  // Use tab->mw for status
}

/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param tab viewer's tab 
 * @param time_interval a pointer where time interval is stored.
 */

void lttvwindow_report_time_window(Tab *tab,
                                   const TimeWindow *time_window)
{
  set_time_window(tab, time_window);
  set_time_window_adjustment(tab, time_window);
}


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param tab viewer's tab 
 * @param time a pointer where time is stored.
 */

void lttvwindow_report_current_time(Tab *tab,
                                    const LttTime *time)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  tab->current_time = *time;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp, &tab->current_time);
}

/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param tab viewer's tab 
 * @param position position of the hpane's dividor.
 */

void lttvwindow_report_dividor(Tab *tab, gint position)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &position);
}

/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param tab viewer's tab 
 * @param top_widget the top widget containing all the other widgets of the
 *                   viewer.
 */

void lttvwindow_report_focus(Tab *tab, GtkWidget *top_widget)
{
  gtk_multi_vpaned_set_focus((GtkWidget*)tab->multi_vpaned,
                             GTK_PANED(gtk_widget_get_parent(top_widget)));
}


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
 * @param tab viewer's tab 
 * @param events_requested the structure of request from.
 */

void lttvwindow_events_request(Tab *tab,
                               const EventsRequest  *events_request)
{
  EventsRequest *alloc = g_new(EventsRequest,1);
  *alloc = *events_request;

  tab->events_requests = g_slist_append(tab->events_requests, alloc);
  
  if(!tab->events_request_pending)
  {
    /* Redraw has +20 priority. We want a prio higher than that, so +19 */
    g_idle_add_full((G_PRIORITY_HIGH_IDLE + 19),
                    (GSourceFunc)execute_events_requests,
                    tab,
                    NULL);
    tab->events_request_pending = TRUE;
  }
}


/**
 * Function to remove data requests related to a viewer.
 *
 * The existing requests's viewer gpointer is compared to the pointer
 * given in argument to establish which data request should be removed.
 * 
 * @param tab the tab the viewer belongs to.
 * @param viewer a pointer to the viewer data structure
 */

gint find_viewer (const EventsRequest *a, gconstpointer b)
{
  return (a->owner != b);
}


void lttvwindow_events_request_remove_all(Tab       *tab,
                                          gconstpointer   viewer)
{
  GSList *element;
  
  while((element = 
            g_slist_find_custom(tab->events_requests, viewer,
                                (GCompareFunc)find_viewer))
              != NULL) {
    EventsRequest *events_request = (EventsRequest *)element->data;
    if(events_request->servicing == TRUE) {
      lttv_hooks_call(events_request->after_request, NULL);
    }
    g_free(events_request);
    tab->events_requests = g_slist_remove_link(tab->events_requests, element);

  }
}



/**
 * Function to get the current time interval shown on the current tab.
 * It will be called by a viewer's hook function to update the 
 * shown time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param tab viewer's tab 
 * @param time_interval a pointer where time interval will be stored.
 */

const TimeWindow *lttvwindow_get_time_window(Tab *tab)
{
  return &(tab->time_window);
  
}


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param tab viewer's tab 
 * @param time a pointer where time will be stored.
 */

const LttTime *lttvwindow_get_current_time(Tab *tab)
{
  return &(tab->current_time);
}


/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param tab viewer's tab 
 * @param filter, a pointer to a filter.
 */
const lttv_filter *lttvwindow_get_filter(Tab *tab)
{
  //FIXME
  g_warning("lttvwindow_get_filter not implemented in viewer.c");
}


/**
 * Function to get the stats of the traceset 
 * @param tab viewer's tab 
 */

LttvTracesetStats* lttvwindow_get_traceset_stats(Tab *tab)
{
  return tab->traceset_info->traceset_context;
}


LttvTracesetContext* lttvwindow_get_traceset_context(Tab *tab)
{
  return (LttvTracesetContext*)tab->traceset_info->traceset_context;
}
