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
 * viewer's insertion menu item and toolbar icon by calling gtkTraceSet's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * Coding standard :
 * pm : parameter
 * l  : local
 * g  : global
 * s  : static
 * h  : hook
 * 
 * Author : Karim Yaghmour
 *          Integrated to LTTng by Mathieu Desnoyers, June 2003
 */

#include <math.h>

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/processTrace.h>
#include <lttv/state.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>
#include <string.h>

//#include "mw_api.h"
#include "gtktreeprivate.h"

#include "icons/hGuiEventsInsert.xpm"


static LttvHooks  *before_event;

/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_event_viewer_data_list = NULL ;

/** hook functions for update time interval, current time ... */
gboolean update_time_window(void * hook_data, void * call_data);
gboolean update_current_time(void * hook_data, void * call_data);
void remove_item_from_queue(GQueue * q, gboolean fromHead);
void remove_all_items_from_queue(GQueue * q);

typedef struct _RawTraceData{
  unsigned  cpu_id;
  char * event_name;
  LttTime time;
  int pid;
  unsigned entry_length;
  char * event_description;
  LttEventPosition ep;
} RawTraceData;

#define RESERVE_BIG_SIZE      1000
#define RESERVE_SMALL_SIZE    100

typedef enum _ScrollDirection{
  SCROLL_STEP_UP,
  SCROLL_STEP_DOWN,
  SCROLL_PAGE_UP,
  SCROLL_PAGE_DOWN,
  SCROLL_JUMP,
  SCROLL_NONE
} ScrollDirection;

typedef struct _EventViewerData {

  mainWindow * mw;
  TimeWindow   time_window;
  LttTime      current_time;
  LttvHooks  * before_event_hooks;

  gboolean     append;                    //prepend or append item 
  GQueue     * raw_trace_data_queue;      //buf to contain raw trace data
  GQueue     * raw_trace_data_queue_tmp;  //tmp buf to contain raw data
  unsigned     current_event_index;
  double       previous_value;            //value of the slide
  TimeInterval time_span;
 // LttTime      trace_start;
 // LttTime      trace_end;
  unsigned     start_event_index;        //the first event shown in the window
  unsigned     end_event_index;          //the last event shown in the window

  //scroll window containing Tree View
  GtkWidget * scroll_win;

  /* Model containing list data */
  GtkListStore *store_m;
  
  GtkWidget *hbox_v;
  /* Widget to display the data in a columned list */
  GtkWidget *tree_v;
  GtkAdjustment *vtree_adjust_c ;
  
  /* Vertical scrollbar and it's adjustment */
  GtkWidget *vscroll_vc;
  GtkAdjustment *vadjust_c ;
	
  /* Selection handler */
  GtkTreeSelection *select_c;
  
  guint num_visible_events;
  guint first_event, last_event;
  
  /* TEST DATA, TO BE READ FROM THE TRACE */
  gint number_of_events ;
  guint currently_selected_event  ;
  gboolean selected_event ;

} EventViewerData ;

//! Event Viewer's constructor hook
GtkWidget *h_gui_events(mainWindow *parent_window);
//! Event Viewer's constructor
EventViewerData *gui_events(mainWindow *parent_window);
//! Event Viewer's destructor
void gui_events_destructor(EventViewerData *event_viewer_data);
void gui_events_free(EventViewerData *event_viewer_data);

static int event_selected_hook(void *hook_data, void *call_data);

void tree_v_set_cursor(EventViewerData *event_viewer_data);
void tree_v_get_cursor(EventViewerData *event_viewer_data);

/* Prototype for selection handler callback */
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
static void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
static void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);
static void tree_v_grab_focus(GtkWidget *widget, gpointer data);


static void get_test_data(double time, guint list_height, 
			  EventViewerData *event_viewer_data);

void add_test_data(EventViewerData *event_viewer_data);

static void get_events(EventViewerData* event_viewer_data, LttTime start, 
		       LttTime end, unsigned max_num_events, unsigned * real_num_events);
static gboolean parse_event(void *hook_data, void *call_data);

static LttvModule *main_win_module;

