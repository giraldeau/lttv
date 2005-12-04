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

 

#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
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
  
typedef struct {
	guint cpu_id;
	guint id;
	guint frequency;
	LttTime total_duration;	
}Irq;

typedef struct {
	guint id;
	guint cpu_id;
	LttTime event_time;
}irq_entry;

enum type_t {
   IRQ_ENTRY,
   IRQ_EXIT		
};

/** Array containing instanced objects. Used when module is unloaded */
static GSList *interrupt_data_list = NULL ;
 

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
  
  GArray *interrupt_counters;
  GArray *active_irq_entry ;
} InterruptEventData ;

/* Function prototypes */
 
static gboolean interrupt_update_time_window(void * hook_data, void * call_data);
GtkWidget *interrupts(Tab *tab);
// Plug-in's constructor
InterruptEventData *system_info(Tab *tab);
// Plug-in's destructor
void interrupt_destructor(InterruptEventData *event_viewer_data);

static void request_event(  InterruptEventData *event_data);  
static guint64 get_event_detail(LttEvent *e, LttField *f);
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean parse_event(void *hook_data, void *call_data);
static gboolean interrupt_show(void *hook_data, void *call_data);
static void calcul_duration(LttTime time_exit,  guint cpu_id,  InterruptEventData *event_data);
static void sum_interrupt_data(irq_entry *e, LttTime time_exit, GArray *interrupt_counters);
/* Enumeration of the columns */
enum{
  CPUID_COLUMN,
  IRQ_ID_COLUMN,
  FREQUENCY_COLUMN,
  DURATION_COLUMN,
  N_COLUMNS
};

/**
 *  constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *interrupts(Tab * tab){

  InterruptEventData* event_data = system_info(tab) ;
  if(event_data)
    return event_data->Hbox;
  else 
    return NULL; 
}

InterruptEventData *system_info(Tab *tab)
{
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  InterruptEventData* event_viewer_data = g_new(InterruptEventData,1) ;
  g_info("system_info \n");
   
  event_viewer_data->tab = tab;
  event_viewer_data->time_window  =  lttvwindow_get_time_window(tab);
  event_viewer_data->interrupt_counters = g_array_new(FALSE, FALSE, sizeof(Irq));
  event_viewer_data->active_irq_entry   =  g_array_new(FALSE, FALSE, sizeof(irq_entry));
  					 	
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
    G_TYPE_UINT64   /* Duration                    */
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
  column = gtk_tree_view_column_new_with_attributes ("Frequency",
                 renderer,
                 "text", FREQUENCY_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Duration (nsec)",
                 renderer,
                 "text", DURATION_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 145);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  event_viewer_data->SelectionTree = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->TreeView));
  gtk_tree_selection_set_mode (event_viewer_data->SelectionTree, GTK_SELECTION_SINGLE);
   
  gtk_container_add (GTK_CONTAINER (event_viewer_data->ScrollWindow), event_viewer_data->TreeView);
   
  event_viewer_data->Hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->Hbox), event_viewer_data->ScrollWindow, TRUE, TRUE, 0);
 
  gtk_widget_show(event_viewer_data->Hbox);
  gtk_widget_show(event_viewer_data->TreeView);

  interrupt_data_list = g_slist_append(interrupt_data_list, event_viewer_data);
  
  lttvwindow_register_time_window_notify(tab,
                                         interrupt_update_time_window,
                                         event_viewer_data);	
  request_event(event_viewer_data);
  return event_viewer_data;
}


