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

#include <lttv/lttv.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/gtktraceset.h>
#include <lttv/hook.h>

#include "drawing.h"
#include "cfv.h"
#include "cfv-private.h"
#include "eventhooks.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

/*****************************************************************************
 *                              drawing functions                            *
 *****************************************************************************/

static gboolean
expose_ruler( GtkWidget *widget, GdkEventExpose *event, gpointer user_data );

static gboolean
motion_notify_ruler(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);


//FIXME Colors will need to be dynamic. Graphic context part not done so far.
typedef enum 
{
  RED,
  GREEN,
  BLUE,
  WHITE,
  BLACK

} ControlFlowColors;

/* Vector of unallocated colors */
static GdkColor CF_Colors [] = 
{
  { 0, 0xffff, 0x0000, 0x0000 },  // RED
  { 0, 0x0000, 0xffff, 0x0000 },  // GREEN
  { 0, 0x0000, 0x0000, 0xffff },  // BLUE
  { 0, 0xffff, 0xffff, 0xffff },  // WHITE
  { 0, 0x0000, 0x0000, 0x0000 } // BLACK
};


/* Function responsible for updating the exposed area.
 * It must call processTrace() to ask for this update.
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
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(
                    drawing->drawing_area),
                "control_flow_data");

  LttTime start, end;
  LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                        control_flow_data->time_window.start_time);

  g_debug("req : window_end : %u, %u", window_end.tv_sec, 
                                      window_end.tv_nsec);

  g_debug("req : time width : %u, %u", control_flow_data->time_window.time_width.tv_sec, 
                                control_flow_data->time_window.time_width.tv_nsec);
  
  g_debug("x is : %i, x+width is : %i", x, x+width);

  convert_pixels_to_time(drawing->drawing_area->allocation.width, x,
        &control_flow_data->time_window.start_time,
        &window_end,
        &start);

  convert_pixels_to_time(drawing->drawing_area->allocation.width, x + width,
        &control_flow_data->time_window.start_time,
        &window_end,
        &end);
  
  LttvTracesetContext * tsc =
        get_traceset_context(control_flow_data->mw);
  LttvTracesetState * tss =
        (LttvTracesetState*)tsc;
  
    //send_test_process(
  //guicontrolflow_get_process_list(drawing->control_flow_data),
  //drawing);
  //send_test_drawing(
  //guicontrolflow_get_process_list(drawing->control_flow_data),
  //drawing, *pixmap, x, y, width, height);
  
  // Let's call processTrace() !!
  EventRequest event_request; // Variable freed at the end of the function.
  event_request.control_flow_data = control_flow_data;
  event_request.time_begin = start;
  event_request.time_end = end;
  event_request.x_begin = x;
  event_request.x_end = x+width;

  g_debug("req : start : %u, %u", event_request.time_begin.tv_sec, 
                                      event_request.time_begin.tv_nsec);

  g_debug("req : end : %u, %u", event_request.time_end.tv_sec, 
                                      event_request.time_end.tv_nsec);
  
  LttvHooks *event = lttv_hooks_new();
  LttvHooks *after_event = lttv_hooks_new();
  LttvHooks *after_traceset = lttv_hooks_new();
  lttv_hooks_add(after_traceset, after_data_request, &event_request);
  lttv_hooks_add(event, draw_event_hook, &event_request);
  //Modified by xiangxiu: state update hooks are added by the main window
  //state_add_event_hooks_api(control_flow_data->mw);
  lttv_hooks_add(after_event, draw_after_hook, &event_request);

  //lttv_process_traceset_seek_time(tsc, start);
  lttv_state_traceset_seek_time_closest(tss, start);
  // FIXME : would like to place the after_traceset hook after the traceset,
  // but the traceset context state is not valid anymore.
  lttv_traceset_context_add_hooks(tsc,
      NULL, after_traceset, NULL, NULL, NULL, NULL,
      //NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, event, after_event);
  lttv_process_traceset(tsc, end, G_MAXULONG);
  //after_data_request((void*)&event_request,(void*)tsc);
  lttv_traceset_context_remove_hooks(tsc,
      NULL, after_traceset, NULL, NULL, NULL, NULL,
     // NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, event, after_event);
  //Modified by xiangxiu: state update hooks are removed by the main window
  //state_remove_event_hooks_api(control_flow_data->mw);

  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(event);
  lttv_hooks_destroy(after_event);

  
}
          
/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
/* As the scaling will always change, it's of no use to copy old
 * pixmap.
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
  get_time_window(drawing->control_flow_data->mw,
        &drawing->control_flow_data->time_window);
  
  /* New pixmap, size of the configure event */
  //GdkPixmap *pixmap = gdk_pixmap_new(widget->window,
  //      widget->allocation.width + SAFETY,
  //      widget->allocation.height + SAFETY,
  //      -1);
  
  g_debug("drawing configure event");
  g_debug("New draw size : %i by %i",widget->allocation.width, widget->allocation.height);
  
    
  if (drawing->pixmap)
    gdk_pixmap_unref(drawing->pixmap);
  
  /* If no old pixmap present */
  //if(drawing->pixmap == NULL)
  {
    drawing->pixmap = gdk_pixmap_new(
    widget->window,
    widget->allocation.width + SAFETY,
    widget->allocation.height + SAFETY,
    //ProcessList_get_height
    // (GuiControlFlow_get_process_list(drawing->control_flow_data)),
    -1);
    drawing->width = widget->allocation.width;
    drawing->height = widget->allocation.height;
    

    // Clear the image
    gdk_draw_rectangle (drawing->pixmap,
          widget->style->black_gc,
          TRUE,
          0, 0,
          widget->allocation.width+SAFETY,
          widget->allocation.height+SAFETY);

    //g_info("init data request");


    /* Initial data request */
    // Do not need to ask for data of 1 pixel : not synchronized with
    // main window time at this moment.
    drawing_data_request(drawing, &drawing->pixmap, 0, 0,
        widget->allocation.width,
        widget->allocation.height);
                          
    drawing->width = widget->allocation.width;
    drawing->height = widget->allocation.height;

    return TRUE;



  }
