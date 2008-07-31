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
#include <ltt/trace.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/filter.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttv_plugin_tab.h>
#include <ltt/time.h>

#include "hDiskPerformanceInsert.xpm" 


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
// fixed #define TRACE_NUMBER 0
#define NO_ITEMS 0

enum{
  DISKNAME_COLUMN,
  BYTES_RD_COLUMN,
  BYTES_RD_SEC_COLUMN,
  NUM_RD_COLUMN,
  BYTES_WR_COLUMN,
  BYTES_WR_SEC_COLUMN,
  NUM_WR_COLUMN,
  N_COLUMNS
};

enum operation_t {
     LTTV_READ_OPERATION = 1,	
     LTTV_WRITE_OPERATION
};

typedef struct _DiskPerformanceData {

  Tab 	     * tab;

  LttvPluginTab *ptab;
   
  LttvHooks  * hooks_trace_after;
  
  LttvHooks  * hooks_trace_before;
  /* time window */
  TimeWindow   time_window;
      
  GtkWidget * scroll_win;
  
  /* Model containing list data */
  GtkListStore *store_m;
  
  GtkWidget *hbox_v;
  
  /* Widget to display the data in a columned list */
  GtkWidget *tree_v;
  
  /* Selection handler */
  GtkTreeSelection *select_c;
  
  GArray *disk_array; 
  
  LttvHooksById * event_by_id_hooks;
  
} DiskPerformanceData;


typedef struct _lttv_block { 
	guint major_number;
	guint minor_number;
	guint size;
} lttv_block;

typedef struct _lttv_total_block {
	char diskname[10];
	guint64 total_bytes_read;
	guint num_read_operations;
	guint64 total_bytes_written;
	guint num_write_operations;
	 
} lttv_total_block;

GSList *g_disk_data_list = NULL ;



/* facility */
GQuark LTT_FACILITY_BLOCK;   

/* events */
GQuark LTT_EVENT_BLOCK_READ;  
GQuark LTT_EVENT_BLOCK_WRITE;   

static DiskPerformanceData *disk_performance_data(LttvPluginTab *ptab);
static void disk_destroy_walk(gpointer data, gpointer user_data);
static gboolean disk_show(void *hook_data, void *call_data);
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean disk_update_time_window(void * hook_data, void * call_data);
static void request_event(  DiskPerformanceData *disk_performance);
void gui_disperformance_free(DiskPerformanceData *event_viewer_data);
static void get_event_detail(LttEvent *e, lttv_block* disk_data);
static char * major_minor_to_diskname( lttv_block* disk_data); 
static void sum_data(char* diskname, guint size, enum operation_t opt, GArray *disk_array);
static GtkWidget *disk_performance(LttvPlugin *plugin);

static gboolean block_read_callback(void *hook_data, void *call_data);

static gboolean block_write_callback(void *hook_data, void *call_data);

 
static gboolean disk_show(void *hook_data, void *call_data){
  
  guint i;
  lttv_total_block element; 
  GtkTreeIter    iter;
  LttTime time_interval;
  guint64 time_interval_64;
  guint64 temp_variable; 
  guint64 bytes_read_per_sec, bytes_written_per_sec;
  g_info(" diskperformance: disk_show() \n");
  DiskPerformanceData *disk_performance = (DiskPerformanceData *)hook_data;
  GArray *disk_array = disk_performance->disk_array;
  time_interval =  ltt_time_sub(disk_performance->time_window.end_time, disk_performance->time_window.start_time); 
  
  time_interval_64  = time_interval.tv_sec;
  time_interval_64 *= NANOSECONDS_PER_SECOND;
  time_interval_64 += time_interval.tv_nsec;
  gtk_list_store_clear(disk_performance->store_m);
  for(i = 0; i < disk_array->len; i++){  
    
    element = g_array_index(disk_array,lttv_total_block,i);  
    temp_variable =  element.total_bytes_read * NANOSECONDS_PER_SECOND;
    bytes_read_per_sec = (guint64) temp_variable / time_interval_64;
    
    temp_variable =  element.total_bytes_written * NANOSECONDS_PER_SECOND;
    bytes_written_per_sec  = (guint64) temp_variable / time_interval_64;
    
    gtk_list_store_append (disk_performance->store_m, &iter);
    gtk_list_store_set (disk_performance->store_m, &iter,
      DISKNAME_COLUMN, element.diskname,
      BYTES_RD_COLUMN, element.total_bytes_read,
      BYTES_RD_SEC_COLUMN,bytes_read_per_sec,
      NUM_RD_COLUMN, element.num_read_operations,
      BYTES_WR_COLUMN, element.total_bytes_written,
      BYTES_WR_SEC_COLUMN, bytes_written_per_sec,
      NUM_WR_COLUMN, element.num_write_operations,
      -1); 
       
  }
  if(disk_performance->disk_array->len) 
    g_array_remove_range (disk_performance->disk_array,0,disk_performance->disk_array->len);
  return FALSE;
}

