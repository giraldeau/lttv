/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Peter Ho
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
 
 /******************************************************************
   
 The standard deviation  calculation is based on: 
 http://en.wikipedia.org/wiki/Standard_deviation
 
 Standard_deviation  = sqrt(1/N Sum ((xi -Xa)^2))
 
 To compute the standard deviation, we need to make  EventRequests to LTTV. In 
 the first EventRequest, we  compute the average duration (Xa)  and the 
 frequency (N) of each IrqID.  We store the information calculated in the first 
 EventRequest in an array  called  FirstRequestIrqExit.
 In the second  EventRequest, we compute the Sum ((xi -Xa)^2) and store this information 
 in a array called SumArray. The function CalculateStandardDeviation() uses FirstRequestIrqExit 
 and SumArray arrays to calculate the standard deviation.
   
 *******************************************************************/

 

#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
#include <lttvwindow/lttvwindow.h>
#include <ltt/time.h>

#include "hInterruptsInsert.xpm" 

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#define NO_ITEMS 0
  
typedef struct 
{
  LttTime duration;
  LttTime start_time;	
  LttTime end_time;	
}MaxDuration;

typedef struct {
	guint cpu_id;
	guint id;
	guint frequency;
	LttTime total_duration;	
	guint average_duration;
	MaxDuration max_irq_handler;
	
}Irq;

typedef struct {
	guint id;
	guint cpu_id;
	LttTime event_time;
}irq_entry;


typedef struct 
{
	guint irqId;
	guint frequency;
	guint64 sumOfDurations;
	
}SumId;

enum type_t {
   IRQ_ENTRY,
   IRQ_EXIT		
};

/** Array containing instanced objects. Used when module is unloaded */
static GSList *interrupt_data_list = NULL ;
 

#define TRACE_NUMBER 0

typedef struct _InterruptEventData {

  /*Graphical Widgets */ 
  GtkWidget * ScrollWindow;
  GtkListStore *ListStore;
  GtkWidget *Hbox;
  GtkWidget *TreeView;
  GtkTreeSelection *SelectionTree;
  
  Tab 	     * tab; /* tab that contains this plug-in*/ 
  LttvHooks  * event_hooks;
  LttvHooks  * hooks_trace_after;
  LttvHooks  * hooks_trace_before;
  TimeWindow   time_window;
  LttvHooksById * event_by_id_hooks;
  GArray *FirstRequestIrqExit;
  GArray *FirstRequestIrqEntry;
  GArray *SecondRequestIrqEntry;
  GArray *SecondRequestIrqExit;
  GArray *SumArray;
  
} InterruptEventData ;


/* Function prototypes */
 
static gboolean interrupt_update_time_window(void * hook_data, void * call_data);
static GtkWidget *interrupts(Tab *tab);
static InterruptEventData *system_info(Tab *tab);
void interrupt_destructor(InterruptEventData *event_viewer_data);
static void FirstRequest(InterruptEventData *event_data );  
static guint64 get_interrupt_id(LttEvent *e);
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean DisplayViewer (void *hook_data, void *call_data);
static void CalculateData(LttTime time_exit,  guint cpu_id,  InterruptEventData *event_data);
static void TotalDurationMaxIrqDuration(irq_entry *e, LttTime time_exit, GArray *FirstRequestIrqExit);
static gboolean FirstRequestIrqEntryCallback(void *hook_data, void *call_data);
static gboolean FirstRequestIrqExitCallback(void *hook_data, void *call_data);
static gboolean SecondRequest(void *hook_data, void *call_data);
static void CalculateAverageDurationForEachIrqId(InterruptEventData *event_data);
static gboolean SecondRequestIrqEntryCallback(void *hook_data, void *call_data);
static gboolean SecondRequestIrqExitCallback(void *hook_data, void *call_data);
static void CalculateXi(LttEvent *event, InterruptEventData *event_data);
static void  SumItems(gint irq_id, LttTime Xi, InterruptEventData *event_data);
static int CalculateStandardDeviation(gint id, InterruptEventData *event_data);
static int FrequencyInHZ(gint frequency, TimeWindow time_window);

