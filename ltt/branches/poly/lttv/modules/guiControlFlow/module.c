/*! \defgroup guiEvents libguiControlFlow: The GUI ControlFlow display plugin */
/*\@{*/

/*! \file guiControlFlow.c
 * \brief Graphical plugin for showing control flow of a trace.
 *
 * This plugin adds a Control Flow Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling gtkTraceSet's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * This plugin uses the gdk library to draw the events and gtk to interact
 * with the user.
 *
 * Author : Mathieu Desnoyers, June 2003
 */

#include <gmodule.h>
#include <lttv/module.h>
#include <lttv/gtkTraceSet.h>

#include "CFV.h"
#include "Event_Hooks.h"

/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the Control Flow Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init() {
	g_critical("GUI ControlFlow Viewer init()");

	/* Register the toolbar insert button */
	ToolbarItemReg(guiEventsInsert_xpm, "Insert Control Flow Viewer", guiEvent);

	/* Register the menu item insert entry */
	MenuItemReg("/", "Insert Control Flow Viewer", guiEvent);
	
}

void destroy_walk(gpointer data, gpointer user_data)
{
	GuiControlFlow_Destructor((ControlFlowData*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
	g_critical("GUI Control Flow Viewer destroy()");
	int i;

	ControlFlowData *Control_Flow_Data;
	
	g_critical("GUI Event Viewer destroy()");

	g_slist_foreach(sControl_Flow_Data_List, destroy_walk, NULL );
	
	/* Unregister the toolbar insert button */
	//ToolbarItemUnreg(hGuiEvents);

	/* Unregister the menu item insert entry */
	//MenuItemUnreg(hGuiEvents);
}

