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
#include <lttvwindow/lttvwindow.h>

#include "hGuiEventsInsert.xpm"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)


/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_event_viewer_data_list = NULL ;

typedef struct _EventFields{
  unsigned  cpu_id;
  char * event_name;
  LttTime time;
  int pid;
  unsigned entry_length;
  char * event_description;
  LttEventPosition *ep;
} EventFields;

#define RESERVE_BIG_SIZE             1000
#define RESERVE_SMALL_SIZE           100
#define RESERVE_SMALL_SIZE_SQUARE    RESERVE_SMALL_SIZE*RESERVE_SMALL_SIZE
#define RESERVE_SMALL_SIZE_CUBE      RESERVE_SMALL_SIZE*RESERVE_SMALL_SIZE_SQUARE

static const LttTime ltt_time_backward = { 1 , 0 };

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

  gboolean     append;                  // prepend or append item 
  GQueue     * event_fields_queue;      // buf to contain event fields
  GQueue     * event_fields_queue_tmp;  // tmp buf to contain event fields
  unsigned     current_event_index;
  
  /* previous value is used to determine if it is a page up/down or
   * step up/down, in which case we move of a certain amount of events (one or
   * the number of events shown on the screen) instead of changing begin time.
   */
  double       previous_value;
  
  gint     start_event_index;       // the first event shown in the window
  gint     end_event_index;         // the last event shown in the window
  gint     size;                    // maxi number of events loaded

  //scroll window containing Tree View
  GtkWidget * scroll_win;

  /* Model containing list data */
  GtkListStore *store_m;
  
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
  gint first_event, last_event;
  
  gint number_of_events ;
  gint currently_selected_event;
  gboolean selected_event ;

} EventViewerData ;

/** hook functions for update time interval, current time ... */
gboolean update_current_time(void * hook_data, void * call_data);
//gboolean show_event_detail(void * hook_data, void * call_data);
gboolean traceset_changed(void * hook_data, void * call_data);
void remove_item_from_queue(GQueue * q, gboolean fromHead);
void remove_all_items_from_queue(GQueue * q);
void add_context_hooks(EventViewerData * event_viewer_data, 
           LttvTracesetContext * tsc);
void remove_context_hooks(EventViewerData * event_viewer_data, 
        LttvTracesetContext * tsc);

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
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
static void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
static void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);


static void get_data(double time, guint list_height, 
        EventViewerData *event_viewer_data);

static void update_raw_data_array(EventViewerData* event_viewer_data, unsigned size);

static void get_events(EventViewerData* event_viewer_data, LttTime start, 
           LttTime end, unsigned max_num_events, unsigned * real_num_events);

static gboolean parse_event(void *hook_data, void *call_data);

/* Enumeration of the columns */
enum
{
  CPUID_COLUMN,
  EVENT_COLUMN,
  TIME_COLUMN,
  PID_COLUMN,
  ENTRY_LEN_COLUMN,
  EVENT_DESCR_COLUMN,
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
  lttv_hooks_add(event_viewer_data->event_hooks, parse_event, event_viewer_data, LTTV_PRIO_DEFAULT);

  event_viewer_data->event_fields_queue     = g_queue_new();
  event_viewer_data->event_fields_queue_tmp = g_queue_new();  

  lttvwindow_register_current_time_notify(tab, 
                update_current_time,event_viewer_data);
  //lttvwindow_register_show_notify(tab, 
  //              show_event_detail,event_viewer_data);
  lttvwindow_register_traceset_notify(tab, 
                traceset_changed,event_viewer_data);

  event_viewer_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (event_viewer_data->scroll_win);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(event_viewer_data->scroll_win), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

  /* TEST DATA, TO BE READ FROM THE TRACE */
  event_viewer_data->currently_selected_event = 0;
  event_viewer_data->selected_event = FALSE;

  /* Create a model for storing the data list */
  event_viewer_data->store_m = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_INT,     /* CPUID                       */
    G_TYPE_STRING,  /* Event                       */
    G_TYPE_UINT64,  /* Time                        */
    G_TYPE_INT,     /* PID                         */
    G_TYPE_INT,     /* Entry length                */
    G_TYPE_STRING); /* Event's description         */
  
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
  //gtk_tree_view_column_set_sizing(event_viewer_data->tree_v, GTK_TREE_VIEW_COLUMN_FIXED);
  
  /* The view now holds a reference.  We can get rid of our own
   * reference */
  g_object_unref (G_OBJECT (event_viewer_data->store_m));
  

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPUID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);

  event_viewer_data->button = column->button;

  g_signal_connect (G_OBJECT(event_viewer_data->button),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)event_viewer_data);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event",
                 renderer,
                 "text", EVENT_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time",
                 renderer,
                 "text", TIME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("PID",
                 renderer,
                 "text", PID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Entry Length",
                 renderer,
                 "text", ENTRY_LEN_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 60);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event Description",
                 renderer,
                 "text", EVENT_DESCR_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v), column);


  /* Setup the selection handler */
  event_viewer_data->select_c = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->tree_v));
  gtk_tree_selection_set_mode (event_viewer_data->select_c, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (event_viewer_data->select_c), "changed",
        G_CALLBACK (tree_selection_changed_cb),
        event_viewer_data);
  
  gtk_container_add (GTK_CONTAINER (event_viewer_data->scroll_win), event_viewer_data->tree_v);

  event_viewer_data->hbox_v = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v), event_viewer_data->scroll_win, TRUE, TRUE, 0);

  /* Create vertical scrollbar and pack it */
  event_viewer_data->vscroll_vc = gtk_vscrollbar_new(NULL);
  gtk_range_set_update_policy (GTK_RANGE(event_viewer_data->vscroll_vc),
             GTK_UPDATE_CONTINUOUS);
             // Changed by MD : more user friendly :)
             //GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v), event_viewer_data->vscroll_vc, FALSE, TRUE, 0);
  
  /* Get the vertical scrollbar's adjustment */
  event_viewer_data->vadjust_c = gtk_range_get_adjustment(GTK_RANGE(event_viewer_data->vscroll_vc));
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
  g_event_viewer_data_list = g_slist_append(g_event_viewer_data_list, event_viewer_data);

  event_viewer_data->first_event = -1 ;
  event_viewer_data->last_event = 0 ;
  
  event_viewer_data->num_visible_events = 1;

  //get the life span of the traceset and set the upper of the scroll bar
  
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  TimeInterval time_span = tsc->time_span;
  end = ltt_time_sub(time_span.end_time, time_span.start_time);

  event_viewer_data->vadjust_c->upper =
              ltt_time_to_double(end);

  event_viewer_data->append = TRUE;

  event_viewer_data->start_event_index = 0;
  event_viewer_data->end_event_index   = event_viewer_data->num_visible_events - 1;  

  /* Set the Selected Event */
  //  tree_v_set_cursor(event_viewer_data);

 // event_viewer_data->current_time_updated = FALSE;
  event_viewer_data->size  = RESERVE_SMALL_SIZE;

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
  
  if(event_viewer_data->selected_event && event_viewer_data->first_event != -1)
    {
      path = gtk_tree_path_new_from_indices(
              event_viewer_data->currently_selected_event-
              event_viewer_data->first_event,
              -1);
      
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
      gtk_tree_path_free(path);
    }
}

