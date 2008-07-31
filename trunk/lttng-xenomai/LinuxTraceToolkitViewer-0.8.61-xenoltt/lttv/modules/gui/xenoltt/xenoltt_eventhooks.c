/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers
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
 *JO
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
#include <ltt/type.h>
#include <ltt/trace.h>

#include <lttv/lttv.h>
#include <lttv/hook.h>
#include <lttv/state.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/support.h>
#include <lttv/stats.h>
#include <lttv/xenoltt_sim.h>

#include <lttv/filter.h>

#include "xenoltt_eventhooks.h"
#include "xfv.h"
#include "xenoltt_threadlist.h"
#include "xenoltt_drawing.h"

#include "TraceControlStart.xpm"

#define MAX_PATH_LEN 256
#define STATE_LINE_WIDTH 2
#define COLLISION_POSITION(height) (((height - STATE_LINE_WIDTH)/2) -3)

extern GSList *g_legend_list;




/* Action to do when background computation completed.
 *
 * Wait for all the awaited computations to be over.
 */

static gint background_ready(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData *)hook_data;
  LttvTrace *trace = (LttvTrace*)call_data;
  
  xenoltt_data->background_info_waiting--;
  
  if(xenoltt_data->background_info_waiting == 0) {
    g_message("xenoltt viewer : background computation data ready.");
        
    drawing_clear(xenoltt_data->drawing);
    threadlist_clear(xenoltt_data->thread_list);
    gtk_widget_set_size_request(xenoltt_data->drawing->drawing_area,-1, threadlist_get_height(xenoltt_data->thread_list));
    
    redraw_notify(xenoltt_data, NULL);
  
  }
 
  return 0;
}


/* Request background computation. Verify if it is in progress or ready first.
 * Only for each trace in the tab's traceset.
 */
static void request_background_data(XenoLTTData *xenoltt_data) {
  LttvTracesetContext * tsc = lttvwindow_get_traceset_context(xenoltt_data->tab);
  gint num_traces = lttv_traceset_number(tsc->ts);
  gint i;
  LttvTrace *trace;
  LttvTraceState *tstate;
  
  LttvHooks *background_ready_hook = lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, xenoltt_data,LTTV_PRIO_DEFAULT);
  xenoltt_data->background_info_waiting = 0;
  
  for(i=0;i<num_traces;i++) {
    trace = lttv_traceset_get(tsc->ts, i);
    tstate = LTTV_TRACE_STATE(tsc->traces[i]);
    
    if(lttvwindowtraces_get_ready(g_quark_from_string("state"), trace)==FALSE
    && !tstate->has_precomputed_states) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("state"),
      trace) == FALSE) {
        /* We first remove requests that could have been done for the same
         * information. Happens when two viewers ask for it before servicing
         * starts.
         */
        if(!lttvwindowtraces_background_request_find(trace, "state"))
          lttvwindowtraces_background_request_queue(
          main_window_get_widget(xenoltt_data->tab), trace, "state");
        lttvwindowtraces_background_notify_queue(xenoltt_data,
          trace,
          ltt_time_infinite,
          NULL,
          background_ready_hook);
        xenoltt_data->background_info_waiting++;
      } else { /* in progress */
        lttvwindowtraces_background_notify_current(xenoltt_data,
          trace,
          ltt_time_infinite,
          NULL,
          background_ready_hook);
        xenoltt_data->background_info_waiting++;
      }
    } else {
      /* Data ready. By its nature, this viewer doesn't need to have
       * its data ready hook called there, because a background
       * request is always linked with a redraw.
       */
    }
    
  }
  lttv_hooks_destroy(background_ready_hook);
}


/***********************************************************************************************************************************/
/***********************************************************************************************************************************/


/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param tab A pointer to the parent tab.
 * @return The widget created.
 */
GtkWidget *h_guixenoltt(LttvPlugin *plugin) {
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_guixenoltt, %p", tab);
  XenoLTTData *xenoltt_data = guixenoltt(ptab);
  
  xenoltt_data->tab = tab;
      
  // Unreg done in the guixenoltt_Destructor
  lttvwindow_register_traceset_notify(tab,traceset_notify,xenoltt_data);
    
  lttvwindow_register_time_window_notify(tab,update_time_window_hook,xenoltt_data);
  lttvwindow_register_current_time_notify(tab,update_current_time_hook,xenoltt_data);
  lttvwindow_register_redraw_notify(tab,redraw_notify,xenoltt_data);
  lttvwindow_register_continue_notify(tab,continue_notify,xenoltt_data);
  request_background_data(xenoltt_data);

  return guixenoltt_get_widget(xenoltt_data);
  
}

void xenolttlegend_destructor(GtkWindow *legend) {
  g_legend_list = g_slist_remove(g_legend_list, legend);
}
/***********************************************************************************************************************************/
void start_clicked (GtkButton *button, gpointer user_data);

typedef struct _ControlData ControlData;
struct _ControlData {
  Tab *tab;                             //< current tab of module

  GtkWidget *window;                  //< window
  
  GtkWidget *main_box;                //< main container
  GtkWidget *start_button;
  GtkWidget *task_selection_combo;
  GtkWidget *period_entry;
  GtkWidget *sim_file_label;
  GtkWidget *file_entry;
  GtkWidget *title;
  GtkWidget *period_settings;
  GtkWidget *task_label;
  GtkWidget *period_label;
};

void prepare_sim_data(ControlData *tcd);

/* Create a simulation settings window */

GtkWidget *h_xenolttsimulation(LttvPlugin *plugin) {
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_Simulation, %p", tab);
   
  ControlData* tcd = g_new(ControlData,1);

  tcd->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(tcd->window), "XenoLTT Simulation");
  
  g_legend_list = g_slist_append(g_legend_list,GTK_WINDOW(tcd->window));
  
  g_object_set_data_full(G_OBJECT(tcd->window),"Simulation",GTK_WINDOW(tcd->window),
                          (GDestroyNotify)xenolttlegend_destructor);
  

  tcd->tab  = tab;
  tcd->main_box = gtk_table_new(3,7,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(tcd->main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(tcd->main_box),5);
  
  gtk_container_add(GTK_CONTAINER(tcd->window), GTK_WIDGET(tcd->main_box));
  
  prepare_sim_data(tcd);
  
  return NULL;

}

/***********************************************************************************************************************************/

