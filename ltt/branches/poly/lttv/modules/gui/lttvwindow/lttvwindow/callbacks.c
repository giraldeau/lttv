/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang, Mathieu Desnoyers
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

#include <limits.h> // for PATH_MAX
#include <stdlib.h>

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
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/gtkdirsel.h>
#include <lttvwindow/lttvfilter.h>


#define DEFAULT_TIME_WIDTH_S   1

extern LttvTrace *g_init_trace ;


/** Array containing instanced objects. */
extern GSList * g_main_window_list;

/** MD : keep old directory. */
static char remember_plugins_dir[PATH_MAX] = "";
static char remember_trace_dir[PATH_MAX] = "";


MainWindow * get_window_data_struct(GtkWidget * widget);
char * get_load_module(char ** load_module_name, int nb_module);
char * get_unload_module(char ** loaded_module_name, int nb_module);
char * get_remove_trace(char ** all_trace_name, int nb_trace);
char * get_selection(char ** all_name, int nb, char *title, char * column_title);
gboolean get_filter_selection(LttvTracesetSelector *s, char *title, char * column_title);
Tab* create_tab(MainWindow * mw, Tab *copy_tab,
		  GtkNotebook * notebook, char * label);

static void insert_viewer(GtkWidget* widget, lttvwindow_viewer_constructor constructor);
void update_filter(LttvTracesetSelector *s,  GtkTreeStore *store );

void checkbox_changed(GtkTreeView *treeview,
		      GtkTreePath *arg1,
		      GtkTreeViewColumn *arg2,
		      gpointer user_data);
void remove_trace_from_traceset_selector(GtkWidget * paned, unsigned i);
void add_trace_into_traceset_selector(GtkWidget * paned, LttTrace * trace);
Tab *create_new_tab(GtkWidget* widget, gpointer user_data);

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

static gboolean viewer_grab_focus(GtkWidget *widget, GdkEventButton *event,
                                  gpointer data)
{
  GtkWidget *viewer = GTK_WIDGET(data);
  GtkWidget *viewer_container = gtk_widget_get_parent(viewer);

  g_debug("FOCUS GRABBED");
  g_object_set_data(G_OBJECT(viewer_container), "focused_viewer", viewer);
  return 0;
}

static void connect_focus_recursive(GtkWidget *widget,
                                    GtkWidget *viewer)
{
  if(GTK_IS_CONTAINER(widget)) {
    gtk_container_forall(GTK_CONTAINER(widget),
                         (GtkCallback)connect_focus_recursive,
                         viewer);

  }
  if(GTK_IS_TREE_VIEW(widget)) {
    gtk_tree_view_set_headers_clickable(widget, TRUE);
  }
  gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT(widget),
                    "button-press-event",
                    G_CALLBACK (viewer_grab_focus),
                    (gpointer)viewer);
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
  GtkWidget * viewer_container;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget * viewer;
  LttvTracesetSelector  * s;
  TimeInterval * time_interval;
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;
  
  if(!page) {
    tab = create_new_tab(widget, NULL);
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  viewer_container = tab->viewer_container;

  s = construct_traceset_selector(tab->traceset_info->traceset);
  viewer = (GtkWidget*)constructor(tab);
  if(viewer)
  {
    //gtk_multivpaned_widget_add(GTK_MULTIVPANED(multivpaned), viewer); 
    
    gtk_box_pack_end(GTK_BOX(viewer_container),
                     viewer,
                     TRUE,
                     TRUE,
                     0);

    /* We want to connect the viewer_grab_focus to EVERY
     * child of this widget. The little trick is to get each child
     * of each GTK_CONTAINER, even subchildren.
     */
    connect_focus_recursive(viewer, viewer);
  }
}

/**
 * Function to set/update traceset for the viewers
 * @param tab viewer's tab 
 * @param traceset traceset of the main window.
 * return value :
 *  0 : traceset updated
 *  1 : no traceset hooks to update; not an error.
 */

int SetTraceset(Tab * tab, LttvTraceset *traceset)
{
  LttvTracesetContext *tsc =
        LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  TimeWindow new_time_window;
  LttTime new_current_time;

  /* Set the tab's time window and current time if
   * out of bounds */
  if(ltt_time_compare(tab->time_window.start_time, time_span.start_time) < 0
     || ltt_time_compare(  ltt_time_add(tab->time_window.start_time,
                                        tab->time_window.time_width),
                           time_span.end_time) > 0) {
    new_time_window.start_time = time_span.start_time;
    
    new_current_time = time_span.start_time;
    
    LttTime tmp_time;

    if(DEFAULT_TIME_WIDTH_S < time_span.end_time.tv_sec)
      tmp_time.tv_sec = DEFAULT_TIME_WIDTH_S;
    else
      tmp_time.tv_sec = time_span.end_time.tv_sec;
    tmp_time.tv_nsec = 0;
    new_time_window.time_width = tmp_time ;
  }
  time_change_manager(tab, new_time_window);
  current_time_change_manager(tab, new_current_time);

#if 0
  /* Set scrollbar */
  GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(tab->scrollbar));
  LttTime upper = ltt_time_sub(time_span.end_time, time_span.start_time);
      
  g_object_set(G_OBJECT(adjustment),
               "lower",
                 0.0, /* lower */
               "upper",
               ltt_time_to_double(upper) 
                 * NANOSECONDS_PER_SECOND, /* upper */
               "step_increment",
               ltt_time_to_double(tab->time_window.time_width)
                             / SCROLL_STEP_PER_PAGE
                             * NANOSECONDS_PER_SECOND, /* step increment */
               "page_increment",
               ltt_time_to_double(tab->time_window.time_width) 
                 * NANOSECONDS_PER_SECOND, /* page increment */
               "page_size",
               ltt_time_to_double(tab->time_window.time_width) 
                 * NANOSECONDS_PER_SECOND, /* page size */
               NULL);
  gtk_adjustment_changed(adjustment);

  g_object_set(G_OBJECT(adjustment),
               "value",
               ltt_time_to_double(
                ltt_time_sub(tab->time_window.start_time, time_span.start_time))
                   * NANOSECONDS_PER_SECOND, /* value */
               NULL);
  gtk_adjustment_value_changed(adjustment);

  /* set the time bar. The value callbacks will change their nsec themself */
  /* start seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry1),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);

  /* end seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry3),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);

   /* current seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry5),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);
#endif //0
  
  /* Finally, call the update hooks of the viewers */
  LttvHooks * tmp;
  LttvAttributeValue value;
  gint retval = 0;

 
  g_assert( lttv_iattribute_find_by_path(tab->attributes,
     "hooks/updatetraceset", LTTV_POINTER, &value));

  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) retval = 1;
  else lttv_hooks_call(tmp,traceset);

 
  return retval;
}

/**
 * Function to set/update filter for the viewers
 * @param tab viewer's tab 
 * @param filter filter of the main window.
 * return value :
 * -1 : error
 *  0 : filters updated
 *  1 : no filter hooks to update; not an error.
 */

int SetFilter(Tab * tab, gpointer filter)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(tab->attributes,
     "hooks/updatefilter", LTTV_POINTER, &value));

  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL) return 1;
  lttv_hooks_call(tmp,filter);

  return 0;
}



/**
 * Function to redraw each viewer belonging to the current tab 
 * @param tab viewer's tab 
 */

void update_traceset(Tab *tab)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, NULL);
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

/* Get the currently focused viewer.
 * If no viewer is focused, use the first one.
 *
 * If no viewer available, return NULL.
 */
GtkWidget *viewer_container_focus(GtkWidget *container)
{
  GtkWidget *widget;

  widget = (GtkWidget*)g_object_get_data(G_OBJECT(container),
                                         "focused_viewer");

  if(widget == NULL) {
    g_debug("no widget focused");
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));

    if(children != NULL)
      widget = GTK_WIDGET(children->data);
      g_object_set_data(G_OBJECT(container),
                        "focused_viewer",
                        widget);
  }
  
  return widget;


}

gint viewer_container_position(GtkWidget *container, GtkWidget *child)
{

  if(child == NULL) return -1;

  gint pos;
  GValue value = { 0, };
  g_value_init(&value, G_TYPE_INT);
  gtk_container_child_get_property(GTK_CONTAINER(container),
                                   child,
                                   "position",
                                   &value);
  pos = g_value_get_int(&value);

  return pos;
}


/* move_*_viewer functions move the selected view up/down in 
 * the current tab
 */

void move_down_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));

  Tab *tab;
  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  //gtk_multivpaned_widget_move_up(GTK_MULTIVPANED(tab->multivpaned));

  /* change the position in the vbox */
  GtkWidget *focus_widget;
  gint position;
  focus_widget = viewer_container_focus(tab->viewer_container);
  position = viewer_container_position(tab->viewer_container, focus_widget);

  if(position > 0) {
    /* can move up one position */
    gtk_box_reorder_child(GTK_BOX(tab->viewer_container),
                          focus_widget,
                          position-1);
  }
  
}

void move_up_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  //gtk_multivpaned_widget_move_down(GTK_MULTIVPANED(tab->multivpaned));
  /* change the position in the vbox */
  GtkWidget *focus_widget;
  gint position;
  focus_widget = viewer_container_focus(tab->viewer_container);
  position = viewer_container_position(tab->viewer_container, focus_widget);

  if(position != -1 &&
  position <
       g_list_length(gtk_container_get_children(
                        GTK_CONTAINER(tab->viewer_container)))-1
      ) {
    /* can move down one position */
    gtk_box_reorder_child(GTK_BOX(tab->viewer_container),
                          focus_widget,
                          position+1);
  }
 
}


/* delete_viewer deletes the selected viewer in the current tab
 */

