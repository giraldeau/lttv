#ifndef _DRAWING_H
#define _DRAWING_H

#include <time.h>
#include <glib.h>

typedef time_t ltt_time;

typedef struct _ltt_time_interval
{
	ltt_time time_begin, time_end;
} ltt_time_interval;



typedef struct _Drawing_t Drawing_t;

Drawing_t *Drawing(void);
void Drawing_destroy(Drawing_t *Drawing);
void Drawing_Resize(Drawing_t *Drawing, guint h, guint w);



void get_time_from_pixels(
		guint area_x,
		guint area_width,
		guint window_width,
		ltt_time *window_time_begin,
		ltt_time *window_time_end,
		ltt_time *time_begin,
		ltt_time *time_end);


#endif // _DRAWING_H
