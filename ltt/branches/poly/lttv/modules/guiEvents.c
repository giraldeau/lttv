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
#include <ltt/facility.h>
#include <string.h>

//#include "mw_api.h"
#include "gtktreeprivate.h"

#include "icons/hGuiEventsInsert.xpm"


static LttvHooks  *before_event;

/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_event_viewer_data_list = NULL ;

typedef struct _RawTraceData{
  unsigned  cpu_id;
  char * event_name;
  LttTime time;
  int pid;
  unsigned entry_length;
  char * event_description;
  LttEventPosition *ep;
} RawTraceData;

#define RESERVE_BIG_SIZE             1000
#define RESERVE_SMALL_SIZE           100
#define RESERVE_SMALL_SIZE_SQUARE    RESERVE_SMALL_SIZE*RESERVE_SMALL_SIZE
#define RESERVE_SMALL_SIZE_CUBE      RESERVE_SMALL_SIZE*RESERVE_SMALL_SIZE_SQUARE

typedef enum _ScrollDirection{
  SCROLL_STEP_UP,
  SCROLL_STEP_DOWN,
  SCROLL_PAGE_UP,
  SCROLL_PAGE_DOWN,
  SCROLL_JUMP,
  SCROLL_NONE
} ScrollDirection;

typedef struct _EventViewerData {

  MainWindow * mw;
  TimeWindow   time_window;
  LttTime      current_time;
  LttvHooks  * before_event_hooks;

  gboolean     append;                    //prepend or append item 
  GQueue     * raw_trace_data_queue;      //buf to contain raw trace data
  GQueue     * raw_trace_data_queue_tmp;  //tmp buf to contain raw data
  unsigned     current_event_index;
  double       previous_value;            //value of the slide
  TimeInterval time_span;
  unsigned     start_event_index;        //the first event shown in the window
  unsigned     end_event_index;          //the last event shown in the window
  unsigned     size;                     //maxi number of events loaded when instance the viewer
  gboolean     shown;                    //indicate if event detail is shown or not
  char *       filter_key;

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

/** hook functions for update time interval, current time ... */
gboolean update_time_window(void * hook_data, void * call_data);
gboolean update_current_time(void * hook_data, void * call_data);
gboolean show_event_detail(void * hook_data, void * call_data);
gboolean traceset_changed(void * hook_data, void * call_data);
void remove_item_from_queue(GQueue * q, gboolean fromHead);
void remove_all_items_from_queue(GQueue * q);
void add_context_hooks(EventViewerData * event_viewer_data, 
		       LttvTracesetContext * tsc);
void remove_context_hooks(EventViewerData * event_viewer_data, 
			  LttvTracesetContext * tsc);

//! Event Viewer's constructor hook
GtkWidget *h_gui_events(MainWindow *parent_window, LttvTracesetSelector * s, char* key);
//! Event Viewer's constructor
EventViewerData *gui_events(MainWindow *parent_window, LttvTracesetSelector *s, char *key);
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

static void update_raw_data_array(EventViewerData* event_viewer_data, unsigned size);

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
	
  /* Register the toolbar insert button */
  toolbar_item_reg(hGuiEventsInsert_xpm, "Insert Event Viewer", h_gui_events);
  
  /* Register the menu item insert entry */
  menu_item_reg("/", "Insert Event Viewer", h_gui_events);
  
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
  
  if(g_event_viewer_data_list){
    g_slist_foreach(g_event_viewer_data_list, event_destroy_walk, NULL );
    g_slist_free(g_event_viewer_data_list);
  }

  /* Unregister the toolbar insert button */
  toolbar_item_unreg(h_gui_events);
	
  /* Unregister the menu item insert entry */
  menu_item_unreg(h_gui_events);
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
h_gui_events(MainWindow * parent_window, LttvTracesetSelector * s, char* key)
{
  EventViewerData* event_viewer_data = gui_events(parent_window, s, key) ;

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
gui_events(MainWindow *parent_window, LttvTracesetSelector * s,char* key )
{
  LttTime start, end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  EventViewerData* event_viewer_data = g_new(EventViewerData,1) ;
  RawTraceData * data;

  event_viewer_data->mw = parent_window;
  get_time_window(event_viewer_data->mw, &event_viewer_data->time_window);
  get_current_time(event_viewer_data->mw, &event_viewer_data->current_time);
  
  event_viewer_data->before_event_hooks = lttv_hooks_new();
  lttv_hooks_add(event_viewer_data->before_event_hooks, parse_event, event_viewer_data);

  event_viewer_data->raw_trace_data_queue     = g_queue_new();
  event_viewer_data->raw_trace_data_queue_tmp = g_queue_new();  

  reg_update_time_window(update_time_window,event_viewer_data, event_viewer_data->mw);
  reg_update_current_time(update_current_time,event_viewer_data, event_viewer_data->mw);
  reg_show_viewer(show_event_detail,event_viewer_data, event_viewer_data->mw);
  reg_update_traceset(traceset_changed,event_viewer_data, event_viewer_data->mw);

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
  gtk_range_set_update_policy (GTK_RANGE(event_viewer_data->vscroll_vc),
			       GTK_UPDATE_DISCONTINUOUS);
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
  get_traceset_time_span(event_viewer_data->mw, &event_viewer_data->time_span);
  
  start = ltt_time_sub(event_viewer_data->time_span.endTime, event_viewer_data->time_span.startTime);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(start) * NANOSECONDS_PER_SECOND;

  event_viewer_data->append = TRUE;

  event_viewer_data->start_event_index = 0;
  event_viewer_data->end_event_index   = event_viewer_data->num_visible_events - 1;  

  /* Set the Selected Event */
  //  tree_v_set_cursor(event_viewer_data);

  event_viewer_data->shown = FALSE;
  event_viewer_data->size  = RESERVE_SMALL_SIZE;
  g_object_set_data(
		    G_OBJECT(event_viewer_data->hbox_v),
		    MAX_NUMBER_EVENT,
		    &event_viewer_data->size);
  
  g_object_set_data(
		    G_OBJECT(event_viewer_data->hbox_v),
		    TRACESET_TIME_SPAN,
		    &event_viewer_data->time_span);

  event_viewer_data->filter_key = g_strdup(key);
  g_object_set_data(
		    G_OBJECT(event_viewer_data->hbox_v),
		    event_viewer_data->filter_key,
		    s);
  
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
		  event_viewer_data->currently_selected_event -= event_viewer_data->num_visible_events-1;
		  
		  //		  gtk_adjustment_set_value(event_viewer_data->vadjust_c,
		  //					   value-(event_viewer_data->num_visible_events-1));
		  //gtk_tree_path_free(path);
		  //path = gtk_tree_path_new_from_indices(0, -1);
		  //gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
		  g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		  
		} else {
		  /* Go to first Event */
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
      set_current_time(event_viewer_data->mw,&ltt_time);
  }else{
    g_warning("Can not get iter\n");
  }
*/
}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  GtkTreePath *tree_path;

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
      
      //      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), tree_path,
      //			       NULL, FALSE);
      gtk_tree_path_free(tree_path);
    }
 
  
}

