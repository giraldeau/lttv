/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Parisa heidari (inspired from CFV by Mathieu Desnoyers)
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttvwindow/lttvwindow.h>

#include "histocfv.h"
#include "histoeventhooks.h"

#include "hHistogramInsert.xpm"


GQuark LTT_NAME_CPU;

/** Array containing instanced objects. Used when module is unloaded */
GSList *g_histo_control_flow_data_list = NULL ;

/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the Histogram Control Flow Viewer functionnality through the
 * gtkTraceSet API.
 */
static void histo_init() {

  g_info("GUI ControlFlow Viewer init()");

  /* Register the toolbar insert button and menu entry*/
  lttvwindow_register_constructor("histogram",
                                  "/",
                                  "Insert Histogram Viewer",
                                  hHistogramInsert_xpm,
                                  "Insert Histogram Viewer",
                                  h_guihistocontrolflow);

  LTT_NAME_CPU = g_quark_from_string("/cpu");
}

void histo_destroy_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI Histogram Control Flow Viewer");
  guihistocontrolflow_destructor_full((HistoControlFlowData*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void histo_destroy() {
  g_info("GUI Histogram Control Flow Viewer destroy()");

  g_slist_foreach(g_histo_control_flow_data_list, histo_destroy_walk, NULL );
  
  g_slist_free(g_histo_control_flow_data_list);

  /* Unregister the toolbar insert button and menu entry */
  lttvwindow_unregister_constructor(h_guihistocontrolflow);
  
}


LTTV_MODULE("guihistogram", "Event Histogram viewer", \
    "Graphical module to view events' density histogram", \
    histo_init, histo_destroy, "lttvwindow")
