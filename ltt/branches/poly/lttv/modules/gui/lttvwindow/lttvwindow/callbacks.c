/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <ltt/time.h>
#include <ltt/event.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttvwindow/mainwindow.h>
#include <lttvwindow/mainwindow-private.h>
#include <lttvwindow/menu.h>
#include <lttvwindow/toolbar.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/gtkdirsel.h>
#include <lttvwindow/lttvfilter.h>

#define PATH_LENGTH          256
#define DEFAULT_TIME_WIDTH_S   1

extern LttvTrace *g_init_trace ;


/** Array containing instanced objects. */
extern GSList * g_main_window_list;

/** MD : keep old directory. */
static char remember_plugins_dir[PATH_LENGTH] = "";
static char remember_trace_dir[PATH_LENGTH] = "";


MainWindow * get_window_data_struct(GtkWidget * widget);
char * get_unload_module(char ** loaded_module_name, int nb_module);
char * get_remove_trace(char ** all_trace_name, int nb_trace);
char * get_selection(char ** all_name, int nb, char *title, char * column_title);
gboolean get_filter_selection(LttvTracesetSelector *s, char *title, char * column_title);
void * create_tab(MainWindow * mw, Tab *copy_tab,
		  GtkNotebook * notebook, char * label);

static void insert_viewer(GtkWidget* widget, lttvwindow_viewer_constructor constructor);
void update_filter(LttvTracesetSelector *s,  GtkTreeStore *store );

void checkbox_changed(GtkTreeView *treeview,
		      GtkTreePath *arg1,
		      GtkTreeViewColumn *arg2,
		      gpointer user_data);
void remove_trace_from_traceset_selector(GtkMultiVPaned * paned, unsigned i);
void add_trace_into_traceset_selector(GtkMultiVPaned * paned, LttTrace * trace);
void create_new_tab(GtkWidget* widget, gpointer user_data);

LttvTracesetSelector * construct_traceset_selector(LttvTraceset * traceset);

static gboolean lttvwindow_process_pending_requests(Tab *tab);

enum {
  CHECKBOX_COLUMN,
  NAME_COLUMN,
  TOTAL_COLUMNS
};

enum
{
  MODULE_COLUMN,
  N_COLUMNS
};

/* Construct a selector(filter), which will be associated with a viewer,
 * and provides an interface for user to select interested events and traces
 */

LttvTracesetSelector * construct_traceset_selector(LttvTraceset * traceset)
{
  LttvTracesetSelector  * s;
  LttvTraceSelector     * trace;
  LttvTracefileSelector * tracefile;
  LttvEventtypeSelector * eventtype;
  int i, j, k, m;
  int nb_trace, nb_tracefile, nb_control, nb_per_cpu, nb_facility, nb_event;
  LttvTrace * trace_v;
  LttTrace  * t;
  LttTracefile *tf;
  LttFacility * fac;
  LttEventType * et;

  s = lttv_traceset_selector_new(lttv_traceset_name(traceset));
  nb_trace = lttv_traceset_number(traceset);
  for(i=0;i<nb_trace;i++){
    trace_v = lttv_traceset_get(traceset, i);
    t       = lttv_trace(trace_v);
    trace   = lttv_trace_selector_new(t);
    lttv_traceset_selector_trace_add(s, trace);

    nb_facility = ltt_trace_facility_number(t);
    for(k=0;k<nb_facility;k++){
      fac = ltt_trace_facility_get(t,k);
      nb_event = (int) ltt_facility_eventtype_number(fac);
      for(m=0;m<nb_event;m++){
	et = ltt_facility_eventtype_get(fac,m);
	eventtype = lttv_eventtype_selector_new(et);
	lttv_trace_selector_eventtype_add(trace, eventtype);
      }
    }

    nb_control = ltt_trace_control_tracefile_number(t);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(t);
    nb_tracefile = nb_control + nb_per_cpu;

    for(j = 0 ; j < nb_tracefile ; j++) {
      if(j < nb_control)
        tf = ltt_trace_control_tracefile_get(t, j);
      else
        tf = ltt_trace_per_cpu_tracefile_get(t, j - nb_control);     
      tracefile = lttv_tracefile_selector_new(tf);  
      lttv_trace_selector_tracefile_add(trace, tracefile);
      lttv_eventtype_selector_copy(trace, tracefile);
    }
  } 
  return s;
}


/* insert_viewer function constructs an instance of a viewer first,
 * then inserts the widget of the instance into the container of the
 * main window
 */

void
insert_viewer_wrap(GtkWidget *menuitem, gpointer user_data)
{
  guint val = 20;

  insert_viewer((GtkWidget*)menuitem, (lttvwindow_viewer_constructor)user_data);
  //  selected_hook(&val);
}


/* internal functions */
void insert_viewer(GtkWidget* widget, lttvwindow_viewer_constructor constructor)
{
  GtkMultiVPaned * multi_vpaned;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * viewer;
  LttvTracesetSelector  * s;
  TimeInterval * time_interval;
  Tab *tab = mw_data->current_tab;

  if(!tab) create_new_tab(widget, NULL);
  multi_vpaned = tab->multi_vpaned;

  s = construct_traceset_selector(tab->traceset_info->traceset);
  viewer = (GtkWidget*)constructor(tab, s, "Traceset_Selector");
  if(viewer)
  {
    gtk_multi_vpaned_widget_add(multi_vpaned, viewer); 
    // We unref here, because it is now referenced by the multi_vpaned!
    g_object_unref(G_OBJECT(viewer));

    // The viewer will show itself when it receives a show notify
    // So we call the show notify hooks here. It will
    // typically add hooks for reading, we call process trace, and the
    // end of reading hook will call gtk_widget_show and unregister the
    // hooks.
    // Note that show notify gets the time_requested through the call_data.
    //show_viewer(mw_data);
    // in expose now call_pending_read_hooks(mw_data);
  }
}


/* get_label function is used to get user input, it displays an input
 * box, which allows user to input a string 
 */

void get_label_string (GtkWidget * text, gchar * label) 
{
  GtkEntry * entry = (GtkEntry*)text;
  if(strlen(gtk_entry_get_text(entry))!=0)
    strcpy(label,gtk_entry_get_text(entry)); 
}

gboolean get_label(MainWindow * mw, gchar * str, gchar* dialogue_title, gchar * label_str)
{
  GtkWidget * dialogue;
  GtkWidget * text;
  GtkWidget * label;
  gint id;

  dialogue = gtk_dialog_new_with_buttons(dialogue_title,NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 

  label = gtk_label_new(label_str);
  gtk_widget_show(label);

  text = gtk_entry_new();
  gtk_widget_show(text);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), label,TRUE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), text,FALSE, FALSE,0);

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
      get_label_string(text,str);
      gtk_widget_destroy(dialogue);
      break;
    case GTK_RESPONSE_REJECT:
    default:
      gtk_widget_destroy(dialogue);
      return FALSE;
  }
  return TRUE;
}


/* get_window_data_struct function is actually a lookup function,
 * given a widget which is in the tree of the main window, it will
 * return the MainWindow data structure associated with main window
 */

MainWindow * get_window_data_struct(GtkWidget * widget)
{
  GtkWidget * mw;
  MainWindow * mw_data;

  mw = lookup_widget(widget, "MWindow");
  if(mw == NULL){
    g_printf("Main window does not exist\n");
    return NULL;
  }
  
  mw_data = (MainWindow *) g_object_get_data(G_OBJECT(mw),"main_window_data");
  if(mw_data == NULL){
    g_printf("Main window data does not exist\n");
    return NULL;
  }
  return mw_data;
}


/* create_new_window function, just constructs a new main window
 */

void create_new_window(GtkWidget* widget, gpointer user_data, gboolean clone)
{
  MainWindow * parent = get_window_data_struct(widget);

  if(clone){
    g_printf("Clone : use the same traceset\n");
    construct_main_window(parent);
  }else{
    g_printf("Empty : traceset is set to NULL\n");
    construct_main_window(NULL);
  }
}


/* move_*_viewer functions move the selected view up/down in 
 * the current tab
 */

void move_up_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_multi_vpaned_widget_move_up(mw->current_tab->multi_vpaned);
}

void move_down_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_multi_vpaned_widget_move_down(mw->current_tab->multi_vpaned);
}


/* delete_viewer deletes the selected viewer in the current tab
 */

void delete_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_multi_vpaned_widget_delete(mw->current_tab->multi_vpaned);
}


/* open_traceset will open a traceset saved in a file
 * Right now, it is not finished yet, (not working)
 * FIXME
 */

