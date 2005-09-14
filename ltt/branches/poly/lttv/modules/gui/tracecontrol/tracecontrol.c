/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Mathieu Desnoyers
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/filter.h>

#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>

#include "hTraceControlInsert.xpm"


GSList *g_control_list = NULL ;

/*! \file lttv/modules/gui/tracecontrol/tracecontrol.c
 *  \brief Graphic trace start/stop control interface.
 *
 * This plugin interacts with lttctl to start/stop tracing. It needs to take the
 * root password to be able to interact with lttctl.
 *
 */

typedef struct _ControlData ControlData;

/*
 * Prototypes
 */
GtkWidget *guicontrol_get_widget(ControlData *tcd);
ControlData *gui_control(Tab *tab);
void gui_control_destructor(ControlData *tcd);
GtkWidget* h_guicontrol(Tab *tab);
void control_destroy_walk(gpointer data, gpointer user_data);
  
/*
 * Callback functions
 */


/**
 *  @struct _ControlData
 *  
 *  @brief Main structure of gui filter
 *  Main struct for the filter gui module
 */
struct _ControlData {
  Tab *tab;                             /**< current tab of module */

  GtkWidget *f_window;                  /**< filter window */
  
  GtkWidget *f_main_box;                /**< main container */

};

/**
 *  @fn GtkWidget* guicontrol_get_widget(ControlData*)
 * 
 *  This function returns the current main widget 
 *  used by this module
 *  @param tcd the module struct
 *  @return The main widget
 */
GtkWidget*
guicontrol_get_widget(ControlData *tcd)
{
  return tcd->f_window;
}

/**
 *  @fn ControlData* gui_control(Tab*)
 * 
 *  Constructor is used to create ControlData data structure.
 *  @param tab The tab structure used by the widget
 *  @return The Filter viewer data created.
 */
ControlData*
gui_control(Tab *tab)
{
  g_debug("filter::gui_control()");

  unsigned i;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  ControlData* tcd = g_new(ControlData,1);

  tcd->tab  = tab;

  tcd->f_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(tcd->f_window), "LTTng Trace Control");
  /* 
   * Initiating GtkTable layout 
   * starts with 2 rows and 5 columns and 
   * expands when expressions added
   */
  tcd->f_main_box = gtk_table_new(13,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(tcd->f_main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(tcd->f_main_box),5);
  
  gtk_container_add(GTK_CONTAINER(tcd->f_window), GTK_WIDGET(tcd->f_main_box));
  
  /*
   *  First half of the filter window
   *  - textual entry of filter expression
   *  - processing button
   */
  GtkWidget *username_label = gtk_label_new("Username:");
  gtk_widget_show (username_label);
  GtkWidget *username_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(username_entry),"root");

  gtk_widget_show (username_entry);

  gtk_table_attach( GTK_TABLE(tcd->f_main_box),username_label,0,6,0,1,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),username_entry,6,7,0,1,GTK_FILL,GTK_FILL,0,0);


  /* 
   * show main container 
   */
  gtk_widget_show(tcd->f_main_box);
  gtk_widget_show(tcd->f_window);
  
  
  g_object_set_data_full(
      G_OBJECT(guifilter_get_widget(tcd)),
      "filter_viewer_data",
      tcd,
      (GDestroyNotify)gui_control_destructor);

  g_control_list = g_slist_append(
      g_control_list,
      tcd);
  
  return tcd;
}


/**
 *  @fn void gui_control_destructor(ControlData*)
 * 
 *  Destructor for the filter gui module
 *  @param tcd The module structure
 */
void
gui_control_destructor(ControlData *tcd)
{
  Tab *tab = tcd->tab;

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(guifilter_get_widget(tcd))){
    g_info("widget still exists");
  }
//  if(tab != NULL) {
//    lttvwindow_unregister_traceset_notify(tcd->tab,
//                                          filter_traceset_changed,
//                                          filter_viewer_data);
//  }
  lttvwindowtraces_background_notify_remove(tcd);
  
  g_control_list = g_slist_remove(g_control_list, tcd);
 
  g_free(tcd);
}


/**
 *  @fn GtkWidget* h_guicontrol(Tab*)
 * 
 *  Control Module's constructor hook
 *
 *  This constructor is given as a parameter to the menuitem and toolbar button
 *  registration. It creates the list.
 *  @param tab A pointer to the parent window.
 *  @return The widget created.
 */
GtkWidget *
h_guicontrol(Tab *tab)
{
  ControlData* f = gui_control(tab) ;

  return NULL;
}

/**
 *  @fn static void init()
 * 
 *  This function initializes the Filter Viewer functionnality through the
 *  gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guicontrol",
                                  "/",
                                  "Insert Tracing Control Module",
                                  hTraceControlInsert_xpm,
                                  "Insert Tracing Control Module",
                                  h_guicontrol);
}

/**
 *  @fn void control_destroy_walk(gpointer,gpointer)
 * 
 *  Initiate the destruction of the current gui module
 *  on the GTK Interface
 */
void 
control_destroy_walk(gpointer data, gpointer user_data)
{
  ControlData *tcd = (ControlData*)data;

  g_debug("traceontrol.c : control_destroy_walk, %p", tcd);

  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guicontrol_get_widget(tcd)))
    gtk_widget_destroy(guicontrol_get_widget(tcd));
}

/**
 *  @fn static void destroy()
 *  @brief plugin's destroy function
 *
 *  This function releases the memory reserved by the module and unregisters
 *  everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
 
  g_slist_foreach(g_control_list, control_destroy_walk, NULL );
  
  lttvwindow_unregister_constructor(h_guicontrol);
  
}


LTTV_MODULE("guitracecontrol", "Trace Control Window", \
    "Graphical module that let user control kernel tracing", \
    init, destroy, "lttvwindow")

