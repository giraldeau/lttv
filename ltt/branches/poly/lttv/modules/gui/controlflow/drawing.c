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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#include <ltt/trace.h>

#include <lttv/lttv.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/lttvwindow.h>
#include <lttv/state.h>
#include <lttv/hook.h>

#include "drawing.h"
#include "eventhooks.h"
#include "cfv.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

//FIXME
#define TRACE_NUMBER 0

#if 0 /* colors for two lines representation */
GdkColor drawing_colors[NUM_COLORS] =
{ /* Pixel, R, G, B */
  { 0, 0, 0, 0 }, /* COL_BLACK */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }, /* COL_WHITE */
  { 0, 0x0FFF, 0xFFFF, 0xFFFF }, /* COL_WAIT_FORK : pale blue */
  { 0, 0xFFFF, 0xFFFF, 0x0000 }, /* COL_WAIT_CPU : yellow */
  { 0, 0xFFFF, 0xA000, 0xFCFF }, /* COL_EXIT : pale magenta */
  { 0, 0xFFFF, 0x0000, 0xFFFF }, /* COL_ZOMBIE : purple */
  { 0, 0xFFFF, 0x0000, 0x0000 }, /* COL_WAIT : red */
  { 0, 0x0000, 0xFFFF, 0x0000 }, /* COL_RUN : green */
  { 0, 0x8800, 0xFFFF, 0x8A00 }, /* COL_USER_MODE : pale green */
  { 0, 0x09FF, 0x01FF, 0xFFFF }, /* COL_SYSCALL : blue */
  { 0, 0xF900, 0x4200, 0xFF00 }, /* COL_TRAP : pale purple */
  { 0, 0xFFFF, 0x5AFF, 0x01FF }, /* COL_IRQ : orange */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }  /* COL_MODE_UNKNOWN : white */

};
#endif //0


GdkColor drawing_colors[NUM_COLORS] =
{ /* Pixel, R, G, B */
  { 0, 0, 0, 0 }, /* COL_BLACK */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }, /* COL_WHITE */
  { 0, 0x0000, 0xFF00, 0x0000 }, /* COL_RUN_USER_MODE : green */
  { 0, 0x0100, 0x9E00, 0xFFFF }, /* COL_RUN_SYSCALL : pale blue */
  { 0, 0xFF00, 0xFF00, 0x0100 }, /* COL_RUN_TRAP : yellow */
  { 0, 0xFFFF, 0x0000, 0x0000 }, /* COL_RUN_IRQ : red */
  { 0, 0xA3FF, 0x0000, 0x0000 }, /* COL_WAIT : dark red */
  { 0, 0x7700, 0x7700, 0x0000 }, /* COL_WAIT_CPU : dark yellow */
  { 0, 0x6400, 0x0000, 0x5D00 }, /* COL_ZOMBIE : dark purple */
  { 0, 0x0700, 0x6400, 0x0000 }, /* COL_WAIT_FORK : dark green */
  { 0, 0x8900, 0x0000, 0x8400 }, /* COL_EXIT : "less dark" magenta */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }, /* COL_MODE_UNKNOWN : white */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }  /* COL_UNNAMED : white */

};

/*
RUN+USER MODE green
RUN+SYSCALL
RUN+TRAP
RUN+IRQ
WAIT+foncé
WAIT CPU + WAIT FORK vert foncé ou jaune
IRQ rouge
TRAP: orange
SYSCALL: bleu pâle

ZOMBIE + WAIT EXIT
*/


/*****************************************************************************
 *                              drawing functions                            *
 *****************************************************************************/

static gboolean
expose_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data );

static gboolean
motion_notify_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);


/* Function responsible for updating the exposed area.
 * It must do an events request to the lttvwindow API to ask for this update.
 * Note : this function cannot clear the background, because it may
 * erase drawing already present (SAFETY).
 */