/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init(LttvModule *self, int argc, char *argv[]) {

  main_win_module = lttv_module_require(self, "mainwin", argc, argv);
  
  if(main_win_module == NULL){
    g_critical("Can't load Control Flow Viewer : missing mainwin\n");
    return;
  }
	

  g_critical("GUI Event Viewer init()");
  
  /* Register the toolbar insert button */
  ToolbarItemReg(hGuiEventsInsert_xpm, "Insert Event Viewer", h_gui_events);
  
  /* Register the menu item insert entry */
  MenuItemReg("/", "Insert Event Viewer", h_gui_events);
  
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
G_MODULE_EXPORT void destroy() {
  int i;
  
  EventViewerData *event_viewer_data;
  
  g_critical("GUI Event Viewer destroy()");

  if(g_event_viewer_data_list){
    g_slist_foreach(g_event_viewer_data_list, event_destroy_walk, NULL );
    g_slist_free(g_event_viewer_data_list);
  }

  /* Unregister the toolbar insert button */
  ToolbarItemUnreg(h_gui_events);
	
  /* Unregister the menu item insert entry */
  MenuItemUnreg(h_gui_events);
}

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
h_gui_events(mainWindow * parent_window)
{
  EventViewerData* event_viewer_data = gui_events(parent_window) ;

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
gui_events(mainWindow *parent_window)
{
  LttTime start, end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  EventViewerData* event_viewer_data = g_new(EventViewerData,1) ;
  RawTraceData * data;
  unsigned size;

  event_viewer_data->mw = parent_window;
  GetTimeWindow(event_viewer_data->mw, &event_viewer_data->time_window);
  GetCurrentTime(event_viewer_data->mw, &event_viewer_data->current_time);
  
  event_viewer_data->before_event_hooks = lttv_hooks_new();
  lttv_hooks_add(event_viewer_data->before_event_hooks, parse_event, event_viewer_data);

  event_viewer_data->raw_trace_data_queue     = g_queue_new();
  event_viewer_data->raw_trace_data_queue_tmp = g_queue_new();  

  RegUpdateTimeWindow(update_time_window,event_viewer_data, event_viewer_data->mw);
  RegUpdateCurrentTime(update_current_time,event_viewer_data, event_viewer_data->mw);

  event_viewer_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show ( event_viewer_data->scroll_win);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(event_viewer_data->scroll_win), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

  /* TEST DATA, TO BE READ FROM THE TRACE */
  event_viewer_data->currently_selected_event = FALSE  ;
  event_viewer_data->selected_event = 0;

  /* Create a model for storing the data list */
  event_viewer_data->store_m = gtk_list_store_new (
		N_COLUMNS,	/* Total number of columns */
		G_TYPE_INT,	/* CPUID                  */
		G_TYPE_STRING,	/* Event                   */
		G_TYPE_UINT64,	/* Time                    */
		G_TYPE_INT,	/* PID                     */
		G_TYPE_INT,	/* Entry length            */
		G_TYPE_STRING);	/* Event's description     */
	
  /* Create the viewer widget for the columned list */
  event_viewer_data->tree_v = gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->store_m));
		
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

  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "grab-focus",
		    G_CALLBACK (tree_v_grab_focus),
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
  column = gtk_tree_view_column_new_with_attributes ("Event's Description",
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
  g_critical("value : %u",event_viewer_data->vtree_adjust_c->upper);
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
  getTracesetTimeSpan(event_viewer_data->mw, &event_viewer_data->time_span);
  
  start = ltt_time_sub(event_viewer_data->time_span.endTime, event_viewer_data->time_span.startTime);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(start) * NANOSECONDS_PER_SECOND;

  event_viewer_data->append = TRUE;

  start.tv_sec = 0;
  start.tv_nsec = 0;
  end.tv_sec = G_MAXULONG;
  end.tv_nsec = G_MAXULONG;

  get_events(event_viewer_data, start,end, RESERVE_SMALL_SIZE, &size);

  event_viewer_data->start_event_index = 0;
  event_viewer_data->end_event_index   = event_viewer_data->num_visible_events - 1;  

  // Test data 
  get_test_data(event_viewer_data->vadjust_c->value,
		event_viewer_data->num_visible_events, 
		event_viewer_data);

  /* Set the Selected Event */
  //  tree_v_set_cursor(event_viewer_data);


  g_object_set_data_full(
			G_OBJECT(event_viewer_data->hbox_v),
			"event_viewer_data",
			event_viewer_data,
			(GDestroyNotify)gui_events_free);
  
  return event_viewer_data;
}

void tree_v_set_cursor(EventViewerData *event_viewer_data)
{
  GtkTreePath *path;
  
  if(event_viewer_data->selected_event && event_viewer_data->first_event != -1)
    {
      //      gtk_adjustment_set_value(event_viewer_data->vadjust_c,
     //			       event_viewer_data->currently_selected_event);
      
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
  g_critical("DEBUG : Event Selected : %i , num: %u", event_viewer_data->selected_event,  event_viewer_data->currently_selected_event) ;
  
  gtk_tree_path_free(path);

}



void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data)
{
  GtkTreePath *path; // = gtk_tree_path_new();
  gint *indices;
  gdouble value;
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), &path, NULL);
  if(path == NULL)
    {
      /* No prior cursor, put it at beginning of page and let the execution do */
      path = gtk_tree_path_new_from_indices(0, -1);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
    }
  
  indices = gtk_tree_path_get_indices(path);
  
  g_critical("DEBUG : move cursor step : %u , int : %i , indice : %i", (guint)arg1, arg2, indices[0]) ;
  
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
		  g_critical("need 1 event down") ;
		  event_viewer_data->currently_selected_event += 1;
		  //		  gtk_adjustment_set_value(event_viewer_data->vadjust_c, value+1);
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
		  g_critical("need 1 event up") ;
		  event_viewer_data->currently_selected_event -= 1;
		  //		  gtk_adjustment_set_value(event_viewer_data->vadjust_c, value-1);
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
	      g_critical("need 1 page down") ;
	      
	      event_viewer_data->currently_selected_event += event_viewer_data->num_visible_events-1;
	      //	      gtk_adjustment_set_value(event_viewer_data->vadjust_c,
	      //				       value+(event_viewer_data->num_visible_events-1));
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
		  g_critical("need 1 page up") ;
		  
		  event_viewer_data->currently_selected_event -= event_viewer_data->num_visible_events-1;
		  
		  //		  gtk_adjustment_set_value(event_viewer_data->vadjust_c,
		  //					   value-(event_viewer_data->num_visible_events-1));
		  //gtk_tree_path_free(path);
		  //path = gtk_tree_path_new_from_indices(0, -1);
		  //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
		  g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		  
		} else {
		  /* Go to first Event */
		  g_critical("need 1 page up") ;
		  
		  event_viewer_data->currently_selected_event == 0 ;
		  //		  gtk_adjustment_set_value(event_viewer_data->vadjust_c,
		  //					   0);
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
	  g_critical("End of buffer") ;
	  event_viewer_data->currently_selected_event = event_viewer_data->number_of_events-1 ;
	  //	  gtk_adjustment_set_value(event_viewer_data->vadjust_c, 
	  //				   event_viewer_data->number_of_events -
	  //				   event_viewer_data->num_visible_events);
	  //gtk_tree_path_free(path);
	  //path = gtk_tree_path_new_from_indices(event_viewer_data->num_visible_events-1, -1);
	  //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
	  g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
	} else {
	  /* Move beginning of buffer */
	  g_critical("Beginning of buffer") ;
	  event_viewer_data->currently_selected_event = 0 ;
	  //	  gtk_adjustment_set_value(event_viewer_data->vadjust_c, 0);
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
  LttTime ltt_time;
  guint64 time;
  GtkTreeIter iter;
  GtkTreeModel* model = GTK_TREE_MODEL(event_viewer_data->store_m);
  GtkTreePath *path;
	
  g_critical("DEBUG : cursor change");
  /* On cursor change, modify the currently selected event by calling
   * the right API function */
  tree_v_get_cursor(event_viewer_data);
/*  
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), &path, NULL);
  if(gtk_tree_model_get_iter(model,&iter,path)){
    gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
    ltt_time.tv_sec = time / NANOSECONDS_PER_SECOND;
    ltt_time.tv_nsec = time % NANOSECONDS_PER_SECOND;
 
    if(ltt_time.tv_sec != event_viewer_data->current_time.tv_sec ||
       ltt_time.tv_nsec != event_viewer_data->current_time.tv_nsec)
      SetCurrentTime(event_viewer_data->mw,&ltt_time);
  }else{
    g_warning("Can not get iter\n");
  }
*/
}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  GtkTreePath *tree_path;

  g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
  
  get_test_data(adjustment->value, event_viewer_data->num_visible_events, 
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
      
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), tree_path,
			       NULL, FALSE);
      gtk_tree_path_free(tree_path);
    }
 
  
}

