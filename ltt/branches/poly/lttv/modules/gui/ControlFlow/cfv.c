
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "cfv.h"
#include "drawing.h"
#include "process-list.h"
#include "event-hooks.h"
#include "cfv-private.h"


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

extern GSList *g_control_flow_data_list;

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
guicontrolflow(void)
{
  GtkWidget *process_list_widget, *drawing_widget;

  ControlFlowData* control_flow_data = g_new(ControlFlowData,1) ;

  /* Create the drawing */
  control_flow_data->drawing = drawing_construct(control_flow_data);
  
  drawing_widget = 
    drawing_get_widget(control_flow_data->drawing);
  
  control_flow_data->number_of_process = 0;

  /* Create the Process list */
  control_flow_data->process_list = processlist_construct();
  
  process_list_widget = 
    processlist_get_widget(control_flow_data->process_list);
  
  //control_flow_data->Inside_HBox_V = gtk_hbox_new(0, 0);
  control_flow_data->h_paned = gtk_hpaned_new();
    
  gtk_paned_pack1(GTK_PANED(control_flow_data->h_paned), process_list_widget, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(control_flow_data->h_paned), drawing_widget, TRUE, TRUE);

  control_flow_data->v_adjust = 
    GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  /* Value */
              0.0,  /* Lower */
              0.0,  /* Upper */
              0.0,  /* Step inc. */
              0.0,  /* Page inc. */
              0.0));  /* page size */
  
  control_flow_data->scrolled_window =
      gtk_scrolled_window_new (NULL,
      control_flow_data->v_adjust);
  
  gtk_scrolled_window_set_policy(
    GTK_SCROLLED_WINDOW(control_flow_data->scrolled_window) ,
    GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_add_with_viewport(
    GTK_SCROLLED_WINDOW(control_flow_data->scrolled_window),
    control_flow_data->h_paned);
  
  /* Set the size of the drawing area */
  //drawing_Resize(drawing, h, w);

  /* Get trace statistics */
  //control_flow_data->Trace_Statistics = get_trace_statistics(Trace);


  gtk_widget_show(drawing_widget);
  gtk_widget_show(process_list_widget);
  gtk_widget_show(control_flow_data->h_paned);
  gtk_widget_show(control_flow_data->scrolled_window);
  
  g_object_set_data_full(
      G_OBJECT(control_flow_data->scrolled_window),
      "control_flow_data",
      control_flow_data,
      (GDestroyNotify)guicontrolflow_destructor);
    
  g_object_set_data(
      G_OBJECT(drawing_widget),
      "control_flow_data",
      control_flow_data);
        
  g_control_flow_data_list = g_slist_append(
      g_control_flow_data_list,
      control_flow_data);

  //WARNING : The widget must be 
  //inserted in the main window before the drawing area
  //can be configured (and this must happend bedore sending
  //data)

  return control_flow_data;

}

/* Destroys widget also */
void
guicontrolflow_destructor_full(ControlFlowData *control_flow_data)
{
  g_info("CFV.c : guicontrolflow_destructor_full, %p", control_flow_data);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(control_flow_data->scrolled_window))
    gtk_widget_destroy(control_flow_data->scrolled_window);
  //control_flow_data->mw = NULL;
  //FIXME guicontrolflow_destructor(control_flow_data);
}

/* When this destructor is called, the widgets are already disconnected */
void
guicontrolflow_destructor(ControlFlowData *control_flow_data)
{
  guint index;
  
  g_info("CFV.c : guicontrolflow_destructor, %p", control_flow_data);
  g_info("%p, %p, %p", update_time_window_hook, control_flow_data, control_flow_data->mw);
  if(GTK_IS_WIDGET(control_flow_data->scrolled_window))
    g_info("widget still exists");
  
  /* Process List is removed with it's widget */
  //ProcessList_destroy(control_flow_data->process_list);
  if(control_flow_data->mw != NULL)
  {
    unreg_update_time_window(update_time_window_hook,
        control_flow_data,
        control_flow_data->mw);
  
    unreg_update_current_time(update_current_time_hook,
        control_flow_data,
        control_flow_data->mw);
  }
  g_info("CFV.c : guicontrolflow_destructor, %p", control_flow_data);
  g_slist_remove(g_control_flow_data_list,control_flow_data);
  g_free(control_flow_data);
}

GtkWidget *guicontrolflow_get_widget(ControlFlowData *control_flow_data)
{
  return control_flow_data->scrolled_window ;
}

ProcessList *guicontrolflow_get_process_list
    (ControlFlowData *control_flow_data)
{
    return control_flow_data->process_list ;
}

TimeWindow *guicontrolflow_get_time_window(ControlFlowData *control_flow_data)
{
  return &control_flow_data->time_window;
}
LttTime *guicontrolflow_get_current_time(ControlFlowData *control_flow_data)
{
  return &control_flow_data->current_time;
}