#ifdef NOTUSE
//  /* Draw empty background */ 
//  gdk_draw_rectangle (pixmap,
//          widget->style->black_gc,
//          TRUE,
//          0, 0,
//          widget->allocation.width,
//          widget->allocation.height);
  
  /* Copy old data to new pixmap */
  gdk_draw_drawable (pixmap,
    widget->style->black_gc,
    drawing->pixmap,
    0, 0,
    0, 0,
    -1, -1);
    
  if (drawing->pixmap)
    gdk_pixmap_unref(drawing->pixmap);

  drawing->pixmap = pixmap;
    
  // Clear the bottom part of the image (SAFETY)
  gdk_draw_rectangle (pixmap,
          widget->style->black_gc,
          TRUE,
          0, drawing->height+SAFETY,
          drawing->width+SAFETY,  // do not overlap
          (widget->allocation.height) - drawing->height);

  // Clear the right part of the image (SAFETY)
  gdk_draw_rectangle (pixmap,
          widget->style->black_gc,
          TRUE,
          drawing->width+SAFETY, 0,
          (widget->allocation.width) - drawing->width,  // do not overlap
          drawing->height+SAFETY);

  /* Clear the backgound for data request, but not SAFETY */
  gdk_draw_rectangle (pixmap,
          drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->width + SAFETY, 0,
          widget->allocation.width - drawing->width,  // do not overlap
          widget->allocation.height+SAFETY);

  /* Request data for missing space */
  g_info("missing data request");
  drawing_data_request(drawing, &pixmap, drawing->width, 0,
      widget->allocation.width - drawing->width,
      widget->allocation.height);
                          
  drawing->width = widget->allocation.width;
  drawing->height = widget->allocation.height;

  return TRUE;
