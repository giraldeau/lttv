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
 * viewer's insertion menu item and toolbar icon by calling viewer.h's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * This plugin uses the gdk library to draw the events and gtk to interact
 * with the user.
 *
 * Author : Mathieu Desnoyers, June 2003
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttvwindow/lttvwindow.h>

#include "xfv.h"
#include "lttv_plugin_xfv.h"
#include "xenoltt_eventhooks.h"

#include "hGuiControlFlowInsert.xpm"
#include "hLegendInsert.xpm"
#include "hGuiSimulationInsert.xpm"

GQuark LTT_NAME_CPU;

/** Array containing instanced objects. Used when module is unloaded */
GSList *g_xenoltt_data_list = NULL ;

GSList *g_legend_list = NULL ;

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

  g_info("GUI Xenomai Event Viewer init()");

  /* Register the toolbar insert button and menu entry*/
  lttvwindow_register_constructor("guixenoltt",
                                  "/",
                                  "Insert Xenomai Event Viewer",
                                  hGuiXenoLTTInsert_xpm,
                                  "Insert Xenomai Event Viewer",
                                  h_guixenoltt);
  
  lttvwindow_register_constructor("guixenolttlegend",
                                  "/",
                                  "Popup Xenomai Event Viewer Legend",
                                  hLegendInsert_xpm,
                                  "Popup Xenomai Event Viewer Legend",
                                  h_xenolttlegend);

  lttvwindow_register_constructor("guixenolttsimulation",
                                  "/",
                                  "Popup Xenomai Simulation Window",
                                  hGuiSimulationInsert_xpm,
                                  "Popup Xenomai Simulation Window",
                                  h_xenolttsimulation);

  LTT_NAME_CPU = g_quark_from_string("/cpu");
  
}

void destroy_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI Xenomai Event Viewer");
  guixenoltt_destructor_full((LttvPluginXFV*)data);
}

void destroy_legend_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI Xenomai Event Viewer");
  xenolttlegend_destructor((GtkWindow*)data);
}

void destroy_simulation_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI Xenomai Simulation Window");
  xenolttlegend_destructor((GtkWindow*)data);
}
/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  g_info("GUI Xenomai Event Viewer destroy()");

  g_slist_foreach(g_xenoltt_data_list, destroy_walk, NULL );
  
  g_slist_free(g_xenoltt_data_list);

  g_slist_foreach(g_legend_list, destroy_legend_walk, NULL );
  
  g_slist_free(g_xenoltt_data_list);
  
  /* Unregister the toolbar insert button and menu entry */
  lttvwindow_unregister_constructor(h_guixenoltt);
  lttvwindow_unregister_constructor(h_xenolttlegend);
  lttvwindow_unregister_constructor(h_xenolttsimulation);
}


LTTV_MODULE("guixenoltt", "Xenomai Event viewer", \
    "Graphical module to view Xenomai Task state and control flow", \
    init, destroy, "lttvwindow", "xenoltt_sim")
