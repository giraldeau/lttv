/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Simon Bouvier-Zappa
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

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/filter.h>

#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>

#include "hGuiFilterInsert.xpm"

typedef struct _FilterViewerData FilterViewerData;

GtkWidget *guifilter_get_widget(FilterViewerData *fvd);
FilterViewerData *gui_filter(Tab *tab);
void gui_filter_destructor(FilterViewerData *fvd);
gboolean filter_traceset_changed(void * hook_data, void * call_data);
gboolean filter_viewer_data(void * hook_data, void * call_data); 
GtkWidget* h_guifilter(Tab *tab);
void statistic_destroy_walk(gpointer data, gpointer user_data);
  
struct _FilterViewerData {
  Tab *tab;

  // temp widget -- still thinking about a formal structure
  GtkWidget *hbox;
  GtkWidget *f_textwnd;
  GtkWidget *f_selectwnd;
  GtkWidget *f_treewnd;
  
};

GtkWidget 
*guifilter_get_widget(FilterViewerData *fvd)
{
  return fvd->hbox;
}

/**
 * Statistic Viewer's constructor
 *
 * This constructor is used to create StatisticViewerData data structure.
 * @return The Statistic viewer data created.
 */
FilterViewerData*
gui_filter(Tab *tab)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  FilterViewerData* fvd = g_new(FilterViewerData,1);

  fvd->tab  = tab;

  lttvwindow_register_traceset_notify(fvd->tab,
                                      filter_traceset_changed,
                                      filter_viewer_data);
//  request_background_data(filter_viewer_data);
  
  fvd->f_textwnd = gtk_entry_new(); //gtk_scrolled_window_new (NULL, NULL);
  gtk_entry_set_text(fvd->f_textwnd,"this is a test");
  gtk_widget_show (fvd->f_textwnd);
    
  fvd->hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(fvd->hbox), fvd->f_textwnd, TRUE, TRUE, 0);
 
  gtk_widget_show(fvd->hbox);
  
  g_object_set_data_full(
      G_OBJECT(guifilter_get_widget(fvd)),
      "filter_viewer_data",
      fvd,
      (GDestroyNotify)gui_filter_destructor);

  return fvd;
}

void
gui_filter_destructor(FilterViewerData *fvd)
{
  Tab *tab = fvd->tab;

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(guifilter_get_widget(fvd))){
    g_info("widget still exists");
  }
  if(tab != NULL) {
    lttvwindow_unregister_traceset_notify(fvd->tab,
                                          filter_traceset_changed,
                                          filter_viewer_data);
  }
  lttvwindowtraces_background_notify_remove(fvd);

  g_free(fvd);
}

gboolean
filter_traceset_changed(void * hook_data, void * call_data) {

  return FALSE;
}

gboolean
filter_viewer_data(void * hook_data, void * call_data) {

  return FALSE;
}

/**
 * Filter Module's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_guifilter(Tab *tab)
{
  FilterViewerData* f = gui_filter(tab) ;

  g_print("FilterViewerData:%p\n",f);
  if(f)
    return guifilter_get_widget(f);
  else return NULL;
  
}



/**
 * plugin's init function
 *
 * This function initializes the Statistic Viewer functionnality through the
 * gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guifilter",
                                  "/",
                                  "Insert Filter Module",
                                  hGuiFilterInsert_xpm,
                                  "Insert Filter Module",
                                  h_guifilter);
}

void filter_destroy_walk(gpointer data, gpointer user_data)
{
  FilterViewerData *fvd = (FilterViewerData*)data;

  g_debug("CFV.c : statistic_destroy_walk, %p", fvd);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guifilter_get_widget(fvd)))
    gtk_widget_destroy(guifilter_get_widget(fvd));
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  
  lttvwindow_unregister_constructor(h_guifilter);
  
}


LTTV_MODULE("guifilter", "Filter window", \
    "Graphical module that let user specify their filtering options", \
    init, destroy, "lttvwindow")

