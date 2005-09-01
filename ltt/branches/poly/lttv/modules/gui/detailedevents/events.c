/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers and XangXiu Yang
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


//*! \defgroup GuiEvents libGuiEvents: The GUI Events display plugin */
/*\@{*/

/*! \file GuiEvents.c
 * \brief Graphical plugin for showing events.
 *
 * This plugin lists all the events contained in the current time interval
 * in a list.
 * 
 * This plugin adds a Events Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling viewer.h's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the lttvwindow.
 *
 * Authors : Mathieu Desnoyers and XangXiu Yang, June to December 2003
 *           Inspired from original LTT, made by Karim Yaghmour
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/filter.h>
#include <lttv/print.h>
#include <lttvwindow/lttvwindow.h>

#include "hGuiEventsInsert.xpm"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)

#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

#define abs(a) (((a)<0)?(-a):(a))


/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_event_viewer_data_list = NULL ;

typedef enum _ScrollDirection{
  SCROLL_STEP_UP,
  SCROLL_STEP_DOWN,
  SCROLL_PAGE_UP,
  SCROLL_PAGE_DOWN,
  SCROLL_JUMP,
  SCROLL_NONE
} ScrollDirection;

typedef struct _EventViewerData {

  Tab * tab;
  LttvHooks  * event_hooks;

  /* previous value is used to determine if it is a page up/down or
   * step up/down, in which case we move of a certain amount of events (one or
   * the number of events shown on the screen) instead of changing begin time.
   */
  double       previous_value;

  //scroll window containing Tree View
  GtkWidget * scroll_win;

  /* Model containing list data */
  GtkListStore *store_m;

  GPtrArray *pos; /* Array of LttvTracesetContextPosition * */
  
  GtkWidget *hbox_v;
  /* Widget to display the data in a columned list */
  GtkWidget *tree_v;
  GtkAdjustment *vtree_adjust_c ;
  GtkWidget *button; /* a button of the header, used to get the header_height */
  gint header_height;
  
  /* Vertical scrollbar and its adjustment */
  GtkWidget *vscroll_vc;
  GtkAdjustment *vadjust_c;
  
  /* Selection handler */
  GtkTreeSelection *select_c;
  
  gint num_visible_events;
  
  gint currently_selected_event;

} EventViewerData ;

/** hook functions for update time interval, current time ... */
gboolean update_current_time(void * hook_data, void * call_data);
//gboolean show_event_detail(void * hook_data, void * call_data);
gboolean traceset_changed(void * hook_data, void * call_data);

//! Event Viewer's constructor hook
GtkWidget *h_gui_events(Tab *tab);
//! Event Viewer's constructor
EventViewerData *gui_events(Tab *tab);
//! Event Viewer's destructor
void gui_events_destructor(EventViewerData *event_viewer_data);
void gui_events_free(EventViewerData *event_viewer_data);

static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data);

void tree_v_set_cursor(EventViewerData *event_viewer_data);
void tree_v_get_cursor(EventViewerData *event_viewer_data);

/* Prototype for selection handler callback */
static void tree_selection_changed_cb (GtkTreeSelection *selection,
    gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
static void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc,
    gpointer data);
static void tree_v_size_request_cb (GtkWidget *widget,
    GtkRequisition *requisition, gpointer data);
static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1,
    gint arg2, gpointer data);


static void get_events(double time, EventViewerData *event_viewer_data);

int event_hook(void *hook_data, void *call_data);

/* Enumeration of the columns */
enum
{
  TRACE_NAME_COLUMN,
  TRACEFILE_NAME_COLUMN,
  CPUID_COLUMN,
  EVENT_COLUMN,
  FACILITY_COLUMN,
  TIME_S_COLUMN,
  TIME_NS_COLUMN,
  PID_COLUMN,
  EVENT_DESCR_COLUMN,
  POSITION_COLUMN,
  N_COLUMNS
};

/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_gui_events(Tab * tab)
{
  EventViewerData* event_viewer_data = gui_events(tab) ;
  if(event_viewer_data)
    return event_viewer_data->hbox_v;
  else return NULL;
  
}

/**
 * Event Viewer's constructor
 *
 * This constructor is used to create EventViewerData data structure.
 * @return The Event viewer data created.
 */