void tree_v_get_cursor(EventViewerData *event_viewer_data)
{
  GtkTreePath *path;
  gint *indices;
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), &path, NULL);
  indices = gtk_tree_path_get_indices(path);
  
  if(indices != NULL)
    {
      event_viewer_data->selected_event = TRUE;
      event_viewer_data->currently_selected_event =
              event_viewer_data->first_event + indices[0];
      
    } else {
      event_viewer_data->selected_event = FALSE;
      event_viewer_data->currently_selected_event = 0;
    }
  
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
  
  if(arg1 == GTK_MOVEMENT_DISPLAY_LINES)
  {
    /* Move one line */
    if(arg2 == 1)
    {
      /* move one line down */
      if(indices[0] == event_viewer_data->num_visible_events - 1)
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
  
  gtk_tree_path_free(path);
}

void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) data;
  Tab *tab = event_viewer_data->tab;
  LttTime current_time = lttvwindow_get_current_time(tab);
  LttTime ltt_time;
  guint64 time;
  GtkTreeIter iter;
  GtkTreeModel* model = GTK_TREE_MODEL(event_viewer_data->store_m);
  GtkTreePath *path;
  
  /* On cursor change, modify the currently selected event by calling
   * the right API function */
  tree_v_get_cursor(event_viewer_data);
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), &path, NULL);
  if(gtk_tree_model_get_iter(model,&iter,path)){
    gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
    ltt_time.tv_sec = time / NANOSECONDS_PER_SECOND;
    ltt_time.tv_nsec = time % NANOSECONDS_PER_SECOND;
 
    if(ltt_time.tv_sec != current_time.tv_sec ||
       ltt_time.tv_nsec != current_time.tv_nsec){
   //   event_viewer_data->current_time_updated = TRUE;
      lttvwindow_report_current_time(tab,ltt_time);
    }
  }else{
    g_warning("Can not get iter\n");
  }

}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  GtkTreePath *tree_path;

  get_data(adjustment->value, event_viewer_data->num_visible_events, 
    event_viewer_data);
  
  
  if(event_viewer_data->currently_selected_event
     >= event_viewer_data->first_event
     &&
     event_viewer_data->currently_selected_event
     <= event_viewer_data->last_event
     &&
     event_viewer_data->selected_event)
    {
      
      tree_path = gtk_tree_path_new_from_indices(
             event_viewer_data->currently_selected_event-
             event_viewer_data->first_event,
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
      get_data(event_viewer_data->vadjust_c->value,
        event_viewer_data->num_visible_events, 
        event_viewer_data);
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


void insert_data_into_model(EventViewerData *event_viewer_data, int start, int end)
{
  int i;
  guint64 real_data;
  EventFields * raw_data;
  GList * first;
  GtkTreeIter iter;

  first = event_viewer_data->event_fields_queue->head;
  for(i=start; i<end; i++){
    if(i>=event_viewer_data->number_of_events) break;    
    raw_data = (EventFields*)g_list_nth_data(first, i);
    
    // Add a new row to the model 
    real_data = raw_data->time.tv_sec;
    real_data *= NANOSECONDS_PER_SECOND;
    real_data += raw_data->time.tv_nsec;
    gtk_list_store_append (event_viewer_data->store_m, &iter);
    gtk_list_store_set (event_viewer_data->store_m, &iter,
      CPUID_COLUMN, raw_data->cpu_id,
      EVENT_COLUMN, raw_data->event_name,
      TIME_COLUMN, real_data,
      PID_COLUMN, raw_data->pid,
      ENTRY_LEN_COLUMN, raw_data->entry_length,
      EVENT_DESCR_COLUMN, raw_data->event_description,
      -1);
  }
}

static void get_data_wrapped(double time_value, gint list_height, 
       EventViewerData *event_viewer_data, LttEvent *ev);

static void get_data(double time_value, guint list_height,
     EventViewerData *event_viewer_data)
{
  LttEvent * ev = ltt_event_new();

  get_data_wrapped(time_value, list_height,
      event_viewer_data, ev);

  ltt_event_destroy(ev);
}

static void get_data_wrapped(double time_value, gint list_height, 
       EventViewerData *event_viewer_data, LttEvent *ev)
{
  int i;
  GtkTreeModel *model = GTK_TREE_MODEL(event_viewer_data->store_m);
  EventFields * event_fields;
  ScrollDirection  direction = SCROLL_NONE;
  GList * first;
  gint event_number = event_viewer_data->start_event_index;
  double value = event_viewer_data->previous_value - time_value;
  LttTime start, end, time;
  gint backward_num, minNum, maxNum;
  LttTracefile * tf;
  unsigned  block_num;
  gint size = 1, count = 0;
  gboolean need_backward_again, backward;
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  TimeInterval time_span = tsc->time_span;

  //  if(event_number > event_viewer_data->last_event ||
  //     event_number + list_height-1 < event_viewer_data->first_event ||
  //     event_viewer_data->first_event == -1)
  {
    /* no event can be reused, clear and start from nothing */
    if(value < 0.0) {
      if(value >= -1.0)      direction = SCROLL_STEP_DOWN;
      else if(value >= -2.0) direction = SCROLL_PAGE_DOWN;
      else direction = SCROLL_JUMP;
    } else if(value > 0.0) {
      if(value <= 1.0) direction = SCROLL_STEP_UP;
      else if(value <= 2.0) direction = SCROLL_PAGE_UP;
      else direction = SCROLL_JUMP;
    } 
    else direction = SCROLL_NONE; /* 0.0 */

    if(g_queue_get_length(event_viewer_data->event_fields_queue)==0)
      direction = SCROLL_JUMP;

    switch(direction){
      case SCROLL_STEP_UP:
        if(direction == SCROLL_STEP_UP) g_debug("direction STEP_UP");
      case SCROLL_PAGE_UP:
        if(direction == SCROLL_PAGE_UP) g_debug("direction PAGE_UP");
  if(direction == SCROLL_PAGE_UP){
    backward = list_height>event_viewer_data->start_event_index ? TRUE : FALSE;
  }else{
    backward = event_viewer_data->start_event_index == 0 ? TRUE : FALSE;
  }
  if(backward){
    first = event_viewer_data->event_fields_queue->head;
    if(!first)break;
    event_fields = (EventFields*)first->data;
    LttTime backward_start = event_fields->time;

    maxNum = RESERVE_SMALL_SIZE_CUBE;
    event_viewer_data->append = FALSE;
    do{
      if(direction == SCROLL_PAGE_UP){
        minNum = list_height - event_viewer_data->start_event_index ;
      }else{
        minNum = 1;
      }
      first = event_viewer_data->event_fields_queue->head;
      if(!first)break;
      event_fields = (EventFields*)first->data;
      end = event_fields->time;

      backward_start = LTT_TIME_MAX(ltt_time_sub(backward_start,
                                                   ltt_time_backward),
                              tsc->time_span.start_time);

      event_viewer_data->current_event_index = event_viewer_data->start_event_index;
      get_events(event_viewer_data, backward_start, end, maxNum, &size);
      event_viewer_data->start_event_index = event_viewer_data->current_event_index;

      if(size < minNum 
          && (ltt_time_compare(backward_start, tsc->time_span.start_time)>0))
        need_backward_again = TRUE;
      else need_backward_again = FALSE;
      if(size == 0){
        count++;
      }else{
        count = 0;
      }
    }while(need_backward_again);
  }
  if(direction == SCROLL_STEP_UP)
    event_number = event_viewer_data->start_event_index - 1;       
  else
    event_number = event_viewer_data->start_event_index - list_height;          
  break;
      case SCROLL_STEP_DOWN:
        g_debug("direction STEP_DOWN");
  if(event_viewer_data->end_event_index == event_viewer_data->number_of_events - 1){
    event_viewer_data->append = TRUE;
    first = event_viewer_data->event_fields_queue->head;
    if(!first)break;
    event_fields = (EventFields*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
    start = event_fields->time;
    start.tv_nsec++;
    end = ltt_time_infinite;
    get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE, &size);
    if(size == 0){
      get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_SQUARE,&size);
      if(size == 0)
        get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_CUBE,&size);
    }
    if(size==0) event_number = event_viewer_data->start_event_index;  
    else event_number = event_viewer_data->number_of_events - size - list_height + 1;
  }else event_number = event_viewer_data->start_event_index + 1;
  break;
      case SCROLL_PAGE_DOWN:
        g_debug("direction PAGE_DOWN");
  i = event_viewer_data->number_of_events - 1 - list_height;
  if((gint)(event_viewer_data->end_event_index) >= i){
    int remain_events = event_viewer_data->number_of_events - 1 
                        -  event_viewer_data->end_event_index;
    event_viewer_data->append = TRUE;
    first = event_viewer_data->event_fields_queue->head;
    if(!first)break;
    event_fields = (EventFields*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
    start = event_fields->time;
    start.tv_nsec++;
    end = ltt_time_infinite;
    get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE,&size);
    if(size == 0){
      get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_SQUARE,&size);
      if(size == 0)
        get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_CUBE,&size);
    }
    remain_events += size;
    if(list_height <= remain_events)
      event_number = event_viewer_data->number_of_events - remain_events - 1; 
    else
      event_number = event_viewer_data->number_of_events - 1 - list_height;     
  }else event_number = event_viewer_data->start_event_index + list_height - 1;
  break;
      case SCROLL_JUMP:
        g_debug("direction SCROLL_JUMP");
  event_viewer_data->append = TRUE;
  remove_all_items_from_queue(event_viewer_data->event_fields_queue);
  end = ltt_time_infinite;
  time = ltt_time_from_double(time_value);
  start = ltt_time_add(time_span.start_time, time);
  event_viewer_data->previous_value = time_value;
  get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE,&size);
  if(size < list_height && size > 0){
    event_viewer_data->append = FALSE;
    first = event_viewer_data->event_fields_queue->head;
    if(!first)break;
    event_fields = (EventFields*)first->data;
    end = event_fields->time;
    if(end.tv_nsec != 0)
      end.tv_nsec--;
    else {
      g_assert(end.tv_sec != 0);
      end.tv_sec--;
      end.tv_nsec = NANOSECONDS_PER_SECOND-1;
    }
      
    gint event_num;
    ltt_event_position_get(event_fields->ep, &block_num, &event_num, &tf);
    
    if(event_num > list_height - size){
      backward_num = event_num > RESERVE_SMALL_SIZE 
        ? event_num - RESERVE_SMALL_SIZE : 1;
      ltt_event_position_set(event_fields->ep, block_num, backward_num);
      ltt_tracefile_seek_position(tf, event_fields->ep);
      g_assert(ltt_tracefile_read(tf,ev) != NULL);
      start = ltt_event_time(ev);
      maxNum = RESERVE_SMALL_SIZE_CUBE;
      event_viewer_data->current_event_index = 0;
      get_events(event_viewer_data, start, end, maxNum, &size);
      event_viewer_data->start_event_index = event_viewer_data->current_event_index;
    }
    event_number = event_viewer_data->event_fields_queue->length - list_height;
  }else if(size == 0){
    get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_SQUARE,&size);
    if(size == 0)
      get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE_CUBE,&size);
    event_number = 0;
  }else{
    event_number = 0;
  }
  break;
      case SCROLL_NONE:
        g_debug("direction SCROLL_NONE");
  event_number = event_viewer_data->current_event_index;
  break;
      default:
    break;
    }

    if(event_number < 0) event_number = 0;

    //update the value of the scroll bar
    if(direction != SCROLL_NONE && direction != SCROLL_JUMP){
        g_debug("updating scrollbar value because near.");
      first = event_viewer_data->event_fields_queue->head;
      if(first){
  event_fields = (EventFields*)g_list_nth_data(first,event_number);
  if(!event_fields) event_fields = (EventFields*)g_list_nth_data(first,0);       
  time = ltt_time_sub(event_fields->time, time_span.start_time);
  event_viewer_data->vadjust_c->value = ltt_time_to_double(time);
  //gtk_adjustment_set_value(event_viewer_data->vadjust_c, 
  //                         ltt_time_to_double(time));
  //g_signal_stop_emission_by_name(G_OBJECT(event_viewer_data->vadjust_c), "value-changed");
  event_viewer_data->previous_value = event_viewer_data->vadjust_c->value;
  //gtk_adjustment_value_changed(event_viewer_data->vadjust_c);
      }
    }
    

    event_viewer_data->start_event_index = event_number;
    event_viewer_data->end_event_index = event_number + list_height - 1;    
    if(event_viewer_data->end_event_index > event_viewer_data->number_of_events - 1){
      event_viewer_data->end_event_index = event_viewer_data->number_of_events - 1;
    }

    first = event_viewer_data->event_fields_queue->head;
    gtk_list_store_clear(event_viewer_data->store_m);
    if(!first){
      //      event_viewer_data->previous_value = 0;
      //      event_viewer_data->vadjust_c->value = 0.0;
      //      gtk_widget_hide(event_viewer_data->vscroll_vc);
      goto LAST;
    }else gtk_widget_show(event_viewer_data->vscroll_vc);

    insert_data_into_model(event_viewer_data,event_number, event_number+list_height);
  }

  event_viewer_data->first_event = event_viewer_data->start_event_index ;
  event_viewer_data->last_event = event_viewer_data->end_event_index ;

 LAST:
  return;

}