/* Enumeration of the columns */
enum{
  CPUID_COLUMN,
  IRQ_ID_COLUMN,
  FREQUENCY_COLUMN,
  DURATION_COLUMN,
  DURATION_STANDARD_DEV_COLUMN,
  MAX_IRQ_HANDLER_COLUMN,
  N_COLUMNS
};
 
 
 
/**
 *  init function
 *
 * 
 * This is the entry point of the viewer.
 *
 */
static void init() {
  g_info("interrupts: init()");
  lttvwindow_register_constructor("interrupts",
                                  "/",
                                  "Insert  Interrupts View",
                                  hInterruptsInsert_xpm,
                                  "Insert Interrupts View",
                                  interrupts);
   
}


/**
 *  Constructor hook
 *
 */
static GtkWidget *interrupts(Tab * tab)
{

  InterruptEventData* event_data = system_info(tab) ;
  if(event_data)
    return event_data->Hbox;
  else 
    return NULL; 
}

/**
 * This function initializes the Event Viewer functionnality through the
 * GTK  API. 
 */
InterruptEventData *system_info(Tab *tab)
{
  
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  InterruptEventData* event_viewer_data = g_new(InterruptEventData,1) ;
      
  event_viewer_data->tab = tab;
  
  /*Get the current time frame from the main window */
  event_viewer_data->time_window  =  lttvwindow_get_time_window(tab);
  
  event_viewer_data->FirstRequestIrqExit = g_array_new(FALSE, FALSE, sizeof(Irq));
  event_viewer_data->FirstRequestIrqEntry   =  g_array_new(FALSE, FALSE, sizeof(irq_entry));
  
  event_viewer_data->SecondRequestIrqEntry   =  g_array_new(FALSE, FALSE, sizeof(irq_entry));
  event_viewer_data->SecondRequestIrqExit = g_array_new(FALSE, FALSE, sizeof(Irq));
   
  event_viewer_data->SumArray = g_array_new(FALSE, FALSE, sizeof(SumId));
  
  
  /*Create tha main window for the viewer */					 	
  event_viewer_data->ScrollWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (event_viewer_data->ScrollWindow);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(event_viewer_data->ScrollWindow), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC/*GTK_POLICY_NEVER*/);
 
  /* Create a model for storing the data list */
  event_viewer_data->ListStore = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_INT,     /* CPUID                       */
    G_TYPE_INT,     /* IRQ_ID                      */
    G_TYPE_INT,     /* Frequency 		   */
    G_TYPE_UINT64,   /* Duration                   */
    G_TYPE_INT,	    /* standard deviation 	   */
    G_TYPE_STRING	    /* Max IRQ handler  	   */
    );  
 
  event_viewer_data->TreeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->ListStore)); 
   
  g_object_unref (G_OBJECT (event_viewer_data->ListStore));
    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPUID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

   
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("IrqId",
                 renderer,
                 "text", IRQ_ID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column,  220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Frequency (Hz)",
                 renderer,
                 "text", FREQUENCY_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total Duration (nsec)",
                 renderer,
                 "text", DURATION_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 145);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Duration standard deviation  (nsec)",
                 renderer,
                 "text", DURATION_STANDARD_DEV_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 200);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Max IRQ handler duration (nsec) [time interval]",
                 renderer,
                 "text", MAX_IRQ_HANDLER_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 250);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);
  
  
  event_viewer_data->SelectionTree = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->TreeView));
  gtk_tree_selection_set_mode (event_viewer_data->SelectionTree, GTK_SELECTION_SINGLE);
   
  gtk_container_add (GTK_CONTAINER (event_viewer_data->ScrollWindow), event_viewer_data->TreeView);
   
  event_viewer_data->Hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->Hbox), event_viewer_data->ScrollWindow, TRUE, TRUE, 0);
 
  gtk_widget_show(event_viewer_data->Hbox);
  gtk_widget_show(event_viewer_data->TreeView);

  interrupt_data_list = g_slist_append(interrupt_data_list, event_viewer_data);
  /* Registration for time notification */
  lttvwindow_register_time_window_notify(tab,
                                         interrupt_update_time_window,
                                         event_viewer_data);	
					 
  
  FirstRequest(event_viewer_data );
  return event_viewer_data;
}

