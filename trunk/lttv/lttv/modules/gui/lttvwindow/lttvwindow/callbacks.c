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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <ltt/trace.h>
#include <ltt/time.h>
#include <ltt/event.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <lttvwindow/mainwindow.h>
#include <lttvwindow/mainwindow-private.h>
#include <lttvwindow/menu.h>
#include <lttvwindow/toolbar.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/lttv_plugin_tab.h>

static LttTime lttvwindow_default_time_width = { 1, 0 };
#define CLIP_BUF 256 // size of clipboard buffer

extern LttvTrace *g_init_trace ;


/** Array containing instanced objects. */
extern GSList * g_main_window_list;

/** MD : keep old directory. */
static char remember_plugins_dir[PATH_MAX] = "";
static char remember_trace_dir[PATH_MAX] = "";

void tab_destructor(LttvPluginTab * ptab);

MainWindow * get_window_data_struct(GtkWidget * widget);
char * get_load_module(MainWindow *mw,
    char ** load_module_name, int nb_module);
char * get_unload_module(MainWindow *mw,
    char ** loaded_module_name, int nb_module);
char * get_remove_trace(MainWindow *mw, char ** all_trace_name, int nb_trace);
char * get_selection(MainWindow *mw,
    char ** all_name, int nb, char *title, char * column_title);
void init_tab(Tab *tab, MainWindow * mw, Tab *copy_tab,
		  GtkNotebook * notebook, char * label);

static void insert_viewer(GtkWidget* widget, lttvwindow_viewer_constructor constructor);

LttvPluginTab *create_new_tab(GtkWidget* widget, gpointer user_data);

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

/* Pasting routines */

static void MEventBox1a_receive(GtkClipboard *clipboard,
                          const gchar *text,
                          gpointer data)
{
  if(text == NULL) return;
  Tab *tab = (Tab *)data;
  gchar buffer[CLIP_BUF];
  gchar *ptr = buffer, *ptr_ssec, *ptr_snsec, *ptr_esec, *ptr_ensec;

  strncpy(buffer, text, CLIP_BUF);
 
  /* start */
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_ssec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_snsec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  /* end */ 
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_esec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_ensec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry1),
                            (double)strtoul(ptr_ssec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry2),
                            (double)strtoul(ptr_snsec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry3),
                            (double)strtoul(ptr_esec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry4),
                            (double)strtoul(ptr_ensec, NULL, 10));
}

static gboolean on_MEventBox1a_paste(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  Tab *tab = (Tab*)data;

  GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_PRIMARY);
  gtk_clipboard_request_text(clip,
                             (GtkClipboardTextReceivedFunc)MEventBox1a_receive,
                             (gpointer)tab);
  return 0;
}


/* Start */
static void MEventBox1b_receive(GtkClipboard *clipboard,
                          const gchar *text,
                          gpointer data)
{
  if(text == NULL) return;
  Tab *tab = (Tab *)data;
  gchar buffer[CLIP_BUF];
  gchar *ptr = buffer, *ptr_sec, *ptr_nsec;

  strncpy(buffer, text, CLIP_BUF);
  
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_sec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_nsec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry1),
                            (double)strtoul(ptr_sec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry2),
                            (double)strtoul(ptr_nsec, NULL, 10));
}

/* Start */
static gboolean on_MEventBox1b_paste(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  Tab *tab = (Tab*)data;

  GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_PRIMARY);
  gtk_clipboard_request_text(clip,
                             (GtkClipboardTextReceivedFunc)MEventBox1b_receive,
                             (gpointer)tab);
  return 0;
}

/* End */
static void MEventBox3b_receive(GtkClipboard *clipboard,
                          const gchar *text,
                          gpointer data)
{
  if(text == NULL) return;
  Tab *tab = (Tab *)data;
  gchar buffer[CLIP_BUF];
  gchar *ptr = buffer, *ptr_sec, *ptr_nsec;

  strncpy(buffer, text, CLIP_BUF);
  
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_sec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_nsec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry3),
                            (double)strtoul(ptr_sec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry4),
                            (double)strtoul(ptr_nsec, NULL, 10));
}

/* End */
static gboolean on_MEventBox3b_paste(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  Tab *tab = (Tab*)data;

  GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_PRIMARY);
  gtk_clipboard_request_text(clip,
                             (GtkClipboardTextReceivedFunc)MEventBox3b_receive,
                             (gpointer)tab);
  return 0;
}

/* Current */
static void MEventBox5b_receive(GtkClipboard *clipboard,
                          const gchar *text,
                          gpointer data)
{
  if(text == NULL) return;
  Tab *tab = (Tab *)data;
  gchar buffer[CLIP_BUF];
  gchar *ptr = buffer, *ptr_sec, *ptr_nsec;

  strncpy(buffer, text, CLIP_BUF);
  
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_sec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_nsec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry5),
                            (double)strtoul(ptr_sec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry6),
                            (double)strtoul(ptr_nsec, NULL, 10));
}

/* Current */
static gboolean on_MEventBox5b_paste(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  Tab *tab = (Tab*)data;

  GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_PRIMARY);
  gtk_clipboard_request_text(clip,
                             (GtkClipboardTextReceivedFunc)MEventBox5b_receive,
                             (gpointer)tab);
  return 0;
}

/* Interval */
static void MEventBox8_receive(GtkClipboard *clipboard,
                          const gchar *text,
                          gpointer data)
{
  if(text == NULL) return;
  Tab *tab = (Tab *)data;
  gchar buffer[CLIP_BUF];
  gchar *ptr = buffer, *ptr_sec, *ptr_nsec;

  strncpy(buffer, text, CLIP_BUF);
  
  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_sec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';
  ptr++;

  while(!isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                       /* remove leading junk */
  ptr_nsec = ptr;
  while(isdigit(*ptr) && ptr < buffer+CLIP_BUF-1) ptr++;
                                                 /* read all the first number */
  *ptr = '\0';

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry7),
                            (double)strtoul(ptr_sec, NULL, 10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry8),
                            (double)strtoul(ptr_nsec, NULL, 10));
}

/* Interval */
static gboolean on_MEventBox8_paste(GtkWidget *widget, GdkEventButton *event,
                                gpointer data)
{
  Tab *tab = (Tab*)data;

  GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_PRIMARY);
  gtk_clipboard_request_text(clip,
                             (GtkClipboardTextReceivedFunc)MEventBox8_receive,
                             (gpointer)tab);
  return 0;
}

