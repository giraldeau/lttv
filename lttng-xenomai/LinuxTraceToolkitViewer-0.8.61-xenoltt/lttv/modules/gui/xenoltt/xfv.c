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

#include "xfv.h"
#include "xenoltt_drawing.h"
#include "xenoltt_threadlist.h"
#include "xenoltt_eventhooks.h"
#include "lttv_plugin_xfv.h"

extern GSList *g_xenoltt_data_list;

static gboolean header_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data){
  XenoLtt_Drawing_t *drawing = (XenoLtt_Drawing_t*)user_data;

  gtk_widget_set_size_request(drawing->ruler, -1, allocation->height);
  gtk_container_check_resize(GTK_CONTAINER(drawing->ruler_hbox));
  return 0;
}

gboolean xfv_scroll_event(GtkWidget *widget, GdkEventScroll *event,gpointer data){
  XenoLTTData *xenoltt_data = (XenoLTTData*)data;
  unsigned int cell_height = get_cell_height(GTK_TREE_VIEW(xenoltt_data->thread_list->thread_list_widget));
  gdouble new;

  switch(event->direction) {
    case GDK_SCROLL_UP:
      {
        new = gtk_adjustment_get_value(xenoltt_data->v_adjust) - cell_height;
      }
      break;
    case GDK_SCROLL_DOWN:
      {
        new = gtk_adjustment_get_value(xenoltt_data->v_adjust) + cell_height;
      }
      break;
    default:
      return FALSE;
  }
  if(new >= xenoltt_data->v_adjust->lower &&
      new <= xenoltt_data->v_adjust->upper 
          - xenoltt_data->v_adjust->page_size)
    gtk_adjustment_set_value(xenoltt_data->v_adjust, new);
  return TRUE;
}


/* Toolbar callbacks */
static void property_button(GtkToolButton *toolbutton, gpointer user_data)
{
  XenoLTTData *xenoltt_data = (XenoLTTData*)user_data;

  g_printf("CFV Property button clicked\n");

}

/* Toolbar callbacks */
static void        filter_button      (GtkToolButton *toolbutton,
                                          gpointer       user_data)
{
  LttvPluginXFV *plugin_xfv = (LttvPluginXFV*)user_data;
  LttvAttribute *attribute;
  LttvAttributeValue value;
  gboolean ret;
  g_printf("Filter button clicked\n");

  attribute = LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
        LTTV_IATTRIBUTE(lttv_global_attributes()),
        LTTV_VIEWER_CONSTRUCTORS));
  g_assert(attribute);

  ret = lttv_iattribute_find_by_path(LTTV_IATTRIBUTE(attribute),
      "guifilter", LTTV_POINTER, &value);
  g_assert(ret);
  lttvwindow_viewer_constructor constructor =
    (lttvwindow_viewer_constructor)*(value.v_pointer);
  if(constructor) constructor(&plugin_xfv->parent);
  else g_warning("Filter module not loaded.");

  //FIXME : viewer returned.
}



/*****************************************************************************
 *                     XenoLTT Viewer class implementation              *
 *****************************************************************************/
/**
 * XenoLTT Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
XenoLTTData *guixenoltt(LttvPluginTab *ptab){
  Tab *tab = ptab->tab;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *thread_list_widget, *drawing_widget, *drawing_area;
  LttvPluginXFV *plugin_xfv = g_object_new(LTTV_TYPE_PLUGIN_XFV, NULL);
  XenoLTTData* xenoltt_data = plugin_xfv->xfd;
  xenoltt_data->ptab = ptab;
  xenoltt_data->tab = ptab->tab;

  xenoltt_data->v_adjust = GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  /* Value */
              0.0,  /* Lower */
              0.0,  /* Upper */
              0.0,  /* Step inc. */
              0.0,  /* Page inc. */
              0.0));  /* page size */

  /* Create the drawing */
  xenoltt_data->drawing = xenoltt_drawing_construct(xenoltt_data);
  
  drawing_widget = drawing_get_widget(xenoltt_data->drawing);
  
  drawing_area = drawing_get_drawing_area(xenoltt_data->drawing);

  xenoltt_data->number_of_thread = 0;
  xenoltt_data->background_info_waiting = 0;

  /* Create the Thread list */
  xenoltt_data->thread_list = threadlist_construct();
  
  thread_list_widget = threadlist_get_widget(xenoltt_data->thread_list);
  
  gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(thread_list_widget),GTK_ADJUSTMENT(xenoltt_data->v_adjust));

  g_signal_connect (G_OBJECT(thread_list_widget),
        "scroll-event",
        G_CALLBACK (xfv_scroll_event),
        (gpointer)xenoltt_data);
   g_signal_connect (G_OBJECT(drawing_area),
        "scroll-event",
        G_CALLBACK (xfv_scroll_event),
        (gpointer)xenoltt_data);
  
  g_signal_connect (G_OBJECT(xenoltt_data->thread_list->button),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)xenoltt_data->drawing);
  
  xenoltt_data->hbox = gtk_hbox_new(FALSE, 1);
  xenoltt_data->toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(xenoltt_data->toolbar),
                              GTK_ORIENTATION_VERTICAL);

  tmp_toolbar_icon = create_pixmap (main_window_get_widget(tab),
      "guifilter16x16.png");
  gtk_widget_show(tmp_toolbar_icon);
  xenoltt_data->button_filter = gtk_tool_button_new(tmp_toolbar_icon,
      "Filter");
  g_signal_connect (G_OBJECT(xenoltt_data->button_filter),
        "clicked",
        G_CALLBACK (filter_button),
        (gpointer)plugin_xfv);
  gtk_toolbar_insert(GTK_TOOLBAR(xenoltt_data->toolbar),
      xenoltt_data->button_filter,
      0);

  tmp_toolbar_icon = create_pixmap (main_window_get_widget(tab),
      "properties.png");
  gtk_widget_show(tmp_toolbar_icon);
  xenoltt_data->button_prop = gtk_tool_button_new(tmp_toolbar_icon,
      "Properties");
  g_signal_connect (G_OBJECT(xenoltt_data->button_prop),
        "clicked",
        G_CALLBACK (property_button),
        (gpointer)xenoltt_data);
  gtk_toolbar_insert(GTK_TOOLBAR(xenoltt_data->toolbar),xenoltt_data->button_prop,1);

  gtk_toolbar_set_style(GTK_TOOLBAR(xenoltt_data->toolbar),GTK_TOOLBAR_ICONS);

  gtk_box_pack_start(GTK_BOX(xenoltt_data->hbox), xenoltt_data->toolbar,FALSE, FALSE, 0);
  
  xenoltt_data->h_paned = gtk_hpaned_new();
  xenoltt_data->box = gtk_event_box_new();
  gtk_box_pack_end(GTK_BOX(xenoltt_data->hbox), xenoltt_data->box,TRUE, TRUE, 0);
  xenoltt_data->top_widget = xenoltt_data->hbox;
  plugin_xfv->parent.top_widget = xenoltt_data->top_widget;
  gtk_container_add(GTK_CONTAINER(xenoltt_data->box),xenoltt_data->h_paned);
      
  gtk_paned_pack1(GTK_PANED(xenoltt_data->h_paned),thread_list_widget, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(xenoltt_data->h_paned),drawing_widget, TRUE, TRUE);
  
  gtk_container_set_border_width(GTK_CONTAINER(xenoltt_data->box), 1);
  

  gtk_widget_show(drawing_widget);
  gtk_widget_show(thread_list_widget);
  gtk_widget_show(xenoltt_data->h_paned);
  gtk_widget_show(xenoltt_data->box);
