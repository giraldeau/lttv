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
typedef struct _FilterViewerDataLine FilterViewerDataLine;

/*
 * Prototypes
 */
GtkWidget *guifilter_get_widget(FilterViewerData *fvd);
FilterViewerData *gui_filter(Tab *tab);
void gui_filter_destructor(FilterViewerData *fvd);
FilterViewerDataLine* gui_filter_add_line(FilterViewerData *fvd);
void gui_filter_line_set_visible(FilterViewerDataLine *fvdl, gboolean v);
void gui_filter_line_reset(FilterViewerDataLine *fvdl);
gboolean filter_traceset_changed(void * hook_data, void * call_data);
gboolean filter_viewer_data(void * hook_data, void * call_data); 
GtkWidget* h_guifilter(Tab *tab);
void filter_destroy_walk(gpointer data, gpointer user_data);
  
/*
 * Callback functions
 */
void callback_process_button(GtkWidget *widget, gpointer data);
void callback_add_button(GtkWidget *widget, gpointer data);
void callback_logical_op_box(GtkWidget *widget, gpointer data);
void callback_expression_field(GtkWidget *widget, gpointer data);

struct _FilterViewerDataLine {
  int row;
  gboolean visible;
  GtkWidget *f_logical_op_box;
  GtkWidget *f_struct_box;
  GtkWidget *f_subfield_box;
  GtkWidget *f_math_op_box;
  GtkWidget *f_value_field;
};

/**
 * @struct _FilterViewerData
 * Main struct for the filter gui module
 */
struct _FilterViewerData {
  Tab *tab;

  GtkWidget *f_main_box;

  GtkWidget *f_hbox1;
  GtkWidget *f_hbox2;
  
  GtkWidget *f_expression_field;
  GtkWidget *f_process_button;

  GtkWidget *f_logical_op_junction_box;

  int rows;
  GPtrArray *f_lines;
 
  GtkWidget *f_add_button;
  
  GtkWidget *f_textwnd;
  GtkWidget *f_selectwnd;
  GtkWidget *f_treewnd;
  
};

/**
 *  This function returns the current main widget 
 *  used by this module
 *  @param fvd the module struct
 *  @return The main widget
 */
GtkWidget 
*guifilter_get_widget(FilterViewerData *fvd)
{
  return fvd->f_main_box;
}

/**
 * Constructor is used to create FilterViewerData data structure.
 * @param the tab structure used by the widget
 * @return The Filter viewer data created.
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
  fvd->f_main_box = gtk_table_new(3,6,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(fvd->f_main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(fvd->f_main_box),5);
  
  /*
   *  First half of the filter window
   *  - textual entry of filter expression
   *  - processing button
   */
  fvd->f_expression_field = gtk_entry_new(); //gtk_scrolled_window_new (NULL, NULL);
//  gtk_entry_set_text(GTK_ENTRY(fvd->f_expression_field),"state.cpu>0");
  gtk_widget_show (fvd->f_expression_field);

  g_signal_connect (G_OBJECT (fvd->f_expression_field), "changed",
      G_CALLBACK (callback_expression_field), (gpointer) fvd); 

  fvd->f_process_button = gtk_button_new_with_label("Process");
  gtk_widget_show (fvd->f_process_button);
  
  g_signal_connect (G_OBJECT (fvd->f_process_button), "clicked",
      G_CALLBACK (callback_process_button), (gpointer) fvd); 
  
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_expression_field,0,5,0,1,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_process_button,5,6,0,1,GTK_FILL,GTK_FILL,0,0);


  
  /*
   *  Second half of the filter window
   *  - combo boxes featuring filtering options added to the expression
   */
  fvd->f_add_button = gtk_button_new_with_label("Add Expression");
  gtk_widget_show (fvd->f_add_button);

  g_signal_connect (G_OBJECT (fvd->f_add_button), "clicked",
      G_CALLBACK (callback_add_button), (gpointer) fvd);
  
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_add_button,5,6,1,2,GTK_FILL,GTK_FILL,0,0);
  
  fvd->f_logical_op_junction_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_junction_box), "");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_junction_box), "&");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_junction_box), "|");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_junction_box), "^");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvd->f_logical_op_junction_box), "!");
  //gtk_widget_show(fvd->f_logical_op_box);
 
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvd->f_logical_op_junction_box,0,1,1,2,GTK_SHRINK,GTK_FILL,0,0);

  /* initialize a new line */
  fvd->f_lines = g_ptr_array_new();
  fvd->rows = 1;
  FilterViewerDataLine* fvdl = gui_filter_add_line(fvd);
  g_ptr_array_add(fvd->f_lines,(gpointer) fvdl);
  
  /* 
   * show main container 
   */
  gtk_widget_show(fvd->f_main_box);
  
  
  g_object_set_data_full(
      G_OBJECT(guifilter_get_widget(fvd)),
      "filter_viewer_data",
      fvd,
      (GDestroyNotify)gui_filter_destructor);

  
  return fvd;
}