static void request_event(  InterruptEventData *event_data){

  event_data->hooks_trace_before = lttv_hooks_new();
  lttv_hooks_add(event_data->hooks_trace_before, trace_header, event_data, LTTV_PRIO_DEFAULT);
  
  event_data->event_hooks = lttv_hooks_new();
  lttv_hooks_add(event_data->event_hooks, parse_event, event_data, LTTV_PRIO_DEFAULT);
 
  event_data->hooks_trace_after = lttv_hooks_new();
  lttv_hooks_add(event_data->hooks_trace_after, interrupt_show, event_data, LTTV_PRIO_DEFAULT);
    
  EventsRequest *events_request = g_new(EventsRequest, 1); 
  events_request->owner       = event_data; 
  events_request->viewer_data = event_data; 
  events_request->servicing   = FALSE;     
  events_request->start_time  = event_data->time_window.start_time; 
  events_request->start_position  = NULL;
  events_request->stop_flag	   = FALSE;
  events_request->end_time 	   = event_data->time_window.end_time;
  events_request->num_events  	   = G_MAXUINT;      
  events_request->end_position     = NULL; 
  events_request->trace 	   = 0;    
  events_request->hooks 	   = NULL; 
  events_request->before_chunk_traceset = NULL; 
  events_request->before_chunk_trace    = event_data->hooks_trace_before; 
  events_request->before_chunk_tracefile= NULL; 
  events_request->event		        = event_data->event_hooks; 
  events_request->event_by_id		= NULL; 
  events_request->after_chunk_tracefile = NULL; 
  events_request->after_chunk_trace     = NULL;   
  events_request->after_chunk_traceset	= NULL; 
  events_request->before_request	= NULL; 
  events_request->after_request		= event_data->hooks_trace_after; 
  
  lttvwindow_events_request(event_data->tab, events_request);   
}

gboolean parse_event(void *hook_data, void *call_data){

  LttTime  event_time; 
  LttEvent *e;
  LttField *field;
  LttEventType *event_type;
  unsigned cpu_id;
  irq_entry entry;
  irq_entry *element; 
  guint i;
 // g_info("interrupts: parse_event() \n");
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray* active_irq_entry  = event_data->active_irq_entry; 
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  //e = tfc->e; 
  e = ltt_tracefile_get_event(tfc->tf);
  
  field = ltt_event_field(e);
  event_time = ltt_event_time(e);
  event_type = ltt_event_eventtype(e);
  cpu_id = ltt_event_cpu_id(e);
  GString * detail_event = g_string_new("");
  if ((ltt_time_compare(event_time,event_data->time_window.start_time) == TRUE) &&    
     (ltt_time_compare(event_data->time_window.end_time,event_time) == TRUE)){
	if (strcmp( g_quark_to_string(ltt_eventtype_name(event_type)),"irq_entry") == 0) {  
	    entry.id = get_event_detail(e, field);
	    entry.cpu_id = cpu_id;
	    entry.event_time =  event_time;
	    g_array_append_val (active_irq_entry, entry);
	    
	}
	if(strcmp( g_quark_to_string(ltt_eventtype_name(event_type)),"irq_exit") == 0) {
	  //printf("event_time: %ld.%ld\n",event_time.tv_sec,event_time.tv_nsec);	
	  calcul_duration( event_time,  cpu_id, event_data);
        }
   } 
   g_string_free(detail_event, TRUE);
   return FALSE;
}

 
void interrupt_destructor(InterruptEventData *event_viewer_data)
{
  /* May already been done by GTK window closing */
  g_info("enter interrupt_destructor \n");
  if(GTK_IS_WIDGET(event_viewer_data->Hbox)){
    gtk_widget_destroy(event_viewer_data->Hbox);
  }
}
   
static guint64 get_event_detail(LttEvent *e, LttField *f){

  LttType *type;
  LttField *element;
  GQuark name;
  int nb, i;
  guint64  irq_id;
  type = ltt_field_type(f);
  nb = ltt_type_member_number(type);
  for(i = 0 ; i < nb-1 ; i++) {
  	element = ltt_field_member(f,i);
  	ltt_type_member_type(type, i, &name);
  	irq_id = ltt_event_get_long_unsigned(e,element);
  }
  return  irq_id;
} 




gboolean trace_header(void *hook_data, void *call_data){

  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttEvent *e;
  LttTime event_time;
  return FALSE;
}