/**
 * 
 * For each trace in the traceset, this function:
 *  - registers a callback function to each hook
 *  - calls lttv_trace_find_hook() registers a hook function to event_by_id_hooks
 *  - calls lttvwindow_events_request() to request data in a specific 
 *    time interval to the main window
 * 
 */
static void FirstRequest(InterruptEventData *event_data )
{
  guint i, k, l, nb_trace;
 
  LttvTraceHook *hook;
   
  guint ret; 
  
  LttvTraceState *ts;
    
  GArray *hooks;
   
  EventsRequest *events_request;
  
  LttvTraceHookByFacility *thf;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(event_data->tab);
  
  
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
  
  /* There are many traces in a traceset. Iteration for each trace. */  
  for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++)
  {
        events_request = g_new(EventsRequest, 1); 
	
      	hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
      	
	hooks = g_array_set_size(hooks, 2);
    
	event_data->hooks_trace_before = lttv_hooks_new();
	
  	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_before, trace_header, event_data, LTTV_PRIO_DEFAULT);	

  	event_data->hooks_trace_after = lttv_hooks_new();
  	
	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_after,  SecondRequest, event_data, LTTV_PRIO_DEFAULT);
 	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
	/* Create event_by_Id hooks */
  	event_data->event_by_id_hooks = lttv_hooks_by_id_new();
  
 	/*Register event_by_id_hooks with a callback function*/ 
          ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
		LTT_FIELD_IRQ_ID, 0, 0,
		FirstRequestIrqEntryCallback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 0));
	 
	 ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
		LTT_FIELD_IRQ_ID, 0, 0,
		FirstRequestIrqExitCallback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 1));
		
	  g_assert(!ret);
 	 /*iterate through the facility list*/
	for(k = 0 ; k < hooks->len; k++) 
	{ 
	        hook = &g_array_index(hooks, LttvTraceHook, k);
		for(l=0; l<hook->fac_list->len; l++) 
		{
			thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l); 
			lttv_hooks_add(lttv_hooks_by_id_find(event_data->event_by_id_hooks, thf->id),
				thf->h,
				event_data,
				LTTV_PRIO_DEFAULT);
			 
		}
	}
	/* Initalize the EventsRequest structure */
	events_request->owner       = event_data; 
	events_request->viewer_data = event_data; 
	events_request->servicing   = FALSE;     
	events_request->start_time  = event_data->time_window.start_time; 
	events_request->start_position  = NULL;
	events_request->stop_flag	   = FALSE;
	events_request->end_time 	   = event_data->time_window.end_time;
	events_request->num_events  	   = G_MAXUINT;      
	events_request->end_position       = NULL; 
	events_request->trace 	   = i;    
	
	events_request->hooks = hooks;
	
	events_request->before_chunk_traceset = NULL; 
	events_request->before_chunk_trace    = event_data->hooks_trace_before; 
	events_request->before_chunk_tracefile= NULL; 
	events_request->event		        = NULL;  
	events_request->event_by_id		= event_data->event_by_id_hooks; 
	events_request->after_chunk_tracefile = NULL; 
	events_request->after_chunk_trace     = NULL;    
	events_request->after_chunk_traceset	= NULL; 
	events_request->before_request		= NULL; 
	events_request->after_request		= event_data->hooks_trace_after; 
	
	lttvwindow_events_request(event_data->tab, events_request);   
   }
   
}

/**
 *  This function is called whenever an irq_entry event occurs.  
 *  
 */ 
static gboolean FirstRequestIrqEntryCallback(void *hook_data, void *call_data)
{
  
  LttTime  event_time; 
  unsigned cpu_id;
  irq_entry entry;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray* FirstRequestIrqEntry  = event_data->FirstRequestIrqEntry; 
  LttEvent *e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
   
  
  entry.id =get_interrupt_id(e);	  
  entry.cpu_id = cpu_id;
  entry.event_time =  event_time;		
  g_array_append_val (FirstRequestIrqEntry, entry);

  return FALSE;
}

/**
 *  This function gets the id of the interrupt. The id is stored in a dynamic structure. 
 *  Refer to the print.c file for howto extract data from a dynamic structure.
 */ 