EventViewerData *
gui_events(Tab *tab)
{
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  EventViewerData* event_viewer_data = g_new(EventViewerData,1) ;

  event_viewer_data->tab = tab;
  
  event_viewer_data->event_hooks = lttv_hooks_new();
  lttv_hooks_add(event_viewer_data->event_hooks,
                 event_hook,
                 event_viewer_data,
                 LTTV_PRIO_DEFAULT);

  lttvwindow_register_current_time_notify(tab, 
                update_current_time,event_viewer_data);
  lttvwindow_register_traceset_notify(tab, 
                traceset_changed,event_viewer_data);

  event_viewer_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (event_viewer_data->scroll_win);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(event_viewer_data->scroll_win), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

  event_viewer_data->currently_selected_event = -1;

  /* Create a model for storing the data list */
  event_viewer_data->store_m = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_STRING,  /* Trace name                  */
    G_TYPE_STRING,  /* Tracefile name              */
    G_TYPE_UINT,    /* CPUID                       */
    G_TYPE_STRING,  /* Event                       */
    G_TYPE_STRING,  /* Facility                    */
    G_TYPE_UINT,    /* Time s                      */
    G_TYPE_UINT,    /* Time ns                     */
    G_TYPE_INT,     /* PID                         */
    G_TYPE_STRING,  /* Event's description         */
    G_TYPE_POINTER);/* Position (not shown)        */
  
  event_viewer_data->pos = g_ptr_array_sized_new(10);
  
  /* Create the viewer widget for the columned list */
  event_viewer_data->tree_v =
    gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->store_m));
    
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "size-allocate",
        G_CALLBACK (tree_v_size_allocate_cb),
        event_viewer_data);
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "size-request",
        G_CALLBACK (tree_v_size_request_cb),
        event_viewer_data);
  
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "cursor-changed",
        G_CALLBACK (tree_v_cursor_changed_cb),
        event_viewer_data);
  
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "move-cursor",
        G_CALLBACK (tree_v_move_cursor_cb),
        event_viewer_data);

  // Use on each column!
  //gtk_tree_view_column_set_sizing(event_viewer_data->tree_v,
  //GTK_TREE_VIEW_COLUMN_FIXED);
  
  /* The view now holds a reference.  We can get rid of our own
   * reference */
  g_object_unref (G_OBJECT (event_viewer_data->store_m));
  

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Trace",
                 renderer,
                 "text", TRACE_NAME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);

  event_viewer_data->button = column->button;

  g_signal_connect (G_OBJECT(event_viewer_data->button),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)event_viewer_data);


  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tracefile",
                 renderer,
                 "text", TRACEFILE_NAME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPUID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event",
                 renderer,
                 "text", EVENT_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Facility",
                 renderer,
                 "text", FACILITY_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time (s)",
                 renderer,
                 "text", TIME_S_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time (ns)",
                 renderer,
                 "text", TIME_NS_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("PID",
                 renderer,
                 "text", PID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event Description",
                 renderer,
                 "text", EVENT_DESCR_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);


  /* Setup the selection handler */
  event_viewer_data->select_c =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->tree_v));
  gtk_tree_selection_set_mode (event_viewer_data->select_c,
      GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (event_viewer_data->select_c), "changed",
        G_CALLBACK (tree_selection_changed_cb),
        event_viewer_data);
  
  gtk_container_add (GTK_CONTAINER (event_viewer_data->scroll_win),
      event_viewer_data->tree_v);

  event_viewer_data->hbox_v = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v),
      event_viewer_data->scroll_win, TRUE, TRUE, 0);

  /* Create vertical scrollbar and pack it */
  event_viewer_data->vscroll_vc = gtk_vscrollbar_new(NULL);
  gtk_range_set_update_policy (GTK_RANGE(event_viewer_data->vscroll_vc),
             GTK_UPDATE_CONTINUOUS);
             // Changed by MD : more user friendly :)
             //GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v),
      event_viewer_data->vscroll_vc, FALSE, TRUE, 0);
  
  /* Get the vertical scrollbar's adjustment */
  event_viewer_data->vadjust_c =
    gtk_range_get_adjustment(GTK_RANGE(event_viewer_data->vscroll_vc));
  event_viewer_data->vtree_adjust_c = gtk_tree_view_get_vadjustment(
                    GTK_TREE_VIEW (event_viewer_data->tree_v));
  
  g_signal_connect (G_OBJECT (event_viewer_data->vadjust_c), "value-changed",
        G_CALLBACK (v_scroll_cb),
        event_viewer_data);
  /* Set the upper bound to the last event number */
  event_viewer_data->previous_value = 0;
  event_viewer_data->vadjust_c->lower = 0.0;
    //event_viewer_data->vadjust_c->upper = event_viewer_data->number_of_events;
  event_viewer_data->vadjust_c->value = 0.0;
  event_viewer_data->vadjust_c->step_increment = 1.0;
  event_viewer_data->vadjust_c->page_increment = 2.0;
    //  event_viewer_data->vtree_adjust_c->upper;
  event_viewer_data->vadjust_c->page_size = 2.0;
  //    event_viewer_data->vtree_adjust_c->upper;
  /*  Raw event trace */
  gtk_widget_show(event_viewer_data->hbox_v);
  gtk_widget_show(event_viewer_data->tree_v);
  gtk_widget_show(event_viewer_data->vscroll_vc);

  /* Add the object's information to the module's array */
  g_event_viewer_data_list = g_slist_append(g_event_viewer_data_list,
      event_viewer_data);

  event_viewer_data->num_visible_events = 1;

  //get the life span of the traceset and set the upper of the scroll bar
  
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  TimeInterval time_span = tsc->time_span;
  end = ltt_time_sub(time_span.end_time, time_span.start_time);

  event_viewer_data->vadjust_c->upper =
              ltt_time_to_double(end);

  /* Set the Selected Event */
  //  tree_v_set_cursor(event_viewer_data);

 // event_viewer_data->current_time_updated = FALSE;
 //
  g_object_set_data_full(
      G_OBJECT(event_viewer_data->hbox_v),
      "event_viewer_data",
      event_viewer_data,
      (GDestroyNotify)gui_events_free);
  

  return event_viewer_data;
}



