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

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

#include <glib.h>
#include <gmodule.h>
#include <lttv/module.h>
#include <lttv/gtkTraceSet.h>

#include "cfv.h"
#include "eventhooks.h"

#include "../icons/hGuiControlFlowInsert.xpm"

static LttvModule *Main_Win_Module;


/** Array containing instanced objects. Used when module is unloaded */
GSList *g_control_flow_data_list = NULL ;




/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the Control Flow Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init(LttvModule *self, int argc, char *argv[]) {

  Main_Win_Module = lttv_module_require(self, "mainwin", argc, argv);
  
  if(Main_Win_Module == NULL)
  {
    g_critical("Can't load Control Flow Viewer : missing mainwin\n");
    return;
  }
  
  g_info("GUI ControlFlow Viewer init()");

  /* Register the toolbar insert button */
  toolbar_item_reg(hGuiControlFlowInsert_xpm, "Insert Control Flow Viewer",
      h_guicontrolflow);

  /* Register the menu item insert entry */
  menu_item_reg("/", "Insert Control Flow Viewer", h_guicontrolflow);
  
}

void destroy_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI Control Flow Viewer");
  guicontrolflow_destructor_full((ControlFlowData*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
  g_info("GUI Control Flow Viewer destroy()");
  int i;

  g_slist_foreach(g_control_flow_data_list, destroy_walk, NULL );
  
  g_slist_free(g_control_flow_data_list);

  /* Unregister the toolbar insert button */
  toolbar_item_unreg(h_guicontrolflow);

  /* Unregister the menu item insert entry */
  menu_item_unreg(h_guicontrolflow);
  
}