static guint64 get_interrupt_id(LttEvent *e)
{
  guint i, num_fields;
  LttEventType *event_type;
  LttField *element;  
  LttField *field;
   guint64  irq_id;
  event_type = ltt_event_eventtype(e);
  num_fields = ltt_eventtype_num_fields(event_type);
  for(i = 0 ; i < num_fields-1 ; i++) 
  {   
        field = ltt_eventtype_field(event_type, i);
  	irq_id = ltt_event_get_long_unsigned(e,field);
  }
  return  irq_id;

} 
/**
 *  This function is called whenever an irq_exit event occurs.  
 *  
 */ 
gboolean FirstRequestIrqExitCallback(void *hook_data, void *call_data)
{
  LttTime  event_time; 
  unsigned cpu_id;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  LttEventType *type = ltt_event_eventtype(e);
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
  
  CalculateData( event_time,  cpu_id, event_data);
   
  return FALSE;
}

/**
 *  This function calculates the duration of an interrupt.  
 *  
 */ 
static void CalculateData(LttTime time_exit,  guint cpu_id,InterruptEventData *event_data)
{
  
  gint i, irq_id;
  irq_entry *element; 
  LttTime duration;
  GArray *FirstRequestIrqExit = event_data->FirstRequestIrqExit;
  GArray *FirstRequestIrqEntry = event_data->FirstRequestIrqEntry;
  for(i = 0; i < FirstRequestIrqEntry->len; i++)
  {
    element = &g_array_index(FirstRequestIrqEntry,irq_entry,i);
    if(element->cpu_id == cpu_id)
    {
      TotalDurationMaxIrqDuration(element,time_exit,  FirstRequestIrqExit);    
      g_array_remove_index(FirstRequestIrqEntry, i);
      break;
    }
  }
} 

/**
 *  This function calculates the total duration  of an interrupt and the longest Irq handler.  
 *  
 */ 
static void TotalDurationMaxIrqDuration(irq_entry *e, LttTime time_exit, GArray *FirstRequestIrqExit){
  Irq irq;
  Irq *element; 
  guint i;
  LttTime duration;
  gboolean  notFound = FALSE;
  memset ((void*)&irq, 0,sizeof(Irq));
  
  /*first time*/
  if(FirstRequestIrqExit->len == NO_ITEMS)
  {
    irq.cpu_id = e->cpu_id;
    irq.id    =  e->id;
    irq.frequency++;
    irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
     
    irq.max_irq_handler.start_time = e->event_time;
    irq.max_irq_handler.end_time = time_exit;
    irq.max_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
     
    
    g_array_append_val (FirstRequestIrqExit, irq);
  }
  else
  {
    for(i = 0; i < FirstRequestIrqExit->len; i++)
    {
      element = &g_array_index(FirstRequestIrqExit,Irq,i);
      if(element->id == e->id)
      {
	notFound = TRUE;
	duration =  ltt_time_sub(time_exit, e->event_time);
	element->total_duration = ltt_time_add(element->total_duration, duration);
	element->frequency++;
	if(ltt_time_compare(duration,element->max_irq_handler.duration) > 0)
	{
	    element->max_irq_handler.duration = duration;
	    element->max_irq_handler.start_time = e->event_time;
	    element->max_irq_handler.end_time  = time_exit;
	}
      }
    }
    if(!notFound)
    {
      irq.cpu_id = e->cpu_id;
      irq.id    =  e->id;
      irq.frequency++;
      irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
      
      irq.max_irq_handler.start_time = e->event_time;
      irq.max_irq_handler.end_time = time_exit;
      irq.max_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
      
      g_array_append_val (FirstRequestIrqExit, irq);
    }
  } 
}

/**
 *  This function  passes the second EventsRequest to LTTV
 *  
 */ 
