/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Parisa heidari (inspired from CFV by Mathieu Desnoyers)
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


/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


/* Event hooks are the drawing hooks called during traceset read. They draw the
 * icons, text, lines and background color corresponding to the events read.
 *
 * Two hooks are used for drawing : before_schedchange and after_schedchange hooks. The
 * before_schedchange is called before the state update that occurs with an event and
 * the after_schedchange hook is called after this state update.
 *
 * The before_schedchange hooks fulfill the task of drawing the visible objects that
 * corresponds to the data accumulated by the after_schedchange hook.
 *
 * The after_schedchange hook accumulates the data that need to be shown on the screen
 * (items) into a queue. Then, the next before_schedchange hook will draw what that
 * queue contains. That's the Right Way (TM) of drawing items on the screen,
 * because we need to draw the background first (and then add icons, text, ...
 * over it), but we only know the length of a background region once the state
 * corresponding to it is over, which happens to be at the next before_schedchange
 * hook.
 *
 * We also have a hook called at the end of a chunk to draw the information left
 * undrawn in each process queue. We use the current time as end of
 * line/background.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#define PANGO_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

//#include <pango/pango.h>

#include <ltt/event.h>
#include <ltt/time.h>
#include <ltt/trace.h>

#include <lttv/lttv.h>
#include <lttv/hook.h>
#include <lttv/state.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/support.h>


#include "histoeventhooks.h"
#include "histocfv.h"
#include "histobuttonwidget.h"
#include "histodrawing.h"


#define MAX_PATH_LEN 256
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
//FIXME
// fixed #define TRACE_NUMBER 0
#define EXTRA_ALLOC 1024 // pixels

/* Action to do when background computation completed.
 *
 * Wait for all the awaited computations to be over.
 */

static gint histo_background_ready(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData *)hook_data;
  LttvTrace *trace = (LttvTrace*)call_data;

  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  histocontrol_flow_data->background_info_waiting--;
  
  if(histocontrol_flow_data->background_info_waiting == 0) {
    g_message("Histocontrol flow viewer : background computation data ready.");

    histo_drawing_clear(drawing,0,drawing->width);
    
    gtk_widget_set_size_request(drawing->drawing_area,
                -1, -1);
    histo_redraw_notify(histocontrol_flow_data, NULL);
  }

  return 0;
}


/* Request background computation. Verify if it is in progress or ready first.
 * Only for each trace in the tab's traceset.
 */
static void histo_request_background_data(HistoControlFlowData *histocontrol_flow_data)
{
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(histocontrol_flow_data->tab);
  gint num_traces = lttv_traceset_number(tsc->ts);
  gint i;
  LttvTrace *trace;
  LttvTraceState *tstate;

  LttvHooks *histo_background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(histo_background_ready_hook, histo_background_ready, histocontrol_flow_data,
      LTTV_PRIO_DEFAULT);
  histocontrol_flow_data->background_info_waiting = 0;
  
  for(i=0;i<num_traces;i++) {
    trace = lttv_traceset_get(tsc->ts, i);
    tstate = LTTV_TRACE_STATE(tsc->traces[i]);

    if(lttvwindowtraces_get_ready(g_quark_from_string("state"),trace)==FALSE
        && !tstate->has_precomputed_states) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("state"),
                                          trace) == FALSE) {
        /* We first remove requests that could have been done for the same
         * information. Happens when two viewers ask for it before servicing
         * starts.
         */
        if(!lttvwindowtraces_background_request_find(trace, "state"))
          lttvwindowtraces_background_request_queue(
              main_window_get_widget(histocontrol_flow_data->tab), trace, "state");
        lttvwindowtraces_background_notify_queue(histocontrol_flow_data,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 histo_background_ready_hook);
        histocontrol_flow_data->background_info_waiting++;
      } else { /* in progress */
      
        lttvwindowtraces_background_notify_current(histocontrol_flow_data,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   histo_background_ready_hook);
        histocontrol_flow_data->background_info_waiting++;
      }
    } else {
      /* Data ready. Be its nature, this viewer doesn't need to have
       * its data ready hook called there, because a background
       * request is always linked with a redraw.
       */
    }
    
  }

  lttv_hooks_destroy(histo_background_ready_hook);
}

/**
 * Histogram Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param tab A pointer to the parent tab.
 * @return The widget created.
 */
