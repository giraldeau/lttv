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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#include <ltt/trace.h>

#include <lttv/lttv.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/lttvwindow.h>
#include <lttv/state.h>
#include <lttv/hook.h>

#include "histodrawing.h"
#include "histoeventhooks.h"
#include "histocfv.h"

#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif
#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

//FIXME
// fixed #define TRACE_NUMBER 0
#define EXTRA_ALLOC 1024 // pixels
#define padding_width 50

#if 0 /* colors for two lines representation */
GdkColor histo_drawing_colors[NUM_COLORS] =
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


 GdkColor histo_drawing_colors[NUM_COLORS] =
{ /* Pixel, R, G, B */
  { 0, 0, 0, 0 }, /* COL_BLACK */
  { 0, 0xFFFF, 0xFFFF, 0xFFFF }, /* COL_WHITE */
  { 0, 0x0000, 0xFF00, 0x0000 }, /* COL_RUN_USER_MODE : green */
  { 0, 0x0100, 0x9E00, 0xFFFF }, /* COL_RUN_SYSCALL : pale blue */
  { 0, 0xFF00, 0xFF00, 0x0100 }, /* COL_RUN_TRAP : yellow */
  { 0, 0xFFFF, 0x5E00, 0x0000 }, /* COL_RUN_IRQ : red */
  { 0, 0x6600, 0x0000, 0x0000 }, /* COL_WAIT : dark red */
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
histo_expose_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data );

static gboolean
histo_expose_vertical_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data );

static gboolean
histo_motion_notify_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);

static gboolean
histo_motion_notify_vertical_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);

/* Function responsible for updating the exposed area.
 * It must do an events request to the lttvwindow API to ask for this update.
 * Note : this function cannot clear the background, because it may
 * erase drawing already present (SAFETY).
 */
void histo_drawing_data_request(histoDrawing_t *drawing,
      gint x, gint y,
      gint width,
      gint height)
{

}
 

void histo_drawing_data_request_begin(EventsRequest *events_request, LttvTracesetState *tss)
{
  g_debug("Begin of data request");
  HistoControlFlowData *cfd = events_request->viewer_data;
  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tss);
  TimeWindow time_window = 
    lttvwindow_get_time_window(cfd->tab);

  guint width = cfd->drawing->width;
  guint x=0;

  cfd->drawing->last_start = events_request->start_time;

  histo_convert_time_to_pixels(
          time_window,
          events_request->start_time,
          width,
          &x);

 }

void histo_drawing_chunk_begin(EventsRequest *events_request, LttvTracesetState *tss)
{
  g_debug("Begin of chunk");
  HistoControlFlowData *cfd = events_request->viewer_data;
  LttvTracesetContext *tsc = LTTV_TRACESET_CONTEXT(tss);

  if(cfd->chunk_has_begun) return;

  cfd->chunk_has_begun = TRUE;
}


void histo_drawing_request_expose(EventsRequest *events_request,
                            LttvTracesetState *tss,
                            LttTime end_time)
{
  HistoControlFlowData *cfd = events_request->viewer_data;
  histoDrawing_t *drawing = cfd->drawing;
   
  guint x, x_end, width;
  LttvTracesetContext *tsc = (LttvTracesetContext*)tss;
    
  TimeWindow time_window = 
        lttvwindow_get_time_window(cfd->tab);

  g_debug("histo request expose");
  
  histo_convert_time_to_pixels(
        time_window,
        end_time,
        drawing->width,
        &x_end);
  x = drawing->damage_begin;

  width = x_end - x;

  drawing->damage_begin = x+width;

  // FIXME ?
  //changed for histogram:
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               x, 0,
                               width,
			       drawing->/*drawing_area->allocation.*/height);
 
  // Update directly when scrolling 
  gdk_window_process_updates(drawing->drawing_area->window,
      TRUE);
}


/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
/* As the scaling will always change, it's of no use to copy old
 * pixmap.
 *
 * Change the size if width or height changes. 
 * (different from control flow viewer!)
 */
