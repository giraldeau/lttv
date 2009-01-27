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
#include <inttypes.h>

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


#include "eventhooks.h"
#include "cfv.h"
#include "processlist.h"
#include "drawing.h"


#define MAX_PATH_LEN 256
#define STATE_LINE_WIDTH 6
#define COLLISION_POSITION(height) (((height - STATE_LINE_WIDTH)/2) -3)

extern GSList *g_legend_list;


/* Action to do when background computation completed.
 *
 * Wait for all the awaited computations to be over.
 */

static gint background_ready(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData *)hook_data;

  resourceview_data->background_info_waiting--;
  
  if(resourceview_data->background_info_waiting == 0) {
    g_message("control flow viewer : background computation data ready.");

    drawing_clear(resourceview_data->drawing);
    processlist_clear(resourceview_data->process_list);
    gtk_widget_set_size_request(
      resourceview_data->drawing->drawing_area,
      -1, processlist_get_height(resourceview_data->process_list));
    redraw_notify(resourceview_data, NULL);
  }

  return 0;
}


/* Request background computation. Verify if it is in progress or ready first.
 * Only for each trace in the tab's traceset.
 */
static void request_background_data(ControlFlowData *resourceview_data)
{
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(resourceview_data->tab);
  gint num_traces = lttv_traceset_number(tsc->ts);
  gint i;
  LttvTrace *trace;
  LttvTraceState *tstate;

  LttvHooks *background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, resourceview_data,
      LTTV_PRIO_DEFAULT);
  resourceview_data->background_info_waiting = 0;
  
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
              main_window_get_widget(resourceview_data->tab), trace, "state");
        lttvwindowtraces_background_notify_queue(resourceview_data,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 background_ready_hook);
        resourceview_data->background_info_waiting++;
      } else { /* in progress */
      
        lttvwindowtraces_background_notify_current(resourceview_data,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   background_ready_hook);
        resourceview_data->background_info_waiting++;
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


/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param tab A pointer to the parent tab.
 * @return The widget created.
 */
GtkWidget *
h_resourceview(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_guicontrolflow, %p", tab);
  ControlFlowData *resourceview_data = resourceview(ptab);
  
  resourceview_data->tab = tab;
  
  // Unreg done in the GuiControlFlow_Destructor
  lttvwindow_register_traceset_notify(tab,
        traceset_notify,
        resourceview_data);
    
  lttvwindow_register_time_window_notify(tab,
                                         update_time_window_hook,
                                         resourceview_data);
  lttvwindow_register_current_time_notify(tab,
                                          update_current_time_hook,
                                          resourceview_data);
  lttvwindow_register_redraw_notify(tab,
                                    redraw_notify,
                                    resourceview_data);
  lttvwindow_register_continue_notify(tab,
                                      continue_notify,
                                      resourceview_data);
  request_background_data(resourceview_data);
  

  return guicontrolflow_get_widget(resourceview_data) ;
  
}

void legend_destructor(GtkWindow *legend)
{
  g_legend_list = g_slist_remove(g_legend_list, legend);
}

/* Create a popup legend */
GtkWidget *
h_legend(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_legend, %p", tab);

  GtkWindow *legend = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
 
  g_legend_list = g_slist_append(
      g_legend_list,
      legend);
 
  g_object_set_data_full(
      G_OBJECT(legend),
      "legend",
      legend,
      (GDestroyNotify)legend_destructor);
  
  gtk_window_set_title(legend, "Control Flow View Legend");

  GtkWidget *pixmap = create_pixmap(GTK_WIDGET(legend), "lttv-color-list.png");
  
  gtk_container_add(GTK_CONTAINER(legend), GTK_WIDGET(pixmap));

  gtk_widget_show(GTK_WIDGET(pixmap));
  gtk_widget_show(GTK_WIDGET(legend));
  

  return NULL; /* This is a popup window */
}


int event_selected_hook(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*) hook_data;
  guint *event_number = (guint*) call_data;

  g_debug("DEBUG : event selected by main window : %u", *event_number);
  
  return 0;
}

static void cpu_set_line_color(PropertiesLine *prop_line, LttvCPUState *s)
{
  GQuark present_state;

  if(s->mode_stack->len == 0)
    present_state = LTTV_CPU_UNKNOWN;
  else
    present_state = ((GQuark*)s->mode_stack->data)[s->mode_stack->len-1];

  if(present_state == LTTV_CPU_IDLE) {
    prop_line->color = drawing_colors_cpu[COL_CPU_IDLE];
  }
  else if(present_state == LTTV_CPU_BUSY) {
    prop_line->color = drawing_colors_cpu[COL_CPU_BUSY];
  }
  else if(present_state == LTTV_CPU_IRQ) {
    prop_line->color = drawing_colors_cpu[COL_CPU_IRQ];
  }
  else if(present_state == LTTV_CPU_SOFT_IRQ) {
    prop_line->color = drawing_colors_cpu[COL_CPU_SOFT_IRQ];
  }
  else if(present_state == LTTV_CPU_TRAP) {
    prop_line->color = drawing_colors_cpu[COL_CPU_TRAP];
  } else {
    prop_line->color = drawing_colors_cpu[COL_CPU_UNKNOWN];
  }
}