GtkWidget *
h_guihistocontrolflow(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  g_info("h_guihistocontrolflow, %p", ptab);
  HistoControlFlowData *histocontrol_flow_data = guihistocontrolflow(ptab) ;
  
  Tab *tab = ptab->tab;
  histocontrol_flow_data->tab = tab;
  
  // Unreg done in the GuiHistoControlFlow_Destructor
  lttvwindow_register_traceset_notify(tab,
        histo_traceset_notify,
        histocontrol_flow_data);
    
  lttvwindow_register_time_window_notify(tab,
                                         histo_update_time_window_hook,
                                         histocontrol_flow_data);
  lttvwindow_register_current_time_notify(tab,
                                          histo_update_current_time_hook,
                                          histocontrol_flow_data);
  lttvwindow_register_redraw_notify(tab,
                                    histo_redraw_notify,
                                    histocontrol_flow_data);
  lttvwindow_register_continue_notify(tab,
                                      histo_continue_notify,
                                      histocontrol_flow_data);
  //added for histogram, enable filter:
  lttvwindow_register_filter_notify(tab,
                histo_filter_changed,histocontrol_flow_data );
  histocontrol_flow_data->histo_main_win_filter = lttvwindow_get_filter(tab);

//  histo_request_background_data(histocontrol_flow_data);
 
  return guihistocontrolflow_get_widget(histocontrol_flow_data) ;
  
}

 

/// added for histogram.
void histo_request_event( HistoControlFlowData *histocontrol_flow_data, guint x, guint width)
{
  if(width < 0) return ;
  
  guint i, nb_trace;
  Tab *tab = histocontrol_flow_data->tab;
  TimeWindow time_window = lttvwindow_get_time_window( tab );
  LttTime time_start, time_end;

  LttvTraceState *ts;
  
  //find the tracehooks 
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(tab);
  
  LttvTraceset *traceset = tsc->ts;
  nb_trace = lttv_traceset_number(traceset);
  guint drawing_width= histocontrol_flow_data->drawing->width;
//start time for chunk.
  histo_convert_pixels_to_time(drawing_width, /*0*/x, time_window,
        &time_start);
//end time for chunk.
  histo_convert_pixels_to_time(drawing_width, 
        /*width*/x+width,time_window,
	&time_end);
  time_end = ltt_time_add(time_end, ltt_time_one); // because main window
                                                   // doesn't deliver end time.
 
  lttvwindow_events_request_remove_all(tab,
                                       histocontrol_flow_data);

 
  // LttvHooksById *histo_event_by_id = lttv_hooks_by_id_new();//if necessary for filter!
  // FIXME : eventually request for more traces 
  // fixed for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++) {
  for(i=0;i<nb_trace;i++) {
	//should be in the loop or before? 
	EventsRequest *histo_events_request = g_new(EventsRequest, 1);
  	
  	LttvHooks  *histo_before_trace_hooks = lttv_hooks_new();
	lttv_hooks_add(histo_before_trace_hooks, histo_before_trace,
		       histo_events_request, LTTV_PRIO_DEFAULT);
  
  	LttvHooks *histo_count_event_hooks = lttv_hooks_new();
	lttv_hooks_add(histo_count_event_hooks, histo_count_event,
		       histo_events_request, LTTV_PRIO_DEFAULT);
 
  	LttvHooks *histo_after_trace_hooks = lttv_hooks_new();
	lttv_hooks_add(histo_after_trace_hooks, histo_after_trace, 
		       histo_events_request, LTTV_PRIO_DEFAULT);
  
	//for chunk:
        LttvHooks *histo_before_chunk_traceset = lttv_hooks_new();
        LttvHooks *histo_after_chunk_traceset  = lttv_hooks_new();

 	lttv_hooks_add(histo_before_chunk_traceset,
                     histo_before_chunk,
                     histo_events_request,
                     LTTV_PRIO_DEFAULT);

        lttv_hooks_add(histo_after_chunk_traceset,
                     histo_after_chunk,
                     histo_events_request,
                     LTTV_PRIO_DEFAULT);
  	ts = (LttvTraceState *)tsc->traces[i];
      // Fill the events request
  	histo_events_request->owner       	    = histocontrol_flow_data; 
  	histo_events_request->viewer_data	    = histocontrol_flow_data; 
  	histo_events_request->servicing   	    = FALSE;     
  	histo_events_request->start_time  	    = time_start;//time_window.start_time;
	  
  	histo_events_request->start_position  	    = NULL;
  	histo_events_request->stop_flag	   	    = FALSE;
  	histo_events_request->end_time 	   	    = time_end;//time_window.end_time;
	
  	histo_events_request->num_events  	    = G_MAXUINT;      
  	histo_events_request->end_position          = NULL; 
  	histo_events_request->trace 	   	    = i;    
  	histo_events_request->hooks 	   	    = NULL; 
  	histo_events_request->before_chunk_traceset = histo_before_chunk_traceset;//NULL; 
  	histo_events_request->before_chunk_trace    = NULL; 
  	histo_events_request->before_chunk_tracefile= NULL; 
  	histo_events_request->event 		    = histo_count_event_hooks; 
   	histo_events_request->event_by_id_channel   = NULL;//histo_event_by_id;//NULL; 
  	histo_events_request->after_chunk_tracefile = NULL; 
  	histo_events_request->after_chunk_trace     = NULL;   
  	histo_events_request->after_chunk_traceset  = histo_after_chunk_traceset;//NULL; 
  	histo_events_request->before_request	    = histo_before_trace_hooks; 
  	histo_events_request->after_request	    = histo_after_trace_hooks; 
  
  	lttvwindow_events_request(histocontrol_flow_data->tab, histo_events_request);
  }
return;
}