static gint data_ready(void *hook_data, void *call_data){
  ControlData *tcd = (ControlData*)hook_data;
  ThreadEventData *temp_thread;
  GArray *list = get_thread_list();
  
  tcd->sim_file_label = gtk_label_new("Simulation directory:");
  gtk_widget_show (tcd->sim_file_label);
  tcd->file_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->file_entry),"/tmp/trace1.xml");
  gtk_widget_show (tcd->file_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->sim_file_label,0,2,0,1,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->file_entry,2,6,0,1,GTK_FILL,GTK_FILL,0,0);
  
 
  //Insert Task selection
  tcd->task_label = gtk_label_new("Task:");
  gtk_widget_show (tcd->task_label);

  tcd->task_selection_combo = gtk_combo_box_new_text();
  // iterate on all task to get the name and default period
  int i;

  gtk_combo_box_append_text(GTK_COMBO_BOX(tcd->task_selection_combo), " - Choose a task - ");
  for(i=0;i<list->len;i++){
    temp_thread = g_array_index(list, ThreadEventData*, i);
    gchar text[MAX_PATH_LEN] = "";
    sprintf(text,"%s (%u)",g_quark_to_string(temp_thread->name),temp_thread->period);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tcd->task_selection_combo), text);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(tcd->task_selection_combo), 0);
  gtk_widget_show (tcd->task_selection_combo);

  // Period Entry
  tcd->period_label = gtk_label_new("New period:");
  gtk_widget_show (tcd->period_label);
  tcd->period_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->period_entry),"");
  gtk_widget_show (tcd->period_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->task_label,0,2,1,2,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->task_selection_combo,2,6,1,2,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->period_label,0,2,2,3,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->period_entry,2,6,2,3,GTK_FILL,GTK_FILL,0,0);
  
  //Insert Start button
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlStart_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  tcd->start_button = gtk_button_new_with_label("Start");
  g_object_set(G_OBJECT(tcd->start_button), "image", image, NULL);
  gtk_button_set_alignment(GTK_BUTTON(tcd->start_button), 0.0, 0.0);
  gtk_widget_show (tcd->start_button);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->start_button,6,7,2,3,GTK_FILL,GTK_FILL,5,2);
 
  
  // User interface guide
  GList *focus_chain = NULL;
      
  focus_chain = g_list_append (focus_chain, tcd->file_entry);
  focus_chain = g_list_append (focus_chain, tcd->task_selection_combo);
  focus_chain = g_list_append (focus_chain, tcd->period_entry);
  focus_chain = g_list_append (focus_chain, tcd->start_button);

  gtk_container_set_focus_chain(GTK_CONTAINER(tcd->main_box), focus_chain);

  g_list_free(focus_chain);  

  g_signal_connect(G_OBJECT(tcd->start_button), "clicked",(GCallback)start_clicked, tcd);

  gtk_widget_show(tcd->main_box);
  gtk_widget_show(tcd->window);
  
  
  return 0;
}

// When Start button is clicked, we need to perform some checks
// First, check if simulation is needed (period not the same)
// Then try to create the file
// Then compute simulation
void start_clicked (GtkButton *button, gpointer user_data){
  ControlData *tcd = (ControlData*)user_data;
  FILE *a_file;
  GArray *list = get_thread_list();
  ThreadEventData *temp_thread;
  gchar msg[256];
  
  const gchar *name;
  GtkTreeIter iter;
  
  gtk_combo_box_get_active_iter(GTK_COMBO_BOX(tcd->task_selection_combo), &iter);
  gtk_tree_model_get(
      gtk_combo_box_get_model(GTK_COMBO_BOX(tcd->task_selection_combo)),
      &iter, 0, &name, -1);
  if(strcmp(name, " - Choose a task - ") == 0){
    strcpy(msg, "Please choose a task in the list");
    GtkWidget *dialogue = 
      gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        msg);
    gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);    
    return;
  }
  
  // test period value
  int i = gtk_combo_box_get_active(GTK_COMBO_BOX(tcd->task_selection_combo)) - 1;
  
  temp_thread = g_array_index(list, ThreadEventData*, i);

  
  const gchar *period = gtk_entry_get_text(GTK_ENTRY(tcd->period_entry));
  int period_value = atoi(period);
  
  if(period_value <= 0){
    strcpy(msg, "Please enter a valid period value");
    GtkWidget *dialogue = 
      gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        msg);
    gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);    
    return;
  }

  // Checkif same period
  else if (period_value == temp_thread->period){
    strcpy(msg, "Please enter a different period value");
    GtkWidget *dialogue = 
      gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        msg);
    gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);    
    return;
  }

  // Everything is ok
  
  // test existence of files in the directory
  const gchar *file_name = gtk_entry_get_text(GTK_ENTRY(tcd->file_entry));
  printf("%s\n",file_name);
   
  // Try to open file in read mode to check if it exist
  a_file = fopen(file_name, "r");
  if(a_file != NULL){

    strcpy(msg, "Wrong file name, file already exist");
    GtkWidget *dialogue = 
      gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        msg);
    gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);    
    fclose(a_file);
    return;
  }
  else{
    // Create the file
    a_file = fopen(file_name, "w");
    if(a_file == NULL){
      sprintf(msg,"Unable to create file: %s",file_name);
      GtkWidget *dialogue = 
        gtk_message_dialog_new(
          GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_ERROR,
          GTK_BUTTONS_OK,
          msg);
      gtk_dialog_run(GTK_DIALOG(dialogue));
      gtk_widget_destroy(dialogue);    
      return;
    }
    
    fprintf(a_file,"<TRACE SIMULATION>\n");
    compute_simulation(i,period_value,a_file);
    fprintf(a_file,"</TRACE SIMULATION>\n");
    fclose(a_file);
    sprintf(msg,"Simulation finished\n\nSimulation file: %s",file_name);
    GtkWidget *dialogue = 
      gtk_message_dialog_new(
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        msg);
    gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);    

  }
}

void prepare_sim_data(ControlData *tcd){
//  ControlData *tcd = (ControlData*)user_data;
  LttvTracesetContext * tsc = lttvwindow_get_traceset_context(tcd->tab);
  gint num_traces = lttv_traceset_number(tsc->ts);
  gint i;
  LttvTrace *trace;
  LttvTraceState *tstate;

  /* Register simulation calculator */
  LttvHooks *hook_adder = lttv_hooks_new();
  lttv_hooks_add(hook_adder, lttv_xenoltt_sim_hook_add_event_hooks, NULL,LTTV_PRIO_DEFAULT);
    lttv_hooks_add(hook_adder, lttv_state_hook_add_event_hooks, NULL,LTTV_PRIO_DEFAULT);
  
  LttvHooks *hook_remover = lttv_hooks_new();
  lttv_hooks_add(hook_remover, lttv_xenoltt_sim_hook_remove_event_hooks,NULL, LTTV_PRIO_DEFAULT);
    lttv_hooks_add(hook_remover, lttv_state_hook_remove_event_hooks,NULL, LTTV_PRIO_DEFAULT);

  /* Add simulation hook adder to attributes */
  lttvwindowtraces_register_computation_hooks(g_quark_from_string("xenoltt_sim"),
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
      NULL, NULL, NULL,
      hook_adder, hook_remover);

  LttvHooks *background_ready_hook = lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, data_ready, tcd, LTTV_PRIO_DEFAULT);
  
  for(i=0;i<num_traces;i++) {
    trace = lttvwindowtraces_get_trace(i);

    if(lttvwindowtraces_get_ready(g_quark_from_string("xenoltt_sim"),trace)==FALSE) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("xenoltt_sim"),trace) == FALSE) {
        if(!lttvwindowtraces_background_request_find(trace, "xenoltt_sim"))
            lttvwindowtraces_background_request_queue(main_window_get_widget(tcd->tab), trace, "xenoltt_sim");
        lttvwindowtraces_background_notify_queue(tcd,trace,ltt_time_infinite,NULL,background_ready_hook);
      } else { /* in progress */
        lttvwindowtraces_background_notify_current(tcd,trace,ltt_time_infinite,NULL,background_ready_hook);
      
      }
    } else {}
  }
  lttv_hooks_destroy(background_ready_hook);
}

/***********************************************************************************************************************************/