static void irq_set_line_color(PropertiesLine *prop_line, LttvIRQState *s)
{
  GQuark present_state;
  if(s->mode_stack->len == 0)
    present_state = LTTV_IRQ_UNKNOWN;
  else
    present_state = ((GQuark*)s->mode_stack->data)[s->mode_stack->len-1];

  if(present_state == LTTV_IRQ_IDLE) {
    prop_line->color = drawing_colors_irq[COL_IRQ_IDLE];
  }
  else if(present_state == LTTV_IRQ_BUSY) {
    prop_line->color = drawing_colors_irq[COL_IRQ_BUSY];
  }
  else {
    prop_line->color = drawing_colors_irq[COL_IRQ_UNKNOWN];
  }
}

static void soft_irq_set_line_color(PropertiesLine *prop_line, LttvSoftIRQState *s)
{
  GQuark present_state;
  if(s->running)
    prop_line->color = drawing_colors_soft_irq[COL_SOFT_IRQ_BUSY];
  else if(s->pending)
    prop_line->color = drawing_colors_soft_irq[COL_SOFT_IRQ_PENDING];
  else
    prop_line->color = drawing_colors_soft_irq[COL_SOFT_IRQ_IDLE];
}

static void trap_set_line_color(PropertiesLine *prop_line, LttvTrapState *s)
{
  GQuark present_state;
  if(s->running == 0)
    prop_line->color = drawing_colors_trap[COL_TRAP_IDLE];
  else
    prop_line->color = drawing_colors_trap[COL_TRAP_BUSY];
}

static void bdev_set_line_color(PropertiesLine *prop_line, LttvBdevState *s)
{
  GQuark present_state;
  if(s == 0 || s->mode_stack->len == 0)
    present_state = LTTV_BDEV_UNKNOWN;
  else
    present_state = ((GQuark*)s->mode_stack->data)[s->mode_stack->len-1];

  if(present_state == LTTV_BDEV_IDLE) {
    prop_line->color = drawing_colors_bdev[COL_BDEV_IDLE];
  }
  else if(present_state == LTTV_BDEV_BUSY_READING) {
    prop_line->color = drawing_colors_bdev[COL_BDEV_BUSY_READING];
  }
  else if(present_state == LTTV_BDEV_BUSY_WRITING) {
    prop_line->color = drawing_colors_bdev[COL_BDEV_BUSY_WRITING];
  }
  else {
    prop_line->color = drawing_colors_bdev[COL_BDEV_UNKNOWN];
  }
}

/* before_schedchange_hook
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


int before_schedchange_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);
  gint target_pid_saved = tfc->target_pid;

  LttTime evtime = ltt_event_time(e);
  LttvFilter *filter = resourceview_data->filter;

  /* we are in a schedchange, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */

  guint pid_out;
  guint pid_in;
  pid_out = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  pid_in = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
// TODO: can't we reenable this? pmf
//  if(pid_in != 0 && pid_out != 0) {
//    /* not a transition to/from idle */
//    return 0;
//  }

  tfc->target_pid = pid_out;

  guint cpu = tfs->cpu;

  guint trace_num = ts->parent.index;
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;
  
  hashed_process_data = resourcelist_obtain_cpu(resourceview_data, trace_num, cpu);
  
  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */
  
  if(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0)
  {
    if(hashed_process_data->x.middle_marked == FALSE) {
  
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);
#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
        return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  } else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);
#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
              || ltt_time_compare(evtime, time_window.end_time) == 1)
      return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;
    convert_time_to_pixels(
              time_window,
              evtime,
              width,
              &x);

    /* Jump over draw if we are at the same x position */
    if(x == hashed_process_data->x.middle &&
         hashed_process_data->x.middle_used)
    {
      if(hashed_process_data->x.middle_marked == FALSE) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    } else {
      DrawContext draw_context;

      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        //PropertiesLine prop_line = prepare_s_e_line(process);
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        cpu_set_line_color(&prop_line, tfs->cpu_state);
        draw_line((void*)&prop_line, (void*)&draw_context);

      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }

  return 0;
}

/* after_schedchange_hook
 * 
 * The draw after hook is called by the reading API to have a
 * particular event drawn on the screen.
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */
int after_schedchange_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  LttEvent *e;

  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = resourceview_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  /* Add process to process list (if not present) */
  LttvProcessState *process_in;
  LttTime birth;
  guint pl_height = 0;
  HashedResourceData *hashed_process_data_in = NULL;

  ProcessList *process_list = resourceview_data->process_list;
  
  guint pid_in;
  {
    guint pid_out;
    pid_out = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
    pid_in = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
  }


  /* Find process pid_in in the list... */
  //process_in = lttv_state_find_process(ts, ANY_CPU, pid_in);
  //process_in = tfs->process;
  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  process_in = ts->running_process[cpu];
  /* It should exist, because we are after the state update. */