//hook,added for histogram
int histo_count_event(void *hook_data, void *call_data){

  guint x;//time to pixel
  guint i;// number of events
  LttTime  event_time; 
  LttEvent *e;
  guint *element;
   
  EventsRequest *events_request = (EventsRequest*)hook_data;
  HistoControlFlowData *histocontrol_flow_data = events_request->viewer_data;
  
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  int width = drawing->width;  
  
  g_info("Histogram: count_event() \n");
  
   
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *histo_filter = histocontrol_flow_data->histo_main_win_filter;
  if(histo_filter != NULL && histo_filter->head != NULL)
    if(!lttv_filter_tree_parse(histo_filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  TimeWindow time_window  =  lttvwindow_get_time_window(histocontrol_flow_data->tab);
  event_time = ltt_event_time(e);
  
  histo_convert_time_to_pixels(
          time_window,
          event_time,
          width,
          &x);
  element = &g_array_index(histocontrol_flow_data->number_of_process, guint, x);
  (*element)++;

  return 0;
}
///befor hook:Added for histogram
int histo_before_trace(void *hook_data, void *call_data){

  EventsRequest *events_request = (EventsRequest*)hook_data;
  HistoControlFlowData *histocontrol_flow_data = events_request->viewer_data;
    
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  
//in order to reset all of the array elements.
  guint i,end;
  end = MIN(histocontrol_flow_data->number_of_process->len,drawing->damage_end);
  for(i=drawing->damage_begin/*0*/; 
	i < end/*histocontrol_flow_data->number_of_process->len*/;i++) 
  {
    g_array_index(histocontrol_flow_data->number_of_process, guint, i) = 0;
  }
  histo_drawing_clear(drawing,drawing->damage_begin/*0*/,
		drawing->damage_end - drawing->damage_begin/*drawing->width*/);
  //g_array_free(histocontrol_flow_data->number_of_process,TRUE);
  //histocontrol_flow_data->number_of_process =g_array_new (FALSE,
  //                                           TRUE,
  //                                           sizeof(guint));//4 byte for guint
  //g_array_set_size (histocontrol_flow_data->number_of_process,
  //                                           drawing->drawing_area->allocation.width);
//  gtk_widget_set_size_request(drawing->drawing_area,-1,-1);
  gtk_widget_queue_draw(drawing->drawing_area);
   return 0;
}
//after hook,added for histogram
int histo_after_trace(void *hook_data, void *call_data){

  EventsRequest *events_request = (EventsRequest*)hook_data;
  HistoControlFlowData *histocontrol_flow_data = events_request->viewer_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  guint x, x_end, width;
  LttTime end_time = events_request->end_time;
  TimeWindow time_window = 
        lttvwindow_get_time_window(histocontrol_flow_data->tab);

  g_debug("histo after trace");
  
  histo_convert_time_to_pixels(
        time_window,
        end_time,
        drawing->width,
        &x_end);
  x = drawing->damage_begin;
  width = x_end - x;
  drawing->damage_begin = x+width;
  histogram_show (histocontrol_flow_data,x,x_end);
  
   return 0;
}

void histogram_show(HistoControlFlowData *histocontrol_flow_data,guint draw_begin,
		    guint draw_end)
{
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  GtkWidget *drawingarea= histo_drawing_get_drawing_area(drawing);
  guint width = drawing->width;
  guint height= drawing->height;//drawingarea->allocation.height;
  
 /* gdk_gc_set_line_attributes(drawing->gc,
                               2,
                               GDK_LINE_SOLID,
                               GDK_CAP_BUTT,
                               GDK_JOIN_MITER);*/
//clean the area!
  histo_drawing_clear(drawing,draw_begin,draw_end);
  LttTime t1,t2;
  TimeWindow time_window =
              lttvwindow_get_time_window(histocontrol_flow_data->tab);
     
  guint val,h_val;
  
  guint i,line_src,line_end;
  guint end_chunk=MIN(draw_end,(histocontrol_flow_data->number_of_process)->len);
  
  for (i=draw_begin/*0*/;i<end_chunk/* (histocontrol_flow_data->number_of_process)->len*/;i++){
  	val=g_array_index(histocontrol_flow_data->number_of_process,guint,i);
  	h_val= height-((height*val)/histocontrol_flow_data->max_height);
	
	histo_convert_pixels_to_time(width, i,
        	time_window,
        	&t1);
  	histo_convert_pixels_to_time(width, i+1,
        	time_window,
        	&t2);
	line_src=i;
	
//check if zoom in is used and more than 1 pixel correspond to each 1nsec
//used for drawing point (not line) on the screen.
/*	while (ltt_time_compare(t1,t2)==0)
	{
		histo_convert_pixels_to_time(width, i++,
        	time_window,
        	&t1);
  		histo_convert_pixels_to_time(width, i+1,
        	time_window,
        	&t2);
		
	
	}//while (t1==t2)
*/ //replaced later for lines.

   if(val > drawing->histo_control_flow_data->max_height){
	//overlimit, yellow color		
	gdk_gc_set_foreground(drawing->gc,&histo_drawing_colors[COL_WHITE] );//COL_RUN_TRAP
        gdk_draw_line (drawing->pixmap,
                              drawing->gc,
                              i/*line_src*/,1,
                              i,/*1*/height);
   }
   else{
	gdk_gc_set_foreground(drawing->gc,&histo_drawing_colors[COL_RUN_USER_MODE] );
	gdk_draw_line (drawing->pixmap,
                              drawing->gc,
                             i/*line_src*/,h_val,
                              i,/*h_val*/height);
  	}

  while ((ltt_time_compare(t1,t2)==0)&&(i<end_chunk))//-1 , i to be incremented later 
   {////
   	i++;
      		
   	if(val > drawing->histo_control_flow_data->max_height){
		//overlimit, yellow color		
		gdk_gc_set_foreground(drawing->gc,
			&histo_drawing_colors[COL_RUN_TRAP] );
        	gdk_draw_line (drawing->pixmap,
                              drawing->gc,
                              i,1,
                              i,height);
   	}
   	else{
		gdk_gc_set_foreground(drawing->gc,&histo_drawing_colors[COL_RUN_USER_MODE] );
		gdk_draw_line (drawing->pixmap,
                              drawing->gc,
                              i,h_val,
                              i,height);
	}
	histo_convert_pixels_to_time(width, i,
        	time_window,
        	&t1);
	if(i<end_chunk-1){
  		histo_convert_pixels_to_time(width, i+1,
        		time_window,
        		&t2);
      	}
    }//while (t1==t2)////
     
  }
   
   histo_drawing_update_vertical_ruler(drawing);
   gtk_widget_queue_draw_area ( drawing->drawing_area,
                               draw_begin, 0,
                               draw_end-draw_begin, drawing->height);
   gdk_window_process_updates(drawingarea->window,TRUE);
}

int histo_event_selected_hook(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*) hook_data;
  guint *event_number = (guint*) call_data;

  g_debug("DEBUG : event selected by main window : %u", *event_number);
  
  return 0;
}



/* histo_before_schedchange_hook
 * 
 * This function basically draw lines and icons. Two types of lines are drawn :
 * one small (3 pixels?) representing the state of the process and the second
 * type is thicker (10 pixels?) representing on which CPU a process is running
 * (and this only in running state).
 *
 * Extremums of the lines :
 * x_min : time of the last event context for this process kept in memory.
 * x_max : time of the current event.
 * y : middle of the process in the process list. The process is found in the
 * list, therefore is it's position in pixels.
 *
 * The choice of lines'color is defined by the context of the last event for this
 * process.
 */

/*
int histo_before_schedchange_hook(void *hook_data, void *call_data)
{
   return 0;
}
*/
 
gint histo_update_time_window_hook(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*) hook_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  
  const TimeWindowNotifyData *histo_time_window_nofify_data = 
                          ((const TimeWindowNotifyData *)call_data);

  TimeWindow *histo_old_time_window = 
    histo_time_window_nofify_data->old_time_window;
  TimeWindow *histo_new_time_window = 
    histo_time_window_nofify_data->new_time_window;
  
  // Update the ruler 
  histo_drawing_update_ruler(drawing,
                       histo_new_time_window);

  /* Two cases : zoom in/out or scrolling */
  
  /* In order to make sure we can reuse the old drawing, the scale must
   * be the same and the new time interval being partly located in the
   * currently shown time interval. (reuse is only for scrolling)
   */

  g_info("Old time window HOOK : %lu, %lu to %lu, %lu",
      histo_old_time_window->start_time.tv_sec,
      histo_old_time_window->start_time.tv_nsec,
      histo_old_time_window->time_width.tv_sec,
      histo_old_time_window->time_width.tv_nsec);

  g_info("New time window HOOK : %lu, %lu to %lu, %lu",
      histo_new_time_window->start_time.tv_sec,
      histo_new_time_window->start_time.tv_nsec,
      histo_new_time_window->time_width.tv_sec,
      histo_new_time_window->time_width.tv_nsec);

 //For Histo,redraw always except if zoom fit is pushed 2 times consequently
 if( histo_new_time_window->start_time.tv_sec == histo_old_time_window->start_time.tv_sec
  && histo_new_time_window->start_time.tv_nsec == histo_old_time_window->start_time.tv_nsec
  && histo_new_time_window->time_width.tv_sec == histo_old_time_window->time_width.tv_sec
  && histo_new_time_window->time_width.tv_nsec == histo_old_time_window->time_width.tv_nsec)
  {
	return 0;  
  }   
  histo_rectangle_pixmap (drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          drawing->width,//+SAFETY, // do not overlap
          -1,drawing);

    drawing->damage_begin = 0;
    drawing->damage_end = drawing->width;

    gtk_widget_queue_draw(drawing->drawing_area);
    histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
  
  gdk_window_process_updates(drawing->drawing_area->window,TRUE);

//show number of event at current time 

  histo_drawing_update_vertical_ruler(drawing);

#if 0

/*//  if( histo_new_time_window->time_width.tv_sec == histo_old_time_window->time_width.tv_sec
  && histo_new_time_window->time_width.tv_nsec == histo_old_time_window->time_width.tv_nsec)
  {
    // Same scale (scrolling) 
    g_info("scrolling");
    /* For histogram,
      while scrolling no matter far or near , 
      right or left it's necessary to redraw whole screen!*/
/*//    LttTime *ns = &histo_new_time_window->start_time;
    LttTime *nw = &histo_new_time_window->time_width;
    LttTime *os = &histo_old_time_window->start_time;
    LttTime *ow = &histo_old_time_window->time_width;
    LttTime histo_old_end = histo_old_time_window->end_time;
    LttTime histo_new_end = histo_new_time_window->end_time;
    //if(ns<os+w<ns+w)
    //if(ns<os+w && os+w<ns+w)
    //if(ns<histo_old_end && os<ns)

    //added for histogram
    gtk_widget_queue_draw(drawing->drawing_area);

          drawing->damage_begin = 0;
          drawing->damage_end = drawing->width;

      //replaced for hisogram
      histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
/*  
    if(ltt_time_compare(*ns, histo_old_end) == -1
        && ltt_time_compare(*os, *ns) == -1)
    {
      g_info("scrolling near right");
      // Scroll right, keep right part of the screen 
      guint x = 0;
      guint width = drawing->width;
      histo_convert_time_to_pixels(
          *histo_old_time_window,
          *ns,
          width,
          &x);

      // Copy old data to new location 
      //replaced for histogram:
	histo_copy_pixmap_region(drawing,NULL,
    		drawing->drawing_area->style->black_gc,//drawing->gc,
	 	NULL,
    		x, 0,
    		0, 0, (drawing->width-x)
		, -1);
  
    if(drawing->damage_begin == drawing->damage_end)
        drawing->damage_begin = drawing->width-x;
      else
        drawing->damage_begin = 0;

      drawing->damage_end = drawing->width;

//(histo) copy corresponding array region too:
  guint i;
  
  for(i=0; i < histocontrol_flow_data->number_of_process->len-x;i++) 
  {
      g_array_index(histocontrol_flow_data->number_of_process, guint, i) = 
         g_array_index(histocontrol_flow_data->number_of_process, guint, i+x);
  }

      // Clear the data request background, but not SAFETY 
 

//not necessary for histo, because in before chunk ,it clears the area
/*	histo_rectangle_pixmap (
	drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1,drawing);
*/
 /*     gtk_widget_queue_draw(drawing->drawing_area);
      //gtk_widget_queue_draw_area (drawing->drawing_area,
      //                          0,0,
      //                          histocontrol_flow_data->drawing->width,
      //                          histocontrol_flow_data->drawing->height);

    // Get new data for the rest.
    //replaced for hisogram 
      histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
    } else { 
      //if(ns<os<ns+w)
      //if(ns<os && os<ns+w)
      //if(ns<os && os<histo_new_end)
      if(ltt_time_compare(*ns,*os) == -1
          && ltt_time_compare(*os,histo_new_end) == -1)
      {
        g_info("scrolling near left");
        // Scroll left, keep left part of the screen 
        guint x = 0;
        guint width = drawing->width;
        histo_convert_time_to_pixels(
            *histo_new_time_window,
            *os,
            width,
            &x);
        
        // Copy old data to new location 
	//replaced for histogram

  	histo_copy_pixmap_region(drawing,NULL,
    		drawing->drawing_area->style->black_gc,//drawing->gc,
	 	NULL,
    		0, 0,
    		x, 0, -1, -1);
	//(histo) copy corresponding array region too:
  	guint i;
  	for(i=histocontrol_flow_data->number_of_process->len; i > x-1;i--) 
  	{
     	 g_array_index(histocontrol_flow_data->number_of_process, guint, i) = 
         g_array_index(histocontrol_flow_data->number_of_process, guint, i-x);
  	}

       if(drawing->damage_begin == drawing->damage_end)
          drawing->damage_end = x;
        else
          drawing->damage_end = 
            drawing->width;

        drawing->damage_begin = 0;

        
//not necessary for histo, because in before chunk ,it clears the area
  /*      histo_rectangle_pixmap (drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1,drawing);
*/
 /*       gtk_widget_queue_draw(drawing->drawing_area);
        //gtk_widget_queue_draw_area (drawing->drawing_area,
        //                        0,0,
        //                        histocontrol_flow_data->drawing->width,
        //                        histocontrol_flow_data->drawing->height);


        // Get new data for the rest. 

//replaced for hisogram
      histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
    
      } else {
        if(ltt_time_compare(*ns,*os) == 0)
        {
          g_info("not scrolling");
        } else {
          g_info("scrolling far");
          // Cannot reuse any part of the screen : far jump 
          
          //not necessary for histo, because in before chunk ,it clears the area
 /*         histo_rectangle_pixmap (histocontrol_flow_data->drawing->drawing_area->style->black_gc,
            TRUE,
            0, 0,
            histocontrol_flow_data->drawing->width,//+SAFETY, // do not overlap
            -1,drawing);
*/
          //gtk_widget_queue_draw_area (drawing->drawing_area,
          //                      0,0,
          //                      histocontrol_flow_data->drawing->width,
          //                      histocontrol_flow_data->drawing->height);
/*          gtk_widget_queue_draw(drawing->drawing_area);

          drawing->damage_begin = 0;
          drawing->damage_end = histocontrol_flow_data->drawing->width;
/*
          histo_drawing_data_request(histocontrol_flow_data->drawing,
              0, 0,
              histocontrol_flow_data->drawing->width,
              histocontrol_flow_data->drawing->height);*/
      //replaced for hisogram
 /*     histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
        }
      }
    }
  } else {
    // Different scale (zoom) 
    g_info("zoom");

 //not necessary for histo, because in before chunk ,it clears the area
 /*
    histo_rectangle_pixmap (drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          histocontrol_flow_data->drawing->width+SAFETY, // do not overlap
          -1,drawing);
*/
    //gtk_widget_queue_draw_area (drawing->drawing_area,
    //                            0,0,
    //                            histocontrol_flow_data->drawing->width,
    //                            histocontrol_flow_data->drawing->height);
/*//    gtk_widget_queue_draw(drawing->drawing_area);
  
    drawing->damage_begin = 0;
    drawing->damage_end = drawing->width;

  //replaced for hisogram
   histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			drawing->damage_end- drawing->damage_begin);
  }

  // Update directly when scrolling 
  gdk_window_process_updates(drawing->drawing_area->window,
      TRUE);

  //show number of event at current time 

  histo_drawing_update_vertical_ruler(drawing);
*/
#endif

//disabled for histogram, always redraw whole screen. 
  return 0;
}