static gboolean trace_header(void *hook_data, void *call_data){
  return FALSE;
}


static gboolean disk_update_time_window(void * hook_data, void * call_data){
     
  DiskPerformanceData *disk_performance = (DiskPerformanceData *) hook_data;
  const TimeWindowNotifyData *time_window_nofify_data =  ((const TimeWindowNotifyData *)call_data);
  disk_performance->time_window = *time_window_nofify_data->new_time_window;
  Tab *tab = disk_performance->tab;
  lttvwindow_events_request_remove_all(tab, disk_performance);
  request_event( disk_performance);  
   
  
    return FALSE;
}
 
void gui_disperformance_free(DiskPerformanceData  *eventdata){ 
  Tab *tab = eventdata->tab;
  g_info("disperformance.c : gui_disperformance_free, %p", eventdata);
  g_info("%p, %p", eventdata, tab);
  if(tab != NULL)
  {
     g_array_free (eventdata->disk_array, TRUE);
     
     lttvwindow_unregister_time_window_notify(tab,
        disk_update_time_window,
        eventdata);
	
     lttvwindow_events_request_remove_all(eventdata->tab,
                                          eventdata);	
     g_disk_data_list = g_slist_remove(g_disk_data_list, eventdata);					  
  }
  g_free(eventdata);					  
  g_info("disperformance.c : gui_disperformance_free end, %p", eventdata);
}
  

 



void disk_destructor_full(DiskPerformanceData *disk_data)
{

  if(GTK_IS_WIDGET(disk_data->hbox_v))
    gtk_widget_destroy(disk_data->hbox_v);

}

static void disk_destroy_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI disk performance Viewer");
  disk_destructor_full((DiskPerformanceData*)data);
}
/**
 *  init function
 *
 * 
 * This is the entry point of the viewer.
 *
 */
static void init()
{
  
  g_info("Init diskPerformance.c");
  
  LTT_FACILITY_BLOCK    = g_quark_from_string("block");
  LTT_EVENT_BLOCK_READ  =  g_quark_from_string("read");   
  LTT_EVENT_BLOCK_WRITE = g_quark_from_string("write"); 
  
  lttvwindow_register_constructor("diskperformance",
                                  "/",
                                  "Insert Disk Performance",
                                  hDiskPerformanceInsert_xpm,
                                  "Insert Disk Performance",
                                  disk_performance);
}

/**
 *  Constructor hook
 *
 */
GtkWidget *disk_performance(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  DiskPerformanceData* disk_data = disk_performance_data(ptab);
  if(disk_data)
    return disk_data->hbox_v;
  else 
    return NULL; 
}

/**
 * This function initializes the Event Viewer functionnality through the
 * GTK  API. 
 */
