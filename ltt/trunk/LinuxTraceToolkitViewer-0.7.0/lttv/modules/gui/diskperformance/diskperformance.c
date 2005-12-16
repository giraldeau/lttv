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

#include "hDiskPerformanceInsert.xpm" 


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
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
  LttvHooks  * event_hooks;
  LttvHooks  * hooks_trace_after;
  LttvHooks  * hooks_trace_before;
  TimeWindow   time_window; 		// time window
      
  GtkWidget * scroll_win;
  /* Model containing list data */
  GtkListStore *store_m;
  GtkWidget *hbox_v;
  /* Widget to display the data in a columned list */
  GtkWidget *tree_v;
  /* Selection handler */
  GtkTreeSelection *select_c;
  
  GArray *disk_array; 
  
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

DiskPerformanceData *disk_performance_data(Tab *tab);
static void disk_destroy_walk(gpointer data, gpointer user_data);
static gboolean parse_event(void *hook_data, void *call_data);
static gboolean disk_show(void *hook_data, void *call_data);
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean disk_update_time_window(void * hook_data, void * call_data);
static void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
static void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void request_event(  DiskPerformanceData *disk_performance);
void gui_disperformance_free(DiskPerformanceData *event_viewer_data);
static void get_event_detail(LttEvent *e, LttField *f, GString * s, lttv_block* disk_data);
static char * major_minor_to_diskname( lttv_block* disk_data); 
static void sum_data(char* diskname, guint size, enum operation_t opt, GArray *disk_array);

GtkWidget *disk_performance(Tab * tab){
 
 DiskPerformanceData* disk_data = disk_performance_data(tab);
 if(disk_data)
    return disk_data->hbox_v;
 else 
    return NULL; 

}

DiskPerformanceData *disk_performance_data(Tab *tab){
  
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  DiskPerformanceData* disk_data = g_new(DiskPerformanceData,1) ;
  
  g_info("enter disk_performance_data \n");
  
  disk_data->tab = tab;
  disk_data->time_window  =  lttvwindow_get_time_window(tab);
  
  disk_data->disk_array = g_array_new(FALSE, FALSE, sizeof(lttv_total_block ));
  
  lttvwindow_register_time_window_notify(tab,
                                         disk_update_time_window,
                                         disk_data);	
					 	
  disk_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (disk_data->scroll_win);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(disk_data->scroll_win), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC/*GTK_POLICY_NEVER*/);
 

  /* Create a model for storing the data list */
  disk_data->store_m = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_STRING,     /* Diskname                 */ // to change from INT to string later 
    G_TYPE_INT64,     /* Bytes read                */
    G_TYPE_INT64,     /* Bytes read/sec  		   */
    G_TYPE_INT,
    G_TYPE_INT64,    /*  bytes written               */
    G_TYPE_INT64,    /*  bytes written/sec            */
    G_TYPE_INT
    );  
 
  disk_data->tree_v = gtk_tree_view_new_with_model (GTK_TREE_MODEL (disk_data->store_m));
 
  g_signal_connect (G_OBJECT (disk_data->tree_v), "size-allocate",
        G_CALLBACK (tree_v_size_allocate_cb),
        disk_data);
  g_signal_connect (G_OBJECT (disk_data->tree_v), "size-request",
        G_CALLBACK (tree_v_size_request_cb),
        disk_data);  
  g_signal_connect (G_OBJECT (disk_data->tree_v), "cursor-changed",
        G_CALLBACK (tree_v_cursor_changed_cb),
        disk_data);
  g_signal_connect (G_OBJECT (disk_data->tree_v), "move-cursor",
        G_CALLBACK (tree_v_move_cursor_cb),
        disk_data);
  
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
  g_signal_connect (G_OBJECT (disk_data->select_c), "changed",
      	            G_CALLBACK (tree_selection_changed_cb),
                    disk_data);
  
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

void request_event(DiskPerformanceData *disk_performance){

  disk_performance->event_hooks = lttv_hooks_new();
  lttv_hooks_add(disk_performance->event_hooks, parse_event, disk_performance, LTTV_PRIO_DEFAULT);

  disk_performance->hooks_trace_after = lttv_hooks_new();
  lttv_hooks_add(disk_performance->hooks_trace_after, disk_show, disk_performance, LTTV_PRIO_DEFAULT);

  disk_performance->hooks_trace_before = lttv_hooks_new();
  lttv_hooks_add(disk_performance->hooks_trace_before, trace_header, disk_performance, LTTV_PRIO_DEFAULT);

  EventsRequest *events_request = g_new(EventsRequest, 1);
  events_request->owner       = disk_performance;
  events_request->viewer_data = disk_performance;
  events_request->servicing   = FALSE;
  events_request->start_time  = disk_performance->time_window.start_time;
  events_request->start_position  = NULL;
  events_request->stop_flag	   = FALSE;
  events_request->end_time 	   = disk_performance->time_window.end_time;
  events_request->num_events  	   = G_MAXUINT;
  events_request->end_position     = NULL;
  events_request->trace 	   = 0;
  events_request->hooks 	   = NULL;
  events_request->before_chunk_traceset = NULL;
  events_request->before_chunk_trace    = disk_performance->hooks_trace_before;
  events_request->before_chunk_tracefile= NULL;
  events_request->event		        = disk_performance->event_hooks;
  events_request->event_by_id		= NULL;
  events_request->after_chunk_tracefile = NULL;
  events_request->after_chunk_trace     = NULL;
  events_request->after_chunk_traceset	= NULL;
  events_request->before_request	= NULL;
  events_request->after_request		= disk_performance->hooks_trace_after;

  lttvwindow_events_request(disk_performance->tab, events_request);

}