/* Create a popup legend */
GtkWidget *h_xenolttlegend(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_legend, %p", tab);

  GtkWindow *xenoltt_settings = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
 
  g_legend_list = g_slist_append(g_legend_list,xenoltt_settings);
 
  g_object_set_data_full(G_OBJECT(xenoltt_settings),"settings",xenoltt_settings,(GDestroyNotify)xenolttlegend_destructor);
  
  gtk_window_set_title(xenoltt_settings, "XenoLTT Settings");

  GtkWidget *pixmap = create_pixmap(GTK_WIDGET(xenoltt_settings), "xenoltt-color-list.png");

  
  gtk_container_add(GTK_CONTAINER(xenoltt_settings), GTK_WIDGET(pixmap));

  gtk_widget_show(GTK_WIDGET(pixmap));
  gtk_widget_show(GTK_WIDGET(xenoltt_settings));
  

  return NULL; /* This is a popup window */
}

/***********************************************************************************************************************************/

int event_selected_hook(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*) hook_data;
  guint *event_number = (guint*) call_data;
  
  g_debug("DEBUG : event selected by main window : %u", *event_number);
  
  return 0;
}

/* Function that selects the color of status&exemode line */
static inline void prepare_s_e_line(LttvXenoThreadState *thread, DrawContext draw_context,LttvTraceState *ts) {

  PropertiesLine prop_line;
  prop_line.style = GDK_LINE_SOLID;
  prop_line.y = MIDDLE;
  
  //If thread is in ovverun state, we want to display a diffent background color
  if(thread->state->mode == LTTV_XENO_MODE_OVERRUN){ 
    prop_line.line_width = 6;
    prop_line.color = drawing_colors[COL_RUN_IRQ];
    draw_line((void*)&prop_line, (void*)&draw_context);
  }  
  
  // We want to draw a line if thread is the owner of at least one synch
  if (lttv_xeno_thread_synch_owner(ts,thread)){
    prop_line.line_width = 1;
    prop_line.color = drawing_colors[COL_RUN_USER_MODE];
    draw_context.drawinfo.y.middle = (draw_context.drawinfo.y.middle/4);
    draw_line((void*)&prop_line, (void*)&draw_context);
    draw_context.drawinfo.y.middle = (draw_context.drawinfo.y.under/2);
  }
  
  if (lttv_xeno_thread_synch_waiting(ts,thread)){
    prop_line.line_width = 1;
    prop_line.color = drawing_colors[COL_RUN_SOFT_IRQ];
    draw_context.drawinfo.y.middle = (draw_context.drawinfo.y.middle/2);
    draw_line((void*)&prop_line, (void*)&draw_context);
    draw_context.drawinfo.y.middle = (draw_context.drawinfo.y.under/2);
  }  
  
  prop_line.line_width = STATE_LINE_WIDTH;

  if(thread->state->status == LTTV_XENO_STATE_INIT) {
    prop_line.color = drawing_colors[COL_EXIT];           // Created = MAGENTA

  } else if(thread->state->status == LTTV_XENO_STATE_START) {
    prop_line.color = drawing_colors[COL_RUN_SOFT_IRQ];  // Started = RED

  } else if(thread->state->status == LTTV_XENO_STATE_RUN) {
    prop_line.color = drawing_colors[COL_RUN_USER_MODE];  // Running = GREEN

  } else if(thread->state->status == LTTV_XENO_STATE_READY) {
      prop_line.color = drawing_colors[COL_WHITE];  // READY = WHITE

  } else if(thread->state->status == LTTV_XENO_STATE_WAIT_PERIOD) {
    prop_line.color = drawing_colors[COL_WAIT];           // WAIT PERIOD = DARK RED

  } else if(thread->state->status == LTTV_XENO_STATE_SUSPEND) {
    prop_line.color = drawing_colors[COL_RUN_SYSCALL];    // SUSPEND = BLUE

  } else if(thread->state->status == LTTV_XENO_STATE_DEAD) {
    prop_line.color = drawing_colors[COL_BLACK];          // DEAD = BLACK

  } else if(thread->state->status == LTTV_XENO_STATE_UNNAMED) {
    prop_line.color = drawing_colors[COL_WHITE];

  } else {
    g_critical("unknown status : %s", g_quark_to_string(thread->state->status));
    g_assert(FALSE);   /* UNKNOWN STATUS */
  }
  
  draw_line((void*)&prop_line, (void*)&draw_context);
}



gint update_time_window_hook(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*) hook_data;
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
  ThreadList *thread_list = xenoltt_data->thread_list;
  
  const TimeWindowNotifyData *time_window_nofify_data =
  ((const TimeWindowNotifyData *)call_data);
  
  TimeWindow *old_time_window = time_window_nofify_data->old_time_window;
  TimeWindow *new_time_window = time_window_nofify_data->new_time_window;
  
  /* Update the ruler */
  drawing_update_ruler(xenoltt_data->drawing,new_time_window);
  
  
  /* Two cases : zoom in/out or scrolling */
  
  /* In order to make sure we can reuse the old drawing, the scale must
   * be the same and the new time interval being partly located in the
   * currently shown time interval. (reuse is only for scrolling)
   */
  
  g_info("Old time window HOOK : %lu, %lu to %lu, %lu",
  old_time_window->start_time.tv_sec,
  old_time_window->start_time.tv_nsec,
  old_time_window->time_width.tv_sec,
  old_time_window->time_width.tv_nsec);
  
  g_info("New time window HOOK : %lu, %lu to %lu, %lu",
  new_time_window->start_time.tv_sec,
  new_time_window->start_time.tv_nsec,
  new_time_window->time_width.tv_sec,
  new_time_window->time_width.tv_nsec);
  
  if( new_time_window->time_width.tv_sec == old_time_window->time_width.tv_sec
  && new_time_window->time_width.tv_nsec == old_time_window->time_width.tv_nsec) {
    /* Same scale (scrolling) */
    g_info("scrolling");
    LttTime *ns = &new_time_window->start_time;
    LttTime *nw = &new_time_window->time_width;
    LttTime *os = &old_time_window->start_time;
    LttTime *ow = &old_time_window->time_width;
    LttTime old_end = old_time_window->end_time;
    LttTime new_end = new_time_window->end_time;
    if(ltt_time_compare(*ns, old_end) == -1
    && ltt_time_compare(*os, *ns) == -1) {
      g_info("scrolling near right");
      /* Scroll right, keep right part of the screen */
      guint x = 0;
      guint width = xenoltt_data->drawing->width;
      convert_time_to_pixels(
      *old_time_window,
      *ns,
      width,
      &x);
      
      /* Copy old data to new location */
      copy_pixmap_region(thread_list,
      NULL,
      xenoltt_data->drawing->drawing_area->style->black_gc,
      NULL,
      x, 0,
      0, 0,
      xenoltt_data->drawing->width-x+SAFETY, -1);
      
      if(drawing->damage_begin == drawing->damage_end)
        drawing->damage_begin = xenoltt_data->drawing->width-x;
      else
        drawing->damage_begin = 0;
      
      drawing->damage_end = xenoltt_data->drawing->width;
      
      /* Clear the data request background, but not SAFETY */
      rectangle_pixmap(thread_list,
      xenoltt_data->drawing->drawing_area->style->black_gc,
      TRUE,
      drawing->damage_begin+SAFETY, 0,
      drawing->damage_end - drawing->damage_begin,  // do not overlap
      -1);
      gtk_widget_queue_draw(drawing->drawing_area);
      
      /* Get new data for the rest. */
      drawing_data_request(xenoltt_data->drawing,
      drawing->damage_begin, 0,
      drawing->damage_end - drawing->damage_begin,
      xenoltt_data->drawing->height);
    } else {
      if(ltt_time_compare(*ns, *os) == -1
      && ltt_time_compare(*os, new_end) == -1) {
        g_info("scrolling near left");
        /* Scroll left, keep left part of the screen */
        guint x = 0;
        guint width = xenoltt_data->drawing->width;
        convert_time_to_pixels(
        *new_time_window,
        *os,
        width,
        &x);
        
        /* Copy old data to new location */
        copy_pixmap_region  (thread_list,
        NULL,
        xenoltt_data->drawing->drawing_area->style->black_gc,
        NULL,
        0, 0,
        x, 0,
        -1, -1);
        
        if(drawing->damage_begin == drawing->damage_end)
          drawing->damage_end = x;
        else
          drawing->damage_end =
          xenoltt_data->drawing->width;
        
        drawing->damage_begin = 0;
        
        rectangle_pixmap(thread_list,
        xenoltt_data->drawing->drawing_area->style->black_gc,
        TRUE,
        drawing->damage_begin, 0,
        drawing->damage_end - drawing->damage_begin,  // do not overlap
        -1);
        
        gtk_widget_queue_draw(drawing->drawing_area);

        
        /* Get new data for the rest. */
        drawing_data_request(xenoltt_data->drawing,
        drawing->damage_begin, 0,
        drawing->damage_end - drawing->damage_begin,
        xenoltt_data->drawing->height);
        
      } else {
        if(ltt_time_compare(*ns, *os) == 0) {
          g_info("not scrolling");
        } else {
          g_info("scrolling far");
          /* Cannot reuse any part of the screen : far jump */
          
          
          rectangle_pixmap(thread_list,
          xenoltt_data->drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          xenoltt_data->drawing->width+SAFETY, // do not overlap
          -1);

          gtk_widget_queue_draw(drawing->drawing_area);
          
          drawing->damage_begin = 0;
          drawing->damage_end = xenoltt_data->drawing->width;
          
          drawing_data_request(xenoltt_data->drawing,
          0, 0,
          xenoltt_data->drawing->width,
          xenoltt_data->drawing->height);
          
        }
      }
    }
  } else {
    /* Different scale (zoom) */
    g_info("zoom");
    
    rectangle_pixmap(thread_list,
    xenoltt_data->drawing->drawing_area->style->black_gc,
    TRUE,
    0, 0,
    xenoltt_data->drawing->width+SAFETY, // do not overlap
    -1);

    gtk_widget_queue_draw(drawing->drawing_area);
    
    drawing->damage_begin = 0;
    drawing->damage_end = xenoltt_data->drawing->width;
    
    drawing_data_request(xenoltt_data->drawing,
    0, 0,
    xenoltt_data->drawing->width,
    xenoltt_data->drawing->height);
  }
  
  /* Update directly when scrolling */
  gdk_window_process_updates(xenoltt_data->drawing->drawing_area->window,
  TRUE);
  
  return 0;
}

