
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/processTrace.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/hook.h>

#include "Drawing.h"
#include "CFV.h"
#include "CFV-private.h"
#include "Event_Hooks.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)


/*****************************************************************************
 *                              Drawing functions                            *
 *****************************************************************************/

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
	{ 0, 0xffff, 0x0000, 0x0000 },	// RED
	{ 0, 0x0000, 0xffff, 0x0000 },	// GREEN
	{ 0, 0x0000, 0x0000, 0xffff },	// BLUE
	{ 0, 0xffff, 0xffff, 0xffff },	// WHITE
	{ 0, 0x0000, 0x0000, 0x0000 }	// BLACK
};


/* Function responsible for updating the exposed area.
 * It must call processTrace() to ask for this update.
 */
void drawing_data_request(Drawing_t *Drawing,
			GdkPixmap **Pixmap,
			gint x, gint y,
		  gint width,
			gint height)
{
  if(width < 0) return ;
  if(height < 0) return ;
	ControlFlowData *control_flow_data =
			(ControlFlowData*)g_object_get_data(
								G_OBJECT(
										Drawing->Drawing_Area_V),
								"Control_Flow_Data");

	LttTime start, end;
	LttTime window_end = ltt_time_add(control_flow_data->Time_Window.time_width,
												control_flow_data->Time_Window.start_time);

	g_critical("req : window_end : %u, %u", window_end.tv_sec, 
																			window_end.tv_nsec);

	g_critical("req : time width : %u, %u", control_flow_data->Time_Window.time_width.tv_sec, 
																control_flow_data->Time_Window.time_width.tv_nsec);
	
	g_critical("x is : %i, x+width is : %i", x, x+width);

	convert_pixels_to_time(Drawing->Drawing_Area_V->allocation.width, x,
				&control_flow_data->Time_Window.start_time,
				&window_end,
				&start);

	convert_pixels_to_time(Drawing->Drawing_Area_V->allocation.width, x + width,
				&control_flow_data->Time_Window.start_time,
				&window_end,
				&end);
	
	LttvTracesetContext * tsc =
				get_traceset_context(control_flow_data->Parent_Window);
	
  gdk_draw_rectangle (*Pixmap,
		      Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      x, y,
		      width,	// do not overlap
		      height);

  //send_test_process(
	//guicontrolflow_get_process_list(Drawing->Control_Flow_Data),
	//Drawing);
  //send_test_drawing(
	//guicontrolflow_get_process_list(Drawing->Control_Flow_Data),
	//Drawing, *Pixmap, x, y, width, height);
  
	// Let's call processTrace() !!
	EventRequest event_request; // Variable freed at the end of the function.
	event_request.Control_Flow_Data = control_flow_data;
	event_request.time_begin = start;
	event_request.time_end = end;

	g_critical("req : start : %u, %u", event_request.time_begin.tv_sec, 
																			event_request.time_begin.tv_nsec);

	g_critical("req : end : %u, %u", event_request.time_end.tv_sec, 
																			event_request.time_end.tv_nsec);
	
	LttvHooks *event = lttv_hooks_new();
	state_add_event_hooks_api(control_flow_data->Parent_Window);
	lttv_hooks_add(event, draw_event_hook, &event_request);

	lttv_process_traceset_seek_time(tsc, start);
	lttv_traceset_context_add_hooks(tsc,
			NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, event, NULL);
	lttv_process_traceset(tsc, end, G_MAXULONG);
	lttv_traceset_context_remove_hooks(tsc, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, event, NULL);

	state_remove_event_hooks_api(control_flow_data->Parent_Window);
	lttv_hooks_destroy(event);
}
		      
