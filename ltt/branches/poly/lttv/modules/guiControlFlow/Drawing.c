
#include "Drawing.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>

/*****************************************************************************
 *                              Drawing functions                            *
 *****************************************************************************/

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
	{ 0, 0xffff, 0x0000, 0x0000 },	// RED
	{ 0, 0x0000, 0xffff, 0x0000 },	// GREEN
	{ 0, 0x0000, 0x0000, 0xffff },	// BLUE
	{ 0, 0xffff, 0xffff, 0xffff },	// WHITE
	{ 0, 0x0000, 0x0000, 0x0000 }	// BLACK
};


struct _Drawing_t {
	GtkWidget	*Drawing_Area_V;
	GdkPixmap	*Pixmap;

	gint 		height, width, depth;

};


Drawing_t *Drawing_construct(void)
{

	Drawing_t *Drawing = g_new(Drawing_t, 1);
		
	Drawing->Drawing_Area_V = gtk_drawing_area_new ();

	g_object_set_data_full(
			G_OBJECT(Drawing->Drawing_Area_V),
			"Link_Drawing_Data",
			Drawing,
			(GDestroyNotify)Drawing_destroy);

	gtk_widget_modify_bg(	Drawing->Drawing_Area_V,
				GTK_STATE_NORMAL,
				&CF_Colors[BLACK]);
	
	gdk_window_get_geometry(Drawing->Drawing_Area_V->window,
			NULL, NULL,
			&(Drawing->width),
			&(Drawing->height),
			&(Drawing->depth));
	
	Drawing->Pixmap = gdk_pixmap_new(
			Drawing->Drawing_Area_V->window,
			Drawing->width,
			Drawing->height,
			Drawing->depth);
			
	return Drawing;
}

void Drawing_destroy(Drawing_t *Drawing)
{

	// Do not unref here, Drawing_t destroyed by it's widget.
	//g_object_unref( G_OBJECT(Drawing->Drawing_Area_V));
		
	g_free(Drawing);
}

GtkWidget *Drawing_getWidget(Drawing_t *Drawing)
{
	return Drawing->Drawing_Area_V;
}

/* get_time_from_pixels
 *
 * Get the time interval from window time and pixels, and pixels requested. This
 * function uses TimeMul, which should only be used if the float value is lower
 * that 4, and here it's always lower than 1, so it's ok.
 */
void convert_pixels_to_time(
		Drawing_t *Drawing,
		guint x,
		LttTime *window_time_begin,
		LttTime *window_time_end,
		LttTime *time)
{
	LttTime window_time_interval;
	
	TimeSub(window_time_interval, *window_time_end, *window_time_begin);

	
	TimeMul(*time, window_time_interval,
			(x/(float)Drawing->width));
	TimeAdd(*time, *window_time_begin, *time);
	
}



void convert_time_to_pixels(
		LttTime window_time_begin,
		LttTime window_time_end,
		LttTime time,
		Drawing_t *Drawing,
		guint *x)
{
	LttTime window_time_interval;
	float interval_float, time_float;
	
	TimeSub(window_time_interval, window_time_end, window_time_begin);
	
	TimeSub(time, time, window_time_begin);
	
	interval_float = (window_time_interval.tv_sec * NANSECOND_CONST)
			+ window_time_interval.tv_nsec;
	time_float = (time.tv_sec * NANSECOND_CONST)
			+ time.tv_nsec;

	*x = (guint)(time_float/interval_float * Drawing->width);
	
}



void Drawing_Resize(Drawing_t *Drawing, guint h, guint w)
{
	Drawing->height = h ;
	Drawing->width = w ;

	gtk_widget_set_size_request (	Drawing->Drawing_Area_V,
					Drawing->width,
					Drawing->height);
	
	
}