static gboolean
histo_configure_event( GtkWidget *widget, GdkEventConfigure *event, 
    gpointer user_data)
{
  histoDrawing_t *drawing = (histoDrawing_t*)user_data;

  /* First, get the new time interval of the main window */
  /* we assume (see documentation) that the main window
   * has updated the time interval before this configure gets
   * executed.
   */
  //lttvwindow_get_time_window(drawing->histo_control_flow_data->mw,
  //      &drawing->histo_control_flow_data->time_window);
  
  /* New pixmap, size of the configure event */
  //GdkPixmap *pixmap = gdk_pixmap_new(widget->window,
  //      widget->allocation.width + SAFETY,
  //      widget->allocation.height + SAFETY,
  //      -1);
    g_debug("drawing configure event");
    g_debug("New alloc draw size : %i by %i",widget->allocation.width,
                                    widget->allocation.height);

/*modified for histogram, if width is not changed, GArray is valid so
 just redraw, else recalculate all.(event request again)*/

//enabled again for histogram:
      if(drawing->pixmap)
        gdk_pixmap_unref(drawing->pixmap);

      drawing->pixmap = gdk_pixmap_new(widget->window,
                                       widget->allocation.width ,//+ SAFETY + EXTRA_ALLOC,
                                       widget->allocation.height + EXTRA_ALLOC,
                                       -1);

//end add
      drawing->alloc_width = drawing->width + SAFETY + EXTRA_ALLOC;
      drawing->alloc_height = drawing->height + EXTRA_ALLOC;
     
    //drawing->height = widget->allocation.height;

   

//     Clear the image
//    gdk_draw_rectangle (drawing->pixmap,
//          widget->style->black_gc,
//          TRUE,
//          0, 0,
//          drawing->width+SAFETY,
//          drawing->height);

    //g_info("init data request");


    /* Initial data request */
    /* no, do initial data request in the expose event */
    // Do not need to ask for data of 1 pixel : not synchronized with
    // main window time at this moment.
    //histo_drawing_data_request(drawing, &drawing->pixmap, 0, 0,
    //    widget->allocation.width,
    //    widget->allocation.height);
                          
    //drawing->width = widget->allocation.width;
    //drawing->height = widget->allocation.height;
  
    drawing->damage_begin = 0;
    drawing->damage_end = widget->allocation.width;
    
    if((widget->allocation.width != 1 &&
        widget->allocation.height != 1)
        /*&& drawing->damage_begin < drawing->damage_end */)
    {

      gdk_draw_rectangle (drawing->pixmap,
      drawing->drawing_area->style->black_gc,
      TRUE,
      0, 0,
      drawing->drawing_area->allocation.width, drawing->drawing_area->allocation.height);
 /*     histo_drawing_data_request(drawing,
                           drawing->damage_begin,
                           0,
                           drawing->damage_end - drawing->damage_begin,
                           drawing->height);*/
    }
//modified for histogram
       
  if(widget->allocation.width == drawing->width) 
  {
    
    drawing->height = widget->allocation.height;
    histogram_show( drawing->histo_control_flow_data,0,
	drawing->histo_control_flow_data->number_of_process->len);
  }
  else
  { 
    drawing->width = widget->allocation.width;
    drawing->height = widget->allocation.height;

    g_array_set_size (drawing->histo_control_flow_data->number_of_process,
                      	 widget->allocation.width);
    histo_request_event( drawing->histo_control_flow_data,drawing->damage_begin
			,drawing->damage_end - drawing->damage_begin);
  }
  return TRUE;
}


/* Redraw the screen from the backing pixmap */
static gboolean
histo_expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  histoDrawing_t *drawing = (histoDrawing_t*)user_data;

  HistoControlFlowData *histo_control_flow_data =
      (HistoControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "histo_control_flow_data");
#if 0
  if(unlikely(drawing->gc == NULL)) {
    drawing->gc = gdk_gc_new(drawing->drawing_area->window);
    gdk_gc_copy(drawing->gc, drawing->drawing_area->style->black_gc);
  }
#endif //0
  TimeWindow time_window = 
      lttvwindow_get_time_window(histo_control_flow_data->tab);
  LttTime current_time = 
      lttvwindow_get_current_time(histo_control_flow_data->tab);

  guint cursor_x=0;

  LttTime window_end = time_window.end_time;


  /* update the screen from the pixmap buffer */