void delete_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  //gtk_multivpaned_widget_delete(GTK_MULTIVPANED(tab->multivpaned));

  GtkWidget *focus_widget = viewer_container_focus(tab->viewer_container);
  
  if(focus_widget != NULL)
    gtk_widget_destroy(focus_widget);

  g_object_set_data(G_OBJECT(tab->viewer_container), "focused_viewer", NULL);
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

static void events_request_free(EventsRequest *events_request)
{
  if(events_request == NULL) return;

  if(events_request->start_position != NULL)
       lttv_traceset_context_position_destroy(events_request->start_position);
  if(events_request->end_position != NULL)
       lttv_traceset_context_position_destroy(events_request->end_position);
  if(events_request->before_chunk_traceset != NULL)
       lttv_hooks_destroy(events_request->before_chunk_traceset);
  if(events_request->before_chunk_trace != NULL)
       lttv_hooks_destroy(events_request->before_chunk_trace);
  if(events_request->before_chunk_tracefile != NULL)
       lttv_hooks_destroy(events_request->before_chunk_tracefile);
  if(events_request->event != NULL)
       lttv_hooks_destroy(events_request->event);
  if(events_request->event_by_id != NULL)
       lttv_hooks_by_id_destroy(events_request->event_by_id);
  if(events_request->after_chunk_tracefile != NULL)
       lttv_hooks_destroy(events_request->after_chunk_tracefile);
  if(events_request->after_chunk_trace != NULL)
       lttv_hooks_destroy(events_request->after_chunk_trace);
  if(events_request->after_chunk_traceset != NULL)
       lttv_hooks_destroy(events_request->after_chunk_traceset);
  if(events_request->before_request != NULL)
       lttv_hooks_destroy(events_request->before_request);
  if(events_request->after_request != NULL)
       lttv_hooks_destroy(events_request->after_request);

  g_free(events_request);
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

#define list_out tab->events_requests

gboolean lttvwindow_process_pending_requests(Tab *tab)
{
  unsigned max_nb_events;
  GdkWindow * win;
  GdkCursor * new;
  GtkWidget* widget;
  LttvTracesetContext *tsc;
  LttvTracefileContext *tfc;
  GSList *list_in = NULL;
  LttTime end_time;
  guint end_nb_events;
  guint count;
  LttvTracesetContextPosition *end_position;
  
  if(tab == NULL) {
    g_critical("Foreground processing : tab does not exist. Processing removed.");
    return FALSE;
  }

  /* There is no events requests pending : we should never have been called! */
  g_assert(g_slist_length(list_out) != 0);

  tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);

  //set the cursor to be X shape, indicating that the computer is busy in doing its job
#if 0
  new = gdk_cursor_new(GDK_X_CURSOR);
  widget = lookup_widget(tab->mw->mwindow, "MToolbar1");
  win = gtk_widget_get_parent_window(widget);  
  gdk_window_set_cursor(win, new);
  gdk_cursor_unref(new);  
  gdk_window_stick(win);
  gdk_window_unstick(win);
#endif //0

  g_debug("SIZE events req len  : %d", g_slist_length(list_out));
  
  /* Preliminary check for no trace in traceset */
  /* Unregister the routine if empty, empty list_out too */
  if(lttv_traceset_number(tsc->ts) == 0) {

    /* - For each req in list_out */
    GSList *iter = list_out;

    while(iter != NULL) {

      gboolean remove = FALSE;
      gboolean free_data = FALSE;
      EventsRequest *events_request = (EventsRequest *)iter->data;
      
      /* - Call end request for req */
      if(events_request->servicing == TRUE)
        lttv_hooks_call(events_request->after_request, (gpointer)tsc);
      
      /* - remove req from list_out */
      /* Destroy the request */
      remove = TRUE;
      free_data = TRUE;

      /* Go to next */
      if(remove)
      {
        GSList *remove_iter = iter;

        iter = g_slist_next(iter);
        if(free_data) events_request_free((EventsRequest*)remove_iter->data);
        list_out = g_slist_remove_link(list_out, remove_iter);
      } else { // not remove
        iter = g_slist_next(iter);
      }
    }
  }
  
  /* 0.1 Lock Traces */
  {
    guint iter_trace=0;
    
    for(iter_trace=0; 
        iter_trace<lttv_traceset_number(tsc->ts);
        iter_trace++) {
      LttvTrace *trace_v = lttv_traceset_get(tsc->ts, iter_trace);

      if(lttvwindowtraces_lock(trace_v) != 0) {
        g_critical("Foreground processing : Unable to get trace lock");
        return TRUE; /* Cannot get lock, try later */
      }
    }
  }

  /* 0.2 Seek tracefiles positions to context position */
  lttv_process_traceset_synchronize_tracefiles(tsc);

  
  /* Events processing algorithm implementation */
  /* Warning : the gtk_events_pending takes a LOT of cpu time. So what we do
   * instead is to leave the control to GTK and take it back.
   */
  /* A. Servicing loop */
  //while( (g_slist_length(list_in) != 0 || g_slist_length(list_out) != 0)) {
  if((g_slist_length(list_in) != 0 || g_slist_length(list_out) != 0)) {
    /* Servicing */
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
        
        {
          EventsRequest *event_request_lpos = (EventsRequest*)g_slist_nth_data(lpos, 0);
          EventsRequest *event_request_ltime = (EventsRequest*)g_slist_nth_data(ltime, 0);
          LttTime lpos_start_time;
          
          if(event_request_lpos != NULL 
              && event_request_lpos->start_position != NULL) {
            lpos_start_time = lttv_traceset_context_position_get_time(
                                      event_request_lpos->start_position);
          }
          
          /* 1.1.3 If lpos.start time < ltime */
          if(event_request_lpos != NULL
              && event_request_lpos->start_position != NULL
              && ltt_time_compare(lpos_start_time,
                              event_request_ltime->start_time)<0) {
            /* Add lpos to list_in, remove them from list_out */
            for(iter=lpos;iter!=NULL;iter=g_slist_next(iter)) {
              /* Add to list_in */
              EventsRequest *event_request_lpos = 
                                    (EventsRequest*)iter->data;

              list_in = g_slist_append(list_in, event_request_lpos);
              /* Remove from list_out */
              list_out = g_slist_remove(list_out, event_request_lpos);
            }
          } else {
            /* 1.1.4 (lpos.start time >= ltime) */
            /* Add ltime to list_in, remove them from list_out */

            for(iter=ltime;iter!=NULL;iter=g_slist_next(iter)) {
              /* Add to list_in */
              EventsRequest *event_request_ltime = 
                                    (EventsRequest*)iter->data;

              list_in = g_slist_append(list_in, event_request_ltime);
              /* Remove from list_out */
              list_out = g_slist_remove(list_out, event_request_ltime);
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
        guint seek_count;

        /* 1.2.1 If first request in list_in is a time request */
        if(events_request->start_position == NULL) {
          /* - If first req in list_in start time != current time */
          if(tfc == NULL || ltt_time_compare(events_request->start_time,
                              tfc->timestamp) != 0)
            /* - Seek to that time */
            g_debug("SEEK TIME : %lu, %lu", events_request->start_time.tv_sec,
              events_request->start_time.tv_nsec);
            //lttv_process_traceset_seek_time(tsc, events_request->start_time);
            lttv_state_traceset_seek_time_closest(LTTV_TRACESET_STATE(tsc),
                                                  events_request->start_time);

            /* Process the traceset with only state hooks */
            seek_count =
               lttv_process_traceset_middle(tsc,
                                            events_request->start_time,
                                            G_MAXUINT, NULL);


        } else {
          LttTime pos_time;
          /* Else, the first request in list_in is a position request */
          /* If first req in list_in pos != current pos */
          g_assert(events_request->start_position != NULL);
          g_debug("SEEK POS time : %lu, %lu", 
                 lttv_traceset_context_position_get_time(
                      events_request->start_position).tv_sec,
                 lttv_traceset_context_position_get_time(
                      events_request->start_position).tv_nsec);

          g_debug("SEEK POS context time : %lu, %lu", 
               lttv_traceset_context_get_current_tfc(tsc)->timestamp.tv_sec,
               lttv_traceset_context_get_current_tfc(tsc)->timestamp.tv_nsec);
          g_assert(events_request->start_position != NULL);
          if(lttv_traceset_context_ctx_pos_compare(tsc,
                     events_request->start_position) != 0) {
            /* 1.2.2.1 Seek to that position */
            g_debug("SEEK POSITION");
            //lttv_process_traceset_seek_position(tsc, events_request->start_position);
            pos_time = lttv_traceset_context_position_get_time(
                                     events_request->start_position);
            
            lttv_state_traceset_seek_time_closest(LTTV_TRACESET_STATE(tsc),
                                                  pos_time);

            /* Process the traceset with only state hooks */
            seek_count =
               lttv_process_traceset_middle(tsc,
                                            ltt_time_infinite,
                                            G_MAXUINT,
                                            events_request->start_position);
            g_assert(lttv_traceset_context_ctx_pos_compare(tsc,
                         events_request->start_position) == 0);


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
            lttv_hooks_call(events_request->before_request, (gpointer)tsc);
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
      
      {
        /* 2.0 For each req of list_in */
        GSList *iter = list_in;
    
        while(iter != NULL) {

          EventsRequest *events_request = (EventsRequest *)iter->data;
          
          /* - Call before chunk
           * - events hooks added
           */
          lttv_process_traceset_begin(tsc, events_request->before_chunk_traceset,
                                         events_request->before_chunk_trace,
                                         events_request->before_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id);

          iter = g_slist_next(iter);
        }
      }

      {
        tfc = lttv_traceset_context_get_current_tfc(tsc);
      
        /* 2.1 For each req of list_out */
        GSList *iter = list_out;
    
        while(iter != NULL) {

          gboolean remove = FALSE;
          gboolean free_data = FALSE;
          EventsRequest *events_request = (EventsRequest *)iter->data;
          
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
            list_in = g_slist_append(list_in, events_request);
            remove = TRUE;
            free_data = FALSE;

            /* - If !servicing */
            if(events_request->servicing == FALSE) {
              /* - begin request hooks called
               * - servicing = TRUE
               */
              lttv_hooks_call(events_request->before_request, (gpointer)tsc);
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

          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) events_request_free((EventsRequest*)remove_iter->data);
            list_out = g_slist_remove_link(list_out, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
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
      g_debug("Calling process traceset middle with %p, %lu sec %lu nsec, %lu nb ev, %p end pos", tsc, end_time.tv_sec, end_time.tv_nsec, end_nb_events, end_position);
      count = lttv_process_traceset_middle(tsc, end_time, end_nb_events, end_position);

      tfc = lttv_traceset_context_get_current_tfc(tsc);
      if(tfc != NULL)
        g_debug("Context time after middle : %lu, %lu", tfc->timestamp.tv_sec,
                                                        tfc->timestamp.tv_nsec);
      else
        g_debug("End of trace reached after middle.");

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
          lttv_hooks_call(events_request->after_request, (gpointer)tsc);
          
          /* - remove req from list_in */
          /* Destroy the request */
          remove = TRUE;
          free_data = TRUE;

          /* Go to next */
          if(remove)
          {
            GSList *remove_iter = iter;

            iter = g_slist_next(iter);
            if(free_data) events_request_free((EventsRequest*)remove_iter->data);
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
           *     current context time >= req.end time
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
                                         events_request->end_time) >= 0
              ||
                  (events_request->end_position != NULL 
                 &&
                  lttv_traceset_context_ctx_pos_compare(tsc,
                            events_request->end_position) == 0)

              ) {
            g_assert(events_request->servicing == TRUE);
            /* - Call end request for req
             * - remove req from list_in */
            lttv_hooks_call(events_request->after_request, (gpointer)tsc);
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
            if(free_data) events_request_free((EventsRequest*)remove_iter->data);
            list_in = g_slist_remove_link(list_in, remove_iter);
          } else { // not remove
            iter = g_slist_next(iter);
          }
        }
      }
    }
  }
  /* End of removed servicing loop : leave control to GTK instead. */
  //  if(gtk_events_pending()) break;
  //}

  /* B. When interrupted between chunks */

  {
    GSList *iter = list_in;
    
    /* 1. for each request in list_in */
    while(iter != NULL) {

      gboolean remove = FALSE;
      gboolean free_data = FALSE;
      EventsRequest *events_request = (EventsRequest *)iter->data;
      
      /* 1.1. Use current postition as start position */
      if(events_request->start_position != NULL)
        lttv_traceset_context_position_destroy(events_request->start_position);
      events_request->start_position = lttv_traceset_context_position_new();
      lttv_traceset_context_position_save(tsc, events_request->start_position);

      /* 1.2. Remove start time */
      events_request->start_time = ltt_time_infinite;
      
      /* 1.3. Move from list_in to list_out */
      remove = TRUE;
      free_data = FALSE;
      list_out = g_slist_append(list_out, events_request);

      /* Go to next */
      if(remove)
      {
        GSList *remove_iter = iter;

        iter = g_slist_next(iter);
        if(free_data) events_request_free((EventsRequest*)remove_iter->data);
        list_in = g_slist_remove_link(list_in, remove_iter);
      } else { // not remove
        iter = g_slist_next(iter);
      }
    }


  }
  
  /* C Unlock Traces */
  {
    //lttv_process_traceset_get_sync_data(tsc);
    
    guint iter_trace;
    
    for(iter_trace=0; 
        iter_trace<lttv_traceset_number(tsc->ts);
        iter_trace++) {
      LttvTrace *trace_v = lttv_traceset_get(tsc->ts, iter_trace);

      lttvwindowtraces_unlock(trace_v);
    }
  }

#if 0
  //set the cursor back to normal
  gdk_window_set_cursor(win, NULL);  
#endif //0

  g_assert(g_slist_length(list_in) == 0);

  if( g_slist_length(list_out) == 0 ) {
    /* Put tab's request pending flag back to normal */
    tab->events_request_pending = FALSE;
    g_debug("remove the idle fct");
    return FALSE; /* Remove the idle function */
  }
  g_debug("leave the idle fct");
  return TRUE; /* Leave the idle function */

  /* We do not use simili-round-robin, it may require to read 1 meg buffers
   * again and again if many tracesets use the same tracefiles. */
  /* Hack for round-robin idle functions */
  /* It will put the idle function at the end of the pool */
  /*g_idle_add_full((G_PRIORITY_HIGH_IDLE + 21),
                  (GSourceFunc)execute_events_requests,
                  tab,
                  NULL);
  return FALSE;
  */
}

#undef list_out


/* add_trace_into_traceset_selector, each instance of a viewer has an associated
 * selector (filter), when a trace is added into traceset, the selector should 
 * reflect the change. The function is used to update the selector 
 */
#if 0
void add_trace_into_traceset_selector(GtkWidget * paned, LttTrace * t)
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

  w = gtk_multivpaned_get_first_widget(GTK_MULTIVPANED(paned));
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

    w = gtk_multivpaned_get_next_widget(GTK_MULTIVPANED(paned));
  }
}
#endif //0

static void lttvwindow_add_trace(Tab *tab, LttvTrace *trace_v)
{
  LttvTraceset *traceset = tab->traceset_info->traceset;
  guint i;
  guint num_traces = lttv_traceset_number(traceset);

 //Verify if trace is already present.
  for(i=0; i<num_traces; i++)
  {
    LttvTrace * trace = lttv_traceset_get(traceset, i);
    if(trace == trace_v)
      return;
  }

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
  lttv_trace_ref(trace_v);  /* local ref */

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

  //FIXME
  //add_trace_into_traceset_selector(GTK_MULTIVPANED(tab->multivpaned), lttv_trace(trace_v));
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
  char abs_path[PATH_MAX];
  gint id;
  gint i;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    tab = create_new_tab(widget, NULL);
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select a trace");
  gtk_dir_selection_hide_fileop_buttons(file_selector);
  
  if(remember_trace_dir[0] != '\0')
    gtk_dir_selection_set_filename(file_selector, remember_trace_dir);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (file_selector);
      strncpy(remember_trace_dir, dir, PATH_MAX);
      if(!dir || strlen(dir) == 0){
      	gtk_widget_destroy((GtkWidget*)file_selector);
      	break;
      }
      get_absolute_pathname(dir, abs_path);
      trace_v = lttvwindowtraces_get_trace_by_name(abs_path);
      if(trace_v == NULL) {
        trace = ltt_trace_open(abs_path);
        if(trace == NULL) {
          g_warning("cannot open trace %s", abs_path);
        } else {
          trace_v = lttv_trace_new(trace);
          lttvwindowtraces_add_trace(trace_v);
          lttvwindow_add_trace(tab, trace_v);
        }
      } else {
        lttvwindow_add_trace(tab, trace_v);
      }

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
#if 0
void remove_trace_from_traceset_selector(GtkWidget * paned, unsigned i)
{
  LttvTracesetSelector * s;
  LttvTraceSelector * t;
  GtkWidget * w; 
  
  w = gtk_multivpaned_get_first_widget(GTK_MULTIVPANED(paned));
  while(w){
    s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
    if(s){
      t = lttv_traceset_selector_trace_get(s,i);
      lttv_traceset_selector_trace_remove(s, i);
      lttv_trace_selector_destroy(t);
    }g_warning("Module dose not support filtering\n");
    w = gtk_multivpaned_get_next_widget(GTK_MULTIVPANED(paned));
  }
}
#endif //0

/* remove_trace removes a trace from the current traceset if all viewers in 
 * the current tab are not interested in the trace. It first displays a 
 * dialogue, which shows all traces in the current traceset, to let user choose 
 * a trace, then it checks if all viewers unselect the trace, if it is true, 
 * it will remove the trace,  recreate the traceset_contex,
 * and redraws all the viewer of the current tab. If there is on trace in the
 * current traceset, it will delete all viewers of the current tab
 */

// MD : no filter version.
void remove_trace(GtkWidget *widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  gint i, j, nb_trace, index=-1;
  char ** name, *remove_trace_name;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  nb_trace =lttv_traceset_number(tab->traceset_info->traceset); 
  name = g_new(char*,nb_trace);
  for(i = 0; i < nb_trace; i++){
    trace_v = lttv_traceset_get(tab->traceset_info->traceset, i);
    trace = lttv_trace(trace_v);
    name[i] = ltt_trace_name(trace);
  }

  remove_trace_name = get_remove_trace(name, nb_trace);


  if(remove_trace_name){

    /* yuk, cut n paste from old code.. should be better (MD)*/
    for(i = 0; i<nb_trace; i++) {
      if(strcmp(remove_trace_name,name[i]) == 0){
        index = i;
      }
    }
    
    traceset = tab->traceset_info->traceset;
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

    trace_v = lttv_traceset_get(traceset, index);

    lttv_traceset_remove(traceset, index);
    lttv_trace_unref(trace_v);  // Remove local reference

    if(lttv_trace_get_ref_number(trace_v) <= 1) {
      /* ref 1 : lttvwindowtraces only*/
      ltt_trace_close(lttv_trace(trace_v));
      /* lttvwindowtraces_remove_trace takes care of destroying
       * the traceset linked with the trace_v and also of destroying
       * the trace_v at the same time.
       */
      lttvwindowtraces_remove_trace(trace_v);
    }
    
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

    SetTraceset(tab, (gpointer)traceset);
  }
  g_free(name);
}

#if 0
void remove_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  gint i, j, nb_trace;
  char ** name, *remove_trace_name;
  MainWindow * mw_data = get_window_data_struct(widget);
  LttvTracesetSelector * s;
  LttvTraceSelector * t;
  GtkWidget * w; 
  gboolean selected;
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

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
        //FIXME
      	w = gtk_multivpaned_get_widget(GTK_MULTIVPANED(tab->multivpaned));
      	if(w){
      	  s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
      	  if(s){
      	    t = lttv_traceset_selector_trace_get(s,i);
      	    lttv_trace_selector_set_selected(t, FALSE);
      	  }

          //check if other viewers select the trace
          w = gtk_multivpaned_get_first_widget(GTK_MULTIVPANED(tab->multivpaned));
          while(w){
            s = g_object_get_data(G_OBJECT(w), "Traceset_Selector");
            if(s){
              t = lttv_traceset_selector_trace_get(s,i);
              selected = lttv_trace_selector_get_selected(t);
              if(selected)break;
            }
            w = gtk_multivpaned_get_next_widget(GTK_MULTIVPANED(tab->multivpaned));
          }
        }else selected = FALSE;

        //if no viewer selects the trace, remove it
        if(!selected){
          remove_trace_from_traceset_selector(GTK_MULTIVPANED(tab->multivpaned), i);

          traceset = tab->traceset_info->traceset;
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


          trace_v = lttv_traceset_get(traceset, i);

          if(lttv_trace_get_ref_number(trace_v) <= 2) {
            /* ref 2 : traceset, local */
            lttvwindowtraces_remove_trace(trace_v);
            ltt_trace_close(lttv_trace(trace_v));
          }
          
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
          //if(nb_trace > 1){

            SetTraceset(tab, (gpointer)traceset);
            // in expose now call_pending_read_hooks(mw_data);

            //lttvwindow_report_current_time(mw_data,&(tab->current_time));
          //}else{
          //  if(tab){
          //    while(tab->multi_vpaned->num_children){
          //      gtk_multi_vpaned_widget_delete(tab->multi_vpaned);
          //    }    
          //  }	    
          //}
        }
        break;
      }
    }
  }

  g_free(name);
}
#endif //0

