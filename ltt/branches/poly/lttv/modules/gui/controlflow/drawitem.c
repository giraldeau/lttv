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



/******************************************************************************
 * drawitem.c
 *
 * This file contains methods responsible for drawing a generic type of data
 * in a drawable. Doing this generically will permit user defined drawing
 * behavior in a later time.
 *
 * This file provides an API which is meant to be reusable for all viewers that
 * need to show information in line, icon, text, background or point form in
 * a drawable area having time for x axis. The y axis, in the control flow
 * viewer case, is corresponding to the different processes, but it can be
 * reused integrally for cpu, and eventually locks, buffers, network
 * interfaces... What will differ between the viewers is the precise
 * information which interests us. We may think that the most useful
 * information for control flow are some specific events, like schedule
 * change, and processes'states. It may differ for a cpu viewer : the
 * interesting information could be more the execution mode of each cpu.
 * This API in meant to make viewer's writers life easier : it will become
 * a simple choice of icons and line types for the precise information
 * the viewer has to provide (agremented with keeping supplementary records
 * and modifying slightly the DrawContext to suit the needs.)
 *
 * We keep each data type in attributes, keys to specific information
 * being formed from the GQuark corresponding to the information received.
 * (facilities / facility_name / events / eventname.)
 * (cpus/cpu_name, process_states/ps_name,
 * execution_modes/em_name, execution_submodes/es_name).
 * The goal is then to provide a generic way to print information on the
 * screen for all this different information.
 *
 * Information can be printed as
 *
 * - text (text + color + size + position (over or under line)
 * - icon (icon filename, corresponding to a loaded icon, accessible through
 *   a GQuark. Icons are loaded statically at the guiControlFlow level during
 *   module initialization and can be added on the fly if not present in the
 *   GQuark.) The habitual place for xpm icons is in
 *   ${prefix}/share/LinuxTraceToolkit.) + position (over or under line)
 * - line (color, width, style)
 * - Arc (big points) (color, size)
 * - background color (color)
 *
 * An item is a leaf of the attributes tree. It is, in that case, including
 * all kind of events categories we can have. It then associates each category
 * with one or more actions (drawing something) or nothing.
 * 
 * Each item has an array of hooks (hook list). Each hook represents an
 * operation to perform. We seek the array each time we want to
 * draw an item. We execute each operation in order. An operation type
 * is associated with each hook to permit user listing and modification
 * of these operations. The operation type is also used to find the
 * corresponding priority for the sorting. Operation type and priorities
 * are enum and a static int table.
 *
 * The array has to be sorted by priority each time we add a task in it.
 * A priority is associated with each operation type. It permits
 * to perform background color selection before line or text drawing. We also
 * draw lines before text, so the text appears over the lines.
 *
 * Executing all the arrays of operations for a specific event (which
 * implies information for state, event, cpu, execution mode and submode)
 * has to be done in a same DrawContext. The goal there is to keep the offset
 * of the text and icons over and under the middle line, so a specific
 * event could be printed as (  R Si 0 for running, scheduled in, cpu 0  ),
 * text being easy to replace with icons. The DrawContext is passed as
 * call_data for the operation hooks.
 *
 * We use the lttv global attributes to keep track of the loaded icons.
 * If we need an icon, we look for it in the icons / icon name pathname.
 * If found, we use the pointer to it. If not, we load the pixmap in
 * memory and set the pointer to the GdkPixmap in the attributes. The
 * structure pointed to contains the pixmap and the mask bitmap.
 * 
 * Author : Mathieu Desnoyers, October 2003
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <string.h>

#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/lttv.h>

#include "drawing.h"
#include "drawitem.h"


#define MAX_PATH_LEN 256

/* drawing hook functions */
gboolean draw_text( void *hook_data, void *call_data)
{
  PropertiesText *properties = (PropertiesText*)hook_data;
  DrawContext *draw_context = (DrawContext*)call_data;

  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *font_desc;// = pango_font_description_new();
  PangoRectangle ink_rect;
    
  layout = draw_context->pango_layout;

  context = pango_layout_get_context(layout);
  font_desc = pango_context_get_font_description(context);

  pango_font_description_set_size(font_desc, properties->size*PANGO_SCALE);
  pango_layout_context_changed(layout);

  pango_layout_set_text(layout, properties->text, -1);
  pango_layout_get_pixel_extents(layout, &ink_rect, NULL);

  gint x=0, y=0;
  gint *offset=NULL;
  gboolean enough_space = FALSE;
  gint width = ink_rect.width;

  switch(properties->position.x) {
    case POS_START:
      x = draw_context->drawinfo.start.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.start.offset.over;
          x += draw_context->drawinfo.start.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.start.offset.middle;
          x += draw_context->drawinfo.start.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.start.offset.under;
          x += draw_context->drawinfo.start.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x + width <= draw_context->drawinfo.end.x)) {
        enough_space = TRUE;
        *offset += width;
      }
      break;
    case POS_END:
      x = draw_context->drawinfo.end.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.end.offset.over;
          x += draw_context->drawinfo.end.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.end.offset.middle;
          x += draw_context->drawinfo.end.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.end.offset.under;
          x += draw_context->drawinfo.end.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x - width >= draw_context->drawinfo.start.x)) {
        enough_space = TRUE;
        *offset -= width;
      }
      break;
  }

  if(unlikely(enough_space))
    gdk_draw_layout_with_colors(draw_context->drawable,
              draw_context->gc,
              x,
              y,
              layout, properties->foreground, properties->background);

  return 0;
}