#if 0
static void on_top_notify(GObject    *gobject,
		GParamSpec *arg1,
		gpointer    user_data)
{
	Tab *tab = (Tab*)user_data;
	g_message("in on_top_notify.\n");

}
#endif //0
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
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(widget), TRUE);
  }
  gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT(widget),
                    "button-press-event",
                    G_CALLBACK (viewer_grab_focus),
                    (gpointer)viewer);
}

/* Stop all the processings and call gtk_main_quit() */
static void mainwindow_quit()
{
  lttvwindowtraces_unregister_requests(g_quark_from_string("stats"));
  lttvwindowtraces_unregister_requests(g_quark_from_string("state"));
  lttvwindowtraces_unregister_computation_hooks(g_quark_from_string("stats"));
  lttvwindowtraces_unregister_computation_hooks(g_quark_from_string("state"));

  gtk_main_quit();
}


/* insert_viewer function constructs an instance of a viewer first,
 * then inserts the widget of the instance into the container of the
 * main window
 */

void
insert_viewer_wrap(GtkWidget *menuitem, gpointer user_data)
{
  insert_viewer((GtkWidget*)menuitem, (lttvwindow_viewer_constructor)user_data);
}


/* internal functions */
void insert_viewer(GtkWidget* widget, lttvwindow_viewer_constructor constructor)
{
  GtkWidget * viewer_container;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget * viewer;
  TimeInterval * time_interval;
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  LttvPluginTab *ptab;
  Tab *tab;
  
  if(!page) {
    ptab = create_new_tab(widget, NULL);
  } else {
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
  }
  tab = ptab->tab;

  viewer_container = tab->viewer_container;

  viewer = (GtkWidget*)constructor(ptab);
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
  TimeWindow new_time_window = tab->time_window;
  LttTime new_current_time = tab->current_time;

  /* Set the tab's time window and current time if
   * out of bounds */
  if(ltt_time_compare(tab->time_window.start_time, time_span.start_time) < 0
     || ltt_time_compare(tab->time_window.end_time,
                           time_span.end_time) > 0) {
    new_time_window.start_time = time_span.start_time;
    
    new_current_time = time_span.start_time;
    
    LttTime tmp_time;
    
    if(ltt_time_compare(lttvwindow_default_time_width,
          ltt_time_sub(time_span.end_time, time_span.start_time)) < 0
        ||
       ltt_time_compare(time_span.end_time, time_span.start_time) == 0)
      tmp_time = lttvwindow_default_time_width;
    else
      tmp_time = time_span.end_time;

    new_time_window.time_width = tmp_time ;
    new_time_window.time_width_double = ltt_time_to_double(tmp_time);
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                            new_time_window.time_width) ;
  }

 
 
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

  time_change_manager(tab, new_time_window);
  current_time_change_manager(tab, new_current_time);

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
#if 0
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
#endif //0


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
    g_info("Main window does not exist\n");
    return NULL;
  }
  
  mw_data = (MainWindow *) g_object_get_data(G_OBJECT(mw),"main_window_data");
  if(mw_data == NULL){
    g_warning("Main window data does not exist\n");
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
    g_info("Clone : use the same traceset\n");
    construct_main_window(parent);
  }else{
    g_info("Empty : traceset is set to NULL\n");
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
  GValue value;
  memset(&value, 0, sizeof(GValue));
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
    LttvPluginTab *ptab;
    ptab = g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
    LttvPluginTab *ptab;
    ptab = g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
  
  gtk_window_set_transient_for(GTK_WINDOW(file_selector),
      GTK_WINDOW(mw_data->mwindow));

  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (file_selector);
      traceset = lttv_traceset_load(dir[0]);
      g_info("Open a trace set %s\n", dir[0]); 
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
 * Process requests for parts of the trace from viewers.
 *
 * These requests are made by lttvwindow_events_request().
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
  GtkWidget* widget;
  LttvTracesetContext *tsc;
  LttvTracefileContext *tfc;
  GSList *list_in = NULL;
  LttTime end_time;
  guint end_nb_events;
  guint count;
  LttvTracesetContextPosition *end_position;
  
  if(lttvwindow_preempt_count > 0) return TRUE;
  
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
  //g_assert(lttv_process_traceset_seek_position(tsc, sync_position) == 0);
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
#ifdef DEBUG
            g_assert(seek_count < LTTV_STATE_SAVE_INTERVAL);
#endif //DEBUG


        } else {
          LttTime pos_time;
					LttvTracefileContext *tfc =
						lttv_traceset_context_get_current_tfc(tsc);
          /* Else, the first request in list_in is a position request */
          /* If first req in list_in pos != current pos */
          g_assert(events_request->start_position != NULL);
          g_debug("SEEK POS time : %lu, %lu", 
                 lttv_traceset_context_position_get_time(
                      events_request->start_position).tv_sec,
                 lttv_traceset_context_position_get_time(
                      events_request->start_position).tv_nsec);
					
					if(tfc) {
						g_debug("SEEK POS context time : %lu, %lu", 
							 tfc->timestamp.tv_sec,
							 tfc->timestamp.tv_nsec);
					} else {
						g_debug("SEEK POS context time : %lu, %lu", 
							 ltt_time_infinite.tv_sec,
							 ltt_time_infinite.tv_nsec);
					}
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
          if(events_request->trace == -1)
            lttv_process_traceset_begin(tsc,
                events_request->before_chunk_traceset,
                events_request->before_chunk_trace,
                events_request->before_chunk_tracefile,
                events_request->event,
                events_request->event_by_id_channel);
          else {
            guint nb_trace = lttv_traceset_number(tsc->ts);
            g_assert((guint)events_request->trace < nb_trace &&
                      events_request->trace > -1);
            LttvTraceContext *tc = tsc->traces[events_request->trace];

            lttv_hooks_call(events_request->before_chunk_traceset, tsc);

            lttv_trace_context_add_hooks(tc,
                                         events_request->before_chunk_trace,
                                         events_request->before_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);
          }
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
          if(events_request->trace == -1)
            lttv_process_traceset_begin(tsc,
                events_request->before_chunk_traceset,
                events_request->before_chunk_trace,
                events_request->before_chunk_tracefile,
                events_request->event,
                events_request->event_by_id_channel);
          else {
            guint nb_trace = lttv_traceset_number(tsc->ts);
            g_assert((guint)events_request->trace < nb_trace &&
                      events_request->trace > -1);
            LttvTraceContext *tc = tsc->traces[events_request->trace];

            lttv_hooks_call(events_request->before_chunk_traceset, tsc);

            lttv_trace_context_add_hooks(tc,
                                         events_request->before_chunk_trace,
                                         events_request->before_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);
          }

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
            if(events_request->trace == -1)
              lttv_process_traceset_begin(tsc,
                  events_request->before_chunk_traceset,
                  events_request->before_chunk_trace,
                  events_request->before_chunk_tracefile,
                  events_request->event,
                  events_request->event_by_id_channel);
            else {
              guint nb_trace = lttv_traceset_number(tsc->ts);
              g_assert((guint)events_request->trace < nb_trace &&
                        events_request->trace > -1);
              LttvTraceContext *tc = tsc->traces[events_request->trace];

              lttv_hooks_call(events_request->before_chunk_traceset, tsc);

              lttv_trace_context_add_hooks(tc,
                                           events_request->before_chunk_trace,
                                           events_request->before_chunk_tracefile,
                                           events_request->event,
                                           events_request->event_by_id_channel);
          }


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
      g_debug("Calling process traceset middle with %p, %lu sec %lu nsec, %u nb ev, %p end pos", tsc, end_time.tv_sec, end_time.tv_nsec, end_nb_events, end_position);
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

          if(events_request->trace == -1) 
               lttv_process_traceset_end(tsc,
                                         events_request->after_chunk_traceset,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);

          else {
            guint nb_trace = lttv_traceset_number(tsc->ts);
            g_assert(events_request->trace < nb_trace &&
                      events_request->trace > -1);
            LttvTraceContext *tc = tsc->traces[events_request->trace];

            lttv_trace_context_remove_hooks(tc,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);
            lttv_hooks_call(events_request->after_chunk_traceset, tsc);


          }

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
          if(events_request->trace == -1) 
               lttv_process_traceset_end(tsc,
                                         events_request->after_chunk_traceset,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);

          else {
            guint nb_trace = lttv_traceset_number(tsc->ts);
            g_assert(events_request->trace < nb_trace &&
                      events_request->trace > -1);
            LttvTraceContext *tc = tsc->traces[events_request->trace];

            lttv_trace_context_remove_hooks(tc,
                                         events_request->after_chunk_trace,
                                         events_request->after_chunk_tracefile,
                                         events_request->event,
                                         events_request->event_by_id_channel);

            lttv_hooks_call(events_request->after_chunk_traceset, tsc);
          }

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
      events_request->start_position = lttv_traceset_context_position_new(tsc);
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
    lttv_process_traceset_get_sync_data(tsc);
    //lttv_traceset_context_position_save(tsc, sync_position);
    
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
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  LttvPluginTab *ptab;
  Tab *tab;

  if(!page) {
    ptab = create_new_tab(widget, NULL);
    tab = ptab->tab;
  } else {
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
  }

  //GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select a trace");
  GtkFileSelection * file_selector = (GtkFileSelection *)gtk_file_selection_new("Select a trace");
  gtk_widget_hide( (file_selector)->file_list->parent) ;
  gtk_file_selection_hide_fileop_buttons(file_selector);
  gtk_window_set_transient_for(GTK_WINDOW(file_selector),
      GTK_WINDOW(mw_data->mwindow));
  
  if(remember_trace_dir[0] != '\0')
    gtk_file_selection_set_filename(file_selector, remember_trace_dir);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_filename (file_selector);
      strncpy(remember_trace_dir, dir, PATH_MAX);
      strncat(remember_trace_dir, "/", PATH_MAX);
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

          GtkWidget *dialogue = 
            gtk_message_dialog_new(
              GTK_WINDOW(gtk_widget_get_toplevel(widget)),
              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_ERROR,
              GTK_BUTTONS_OK,
              "Cannot open trace : maybe you should enter in the trace "
              "directory to select it ?");
          gtk_dialog_run(GTK_DIALOG(dialogue));
          gtk_widget_destroy(dialogue);

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

/* remove_trace removes a trace from the current traceset if all viewers in 
 * the current tab are not interested in the trace. It first displays a 
 * dialogue, which shows all traces in the current traceset, to let user choose 
 * a trace, then it checks if all viewers unselect the trace, if it is true, 
 * it will remove the trace,  recreate the traceset_contex,
 * and redraws all the viewer of the current tab. If there is on trace in the
 * current traceset, it will delete all viewers of the current tab
 *
 * It destroys the filter tree. FIXME... we should request for an update
 * instead.
 */

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
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
  }

  nb_trace =lttv_traceset_number(tab->traceset_info->traceset); 
  name = g_new(char*,nb_trace);
  for(i = 0; i < nb_trace; i++){
    trace_v = lttv_traceset_get(tab->traceset_info->traceset, i);
    trace = lttv_trace(trace_v);
    name[i] = g_quark_to_string(ltt_trace_name(trace));
  }

  remove_trace_name = get_remove_trace(mw_data, name, nb_trace);


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
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
  tab->stop_foreground = TRUE;
  g_idle_remove_by_data(tab);
  g_assert(g_slist_length(tab->events_requests) == 0);
}