/* Redraw all the viewers in the current tab */
void redraw(GtkWidget *widget, gpointer user_data)
{
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;
  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(tab->attributes, "hooks/redraw", LTTV_POINTER, &value));

  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp != NULL)
    lttv_hooks_call(tmp,NULL);
}


void continue_processing(GtkWidget *widget, gpointer user_data)
{
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;
  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(tab->attributes,
     "hooks/continue", LTTV_POINTER, &value));

  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp != NULL)
    lttv_hooks_call(tmp,NULL);
}

/* Stop the processing for the calling main window's current tab.
 * It removes every processing requests that are in its list. It does not call
 * the end request hooks, because the request is not finished.
 */

void stop_processing(GtkWidget *widget, gpointer user_data)
{
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;
  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }
  GSList *iter = tab->events_requests;
  
  while(iter != NULL) {
    GSList *remove_iter = iter;
    iter = g_slist_next(iter);
    
    g_free(remove_iter->data);
    tab->events_requests = 
                       g_slist_remove_link(tab->events_requests, remove_iter);
  }
  tab->events_request_pending = FALSE;
  g_idle_remove_by_data(tab);
  g_assert(g_slist_length(tab->events_requests) == 0);
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
  TimeInterval time_span;
  TimeWindow new_time_window;
  LttTime    current_time, time_delta, time_s, time_e, time_tmp;
  MainWindow * mw_data = get_window_data_struct(widget);
  LttvTracesetContext *tsc;
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  if(size == 1) return;

  tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  time_span = tsc->time_span;
  new_time_window =  tab->time_window;
  current_time = tab->current_time;
  
  time_delta = ltt_time_sub(time_span.end_time,time_span.start_time);
  if(size == 0){
    new_time_window.start_time = time_span.start_time;
    new_time_window.time_width = time_delta;
  }else{
    new_time_window.time_width = ltt_time_div(new_time_window.time_width, size);
    if(ltt_time_compare(new_time_window.time_width,time_delta) > 0)
    { /* Case where zoom out is bigger than trace length */
      new_time_window.start_time = time_span.start_time;
      new_time_window.time_width = time_delta;
    }
    else
    {
      /* Center the image on the current time */
      new_time_window.start_time = 
        ltt_time_sub(current_time, ltt_time_div(new_time_window.time_width, 2.0));
      /* If on borders, don't fall off */
      if(ltt_time_compare(new_time_window.start_time, time_span.start_time) <0)
      {
        new_time_window.start_time = time_span.start_time;
      }
      else 
      {
        if(ltt_time_compare(
           ltt_time_add(new_time_window.start_time, new_time_window.time_width),
           time_span.end_time) > 0)
        {
          new_time_window.start_time = 
                  ltt_time_sub(time_span.end_time, new_time_window.time_width);
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
  //set_time_window(tab, &new_time_window);
  // in expose now call_pending_read_hooks(mw_data);
  //gtk_multi_vpaned_set_adjust(tab->multi_vpaned, &new_time_window, FALSE);
  //
  //

 LttTime rel_time =
       ltt_time_sub(new_time_window.start_time, time_span.start_time); 
 if(   ltt_time_to_double(new_time_window.time_width)
                             * NANOSECONDS_PER_SECOND
                             / SCROLL_STEP_PER_PAGE/* step increment */
       +
       ltt_time_to_double(rel_time) * NANOSECONDS_PER_SECOND /* page size */
                    == 
       ltt_time_to_double(rel_time) * NANOSECONDS_PER_SECOND /* page size */
       ) {
    g_warning("Can not zoom that far due to scrollbar precision");
 } else if(
     ltt_time_compare(
       ltt_time_from_double( 
            ltt_time_to_double(new_time_window.time_width)
                                 /SCROLL_STEP_PER_PAGE ),
       ltt_time_zero)
     == 0 ) {
    g_warning("Can not zoom that far due to time nanosecond precision");
 } else {
   time_change_manager(tab, new_time_window);
#if 0
    /* Set scrollbar */
    GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(tab->scrollbar));
        
    g_object_set(G_OBJECT(adjustment),
                 //"value",
                 //ltt_time_to_double(new_time_window.start_time) 
                 //  * NANOSECONDS_PER_SECOND, /* value */
                 "lower",
                   0.0, /* lower */
                 "upper",
                 ltt_time_to_double(
                   ltt_time_sub(time_span.end_time, time_span.start_time))
                   * NANOSECONDS_PER_SECOND, /* upper */
                 "step_increment",
                 ltt_time_to_double(new_time_window.time_width)
                               / SCROLL_STEP_PER_PAGE
                               * NANOSECONDS_PER_SECOND, /* step increment */
                 "page_increment",
                 ltt_time_to_double(new_time_window.time_width) 
                   * NANOSECONDS_PER_SECOND, /* page increment */
                 "page_size",
                 ltt_time_to_double(new_time_window.time_width) 
                   * NANOSECONDS_PER_SECOND, /* page size */
                 NULL);
    gtk_adjustment_changed(adjustment);
    //gtk_range_set_adjustment(GTK_RANGE(tab->scrollbar), adjustment);
    //gtk_adjustment_value_changed(adjustment);
    g_object_set(G_OBJECT(adjustment),
                 "value",
                 ltt_time_to_double(
                   ltt_time_sub(new_time_window.start_time, time_span.start_time))
                   * NANOSECONDS_PER_SECOND, /* value */
                 NULL);
    gtk_adjustment_value_changed(adjustment);
   

    //g_object_set(G_OBJECT(adjustment),
    //             "value",
    //             ltt_time_to_double(time_window->start_time) 
    //               * NANOSECONDS_PER_SECOND, /* value */
    //               NULL);
    /* Note : the set value will call set_time_window if scrollbar value changed
     */
    //gtk_adjustment_set_value(adjustment,
    //                         ltt_time_to_double(new_time_window.start_time)
    //                         * NANOSECONDS_PER_SECOND);
#endif //0
  }
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

Tab *create_new_tab(GtkWidget* widget, gpointer user_data){
  gchar label[PATH_MAX];
  MainWindow * mw_data = get_window_data_struct(widget);

  GtkNotebook * notebook = (GtkNotebook *)lookup_widget(widget, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return NULL;
  }
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *copy_tab;

  if(!page) {
    copy_tab = NULL;
  } else {
    copy_tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }
  
  strcpy(label,"Page");
  if(get_label(mw_data, label,"Get the name of the tab","Please input tab's name"))    
    return (create_tab (mw_data, copy_tab, notebook, label));
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
on_close_tab_activate                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
  gint page_num;
  GtkWidget * notebook;
  GtkWidget * page;
  MainWindow * mw_data = get_window_data_struct(widget);
  notebook = lookup_widget(widget, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }

  page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  
  gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);

}

void
on_close_tab_X_clicked                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
  gint page_num;
  GtkWidget *notebook = lookup_widget(widget, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
 
  if((page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), widget)) != -1)
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);

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

