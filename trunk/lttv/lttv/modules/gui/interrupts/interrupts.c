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

Each field of the interrupt viewer is summarized as follows:
   
- CPUID: processor ID

- IrqId: IRQ ID

- Frequency (Hz): the number of interrupts per second (Hz). 
                  We compute the total number of interrupts. Then 
		  we divide it by the time interval.

- Total Duration (nsec): the sum of each interrupt duration in nsec. 
	         For a given Irq ID, we sum the duration of each interrupt
		 to give us the total duration 

- Duration Standard_deviation  = sqrt(1/N Sum ((xi -Xa)^2)) where
	N: number of interrupts 
	xi: duration of an interrupt (nsec)
	Xa: average duration (nsec)
  The formula is taken from wikipedia: http://en.wikipedia.org/wiki/Standard_deviation.	
  To calculate the duration standard deviation, we make two EventsRequest passes to the main window.
  In the first EventsRequest pass, we calculate the total number of interrupts to compute for 
  the average Xa. In the second  EventsRequest pass, calculate the standard deviation.
  

- Max IRQ handler duration (nsec) [time interval]:   the longest IRQ handler duration in nsec.  

- Min IRQ handler duration (nsec) [time interval]:   the shortest IRQ handler duration in nsec.  

- Average period (nsec): 1/Frequency(in HZ). The frequency is computed above.

-Period Standard_deviation  = sqrt(1/N Sum ((xi -Xa)^2)) where
N: number of interrupts 
xi: duration of an interrupt
Xa: Period = 1/Frequency  (in Hz)
 
-Frequency Standard_deviation  = sqrt(1/N Sum ((xi -Xa)^2)) 
N:  number of interrupts 
xi: duration of an interrupt
Xa: Frequency  (Hz)

  
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
#include <ltt/trace.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/filter.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttv_plugin_tab.h>
#include <ltt/time.h>

#include "hInterruptsInsert.xpm" 

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define NO_ITEMS 0
  
typedef struct 
{
  LttTime duration;
  LttTime start_time;	
  LttTime end_time;	
}IrqDuration;

typedef struct {
	guint cpu_id;
	guint id;
	guint TotalNumberOfInterrupts;
	LttTime total_duration;	
	guint average_duration;
	IrqDuration max_irq_handler;
	IrqDuration min_irq_handler;
}Irq;

typedef struct {
	guint id;
	guint cpu_id;
	LttTime event_time;
}irq_entry;


typedef struct 
{
	guint irqId;
	guint TotalNumberOfInterrupts;//frequency;// 
	guint64 sumOfDurations; // to store the Sum ((xi -Xa)^2) of the duration Standard deviation
	guint64 sumOfPeriods;   // to store  the Sum ((xi -Xa)^2) of the period Standard deviation
	guint64 sumOfFrequencies;// to store the Sum ((xi -Xa)^2) of the frequency Standard deviation
	
}SumId;

enum type_t {
   IRQ_ENTRY,
   IRQ_EXIT		
};

/** Array containing instanced objects. Used when module is unloaded */
static GSList *interrupt_data_list = NULL ;
 

//fixed #define TRACE_NUMBER 0

typedef struct _InterruptEventData {

  /*Graphical Widgets */ 
  GtkWidget * ScrollWindow;
  GtkListStore *ListStore;
  GtkWidget *Hbox;
  GtkWidget *TreeView;
  GtkTreeSelection *SelectionTree;
  
  Tab 	     * tab; /* tab that contains this plug-in*/ 
  LttvPluginTab *ptab;
  LttvHooks  * event_hooks;
  LttvHooks  * hooks_trace_after;
  LttvHooks  * hooks_trace_before;
  TimeWindow   time_window;
  LttvHooksByIdChannelArray * event_by_id_hooks;
  GArray *FirstRequestIrqExit;
  GArray *FirstRequestIrqEntry;
  GArray *SecondRequestIrqEntry;
  GArray *SecondRequestIrqExit;
  GArray *SumArray;
  
} InterruptEventData ;