gint histo_traceset_notify(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*) hook_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;

  if(unlikely(drawing->gc == NULL)) {
    return FALSE;
  }
  if(drawing->dotted_gc == NULL) {
    return FALSE;
  }

  histo_drawing_clear(drawing,0,drawing->width);
  
  guint i;
  for(i=0;i < histocontrol_flow_data->number_of_process->len;i++) 
  {
    g_array_index(histocontrol_flow_data->number_of_process, guint, i) = 0;
  }
  gtk_widget_set_size_request(
      drawing->drawing_area,
                -1, -1);
  histo_redraw_notify(histocontrol_flow_data, NULL);

  ///histo_request_background_data(histocontrol_flow_data);
 
  return FALSE;
}

gint histo_redraw_notify(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*) hook_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  drawing->damage_begin = 0;
  drawing->damage_end = drawing->width;

  // fun feature, to be separated someday... 
  
  histo_drawing_clear(drawing,0,drawing->width);
  
  gtk_widget_set_size_request(
      drawing->drawing_area,
                -1, -1);
  // Clear the images
  
  histo_rectangle_pixmap (widget->style->black_gc,
        TRUE,
        0, 0,
        drawing->alloc_width,
        -1,drawing);
  gtk_widget_queue_draw(widget);
  
 
  if(drawing->damage_begin < drawing->damage_end)
  {
    	//replaced for histogram 
  	histo_request_event(histocontrol_flow_data,0,drawing->width);
  }
  

  //gtk_widget_queue_draw_area(drawing->drawing_area,
  //                           0,0,
  //                           drawing->width,
  //                           drawing->height);
  return FALSE;

}


