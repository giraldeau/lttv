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

#include "cpucfv.h"
#include "cpueventhooks.h"

#include "hCpuUsage.xpm"


GQuark LTT_NAME_CPU;

/** Array containing instanced objects. Used when module is unloaded */
GSList *g_cpu_control_flow_data_list = NULL ;

/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the Cpugram Control Flow Viewer functionnality through the
 * gtkTraceSet API.
 */
static void cpu_init() {

  g_info("GUI CPU Usage Viewer init()");

  /* Register the toolbar insert button and menu entry*/
  lttvwindow_register_constructor("cpu",
                                  "/",
                                  "Insert CPU Usage Viewer",
                                  hCpuUsage_xpm,
                                  "Insert CPU Usage Viewer",
                                  h_guicpucontrolflow);

  LTT_NAME_CPU = g_quark_from_string("/cpu");
}

void cpu_destroy_walk(gpointer data, gpointer user_data)
{
  g_info("Walk destroy GUI CPU Usage Viewer");
  guicpucontrolflow_destructor_full((CpuControlFlowData*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void cpu_destroy() {
  g_info("GUI CPU Usage Viewer destroy()");

  g_slist_foreach(g_cpu_control_flow_data_list, cpu_destroy_walk, NULL );
  
  g_slist_free(g_cpu_control_flow_data_list);

  /* Unregister the toolbar insert button and menu entry */
  lttvwindow_unregister_constructor(h_guicpucontrolflow);
  
}


LTTV_MODULE("guicpuview", "CPU usage viewer", \
    "Graphical module to view cpu usage over time", \
    cpu_init, cpu_destroy, "lttvwindow")
