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
  ControlFlowData *control_flow_data = (ControlFlowData *)hook_data;
  LttvTrace *trace = (LttvTrace*)call_data;

  control_flow_data->background_info_waiting--;
  
  if(control_flow_data->background_info_waiting == 0) {
    g_message("control flow viewer : background computation data ready.");

    drawing_clear(control_flow_data->drawing);
    processlist_clear(control_flow_data->process_list);
    gtk_widget_set_size_request(
      control_flow_data->drawing->drawing_area,
                -1, processlist_get_height(control_flow_data->process_list));
    redraw_notify(control_flow_data, NULL);
  }

  return 0;
}


/* Request background computation. Verify if it is in progress or ready first.
 * Only for each trace in the tab's traceset.
 */
static void request_background_data(ControlFlowData *control_flow_data)
{
  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(control_flow_data->tab);
  gint num_traces = lttv_traceset_number(tsc->ts);
  gint i;
  LttvTrace *trace;
  LttvTraceState *tstate;

  LttvHooks *background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, control_flow_data,
      LTTV_PRIO_DEFAULT);
  control_flow_data->background_info_waiting = 0;
  
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
              main_window_get_widget(control_flow_data->tab), trace, "state");
        lttvwindowtraces_background_notify_queue(control_flow_data,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 background_ready_hook);
        control_flow_data->background_info_waiting++;
      } else { /* in progress */
      
        lttvwindowtraces_background_notify_current(control_flow_data,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   background_ready_hook);
        control_flow_data->background_info_waiting++;
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
h_guicontrolflow(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  Tab *tab = ptab->tab;
  g_info("h_guicontrolflow, %p", tab);
  ControlFlowData *control_flow_data = guicontrolflow(ptab);
  
  control_flow_data->tab = tab;
  
  // Unreg done in the GuiControlFlow_Destructor
  lttvwindow_register_traceset_notify(tab,
        traceset_notify,
        control_flow_data);
    
  lttvwindow_register_time_window_notify(tab,
                                         update_time_window_hook,
                                         control_flow_data);
  lttvwindow_register_current_time_notify(tab,
                                          update_current_time_hook,
                                          control_flow_data);
  lttvwindow_register_redraw_notify(tab,
                                    redraw_notify,
                                    control_flow_data);
  lttvwindow_register_continue_notify(tab,
                                      continue_notify,
                                      control_flow_data);
  request_background_data(control_flow_data);
  

  return guicontrolflow_get_widget(control_flow_data) ;
  
}

int event_selected_hook(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  guint *event_number = (guint*) call_data;

  g_debug("DEBUG : event selected by main window : %u", *event_number);
  
  return 0;
}

/* Function that selects the color of status&exemode line */
static inline PropertiesLine prepare_s_e_line(LttvProcessState *process)
{
  PropertiesLine prop_line;
  prop_line.line_width = STATE_LINE_WIDTH;
  prop_line.style = GDK_LINE_SOLID;
  prop_line.y = MIDDLE;
  //GdkColormap *colormap = gdk_colormap_get_system();
  
  if(process->state->s == LTTV_STATE_RUN) {
    if(process->state->t == LTTV_STATE_USER_MODE)
      prop_line.color = drawing_colors[COL_RUN_USER_MODE];
    else if(process->state->t == LTTV_STATE_SYSCALL)
      prop_line.color = drawing_colors[COL_RUN_SYSCALL];
    else if(process->state->t == LTTV_STATE_TRAP)
      prop_line.color = drawing_colors[COL_RUN_TRAP];
    else if(process->state->t == LTTV_STATE_IRQ)
      prop_line.color = drawing_colors[COL_RUN_IRQ];
    else if(process->state->t == LTTV_STATE_SOFT_IRQ)
      prop_line.color = drawing_colors[COL_RUN_SOFT_IRQ];
    else if(process->state->t == LTTV_STATE_MODE_UNKNOWN)
      prop_line.color = drawing_colors[COL_MODE_UNKNOWN];
    else
      g_assert(FALSE);   /* RUNNING MODE UNKNOWN */
  } else if(process->state->s == LTTV_STATE_WAIT) {
    /* We don't show if we wait while in user mode, trap, irq or syscall */
    prop_line.color = drawing_colors[COL_WAIT];
  } else if(process->state->s == LTTV_STATE_WAIT_CPU) {
    /* We don't show if we wait for CPU while in user mode, trap, irq
     * or syscall */
    prop_line.color = drawing_colors[COL_WAIT_CPU];
  } else if(process->state->s == LTTV_STATE_ZOMBIE) {
    prop_line.color = drawing_colors[COL_ZOMBIE];
  } else if(process->state->s == LTTV_STATE_WAIT_FORK) {
    prop_line.color = drawing_colors[COL_WAIT_FORK];
  } else if(process->state->s == LTTV_STATE_EXIT) {
    prop_line.color = drawing_colors[COL_EXIT];
  } else if(process->state->s == LTTV_STATE_UNNAMED) {
    prop_line.color = drawing_colors[COL_UNNAMED];
  } else if(process->state->s == LTTV_STATE_DEAD) {
    prop_line.color = drawing_colors[COL_DEAD];
  } else {
		g_critical("unknown state : %s", g_quark_to_string(process->state->s));
    g_assert(FALSE);   /* UNKNOWN STATE */
	}
  
  return prop_line;

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
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);
  gint target_pid_saved = tfc->target_pid;

  LttTime evtime = ltt_event_time(e);
  LttvFilter *filter = control_flow_data->filter;

  /* we are in a schedchange, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */

  guint pid_out;
  guint pid_in;
  guint state_out;
  {
    pid_out = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
    pid_in = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
    state_out = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 2));
  }
  
  tfc->target_pid = pid_out;
  if(!filter || !filter->head ||
    lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL)) { 
    /* For the pid_out */
    /* First, check if the current process is in the state computation
     * process list. If it is there, that means we must add it right now and
     * draw items from the beginning of the read for it. If it is not
     * present, it's a new process and it was not present : it will
     * be added after the state update.  */
    guint cpu = tfs->cpu;
    guint trace_num = ts->parent.index;
    LttvProcessState *process = ts->running_process[cpu];
    /* unknown state, bad current pid */
    if(process->pid != pid_out)
      process = lttv_state_find_process(ts,
          tfs->cpu, pid_out);
    
    if(process != NULL) {
      /* Well, the process_out existed : we must get it in the process hash
       * or add it, and draw its items.
       */
       /* Add process to process list (if not present) */
      guint pl_height = 0;
      HashedProcessData *hashed_process_data = NULL;
      ProcessList *process_list = control_flow_data->process_list;
      LttTime birth = process->creation_time;
      
      hashed_process_data = processlist_get_process_data(process_list,
              pid_out,
              process->cpu,
              &birth,
              trace_num);
      if(hashed_process_data == NULL)
      {
        g_assert(pid_out == 0 || pid_out != process->ppid);
        /* Process not present */
        ProcessInfo *process_info;
        Drawing_t *drawing = control_flow_data->drawing;
        processlist_add(process_list,
            drawing,
            pid_out,
            process->tgid,
            process->cpu,
            process->ppid,
            &birth,
            trace_num,
            process->name,
            process->brand,
            &pl_height,
            &process_info,
            &hashed_process_data);
        gtk_widget_set_size_request(drawing->drawing_area,
                                    -1,
                                    pl_height);
        gtk_widget_queue_draw(drawing->drawing_area);

      }
  
      /* Now, the process is in the state hash and our own process hash.
       * We definitely can draw the items related to the ending state.
       */
      
      if(ltt_time_compare(hashed_process_data->next_good_time,
                          evtime) > 0)
      {
        if(hashed_process_data->x.middle_marked == FALSE) {
    
          TimeWindow time_window = 
            lttvwindow_get_time_window(control_flow_data->tab);
#ifdef EXTRA_CHECK
          if(ltt_time_compare(evtime, time_window.start_time) == -1
                || ltt_time_compare(evtime, time_window.end_time) == 1)
                    return FALSE;
#endif //EXTRA_CHECK
          Drawing_t *drawing = control_flow_data->drawing;
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
          lttvwindow_get_time_window(control_flow_data->tab);
#ifdef EXTRA_CHECK
        if(ltt_time_compare(evtime, time_window.start_time) == -1
              || ltt_time_compare(evtime, time_window.end_time) == 1)
                  return FALSE;
#endif //EXTRA_CHECK
        Drawing_t *drawing = control_flow_data->drawing;
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
            PropertiesLine prop_line = prepare_s_e_line(process);
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
    }
  }

  tfc->target_pid = pid_in;
  if(!filter || !filter->head ||
    lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL)) { 
    /* For the pid_in */
    /* First, check if the current process is in the state computation
     * process list. If it is there, that means we must add it right now and
     * draw items from the beginning of the read for it. If it is not
     * present, it's a new process and it was not present : it will
     * be added after the state update.  */
    LttvProcessState *process;
    process = lttv_state_find_process(ts,
        tfs->cpu, pid_in);
    guint trace_num = ts->parent.index;
    
    if(process != NULL) {
      /* Well, the process existed : we must get it in the process hash
       * or add it, and draw its items.
       */
       /* Add process to process list (if not present) */
      guint pl_height = 0;
      HashedProcessData *hashed_process_data = NULL;
      ProcessList *process_list = control_flow_data->process_list;
      LttTime birth = process->creation_time;
      
      hashed_process_data = processlist_get_process_data(process_list,
              pid_in,
              tfs->cpu,
              &birth,
              trace_num);
      if(hashed_process_data == NULL)
      {
        g_assert(pid_in == 0 || pid_in != process->ppid);
        /* Process not present */
        ProcessInfo *process_info;
        Drawing_t *drawing = control_flow_data->drawing;
        processlist_add(process_list,
            drawing,
            pid_in,
            process->tgid,
            tfs->cpu,
            process->ppid,
            &birth,
            trace_num,
            process->name,
            process->brand,
            &pl_height,
            &process_info,
            &hashed_process_data);
        gtk_widget_set_size_request(drawing->drawing_area,
                                    -1,
                                    pl_height);
        gtk_widget_queue_draw(drawing->drawing_area);

      }
      //We could set the current process and hash here, but will be done
      //by after schedchange hook
    
      /* Now, the process is in the state hash and our own process hash.
       * We definitely can draw the items related to the ending state.
       */
      
      if(ltt_time_compare(hashed_process_data->next_good_time,
                          evtime) > 0)
      {
        if(hashed_process_data->x.middle_marked == FALSE) {

          TimeWindow time_window = 
            lttvwindow_get_time_window(control_flow_data->tab);
#ifdef EXTRA_CHECK
          if(ltt_time_compare(evtime, time_window.start_time) == -1
                || ltt_time_compare(evtime, time_window.end_time) == 1)
                    return FALSE;
#endif //EXTRA_CHECK
          Drawing_t *drawing = control_flow_data->drawing;
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
          lttvwindow_get_time_window(control_flow_data->tab);
#ifdef EXTRA_CHECK
        if(ltt_time_compare(evtime, time_window.start_time) == -1
              || ltt_time_compare(evtime, time_window.end_time) == 1)
                  return FALSE;
#endif //EXTRA_CHECK
        Drawing_t *drawing = control_flow_data->drawing;
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
            PropertiesLine prop_line = prepare_s_e_line(process);
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
    } else
      g_warning("Cannot find pin_in in schedchange %u", pid_in);
  }
  tfc->target_pid = target_pid_saved;
  return 0;




  /* Text dump */
#ifdef DONTSHOW
  GString *string = g_string_new("");;
  gboolean field_names = TRUE, state = TRUE;

  lttv_event_to_string(e, tfc->tf, string, TRUE, field_names, tfs);
  g_string_append_printf(string,"\n");  

  if(state) {
    g_string_append_printf(string, " %s",
        g_quark_to_string(tfs->process->state->s));
  }

  g_info("%s",string->str);

  g_string_free(string, TRUE);
  
  /* End of text dump */
#endif //DONTSHOW

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
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  LttTime evtime = ltt_event_time(e);

  /* Add process to process list (if not present) */
  LttvProcessState *process_in;
  LttTime birth;
  guint pl_height = 0;
  HashedProcessData *hashed_process_data_in = NULL;

  ProcessList *process_list = control_flow_data->process_list;
  
  guint pid_in;
  {
    guint pid_out;
    pid_out = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
    pid_in = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
  }

  tfc->target_pid = pid_in;
  if(!filter || !filter->head ||
    lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL)) { 
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
  
    hashed_process_data_in = processlist_get_process_data(process_list,
            pid_in,
            process_in->cpu,
            &birth,
            trace_num);
    if(hashed_process_data_in == NULL)
    {
      g_assert(pid_in == 0 || pid_in != process_in->ppid);
      ProcessInfo *process_info;
      Drawing_t *drawing = control_flow_data->drawing;
      /* Process not present */
      processlist_add(process_list,
          drawing,
          pid_in,
          process_in->tgid,
          process_in->cpu,
          process_in->ppid,
          &birth,
          trace_num,
          process_in->name,
          process_in->brand,
          &pl_height,
          &process_info,
          &hashed_process_data_in);
          gtk_widget_set_size_request(drawing->drawing_area,
                                      -1,
                                      pl_height);
          gtk_widget_queue_draw(drawing->drawing_area);
    }
    /* Set the current process */
    process_list->current_hash_data[trace_num][process_in->cpu] =
                                               hashed_process_data_in;
  
    if(ltt_time_compare(hashed_process_data_in->next_good_time,
                            evtime) <= 0)
    {
      TimeWindow time_window = 
      lttvwindow_get_time_window(control_flow_data->tab);
  
#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return FALSE;
#endif //EXTRA_CHECK
      Drawing_t *drawing = control_flow_data->drawing;
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
  }

  return 0;
}




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
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  /* we are in a execmode, before the state update. We must draw the
   * items corresponding to the state before it changes : now is the right
   * time to do it.
   */
  /* For the pid */
  //LttvProcessState *process = tfs->process;
  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];
  g_assert(process != NULL);

  guint pid = process->pid;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedProcessData *hashed_process_data = NULL;
  ProcessList *process_list = control_flow_data->process_list;
  LttTime birth = process->creation_time;
 
  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
    hashed_process_data = processlist_get_process_data(process_list,
            pid,
            process->cpu,
            &birth,
            trace_num);
    if(unlikely(hashed_process_data == NULL))
    {
      g_assert(pid == 0 || pid != process->ppid);
      ProcessInfo *process_info;
      /* Process not present */
      Drawing_t *drawing = control_flow_data->drawing;
      processlist_add(process_list,
          drawing,
          pid,
          process->tgid,
          process->cpu,
          process->ppid,
          &birth,
          trace_num,
          process->name,
          process->brand,
          &pl_height,
          &process_info,
          &hashed_process_data);
        gtk_widget_set_size_request(drawing->drawing_area,
                                    -1,
                                    pl_height);
        gtk_widget_queue_draw(drawing->drawing_area);
    }
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
        lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return FALSE;
#endif //EXTRA_CHECK
      Drawing_t *drawing = control_flow_data->drawing;
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
      lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return FALSE;
#endif //EXTRA_CHECK
    Drawing_t *drawing = control_flow_data->drawing;
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
        PropertiesLine prop_line = prepare_s_e_line(process);
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

/* before_process_exit_hook
 * 
 * Draw lines for process event.
 *
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */


int before_process_exit_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;

  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  /* Add process to process list (if not present) */
  //LttvProcessState *process = tfs->process;
  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];
  guint pid = process->pid;
  LttTime birth;
  guint pl_height = 0;
  HashedProcessData *hashed_process_data = NULL;

  ProcessList *process_list = control_flow_data->process_list;
  
  g_assert(process != NULL);

  birth = process->creation_time;

  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
    hashed_process_data = processlist_get_process_data(process_list,
          pid,
          process->cpu,
          &birth,
          trace_num);
    if(unlikely(hashed_process_data == NULL))
    {
      g_assert(pid == 0 || pid != process->ppid);
      /* Process not present */
      Drawing_t *drawing = control_flow_data->drawing;
      ProcessInfo *process_info;
      processlist_add(process_list,
          drawing,
          pid,
          process->tgid,
          process->cpu,
          process->ppid,
          &birth,
          trace_num,
          process->name,
          process->brand,
          &pl_height,
          &process_info,
          &hashed_process_data);
      gtk_widget_set_size_request(drawing->drawing_area,
                                  -1,
                                  pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }
  }

  /* Now, the process is in the state hash and our own process hash.
   * We definitely can draw the items related to the ending state.
   */
  
  if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                      evtime) > 0))
  {
    if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
      TimeWindow time_window = 
        lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return FALSE;
#endif //EXTRA_CHECK
      Drawing_t *drawing = control_flow_data->drawing;
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
      lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return FALSE;
#endif //EXTRA_CHECK
    Drawing_t *drawing = control_flow_data->drawing;
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
        PropertiesLine prop_line = prepare_s_e_line(process);
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



/* before_process_release_hook
 * 
 * Draw lines for process event.
 *
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */


int before_process_release_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;

  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  guint trace_num = ts->parent.index;

  guint pid;
  {
    pid = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  }

  /* Add process to process list (if not present) */
  /* Don't care about the process if it's not in the state hash already :
   * that means a process that has never done anything in the trace and
   * unknown suddently gets destroyed : no state meaningful to show. */
  LttvProcessState *process = lttv_state_find_process(ts, ANY_CPU, pid);

  if(process != NULL) {
    LttTime birth;
    guint pl_height = 0;
    HashedProcessData *hashed_process_data = NULL;

    ProcessList *process_list = control_flow_data->process_list;
    
    birth = process->creation_time;

    /* Cannot use current process : this event happens on another process,
     * action done by the parent. */
    hashed_process_data = processlist_get_process_data(process_list,
          pid,
          process->cpu,
          &birth,
          trace_num);
    if(unlikely(hashed_process_data == NULL))
      /*
       * Process already been scheduled out EXIT_DEAD, not in the process list
       * anymore. Just return.
       */
      return FALSE;

    /* Now, the process is in the state hash and our own process hash.
     * We definitely can draw the items related to the ending state.
     */
    
    if(likely(ltt_time_compare(hashed_process_data->next_good_time,
                        evtime) > 0))
    {
      if(unlikely(hashed_process_data->x.middle_marked == FALSE)) {
        TimeWindow time_window = 
          lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
        if(ltt_time_compare(evtime, time_window.start_time) == -1
              || ltt_time_compare(evtime, time_window.end_time) == 1)
                  return FALSE;
#endif //EXTRA_CHECK
        Drawing_t *drawing = control_flow_data->drawing;
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
        lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
      if(ltt_time_compare(evtime, time_window.start_time) == -1
            || ltt_time_compare(evtime, time_window.end_time) == 1)
                return FALSE;
#endif //EXTRA_CHECK
      Drawing_t *drawing = control_flow_data->drawing;
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
          PropertiesLine prop_line = prepare_s_e_line(process);
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
  }

  return 0;
}





/* after_process_fork_hook
 * 
 * Create the processlist entry for the child process. Put the last
 * position in x at the current time value.
 *
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */
int after_process_fork_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  guint child_pid;
  {
    child_pid = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 1));
  }

  /* Add process to process list (if not present) */
  LttvProcessState *process_child;
  LttTime birth;
  guint pl_height = 0;
  HashedProcessData *hashed_process_data_child = NULL;

  ProcessList *process_list = control_flow_data->process_list;

  /* Find child in the list... */
  process_child = lttv_state_find_process(ts, ANY_CPU, child_pid);
  /* It should exist, because we are after the state update. */
  g_assert(process_child != NULL);

  birth = process_child->creation_time;
  guint trace_num = ts->parent.index;

  /* Cannot use current process, because this action is done by the parent
   * on its child. */
  hashed_process_data_child = processlist_get_process_data(process_list,
          child_pid,
          process_child->cpu,
          &birth,
          trace_num);
  if(likely(hashed_process_data_child == NULL))
  {
    g_assert(child_pid == 0 || child_pid != process_child->ppid);
    /* Process not present */
    Drawing_t *drawing = control_flow_data->drawing;
    ProcessInfo *process_info;
    processlist_add(process_list,
        drawing,
        child_pid,
        process_child->tgid,
        process_child->cpu,
        process_child->ppid,
        &birth,
        trace_num,
        process_child->name,
        process_child->brand,
        &pl_height,
        &process_info,
        &hashed_process_data_child);
      gtk_widget_set_size_request(drawing->drawing_area,
                                  -1,
                                  pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
  } else {
          processlist_set_ppid(process_list, process_child->ppid,
                               hashed_process_data_child);
          processlist_set_tgid(process_list, process_child->tgid,
                               hashed_process_data_child);
  }


  if(likely(ltt_time_compare(hashed_process_data_child->next_good_time,
                        evtime) <= 0))
  {
    TimeWindow time_window = 
      lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return FALSE;
#endif //EXTRA_CHECK
    Drawing_t *drawing = control_flow_data->drawing;
    guint width = drawing->width;
    guint new_x;
    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &new_x);

    if(likely(hashed_process_data_child->x.over != new_x)) {
      hashed_process_data_child->x.over = new_x;
      hashed_process_data_child->x.over_used = FALSE;
      hashed_process_data_child->x.over_marked = FALSE;
    }
    if(likely(hashed_process_data_child->x.middle != new_x)) {
      hashed_process_data_child->x.middle = new_x;
      hashed_process_data_child->x.middle_used = FALSE;
      hashed_process_data_child->x.middle_marked = FALSE;
    }
    if(likely(hashed_process_data_child->x.under != new_x)) {
      hashed_process_data_child->x.under = new_x;
      hashed_process_data_child->x.under_used = FALSE;
      hashed_process_data_child->x.under_marked = FALSE;
    }
  }
  return FALSE;
}