void add_test_data(EventViewerData *event_viewer_data)
{
  GtkTreeIter iter;
  int i;
  
  for(i=0; i<10; i++)
    {
      /* Add a new row to the model */
      gtk_list_store_append (event_viewer_data->store_m, &iter);
      gtk_list_store_set (event_viewer_data->store_m, &iter,
        CPUID_COLUMN, 0,
        EVENT_COLUMN, "event irq",
        TIME_COLUMN, i,
        PID_COLUMN, 100,
        ENTRY_LEN_COLUMN, 17,
        EVENT_DESCR_COLUMN, "Detailed information",
        -1);
    }
  
}

void
gui_events_free(EventViewerData *event_viewer_data)
{
  Tab *tab = event_viewer_data->tab;

  if(event_viewer_data){
    lttv_hooks_remove(event_viewer_data->event_hooks,parse_event);
    lttv_hooks_destroy(event_viewer_data->event_hooks);
    
    remove_all_items_from_queue (event_viewer_data->event_fields_queue);
    g_queue_free(event_viewer_data->event_fields_queue);
    g_queue_free(event_viewer_data->event_fields_queue_tmp);

    lttvwindow_unregister_current_time_notify(tab,
                        update_current_time, event_viewer_data);
    //lttvwindow_unregister_show_notify(tab,
    //                    show_event_detail, event_viewer_data);
    lttvwindow_unregister_traceset_notify(tab,
                        traceset_changed, event_viewer_data);

    g_event_viewer_data_list = g_slist_remove(g_event_viewer_data_list, event_viewer_data);
    g_free(event_viewer_data);
  }
}

