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


#ifndef _HISTODRAWING_H
#define _HISTODRAWING_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttvwindow/lttvwindow.h>
#include "histocfv.h"
#include "histodrawitem.h"


#define SAFETY 50 // safety pixels at right and bottom of pixmap buffer

typedef enum _draw_color {
                COL_BLACK,
                COL_WHITE,
                COL_RUN_USER_MODE,/* green */
                COL_RUN_SYSCALL,  /* pale blue */
                COL_RUN_TRAP,     /* yellow */
                COL_RUN_IRQ,      /* red */
                COL_WAIT,         /* dark red */
                COL_WAIT_CPU,     /* dark yellow */
                COL_ZOMBIE,       /* dark purple */
                COL_WAIT_FORK,    /* dark green */
                COL_EXIT,         /* "less dark" magenta */
                COL_MODE_UNKNOWN, /* white */
                COL_UNNAMED,      /* white */
                NUM_COLORS } draw_color; 

extern GdkColor histo_drawing_colors[NUM_COLORS];

/* This part of the viewer does :
 * Draw horizontal lines, getting graphic context as arg.
 * Copy region of the screen into another.
 * Modify the boundaries to reflect a scale change. (resize)
 * Refresh the physical screen with the pixmap
 * A helper function is provided here to convert from time to process
 * identifier to pixels and the contrary (will be useful for mouse selection).
 * Insert an empty square in the drawing, moving the bottom part.
 *
 * Note: The last point is exactly why it would not be so easy to add the
 * vertical line functionnality as in the original version of LTT. In order
 * to do so, we should keep all processes in the list for the duration of
 * all the trace instead of dynamically adding and removing them when we
 * scroll. Another possibility is to redraw all the visible area when a new
 * process is added to the list. The second solution seems more appropriate
 * to me.
 * 
 *
 * The pixmap used has the width of the physical window, but the height
 * of the shown processes.
 */

#ifndef TYPE_histoDrawing_t_DEFINED
#define TYPE_histoDrawing_t_DEFINED
typedef struct _histoDrawing_t histoDrawing_t;
#endif //TYPE_DRAWING_T_DEFINED

#ifndef TYPE_HistoControlFlowData_DEFINED
#define TYPE_HistoControlFlowData_DEFINED
typedef struct _HistoControlFlowData HistoControlFlowData;
#endif //TYPE_HistoControlFlowData_DEFINED

struct _histoDrawing_t {
  GtkWidget *vbox;
  GtkWidget *drawing_area;
  /*
  GtkWidget *hbox;
  GtkWidget *viewport;
  GtkWidget *scrollbar;*///at this time,not necessary for histogram
  
  GtkWidget *ruler_hbox;
  GtkWidget *ruler;
  GtkWidget *padding;
//vertical ruler
  GtkWidget *vruler_drawing_hbox;
  GtkWidget *vertical_ruler;

  GdkPixmap *pixmap;
  HistoControlFlowData *histo_control_flow_data;
  
  PangoLayout *pango_layout;

  gint      height, width, depth;
  /* height and width of allocated buffer pixmap */
  gint      alloc_height, alloc_width;
  
  /* X coordinate of damaged region */
  gint      damage_begin, damage_end; /* damaged region to be exposed,
                                         updated per chunk */
  LttTime   last_start;               
  GdkGC     *dotted_gc;
  GdkGC     *gc;
  GdkGC     *ruler_gc_butt;
  GdkGC     *ruler_gc_round;

  /* Position of the horizontal selector, -1 for none */
  gint horizontal_sel;
};

histoDrawing_t *histo_drawing_construct(HistoControlFlowData *histo_control_flow_data);
void histo_drawing_destroy(histoDrawing_t *drawing);


void histo_drawing_data_request(histoDrawing_t *drawing,
      gint x, gint y,
      gint width,
      gint height);

 GtkWidget *histo_drawing_get_widget(histoDrawing_t *drawing);
GtkWidget *histo_drawing_get_drawing_area(histoDrawing_t *drawing);

 void histo_drawing_draw_line(histoDrawing_t *drawing,
      GdkPixmap *pixmap,
      guint x1, guint y1,
      guint x2, guint y2,
      GdkGC *GC);

//void drawing_copy(histoDrawing_t *drawing,
//    guint xsrc, guint ysrc,
//    guint xdest, guint ydest,
//    guint width, guint height);

/* Clear the drawing : make it 1xwidth. */
void histo_drawing_clear(histoDrawing_t *drawing,guint clear_from,guint clear_to);

/* Insert a square corresponding to a new process in the list */
static void drawing_insert_square(histoDrawing_t *drawing,
        guint y,
        guint height);

/* Remove a square corresponding to a removed process in the list */
static void drawing_remove_square(histoDrawing_t *drawing,
        guint y,
        guint height);

void histo_drawing_update_ruler(histoDrawing_t *drawing, TimeWindow *time_window);

void histo_drawing_update_vertical_ruler(histoDrawing_t *drawing);//, TimeWindow *time_window);

void histo_drawing_request_expose(EventsRequest *events_request,
                            LttvTracesetState *tss,
                            LttTime end_time);

void histo_drawing_data_request_begin(EventsRequest *events_request,
                                LttvTracesetState *tss);
void histo_drawing_chunk_begin(EventsRequest *events_request, LttvTracesetState *tss);



static void
tree_row_activated(GtkTreeModel *treemodel,
                   GtkTreePath *arg1,
                   GtkTreeViewColumn *arg2,
                   gpointer user_data);


/* histo_convert_pixels_to_time
 *
 * Convert from window pixel and time interval to an absolute time.
 */
static inline void histo_convert_pixels_to_time(
    gint width,
    guint x,
    TimeWindow time_window,
    LttTime *time)
{
  double time_d;
  
  time_d = time_window.time_width_double;
  time_d = time_d / (double)width * (double)x;
  *time = ltt_time_from_double(time_d);
  *time = ltt_time_add(time_window.start_time, *time);
}


static inline void histo_convert_time_to_pixels(
    TimeWindow time_window,
    LttTime time,
    int width,
    guint *x)
{
  double time_d;
#ifdef EXTRA_CHECK 
  g_assert(ltt_time_compare(window_time_begin, time) <= 0 &&
           ltt_time_compare(window_time_end, time) >= 0);
#endif //EXTRA_CHECK
  
  time = ltt_time_sub(time, time_window.start_time);
  
  time_d = ltt_time_to_double(time);
  
  if(time_window.time_width_double == 0.0) {
    g_assert(time_d == 0.0);
    *x = 0;
  } else {
    *x = (guint)(time_d / time_window.time_width_double * (double)width);
  }
  
}

#endif // _DRAWING_H