/* after_process_exit_hook
 * 
 * Create the processlist entry for the child process. Put the last
 * position in x at the current time value.
 *
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */
int after_process_exit_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  /* Add process to process list (if not present) */
  //LttvProcessState *process = tfs->process;
  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];

  /* It should exist, because we are after the state update. */
  g_assert(process != NULL);

  guint pid = process->pid;
  LttTime birth;
  guint pl_height = 0;
  HashedProcessData *hashed_process_data = NULL;

  ProcessList *process_list = control_flow_data->process_list;

  birth = process->creation_time;

  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL) ){
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
    hashed_process_data = processlist_get_process_data(process_list,
            pid,
            process->cpu,
            &birth,
            trace_num);
    if(unlikely(hashed_process_data == NULL))
    {
      g_assert(pid == 0 || pid != process->ppid);
      /* Process not present */
      Drawing_t *drawing = control_flow_data->drawing;
      ProcessInfo *process_info;
      processlist_add(process_list,
          drawing,
          pid,
          process->tgid,
          process->cpu,
          process->ppid,
          &birth,
          trace_num,
          process->name,
          process->brand,
          &pl_height,
          &process_info,
          &hashed_process_data);
      gtk_widget_set_size_request(drawing->drawing_area,
                                  -1,
                                  pl_height);
      gtk_widget_queue_draw(drawing->drawing_area);
    }

    /* Set the current process */
    process_list->current_hash_data[trace_num][process->cpu] =
                                             hashed_process_data;
  }

  if(unlikely(ltt_time_compare(hashed_process_data->next_good_time,
                        evtime) <= 0))
  {
    TimeWindow time_window = 
      lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
    if(ltt_time_compare(evtime, time_window.start_time) == -1
          || ltt_time_compare(evtime, time_window.end_time) == 1)
              return FALSE;
#endif //EXTRA_CHECK
    Drawing_t *drawing = control_flow_data->drawing;
    guint width = drawing->width;
    guint new_x;
    convert_time_to_pixels(
        time_window,
        evtime,
        width,
        &new_x);
    if(unlikely(hashed_process_data->x.middle != new_x)) {
      hashed_process_data->x.middle = new_x;
      hashed_process_data->x.middle_used = FALSE;
      hashed_process_data->x.middle_marked = FALSE;
    }
  }

  return FALSE;
}


