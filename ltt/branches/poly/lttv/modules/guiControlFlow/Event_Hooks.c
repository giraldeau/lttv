/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

//#define PANGO_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

//#include <pango/pango.h>

#include <lttv/hook.h>
#include <lttv/common.h>

#include "Event_Hooks.h"
#include "CFV.h"
#include "Process_List.h"
#include "Drawing.h"
#include "CFV-private.h"

#define MAX_PATH_LEN 256

//FIXME : remove this include when tests finished.
#include <string.h>

void test_draw_item(Drawing_t *Drawing,
			GdkPixmap *Pixmap) 
{
	PropertiesIcon properties_icon;
	DrawContext draw_context;
	
	DrawInfo current, previous;
	ItemInfo over, middle, under, modify_over, modify_middle, modify_under;

	int i,j;
	
	for(i=0; i<1024;i=i+15)
	{
		for(j=0;j<768;j=j+15)
		{
			over.x = i;
			over.y = j;
			over.ts =  NULL;
			over.tfs = NULL;

			current.modify_over = &over;
	
			draw_context.drawable = Pixmap;
			draw_context.gc = Drawing->Drawing_Area_V->style->black_gc;

			draw_context.Current = &current;
			draw_context.Previous = NULL;
	
			properties_icon.icon_name = g_new(char, MAX_PATH_LEN);
			strncpy(properties_icon.icon_name, 
				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/mini-display.xpm",
				MAX_PATH_LEN);
			properties_icon.width = -1;
			properties_icon.height = -1;
			properties_icon.position = OVER;
			draw_icon(&properties_icon, &draw_context);
			g_free(properties_icon.icon_name);
		}
	}

}

/* NOTE : no drawing data should be sent there, since the drawing widget
 * has not been initialized */
void send_test_drawing(ProcessList *Process_List,
			Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			gint x, gint y, // y not used here?
		  gint width,
			gint height) // height won't be used here ?
{
	int i,j;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *FontDesc;// = pango_font_description_new();
	gint Font_Size;

	//icon
	//GdkBitmap *mask = g_new(GdkBitmap, 1);
	//GdkPixmap *icon_pixmap = g_new(GdkPixmap, 1);
	GdkGC * gc;
	// rectangle
	GdkColor color = { 0, 0xffff, 0x0000, 0x0000 };
	
	gc = gdk_gc_new(Pixmap);
	/* Sent text data */
	layout = gtk_widget_create_pango_layout(Drawing->Drawing_Area_V,
			NULL);
	context = pango_layout_get_context(layout);
	FontDesc = pango_context_get_font_description(context);
	Font_Size = pango_font_description_get_size(FontDesc);
	pango_font_description_set_size(FontDesc, Font_Size-3*PANGO_SCALE);
	
	


	LttTime birth;
	birth.tv_sec = 12000;
	birth.tv_nsec = 55500;
	g_info("we have : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	processlist_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	
	g_info("we draw : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	pango_layout_set_text(layout, "Test", -1);
	gdk_draw_layout(Pixmap, Drawing->Drawing_Area_V->style->black_gc,
			0, y+height, layout);

	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	processlist_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_info("y : %u, height : %u", y, height);

	

	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);

	/* Draw rectangle (background color) */
	gdk_gc_copy(gc, Drawing->Drawing_Area_V->style->black_gc);
	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_draw_rectangle(Pixmap, gc,
					TRUE,
					x, y, width, height);

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	
	/* Draw arc */
	gdk_draw_arc(Pixmap, Drawing->Drawing_Area_V->style->black_gc,
							TRUE, 100, y, height/2, height/2, 0, 360*64);

	g_info("y : %u, height : %u", y, height);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		processlist_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		

		drawing_draw_line(
			Drawing, Pixmap, x,
			y+(height/2), x + width, y+(height/2),
			Drawing->Drawing_Area_V->style->black_gc);

		g_critical("y : %u, height : %u", y, height);

	}

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_info("y : %u, height : %u", y, height);
	

	/* IMPORTANT : This action uses the cpu heavily! */
	//icon_pixmap = gdk_pixmap_create_from_xpm(Pixmap, &mask, NULL,
//				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/move_message.xpm");
	//				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/mini-display.xpm");

	//		gdk_gc_set_clip_mask(Drawing->Drawing_Area_V->style->black_gc, mask);

//	for(i=x;i<x+width;i=i+15)
//	{
//		for(j=0;j<height*20;j=j+15)
//		{
			
			/* Draw icon */
			//gdk_gc_copy(gc, Drawing->Drawing_Area_V->style->black_gc);
//			gdk_gc_set_clip_origin(Drawing->Drawing_Area_V->style->black_gc, i, j);
//			gdk_draw_drawable(Pixmap, 
//					Drawing->Drawing_Area_V->style->black_gc,
//					icon_pixmap,
//					0, 0, i, j, -1, -1);

//		}
//	}

	test_draw_item(Drawing,Pixmap);
	
	//gdk_gc_set_clip_origin(Drawing->Drawing_Area_V->style->black_gc, 0, 0);
	//gdk_gc_set_clip_mask(Drawing->Drawing_Area_V->style->black_gc, NULL);

	//g_free(icon_pixmap);
	//g_free(mask);






	pango_font_description_set_size(FontDesc, Font_Size);
	g_object_unref(layout);
	g_free(gc);
}