#ifdef EXTRA_CHECK
  g_assert(process_in != NULL);
#endif //EXTRA_CHECK
  birth = process_in->creation_time;

  //hashed_process_data_in = processlist_get_process_data(process_list, cpuq, trace_num);
  hashed_process_data_in = resourcelist_obtain_cpu(resourceview_data, trace_num, cpu);

  /* Set the current process */
  process_list->current_hash_data[trace_num][process_in->cpu] =
                                             hashed_process_data_in;

  if(ltt_time_compare(hashed_process_data_in->next_good_time,
                          evtime) <= 0)
  {
    TimeWindow time_window = 
    lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
        || ltt_time_compare(evtime, time_window.end_time) == 1)
            return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint new_x;
    
    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &new_x);

    if(hashed_process_data_in->x.middle != new_x) {
      hashed_process_data_in->x.middle = new_x;
      hashed_process_data_in->x.middle_used = FALSE;
      hashed_process_data_in->x.middle_marked = FALSE;
    }
  }
  return 0;
}

int before_execmode_hook_irq(void *hook_data, void *call_data);
int before_execmode_hook_soft_irq(void *hook_data, void *call_data);
int before_execmode_hook_trap(void *hook_data, void *call_data);

/* before_execmode_hook
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

int before_execmode_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttTime evtime = ltt_event_time(e);

  before_execmode_hook_irq(hook_data, call_data);
  before_execmode_hook_soft_irq(hook_data, call_data);
  before_execmode_hook_trap(hook_data, call_data);

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */
  guint cpu = tfs->cpu;

  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];
  g_assert(process != NULL);

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;

  LttTime birth = process->creation_time;
 
  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
      hashed_process_data = resourcelist_obtain_cpu(resourceview_data, trace_num, cpu);

    /* Set the current process */
    process_list->current_hash_data[trace_num][process->cpu] =
                                               hashed_process_data;
  }

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */

  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  }
  else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;

    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &x);


    /* Jump over draw if we are at the same x position */
    if(unlikely(x == hashed_process_data->x.middle &&
             hashed_process_data->x.middle_used))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    }
    else {

      DrawContext draw_context;
      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        cpu_set_line_color(&prop_line, tfs->cpu_state);
        draw_line((void*)&prop_line, (void*)&draw_context);
      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }
  
  return 0;
}

int before_execmode_hook_irq(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  struct marker_info *minfo;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttTime evtime = ltt_event_time(e);

  LttTrace *trace = tfc->t_context->t;

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */

  guint64 irq;
  guint cpu = tfs->cpu;

  /*
   * Check for LTT_CHANNEL_KERNEL channel name and event ID
   * corresponding to LTT_EVENT_IRQ_ENTRY or LTT_EVENT_IRQ_EXIT.
   */
  if (tfc->tf->name != LTT_CHANNEL_KERNEL)
    return 0;
  minfo = marker_get_info_from_id(tfc->tf->mdata, e->event_id);
  g_assert(minfo != NULL);
  if (minfo->name == LTT_EVENT_IRQ_ENTRY) {
    irq = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  } else if (minfo->name == LTT_EVENT_IRQ_EXIT) {
    irq = ts->cpu_states[cpu].last_irq;
  } else
    return 0;

  guint trace_num = ts->parent.index;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;

  hashed_process_data = resourcelist_obtain_irq(resourceview_data, trace_num, irq);
  // TODO: fix this, it's ugly and slow:
  GQuark name;
  {
    gchar *str;
    str = g_strdup_printf("IRQ %" PRIu64 " [%s]", irq, (char*)g_quark_to_string(ts->irq_names[irq]));
    name = g_quark_from_string(str);
    g_free(str);
  }
  gtk_tree_store_set(resourceview_data->process_list->list_store, &hashed_process_data->y_iter, NAME_COLUMN, g_quark_to_string(name), -1);

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */

  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  }
  else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;

    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &x);


    /* Jump over draw if we are at the same x position */
    if(unlikely(x == hashed_process_data->x.middle &&
             hashed_process_data->x.middle_used))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    }
    else {

      DrawContext draw_context;
      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        irq_set_line_color(&prop_line, &ts->irq_states[irq]);
        draw_line((void*)&prop_line, (void*)&draw_context);
      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }
  
  return 0;
}