#endif //NOTUSE
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

  g_debug("drawing expose event");
  
  guint x=0;
  LttTime* current_time = 
      guicontrolflow_get_current_time(control_flow_data);

  LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                      control_flow_data->time_window.start_time);

  convert_time_to_pixels(
        control_flow_data->time_window.start_time,
        window_end,
        *current_time,
        widget->allocation.width,
        &x);
  
  gdk_draw_pixmap(widget->window,
      widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
      drawing->pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);

  if(x >= event->area.x && x <= event->area.x+event->area.width)
  {
    gint8 dash_list[] = { 1, 2 };
    GdkGC *gc = gdk_gc_new(control_flow_data->drawing->pixmap);
    gdk_gc_copy(gc, widget->style->white_gc);
    gdk_gc_set_line_attributes(gc,
                               1,
                               GDK_LINE_ON_OFF_DASH,
                               GDK_CAP_BUTT,
                               GDK_JOIN_MITER);
    gdk_gc_set_dashes(gc,
                      0,
                      dash_list,
                      2);
    drawing_draw_line(NULL, widget->window,
                  x, event->area.y,
                  x, event->area.y+event->area.height,
                  gc);
    gdk_gc_unref(gc);
  }
  return FALSE;
}

/* mouse click */
static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "control_flow_data");
  Drawing_t *drawing = control_flow_data->drawing;


  g_debug("click");
  if(event->button == 1)
  {
    LttTime time;

    LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                        control_flow_data->time_window.start_time);


    /* left mouse button click */
    g_debug("x click is : %f", event->x);

    convert_pixels_to_time(widget->allocation.width, (guint)event->x,
        &control_flow_data->time_window.start_time,
        &window_end,
        &time);

    set_current_time(control_flow_data->mw, &time);

  }

  set_focused_pane(control_flow_data->mw, gtk_widget_get_parent(control_flow_data->scrolled_window));
  
  return FALSE;
}




Drawing_t *drawing_construct(ControlFlowData *control_flow_data)
{
  Drawing_t *drawing = g_new(Drawing_t, 1);
  
  drawing->control_flow_data = control_flow_data;

  drawing->vbox = gtk_vbox_new(FALSE, 1);
  drawing->ruler = gtk_drawing_area_new ();
  gtk_widget_set_size_request(drawing->ruler, -1, 27);
  
  drawing->drawing_area = gtk_drawing_area_new ();

  gtk_box_pack_start(GTK_BOX(drawing->vbox), drawing->ruler, 
                     FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(drawing->vbox), drawing->drawing_area,
                   TRUE, TRUE, 0);
  
  drawing->pango_layout =
    gtk_widget_create_pango_layout(drawing->drawing_area, NULL);
  
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

  gtk_widget_add_events(drawing->drawing_area, GDK_BUTTON_PRESS_MASK);
  
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


  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "expose_event",
        G_CALLBACK (expose_event),
        (gpointer)drawing);

  g_signal_connect (G_OBJECT(drawing->drawing_area),
        "button-press-event",
        G_CALLBACK (button_press_event),
        (gpointer)drawing);
  
  gtk_widget_show(drawing->ruler);
  gtk_widget_show(drawing->drawing_area);
    
  
  return drawing;
}

void drawing_destroy(Drawing_t *drawing)
{

  // Do not unref here, Drawing_t destroyed by it's widget.
  //g_object_unref( G_OBJECT(drawing->drawing_area));
    
  g_free(drawing->pango_layout);
  g_free(drawing);
}

GtkWidget *drawing_get_drawing_area(Drawing_t *drawing)
{
  return drawing->drawing_area;
}

GtkWidget *drawing_get_widget(Drawing_t *drawing)
{
  return drawing->vbox;
}

/* convert_pixels_to_time
 *
 * Convert from window pixel and time interval to an absolute time.
 */
void convert_pixels_to_time(
    gint width,
    guint x,
    LttTime *window_time_begin,
    LttTime *window_time_end,
    LttTime *time)
{
  LttTime window_time_interval;
  
  window_time_interval = ltt_time_sub(*window_time_end, 
            *window_time_begin);
  *time = ltt_time_mul(window_time_interval, (x/(float)width));
  *time = ltt_time_add(*window_time_begin, *time);
}