//added again for histogram:

  gdk_draw_pixmap(widget->window,
      widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
      drawing->pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);
 //0

  drawing->height = drawing-> drawing_area ->allocation.height;

#if 0
  copy_pixmap_to_screen(histo_control_flow_data->process_list,
                        widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);
#endif //0


 /* //disabled for histogram
  copy_pixmap_to_screen(histo_control_flow_data->process_list,
                        widget->window,
                        drawing->gc,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);*/

  /* Erase the dotted lines left.. */
  if(widget->allocation.height > drawing->height)
  {
    gdk_draw_rectangle (widget->window,
      drawing->drawing_area->style->black_gc,
      TRUE,
      event->area.x, drawing->height,
      event->area.width,  // do not overlap
      widget->allocation.height - drawing->height);
  }
  if(ltt_time_compare(time_window.start_time, current_time) <= 0 &&
           ltt_time_compare(window_end, current_time) >= 0)
  {
    /* Draw the dotted lines */
    histo_convert_time_to_pixels(
          time_window,
          current_time,
          drawing->width,
          &cursor_x);

#if 0
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
#endif //0
    gint height_tot = MAX(widget->allocation.height, drawing->height);
    gdk_draw_line(widget->window,
                  drawing->dotted_gc,
                  cursor_x, 0,
                  cursor_x, height_tot);
  }
  
  return FALSE;
}

static gboolean
histo_after_expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  //g_assert(0);
  g_debug("AFTER EXPOSE");

  return FALSE;
}

/* mouse click */
static gboolean
histo_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
  HistoControlFlowData *histo_control_flow_data =
      (HistoControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "histo_control_flow_data");
  histoDrawing_t *drawing = histo_control_flow_data->drawing;
  TimeWindow time_window =
               lttvwindow_get_time_window(histo_control_flow_data->tab);

  g_debug("click");
  if(event->button == 1)
  {
    LttTime time;

    /* left mouse button click */
    g_debug("x click is : %f", event->x);

    histo_convert_pixels_to_time(drawing->width, (guint)event->x,
        time_window,
        &time);

    lttvwindow_report_current_time(histo_control_flow_data->tab, time);
    ////report event->y for vertical zoom +,-
  }

  return FALSE;
}
/*
 //Viewer's vertical scroll bar is already omitted, not needed for histogram.
static gboolean
scrollbar_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  histoDrawing_t *drawing = (histoDrawing_t*)user_data;

  gtk_widget_set_size_request(drawing->padding, allocation->width, -1);
  //gtk_widget_queue_resize(drawing->padding);
  //gtk_widget_queue_resize(drawing->ruler);
  gtk_container_check_resize(GTK_CONTAINER(drawing->ruler_hbox));
  return 0;
}
*/