void open_traceset(GtkWidget * widget, gpointer user_data)
{
  char ** dir;
  gint id;
  LttvTraceset * traceset;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkFileSelection * file_selector = 
    (GtkFileSelection *)gtk_file_selection_new("Select a traceset");

  gtk_file_selection_hide_fileop_buttons(file_selector);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (file_selector);
      traceset = lttv_traceset_load(dir[0]);
      g_printf("Open a trace set %s\n", dir[0]); 
      //Not finished yet
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }

}


/* lttvwindow_process_pending_requests
 * 
 * This internal function gets called by g_idle, taking care of the pending
 * requests. It is responsible for concatenation of time intervals and position
 * requests. It does it with the following algorithm organizing process traceset
 * calls. Here is the detailed description of the way it works :
 *
 * - Events Requests Servicing Algorithm
 *
 *   Data structures necessary :
 *
 *   List of requests added to context : list_in
 *   List of requests not added to context : list_out
 *
 *   Initial state :
 *
 *   list_in : empty
 *   list_out : many events requests
 *
 *  FIXME : insert rest of algorithm here
 *
 */


gboolean lttvwindow_process_pending_requests(Tab *tab)
{
  unsigned max_nb_events;
  GdkWindow * win;
  GdkCursor * new;
  GtkWidget* widget;
  LttvTracesetContext *tsc;
  LttvTracefileContext *tfc;
  GSList *events_requests = tab->events_requests;
  GSList *list_out = events_requests;
  GSList *list_in = NULL;
  LttTime end_time;
  guint end_nb_events;
  guint count;
  LttvTracesetContextPosition *end_position;
    
  
  /* Current tab check : if no current tab is present, no hooks to call. */
  /* (Xang Xiu) It makes the expose works..  MD:? */
  if(tab == NULL)
    return FALSE;

  /* There is no events requests pending : we should never have been called! */
  g_assert(g_slist_length(events_requests) != 0);

  tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);

  //set the cursor to be X shape, indicating that the computer is busy in doing its job
  new = gdk_cursor_new(GDK_X_CURSOR);
  widget = lookup_widget(tab->mw->mwindow, "MToolbar1");
  win = gtk_widget_get_parent_window(widget);  
  gdk_window_set_cursor(win, new);
  gdk_cursor_unref(new);  
  gdk_window_stick(win);
  gdk_window_unstick(win);

  g_debug("SIZE events req len  : %d", g_slist_length(events_requests));
  
  /* Events processing algorithm implementation */
  /* A. Servicing loop */
  while( (g_slist_length(list_in) != 0 || g_slist_length(list_out) != 0)
         && !gtk_events_pending() ) {

    /* 1. If list_in is empty (need a seek) */
    if( g_slist_length(list_in) ==  0 ) {

      /* list in is empty, need a seek */
      {
        /* 1.1 Add requests to list_in */
        GSList *ltime = NULL;
        GSList *lpos = NULL;
        GSList *iter = NULL;
        
        /* 1.1.1 Find all time requests with the lowest start time in list_out
         * (ltime)
         */
        if(g_slist_length(list_out) > 0)
          ltime = g_slist_append(ltime, g_slist_nth_data(list_out, 0));
        for(iter=g_slist_nth(list_out,1);iter!=NULL;iter=g_slist_next(iter)) {
          /* Find all time requests with the lowest start time in list_out */
          guint index_ltime = g_array_index(ltime, guint, 0);
          EventsRequest *event_request_ltime = (EventsRequest*)g_slist_nth_data(ltime, 0);
          EventsRequest *event_request_list_out = (EventsRequest*)iter->data;

          int comp;
          comp = ltt_time_compare(event_request_ltime->start_time,
                                  event_request_list_out->start_time);
          if(comp == 0)
            ltime = g_slist_append(ltime, event_request_list_out);
          else if(comp > 0) {
            /* Remove all elements from ltime, and add current */
            while(ltime != NULL)
              ltime = g_slist_delete_link(ltime, g_slist_nth(ltime, 0));
            ltime = g_slist_append(ltime, event_request_list_out);
          }
        }
        
        /* 1.1.2 Find all position requests with the lowest position in list_out
         * (lpos)
         */
        if(g_slist_length(list_out) > 0)
          lpos = g_slist_append(lpos, g_slist_nth_data(list_out, 0));
        for(iter=g_slist_nth(list_out,1);iter!=NULL;iter=g_slist_next(iter)) {
          /* Find all position requests with the lowest position in list_out */
          guint index_lpos = g_array_index(lpos, guint, 0);
          EventsRequest *event_request_lpos = (EventsRequest*)g_slist_nth_data(lpos, 0);
          EventsRequest *event_request_list_out = (EventsRequest*)iter->data;

          int comp;
          if(event_request_lpos->start_position != NULL
              && event_request_list_out->start_position != NULL)
          {
            comp = lttv_traceset_context_pos_pos_compare
                                 (event_request_lpos->start_position,
                                  event_request_list_out->start_position);
          } else {
            comp = -1;
          }
          if(comp == 0)
            lpos = g_slist_append(lpos, event_request_list_out);
          else if(comp > 0) {
            /* Remove all elements from lpos, and add current */
            while(lpos != NULL)
              lpos = g_slist_delete_link(lpos, g_slist_nth(lpos, 0));
            lpos = g_slist_append(lpos, event_request_list_out);
          }
        }
        
        /* 1.1.3 If lpos.start time < ltime */
        {
          EventsRequest *event_request_lpos = (EventsRequest*)g_slist_nth_data(lpos, 0);
          EventsRequest *event_request_ltime = (EventsRequest*)g_slist_nth_data(ltime, 0);
          LttTime lpos_start_time;
          
          if(event_request_lpos != NULL 
              && event_request_lpos->start_position != NULL) {

            lpos_start_time = lttv_traceset_context_position_get_time(
                                      event_request_lpos->start_position);
            if(ltt_time_compare(lpos_start_time,
                                event_request_ltime->start_time)<0) {
              /* Add lpos to list_in, remove them from list_out */
              
              for(iter=lpos;iter!=NULL;iter=g_slist_next(iter)) {
                /* Add to list_in */
                EventsRequest *event_request_lpos = 
                                      (EventsRequest*)iter->data;

                g_slist_append(list_in, event_request_lpos);
                /* Remove from list_out */
                g_slist_remove(list_out, event_request_lpos);
              }
            }
          } else {
            /* 1.1.4 (lpos.start time >= ltime) */
            /* Add ltime to list_in, remove them from list_out */

            for(iter=ltime;iter!=NULL;iter=g_slist_next(iter)) {
              /* Add to list_in */
              EventsRequest *event_request_ltime = 
                                    (EventsRequest*)iter->data;

              g_slist_append(list_in, event_request_ltime);
              /* Remove from list_out */
              g_slist_remove(list_out, event_request_ltime);
            }
          }
        }
        g_slist_free(lpos);
        g_slist_free(ltime);
      }

      /* 1.2 Seek */
      {
        tfc = lttv_traceset_context_get_current_tfc(tsc);
        g_assert(g_slist_length(list_in)>0);
        EventsRequest *events_request = g_slist_nth_data(list_in, 0);

        /* 1.2.1 If first request in list_in is a time request */
        if(events_request->start_position == NULL) {
          /* - If first req in list_in start time != current time */
          if(tfc != NULL && ltt_time_compare(events_request->start_time,
                              tfc->timestamp) != 0)
            /* - Seek to that time */
            lttv_process_traceset_seek_time(tsc, events_request->start_time);
        } else {
          /* Else, the first request in list_in is a position request */
          /* If first req in list_in pos != current pos */
          g_assert(events_request->start_position != NULL);
          if(lttv_traceset_context_ctx_pos_compare(tsc,
                     events_request->start_position) != 0) {
            /* 1.2.2.1 Seek to that position */
            lttv_process_traceset_seek_position(tsc, events_request->start_position);
          }
        }
      }

      /* 1.3 Add hooks and call before request for all list_in members */
      {
        GSList *iter = NULL;

        for(iter=list_in;iter!=NULL;iter=g_slist_next(iter)) {
          EventsRequest *events_request = (EventsRequest*)iter->data;
          /* 1.3.1 If !servicing */
          if(events_request->servicing == FALSE) {
            /* - begin request hooks called
             * - servicing = TRUE
             */
            lttv_hooks_call(events_request->before_request, NULL);
            events_request->servicing = TRUE;
          }
          /* 1.3.2 call before chunk
           * 1.3.3 events hooks added
           */
          lttv_process_traceset_begin(tsc, events_request->before_chunk_traceset,
                                           events_request->before_chunk_trace,
                                           events_request->before_chunk_tracefile,
                                           events_request->event,
                                           events_request->event_by_id);
        }
      }
    } else {
      /* 2. Else, list_in is not empty, we continue a read */
      GSList *iter = NULL;
      tfc = lttv_traceset_context_get_current_tfc(tsc);
    
      /* 2.1 For each req of list_out */
      for(iter=list_out;iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        /* if req.start time == current context time 
         * or req.start position == current position*/
        if(  ltt_time_compare(events_request->start_time,
                            tfc->timestamp) == 0 
           ||
             (events_request->start_position != NULL 
             &&
             lttv_traceset_context_ctx_pos_compare(tsc,
                     events_request->start_position) == 0)
           ) {
          /* - Add to list_in, remove from list_out */
          g_slist_append(list_in, events_request);
          g_slist_remove(list_out, events_request);

          /* - If !servicing */
          if(events_request->servicing == FALSE) {
            /* - begin request hooks called
             * - servicing = TRUE
             */
            lttv_hooks_call(events_request->before_request, NULL);
            events_request->servicing = TRUE;
          }
          /* call before chunk
           * events hooks added
           */
          lttv_process_traceset_begin(tsc, events_request->before_chunk_traceset,
                                           events_request->before_chunk_trace,
                                           events_request->before_chunk_tracefile,
                                           events_request->event,
                                           events_request->event_by_id);
        }
      }
    }

    /* 3. Find end criterions */
    {
      /* 3.1 End time */
      GSList *iter;
      
      /* 3.1.1 Find lowest end time in list_in */
      g_assert(g_slist_length(list_in)>0);
      end_time = ((EventsRequest*)g_slist_nth_data(list_in,0))->end_time;
      
      for(iter=g_slist_nth(list_in,1);iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        if(ltt_time_compare(events_request->end_time,
                            end_time) < 0)
          end_time = events_request->end_time;
      }
       
      /* 3.1.2 Find lowest start time in list_out */
      for(iter=list_out;iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        if(ltt_time_compare(events_request->start_time,
                            end_time) < 0)
          end_time = events_request->start_time;
      }
    }

    {
      /* 3.2 Number of events */

      /* 3.2.1 Find lowest number of events in list_in */
      GSList *iter;

      end_nb_events = ((EventsRequest*)g_slist_nth_data(list_in,0))->num_events;

      for(iter=g_slist_nth(list_in,1);iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        if(events_request->num_events < end_nb_events)
          end_nb_events = events_request->num_events;
      }

      /* 3.2.2 Use min(CHUNK_NUM_EVENTS, min num events in list_in) as
       * num_events */
      
      end_nb_events = MIN(CHUNK_NUM_EVENTS, end_nb_events);
    }

    {
      /* 3.3 End position */

      /* 3.3.1 Find lowest end position in list_in */
      GSList *iter;

      end_position =((EventsRequest*)g_slist_nth_data(list_in,0))->end_position;

      for(iter=g_slist_nth(list_in,1);iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        if(events_request->end_position != NULL && end_position != NULL &&
            lttv_traceset_context_pos_pos_compare(events_request->end_position,
                                                 end_position) <0)
          end_position = events_request->end_position;
      }
    }
    
    {  
      /* 3.3.2 Find lowest start position in list_out */
      GSList *iter;

      for(iter=list_out;iter!=NULL;iter=g_slist_next(iter)) {
        EventsRequest *events_request = (EventsRequest*)iter->data;

        if(events_request->end_position != NULL && end_position != NULL &&
            lttv_traceset_context_pos_pos_compare(events_request->end_position,
                                                 end_position) <0)
          end_position = events_request->end_position;
      }
    }

    {
      /* 4. Call process traceset middle */
      count = lttv_process_traceset_middle(tsc, end_time, end_nb_events, end_position);
    }
    {
      /* 5. After process traceset middle */
      tfc = lttv_traceset_context_get_current_tfc(tsc);

      /* - if current context time > traceset.end time */
      if(tfc == NULL || ltt_time_compare(tfc->timestamp,
                                         tsc->time_span.end_time) > 0) {
        /* - For each req in list_in */
        GSList *iter = list_in;
    
        while(iter != NULL) {

          gboolean remove = FALSE;
          gboolean free_data = FALSE;
          EventsRequest *events_request = (EventsRequest *)iter->data;
          
          /* - Remove events hooks for req
           * - Call end chunk for req
           */
          lttv_process_traceset_end(tsc, events_request->after_chunk_traceset,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id);
          /* - Call end request for req */
          lttv_hooks_call(events_request->after_request, NULL);
          
          /* - remove req from list_in */
          /* Destroy the request */
          remove = TRUE;
          free_data = TRUE;

          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) g_free(remove_iter->data);
            list_in = g_slist_remove_link(list_in, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
        }
      }

      {
        /* 5.1 For each req in list_in */
        GSList *iter = list_in;
    
        while(iter != NULL) {

          gboolean remove = FALSE;
          gboolean free_data = FALSE;
          EventsRequest *events_request = (EventsRequest *)iter->data;
          
          /* - Remove events hooks for req
           * - Call end chunk for req
           */
          lttv_process_traceset_end(tsc, events_request->after_chunk_traceset,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id);

          /* - req.num -= count */
          g_assert(events_request->num_events >= count);
          events_request->num_events -= count;
          
          g_assert(tfc != NULL);
          /* - if req.num == 0
           *   or
           *     current context time > req.end time
           *   or
           *     req.end pos == current pos
           *   or
           *     req.stop_flag == TRUE
           */
          if(   events_request->num_events == 0
              ||
                events_request->stop_flag == TRUE
              ||
                ltt_time_compare(tfc->timestamp,
                                         events_request->end_time) > 0
              ||
                  (events_request->start_position != NULL 
                 &&
                  lttv_traceset_context_ctx_pos_compare(tsc,
                            events_request->start_position) != 0)

              ) {
            /* - Call end request for req
             * - remove req from list_in */
            lttv_hooks_call(events_request->after_request, NULL);
            /* - remove req from list_in */
            /* Destroy the request */
            remove = TRUE;
            free_data = TRUE;
          }
          
          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) g_free(remove_iter->data);
            list_in = g_slist_remove_link(list_in, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
        }
      }
    }
  }

  /* B. When interrupted between chunks */

  {
    GSList *iter = list_in;
    
    /* 1. for each request in list_in */
    while(iter != NULL) {

      gboolean remove = FALSE;
      gboolean free_data = FALSE;
      EventsRequest *events_request = (EventsRequest *)iter->data;
      
      /* 1.1. Use current postition as start position */
      g_free(events_request->start_position);
      lttv_traceset_context_position_save(tsc, events_request->start_position);

      /* 1.2. Remove start time */
      events_request->start_time.tv_sec = G_MAXUINT;
      events_request->start_time.tv_nsec = G_MAXUINT;
      
      /* 1.3. Move from list_in to list_out */
      remove = TRUE;
      free_data = FALSE;
      list_out = g_slist_append(list_out, events_request);

      /* Go to next */
      if(remove)
      {
        GSList *remove_iter = iter;

        iter = g_slist_next(iter);
        if(free_data) g_free(remove_iter->data);
        list_in = g_slist_remove_link(list_in, remove_iter);
      } else { // not remove
        iter = g_slist_next(iter);
      }
    }


  }

  //set the cursor back to normal
  gdk_window_set_cursor(win, NULL);  


  g_assert(g_slist_length(list_in) == 0);
  if( g_slist_length(list_out) == 0 ) {
    /* Put tab's request pending flag back to normal */
    tab->events_request_pending = FALSE;
    return FALSE; /* Remove the idle function */
  }
  return TRUE; /* Leave the idle function */
}