#if 0
void
on_trace_filter_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  LttvTracesetSelector * s;
  GtkWidget * w;
  GtkWidget * notebook = lookup_widget(GTK_WIDGET(menuitem), "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
  }

  w = gtk_multivpaned_get_widget(GTK_MULTIVPANED(tab->multivpaned));
  
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
#endif //0

void
on_trace_facility_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Trace facility selector: %s\n");  
}


/* Dispaly a file selection dialogue to let user select a library, then call
 * lttv_library_load().
 */

void
on_load_library_activate                (GtkMenuItem     *menuitem,
                                         gpointer         user_data)
{
  GError *error = NULL;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  gchar load_module_path_alter[PATH_MAX];
  {
    GPtrArray *name;
    guint nb,i;
    gchar *load_module_path;
    name = g_ptr_array_new();
    nb = lttv_library_path_number();
    /* ask for the library path */

    for(i=0;i<nb;i++){
      gchar *path;
      path = lttv_library_path_get(i);
      g_ptr_array_add(name, path);
    }

    load_module_path = get_selection((char **)(name->pdata), name->len,
                             "Select a library path", "Library paths");
    if(load_module_path != NULL)
      strncpy(load_module_path_alter, load_module_path, PATH_MAX-1); // -1 for /

    g_ptr_array_free(name, TRUE);

    if(load_module_path == NULL) return;
  }

  {
    /* Make sure the module path ends with a / */
    gchar *ptr = load_module_path_alter;

    ptr = strchr(ptr, '\0');

    if(*(ptr-1) != '/') {
      *ptr = '/';
      *(ptr+1) = '\0';
    }
  }

  {
    /* Ask for the library to load : list files in the previously selected
     * directory */
    gchar str[PATH_MAX];
    gchar ** dir;
    gint id;
    GtkFileSelection * file_selector =
      (GtkFileSelection *)gtk_file_selection_new("Select a module");
    gtk_file_selection_set_filename(file_selector, load_module_path_alter);
    gtk_file_selection_hide_fileop_buttons(file_selector);
    
    str[0] = '\0';
    id = gtk_dialog_run(GTK_DIALOG(file_selector));
    switch(id){
      case GTK_RESPONSE_ACCEPT:
      case GTK_RESPONSE_OK:
        dir = gtk_file_selection_get_selections (file_selector);
        strncpy(str,dir[0],PATH_MAX);
        strncpy(remember_plugins_dir,dir[0],PATH_MAX);
        /* only keep file name */
        gchar *str1;
        str1 = strrchr(str,'/');
        if(str1)str1++;
        else{
          str1 = strrchr(str,'\\');
          str1++;
        }
#if 0
        /* remove "lib" */
        if(*str1 == 'l' && *(str1+1)== 'i' && *(str1+2)=='b')
          str1=str1+3;
         remove info after . */
        {
          gchar *str2 = str1;

          str2 = strrchr(str2, '.');
          if(str2 != NULL) *str2 = '\0';
        }
        lttv_module_require(str1, &error);
#endif //0   
        lttv_library_load(str1, &error);
        if(error != NULL) g_warning(error->message);
        else g_printf("Load library: %s\n", str);
        g_strfreev(dir);
      case GTK_RESPONSE_REJECT:
      case GTK_RESPONSE_CANCEL:
      default:
        gtk_widget_destroy((GtkWidget*)file_selector);
        break;
    }

  }



}