/**
 *  Adds a filter option line on the module tab
 *  @param fvd The filter module structure 
 *  @return The line structure
 */
FilterViewerDataLine*
gui_filter_add_line(FilterViewerData* fvd) {

  FilterViewerDataLine* fvdl = g_new(FilterViewerDataLine,1);

  fvdl->row = fvd->rows;
  fvdl->visible = TRUE;
  
  fvdl->f_struct_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_struct_box), "");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_struct_box), "event");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_struct_box), "tracefile");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_struct_box), "trace");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_struct_box), "state");
  gtk_widget_show(fvdl->f_struct_box);
 
  fvdl->f_subfield_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "name");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "category");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "tsc");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "pid");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "ppid");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "creation time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "insertion time");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "process name");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "execution mode");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "execution submode");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "process status");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_subfield_box), "cpu");
  gtk_widget_show(fvdl->f_subfield_box);
 
  fvdl->f_math_op_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), "");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), "=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), "!=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), "<");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), "<=");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), ">");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_math_op_box), ">=");
  gtk_widget_show(fvdl->f_math_op_box);

  fvdl->f_value_field = gtk_entry_new();
  gtk_widget_show(fvdl->f_value_field);
 
  fvdl->f_logical_op_box = gtk_combo_box_new_text();
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_logical_op_box), "");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_logical_op_box), "&");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_logical_op_box), "|");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_logical_op_box), "^");
  gtk_combo_box_append_text (GTK_COMBO_BOX (fvdl->f_logical_op_box), "!");
  gtk_widget_show(fvdl->f_logical_op_box);

  gtk_widget_set_events(fvdl->f_logical_op_box,
      GDK_ENTER_NOTIFY_MASK |
      GDK_LEAVE_NOTIFY_MASK |
      GDK_FOCUS_CHANGE_MASK);

  g_signal_connect (G_OBJECT (fvdl->f_logical_op_box), "changed",
    G_CALLBACK (callback_logical_op_box), (gpointer) fvd); 
  
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvdl->f_struct_box,0,1,fvd->rows+1,fvd->rows+2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvdl->f_subfield_box,1,2,fvd->rows+1,fvd->rows+2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvdl->f_math_op_box,2,3,fvd->rows+1,fvd->rows+2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvdl->f_value_field,3,4,fvd->rows+1,fvd->rows+2,GTK_SHRINK,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(fvd->f_main_box),fvdl->f_logical_op_box,4,5,fvd->rows+1,fvd->rows+2,GTK_SHRINK,GTK_FILL,0,0);
 
  return fvdl;
}

void gui_filter_line_set_visible(FilterViewerDataLine *fvdl, gboolean v) {

  fvdl->visible = v;
  if(v) {
    gtk_widget_show(fvdl->f_struct_box);
    gtk_widget_show(fvdl->f_subfield_box);
    gtk_widget_show(fvdl->f_math_op_box);
    gtk_widget_show(fvdl->f_value_field);
    gtk_widget_show(fvdl->f_logical_op_box);
  } else {
    gtk_widget_hide(fvdl->f_struct_box);
    gtk_widget_hide(fvdl->f_subfield_box);
    gtk_widget_hide(fvdl->f_math_op_box);
    gtk_widget_hide(fvdl->f_value_field);
    gtk_widget_hide(fvdl->f_logical_op_box);
 } 
  
}

void gui_filter_line_reset(FilterViewerDataLine *fvdl) {

  gtk_combo_box_set_active(GTK_COMBO_BOX(fvdl->f_struct_box),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(fvdl->f_subfield_box),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(fvdl->f_math_op_box),0);
  gtk_entry_set_text(GTK_COMBO_BOX(fvdl->f_value_field),"");
  gtk_combo_box_set_active(GTK_COMBO_BOX(fvdl->f_logical_op_box),0);
}

/**
 *  Destructor for the filter gui module
 *  @param fvd The module structure
 */
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

/**
 *  Hook function
 *  @param hook_data The hook data
 *  @param call_data The call data
 *  @return Success/Failure of operation
 */
gboolean
filter_traceset_changed(void * hook_data, void * call_data) {

  return FALSE;
}

/**
 *  Hook function
 *  @param hook_data The hook data
 *  @param call_data The call data
 *  @return Success/Failure of operation
 */
gboolean
filter_viewer_data(void * hook_data, void * call_data) {

  return FALSE;
}

/**
 * Filter Module's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param tab A pointer to the parent window.
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
 * This function initializes the Filter Viewer functionnality through the
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

/**
 *  Initiate the destruction of the current gui module
 *  on the GTK Interface
 */