gint traceset_notify(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*) hook_data;
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
  
  if(unlikely(drawing->gc == NULL)) {
    return FALSE;
  }
  if(drawing->dotted_gc == NULL) {
    return FALSE;
  }
  
  drawing_clear(xenoltt_data->drawing);
  threadlist_clear(xenoltt_data->thread_list);
  gtk_widget_set_size_request(
  xenoltt_data->drawing->drawing_area,
  -1, threadlist_get_height(xenoltt_data->thread_list));
  redraw_notify(xenoltt_data, NULL);
  
  request_background_data(xenoltt_data);
  
  return FALSE;
}

gint redraw_notify(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*) hook_data;
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
  GtkWidget *widget = drawing->drawing_area;
  
  drawing->damage_begin = 0;
  drawing->damage_end = drawing->width;
  
  /* fun feature, to be separated someday... */
  drawing_clear(xenoltt_data->drawing);
  threadlist_clear(xenoltt_data->thread_list);
  gtk_widget_set_size_request(
  xenoltt_data->drawing->drawing_area,
  -1, threadlist_get_height(xenoltt_data->thread_list));
  // Clear the images
  rectangle_pixmap(xenoltt_data->thread_list,
  widget->style->black_gc,
  TRUE,
  0, 0,
  drawing->alloc_width,
  -1);
  
  gtk_widget_queue_draw(drawing->drawing_area);
  
  if(drawing->damage_begin < drawing->damage_end) {
    drawing_data_request(drawing,
    drawing->damage_begin,
    0,
    drawing->damage_end-drawing->damage_begin,
    drawing->height);
  }

  return FALSE;
  
}


gint continue_notify(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*) hook_data;
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;

  
  if(drawing->damage_begin < drawing->damage_end) {
    drawing_data_request(drawing,
    drawing->damage_begin,
    0,
    drawing->damage_end-drawing->damage_begin,
    drawing->height);
  }
  
  return FALSE;
}


gint update_current_time_hook(void *hook_data, void *call_data) {
  XenoLTTData *xenoltt_data = (XenoLTTData*)hook_data;
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
  
  LttTime current_time = *((LttTime*)call_data);
  
  TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);
  
  LttTime time_begin = time_window.start_time;
  LttTime width = time_window.time_width;
  LttTime half_width; {
    guint64 time_ll = ltt_time_to_uint64(width);
    time_ll = time_ll >> 1; /* divide by two */
    half_width = ltt_time_from_uint64(time_ll);
  }
  LttTime time_end = ltt_time_add(time_begin, width);
  
  LttvTracesetContext * tsc =
  lttvwindow_get_traceset_context(xenoltt_data->tab);
  
  LttTime trace_start = tsc->time_span.start_time;
  LttTime trace_end = tsc->time_span.end_time;
  
  g_info("New current time HOOK : %lu, %lu", current_time.tv_sec,
  current_time.tv_nsec);
  
  
  
  /* If current time is inside time interval, just move the highlight
   * bar */
  
  /* Else, we have to change the time interval. We have to tell it
   * to the main window. */
  /* The time interval change will take care of placing the current
   * time at the center of the visible area, or nearest possible if we are
   * at one end of the trace. */
  
  
  if(ltt_time_compare(current_time, time_begin) < 0) {
    TimeWindow new_time_window;
    
    if(ltt_time_compare(current_time,
    ltt_time_add(trace_start, half_width)) < 0)
      time_begin = trace_start;
    else
      time_begin = ltt_time_sub(current_time, half_width);
    
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;
    new_time_window.time_width_double = ltt_time_to_double(width);
    new_time_window.end_time = ltt_time_add(time_begin, width);
    
    lttvwindow_report_time_window(xenoltt_data->tab, new_time_window);
  }
  else if(ltt_time_compare(current_time, time_end) > 0) {
    TimeWindow new_time_window;
    
    if(ltt_time_compare(current_time, ltt_time_sub(trace_end, half_width)) > 0)
      time_begin = ltt_time_sub(trace_end, width);
    else
      time_begin = ltt_time_sub(current_time, half_width);
    
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;
    new_time_window.time_width_double = ltt_time_to_double(width);
    new_time_window.end_time = ltt_time_add(time_begin, width);
    
    lttvwindow_report_time_window(xenoltt_data->tab, new_time_window);
    
  }
  gtk_widget_queue_draw(xenoltt_data->drawing->drawing_area);
  
  /* Update directly when scrolling */
  gdk_window_process_updates(xenoltt_data->drawing->drawing_area->window,
  TRUE);
  
  return 0;
}