gint histo_continue_notify(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*) hook_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;

  //g_assert(widget->allocation.width == drawing->damage_end);
  
  if(drawing->damage_begin < drawing->damage_end)
  {
  	histo_request_event(histocontrol_flow_data,drawing->damage_begin,
			    drawing->damage_end-drawing->damage_begin);
  }

  return FALSE;
}


gint histo_update_current_time_hook(void *hook_data, void *call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*)hook_data;
  histoDrawing_t *drawing = histocontrol_flow_data->drawing;

  LttTime current_time = *((LttTime*)call_data);
  
  TimeWindow time_window =
            lttvwindow_get_time_window(histocontrol_flow_data->tab);
  
  LttTime time_begin = time_window.start_time;
  LttTime width = time_window.time_width;
  LttTime half_width;
  {
    guint64 time_ll = ltt_time_to_uint64(width);
    time_ll = time_ll >> 1; /* divide by two */
    half_width = ltt_time_from_uint64(time_ll);
  }
  LttTime time_end = ltt_time_add(time_begin, width);

  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(histocontrol_flow_data->tab);
  
  LttTime trace_start = tsc->time_span.start_time;
  LttTime trace_end = tsc->time_span.end_time;
  
  g_info("Histogram: New current time HOOK : %lu, %lu", current_time.tv_sec,
              current_time.tv_nsec);


  
  /* If current time is inside time interval, just move the highlight
   * bar */

  /* Else, we have to change the time interval. We have to tell it
   * to the main window. */
  /* The time interval change will take care of placing the current
   * time at the center of the visible area, or nearest possible if we are
   * at one end of the trace. */
  
  
  if(ltt_time_compare(current_time, time_begin) < 0)
  {
    TimeWindow histo_new_time_window;

    if(ltt_time_compare(current_time,
          ltt_time_add(trace_start,half_width)) < 0)
      time_begin = trace_start;
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    histo_new_time_window.start_time = time_begin;
    histo_new_time_window.time_width = width;
    histo_new_time_window.time_width_double = ltt_time_to_double(width);
    histo_new_time_window.end_time = ltt_time_add(time_begin, width);

    lttvwindow_report_time_window(histocontrol_flow_data->tab, histo_new_time_window);
  }
  else if(ltt_time_compare(current_time, time_end) > 0)
  {
    TimeWindow histo_new_time_window;

    if(ltt_time_compare(current_time, ltt_time_sub(trace_end, half_width)) > 0)
      time_begin = ltt_time_sub(trace_end,width);
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    histo_new_time_window.start_time = time_begin;
    histo_new_time_window.time_width = width;
    histo_new_time_window.time_width_double = ltt_time_to_double(width);
    histo_new_time_window.end_time = ltt_time_add(time_begin, width);

    lttvwindow_report_time_window(histocontrol_flow_data->tab, histo_new_time_window);
    
  }
  gtk_widget_queue_draw(drawing->drawing_area);
  
  /* Update directly when scrolling */
  gdk_window_process_updates(drawing->drawing_area->window,
      TRUE);

  histo_drawing_update_vertical_ruler(drawing);
                         
  return 0;
}