/* Get the filename of the process to print */
int after_fs_exec_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];
  g_assert(process != NULL);

  guint pid = process->pid;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedProcessData *hashed_process_data = NULL;
  ProcessList *process_list = control_flow_data->process_list;
  LttTime birth = process->creation_time;
 
  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
    hashed_process_data = processlist_get_process_data(process_list,
            pid,
            process->cpu,
            &birth,
            trace_num);
    if(unlikely(hashed_process_data == NULL))
    {
      g_assert(pid == 0 || pid != process->ppid);
      ProcessInfo *process_info;
      /* Process not present */
      Drawing_t *drawing = control_flow_data->drawing;
      processlist_add(process_list,
          drawing,
          pid,
          process->tgid,
          process->cpu,
          process->ppid,
          &birth,
          trace_num,
          process->name,
          process->brand,
          &pl_height,
          &process_info,
          &hashed_process_data);
        gtk_widget_set_size_request(drawing->drawing_area,
                                    -1,
                                    pl_height);
        gtk_widget_queue_draw(drawing->drawing_area);
    }
    /* Set the current process */
    process_list->current_hash_data[trace_num][process->cpu] =
                                               hashed_process_data;
  }

  processlist_set_name(process_list, process->name, hashed_process_data);

  return 0;

}

/* Get the filename of the process to print */
int after_user_generic_thread_brand_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  guint cpu = tfs->cpu;
  guint trace_num = ts->parent.index;
  LttvProcessState *process = ts->running_process[cpu];
  g_assert(process != NULL);

  guint pid = process->pid;

  /* Well, the process_out existed : we must get it in the process hash
   * or add it, and draw its items.
   */
   /* Add process to process list (if not present) */
  guint pl_height = 0;
  HashedProcessData *hashed_process_data = NULL;
  ProcessList *process_list = control_flow_data->process_list;
  LttTime birth = process->creation_time;
 
  if(likely(process_list->current_hash_data[trace_num][cpu] != NULL)) {
    hashed_process_data = process_list->current_hash_data[trace_num][cpu];
  } else {
    hashed_process_data = processlist_get_process_data(process_list,
            pid,
            process->cpu,
            &birth,
            trace_num);
    if(unlikely(hashed_process_data == NULL))
    {
      g_assert(pid == 0 || pid != process->ppid);
      ProcessInfo *process_info;
      /* Process not present */
      Drawing_t *drawing = control_flow_data->drawing;
      processlist_add(process_list,
          drawing,
          pid,
          process->tgid,
          process->cpu,
          process->ppid,
          &birth,
          trace_num,
          process->name,
	  process->brand,
          &pl_height,
          &process_info,
          &hashed_process_data);
        gtk_widget_set_size_request(drawing->drawing_area,
                                    -1,
                                    pl_height);
        gtk_widget_queue_draw(drawing->drawing_area);
    }
    /* Set the current process */
    process_list->current_hash_data[trace_num][process->cpu] =
                                               hashed_process_data;
  }

  processlist_set_brand(process_list, process->brand, hashed_process_data);

  return 0;

}