typedef struct _ClosureData {
  EventsRequest *events_request;
  LttvTracesetState *tss;
  LttTime end_time;
  guint x_end;
} ClosureData;


void draw_closure(gpointer key, gpointer value, gpointer user_data) {
  XenoThreadInfo *thread_Info = (XenoThreadInfo*)key;
  HashedThreadData *hashed_thread_data = (HashedThreadData*)value;
  ClosureData *closure_data = (ClosureData*)user_data;
  
  EventsRequest *events_request = closure_data->events_request;
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracesetState *tss = closure_data->tss;
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;
  
  LttTime evtime = closure_data->end_time;
  
  {
    /* For the thread */
    /* First, check if the current thread is in the state computation
     * thread list. If it is there, that means we must add it right now and
     * draw items from the beginning of the read for it. If it is not
     * present, it's a new process and it was not present : it will
     * be added after the state update.  */
#ifdef EXTRA_CHECK
    g_assert(lttv_traceset_number(tsc->ts) > 0);
#endif //EXTRA_CHECK
LttvTraceContext *tc = tsc->traces[thread_Info->trace_num];
LttvTraceState *ts = (LttvTraceState*)tc;

    LttvXenoThreadState *thread;
    thread = lttv_xeno_state_find_thread(ts, thread_Info->cpu,thread_Info->address);

    if(unlikely(thread != NULL)) {
      ThreadList *thread_list = xenoltt_data->thread_list;
#ifdef EXTRA_CHECK
      /* Should be alike when background info is ready */
      if(xenoltt_data->background_info_waiting==0)
#endif //EXTRA_CHECK
      
      /* Now, the process is in the state hash and our own process hash.
       * We definitely can draw the items related to the ending state.
       */
      
      if(unlikely(ltt_time_compare(hashed_thread_data->next_good_time,evtime) <= 0)) {
        TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);
        
#ifdef EXTRA_CHECK
        if(ltt_time_compare(evtime, time_window.start_time) == -1
        || ltt_time_compare(evtime, time_window.end_time) == 1)
          return;
#endif //EXTRA_CHECK
        XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
        guint width = drawing->width;
        
        guint x = closure_data->x_end;
        
        DrawContext draw_context;
        
        /* Now create the drawing context that will be used to draw
         * items related to the last state. */
        draw_context.drawable = hashed_thread_data->pixmap;
        draw_context.gc = drawing->gc;
        draw_context.pango_layout = drawing->pango_layout;
        draw_context.drawinfo.end.x = x;
        
        draw_context.drawinfo.y.over = 1;
        draw_context.drawinfo.y.middle = (hashed_thread_data->height/2);
        draw_context.drawinfo.y.under = hashed_thread_data->height;
        
        draw_context.drawinfo.start.offset.over = 0;
        draw_context.drawinfo.start.offset.middle = 0;
        draw_context.drawinfo.start.offset.under = 0;
        draw_context.drawinfo.end.offset.over = 0;
        draw_context.drawinfo.end.offset.middle = 0;
        draw_context.drawinfo.end.offset.under = 0;
        
        if(unlikely(x == hashed_thread_data->x.middle &&
        hashed_thread_data->x.middle_used)) {
          /* Jump */
        } else {
          draw_context.drawinfo.start.x = hashed_thread_data->x.middle;
          /* Draw the line */
//          PropertiesLine prop_line = 
          prepare_s_e_line(thread,draw_context,ts);
//          draw_line((void*)&prop_line, (void*)&draw_context);
          
          /* become the last x position */
          if(likely(x != hashed_thread_data->x.middle)) {
            hashed_thread_data->x.middle = x;
            /* but don't use the pixel */
            hashed_thread_data->x.middle_used = FALSE;
            
            /* Calculate the next good time */
            convert_pixels_to_time(width, x+1, time_window,
            &hashed_thread_data->next_good_time);
          }
        }
      }
    }
  }
  return;
}


int xenoltt_before_chunk(void *hook_data, void *call_data) {
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;

  drawing_chunk_begin(events_request, tss);

return 0;
}

int xenoltt_before_request(void *hook_data, void *call_data) {
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  
  drawing_data_request_begin(events_request, tss);
  
  return 0;
}


/*
 * after request is necessary in addition of after chunk in order to draw
 * lines until the end of the screen. after chunk just draws lines until
 * the last event.
 *
 * for each process
 *    draw closing line
 *    expose
 */
int xenoltt_after_request(void *hook_data, void *call_data) {
  EventsRequest *events_request = (EventsRequest*)hook_data;
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  
  ThreadList *thread_list = xenoltt_data->thread_list;
  LttTime end_time = events_request->end_time;
  
  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;
  closure_data.end_time = end_time;
  
  TimeWindow time_window =
  lttvwindow_get_time_window(xenoltt_data->tab);
  guint width = xenoltt_data->drawing->width;
  convert_time_to_pixels(
  time_window,
  end_time,
  width,
  &closure_data.x_end);
  
  
  /* Draw last items */
  g_hash_table_foreach(thread_list->thread_hash, draw_closure,
  (void*)&closure_data);
  
  
  /* Request expose */
  drawing_request_expose(events_request, tss, end_time);
  return 0;
}

/*
 * for each process
 *    draw closing line
 * expose
 */
int xenoltt_after_chunk(void *hook_data, void *call_data) {
  EventsRequest *events_request = (EventsRequest*)hook_data;
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  LttvTracesetContext *tsc = (LttvTracesetContext*)call_data;
  LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
  LttTime end_time;
  
  ThreadList *thread_list = xenoltt_data->thread_list;
  guint i;
  LttvTraceset *traceset = tsc->ts;
  guint nb_trace = lttv_traceset_number(traceset);
  
  /* Only execute when called for the first trace's events request */
  if(!thread_list->current_hash_data) return;
  
  for(i = 0 ; i < nb_trace ; i++) {
    g_free(thread_list->current_hash_data[i]);
  }
  g_free(thread_list->current_hash_data);
  thread_list->current_hash_data = NULL;
  
  if(tfc != NULL)
    end_time = LTT_TIME_MIN(tfc->timestamp, events_request->end_time);
  else /* end of traceset, or position now out of request : end */
    end_time = events_request->end_time;
  
  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;
  closure_data.end_time = end_time;
  
  TimeWindow time_window =
  lttvwindow_get_time_window(xenoltt_data->tab);
  guint width = xenoltt_data->drawing->width;
  convert_time_to_pixels(
  time_window,
  end_time,
  width,
  &closure_data.x_end);
  
  /* Draw last items */
  g_hash_table_foreach(thread_list->thread_hash, draw_closure,
    (void*)&closure_data);

/* Request expose (updates damages zone also) */
drawing_request_expose(events_request, tss, end_time);

return 0;
}

/******************************************************************************
 * Xenoami Thread Initialization hook
 ******************************************************************************/