void drawing_data_request(Drawing_t *drawing,
      GdkPixmap **pixmap,
      gint x, gint y,
      gint width,
      gint height)
{
  if(width < 0) return ;
  if(height < 0) return ;

  Tab *tab = drawing->control_flow_data->tab;
  TimeWindow time_window =
              lttvwindow_get_time_window(tab);

  ControlFlowData *control_flow_data = drawing->control_flow_data;
  //    (ControlFlowData*)g_object_get_data(
  //               G_OBJECT(drawing->drawing_area), "control_flow_data");

  LttTime start, time_end;
  LttTime window_end = time_window.end_time;

  g_debug("req : window start_time : %lu, %lu", time_window.start_time.tv_sec, 
                                       time_window.start_time.tv_nsec);

  g_debug("req : window time width : %lu, %lu", time_window.time_width.tv_sec, 
                                       time_window.time_width.tv_nsec);
  
  g_debug("req : window_end : %lu, %lu", window_end.tv_sec, 
                                       window_end.tv_nsec);

  g_debug("x is : %i, x+width is : %i", x, x+width);

  convert_pixels_to_time(drawing->width, x,
        time_window,
        &start);

  convert_pixels_to_time(drawing->width, x+width,
        time_window,
        &time_end);
  time_end = ltt_time_add(time_end, ltt_time_one); // because main window
                                                   // doesn't deliver end time.

  lttvwindow_events_request_remove_all(tab,
                                       control_flow_data);

  {
    /* find the tracehooks */
    LttvTracesetContext *tsc = lttvwindow_get_traceset_context(tab);

    LttvTraceset *traceset = tsc->ts;

    guint i, k, nb_trace;

    LttvTraceState *ts;

    LttvTracefileState *tfs;

    GArray *hooks;

    LttvTraceHook hook;

    nb_trace = lttv_traceset_number(traceset);
    // FIXME : eventually request for more traces
    // for(i = 0 ; i < nb_trace ; i++) {
    for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++)
    {
      EventsRequest *events_request = g_new(EventsRequest, 1);
      // Create the hooks
      //LttvHooks *event = lttv_hooks_new();
      LttvHooksById *event_by_id = lttv_hooks_by_id_new();
      LttvHooks *before_chunk_traceset = lttv_hooks_new();
      LttvHooks *after_chunk_traceset = lttv_hooks_new();
      LttvHooks *before_request_hook = lttv_hooks_new();
      LttvHooks *after_request_hook = lttv_hooks_new();

      lttv_hooks_add(before_chunk_traceset,
                     before_chunk,
                     events_request,
                     LTTV_PRIO_DEFAULT);

      lttv_hooks_add(after_chunk_traceset,
                     after_chunk,
                     events_request,
                     LTTV_PRIO_DEFAULT);

      lttv_hooks_add(before_request_hook,
                     before_request,
                     events_request,
                     LTTV_PRIO_DEFAULT);

      lttv_hooks_add(after_request_hook,
                     after_request,
                     events_request,
                     LTTV_PRIO_DEFAULT);


      ts = (LttvTraceState *)tsc->traces[i];

      /* Find the eventtype id for the following events and register the
         associated by id hooks. */

      hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
      g_array_set_size(hooks, 10);

      /* before hooks */
      
      lttv_trace_find_hook(ts->parent.t, "core","syscall_entry","syscall_id", 
    NULL, NULL, before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 0));

      lttv_trace_find_hook(ts->parent.t, "core", "syscall_exit", NULL, NULL, 
          NULL, before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 1));

      lttv_trace_find_hook(ts->parent.t, "core", "trap_entry", "trap_id",
    NULL, NULL, before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 2));

      lttv_trace_find_hook(ts->parent.t, "core", "trap_exit", NULL, NULL, NULL, 
          before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 3));

      lttv_trace_find_hook(ts->parent.t, "core", "irq_entry", "irq_id", NULL, 
          NULL, before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 4));

      lttv_trace_find_hook(ts->parent.t, "core", "irq_exit", NULL, NULL, NULL, 
          before_execmode_hook, &g_array_index(hooks, LttvTraceHook, 5));

      lttv_trace_find_hook(ts->parent.t, "core", "schedchange", "in", "out", 
        "out_state", before_schedchange_hook, 
        &g_array_index(hooks, LttvTraceHook, 6));

      lttv_trace_find_hook(ts->parent.t, "core", "process", "event_sub_id", 
          "event_data1", "event_data2", before_process_hook,
          &g_array_index(hooks, LttvTraceHook, 7));

#if 0
      lttv_trace_find_hook(ts->parent.t, "core", "process_fork", "child_pid", 
          NULL, NULL, process_fork, &g_array_index(hooks, LttvTraceHook, 7));

      lttv_trace_find_hook(ts->parent.t, "core", "process_exit", NULL, NULL, 
          NULL, process_exit, &g_array_index(hooks, LttvTraceHook, 8));
#endif //0

      /* after hooks */
      
#if 0
      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core","syscall_entry","syscall_id", 
    NULL, NULL, after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 8));
      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core", "syscall_exit", NULL, NULL, 
          NULL, after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 9));

      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core", "trap_entry", "trap_id",
    NULL, NULL, after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 10));

      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core", "trap_exit", NULL, NULL, NULL, 
          after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 11));

      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core", "irq_entry", "irq_id", NULL, 
          NULL, after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 12));

      /**** DESACTIVATED ****/
      lttv_trace_find_hook(ts->parent.t, "core", "irq_exit", NULL, NULL, NULL, 
          after_execmode_hook, &g_array_index(hooks, LttvTraceHook, 13));