gint get_cell_height(GtkTreeView *TreeView)
{
  gint height, width;
  GtkTreeViewColumn *column = gtk_tree_view_get_column(TreeView, 0);
  GList *Render_List = gtk_tree_view_column_get_cell_renderers(column);
  GtkCellRenderer *renderer = g_list_first(Render_List)->data;
  
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
			TREE_VIEW_HEADER_HEIGHT (GTK_TREE_VIEW(event_viewer_data->tree_v)) )
    / (double)cell_height ;
  
  event_viewer_data->num_visible_events = ceil(exact_num_visible) ;
  
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
	
  h = cell_height + TREE_VIEW_HEADER_HEIGHT
    (GTK_TREE_VIEW(event_viewer_data->tree_v));
  requisition->height = h;
	
}

gboolean show_event_detail(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTracesetContext * tsc = get_traceset_context(event_viewer_data->mw);

  if(event_viewer_data->shown == FALSE){
    event_viewer_data->shown = TRUE;
    update_raw_data_array(event_viewer_data, 
			  event_viewer_data->raw_trace_data_queue_tmp->length);

    get_test_data(event_viewer_data->vadjust_c->value,
		  event_viewer_data->num_visible_events, 
		  event_viewer_data);

    remove_context_hooks(event_viewer_data,tsc);
  }

  return FALSE;
}

