#ifndef _DRAWING_H
#define _DRAWING_H

#include <glib.h>
#include <ltt/ltt.h>

/* This part of the viewer does :
 * Draw horizontal lines, getting line color and width as arguments.
 * Copy region of the screen into another.
 * Modify the boundaries to reflect a scale change.
 *
 * A helper function is provided here to convert from time and process
 * identifier to pixels and the contrary (will be useful for mouse selection).
 */

typedef struct _Drawing_t Drawing_t;

Drawing_t *Drawing(void);
void Drawing_destroy(Drawing_t *Drawing);
void Drawing_Resize(Drawing_t *Drawing, guint h, guint w);



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