histoDrawing_t *histo_drawing_construct(HistoControlFlowData *histo_control_flow_data)
{
  histoDrawing_t *drawing = g_new(histoDrawing_t, 1);
  
  drawing->histo_control_flow_data = histo_control_flow_data;

  drawing->vbox = gtk_vbox_new(FALSE, 1);

  
  drawing->ruler_hbox = gtk_hbox_new(FALSE, 1);
  drawing->ruler = gtk_drawing_area_new ();
  //gtk_widget_set_size_request(drawing->ruler, -1, 27);
  
  drawing->padding = gtk_drawing_area_new ();
  //gtk_widget_set_size_request(drawing->padding, -1, 27);
   
  gtk_box_pack_start(GTK_BOX(drawing->ruler_hbox), drawing->padding,FALSE, FALSE, 0);
  
  gtk_box_pack_end(GTK_BOX(drawing->ruler_hbox), drawing->ruler, 
                     TRUE, TRUE, 0);

  drawing->drawing_area = gtk_drawing_area_new ();
  
  drawing->gc = NULL;
  /* 
  ///at this time not necessary for histogram
  drawing->hbox = gtk_hbox_new(FALSE, 1);
 
  drawing->viewport = gtk_viewport_new(NULL, histo_control_flow_data->v_adjust);
  drawing->scrollbar = gtk_vscrollbar_new(histo_control_flow_data->v_adjust);
  gtk_box_pack_start(GTK_BOX(drawing->hbox), drawing->viewport, 
                     TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->hbox), drawing->scrollbar, 
                     FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(drawing->viewport),
                    drawing->drawing_area);*/

  //add vertical ruler:
  drawing->vruler_drawing_hbox = gtk_hbox_new(FALSE, 1);
  drawing-> vertical_ruler =gtk_drawing_area_new ();
  gtk_box_pack_start(GTK_BOX(drawing->vruler_drawing_hbox), drawing->vertical_ruler, 
                     	FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->vruler_drawing_hbox), drawing->drawing_area,
                   	TRUE, TRUE, 1);
  gtk_widget_set_size_request(drawing->vertical_ruler, padding_width, -1);

  gtk_box_pack_start(GTK_BOX(drawing->vbox), drawing->ruler_hbox, 
                     	FALSE, FALSE, 1);
  gtk_box_pack_end(GTK_BOX(drawing->vbox), drawing->vruler_drawing_hbox/*drawing_area*/,
                   	TRUE, TRUE, 1);
  
  drawing->pango_layout =
    	gtk_widget_create_pango_layout(drawing->drawing_area, NULL);

  drawing->height = 1;
  drawing->width = 1;
  drawing->depth = 0;
  drawing->alloc_height = 1;
  drawing->alloc_width = 1;
  
  drawing->damage_begin = 0;
  drawing->damage_end = 0;
  drawing->horizontal_sel = -1;
  
  //gtk_widget_set_size_request(drawing->drawing_area->window, 50, 50);
  g_object_set_data_full(
      G_OBJECT(drawing->drawing_area),
      "histo_Link_drawing_Data",
      drawing,
      (GDestroyNotify)histo_drawing_destroy);

  g_object_set_data(
      G_OBJECT(drawing->ruler),
      "histo_drawing",
      drawing);

  g_object_set_data(
      G_OBJECT(drawing->vertical_ruler),
      "histo_drawing",
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
        G_CALLBACK (histo_configure_event),
        (gpointer)drawing);
 
  g_signal_connect (G_OBJECT(drawing->ruler),
        "expose_event",
        G_CALLBACK(histo_expose_ruler),
        (gpointer)drawing);

  gtk_widget_add_events(drawing->ruler, GDK_POINTER_MOTION_MASK);
  gtk_widget_add_events(drawing->vertical_ruler, GDK_POINTER_MOTION_MASK);

  g_signal_connect (G_OBJECT(drawing->ruler),
        "motion-notify-event",
        G_CALLBACK(histo_motion_notify_ruler),
        (gpointer)drawing);

 
  g_signal_connect (G_OBJECT(drawing->vertical_ruler),
        "expose_event",
        G_CALLBACK(histo_expose_vertical_ruler),
        (gpointer)drawing);

  g_signal_connect (G_OBJECT(drawing->vertical_ruler),
        "motion-notify-event",
        G_CALLBACK(histo_motion_notify_vertical_ruler),
        (gpointer)drawing);