void send_test_process(ProcessList *Process_List, Drawing_t *Drawing)
{
	guint height, y;
	int i;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;

	LttTime birth;

	if(Process_List->Test_Process_Sent) return;

	birth.tv_sec = 12000;
	birth.tv_nsec = 55500;

	processlist_add(Process_List,
			1,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	processlist_add(Process_List,
			156,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	processlist_add(Process_List,
			10,
			&birth,
			&height);
	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//drawing_insert_square( Drawing, height, 5);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		processlist_add(Process_List,
				i,
				&birth,
				&height);
		processlist_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		drawing_insert_square( Drawing, y, height);
	
	//	g_critical("y : %u, height : %u", y, height);
	
	}
	//g_critical("height : %u", height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	processlist_add(Process_List,
			10,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	processlist_add(Process_List,
			10000,
			&birth,
			&height);
	processlist_get_process_pixels(Process_List,
					10000,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//drawing_insert_square( Drawing, height, 5);
	//g_critical("height : %u", height);


	processlist_get_process_pixels(Process_List,
				10000,
				&birth,
				&y, &height);
	processlist_remove( 	Process_List,
				10000,
				&birth);

	drawing_remove_square( Drawing, y, height);
	
	if(got_RowRef = 
		(GtkTreeRowReference*)g_hash_table_lookup(
					Process_List->Process_Hash,
					&Process_Info))
	{
		g_critical("key found");
		g_critical("position in the list : %s",
			gtk_tree_path_to_string (
			gtk_tree_row_reference_get_path(
				(GtkTreeRowReference*)got_RowRef)
			));
		
	}

	Process_List->Test_Process_Sent = TRUE;

}



/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param pmParentWindow A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_guicontrolflow(MainWindow *pmParentWindow, LttvTracesetSelector * s, char * key)
{
	g_info("h_guicontrolflow, %p, %p, %s", pmParentWindow, s, key);
	ControlFlowData *Control_Flow_Data = guicontrolflow() ;
	
	Control_Flow_Data->Parent_Window = pmParentWindow;
	TimeWindow *time_window = guicontrolflow_get_time_window(Control_Flow_Data);
	time_window->start_time.tv_sec = 0;
	time_window->start_time.tv_nsec = 0;
	time_window->time_width.tv_sec = 0;
	time_window->time_width.tv_nsec = 0;

	LttTime *current_time = guicontrolflow_get_current_time(Control_Flow_Data);
	current_time->tv_sec = 0;
	current_time->tv_nsec = 0;
	
	//g_critical("time width1 : %u",time_window->time_width);
	
	get_time_window(pmParentWindow,
			time_window);
	get_current_time(pmParentWindow,
			current_time);

	//g_critical("time width2 : %u",time_window->time_width);
	// Unreg done in the GuiControlFlow_Destructor
	reg_update_time_window(update_time_window_hook, Control_Flow_Data,
				pmParentWindow);
	reg_update_current_time(update_current_time_hook, Control_Flow_Data,
				pmParentWindow);
	return guicontrolflow_get_widget(Control_Flow_Data) ;
	
}

int event_selected_hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	guint *Event_Number = (guint*) call_data;

	g_critical("DEBUG : event selected by main window : %u", *Event_Number);
	
//	Control_Flow_Data->Currently_Selected_Event = *Event_Number;
//	Control_Flow_Data->Selected_Event = TRUE ;
	
//	tree_v_set_cursor(Control_Flow_Data);

}

/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in Event_Request.
 */
int draw_before_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	//EventsContext Events_Context = (EventsContext*)call_data;
	
	//Event_Request->Events_Context = Events_Context;

	return 0;
}

/*
 * The draw event hook is called by the reading API to have a
 * particular event drawn on the screen.
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function basically draw lines and icons. Two types of lines are drawn :
 * one small (3 pixels?) representing the state of the process and the second
 * type is thicker (10 pixels?) representing on which CPU a process is running
 * (and this only in running state).
 *
 * Extremums of the lines :
 * x_min : time of the last event context for this process kept in memory.
 * x_max : time of the current event.
 * y : middle of the process in the process list. The process is found in the
 * list, therefore is it's position in pixels.
 *
 * The choice of lines'color is defined by the context of the last event for this
 * process.
 */
int draw_event_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	
	
	return 0;
}


int draw_after_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	
	g_free(Event_Request);
	return 0;
}




void update_time_window_hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	TimeWindow* Time_Window = 
		guicontrolflow_get_time_window(Control_Flow_Data);
	TimeWindow *New_Time_Window = ((TimeWindow*)call_data);

	// As the time interval change will mostly be used for
	// zoom in and out, it's not useful to keep old drawing
	// sections, as scale will be changed.
	

	*Time_Window = *New_Time_Window;
	g_info("New time window HOOK : %u, %u to %u, %u",
			Time_Window->start_time.tv_sec,
			Time_Window->start_time.tv_nsec,
			Time_Window->time_width.tv_sec,
			Time_Window->time_width.tv_nsec);

  drawing_data_request(Control_Flow_Data->Drawing,
			&Control_Flow_Data->Drawing->Pixmap,
			0, 0,
			Control_Flow_Data->Drawing->width,
		   	Control_Flow_Data->Drawing->height);

	drawing_refresh(Control_Flow_Data->Drawing,
			0, 0,
			Control_Flow_Data->Drawing->width,
			Control_Flow_Data->Drawing->height);

}

void update_current_time_hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	LttTime* Current_Time = 
		guicontrolflow_get_current_time(Control_Flow_Data);
	*Current_Time = *((LttTime*)call_data);
	g_info("New Current time HOOK : %u, %u", Current_Time->tv_sec,
							Current_Time->tv_nsec);

	/* If current time is inside time interval, just move the highlight
	 * bar */

	/* Else, we have to change the time interval. We have to tell it
	 * to the main window. */
	/* The time interval change will take care of placing the current
	 * time at the center of the visible area */
	
}