DiskPerformanceData *disk_performance_data(LttvPluginTab *ptab)
{ 
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  DiskPerformanceData* disk_data = g_new(DiskPerformanceData,1) ;
  
  g_info("enter disk_performance_data \n");
  Tab *tab = ptab->tab;
  disk_data->tab = tab;
  disk_data->ptab = ptab;
  disk_data->time_window  =  lttvwindow_get_time_window(tab);
  
  disk_data->disk_array = g_array_new(FALSE, FALSE, sizeof(lttv_total_block ));
  
  lttvwindow_register_time_window_notify(tab,
                                         disk_update_time_window,
                                         disk_data);	
					 	
  disk_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (disk_data->scroll_win);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(disk_data->scroll_win), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  /* Create a model for storing the data list */
  disk_data->store_m = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_STRING,     /* Diskname                 */  
    G_TYPE_INT64,     /* Bytes read                */
    G_TYPE_INT64,     /* Bytes read/sec 	   */
    G_TYPE_INT,
    G_TYPE_INT64,    /*  bytes written             */
    G_TYPE_INT64,    /*  bytes written/sec         */
    G_TYPE_INT
    );  
 
  disk_data->tree_v = gtk_tree_view_new_with_model (GTK_TREE_MODEL (disk_data->store_m));
   
  g_object_unref (G_OBJECT (disk_data->store_m));
    
  renderer = gtk_cell_renderer_text_new ();
  
  column = gtk_tree_view_column_new_with_attributes ("DiskName",
                 renderer,
                 "text", DISKNAME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("BytesRead",
                 renderer,
                 "text", BYTES_RD_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column,  220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("BytesRead/sec",
                 renderer,
                 "text", BYTES_RD_SEC_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("NumReadOperations",
                 renderer,
                 "text",NUM_RD_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("BytesWritten",
                 renderer,
                 "text", BYTES_WR_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 145);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("BytesWritten/sec",
                 renderer,
                 "text", BYTES_WR_SEC_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("NumWriteOperations",
                 renderer,
                 "text",NUM_WR_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 145);
  gtk_tree_view_append_column (GTK_TREE_VIEW (disk_data->tree_v), column);
  
  disk_data->select_c = gtk_tree_view_get_selection (GTK_TREE_VIEW (disk_data->tree_v));
  gtk_tree_selection_set_mode (disk_data->select_c, GTK_SELECTION_SINGLE);
   
  gtk_container_add (GTK_CONTAINER (disk_data->scroll_win), disk_data->tree_v);

  disk_data->hbox_v = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(disk_data->hbox_v), disk_data->scroll_win, TRUE, TRUE, 0);
  
  gtk_widget_show(disk_data->hbox_v);
  gtk_widget_show(disk_data->tree_v);
 
   
  g_disk_data_list = g_slist_append(g_disk_data_list, disk_data);
  g_object_set_data_full(G_OBJECT(disk_data->hbox_v),
      "disk_data",
       disk_data,
      (GDestroyNotify)gui_disperformance_free);
  
  request_event(disk_data);
  return disk_data;
}

/**
 * 
 * For each trace in the traceset, this function:
 *  - calls lttv_trace_find_hook() & registers a hook function to event_by_id_hooks
 *  - registers a callback function to each hook
 *  - calls lttvwindow_events_request() to request data in a specific 
 *    time interval to the main window
 * 
 */
static void request_event(DiskPerformanceData *disk_performance)
{
  guint i, k, l, nb_trace;
  
  GArray *hooks;
  
  guint ret; 
  
  LttvTraceHook *hook;
  
  LttvTraceState *ts;
    
  LttvTraceHookByFacility *thf;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(disk_performance->tab);
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
    
  //for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++) {
  for(i = 0 ; i < nb_trace ; i++) {
  	EventsRequest *events_request = g_new(EventsRequest, 1);
	
	hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
	
	hooks = g_array_set_size(hooks, 2);
  	
	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
        
	disk_performance->event_by_id_hooks = lttv_hooks_by_id_new();
	/* Register event_by_id_hooks with a callback function */ 
        ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_BLOCK, LTT_EVENT_BLOCK_READ,
		0, 0, 0,
		block_read_callback,
		disk_performance,
		&g_array_index(hooks, LttvTraceHook, 0));
	 
	ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_BLOCK, LTT_EVENT_BLOCK_WRITE,
		0, 0, 0,
		block_write_callback,
		disk_performance,
		&g_array_index(hooks, LttvTraceHook, 1));
		
	g_assert(!ret);
 	
	/*iterate through the facility list*/
	for(k = 0 ; k < hooks->len; k++) 
	{ 
	        hook = &g_array_index(hooks, LttvTraceHook, k);
		for(l=0; l<hook->fac_list->len; l++) 
		{
			thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l); 
			lttv_hooks_add(lttv_hooks_by_id_find(disk_performance->event_by_id_hooks, thf->id),
				thf->h,
				disk_performance,
				LTTV_PRIO_DEFAULT);
			 
		}
	}
	
	disk_performance->hooks_trace_after = lttv_hooks_new();
	/* Registers  a hook function */
	lttv_hooks_add(disk_performance->hooks_trace_after, disk_show, disk_performance, LTTV_PRIO_DEFAULT);
	
	disk_performance->hooks_trace_before = lttv_hooks_new();
	/* Registers  a hook function */
	lttv_hooks_add(disk_performance->hooks_trace_before, trace_header, disk_performance, LTTV_PRIO_DEFAULT);
	
	/* Initalize the EventsRequest structure */
	events_request->owner       = disk_performance;
	events_request->viewer_data = disk_performance;
	events_request->servicing   = FALSE;
	events_request->start_time  = disk_performance->time_window.start_time;
	events_request->start_position  = NULL;
	events_request->stop_flag	   = FALSE;
	events_request->end_time 	   = disk_performance->time_window.end_time;
	events_request->num_events  	   = G_MAXUINT;
	events_request->end_position       = NULL;
	events_request->trace 	   	   = i;
	events_request->hooks 	   	   = hooks;
	events_request->before_chunk_traceset = NULL;
	events_request->before_chunk_trace    = disk_performance->hooks_trace_before;
	events_request->before_chunk_tracefile= NULL;
	events_request->event		        = NULL; 
	events_request->event_by_id		= disk_performance->event_by_id_hooks;
	events_request->after_chunk_tracefile = NULL;
	events_request->after_chunk_trace     = NULL;
	events_request->after_chunk_traceset	= NULL;
	events_request->before_request	= NULL;
	events_request->after_request	= disk_performance->hooks_trace_after;
	
	lttvwindow_events_request(disk_performance->tab, events_request);
  }
   
}