//  gtk_widget_show(xenoltt_data->toolbar);
//  gtk_widget_show(GTK_WIDGET(xenoltt_data->button_prop));
//  gtk_widget_show(GTK_WIDGET(xenoltt_data->button_filter));
  gtk_widget_show(xenoltt_data->hbox);
  
  g_object_set_data_full(
      G_OBJECT(xenoltt_data->top_widget),
      "plugin_data",
      plugin_xfv,
      (GDestroyNotify)guixenoltt_destructor);
    
  g_object_set_data(
      G_OBJECT(drawing_area),
      "xenoltt_data",
      xenoltt_data);
        
  g_xenoltt_data_list = g_slist_append(g_xenoltt_data_list,plugin_xfv);
  
  
  xenoltt_data->filter = NULL;

  //WARNING : The widget must be 
  //inserted in the main window before the drawing area
  //can be configured (and this must happend bedore sending
  //data)
  return xenoltt_data;

}

/* Destroys widget also */
void guixenoltt_destructor_full(gpointer data){
  LttvPluginXFV *plugin_xfv = (LttvPluginXFV*)data;
  g_info("XFV.c : guixenoltt_destructor_full, %p", plugin_xfv);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guixenoltt_get_widget(plugin_xfv->xfd)))
    gtk_widget_destroy(guixenoltt_get_widget(plugin_xfv->xfd));
}

/* When this destructor is called, the widgets are already disconnected */
void guixenoltt_destructor(gpointer data){
  LttvPluginXFV *plugin_xfv = (LttvPluginXFV*)data;
  Tab *tab = plugin_xfv->xfd->tab;
  XenoLTTData *xenoltt_data = plugin_xfv->xfd;
  
  g_info("CFV.c : guixenoltt_destructor, %p", plugin_xfv);
  g_info("%p, %p, %p", update_time_window_hook, plugin_xfv, tab);
  if(GTK_IS_WIDGET(guixenoltt_get_widget(plugin_xfv->xfd)))
    g_info("widget still exists");
  
  lttv_filter_destroy(plugin_xfv->xfd->filter);
  /* Thread List is removed with it's widget */
  if(tab != NULL){
      /* Delete reading hooks */
    lttvwindow_unregister_traceset_notify(tab,traceset_notify,xenoltt_data);
    
    lttvwindow_unregister_time_window_notify(tab,update_time_window_hook,xenoltt_data);
  
    lttvwindow_unregister_current_time_notify(tab,update_current_time_hook,xenoltt_data);

    lttvwindow_unregister_redraw_notify(tab, redraw_notify, xenoltt_data);
    lttvwindow_unregister_continue_notify(tab,continue_notify,xenoltt_data);
    
    lttvwindow_events_request_remove_all(xenoltt_data->tab,xenoltt_data);

  }
  lttvwindowtraces_background_notify_remove(xenoltt_data);
  g_xenoltt_data_list = g_slist_remove(g_xenoltt_data_list, xenoltt_data);

  g_info("XFV.c : guixenoltt_destructor end, %p", xenoltt_data);
  g_object_unref(plugin_xfv);
}