static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)user_data;

  event_viewer_data->header_height = allocation->height;

  return 0;
}


void tree_v_set_cursor(EventViewerData *event_viewer_data)
{
  GtkTreePath *path;
  
  if(event_viewer_data->currently_selected_event != -1)
    {
      path = gtk_tree_path_new_from_indices(
              event_viewer_data->currently_selected_event,
              -1);
      
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
          path, NULL, FALSE);
      gtk_tree_path_free(path);
    }
}

void tree_v_get_cursor(EventViewerData *event_viewer_data)
{
  GtkTreePath *path;
  gint *indices;
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
      &path, NULL);
  indices = gtk_tree_path_get_indices(path);
  
  if(indices != NULL)
      event_viewer_data->currently_selected_event = indices[0];
    else
      event_viewer_data->currently_selected_event = -1;
  
  gtk_tree_path_free(path);

}



void tree_v_move_cursor_cb (GtkWidget *widget,
                            GtkMovementStep arg1,
                            gint arg2,
                            gpointer data)
{
  GtkTreePath *path; // = gtk_tree_path_new();
  gint *indices;
  gdouble value;
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
                            &path, NULL);
  if(path == NULL)
  {
    /* No prior cursor, put it at beginning of page
     * and let the execution do */
    path = gtk_tree_path_new_from_indices(0, -1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
                                path, NULL, FALSE);
  }

  indices = gtk_tree_path_get_indices(path);
  
  value = gtk_adjustment_get_value(event_viewer_data->vadjust_c);
  
  g_debug("tree view move cursor : arg1 is %u and arg2 is %d",
      (guint)arg1, arg2);
  