/**
 *  This function is called whenever a read event occurs.  
 *  
 */ 
static gboolean block_read_callback(void *hook_data, void *call_data)
{
  LttEvent *e;
  LttTime  event_time; 
  unsigned cpu_id; 
  lttv_block block_read;
  char *diskname;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  DiskPerformanceData *disk_performance = (DiskPerformanceData *)hook_data;
  GArray *disk_array = disk_performance->disk_array; 
  e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
  
  get_event_detail(e, &block_read);
  diskname = major_minor_to_diskname(&block_read);
  sum_data(diskname, block_read.size,LTTV_READ_OPERATION, disk_array);
 
  return FALSE;
}

/**
 *  This function is called whenever a write event occurs.  
 *  
 */ 
static gboolean block_write_callback(void *hook_data, void *call_data)
{
  LttEvent *e;
  LttTime  event_time; 
  unsigned cpu_id;
  lttv_block  block_write;
 char *diskname;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  DiskPerformanceData *disk_performance = (DiskPerformanceData *)hook_data;
  GArray *disk_array = disk_performance->disk_array; 
  e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
  
  get_event_detail(e, &block_write);
  diskname = major_minor_to_diskname(&block_write);
  sum_data(diskname, block_write.size,LTTV_WRITE_OPERATION, disk_array);
  
 return FALSE;
}

/**
 *  This function extracts  the major, minor and size 
 *  
 */ 
static void  get_event_detail(LttEvent *e, lttv_block* disk_data)
{
  guint i, num_fields;
  LttEventType *event_type;
  LttField *element;  
  LttField *field;
  event_type = ltt_event_eventtype(e);
  num_fields = ltt_eventtype_num_fields(event_type);
  
  for(i = 0 ; i < num_fields ; i++) 
  {
  	element = ltt_eventtype_field(event_type,i);
	switch(i)
	{
	  case 0:
	  	disk_data->major_number = ltt_event_get_long_unsigned(e, element); 
	  break;
	  	
	  case 1:
	  	disk_data->minor_number = ltt_event_get_long_unsigned(e, element); 
	  break;
	  case 2:
	  	disk_data->size = ltt_event_get_long_unsigned(e, element); 
	  break;
	}
	 
  }
   
}


 
/**
 *  This function calculates: the number of operations, the total bytes read or written,  
 *  the average number of bytes read or written by sec.
 */ 
