#ifndef _DRAWING_H
#define _DRAWING_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include "CFV.h"

/* This part of the viewer does :
 * Draw horizontal lines, getting graphic context as arg.
 * Copy region of the screen into another.
 * Modify the boundaries to reflect a scale change. (resize)
 * Refresh the physical screen with the pixmap
 * A helper function is provided here to convert from time and process
 * identifier to pixels and the contrary (will be useful for mouse selection).
 * Insert an empty square in the drawing, moving the bottom part.
 *
 * The pixmap used has the width of the physical window, but the height
 * of the shown processes.
 */

typedef struct _Drawing_t Drawing_t;


//FIXME : TEMPORARILY PLACED HERE FOR GC !!
struct _Drawing_t {
	GtkWidget	*Drawing_Area_V;
	GdkPixmap	*Pixmap;
	ControlFlowData	*Control_Flow_Data;

	gint 		height, width, depth;

};


Drawing_t *drawing_construct(ControlFlowData *Control_Flow_Data);
void drawing_destroy(Drawing_t *Drawing);

GtkWidget *drawing_get_widget(Drawing_t *Drawing);
	
//void Drawing_Refresh (	Drawing_t *Drawing,
//			guint x, guint y,
//			guint width, guint height);

void drawing_draw_line(	Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			guint x1, guint y1,
			guint x2, guint y2,
			GdkGC *GC);

//void Drawing_copy( Drawing_t *Drawing,
//		guint xsrc, guint ysrc,
//		guint xdest, guint ydest,
//		guint width, guint height);

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
		Drawing_t *Drawing,
		guint x,
		LttTime *window_time_begin,
		LttTime *window_time_end,
		LttTime *begin);

void convert_time_to_pixels(
		LttTime window_time_begin,
		LttTime window_time_end,
		LttTime time,
		Drawing_t *Drawing,
		guint *x);

#endif // _DRAWING_H