gboolean histo_filter_changed(void * hook_data, void * call_data)
{
  HistoControlFlowData *histocontrol_flow_data = (HistoControlFlowData*)hook_data;
  histoDrawing_t *drawing =histocontrol_flow_data->drawing;

  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(histocontrol_flow_data->tab);

  histocontrol_flow_data->histo_main_win_filter = 
    (LttvFilter*)call_data;
  //get_events(event_viewer_data->vadjust_c->value, event_viewer_data);
  gtk_widget_set_size_request(
      drawing->drawing_area,
                -1, -1);
  drawing->damage_begin = 0;
  drawing->damage_end = drawing->width;

 /* //done in, before request!
  histo_drawing_clear(drawing,0,drawing->width);
  guint i;
  for(i=0;i < histocontrol_flow_data->number_of_process->len;i++) 
  {
    g_array_index(histocontrol_flow_data->number_of_process, guint, i) = 0;
  }*/
 
  histo_request_event(histocontrol_flow_data,0,drawing->width);
  
  return FALSE;
}

typedef struct _histo_ClosureData {
  EventsRequest *events_request;
  LttvTracesetState *tss;
  LttTime end_time;
  guint x_end;
} histo_ClosureData;
  


int histo_before_chunk(void *hook_data, void *call_data)
{
  EventsRequest *histo_events_request = (EventsRequest*)hook_data;
  LttvTracesetState *histo_tss = (LttvTracesetState*)call_data;
  HistoControlFlowData *histo_cfd = (HistoControlFlowData*)histo_events_request->viewer_data;
#if 0  
  /* Desactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(cfd->process_list->list_store),
      TRACE_COLUMN,
      GTK_SORT_ASCENDING);
#endif //0
  histo_drawing_chunk_begin(histo_events_request, histo_tss);

  return 0;
}

/*int histo_before_request(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
 
  histo_drawing_data_request_begin(events_request, tss);

  return 0;
}
*/


/*
 * after request is necessary in addition of after chunk in order to draw 
 * lines until the end of the screen. after chunk just draws lines until
 * the last event.
 * 
 * for each process
 *    draw closing line
 *    expose
 */
/*int histo_after_request(void *hook_data, void *call_data)
{
  return 0;
}
*/
/*
 * for each process
 *    draw closing line
 * expose
 */

int histo_after_chunk(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  HistoControlFlowData *histocontrol_flow_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  LttvTracesetContext *tsc = (LttvTracesetContext*)call_data;
  LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
  LttTime end_time;

  histoDrawing_t *drawing = histocontrol_flow_data->drawing;

  if(!histocontrol_flow_data->chunk_has_begun)
	  return 0;

  histocontrol_flow_data->chunk_has_begun = TRUE;

  if(tfc != NULL)
    end_time = LTT_TIME_MIN(tfc->timestamp, events_request->end_time);
  else /* end of traceset, or position now out of request : end */
    end_time = events_request->end_time;
  
  guint x, x_end, width;
  
  TimeWindow time_window = 
        lttvwindow_get_time_window(histocontrol_flow_data->tab);

  g_debug("histo after chunk");

  histo_convert_time_to_pixels(
        time_window,
        end_time,
        drawing->width,
        &x_end);
  x = drawing->damage_begin;
  width = x_end - x;
  drawing->damage_begin = x+width;

  histogram_show (histocontrol_flow_data,x,x_end);
  
  return 0;
}