/* save will save the traceset to a file
 * Not implemented yet FIXME
 */

void save(GtkWidget * widget, gpointer user_data)
{
  g_info("Save\n");
}

void save_as(GtkWidget * widget, gpointer user_data)
{
  g_info("Save as\n");
}


/* zoom will change the time_window of all the viewers of the 
 * current tab, and redisplay them. The main functionality is to 
 * determine the new time_window of the current tab
 */

void zoom(GtkWidget * widget, double size)
{
  TimeInterval time_span;
  TimeWindow new_time_window;
  LttTime    current_time, time_delta;
  MainWindow * mw_data = get_window_data_struct(widget);
  LttvTracesetContext *tsc;
  GtkWidget * notebook = lookup_widget(widget, "MNotebook");

  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *tab;

  if(!page) {
    return;
  } else {
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
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
    new_time_window.time_width_double = ltt_time_to_double(time_delta);
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                            new_time_window.time_width) ;
  }else{
    new_time_window.time_width = ltt_time_div(new_time_window.time_width, size);
    new_time_window.time_width_double = 
                   ltt_time_to_double(new_time_window.time_width);
    if(ltt_time_compare(new_time_window.time_width,time_delta) > 0)
    { /* Case where zoom out is bigger than trace length */
      new_time_window.start_time = time_span.start_time;
      new_time_window.time_width = time_delta;
      new_time_window.time_width_double = ltt_time_to_double(time_delta);
      new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                            new_time_window.time_width) ;
    }
    else
    {
      /* Center the image on the current time */
      new_time_window.start_time = 
        ltt_time_sub(current_time,
            ltt_time_from_double(new_time_window.time_width_double/2.0));
      new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                            new_time_window.time_width) ;
      /* If on borders, don't fall off */
      if(ltt_time_compare(new_time_window.start_time, time_span.start_time) <0
       || ltt_time_compare(new_time_window.start_time, time_span.end_time) >0)
      {
        new_time_window.start_time = time_span.start_time;
        new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                            new_time_window.time_width) ;
      }
      else 
      {
        if(ltt_time_compare(new_time_window.end_time,
                            time_span.end_time) > 0
         || ltt_time_compare(new_time_window.end_time,
                            time_span.start_time) < 0)
        {
          new_time_window.start_time = 
                  ltt_time_sub(time_span.end_time, new_time_window.time_width);

          new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                                  new_time_window.time_width) ;
        }
      }
      
    }
  }

 if(ltt_time_compare(new_time_window.time_width, ltt_time_zero) == 0) {
    g_warning("Zoom more than 1 ns impossible");
 } else {
   time_change_manager(tab, new_time_window);
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
  g_info("Go to time\n");  
}