static gboolean interrupt_show(void *hook_data, void *call_data){
  
  gint i;	
  Irq element; 
  LttTime average_duration;
  GtkTreeIter    iter;
  guint64 real_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray *interrupt_counters = event_data->interrupt_counters;  
  g_info("interrupts: interrupt_show() \n");
  gtk_list_store_clear(event_data->ListStore);
  for(i = 0; i < interrupt_counters->len; i++){  
    element = g_array_index(interrupt_counters,Irq,i);  
    real_data = element.total_duration.tv_sec;
    real_data *= NANOSECONDS_PER_SECOND;
    real_data += element.total_duration.tv_nsec;
    //printf("total_duration:%ld\n",  element.total_duration.tv_nsec);
    gtk_list_store_append (event_data->ListStore, &iter);
    gtk_list_store_set (event_data->ListStore, &iter,
      CPUID_COLUMN, element.cpu_id,
      IRQ_ID_COLUMN,  element.id,
      FREQUENCY_COLUMN, element.frequency,
      DURATION_COLUMN, real_data,
      -1);
  } 
  
  if(event_data->interrupt_counters->len)
     g_array_remove_range (event_data->interrupt_counters,0,event_data->interrupt_counters->len);
   
  if(event_data->active_irq_entry->len)
    g_array_remove_range (event_data->active_irq_entry,0,event_data->active_irq_entry->len);
  return FALSE;
}

static void sum_interrupt_data(irq_entry *e, LttTime time_exit, GArray *interrupt_counters){
  Irq irq;
  Irq *element; 
  guint i;
  LttTime duration;
  gboolean  notFound = FALSE;
  memset ((void*)&irq, 0,sizeof(Irq));
  
  //printf("time_exit: %ld.%ld\n",time_exit.tv_sec,time_exit.tv_nsec);	
  if(interrupt_counters->len == NO_ITEMS){
    irq.cpu_id = e->cpu_id;
    irq.id    =  e->id;
    irq.frequency++;
    irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
    g_array_append_val (interrupt_counters, irq);
  }
  else{
    for(i = 0; i < interrupt_counters->len; i++){
      element = &g_array_index(interrupt_counters,Irq,i);
      if(element->id == e->id){
	notFound = TRUE;
	duration =  ltt_time_sub(time_exit, e->event_time);
	element->total_duration = ltt_time_add(element->total_duration, duration);
	element->frequency++;
      }
    }
    if(!notFound){
      irq.cpu_id = e->cpu_id;
      irq.id    =  e->id;
      irq.frequency++;
      irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
      g_array_append_val (interrupt_counters, irq);
    }
  } 
}
 
static void calcul_duration(LttTime time_exit,  guint cpu_id,InterruptEventData *event_data){
  
  gint i, irq_id;
  irq_entry *element; 
  LttTime duration;
  GArray *interrupt_counters = event_data->interrupt_counters;
  GArray *active_irq_entry = event_data->active_irq_entry;
  for(i = 0; i < active_irq_entry->len; i++){
    element = &g_array_index(active_irq_entry,irq_entry,i);
    if(element->cpu_id == cpu_id){
      sum_interrupt_data(element,time_exit,  interrupt_counters);    
      g_array_remove_index(active_irq_entry, i);
     // printf("array length:%d\n",active_irq_entry->len);
      break;
    }
  }
}


gboolean interrupt_update_time_window(void * hook_data, void * call_data){
 
  InterruptEventData *event_data = (InterruptEventData *) hook_data;
  const TimeWindowNotifyData *time_window_nofify_data =  ((const TimeWindowNotifyData *)call_data);
  event_data->time_window = *time_window_nofify_data->new_time_window;
  g_info("interrupts: interrupt_update_time_window()\n");
  Tab *tab = event_data->tab;
  lttvwindow_events_request_remove_all(tab, event_data);
  request_event(event_data);
   
  
    return FALSE;
}

/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
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

void interrupt_destroy_walk(gpointer data, gpointer user_data){
  g_info("interrupt_destroy_walk");
  interrupt_destructor((InterruptEventData*)data);

}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  g_info("Destroy  interrupts");
  g_slist_foreach(interrupt_data_list, interrupt_destroy_walk, NULL );
  g_slist_free(interrupt_data_list); 
  lttvwindow_unregister_constructor(interrupts);
  
}

LTTV_MODULE("interrupts", "interrupts info view", \
    "Graphical module to display interrupts performance", \
	    init, destroy, "lttvwindow")