/* add_trace_into_traceset_selector, each instance of a viewer has an associated
 * selector (filter), when a trace is added into traceset, the selector should 
 * reflect the change. The function is used to update the selector 
 */

void add_trace_into_traceset_selector(GtkMultiVPaned * paned, LttTrace * t)
{
  int j, k, m, nb_tracefile, nb_control, nb_per_cpu, nb_facility, nb_event;
  LttvTracesetSelector  * s;
  LttvTraceSelector     * trace;
  LttvTracefileSelector * tracefile;
  LttvEventtypeSelector * eventtype;
  LttTracefile          * tf;
  GtkWidget             * w;
  LttFacility           * fac;
  LttEventType          * et;

  w = gtk_multi_vpaned_get_first_widget(paned);  
  while(w){
    s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
    
    if(s){
      trace   = lttv_trace_selector_new(t);
      lttv_traceset_selector_trace_add(s, trace);

      nb_facility = ltt_trace_facility_number(t);
      for(k=0;k<nb_facility;k++){
	fac = ltt_trace_facility_get(t,k);
	nb_event = (int) ltt_facility_eventtype_number(fac);
	for(m=0;m<nb_event;m++){
	  et = ltt_facility_eventtype_get(fac,m);
	  eventtype = lttv_eventtype_selector_new(et);
	  lttv_trace_selector_eventtype_add(trace, eventtype);
	}
      }
      
      nb_control = ltt_trace_control_tracefile_number(t);
      nb_per_cpu = ltt_trace_per_cpu_tracefile_number(t);
      nb_tracefile = nb_control + nb_per_cpu;
      
      for(j = 0 ; j < nb_tracefile ; j++) {
	if(j < nb_control)
	  tf = ltt_trace_control_tracefile_get(t, j);
	else
	  tf = ltt_trace_per_cpu_tracefile_get(t, j - nb_control);     
	tracefile = lttv_tracefile_selector_new(tf);  
	lttv_trace_selector_tracefile_add(trace, tracefile);
	lttv_eventtype_selector_copy(trace, tracefile);
      }
    }else g_warning("Module does not support filtering\n");

    w = gtk_multi_vpaned_get_next_widget(paned);  
  }
}


