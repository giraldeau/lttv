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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <lttv/lttv.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/support.h>

#include "cfv.h"
#include "drawing.h"
#include "processlist.h"
#include "eventhooks.h"
#include "lttv_plugin_cfv.h"

extern GSList *g_control_flow_data_list;

static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  Drawing_t *drawing = (Drawing_t*)user_data;

  gtk_widget_set_size_request(drawing->ruler, -1, allocation->height);
  //gtk_widget_queue_resize(drawing->padding);
  //gtk_widget_queue_resize(drawing->ruler);
  gtk_container_check_resize(GTK_CONTAINER(drawing->ruler_hbox));
  return 0;
}

gboolean cfv_scroll_event(GtkWidget *widget, GdkEventScroll *event,
    gpointer data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*)data;
  unsigned int cell_height =
    get_cell_height(
        GTK_TREE_VIEW(control_flow_data->process_list->process_list_widget));
  gdouble new;

  switch(event->direction) {
    case GDK_SCROLL_UP:
      {
        new = gtk_adjustment_get_value(control_flow_data->v_adjust) 
                                  - cell_height;
      }
      break;
    case GDK_SCROLL_DOWN:
      {
        new = gtk_adjustment_get_value(control_flow_data->v_adjust) 
                                  + cell_height;
      }
      break;
    default:
      return FALSE;
  }
  if(new >= control_flow_data->v_adjust->lower &&
      new <= control_flow_data->v_adjust->upper 
          - control_flow_data->v_adjust->page_size)
    gtk_adjustment_set_value(control_flow_data->v_adjust, new);
  return TRUE;
}


/* Toolbar callbacks */
static void        property_button      (GtkToolButton *toolbutton,
                                          gpointer       user_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*)user_data;

  g_printf("CFV Property button clicked\n");

}

/*****************************************************************************
 *                     Control Flow Viewer class implementation              *
 *****************************************************************************/
/**
 * Control Flow Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
ControlFlowData *
resourceview(LttvPluginTab *ptab)
{
  Tab *tab = ptab->tab;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *process_list_widget, *drawing_widget, *drawing_area;
  //ControlFlowData* control_flow_data = g_new(ControlFlowData,1) ;
  LttvPluginCFV *plugin_cfv = g_object_new(LTTV_TYPE_PLUGIN_CFV, NULL);
  GtkTooltips *tooltips = gtk_tooltips_new();
  ControlFlowData* control_flow_data = plugin_cfv->cfd;
  control_flow_data->ptab = ptab;
  control_flow_data->tab = ptab->tab;

  control_flow_data->v_adjust = 
    GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  /* Value */
              0.0,  /* Lower */
              0.0,  /* Upper */
              0.0,  /* Step inc. */
              0.0,  /* Page inc. */
              0.0));  /* page size */

  /* Create the drawing */
  control_flow_data->drawing = drawing_construct(control_flow_data);
  
  drawing_widget = 
    drawing_get_widget(control_flow_data->drawing);
  
  drawing_area = 
    drawing_get_drawing_area(control_flow_data->drawing);

  control_flow_data->number_of_process = 0;
  control_flow_data->background_info_waiting = 0;

  /* Create the Process list */
  control_flow_data->process_list = processlist_construct();
  
  process_list_widget = 
    processlist_get_widget(control_flow_data->process_list);
  
  gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(process_list_widget),
                                GTK_ADJUSTMENT(
                                   control_flow_data->v_adjust));

  g_signal_connect (G_OBJECT(process_list_widget),
        "scroll-event",
        G_CALLBACK (cfv_scroll_event),
        (gpointer)control_flow_data);
   g_signal_connect (G_OBJECT(drawing_area),
        "scroll-event",
        G_CALLBACK (cfv_scroll_event),
        (gpointer)control_flow_data);
  
  g_signal_connect (G_OBJECT(control_flow_data->process_list->button),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)control_flow_data->drawing);
#if 0  /* not ready */
  g_signal_connect (
       // G_OBJECT(control_flow_data->process_list->process_list_widget),
        G_OBJECT(control_flow_data->process_list->list_store),
        "row-changed",
        G_CALLBACK (tree_row_activated),
        (gpointer)control_flow_data);
#endif //0
  
  control_flow_data->hbox = gtk_hbox_new(FALSE, 1);
  control_flow_data->toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(control_flow_data->toolbar),
                              GTK_ORIENTATION_VERTICAL);