static gboolean SecondRequest(void *hook_data, void *call_data)
{
 
  guint i, k, l, nb_trace;
 
  LttvTraceHook *hook;
   
  guint ret; 
  
  LttvTraceState *ts;
    
  GArray *hooks;
   
  EventsRequest *events_request;
  
  LttvTraceHookByFacility *thf;
  
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(event_data->tab);
  
  CalculateAverageDurationForEachIrqId(event_data);
   
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
  
  /* There are many traces in a traceset. Iteration for each trace. */  
  for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++)
  {
        events_request = g_new(EventsRequest, 1); 
	
      	hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
      	
	hooks = g_array_set_size(hooks, 2);
    
	event_data->hooks_trace_after = lttv_hooks_new();
  	
	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_after, DisplayViewer, event_data, LTTV_PRIO_DEFAULT);
	
  	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
	/* Create event_by_Id hooks */
  	event_data->event_by_id_hooks = lttv_hooks_by_id_new();
  
 	/*Register event_by_id_hooks with a callback function*/ 
          ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
		LTT_FIELD_IRQ_ID, 0, 0,
		SecondRequestIrqEntryCallback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 0));
	 
	 ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
		LTT_FIELD_IRQ_ID, 0, 0,
		SecondRequestIrqExitCallback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 1));
		
	  g_assert(!ret);
	  
 	/* iterate through the facility list */
	for(k = 0 ; k < hooks->len; k++) 
	{ 
	        hook = &g_array_index(hooks, LttvTraceHook, k);
		for(l=0; l<hook->fac_list->len; l++) 
		{
			thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l); 
			lttv_hooks_add(lttv_hooks_by_id_find(event_data->event_by_id_hooks, thf->id),
				thf->h,
				event_data,
				LTTV_PRIO_DEFAULT);
			 
		}
	}
	/* Initalize the EventsRequest structure */
	events_request->owner       = event_data; 
	events_request->viewer_data = event_data; 
	events_request->servicing   = FALSE;     
	events_request->start_time  = event_data->time_window.start_time; 
	events_request->start_position  = NULL;
	events_request->stop_flag	   = FALSE;
	events_request->end_time 	   = event_data->time_window.end_time;
	events_request->num_events  	   = G_MAXUINT;      
	events_request->end_position       = NULL; 
	events_request->trace 	   = i;    
	
	events_request->hooks = hooks;
	
	events_request->before_chunk_traceset = NULL; 
	events_request->before_chunk_trace    = NULL; 
	events_request->before_chunk_tracefile= NULL; 
	events_request->event		        = NULL;  
	events_request->event_by_id		= event_data->event_by_id_hooks; 
	events_request->after_chunk_tracefile = NULL; 
	events_request->after_chunk_trace     = NULL;    
	events_request->after_chunk_traceset	= NULL; 
	events_request->before_request		= NULL; 
	events_request->after_request		= event_data->hooks_trace_after; 
	
	lttvwindow_events_request(event_data->tab, events_request);   
   }
   return FALSE;
}

static void CalculateAverageDurationForEachIrqId(InterruptEventData *event_data)
{
  guint64 real_data;
  Irq *element; 
  gint i;
  GArray* FirstRequestIrqExit  = event_data->FirstRequestIrqExit; 
  for(i = 0; i < FirstRequestIrqExit->len; i++)
  {  
    element = &g_array_index(FirstRequestIrqExit,Irq,i);  
    real_data = element->total_duration.tv_sec;
    real_data *= NANOSECONDS_PER_SECOND;
    real_data += element->total_duration.tv_nsec;
    element->average_duration = real_data / element->frequency;
  }

}

/**
 *  This function is called whenever an irq_entry event occurs.  Use in the second request
 *  
 */ 
static gboolean SecondRequestIrqEntryCallback(void *hook_data, void *call_data)
{

  LttTime  event_time; 
  unsigned cpu_id;
  irq_entry entry;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray* SecondRequestIrqEntry  = event_data->SecondRequestIrqEntry; 
  LttEvent *e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
   
  
  entry.id =get_interrupt_id(e);	  
  entry.cpu_id = cpu_id;
  entry.event_time =  event_time;		
  g_array_append_val (SecondRequestIrqEntry, entry);
 
  return FALSE;
}

/**
 *  This function is called whenever an irq_exit event occurs in the second request. 
 *  
 */ 
static gboolean SecondRequestIrqExitCallback(void *hook_data, void *call_data)
{
   
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttEvent *event = ltt_tracefile_get_event(tfc->tf);
  
  CalculateXi(event, event_data);
  return FALSE;
} 


/**
 *  This function is called whenever an irq_exit event occurs in the second request.  
 *  
 */ 