/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event, 
		gpointer user_data)
{
  Drawing_t *Drawing = (Drawing_t*)user_data;


	/* First, get the new time interval of the main window */
	/* we assume (see documentation) that the main window
	 * has updated the time interval before this configure gets
	 * executed.
	 */
	get_time_window(Drawing->Control_Flow_Data->Parent_Window,
			&Drawing->Control_Flow_Data->Time_Window);
		
  /* New Pixmap, size of the configure event */
  GdkPixmap *Pixmap = gdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
	
  g_critical("drawing configure event");

  /* If no old Pixmap present */
  if(Drawing->Pixmap == NULL)
  {
  	Drawing->Pixmap = gdk_pixmap_new(
		widget->window,
		widget->allocation.width,
		widget->allocation.height,
		//ProcessList_get_height
		// (GuiControlFlow_get_Process_List(Drawing->Control_Flow_Data)),
		-1);
	Drawing->width = widget->allocation.width;
	Drawing->height = widget->allocation.height;
g_critical("init data");
	/* Initial data request */
 	drawing_data_request(Drawing, &Drawing->Pixmap, 0, 0,
		   	widget->allocation.width,
			widget->allocation.height);

  }
//  /* Draw empty background */ 
//  gdk_draw_rectangle (Pixmap,
//		      widget->style->black_gc,
//		      TRUE,
//		      0, 0,
//		      widget->allocation.width,
//		      widget->allocation.height);
  
  /* Copy old data to new pixmap */
  gdk_draw_drawable (Pixmap,
		  widget->style->white_gc,
		  Drawing->Pixmap,
		  0, 0,
		  0, 0,
		  -1, -1);

  if (Drawing->Pixmap)
    gdk_pixmap_unref(Drawing->Pixmap);

  Drawing->Pixmap = Pixmap;

   /* Request data for missing space */
g_critical("missing data");
   drawing_data_request(Drawing, &Pixmap, Drawing->width, 0,
		   	widget->allocation.width - Drawing->width,
			widget->allocation.height);
	 // we do not request data vertically!
//   drawing_data_request(Drawing, &Pixmap, 0, Drawing->height,
//		   Drawing->width,
//		   widget->allocation.height - Drawing->height);
		                      
//   gdk_draw_rectangle (Pixmap,
//		      widget->style->white_gc,
//		      TRUE,
//		      Drawing->width, 0,
//		      widget->allocation.width -
//		      			Drawing->width,
//		      widget->allocation.height);
		
	 	// Clear the bottom part of the image
    gdk_draw_rectangle (Pixmap,
		      widget->style->white_gc,
		      TRUE,
		      0, Drawing->height,
		      Drawing->width,	// do not overlap
		      widget->allocation.height -	Drawing->height);
 
  Drawing->width = widget->allocation.width;
  Drawing->height = widget->allocation.height;

  return TRUE;
}


/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  Drawing_t *Drawing = (Drawing_t*)user_data;
  g_critical("drawing expose event");

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  Drawing->Pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

Drawing_t *drawing_construct(ControlFlowData *Control_Flow_Data)
{
	Drawing_t *Drawing = g_new(Drawing_t, 1);
		
	Drawing->Drawing_Area_V = gtk_drawing_area_new ();
	Drawing->Control_Flow_Data = Control_Flow_Data;

	//gtk_widget_set_size_request(Drawing->Drawing_Area_V->window, 50, 50);
	g_object_set_data_full(
			G_OBJECT(Drawing->Drawing_Area_V),
			"Link_Drawing_Data",
			Drawing,
			(GDestroyNotify)drawing_destroy);

	//gtk_widget_modify_bg(	Drawing->Drawing_Area_V,
	//			GTK_STATE_NORMAL,
	//			&CF_Colors[BLACK]);
	
	//gdk_window_get_geometry(Drawing->Drawing_Area_V->window,
	//		NULL, NULL,
	//		&(Drawing->width),
	//		&(Drawing->height),
	//		-1);
	
	//Drawing->Pixmap = gdk_pixmap_new(
	//		Drawing->Drawing_Area_V->window,
	//		Drawing->width,
	//		Drawing->height,
	//		Drawing->depth);
	
	Drawing->Pixmap = NULL;

// 	Drawing->Pixmap = gdk_pixmap_new(Drawing->Drawing_Area_V->window,
//			  Drawing->Drawing_Area_V->allocation.width,
//			  Drawing->Drawing_Area_V->allocation.height,
//			  -1);

	g_signal_connect (G_OBJECT(Drawing->Drawing_Area_V),
				"configure_event",
				G_CALLBACK (configure_event),
				(gpointer)Drawing);
	
	g_signal_connect (G_OBJECT(Drawing->Drawing_Area_V),
				"expose_event",
				G_CALLBACK (expose_event),
				(gpointer)Drawing);

	return Drawing;
}

void drawing_destroy(Drawing_t *Drawing)
{

	// Do not unref here, Drawing_t destroyed by it's widget.
	//g_object_unref( G_OBJECT(Drawing->Drawing_Area_V));
		
	g_free(Drawing);
}