void
gui_events_destructor(EventViewerData *event_viewer_data)
{

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(event_viewer_data->hbox_v)){
    gtk_widget_destroy(event_viewer_data->hbox_v);
  }
}


static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(event_viewer_data->store_m);
  gchar *event;
  
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, EVENT_COLUMN, &event, -1);
      
      g_free (event);
    }
}


#if 0
/* If every module uses the filter, maybe these two 
 * (add/remove_context_hooks functions) should be put in common place
 */
void add_context_hooks(EventViewerData * event_viewer_data, 
           LttvTracesetContext * tsc)
{
  gint i, j, k, m,n, nbi, id;
  gint nb_tracefile, nb_facility, nb_event;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  gboolean selected;
  LttFacility           * fac;
  LttEventType          * et;

  //if there are hooks for traceset, add them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_trace_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, add them here

    nb_tracefile = ltt_trace_control_tracefile_number(trace) +
        ltt_trace_per_cpu_tracefile_number(trace);
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      tfc = tc->tracefiles[j];
      
      //if there are hooks for tracefile, add them here
      //      lttv_tracefile_context_add_hooks(tfc, NULL,NULL,NULL,NULL,
      //               event_viewer_data->event_hooks,NULL);

      nb_facility = ltt_trace_facility_number(trace);
      n = 0;
      for(k=0;k<nb_facility;k++){
  fac = ltt_trace_facility_get(trace,k);
  nb_event = (int) ltt_facility_eventtype_number(fac);
  for(m=0;m<nb_event;m++){
    et = ltt_facility_eventtype_get(fac,m);
    eventtype = lttv_tracefile_selector_eventtype_get(tf_s, n);
    selected = lttv_eventtype_selector_get_selected(eventtype);
    if(selected){
      id = (gint) ltt_eventtype_id(et);
      lttv_tracefile_context_add_hooks_by_id(tfc,id, 
               event_viewer_data->event_hooks);
    }
    n++;
  }
      }

    }
  }
  
  //add hooks for process_traceset
  //    lttv_traceset_context_add_hooks(tsc, NULL, NULL, NULL, NULL, NULL, NULL,
  //        NULL, NULL, NULL,event_viewer_data->event_hooks,NULL);  
}