static void CalculateXi(LttEvent *event_irq_exit, InterruptEventData *event_data)
{
  gint i, irq_id;
  irq_entry *element; 
  LttTime Xi;
  LttTime  exit_time; 
  unsigned cpu_id;
  
  GArray *SecondRequestIrqExit = event_data->SecondRequestIrqExit;
  GArray *SecondRequestIrqEntry = event_data->SecondRequestIrqEntry;
  cpu_id = ltt_event_cpu_id(event_irq_exit);
  for(i = 0; i < SecondRequestIrqEntry->len; i++)
  {
    element = &g_array_index(SecondRequestIrqEntry,irq_entry,i);
    if(element->cpu_id == cpu_id)
    {
     
      /* time calculation */	
      exit_time = ltt_event_time(event_irq_exit);
      Xi   =  ltt_time_sub(exit_time, element->event_time);
      irq_id    =  element->id;
         
      SumItems(irq_id, Xi,event_data);
      g_array_remove_index(SecondRequestIrqEntry, i);
      break;
    }
  }
}


/**
 *  This function computes the Sum ((xi -Xa)^2) and store the result in SumArray
 *  
 */ 
static void  SumItems(gint irq_id, LttTime Xi, InterruptEventData *event_data)
{
  gint i;
  guint time_in_ns;
   
  gint temp;
  Irq *average; 
  SumId *sumItem; 
  SumId sum;
  gboolean  notFound = FALSE;
  GArray *FirstRequestIrqExit = event_data->FirstRequestIrqExit;
  GArray *SumArray = event_data->SumArray;
  time_in_ns  = Xi.tv_sec;
  time_in_ns *= NANOSECONDS_PER_SECOND;
  time_in_ns += Xi.tv_nsec;
    
  for(i = 0; i < FirstRequestIrqExit->len; i++)
  {
  	average = &g_array_index(FirstRequestIrqExit,Irq,i);
	if(irq_id == average->id)
	{
	    temp = time_in_ns - average->average_duration;
	    sum.sumOfDurations =  pow (temp , 2);
	    //printf("one : %d\n", sum.sumOfDurations);	    
	    sum.irqId = irq_id;
	    sum.frequency = average->frequency;
	    if(event_data->SumArray->len == NO_ITEMS)		 
	    {   
	     	g_array_append_val (SumArray, sum);
	    }
	    else
 	    {
	        
	        for(i = 0; i < SumArray->len; i++)
    		{
      		  sumItem = &g_array_index(SumArray, SumId, i);
      		  if(sumItem->irqId == irq_id)
      		  { 
		     notFound = TRUE;
		     sumItem->sumOfDurations  += sum.sumOfDurations;
 		     
      		  }
    		}
		if(!notFound)
    		{
   		   g_array_append_val (SumArray, sum);
		}
    
	   
	    }
         
	}
  } 	
}

/**
 *  This function displays the result on the viewer 
 *  
 */ 