GtkWidget *drawing_get_widget(Drawing_t *Drawing)
{
	return Drawing->Drawing_Area_V;
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

void drawing_refresh (	Drawing_t *Drawing,
			guint x, guint y,
			guint width, guint height)
{
	g_info("Drawing.c : drawing_refresh %u, %u, %u, %u", x, y, width, height);
	GdkRectangle update_rect;

	gdk_draw_drawable(
		Drawing->Drawing_Area_V->window,
		Drawing->Drawing_Area_V->
		 style->fg_gc[GTK_WIDGET_STATE (Drawing->Drawing_Area_V)],
		GDK_DRAWABLE(Drawing->Pixmap),
		x, y,
		x, y,
		width, height);

	update_rect.x = 0 ;
	update_rect.y = 0 ;
	update_rect.width = Drawing->width;
	update_rect.height = Drawing->height ;
	gtk_widget_draw( Drawing->Drawing_Area_V, &update_rect);

}


void drawing_draw_line(	Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			guint x1, guint y1,
			guint x2, guint y2,
			GdkGC *GC)
{
	gdk_draw_line (Pixmap,
			GC,
			x1, y1, x2, y2);
}




void drawing_resize(Drawing_t *Drawing, guint h, guint w)
{
	Drawing->height = h ;
	Drawing->width = w ;

	gtk_widget_set_size_request (	Drawing->Drawing_Area_V,
					Drawing->width,
					Drawing->height);
	
	
}


/* Insert a square corresponding to a new process in the list */
/* Applies to whole Drawing->width */
void drawing_insert_square(Drawing_t *Drawing,
				guint y,
				guint height)
{
	GdkRectangle update_rect;

	/* Allocate a new pixmap with new height */
	GdkPixmap *Pixmap = gdk_pixmap_new(Drawing->Drawing_Area_V->window,
			  Drawing->width,
			  Drawing->height + height,
			  -1);
	
	/* Copy the high region */
	gdk_draw_drawable (Pixmap,
		Drawing->Drawing_Area_V->style->black_gc,
		Drawing->Pixmap,
		0, 0,
		0, 0,
		Drawing->width, y);




	/* add an empty square */
	gdk_draw_rectangle (Pixmap,
		Drawing->Drawing_Area_V->style->black_gc,
		TRUE,
		0, y,
		Drawing->width,	// do not overlap
		height);



	/* copy the bottom of the region */
 	gdk_draw_drawable (Pixmap,
		Drawing->Drawing_Area_V->style->black_gc,
		Drawing->Pixmap,
		0, y,
		0, y + height,
		Drawing->width, Drawing->height - y);




	if (Drawing->Pixmap)
		gdk_pixmap_unref(Drawing->Pixmap);

	Drawing->Pixmap = Pixmap;
	
	Drawing->height+=height;

	/* Rectangle to update, from new Drawing dimensions */
	update_rect.x = 0 ;
	update_rect.y = y ;
	update_rect.width = Drawing->width;
	update_rect.height = Drawing->height - y ;
	gtk_widget_draw( Drawing->Drawing_Area_V, &update_rect);
}


/* Remove a square corresponding to a removed process in the list */
void drawing_remove_square(Drawing_t *Drawing,
				guint y,
				guint height)
{
	GdkRectangle update_rect;
	
	/* Allocate a new pixmap with new height */
	GdkPixmap *Pixmap = gdk_pixmap_new(
			Drawing->Drawing_Area_V->window,
			Drawing->width,
			Drawing->height - height,
			-1);
	
	/* Copy the high region */
	gdk_draw_drawable (Pixmap,
		Drawing->Drawing_Area_V->style->black_gc,
		Drawing->Pixmap,
		0, 0,
		0, 0,
		Drawing->width, y);



	/* Copy up the bottom of the region */
 	gdk_draw_drawable (Pixmap,
		Drawing->Drawing_Area_V->style->black_gc,
		Drawing->Pixmap,
		0, y + height,
		0, y,
		Drawing->width, Drawing->height - y - height);


	if (Drawing->Pixmap)
		gdk_pixmap_unref(Drawing->Pixmap);

	Drawing->Pixmap = Pixmap;
	
	Drawing->height-=height;
	
	/* Rectangle to update, from new Drawing dimensions */
	update_rect.x = 0 ;
	update_rect.y = y ;
	update_rect.width = Drawing->width;
	update_rect.height = Drawing->height - y ;
	gtk_widget_draw( Drawing->Drawing_Area_V, &update_rect);
}