/* after_event_enum_process_hook
 * 
 * Create the processlist entry for the child process. Put the last
 * position in x at the current time value.
 *
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function adds items to be drawn in a queue for each process.
 * 
 */
int after_event_enum_process_hook(void *hook_data, void *call_data)
{
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  guint first_cpu, nb_cpus, cpu;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  LttTime evtime = ltt_event_time(e);

  /* Add process to process list (if not present) */
  LttvProcessState *process_in;
  LttTime birth;
  guint pl_height = 0;
  HashedProcessData *hashed_process_data_in = NULL;

  ProcessList *process_list = control_flow_data->process_list;
  guint trace_num = ts->parent.index;
  
  guint pid_in;
  {
    pid_in = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
  }
  
  if(pid_in == 0) {
    first_cpu = 0;
    nb_cpus = ltt_trace_get_num_cpu(ts->parent.t);
  } else {
    first_cpu = ANY_CPU;
    nb_cpus = ANY_CPU+1;
  }

  for(cpu = first_cpu; cpu < nb_cpus; cpu++) {
    /* Find process pid_in in the list... */
    process_in = lttv_state_find_process(ts, cpu, pid_in);
    //process_in = tfs->process;
    //guint cpu = tfs->cpu;
    //guint trace_num = ts->parent.index;
    //process_in = ts->running_process[cpu];
    /* It should exist, because we are after the state update. */
  #ifdef EXTRA_CHECK
    //g_assert(process_in != NULL);
  #endif //EXTRA_CHECK
    birth = process_in->creation_time;

    hashed_process_data_in = processlist_get_process_data(process_list,
            pid_in,
            process_in->cpu,
            &birth,
            trace_num);
    if(hashed_process_data_in == NULL)
    {
      if(pid_in != 0 && pid_in == process_in->ppid)
        g_critical("TEST %u , %u", pid_in, process_in->ppid);
      g_assert(pid_in == 0 || pid_in != process_in->ppid);
      ProcessInfo *process_info;
      Drawing_t *drawing = control_flow_data->drawing;
      /* Process not present */
      processlist_add(process_list,
          drawing,
          pid_in,
          process_in->tgid,
          process_in->cpu,
          process_in->ppid,
          &birth,
          trace_num,
          process_in->name,
          process_in->brand,
          &pl_height,
          &process_info,
          &hashed_process_data_in);
          gtk_widget_set_size_request(drawing->drawing_area,
                                      -1,
                                      pl_height);
          gtk_widget_queue_draw(drawing->drawing_area);
    } else {
      processlist_set_name(process_list, process_in->name,
                           hashed_process_data_in);
      processlist_set_ppid(process_list, process_in->ppid,
                           hashed_process_data_in);
      processlist_set_tgid(process_list, process_in->tgid,
                           hashed_process_data_in);
    }
  }
  return 0;
}