#endif //0

      lttv_trace_find_hook(ts->parent.t, "core", "schedchange", "in", "out", 
        "out_state", after_schedchange_hook, 
        &g_array_index(hooks, LttvTraceHook, 8));

      lttv_trace_find_hook(ts->parent.t, "core", "process", "event_sub_id", 
          "event_data1", "event_data2", after_process_hook,
          &g_array_index(hooks, LttvTraceHook, 9));

#if 0
      lttv_trace_find_hook(ts->parent.t, "core", "process_fork", "child_pid", 
          NULL, NULL, process_fork, &g_array_index(hooks, LttvTraceHook, 7));

      lttv_trace_find_hook(ts->parent.t, "core", "process_exit", NULL, NULL, 
          NULL, process_exit, &g_array_index(hooks, LttvTraceHook, 8));
#endif //0


      
      /* Add these hooks to each event_by_id hooks list */
      /* add before */
      for(k = 0 ; k < 8 ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(event_by_id, 
                        hook.id), hook.h,
                        events_request,
                        LTTV_PRIO_STATE-5);
      }

      /* add after */
      for(k = 8 ; k < 10 ; k++) {
        hook = g_array_index(hooks, LttvTraceHook, k);
        lttv_hooks_add(lttv_hooks_by_id_find(event_by_id, 
                       hook.id), hook.h,
                       events_request,
                       LTTV_PRIO_STATE+5);
      }

      events_request->hooks = hooks;

      // Fill the events request
      events_request->owner = control_flow_data;
      events_request->viewer_data = control_flow_data;
      events_request->servicing = FALSE;
      events_request->start_time = start;
      events_request->start_position = NULL;
      events_request->stop_flag = FALSE;
      events_request->end_time = time_end;
      events_request->num_events = G_MAXUINT;
      events_request->end_position = NULL;
      events_request->trace = i;    /* FIXME */
      events_request->before_chunk_traceset = before_chunk_traceset;
      events_request->before_chunk_trace = NULL;
      events_request->before_chunk_tracefile = NULL;
      events_request->event = NULL;
      events_request->event_by_id = event_by_id;
      events_request->after_chunk_tracefile = NULL;
      events_request->after_chunk_trace = NULL;
      events_request->after_chunk_traceset = after_chunk_traceset;
      events_request->before_request = before_request_hook;
      events_request->after_request = after_request_hook;

      g_debug("req : start : %lu, %lu", start.tv_sec, 
                                          start.tv_nsec);

      g_debug("req : end : %lu, %lu", time_end.tv_sec, 
                                         time_end.tv_nsec);

      lttvwindow_events_request(tab, events_request);

    }

  }

#if 0
  lttv_hooks_add(event,
                 before_schedchange_hook,
                 events_request,
                 LTTV_PRIO_STATE-5);
  lttv_hooks_add(event,
                 after_schedchange_hook,
                 events_request,
                 LTTV_PRIO_STATE+5);
  lttv_hooks_add(event,
                 before_execmode_hook,
                 events_request,
                 LTTV_PRIO_STATE-5);
  lttv_hooks_add(event,
                 after_execmode_hook,
                 events_request,
                 LTTV_PRIO_STATE+5);
  lttv_hooks_add(event,
                 before_process_hook,
                 events_request,
                 LTTV_PRIO_STATE-5);
  lttv_hooks_add(event,
                 after_process_hook,
                 events_request,
                 LTTV_PRIO_STATE+5);
#endif //0

}
 

static void set_last_start(gpointer key, gpointer value, gpointer user_data)
{
  ProcessInfo *process_info = (ProcessInfo*)key;
  HashedProcessData *hashed_process_data = (HashedProcessData*)value;
  guint x = (guint)user_data;

  hashed_process_data->x.over = x;
  hashed_process_data->x.over_used = FALSE;
  hashed_process_data->x.over_marked = FALSE;
  hashed_process_data->x.middle = x;
  hashed_process_data->x.middle_used = FALSE;
  hashed_process_data->x.middle_marked = FALSE;
  hashed_process_data->x.under = x;
  hashed_process_data->x.under_used = FALSE;
  hashed_process_data->x.under_marked = FALSE;
  hashed_process_data->next_good_time = ltt_time_zero;

  return;
}