/* To speed up the process, search in already loaded icons list first. Only
 * load it if not present.
 */
gboolean draw_icon( void *hook_data, void *call_data)
{
  PropertiesIcon *properties = (PropertiesIcon*)hook_data;
  DrawContext *draw_context = (DrawContext*)call_data;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvAttributeValue value;
  gchar icon_name[MAX_PATH_LEN] = "icons/";
  IconStruct *icon_info;

  strcat(icon_name, properties->icon_name);
  
  g_assert(lttv_iattribute_find_by_path(attributes, icon_name,
      LTTV_POINTER, &value));
  if(unlikely(*(value.v_pointer) == NULL))
  {
    *(value.v_pointer) = icon_info = g_new(IconStruct,1);
    
    icon_info->pixmap = gdk_pixmap_create_from_xpm(draw_context->drawable,
                          &icon_info->mask, NULL, properties->icon_name);
  }
  else
  {
    icon_info = *(value.v_pointer);
  }
  
  gint x=0, y=0;
  gint *offset=NULL;
  gboolean enough_space = FALSE;
  gint width = properties->width;
  
  switch(properties->position.x) {
    case POS_START:
      x = draw_context->drawinfo.start.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.start.offset.over;
          x += draw_context->drawinfo.start.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.start.offset.middle;
          x += draw_context->drawinfo.start.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.start.offset.under;
          x += draw_context->drawinfo.start.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x + width <= draw_context->drawinfo.end.x)) {
        enough_space = TRUE;
        *offset += width;
      }
      break;
    case POS_END:
      x = draw_context->drawinfo.end.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.end.offset.over;
          x += draw_context->drawinfo.end.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.end.offset.middle;
          x += draw_context->drawinfo.end.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.end.offset.under;
          x += draw_context->drawinfo.end.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x - width >= draw_context->drawinfo.start.x)) {
        enough_space = TRUE;
        *offset -= width;
      }
      break;
  }

  if(unlikely(enough_space)) {
    gdk_gc_set_clip_mask(draw_context->gc, icon_info->mask);

    gdk_gc_set_clip_origin(
        draw_context->gc,
        x,
        y);
    gdk_draw_drawable(draw_context->drawable, 
        draw_context->gc,
        icon_info->pixmap,
        0, 0,
        x,
        y,
        properties->width, properties->height);

    gdk_gc_set_clip_origin(draw_context->gc, 0, 0);
    gdk_gc_set_clip_mask(draw_context->gc, NULL);
  }
  return 0;
}