/* Function prototypes */
 
static gboolean interrupt_update_time_window(void * hook_data, void * call_data);
static GtkWidget *interrupts(LttvPlugin *plugin);
static InterruptEventData *system_info(LttvPluginTab *ptab);
void interrupt_destructor(InterruptEventData *event_viewer_data);
static void FirstRequest(InterruptEventData *event_data );  
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean DisplayViewer (void *hook_data, void *call_data);
static void CalculateData(LttTime time_exit,  guint cpu_id,  InterruptEventData *event_data);
static void CalculateTotalDurationAndMaxIrqDurationAndMinIrqDuration(irq_entry *e, LttTime time_exit, GArray *FirstRequestIrqExit);
static gboolean FirstRequestIrqEntryCallback(void *hook_data, void *call_data);
static gboolean FirstRequestIrqExitCallback(void *hook_data, void *call_data);
static gboolean SecondRequest(void *hook_data, void *call_data);
static void CalculateAverageDurationForEachIrqId(InterruptEventData *event_data);
static gboolean SecondRequestIrqEntryCallback(void *hook_data, void *call_data);
static gboolean SecondRequestIrqExitCallback(void *hook_data, void *call_data);
static void CalculateXi(LttEvent *event, InterruptEventData *event_data, guint cpu_id);
static void  SumItems(gint irq_id, LttTime Xi, InterruptEventData *event_data);
static int CalculateDurationStandardDeviation(gint id, InterruptEventData *event_data);
static int CalculatePeriodStandardDeviation(gint id, InterruptEventData *event_data);
static int FrequencyInHZ(gint NumberOfInterruptions, TimeWindow time_window);
static  guint64 CalculatePeriodInnerPart(guint Xi, guint FrequencyHZ);
static  guint64 CalculateFrequencyInnerPart(guint Xi_in_ns,  guint FrequencyHZ); 
static void InterruptFree(InterruptEventData *event_viewer_data);
static int CalculateFrequencyStandardDeviation(gint id, InterruptEventData *event_data);

/* Enumeration of the columns */
enum{
  CPUID_COLUMN,
  IRQ_ID_COLUMN,
  FREQUENCY_COLUMN,
  DURATION_COLUMN,
  DURATION_STANDARD_DEV_COLUMN,
  MAX_IRQ_HANDLER_COLUMN,
  MIN_IRQ_HANDLER_COLUMN,
  AVERAGE_PERIOD,
  PERIOD_STANDARD_DEV_COLUMN,
  FREQUENCY_STANDARD_DEV_COLUMN, 
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
static GtkWidget *interrupts(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  InterruptEventData* event_data = system_info(ptab) ;
  if(event_data)
    return event_data->Hbox;
  else 
    return NULL; 
}

/**
 * This function initializes the Event Viewer functionnality through the
 * GTK  API. 
 */
InterruptEventData *system_info(LttvPluginTab *ptab)
{
  
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  InterruptEventData* event_viewer_data = g_new(InterruptEventData,1) ;
  Tab *tab = ptab->tab;   
  event_viewer_data->ptab = ptab;
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
    G_TYPE_STRING,	    /* Max IRQ handler  	   */
    G_TYPE_STRING,	    /* Min IRQ handler  	   */
    G_TYPE_INT,		    /* Average period 		   */
    G_TYPE_INT, 	    /* period standard deviation   */
    G_TYPE_INT 	    	    /* frequency standard deviation   */
    
    );  
 