void drawing_data_request_begin(EventsRequest *events_request, LttvTracesetState *tss)
{
  g_debug("Begin of data request");
  ControlFlowData *cfd = events_request->viewer_data;
  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tss);
  TimeWindow time_window = 
    lttvwindow_get_time_window(cfd->tab);

  guint width = cfd->drawing->width;
  guint x=0;

  cfd->drawing->last_start = events_request->start_time;

  convert_time_to_pixels(
          time_window,
          events_request->start_time,
          width,
          &x);

  g_hash_table_foreach(cfd->process_list->process_hash, set_last_start,
                            (gpointer)x);

}

void drawing_chunk_begin(EventsRequest *events_request, LttvTracesetState *tss)
{
  g_debug("Begin of chunk");
  ControlFlowData *cfd = events_request->viewer_data;
  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tss);
  //LttTime current_time = lttv_traceset_context_get_current_tfc(tsc)->timestamp;
  guint num_cpu = 
    ltt_trace_per_cpu_tracefile_number(tss->parent.traces[TRACE_NUMBER]->t);

  cfd->process_list->current_hash_data = g_new(HashedProcessData*,num_cpu);
  memset(cfd->process_list->current_hash_data, 0,
         sizeof(HashedProcessData*)*num_cpu);
  //cfd->drawing->last_start = LTT_TIME_MIN(current_time,
  //                                        events_request->end_time);
}


void drawing_request_expose(EventsRequest *events_request,
                            LttvTracesetState *tss,
                            LttTime end_time)
{
  gint x, x_end, width;

  ControlFlowData *cfd = events_request->viewer_data;
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;
  Drawing_t *drawing = cfd->drawing;
  
  TimeWindow time_window = 
        lttvwindow_get_time_window(cfd->tab);

  g_debug("request expose");
  
  convert_time_to_pixels(
        time_window,
        end_time,
        drawing->width,
        &x_end);
  x = drawing->damage_begin;

  width = x_end - x;

  drawing->damage_begin = x+width;

  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               x, 0,
                               width, drawing->height);
 
}


/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
/* As the scaling will always change, it's of no use to copy old
 * pixmap.
 *
 * Only change the size if width changes. The height is specified and changed
 * when process ID are added or removed from the process list.
 */
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event, 
    gpointer user_data)
{
  Drawing_t *drawing = (Drawing_t*)user_data;


  /* First, get the new time interval of the main window */
  /* we assume (see documentation) that the main window
   * has updated the time interval before this configure gets
   * executed.
   */
  //lttvwindow_get_time_window(drawing->control_flow_data->mw,
  //      &drawing->control_flow_data->time_window);
  
  /* New pixmap, size of the configure event */
  //GdkPixmap *pixmap = gdk_pixmap_new(widget->window,
  //      widget->allocation.width + SAFETY,
  //      widget->allocation.height + SAFETY,
  //      -1);
  
  if(widget->allocation.width != drawing->width) {
    g_debug("drawing configure event");
    g_debug("New alloc draw size : %i by %i",widget->allocation.width,
                                    widget->allocation.height);
  
    
    if (drawing->pixmap)
      gdk_pixmap_unref(drawing->pixmap);
  
    drawing->width = widget->allocation.width;
    //drawing->height = widget->allocation.height;

    drawing->pixmap = gdk_pixmap_new(widget->window,
                                     drawing->width + SAFETY,
                                     drawing->height,
                                     -1);
    //ProcessList_get_height
    // (GuiControlFlow_get_process_list(drawing->control_flow_data)),
    

    // Clear the image
    gdk_draw_rectangle (drawing->pixmap,
          widget->style->black_gc,
          TRUE,
          0, 0,
          drawing->width+SAFETY,
          drawing->height);

    //g_info("init data request");


    /* Initial data request */
    /* no, do initial data request in the expose event */
    // Do not need to ask for data of 1 pixel : not synchronized with
    // main window time at this moment.
    //drawing_data_request(drawing, &drawing->pixmap, 0, 0,
    //    widget->allocation.width,
    //    widget->allocation.height);
                          
    //drawing->width = widget->allocation.width;
    //drawing->height = widget->allocation.height;
  
    drawing->damage_begin = 0;
    drawing->damage_end = widget->allocation.width;

    if(drawing->damage_begin < drawing->damage_end)
    {
      drawing_data_request(drawing,
                           &drawing->pixmap,
                           drawing->damage_begin,
                           0,
                           drawing->damage_end - drawing->damage_begin,
                           drawing->height);
    }
  }
  return TRUE;
}