void show_time_frame(GtkWidget * widget, gpointer user_data)
{
  g_info("Show time frame\n");  
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

LttvPluginTab *create_new_tab(GtkWidget* widget, gpointer user_data)
{
  gchar label[PATH_MAX];
  MainWindow * mw_data = get_window_data_struct(widget);

  GtkNotebook * notebook = (GtkNotebook *)lookup_widget(widget, "MNotebook");
  if(notebook == NULL){
    g_info("Notebook does not exist\n");
    return NULL;
  }
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  Tab *copy_tab;

  if(!page) {
    copy_tab = NULL;
  } else {
    LttvPluginTab *ptab;
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    copy_tab = ptab->tab;
  }
  
  strcpy(label,"Page");
  if(get_label(mw_data, label,"Get the name of the tab","Please input tab's name")) {
    LttvPluginTab *ptab;
    
    ptab = g_object_new(LTTV_TYPE_PLUGIN_TAB, NULL);
    init_tab (ptab->tab, mw_data, copy_tab, notebook, label);
    ptab->parent.top_widget = ptab->tab->top_widget;
    g_object_set_data_full(
           G_OBJECT(ptab->tab->vbox),
           "Tab_Plugin",
           ptab,
	   (GDestroyNotify)tab_destructor);
    return ptab;
  }
  else return NULL;
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
    g_info("Notebook does not exist\n");
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
    g_info("Notebook does not exist\n");
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
  mainwindow_quit();
}


void
on_cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Cut\n");
}


void
on_copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Copye\n");
}


void
on_paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Paste\n");
}


void
on_delete_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Delete\n");
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
on_trace_facility_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	g_info("Trace facility selector: %s\n", "");
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

    load_module_path = get_selection(mw_data,
                             (char **)(name->pdata), name->len,
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
    
    gtk_window_set_transient_for(GTK_WINDOW(file_selector),
        GTK_WINDOW(mw_data->mwindow));

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
        if(error != NULL) g_warning("%s", error->message);
        else g_info("Load library: %s\n", str);
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

  LttvLibrary *library = NULL;

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
  lib_name = get_selection(mw_data, (char **)(name->pdata), name->len,
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

  if(library != NULL) lttv_library_unload(library);
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

  LttvLibrary *library = NULL;
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
    lib_name = get_selection(mw_data,(char **)(name->pdata), name->len,
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
    nb = lttv_library_module_number(library);
    LttvModuleInfo *module_info = g_new(LttvModuleInfo,nb);
    name = g_ptr_array_new();
    /* ask for the module name */

    for(i=0;i<nb;i++){
      LttvModule *iter_module = lttv_library_module_get(library, i);
      lttv_module_info(iter_module, &module_info[i]);

      gchar *path = module_info[i].name;
      g_ptr_array_add(name, path);
    }
    module_name = get_selection(mw_data, (char **)(name->pdata), name->len,
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
  if(error != NULL) g_warning("%s", error->message);
  else g_info("Load module: %s", module_name_out);


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
        else g_info("Load library: %s\n", str);
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

  LttvLibrary *library = NULL;
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
    lib_name = get_selection(mw_data, (char **)(name->pdata), name->len,
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

  LttvModule *module = NULL;
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
    module_name = get_selection(mw_data, (char **)(name->pdata), name->len,
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
  g_info("Release module: %s\n", module_info.name);
 
  lttv_module_release(module);
}


/* Display a directory dialogue to let user select a path for library searching
 */

void
on_add_library_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  //GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select library path");
  GtkFileSelection * file_selector = (GtkFileSelection *)gtk_file_selection_new("Select a trace");
  gtk_widget_hide( (file_selector)->file_list->parent) ;

  gtk_window_set_transient_for(GTK_WINDOW(file_selector),
      GTK_WINDOW(mw_data->mwindow));

  const char * dir;
  gint id;

  if(remember_plugins_dir[0] != '\0')
    gtk_file_selection_set_filename(file_selector, remember_plugins_dir);

  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_filename (file_selector);
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
    lib_path = get_selection(mw_data, (char **)(name->pdata), name->len,
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
  g_info("Color\n");
}


void
on_save_configuration_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Save configuration\n");
}


void
on_content_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_info("Content\n");
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
  gtk_window_set_transient_for(about_window, GTK_WINDOW(window_widget));
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
<big>Linux Trace Toolkit " VERSION "</big>");
  gtk_label_set_justify(GTK_LABEL(label1), GTK_JUSTIFY_CENTER);
  
  GtkWidget *label2 = gtk_label_new("");
  gtk_misc_set_padding(GTK_MISC(label2), 10, 20);
  gtk_label_set_markup(GTK_LABEL(label2), "\
Contributors :\n\
\n\
Michel Dagenais (New trace format, lttv main)\n\
Mathieu Desnoyers (Kernel Tracer, Directory structure, build with automake/conf,\n\
                   lttv gui, control flow view, gui cooperative trace reading\n\
                   scheduler with interruptible foreground and background\n\
                   computation, detailed event list (rewrite), trace reading\n\
                   library (rewrite))\n\
Benoit Des Ligneris, Eric Clement (Cluster adaptation, work in progress)\n\
Xang-Xiu Yang (new trace reading library and converter, lttv gui, \n\
               detailed event list and statistics view)\n\
Tom Zanussi (RelayFS)\n\
\n\
Inspired from the original Linux Trace Toolkit Visualizer made by\n\
Karim Yaghmour");

  GtkWidget *label3 = gtk_label_new("");
  gtk_label_set_markup(GTK_LABEL(label3), "\
Linux Trace Toolkit Viewer, Copyright (C) 2004, 2005, 2006\n\
                                                Michel Dagenais\n\
                                                Mathieu Desnoyers\n\
                                                Xang-Xiu Yang\n\
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

  g_info("There are now : %d windows\n",g_slist_length(g_main_window_list));
  if(g_slist_length(g_main_window_list) == 0)
    mainwindow_quit();
}

gboolean    
on_MWindow_configure                   (GtkWidget         *widget,
                                        GdkEventConfigure *event,
                                        gpointer           user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)widget);
	
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
  LttTime end_time = new_time_window.end_time;
  LttTime time_width = new_time_window.time_width;

  g_assert(ltt_time_compare(start_time, end_time) < 0);
  
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
               ltt_time_to_double(upper), /* upper */
               "step_increment",
               new_time_window.time_width_double
                             / SCROLL_STEP_PER_PAGE, /* step increment */
               "page_increment",
               new_time_window.time_width_double, 
                                                     /* page increment */
               "page_size",
               new_time_window.time_width_double, /* page size */
               NULL);
  gtk_adjustment_changed(adjustment);

 // g_object_set(G_OBJECT(adjustment),
 //              "value",
 //              ltt_time_to_double(
 //               ltt_time_sub(start_time, time_span.start_time))
 //                 , /* value */
 //              NULL);
  //gtk_adjustment_value_changed(adjustment);
  gtk_range_set_value(GTK_RANGE(tab->scrollbar),
               ltt_time_to_double(
                ltt_time_sub(start_time, time_span.start_time)) /* value */);

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

  /* width seconds */
  if(time_width.tv_nsec == 0) {
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry7),
                              (double)1,
                              (double)upper.tv_sec);
  } else {
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry7),
                              (double)0,
                              (double)upper.tv_sec);
  }
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry7),
                            (double)time_width.tv_sec);

  /* width nanoseconds */
  if(time_width.tv_sec == upper.tv_sec) {
    if(time_width.tv_sec == 0) {
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry8),
                                (double)1,
                                (double)upper.tv_nsec);
    } else {
      gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry8),
                                (double)0,
                                (double)upper.tv_nsec);
    }
  }
  else if(time_width.tv_sec == 0) {
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry8),
                              1.0,
                              (double)upper.tv_nsec);
  }
  else /* anywhere else */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(tab->MEntry8),
                              0.0,
                              (double)NANOSECONDS_PER_SECOND-1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab->MEntry8),
                            (double)time_width.tv_nsec);

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
 
  LttTime end_time = new_time_window.end_time;

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

  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push back end time : keep the same time width
     * if possible, else end traceset time */
    end_time = LTT_TIME_MIN(ltt_time_add(new_time_window.start_time,
                                         new_time_window.time_width),
                            time_span.end_time);
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);
  new_time_window.time_width_double =
              ltt_time_to_double(new_time_window.time_width);

  new_time_window.end_time = end_time;

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
 
  LttTime end_time = new_time_window.end_time;

  new_time_window.start_time.tv_nsec = value;

  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push back end time : keep the same time width
     * if possible, else end traceset time */
    end_time = LTT_TIME_MIN(ltt_time_add(new_time_window.start_time,
                                         new_time_window.time_width),
                            time_span.end_time);
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);
  new_time_window.time_width_double =
              ltt_time_to_double(new_time_window.time_width);

  new_time_window.end_time = end_time;

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
 
  LttTime end_time = new_time_window.end_time;

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

  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push front start time : keep the same time width
     * if possible, else end traceset time */
    new_time_window.start_time = LTT_TIME_MAX(
                                  ltt_time_sub(end_time,
                                               new_time_window.time_width),
                                  time_span.start_time);
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);
  new_time_window.time_width_double =
              ltt_time_to_double(new_time_window.time_width);

  new_time_window.end_time = end_time;
  
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
 
  LttTime end_time = new_time_window.end_time;

  end_time.tv_nsec = value;

  if(ltt_time_compare(new_time_window.start_time, end_time) >= 0) {
    /* Then, we must push front start time : keep the same time width
     * if possible, else end traceset time */
    new_time_window.start_time = LTT_TIME_MAX(
                                ltt_time_sub(end_time,
                                             new_time_window.time_width),
                                time_span.start_time);
  }

  /* Fix the time width to fit start time and end time */
  new_time_window.time_width = ltt_time_sub(end_time,
                                            new_time_window.start_time);
  new_time_window.time_width_double =
              ltt_time_to_double(new_time_window.time_width);
  new_time_window.end_time = end_time;

  time_change_manager(tab, new_time_window);

}