void insert_data_into_model(EventViewerData *event_viewer_data, int start, int end)
{
  int i;
  guint64 real_data;
  RawTraceData * raw_data;
  GList * first;
  GtkTreeIter iter;

  first = event_viewer_data->raw_trace_data_queue->head;
  for(i=start; i<end; i++){
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
  }
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
  GdkWindow * win;
  GdkCursor * new;
  GtkWidget* widget = gtk_widget_get_parent(event_viewer_data->hbox_v);
  
  if(widget){
    new = gdk_cursor_new(GDK_X_CURSOR);
    win = gtk_widget_get_parent_window(widget);  
    gdk_window_set_cursor(win, new);
    gdk_cursor_unref(new);  
    gdk_window_stick(win);
    gdk_window_unstick(win);
  }


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
	    if(!first)break;
	    raw_data = (RawTraceData*)g_list_nth_data(first,0);
	    end = raw_data->time;
	    end.tv_nsec--;
	    ltt_event_position_get(raw_data->ep, &block_num, &event_num, &tf);
	    if(size !=0){
	      if(event_num > minNum){
		backward_num = event_num > RESERVE_SMALL_SIZE 
		              ? event_num - RESERVE_SMALL_SIZE : 1;
		ltt_event_position_set(raw_data->ep, block_num, backward_num);
		ltt_tracefile_seek_position(tf, raw_data->ep);
		ev = ltt_tracefile_read(tf);
		start = ltt_event_time(ev);
		maxNum = RESERVE_SMALL_SIZE_CUBE;
	      }else{
		if(block_num > 1){
		  ltt_event_position_set(raw_data->ep, block_num-1, 1);
		  ltt_tracefile_seek_position(tf, raw_data->ep);
		  ev = ltt_tracefile_read(tf);
		  start = ltt_event_time(ev);	    	      
		}else{
		  start.tv_sec  = 0;
		  start.tv_nsec = 0;		
		}
		maxNum = RESERVE_SMALL_SIZE_CUBE;
	      }
	    }else{
	      if(block_num > count){
		ltt_event_position_set(raw_data->ep, block_num-count, 1);
		ltt_tracefile_seek_position(tf, raw_data->ep);
		ev = ltt_tracefile_read(tf);
		start = ltt_event_time(ev);	    	      
	      }else{
		start.tv_sec  = 0;
		start.tv_nsec = 0;		
	      }	      
	      maxNum = RESERVE_SMALL_SIZE_CUBE;
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
	  if(!first)break;
	  raw_data = (RawTraceData*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
	  start = raw_data->time;
	  start.tv_nsec++;
	  end.tv_sec = G_MAXULONG;
	  end.tv_nsec = G_MAXULONG;
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
	i = event_viewer_data->number_of_events - 1 - list_height;
	if((gint)(event_viewer_data->end_event_index) >= i){
	  int remain_events = event_viewer_data->number_of_events - 1 
	                      -  event_viewer_data->end_event_index;
	  event_viewer_data->append = TRUE;
	  first = event_viewer_data->raw_trace_data_queue->head;
	  if(!first)break;
	  raw_data = (RawTraceData*)g_list_nth_data(first,event_viewer_data->number_of_events - 1);
	  start = raw_data->time;
	  start.tv_nsec++;
	  end.tv_sec = G_MAXULONG;
	  end.tv_nsec = G_MAXULONG;
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
	event_viewer_data->append = TRUE;
	remove_all_items_from_queue(event_viewer_data->raw_trace_data_queue);
	end.tv_sec = G_MAXULONG;
	end.tv_nsec = G_MAXULONG;
	time = ltt_time_from_double(time_value / NANOSECONDS_PER_SECOND);
	start = ltt_time_add(event_viewer_data->time_span.startTime, time);
	event_viewer_data->previous_value = time_value;
	get_events(event_viewer_data, start, end, RESERVE_SMALL_SIZE,&size);
	if(size < list_height && size > 0){
	  event_viewer_data->append = FALSE;
	  first = event_viewer_data->raw_trace_data_queue->head;
	  if(!first)break;
	  raw_data = (RawTraceData*)g_list_nth_data(first,0);
	  end = raw_data->time;
	  end.tv_nsec--;
	  ltt_event_position_get(raw_data->ep, &block_num, &event_num, &tf);
	  
	  if(event_num > list_height - size){
	    backward_num = event_num > RESERVE_SMALL_SIZE 
	      ? event_num - RESERVE_SMALL_SIZE : 1;
	    ltt_event_position_set(raw_data->ep, block_num, backward_num);
	    ltt_tracefile_seek_position(tf, raw_data->ep);
	    ev = ltt_tracefile_read(tf);
	    start = ltt_event_time(ev);
	    maxNum = RESERVE_SMALL_SIZE_CUBE;
	    event_viewer_data->current_event_index = 0;
	    get_events(event_viewer_data, start, end, maxNum, &size);
	    event_viewer_data->start_event_index = event_viewer_data->current_event_index;
	  }
	  event_number = event_viewer_data->raw_trace_data_queue->length - list_height;
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
	event_number = event_viewer_data->current_event_index;
	break;
      default:
	  break;
    }

    if(event_number < 0) event_number = 0;

    //update the value of the scroll bar
    if(direction != SCROLL_NONE && direction != SCROLL_JUMP){
      first = event_viewer_data->raw_trace_data_queue->head;
      if(first){
	raw_data = (RawTraceData*)g_list_nth_data(first,event_number);
	if(!raw_data) raw_data = (RawTraceData*)g_list_nth_data(first,0);       
	time = ltt_time_sub(raw_data->time, event_viewer_data->time_span.startTime);
	event_viewer_data->vadjust_c->value = ltt_time_to_double(time) * NANOSECONDS_PER_SECOND;
	g_signal_stop_emission_by_name(G_OBJECT(event_viewer_data->vadjust_c), "value-changed");
	event_viewer_data->previous_value = event_viewer_data->vadjust_c->value;
      }
    }
    

    event_viewer_data->start_event_index = event_number;
    event_viewer_data->end_event_index = event_number + list_height - 1;    
    if(event_viewer_data->end_event_index > event_viewer_data->number_of_events - 1){
      event_viewer_data->end_event_index = event_viewer_data->number_of_events - 1;
    }

    first = event_viewer_data->raw_trace_data_queue->head;
    gtk_list_store_clear(event_viewer_data->store_m);
    if(!first){
      //      event_viewer_data->previous_value = 0;
      //      event_viewer_data->vadjust_c->value = 0.0;
      //      gtk_widget_hide(event_viewer_data->vscroll_vc);
      goto LAST;
    }else gtk_widget_show(event_viewer_data->vscroll_vc);

    insert_data_into_model(event_viewer_data,event_number, event_number+list_height);
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

 LAST:
  if(widget)
     gdk_window_set_cursor(win, NULL);  

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

    unreg_update_time_window(update_time_window,event_viewer_data, event_viewer_data->mw);
    unreg_update_current_time(update_current_time,event_viewer_data, event_viewer_data->mw);
    unreg_show_viewer(show_event_detail,event_viewer_data, event_viewer_data->mw);
    unreg_update_traceset(traceset_changed,event_viewer_data, event_viewer_data->mw);

    g_event_viewer_data_list = g_slist_remove(g_event_viewer_data_list, event_viewer_data);
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
    g_free(event_viewer_data->filter_key);
    event_viewer_data = NULL;
  }
  
  /* Destroy the Tree View */
  //gtk_widget_destroy(event_viewer_data->tree_v);
  
  /*  Clear raw event list */
  //gtk_list_store_clear(event_viewer_data->store_m);
  //gtk_widget_destroy(GTK_WIDGET(event_viewer_data->store_m));
  
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
      
      g_free (event);
    }
}


int event_selected_hook(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  guint *event_number = (guint*) call_data;
  
  event_viewer_data->currently_selected_event = *event_number;
  event_viewer_data->selected_event = TRUE ;
  
  tree_v_set_cursor(event_viewer_data);

}

/* If every module uses the filter, maybe these two 
 * (add/remove_context_hooks functions) should be put in common place
 */
void add_context_hooks(EventViewerData * event_viewer_data, 
		       LttvTracesetContext * tsc)
{
  gint i, j, k, m,n, nbi, id;
  gint nb_tracefile, nb_control, nb_per_cpu, nb_facility, nb_event;
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

  //if there are hooks for traceset, add them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_trace_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, add them here

    nb_control = ltt_trace_control_tracefile_number(trace);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(trace);
    nb_tracefile = nb_control + nb_per_cpu;
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      
      if(j < nb_control)
	tfc = tc->control_tracefiles[j];
      else
	tfc = tc->per_cpu_tracefiles[j - nb_control];
      
      //if there are hooks for tracefile, add them here
      //      lttv_tracefile_context_add_hooks(tfc, NULL,NULL,NULL,NULL,
      //				       event_viewer_data->before_event_hooks,NULL);

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
						   event_viewer_data->before_event_hooks,
						   NULL);
	  }
	  n++;
	}
      }

    }
  }
  
  //add hooks for process_traceset
  //    context_add_hooks_api(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
  //			  NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);  
}


