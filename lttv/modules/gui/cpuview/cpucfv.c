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
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <lttv/lttv.h>
#include <lttvwindow/lttvwindow.h>

#include "cpucfv.h"
#include "cpudrawing.h"
#include "cpubuttonwidget.h"
#include "cpueventhooks.h"

#define PREDEFINED_HEIGHT 5000

extern GSList *g_cpu_control_flow_data_list;

static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  cpuDrawing_t *drawing = (cpuDrawing_t*)user_data;

  gtk_widget_set_size_request(drawing->ruler, -1, allocation->height);
  //gtk_widget_queue_resize(drawing->padding);
  //gtk_widget_queue_resize(drawing->ruler);
  gtk_container_check_resize(GTK_CONTAINER(drawing->ruler_hbox));
  return 0;
}


/*****************************************************************************
 *              Cpu Control Flow Viewer class implementation              *
 *****************************************************************************/
/**
 * Cpu Control Flow Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
CpuControlFlowData *
guicpucontrolflow(LttvPluginTab *ptab)
{
  GtkWidget *button_widget, *drawing_widget, *drawing_area;
  GtkWidget *buttonP,*buttonM;
  cpuDrawing_t *drawing;
  CpuControlFlowData* cpu_control_flow_data = g_new(CpuControlFlowData,1) ;
  
  cpu_control_flow_data->ptab = ptab;
  cpu_control_flow_data->tab = ptab->tab;
  cpu_control_flow_data->max_height = PREDEFINED_HEIGHT;
 
  /*cpu_control_flow_data->v_adjust =
    GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  // Value 
              0.0,  // Lower 
              0.0,  // Upper 
              0.0,  // Step inc. 
              0.0,  // Page inc. 
              0.0));  // page size */

  // Create the drawing 
  cpu_control_flow_data->drawing = cpu_drawing_construct(cpu_control_flow_data);
  
  drawing = cpu_control_flow_data->drawing;
  drawing_widget = cpu_drawing_get_widget(drawing);
  
  drawing_area = cpu_drawing_get_drawing_area(drawing);

  cpu_control_flow_data->number_of_process = 0;
  
  ///cpu_control_flow_data->background_info_waiting = 0;

  // Create the Button widget 
  cpu_control_flow_data->buttonwidget = cpu_buttonwidget_construct(cpu_control_flow_data);
  
  button_widget = cpu_buttonwidget_get_widget( cpu_control_flow_data-> buttonwidget);
  buttonP =cpu_control_flow_data-> buttonwidget->buttonP;
  buttonM =cpu_control_flow_data-> buttonwidget->buttonM;

  //set the size of ruler fix
  gtk_widget_set_size_request(cpu_control_flow_data->drawing->ruler, -1, 25);
  gtk_container_check_resize(GTK_CONTAINER(cpu_control_flow_data->drawing->ruler_hbox));

