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

#include <glib.h>
#include <gmodule.h>
#include <gtk.h>
#include <gdk.h>

#include <lttv/module.h>

#include "guiControlFlow.h"
#include "icons/guiControlFlowInsert.xpm"

//! Event Viewer's constructor
GtkWidget *guiControlFlow(GtkWidget *ParentWindow);

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

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
	g_critical("GUI Control Flow Viewer destroy()");

	/* Unregister the toolbar insert button */
	ToolbarItemUnreg(guiEvent);

	/* Unregister the menu item insert entry */
	MenuItemUnreg(guiEvents);
}

/**
 * Control Flow Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
static GtkWidget *
guiEvents(GtkWidget *ParentWindow)
{
	GtkWidget *drawing_area = gtk_drawing_area_new ();

	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                          G_CALLBACK (expose_event_callback), NULL);
}



/*\@}*/