int before_execmode_hook_soft_irq(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  struct marker_info *minfo;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttTime evtime = ltt_event_time(e);

  LttTrace *trace = tfc->t_context->t;

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */

  guint64 softirq;
  guint cpu = tfs->cpu;

  /*
   * Check for LTT_CHANNEL_KERNEL channel name and event ID
   * corresponding to LTT_EVENT_SOFT_IRQ_RAISE, LTT_EVENT_SOFT_IRQ_ENTRY
   * or LTT_EVENT_SOFT_IRQ_EXIT.
   */
  if (tfc->tf->name != LTT_CHANNEL_KERNEL)
    return 0;
  minfo = marker_get_info_from_id(tfc->tf->mdata, e->event_id);
  g_assert(minfo != NULL);
  if (minfo->name == LTT_EVENT_SOFT_IRQ_RAISE
      || minfo->name == LTT_EVENT_SOFT_IRQ_ENTRY) {
    softirq = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  } else if (minfo->name == LTT_EVENT_SOFT_IRQ_EXIT) {
    softirq = ts->cpu_states[cpu].last_soft_irq;
  } else
    return 0;

  guint trace_num = ts->parent.index;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;

  hashed_process_data = resourcelist_obtain_soft_irq(resourceview_data, trace_num, softirq);

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */

  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  }
  else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;

    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &x);


    /* Jump over draw if we are at the same x position */
    if(unlikely(x == hashed_process_data->x.middle &&
             hashed_process_data->x.middle_used))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    }
    else {

      DrawContext draw_context;
      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        soft_irq_set_line_color(&prop_line, &ts->soft_irq_states[softirq]);
        draw_line((void*)&prop_line, (void*)&draw_context);
      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }
  
  return 0;
}

int before_execmode_hook_trap(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;
  struct marker_info *minfo;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttTime evtime = ltt_event_time(e);

  LttTrace *trace = tfc->t_context->t;

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */

  guint64 trap;
  guint cpu = tfs->cpu;

  /*
   * Check for LTT_CHANNEL_KERNEL channel name and event ID
   * corresponding to LTT_EVENT_TRAP/PAGE_FAULT_ENTRY or
   * LTT_EVENT_TRAP/PAGE_FAULT_EXIT.
   */
  if (tfc->tf->name != LTT_CHANNEL_KERNEL)
    return 0;
  minfo = marker_get_info_from_id(tfc->tf->mdata, e->event_id);
  g_assert(minfo != NULL);
  if (minfo->name == LTT_EVENT_TRAP_ENTRY
      || minfo->name == LTT_EVENT_PAGE_FAULT_ENTRY
      || minfo->name == LTT_EVENT_PAGE_FAULT_NOSEM_ENTRY) {
    trap = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  } else if (minfo->name == LTT_EVENT_TRAP_EXIT
             || minfo->name == LTT_EVENT_PAGE_FAULT_EXIT
             || minfo->name == LTT_EVENT_PAGE_FAULT_NOSEM_EXIT) {
    trap = ts->cpu_states[cpu].last_trap;
  } else
    return 0;

  guint trace_num = ts->parent.index;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;

  hashed_process_data = resourcelist_obtain_trap(resourceview_data, trace_num, trap);

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */

  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  }
  else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;

    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &x);


    /* Jump over draw if we are at the same x position */
    if(unlikely(x == hashed_process_data->x.middle &&
             hashed_process_data->x.middle_used))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    }
    else {

      DrawContext draw_context;
      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        trap_set_line_color(&prop_line, &ts->trap_states[trap]);
        draw_line((void*)&prop_line, (void*)&draw_context);
      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }
  
  return 0;
}


int before_bdev_event_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttTime evtime = ltt_event_time(e);

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */

  guint cpu = tfs->cpu;
  guint8 major = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  guint8 minor = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
  guint oper = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 2));
  gint devcode_gint = MKDEV(major,minor);

  guint trace_num = ts->parent.index;

  LttvBdevState *bdev = g_hash_table_lookup(ts->bdev_states, &devcode_gint); 
  /* the result of the lookup might be NULL. that's ok, the rest of the function
     should understand it was not found and that its state is unknown */

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedResourceData *hashed_process_data = NULL;
  ProcessList *process_list = resourceview_data->process_list;
//  LttTime birth = process->creation_time;
 
//  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
//    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
//  } else {
  hashed_process_data = resourcelist_obtain_bdev(resourceview_data, trace_num, devcode_gint);
    ////hashed_process_data = processlist_get_process_data(process_list, resourceq, trace_num);
//    hashed_process_data = processlist_get_process_data(process_list,
//            pid,
//            process->cpu,
//            &birth,
//            trace_num);
//
    /* Set the current process */