  event_viewer_data->TreeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->ListStore)); 
   
  g_object_unref (G_OBJECT (event_viewer_data->ListStore));
    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPU ID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

   
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("IRQ ID",
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

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Min IRQ handler duration (nsec) [time interval]",
                 renderer,
                 "text", MIN_IRQ_HANDLER_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 250);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);
    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (" Average period (nsec)",
                 renderer,
                 "text", AVERAGE_PERIOD,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 200);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Period standard deviation (nsec)",
                 renderer,
                 "text", PERIOD_STANDARD_DEV_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 200);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Frequency standard deviation (Hz)",
                 renderer,
                 "text", FREQUENCY_STANDARD_DEV_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 200);
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
					 
  g_object_set_data_full(G_OBJECT(event_viewer_data->Hbox),
      "event_data",
       event_viewer_data,
      (GDestroyNotify) InterruptFree);  
  
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
  
  LttvTraceHook *th;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(event_data->tab);
  
  
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
  
  /* There are many traces in a traceset. Iteration for each trace. */  
  //for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++) {
  for(i = 0 ; i < nb_trace ; i++) {
        events_request = g_new(EventsRequest, 1); 
	
      	hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 2);
    
	event_data->hooks_trace_before = lttv_hooks_new();
	
  	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_before, trace_header, event_data, LTTV_PRIO_DEFAULT);	

  	event_data->hooks_trace_after = lttv_hooks_new();
  	
	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_after,  SecondRequest, event_data, LTTV_PRIO_DEFAULT);
 	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
	/* Create event_by_Id hooks */
 	event_data->event_by_id_hooks = lttv_hooks_by_id_channel_new();
  
 	/*Register event_by_id_hooks with a callback function*/ 
        lttv_trace_find_hook(ts->parent.t,
		LTT_CHANNEL_KERNEL,
                LTT_EVENT_IRQ_ENTRY,
		FIELD_ARRAY(LTT_FIELD_IRQ_ID),
		FirstRequestIrqEntryCallback,
		events_request,
		&hooks);
	 
	lttv_trace_find_hook(ts->parent.t,
		LTT_CHANNEL_KERNEL,
                LTT_EVENT_IRQ_EXIT,
		NULL,
		FirstRequestIrqExitCallback,
		events_request,
		&hooks);
		
 	 /*iterate through the facility list*/
	for(k = 0 ; k < hooks->len; k++) 
	{ 
		th = &g_array_index(hooks, LttvTraceHook, k); 
		lttv_hooks_add(lttv_hooks_by_id_channel_find(
			                       event_data->event_by_id_hooks,
		                               th->channel, th->id),
			th->h,
			th,
			LTTV_PRIO_DEFAULT);
			 
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
	events_request->event_by_id_channel     = event_data->event_by_id_hooks; 
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
  LttvTraceHook *th = (LttvTraceHook*) hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  InterruptEventData *event_data = events_request->viewer_data;
  GArray* FirstRequestIrqEntry  = event_data->FirstRequestIrqEntry; 
  LttEvent *e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = tfs->cpu;
   
  
  entry.id = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));	  
  entry.cpu_id = cpu_id;
  entry.event_time =  event_time;		
  g_array_append_val (FirstRequestIrqEntry, entry);

  return FALSE;
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
  LttvTraceHook *th = (LttvTraceHook*) hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  InterruptEventData *event_data = events_request->viewer_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  event_time = ltt_event_time(e);
  cpu_id = tfs->cpu;
  
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
  for(i = FirstRequestIrqEntry->len-1; i >=0; i--)
  {
    element = &g_array_index(FirstRequestIrqEntry,irq_entry,i);
    if(element->cpu_id == cpu_id)
    {
      CalculateTotalDurationAndMaxIrqDurationAndMinIrqDuration(element,time_exit,  FirstRequestIrqExit);    
      g_array_remove_index(FirstRequestIrqEntry, i);
      break;
    }
  }
} 
 

/**
 *  This function calculates the total duration  of an interrupt and the longest & shortest Irq handlers.  
 *  
 */ 