/* value changed for time frame interval s
 *
 * Check time span : if ns is out of range, clip it the nearest good value.
 */
void
on_MEntry7_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);
  LttTime    current_time, time_delta;
  TimeWindow new_time_window =  tab->time_window;
  current_time = tab->current_time;
  
  time_delta = ltt_time_sub(time_span.end_time,time_span.start_time);
  new_time_window.time_width.tv_sec = value;
  new_time_window.time_width_double = 
                 ltt_time_to_double(new_time_window.time_width);
  if(ltt_time_compare(new_time_window.time_width,time_delta) > 0)
  { /* Case where zoom out is bigger than trace length */
    new_time_window.start_time = time_span.start_time;
    new_time_window.time_width = time_delta;
    new_time_window.time_width_double = ltt_time_to_double(time_delta);
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
  }
  else
  {
    /* Center the image on the current time */
    new_time_window.start_time = 
      ltt_time_sub(current_time,
          ltt_time_from_double(new_time_window.time_width_double/2.0));
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
    /* If on borders, don't fall off */
    if(ltt_time_compare(new_time_window.start_time, time_span.start_time) <0
     || ltt_time_compare(new_time_window.start_time, time_span.end_time) >0)
    {
      new_time_window.start_time = time_span.start_time;
      new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
    }
    else 
    {
      if(ltt_time_compare(new_time_window.end_time,
                          time_span.end_time) > 0
       || ltt_time_compare(new_time_window.end_time,
                          time_span.start_time) < 0)
      {
        new_time_window.start_time = 
                ltt_time_sub(time_span.end_time, new_time_window.time_width);

        new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                                new_time_window.time_width) ;
      }
    }
    
  }

  if(ltt_time_compare(new_time_window.time_width, ltt_time_zero) == 0) {
    g_warning("Zoom more than 1 ns impossible");
  } else {
   time_change_manager(tab, new_time_window);
  }
}