static void lttvwindow_add_trace(Tab *tab, LttvTrace *trace_v)
{
  LttvTraceset *traceset = tab->traceset_info->traceset;
  guint i;

  //Keep a reference to the traces so they are not freed.
  for(i=0; i<lttv_traceset_number(traceset); i++)
  {
    LttvTrace * trace = lttv_traceset_get(traceset, i);
    lttv_trace_ref(trace);
  }

  //remove state update hooks
  lttv_state_remove_event_hooks(
     (LttvTracesetState*)tab->traceset_info->traceset_context);

  lttv_context_fini(LTTV_TRACESET_CONTEXT(
          tab->traceset_info->traceset_context));
  g_object_unref(tab->traceset_info->traceset_context);

  lttv_traceset_add(traceset, trace_v);

  /* Create new context */
  tab->traceset_info->traceset_context =
                          g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  lttv_context_init(
            LTTV_TRACESET_CONTEXT(tab->traceset_info->
                                      traceset_context),
            traceset); 
  //add state update hooks
  lttv_state_add_event_hooks(
  (LttvTracesetState*)tab->traceset_info->traceset_context);
  //Remove local reference to the traces.
  for(i=0; i<lttv_traceset_number(traceset); i++)
  {
    LttvTrace * trace = lttv_traceset_get(traceset, i);
    lttv_trace_unref(trace);
  }

  add_trace_into_traceset_selector(tab->multi_vpaned, lttv_trace(trace_v));
}

/* add_trace adds a trace into the current traceset. It first displays a 
 * directory selection dialogue to let user choose a trace, then recreates
 * tracset_context, and redraws all the viewer of the current tab 
 */

void add_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  const char * dir;
  gint id;
  gint i;
  MainWindow * mw_data = get_window_data_struct(widget);
  Tab *tab = mw_data->current_tab;

  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select a trace");
  gtk_dir_selection_hide_fileop_buttons(file_selector);
  
  if(!tab) create_new_tab(widget, NULL);
  
  if(remember_trace_dir[0] != '\0')
    gtk_dir_selection_set_filename(file_selector, remember_trace_dir);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (file_selector);
      strncpy(remember_trace_dir, dir, PATH_LENGTH);
      if(!dir || strlen(dir) == 0){
      	gtk_widget_destroy((GtkWidget*)file_selector);
      	break;
      }
      trace = ltt_trace_open(dir);
      if(trace == NULL) g_critical("cannot open trace %s", dir);
      trace_v = lttv_trace_new(trace);

      lttvwindow_add_trace(tab, trace_v);

      gtk_widget_destroy((GtkWidget*)file_selector);
      
      //update current tab
      //update_traceset(mw_data);

      /* Call the updatetraceset hooks */
      
      traceset = tab->traceset_info->traceset;
      SetTraceset(tab, traceset);
      // in expose now call_pending_read_hooks(mw_data);
      
      //lttvwindow_report_current_time(mw_data,&(tab->current_time));
      break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
}


/* remove_trace_into_traceset_selector, each instance of a viewer has an associated
 * selector (filter), when a trace is remove from traceset, the selector should 
 * reflect the change. The function is used to update the selector 
 */

void remove_trace_from_traceset_selector(GtkMultiVPaned * paned, unsigned i)
{
  LttvTracesetSelector * s;
  LttvTraceSelector * t;
  GtkWidget * w; 
  
  w = gtk_multi_vpaned_get_first_widget(paned);  
  while(w){
    s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
    if(s){
      t = lttv_traceset_selector_trace_get(s,i);
      lttv_traceset_selector_trace_remove(s, i);
      lttv_trace_selector_destroy(t);
    }g_warning("Module dose not support filtering\n");
    w = gtk_multi_vpaned_get_next_widget(paned);  
  }
}


/* remove_trace removes a trace from the current traceset if all viewers in 
 * the current tab are not interested in the trace. It first displays a 
 * dialogue, which shows all traces in the current traceset, to let user choose 
 * a trace, then it checks if all viewers unselect the trace, if it is true, 
 * it will remove the trace,  recreate the traceset_contex,
 * and redraws all the viewer of the current tab. If there is on trace in the
 * current traceset, it will delete all viewers of the current tab
 */

void remove_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  gint i, j, nb_trace;
  char ** name, *remove_trace_name;
  MainWindow * mw_data = get_window_data_struct(widget);
  Tab *tab = mw_data->current_tab;
  LttvTracesetSelector * s;
  LttvTraceSelector * t;
  GtkWidget * w; 
  gboolean selected;
  
  if(!tab) return;
  
  nb_trace =lttv_traceset_number(tab->traceset_info->traceset); 
  name = g_new(char*,nb_trace);
  for(i = 0; i < nb_trace; i++){
    trace_v = lttv_traceset_get(tab->traceset_info->traceset, i);
    trace = lttv_trace(trace_v);
    name[i] = ltt_trace_name(trace);
  }

  remove_trace_name = get_remove_trace(name, nb_trace);

  if(remove_trace_name){
    for(i=0; i<nb_trace; i++){
      if(strcmp(remove_trace_name,name[i]) == 0){
	//unselect the trace from the current viewer
	w = gtk_multi_vpaned_get_widget(tab->multi_vpaned);  
	if(w){
	  s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
	  if(s){
	    t = lttv_traceset_selector_trace_get(s,i);
	    lttv_trace_selector_set_selected(t, FALSE);
	  }

	  //check if other viewers select the trace
	  w = gtk_multi_vpaned_get_first_widget(tab->multi_vpaned);  
	  while(w){
	    s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
	    if(s){
	      t = lttv_traceset_selector_trace_get(s,i);
	      selected = lttv_trace_selector_get_selected(t);
	      if(selected)break;
	    }
	    w = gtk_multi_vpaned_get_next_widget(tab->multi_vpaned);  
	  }
	}else selected = FALSE;

	//if no viewer selects the trace, remove it
	if(!selected){
	  remove_trace_from_traceset_selector(tab->multi_vpaned, i);

	  traceset = tab->traceset_info->traceset;
	  trace_v = lttv_traceset_get(traceset, i);
	  if(lttv_trace_get_ref_number(trace_v) <= 1)
	    ltt_trace_close(lttv_trace(trace_v));

    //Keep a reference to the traces so they are not freed.
    for(j=0; j<lttv_traceset_number(traceset); j++)
    {
      LttvTrace * trace = lttv_traceset_get(traceset, j);
      lttv_trace_ref(trace);
    }

    //remove state update hooks
    lttv_state_remove_event_hooks(
         (LttvTracesetState*)tab->traceset_info->traceset_context);
    lttv_context_fini(LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context));
    g_object_unref(tab->traceset_info->traceset_context);

    
	  lttv_traceset_remove(traceset, i);
    lttv_trace_unref(trace_v);  // Remove local reference
	  if(!lttv_trace_get_ref_number(trace_v))
	     lttv_trace_destroy(trace_v);
    
	  tab->traceset_info->traceset_context =
	    g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    lttv_context_init(
			    LTTV_TRACESET_CONTEXT(tab->
				      traceset_info->traceset_context),traceset);      
      //add state update hooks
    lttv_state_add_event_hooks(
      (LttvTracesetState*)tab->traceset_info->traceset_context);

    //Remove local reference to the traces.
    for(j=0; j<lttv_traceset_number(traceset); j++)
    {
      LttvTrace * trace = lttv_traceset_get(traceset, j);
      lttv_trace_unref(trace);
    }


	  //update current tab
	  //update_traceset(mw_data);
	  if(nb_trace > 1){

      SetTraceset(mw_data, (gpointer)traceset);
  	  // in expose now call_pending_read_hooks(mw_data);

	    //lttvwindow_report_current_time(mw_data,&(tab->current_time));
	  }else{
	    if(tab){
	      while(tab->multi_vpaned->num_children){
		gtk_multi_vpaned_widget_delete(tab->multi_vpaned);
	      }    
	    }	    
	  }
	}
	break;
      }
    }
  }

  g_free(name);
}