int xenoltt_thread_init(void *hook_data, void *call_data){
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;
  
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  
  LttTime evtime = ltt_event_time(e);
  
  GQuark name = g_quark_from_string(ltt_event_get_string(e, thf->f1));
  gulong address = ltt_event_get_long_unsigned(e, thf->f2);
  guint prio = ltt_event_get_unsigned(e, thf->f3);
  
  guint trace_num = ts->parent.index;
  
  
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */
  thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);

  if (thread != NULL){
    birth = thread->creation_time;
    /* Add thread to thread list (if not present) */
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;

    hashed_thread_data = threadlist_get_thread_data(thread_list,thread->address,tfs->cpu, &birth, trace_num);
    XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;

    if(hashed_thread_data == NULL){
      /* Xenomai Thread not present */
      XenoThreadInfo *thread_Info;
      threadlist_add(thread_list,
      drawing,
      address,
      thread->prio,
      tfs->cpu,
      thread->period,
      &birth,
      trace_num,
      thread->name,
      &pl_height,
      &thread_Info,
      &hashed_thread_data);
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    } 
  }
/*
 else{
    g_warning("Cannot find thread initialization %s - %u", g_quark_to_string(name), address);
  }
*/
  return 0;
}

/******************************************************************************
 * Xenoami Thread Set Period hook
 ******************************************************************************/
int xenoltt_thread_set_period(void *hook_data, void *call_data){
  
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;
  
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  
  GQuark name = g_quark_from_string(ltt_event_get_string(e, thf->f1));
  gulong address = ltt_event_get_long_unsigned(e, thf->f2);
  guint period = ltt_event_get_unsigned(e, thf->f3);
  gulong timer_address = ltt_event_get_long_unsigned(e,ltt_eventtype_field_by_name(ltt_event_eventtype(e),g_quark_from_string("timer_address")));
  LttTime evtime = ltt_event_time(e);
  
  guint trace_num = ts->parent.index;
  
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */
  thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);

  if (thread != NULL){ // Thread present in table
    birth = thread->creation_time;
    // Add thread to thread list (if not present)
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;

    hashed_thread_data = threadlist_get_thread_data(thread_list,thread->address,tfs->cpu, &birth, trace_num);
    if(hashed_thread_data != NULL){

      threadlist_set_period(thread_list, period, hashed_thread_data);

      //Save the timer address
      hashed_thread_data->timer_address = timer_address;

      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }
    else{
      // Xenomai Thread not present
      XenoThreadInfo *thread_Info;
      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      threadlist_add(thread_list,
      drawing,
      address,
      thread->prio, // Priority
      tfs->cpu,
      thread->period, //Period
      &birth,
      trace_num,
      thread->name,
      &pl_height,
      &thread_Info,
      &hashed_thread_data);
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }  
  }
/*
 else{
    g_warning("Cannot find thread in set_period %s - %u", g_quark_to_string(name), address);
  }
*/
  return 0;
}



/******************************************************************************/

int xenoltt_before_thread_hook(void *hook_data, void *call_data){
  
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;
  
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  
  GQuark event_name = ltt_eventtype_name(ltt_event_eventtype(e));
  
  gulong address = ltt_event_get_long_unsigned(e, thf->f1);
  
  guint trace_num = ts->parent.index;
  
  LttTime evtime = ltt_event_time(e);
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */

  if (event_name == LTT_EVENT_XENOLTT_TIMER_TICK)
    thread = lttv_xeno_state_find_thread_from_timer(ts, ANY_CPU, address);
  else 
    thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);
  
  if (thread != NULL){ // Thread present in table
    birth = thread->creation_time;
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;
    hashed_thread_data = threadlist_get_thread_data(thread_list,thread->address,tfs->cpu, &birth, trace_num);

    if(hashed_thread_data == NULL){ // Xenomai Thread not present
      XenoThreadInfo *thread_Info;
      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      threadlist_add(thread_list,drawing,thread->address,thread->prio,tfs->cpu,
                      thread->period, &birth, trace_num, thread->name, &pl_height, 
                      &thread_Info, &hashed_thread_data);
      
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }
    else{

      /* Now, the process is in the state hash and our own process hash.
       * We definitely can draw the items related to the ending state.
       */

      if(ltt_time_compare(hashed_thread_data->next_good_time,evtime) > 0) {
        if(hashed_thread_data->x.middle_marked == FALSE) {

          TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);
  #ifdef EXTRA_CHECK
            if(ltt_time_compare(evtime, time_window.start_time) == -1 || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
  #endif //EXTRA_CHECK
            XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
            guint width = drawing->width;
            guint x;
            convert_time_to_pixels(time_window,evtime,width,&x);

            // Draw collision indicator 
            gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
            gdk_draw_point(hashed_thread_data->pixmap,drawing->gc,x,
              COLLISION_POSITION(hashed_thread_data->height));
            hashed_thread_data->x.middle_marked = TRUE;
        }
      } else {
        TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);
  #ifdef EXTRA_CHECK
          if(ltt_time_compare(evtime, time_window.start_time) == -1 || ltt_time_compare(evtime, time_window.end_time) == 1)
            return;
  #endif //EXTRA_CHECK
          XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
          guint width = drawing->width;
          guint x;
          convert_time_to_pixels(time_window,evtime,width,&x);

          /* Jump over draw if we are at the same x position */
          if(x == hashed_thread_data->x.middle && hashed_thread_data->x.middle_used) {
            if(hashed_thread_data->x.middle_marked == FALSE) {
              // Draw collision indicator 
              gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
              gdk_draw_point(hashed_thread_data->pixmap,drawing->gc,x,
                COLLISION_POSITION(hashed_thread_data->height));
              hashed_thread_data->x.middle_marked = TRUE;
            }
          } else {
            DrawContext draw_context;


            /* Now create the drawing context that will be used to draw
             * items related to the last state. */
            draw_context.drawable = hashed_thread_data->pixmap;
            draw_context.gc = drawing->gc;
            draw_context.pango_layout = drawing->pango_layout;
            draw_context.drawinfo.start.x = hashed_thread_data->x.middle;
            draw_context.drawinfo.end.x = x;

            draw_context.drawinfo.y.over = 1;
            draw_context.drawinfo.y.middle = (hashed_thread_data->height/2);
            draw_context.drawinfo.y.under = hashed_thread_data->height;

            draw_context.drawinfo.start.offset.over = 0;
            draw_context.drawinfo.start.offset.middle = 0;
            draw_context.drawinfo.start.offset.under = 0;
            draw_context.drawinfo.end.offset.over = 0;
            draw_context.drawinfo.end.offset.middle = 0;
            draw_context.drawinfo.end.offset.under = 0;

            prepare_s_e_line(thread,draw_context,ts); // Draw the line

            /* become the last x position */
            hashed_thread_data->x.middle = x;
            hashed_thread_data->x.middle_used = TRUE;
            hashed_thread_data->x.middle_marked = FALSE;

            /* Calculate the next good time */
            convert_pixels_to_time(width, x+1, time_window,
            &hashed_thread_data->next_good_time);
          }
      }
    }
  }
/*
 else{
    g_warning("Cannot find thread in before hook - %u", g_quark_to_string(address));
  }
*/
  return 0;
}

