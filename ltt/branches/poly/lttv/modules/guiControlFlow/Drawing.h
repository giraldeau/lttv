#ifndef _DRAWING_H
#define _DRAWING_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include "CFV.h"
#include "Draw_Item.h"


#define SAFETY 50 // safety pixels at right and bottom of pixmap buffer

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

typedef struct _Drawing_t Drawing_t;

struct _Drawing_t {
  GtkWidget *drawing_area;
  GdkPixmap *pixmap;
  ControlFlowData *control_flow_data;
  
  PangoLayout *pango_layout;

  gint    height, width, depth;
  
};

Drawing_t *drawing_construct(ControlFlowData *control_flow_data);
void drawing_destroy(Drawing_t *Drawing);

GtkWidget *drawing_get_widget(Drawing_t *Drawing);
  
//void Drawing_Refresh (  Drawing_t *Drawing,
//      guint x, guint y,
//      guint width, guint height);

void drawing_draw_line( Drawing_t *Drawing,
      GdkPixmap *pixmap,
      guint x1, guint y1,
      guint x2, guint y2,
      GdkGC *GC);

//void Drawing_copy( Drawing_t *Drawing,
//    guint xsrc, guint ysrc,
//    guint xdest, guint ydest,
//    guint width, guint height);

/* Insert a square corresponding to a new process in the list */
void drawing_insert_square(Drawing_t *Drawing,
        guint y,
        guint height);

/* Remove a square corresponding to a removed process in the list */
void drawing_remove_square(Drawing_t *Drawing,
        guint y,
        guint height);


//void Drawing_Resize(Drawing_t *Drawing, guint h, guint w);

void convert_pixels_to_time(
    gint width,
    guint x,
    LttTime *window_time_begin,
    LttTime *window_time_end,
    LttTime *time);

void convert_time_to_pixels(
    LttTime window_time_begin,
    LttTime window_time_end,
    LttTime time,
    gint width,
    guint *x);

#endif // _DRAWING_H