static void CalculateTotalDurationAndMaxIrqDurationAndMinIrqDuration(irq_entry *e, LttTime time_exit, GArray *FirstRequestIrqExit)
{
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
    irq.TotalNumberOfInterrupts++;
    irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
     
    irq.max_irq_handler.start_time = e->event_time;
    irq.max_irq_handler.end_time = time_exit;
    irq.max_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
    
    irq.min_irq_handler.start_time = e->event_time;
    irq.min_irq_handler.end_time = time_exit;
    irq.min_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
     
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
	element->TotalNumberOfInterrupts++;
	// Max irq handler
	if(ltt_time_compare(duration,element->max_irq_handler.duration) > 0)
	{
	    element->max_irq_handler.duration = duration;
	    element->max_irq_handler.start_time = e->event_time;
	    element->max_irq_handler.end_time  = time_exit;
	}
	// Min irq handler
	if(ltt_time_compare(duration,element->min_irq_handler.duration) < 0)
	{
	    element->min_irq_handler.duration = duration;
	    element->min_irq_handler.start_time = e->event_time;
	    element->min_irq_handler.end_time  = time_exit;
	}
      }
    }
    if(!notFound)
    {
      irq.cpu_id = e->cpu_id;
      irq.id    =  e->id;
      irq.TotalNumberOfInterrupts++;
      irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
      // Max irq handler
      irq.max_irq_handler.start_time = e->event_time;
      irq.max_irq_handler.end_time = time_exit;
      irq.max_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
      // Min irq handler
      irq.min_irq_handler.start_time = e->event_time;
      irq.min_irq_handler.end_time = time_exit;
      irq.min_irq_handler.duration = ltt_time_sub(time_exit, e->event_time);
      
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
  
  LttvTraceHook *th;
  
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(event_data->tab);
  
  CalculateAverageDurationForEachIrqId(event_data);
   
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
  
  /* There are many traces in a traceset. Iteration for each trace. */  
  for(i = 0 ; i < nb_trace ; i++) {
  // fixed for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++) {
        events_request = g_new(EventsRequest, 1); 
	
      	hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 2);
      	
	event_data->hooks_trace_after = lttv_hooks_new();
  	
	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_after, DisplayViewer, event_data, LTTV_PRIO_DEFAULT);
	
  	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
	/* Create event_by_Id hooks */
  	event_data->event_by_id_hooks = lttv_hooks_by_id_channel_new();
  
 	/*Register event_by_id_hooks with a callback function*/ 
          ret = lttv_trace_find_hook(ts->parent.t,
		LTT_CHANNEL_KERNEL,
		LTT_EVENT_IRQ_ENTRY,
		FIELD_ARRAY(LTT_FIELD_IRQ_ID),
		SecondRequestIrqEntryCallback,
		events_request,
		&hooks);
	 
	 ret = lttv_trace_find_hook(ts->parent.t,
		LTT_CHANNEL_KERNEL,
		LTT_EVENT_IRQ_EXIT,
		NULL,
		SecondRequestIrqExitCallback,
		events_request,
		&hooks);
		
	  g_assert(!ret);
	  
 	/* iterate through the facility list */
	for(k = 0 ; k < hooks->len; k++) 
	{ 
		th = &g_array_index(hooks, LttvTraceHook, k); 
		lttv_hooks_add(lttv_hooks_by_id_channel_find(
			    event_data->event_by_id_hooks,
			    th->channel, th->id),
			th->h,
			th,
			LTTV_PRIO_DEFAULT);
			 
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
	events_request->event_by_id_channel	= event_data->event_by_id_hooks; 
	events_request->after_chunk_tracefile = NULL; 
	events_request->after_chunk_trace     = NULL;    
	events_request->after_chunk_traceset	= NULL; 
	events_request->before_request		= NULL; 
	events_request->after_request		= event_data->hooks_trace_after; 
	
	lttvwindow_events_request(event_data->tab, events_request);   
   }
   return FALSE;
}