void remove_context_hooks(EventViewerData * event_viewer_data, 
        LttvTracesetContext * tsc)
{
  gint i, j, k, m, nbi, n, id;
  gint nb_tracefile, nb_facility, nb_event;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  LttvTracesetSelector  * ts_s;
  LttvTraceSelector     * t_s;
  LttvTracefileSelector * tf_s;
  gboolean selected;
  LttFacility           * fac;
  LttEventType          * et;
  LttvEventtypeSelector * eventtype;

  ts_s = (LttvTracesetSelector*)g_object_get_data(G_OBJECT(event_viewer_data->hbox_v), 
              event_viewer_data->filter_key);

  //if there are hooks for traceset, remove them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_trace_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, remove them here

    nb_tracefile = ltt_trace_control_tracefile_number(trace) +
        ltt_trace_per_cpu_tracefile_number(trace);
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      tfc = tc->tracefiles[j];
      
      //if there are hooks for tracefile, remove them here
      //      lttv_tracefile_context_remove_hooks(tfc, NULL,NULL,NULL,NULL,
      //            event_viewer_data->event_hooks,NULL);

      nb_facility = ltt_trace_facility_number(trace);
      n = 0;
      for(k=0;k<nb_facility;k++){
  fac = ltt_trace_facility_get(trace,k);
  nb_event = (int) ltt_facility_eventtype_number(fac);
  for(m=0;m<nb_event;m++){
    et = ltt_facility_eventtype_get(fac,m);
    eventtype = lttv_tracefile_selector_eventtype_get(tf_s, n);
    selected = lttv_eventtype_selector_get_selected(eventtype);
    if(selected){
      id = (gint) ltt_eventtype_id(et);
      lttv_tracefile_context_remove_hooks_by_id(tfc,id); 
    }
    n++;
  }
      }
    }
  }
  //remove hooks from context
  //    lttv_traceset_context_remove_hooks(tsc, NULL, NULL, NULL, NULL, NULL, NULL,
  //           NULL, NULL, NULL,event_viewer_data->event_hooks,NULL);
}
#endif //0