/*//not necessary for historam. 
  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "size-allocate",
        G_CALLBACK(scrollbar_size_allocate),
        (gpointer)drawing); */


  gtk_widget_set_size_request(drawing->padding, padding_width, -1);//use it for vertical ruler

  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "expose_event",
        G_CALLBACK (histo_expose_event),
        (gpointer)drawing);

  g_signal_connect_after (G_OBJECT(drawing->drawing_area),
        "expose_event",
        G_CALLBACK (histo_after_expose_event),
        (gpointer)drawing);

  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "button-press-event",
        G_CALLBACK (histo_button_press_event),
        (gpointer)drawing);
  
  gtk_widget_show(drawing->ruler);
  gtk_widget_show(drawing->padding);
  gtk_widget_show(drawing->ruler_hbox);
  gtk_widget_show(drawing->vertical_ruler);
  gtk_widget_show(drawing->vruler_drawing_hbox);
  gtk_widget_show(drawing->drawing_area);
  
 /// gtk_widget_show(drawing->viewport);
 /// gtk_widget_show(drawing->scrollbar);
 /// gtk_widget_show(drawing->hbox);

  /* Allocate the colors */
  GdkColormap* colormap = gdk_colormap_get_system();
  gboolean success[NUM_COLORS];
  gdk_colormap_alloc_colors(colormap, histo_drawing_colors, NUM_COLORS, FALSE,
                            TRUE, success);
  
  drawing->gc =
    gdk_gc_new(GDK_DRAWABLE(main_window_get_widget(histo_control_flow_data->tab)->window));
  drawing->dotted_gc =
    gdk_gc_new(GDK_DRAWABLE(main_window_get_widget(histo_control_flow_data->tab)->window));

  gdk_gc_copy(drawing->gc,
      main_window_get_widget(histo_control_flow_data->tab)->style->black_gc);
  gdk_gc_copy(drawing->dotted_gc,
      main_window_get_widget(histo_control_flow_data->tab)->style->white_gc);
  
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

  drawing->ruler_gc_butt = 
    gdk_gc_new(GDK_DRAWABLE(main_window_get_widget(histo_control_flow_data->tab)->window));
  gdk_gc_copy(drawing->ruler_gc_butt, 
      main_window_get_widget(histo_control_flow_data->tab)->style->black_gc);
  drawing->ruler_gc_round = 
    gdk_gc_new(GDK_DRAWABLE(main_window_get_widget(histo_control_flow_data->tab)->window));
  gdk_gc_copy(drawing->ruler_gc_round, 
      main_window_get_widget(histo_control_flow_data->tab)->style->black_gc);


  gdk_gc_set_line_attributes(drawing->ruler_gc_butt,
                               2,
                               GDK_LINE_SOLID,
                               GDK_CAP_BUTT,
                               GDK_JOIN_MITER);

  gdk_gc_set_line_attributes(drawing->ruler_gc_round,
                             2,
                             GDK_LINE_SOLID,
                             GDK_CAP_ROUND,
                             GDK_JOIN_ROUND);
 return drawing;
}

void histo_drawing_destroy(histoDrawing_t *drawing)
{
  g_info("histo_drawing_destroy %p", drawing);

  /* Free the colors */
  GdkColormap* colormap = gdk_colormap_get_system();

  gdk_colormap_free_colors(colormap, histo_drawing_colors, NUM_COLORS);
  
  // Do not unref here, histoDrawing_t destroyed by it's widget.
  //g_object_unref( G_OBJECT(drawing->drawing_area));
  if(drawing->gc != NULL)
    gdk_gc_unref(drawing->gc);
  
  g_object_unref(drawing->pango_layout);
  if(drawing->dotted_gc != NULL) gdk_gc_unref(drawing->dotted_gc);
  if(drawing->ruler_gc_butt != NULL) gdk_gc_unref(drawing->ruler_gc_butt);
  if(drawing->ruler_gc_round != NULL) gdk_gc_unref(drawing->ruler_gc_round);

  //added for histogram
  if(drawing->pixmap)
        gdk_pixmap_unref(drawing->pixmap);
  g_free(drawing);
  g_info("histo_drawing_destroy end");
}

 GtkWidget *histo_drawing_get_drawing_area(histoDrawing_t *drawing)
{
  return drawing->drawing_area;
}

 GtkWidget *histo_drawing_get_widget(histoDrawing_t *drawing)
{
  return drawing->vbox;
}

 void histo_drawing_draw_line( histoDrawing_t *drawing,
      GdkPixmap *pixmap,
      guint x1, guint y1,
      guint x2, guint y2,
      GdkGC *GC)
{
  gdk_draw_line (pixmap,
      GC,
      x1, y1, x2, y2);
}

void histo_drawing_clear(histoDrawing_t *drawing,guint clear_from,guint clear_to)
{ 
  
  HistoControlFlowData *cfd = drawing->histo_control_flow_data;
  guint clear_width = clear_to- clear_from;
 /* 
  //disabled for histogram
  rectangle_pixmap(cfd->process_list,
      drawing->drawing_area->style->black_gc,
      TRUE,
      0, 0,
      drawing->alloc_width,  // do not overlap
      -1);*/
  //instead, this is added for histogram

   histo_rectangle_pixmap (drawing->drawing_area->style->black_gc,
          TRUE,
          clear_from/*0*/, 0,
          clear_width/*drawing->width*/,
          -1,drawing);


   
/*  gdk_draw_rectangle (drawing->pixmap,
      drawing->drawing_area->style->black_gc,
      TRUE,
      0,0,
      drawing->drawing_area->allocation.width,drawing->drawing_area->allocation.height );
*/
   
  /* ask for the buffer to be redrawn */
//enabled again for histogram.
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               clear_from, 0,
                               clear_width, drawing->height);
  gdk_window_process_updates(drawing->drawing_area->window,TRUE);