void
on_MEntry8_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data)
{
  Tab *tab =(Tab *)user_data;
  LttvTracesetContext * tsc = 
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;
  gint value = gtk_spin_button_get_value_as_int(spinbutton);
  LttTime    current_time, time_delta;
  TimeWindow new_time_window =  tab->time_window;
  current_time = tab->current_time;
  
  time_delta = ltt_time_sub(time_span.end_time,time_span.start_time);
  new_time_window.time_width.tv_nsec = value;
  new_time_window.time_width_double = 
                 ltt_time_to_double(new_time_window.time_width);
  if(ltt_time_compare(new_time_window.time_width,time_delta) > 0)
  { /* Case where zoom out is bigger than trace length */
    new_time_window.start_time = time_span.start_time;
    new_time_window.time_width = time_delta;
    new_time_window.time_width_double = ltt_time_to_double(time_delta);
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
  }
  else
  {
    /* Center the image on the current time */
    new_time_window.start_time = 
      ltt_time_sub(current_time,
          ltt_time_from_double(new_time_window.time_width_double/2.0));
    new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
    /* If on borders, don't fall off */
    if(ltt_time_compare(new_time_window.start_time, time_span.start_time) <0
     || ltt_time_compare(new_time_window.start_time, time_span.end_time) >0)
    {
      new_time_window.start_time = time_span.start_time;
      new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                          new_time_window.time_width) ;
    }
    else 
    {
      if(ltt_time_compare(new_time_window.end_time,
                          time_span.end_time) > 0
       || ltt_time_compare(new_time_window.end_time,
                          time_span.start_time) < 0)
      {
        new_time_window.start_time = 
                ltt_time_sub(time_span.end_time, new_time_window.time_width);

        new_time_window.end_time = ltt_time_add(new_time_window.start_time,
                                                new_time_window.time_width) ;
      }
    }
    
  }

  if(ltt_time_compare(new_time_window.time_width, ltt_time_zero) == 0) {
    g_warning("Zoom more than 1 ns impossible");
  } else {
   time_change_manager(tab, new_time_window);
  }
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

void current_position_change_manager(Tab *tab,
                                     LttvTracesetContextPosition *pos)
{
  LttvTracesetContext *tsc =
    LTTV_TRACESET_CONTEXT(tab->traceset_info->traceset_context);
  TimeInterval time_span = tsc->time_span;

  g_assert(lttv_process_traceset_seek_position(tsc, pos) == 0);
  LttTime new_time = lttv_traceset_context_position_get_time(pos);
  /* Put the context in a state coherent position */
  lttv_state_traceset_seek_time_closest((LttvTracesetState*)tsc, ltt_time_zero);
  
  current_time_change_manager(tab, new_time);
  
  set_current_position(tab, pos);
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

  time = ltt_time_add(ltt_time_from_double(value),
                      time_span.start_time);

  new_time_window.start_time = time;
  
  page_size = adjust->page_size;

  new_time_window.time_width = 
    ltt_time_from_double(page_size);

  new_time_window.time_width_double =
              page_size;

  new_time_window.end_time = ltt_time_add(new_time_window.start_time, 
                                          new_time_window.time_width);


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
  time = ltt_time_add(ltt_time_from_double(value),
                      time_span.start_time);

  time_window.start_time = time;
  
  page_size = adjust->page_size;

  time_window.time_width = 
    ltt_time_from_double(page_size);
  //time = ltt_time_sub(time_span.end_time, time);
  //if(ltt_time_compare(time,time_window.time_width) < 0){
  //  time_window.time_width = time;
  //}

  /* call viewer hooks for new time window */
  set_time_window(tab, &time_window);
#endif //0
}


/* Display a dialogue showing all eventtypes and traces, let user to select the interested
 * eventtypes, tracefiles and traces (filter)
 */

/* Select a trace which will be removed from traceset
 */

char * get_remove_trace(MainWindow *mw_data, 
    char ** all_trace_name, int nb_trace)
{
  return get_selection(mw_data, all_trace_name, nb_trace, 
		       "Select a trace", "Trace pathname");
}


/* Select a module which will be loaded
 */

char * get_load_module(MainWindow *mw_data, 
    char ** load_module_name, int nb_module)
{
  return get_selection(mw_data, load_module_name, nb_module, 
		       "Select a module to load", "Module name");
}




/* Select a module which will be unloaded
 */

char * get_unload_module(MainWindow *mw_data,
    char ** loaded_module_name, int nb_module)
{
  return get_selection(mw_data, loaded_module_name, nb_module, 
		       "Select a module to unload", "Module name");
}


/* Display a dialogue which shows all selectable items, let user to 
 * select one of them
 */

char * get_selection(MainWindow *mw_data,
    char ** loaded_module_name, int nb_module,
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
  gtk_window_set_transient_for(GTK_WINDOW(dialogue), 
      GTK_WINDOW(mw_data->mwindow));

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
  GtkTreeModel **store_model = (GtkTreeModel**)&store;
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      if (gtk_tree_selection_get_selected (select, store_model, &iter)){
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
  guint i;
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

MainWindow *construct_main_window(MainWindow * parent)
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
    g_info("Notebook does not exist\n");
    /* FIXME : destroy partially created widgets */
    g_free(new_m_window);
    return NULL;
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
      LttvPluginTab *ptab;
      ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
      parent_tab = ptab->tab;
    }
    LttvPluginTab *ptab = g_object_new(LTTV_TYPE_PLUGIN_TAB, NULL);
    init_tab(ptab->tab,
		    new_m_window, parent_tab, notebook, "Traceset");
    ptab->parent.top_widget = ptab->tab->top_widget;
    g_object_set_data_full(
           G_OBJECT(ptab->tab->vbox),
           "Tab_Plugin",
           ptab,
	   (GDestroyNotify)tab_destructor);
    new_tab = ptab->tab;
  } else {
    LttvPluginTab *ptab = g_object_new(LTTV_TYPE_PLUGIN_TAB, NULL);
    init_tab(ptab->tab, new_m_window, NULL, notebook, "Traceset");
    ptab->parent.top_widget = ptab->tab->top_widget;
    g_object_set_data_full(
           G_OBJECT(ptab->tab->vbox),
           "Tab_Plugin",
           ptab,
	   (GDestroyNotify)tab_destructor);
    new_tab = ptab->tab;
  }

  /* Insert default viewers */
  {
    LttvAttributeType type;
    LttvAttributeName name;
    LttvAttributeValue value;
    LttvAttribute *attribute;
    
    LttvIAttribute *attributes_global = 
       LTTV_IATTRIBUTE(lttv_global_attributes());

    attribute = LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
				       LTTV_IATTRIBUTE(attributes_global),
				       LTTV_VIEWER_CONSTRUCTORS));
    g_assert(attribute);

    name = g_quark_from_string("guievents");
    type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                       name, &value);
    if(type == LTTV_POINTER) {
      lttvwindow_viewer_constructor viewer_constructor = 
                (lttvwindow_viewer_constructor)*value.v_pointer;
      insert_viewer(new_window, viewer_constructor);
    }

    name = g_quark_from_string("guicontrolflow");
    type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                       name, &value);
    if(type == LTTV_POINTER) {
      lttvwindow_viewer_constructor viewer_constructor = 
                (lttvwindow_viewer_constructor)*value.v_pointer;
      insert_viewer(new_window, viewer_constructor);
    }

    name = g_quark_from_string("guistatistics");
    type = lttv_iattribute_get_by_name(LTTV_IATTRIBUTE(attribute),
                                       name, &value);
    if(type == LTTV_POINTER) {
      lttvwindow_viewer_constructor viewer_constructor = 
                (lttvwindow_viewer_constructor)*value.v_pointer;
      insert_viewer(new_window, viewer_constructor);
    }
  }

  g_info("There are now : %d windows\n",g_slist_length(g_main_window_list));

  return new_m_window;
}