static gboolean DisplayViewer(void *hook_data, void *call_data)
{
  
  guint average;
  gint i;	
  Irq element; 
  LttTime average_duration;
  GtkTreeIter    iter;
  guint64 real_data;
  guint maxIRQduration;
  char maxIrqHandler[80];
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray *FirstRequestIrqExit = event_data->FirstRequestIrqExit;  
  gtk_list_store_clear(event_data->ListStore);
  for(i = 0; i < FirstRequestIrqExit->len; i++)
  {  
    element = g_array_index(FirstRequestIrqExit,Irq,i);  
    real_data = element.total_duration.tv_sec;
    real_data *= NANOSECONDS_PER_SECOND;
    real_data += element.total_duration.tv_nsec;
    
    
    maxIRQduration  = element.max_irq_handler.duration.tv_sec;
    maxIRQduration *= NANOSECONDS_PER_SECOND;
    maxIRQduration += element.max_irq_handler.duration.tv_nsec;
    
    sprintf(maxIrqHandler, "%d [%d.%d - %d.%d]",maxIRQduration, element.max_irq_handler.start_time.tv_sec, \ 			 
   			    element.max_irq_handler.start_time.tv_nsec, element.max_irq_handler.end_time.tv_sec, \ 
    			    element.max_irq_handler.end_time.tv_nsec) ;
    
     
    
    gtk_list_store_append (event_data->ListStore, &iter);
    gtk_list_store_set (event_data->ListStore, &iter,
      CPUID_COLUMN, element.cpu_id,
      IRQ_ID_COLUMN,  element.id,
      FREQUENCY_COLUMN, FrequencyInHZ(element.frequency,event_data->time_window),
      DURATION_COLUMN, real_data,
      DURATION_STANDARD_DEV_COLUMN, CalculateStandardDeviation(element.id, event_data),
      MAX_IRQ_HANDLER_COLUMN, maxIrqHandler,
      -1);
     
     
  } 
   
   
  if(event_data->FirstRequestIrqExit->len)
  {
     g_array_remove_range (event_data->FirstRequestIrqExit,0,event_data->FirstRequestIrqExit->len);
  }
  
  if(event_data->FirstRequestIrqEntry->len)
  {
    g_array_remove_range (event_data->FirstRequestIrqEntry,0,event_data->FirstRequestIrqEntry->len);
  }
   
  if(event_data->SecondRequestIrqEntry->len)
  {
    g_array_remove_range (event_data->SecondRequestIrqEntry,0,event_data->SecondRequestIrqEntry->len);
  }
  
  if(event_data->SecondRequestIrqExit->len)
  {
    g_array_remove_range (event_data->SecondRequestIrqExit,0, event_data->SecondRequestIrqExit->len);
  } 
    
  if(event_data->SumArray->len)
  {
    g_array_remove_range (event_data->SumArray,0, event_data->SumArray->len);
  }
    
  return FALSE;
}


static int FrequencyInHZ(gint frequency, TimeWindow time_window)
{
  guint64 frequencyHz = 0;
  double timeSec; 
  guint64 time = time_window.time_width.tv_sec;
  time      *= NANOSECONDS_PER_SECOND;
  time      += time_window.time_width.tv_nsec; 
  timeSec = time/NANOSECONDS_PER_SECOND;
  frequencyHz = frequency/timeSec;
  return  frequencyHz;
}

/**
 *  This function calculatees the standard deviation
 *  
 */ 
static int CalculateStandardDeviation(gint id, InterruptEventData *event_data)
{
  int i;
  SumId sumId;
  double inner_component;
  int deviation = 0;
  for(i = 0; i < event_data->SumArray->len; i++)
  {  
    sumId  = g_array_index(event_data->SumArray, SumId, i);  
    if(id == sumId.irqId)
    {
  	inner_component = sumId.sumOfDurations/ sumId.frequency;
	deviation =  sqrt(inner_component);
  	return deviation;
    }    
  }
  return deviation; 
}
/*
 * This function is called by the main window
 * when the time interval needs to be updated.
 **/ 
gboolean interrupt_update_time_window(void * hook_data, void * call_data)
{
  InterruptEventData *event_data = (InterruptEventData *) hook_data;
  const TimeWindowNotifyData *time_window_nofify_data =  ((const TimeWindowNotifyData *)call_data);
  event_data->time_window = *time_window_nofify_data->new_time_window;
  g_info("interrupts: interrupt_update_time_window()\n");
  Tab *tab = event_data->tab;
  lttvwindow_events_request_remove_all(tab, event_data);
  FirstRequest(event_data );
  return FALSE;
}


gboolean trace_header(void *hook_data, void *call_data)
{

  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttEvent *e;
  LttTime event_time;
  return FALSE;
}

void interrupt_destroy_walk(gpointer data, gpointer user_data)
{
  g_info("interrupt_destroy_walk");
  interrupt_destructor((InterruptEventData*)data);

}

void interrupt_destructor(InterruptEventData *event_viewer_data)
{
  /* May already been done by GTK window closing */
  g_info("enter interrupt_destructor \n");
  if(GTK_IS_WIDGET(event_viewer_data->Hbox))
  {
    gtk_widget_destroy(event_viewer_data->Hbox);
  }
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() 
{
    
  g_info("Destroy  interrupts");
  g_slist_foreach(interrupt_data_list, interrupt_destroy_walk, NULL );
  g_slist_free(interrupt_data_list); 
  lttvwindow_unregister_constructor(interrupts);
  
}

LTTV_MODULE("interrupts", "interrupts info view", \
    "Graphical module to display interrupts performance", \
	    init, destroy, "lttvwindow")