/* save will save the traceset to a file
 * Not implemented yet FIXME
 */

void save(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save\n");
}

void save_as(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save as\n");
}


/* zoom will change the time_window of all the viewers of the 
 * current tab, and redisplay them. The main functionality is to 
 * determine the new time_window of the current tab
 */

void zoom(GtkWidget * widget, double size)
{
  TimeInterval *time_span;
  TimeWindow new_time_window;
  LttTime    current_time, time_delta, time_s, time_e, time_tmp;
  MainWindow * mw_data = get_window_data_struct(widget);
  Tab *tab = mw_data->current_tab;
  LttvTracesetContext *tsc;

  if(tab==NULL) return;
  if(size == 1) return;

  tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  time_span = &tsc->time_span;
  new_time_window =  tab->time_window;
  current_time = tab->current_time;
  
  time_delta = ltt_time_sub(time_span->end_time,time_span->start_time);
  if(size == 0){
    new_time_window.start_time = time_span->start_time;
    new_time_window.time_width = time_delta;
  }else{
    new_time_window.time_width = ltt_time_div(new_time_window.time_width, size);
    if(ltt_time_compare(new_time_window.time_width,time_delta) > 0)
    { /* Case where zoom out is bigger than trace length */
      new_time_window.start_time = time_span->start_time;
      new_time_window.time_width = time_delta;
    }
    else
    {
      /* Center the image on the current time */
      g_critical("update is HERE");
      new_time_window.start_time = 
        ltt_time_sub(current_time, ltt_time_div(new_time_window.time_width, 2.0));
      /* If on borders, don't fall off */
      if(ltt_time_compare(new_time_window.start_time, time_span->start_time) <0)
      {
        new_time_window.start_time = time_span->start_time;
      }
      else 
      {
        if(ltt_time_compare(
           ltt_time_add(new_time_window.start_time, new_time_window.time_width),
           time_span->end_time) > 0)
        {
          new_time_window.start_time = 
                  ltt_time_sub(time_span->end_time, new_time_window.time_width);
        }
      }
      
    }

    

    //time_tmp = ltt_time_div(new_time_window.time_width, 2);
    //if(ltt_time_compare(current_time, time_tmp) < 0){
    //  time_s = time_span->startTime;
    //} else {
    //  time_s = ltt_time_sub(current_time,time_tmp);
    //}
    //time_e = ltt_time_add(current_time,time_tmp);
    //if(ltt_time_compare(time_span->startTime, time_s) > 0){
    //  time_s = time_span->startTime;
    //}else if(ltt_time_compare(time_span->endTime, time_e) < 0){
    //  time_e = time_span->endTime;
    //  time_s = ltt_time_sub(time_e,new_time_window.time_width);
    //}
    //new_time_window.start_time = time_s;    
  }

  //lttvwindow_report_time_window(mw_data, &new_time_window);
  //call_pending_read_hooks(mw_data);

  //lttvwindow_report_current_time(mw_data,&(tab->current_time));
  set_time_window(tab, &new_time_window);
  // in expose now call_pending_read_hooks(mw_data);
  gtk_multi_vpaned_set_adjust(tab->multi_vpaned, &new_time_window, FALSE);
}

void zoom_in(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 2);
}

void zoom_out(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 0.5);
}

void zoom_extended(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 0);
}

void go_to_time(GtkWidget * widget, gpointer user_data)
{
  g_printf("Go to time\n");
}

void show_time_frame(GtkWidget * widget, gpointer user_data)
{
  g_printf("Show time frame\n");  
}


/* callback function */

void
on_empty_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)menuitem, user_data, FALSE);
}


void
on_clone_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)menuitem, user_data, TRUE);
}


/* create_new_tab calls create_tab to construct a new tab in the main window
 */

void create_new_tab(GtkWidget* widget, gpointer user_data){
  gchar label[PATH_LENGTH];
  MainWindow * mw_data = get_window_data_struct(widget);

  GtkNotebook * notebook = (GtkNotebook *)lookup_widget(widget, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  
  
  strcpy(label,"Page");
  if(get_label(mw_data, label,"Get the name of the tab","Please input tab's name"))    
    create_tab (mw_data, NULL, notebook, label);
}

void
on_tab_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_new_tab((GtkWidget*)menuitem, user_data);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)menuitem, user_data);
}


void
on_close_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  main_window_destructor(mw_data);  
}


/* remove the current tab from the main window
 */

void
on_close_tab_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gint page_num;
  GtkWidget * notebook;
  GtkWidget * page;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  notebook = lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }

  page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  
  gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);

  if( gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 0 )
    mw_data->current_tab = NULL;
  else {
    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
    mw_data->current_tab =
                     (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }
}


void
on_add_trace_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)menuitem, user_data);
}


void
on_remove_trace_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)menuitem, user_data);
}


void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save((GtkWidget*)menuitem, user_data);
}


void
on_save_as_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)menuitem, user_data);
}


void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_main_quit ();
}


void
on_cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Cut\n");
}


void
on_copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Copye\n");
}


void
on_paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Paste\n");
}


void
on_delete_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Delete\n");
}


void
on_zoom_in_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_out_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_extended_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)menuitem, user_data); 
}


void
on_go_to_time_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)menuitem, user_data); 
}


void
on_show_time_frame_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)menuitem, user_data); 
}


void
on_move_viewer_up_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)menuitem, user_data);
}


void
on_move_viewer_down_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)menuitem, user_data);
}


void
on_remove_viewer_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)menuitem, user_data);
}

void
on_trace_filter_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  LttvTracesetSelector * s;
  Tab *tab = mw_data->current_tab;
  GtkWidget * w;
  
  if(tab == NULL) return ;

  w = gtk_multi_vpaned_get_widget(tab->multi_vpaned);
  
  s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
  if(!s){
    g_printf("There is no viewer yet\n");      
    return;
  }
  if(get_filter_selection(s, "Configure trace and tracefile filter", "Select traces and tracefiles")){
    //FIXME report filter change
    //update_traceset(mw_data);
    //call_pending_read_hooks(mw_data);
    //lttvwindow_report_current_time(mw_data,&(tab->current_time));
  }
}

void
on_trace_facility_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Trace facility selector: %s\n");  
}


/* Dispaly a file selection dialogue to let user select a module, then call
 * lttv_module_load(), finally insert tool button and menu entry in the main window
 * for the loaded  module
 */

void
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  char ** dir;
  gint id;
  char str[PATH_LENGTH], *str1;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  GtkFileSelection * file_selector = (GtkFileSelection *)gtk_file_selection_new("Select a module");
  if(remember_plugins_dir[0] != '\0')
    gtk_file_selection_set_filename(file_selector, remember_plugins_dir);
  gtk_file_selection_hide_fileop_buttons(file_selector);
  
  str[0] = '\0';
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (file_selector);
      strncpy(str,dir[0],PATH_LENGTH);
      strncpy(remember_plugins_dir,dir[0],PATH_LENGTH);
      str1 = strrchr(str,'/');
      if(str1)str1++;
      else{
	str1 = strrchr(str,'\\');
	str1++;
      }
      lttv_module_require(str1, NULL);
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
  g_printf("Load module: %s\n", str);
}


/* Display all loaded modules, let user to select a module to unload
 * by calling lttv_module_unload
 */

void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  int i;
  GPtrArray *name;
  char *unload_module_name;
  guint nb;
  LttvLibrary *library;
  LttvLibraryInfo library_info;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  
  name  = g_ptr_array_new();
  nb = lttv_library_number();

  for(i=0;i<nb;i++){
    library = lttv_library_get(i);
    lttv_library_info(library, &library_info);
    if(library_info.load_count > 0) g_ptr_array_add(name, library_info.name);
  }

  unload_module_name =get_unload_module((char **)(name->pdata), name->len);
  
  if(unload_module_name){
    for(i=0;i<nb;i++){
      library = lttv_library_get(i);
      lttv_library_info(library, &library_info);
      if(strcmp(unload_module_name, library_info.name) == 0){
	      lttv_library_unload(library);
	break;
      }
    }    
  }

  g_ptr_array_free(name, TRUE);
}


/* Display a directory dialogue to let user select a path for module searching
 */

void
on_add_module_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select module path");
  const char * dir;
  gint id;

  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  if(remember_plugins_dir[0] != '\0')
    gtk_dir_selection_set_filename(file_selector, remember_plugins_dir);

  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (file_selector);
      strncpy(remember_plugins_dir,dir,PATH_LENGTH);
      strncat(remember_plugins_dir,"/",PATH_LENGTH);
      lttv_library_path_add(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
}


void
on_color_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Color\n");
}