//    process_list->current_hash_data[trace_num][process->cpu] =
//                                               hashed_process_data;
//  }

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */

  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;
      guint x;
      convert_time_to_pixels(
                time_window,
                evtime,
                width,
                &x);

      /* Draw collision indicator */
      gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
      gdk_draw_point(hashed_process_data->pixmap,
                     drawing->gc,
                     x,
                     COLLISION_POSITION(hashed_process_data->height));
      hashed_process_data->x.middle_marked = TRUE;
    }
  }
  else {
    TimeWindow time_window = 
      lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return;
#endif //EXTRA_CHECK
    Drawing_t *drawing = resourceview_data->drawing;
    guint width = drawing->width;
    guint x;

    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &x);


    /* Jump over draw if we are at the same x position */
    if(unlikely(x == hashed_process_data->x.middle &&
             hashed_process_data->x.middle_used))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(hashed_process_data->pixmap,
                       drawing->gc,
                       x,
                       COLLISION_POSITION(hashed_process_data->height));
        hashed_process_data->x.middle_marked = TRUE;
      }
      /* jump */
    }
    else {

      DrawContext draw_context;
      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.start.x = hashed_process_data->x.middle;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;

      {
        /* Draw the line */
        PropertiesLine prop_line;
        prop_line.line_width = STATE_LINE_WIDTH;
        prop_line.style = GDK_LINE_SOLID;
        prop_line.y = MIDDLE;
        bdev_set_line_color(&prop_line, bdev);
        draw_line((void*)&prop_line, (void*)&draw_context);
      }
      /* become the last x position */
      hashed_process_data->x.middle = x;
      hashed_process_data->x.middle_used = TRUE;
      hashed_process_data->x.middle_marked = FALSE;

      /* Calculate the next good time */
      convert_pixels_to_time(width, x+1, time_window,
                             &hashed_process_data->next_good_time);
    }
  }
  
  return 0;
}

gint update_time_window_hook(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = resourceview_data->drawing;
  ProcessList *process_list = resourceview_data->process_list;

  const TimeWindowNotifyData *time_window_nofify_data = 
                          ((const TimeWindowNotifyData *)call_data);

  TimeWindow *old_time_window = 
    time_window_nofify_data->old_time_window;
  TimeWindow *new_time_window = 
    time_window_nofify_data->new_time_window;
  
  /* Update the ruler */
  drawing_update_ruler(resourceview_data->drawing,
                       new_time_window);


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
  && new_time_window->time_width.tv_nsec == old_time_window->time_width.tv_nsec)
  {
    /* Same scale (scrolling) */
    g_info("scrolling");
    LttTime *ns = &new_time_window->start_time;
    LttTime *nw = &new_time_window->time_width;
    LttTime *os = &old_time_window->start_time;
    LttTime *ow = &old_time_window->time_width;
    LttTime old_end = old_time_window->end_time;
    LttTime new_end = new_time_window->end_time;
    //if(ns<os+w<ns+w)
    //if(ns<os+w && os+w<ns+w)
    //if(ns<old_end && os<ns)
    if(ltt_time_compare(*ns, old_end) == -1
        && ltt_time_compare(*os, *ns) == -1)
    {
      g_info("scrolling near right");
      /* Scroll right, keep right part of the screen */
      guint x = 0;
      guint width = resourceview_data->drawing->width;
      convert_time_to_pixels(
          *old_time_window,
          *ns,
          width,
          &x);

      /* Copy old data to new location */
      copy_pixmap_region(process_list,
                  NULL,
                  resourceview_data->drawing->drawing_area->style->black_gc,
                  NULL,
                  x, 0,
                  0, 0,
                  resourceview_data->drawing->width-x+SAFETY, -1);

      if(drawing->damage_begin == drawing->damage_end)
        drawing->damage_begin = resourceview_data->drawing->width-x;
      else
        drawing->damage_begin = 0;

      drawing->damage_end = resourceview_data->drawing->width;

      /* Clear the data request background, but not SAFETY */
      rectangle_pixmap(process_list,
          resourceview_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin+SAFETY, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1);
      gtk_widget_queue_draw(drawing->drawing_area);

      /* Get new data for the rest. */
      drawing_data_request(resourceview_data->drawing,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,
          resourceview_data->drawing->height);
    } else { 
      if(ltt_time_compare(*ns,*os) == -1
          && ltt_time_compare(*os,new_end) == -1)
      {
        g_info("scrolling near left");
        /* Scroll left, keep left part of the screen */
        guint x = 0;
        guint width = resourceview_data->drawing->width;
        convert_time_to_pixels(
            *new_time_window,
            *os,
            width,
            &x);
        
        /* Copy old data to new location */
        copy_pixmap_region  (process_list,
            NULL,
            resourceview_data->drawing->drawing_area->style->black_gc,
            NULL,
            0, 0,
            x, 0,
            -1, -1);
  
        if(drawing->damage_begin == drawing->damage_end)
          drawing->damage_end = x;
        else
          drawing->damage_end = 
            resourceview_data->drawing->width;

        drawing->damage_begin = 0;
        
        rectangle_pixmap (process_list,
          resourceview_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1);

        gtk_widget_queue_draw(drawing->drawing_area);

        /* Get new data for the rest. */
        drawing_data_request(resourceview_data->drawing,
            drawing->damage_begin, 0,
            drawing->damage_end - drawing->damage_begin,
            resourceview_data->drawing->height);
    
      } else {
        if(ltt_time_compare(*ns,*os) == 0)
        {
          g_info("not scrolling");
        } else {
          g_info("scrolling far");
          /* Cannot reuse any part of the screen : far jump */
          
          
          rectangle_pixmap (process_list,
            resourceview_data->drawing->drawing_area->style->black_gc,
            TRUE,
            0, 0,
            resourceview_data->drawing->width+SAFETY, // do not overlap
            -1);

          gtk_widget_queue_draw(drawing->drawing_area);

          drawing->damage_begin = 0;
          drawing->damage_end = resourceview_data->drawing->width;

          drawing_data_request(resourceview_data->drawing,
              0, 0,
              resourceview_data->drawing->width,
              resourceview_data->drawing->height);
      
        }
      }
    }
  } else {
    /* Different scale (zoom) */
    g_info("zoom");

    rectangle_pixmap (process_list,
          resourceview_data->drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          resourceview_data->drawing->width+SAFETY, // do not overlap
          -1);

    gtk_widget_queue_draw(drawing->drawing_area);
  
    drawing->damage_begin = 0;
    drawing->damage_end = resourceview_data->drawing->width;

    drawing_data_request(resourceview_data->drawing,
        0, 0,
        resourceview_data->drawing->width,
        resourceview_data->drawing->height);
  }

  /* Update directly when scrolling */
  gdk_window_process_updates(resourceview_data->drawing->drawing_area->window,
      TRUE);

  return 0;
}

