
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "CFV.h"
#include "Drawing.h"
#include "Process_List.h"
#include "Event_Hooks.h"
#include "CFV-private.h"


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

extern GSList *gControl_Flow_Data_List;

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
  GtkWidget *process_list_Widget, *Drawing_Widget;//, *button;

  ControlFlowData* Control_Flow_Data = g_new(ControlFlowData,1) ;

  /* Create the Drawing */
  Control_Flow_Data->Drawing = drawing_construct(Control_Flow_Data);
  
  Drawing_Widget = 
    drawing_get_widget(Control_Flow_Data->Drawing);
  
  Control_Flow_Data->number_of_process = 0;

  /* Create the Process list */
  Control_Flow_Data->process_list = processlist_construct();
  
  process_list_Widget = 
    processlist_get_widget(Control_Flow_Data->process_list);
  
  //Control_Flow_Data->Inside_HBox_V = gtk_hbox_new(0, 0);
  Control_Flow_Data->h_paned = gtk_hpaned_new();
    
  gtk_paned_pack1(GTK_PANED(Control_Flow_Data->h_paned), process_list_Widget, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(Control_Flow_Data->h_paned), Drawing_Widget, TRUE, TRUE);

  Control_Flow_Data->v_adjust = 
    GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  /* Value */
              0.0,  /* Lower */
              0.0,  /* Upper */
              0.0,  /* Step inc. */
              0.0,  /* Page inc. */
              0.0));  /* page size */
  
  Control_Flow_Data->scrolled_window =
      gtk_scrolled_window_new (NULL,
      Control_Flow_Data->v_adjust);
  
  gtk_scrolled_window_set_policy(
    GTK_SCROLLED_WINDOW(Control_Flow_Data->scrolled_window) ,
    GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_add_with_viewport(
    GTK_SCROLLED_WINDOW(Control_Flow_Data->scrolled_window),
    Control_Flow_Data->h_paned);
  
  /* Set the size of the drawing area */
  //Drawing_Resize(Drawing, h, w);

  /* Get trace statistics */
  //Control_Flow_Data->Trace_Statistics = get_trace_statistics(Trace);


  gtk_widget_show(Drawing_Widget);
  gtk_widget_show(process_list_Widget);
  gtk_widget_show(Control_Flow_Data->h_paned);
  gtk_widget_show(Control_Flow_Data->scrolled_window);
  
  g_object_set_data_full(
      G_OBJECT(Control_Flow_Data->scrolled_window),
      "Control_Flow_Data",
      Control_Flow_Data,
      (GDestroyNotify)guicontrolflow_destructor);
    
  g_object_set_data(
      G_OBJECT(Drawing_Widget),
      "Control_Flow_Data",
      Control_Flow_Data);
        
  gControl_Flow_Data_List = g_slist_append(
      gControl_Flow_Data_List,
      Control_Flow_Data);

  //WARNING : The widget must be 
  //inserted in the main window before the Drawing area
  //can be configured (and this must happend bedore sending
  //data)

  return Control_Flow_Data;

}

/* Destroys widget also */
void
guicontrolflow_destructor_full(ControlFlowData *Control_Flow_Data)
{
  g_info("CFV.c : guicontrolflow_destructor_full, %p", Control_Flow_Data);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(Control_Flow_Data->scrolled_window))
    gtk_widget_destroy(Control_Flow_Data->scrolled_window);
  //Control_Flow_Data->mw = NULL;
  //FIXME guicontrolflow_destructor(Control_Flow_Data);
}

/* When this destructor is called, the widgets are already disconnected */
void
guicontrolflow_destructor(ControlFlowData *Control_Flow_Data)
{
  guint index;
  
  g_info("CFV.c : guicontrolflow_destructor, %p", Control_Flow_Data);
  g_info("%p, %p, %p", update_time_window_hook, Control_Flow_Data, Control_Flow_Data->mw);
  if(GTK_IS_WIDGET(Control_Flow_Data->scrolled_window))
    g_info("widget still exists");
  
  /* Process List is removed with it's widget */
  //ProcessList_destroy(Control_Flow_Data->process_list);
  if(Control_Flow_Data->mw != NULL)
  {
    unreg_update_time_window(update_time_window_hook,
        Control_Flow_Data,
        Control_Flow_Data->mw);
  
    unreg_update_current_time(update_current_time_hook,
        Control_Flow_Data,
        Control_Flow_Data->mw);
  }
  g_info("CFV.c : guicontrolflow_destructor, %p", Control_Flow_Data);
  g_slist_remove(gControl_Flow_Data_List,Control_Flow_Data);
  g_free(Control_Flow_Data);
}

GtkWidget *guicontrolflow_get_widget(ControlFlowData *Control_Flow_Data)
{
  return Control_Flow_Data->scrolled_window ;
}

ProcessList *guicontrolflow_get_process_list
    (ControlFlowData *Control_Flow_Data)
{
    return Control_Flow_Data->process_list ;
}

TimeWindow *guicontrolflow_get_time_window(ControlFlowData *Control_Flow_Data)
{
  return &Control_Flow_Data->time_window;
}
LttTime *guicontrolflow_get_current_time(ControlFlowData *Control_Flow_Data)
{
  return &Control_Flow_Data->current_time;
}