gint update_time_window_hook(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;
  ProcessList *process_list = control_flow_data->process_list;

  const TimeWindowNotifyData *time_window_nofify_data = 
                          ((const TimeWindowNotifyData *)call_data);

  TimeWindow *old_time_window = 
    time_window_nofify_data->old_time_window;
  TimeWindow *new_time_window = 
    time_window_nofify_data->new_time_window;
  
  /* Update the ruler */
  drawing_update_ruler(control_flow_data->drawing,
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
      guint width = control_flow_data->drawing->width;
      convert_time_to_pixels(
          *old_time_window,
          *ns,
          width,
          &x);

      /* Copy old data to new location */
      copy_pixmap_region(process_list,
                  NULL,
                  control_flow_data->drawing->drawing_area->style->black_gc,
                  NULL,
                  x, 0,
                  0, 0,
                  control_flow_data->drawing->width-x+SAFETY, -1);

      if(drawing->damage_begin == drawing->damage_end)
        drawing->damage_begin = control_flow_data->drawing->width-x;
      else
        drawing->damage_begin = 0;

      drawing->damage_end = control_flow_data->drawing->width;

      /* Clear the data request background, but not SAFETY */
      rectangle_pixmap(process_list,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin+SAFETY, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1);
      gtk_widget_queue_draw(drawing->drawing_area);
      //gtk_widget_queue_draw_area (drawing->drawing_area,
      //                          0,0,
      //                          control_flow_data->drawing->width,
      //                          control_flow_data->drawing->height);

      /* Get new data for the rest. */
      drawing_data_request(control_flow_data->drawing,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,
          control_flow_data->drawing->height);
    } else { 
      //if(ns<os<ns+w)
      //if(ns<os && os<ns+w)
      //if(ns<os && os<new_end)
      if(ltt_time_compare(*ns,*os) == -1
          && ltt_time_compare(*os,new_end) == -1)
      {
        g_info("scrolling near left");
        /* Scroll left, keep left part of the screen */
        guint x = 0;
        guint width = control_flow_data->drawing->width;
        convert_time_to_pixels(
            *new_time_window,
            *os,
            width,
            &x);
        
        /* Copy old data to new location */
        copy_pixmap_region  (process_list,
            NULL,
            control_flow_data->drawing->drawing_area->style->black_gc,
            NULL,
            0, 0,
            x, 0,
            -1, -1);
  
        if(drawing->damage_begin == drawing->damage_end)
          drawing->damage_end = x;
        else
          drawing->damage_end = 
            control_flow_data->drawing->width;

        drawing->damage_begin = 0;
        
        rectangle_pixmap (process_list,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          -1);

        gtk_widget_queue_draw(drawing->drawing_area);
        //gtk_widget_queue_draw_area (drawing->drawing_area,
        //                        0,0,
        //                        control_flow_data->drawing->width,
        //                        control_flow_data->drawing->height);


        /* Get new data for the rest. */
        drawing_data_request(control_flow_data->drawing,
            drawing->damage_begin, 0,
            drawing->damage_end - drawing->damage_begin,
            control_flow_data->drawing->height);
    
      } else {
        if(ltt_time_compare(*ns,*os) == 0)
        {
          g_info("not scrolling");
        } else {
          g_info("scrolling far");
          /* Cannot reuse any part of the screen : far jump */
          
          
          rectangle_pixmap (process_list,
            control_flow_data->drawing->drawing_area->style->black_gc,
            TRUE,
            0, 0,
            control_flow_data->drawing->width+SAFETY, // do not overlap
            -1);

          //gtk_widget_queue_draw_area (drawing->drawing_area,
          //                      0,0,
          //                      control_flow_data->drawing->width,
          //                      control_flow_data->drawing->height);
          gtk_widget_queue_draw(drawing->drawing_area);

          drawing->damage_begin = 0;
          drawing->damage_end = control_flow_data->drawing->width;

          drawing_data_request(control_flow_data->drawing,
              0, 0,
              control_flow_data->drawing->width,
              control_flow_data->drawing->height);
      
        }
      }
    }
  } else {
    /* Different scale (zoom) */
    g_info("zoom");

    rectangle_pixmap (process_list,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          control_flow_data->drawing->width+SAFETY, // do not overlap
          -1);

    //gtk_widget_queue_draw_area (drawing->drawing_area,
    //                            0,0,
    //                            control_flow_data->drawing->width,
    //                            control_flow_data->drawing->height);
    gtk_widget_queue_draw(drawing->drawing_area);
  
    drawing->damage_begin = 0;
    drawing->damage_end = control_flow_data->drawing->width;

    drawing_data_request(control_flow_data->drawing,
        0, 0,
        control_flow_data->drawing->width,
        control_flow_data->drawing->height);
  }

  /* Update directly when scrolling */
  gdk_window_process_updates(control_flow_data->drawing->drawing_area->window,
      TRUE);

  return 0;
}