gboolean update_current_time(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  const LttTime * current_time = (LttTime*)call_data;
  guint64 nsec = (guint64)current_time->tv_sec * NANOSECONDS_PER_SECOND 
                  + (guint64)current_time->tv_nsec;
  GtkTreeIter iter;
  guint64 time;
  GtkTreeModel* model = (GtkTreeModel*)event_viewer_data->store_m;
  GList * list;
  EventFields * data, *data1;
  GtkTreePath* path;
  char str_path[64];
  guint i;
  gint  j;
  LttTime t;
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(event_viewer_data->tab);
  TimeInterval time_span = tsc->time_span;

  if(!event_viewer_data->event_fields_queue->head) return FALSE;
#if 0
  if(event_viewer_data->current_time_updated ){
    event_viewer_data->current_time_updated = FALSE;
    return FALSE;
  }
#endif //0
  //check if the event is shown in the current viewer
  gint count = 0;
  gboolean event_shown = FALSE;
  if(gtk_tree_model_get_iter_first(model, &iter)){
    gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
    if(time > nsec){
      /* Event is before the list */
    } else {
      /* Event can be in the list */
      if(time >= nsec){
        /* found */
        event_shown = TRUE;
      } else {
        while(gtk_tree_model_iter_next(model, &iter)) {
          gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
          count++;
          if(time >= nsec){
            /* found */
            event_shown = TRUE;
            break;
          }
        }
      }
    }
  }

  if(!event_shown)
  {
    //the event is not shown in the current viewer
    count = 0;
    //check if the event is in the buffer
    list = event_viewer_data->event_fields_queue->head;
    data = (EventFields*)g_list_nth_data(list,0);
    data1 = (EventFields*)g_list_nth_data(list,event_viewer_data->event_fields_queue->length-1);
#if 0
    //the event is in the buffer
    if(ltt_time_compare(data->time, *current_time)<=0 &&
       ltt_time_compare(data1->time, *current_time)>=0){
      for(i=0;i<event_viewer_data->event_fields_queue->length;i++){
  data = (EventFields*)g_list_nth_data(list,i);
  if(ltt_time_compare(data->time, *current_time) < 0){
    count++;
    continue;
  }
  break;
      }
      if((gint)event_viewer_data->event_fields_queue->length-count
              < event_viewer_data->num_visible_events){
  j = event_viewer_data->event_fields_queue->length - event_viewer_data->num_visible_events;
  count -= j;
  data = (EventFields*)g_list_nth_data(list,j);
      }else{
  j = count;
  count = 0;
      }
      t = ltt_time_sub(data->time, time_span.start_time);
      event_viewer_data->vadjust_c->value = ltt_time_to_double(t);
      //gtk_adjustment_set_value(event_viewer_data->vadjust_c, 
      //                         ltt_time_to_double(t));
      //g_signal_stop_emission_by_name(G_OBJECT(event_viewer_data->vadjust_c), "value-changed");
      event_viewer_data->previous_value = event_viewer_data->vadjust_c->value;
      insert_data_into_model(event_viewer_data,j, j+event_viewer_data->num_visible_events); 
      //gtk_adjustment_value_changed(event_viewer_data->vadjust_c);

    }else{//the event is not in the buffer
#endif //0
      LttTime start = ltt_time_sub(*current_time, time_span.start_time);
      double position = ltt_time_to_double(start);
      gtk_adjustment_set_value(event_viewer_data->vadjust_c, position);
    //}
  }

  sprintf(str_path,"%d",count);
  path = gtk_tree_path_new_from_string (str_path);
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
  remove_all_items_from_queue(event_viewer_data->event_fields_queue);
  gtk_list_store_clear(event_viewer_data->store_m);
  event_viewer_data->append = TRUE;

  end = ltt_time_sub(time_span.end_time, time_span.start_time);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(end);
  g_signal_emit_by_name(event_viewer_data->vadjust_c, "value-changed");
  //  event_viewer_data->vadjust_c->value = 0;

  return FALSE;
}


void update_raw_data_array(EventViewerData* event_viewer_data, unsigned size)
{
  EventFields * data;
  if(size > 0){
    int pid, tmpPid, i,j,len;
    GList * list, *tmpList;
    GArray * pid_array, * tmp_pid_array;
    
    pid_array     = g_array_sized_new(FALSE, TRUE, sizeof(int), 10);
    tmp_pid_array = g_array_sized_new(FALSE, TRUE, sizeof(int), 10);

    //if the queue is full, remove some data, keep the size of the queue constant
    while(event_viewer_data->event_fields_queue->length + size > RESERVE_BIG_SIZE){
      remove_item_from_queue(event_viewer_data->event_fields_queue,
           event_viewer_data->append);
    }

    //update pid if it is not known
    if(event_viewer_data->event_fields_queue->length > 0){
      list    = event_viewer_data->event_fields_queue->head;
      tmpList = event_viewer_data->event_fields_queue_tmp->head;
      if(event_viewer_data->append){
  for(i=(gint)event_viewer_data->event_fields_queue->length-1;i>=0;i--){
    data = (EventFields*)g_list_nth_data(list,i);
    len = data->pid==0 ? -2 : data->pid;
    if(data->cpu_id+1 > pid_array->len){
      pid_array = g_array_set_size(pid_array,data->cpu_id+1);     
      pid_array = g_array_insert_val(pid_array,data->cpu_id,len);
      pid_array = g_array_remove_index(pid_array,data->cpu_id+1);
    }else if(data->cpu_id+1 < pid_array->len){
      pid = g_array_index(pid_array,int,data->cpu_id);
      if(pid == 0){
        pid_array = g_array_insert_val(pid_array,data->cpu_id,len);
        pid_array = g_array_remove_index(pid_array,data->cpu_id+1);
      }
    }   
  }

  for(i=0;i<(gint)event_viewer_data->event_fields_queue_tmp->length;i++){
    data = (EventFields*)g_list_nth_data(tmpList,i);
    len = data->pid==0 ? -2 : data->pid;
    if(data->cpu_id+1 > tmp_pid_array->len){
      tmp_pid_array = g_array_set_size(tmp_pid_array,data->cpu_id+1);     
      tmp_pid_array = g_array_insert_val(tmp_pid_array,data->cpu_id,len);
      tmp_pid_array = g_array_remove_index(tmp_pid_array,data->cpu_id+1);
    }else if(data->cpu_id+1 < tmp_pid_array->len){
      pid = g_array_index(tmp_pid_array,int,data->cpu_id);
      if(pid == 0){
        tmp_pid_array = g_array_insert_val(tmp_pid_array,data->cpu_id,len);
        tmp_pid_array = g_array_remove_index(tmp_pid_array,data->cpu_id+1);
      }
    }   
  }
      }else{
  for(i=0;i<(gint)event_viewer_data->event_fields_queue->length;i++){
    data = (EventFields*)g_list_nth_data(list,i);
    len = data->pid==0 ? -2 : data->pid;
    if(data->cpu_id+1 > pid_array->len){
      pid_array = g_array_set_size(pid_array,data->cpu_id+1);     
      pid_array = g_array_insert_val(pid_array,data->cpu_id,len);
      pid_array = g_array_remove_index(pid_array,data->cpu_id+1);
    }else if(data->cpu_id+1 < pid_array->len){
      pid = g_array_index(pid_array,int,data->cpu_id);
      if(pid == 0){
        pid_array = g_array_insert_val(pid_array,data->cpu_id,len);
        pid_array = g_array_remove_index(pid_array,data->cpu_id+1);
      }
    }   
  }

  for(i=(gint)event_viewer_data->event_fields_queue_tmp->length-1;i>=0;i--){
    data = (EventFields*)g_list_nth_data(tmpList,i);
    len = data->pid==0 ? -2 : data->pid;
    if(data->cpu_id+1 > tmp_pid_array->len){
      tmp_pid_array = g_array_set_size(tmp_pid_array,data->cpu_id+1);     
      tmp_pid_array = g_array_insert_val(tmp_pid_array,data->cpu_id,len);
      tmp_pid_array = g_array_remove_index(tmp_pid_array,data->cpu_id+1);
    }else if(data->cpu_id+1 < tmp_pid_array->len){
      pid = g_array_index(tmp_pid_array,int,data->cpu_id);
      if(pid == 0){
        tmp_pid_array = g_array_insert_val(tmp_pid_array,data->cpu_id,len);
        tmp_pid_array = g_array_remove_index(tmp_pid_array,data->cpu_id+1);
      }
    }   
  }
      }
      
      len = (pid_array->len > tmp_pid_array->len) ? 
                                    tmp_pid_array->len : pid_array->len;
      for(j=0;j<len;j++){
  pid = g_array_index(pid_array,int, j);
  tmpPid = g_array_index(tmp_pid_array,int,j);
  if(pid == -2)pid = 0;
  if(tmpPid == -2) tmpPid = 0;
  
  if(pid == -1 && tmpPid != -1){
    for(i=0;i<(gint)event_viewer_data->event_fields_queue->length;i++){
      data = (EventFields*)g_list_nth_data(list,i);
      if(data->pid == -1 && (gint)data->cpu_id == j) data->pid = tmpPid;
    }
  }else if(pid != -1 && tmpPid == -1){
    for(i=0;i<(gint)event_viewer_data->event_fields_queue_tmp->length;i++){
      data = (EventFields*)g_list_nth_data(tmpList,i);
      if(data->pid == -1 && (gint)data->cpu_id == j) data->pid = pid;
    }
  }
      }
    }

    g_array_free(pid_array,TRUE);
    g_array_free(tmp_pid_array, TRUE);

    //add data from tmp queue into the queue
    event_viewer_data->number_of_events = 
                     event_viewer_data->event_fields_queue->length 
                     + event_viewer_data->event_fields_queue_tmp->length;
    if(event_viewer_data->append){
      if(event_viewer_data->event_fields_queue->length > 0)
  event_viewer_data->current_event_index = event_viewer_data->event_fields_queue->length - 1;
      else event_viewer_data->current_event_index = 0;
      while((data = g_queue_pop_head(event_viewer_data->event_fields_queue_tmp)) != NULL){
  g_queue_push_tail(event_viewer_data->event_fields_queue, data);
      }
    }else{
      event_viewer_data->current_event_index += event_viewer_data->event_fields_queue_tmp->length;
      while((data = g_queue_pop_tail(event_viewer_data->event_fields_queue_tmp)) != NULL){
  g_queue_push_head(event_viewer_data->event_fields_queue, data);
      }
    }
  }
}

void get_events(EventViewerData* event_viewer_data, LttTime start, 
    LttTime end,unsigned max_num_events, unsigned * real_num_events)
{
  LttvTracesetContext * tsc = lttvwindow_get_traceset_context(event_viewer_data->tab);
  Tab *tab = event_viewer_data->tab;

  //add_context_hooks(event_viewer_data,tsc);
  //seek state because we use the main window's traceset context.
  lttv_state_traceset_seek_time_closest(LTTV_TRACESET_STATE(tsc), start);
  lttv_process_traceset_middle(tsc, start, G_MAXUINT, NULL);
  lttv_process_traceset_begin(tsc,
                              NULL,
                              NULL,
                              NULL,
                              event_viewer_data->event_hooks,
                              NULL);
  if(event_viewer_data->append == TRUE) {
    /* append data */
    lttv_process_traceset_middle(tsc, end, max_num_events, NULL);
  } else{
    guint count;
    LttvTracefileContext *tfc;
    /* prepend data */
    do {
      /* clear the temp list */
      while(g_queue_pop_head(event_viewer_data->event_fields_queue_tmp));
      /* read max_num events max */
      count = lttv_process_traceset_middle(tsc, end, max_num_events, NULL);
      /* loop if reached the max number of events to read, but not
       * if end of trace or end time reached.*/
      tfc = lttv_traceset_context_get_current_tfc(tsc);
    } while(max_num_events == count 
             && (tfc != NULL && ltt_time_compare(tfc->timestamp, end) < 0));

  }
  
  //remove_context_hooks(event_viewer_data,tsc);
  lttv_process_traceset_end(tsc,
                            NULL,
                            NULL,
                            NULL,
                            event_viewer_data->event_hooks,
                            NULL);
  int size;

  size = event_viewer_data->event_fields_queue_tmp->length;
  *real_num_events = size;
  
  update_raw_data_array(event_viewer_data,size);

#if 0
  EventsRequest *events_request = g_new(EventsRequest, 1);
  // Create the hooks
  LttvHooks *event = lttv_hooks_new();
  LttvHooks *after_request = lttv_hooks_new();

  lttv_hooks_add(after_request,
                 after_get_events,
                 events_request,
                 LTTV_PRIO_DEFAULT);
  lttv_hooks_add(event,
                 parse_event,
                 event_viewer_data,
                 LTTV_PRIO_DEFAULT);

  // Fill the events request
  events_request->owner = event_viewer_data;
  events_request->viewer_data = event_viewer_data;
  events_request->servicing = FALSE;
  events_request->start_time = start;
  events_request->start_position = NULL;
  events_request->stop_flag = FALSE;
  events_request->end_time = ltt_time_infinite;
  events_request->num_events = max_num_events;
  events_request->end_position = NULL;
  events_request->trace = -1;  /* FIXME */
  events_request->hooks = NULL;  /* FIXME */
  events_request->before_chunk_traceset = NULL;
  events_request->before_chunk_trace = NULL;
  events_request->before_chunk_tracefile = NULL;
  events_request->event = event;
  events_request->event_by_id = NULL;
  events_request->after_chunk_tracefile = NULL;
  events_request->after_chunk_trace = NULL;
  events_request->after_chunk_traceset = NULL;
  events_request->before_request = NULL;
  events_request->after_request = after_request;

  g_debug("req : start : %u, %u", start.tv_sec, 
                                      start.tv_nsec);

  lttvwindow_events_request_remove_all(tab,
                                       event_viewer_data);
  lttvwindow_events_request(tab, events_request);
#endif //0
}
#if 0
static int after_get_events(void *hook_data, void *call_data)
{  
  EventViewerData *event_viewer_data = (EventViewerData *)hook_data;
  int size;

  size = event_viewer_data->event_fields_queue_tmp->length;
  *real_num_events = size;
  
  update_raw_data_array(event_viewer_data,size);
}
#endif //0

static void get_event_detail(LttEvent *e, LttField *f, GString * s)
{
  LttType *type;
  LttField *element;
  char *name;
  int nb, i;

  type = ltt_field_type(f);
  switch(ltt_type_class(type)) {
    case LTT_INT:
      g_string_append_printf(s, " %ld", ltt_event_get_long_int(e,f));
      break;

    case LTT_UINT:
      g_string_append_printf(s, " %lu", ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_FLOAT:
      g_string_append_printf(s, " %g", ltt_event_get_double(e,f));
      break;

    case LTT_STRING:
      g_string_append_printf(s, " \"%s\"", ltt_event_get_string(e,f));
      break;

    case LTT_ENUM:
      g_string_append_printf(s, " %s", ltt_enum_string_get(type,
          ltt_event_get_unsigned(e,f)-1));
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      g_string_append_printf(s, " {");
      nb = ltt_event_field_element_number(e,f);
      element = ltt_field_element(f);
      for(i = 0 ; i < nb ; i++) {
        ltt_event_field_element_select(e,f,i);
        get_event_detail(e, element, s);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_STRUCT:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        ltt_type_member_type(type, i, &name);
        g_string_append_printf(s, " %s = ", name);        
        get_event_detail(e, element, s);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_UNION:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        ltt_type_member_type(type, i, &name);
        g_string_append_printf(s, " %s = ", name);        
        get_event_detail(e, element, s);
      }
      g_string_append_printf(s, " }");
      break;

  }
  
}

static void get_pid(unsigned * in, unsigned * out, char * s)
{
  char * str;
  str = strstr(s, "out =");
  if (str){
    str = str + 5;
    sscanf(str,"%d", out);
  }else{
    g_warning("Can not find out pid\n");
  }

  str = strstr(s,"in =");
  if (str){
    str = str + 4;
    sscanf(str,"%d", in);
  }else{
    g_warning("Can not find in pid\n");
  }
}

gboolean parse_event(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData *)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  EventFields * tmp_event_fields,*prev_event_fields = NULL, *data=NULL;
  LttEvent *e;
  LttTime time;
  LttField * field;
  unsigned in=0, out=0;
  int i;
  GString * detail_event = g_string_new("");
  GList * list;

  e = tfc->e;
  field = ltt_event_field(e);
  time = ltt_event_time(e);

  tmp_event_fields = g_new(EventFields,1);
  tmp_event_fields->cpu_id = ltt_event_cpu_id(e);
  tmp_event_fields->event_name = g_strdup(ltt_eventtype_name(ltt_event_eventtype(e)));
  tmp_event_fields->time = time;
  tmp_event_fields->ep = ltt_event_position_new();

  if(event_viewer_data->event_fields_queue_tmp->length){ 
    list = event_viewer_data->event_fields_queue_tmp->head;
    for(i=event_viewer_data->event_fields_queue_tmp->length-1;i>=0;i--){
      data = (EventFields *)g_list_nth_data(list,i);
      if(data->cpu_id == tmp_event_fields->cpu_id){
  prev_event_fields = data;
  break;
      }
    }    
  }  

  if(prev_event_fields) tmp_event_fields->pid = prev_event_fields->pid;
  else tmp_event_fields->pid = -1;

  tmp_event_fields->entry_length = field == NULL ? 0 : ltt_field_size(field);
  if(field) get_event_detail(e, field, detail_event);
  tmp_event_fields->event_description  = g_strdup(detail_event->str);

  if(strcmp(tmp_event_fields->event_name, "schedchange") == 0){
    get_pid(&in, &out, detail_event->str);
  }


  if(in != 0 || out != 0){
    tmp_event_fields->pid = in;
    if(prev_event_fields && prev_event_fields->pid == -1){
      list = event_viewer_data->event_fields_queue_tmp->head;
      for(i=0;i<(gint)event_viewer_data->event_fields_queue_tmp->length;i++){
  data = (EventFields *)g_list_nth_data(list,i);
  if(data->cpu_id == tmp_event_fields->cpu_id){
    data->pid = out;
  }
      }
    }
  }
  
  ltt_event_position(e, tmp_event_fields->ep);

  if(event_viewer_data->event_fields_queue_tmp->length >= RESERVE_SMALL_SIZE){
    if(event_viewer_data->append){
      list = g_list_last(event_viewer_data->event_fields_queue_tmp->head);
      data = (EventFields *)(list->data);
      if(data->time.tv_sec  == time.tv_sec &&
   data->time.tv_nsec == time.tv_nsec){
  g_queue_push_tail(event_viewer_data->event_fields_queue_tmp,tmp_event_fields);      
      }else{
  g_free(tmp_event_fields);          
      }
    }else{
      remove_item_from_queue(event_viewer_data->event_fields_queue_tmp,TRUE);
      g_queue_push_tail(event_viewer_data->event_fields_queue_tmp,tmp_event_fields);      
    }
  }else{
    g_queue_push_tail (event_viewer_data->event_fields_queue_tmp,tmp_event_fields);
  }

  g_string_free(detail_event, TRUE);

  return FALSE;
}

void remove_item_from_queue(GQueue * q, gboolean fromHead)
{
  EventFields *data1, *data2 = NULL;
  GList * list;

  if(fromHead){
    data1 = (EventFields *)g_queue_pop_head(q);
    list  = g_list_first(q->head);
    if(list)
      data2 = (EventFields *)(list->data);
  }else{
    data1 = (EventFields *)g_queue_pop_tail(q);
    list  = g_list_last(q->head);
    if(list)
      data2 = (EventFields *)(list->data);
  }

  if(data2){
    if(data1->time.tv_sec  == data2->time.tv_sec &&
       data1->time.tv_nsec == data2->time.tv_nsec){
      remove_item_from_queue(q, fromHead);
    }
  }

  g_free(data1);

  return;
}

void remove_all_items_from_queue(GQueue *q)
{
  EventFields *data;
  while((data = (EventFields *)g_queue_pop_head(q)) != NULL){
    g_free(data);
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
	    init, destroy, "lttvwindow")