gint traceset_notify(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = resourceview_data->drawing;

  if(unlikely(drawing->gc == NULL)) {
    return FALSE;
  }
  if(drawing->dotted_gc == NULL) {
    return FALSE;
  }

  drawing_clear(resourceview_data->drawing);
  processlist_clear(resourceview_data->process_list);
  gtk_widget_set_size_request(
      resourceview_data->drawing->drawing_area,
                -1, processlist_get_height(resourceview_data->process_list));
  redraw_notify(resourceview_data, NULL);

  request_background_data(resourceview_data);
 
  return FALSE;
}

gint redraw_notify(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = resourceview_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  drawing->damage_begin = 0;
  drawing->damage_end = drawing->width;

  /* fun feature, to be separated someday... */
  drawing_clear(resourceview_data->drawing);
  processlist_clear(resourceview_data->process_list);
  gtk_widget_set_size_request(
      resourceview_data->drawing->drawing_area,
                -1, processlist_get_height(resourceview_data->process_list));
  // Clear the images
  rectangle_pixmap (resourceview_data->process_list,
        widget->style->black_gc,
        TRUE,
        0, 0,
        drawing->alloc_width,
        -1);

  gtk_widget_queue_draw(drawing->drawing_area);
  
  if(drawing->damage_begin < drawing->damage_end)
  {
    drawing_data_request(drawing,
                         drawing->damage_begin,
                         0,
                         drawing->damage_end-drawing->damage_begin,
                         drawing->height);
  }

  return FALSE;

}


gint continue_notify(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = resourceview_data->drawing;

  if(drawing->damage_begin < drawing->damage_end)
  {
    drawing_data_request(drawing,
                         drawing->damage_begin,
                         0,
                         drawing->damage_end-drawing->damage_begin,
                         drawing->height);
  }

  return FALSE;
}


gint update_current_time_hook(void *hook_data, void *call_data)
{
  ControlFlowData *resourceview_data = (ControlFlowData*)hook_data;
  Drawing_t *drawing = resourceview_data->drawing;

  LttTime current_time = *((LttTime*)call_data);
  
  TimeWindow time_window =
            lttvwindow_get_time_window(resourceview_data->tab);
  
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
        lttvwindow_get_traceset_context(resourceview_data->tab);
  
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
  
  
  if(ltt_time_compare(current_time, time_begin) < 0)
  {
    TimeWindow new_time_window;

    if(ltt_time_compare(current_time,
          ltt_time_add(trace_start,half_width)) < 0)
      time_begin = trace_start;
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;
    new_time_window.time_width_double = ltt_time_to_double(width);
    new_time_window.end_time = ltt_time_add(time_begin, width);

    lttvwindow_report_time_window(resourceview_data->tab, new_time_window);
  }
  else if(ltt_time_compare(current_time, time_end) > 0)
  {
    TimeWindow new_time_window;

    if(ltt_time_compare(current_time, ltt_time_sub(trace_end, half_width)) > 0)
      time_begin = ltt_time_sub(trace_end,width);
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;
    new_time_window.time_width_double = ltt_time_to_double(width);
    new_time_window.end_time = ltt_time_add(time_begin, width);

    lttvwindow_report_time_window(resourceview_data->tab, new_time_window);
    
  }
  gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
  
  /* Update directly when scrolling */
  gdk_window_process_updates(resourceview_data->drawing->drawing_area->window,
      TRUE);
                             
  return 0;
}