void
on_filter_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Filter\n");
}


void
on_save_configuration_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Save configuration\n");
}


void
on_content_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Content\n");
}


void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("About...\n");
}


void
on_button_new_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)button, user_data, TRUE);
}

void
on_button_new_tab_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  create_new_tab((GtkWidget*)button, user_data);
}

void
on_button_open_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)button, user_data);
}


void
on_button_add_trace_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)button, user_data);
}


void
on_button_remove_trace_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)button, user_data);
}


void
on_button_save_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  save((GtkWidget*)button, user_data);
}


void
on_button_save_as_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)button, user_data);
}


void
on_button_zoom_in_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)button, user_data); 
}


void
on_button_zoom_out_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)button, user_data); 
}


void
on_button_zoom_extended_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)button, user_data); 
}


void
on_button_go_to_time_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)button, user_data); 
}


void
on_button_show_time_frame_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)button, user_data); 
}


void
on_button_move_up_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)button, user_data);
}


void
on_button_move_down_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)button, user_data);
}


void
on_button_delete_viewer_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)button, user_data);
}

void
on_MWindow_destroy                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
  MainWindow *main_window = get_window_data_struct(widget);
  LttvIAttribute *attributes = main_window->attributes;
  LttvAttributeValue value;
 
  //This is unnecessary, since widgets will be destroyed
  //by the main window widget anyway.
  //remove_all_menu_toolbar_constructors(main_window, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes,
           "viewers/menu", LTTV_POINTER, &value));
  lttv_menus_destroy((LttvMenus*)*(value.v_pointer));

  g_assert(lttv_iattribute_find_by_path(attributes,
           "viewers/toolbar", LTTV_POINTER, &value));
  lttv_toolbars_destroy((LttvToolbars*)*(value.v_pointer));

  g_object_unref(main_window->attributes);
  g_main_window_list = g_slist_remove(g_main_window_list, main_window);

  g_printf("There are now : %d windows\n",g_slist_length(g_main_window_list));
  if(g_slist_length(g_main_window_list) == 0)
    gtk_main_quit ();
}

gboolean    
on_MWindow_configure                   (GtkWidget         *widget,
                                        GdkEventConfigure *event,
                                        gpointer           user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)widget);
  float width = event->width;
  Tab * tab = mw_data->tab;
  TimeWindow time_win;
  double ratio;
  TimeInterval *time_span;
  LttTime time;
	
	// MD : removed time width modification upon resizing of the main window.
	// The viewers will redraw themselves completely, without time interval
	// modification.
/*  while(tab){
    if(mw_data->window_width){
      time_span = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->Time_Span ;
      time_win = tab->time_window;
      ratio = width / mw_data->window_width;
      tab->time_window.time_width = ltt_time_mul(time_win.time_width,ratio);
      time = ltt_time_sub(time_span->endTime, time_win.start_time);
      if(ltt_time_compare(time, tab->time_window.time_width) < 0){
	tab->time_window.time_width = time;
      }
    } 
    tab = tab->next;
  }

  mw_data->window_width = (int)width;
	*/
  return FALSE;
}

/* Set current tab
 */

void
on_MNotebook_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
  MainWindow * mw = get_window_data_struct((GtkWidget*)notebook);
  Tab * tab = mw->tab;
 
  while(page_num){
    tab = tab->next;
    page_num--;
  }
  mw->current_tab = tab;
}


/* callback function to check or uncheck the check box (filter)
 */

void checkbox_changed(GtkTreeView *treeview,
		      GtkTreePath *arg1,
		      GtkTreeViewColumn *arg2,
		      gpointer user_data)
{
  GtkTreeStore * store = (GtkTreeStore *)gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  gboolean value;

  if (gtk_tree_model_get_iter ((GtkTreeModel *)store, &iter, arg1)){
    gtk_tree_model_get ((GtkTreeModel *)store, &iter, CHECKBOX_COLUMN, &value, -1);
    value = value? FALSE : TRUE;
    gtk_tree_store_set (GTK_TREE_STORE (store), &iter, CHECKBOX_COLUMN, value, -1);    
  }  
  
}


/* According to user's selection, update selector(filter)
 */

void update_filter(LttvTracesetSelector *s,  GtkTreeStore *store )
{
  GtkTreeIter iter, child_iter, child_iter1, child_iter2;
  int i, j, k, nb_eventtype;
  LttvTraceSelector     * trace;
  LttvTracefileSelector * tracefile;
  LttvEventtypeSelector * eventtype;
  gboolean value, value1, value2;

  if(gtk_tree_model_get_iter_first((GtkTreeModel*)store, &iter)){
    i = 0;
    do{
      trace = lttv_traceset_selector_trace_get(s, i);
      nb_eventtype = lttv_trace_selector_eventtype_number(trace);
      gtk_tree_model_get ((GtkTreeModel*)store, &iter, CHECKBOX_COLUMN, &value,-1);
      if(value){
	j = 0;
	if(gtk_tree_model_iter_children ((GtkTreeModel*)store, &child_iter, &iter)){
	  do{
	    if(j<1){//eventtype selector for trace
	      gtk_tree_model_get ((GtkTreeModel*)store, &child_iter, CHECKBOX_COLUMN, &value2,-1);
	      if(value2){
		k=0;
		if(gtk_tree_model_iter_children ((GtkTreeModel*)store, &child_iter1, &child_iter)){
		  do{
		    eventtype = lttv_trace_selector_eventtype_get(trace,k);
		    gtk_tree_model_get ((GtkTreeModel*)store, &child_iter1, CHECKBOX_COLUMN, &value2,-1);
		    lttv_eventtype_selector_set_selected(eventtype,value2);
		    k++;
		  }while(gtk_tree_model_iter_next((GtkTreeModel*)store, &child_iter1));
		}
	      }
	    }else{ //tracefile selector
	      tracefile = lttv_trace_selector_tracefile_get(trace, j - 1);
	      gtk_tree_model_get ((GtkTreeModel*)store, &child_iter, CHECKBOX_COLUMN, &value1,-1);
	      lttv_tracefile_selector_set_selected(tracefile,value1);
	      if(value1){
		gtk_tree_model_iter_children((GtkTreeModel*)store, &child_iter1, &child_iter); //eventtype selector
		gtk_tree_model_get ((GtkTreeModel*)store, &child_iter1, CHECKBOX_COLUMN, &value2,-1);
		if(value2){
		  k = 0;
		  if(gtk_tree_model_iter_children ((GtkTreeModel*)store, &child_iter2, &child_iter1)){
		    do{//eventtype selector for tracefile
		      eventtype = lttv_tracefile_selector_eventtype_get(tracefile,k);
		      gtk_tree_model_get ((GtkTreeModel*)store, &child_iter2, CHECKBOX_COLUMN, &value2,-1);
		      lttv_eventtype_selector_set_selected(eventtype,value2);
		      k++;
		    }while(gtk_tree_model_iter_next((GtkTreeModel*)store, &child_iter2));
		  }
		}
	      }
	    }
	    j++;
	  }while(gtk_tree_model_iter_next((GtkTreeModel*)store, &child_iter));
	}
      }
      lttv_trace_selector_set_selected(trace,value);
      i++;
    }while(gtk_tree_model_iter_next((GtkTreeModel*)store, &iter));
  }
}


/* Display a dialogue showing all eventtypes and traces, let user to select the interested
 * eventtypes, tracefiles and traces (filter)
 */