gint traceset_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;

  if(unlikely(drawing->gc == NULL)) {
    return FALSE;
  }
  if(drawing->dotted_gc == NULL) {
    return FALSE;
  }

  drawing_clear(control_flow_data->drawing);
  processlist_clear(control_flow_data->process_list);
  gtk_widget_set_size_request(
      control_flow_data->drawing->drawing_area,
                -1, processlist_get_height(control_flow_data->process_list));
  redraw_notify(control_flow_data, NULL);

  request_background_data(control_flow_data);
 
  return FALSE;
}

gint redraw_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  drawing->damage_begin = 0;
  drawing->damage_end = drawing->width;

  /* fun feature, to be separated someday... */
  drawing_clear(control_flow_data->drawing);
  processlist_clear(control_flow_data->process_list);
  gtk_widget_set_size_request(
      control_flow_data->drawing->drawing_area,
                -1, processlist_get_height(control_flow_data->process_list));
  // Clear the images
  rectangle_pixmap (control_flow_data->process_list,
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

  //gtk_widget_queue_draw_area(drawing->drawing_area,
  //                           0,0,
  //                           drawing->width,
  //                           drawing->height);
  return FALSE;

}


gint continue_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;

  //g_assert(widget->allocation.width == drawing->damage_end);

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
  ControlFlowData *control_flow_data = (ControlFlowData*)hook_data;
  Drawing_t *drawing = control_flow_data->drawing;

  LttTime current_time = *((LttTime*)call_data);
  
  TimeWindow time_window =
            lttvwindow_get_time_window(control_flow_data->tab);
  
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
        lttvwindow_get_traceset_context(control_flow_data->tab);
  
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

    lttvwindow_report_time_window(control_flow_data->tab, new_time_window);
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

    lttvwindow_report_time_window(control_flow_data->tab, new_time_window);
    
  }
  gtk_widget_queue_draw(control_flow_data->drawing->drawing_area);
  
  /* Update directly when scrolling */
  gdk_window_process_updates(control_flow_data->drawing->drawing_area->window,
      TRUE);
                             
  return 0;
}