static void sum_data(char* diskname, guint size, enum operation_t operation, GArray *disk_array)
{
  
  lttv_total_block data;
  lttv_total_block *element; 
  guint i;
  gboolean  notFound = FALSE;
  
  memset ((void*)&data, 0,sizeof(lttv_total_block));
   
  if(disk_array->len == NO_ITEMS){
	strcpy(data.diskname, diskname);
	if(operation == LTTV_READ_OPERATION){
	   data.total_bytes_read = size;
	   data.num_read_operations++;
	}
	else{
	   data.total_bytes_written = size;
	   data.num_write_operations ++;
	}
	g_array_append_val (disk_array, data);
  } 
  else{
  	for(i = 0; i < disk_array->len; i++){
	    element = &g_array_index(disk_array,lttv_total_block,i);
	    if(strcmp(element->diskname,diskname) == 0){
	    	if(operation == LTTV_READ_OPERATION){
		  element->num_read_operations++;	
		  element->total_bytes_read += size;
		}
		else{
		  element->num_write_operations ++;
		  element->total_bytes_written += size;
		}
		notFound = TRUE;
	    }
	}
	if(!notFound){
	    strcpy(data.diskname, diskname);
	    if(operation == LTTV_READ_OPERATION){
	      data.total_bytes_read = size;
	      data.num_read_operations ++;
	    }
	    else{
	      data.total_bytes_written = size;
	      data.num_write_operations ++;
	    }
	    g_array_append_val (disk_array, data);
	}	
  }
}

  
static void destroy()
{
  g_info("Destroy diskPerformance");
  g_slist_foreach(g_disk_data_list, disk_destroy_walk, NULL );
  g_slist_free(g_disk_data_list);
  
  lttvwindow_unregister_constructor(disk_performance);
  
}

/**
 *  This function convert  the major and minor  number to the corresponding disk. 
 *  Data taken from Documentation/devices.txt of the kernel tree.
 */ 