void convert_time_to_pixels(
    LttTime window_time_begin,
    LttTime window_time_end,
    LttTime time,
    int width,
    guint *x)
{
  LttTime window_time_interval;
  float interval_float, time_float;
  
  window_time_interval = ltt_time_sub(window_time_end,window_time_begin);
  
  time = ltt_time_sub(time, window_time_begin);
  
  interval_float = ltt_time_to_double(window_time_interval);
  time_float = ltt_time_to_double(time);

  *x = (guint)(time_float/interval_float * width);
  
}

void drawing_refresh (  Drawing_t *drawing,
      guint x, guint y,
      guint width, guint height)
{
  g_info("Drawing.c : drawing_refresh %u, %u, %u, %u", x, y, width, height);
  GdkRectangle update_rect;

  gdk_draw_drawable(
    drawing->drawing_area->window,
    drawing->drawing_area->
     style->fg_gc[GTK_WIDGET_STATE (drawing->drawing_area)],
    GDK_DRAWABLE(drawing->pixmap),
    x, y,
    x, y,
    width, height);

  update_rect.x = 0 ;
  update_rect.y = 0 ;
  update_rect.width = drawing->width;
  update_rect.height = drawing->height ;
  gtk_widget_draw( drawing->drawing_area, &update_rect);

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




void drawing_resize(Drawing_t *drawing, guint h, guint w)
{
  drawing->height = h ;
  drawing->width = w ;

  gtk_widget_set_size_request ( drawing->drawing_area,
          drawing->width,
          drawing->height);
  
  
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
        drawing->height + height + SAFETY,
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
    drawing->width+SAFETY, drawing->height - y + SAFETY);




  if (drawing->pixmap)
    gdk_pixmap_unref(drawing->pixmap);

  drawing->pixmap = pixmap;
  
  drawing->height+=height;

  /* Rectangle to update, from new drawing dimensions */
  //update_rect.x = 0 ;
  //update_rect.y = y ;
  //update_rect.width = drawing->width;
  //update_rect.height = drawing->height - y ;
  //gtk_widget_draw( drawing->drawing_area, &update_rect);
}


/* Remove a square corresponding to a removed process in the list */
void drawing_remove_square(Drawing_t *drawing,
        guint y,
        guint height)
{
  //GdkRectangle update_rect;
  
  /* Allocate a new pixmap with new height */
  GdkPixmap *pixmap = gdk_pixmap_new(
      drawing->drawing_area->window,
      drawing->width + SAFETY,
      drawing->height - height + SAFETY,
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
    drawing->width, drawing->height - y - height + SAFETY);


  if (drawing->pixmap)
    gdk_pixmap_unref(drawing->pixmap);

  drawing->pixmap = pixmap;
  
  drawing->height-=height;
  
  /* Rectangle to update, from new drawing dimensions */
  //update_rect.x = 0 ;
  //update_rect.y = y ;
  //update_rect.width = drawing->width;
  //update_rect.height = drawing->height - y ;
  //gtk_widget_draw( drawing->drawing_area, &update_rect);
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

  gchar text[255];
  
  PangoContext *context;
  PangoLayout *layout;
  PangoAttribute *attribute;
  PangoFontDescription *FontDesc;
  gint Font_Size;
  PangoRectangle ink_rect;
  guint global_width=0;
  GdkColor foreground = { 0, 0, 0, 0 };
  GdkColor background = { 0, 0xffff, 0xffff, 0xffff };

  LttTime window_end = 
    ltt_time_add(drawing->control_flow_data->time_window.time_width,
                 drawing->control_flow_data->time_window.start_time);
  LttTime half_width =
    ltt_time_div(drawing->control_flow_data->time_window.time_width,2.0);
  LttTime window_middle =
    ltt_time_add(half_width,
                 drawing->control_flow_data->time_window.start_time);
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
    drawing->control_flow_data->time_window.start_time.tv_sec,
    drawing->control_flow_data->time_window.start_time.tv_nsec);

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