#if 0
  if(arg1 == GTK_MOVEMENT_DISPLAY_LINES)
  {
    /* Move one line */
    if(arg2 == 1)
    {
      /* move one line down */
      if(indices[0]) // Do we need an empty field here (before first)?
      {
        if(value + event_viewer_data->num_visible_events <= 
            event_viewer_data->number_of_events -1)
        {
          event_viewer_data->currently_selected_event += 1;
          //      gtk_adjustment_set_value(event_viewer_data->vadjust_c, value+1);
          //gtk_tree_path_free(path);
          //path = gtk_tree_path_new_from_indices(event_viewer_data->num_visible_events-1, -1);
          //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
          g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
        }
      }
    } else {
      /* Move one line up */
      if(indices[0] == 0)
      {
        if(value - 1 >= 0 )
        {
          event_viewer_data->currently_selected_event -= 1;
          //      gtk_adjustment_set_value(event_viewer_data->vadjust_c, value-1);
          //gtk_tree_path_free(path);
          //path = gtk_tree_path_new_from_indices(0, -1);
          //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
          g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
        }
      }
    }
  }
  
  if(arg1 == GTK_MOVEMENT_PAGES)
  {
    /* Move one page */
    if(arg2 == 1)
    {
      if(event_viewer_data->num_visible_events == 1)
        value += 1 ;
      /* move one page down */
      if(value + event_viewer_data->num_visible_events-1 <= 
                      event_viewer_data->number_of_events )
      {
        event_viewer_data->currently_selected_event += 
                                  event_viewer_data->num_visible_events-1;
        //        gtk_adjustment_set_value(event_viewer_data->vadjust_c,
        //               value+(event_viewer_data->num_visible_events-1));
        //gtk_tree_path_free(path);
        //path = gtk_tree_path_new_from_indices(0, -1);
        //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
        g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
      }
    } else {
      /* Move one page up */
      if(event_viewer_data->num_visible_events == 1)
        value -= 1 ;

      if(indices[0] < event_viewer_data->num_visible_events - 2 )
      {
        if(value - (event_viewer_data->num_visible_events-1) >= 0)
        {
          event_viewer_data->currently_selected_event -=
                          event_viewer_data->num_visible_events-1;
      
          //      gtk_adjustment_set_value(event_viewer_data->vadjust_c,
          //             value-(event_viewer_data->num_visible_events-1));
          //gtk_tree_path_free(path);
          //path = gtk_tree_path_new_from_indices(0, -1);
          //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
          g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
      
        } else {
          /* Go to first Event */
          event_viewer_data->currently_selected_event == 0 ;
          //      gtk_adjustment_set_value(event_viewer_data->vadjust_c,
          //             0);
          //gtk_tree_path_free(path);
          //path = gtk_tree_path_new_from_indices(0, -1);
          //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
          g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
      
        }
      }
    }
  }
  
  if(arg1 == GTK_MOVEMENT_BUFFER_ENDS)
  {
    /* Move to the ends of the buffer */
    if(arg2 == 1)
    {
      /* move end of buffer */
      event_viewer_data->currently_selected_event =
                            event_viewer_data->number_of_events-1 ;
      //    gtk_adjustment_set_value(event_viewer_data->vadjust_c, 
      //           event_viewer_data->number_of_events -
      //           event_viewer_data->num_visible_events);
      //gtk_tree_path_free(path);
      //path = gtk_tree_path_new_from_indices(event_viewer_data->num_visible_events-1, -1);
      //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
      g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
    } else {
      /* Move beginning of buffer */
      event_viewer_data->currently_selected_event = 0 ;
      //    gtk_adjustment_set_value(event_viewer_data->vadjust_c, 0);
        //gtk_tree_path_free(path);
        //path = gtk_tree_path_new_from_indices(0, -1);
        //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
      g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
    }
  }
#endif //0
  gtk_tree_path_free(path);
}

void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) data;
  Tab *tab = event_viewer_data->tab;
  GtkTreeIter iter;
  GtkTreeModel* model = GTK_TREE_MODEL(event_viewer_data->store_m);
  GtkTreePath *path;
  LttvTracesetContextPosition *pos;

  
  /* On cursor change, modify the currently selected event by calling
   * the right API function */
  tree_v_get_cursor(event_viewer_data);
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
      &path, NULL);
  if(gtk_tree_model_get_iter(model,&iter,path)){
    gtk_tree_model_get(model, &iter, POSITION_COLUMN, &pos, -1);
 
    lttvwindow_report_current_position(tab, pos);
  }else{
    g_warning("Can not get iter\n");
  }

}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  GtkTreePath *tree_path;

  get_events(adjustment->value, event_viewer_data);
  
  if(event_viewer_data->currently_selected_event != -1) {
      
      tree_path = gtk_tree_path_new_from_indices(
             event_viewer_data->currently_selected_event,
             -1);
      
      //      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), tree_path,
      //             NULL, FALSE);
      gtk_tree_path_free(tree_path);
  }
}

