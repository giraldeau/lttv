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

/*
 * TODO
 * - connect the gui filter to the core filter
 */

typedef struct _FilterViewerData FilterViewerData;

/*
 * Prototypes
 */
GtkWidget *guifilter_get_widget(FilterViewerData *fvd);
FilterViewerData *gui_filter(Tab *tab);
void gui_filter_destructor(FilterViewerData *fvd);
gboolean filter_traceset_changed(void * hook_data, void * call_data);
gboolean filter_viewer_data(void * hook_data, void * call_data); 
GtkWidget* h_guifilter(Tab *tab);
void statistic_destroy_walk(gpointer data, gpointer user_data);
  
struct _FilterViewerData {
  Tab *tab;

  GtkWidget *f_main_box;

  GtkWidget *f_hbox1;
  GtkWidget *f_hbox2;
  
  GtkWidget *f_expression_field;
  GtkWidget *f_process_button;

  GtkWidget *f_logical_op_box;
  GtkWidget *f_struct_box;
  GtkWidget *f_subfield_box;
  GtkWidget *f_math_op_box;
  GtkWidget *f_value_field;

  GtkWidget *f_textwnd;
  GtkWidget *f_selectwnd;
  GtkWidget *f_treewnd;
  
};

GtkWidget 
*guifilter_get_widget(FilterViewerData *fvd)
{
  return fvd->f_main_box;
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
  g_print("filter::gui_filter()");
  
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  FilterViewerData* fvd = g_new(FilterViewerData,1);

  fvd->tab  = tab;

  lttvwindow_register_traceset_notify(fvd->tab,
                                      filter_traceset_changed,
                                      filter_viewer_data);
//  request_background_data(filter_viewer_data);
 
  /* 
   * Initiating GtkTable layout 
   * starts with 2 rows and 5 columns and 
   * expands when expressions added
   */
  fvd->f_main_box = gtk_table_new(2,5,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(fvd->f_main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(fvd->f_main_box),5);
  
  /*
   *  First half of the filter window
   *  - textual entry of filter expression
   *  - processing button
   */
  fvd->f_expression_field = gtk_entry_new(); //gtk_scrolled_window_new (NULL, NULL);
  gtk_entry_set_text(GTK_ENTRY(fvd->f_expression_field),"state.cpu>0");
  gtk_widget_show (fvd->f_expression_field);

  fvd->f_process_button = gtk_button_new_with_label("Process");
  gtk_widget_show (fvd->f_process_button);
  
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_expression_field,0,4,0,1,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_process_button,4,5,0,1,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_process_button,4,5,0,1,GTK_SHRINK,GTK_FILL,0,0);
  
  /*
   *  Second half of the filter window
   *  - combo boxes featuring filtering options added to the expression
   */
  fvd->f_logical_op_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_box), "&");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_box), "|");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_box), "^");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_box), "!");
  gtk_widget_show(fvd->f_logical_op_box);
  
  fvd->f_struct_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_struct_box), "event");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_struct_box), "tracefile");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_struct_box), "trace");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_struct_box), "state");
  gtk_widget_show(fvd->f_struct_box);
 
  fvd->f_subfield_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "name");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "category");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "tsc");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "pid");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "ppid");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "creation time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "insertion time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "process name");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "execution mode");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "execution submode");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "process status");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_subfield_box), "cpu");
  gtk_widget_show(fvd->f_subfield_box);
 
  fvd->f_math_op_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), "=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), "!=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), "<");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), "<=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), ">");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_math_op_box), ">=");
  gtk_widget_show(fvd->f_math_op_box);

  fvd->f_value_field = gtk_entry_new();
  gtk_widget_show(fvd->f_value_field);
  
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_logical_op_box,0,1,1,2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_struct_box,1,2,1,2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_subfield_box,2,3,1,2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_math_op_box,3,4,1,2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_value_field,4,5,1,2,GTK_SHRINK,GTK_FILL,0,0);
   
  /* show main container */
  gtk_widget_show(fvd->f_main_box);
  
  
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

