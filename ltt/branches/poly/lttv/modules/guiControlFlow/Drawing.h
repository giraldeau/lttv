#ifndef _DRAWING_H
#define _DRAWING_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <ltt/ltt.h>

/* This part of the viewer does :
 * Draw horizontal lines, getting graphic context as arg.
 * Copy region of the screen into another.
 * Modify the boundaries to reflect a scale change. (resize)
 * Refresh the physical screen with the pixmap
 * A helper function is provided here to convert from time and process
 * identifier to pixels and the contrary (will be useful for mouse selection).
 */

typedef struct _Drawing_t Drawing_t;

Drawing_t *Drawing_construct(void);
void Drawing_destroy(Drawing_t *Drawing);

GtkWidget *Drawing_getWidget(Drawing_t *Drawing);
	
//void Drawing_Refresh (	Drawing_t *Drawing,
//			guint x, guint y,
//			guint width, guint height);

void Drawing_draw_line(	Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			guint x1, guint y1,
			guint x2, guint y2,
			GdkGC *GC);

//void Drawing_copy( Drawing_t *Drawing,
//		guint xsrc, guint ysrc,
//		guint xdest, guint ydest,
//		guint width, guint height);


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