typedef struct _ClosureData {
  EventsRequest *events_request;
  LttvTracesetState *tss;
  LttTime end_time;
  guint x_end;
} ClosureData;
  

void draw_closure(gpointer key, gpointer value, gpointer user_data)
{
  ProcessInfo *process_info = (ProcessInfo*)key;
  HashedProcessData *hashed_process_data = (HashedProcessData*)value;
  ClosureData *closure_data = (ClosureData*)user_data;
    
  EventsRequest *events_request = closure_data->events_request;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracesetState *tss = closure_data->tss;
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;

  LttTime evtime = closure_data->end_time;

  gboolean dodraw = TRUE;

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

#if 0
    //FIXME : optimize data structures.
    LttvTracefileState *tfs;
    LttvTracefileContext *tfc;
    guint i;
    for(i=0;i<tc->tracefiles->len;i++) {
      tfc = g_array_index(tc->tracefiles, LttvTracefileContext*, i);
      if(ltt_tracefile_name(tfc->tf) == LTT_NAME_CPU
          && tfs->cpu == process_info->cpu)
        break;

    }
    g_assert(i<tc->tracefiles->len);
    tfs = LTTV_TRACEFILE_STATE(tfc);
#endif //0
 //   LttvTracefileState *tfs =
 //    (LttvTracefileState*)tsc->traces[process_info->trace_num]->
 //                        tracefiles[process_info->cpu];
 
    LttvProcessState *process;
    process = lttv_state_find_process(ts, process_info->cpu,
                                      process_info->pid);

    if(unlikely(process != NULL)) {
      
       LttvFilter *filter = control_flow_data->filter;
       if(filter != NULL && filter->head != NULL)
         if(!lttv_filter_tree_parse(filter->head,NULL,NULL,
             tc->t,NULL,process,tc))
           dodraw = FALSE;

      /* Only draw for processes that are currently in the trace states */

      ProcessList *process_list = control_flow_data->process_list;
#ifdef EXTRA_CHECK
      /* Should be alike when background info is ready */
      if(control_flow_data->background_info_waiting==0)
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
          lttvwindow_get_time_window(control_flow_data->tab);

#ifdef EXTRA_CHECK
        if(ltt_time_compare(evtime, time_window.start_time) == -1
              || ltt_time_compare(evtime, time_window.end_time) == 1)
                  return;
#endif //EXTRA_CHECK
        Drawing_t *drawing = control_flow_data->drawing;
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
                  PropertiesLine prop_line = prepare_s_e_line(process);
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
  return;
}

int before_chunk(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  ControlFlowData *cfd = (ControlFlowData*)events_request->viewer_data;
#if 0  
  /* Desactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(cfd->process_list->list_store),
      TRACE_COLUMN,
      GTK_SORT_ASCENDING);
#endif //0
  drawing_chunk_begin(events_request, tss);

  return 0;
}

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
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  
  ProcessList *process_list = control_flow_data->process_list;
  LttTime end_time = events_request->end_time;

  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;
  closure_data.end_time = end_time;

  TimeWindow time_window = 
          lttvwindow_get_time_window(control_flow_data->tab);
  guint width = control_flow_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            end_time,
            width,
            &closure_data.x_end);


  /* Draw last items */
  g_hash_table_foreach(process_list->process_hash, draw_closure,
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
int after_chunk(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;
  LttvTracesetState *tss = (LttvTracesetState*)call_data;
  LttvTracesetContext *tsc = (LttvTracesetContext*)call_data;
  LttvTracefileContext *tfc = lttv_traceset_context_get_current_tfc(tsc);
  LttTime end_time;
  
  ProcessList *process_list = control_flow_data->process_list;
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
          lttvwindow_get_time_window(control_flow_data->tab);
  guint width = control_flow_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            end_time,
            width,
            &closure_data.x_end);

  /* Draw last items */
  g_hash_table_foreach(process_list->process_hash, draw_closure,
                        (void*)&closure_data);
#if 0
  /* Reactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(control_flow_data->process_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);

  update_index_to_pixmap(control_flow_data->process_list);
  /* Request a full expose : drawing scrambled */
  gtk_widget_queue_draw(control_flow_data->drawing->drawing_area);
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
  LttvTraceHook *th = (LttvTraceHook*)hook_data;
  EventsRequest *events_request = (EventsRequest*)th->hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttvTraceState *ts = (LttvTraceState *)tfc->t_context;

  LttvTracesetState *tss = (LttvTracesetState*)tfc->t_context->ts_context;
  ProcessList *process_list = control_flow_data->process_list;

  LttEvent *e;
  e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = control_flow_data->filter;
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
          lttvwindow_get_time_window(control_flow_data->tab);
  guint width = control_flow_data->drawing->width;
  convert_time_to_pixels(
            time_window,
            evtime,
            width,
            &closure_data.x_end);

  /* Draw last items */
  g_hash_table_foreach(process_list->process_hash, draw_closure,
                        (void*)&closure_data);
#if 0
  /* Reactivate sort */
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(control_flow_data->process_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);

  update_index_to_pixmap(control_flow_data->process_list);
  /* Request a full expose : drawing scrambled */
  gtk_widget_queue_draw(control_flow_data->drawing->drawing_area);
#endif //0
  /* Request expose (updates damages zone also) */
  drawing_request_expose(events_request, tss, evtime);

  return 0;
}