/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  Drawing_t *drawing = (Drawing_t*)user_data;

  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "control_flow_data");
  if(unlikely(drawing->gc == NULL)) {
    drawing->gc = gdk_gc_new(drawing->drawing_area->window);
    gdk_gc_copy(drawing->gc, drawing->drawing_area->style->black_gc);
  }

  TimeWindow time_window = 
      lttvwindow_get_time_window(control_flow_data->tab);
  LttTime current_time = 
      lttvwindow_get_current_time(control_flow_data->tab);

  guint cursor_x=0;

  LttTime window_end = time_window.end_time;

  /* update the screen from the pixmap buffer */
  gdk_draw_pixmap(widget->window,
      widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
      drawing->pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);

  
  if(ltt_time_compare(time_window.start_time, current_time) <= 0 &&
           ltt_time_compare(window_end, current_time) >= 0)
  {
    /* Draw the dotted lines */
    convert_time_to_pixels(
          time_window,
          current_time,
          drawing->width,
          &cursor_x);


    if(drawing->dotted_gc == NULL) {

      drawing->dotted_gc = gdk_gc_new(drawing->drawing_area->window);
      gdk_gc_copy(drawing->dotted_gc, widget->style->white_gc);
   
      gint8 dash_list[] = { 1, 2 };
      gdk_gc_set_line_attributes(drawing->dotted_gc,
                                 1,
                                 GDK_LINE_ON_OFF_DASH,
                                 GDK_CAP_BUTT,
                                 GDK_JOIN_MITER);
      gdk_gc_set_dashes(drawing->dotted_gc,
                        0,
                        dash_list,
                        2);
    }

    drawing_draw_line(NULL, widget->window,
                  cursor_x, 0,
                  cursor_x, drawing->height,
                  drawing->dotted_gc);
  }
  return FALSE;
}

static gboolean
after_expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  //g_assert(0);
  g_debug("AFTER EXPOSE");

  return FALSE;


}

#if 0
void
tree_row_activated(GtkTreeModel *treemodel,
                   GtkTreePath *arg1,
                   GtkTreeViewColumn *arg2,
                   gpointer user_data)
{
  ControlFlowData *cfd = (ControlFlowData*)user_data;
  Drawing_t *drawing = cfd->drawing;
  GtkTreeView *treeview = cfd->process_list->process_list_widget;
  gint *path_indices;
  gint height;
  
  path_indices =  gtk_tree_path_get_indices (arg1);

  height = get_cell_height(cfd->process_list,
        GTK_TREE_VIEW(treeview));
  drawing->horizontal_sel = height * path_indices[0];
  g_critical("new hor sel : %i", drawing->horizontal_sel);
}
#endif //0

/* mouse click */
static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "control_flow_data");
  Drawing_t *drawing = control_flow_data->drawing;
  TimeWindow time_window =
               lttvwindow_get_time_window(control_flow_data->tab);

  g_debug("click");
  if(event->button == 1)
  {
    LttTime time;

    /* left mouse button click */
    g_debug("x click is : %f", event->x);

    convert_pixels_to_time(drawing->width, (guint)event->x,
        time_window,
        &time);

    lttvwindow_report_current_time(control_flow_data->tab, time);

  }

  return FALSE;
}

static gboolean
scrollbar_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  Drawing_t *drawing = (Drawing_t*)user_data;

  gtk_widget_set_size_request(drawing->padding, allocation->width, -1);
  //gtk_widget_queue_resize(drawing->padding);
  //gtk_widget_queue_resize(drawing->ruler);
  gtk_container_check_resize(GTK_CONTAINER(drawing->ruler_hbox));
  return 0;
}



