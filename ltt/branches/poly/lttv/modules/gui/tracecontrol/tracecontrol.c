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
#include "TraceControlStart.xpm"
#include "TraceControlPause.xpm"
#include "TraceControlStop.xpm"


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
  tcd->f_main_box = gtk_table_new(14,7,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(tcd->f_main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(tcd->f_main_box),5);
  
  gtk_container_add(GTK_CONTAINER(tcd->f_window), GTK_WIDGET(tcd->f_main_box));
  
  /*
   * start/pause/stop buttons
   */
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlStart_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  GtkWidget *start_button = gtk_button_new_with_label("start");
  gtk_button_set_image(GTK_BUTTON(start_button), image);
  gtk_button_set_alignment(GTK_BUTTON(start_button), 0.0, 0.0);
  gtk_widget_show (start_button);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),start_button,6,7,0,1,GTK_FILL,GTK_FILL,2,2);
  
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlPause_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  GtkWidget *pause_button = gtk_button_new_with_label("pause");
  gtk_button_set_image(GTK_BUTTON(pause_button), image);
  gtk_button_set_alignment(GTK_BUTTON(pause_button), 0.0, 0.0);
  gtk_widget_show (pause_button);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),pause_button,6,7,1,2,GTK_FILL,GTK_FILL,2,2);

  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlStop_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  GtkWidget *stop_button = gtk_button_new_with_label("stop");
  gtk_button_set_image(GTK_BUTTON(stop_button), image);
  gtk_button_set_alignment(GTK_BUTTON(stop_button), 0.0, 0.0);
  gtk_widget_show (stop_button);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),stop_button,6,7,2,3,GTK_FILL,GTK_FILL,2,2);
  
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
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),username_label,0,2,0,1,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),username_entry,2,6,0,1,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);



  GtkWidget *password_label = gtk_label_new("Password:");
  gtk_widget_show (password_label);
  GtkWidget *password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
  gtk_widget_show (password_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),password_label,0,2,1,2,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),password_entry,2,6,1,2,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


  GtkWidget *channel_dir_label = gtk_label_new("Channel directory:");
  gtk_widget_show (channel_dir_label);
  GtkWidget *channel_dir_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(channel_dir_entry),"/mnt/relayfs/ltt");
  gtk_widget_show (channel_dir_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),channel_dir_label,0,2,2,3,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),channel_dir_entry,2,6,2,3,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *trace_dir_label = gtk_label_new("Trace directory:");
  gtk_widget_show (trace_dir_label);
  GtkWidget *trace_dir_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(trace_dir_entry),"/tmp/trace1");
  gtk_widget_show (trace_dir_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_dir_label,0,2,3,4,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_dir_entry,2,6,3,4,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *trace_name_label = gtk_label_new("Trace name:");
  gtk_widget_show (trace_name_label);
  GtkWidget *trace_name_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(trace_name_entry),"trace");
  gtk_widget_show (trace_name_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_name_label,0,2,4,5,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_name_entry,2,6,4,5,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *trace_mode_label = gtk_label_new("Trace mode ");
  gtk_widget_show (trace_mode_label);
  GtkWidget *trace_mode_combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(trace_mode_combo), 
      "normal");
  gtk_combo_box_append_text(GTK_COMBO_BOX(trace_mode_combo), 
      "flight recorder");
  gtk_combo_box_set_active(GTK_COMBO_BOX(trace_mode_combo), 0);
  gtk_widget_show (trace_mode_combo);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_mode_label,0,2,5,6,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),trace_mode_combo,2,6,5,6,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *start_daemon_label = gtk_label_new("Start daemon ");
  gtk_widget_show (start_daemon_label);
  GtkWidget *start_daemon_check = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(start_daemon_check), TRUE);
  gtk_widget_show (start_daemon_check);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),start_daemon_label,0,2,6,7,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),start_daemon_check,2,6,6,7,GTK_FILL,GTK_FILL,0,0);

  GtkWidget *optional_label = gtk_label_new("Optional fields ");
  gtk_widget_show (optional_label);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),optional_label,0,6,7,8,GTK_FILL,GTK_FILL,2,2);

  GtkWidget *subbuf_size_label = gtk_label_new("Subbuffer size:");
  gtk_widget_show (subbuf_size_label);
  GtkWidget *subbuf_size_entry = gtk_entry_new();
  gtk_widget_show (subbuf_size_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),subbuf_size_label,0,2,8,9,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),subbuf_size_entry,2,6,8,9,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *subbuf_num_label = gtk_label_new("Number of subbuffers:");
  gtk_widget_show (subbuf_num_label);
  GtkWidget *subbuf_num_entry = gtk_entry_new();
  gtk_widget_show (subbuf_num_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),subbuf_num_label,0,2,9,10,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),subbuf_num_entry,2,6,9,10,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  GtkWidget *lttctl_path_label = gtk_label_new("path to lttctl:");
  gtk_widget_show (lttctl_path_label);
  GtkWidget *lttctl_path_entry = gtk_entry_new();
  gtk_widget_show (lttctl_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),lttctl_path_label,0,2,10,11,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),lttctl_path_entry,2,6,10,11,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


  GtkWidget *lttd_path_label = gtk_label_new("path to lttd:");
  gtk_widget_show (lttd_path_label);
  GtkWidget *lttd_path_entry = gtk_entry_new();
  gtk_widget_show (lttd_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),lttd_path_label,0,2,11,12,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),lttd_path_entry,2,6,11,12,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  
  GtkWidget *fac_path_label = gtk_label_new("path to facilities:");
  gtk_widget_show (fac_path_label);
  GtkWidget *fac_path_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(fac_path_entry),"/usr/share/LinuxTraceToolkitViewer/facilities");
  gtk_widget_set_size_request(fac_path_entry, 250, -1);
  gtk_widget_show (fac_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),fac_path_label,0,2,12,13,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->f_main_box),fac_path_entry,2,6,12,13,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


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