gint get_cell_height(GtkTreeView *TreeView)
{
  gint height, width;
  GtkTreeViewColumn *column = gtk_tree_view_get_column(TreeView, 0);
  GList *Render_List = gtk_tree_view_column_get_cell_renderers(column);
  GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
  
  gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height);
  g_critical("cell 0 height : %u",height);
  
  return height;
}

void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
  gint last_num_visible_events = event_viewer_data->num_visible_events;
  gdouble exact_num_visible;
  
  g_critical("size-allocate");
  
  exact_num_visible = ( alloc->height -
			TREE_VIEW_HEADER_HEIGHT (GTK_TREE_VIEW(event_viewer_data->tree_v)) )
    / (double)cell_height ;
  
  event_viewer_data->num_visible_events = ceil(exact_num_visible) ;
  
  g_critical("number of events shown : %u",event_viewer_data->num_visible_events);
  g_critical("ex number of events shown : %f",exact_num_visible);

/*
  event_viewer_data->vadjust_c->page_increment = 
    floor(exact_num_visible);
  event_viewer_data->vadjust_c->page_size =
    floor(exact_num_visible);
*/
  
  if(event_viewer_data->num_visible_events != last_num_visible_events)
    {
      get_test_data(event_viewer_data->vadjust_c->value,
		    event_viewer_data->num_visible_events, 
		    event_viewer_data);
    }
  

}

void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data)
{
  gint h;
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
	
  g_critical("size-request");

  h = cell_height + TREE_VIEW_HEADER_HEIGHT
    (GTK_TREE_VIEW(event_viewer_data->tree_v));
  requisition->height = h;
	
}