gboolean get_filter_selection(LttvTracesetSelector *s,char *title, char * column_title)
{
  GtkWidget         * dialogue;
  GtkTreeStore      * store;
  GtkWidget         * tree;
  GtkWidget         * scroll_win;
  GtkCellRenderer   * renderer;
  GtkTreeViewColumn * column;
  GtkTreeIter         iter, child_iter, child_iter1, child_iter2;
  int i, j, k, id, nb_trace, nb_tracefile, nb_eventtype;
  LttvTraceSelector     * trace;
  LttvTracefileSelector * tracefile;
  LttvEventtypeSelector * eventtype;
  char * name;
  gboolean checked;

  dialogue = gtk_dialog_new_with_buttons(title,
					 NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 
  gtk_window_set_default_size((GtkWindow*)dialogue, 300, 500);

  store = gtk_tree_store_new (TOTAL_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING);
  tree  = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));
  g_signal_connect (G_OBJECT (tree), "row-activated",
		    G_CALLBACK (checkbox_changed),
  		    NULL);  


  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio((GtkCellRendererToggle *)renderer, FALSE);

  g_object_set (G_OBJECT (renderer),"activatable", TRUE, NULL);

  column   = gtk_tree_view_column_new_with_attributes ("Checkbox",
				  		       renderer,
						       "active", CHECKBOX_COLUMN,
						       NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_fixed_width (column, 20);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes (column_title,
				  		       renderer,
						       "text", NAME_COLUMN,
						       NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (tree), FALSE);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), 
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroll_win), tree);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), scroll_win,TRUE, TRUE,0);

  gtk_widget_show(scroll_win);
  gtk_widget_show(tree);
  
  nb_trace = lttv_traceset_selector_trace_number(s);
  for(i=0;i<nb_trace;i++){
    trace = lttv_traceset_selector_trace_get(s, i);
    name  = lttv_trace_selector_get_name(trace);    
    gtk_tree_store_append (store, &iter, NULL);
    checked = lttv_trace_selector_get_selected(trace);
    gtk_tree_store_set (store, &iter, 
			CHECKBOX_COLUMN,checked,
			NAME_COLUMN,name,
			-1);

    gtk_tree_store_append (store, &child_iter, &iter);      
    gtk_tree_store_set (store, &child_iter, 
			CHECKBOX_COLUMN, checked,
			NAME_COLUMN,"eventtype",
			-1);
    
    nb_eventtype = lttv_trace_selector_eventtype_number(trace);
    for(j=0;j<nb_eventtype;j++){
      eventtype = lttv_trace_selector_eventtype_get(trace,j);
      name      = lttv_eventtype_selector_get_name(eventtype);    
      checked   = lttv_eventtype_selector_get_selected(eventtype);
      gtk_tree_store_append (store, &child_iter1, &child_iter);      
      gtk_tree_store_set (store, &child_iter1, 
			  CHECKBOX_COLUMN, checked,
			  NAME_COLUMN,name,
			  -1);
    }

    nb_tracefile = lttv_trace_selector_tracefile_number(trace);
    for(j=0;j<nb_tracefile;j++){
      tracefile = lttv_trace_selector_tracefile_get(trace, j);
      name      = lttv_tracefile_selector_get_name(tracefile);    
      gtk_tree_store_append (store, &child_iter, &iter);
      checked = lttv_tracefile_selector_get_selected(tracefile);
      gtk_tree_store_set (store, &child_iter, 
			  CHECKBOX_COLUMN, checked,
			  NAME_COLUMN,name,
			  -1);

      gtk_tree_store_append (store, &child_iter1, &child_iter);      
      gtk_tree_store_set (store, &child_iter1, 
			  CHECKBOX_COLUMN, checked,
			  NAME_COLUMN,"eventtype",
			  -1);
    
      for(k=0;k<nb_eventtype;k++){
	eventtype = lttv_tracefile_selector_eventtype_get(tracefile,k);
	name      = lttv_eventtype_selector_get_name(eventtype);    
	checked   = lttv_eventtype_selector_get_selected(eventtype);
	gtk_tree_store_append (store, &child_iter2, &child_iter1);      
	gtk_tree_store_set (store, &child_iter2, 
			    CHECKBOX_COLUMN, checked,
			    NAME_COLUMN,name,
			    -1);
      }
    }
  }

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      update_filter(s, store);
      gtk_widget_destroy(dialogue);
      return TRUE;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }
  return FALSE;
}


/* Select a trace which will be removed from traceset
 */

char * get_remove_trace(char ** all_trace_name, int nb_trace)
{
  return get_selection(all_trace_name, nb_trace, 
		       "Select a trace", "Trace pathname");
}


/* Select a module which will be unloaded
 */

char * get_unload_module(char ** loaded_module_name, int nb_module)
{
  return get_selection(loaded_module_name, nb_module, 
		       "Select an unload module", "Module pathname");
}


/* Display a dialogue which shows all selectable items, let user to 
 * select one of them
 */

char * get_selection(char ** loaded_module_name, int nb_module,
		     char *title, char * column_title)
{
  GtkWidget         * dialogue;
  GtkWidget         * scroll_win;
  GtkWidget         * tree;
  GtkListStore      * store;
  GtkTreeViewColumn * column;
  GtkCellRenderer   * renderer;
  GtkTreeSelection  * select;
  GtkTreeIter         iter;
  gint                id, i;
  char              * unload_module_name = NULL;

  dialogue = gtk_dialog_new_with_buttons(title,
					 NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 
  gtk_window_set_default_size((GtkWindow*)dialogue, 500, 200);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show ( scroll_win);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  store = gtk_list_store_new (N_COLUMNS,G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));
  gtk_widget_show ( tree);
  g_object_unref (G_OBJECT (store));
		
  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes (column_title,
						     renderer,
						     "text", MODULE_COLUMN,
						     NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_fixed_width (column, 150);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  gtk_container_add (GTK_CONTAINER (scroll_win), tree);  

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), scroll_win,TRUE, TRUE,0);

  for(i=0;i<nb_module;i++){
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, MODULE_COLUMN,loaded_module_name[i],-1);
  }

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      if (gtk_tree_selection_get_selected (select, (GtkTreeModel**)&store, &iter)){
	  gtk_tree_model_get ((GtkTreeModel*)store, &iter, MODULE_COLUMN, &unload_module_name, -1);
      }
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }

  return unload_module_name;
}


/* Insert all menu entry and tool buttons into this main window
 * for modules.
 *
 */