static __inline gint get_cell_height(GtkTreeView *TreeView)
{
  gint height;
  GtkTreeViewColumn *column = gtk_tree_view_get_column(TreeView, 0);
  
  gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height);
  
  return height;
}

void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
  gint last_num_visible_events = event_viewer_data->num_visible_events;
  gdouble exact_num_visible;
  
  exact_num_visible = ( alloc->height -
          event_viewer_data->header_height )
            / (double)cell_height ;
  
  event_viewer_data->num_visible_events = ceil(exact_num_visible) ;
  
/*
  event_viewer_data->vadjust_c->page_increment = 
    floor(exact_num_visible);
  event_viewer_data->vadjust_c->page_size =
    floor(exact_num_visible);
*/
 
  g_debug("size allocate : last_num_visible_events : %d,\
           num_visible_events : %d",
           last_num_visible_events,
           event_viewer_data->num_visible_events);
  if(event_viewer_data->num_visible_events != last_num_visible_events)
    {
      get_events(event_viewer_data->vadjust_c->value, event_viewer_data);
    }
  

}

void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data)
{
  gint h;
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
  
  h = cell_height + event_viewer_data->header_height;
  requisition->height = h;
  
}

#if 0
gboolean show_event_detail(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTracesetContext * tsc = lttvwindow_get_traceset_context(event_viewer_data->tab);

  if(event_viewer_data->event_fields_queue_tmp->length == 0 &&
     event_viewer_data->event_fields_queue->length == 0){
    event_viewer_data->shown = FALSE;
    return FALSE;
  }

  if(event_viewer_data->shown == FALSE){
    event_viewer_data->shown = TRUE;
    update_raw_data_array(event_viewer_data, 
        event_viewer_data->event_fields_queue_tmp->length);

    get_data(event_viewer_data->vadjust_c->value,
      event_viewer_data->num_visible_events, 
      event_viewer_data);

    remove_context_hooks(event_viewer_data,tsc);
  }

  return FALSE;
}
#endif //0