void get_test_data(double time_value, guint list_height, 
		   EventViewerData *event_viewer_data)
{
  GtkTreeIter iter;
  int i;
  GtkTreeModel *model = GTK_TREE_MODEL(event_viewer_data->store_m);
  GtkTreePath *tree_path;
  RawTraceData * raw_data;
  ScrollDirection  direction = SCROLL_NONE;
  GList * first;
  int event_number;
  double value = event_viewer_data->previous_value - time_value;
  LttTime start, end, time;
  LttEvent * ev;
  unsigned backward_num, minNum, maxNum;
  LttTracefile * tf;
  unsigned  block_num, event_num;
  unsigned size = 1, count = 0;
  gboolean need_backward_again, backward;

  g_warning("DEBUG : get_test_data, time value  %f\n", time_value);
  
  //	if(event_number > event_viewer_data->last_event ||
  //		 event_number + list_height-1 < event_viewer_data->first_event ||
  //		 event_viewer_data->first_event == -1)
  {
    /* no event can be reused, clear and start from nothing */
    if(value == -1.0)      direction = SCROLL_STEP_DOWN;
    else if(value == 1.0 ) direction = SCROLL_STEP_UP;
    else if(value == -2.0) direction = SCROLL_PAGE_DOWN;
    else if(value == 2.0 ) direction = SCROLL_PAGE_UP;
    else if(value == 0.0 ) direction = SCROLL_NONE;
    else direction = SCROLL_JUMP;

    switch(direction){
      case SCROLL_STEP_UP:
      case SCROLL_PAGE_UP:
	if(direction == SCROLL_PAGE_UP){
	  backward = list_height>event_viewer_data->start_event_index ? TRUE : FALSE;
	}else{
	  backward = event_viewer_data->start_event_index == 0 ? TRUE : FALSE;
	}
	if(backward){
	  event_viewer_data->append = FALSE;
	  do{
	    if(direction == SCROLL_PAGE_UP){
	      minNum = list_height - event_viewer_data->start_event_index ;
	    }else{
	      minNum = 1;
	    }

	    first = event_viewer_data->raw_trace_data_queue->head;
	    raw_data = (RawTraceData*)g_list_nth_data(first,0);
	    end = raw_data->time;
	    end.tv_nsec--;
	    ltt_event_position_get(&raw_data->ep, &block_num, &event_num, &tf);
	    if(size !=0){
	      if(event_num > minNum){
		backward_num = event_num > RESERVE_SMALL_SIZE 
		              ? event_num - RESERVE_SMALL_SIZE : 1;
		ltt_event_position_set(&raw_data->ep, block_num, backward_num);
		ltt_tracefile_seek_position(tf, &raw_data->ep);
		ev = ltt_tracefile_read(tf);
		start = ltt_event_time(ev);
		maxNum = G_MAXULONG;
	      }else{
		if(block_num > 1){
		  ltt_event_position_set(&raw_data->ep, block_num-1, 1);
		  ltt_tracefile_seek_position(tf, &raw_data->ep);
		  ev = ltt_tracefile_read(tf);
		  start = ltt_event_time(ev);	    	      
		}else{
		  start.tv_sec  = 0;
		  start.tv_nsec = 0;		
		}
		maxNum = G_MAXULONG;
	      }
	    }else{
	      if(block_num > count){
		ltt_event_position_set(&raw_data->ep, block_num-count, 1);
		ltt_tracefile_seek_position(tf, &raw_data->ep);
		ev = ltt_tracefile_read(tf);
		start = ltt_event_time(ev);	    	      
	      }else{
		start.tv_sec  = 0;
		start.tv_nsec = 0;		
	      }	      
	      maxNum = G_MAXULONG;
	    }

	    event_viewer_data->current_event_index = event_viewer_data->start_event_index;
	    get_events(event_viewer_data, start, end, maxNum, &size);
	    event_viewer_data->start_event_index = event_viewer_data->current_event_index;

	    if(size < minNum && (start.tv_sec !=0 || start.tv_nsec !=0))
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
	if(event_viewer_data->end_event_index == event_viewer_data->number_of_events - 1){
	  event_viewer_data->append = TRUE;
	  first = event_viewer_data->raw_trace_data_queue->head;
	  raw_data = (RawTraceData*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
	  start = raw_data->time;
	  start.tv_nsec++;
	  end.tv_sec = G_MAXULONG;
	  end.tv_nsec = G_MAXULONG;
	  get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE, &size);
	}else size = 1;
	if(size > 0) event_number = event_viewer_data->start_event_index + 1;	
	else         event_number = event_viewer_data->start_event_index;
	break;
      case SCROLL_PAGE_DOWN:
	if(event_viewer_data->end_event_index >= event_viewer_data->number_of_events - 1 - list_height){
	  event_viewer_data->append = TRUE;
	  first = event_viewer_data->raw_trace_data_queue->head;
	  raw_data = (RawTraceData*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
	  start = raw_data->time;
	  start.tv_nsec++;
	  end.tv_sec = G_MAXULONG;
	  end.tv_nsec = G_MAXULONG;
	  get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE,&size);
	}
	if(list_height <= event_viewer_data->number_of_events - 1 - event_viewer_data->end_event_index)
	  event_number = event_viewer_data->start_event_index + list_height - 1;	
	else
	  event_number = event_viewer_data->number_of_events - 1 - list_height;		  
	break;
      case SCROLL_JUMP:
	event_viewer_data->append = TRUE;
	remove_all_items_from_queue(event_viewer_data->raw_trace_data_queue);
	end.tv_sec = G_MAXULONG;
	end.tv_nsec = G_MAXULONG;
	time = ltt_time_from_double(time_value / NANOSECONDS_PER_SECOND);
	start = ltt_time_add(event_viewer_data->time_span.startTime, time);
	event_viewer_data->previous_value = time_value;
	get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE,&size);
	if(size < list_height){
	  event_viewer_data->append = FALSE;
	  first = event_viewer_data->raw_trace_data_queue->head;
	  raw_data = (RawTraceData*)g_list_nth_data(first,0);
	  end = raw_data->time;
	  end.tv_nsec--;
	  ltt_event_position_get(&raw_data->ep, &block_num, &event_num, &tf);
	  
	  if(event_num > list_height - size){
	    backward_num = event_num > RESERVE_SMALL_SIZE 
	      ? event_num - RESERVE_SMALL_SIZE : 1;
	    ltt_event_position_set(&raw_data->ep, block_num, backward_num);
	    ltt_tracefile_seek_position(tf, &raw_data->ep);
	    ev = ltt_tracefile_read(tf);
	    start = ltt_event_time(ev);
	    maxNum = G_MAXULONG;
	    event_viewer_data->current_event_index = 0;
	    get_events(event_viewer_data, start, end, maxNum, &size);
	    event_viewer_data->start_event_index = event_viewer_data->current_event_index;
	  }
	  event_number = event_viewer_data->raw_trace_data_queue->length - list_height;
	}else{
	  event_number = 0;
	}
	break;
      case SCROLL_NONE:
	event_number = event_viewer_data->current_event_index;
	break;
      default:
	  break;
    }

    //update the value of the scroll bar
    if(direction != SCROLL_NONE && direction != SCROLL_JUMP){
      first = event_viewer_data->raw_trace_data_queue->head;
      raw_data = (RawTraceData*)g_list_nth_data(first,event_number);
      time = ltt_time_sub(raw_data->time, event_viewer_data->time_span.startTime);
      event_viewer_data->vadjust_c->value = ltt_time_to_double(time) * NANOSECONDS_PER_SECOND;
      g_signal_stop_emission_by_name(G_OBJECT(event_viewer_data->vadjust_c), "value-changed");
      event_viewer_data->previous_value = value;
    }
    

    event_viewer_data->start_event_index = event_number;
    event_viewer_data->end_event_index = event_number + list_height - 1;    

    first = event_viewer_data->raw_trace_data_queue->head;
    gtk_list_store_clear(event_viewer_data->store_m);
    for(i=event_number; i<event_number+list_height; i++)
      {
	guint64 real_data;

	if(i>=event_viewer_data->number_of_events) break;
       
	raw_data = (RawTraceData*)g_list_nth_data(first, i);

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
/*
	gtk_list_store_append (event_viewer_data->store_m, &iter);
	gtk_list_store_set (event_viewer_data->store_m, &iter,
			    CPUID_COLUMN, 0,
			    EVENT_COLUMN, "event irq",
			    TIME_COLUMN, i,
			    PID_COLUMN, 100,
			    ENTRY_LEN_COLUMN, 17,
			    EVENT_DESCR_COLUMN, "Detailed information",
			    -1);
*/
      }
  }
#ifdef DEBUG //do not use this, it's slower and broken
  //	} else {
  /* Some events will be reused */
  if(event_number < event_viewer_data->first_event)
    {
      /* scrolling up, prepend events */
      tree_path = gtk_tree_path_new_from_indices
	(event_number+list_height-1 -
	 event_viewer_data->first_event + 1,
	 -1);
      for(i=0; i<event_viewer_data->last_event-(event_number+list_height-1);
	  i++)
	{
	  /* Remove the last events from the list */
	  if(gtk_tree_model_get_iter(model, &iter, tree_path))
	    gtk_list_store_remove(event_viewer_data->store_m, &iter);
	}
      
      for(i=event_viewer_data->first_event-1; i>=event_number; i--)
	{
	  if(i>=event_viewer_data->number_of_events) break;
	  /* Prepend new events */
	  gtk_list_store_prepend (event_viewer_data->store_m, &iter);
	  gtk_list_store_set (event_viewer_data->store_m, &iter,
			      CPUID_COLUMN, 0,
			      EVENT_COLUMN, "event irq",
			      TIME_COLUMN, i,
			      PID_COLUMN, 100,
			      ENTRY_LEN_COLUMN, 17,
			      EVENT_DESCR_COLUMN, "Detailed information",
			      -1);
	}
    } else {
      /* Scrolling down, append events */
      for(i=event_viewer_data->first_event; i<event_number; i++)
	{
	  /* Remove these events from the list */
	  gtk_tree_model_get_iter_first(model, &iter);
	  gtk_list_store_remove(event_viewer_data->store_m, &iter);
	}
      for(i=event_viewer_data->last_event+1; i<event_number+list_height; i++)
	{
	  if(i>=event_viewer_data->number_of_events) break;
	  /* Append new events */
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
  //}
#endif //DEBUG
  event_viewer_data->first_event = event_viewer_data->start_event_index ;
  event_viewer_data->last_event = event_viewer_data->end_event_index ;



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
  if(event_viewer_data){
    lttv_hooks_remove(event_viewer_data->before_event_hooks,parse_event);
    lttv_hooks_destroy(event_viewer_data->before_event_hooks);
    
    remove_all_items_from_queue (event_viewer_data->raw_trace_data_queue);
    g_queue_free(event_viewer_data->raw_trace_data_queue);
    g_queue_free(event_viewer_data->raw_trace_data_queue_tmp);

    UnregUpdateTimeWindow(update_time_window,event_viewer_data, event_viewer_data->mw);
    UnregUpdateCurrentTime(update_current_time,event_viewer_data, event_viewer_data->mw);

    g_event_viewer_data_list = g_slist_remove(g_event_viewer_data_list, event_viewer_data);
    g_warning("Delete Event data\n");
    g_free(event_viewer_data);
  }
}

void
gui_events_destructor(EventViewerData *event_viewer_data)
{
  guint index;

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(event_viewer_data->hbox_v)){
    gtk_widget_destroy(event_viewer_data->hbox_v);
    event_viewer_data = NULL;
  }
  
  /* Destroy the Tree View */
  //gtk_widget_destroy(event_viewer_data->tree_v);
  
  /*  Clear raw event list */
  //gtk_list_store_clear(event_viewer_data->store_m);
  //gtk_widget_destroy(GTK_WIDGET(event_viewer_data->store_m));
  
  g_warning("Delete Event data from destroy\n");
  //gui_events_free(event_viewer_data);
}

//FIXME : call hGuiEvents_Destructor for corresponding data upon widget destroy

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
      
      g_print ("Event selected :  %s\n", event);
      
      g_free (event);
    }
}