void remove_context_hooks(EventViewerData * event_viewer_data, 
			  LttvTracesetContext * tsc)
{
  gint i, j, k, m, nbi, n, id;
  gint nb_tracefile, nb_control, nb_per_cpu, nb_facility, nb_event;
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

    nb_control = ltt_trace_control_tracefile_number(trace);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(trace);
    nb_tracefile = nb_control + nb_per_cpu;
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      
      if(j < nb_control)
	tfc = tc->control_tracefiles[j];
      else
	tfc = tc->per_cpu_tracefiles[j - nb_control];
      
      //if there are hooks for tracefile, remove them here
      //      lttv_tracefile_context_remove_hooks(tfc, NULL,NULL,NULL,NULL,
      //					  event_viewer_data->before_event_hooks,NULL);

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
  //    context_remove_hooks_api(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
  //			     NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);
}


gboolean update_time_window(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTracesetContext * tsc = get_traceset_context(event_viewer_data->mw);

  if(event_viewer_data->shown == FALSE){
    event_viewer_data->time_window = *(TimeWindow*)call_data;
    
    add_context_hooks(event_viewer_data, tsc);
  }

  return FALSE;
}

gboolean update_current_time(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  event_viewer_data->current_time = *(LttTime*)call_data;
  guint64 nsec = event_viewer_data->current_time.tv_sec * NANOSECONDS_PER_SECOND 
                  + event_viewer_data->current_time.tv_nsec;
  GtkTreeIter iter;
  guint64 time;
  int count = -1;
  GtkTreeModel* model = (GtkTreeModel*)event_viewer_data->store_m;
  GList * list;
  RawTraceData * data, *data1;
  GtkTreePath* path;
  char str_path[64];
  int i, j;

  //check if the event is shown in the current viewer
  if(gtk_tree_model_get_iter_first(model, &iter)){
    while(1){
      gtk_tree_model_get(model, &iter, TIME_COLUMN, &time, -1);
      if(time < nsec){
	if(!gtk_tree_model_iter_next(model, &iter)){
	  count = -1;
	  break;
	}
	count++;
      }else{
	break;
      }
    }
    //    event_selected_hook(event_viewer_data, &count);
  }

  //the event is not shown in the current viewer
  if(count == -1){
    count = 0;
    //check if the event is in the buffer
    list = event_viewer_data->raw_trace_data_queue->head;
    data = (RawTraceData*)g_list_nth_data(list,0);
    data1 = (RawTraceData*)g_list_nth_data(list,event_viewer_data->raw_trace_data_queue->length-1);

    //the event is in the buffer
    if(ltt_time_compare(data->time, event_viewer_data->current_time)<=0 &&
       ltt_time_compare(data1->time, event_viewer_data->current_time)>=0){
      for(i=0;i<event_viewer_data->raw_trace_data_queue->length;i++){
	data = (RawTraceData*)g_list_nth_data(list,i);
	if(ltt_time_compare(data->time, event_viewer_data->current_time) < 0){
	  count++;
	  continue;
	}
	break;
      }
      if(event_viewer_data->raw_trace_data_queue->length-count < event_viewer_data->num_visible_events){
	j = event_viewer_data->raw_trace_data_queue->length - event_viewer_data->num_visible_events;
	count -= j;
      }else{
	j = count;
	count = 0;
      }
      insert_data_into_model(event_viewer_data,j, j+event_viewer_data->num_visible_events);      
    }else{//the event is not in the buffer
      LttTime start = ltt_time_sub(event_viewer_data->current_time, event_viewer_data->time_span.startTime);
      double position = ltt_time_to_double(start) * NANOSECONDS_PER_SECOND;
      get_test_data(position,
		    event_viewer_data->num_visible_events, 
		    event_viewer_data);      
    }
  }

  sprintf(str_path,"%d\0",count);
  path = gtk_tree_path_new_from_string (str_path);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v), path, NULL, FALSE);
  gtk_tree_path_free(path);  

  return FALSE;
}

