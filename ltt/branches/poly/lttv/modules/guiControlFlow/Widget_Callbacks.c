/*****************************************************************************
 *                       Callbacks used for the viewer                       *
 *****************************************************************************/
void expose_event_cb (GtkWidget *widget, GdkEventExpose *expose, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;

	EventRequest *Event_Request = g_new(sizeof(EventRequest));
	
	Event_Request->Control_Flow_Data = Control_Flow_Data;
	
	/* Calculate, from pixels in expose, the time interval to get data */
	
	get_time_from_pixels(expose->area.x, expose->area.width,
													Control_Flow_Data->Drawing_Area_Info.width,
													&Control_Flow_Data->Begin_Time, &Control_Flow_Data->End_Time,
													&Event_Request->time_begin, &Event_Request->time_end)
	
	/* Look in statistics of the trace the processes present during the
	 * whole time interval _shown on the screen_. Modify the list of 
	 * processes to match it. NOTE : modify, not recreate. If recreation is
	 * needed,keep a pointer to the currently selected event in the list.
	 */
	
	/* Call the reading API to have events sent to drawing hooks */
	lttv_trace_set_process( Control_Flow_Data->Trace_Set,
													Draw_Before_Hooks,
													Draw_Event_Hooks,
													Draw_After_Hooks,
													NULL, //FIXME : filter here
													Event_Request->time_begin,
													Event_Request->time_end);

}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	GtkTreePath *Tree_Path;

	g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
	
	//get_test_data((int)adjustment->value, Control_Flow_Data->Num_Visible_Events, 
	//								 Control_Flow_Data);
	
	

}