Drawing_t *drawing_construct(ControlFlowData *control_flow_data)
{
  Drawing_t *drawing = g_new(Drawing_t, 1);
  
  drawing->control_flow_data = control_flow_data;

  drawing->vbox = gtk_vbox_new(FALSE, 1);

  
  drawing->ruler_hbox = gtk_hbox_new(FALSE, 1);
  drawing->ruler = gtk_drawing_area_new ();
  //gtk_widget_set_size_request(drawing->ruler, -1, 27);
  
  drawing->padding = gtk_drawing_area_new ();
  //gtk_widget_set_size_request(drawing->padding, -1, 27);
  gtk_box_pack_start(GTK_BOX(drawing->ruler_hbox), drawing->ruler, 
                     TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->ruler_hbox), drawing->padding, 
                     FALSE, FALSE, 0);
  


  drawing->drawing_area = gtk_drawing_area_new ();
  
  drawing->gc = NULL;
  
  drawing->hbox = gtk_hbox_new(FALSE, 1);
  drawing->viewport = gtk_viewport_new(NULL, control_flow_data->v_adjust);
  drawing->scrollbar = gtk_vscrollbar_new(control_flow_data->v_adjust);
  gtk_box_pack_start(GTK_BOX(drawing->hbox), drawing->viewport, 
                     TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->hbox), drawing->scrollbar, 
                     FALSE, FALSE, 0);
  
  //drawing->scrolled_window =
  //    gtk_scrolled_window_new (NULL,
  //    control_flow_data->v_adjust);
  
  //gtk_scrolled_window_set_policy(
  //  GTK_SCROLLED_WINDOW(drawing->scrolled_window),
  //  GTK_POLICY_NEVER,
  //  GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(drawing->viewport),
                    drawing->drawing_area);
  //gtk_scrolled_window_add_with_viewport(
  //  GTK_SCROLLED_WINDOW(drawing->scrolled_window),
  //  drawing->drawing_area);

  gtk_box_pack_start(GTK_BOX(drawing->vbox), drawing->ruler_hbox, 
                     FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->vbox), drawing->hbox,
                   TRUE, TRUE, 0);
  
  drawing->pango_layout =
    gtk_widget_create_pango_layout(drawing->drawing_area, NULL);

  drawing->dotted_gc = NULL;

  drawing->height = 1;
  drawing->width = 1;
  drawing->depth = 0;
  
  drawing->damage_begin = 0;
  drawing->damage_end = 0;
  drawing->horizontal_sel = -1;
  
  //gtk_widget_set_size_request(drawing->drawing_area->window, 50, 50);
  g_object_set_data_full(
      G_OBJECT(drawing->drawing_area),
      "Link_drawing_Data",
      drawing,
      (GDestroyNotify)drawing_destroy);

  g_object_set_data(
      G_OBJECT(drawing->ruler),
      "drawing",
      drawing);


  //gtk_widget_modify_bg( drawing->drawing_area,
  //      GTK_STATE_NORMAL,
  //      &CF_Colors[BLACK]);
  
  //gdk_window_get_geometry(drawing->drawing_area->window,
  //    NULL, NULL,
  //    &(drawing->width),
  //    &(drawing->height),
  //    -1);
  
  //drawing->pixmap = gdk_pixmap_new(
  //    drawing->drawing_area->window,
  //    drawing->width,
  //    drawing->height,
  //    drawing->depth);
  
  drawing->pixmap = NULL;

//  drawing->pixmap = gdk_pixmap_new(drawing->drawing_area->window,
//        drawing->drawing_area->allocation.width,
//        drawing->drawing_area->allocation.height,
//        -1);

  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "configure_event",
        G_CALLBACK (configure_event),
        (gpointer)drawing);
 
  g_signal_connect (G_OBJECT(drawing->ruler),
        "expose_event",
        G_CALLBACK(expose_ruler),
        (gpointer)drawing);

  gtk_widget_add_events(drawing->ruler, GDK_POINTER_MOTION_MASK);

  g_signal_connect (G_OBJECT(drawing->ruler),
        "motion-notify-event",
        G_CALLBACK(motion_notify_ruler),
        (gpointer)drawing);


  g_signal_connect (G_OBJECT(drawing->scrollbar),
        "size-allocate",
        G_CALLBACK(scrollbar_size_allocate),
        (gpointer)drawing);



  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "expose_event",
        G_CALLBACK (expose_event),
        (gpointer)drawing);

  g_signal_connect_after (G_OBJECT(drawing->drawing_area),
        "expose_event",
        G_CALLBACK (after_expose_event),
        (gpointer)drawing);

  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "button-press-event",
        G_CALLBACK (button_press_event),
        (gpointer)drawing);
  
  gtk_widget_show(drawing->ruler);
  gtk_widget_show(drawing->padding);
  gtk_widget_show(drawing->ruler_hbox);

  gtk_widget_show(drawing->drawing_area);
  //gtk_widget_show(drawing->scrolled_window);
  gtk_widget_show(drawing->viewport);
  gtk_widget_show(drawing->scrollbar);
  gtk_widget_show(drawing->hbox);

  /* Allocate the colors */
  GdkColormap* colormap = gdk_colormap_get_system();
  gboolean success[NUM_COLORS];
  gdk_colormap_alloc_colors(colormap, drawing_colors, NUM_COLORS, FALSE,
                            TRUE, success);
  
  return drawing;
}

void drawing_destroy(Drawing_t *drawing)
{
  g_info("drawing_destroy %p", drawing);

  /* Free the colors */
  GdkColormap* colormap = gdk_colormap_get_system();

  gdk_colormap_free_colors(colormap, drawing_colors, NUM_COLORS);
  


  // Do not unref here, Drawing_t destroyed by it's widget.
  //g_object_unref( G_OBJECT(drawing->drawing_area));
  if(drawing->gc != NULL)
    gdk_gc_unref(drawing->gc);
  
  g_free(drawing->pango_layout);
  if(!drawing->dotted_gc) gdk_gc_unref(drawing->dotted_gc);
  g_free(drawing);
  g_info("drawing_destroy end");
}

