/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include <lttv/hook.h>
#include <lttv/common.h>

#include "Event_Hooks.h"
#include "CFV.h"
#include "Process_List.h"
#include "Drawing.h"

void send_test_data(ProcessList *Process_List, Drawing_t *Drawing)
{
	guint height, size;
	int i;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;

	LttTime birth;
	birth.tv_sec = 12000;
	birth.tv_nsec = 55500;

	ProcessList_add(Process_List,
			1,
			&birth,
			&height);
	Drawing_Insert_Square( Drawing, height, 5);
	
	g_critical("height : %u", height);
	
	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	ProcessList_add(Process_List,
			156,
			&birth,
			&height);
	Drawing_Insert_Square( Drawing, height, 5);

	g_critical("height : %u", height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	ProcessList_add(Process_List,
			10,
			&birth,
			&height);
	Drawing_Insert_Square( Drawing, height, 5);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		ProcessList_add(Process_List,
				i,
				&birth,
				&height);
	Drawing_Insert_Square( Drawing, height, 5);

	}
	g_critical("height : %u", height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	ProcessList_add(Process_List,
			10,
			&birth,
			&height);
	Drawing_Insert_Square( Drawing, height, 5);
	g_critical("height : %u", height);

	ProcessList_add(Process_List,
			10000,
			&birth,
			&height);
	Drawing_Insert_Square( Drawing, height, 5);
	g_critical("height : %u", height);


	ProcessList_get_process_pixels(Process_List,
				10000,
				&birth,
				&height, &size);
	ProcessList_remove( 	Process_List,
				10000,
				&birth);

	Drawing_Remove_Square( Drawing, height, 5);
	
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
hGuiControlFlow(mainWindow *pmParentWindow)
{
	g_critical("hGuiControlFlow");
	ControlFlowData *Control_Flow_Data = GuiControlFlow() ;

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
