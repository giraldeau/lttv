/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */



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
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttvgui/gtktraceset.h>

#include "cfv.h"
#include "eventhooks.h"

#include "hGuiControlFlowInsert.xpm"

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
static void init() {

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
static void destroy() {
  g_info("GUI Control Flow Viewer destroy()");
  int i;

  g_slist_foreach(g_control_flow_data_list, destroy_walk, NULL );
  
  g_slist_free(g_control_flow_data_list);

  /* Unregister the toolbar insert button */
  toolbar_item_unreg(h_guicontrolflow);

  /* Unregister the menu item insert entry */
  menu_item_unreg(h_guicontrolflow);
  
}


LTTV_MODULE("guicontrolflow", "Control flow viewer", \
    "Graphical module to view processes state and control flow", \
    init, destroy, "lttvwindow")
