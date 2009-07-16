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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

__EXPORT gint lttvwindow_preempt_count = 0;

/* set_time_window 
 *
 * It updates the time window of the tab, then calls the updatetimewindow
 * hooks of each viewer.
 *
 * This is called whenever the scrollbar value changes.
 */

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
  if(tmp != NULL) lttv_hooks_call(tmp, &time_window_notify_data);

  //gtk_multi_vpaned_set_adjust(tab->multi_vpaned, new_time_window, FALSE);

}

/* set_current_time
 *
 * It updates the current time of the tab, then calls the updatetimewindow
 * hooks of each viewer.
 *
 * This is called whenever the current time value changes.
 */

void set_current_time(Tab *tab, const LttTime *current_time)
{
  LttvAttributeValue value;
  LttvHooks * tmp;

  tab->current_time = *current_time;

  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp != NULL) lttv_hooks_call(tmp, &tab->current_time);
}

/* set_current_position
 *
 * It updates the current time of the tab, then calls the updatetimewindow
 * hooks of each viewer.
 *
 * This is called whenever the current time value changes.
 */

void set_current_position(Tab *tab, const LttvTracesetContextPosition *pos)
{
  LttvAttributeValue value;
  LttvHooks * tmp;

  tab->current_time = lttv_traceset_context_position_get_time(pos);

  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrentposition", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp != NULL) lttv_hooks_call(tmp, pos);
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
  GtkWidget * tool_menu_title_menu, *widget;

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
 * @param name name of the viewer
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param pixmap Image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

__EXPORT void lttvwindow_register_constructor
                            (char *  name,
                             char *  menu_path, 
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

  if(view_constructor == NULL) return;
  
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
  {
    LttvAttribute *attribute;
    gboolean result;

    attribute = LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
                  LTTV_IATTRIBUTE(attributes_global),
                  LTTV_VIEWER_CONSTRUCTORS));
    g_assert(attribute);
  
    result = lttv_iattribute_find_by_path(LTTV_IATTRIBUTE(attribute),
                            name, LTTV_POINTER, &value);
    g_assert(result);

    *(value.v_pointer) = view_constructor;

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


__EXPORT void lttvwindow_unregister_constructor
                  (lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvMenus * menu;
  LttvAttributeValue value;
	gboolean is_named;

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

  {
    LttvAttribute *attribute;
    attribute = LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
         LTTV_IATTRIBUTE(attributes_global),
         LTTV_VIEWER_CONSTRUCTORS));
    g_assert(attribute);
  
    guint num = lttv_iattribute_get_number(LTTV_IATTRIBUTE(attribute));
    guint i;
    LttvAttributeName name;
    LttvAttributeValue value;
    LttvAttributeType type;
    
    for(i=0;i<num;i++) {
      type = lttv_iattribute_get(LTTV_IATTRIBUTE(attribute), i, &name, &value,
					&is_named);
      g_assert(type == LTTV_POINTER);
      if(*(value.v_pointer) == view_constructor) {
        lttv_iattribute_remove(LTTV_IATTRIBUTE(attribute), i);
        break;
      }
    }
  }
}


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */
__EXPORT void lttvwindow_register_time_window_notify(Tab *tab,
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

__EXPORT void lttvwindow_unregister_time_window_notify(Tab *tab,
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

__EXPORT void lttvwindow_register_traceset_notify(Tab *tab,
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

__EXPORT void lttvwindow_unregister_traceset_notify(Tab *tab,
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
 * Function to register a hook function for a viewer be completely redrawn.
 * 
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_register_redraw_notify(Tab *tab,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/redraw", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a hook function for a viewer be completely redrawn.
 *
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_unregister_redraw_notify(Tab *tab,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/redraw", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

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

__EXPORT void lttvwindow_register_continue_notify(Tab *tab,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/continue", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * Function to unregister a hook function for a viewer to re-do the events
 * requests for the needed interval.
 *
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_unregister_continue_notify(Tab *tab,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/continue", LTTV_POINTER, &value));
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

__EXPORT void lttvwindow_register_filter_notify(Tab *tab,
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

__EXPORT void lttvwindow_unregister_filter_notify(Tab *tab,
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
 * function to register a hook function for a viewer to set/update its 
 * current time.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_register_current_time_notify(Tab *tab,
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
 * function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_unregister_current_time_notify(Tab *tab,
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
 * function to register a hook function for a viewer to set/update its 
 * current position.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_register_current_position_notify(Tab *tab,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrentposition", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data, LTTV_PRIO_DEFAULT);
}


/**
 * function to unregister a viewer's hook function which is used to 
 * set/update the current position of the viewer.
 * @param tab viewer's tab 
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 */

__EXPORT void lttvwindow_unregister_current_position_notify(Tab *tab,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatecurrentposition", LTTV_POINTER, &value));
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
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param tab viewer's tab 
 * @param time_interval a pointer where time interval is stored.
 */

__EXPORT void lttvwindow_report_time_window(Tab *tab,
                                            TimeWindow time_window)
{
  //set_time_window(tab, time_window);
  //set_time_window_adjustment(tab, time_window);

  time_change_manager(tab, time_window);

  
#if 0    
  /* Set scrollbar */
  LttvTracesetContext *tsc =
        LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(tab->scrollbar));
  g_object_set(G_OBJECT(adjustment),
               "lower",
               0.0, /* lower */
               "upper",
               ltt_time_to_double(
                 ltt_time_sub(time_span.end_time, time_span.start_time)) 
                 , /* upper */
               "step_increment",
               ltt_time_to_double(time_window->time_width)
                             / SCROLL_STEP_PER_PAGE
                            , /* step increment */
               "page_increment",
               ltt_time_to_double(time_window->time_width) 
                 , /* page increment */
               "page_size",
               ltt_time_to_double(time_window->time_width) 
                 , /* page size */
               NULL);
  gtk_adjustment_changed(adjustment);

  //g_object_set(G_OBJECT(adjustment),
  //             "value",
  //             ltt_time_to_double(time_window->start_time) 
  //               , /* value */
  //               NULL);
  /* Note : the set value will call set_time_window if scrollbar value changed
   */
  gtk_adjustment_set_value(adjustment,
                           ltt_time_to_double(
                             ltt_time_sub(time_window->start_time,
                                          time_span.start_time))
                           );
#endif //0
}


/**
 * Function to set the current time of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param tab viewer's tab 
 * @param time a pointer where time is stored.
 */

__EXPORT void lttvwindow_report_current_time(Tab *tab,
                                    LttTime time)
{
  current_time_change_manager(tab, time);
}

/**
 * Function to set the current event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param tab viewer's tab 
 * @param time a pointer where time is stored.
 */

__EXPORT void lttvwindow_report_current_position(Tab *tab,
                                        LttvTracesetContextPosition *pos)
{
  current_position_change_manager(tab, pos);
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

__EXPORT void lttvwindow_events_request(Tab *tab,
                                        EventsRequest  *events_request)
{
  tab->events_requests = g_slist_append(tab->events_requests, events_request);
  
  if(!tab->events_request_pending)
  {
    /* Redraw has +20 priority. We want to let the redraw be done while we do
     * our job. Mathieu : test with high prio higher than events for better
     * scrolling. */
    /* Mathieu, 2008 : ok, finally, the control flow view needs the cell updates
     * to come soon enough so we can have one active cell to get the pixmap
     * buffer height from. Therefore, let the gdk events run before the events
     * requests.
     */
    g_idle_add_full((G_PRIORITY_HIGH_IDLE + 21),
    //g_idle_add_full((G_PRIORITY_DEFAULT + 2),
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


__EXPORT void lttvwindow_events_request_remove_all(Tab       *tab,
                                          gconstpointer   viewer)
{
  GSList *element = tab->events_requests;
  
  while((element = 
            g_slist_find_custom(element, viewer,
                                (GCompareFunc)find_viewer))
              != NULL) {
    EventsRequest *events_request = (EventsRequest *)element->data;
    // Modified so a viewer being destroyed won't have its after_request
    // called. Not so important anyway. Note that a viewer that call this
    // remove_all function will not get its after_request called.
    //if(events_request->servicing == TRUE) {
    //  lttv_hooks_call(events_request->after_request, NULL);
    //}
    events_request_free(events_request);
    //g_free(events_request);
    tab->events_requests = g_slist_remove_link(tab->events_requests, element);
    element = g_slist_next(element);
    if(element == NULL) break;   /* end of list */
  }
  if(g_slist_length(tab->events_requests) == 0) {
    tab->events_request_pending = FALSE;
    g_idle_remove_by_data(tab);
  }

}


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

__EXPORT gboolean lttvwindow_events_request_pending(Tab            *tab)
{
  GSList *element = tab->events_requests;

  if(element == NULL) return FALSE;
  else return TRUE;
}


/**
 * Function to get the current time interval shown on the current tab.
 * It will be called by a viewer's hook function to update the 
 * shown time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param tab viewer's tab 
 * @return time window.
 */

__EXPORT TimeWindow lttvwindow_get_time_window(Tab *tab)
{
  return tab->time_window;
}


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param tab viewer's tab 
 * @return time
 */

__EXPORT LttTime lttvwindow_get_current_time(Tab *tab)
{
  return tab->current_time;
}


/**
 * Function to get the filter of the current tab.
 * @param filter, a pointer to a filter.
 *
 * returns the current filter
 */
__EXPORT LttvFilter *lttvwindow_get_filter(Tab *tab)
{
  return g_object_get_data(G_OBJECT(tab->vbox), "filter");
}

/**
 * Function to set the filter of the current tab.
 * It should be called by the filter GUI to tell the
 * main window to update the filter tab's lttv_filter.
 *
 * This function does change the current filter, removing the
 * old one when necessary, and call the updatefilter hooks
 * of the registered viewers.
 *
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */
void lttvwindow_report_filter(Tab *tab, LttvFilter *filter)
{
  LttvAttributeValue value;
  LttvHooks * tmp;

  //lttv_filter_destroy(tab->filter);
  //tab->filter = filter;
  
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, filter);
}



/**
 * Function to get the stats of the traceset 
 * @param tab viewer's tab 
 */

__EXPORT LttvTracesetStats* lttvwindow_get_traceset_stats(Tab *tab)
{
  return tab->traceset_info->traceset_context;
}

__EXPORT LttvTracesetContext* lttvwindow_get_traceset_context(Tab *tab)
{
  return (LttvTracesetContext*)tab->traceset_info->traceset_context;
}


void events_request_free(EventsRequest *events_request)
{
  if(events_request == NULL) return;

  if(events_request->start_position != NULL)
       lttv_traceset_context_position_destroy(events_request->start_position);
  if(events_request->end_position != NULL)
       lttv_traceset_context_position_destroy(events_request->end_position);
  if(events_request->hooks != NULL) {
    GArray *hooks = events_request->hooks;
    lttv_trace_hook_remove_all(&hooks);
    g_array_free(events_request->hooks, TRUE);
  }
  if(events_request->before_chunk_traceset != NULL)
       lttv_hooks_destroy(events_request->before_chunk_traceset);
  if(events_request->before_chunk_trace != NULL)
       lttv_hooks_destroy(events_request->before_chunk_trace);
  if(events_request->before_chunk_tracefile != NULL)
       lttv_hooks_destroy(events_request->before_chunk_tracefile);
  if(events_request->event != NULL)
       lttv_hooks_destroy(events_request->event);
  if(events_request->event_by_id_channel != NULL)
       lttv_hooks_by_id_channel_destroy(events_request->event_by_id_channel);
  if(events_request->after_chunk_tracefile != NULL)
       lttv_hooks_destroy(events_request->after_chunk_tracefile);
  if(events_request->after_chunk_trace != NULL)
       lttv_hooks_destroy(events_request->after_chunk_trace);
  if(events_request->after_chunk_traceset != NULL)
       lttv_hooks_destroy(events_request->after_chunk_traceset);
  if(events_request->before_request != NULL)
       lttv_hooks_destroy(events_request->before_request);
  if(events_request->after_request != NULL)
       lttv_hooks_destroy(events_request->after_request);

  g_free(events_request);
}



__EXPORT GtkWidget *main_window_get_widget(Tab *tab)
{
  return tab->mw->mwindow;
}