static void get_events(double new_value, EventViewerData *event_viewer_data)
{
  LttvTracesetStats *tss =
    lttvwindow_get_traceset_stats(event_viewer_data->tab);
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;
  guint i;
  
  double value = new_value - event_viewer_data->previous_value;
  event_viewer_data->previous_value = new_value;

  /* See where we have to scroll... */
  ScrollDirection direction;
  gint relative_position;
  
  if(value < 0.0) {
    if(value >= -1.0) direction = SCROLL_STEP_UP;
    else {
      if(value >= -2.0) direction = SCROLL_PAGE_UP;
      else direction = SCROLL_JUMP;
    }
  } else if(value > 0.0) {
    if(value <= 1.0) direction = SCROLL_STEP_DOWN;
    else {
      if(value <= 2.0) direction = SCROLL_PAGE_DOWN;
      else direction = SCROLL_JUMP;
    }
  } else direction = SCROLL_NONE; /* 0.0 */


  switch(direction) {
  case SCROLL_STEP_UP:
    g_debug("get_events : SCROLL_STEP_UP");
    relative_position = -1;
    break;
  case SCROLL_STEP_DOWN:
    g_debug("get_events : SCROLL_STEP_DOWN");
    relative_position = 1;
    break;
  case SCROLL_PAGE_UP:
    g_debug("get_events : SCROLL_PAGE_UP");
    relative_position = -(event_viewer_data->num_visible_events);
    break;
  case SCROLL_PAGE_DOWN:
    g_debug("get_events : SCROLL_PAGE_DOWN");
    relative_position = event_viewer_data->num_visible_events;
    break;
  case SCROLL_JUMP:
    g_debug("get_events : SCROLL_JUMP");
    relative_position = 0;
    break;
  case SCROLL_NONE:
    g_debug("get_events : SCROLL_NONE");
    goto end;
    break;
  }

  LttTime time = ltt_time_from_double(new_value);
  time = ltt_time_add(tsc->time_span.start_time, time);

  if(relative_position) {
  
    LttvTracesetContextPosition *pos =
        lttv_traceset_context_position_new(tsc);
    
  /* Remember the beginning position */
    if(event_viewer_data->pos->len > 0) {
      LttvTracesetContextPosition *first_pos = 
        (LttvTracesetContextPosition*)g_ptr_array_index(event_viewer_data->pos,
                                                        0);
      lttv_traceset_context_position_copy(pos, first_pos);
      g_assert(lttv_process_traceset_seek_position(tsc, pos) == 0); 
    } else {
      /* There is nothing in the list : simply seek to the time value. */
      lttv_state_traceset_seek_time_closest((LttvTracesetState*)tsc,
          time);
    }
    
  /* Clear the model (don't forget to free the TCS positions!) */
    gtk_list_store_clear(event_viewer_data->store_m);
    for(i=0;i<event_viewer_data->pos->len;i++) {
      LttvTracesetContextPosition *cur_pos = 
        (LttvTracesetContextPosition*)g_ptr_array_index(event_viewer_data->pos,
                                                        i);
      lttv_traceset_context_position_destroy(cur_pos);
    }
    g_ptr_array_set_size(event_viewer_data->pos, 0);

  /* Note that, as we mess with the tsc position, this function CANNOT be called
   * from a hook inside the lttv_process_traceset_middle. */
  /* As the lttvwindow API keeps a sync_position inside the tsc to go back at
   * the right spot after being interrupted, it's ok to change the tsc position,
   * as long as we do not touch the sync_position. */

  /* Get the beginning position of the read (with seek backward or seek forward)
   */
    if(relative_position > 0) {
      guint count;
      count = lttv_process_traceset_seek_n_forward(tsc, relative_position,
          lttvwindow_get_filter(event_viewer_data->tab));
    } else {  /* < 0 */
      guint count;
      count = lttv_process_traceset_seek_n_backward(tsc, abs(relative_position),
          seek_back_default_offset, lttv_process_traceset_seek_time,
          lttvwindow_get_filter(event_viewer_data->tab));
    }

    lttv_traceset_context_position_destroy(pos);
  } else {
    /* Seek by time */
    lttv_state_traceset_seek_time_closest((LttvTracesetState*)tsc,
          time);
  }
 
  /* Mathieu :
   * I make the choice not to use the mainwindow lttvwindow API here : it will
   * be faster to read the events ourself from lttv_process_traceset_middle, as
   * we are already at the right read position. It would be costier to use the
   * seek time closest for nothing, as we only have few events to read. 
   */

  lttv_process_traceset_begin(tsc,
      NULL, NULL, NULL, event_viewer_data->event_hooks, NULL);
  
  lttv_process_traceset_middle(tsc, ltt_time_infinite, G_MAXUINT, NULL);
  
  lttv_process_traceset_end(tsc,
      NULL, NULL, NULL, event_viewer_data->event_hooks, NULL);
  
end:
  return;
}



int event_hook(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext*)call_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = lttvwindow_get_filter(event_viewer_data->tab);
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc))
      return FALSE;

  LttFacility *facility = ltt_event_facility(e);
  LttEventType *event_type = ltt_event_eventtype(e);
  LttField *field = ltt_event_field(e);
  LttTime time = ltt_event_time(e);

  guint cpu = ltt_tracefile_num(tfc->tf);
  LttvTraceState *ts = (LttvTraceState*)tfc->t_context;
  LttvProcessState *process = ts->running_process[cpu];
  
  GtkTreeIter iter;

  GString *desc = g_string_new("");
  
  LttvTracesetContextPosition *pos =
    lttv_traceset_context_position_new(tfc->t_context->ts_context);

  if(field)
    lttv_print_field(e, field, desc, TRUE);

  g_info("field : %s", desc->str);
  
  gtk_list_store_append (event_viewer_data->store_m, &iter);
  gtk_list_store_set (event_viewer_data->store_m, &iter,
      TRACE_NAME_COLUMN, g_quark_to_string(ltt_trace_name(tfc->t_context->t)),
      TRACEFILE_NAME_COLUMN, g_quark_to_string(ltt_tracefile_name(tfc->tf)),
      CPUID_COLUMN, cpu,
      FACILITY_COLUMN, g_quark_to_string(ltt_facility_name(facility)),
      EVENT_COLUMN, g_quark_to_string(ltt_eventtype_name(event_type)),
      TIME_S_COLUMN, time.tv_sec,
      TIME_NS_COLUMN, time.tv_nsec,
      PID_COLUMN, process->pid,
      EVENT_DESCR_COLUMN, desc->str,
      POSITION_COLUMN, pos,
      -1);

  g_ptr_array_add(event_viewer_data->pos, pos);
  
  g_string_free(desc, TRUE);

  if(event_viewer_data->pos->len >= event_viewer_data->num_visible_events)
    return TRUE;
  else
    return FALSE;
}