typedef struct _ClosureData {
  EventsRequest *events_request;
  LttvTracesetState *tss;
  LttTime end_time;
  guint x_end;
} ClosureData;
  
/* Draw line until end of the screen */

void draw_closure(gpointer key, gpointer value, gpointer user_data)
{
  ResourceUniqueNumeric *process_info = (ResourceUniqueNumeric*)key;
  HashedResourceData *hashed_process_data = (HashedResourceData*)value;
  ClosureData *closure_data = (ClosureData*)user_data;
    
  EventsRequest *events_request = closure_data->events_request;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracesetState *tss = closure_data->tss;
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;

  LttTime evtime = closure_data->end_time;

  gboolean dodraw = TRUE;

  if(hashed_process_data->type == RV_RESOURCE_MACHINE)
	return;

  { 
    /* For the process */
    /* First, check if the current process is in the state computation
     * process list. If it is there, that means we must add it right now and
     * draw items from the beginning of the read for it. If it is not
     * present, it's a new process and it was not present : it will
     * be added after the state update.  */
#ifdef EXTRA_CHECK
    g_assert(lttv_traceset_number(tsc->ts) > 0);
#endif //EXTRA_CHECK
    LttvTraceContext *tc = tsc->traces[process_info->trace_num];
    LttvTraceState *ts = (LttvTraceState*)tc;

    /* Only draw for processes that are currently in the trace states */

    ProcessList *process_list = resourceview_data->process_list;
#ifdef EXTRA_CHECK
    /* Should be alike when background info is ready */
    if(resourceview_data->background_info_waiting==0)
        g_assert(ltt_time_compare(process->creation_time,
                                  process_info->birth) == 0);
#endif //EXTRA_CHECK
    
    /* Now, the process is in the state hash and our own process hash.
     * We definitely can draw the items related to the ending state.
     */
    
    if(unlikely(ltt_time_compare(hashed_process_data->next_good_time,
                          evtime) <= 0))
    {
      TimeWindow time_window = 
        lttvwindow_get_time_window(resourceview_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return;
#endif //EXTRA_CHECK
      Drawing_t *drawing = resourceview_data->drawing;
      guint width = drawing->width;

      guint x = closure_data->x_end;

      DrawContext draw_context;

      /* Now create the drawing context that will be used to draw
       * items related to the last state. */
      draw_context.drawable = hashed_process_data->pixmap;
      draw_context.gc = drawing->gc;
      draw_context.pango_layout = drawing->pango_layout;
      draw_context.drawinfo.end.x = x;

      draw_context.drawinfo.y.over = 1;
      draw_context.drawinfo.y.middle = (hashed_process_data->height/2);
      draw_context.drawinfo.y.under = hashed_process_data->height;

      draw_context.drawinfo.start.offset.over = 0;
      draw_context.drawinfo.start.offset.middle = 0;
      draw_context.drawinfo.start.offset.under = 0;
      draw_context.drawinfo.end.offset.over = 0;
      draw_context.drawinfo.end.offset.middle = 0;
      draw_context.drawinfo.end.offset.under = 0;
#if 0
      /* Jump over draw if we are at the same x position */
      if(x == hashed_process_data->x.over)
      {
        /* jump */
      } else {
        draw_context.drawinfo.start.x = hashed_process_data->x.over;
        /* Draw the line */
        PropertiesLine prop_line = prepare_execmode_line(process);
        draw_line((void*)&prop_line, (void*)&draw_context);

        hashed_process_data->x.over = x;
      }
#endif //0

      if(unlikely(x == hashed_process_data->x.middle &&
          hashed_process_data->x.middle_used)) {
#if 0 /* do not mark closure : not missing information */
      if(hashed_process_data->x.middle_marked == FALSE) {
        /* Draw collision indicator */
        gdk_gc_set_foreground(drawing->gc, &drawing_colors[COL_WHITE]);
        gdk_draw_point(drawing->pixmap,
                       drawing->gc,
                       x,
                       y+(height/2)-3);
        hashed_process_data->x.middle_marked = TRUE;
      }
#endif //0
        /* Jump */
      } else {
        draw_context.drawinfo.start.x = hashed_process_data->x.middle;
        /* Draw the line */
        if(dodraw) {
          PropertiesLine prop_line;
          prop_line.line_width = STATE_LINE_WIDTH;
          prop_line.style = GDK_LINE_SOLID;
          prop_line.y = MIDDLE;
          if(hashed_process_data->type == RV_RESOURCE_CPU)
            cpu_set_line_color(&prop_line, &ts->cpu_states[process_info->id]);
          else if(hashed_process_data->type == RV_RESOURCE_IRQ)
            irq_set_line_color(&prop_line, &ts->irq_states[process_info->id]);
          else if(hashed_process_data->type == RV_RESOURCE_SOFT_IRQ)
            soft_irq_set_line_color(&prop_line, &ts->soft_irq_states[process_info->id]);
          else if(hashed_process_data->type == RV_RESOURCE_TRAP)
            trap_set_line_color(&prop_line, &ts->trap_states[process_info->id]);
          else if(hashed_process_data->type == RV_RESOURCE_BDEV) {
            gint devcode_gint = process_info->id;
            LttvBdevState *bdev = g_hash_table_lookup(ts->bdev_states, &devcode_gint);
            // the lookup may return null; bdev_set_line_color must act appropriately
            bdev_set_line_color(&prop_line, bdev);
          }
          
          draw_line((void*)&prop_line, (void*)&draw_context);
        }

         /* become the last x position */
        if(likely(x != hashed_process_data->x.middle)) {
          hashed_process_data->x.middle = x;
          /* but don't use the pixel */
          hashed_process_data->x.middle_used = FALSE;

          /* Calculate the next good time */
          convert_pixels_to_time(width, x+1, time_window,
                                &hashed_process_data->next_good_time);
        }
      }
    }
  }
}

int before_chunk(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  ControlFlowData *cfd = (ControlFlowData*)events_request->viewer_data;
#if 0  
  /* Deactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(cfd->process_list->list_store),
      TRACE_COLUMN,
      GTK_SORT_ASCENDING);
#endif //0
  drawing_chunk_begin(events_request, tss);

  return 0;
}

/* before_request
 *
 * This gets executed just before an events request is executed
 */

int before_request(void *hook_data, void *call_data)
{
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
int after_request(void *hook_data, void *call_data)
{
  guint i;
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  
  ProcessList *process_list = resourceview_data->process_list;
  LttTime end_time = events_request->end_time;

  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;
  closure_data.end_time = end_time;

  TimeWindow time_window = 
          lttvwindow_get_time_window(resourceview_data->tab);
  guint width = resourceview_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            end_time,
            width,
            &closure_data.x_end);


  /* Draw last items */
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(resourcelist_get_resource_hash_table(resourceview_data, i), draw_closure,
                        (void*)&closure_data);
  }
  
  /* Request expose */
  drawing_request_expose(events_request, tss, end_time);
  return 0;
}

/*
 * for each process
 *    draw closing line
 * expose
 */
int after_chunk(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  LttvTracesetContext *tsc = (LttvTracesetContext*)call_data;
  LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
  LttTime end_time;
  
  ProcessList *process_list = resourceview_data->process_list;
  guint i;
  LttvTraceset *traceset = tsc->ts;
  guint nb_trace = lttv_traceset_number(traceset);

  /* Only execute when called for the first trace's events request */
  if(!process_list->current_hash_data)
	  return 0;

  for(i = 0 ; i < nb_trace ; i++) {
    g_free(process_list->current_hash_data[i]);
  }
  g_free(process_list->current_hash_data);
  process_list->current_hash_data = NULL;

  if(tfc != NULL)
    end_time = LTT_TIME_MIN(tfc->timestamp, events_request->end_time);
  else /* end of traceset, or position now out of request : end */
    end_time = events_request->end_time;
  
  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;
  closure_data.end_time = end_time;

  TimeWindow time_window = 
          lttvwindow_get_time_window(resourceview_data->tab);
  guint width = resourceview_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            end_time,
            width,
            &closure_data.x_end);

  /* Draw last items */
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(resourcelist_get_resource_hash_table(resourceview_data, i), draw_closure,
                        (void*)&closure_data);
  }