// When the thread switch is read, we need to change the state of the thread_out also
int xenoltt_before_thread_switch_hook(void *hook_data, void *call_data){
  
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;  
  XenoLTTData *xenoltt_data = events_request->viewer_data;  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  gulong address = ltt_event_get_long_unsigned(e, thf->f2);
  
  guint trace_num = ts->parent.index;
  
  LttTime evtime = ltt_event_time(e);
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */

  thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);
  
  if (thread != NULL){ // Thread present in table
    birth = thread->creation_time;
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;
    hashed_thread_data = threadlist_get_thread_data(thread_list,thread->address,tfs->cpu, &birth, trace_num);

    if(hashed_thread_data == NULL){ // Xenomai Thread not present
      XenoThreadInfo *thread_Info;
      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      threadlist_add(thread_list,drawing,thread->address,thread->prio, tfs->cpu,
                    thread->period, &birth,trace_num, thread->name, &pl_height,
                    &thread_Info,&hashed_thread_data);
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }
    else{
        TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);

        XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
        guint width = drawing->width;
        guint x;
        convert_time_to_pixels(time_window,evtime,width,&x);

        DrawContext draw_context;


        /* Now create the drawing context that will be used to draw
         * items related to the last state. */
        draw_context.drawable = hashed_thread_data->pixmap;
        draw_context.gc = drawing->gc;
        draw_context.pango_layout = drawing->pango_layout;
        draw_context.drawinfo.start.x = hashed_thread_data->x.middle;
        draw_context.drawinfo.end.x = x;

        draw_context.drawinfo.y.over = 1;
        draw_context.drawinfo.y.middle = (hashed_thread_data->height/2);
        draw_context.drawinfo.y.under = hashed_thread_data->height;

        draw_context.drawinfo.start.offset.over = 0;
        draw_context.drawinfo.start.offset.middle = 0;
        draw_context.drawinfo.start.offset.under = 0;
        draw_context.drawinfo.end.offset.over = 0;
        draw_context.drawinfo.end.offset.middle = 0;
        draw_context.drawinfo.end.offset.under = 0;

        
        prepare_s_e_line(thread,draw_context,ts); // Draw the line

        /* become the last x position */
        hashed_thread_data->x.middle = x;
        hashed_thread_data->x.middle_used = TRUE;
        hashed_thread_data->x.middle_marked = FALSE;

        /* Calculate the next good time */
        convert_pixels_to_time(width, x+1, time_window,
        &hashed_thread_data->next_good_time);
    }
  }
/*
 else{
    g_warning("Cannot find thread in before hook - %u", g_quark_to_string(address));
  }
*/
  return 0;
}

/******************************************************************************/
/* xenoltt_after_thread_hook */

int xenoltt_after_thread_hook(void *hook_data, void *call_data){
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;
  
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  
  GQuark event_name = ltt_eventtype_name(ltt_event_eventtype(e));
  
  gulong address = ltt_event_get_long_unsigned(e, thf->f1);
  
  guint trace_num = ts->parent.index;
  LttTime evtime = ltt_event_time(e);
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */

  if (event_name == LTT_EVENT_XENOLTT_TIMER_TICK){
    thread = lttv_xeno_state_find_thread_from_timer(ts, ANY_CPU,address);
  }
  else thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);
  
  if (thread != NULL){ // Thread present in table
    birth = thread->creation_time;
    // Add thread to thread list (if not present)
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;
    guint pl_height = 0;

    hashed_thread_data = threadlist_get_thread_data(thread_list, thread->address,tfs->cpu, &birth, trace_num);

    if(hashed_thread_data == NULL){
      // Xenomai Thread not present
      XenoThreadInfo *thread_Info;
      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      threadlist_add(thread_list,
      drawing,
      thread->address,
      thread->prio, // Priority
      tfs->cpu,
      thread->period, //Period
      &birth,
      trace_num,
      thread->name,
      &pl_height,
      &thread_Info,
      &hashed_thread_data);
      gtk_widget_set_size_request(drawing->drawing_area, -1, pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }

    /* Set the current Xenomai thread */
    thread_list->current_hash_data[trace_num][tfs->cpu] = hashed_thread_data;

    if(ltt_time_compare(hashed_thread_data->next_good_time, evtime) <= 0){
      TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);

  #ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
  #endif //EXTRA_CHECK
      XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
      guint width = drawing->width;
      guint new_x;

      convert_time_to_pixels(time_window,evtime,width,&new_x);

      if(hashed_thread_data->x.middle != new_x) {
        hashed_thread_data->x.middle = new_x;
        hashed_thread_data->x.middle_used = FALSE;
        hashed_thread_data->x.middle_marked = FALSE;      
      }
    }
  }
/*
 else{
    g_warning("Cannot find thread in after hook %u", g_quark_to_string(address));
  }
*/
  return 0;
}