void filter_destroy_walk(gpointer data, gpointer user_data)
{
  FilterViewerData *fvd = (FilterViewerData*)data;

  g_debug("CFV.c : filter_destroy_walk, %p", fvd);
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

/**
 *  The Process Button callback function
 *  @param widget The Button widget passed to the callback function
 *  @param data Data sent along with the callback function
 */
void callback_process_button(GtkWidget *widget, gpointer data) {

  FilterViewerData *fvd = (FilterViewerData*)data;

  if(strlen(gtk_entry_get_text(GTK_ENTRY(fvd->f_expression_field))) !=0)
    lttv_filter_new(gtk_entry_get_text(GTK_ENTRY(fvd->f_expression_field)),NULL);
}

/**
 *  The Add Button callback function
 *  @param widget The Button widget passed to the callback function
 *  @param data Data sent along with the callback function
 */
void callback_expression_field(GtkWidget *widget, gpointer data) {

  FilterViewerData *fvd = (FilterViewerData*)data;

  if(strlen(gtk_entry_get_text(GTK_ENTRY(fvd->f_expression_field))) !=0) {
    gtk_widget_show(fvd->f_logical_op_junction_box);
  } else {
    gtk_widget_hide(fvd->f_logical_op_junction_box);
  }
}


/**
 *  The Add Button callback function
 *  @param widget The Button widget passed to the callback function
 *  @param data Data sent along with the callback function
 */
void callback_add_button(GtkWidget *widget, gpointer data) {

  g_print("filter::callback_add_button()\n");

  FilterViewerData *fvd = (FilterViewerData*)data;
  FilterViewerDataLine *fvdl = NULL;
  GString* a_filter_string = g_string_new("");
  
//  g_string_append(a_filter_string,gtk_combo_box_get_active_text(GTK_COMBO_BOX(fvd->f_logical_op_junction_box)));
  gtk_combo_box_set_active(fvd->f_logical_op_junction_box,0);

  g_string_append_c(a_filter_string,"(");
  
  int i;
  for(i=0;i<fvd->f_lines->len;i++) {
    fvdl = (FilterViewerDataLine*)g_ptr_array_index(fvd->f_lines,i);
//    g_string_append(a_filter_string,gtk_combo_box_get_active_text(GTK_COMBO_BOX(fvdl->f_struct_box)));
    g_string_append_c(a_filter_string,".");
//    g_string_append(a_filter_string,gtk_combo_box_get_active_text(GTK_COMBO_BOX(fvdl->f_subfield_box)));
//    g_string_append(a_filter_string,gtk_combo_box_get_active_text(GTK_COMBO_BOX(fvdl->f_math_op_box)));
//    g_string_append(a_filter_string,gtk_entry_get_text(GTK_ENTRY(fvdl->f_value_field)));
//    g_string_append(a_filter_string,gtk_combo_box_get_active_text(GTK_COMBO_BOX(fvdl->f_logical_op_box)));
    gui_filter_line_reset(fvdl);
    if(i) gui_filter_line_set_visible(fvdl,FALSE);
  }

  g_string_append(a_filter_string,")");

  gtk_entry_set_text(GTK_ENTRY(fvdl->f_value_field),a_filter_string->str);
  
}

/**
 *  The logical op box callback function 
 *  @param widget The Button widget passed to the callback function
 *  @param data Data sent along with the callback function
 */
void callback_logical_op_box(GtkWidget *widget, gpointer data) {
 
  g_print("filter::callback_logical_op_box()\n");

  FilterViewerData *fvd = (FilterViewerData*)data;
  FilterViewerDataLine *fvdl = NULL;
  
  int i;
  for(i=0;i<fvd->f_lines->len;i++) {
    fvdl = (FilterViewerDataLine*)g_ptr_array_index(fvd->f_lines,i);
    if(fvdl->f_logical_op_box == widget) {
      if(gtk_combo_box_get_active(fvdl->f_logical_op_box) == 0) return;
      if(i==fvd->f_lines->len-1) {  /* create a new line */
        fvd->rows++;
        FilterViewerDataLine* fvdl2 = gui_filter_add_line(fvd);
        g_ptr_array_add(fvd->f_lines,(gpointer) fvdl2);
      } else {
        FilterViewerDataLine *fvdl2 = (FilterViewerDataLine*)g_ptr_array_index(fvd->f_lines,i+1);
        if(!fvdl2->visible) gui_filter_line_set_visible(fvdl2,TRUE); 
      }
    }
  }
  
}

LTTV_MODULE("guifilter", "Filter window", \
    "Graphical module that let user specify their filtering options", \
    init, destroy, "lttvwindow")