//disabled instead for histogram
  //gtk_widget_queue_draw ( drawing->drawing_area);
  return;
}

#if 0
/* Insert a square corresponding to a new process in the list */
/* Applies to whole drawing->width */
void drawing_insert_square(histoDrawing_t *drawing,
        guint y,
        guint height)
{
  //GdkRectangle update_rect;
  gboolean reallocate = FALSE;
  GdkPixmap *new_pixmap;

  /* Allocate a new pixmap with new height */
  if(drawing->alloc_height < drawing->height + height) {

    new_pixmap = gdk_pixmap_new(drawing->drawing_area->window,
                                     drawing->width + SAFETY + EXTRA_ALLOC,
                                     drawing->height + height + EXTRA_ALLOC,
                                     -1);
    drawing->alloc_width = drawing->width + SAFETY + EXTRA_ALLOC;
    drawing->alloc_height = drawing->height + height + EXTRA_ALLOC;
    reallocate = TRUE;

    /* Copy the high region */
    gdk_draw_pixmap (new_pixmap,
      drawing->drawing_area->style->black_gc,
      drawing->pixmap,
      0, 0,
      0, 0,
      drawing->width + SAFETY, y);

  } else {
    new_pixmap = drawing->pixmap;
  }

  //GdkPixmap *pixmap = gdk_pixmap_new(drawing->drawing_area->window,
  //      drawing->width + SAFETY,
  //      drawing->height + height,
  //      -1);
  
  /* add an empty square */
  gdk_draw_rectangle (new_pixmap,
    drawing->drawing_area->style->black_gc,
    TRUE,
    0, y,
    drawing->width + SAFETY,  // do not overlap
    height);

  /* copy the bottom of the region */
  gdk_draw_pixmap (new_pixmap,
    drawing->drawing_area->style->black_gc,
    drawing->pixmap,
    0, y,
    0, y + height,
    drawing->width+SAFETY, drawing->height - y);


  if(reallocate && likely(drawing->pixmap)) {
    gdk_pixmap_unref(drawing->pixmap);
    drawing->pixmap = new_pixmap;
  }
  
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
void drawing_remove_square(histoDrawing_t *drawing,
        guint y,
        guint height)
{
  GdkPixmap *pixmap;

  if(unlikely((guint)drawing->height == height)) {
    //pixmap = gdk_pixmap_new(
    //    drawing->drawing_area->window,
    //    drawing->width + SAFETY,
    //    1,
    //    -1);
    pixmap = drawing->pixmap;
    drawing->height=1;
  } else {
    /* Allocate a new pixmap with new height */
     //pixmap = gdk_pixmap_new(
     //   drawing->drawing_area->window,
     //   drawing->width + SAFETY,
     //   drawing->height - height,
     //   -1);
     /* Keep the same preallocated pixmap */
    pixmap = drawing->pixmap;
   
    /* Copy the high region */
    gdk_draw_pixmap (pixmap,
      drawing->drawing_area->style->black_gc,
      drawing->pixmap,
      0, 0,
      0, 0,
      drawing->width + SAFETY, y);

    /* Copy up the bottom of the region */
    gdk_draw_pixmap (pixmap,
      drawing->drawing_area->style->black_gc,
      drawing->pixmap,
      0, y + height,
      0, y,
      drawing->width, drawing->height - y - height);

    drawing->height-=height;
  }

  //if(likely(drawing->pixmap))
  //  gdk_pixmap_unref(drawing->pixmap);

  //drawing->pixmap = pixmap;
  
  gtk_widget_set_size_request(drawing->drawing_area,
                             -1,
                             drawing->height);
  gtk_widget_queue_resize_no_redraw(drawing->drawing_area);
  /* ask for the buffer to be redrawn */
  gtk_widget_queue_draw_area ( drawing->drawing_area,
                               0, y,
                               drawing->width, MAX(drawing->height-y, 1));
}
#endif //0

void histo_drawing_update_ruler(histoDrawing_t *drawing, TimeWindow *time_window)
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
histo_expose_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  histoDrawing_t *drawing = (histoDrawing_t*)user_data;
  TimeWindow time_window = lttvwindow_get_time_window(drawing->histo_control_flow_data->tab);
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

  gdk_draw_line (drawing->ruler->window,
                  drawing->ruler_gc_butt,
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
      drawing->ruler_gc_butt,
      0,
      6,
      layout, &foreground, &background);

  gdk_draw_line (drawing->ruler->window,
                   drawing->ruler_gc_round,
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
      drawing->ruler_gc_butt,
      drawing->ruler->allocation.width - ink_rect.width,
      6,
      layout, &foreground, &background);

    gdk_draw_line (drawing->ruler->window,
                   drawing->ruler_gc_butt,
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
      drawing->ruler_gc_butt,
      (drawing->ruler->allocation.width - ink_rect.width)/2,
      6,
      layout, &foreground, &background);

    gdk_draw_line (drawing->ruler->window,
                   drawing->ruler_gc_butt,
                   drawing->ruler->allocation.width/2, 1,
                   drawing->ruler->allocation.width/2, 7);
  }

  g_object_unref(layout);
   
  return FALSE;
}

 void histo_drawing_update_vertical_ruler(histoDrawing_t *drawing)//, TimeWindow *time_window)
{
  GtkRequisition req;
  GdkRectangle rect;
  
  req.width = drawing->vertical_ruler->allocation.width;
  req.height = drawing->vertical_ruler->allocation.height;
 
  rect.x = 0;
  rect.y = 0;
  rect.width = req.width;
  rect.height = req.height;

  gtk_widget_queue_draw(drawing->vertical_ruler);
  //gtk_widget_draw( drawing->ruler, &rect);
}