#if 0
  /* Reactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(resourceview_data->process_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);

  update_index_to_pixmap(resourceview_data->process_list);
  /* Request a full expose : drawing scrambled */
  gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
#endif //0
  /* Request expose (updates damages zone also) */
  drawing_request_expose(events_request, tss, end_time);

  return 0;
}

/* after_statedump_end
 * 
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */
int before_statedump_end(void *hook_data, void *call_data)
{
  gint i;

  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *resourceview_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttvTracesetState *tss = (LttvTracesetState*)tfc->t_context->ts_context;
  ProcessList *process_list = resourceview_data->process_list;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = resourceview_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  ClosureData closure_data;
  closure_data.events_request = events_request;
  closure_data.tss = tss;
  closure_data.end_time = evtime;

  TimeWindow time_window = 
          lttvwindow_get_time_window(resourceview_data->tab);
  guint width = resourceview_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            evtime,
            width,
            &closure_data.x_end);

  /* Draw last items */
  
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(resourcelist_get_resource_hash_table(resourceview_data, i), draw_closure,
                        (void*)&closure_data);
  }
#if 0
  /* Reactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(resourceview_data->process_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);

  update_index_to_pixmap(resourceview_data->process_list);
  /* Request a full expose : drawing scrambled */
  gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
#endif //0
  /* Request expose (updates damages zone also) */
  drawing_request_expose(events_request, tss, evtime);

  return 0;
}