/* Display all loaded modules, let user to select a module to unload
 * by calling lttv_module_unload
 */

void
on_unload_library_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  LttvLibrary *library;
  {
    GPtrArray *name;
    guint nb,i;
    gchar *lib_name;
    name = g_ptr_array_new();
    nb = lttv_library_number();
    LttvLibraryInfo *lib_info = g_new(LttvLibraryInfo,nb);
    /* ask for the library name */

    for(i=0;i<nb;i++){
      LttvLibrary *iter_lib = lttv_library_get(i);
      lttv_library_info(iter_lib, &lib_info[i]);
      
      gchar *path = lib_info[i].name;
      g_ptr_array_add(name, lib_info[i].name);
    }
    lib_name = get_selection((char **)(name->pdata), name->len,
                             "Select a library", "Libraries");
    if(lib_name != NULL) {
      for(i=0;i<nb;i++){
        if(strcmp(lib_name, lib_info[i].name) == 0) {
          library = lttv_library_get(i);
          break;
        }
      }
    }
    g_ptr_array_free(name, TRUE);
    g_free(lib_info);

    if(lib_name == NULL) return;
  }
  
  lttv_library_unload(library);
}


/* Dispaly a file selection dialogue to let user select a module, then call
 * lttv_module_require().
 */

void
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GError *error = NULL;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  LttvLibrary *library;
  {
    GPtrArray *name;
    guint nb,i;
    gchar *lib_name;
    name = g_ptr_array_new();
    nb = lttv_library_number();
    LttvLibraryInfo *lib_info = g_new(LttvLibraryInfo,nb);
    /* ask for the library name */

    for(i=0;i<nb;i++){
      LttvLibrary *iter_lib = lttv_library_get(i);
      lttv_library_info(iter_lib, &lib_info[i]);
      
      gchar *path = lib_info[i].name;
      g_ptr_array_add(name, path);
    }
    lib_name = get_selection((char **)(name->pdata), name->len,
                             "Select a library", "Libraries");
    if(lib_name != NULL) {
      for(i=0;i<nb;i++){
        if(strcmp(lib_name, lib_info[i].name) == 0) {
          library = lttv_library_get(i);
          break;
        }
      }
    }
    g_ptr_array_free(name, TRUE);
    g_free(lib_info);

    if(lib_name == NULL) return;
  }

  //LttvModule *module;
  gchar module_name_out[PATH_MAX];
  {
    /* Ask for the module to load : list modules in the selected lib */
    GPtrArray *name;
    guint nb,i;
    gchar *module_name;
    LttvModuleInfo *module_info = g_new(LttvModuleInfo,nb);
    name = g_ptr_array_new();
    nb = lttv_library_module_number(library);
    /* ask for the module name */

    for(i=0;i<nb;i++){
      LttvModule *iter_module = lttv_library_module_get(library, i);
      lttv_module_info(iter_module, &module_info[i]);

      gchar *path = module_info[i].name;
      g_ptr_array_add(name, path);
    }
    module_name = get_selection((char **)(name->pdata), name->len,
                             "Select a module", "Modules");
    if(module_name != NULL) {
      for(i=0;i<nb;i++){
        if(strcmp(module_name, module_info[i].name) == 0) {
          strncpy(module_name_out, module_name, PATH_MAX);
          //module = lttv_library_module_get(i);
          break;
        }
      }
    }

    g_ptr_array_free(name, TRUE);
    g_free(module_info);

    if(module_name == NULL) return;
  }
  
  lttv_module_require(module_name_out, &error);
  if(error != NULL) g_warning(error->message);
  else g_printf("Load module: %s\n", module_name_out);


#if 0
  {


    gchar str[PATH_MAX];
    gchar ** dir;
    gint id;
    GtkFileSelection * file_selector =
      (GtkFileSelection *)gtk_file_selection_new("Select a module");
    gtk_file_selection_set_filename(file_selector, load_module_path_alter);
    gtk_file_selection_hide_fileop_buttons(file_selector);
    
    str[0] = '\0';
    id = gtk_dialog_run(GTK_DIALOG(file_selector));
    switch(id){
      case GTK_RESPONSE_ACCEPT:
      case GTK_RESPONSE_OK:
        dir = gtk_file_selection_get_selections (file_selector);
        strncpy(str,dir[0],PATH_MAX);
        strncpy(remember_plugins_dir,dir[0],PATH_MAX);
        {
          /* only keep file name */
          gchar *str1;
          str1 = strrchr(str,'/');
          if(str1)str1++;
          else{
            str1 = strrchr(str,'\\');
            str1++;
          }
#if 0
        /* remove "lib" */
        if(*str1 == 'l' && *(str1+1)== 'i' && *(str1+2)=='b')
          str1=str1+3;
         remove info after . */
        {
          gchar *str2 = str1;

          str2 = strrchr(str2, '.');
          if(str2 != NULL) *str2 = '\0';
        }
        lttv_module_require(str1, &error);
#endif //0   
        lttv_library_load(str1, &error);
        if(error != NULL) g_warning(error->message);
        else g_printf("Load library: %s\n", str);
        g_strfreev(dir);
      case GTK_RESPONSE_REJECT:
      case GTK_RESPONSE_CANCEL:
      default:
        gtk_widget_destroy((GtkWidget*)file_selector);
        break;
    }

  }
#endif //0


}



/* Display all loaded modules, let user to select a module to unload
 * by calling lttv_module_unload
 */

void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GError *error = NULL;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  LttvLibrary *library;
  {
    GPtrArray *name;
    guint nb,i;
    gchar *lib_name;
    name = g_ptr_array_new();
    nb = lttv_library_number();
    LttvLibraryInfo *lib_info = g_new(LttvLibraryInfo,nb);
    /* ask for the library name */

    for(i=0;i<nb;i++){
      LttvLibrary *iter_lib = lttv_library_get(i);
      lttv_library_info(iter_lib, &lib_info[i]);
      
      gchar *path = lib_info[i].name;
      g_ptr_array_add(name, path);
    }
    lib_name = get_selection((char **)(name->pdata), name->len,
                             "Select a library", "Libraries");
    if(lib_name != NULL) {
      for(i=0;i<nb;i++){
        if(strcmp(lib_name, lib_info[i].name) == 0) {
          library = lttv_library_get(i);
          break;
        }
      }
    }
    g_ptr_array_free(name, TRUE);
    g_free(lib_info);

    if(lib_name == NULL) return;
  }

  LttvModule *module;
  {
    /* Ask for the module to load : list modules in the selected lib */
    GPtrArray *name;
    guint nb,i;
    gchar *module_name;
    nb = lttv_library_module_number(library);
    LttvModuleInfo *module_info = g_new(LttvModuleInfo,nb);
    name = g_ptr_array_new();
    /* ask for the module name */

    for(i=0;i<nb;i++){
      LttvModule *iter_module = lttv_library_module_get(library, i);
      lttv_module_info(iter_module, &module_info[i]);

      gchar *path = module_info[i].name;
      if(module_info[i].use_count > 0) g_ptr_array_add(name, path);
    }
    module_name = get_selection((char **)(name->pdata), name->len,
                             "Select a module", "Modules");
    if(module_name != NULL) {
      for(i=0;i<nb;i++){
        if(strcmp(module_name, module_info[i].name) == 0) {
          module = lttv_library_module_get(library, i);
          break;
        }
      }
    }

    g_ptr_array_free(name, TRUE);
    g_free(module_info);

    if(module_name == NULL) return;
  }
  
  LttvModuleInfo module_info;
  lttv_module_info(module, &module_info);
  g_printf("Release module: %s\n", module_info.name);
 
  lttv_module_release(module);
}