/* Free the memory occupied by a tab structure
 * destroy the tab
 */

void tab_destructor(LttvPluginTab * ptab)
{
  int i, nb, ref_count;
  LttvTrace * trace;
  Tab *tab = ptab->tab;

  gtk_object_destroy(GTK_OBJECT(tab->tooltips));
  
  if(tab->attributes)
    g_object_unref(tab->attributes);

  if(tab->interrupted_state)
    g_object_unref(tab->interrupted_state);


  if(tab->traceset_info->traceset_context != NULL){
    //remove state update hooks
    lttv_state_remove_event_hooks(
         (LttvTracesetState*)tab->traceset_info->
                              traceset_context);
    lttv_context_fini(LTTV_TRACESET_CONTEXT(tab->traceset_info->
					    traceset_context));
    g_object_unref(tab->traceset_info->traceset_context);
  }
  if(tab->traceset_info->traceset != NULL) {
    nb = lttv_traceset_number(tab->traceset_info->traceset);
    for(i = 0 ; i < nb ; i++) {
      trace = lttv_traceset_get(tab->traceset_info->traceset, i);
      ref_count = lttv_trace_get_ref_number(trace);
      if(ref_count <= 1){
	      ltt_trace_close(lttv_trace(trace));
      }
    }
  }
  lttv_traceset_destroy(tab->traceset_info->traceset);
  /* Remove the idle events requests processing function of the tab */
  g_idle_remove_by_data(tab);

  g_slist_free(tab->events_requests);
  g_free(tab->traceset_info);
  //g_free(tab);
  g_object_unref(ptab);
}


/* Create a tab and insert it into the current main window
 */