int xenoltt_draw_icons(void *hook_data, void *call_data){

  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  EventsRequest *events_request = (EventsRequest*)thf->hook_data;
  
  XenoLTTData *xenoltt_data = events_request->viewer_data;
  
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
    
  // We must update the state of the current Xenomai Thread
  GQuark event_name = ltt_eventtype_name(ltt_event_eventtype(e));
    
  guint trace_num = ts->parent.index;
  LttTime evtime = ltt_event_time(e);
 
  gulong address = ltt_event_get_long_unsigned(e, thf->f2);
  
  LttvXenoThreadState *thread;
  LttTime birth;
  guint pl_height = 0;
  /* Find thread in the list... */
  
  HashedThreadData *hashed_thread_data = NULL;
  ThreadList *thread_list = xenoltt_data->thread_list;
  
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;

  if (event_name == LTT_EVENT_XENOLTT_TIMER_TICK){
    thread = lttv_xeno_state_find_thread_from_timer(ts, ANY_CPU,address);
  }
  else thread = lttv_xeno_state_find_thread(ts, ANY_CPU, address);
  
  if (thread != NULL){ // Thread present in table
    birth = thread->creation_time;
    // Add thread to thread list (if not present)
    HashedThreadData *hashed_thread_data = NULL;
    ThreadList *thread_list = xenoltt_data->thread_list;
    guint pl_height = 0;

    hashed_thread_data = threadlist_get_thread_data(thread_list, thread->address,tfs->cpu, &birth, trace_num);

    if(hashed_thread_data != NULL){
      TimeWindow time_window = lttvwindow_get_time_window(xenoltt_data->tab);
      guint width = drawing->width;
      guint new_x;


      convert_time_to_pixels(time_window,evtime,width,&new_x);

      DrawContext draw_context;

      /* Now create the drawing context that will be used to draw
       * items related to the event. */
      draw_context.drawable = hashed_thread_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;

      draw_context.drawinfo.y.over = -1;
      draw_context.drawinfo.y.middle = (hashed_thread_data->height/2);
      draw_context.drawinfo.y.under = hashed_thread_data->height;

      draw_context.drawinfo.start.x = new_x;
      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;

      draw_context.drawinfo.end.x = new_x + 2;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      if(event_name == LTT_EVENT_XENOLTT_THREAD_INIT) {
        // When a thread is initialized, we place a small green flag
        PropertiesIcon prop_icon;
        prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagThreadInit.xpm";
        prop_icon.width = 6;
        prop_icon.height = 13;
        prop_icon.position.x = POS_START;
        prop_icon.position.y = OVER;

        draw_context.drawinfo.end.x = new_x + 6;
        draw_icon((void*)&prop_icon, (void*)&draw_context); 

        PropertiesText prop_text;
        prop_text.foreground = &drawing_colors[COL_RUN_USER_MODE];
        prop_text.background = &drawing_colors[COL_BLACK];
        prop_text.size = 8;
        prop_text.text = ltt_event_get_string(e, thf->f1);  
        prop_text.position.x = POS_START;
        prop_text.position.y = OVER;

        draw_context.drawinfo.end.x = new_x + 1000;
        draw_text((void*)&prop_text, (void*)&draw_context);


      } else if(event_name == LTT_EVENT_XENOLTT_THREAD_SET_PERIOD) {
        PropertiesArc prop_arc;
        prop_arc.color = &drawing_colors[COL_RUN_TRAP];
        prop_arc.size = 6;
        prop_arc.filled = 1;
        prop_arc.position.x = POS_START;
        prop_arc.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 6;
        draw_arc((void*)&prop_arc, (void*)&draw_context); 

        guint period = ltt_event_get_unsigned(e, thf->f3);  
        gchar text[MAX_PATH_LEN] = "Period :";
        sprintf(text,"%s %u",text,period);
        //We must update the thread priority in the list
        threadlist_set_period(thread_list, period, hashed_thread_data);

        PropertiesText prop_text;
        prop_text.foreground = &drawing_colors[COL_RUN_TRAP]; //Yellow
        prop_text.background = &drawing_colors[COL_BLACK];
        prop_text.size = 8;
        prop_text.text = text;  
        prop_text.position.x = POS_START;
        prop_text.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 1000;
        draw_text((void*)&prop_text, (void*)&draw_context);

      } else if(event_name == LTT_EVENT_XENOLTT_THREAD_RENICE) {
        PropertiesArc prop_arc;
        prop_arc.color = &drawing_colors[COL_WHITE];
        prop_arc.size = 6;
        prop_arc.filled = 1;
        prop_arc.position.x = POS_START;
        prop_arc.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 6;
        draw_arc((void*)&prop_arc, (void*)&draw_context); 

        guint prio = ltt_event_get_unsigned(e, thf->f3);  
        gchar text[MAX_PATH_LEN] = "Priority :";
        sprintf(text,"%s %u",text,prio);
        //We must update the thread priority in the list
        threadlist_set_prio(thread_list,prio,hashed_thread_data);

        PropertiesText prop_text;
        prop_text.foreground = &drawing_colors[COL_WHITE];
        prop_text.background = &drawing_colors[COL_BLACK];
        prop_text.size = 8;
        prop_text.text = text;  
        prop_text.position.x = POS_START;
        prop_text.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 500;
        draw_text((void*)&prop_text, (void*)&draw_context);

      } else if(event_name == LTT_EVENT_XENOLTT_TIMER_TICK) {
        PropertiesLine prop_line;
        prop_line.line_width = 20;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        prop_line.color = drawing_colors[COL_WHITE];           // WHITE

        draw_context.drawinfo.start.x = new_x;
        draw_context.drawinfo.end.x = new_x + 2;
        draw_line((void*)&prop_line, (void*)&draw_context); 

      } else if(event_name == LTT_EVENT_XENOLTT_THREAD_DELETE) {
        // When a thread is deleted, we place a small red flag
        PropertiesIcon prop_icon;
        prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagThreadDelete.xpm";
        prop_icon.width = 6;
        prop_icon.height = 13;
        prop_icon.position.x = POS_START;
        prop_icon.position.y = OVER;

        draw_context.drawinfo.end.x = new_x + 10;
        draw_icon((void*)&prop_icon, (void*)&draw_context);       

      } else if(event_name == LTT_EVENT_XENOLTT_THREAD_MISSED_PERIOD) {
        PropertiesArc prop_arc;
        prop_arc.color = &drawing_colors[COL_WHITE];
        prop_arc.size = 6;
        prop_arc.filled = 1;
        prop_arc.position.x = POS_START;
        prop_arc.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 6;
        draw_arc((void*)&prop_arc, (void*)&draw_context); 

        guint overruns = ltt_event_get_unsigned(e, thf->f3);  
        gchar text[MAX_PATH_LEN] = "Overruns :";
        sprintf(text,"%s %u",text,overruns);

        PropertiesText prop_text;
        prop_text.foreground = &drawing_colors[COL_WHITE];
        prop_text.background = &drawing_colors[COL_BLACK];
        prop_text.size = 8;
        prop_text.text = text;  
        prop_text.position.x = POS_START;
        prop_text.position.y = MIDDLE;

        draw_context.drawinfo.end.x = new_x + 1000;
        draw_text((void*)&prop_text, (void*)&draw_context);
        
      } else if(event_name == LTT_EVENT_XENOLTT_SYNCH_SET_OWNER || 
                event_name == LTT_EVENT_XENOLTT_SYNCH_WAKEUP1 ||
                event_name == LTT_EVENT_XENOLTT_SYNCH_WAKEUPX) {
                  
        gulong synch_address = ltt_event_get_long_unsigned(e, thf->f3);
        
        LttvXenoSynchState *synch = lttv_xeno_state_find_synch(ts,synch_address);
        
        if (synch != NULL){
          // When a thread has a synch, we place a small blue flag
          PropertiesIcon prop_icon;
          prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagSynchOwner.xpm";
          prop_icon.width = 6;
          prop_icon.height = 13;
          prop_icon.position.x = POS_START;
          prop_icon.position.y = OVER;

          int i;
          LttvXenoThreadState *temp_thread;
          
          // If the thread has a lower priority than another we need to inform
          // about priority inversion
          for(i=0;i<synch->state->waiting_threads->len;i++){
            temp_thread = g_array_index(synch->state->waiting_threads, LttvXenoThreadState*, i);
            if (temp_thread->address != thread->address){
              if (thread->prio < temp_thread->prio){
                prop_icon.width = 13;
                prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagPriority.xpm";
              }
            }
          }
          
          draw_context.drawinfo.end.x = new_x + 10;
          draw_icon((void*)&prop_icon, (void*)&draw_context); 
        }
      } else if(event_name == LTT_EVENT_XENOLTT_SYNCH_SLEEP_ON) {
                  
        gulong synch_address = ltt_event_get_long_unsigned(e, thf->f3);
        
        LttvXenoSynchState *synch = lttv_xeno_state_find_synch(ts,synch_address);
        
        if (synch != NULL){
          // When a thread has a synch, we place a small blue flag
          PropertiesIcon prop_icon;
          prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagSynchSleep.xpm";
          prop_icon.width = 6;
          prop_icon.height = 13;
          prop_icon.position.x = POS_START;
          prop_icon.position.y = OVER;

          draw_context.drawinfo.end.x = new_x + 10;
          draw_icon((void*)&prop_icon, (void*)&draw_context); 
        }
      } else if(event_name == LTT_EVENT_XENOLTT_SYNCH_UNLOCK) {
                  
        gulong synch_address = ltt_event_get_long_unsigned(e, thf->f3);
        
        LttvXenoSynchState *synch = lttv_xeno_state_find_synch(ts,synch_address);
        
        if (synch != NULL){
          // When a thread has a synch, we place a small blue flag
          PropertiesIcon prop_icon;
          prop_icon.icon_name = "/usr/local/share/LinuxTraceToolkitViewer/pixmaps/FlagSynchUnlock.xpm";
          prop_icon.width = 6;
          prop_icon.height = 13;
          prop_icon.position.x = POS_START;
          prop_icon.position.y = OVER;

          draw_context.drawinfo.end.x = new_x + 10;
          draw_icon((void*)&prop_icon, (void*)&draw_context); 
        }
      }
    } 
  }
  return 0;
}