/* Display a directory dialogue to let user select a path for library searching
 */

void
on_add_library_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select library path");
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
      strncpy(remember_plugins_dir,dir,PATH_MAX);
      strncat(remember_plugins_dir,"/",PATH_MAX);
      lttv_library_path_add(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
}


/* Display a directory dialogue to let user select a path for library searching
 */

void
on_remove_library_search_path_activate     (GtkMenuItem     *menuitem,
                                            gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  const char *lib_path;
  {
    GPtrArray *name;
    guint nb,i;
    gchar *lib_name;
    name = g_ptr_array_new();
    nb = lttv_library_path_number();
    /* ask for the library name */

    for(i=0;i<nb;i++){
      gchar *path = lttv_library_path_get(i);
      g_ptr_array_add(name, path);
    }
    lib_path = get_selection((char **)(name->pdata), name->len,
                             "Select a library path", "Library paths");

    g_ptr_array_free(name, TRUE);

    if(lib_path == NULL) return;
  }
  
  lttv_library_path_remove(lib_path);
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


static void 
on_about_close_activate                (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *about_widget = GTK_WIDGET(user_data);

  gtk_widget_destroy(about_widget);
}

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow *main_window = get_window_data_struct(GTK_WIDGET(menuitem));
  GtkWidget *window_widget = main_window->mwindow;
  GtkWidget *about_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkWindow *about_window = GTK_WINDOW(about_widget);
  gint window_width, window_height;
  
  gtk_window_set_title(about_window, "About Linux Trace Toolkit");

  gtk_window_set_resizable(about_window, FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(window_widget), about_window);
  gtk_window_set_destroy_with_parent(about_window, TRUE);
  gtk_window_set_modal(about_window, FALSE);

  /* Put the about window at the center of the screen */
  gtk_window_get_size(about_window, &window_width, &window_height);
  gtk_window_move (about_window,
                   (gdk_screen_width() - window_width)/2,
                   (gdk_screen_height() - window_height)/2);
 
  GtkWidget *vbox = gtk_vbox_new(FALSE, 1);

  gtk_container_add(GTK_CONTAINER(about_widget), vbox);

    
  /* Text to show */
  GtkWidget *label1 = gtk_label_new("");
  gtk_misc_set_padding(GTK_MISC(label1), 10, 20);
  gtk_label_set_markup(GTK_LABEL(label1), "\
<big>Linux Trace Toolkit</big>");
  gtk_label_set_justify(GTK_LABEL(label1), GTK_JUSTIFY_CENTER);
  
  GtkWidget *label2 = gtk_label_new("");
  gtk_misc_set_padding(GTK_MISC(label2), 10, 20);
  gtk_label_set_markup(GTK_LABEL(label2), "\
Project author: Karim Yaghmour\n\
\n\
Contributors :\n\
\n\
Michel Dagenais (New trace format, lttv main)\n\
Mathieu Desnoyers (Directory structure, build with automake/conf,\n\
                   lttv gui, control flow view, gui green threads\n\
                   with interruptible foreground and background computation,\n\
                   detailed event list)\n\
Benoit Des Ligneris (Cluster adaptation)\n\
Xang-Xiu Yang (new trace reading library and converter, lttv gui, \n\
               detailed event list and statistics view)\n\
Tom Zanussi (RelayFS)");

  GtkWidget *label3 = gtk_label_new("");
  gtk_label_set_markup(GTK_LABEL(label3), "\
Linux Trace Toolkit, Copyright (C) 2004  Karim Yaghmour\n\
Linux Trace Toolkit comes with ABSOLUTELY NO WARRANTY.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions. See COPYING for details.");
  gtk_misc_set_padding(GTK_MISC(label3), 10, 20);

  gtk_box_pack_start_defaults(GTK_BOX(vbox), label1);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), label2);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), label3);

  GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  GtkWidget *close_button = gtk_button_new_with_mnemonic("_Close");
  gtk_box_pack_end(GTK_BOX(hbox), close_button, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(close_button), 20);

  g_signal_connect(G_OBJECT(close_button), "clicked",
      G_CALLBACK(on_about_close_activate),
      (gpointer)about_widget);
  
  gtk_widget_show_all(about_widget);
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
on_button_redraw_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  redraw((GtkWidget*)button, user_data);
}

void
on_button_continue_processing_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
  continue_processing((GtkWidget*)button, user_data);
}

void
on_button_stop_processing_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_processing((GtkWidget*)button, user_data);
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

}


void time_change_manager               (Tab *tab,
                                        TimeWindow new_time_window)
{
  /* Only one source of time change */
  if(tab->time_manager_lock == TRUE) return;

  tab->time_manager_lock = TRUE;

  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  LttTime start_time = new_time_window.start_time;
  LttTime end_time = ltt_time_add(new_time_window.start_time,
                                  new_time_window.time_width);

  /* Set scrollbar */
  GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(tab->scrollbar));
  LttTime upper = ltt_time_sub(time_span.end_time, time_span.start_time);
#if 0  
  gtk_range_set_increments(GTK_RANGE(tab->scrollbar),
               ltt_time_to_double(new_time_window.time_width)
                             / SCROLL_STEP_PER_PAGE
                             * NANOSECONDS_PER_SECOND, /* step increment */
               ltt_time_to_double(new_time_window.time_width) 
                 * NANOSECONDS_PER_SECOND); /* page increment */
  gtk_range_set_range(GTK_RANGE(tab->scrollbar),
                 0.0, /* lower */
               ltt_time_to_double(upper) 
                 * NANOSECONDS_PER_SECOND); /* upper */
#endif //0
  g_object_set(G_OBJECT(adjustment),
               "lower",
                 0.0, /* lower */
               "upper",
               ltt_time_to_double(upper) 
                 * NANOSECONDS_PER_SECOND, /* upper */
               "step_increment",
               ltt_time_to_double(new_time_window.time_width)
                             / SCROLL_STEP_PER_PAGE
                             * NANOSECONDS_PER_SECOND, /* step increment */
               "page_increment",
               ltt_time_to_double(new_time_window.time_width) 
                 * NANOSECONDS_PER_SECOND, /* page increment */
               "page_size",
               ltt_time_to_double(new_time_window.time_width) 
                 * NANOSECONDS_PER_SECOND, /* page size */
               NULL);
  gtk_adjustment_changed(adjustment);

 // g_object_set(G_OBJECT(adjustment),
 //              "value",
 //              ltt_time_to_double(
 //               ltt_time_sub(start_time, time_span.start_time))
 //                  * NANOSECONDS_PER_SECOND, /* value */
 //              NULL);
  //gtk_adjustment_value_changed(adjustment);
  gtk_range_set_value(GTK_RANGE(tab->scrollbar),
               ltt_time_to_double(
                ltt_time_sub(start_time, time_span.start_time))
                   * NANOSECONDS_PER_SECOND /* value */);

  /* set the time bar. */
  /* start seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry1),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry1),
                            (double)start_time.tv_sec);

  /* start nanoseconds */
  if(start_time.tv_sec == time_span.start_time.tv_sec) {
    /* can be both beginning and end at the same time. */
    if(start_time.tv_sec == time_span.end_time.tv_sec) {
      /* If we are at the end, max nsec to end..  -1 (not zero length) */
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry2),
                                (double)time_span.start_time.tv_nsec,
                                (double)time_span.end_time.tv_nsec-1);
    } else {
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry2),
                                (double)time_span.start_time.tv_nsec,
                                (double)NANOSECONDS_PER_SECOND-1);
    }
  } else if(start_time.tv_sec == time_span.end_time.tv_sec) {
      /* If we are at the end, max nsec to end..  -1 (not zero length) */
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry2),
                                0.0,
                                (double)time_span.end_time.tv_nsec-1);
  } else /* anywhere else */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry2),
                              0.0,
                              (double)NANOSECONDS_PER_SECOND-1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry2),
                            (double)start_time.tv_nsec);

  /* end seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry3),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry3),
                            (double)end_time.tv_sec);

  /* end nanoseconds */
  if(end_time.tv_sec == time_span.start_time.tv_sec) {
    /* can be both beginning and end at the same time. */
    if(end_time.tv_sec == time_span.end_time.tv_sec) {
      /* If we are at the end, max nsec to end.. */
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry4),
                                (double)time_span.start_time.tv_nsec+1,
                                (double)time_span.end_time.tv_nsec);
    } else {
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry4),
                                (double)time_span.start_time.tv_nsec+1,
                                (double)NANOSECONDS_PER_SECOND-1);
    }
  }
  else if(end_time.tv_sec == time_span.end_time.tv_sec) {
    /* If we are at the end, max nsec to end.. */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry4),
                              0.0,
                              (double)time_span.end_time.tv_nsec);
  }
  else /* anywhere else */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry4),
                              0.0,
                              (double)NANOSECONDS_PER_SECOND-1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry4),
                            (double)end_time.tv_nsec);

  /* call viewer hooks for new time window */
  set_time_window(tab, &new_time_window);

  tab->time_manager_lock = FALSE;
}