static char * major_minor_to_diskname( lttv_block* disk_data)
{
  switch(disk_data->major_number)
  {
     /* IDE Disks */
     case 3: /* First MFM, RLL and IDE hard disk/CD-ROM interface */
     	if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hda";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdb";
     break;
     
     case 22: /*Second IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdc";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdd";
     break;
       
     case 33: /* Third IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hde";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdf";
     break;
       
     case 34: /* Fourth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdg";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdh";
     break;
     
     case 56: /* Fifth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdi";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdj";
     break;
     
     case 57: /* Sixth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdk";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdl";
     break;

     case 88: /* Seventh IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdm";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdn";
     break;          
     
     case  89: /* Eighth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdo";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdp";
     break;          
       
     case  90: /* Ninth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hdq";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdr";
     break;          
     
     case  91: /* Tenth IDE hard disk/CD-ROM interface */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 64))
	  return "/dev/hds";
        if((disk_data->minor_number >= 64) && (disk_data->minor_number < 128))		
	  return  "/dev/hdt";
     break;          
     
     /* SCSI Disks */
      case  8: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sda"; //	First SCSI disk whole disk
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdb";// Second SCSI disk whole disk
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdc";// Third SCSI disk whole disk
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdd";// Fourth SCSI disk whole disk	    
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sde";// Fifth SCSI disk whole disk	    
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdf";// Sixth SCSI disk whole disk	    	  
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdg";// seventh SCSI disk whole disk	    	  
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdh";// eighth SCSI disk whole disk	    	  	  
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdi";// 9th SCSI disk whole disk	    	  	    
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdj";// 10th SCSI disk whole disk	    	  	      
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdk";// 11th SCSI disk whole disk	    	  	      
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdl";// 12th SCSI disk whole disk	    	  	      
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdm";// 13th SCSI disk whole disk	    	  	      

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdn";// 14th SCSI disk whole disk	    	  	      	
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdo";// 15th SCSI disk whole disk	    	  	      		    
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdp";// 16th SCSI disk whole disk	    	  	      	
      break;          
     
      case  65: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdq"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdr"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sds"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdt";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdu"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdv"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdw"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdy"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdx"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdz"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdaa"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdab"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdac"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdad"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdae"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdaf"; 
      break;     

      case  66: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdag"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdah"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdai"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdaj";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdak"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdal"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdam"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdan"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdao"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdap"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdaq"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdar"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdas"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdat"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdau"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdav"; 
      break;     

      
      case  67: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdaw"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdax"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sday"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdaz";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdba"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdbb"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdbc"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdbd"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdbe"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdbf"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdbg"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdbh"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdbi"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdbj"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdbk"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdbl"; 
      break;     
      
      case  68 : /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdbm"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdbm"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdbo"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdbp";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdbq"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdbr"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdbs"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdbt"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdbu"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdbv"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdbw"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdbx"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdby"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdbz"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdca"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdcb"; 
      break;     
      case  69 : /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdcc"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdcd"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdce"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdcf";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdcg"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdch"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdci"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdcj"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdck"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdcl"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdcm"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdcn"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdco"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdcp"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdcq"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdcr"; 
      break;     

      
      case  70 : /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdcs"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdct"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdcu"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdcv";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdcw"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdcx"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdcy"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdcz"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdda"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sddb"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sddc"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sddd"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdde"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sddf"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sddg"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sddh"; 
      break;      
 
      case  71: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sddi"; 
        
	if((disk_data->minor_number >=  16) && (disk_data->minor_number < 32))		
	  return  "/dev/sddj"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sddk"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sddl";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sddm"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sddn"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sddo"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sddp"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sddq"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sddr"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdds"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sddt"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sddu"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sddv"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sddw"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sddx"; 
      break;      
       
      case  128: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sddy"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sddz"; 
	 
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdea"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sdeb";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdec"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sded"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdee"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdef"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdeg"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdeh"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdei"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdej"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdek"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdel"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdem"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sden"; 
      break;      

      case  129: /* SCSI disk devices */
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 16))
	  return "/dev/sdeo"; 
        
	if((disk_data->minor_number >= 16) && (disk_data->minor_number < 32))		
	  return  "/dev/sdep"; 
	
	if((disk_data->minor_number >= 32) && (disk_data->minor_number < 48))		
	  return  "/dev/sdeq"; 
	
	if((disk_data->minor_number >= 48) && (disk_data->minor_number < 64))		
	  return  "/dev/sder";  
	
	if((disk_data->minor_number >= 64) && (disk_data->minor_number < 80))		
	  return  "/dev/sdes"; 
	
	if((disk_data->minor_number >= 80) && (disk_data->minor_number < 96))		
	  return  "/dev/sdet"; 
      
        if((disk_data->minor_number >= 96) && (disk_data->minor_number < 112))		
	  return  "/dev/sdeu"; 
	
	if((disk_data->minor_number >= 112) && (disk_data->minor_number < 128))		
	  return  "/dev/sdev"; 
	
	if((disk_data->minor_number >= 128) && (disk_data->minor_number < 144))		
	  return  "/dev/sdew"; 
	  
	if((disk_data->minor_number >= 144) && (disk_data->minor_number < 160))		
	  return  "/dev/sdez"; 
	  
        if((disk_data->minor_number >= 160) && (disk_data->minor_number < 176))		
	  return  "/dev/sdey"; 
      
        if((disk_data->minor_number >= 176) && (disk_data->minor_number < 192))		
	  return  "/dev/sdez"; 
      
        if((disk_data->minor_number >= 192) && (disk_data->minor_number < 208))		
	  return  "/dev/sdfa"; 

	if((disk_data->minor_number >= 208) && (disk_data->minor_number < 224))		
	  return  "/dev/sdfb"; 
	
	if((disk_data->minor_number >= 224) && (disk_data->minor_number < 240))		
	  return  "/dev/sdfc"; 
	
	if((disk_data->minor_number >= 240) && (disk_data->minor_number < 256))		
	  return  "/dev/sdfd"; 
      break;      
      /*USB block devices*/
      case  180: 
        if( (disk_data->minor_number >= 0) && (disk_data->minor_number < 8))
	  return "/dev/uba"; 
	if( (disk_data->minor_number >= 8) && (disk_data->minor_number < 16))
	  return "/dev/ubb"; 
	if( (disk_data->minor_number >= 16) && (disk_data->minor_number < 24))
	  return "/dev/ubc"; 
      break;      
   
  }


}
LTTV_MODULE("diskperformance", "disk info view", \
	    "Produce disk I/O performance", \
	    init, destroy, "lttvwindow") 
	    