/*//or set the size of ruler by button P
  g_signal_connect (G_OBJECT(buttonP),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)cpu_control_flow_data->drawing);*/

 
  ///cpu_control_flow_data->h_paned = gtk_hpaned_new();

  ///changed for cpugram
  cpu_control_flow_data->box = gtk_hbox_new(FALSE, 0);
  cpu_control_flow_data->ev_box = gtk_event_box_new();

 /// cpu_control_flow_data->top_widget =gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(cpu_control_flow_data->ev_box),
                        drawing_widget);
 ///Now add button widget and drawing area on the top_widget.
  gtk_box_pack_start (GTK_BOX (cpu_control_flow_data->box),
            button_widget,FALSE,FALSE, 10);
  gtk_box_pack_end (GTK_BOX (cpu_control_flow_data->box),
                cpu_control_flow_data->ev_box,TRUE,TRUE, 0);
  cpu_control_flow_data->top_widget = cpu_control_flow_data->box;

  /*gtk_container_add(GTK_CONTAINER(cpu_control_flow_data->box),
                    cpu_control_flow_data->h_paned);
      
  gtk_paned_pack1(GTK_PANED(cpu_control_flow_data->h_paned),
                  button_widget->vbox1, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(cpu_control_flow_data->h_paned),
                  drawing_widget, TRUE, TRUE);
  */
  gtk_container_set_border_width(GTK_CONTAINER(cpu_control_flow_data->ev_box), 1);
  
  // Set the size of the drawing area 
  //drawing_Resize(drawing, h, w);

  // Get trace statistics 
  //cpu_control_flow_data->Trace_Statistics = get_trace_statistics(Trace);

  gtk_widget_show(drawing_widget);
  gtk_widget_show(button_widget);
  /*gtk_widget_show(cpu_control_flow_data->h_paned);*/
  gtk_widget_show(cpu_control_flow_data->box);
  gtk_widget_show(cpu_control_flow_data->ev_box);
  gtk_widget_show(cpu_control_flow_data->top_widget);
  g_object_set_data_full(
      G_OBJECT(cpu_control_flow_data->top_widget),
      "cpu_control_flow_data",
      cpu_control_flow_data,
      (GDestroyNotify)guicpucontrolflow_destructor);
    
  g_object_set_data(
      G_OBJECT(drawing_area),
      "cpu_control_flow_data",
      cpu_control_flow_data);
        
  g_cpu_control_flow_data_list = g_slist_append(
      g_cpu_control_flow_data_list,
      cpu_control_flow_data);
  cpu_control_flow_data->number_of_process =g_array_new (FALSE,
                                             TRUE,
                                             sizeof(guint));
  g_array_set_size (cpu_control_flow_data->number_of_process,
                                             drawing_area->allocation.width);
  
  //WARNING : The widget must be 
  //inserted in the main window before the drawing area
  //can be configured (and this must happend before sending
  //data)
  
  return cpu_control_flow_data;

}

/* Destroys widget also */
void guicpucontrolflow_destructor_full(CpuControlFlowData *cpu_control_flow_data)
{
  g_info("HISTOCFV.c : guicpucontrolflow_destructor_full, %p", cpu_control_flow_data);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guicpucontrolflow_get_widget(cpu_control_flow_data)))
    gtk_widget_destroy(guicpucontrolflow_get_widget(cpu_control_flow_data));
  //cpu_control_flow_data->mw = NULL;
  //FIXME guicpucontrolflow_destructor(cpu_control_flow_data);
}

/* When this destructor is called, the widgets are already disconnected */
void guicpucontrolflow_destructor(CpuControlFlowData *cpu_control_flow_data)
{
  Tab *tab = cpu_control_flow_data->tab;
  
  g_info("HISTOCFV.c : guicpucontrolflow_destructor, %p", cpu_control_flow_data);
  g_info("%p, %p, %p", cpu_update_time_window_hook, cpu_control_flow_data, tab);
  if(GTK_IS_WIDGET(guicpucontrolflow_get_widget(cpu_control_flow_data)))
    g_info("widget still exists");
  
  /* ButtonWidget is removed with it's widget */
  //buttonwidget_destroy(cpu_control_flow_data->buttonwidget);
  if(tab != NULL)
  {
      // Delete reading hooks
    lttvwindow_unregister_traceset_notify(tab,
        cpu_traceset_notify,
        cpu_control_flow_data);
    
    lttvwindow_unregister_time_window_notify(tab,
        cpu_update_time_window_hook,
        cpu_control_flow_data);
  
    lttvwindow_unregister_current_time_notify(tab,
        cpu_update_current_time_hook,
        cpu_control_flow_data);

    lttvwindow_unregister_redraw_notify(tab, cpu_redraw_notify, cpu_control_flow_data);
    lttvwindow_unregister_continue_notify(tab,
                                          cpu_continue_notify,
                                          cpu_control_flow_data);
    
    lttvwindow_events_request_remove_all(cpu_control_flow_data->tab,
                                         cpu_control_flow_data);

    lttvwindow_unregister_filter_notify(tab,
                        cpu_filter_changed, cpu_control_flow_data);

  }
  lttvwindowtraces_background_notify_remove(cpu_control_flow_data);
  g_cpu_control_flow_data_list =
         g_slist_remove(g_cpu_control_flow_data_list,cpu_control_flow_data);

  g_array_free(cpu_control_flow_data->number_of_process, TRUE);

  g_info("HISTOCFV.c : guicpucontrolflow_destructor end, %p", cpu_control_flow_data);
  g_free(cpu_control_flow_data);
 
}