/* value changed for frame start s
 *
 * Check time span : if ns is out of range, clip it the nearest good value.
 */
void
on_MEntry1_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);

  TimeWindow new_time_window = tab->time_window;
 
  LttTime end_time = ltt_time_add(new_time_window.start_time,
                                  new_time_window.time_width);

  new_time_window.start_time.tv_sec = value;

  /* start nanoseconds */
  if(new_time_window.start_time.tv_sec == time_span.start_time.tv_sec) {
    if(new_time_window.start_time.tv_sec == time_span.end_time.tv_sec) {
      if(new_time_window.start_time.tv_nsec > time_span.end_time.tv_nsec)
        new_time_window.start_time.tv_nsec = time_span.end_time.tv_nsec-1;
      if(new_time_window.start_time.tv_nsec < time_span.start_time.tv_nsec)
        new_time_window.start_time.tv_nsec = time_span.start_time.tv_nsec;
    } else {
      if(new_time_window.start_time.tv_nsec < time_span.start_time.tv_nsec)
        new_time_window.start_time.tv_nsec = time_span.start_time.tv_nsec;
    }
  }
  else if(new_time_window.start_time.tv_sec == time_span.end_time.tv_sec) {
    if(new_time_window.start_time.tv_nsec > time_span.end_time.tv_nsec)
      new_time_window.start_time.tv_nsec = time_span.end_time.tv_nsec-1;
  }

  /* check if end time selected is below or equal */
  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push back end time : keep the same time width
     * if possible, else end traceset time */
    end_time = LTT_TIME_MIN(time_span.end_time,
                                  ltt_time_add(new_time_window.start_time,
                                               new_time_window.time_width)
                                 );
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);

  time_change_manager(tab, new_time_window);

}

void
on_MEntry2_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);

  TimeWindow new_time_window = tab->time_window;
 
  LttTime end_time = ltt_time_add(new_time_window.start_time,
                                  new_time_window.time_width);

  new_time_window.start_time.tv_nsec = value;

  /* check if end time selected is below or equal */
  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push back end time : keep the same time width
     * if possible, else end traceset time */
    end_time = LTT_TIME_MIN(time_span.end_time,
                                  ltt_time_add(new_time_window.start_time,
                                               new_time_window.time_width)
                                 );
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);

  time_change_manager(tab, new_time_window);

}

void
on_MEntry3_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);

  TimeWindow new_time_window = tab->time_window;
 
  LttTime end_time = ltt_time_add(new_time_window.start_time,
                                  new_time_window.time_width);
  end_time.tv_sec = value;

  /* end nanoseconds */
  if(end_time.tv_sec == time_span.start_time.tv_sec) {
    if(end_time.tv_sec == time_span.end_time.tv_sec) {
      if(end_time.tv_nsec > time_span.end_time.tv_nsec)
        end_time.tv_nsec = time_span.end_time.tv_nsec;
      if(end_time.tv_nsec < time_span.start_time.tv_nsec)
        end_time.tv_nsec = time_span.start_time.tv_nsec+1;
    } else {
      if(end_time.tv_nsec < time_span.start_time.tv_nsec)
        end_time.tv_nsec = time_span.start_time.tv_nsec+1;
    }
  }
  else if(end_time.tv_sec == time_span.end_time.tv_sec) {
    if(end_time.tv_nsec > time_span.end_time.tv_nsec)
      end_time.tv_nsec = time_span.end_time.tv_nsec;
  }

  /* check if end time selected is below or equal */
  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push front start time : keep the same time width
     * if possible, else end traceset time */
    new_time_window.start_time = LTT_TIME_MAX(time_span.start_time,
                                        ltt_time_sub(end_time,
                                                     new_time_window.time_width)
                                             );
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);

  time_change_manager(tab, new_time_window);

}

void
on_MEntry4_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);

  TimeWindow new_time_window = tab->time_window;
 
  LttTime end_time = ltt_time_add(new_time_window.start_time,
                                  new_time_window.time_width);
  end_time.tv_nsec = value;

  /* check if end time selected is below or equal */
  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push front start time : keep the same time width
     * if possible, else end traceset time */
    new_time_window.start_time = LTT_TIME_MAX(time_span.start_time,
                                        ltt_time_sub(end_time,
                                                     new_time_window.time_width)
                                             );
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);

  time_change_manager(tab, new_time_window);

}


void current_time_change_manager       (Tab *tab,
                                        LttTime new_current_time)
{
  /* Only one source of time change */
  if(tab->current_time_manager_lock == TRUE) return;

  tab->current_time_manager_lock = TRUE;

  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;

  /* current seconds */
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry5),
                            (double)time_span.start_time.tv_sec,
                            (double)time_span.end_time.tv_sec);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry5),
                            (double)new_current_time.tv_sec);


  /* start nanoseconds */
  if(new_current_time.tv_sec == time_span.start_time.tv_sec) {
    /* can be both beginning and end at the same time. */
    if(new_current_time.tv_sec == time_span.end_time.tv_sec) {
      /* If we are at the end, max nsec to end..  */
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry6),
                                (double)time_span.start_time.tv_nsec,
                                (double)time_span.end_time.tv_nsec);
    } else {
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry6),
                                (double)time_span.start_time.tv_nsec,
                                (double)NANOSECONDS_PER_SECOND-1);
    }
  } else if(new_current_time.tv_sec == time_span.end_time.tv_sec) {
      /* If we are at the end, max nsec to end..  */
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry6),
                                0.0,
                                (double)time_span.end_time.tv_nsec);
  } else /* anywhere else */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry6),
                              0.0,
                              (double)NANOSECONDS_PER_SECOND-1);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry6),
                            (double)new_current_time.tv_nsec);

  set_current_time(tab, &new_current_time);

  tab->current_time_manager_lock = FALSE;
}

void
on_MEntry5_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab = (Tab*)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);
  LttTime new_current_time = tab->current_time;
  new_current_time.tv_sec = value;

  /* current nanoseconds */
  if(new_current_time.tv_sec == time_span.start_time.tv_sec) {
    if(new_current_time.tv_sec == time_span.end_time.tv_sec) {
      if(new_current_time.tv_nsec > time_span.end_time.tv_nsec)
        new_current_time.tv_nsec = time_span.end_time.tv_nsec;
      if(new_current_time.tv_nsec < time_span.start_time.tv_nsec)
        new_current_time.tv_nsec = time_span.start_time.tv_nsec;
    } else {
      if(new_current_time.tv_nsec < time_span.start_time.tv_nsec)
        new_current_time.tv_nsec = time_span.start_time.tv_nsec;
    }
  }
  else if(new_current_time.tv_sec == time_span.end_time.tv_sec) {
    if(new_current_time.tv_nsec > time_span.end_time.tv_nsec)
      new_current_time.tv_nsec = time_span.end_time.tv_nsec;
  }

  current_time_change_manager(tab, new_current_time);
}

void
on_MEntry6_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab = (Tab*)user_data;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);
  LttTime new_current_time = tab->current_time;
  new_current_time.tv_nsec = value;

  current_time_change_manager(tab, new_current_time);
}