static gboolean disk_update_time_window(void * hook_data, void * call_data){
     
  DiskPerformanceData *disk_performance = (DiskPerformanceData *) hook_data;
  const TimeWindowNotifyData *time_window_nofify_data =  ((const TimeWindowNotifyData *)call_data);
  disk_performance->time_window = *time_window_nofify_data->new_time_window;
  /*
  printf("end_time: %ld.%ld\n",    disk_performance->time_window.end_time.tv_sec,disk_performance->time_window.end_time.tv_nsec);
  */
  Tab *tab = disk_performance->tab;
  lttvwindow_events_request_remove_all(tab, disk_performance);
  request_event( disk_performance);  
   
  
    return FALSE;
}

void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data){
  g_info("enter tree_v_size_allocate_cb\n");
}

void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data){

}

static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data){

}

static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data){
}
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data){

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
  

static char * major_minor_to_diskname( lttv_block* disk_data){
  if (disk_data->major_number == 3 && disk_data->minor_number == 0)
 	return "hda";
  if (disk_data->major_number == 4 && disk_data->minor_number == 0)	
	return "hdb";
}
 

static void sum_data(char* diskname, guint size, enum operation_t operation, GArray *disk_array){
  
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

static void  get_event_detail(LttEvent *e, LttField *f, GString * s, lttv_block* disk_data){
  LttType *type;
  LttField *element;
  //char *name;
  GQuark name;
  int nb, i;
  static int count;
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
        get_event_detail(e, element, s, disk_data);
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
	if(i== 0)
  	  disk_data->major_number = ltt_event_get_long_unsigned(e, element); 
        if(i== 1)
	  disk_data->minor_number = ltt_event_get_long_unsigned(e, element); 
	if(i==2)
	  disk_data->size = ltt_event_get_long_unsigned(e, element); 
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
	get_event_detail(e, element, s, disk_data);
      }
      g_string_append_printf(s, " }");
      break;

  }
  
}
 

gboolean parse_event(void *hook_data, void *call_data){

  static LttTime time_entry, previous_time, event_time; 
  LttEvent *e;
  LttField *field;
  LttEventType *event_type;
  gint i;
  unsigned cpu_id;
  lttv_block block_read, block_write;
  char *diskname;
 
  gboolean  notFound = FALSE;
  DiskPerformanceData * disk_performance  = (DiskPerformanceData *)hook_data;
  GArray *disk_array = disk_performance->disk_array; // pho
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  
  //e = tfc->e;
  e = ltt_tracefile_get_event(tfc->tf); 
  previous_time = event_time;
  field = ltt_event_field(e);
  event_time = ltt_event_time(e);
  event_type = ltt_event_eventtype(e);
  cpu_id = ltt_event_cpu_id(e);
  GString * detail_event = g_string_new("");
  
  if ((ltt_time_compare(event_time,disk_performance->time_window.start_time) == TRUE) &&    
     (ltt_time_compare(disk_performance->time_window.end_time,event_time) == TRUE)){
     if (strcmp( g_quark_to_string(ltt_eventtype_name(event_type)),"block_read") == 0) {    	
	   get_event_detail(e, field, detail_event, &block_read); 
	   diskname = major_minor_to_diskname(&block_read);
	   sum_data(diskname, block_read.size,LTTV_READ_OPERATION, disk_array);
	    
     }
     if (strcmp( g_quark_to_string(ltt_eventtype_name(event_type)),"block_write") == 0) {    	
          get_event_detail(e, field, detail_event, &block_write); 
	  diskname = major_minor_to_diskname(&block_write);
	  sum_data(diskname, block_write.size,LTTV_WRITE_OPERATION, disk_array);
     }
	   
  }
   g_string_free(detail_event, TRUE);
   return FALSE;
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

static void init()
{
  
  g_info("Init diskPerformance.c");
 
 lttvwindow_register_constructor("diskperformance",
                                  "/",
                                  "Insert Disk Performance",
                                  hDiskPerformanceInsert_xpm,
                                  "Insert Disk Performance",
                                  disk_performance);
  
}


static void destroy()
{
  g_info("Destroy diskPerformance");
  g_slist_foreach(g_disk_data_list, disk_destroy_walk, NULL );
  g_slist_free(g_disk_data_list);
  
  lttvwindow_unregister_constructor(disk_performance);
  
}


LTTV_MODULE("diskperformance", "disk info view", \
	    "Produce disk I/O performance", \
	    init, destroy, "lttvwindow") 
	    