int event_selected_hook(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  guint *event_number = (guint*) call_data;
  
  g_critical("DEBUG : event selected by main window : %u", *event_number);
  
  event_viewer_data->currently_selected_event = *event_number;
  event_viewer_data->selected_event = TRUE ;
  
  tree_v_set_cursor(event_viewer_data);

}


gboolean update_time_window(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  event_viewer_data->time_window = *(TimeWindow*)call_data;

  return FALSE;
}

gboolean update_current_time(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  event_viewer_data->current_time = *(LttTime*)call_data;
  uint64_t nsec = event_viewer_data->current_time.tv_sec * NANOSECONDS_PER_SECOND 
                  + event_viewer_data->current_time.tv_nsec;
  GtkTreeIter iter;
  uint64_t time;
  int count = 0;
  GtkTreeModel* model = (GtkTreeModel*)event_viewer_data->store_m;


  if(gtk_tree_model_get_iter_first(model, &iter)){
    while(1){
      gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
      if(time < nsec){
	if(!gtk_tree_model_iter_next(model, &iter)){
	  return TRUE;
	}
	count++;
      }else{
	break;
      }
    }
    //    event_selected_hook(event_viewer_data, &count);
  }

  return FALSE;
}

void tree_v_grab_focus(GtkWidget *widget, gpointer data){
  EventViewerData *event_viewer_data = (EventViewerData *)data;
  mainWindow * mw = event_viewer_data->mw;
  SetFocusedPane(mw, gtk_widget_get_parent(event_viewer_data->hbox_v));
}