GtkWidget *drawing_get_drawing_area(Drawing_t *drawing)
{
  return drawing->drawing_area;
}

GtkWidget *drawing_get_widget(Drawing_t *drawing)
{
  return drawing->vbox;
}

void drawing_draw_line( Drawing_t *drawing,
      GdkPixmap *pixmap,
      guint x1, guint y1,
      guint x2, guint y2,
      GdkGC *GC)
{
  gdk_draw_line (pixmap,
      GC,
      x1, y1, x2, y2);
}

void drawing_clear(Drawing_t *drawing)
{ 
  if (drawing->pixmap)
    gdk_pixmap_unref(drawing->pixmap);

  drawing->height = 1;
  /* Allocate a new pixmap with new height */
  drawing->pixmap = gdk_pixmap_new(drawing->drawing_area->window,
                                   drawing->width + SAFETY,
                                   drawing->height,
                                     -1);

  gtk_widget_set_size_request(drawing->drawing_area,
                             -1,
                             drawing->height);
  gtk_widget_queue_resize_no_redraw(drawing->drawing_area);
  
  /* ask for the buffer to be redrawn */
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               0, 0,
                               drawing->width, drawing->height);
}


/* Insert a square corresponding to a new process in the list */
/* Applies to whole drawing->width */
void drawing_insert_square(Drawing_t *drawing,
        guint y,
        guint height)
{
  //GdkRectangle update_rect;

  /* Allocate a new pixmap with new height */
  GdkPixmap *pixmap = gdk_pixmap_new(drawing->drawing_area->window,
        drawing->width + SAFETY,
        drawing->height + height,
        -1);
  
  /* Copy the high region */
  gdk_draw_drawable (pixmap,
    drawing->drawing_area->style->black_gc,
    drawing->pixmap,
    0, 0,
    0, 0,
    drawing->width + SAFETY, y);




  /* add an empty square */
  gdk_draw_rectangle (pixmap,
    drawing->drawing_area->style->black_gc,
    TRUE,
    0, y,
    drawing->width + SAFETY,  // do not overlap
    height);



  /* copy the bottom of the region */
  gdk_draw_drawable (pixmap,
    drawing->drawing_area->style->black_gc,
    drawing->pixmap,
    0, y,
    0, y + height,
    drawing->width+SAFETY, drawing->height - y);


  if(likely(drawing->pixmap))
    gdk_pixmap_unref(drawing->pixmap);

  drawing->pixmap = pixmap;
  
  if(unlikely(drawing->height==1)) drawing->height = height;
  else drawing->height += height;
  
  gtk_widget_set_size_request(drawing->drawing_area,
                             -1,
                             drawing->height);
  gtk_widget_queue_resize_no_redraw(drawing->drawing_area);
  
  /* ask for the buffer to be redrawn */
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               0, y,
                               drawing->width, drawing->height-y);
}


/* Remove a square corresponding to a removed process in the list */
void drawing_remove_square(Drawing_t *drawing,
        guint y,
        guint height)
{
  GdkPixmap *pixmap;

  if(unlikely((guint)drawing->height == height)) {
    pixmap = gdk_pixmap_new(
        drawing->drawing_area->window,
        drawing->width + SAFETY,
        1,
        -1);
    drawing->height=1;
  } else {
    /* Allocate a new pixmap with new height */
     pixmap = gdk_pixmap_new(
        drawing->drawing_area->window,
        drawing->width + SAFETY,
        drawing->height - height,
        -1);
   
    /* Copy the high region */
    gdk_draw_drawable (pixmap,
      drawing->drawing_area->style->black_gc,
      drawing->pixmap,
      0, 0,
      0, 0,
      drawing->width + SAFETY, y);

    /* Copy up the bottom of the region */
    gdk_draw_drawable (pixmap,
      drawing->drawing_area->style->black_gc,
      drawing->pixmap,
      0, y + height,
      0, y,
      drawing->width, drawing->height - y - height);

    drawing->height-=height;
  }

  if(likely(drawing->pixmap))
    gdk_pixmap_unref(drawing->pixmap);

  drawing->pixmap = pixmap;
  
  gtk_widget_set_size_request(drawing->drawing_area,
                             -1,
                             drawing->height);
  gtk_widget_queue_resize_no_redraw(drawing->drawing_area);
  /* ask for the buffer to be redrawn */
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               0, y,
                               drawing->width, MAX(drawing->height-y, 1));
}

