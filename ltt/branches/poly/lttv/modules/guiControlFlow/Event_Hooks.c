/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


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


/* NOTE : no drawing data should be sent there, as the drawing widget
 * has not been initialized */
void send_test_drawing(ProcessList *Process_List,
			Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			gint x, gint y, // y not used here?
		   	gint width,
			gint height) // height won't be used here ?
{
	int i;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *FontDesc;// = pango_font_description_new();
	gint Font_Size;

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
	g_critical("we have : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	ProcessList_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	
	g_critical("we draw : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	Drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	pango_layout_set_text(layout, "Test", -1);
	gdk_draw_layout(Pixmap, Drawing->Drawing_Area_V->style->black_gc,
			0, y+height, layout);

	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	ProcessList_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	

	Drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_critical("y : %u, height : %u", y, height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	ProcessList_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	

	Drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_critical("y : %u, height : %u", y, height);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		ProcessList_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		

		Drawing_draw_line(
			Drawing, Pixmap, x,
			y+(height/2), x + width, y+(height/2),
			Drawing->Drawing_Area_V->style->black_gc);

		g_critical("y : %u, height : %u", y, height);

	}

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	ProcessList_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	

	Drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_critical("y : %u, height : %u", y, height);


	pango_font_description_set_size(FontDesc, Font_Size);
	g_free(layout);
	//g_free(context);
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

	ProcessList_add(Process_List,
			1,
			&birth,
			&y);
	ProcessList_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	Drawing_Insert_Square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	ProcessList_add(Process_List,
			156,
			&birth,
			&y);
	ProcessList_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	Drawing_Insert_Square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	ProcessList_add(Process_List,
			10,
			&birth,
			&height);
	ProcessList_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	Drawing_Insert_Square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//Drawing_Insert_Square( Drawing, height, 5);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		ProcessList_add(Process_List,
				i,
				&birth,
				&height);
		ProcessList_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		Drawing_Insert_Square( Drawing, y, height);
	
	//	g_critical("y : %u, height : %u", y, height);
	
	}
	//g_critical("height : %u", height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	ProcessList_add(Process_List,
			10,
			&birth,
			&y);
	ProcessList_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	Drawing_Insert_Square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	ProcessList_add(Process_List,
			10000,
			&birth,
			&height);
	ProcessList_get_process_pixels(Process_List,
					10000,
					&birth,
					&y,
					&height);
	Drawing_Insert_Square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//Drawing_Insert_Square( Drawing, height, 5);
	//g_critical("height : %u", height);


	ProcessList_get_process_pixels(Process_List,
				10000,
				&birth,
				&y, &height);
	ProcessList_remove( 	Process_List,
				10000,
				&birth);

	Drawing_Remove_Square( Drawing, y, height);
	
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
hGuiControlFlow(MainWindow *pmParentWindow, LttvTracesetSelector * s, char * key)
{
	g_critical("hGuiControlFlow");
	ControlFlowData *Control_Flow_Data = GuiControlFlow() ;

	get_time_window(pmParentWindow,
			GuiControlFlow_get_Time_Window(Control_Flow_Data));
	get_current_time(pmParentWindow,
			GuiControlFlow_get_Current_Time(Control_Flow_Data));

	// Unreg done in the GuiControlFlow_Destructor
	reg_update_time_window(Update_Time_Window_Hook, Control_Flow_Data,
				pmParentWindow);
	reg_update_current_time(Update_Current_Time_Hook, Control_Flow_Data,
				pmParentWindow);
	return GuiControlFlow_get_Widget(Control_Flow_Data) ;
	
}

int Event_Selected_Hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	guint *Event_Number = (guint*) call_data;

	g_critical("DEBUG : event selected by main window : %u", *Event_Number);
	
//	Control_Flow_Data->Currently_Selected_Event = *Event_Number;
//	Control_Flow_Data->Selected_Event = TRUE ;
	
//	Tree_V_set_cursor(Control_Flow_Data);

}

#ifdef DEBUG
/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in Event_Request.
 */
int Draw_Before_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	EventsContext Events_Context = (EventsContext*)call_data;
	
	Event_Request->Events_Context = Events_Context;

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
int Draw_Event_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	
	return 0;
}


int Draw_After_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	
	g_free(Event_Request);
	return 0;
}
#endif




void Update_Time_Window_Hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	TimeWindow* Time_Window = 
		GuiControlFlow_get_Time_Window(Control_Flow_Data);
	TimeWindow *New_Time_Window = ((TimeWindow*)call_data);

	// As the time interval change will mostly be used for
	// zoom in and out, it's not useful to keep old drawing
	// sections, as scale will be changed.
	

	*Time_Window = *New_Time_Window;
	g_critical("New time window HOOK : %u, %u to %u, %u",
			Time_Window->start_time.tv_sec,
			Time_Window->start_time.tv_nsec,
			Time_Window->time_width.tv_sec,
			Time_Window->time_width.tv_nsec);

   	Drawing_Data_Request(Control_Flow_Data->Drawing,
			&Control_Flow_Data->Drawing->Pixmap,
			0, 0,
			Control_Flow_Data->Drawing->width,
		   	Control_Flow_Data->Drawing->height);

	Drawing_Refresh(Control_Flow_Data->Drawing,
			0, 0,
			Control_Flow_Data->Drawing->width,
			Control_Flow_Data->Drawing->height);

}

void Update_Current_Time_Hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	LttTime* Current_Time = 
		GuiControlFlow_get_Current_Time(Control_Flow_Data);
	*Current_Time = *((LttTime*)call_data);
	g_critical("New Current time HOOK : %u, %u", Current_Time->tv_sec,
							Current_Time->tv_nsec);

	/* If current time is inside time interval, just move the highlight
	 * bar */

	/* Else, we have to change the time interval. We have to tell it
	 * to the main window. */
	/* The time interval change will take care of placing the current
	 * time at the center of the visible area */
	
}