gboolean draw_line( void *hook_data, void *call_data)
{
  PropertiesLine *properties = (PropertiesLine*)hook_data;
  DrawContext *draw_context = (DrawContext*)call_data;
  
  gdk_gc_set_foreground(draw_context->gc, &properties->color);
  //gdk_gc_set_rgb_fg_color(draw_context->gc, &properties->color);
  gdk_gc_set_line_attributes( draw_context->gc,
                              properties->line_width,
                              properties->style,
                              GDK_CAP_BUTT,
                              GDK_JOIN_MITER);
  //g_critical("DRAWING LINE : x1: %i, y1: %i, x2:%i, y2:%i", 
  //    draw_context->previous->middle->x,
  //    draw_context->previous->middle->y,
  //    draw_context->drawinfo.middle.x,
  //    draw_context->drawinfo.middle.y);

  gint x_begin=0, x_end=0, y=0;
  
  x_begin = draw_context->drawinfo.start.x;
  x_end = draw_context->drawinfo.end.x;

  switch(properties->y) {
    case OVER:
      y = draw_context->drawinfo.y.over;
      break;
    case MIDDLE:
      y = draw_context->drawinfo.y.middle;
      break;
    case UNDER:
      y = draw_context->drawinfo.y.under;
      break;
  }

  drawing_draw_line(
    NULL, draw_context->drawable,
    x_begin,
    y,
    x_end,
    y,
    draw_context->gc);
  
  return 0;
}

gboolean draw_arc( void *hook_data, void *call_data)
{
  PropertiesArc *properties = (PropertiesArc*)hook_data;
  DrawContext *draw_context = (DrawContext*)call_data;

  gdk_gc_set_foreground(draw_context->gc, properties->color);
  //gdk_gc_set_rgb_fg_color(draw_context->gc, properties->color);

  gint x=0, y=0;
  gint *offset=NULL;
  gboolean enough_space = FALSE;
  gint width = properties->size;
  
  switch(properties->position.x) {
    case POS_START:
      x = draw_context->drawinfo.start.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.start.offset.over;
          x += draw_context->drawinfo.start.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.start.offset.middle;
          x += draw_context->drawinfo.start.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.start.offset.under;
          x += draw_context->drawinfo.start.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x + width <= draw_context->drawinfo.end.x)) {
        enough_space = TRUE;
        *offset += width;
      }
      break;
    case POS_END:
      x = draw_context->drawinfo.end.x;
      switch(properties->position.y) {
        case OVER:
          offset = &draw_context->drawinfo.end.offset.over;
          x += draw_context->drawinfo.end.offset.over;
          y = draw_context->drawinfo.y.over;
          break;
        case MIDDLE:
          offset = &draw_context->drawinfo.end.offset.middle;
          x += draw_context->drawinfo.end.offset.middle;
          y = draw_context->drawinfo.y.middle;
          break;
        case UNDER:
          offset = &draw_context->drawinfo.end.offset.under;
          x += draw_context->drawinfo.end.offset.under;
          y = draw_context->drawinfo.y.under;
          break;
      }
      /* verify if there is enough space to draw */
      if(unlikely(x - width >= draw_context->drawinfo.start.x)) {
        enough_space = TRUE;
        *offset -= width;
      }
      break;
  }

  if(unlikely(enough_space))
    gdk_draw_arc(draw_context->drawable, draw_context->gc,
          properties->filled,
          x,
          y,
          properties->size, properties->size, 0, 360*64);
  
  return 0;
}

gboolean draw_bg( void *hook_data, void *call_data)
{
  PropertiesBG *properties = (PropertiesBG*)hook_data;
  DrawContext *draw_context = (DrawContext*)call_data;

  gdk_gc_set_foreground(draw_context->gc, properties->color);
  //gdk_gc_set_rgb_fg_color(draw_context->gc, properties->color);

  //g_critical("DRAWING RECT : x: %i, y: %i, w:%i, h:%i, val1 :%i, val2:%i ", 
  //    draw_context->previous->over->x,
  //    draw_context->previous->over->y,
  //    draw_context->drawinfo.over.x - draw_context->previous->over->x,
  //    draw_context->previous->under->y-draw_context->previous->over->y,
  //    draw_context->drawinfo.over.x,
  //    draw_context->previous->over->x);
  gdk_draw_rectangle(draw_context->drawable, draw_context->gc,
          TRUE,
          draw_context->drawinfo.start.x,
          draw_context->drawinfo.y.over,
          draw_context->drawinfo.end.x - draw_context->drawinfo.start.x,
          draw_context->drawinfo.y.under - draw_context->drawinfo.y.over);

  return 0;
}