void drawing_update_ruler(Drawing_t *drawing, TimeWindow *time_window)
{
  GtkRequisition req;
  GdkRectangle rect;
  
  req.width = drawing->ruler->allocation.width;
  req.height = drawing->ruler->allocation.height;

 
  rect.x = 0;
  rect.y = 0;
  rect.width = req.width;
  rect.height = req.height;

  gtk_widget_queue_draw(drawing->ruler);
  //gtk_widget_draw( drawing->ruler, &rect);
}

/* Redraw the ruler */
static gboolean
expose_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  Drawing_t *drawing = (Drawing_t*)user_data;
  TimeWindow time_window = lttvwindow_get_time_window(drawing->control_flow_data->tab);
  gchar text[255];
  
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *FontDesc;
  PangoRectangle ink_rect;
  gint global_width=0;
  GdkColor foreground = { 0, 0, 0, 0 };
  GdkColor background = { 0, 0xffff, 0xffff, 0xffff };

  LttTime window_end = time_window.end_time;
  LttTime half_width =
    ltt_time_div(time_window.time_width,2.0);
  LttTime window_middle =
    ltt_time_add(half_width,
                 time_window.start_time);
  g_debug("ruler expose event");
 
  gdk_draw_rectangle (drawing->ruler->window,
          drawing->ruler->style->white_gc,
          TRUE,
          event->area.x, event->area.y,
          event->area.width,
          event->area.height);

   GdkGC *gc = gdk_gc_new(drawing->ruler->window);
   gdk_gc_copy(gc, drawing->ruler->style->black_gc);
   gdk_gc_set_line_attributes(gc,
                               2,
                               GDK_LINE_SOLID,
                               GDK_CAP_BUTT,
                               GDK_JOIN_MITER);
  gdk_draw_line (drawing->ruler->window,
                  gc,
                  event->area.x, 1,
                  event->area.x + event->area.width, 1);


  snprintf(text, 255, "%lus\n%luns",
    time_window.start_time.tv_sec,
    time_window.start_time.tv_nsec);

  layout = gtk_widget_create_pango_layout(drawing->drawing_area, NULL);

  context = pango_layout_get_context(layout);
  FontDesc = pango_context_get_font_description(context);

  pango_font_description_set_size(FontDesc, 6*PANGO_SCALE);
  pango_layout_context_changed(layout);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_width += ink_rect.width;

  gdk_draw_layout_with_colors(drawing->ruler->window,
      gc,
      0,
      6,
      layout, &foreground, &background);

  gdk_gc_set_line_attributes(gc,
                             2,
                             GDK_LINE_SOLID,
                             GDK_CAP_ROUND,
                             GDK_JOIN_ROUND);

  gdk_draw_line (drawing->ruler->window,
                   gc,
                   1, 1,
                   1, 7);


  snprintf(text, 255, "%lus\n%luns", window_end.tv_sec,
                                     window_end.tv_nsec);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_width += ink_rect.width;

  if(global_width <= drawing->ruler->allocation.width)
  {
    gdk_draw_layout_with_colors(drawing->ruler->window,
      gc,
      drawing->ruler->allocation.width - ink_rect.width,
      6,
      layout, &foreground, &background);

    gdk_gc_set_line_attributes(gc,
                               2,
                               GDK_LINE_SOLID,
                               GDK_CAP_ROUND,
                               GDK_JOIN_ROUND);

    gdk_draw_line (drawing->ruler->window,
                   gc,
                   drawing->ruler->allocation.width-1, 1,
                   drawing->ruler->allocation.width-1, 7);
  }


  snprintf(text, 255, "%lus\n%luns", window_middle.tv_sec,
                                     window_middle.tv_nsec);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_width += ink_rect.width;

  if(global_width <= drawing->ruler->allocation.width)
  {
    gdk_draw_layout_with_colors(drawing->ruler->window,
      gc,
      (drawing->ruler->allocation.width - ink_rect.width)/2,
      6,
      layout, &foreground, &background);

    gdk_gc_set_line_attributes(gc,
                               2,
                               GDK_LINE_SOLID,
                               GDK_CAP_ROUND,
                               GDK_JOIN_ROUND);

    gdk_draw_line (drawing->ruler->window,
                   gc,
                   drawing->ruler->allocation.width/2, 1,
                   drawing->ruler->allocation.width/2, 7);




  }

  gdk_gc_unref(gc);
  g_object_unref(layout);
   
  return FALSE;
}


/* notify mouse on ruler */
static gboolean
motion_notify_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  //g_debug("motion");
  //eventually follow mouse and show time here
}