void get_events(EventViewerData* event_viewer_data, LttTime start, 
		LttTime end,unsigned max_num_events, unsigned * real_num_events)
{
  int size;
  RawTraceData * data;
  contextAddHooks(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
		  NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);
  processTraceset(event_viewer_data->mw, start, end, max_num_events);
  contextRemoveHooks(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
		     NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);

  size = event_viewer_data->raw_trace_data_queue_tmp->length;
  *real_num_events = size;
  if(size > 0){
    int pid, tmpPid, i;
    GList * list, *tmpList;

    //if the queue is full, remove some data, keep the size of the queue constant
    while(event_viewer_data->raw_trace_data_queue->length + size > RESERVE_BIG_SIZE){
      remove_item_from_queue(event_viewer_data->raw_trace_data_queue,
			     event_viewer_data->append);
    }

    //update pid if it is not known
    if(event_viewer_data->raw_trace_data_queue->length > 0){
      list    = event_viewer_data->raw_trace_data_queue->head;
      tmpList = event_viewer_data->raw_trace_data_queue_tmp->head;
      if(event_viewer_data->append){
	data = (RawTraceData*)g_list_nth_data(list, event_viewer_data->raw_trace_data_queue->length-1);
	pid  = data->pid;
	data = (RawTraceData*)g_list_nth_data(tmpList, 0);
	tmpPid = data->pid;
      }else{
	data = (RawTraceData*)g_list_nth_data(list, 0);
	pid  = data->pid;
	data = (RawTraceData*)g_list_nth_data(tmpList, event_viewer_data->raw_trace_data_queue_tmp->length-1);
	tmpPid = data->pid;
      }
      
      if(pid == -1 && tmpPid != -1){
	for(i=0;i<event_viewer_data->raw_trace_data_queue->length;i++){
	  data = (RawTraceData*)g_list_nth_data(list,i);
	  if(data->pid == -1) data->pid = tmpPid;
	}
      }else if(pid != -1 && tmpPid == -1){
	for(i=0;i<event_viewer_data->raw_trace_data_queue_tmp->length;i++){
	  data = (RawTraceData*)g_list_nth_data(tmpList,i);
	  if(data->pid == -1) data->pid = tmpPid;
	}
      }
    }

    //add data from tmp queue into the queue
    event_viewer_data->number_of_events = event_viewer_data->raw_trace_data_queue->length 
                                        + event_viewer_data->raw_trace_data_queue_tmp->length;
    if(event_viewer_data->append){
      if(event_viewer_data->raw_trace_data_queue->length > 0)
	event_viewer_data->current_event_index = event_viewer_data->raw_trace_data_queue->length - 1;
      else event_viewer_data->current_event_index = 0;
      while((data = g_queue_pop_head(event_viewer_data->raw_trace_data_queue_tmp)) != NULL){
	g_queue_push_tail(event_viewer_data->raw_trace_data_queue, data);
      }
    }else{
      event_viewer_data->current_event_index += event_viewer_data->raw_trace_data_queue_tmp->length;
      while((data = g_queue_pop_tail(event_viewer_data->raw_trace_data_queue_tmp)) != NULL){
	g_queue_push_head(event_viewer_data->raw_trace_data_queue, data);
      }
    }
  }
}

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

  RawTraceData * tmp_raw_trace_data,*prev_raw_trace_data = NULL, *data=NULL;
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

  if(event_viewer_data->raw_trace_data_queue_tmp->length){ 
    list = g_list_last(event_viewer_data->raw_trace_data_queue_tmp->head);
    prev_raw_trace_data = (RawTraceData *)(list->data);
  }

  tmp_raw_trace_data = g_new(RawTraceData,1);
  tmp_raw_trace_data->cpu_id = ltt_event_cpu_id(e);
  tmp_raw_trace_data->event_name = g_strdup(ltt_eventtype_name(ltt_event_eventtype(e)));
  tmp_raw_trace_data->time = time;

  if(prev_raw_trace_data) tmp_raw_trace_data->pid = prev_raw_trace_data->pid;
  else tmp_raw_trace_data->pid = -1;

  tmp_raw_trace_data->entry_length = field == NULL ? 0 : field->field_size;
  if(field) get_event_detail(e, field, detail_event);
  tmp_raw_trace_data->event_description  = g_strdup(detail_event->str);

  if(strcmp(tmp_raw_trace_data->event_name, "schedchange") == 0){
    get_pid(&in, &out, detail_event->str);
  }


  if(in != 0 || out != 0){
    tmp_raw_trace_data->pid = in;
    if(prev_raw_trace_data && prev_raw_trace_data->pid == -1){
      list = event_viewer_data->raw_trace_data_queue_tmp->head;
      for(i=0;i<event_viewer_data->raw_trace_data_queue_tmp->length;i++){
 	data = (RawTraceData *)g_list_nth_data(list,i);
	data->pid = out;
      }
    }
  }
  
  ltt_event_position(e, &tmp_raw_trace_data->ep);

  if(event_viewer_data->raw_trace_data_queue_tmp->length >= RESERVE_SMALL_SIZE){
    if(event_viewer_data->append){
      list = g_list_last(event_viewer_data->raw_trace_data_queue_tmp->head);
      data = (RawTraceData *)(list->data);
      if(data->time.tv_sec  == time.tv_sec &&
	 data->time.tv_nsec == time.tv_nsec){
	g_queue_push_tail(event_viewer_data->raw_trace_data_queue_tmp,tmp_raw_trace_data);      
      }else{
	g_free(tmp_raw_trace_data);          
      }
    }else{
      remove_item_from_queue(event_viewer_data->raw_trace_data_queue_tmp,TRUE);
      g_queue_push_tail(event_viewer_data->raw_trace_data_queue_tmp,tmp_raw_trace_data);      
    }
  }else{
    g_queue_push_tail (event_viewer_data->raw_trace_data_queue_tmp,tmp_raw_trace_data);
  }

  g_string_free(detail_event, TRUE);

  return FALSE;
}