void scroll_value_changed_cb(GtkWidget *scrollbar,
                             gpointer user_data)
{
  Tab *tab = (Tab *)user_data;
  TimeWindow new_time_window;
  LttTime time;
  GtkAdjustment *adjust = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
  gdouble value = gtk_adjustment_get_value(adjust);
 // gdouble upper, lower, ratio, page_size;
  gdouble page_size;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;

  time = ltt_time_add(ltt_time_from_double(value/NANOSECONDS_PER_SECOND),
                      time_span.start_time);

  new_time_window.start_time = time;
  
  page_size = adjust->page_size;

  new_time_window.time_width = 
    ltt_time_from_double(page_size/NANOSECONDS_PER_SECOND);


  time_change_manager(tab, new_time_window);
#if 0
  //time_window = tab->time_window;

  lower = adjust->lower;
  upper = adjust->upper;
  ratio = (value - lower) / (upper - lower);
  g_info("lower %lu, upper %lu, value %lu, ratio %lu", lower, upper, value, ratio);
  
  //time = ltt_time_sub(time_span->end_time, time_span->start_time);
  //time = ltt_time_mul(time, (float)ratio);
  //time = ltt_time_add(time_span->start_time, time);
  time = ltt_time_add(ltt_time_from_double(value/NANOSECONDS_PER_SECOND),
                      time_span.start_time);

  time_window.start_time = time;
  
  page_size = adjust->page_size;

  time_window.time_width = 
    ltt_time_from_double(page_size/NANOSECONDS_PER_SECOND);
  //time = ltt_time_sub(time_span.end_time, time);
  //if(ltt_time_compare(time,time_window.time_width) < 0){
  //  time_window.time_width = time;
  //}

  /* call viewer hooks for new time window */
  set_time_window(tab, &time_window);
#endif //0
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


/* Select a module which will be loaded
 */

char * get_load_module(char ** load_module_name, int nb_module)
{
  return get_selection(load_module_name, nb_module, 
		       "Select a module to load", "Module name");
}




/* Select a module which will be unloaded
 */

char * get_unload_module(char ** loaded_module_name, int nb_module)
{
  return get_selection(loaded_module_name, nb_module, 
		       "Select a module to unload", "Module name");
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
  Tab *new_tab;
         
  new_m_window = g_new(MainWindow, 1);

  // Add the object's information to the module's array 
  g_main_window_list = g_slist_append(g_main_window_list, new_m_window);

  new_window  = create_MWindow();
  gtk_widget_show (new_window);
    
  new_m_window->mwindow = new_window;
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
  //gtk_notebook_popup_enable (GTK_NOTEBOOK(notebook));
  //for now there is no name field in LttvTraceset structure
  //Use "Traceset" as the label for the default tab
  if(parent) {
    GtkWidget * parent_notebook = lookup_widget(parent->mwindow, "MNotebook");
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(parent_notebook),
                 gtk_notebook_get_current_page(GTK_NOTEBOOK(parent_notebook)));
    Tab *parent_tab;

    if(!page) {
      parent_tab = NULL;
    } else {
      parent_tab = (Tab *)g_object_get_data(G_OBJECT(page), "Tab_Info");
    }
    new_tab = create_tab(new_m_window, parent_tab, notebook, "Traceset");
  } else {
    new_tab = create_tab(new_m_window, NULL, notebook, "Traceset");
    /* First window, use command line trace */
    if(g_init_trace != NULL){
      lttvwindow_add_trace(new_tab,
                           g_init_trace);

    }
    LttvTraceset *traceset = new_tab->traceset_info->traceset;
    SetTraceset(new_tab, traceset);
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
      }
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

Tab* create_tab(MainWindow * mw, Tab *copy_tab, 
		  GtkNotebook * notebook, char * label)
{
  GList * list;
  Tab * tab;
  LttTime tmp_time;
  
  //create a new tab data structure
  tab = g_new(Tab,1);

  //construct and initialize the traceset_info
  tab->traceset_info = g_new(TracesetInfo,1);

  if(copy_tab) {
    tab->traceset_info->traceset = 
      lttv_traceset_copy(copy_tab->traceset_info->traceset);
  } else {
    tab->traceset_info->traceset = lttv_traceset_new();
  }

#ifdef DEBUG
  lttv_attribute_write_xml(
      lttv_traceset_attribute(tab->traceset_info->traceset),
      stdout,
      0, 4);
  fflush(stdout);
#endif //DEBUG

  tab->time_manager_lock = FALSE;
  tab->current_time_manager_lock = FALSE;

  //FIXME copy not implemented in lower level
  tab->traceset_info->traceset_context =
    g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  g_assert(tab->traceset_info->traceset_context != NULL);
  lttv_context_init(
	    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context),
	                          tab->traceset_info->traceset);
  //add state update hooks
  lttv_state_add_event_hooks(
       (LttvTracesetState*)tab->traceset_info->traceset_context);
  
  //determine the current_time and time_window of the tab
#if 0
  if(copy_tab != NULL){
    tab->time_window      = copy_tab->time_window;
    tab->current_time     = copy_tab->current_time;
  }else{
    tab->time_window.start_time = 
	    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->
                             time_span.start_time;
    if(DEFAULT_TIME_WIDTH_S <
              LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->
                             time_span.end_time.tv_sec)
      tmp_time.tv_sec = DEFAULT_TIME_WIDTH_S;
    else
      tmp_time.tv_sec =
              LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->
                             time_span.end_time.tv_sec;
    tmp_time.tv_nsec = 0;
    tab->time_window.time_width = tmp_time ;
    tab->current_time.tv_sec = 
       LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->
                             time_span.start_time.tv_sec;
    tab->current_time.tv_nsec = 
       LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context)->
                             time_span.start_time.tv_nsec;
  }
#endif //0
  tab->attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  tab->interrupted_state = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
 
  tab->vbox = gtk_vbox_new(FALSE, 2);
  tab->viewer_container = gtk_vbox_new(TRUE, 2);
  tab->scrollbar = gtk_hscrollbar_new(NULL);
  //tab->multivpaned = gtk_multi_vpaned_new();
  
  gtk_box_pack_start(GTK_BOX(tab->vbox),
                     tab->viewer_container,
                     TRUE, /* expand */
                     TRUE, /* Give the extra space to the child */
                     0);    /* No padding */

  /* Create the timebar */
  {
    tab->MTimebar = gtk_hbox_new(FALSE, 2);
    gtk_widget_show(tab->MTimebar);

    tab->MText1 = gtk_label_new("Time Frame  start: ");
    gtk_widget_show(tab->MText1);
    tab->MText2 = gtk_label_new("s");
    gtk_widget_show(tab->MText2);
    tab->MText3a = gtk_label_new("ns");
    gtk_widget_show(tab->MText3a);
    tab->MText3b = gtk_label_new("end:");
    gtk_widget_show(tab->MText3b);
    tab->MText4 = gtk_label_new("s");
    gtk_widget_show(tab->MText4);
    tab->MText5a = gtk_label_new("ns");
    gtk_widget_show(tab->MText5a);
    tab->MText5b = gtk_label_new("Current Time:");
    gtk_widget_show(tab->MText5b);
    tab->MText6 = gtk_label_new("s");
    gtk_widget_show(tab->MText6);
    tab->MText7 = gtk_label_new("ns");
    gtk_widget_show(tab->MText7);

    tab->MEntry1 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry1),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry1),TRUE);
    gtk_widget_show(tab->MEntry1);
    tab->MEntry2 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry2),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry2),TRUE);
    gtk_widget_show(tab->MEntry2);
    tab->MEntry3 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry3),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry3),TRUE);
    gtk_widget_show(tab->MEntry3);
    tab->MEntry4 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry4),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry4),TRUE);
    gtk_widget_show(tab->MEntry4);
    tab->MEntry5 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry5),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry5),TRUE);
    gtk_widget_show(tab->MEntry5);
    tab->MEntry6 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry6),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry6),TRUE);
    gtk_widget_show(tab->MEntry6);

    
    GtkWidget *temp_widget;
    
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText3a, FALSE, FALSE, 0);
    temp_widget = gtk_vseparator_new();
    gtk_widget_show(temp_widget);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), temp_widget, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText3b, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry3, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText4, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry4, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText5a, FALSE, FALSE, 0);
    temp_widget = gtk_vseparator_new();
    gtk_widget_show(temp_widget);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MText7, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MEntry6, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MText6, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MEntry5, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MText5b, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), temp_widget, FALSE, FALSE, 0);
  }

  gtk_box_pack_end(GTK_BOX(tab->vbox),
                   tab->scrollbar,
                   FALSE, /* Do not expand */
                   FALSE, /* Fill has no effect here  (expand false) */
                   0);    /* No padding */
  
  gtk_box_pack_end(GTK_BOX(tab->vbox),
                   tab->MTimebar,
                   FALSE, /* Do not expand */
                   FALSE, /* Fill has no effect here  (expand false) */
                   0);    /* No padding */

  g_object_set_data(G_OBJECT(tab->viewer_container), "focused_viewer", NULL);


  tab->mw   = mw;
  
  /*{
    // Display a label with a X
    GtkWidget *w_hbox = gtk_hbox_new(FALSE, 4);
    GtkWidget *w_label = gtk_label_new (label);
    GtkWidget *pixmap = create_pixmap(GTK_WIDGET(notebook), "close.png");
    GtkWidget *w_button = gtk_button_new ();
    gtk_container_add(GTK_CONTAINER(w_button), pixmap);
    //GtkWidget *w_button = gtk_button_new_with_label("x");

    gtk_button_set_relief(GTK_BUTTON(w_button), GTK_RELIEF_NONE);
    
    gtk_box_pack_start(GTK_BOX(w_hbox), w_label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(w_hbox), w_button, FALSE,
                       FALSE, 0);

    g_signal_connect_swapped (w_button, "clicked",
                      G_CALLBACK (on_close_tab_X_clicked),
                      tab->multi_vpaned);

    gtk_widget_set_state(w_button, GTK_STATE_ACTIVE);

    gtk_widget_show (w_label);
    gtk_widget_show (pixmap);
    gtk_widget_show (w_button);
    gtk_widget_show (w_hbox);

    tab->label = w_hbox;
  }*/


  tab->label = gtk_label_new (label);

  gtk_widget_show(tab->label);
  gtk_widget_show(tab->scrollbar);
  gtk_widget_show(tab->viewer_container);
  gtk_widget_show(tab->vbox);
  //gtk_widget_show(tab->multivpaned);


  /* Start with empty events requests list */
  tab->events_requests = NULL;
  tab->events_request_pending = FALSE;

  g_object_set_data_full(
           G_OBJECT(tab->vbox),
           "Tab_Info",
	         tab,
	         (GDestroyNotify)tab_destructor);

  g_signal_connect(G_OBJECT(tab->scrollbar), "value-changed",
      G_CALLBACK(scroll_value_changed_cb), tab);

  g_signal_connect ((gpointer) tab->MEntry1, "value-changed",
                    G_CALLBACK (on_MEntry1_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry2, "value-changed",
                    G_CALLBACK (on_MEntry2_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry3, "value-changed",
                    G_CALLBACK (on_MEntry3_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry4, "value-changed",
                    G_CALLBACK (on_MEntry4_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry5, "value-changed",
                    G_CALLBACK (on_MEntry5_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry6, "value-changed",
                    G_CALLBACK (on_MEntry6_value_changed),
                    tab);

  //g_signal_connect(G_OBJECT(tab->scrollbar), "changed",
  //    G_CALLBACK(scroll_value_changed_cb), tab);


 //insert tab into notebook
  gtk_notebook_append_page(notebook,
                           tab->vbox,
                           tab->label);  
  list = gtk_container_get_children(GTK_CONTAINER(notebook));
  gtk_notebook_set_current_page(notebook,g_list_length(list)-1);
  // always show : not if(g_list_length(list)>1)
  gtk_notebook_set_show_tabs(notebook, TRUE);
 
  return tab;
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