/* notify mouse on ruler */
static gboolean
histo_motion_notify_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  //g_debug("motion");
  //eventually follow mouse and show time here
	return FALSE;
}

static gboolean
histo_motion_notify_vertical_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  //g_debug("motion");
  //eventually follow mouse and show time here
	return FALSE;
}



/* Redraw the vertical ruler */
static gboolean
histo_expose_vertical_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  histoDrawing_t *drawing = (histoDrawing_t*)user_data;
  HistoControlFlowData *histo_cfv = drawing->histo_control_flow_data;
  gchar text[255];
  
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *FontDesc;
  PangoRectangle ink_rect;
  gint global_height=0;
  GdkColor foreground = { 0, 0, 0, 0 };
  GdkColor background = { 0, 0xffff, 0xffff, 0xffff };
  GdkColor red ={ 0, 0xFFFF, 0x1E00, 0x1000 };
  GdkColor magneta ={ 0, 0x8900, 0x0000, 0x8400 };
  g_debug("vertical ruler expose event");
 
  gdk_draw_rectangle (drawing->vertical_ruler->window,
          drawing->vertical_ruler->style->white_gc,
          TRUE,
          event->area.x, event->area.y,
          event->area.width,
          event->area.height);

  gdk_draw_line (drawing->vertical_ruler->window,
                  drawing->ruler_gc_butt,
                  padding_width-1/*event->area.width-1*/,event->area.y,
                  padding_width-1/*event->area.width-1*/,event->area.y + event->area.height);

  snprintf(text, 255, "%.1f", (float)histo_cfv->max_height);

  layout = gtk_widget_create_pango_layout(drawing->drawing_area, NULL);

  context = pango_layout_get_context(layout);
  FontDesc = pango_context_get_font_description(context);

  pango_font_description_set_size(FontDesc, 6*PANGO_SCALE);
  pango_layout_context_changed(layout);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_height += ink_rect.height;

  gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      drawing->ruler_gc_butt,
      1,
      1,
      layout, &foreground, &background);

  gdk_draw_line (drawing->vertical_ruler->window,
                   drawing->ruler_gc_round,
                   drawing->vertical_ruler-> allocation.width-1, 1,
                   drawing->vertical_ruler-> allocation.width-7, 1);


  snprintf(text, 255, "%d", 0);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_height += ink_rect.height;

  if(global_height <= drawing->vertical_ruler->allocation.height)
  {
    gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      drawing->ruler_gc_butt,
      1,
      drawing->vertical_ruler->allocation.height - ink_rect.height-2,
      layout, &foreground, &background);

    gdk_draw_line (drawing->vertical_ruler->window,
                    drawing->ruler_gc_butt,
                    drawing->vertical_ruler-> allocation.width-1,
		    drawing->vertical_ruler->allocation.height-1,
                    drawing->vertical_ruler-> allocation.width-7,
  		    drawing->vertical_ruler->allocation.height-1);
  }


  snprintf(text, 255, "%.1f",(float) histo_cfv->max_height/2.0);

  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  global_height += ink_rect.height;

  if(global_height <= drawing->vertical_ruler->allocation.height)
  {
    gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      drawing->ruler_gc_butt,
      1,
      (drawing->vertical_ruler->allocation.height - ink_rect.height)/2,
      layout, &foreground, &background);

    gdk_draw_line (drawing->vertical_ruler->window,
                   drawing->ruler_gc_butt,
                   drawing->vertical_ruler-> allocation.width-1,
		   drawing->vertical_ruler-> allocation.height/2,
                   drawing->vertical_ruler-> allocation.width-7,
		   drawing->vertical_ruler->allocation.height/2);
  }

  //show number of events at current time:
  LttTime current_time = 
      lttvwindow_get_current_time(histo_cfv->tab);
  TimeWindow time_window =
            lttvwindow_get_time_window(histo_cfv->tab);
  LttTime time_begin = time_window.start_time;
  LttTime time_width = time_window.time_width;
  LttTime time_end = ltt_time_add(time_begin, time_width);
  if((ltt_time_compare(current_time, time_begin) >= 0)&&
	(ltt_time_compare(current_time, time_end) <= 0))
  {
     guint *events_at_currenttime;
     guint max_height=histo_cfv ->max_height;
     guint x;
     histo_convert_time_to_pixels(
                    time_window,
                    current_time,
                    drawing->width,
                    &x);
  //   if(x_test<histo_cfv->number_of_process->len)

     {
     	events_at_currenttime = 
		&g_array_index(histo_cfv->number_of_process,guint,x);


	    if((*events_at_currenttime) > max_height)
	    {	
		snprintf(text, 255, "OverFlow!");
	    	pango_layout_set_text(layout, text, -1);
  		pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  		global_height += ink_rect.height;
		gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				(drawing->vertical_ruler->allocation.height - ink_rect.height)/5, 
      				layout, &red, &background);
	    }else
  	//    if((*events_at_currenttime) <= max_height)
	    {
  		snprintf(text, 255, "%.1f",
			(float) *events_at_currenttime);

  		pango_layout_set_text(layout, text, -1);
  		pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
  		global_height += ink_rect.height;

		if ((*events_at_currenttime) == 0)
		{
  			gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				(drawing->vertical_ruler->allocation.height - ink_rect.height)-2, 
      			layout, &red, &background);
		}
		else if ((*events_at_currenttime) == max_height)
		{
  			gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				1, 
      			layout, &red, &background);
		}
		/*else if ((*events_at_currenttime) == max_height/2) 
		{
  			gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				(drawing->vertical_ruler->allocation.height - ink_rect.height)/2, 
      			layout, &red, &background);
		}*/
		else if ((*events_at_currenttime) > max_height/2) 
        	{
  			gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				(drawing->vertical_ruler->allocation.height - ink_rect.height)/4, 
      			layout, &red, &background);
		}
		else{
			gdk_draw_layout_with_colors(drawing->vertical_ruler->window,
      				drawing->ruler_gc_butt,
      				1,
      				((drawing->vertical_ruler->allocation.height 
					- ink_rect.height)*3)/4, 	
      				layout, &red, &background);
		}
	    }
	    
     	}
   }

  g_object_unref(layout);
   
  return FALSE;
}