void remove_item_from_queue(GQueue * q, gboolean fromHead)
{
  RawTraceData *data1, *data2 = NULL;
  GList * list;

  if(fromHead){
    data1 = (RawTraceData *)g_queue_pop_head(q);
    list  = g_list_first(q->head);
    if(list)
      data2 = (RawTraceData *)(list->data);
  }else{
    data1 = (RawTraceData *)g_queue_pop_tail(q);
    list  = g_list_last(q->head);
    if(list)
      data2 = (RawTraceData *)(list->data);
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
  RawTraceData *data;
  while((data = (RawTraceData *)g_queue_pop_head(q)) != NULL){
    g_free(data);
  }  
}





/* Imported code from LTT 0.9.6pre2 tracevisualizer */
#ifdef DEBUG

/******************************************************************
 * Function :
 *    WDI_gtk_clist_set_last_row_data_full()
 * Description :
 *    Appends data to the last row of a GtkClist.
 * Parameters :
 * Return values :
 *    NONE.
 * History :
 *    J.H.D., 27/08/99, Initial typing.
 * Note :
 *    Based on gtk_clist_set_row_data_full() version 1.2.3.
 *    Much faster than using gtk_clist_set_row_data_full().
 ******************************************************************/
static void WDI_gtk_clist_set_last_row_data_full(GtkCList*         pmClist,
					  gpointer          pmData,
					  GtkDestroyNotify  pmDestroy)
{
  GtkCListRow *pClistRow;

  g_return_if_fail (pmClist != NULL);
  g_return_if_fail (GTK_IS_CLIST (pmClist));
  g_return_if_fail (pmClist->row_list_end != NULL);

  pClistRow = pmClist->row_list_end->data;
  pClistRow->data    = pmData;
  pClistRow->destroy = pmDestroy;
}


/******************************************************************
 * Function :
 *    SHRTEventSelect()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTEventSelect(GtkWidget*      pmCList,
		     gint            pmRow,
		     gint            pmColumn,
		     GdkEventButton* pmEvent,
		     gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Store the selected event */
  pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), pmRow);
  pSysView->Window->EventSelected = TRUE;
}

/******************************************************************
 * Function :
 *    SHRTEventButtonPress()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTEventButtonPress(GtkWidget*      pmCList,
			  GdkEventButton* pmEvent,
			  gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */
  gint         row, column;     /* The clicked row and column */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* if we have a right-click event */
  if(pmEvent->button == 3)
    /* If we clicked on an item, get its row and column values */
    if(gtk_clist_get_selection_info(GTK_CLIST(pmCList), pmEvent->x, pmEvent->y, &row, &column))
      {
      /* Highlight the selected row */
      gtk_clist_select_row(GTK_CLIST(pmCList), row, column);

      /* Store the selected event */
      pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), row);
      pSysView->Window->EventSelected = TRUE;

      /* Display the popup menu */
      gtk_menu_popup(GTK_MENU(pSysView->Window->RawEventPopup),
		     NULL, NULL, NULL, NULL,
		     pmEvent->button, GDK_CURRENT_TIME);
      }
}


/******************************************************************
 * Function :
 *    SHRTVAdjustValueChanged()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTVAdjustValueChanged(GtkAdjustment*  pmVAdjust,
			     gpointer        pmData)
{
  event        lEvent;          /* Event used for searching */
  guint32      lPosition;       /* The position to scroll to */
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Is there an event database? */
  if(pSysView->EventDB == NULL)
    return;

  /* Set the pointer to the first event */
  if(pSysView->EventDB->TraceStart == NULL)
    return;

  /* Are we closer to the beginning? */
  if((pmVAdjust->value - (pmVAdjust->upper / 2)) < 0)
    {
    /* Set the navigation pointer to the beginning of the list */
    lEvent =  pSysView->EventDB->FirstEvent;

    /* Calculate distance from beginning */
    lPosition = (guint32) pmVAdjust->value;

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventNext(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }
  else
    {
    /* Set the navigation pointer to the end of the list */
    lEvent = pSysView->EventDB->LastEvent;

    /* Calculate distance from end */
    lPosition = (guint32) (pmVAdjust->upper - pmVAdjust->value);

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventPrev(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }

  /* Fill the event list according to what was found */
  WDFillEventList(pSysView->Window->RTCList,
		  pSysView->EventDB,
		  pSysView->System,
		  &lEvent,
		  &(pSysView->Window->LastSelectedEvent));
}



/******************************************************************
 * Function :
 *    WDConnectSignals()
 * Description :
 *    Attaches signal handlers to the window items.
 * Parameters :
 *    pmSysView, System view for which signals have to be connected
 * Return values :
 *    NONE
 * History :
 * Note :
 *    This function attaches a pointer to the main window during
 *    the connect. This means that the handlers will get a pointer
 *    to the window in the data argument.
 ******************************************************************/
static void WDConnectSignals(systemView* pmSysView)
{
  /* Raw event Popup menu */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawGotoProcess),
		     "activate",
		     GTK_SIGNAL_FUNC(SHGotoProcAnalysis),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawViewEvent),
		     "activate",
		     GTK_SIGNAL_FUNC(SHViewEventInEG),
		     pmSysView);

  /* Set event list callbacks */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "select_row",
		     GTK_SIGNAL_FUNC(SHRTEventSelect),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "button-press-event",
		     GTK_SIGNAL_FUNC(SHRTEventButtonPress),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTVAdjust),
		     "value-changed",
		     GTK_SIGNAL_FUNC(SHRTVAdjustValueChanged),
		     pmSysView);


}


/******************************************************************
 * Function :
 *    WDFillEventList()
 * Description :
 *    Fills the window's event list using the trace database.
 * Parameters :
 *    pmList, The list to be filled.
 *    pmTraceDB, The database of events.
 *    pmSystem, The system to which this list belongs.
 *    pmEvent, Event from which we start drawing.
 *    pmSelectedEvent, Event selected if any.
 * Return values :
 *    NONE.
 * History :
 *    K.Y., 18/06/99, Initial typing.
 * Note :
 ******************************************************************/