gboolean update_current_time(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  const LttTime * current_time = (LttTime*)call_data;
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  
  LttTime time = ltt_time_sub(*current_time, tsc->time_span.start_time);
  double new_value = ltt_time_to_double(time);
  GtkTreePath *path;

  gtk_adjustment_set_value(event_viewer_data->vadjust_c, new_value);
 
  event_viewer_data->currently_selected_event = 0;

  path = gtk_tree_path_new_from_indices(
             event_viewer_data->currently_selected_event,
             -1);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
  //g_signal_stop_emission_by_name(G_OBJECT(event_viewer_data->tree_v), "cursor-changed");
  gtk_tree_path_free(path);  

  return FALSE;
}

gboolean traceset_changed(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  TimeInterval time_span = tsc->time_span;
 
  LttTime end;
  gtk_list_store_clear(event_viewer_data->store_m);
  g_ptr_array_set_size(event_viewer_data->pos, 0);

  end = ltt_time_sub(time_span.end_time, time_span.start_time);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(end);
  g_signal_emit_by_name(event_viewer_data->vadjust_c, "value-changed");
  //  event_viewer_data->vadjust_c->value = 0;

  return FALSE;
}





void gui_events_free(EventViewerData *event_viewer_data)
{
  Tab *tab = event_viewer_data->tab;
  guint i;
 
  if(event_viewer_data){
    lttv_hooks_remove(event_viewer_data->event_hooks,event_hook);
    lttv_hooks_destroy(event_viewer_data->event_hooks);
    
    for(i=0;i<event_viewer_data->pos->len;i++) {
      LttvTracesetContextPosition *cur_pos = 
        (LttvTracesetContextPosition*)g_ptr_array_index(event_viewer_data->pos,
                                                        i);
      lttv_traceset_context_position_destroy(cur_pos);
    }
    g_ptr_array_free(event_viewer_data->pos, TRUE);
    
    lttvwindow_unregister_current_time_notify(tab,
                        update_current_time, event_viewer_data);
    //lttvwindow_unregister_show_notify(tab,
    //                    show_event_detail, event_viewer_data);
    lttvwindow_unregister_traceset_notify(tab,
                        traceset_changed, event_viewer_data);

    g_event_viewer_data_list = g_slist_remove(g_event_viewer_data_list,
        event_viewer_data);
    g_free(event_viewer_data);
  }
}



void gui_events_destructor(EventViewerData *event_viewer_data)
{
  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(event_viewer_data->hbox_v)){
    gtk_widget_destroy(event_viewer_data->hbox_v);
  }
}



/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guievents",
                                  "/",
                                  "Insert Event Viewer",
                                  hGuiEventsInsert_xpm,
                                  "Insert Event Viewer",
                                  h_gui_events);
}

void event_destroy_walk(gpointer data, gpointer user_data)
{
  gui_events_destructor((EventViewerData*)data);
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  
  g_slist_foreach(g_event_viewer_data_list, event_destroy_walk, NULL );
  g_slist_free(g_event_viewer_data_list);

  lttvwindow_unregister_constructor(h_gui_events);
  
}




LTTV_MODULE("guievents", "Detailed events view", \
    "Graphical module to display a detailed event list", \
	    init, destroy, "lttvwindow", "print")
