
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "CFV.h"
#include "Drawing.h"
#include "Process_List.h"


extern GSList *gControl_Flow_Data_List;

/*****************************************************************************
 *                     Control Flow Viewer class implementation              *
 *****************************************************************************/


struct _ControlFlowData {

	GtkWidget *Drawing_Area_V;
	GtkWidget *Scrolled_Window_VC;
	
	ProcessList *Process_List;
	Drawing_t *Drawing;

	//GtkWidget *HBox_V;
	GtkWidget *Inside_HBox_V;

	GtkAdjustment *VAdjust_C ;
	
	/* Trace information */
	//TraceSet *Trace_Set;
	//TraceStatistics *Trace_Statistics;
	
	/* Shown events information */
	guint First_Event, Last_Event;
	LttTime Begin_Time, End_Time;
	
	
	/* TEST DATA, TO BE READ FROM THE TRACE */
	gint Number_Of_Events ;
	guint Currently_Selected_Event  ;
	gboolean Selected_Event ;
	guint Number_Of_Process;

} ;


/**
 * Control Flow Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
ControlFlowData *
GuiControlFlow(void)
{
	GtkWidget *Process_List_Widget;

	ControlFlowData* Control_Flow_Data = g_new(ControlFlowData,1) ;

	/* Create the Drawing */
	//Control_Flow_Data->Drawing = Drawing();
	
	/* TEST DATA, TO BE READ FROM THE TRACE */
	Control_Flow_Data->Number_Of_Events = 1000 ;
	Control_Flow_Data->Currently_Selected_Event = FALSE  ;
	Control_Flow_Data->Selected_Event = 0;
	Control_Flow_Data->Number_Of_Process = 10;

	/* FIXME register Event_Selected_Hook */
	


	/* Create the Process list */
	Control_Flow_Data->Process_List = ProcessList_construct();
	
	Process_List_Widget = 
		ProcessList_getWidget(Control_Flow_Data->Process_List);
	
	Control_Flow_Data->Inside_HBox_V = gtk_hbox_new(0, 0);

	gtk_box_pack_start(
		GTK_BOX(Control_Flow_Data->Inside_HBox_V),
		Process_List_Widget, TRUE, TRUE, 0); // FALSE TRUE
	//gtk_box_pack_start(
	//	GTK_BOX(Control_Flow_Data->Inside_HBox_V),
	//	Control_Flow_Data->Drawing_Area_V, TRUE, TRUE, 0);


	Control_Flow_Data->VAdjust_C = 
		GTK_ADJUSTMENT(gtk_adjustment_new(	0.0,	/* Value */
							0.0,	/* Lower */
							0.0,	/* Upper */
							0.0,	/* Step inc. */
							0.0,	/* Page inc. */
							0.0));	/* page size */
	
	Control_Flow_Data->Scrolled_Window_VC =
			gtk_scrolled_window_new (NULL,
			Control_Flow_Data->VAdjust_C);
	
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(Control_Flow_Data->Scrolled_Window_VC) ,
		GTK_POLICY_NEVER,
		GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(Control_Flow_Data->Scrolled_Window_VC),
		Control_Flow_Data->Inside_HBox_V);
	
	
	//g_signal_connect (G_OBJECT (Control_Flow_Data->Drawing_Area_V),
	//		"expose_event",
	//		G_CALLBACK (expose_event_cb),
	//		Control_Flow_Data);


	
	//g_signal_connect (G_OBJECT (Control_Flow_Data->VAdjust_C),
	//		"value-changed",
	//                G_CALLBACK (v_scroll_cb),
	//                Control_Flow_Data);


	/* Set the size of the drawing area */
	//Drawing_Resize(Drawing, h, w);

	/* Get trace statistics */
	//Control_Flow_Data->Trace_Statistics = get_trace_statistics(Trace);


	//gtk_widget_show(Control_Flow_Data->Drawing_Area_V);
	gtk_widget_show(Process_List_Widget);
	gtk_widget_show(Control_Flow_Data->Inside_HBox_V);
	gtk_widget_show(Control_Flow_Data->Scrolled_Window_VC);
	
	g_object_set_data_full(
			G_OBJECT(Control_Flow_Data->Scrolled_Window_VC),
			"Control_Flow_Data",
			Control_Flow_Data,
			(GDestroyNotify)GuiControlFlow_Destructor);
			
	g_slist_append(gControl_Flow_Data_List,Control_Flow_Data);

	return Control_Flow_Data;

}

/* Destroys widget also */
void
GuiControlFlow_Destructor_Full(ControlFlowData *Control_Flow_Data)
{
	/* May already have been done by GTK window closing */
	if(GTK_IS_WIDGET(Control_Flow_Data->Scrolled_Window_VC))
		gtk_widget_destroy(Control_Flow_Data->Scrolled_Window_VC);

	GuiControlFlow_Destructor(Control_Flow_Data);
}

void
GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data)
{
	guint index;
	
	/* Process List is removed with it's widget */
	//ProcessList_destroy(Control_Flow_Data->Process_List);
	
	g_slist_remove(gControl_Flow_Data_List,Control_Flow_Data);
	g_free(Control_Flow_Data);
}

GtkWidget *GuiControlFlow_get_Widget(ControlFlowData *Control_Flow_Data)
{
	return Control_Flow_Data->Scrolled_Window_VC ;
}