static void WDFillEventList(GtkWidget*  pmList,
		     db*         pmTraceDB,
		     systemInfo* pmSystem,
		     event*      pmEvent,
		     event*      pmSelectedEvent)
{
  gint                i = 0;                              /* Generic index */
  event               lEvent;                             /* Generic event */
  gchar               lTimeStr[TIME_STR_LEN];             /* Time of event */
  static gchar*       lString[RTCLIST_NB_COLUMNS]={'\0'}; /* Strings describing event */
  process*            pProcess;                           /* Generic process pointer */
#if SUPP_RTAI
  RTAItask*           pTask = NULL;                       /* Generic task pointer */
#endif /* SUPP_RTAI */
  eventDescription    lEventDesc;                         /* Description of event */

  /* Did we allocate space for strings */
  if(lString[0] == NULL)
    /* Allocate space for strings */
    for (i = 0;  i < RTCLIST_NB_COLUMNS - 1; i++)
      lString[i] = (char*) g_malloc(MW_DEFAULT_STRLEN);

  /* Allocate space for description string */
  lString[RTCLIST_NB_COLUMNS - 1] = (char*) g_malloc(MW_LONG_STRLEN);

  /* If no event was supplied, start at the beginning */
  if(pmEvent == NULL)
    lEvent = pmTraceDB->FirstEvent;
  else
    lEvent = *pmEvent;

  /* Freeze and clear clist */
  gtk_clist_freeze(GTK_CLIST(pmList));
  gtk_clist_clear(GTK_CLIST(pmList));

  /* Reset index */
  i = 0;

  /* Go through the event list */
  do
    {
    /* Get the event description */
    DBEventDescription(pmTraceDB, &lEvent, TRUE, &lEventDesc);

    /* Get the event's process */
    pProcess = DBEventProcess(pmTraceDB, &lEvent, pmSystem, FALSE);

#if SUPP_RTAI
    /* Does this trace contain RTAI information */
    if(pmTraceDB->SystemType == TRACE_SYS_TYPE_RTAI_LINUX)
      /* Get the RTAI task to which this event belongs */
      pTask = RTAIDBEventTask(pmTraceDB, &lEvent, pmSystem, FALSE);
#endif /* SUPP_RTAI */

    /* Set the event's entry in the list of raw events displayed */
    sRawEventsDisplayed[i] = lEvent;

    /* Add text describing the event */
    /*  The CPU ID */
    if(pmTraceDB->LogCPUID == TRUE)
      snprintf(lString[0], MW_DEFAULT_STRLEN, "%d", lEventDesc.CPUID);
    else
      snprintf(lString[0], MW_DEFAULT_STRLEN, "0");

    /*  The event ID */
    snprintf(lString[1], MW_DEFAULT_STRLEN, "%s", pmTraceDB->EventString(pmTraceDB, lEventDesc.ID, &lEvent));

    /*  The event's time of occurence */
    DBFormatTimeInReadableString(lTimeStr,
				 lEventDesc.Time.tv_sec,
				 lEventDesc.Time.tv_usec);    
    snprintf(lString[2], MW_DEFAULT_STRLEN, "%s", lTimeStr);

    /* Is this an RT event */
    if(lEventDesc.ID <= TRACE_MAX)
      {
      /*  The PID of the process to which the event belongs */
      if(pProcess != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "%d", pProcess->PID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "N/A");
      }
#if SUPP_RTAI
    else
      {
      /*  The TID of the task to which the event belongs */
      if(pTask != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:%d", pTask->TID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:N/A");
      }
#endif /* SUPP_RTAI */

    /*  The size of the entry */
    snprintf(lString[4], MW_DEFAULT_STRLEN, "%d", lEventDesc.Size);

    /*  The string describing the event */
    snprintf(lString[5], MW_LONG_STRLEN, "%s", lEventDesc.String);

    /* Insert the entry into the list */
    gtk_clist_append(GTK_CLIST(pmList), lString);

    /* Set the row's data to point to the current event */
    WDI_gtk_clist_set_last_row_data_full(GTK_CLIST(pmList), (gpointer) &(sRawEventsDisplayed[i]), NULL);

    /* Was this the last selected event */
    if(DBEventsEqual(lEvent, (*pmSelectedEvent)))
      gtk_clist_select_row(GTK_CLIST(pmList), i, 0);

    /* Go to next row */
    i++;
    } while((DBEventNext(pmTraceDB, &lEvent) == TRUE) && (i < RTCLIST_NB_ROWS));

  /* Resize the list's length */
  gtk_widget_queue_resize(pmList);

  /* Thaw the clist */
  gtk_clist_thaw(GTK_CLIST(pmList));
}

#endif //DEBUG

static void destroy_cb( GtkWidget *widget,
		                        gpointer   data )
{ 
	    gtk_main_quit ();
}


/*
int main(int argc, char **argv)
{
	GtkWidget *Window;
	GtkWidget *ListViewer;
	GtkWidget *VBox_V;
	EventViewerData *event_viewer_data;
	guint ev_sel = 444 ;
	
	// Initialize i18n support 
  gtk_set_locale ();

  // Initialize the widget set 
  gtk_init (&argc, &argv);

	init();

  Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (Window), ("Test Window"));
	
	g_signal_connect (G_OBJECT (Window), "destroy",
			G_CALLBACK (destroy_cb), NULL);


  VBox_V = gtk_vbox_new(0, 0);
	gtk_container_add (GTK_CONTAINER (Window), VBox_V);

   //ListViewer = h_gui_events(Window);
  //gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, TRUE, TRUE, 0);

  //ListViewer = h_gui_events(Window);
  //gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, FALSE, TRUE, 0);
	
	event_viewer_data = gui_events(g_new(mainWindow,1));
	ListViewer = event_viewer_data->hbox_v;
	gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, TRUE, TRUE, 0);
	
  gtk_widget_show (VBox_V);
	gtk_widget_show (Window);

	//	event_selected_hook(event_viewer_data, &ev_sel);
	
	gtk_main ();

	g_critical("main loop finished");
  
	//hGuiEvents_Destructor(ListViewer);

	//g_critical("GuiEvents Destructor finished");
	destroy();
	
	return 0;
}
*/

/*\@}*/