void init_tab(Tab *tab, MainWindow * mw, Tab *copy_tab, 
		  GtkNotebook * notebook, char * label)
{
  GList * list;
  //Tab * tab;
  //LttvFilter *filter = NULL;
  
  //create a new tab data structure
  //tab = g_new(Tab,1);

  //construct and initialize the traceset_info
  tab->traceset_info = g_new(TracesetInfo,1);

  if(copy_tab) {
    tab->traceset_info->traceset = 
      lttv_traceset_copy(copy_tab->traceset_info->traceset);
    
    /* Copy the previous tab's filter */
    /* We can clone the filter, as we copy the trace set also */
    /* The filter must always be in sync with the trace set */
    tab->filter = lttv_filter_clone(copy_tab->filter);
  } else {
    tab->traceset_info->traceset = lttv_traceset_new();
    tab->filter = NULL;
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
  tab->top_widget = tab->vbox;
  //g_object_set_data_full(G_OBJECT(tab->top_widget), "filter",
//		  filter, (GDestroyNotify)lttv_filter_destroy);

//  g_signal_connect (G_OBJECT(tab->top_widget),
//                      "notify",
//                      G_CALLBACK (on_top_notify),
//                      (gpointer)tab);

  tab->viewer_container = gtk_vbox_new(TRUE, 2);
  tab->scrollbar = gtk_hscrollbar_new(NULL);
  //tab->multivpaned = gtk_multi_vpaned_new();
 
  gtk_box_pack_start(GTK_BOX(tab->vbox),
                     tab->viewer_container,
                     TRUE, /* expand */
                     TRUE, /* Give the extra space to the child */
                     0);    /* No padding */
  
//  if(copy_tab) {
//    tab->time_window = copy_tab->time_window;
//    tab->current_time = copy_tab->current_time;
//  }

  /* Create the timebar */
  {
    tab->MTimebar = gtk_hbox_new(FALSE, 2);
    gtk_widget_show(tab->MTimebar);
    tab->tooltips = gtk_tooltips_new();

    tab->MEventBox1a = gtk_event_box_new();
    gtk_widget_show(tab->MEventBox1a);
    gtk_tooltips_set_tip(tab->tooltips, tab->MEventBox1a, 
        "Paste Start and End Times Here", "");
    tab->MText1a = gtk_label_new("Time Frame ");
    gtk_widget_show(tab->MText1a);
    gtk_container_add(GTK_CONTAINER(tab->MEventBox1a), tab->MText1a);
    tab->MEventBox1b = gtk_event_box_new();
    gtk_widget_show(tab->MEventBox1b);
    gtk_tooltips_set_tip(tab->tooltips, tab->MEventBox1b, 
        "Paste Start Time Here", "");
    tab->MText1b = gtk_label_new("start: ");
    gtk_widget_show(tab->MText1b);
    gtk_container_add(GTK_CONTAINER(tab->MEventBox1b), tab->MText1b);
    tab->MText2 = gtk_label_new("s");
    gtk_widget_show(tab->MText2);
    tab->MText3a = gtk_label_new("ns");
    gtk_widget_show(tab->MText3a);

    tab->MEventBox3b = gtk_event_box_new();
    gtk_widget_show(tab->MEventBox3b);
    gtk_tooltips_set_tip(tab->tooltips, tab->MEventBox3b, 
        "Paste End Time Here", "");
    tab->MText3b = gtk_label_new("end:");
    gtk_widget_show(tab->MText3b);
    gtk_container_add(GTK_CONTAINER(tab->MEventBox3b), tab->MText3b);
    tab->MText4 = gtk_label_new("s");
    gtk_widget_show(tab->MText4);
    tab->MText5a = gtk_label_new("ns");
    gtk_widget_show(tab->MText5a);

    tab->MEventBox8 = gtk_event_box_new();
    gtk_widget_show(tab->MEventBox8);
    gtk_tooltips_set_tip(tab->tooltips, tab->MEventBox8, 
        "Paste Time Interval here", "");
    tab->MText8 = gtk_label_new("Time Interval:");
    gtk_widget_show(tab->MText8);
    gtk_container_add(GTK_CONTAINER(tab->MEventBox8), tab->MText8);
    tab->MText9 = gtk_label_new("s");
    gtk_widget_show(tab->MText9);
    tab->MText10 = gtk_label_new("ns");
    gtk_widget_show(tab->MText10);

    tab->MEventBox5b = gtk_event_box_new();
    gtk_widget_show(tab->MEventBox5b);
    gtk_tooltips_set_tip(tab->tooltips, tab->MEventBox5b, 
        "Paste Current Time Here", "");
    tab->MText5b = gtk_label_new("Current Time:");
    gtk_widget_show(tab->MText5b);
    gtk_container_add(GTK_CONTAINER(tab->MEventBox5b), tab->MText5b);
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
    tab->MEntry7 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry7),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry7),TRUE);
    gtk_widget_show(tab->MEntry7);
    tab->MEntry8 = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tab->MEntry8),0);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(tab->MEntry8),TRUE);
    gtk_widget_show(tab->MEntry8);
    
    GtkWidget *temp_widget;
    
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEventBox1a, FALSE,
                         FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEventBox1b, FALSE,
                         FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText3a, FALSE, FALSE, 0);
    temp_widget = gtk_vseparator_new();
    gtk_widget_show(temp_widget);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), temp_widget, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEventBox3b, FALSE,
                         FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry3, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText4, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry4, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText5a, FALSE, FALSE, 0);
    temp_widget = gtk_vseparator_new();
    gtk_widget_show(temp_widget);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), temp_widget, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEventBox8, FALSE,
                         FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry7, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText9, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MEntry8, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (tab->MTimebar), tab->MText10, FALSE, FALSE, 0);

    temp_widget = gtk_vseparator_new();
    gtk_widget_show(temp_widget);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MText7, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MEntry6, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MText6, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MEntry5, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), tab->MEventBox5b, FALSE,
                         FALSE, 0);
    gtk_box_pack_end (GTK_BOX (tab->MTimebar), temp_widget, FALSE, FALSE, 0);
    

    //GtkWidget *test = gtk_button_new_with_label("drop");
    //gtk_button_set_relief(GTK_BUTTON(test), GTK_RELIEF_NONE);
    //gtk_widget_show(test);
    //gtk_box_pack_end(GTK_BOX (tab->MTimebar), test, FALSE, FALSE, 0);
    //gtk_widget_add_events(tab->MText1, GDK_ALL_EVENTS_MASK);//GDK_BUTTON_PRESS_MASK);
    /*GtkWidget *event_box = gtk_event_box_new();
    gtk_widget_show(event_box);
    gtk_tooltips_set_tip(tooltips, event_box, 
        "Paste Current Time Here", "");
    gtk_box_pack_end(GTK_BOX (tab->MTimebar), event_box, FALSE, FALSE, 0);
    GtkWidget *test = gtk_label_new("drop");
    gtk_container_add(GTK_CONTAINER(event_box), test);
    gtk_widget_show(test);
    g_signal_connect (G_OBJECT(event_box),
                      "button-press-event",
                      G_CALLBACK (on_MText1_paste),
                      (gpointer)tab);
*/

    g_signal_connect (G_OBJECT(tab->MEventBox1a),
                      "button-press-event",
                      G_CALLBACK (on_MEventBox1a_paste),
                      (gpointer)tab);

    g_signal_connect (G_OBJECT(tab->MEventBox1b),
                      "button-press-event",
                      G_CALLBACK (on_MEventBox1b_paste),
                      (gpointer)tab);
    g_signal_connect (G_OBJECT(tab->MEventBox3b),
                      "button-press-event",
                      G_CALLBACK (on_MEventBox3b_paste),
                      (gpointer)tab);
    g_signal_connect (G_OBJECT(tab->MEventBox5b),
                      "button-press-event",
                      G_CALLBACK (on_MEventBox5b_paste),
                      (gpointer)tab);
    g_signal_connect (G_OBJECT(tab->MEventBox8),
                      "button-press-event",
                      G_CALLBACK (on_MEventBox8_paste),
                      (gpointer)tab);
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
  tab->stop_foreground = FALSE;
  


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
  g_signal_connect ((gpointer) tab->MEntry7, "value-changed",
                    G_CALLBACK (on_MEntry7_value_changed),
                    tab);
  g_signal_connect ((gpointer) tab->MEntry8, "value-changed",
                    G_CALLBACK (on_MEntry8_value_changed),
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
 
  if(copy_tab) {
    lttvwindow_report_time_window(tab, copy_tab->time_window);
    lttvwindow_report_current_time(tab, copy_tab->current_time);
  } else {
    TimeWindow time_window;

    time_window.start_time = ltt_time_zero;
    time_window.end_time = ltt_time_add(time_window.start_time,
        lttvwindow_default_time_width);
    time_window.time_width = lttvwindow_default_time_width;
    time_window.time_width_double = ltt_time_to_double(time_window.time_width);

    lttvwindow_report_time_window(tab, time_window);
    lttvwindow_report_current_time(tab, ltt_time_zero);
  }
 
  LttvTraceset *traceset = tab->traceset_info->traceset;
  SetTraceset(tab, traceset);
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


__EXPORT void create_main_window_with_trace_list(GSList *traces)
{
  GSList *iter = NULL;

  /* Create window */
  MainWindow *mw = construct_main_window(NULL);
  GtkWidget *widget = mw->mwindow;

  GtkWidget * notebook = lookup_widget(widget, "MNotebook");
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                      gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  LttvPluginTab *ptab;
  Tab *tab;
  
  if(!page) {
    ptab = create_new_tab(widget, NULL);
    tab = ptab->tab;
  } else {
    ptab = (LttvPluginTab *)g_object_get_data(G_OBJECT(page), "Tab_Plugin");
    tab = ptab->tab;
  }

  for(iter=traces; iter!=NULL; iter=g_slist_next(iter)) {
    gchar *path = (gchar*)iter->data;
    /* Add trace */
    gchar abs_path[PATH_MAX];
    LttvTrace *trace_v;
    LttTrace *trace;

    get_absolute_pathname(path, abs_path);
    trace_v = lttvwindowtraces_get_trace_by_name(abs_path);
    if(trace_v == NULL) {
      trace = ltt_trace_open(abs_path);
      if(trace == NULL) {
        g_warning("cannot open trace %s", abs_path);

        GtkWidget *dialogue = 
          gtk_message_dialog_new(
            GTK_WINDOW(gtk_widget_get_toplevel(widget)),
            GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Cannot open trace : maybe you should enter in the directory "
            "to select it ?");
        gtk_dialog_run(GTK_DIALOG(dialogue));
        gtk_widget_destroy(dialogue);
      } else {
        trace_v = lttv_trace_new(trace);
        lttvwindowtraces_add_trace(trace_v);
        lttvwindow_add_trace(tab, trace_v);
      }
    } else {
      lttvwindow_add_trace(tab, trace_v);
    }
  }
  
  LttvTraceset *traceset;

  traceset = tab->traceset_info->traceset;
  SetTraceset(tab, traceset);
}