gboolean traceset_changed(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttTime start;
  
  remove_all_items_from_queue(event_viewer_data->raw_trace_data_queue);
  gtk_list_store_clear(event_viewer_data->store_m);
  event_viewer_data->shown = FALSE;
  event_viewer_data->append = TRUE;

  get_traceset_time_span(event_viewer_data->mw, &event_viewer_data->time_span);
  start = ltt_time_sub(event_viewer_data->time_span.endTime, event_viewer_data->time_span.startTime);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(start) * NANOSECONDS_PER_SECOND;
  //  event_viewer_data->vadjust_c->value = 0;

  return FALSE;
}


void tree_v_grab_focus(GtkWidget *widget, gpointer data){
  EventViewerData *event_viewer_data = (EventViewerData *)data;
  MainWindow * mw = event_viewer_data->mw;
  set_focused_pane(mw, gtk_widget_get_parent(event_viewer_data->hbox_v));
}

void update_raw_data_array(EventViewerData* event_viewer_data, unsigned size)
{
  RawTraceData * data;
  if(size > 0){
    int pid, tmpPid, i,j,len;
    GList * list, *tmpList;
    GArray * pid_array, * tmp_pid_array;
    
    pid_array     = g_array_sized_new(FALSE, TRUE, sizeof(int), 10);
    tmp_pid_array = g_array_sized_new(FALSE, TRUE, sizeof(int), 10);

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
	for(i= event_viewer_data->raw_trace_data_queue->length-1;i>=0;i--){
	  data = (RawTraceData*)g_list_nth_data(list,i);
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

	for(i=0;i<event_viewer_data->raw_trace_data_queue_tmp->length;i++){
	  data = (RawTraceData*)g_list_nth_data(tmpList,i);
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
	for(i=0;i<event_viewer_data->raw_trace_data_queue->length;i++){
	  data = (RawTraceData*)g_list_nth_data(list,i);
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

	for(i=event_viewer_data->raw_trace_data_queue_tmp->length-1;i>=0;i--){
	  data = (RawTraceData*)g_list_nth_data(tmpList,i);
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
      
      len = pid_array->len > tmp_pid_array->len ? tmp_pid_array->len : pid_array->len;
      for(j=0;j<len;j++){
	pid = g_array_index(pid_array,int, j);
	tmpPid = g_array_index(tmp_pid_array,int,j);
	if(pid == -2)pid = 0;
	if(tmpPid == -2) tmpPid = 0;
	
	if(pid == -1 && tmpPid != -1){
	  for(i=0;i<event_viewer_data->raw_trace_data_queue->length;i++){
	    data = (RawTraceData*)g_list_nth_data(list,i);
	    if(data->pid == -1 && data->cpu_id == j) data->pid = tmpPid;
	  }
	}else if(pid != -1 && tmpPid == -1){
	  for(i=0;i<event_viewer_data->raw_trace_data_queue_tmp->length;i++){
	    data = (RawTraceData*)g_list_nth_data(tmpList,i);
	    if(data->pid == -1 && data->cpu_id == j) data->pid = pid;
	  }
	}
      }
    }

    g_array_free(pid_array,TRUE);
    g_array_free(tmp_pid_array, TRUE);

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

void get_events(EventViewerData* event_viewer_data, LttTime start, 
		LttTime end,unsigned max_num_events, unsigned * real_num_events)
{
  int size;
  LttvTracesetContext * tsc = get_traceset_context(event_viewer_data->mw);

  //  context_add_hooks_api(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
  //			NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);
  add_context_hooks(event_viewer_data,tsc);

  process_traceset_api(event_viewer_data->mw, start, end, max_num_events);

  remove_context_hooks(event_viewer_data,tsc);
  //  context_remove_hooks_api(event_viewer_data->mw, NULL, NULL, NULL, NULL, NULL, NULL,
  //			   NULL, NULL, NULL,event_viewer_data->before_event_hooks,NULL);

  size = event_viewer_data->raw_trace_data_queue_tmp->length;
  *real_num_events = size;
  
  update_raw_data_array(event_viewer_data,size);
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

  tmp_raw_trace_data = g_new(RawTraceData,1);
  tmp_raw_trace_data->cpu_id = ltt_event_cpu_id(e);
  tmp_raw_trace_data->event_name = g_strdup(ltt_eventtype_name(ltt_event_eventtype(e)));
  tmp_raw_trace_data->time = time;
  tmp_raw_trace_data->ep = ltt_event_position_new();

  if(event_viewer_data->raw_trace_data_queue_tmp->length){ 
    list = event_viewer_data->raw_trace_data_queue_tmp->head;
    for(i=event_viewer_data->raw_trace_data_queue_tmp->length-1;i>=0;i--){
      data = (RawTraceData *)g_list_nth_data(list,i);
      if(data->cpu_id == tmp_raw_trace_data->cpu_id){
	prev_raw_trace_data = data;
	break;
      }
    }    
  }  

  if(prev_raw_trace_data) tmp_raw_trace_data->pid = prev_raw_trace_data->pid;
  else tmp_raw_trace_data->pid = -1;

  tmp_raw_trace_data->entry_length = field == NULL ? 0 : ltt_field_size(field);
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
	if(data->cpu_id == tmp_raw_trace_data->cpu_id){
	  data->pid = out;
	}
      }
    }
  }
  
  ltt_event_position(e, tmp_raw_trace_data->ep);

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