//  tmp_toolbar_icon = create_pixmap (main_window_get_widget(tab),
//      "properties.png");
//  gtk_widget_show(tmp_toolbar_icon);
//  control_flow_data->button_prop = gtk_tool_button_new(tmp_toolbar_icon,
//      "Properties");
//  g_signal_connect (G_OBJECT(control_flow_data->button_prop),
//        "clicked",
//        G_CALLBACK (property_button),
//        (gpointer)control_flow_data);
//  gtk_toolbar_insert(GTK_TOOLBAR(control_flow_data->toolbar),
//      control_flow_data->button_prop,
//      1);

  gtk_toolbar_set_style(GTK_TOOLBAR(control_flow_data->toolbar),
      GTK_TOOLBAR_ICONS);

  gtk_box_pack_start(GTK_BOX(control_flow_data->hbox), 
      control_flow_data->toolbar,
      FALSE, FALSE, 0);
  control_flow_data->h_paned = gtk_hpaned_new();
  control_flow_data->box = gtk_event_box_new();
  gtk_box_pack_end(GTK_BOX(control_flow_data->hbox), 
      control_flow_data->box,
      TRUE, TRUE, 0);
  control_flow_data->top_widget = control_flow_data->hbox;
  plugin_cfv->parent.top_widget = control_flow_data->top_widget;
  gtk_container_add(GTK_CONTAINER(control_flow_data->box),
                    control_flow_data->h_paned);
      
  gtk_paned_pack1(GTK_PANED(control_flow_data->h_paned),
                  process_list_widget, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(control_flow_data->h_paned),
                  drawing_widget, TRUE, TRUE);
  
  gtk_container_set_border_width(GTK_CONTAINER(control_flow_data->box), 1);
  
  /* Set the size of the drawing area */
  //drawing_Resize(drawing, h, w);

  /* Get trace statistics */
  //control_flow_data->Trace_Statistics = get_trace_statistics(Trace);

  gtk_widget_show(drawing_widget);
  gtk_widget_show(process_list_widget);
  gtk_widget_show(control_flow_data->h_paned);
  gtk_widget_show(control_flow_data->box);
  gtk_widget_show(control_flow_data->toolbar);
//  gtk_widget_show(GTK_WIDGET(control_flow_data->button_prop));
  gtk_widget_show(control_flow_data->hbox);
  
  g_object_set_data_full(
      G_OBJECT(control_flow_data->top_widget),
      "plugin_data",
      plugin_cfv,
      (GDestroyNotify)guicontrolflow_destructor);
    
  g_object_set_data(
      G_OBJECT(drawing_area),
      "resourceview_data",
      control_flow_data);
        
  g_object_set_data(
      G_OBJECT(control_flow_data->process_list->process_list_widget),
      "resourceview_data",
      control_flow_data);
        
  g_control_flow_data_list = g_slist_append(
      g_control_flow_data_list,
      plugin_cfv);
  
  control_flow_data->filter = NULL;

  //WARNING : The widget must be 
  //inserted in the main window before the drawing area
  //can be configured (and this must happen before sending
  //data)
  
  return control_flow_data;

}

/* Destroys widget also */
void
guicontrolflow_destructor_full(gpointer data)
{
  LttvPluginCFV *plugin_cfv = (LttvPluginCFV*)data;
  g_info("CFV.c : guicontrolflow_destructor_full, %p", plugin_cfv);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guicontrolflow_get_widget(plugin_cfv->cfd)))
    gtk_widget_destroy(guicontrolflow_get_widget(plugin_cfv->cfd));
  //control_flow_data->mw = NULL;
  //FIXME guicontrolflow_destructor(control_flow_data);
}

/* When this destructor is called, the widgets are already disconnected */
void
guicontrolflow_destructor(gpointer data)
{
  LttvPluginCFV *plugin_cfv = (LttvPluginCFV*)data;
  Tab *tab = plugin_cfv->cfd->tab;
  ControlFlowData *control_flow_data = plugin_cfv->cfd;
  
  g_info("CFV.c : guicontrolflow_destructor, %p", plugin_cfv);
  g_info("%p, %p, %p", update_time_window_hook, plugin_cfv, tab);
  if(GTK_IS_WIDGET(guicontrolflow_get_widget(plugin_cfv->cfd)))
    g_info("widget still exists");
  
  lttv_filter_destroy(plugin_cfv->cfd->filter);
  /* Process List is removed with it's widget */
  //ProcessList_destroy(control_flow_data->process_list);
  if(tab != NULL)
  {
      /* Delete reading hooks */
    lttvwindow_unregister_traceset_notify(tab,
        traceset_notify,
        control_flow_data);
    
    lttvwindow_unregister_time_window_notify(tab,
        update_time_window_hook,
        control_flow_data);
  
    lttvwindow_unregister_current_time_notify(tab,
        update_current_time_hook,
        control_flow_data);

    lttvwindow_unregister_redraw_notify(tab, redraw_notify, control_flow_data);
    lttvwindow_unregister_continue_notify(tab,
                                          continue_notify,
                                          control_flow_data);
    
    lttvwindow_events_request_remove_all(control_flow_data->tab,
                                         control_flow_data);

  }
  lttvwindowtraces_background_notify_remove(control_flow_data);
  g_control_flow_data_list = 
         g_slist_remove(g_control_flow_data_list, control_flow_data);

  g_info("CFV.c : guicontrolflow_destructor end, %p", control_flow_data);
  //g_free(control_flow_data);
  g_object_unref(plugin_cfv);
}