/**
 *  This function calculates the average  duration for each Irq Id
 *  
 */ 
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
    if(element->TotalNumberOfInterrupts != 0)
       element->average_duration = real_data / element->TotalNumberOfInterrupts;
    else
       element->average_duration = 0;
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
  LttvTraceHook *th = (LttvTraceHook *)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  InterruptEventData *event_data = events_request->viewer_data;
  GArray* SecondRequestIrqEntry  = event_data->SecondRequestIrqEntry; 
  LttEvent *e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = tfs->cpu;
   
  
  entry.id = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));	  
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
  LttvTraceHook *th = (LttvTraceHook *)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  InterruptEventData *event_data = events_request->viewer_data;
  
  LttEvent *event = ltt_tracefile_get_event(tfc->tf);
  
  CalculateXi(event, event_data, tfs->cpu);
  return FALSE;
} 


/**
 *  This function is called whenever an irq_exit event occurs in the second request.  
 *  
 */ 
static void CalculateXi(LttEvent *event_irq_exit, InterruptEventData *event_data, guint cpu_id)
{
  gint i, irq_id;
  irq_entry *element; 
  LttTime Xi;
  LttTime  exit_time; 
  
  GArray *SecondRequestIrqExit = event_data->SecondRequestIrqExit;
  GArray *SecondRequestIrqEntry = event_data->SecondRequestIrqEntry;
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
  guint Xi_in_ns;
   
  gint duration_inner_part;
  guint64 period_inner_part;
  guint64 frequency_inner_part;
  
  Irq *average; 
  SumId *sumItem; 
  SumId sum;
  int FrequencyHZ =  0; 
  gboolean  notFound = FALSE;
  GArray *FirstRequestIrqExit = event_data->FirstRequestIrqExit;
  GArray *SumArray = event_data->SumArray;
  Xi_in_ns  = Xi.tv_sec;
  Xi_in_ns *= NANOSECONDS_PER_SECOND;
  Xi_in_ns += Xi.tv_nsec;
    
  for(i = 0; i < FirstRequestIrqExit->len; i++)
  {
  	average = &g_array_index(FirstRequestIrqExit,Irq,i);
	if(irq_id == average->id)
	{
	    duration_inner_part = Xi_in_ns - average->average_duration;
	    FrequencyHZ = FrequencyInHZ(average->TotalNumberOfInterrupts, event_data->time_window);
	    sum.irqId = irq_id;
	    // compute  (xi -Xa)^2 of the duration Standard deviation
	    sum.TotalNumberOfInterrupts = average->TotalNumberOfInterrupts;
	    sum.sumOfDurations =  pow (duration_inner_part , 2);
	     
	    // compute  (xi -Xa)^2 of the period Standard deviation
	    period_inner_part = CalculatePeriodInnerPart(Xi_in_ns, FrequencyHZ); 
	    
	    // compute (xi -Xa)^2 of the frequency Standard deviation
	    frequency_inner_part =  CalculateFrequencyInnerPart(Xi_in_ns, FrequencyHZ); 
	    
	    sum.sumOfPeriods = period_inner_part;
	    
	    sum.sumOfFrequencies = frequency_inner_part;
	    
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
		     sumItem->sumOfPeriods += sum.sumOfPeriods;
 		     sumItem->sumOfFrequencies += sum.sumOfFrequencies;
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
 *  This function computes the inner part of the period standard deviation  = sqrt(1/N Sum ((xi -Xa)^2))  
 *  The inner part is: (xi -Xa)^2
 */  
static  guint64 CalculatePeriodInnerPart(guint Xi, guint FrequencyHZ)
{

  double periodInSec; /*period  in sec*/
  int periodInNSec;
  gint difference;
  guint64 result;
  periodInSec = (double)1/FrequencyHZ;
  periodInSec *= NANOSECONDS_PER_SECOND;
  periodInNSec = (int)periodInSec; 
  
  difference = Xi - periodInNSec;
  result = pow (difference , 2);
  return result; 
}

/**
 *  This function computes the inner part of the frequency standard deviation  = sqrt(1/N Sum ((xi -Xa)^2))  
 *  The inner part is: (xi -Xa)^2
 */  
static  guint64 CalculateFrequencyInnerPart(guint Xi_in_ns,  guint FrequencyHZ)
{
  guint64 result;
  gint difference;
  
  difference = Xi_in_ns - FrequencyHZ;
  result = pow (difference , 2);
  return result;
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
  guint minIRQduration;
  double periodInSec;
  int periodInNsec = 0;
  char maxIrqHandler[80];
  char minIrqHandler[80];
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray *FirstRequestIrqExit = event_data->FirstRequestIrqExit;  
  int FrequencyHZ =  0; 
  periodInSec = 0;
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
    
    sprintf(maxIrqHandler, "%d [%lu.%lu - %lu.%lu]",maxIRQduration, element.max_irq_handler.start_time.tv_sec, \
    element.max_irq_handler.start_time.tv_nsec, element.max_irq_handler.end_time.tv_sec, \
    element.max_irq_handler.end_time.tv_nsec) ;
    
    minIRQduration  =  element.min_irq_handler.duration.tv_sec;
    minIRQduration *= NANOSECONDS_PER_SECOND;
    minIRQduration += element.min_irq_handler.duration.tv_nsec;
    sprintf(minIrqHandler, "%d [%lu.%lu - %lu.%lu]",minIRQduration, element.min_irq_handler.start_time.tv_sec, \
    element.min_irq_handler.start_time.tv_nsec, element.min_irq_handler.end_time.tv_sec, \
    element.min_irq_handler.end_time.tv_nsec) ;
 
    
    FrequencyHZ = FrequencyInHZ(element.TotalNumberOfInterrupts,event_data->time_window);
   
   if(FrequencyHZ != 0)
   {
      periodInSec = (double)1/FrequencyHZ;
      periodInSec *= NANOSECONDS_PER_SECOND;
      periodInNsec = (int)periodInSec;
     
   }
     
    gtk_list_store_append (event_data->ListStore, &iter);
    gtk_list_store_set (event_data->ListStore, &iter,
      CPUID_COLUMN, element.cpu_id,
      IRQ_ID_COLUMN,  element.id,
      FREQUENCY_COLUMN, FrequencyHZ,
      DURATION_COLUMN, real_data,
      DURATION_STANDARD_DEV_COLUMN, CalculateDurationStandardDeviation(element.id, event_data),
      MAX_IRQ_HANDLER_COLUMN, maxIrqHandler,
      MIN_IRQ_HANDLER_COLUMN, minIrqHandler,
      AVERAGE_PERIOD , periodInNsec,
      PERIOD_STANDARD_DEV_COLUMN,  CalculatePeriodStandardDeviation(element.id, event_data),
      FREQUENCY_STANDARD_DEV_COLUMN, CalculateFrequencyStandardDeviation(element.id, event_data),
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


/**
 *  This function converts the number of interrupts over a time window to
 *  frequency in HZ
 */ 
static int FrequencyInHZ(gint NumerofInterruptions, TimeWindow time_window)
{
  guint64 frequencyHz = 0;
  double timeSec;  // time in second
  double result; 
  result  = ltt_time_to_double(time_window.time_width);
  timeSec = (result/NANOSECONDS_PER_SECOND);  //time in second
  frequencyHz = NumerofInterruptions / timeSec;  
  return  frequencyHz;
}

/**
 *  This function calculates the duration standard deviation
 *  Duration standard deviation = sqrt(1/N Sum ((xi -Xa)^2)) 
 *  Where: 
 *   sumId.sumOfDurations -> Sum ((xi -Xa)^2)
 *   inner_component -> 1/N Sum ((xi -Xa)^2)
 *   deviation-> sqrt(1/N Sum ((xi -Xa)^2)) 
 */ 
static int CalculateDurationStandardDeviation(gint id, InterruptEventData *event_data)
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
        if(sumId.TotalNumberOfInterrupts != 0)
  	  inner_component = sumId.sumOfDurations/ sumId.TotalNumberOfInterrupts;
	 else  
	  inner_component = 0.0;
	deviation =  sqrt(inner_component);
  	return deviation;
    }    
  }
  return deviation; 
}


/**
 *  This function calculates the period standard deviation
 *  Period standard deviation = sqrt(1/N Sum ((xi -Xa)^2)) 
 *  Where: 
 *   sumId.sumOfPeriods -> Sum ((xi -Xa)^2)
 *   inner_component -> 1/N Sum ((xi -Xa)^2)
 *   period_standard_deviation-> sqrt(1/N Sum ((xi -Xa)^2)) 
 
 *  
 */ 
static int CalculatePeriodStandardDeviation(gint id, InterruptEventData *event_data)
{
   int i;
   SumId sumId;
   guint64 inner_component;
   guint64 period_standard_deviation = 0;
   
   for(i = 0; i < event_data->SumArray->len; i++)
   {  
      sumId  = g_array_index(event_data->SumArray, SumId, i);  
      if(id == sumId.irqId)
      {
        if(sumId.TotalNumberOfInterrupts != 0)
           inner_component = sumId.sumOfPeriods / sumId.TotalNumberOfInterrupts;
	else
	   inner_component = 0;
	   
	period_standard_deviation =  sqrt(inner_component);
      }
   }
   
   return period_standard_deviation;
}

/**
 *  This function calculates the frequency standard deviation
 *  Frequency standard deviation = sqrt(1/N Sum ((xi -Xa)^2)) 
 *  Where: 
 *   sumId.sumOfFrequencies -> Sum ((xi -Xa)^2)
 *   inner_component -> 1/N Sum ((xi -Xa)^2)
 *   frequency_standard_deviation-> sqrt(1/N Sum ((xi -Xa)^2)) 
 *  
 */ 
static int CalculateFrequencyStandardDeviation(gint id, InterruptEventData *event_data)
{
   int i;
   SumId sumId;
   guint64 inner_component;
   guint64 frequency_standard_deviation = 0;
   for(i = 0; i < event_data->SumArray->len; i++)
   {  
     sumId  = g_array_index(event_data->SumArray, SumId, i); 	
     if(id == sumId.irqId)
     {
        if(sumId.TotalNumberOfInterrupts != 0)
           inner_component = sumId.sumOfFrequencies / sumId.TotalNumberOfInterrupts;
	else
	   inner_component = 0;
       
        frequency_standard_deviation =  sqrt(inner_component);	   
     }
   }
   return frequency_standard_deviation;
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
  InterruptEventData *event_data = (InterruptEventData*) data;
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
    This function is called when the viewer is destroyed to free hooks and memory
*/
static void InterruptFree(InterruptEventData *event_viewer_data)
{
  Tab *tab = event_viewer_data->tab;
  if(tab != NULL)
  {
  
     g_array_free(event_viewer_data->FirstRequestIrqExit, TRUE);
     g_array_free(event_viewer_data->FirstRequestIrqEntry, TRUE);
     g_array_free(event_viewer_data->SecondRequestIrqEntry, TRUE);
     g_array_free(event_viewer_data->SecondRequestIrqExit, TRUE);
     g_array_free(event_viewer_data->SumArray, TRUE);
     
     lttvwindow_unregister_time_window_notify(tab, interrupt_update_time_window, event_viewer_data);
     	
     lttvwindow_events_request_remove_all(event_viewer_data->tab,
                                          event_viewer_data);	
					  
     interrupt_data_list = g_slist_remove(interrupt_data_list, event_viewer_data);					  
      
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