void add_all_menu_toolbar_constructors(MainWindow * mw, gpointer user_data)
{
  int i;
  GdkPixbuf *pixbuf;
  lttvwindow_viewer_constructor constructor;
  LttvMenus * global_menu, * instance_menu;
  LttvToolbars * global_toolbar, * instance_toolbar;
  LttvMenuClosure *menu_item;
  LttvToolbarClosure *toolbar_item;
  LttvAttributeValue value;
  LttvIAttribute *global_attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvIAttribute *attributes = mw->attributes;
  GtkWidget * tool_menu_title_menu, *new_widget, *pixmap;

  g_assert(lttv_iattribute_find_by_path(global_attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_menus_new();
  global_menu = (LttvMenus*)*(value.v_pointer);

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_menus_new();
  instance_menu = (LttvMenus*)*(value.v_pointer);



  g_assert(lttv_iattribute_find_by_path(global_attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_toolbars_new();
  global_toolbar = (LttvToolbars*)*(value.v_pointer);

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    *(value.v_pointer) = lttv_toolbars_new();
  instance_toolbar = (LttvToolbars*)*(value.v_pointer);

  /* Add missing menu entries to window instance */
  for(i=0;i<global_menu->len;i++) {
    menu_item = &g_array_index(global_menu, LttvMenuClosure, i);

    //add menu_item to window instance;
    constructor = menu_item->con;
    tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
    new_widget =
      gtk_menu_item_new_with_mnemonic (menu_item->menu_text);
    gtk_container_add (GTK_CONTAINER (tool_menu_title_menu),
        new_widget);
    g_signal_connect ((gpointer) new_widget, "activate",
        G_CALLBACK (insert_viewer_wrap),
        constructor);  
    gtk_widget_show (new_widget);
    lttv_menus_add(instance_menu, menu_item->con, 
        menu_item->menu_path,
        menu_item->menu_text,
        new_widget);

  }

  /* Add missing toolbar entries to window instance */
  for(i=0;i<global_toolbar->len;i++) {
    toolbar_item = &g_array_index(global_toolbar, LttvToolbarClosure, i);

    //add toolbar_item to window instance;
    constructor = toolbar_item->con;
    tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar1");
    pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)toolbar_item->pixmap);
    pixmap = gtk_image_new_from_pixbuf(pixbuf);
    new_widget =
       gtk_toolbar_append_element (GTK_TOOLBAR (tool_menu_title_menu),
          GTK_TOOLBAR_CHILD_BUTTON,
          NULL,
          "",
          toolbar_item->tooltip, NULL,
          pixmap, NULL, NULL);
    gtk_label_set_use_underline(
        GTK_LABEL (((GtkToolbarChild*) (
                         g_list_last (GTK_TOOLBAR 
                            (tool_menu_title_menu)->children)->data))->label),
        TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (new_widget), 1);
    g_signal_connect ((gpointer) new_widget,
        "clicked",
        G_CALLBACK (insert_viewer_wrap),
        constructor);       
    gtk_widget_show (new_widget);
 
    lttv_toolbars_add(instance_toolbar, toolbar_item->con, 
                      toolbar_item->tooltip,
                      toolbar_item->pixmap,
                      new_widget);

  }

}


/* Create a main window
 */

void construct_main_window(MainWindow * parent)
{
  g_debug("construct_main_window()");
  GtkWidget  * new_window; /* New generated main window */
  MainWindow * new_m_window;/* New main window structure */
  GtkNotebook * notebook;
  LttvIAttribute *attributes =
	  LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  LttvAttributeValue value;
         
  new_m_window = g_new(MainWindow, 1);

  // Add the object's information to the module's array 
  g_main_window_list = g_slist_append(g_main_window_list, new_m_window);


  new_window  = create_MWindow();
  gtk_widget_show (new_window);
    
  new_m_window->mwindow = new_window;
  new_m_window->tab = NULL;
  new_m_window->current_tab = NULL;
  new_m_window->attributes = attributes;

  g_assert(lttv_iattribute_find_by_path(attributes,
           "viewers/menu", LTTV_POINTER, &value));
  *(value.v_pointer) = lttv_menus_new();

  g_assert(lttv_iattribute_find_by_path(attributes,
           "viewers/toolbar", LTTV_POINTER, &value));
  *(value.v_pointer) = lttv_toolbars_new();

  add_all_menu_toolbar_constructors(new_m_window, NULL);
  
  g_object_set_data_full(G_OBJECT(new_window),
                         "main_window_data",
                         (gpointer)new_m_window,
                         (GDestroyNotify)g_free);
  //create a default tab
  notebook = (GtkNotebook *)lookup_widget(new_m_window->mwindow, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  //for now there is no name field in LttvTraceset structure
  //Use "Traceset" as the label for the default tab
  if(parent)
    create_tab(new_m_window, parent->current_tab, notebook, "Traceset");
  else {
    create_tab(new_m_window, NULL, notebook, "Traceset");
    if(g_init_trace != NULL){
      lttvwindow_add_trace(new_m_window->current_tab,
                           g_init_trace);
    }
  }

  g_printf("There are now : %d windows\n",g_slist_length(g_main_window_list));
}


/* Free the memory occupied by a tab structure
 * destroy the tab
 */

void tab_destructor(Tab * tab_instance)
{
  int i, nb, ref_count;
  LttvTrace * trace;

  if(tab_instance->attributes)
    g_object_unref(tab_instance->attributes);

  if(tab_instance->interrupted_state)
    g_object_unref(tab_instance->interrupted_state);


  if(tab_instance->mw->tab == tab_instance){
    tab_instance->mw->tab = tab_instance->next;
  }else{
    Tab * tmp1, *tmp = tab_instance->mw->tab;
    while(tmp != tab_instance){
      tmp1 = tmp;
      tmp = tmp->next;
    }
    tmp1->next = tab_instance->next;
  }

  if(tab_instance->traceset_info->traceset_context != NULL){
    //remove state update hooks
    lttv_state_remove_event_hooks(
         (LttvTracesetState*)tab_instance->traceset_info->
                              traceset_context);
    lttv_context_fini(LTTV_TRACESET_CONTEXT(tab_instance->traceset_info->
					    traceset_context));
    g_object_unref(tab_instance->traceset_info->traceset_context);
  }
  if(tab_instance->traceset_info->traceset != NULL) {
    nb = lttv_traceset_number(tab_instance->traceset_info->traceset);
    for(i = 0 ; i < nb ; i++) {
      trace = lttv_traceset_get(tab_instance->traceset_info->traceset, i);
      ref_count = lttv_trace_get_ref_number(trace);
      if(ref_count <= 1){
	ltt_trace_close(lttv_trace(trace));
	lttv_trace_destroy(trace);
      }
      //      lttv_trace_destroy(trace);
    }
  }  
  lttv_traceset_destroy(tab_instance->traceset_info->traceset);
  /* Remove the idle events requests processing function of the tab */
  g_idle_remove_by_data(tab_instance);

  g_slist_free(tab_instance->events_requests);
  g_free(tab_instance->traceset_info);
  g_free(tab_instance);
}


/* Create a tab and insert it into the current main window
 */

void * create_tab(MainWindow * mw, Tab *copy_tab, 
		  GtkNotebook * notebook, char * label)
{
  GList * list;
  Tab * tmp_tab;
  LttTime tmp_time;
  
  //create a new tab data structure
  tmp_tab = mw->tab;
  while(tmp_tab && tmp_tab->next) tmp_tab = tmp_tab->next;
  if(!tmp_tab){
    mw->current_tab = NULL;
    tmp_tab = g_new(Tab,1);
    mw->tab = tmp_tab;    
  }else{
    tmp_tab->next = g_new(Tab,1);
    tmp_tab = tmp_tab->next;
  }

  //construct and initialize the traceset_info
  tmp_tab->traceset_info = g_new(TracesetInfo,1);

  if(copy_tab) {
    tmp_tab->traceset_info->traceset = 
      lttv_traceset_copy(copy_tab->traceset_info->traceset);
  } else {
    tmp_tab->traceset_info->traceset = lttv_traceset_new();
  }

//FIXME : this is g_debug level
  lttv_attribute_write_xml(
      lttv_traceset_attribute(tmp_tab->traceset_info->traceset),
      stdout,
      0, 4);
  fflush(stdout);


  //FIXME copy not implemented in lower level
  tmp_tab->traceset_info->traceset_context =
    g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  g_assert(tmp_tab->traceset_info->traceset_context != NULL);
  lttv_context_init(
	    LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context),
	                          tmp_tab->traceset_info->traceset);
  //add state update hooks
  lttv_state_add_event_hooks(
       (LttvTracesetState*)tmp_tab->traceset_info->traceset_context);
  
  //determine the current_time and time_window of the tab
  if(mw->current_tab){
    // Will have to read directly at the main window level, as we want
    // to be able to modify a traceset on the fly.
    tmp_tab->time_window      = mw->current_tab->time_window;
    tmp_tab->current_time     = mw->current_tab->current_time;
  }else{
    // Will have to read directly at the main window level, as we want
    // to be able to modify a traceset on the fly.
    // get_traceset_time_span(mw_data,&tmp_tab->traceStartTime, &tmp_tab->traceEndTime);
    tmp_tab->time_window.start_time   = 
	    LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context)->time_span.start_time;
    if(DEFAULT_TIME_WIDTH_S <
              LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context)->time_span.end_time.tv_sec)
      tmp_time.tv_sec = DEFAULT_TIME_WIDTH_S;
    else
      tmp_time.tv_sec =
              LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context)->time_span.end_time.tv_sec;
    tmp_time.tv_nsec = 0;
    tmp_tab->time_window.time_width = tmp_time ;
    tmp_tab->current_time.tv_sec = 
       LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context)->time_span.start_time.tv_sec;
    tmp_tab->current_time.tv_nsec = 
       LTTV_TRACESET_CONTEXT(tmp_tab->traceset_info->traceset_context)->time_span.start_time.tv_nsec;
  }
  /* Become the current tab */
  mw->current_tab = tmp_tab;

  tmp_tab->attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  tmp_tab->interrupted_state = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
  tmp_tab->multi_vpaned = (GtkMultiVPaned*)gtk_multi_vpaned_new();
  tmp_tab->multi_vpaned->tab = mw->current_tab;
  gtk_widget_show((GtkWidget*)tmp_tab->multi_vpaned);
  tmp_tab->next = NULL;    
  tmp_tab->mw   = mw;

  tmp_tab->label = gtk_label_new (label);
  gtk_widget_show (tmp_tab->label);

  /* Start with empty events requests list */
  tmp_tab->events_requests = NULL;
  tmp_tab->events_request_pending = FALSE;

  g_object_set_data_full(
           G_OBJECT(tmp_tab->multi_vpaned),
           "Tab_Info",
	   tmp_tab,
	   (GDestroyNotify)tab_destructor);

 //insert tab into notebook
  gtk_notebook_append_page(notebook, (GtkWidget*)tmp_tab->multi_vpaned, tmp_tab->label);  
  list = gtk_container_get_children(GTK_CONTAINER(notebook));
  gtk_notebook_set_current_page(notebook,g_list_length(list)-1);
  // always show : not if(g_list_length(list)>1)
  gtk_notebook_set_show_tabs(notebook, TRUE);
 
}

/**
 * Function to show each viewer in the current tab.
 * It will be called by main window, call show on each registered viewer,
 * will call process traceset and then it will
 * @param main_win the main window the viewer belongs to.
 */
//FIXME Only one time request maximum for now!
void show_viewer(MainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  int i;
  LttvTracesetContext * tsc;
  Tab *tab = main_win->current_tab;
  
  if(tab == NULL) return ;
  
  tsc =(LttvTracesetContext*)tab->traceset_info->traceset_context;
  
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL)
  {
    g_warning("The viewer(s) did not add any show hook");
    return;
  }

  
  // Call the show, where viewers add hooks to context and fill the
  // time and number of events requested it the time_requests GArray.
  lttv_hooks_call(tmp, NULL);

 
}

/*
 * execute_events_requests
 *
 * Idle function that executes the pending requests for a tab.
 *
 * @return return value : TRUE : keep the idle function, FALSE : remove it.
 */
gboolean execute_events_requests(Tab *tab)
{
  return ( lttvwindow_process_pending_requests(tab) );
}

