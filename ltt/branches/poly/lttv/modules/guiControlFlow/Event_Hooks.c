/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

//#define PANGO_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <assert.h>
#include <string.h>

//#include <pango/pango.h>

#include <ltt/event.h>
#include <ltt/time.h>
#include <ltt/type.h>

#include <lttv/hook.h>
#include <lttv/common.h>
#include <lttv/state.h>


#include "Event_Hooks.h"
#include "CFV.h"
#include "Process_List.h"
#include "Drawing.h"
#include "CFV-private.h"


#define MAX_PATH_LEN 256


/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param pmParentWindow A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_guicontrolflow(MainWindow *pmParentWindow, LttvTracesetSelector * s, char * key)
{
	g_info("h_guicontrolflow, %p, %p, %s", pmParentWindow, s, key);
	ControlFlowData *Control_Flow_Data = guicontrolflow() ;
	
	Control_Flow_Data->Parent_Window = pmParentWindow;
	TimeWindow *time_window = guicontrolflow_get_time_window(Control_Flow_Data);
	time_window->start_time.tv_sec = 0;
	time_window->start_time.tv_nsec = 0;
	time_window->time_width.tv_sec = 0;
	time_window->time_width.tv_nsec = 0;

	LttTime *current_time = guicontrolflow_get_current_time(Control_Flow_Data);
	current_time->tv_sec = 0;
	current_time->tv_nsec = 0;
	
	//g_critical("time width1 : %u",time_window->time_width);
	
	get_time_window(pmParentWindow,
			time_window);
	get_current_time(pmParentWindow,
			current_time);

	//g_critical("time width2 : %u",time_window->time_width);
	// Unreg done in the GuiControlFlow_Destructor
	reg_update_time_window(update_time_window_hook, Control_Flow_Data,
				pmParentWindow);
	reg_update_current_time(update_current_time_hook, Control_Flow_Data,
				pmParentWindow);
	return guicontrolflow_get_widget(Control_Flow_Data) ;
	
}

int event_selected_hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	guint *Event_Number = (guint*) call_data;

	g_critical("DEBUG : event selected by main window : %u", *Event_Number);
	
//	Control_Flow_Data->Currently_Selected_Event = *Event_Number;
//	Control_Flow_Data->Selected_Event = TRUE ;
	
//	tree_v_set_cursor(Control_Flow_Data);

}

/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in Event_Request.
 */
int draw_before_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	//EventsContext Events_Context = (EventsContext*)call_data;
	
	//Event_Request->Events_Context = Events_Context;

	return 0;
}

/*
 * The draw event hook is called by the reading API to have a
 * particular event drawn on the screen.
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function basically draw lines and icons. Two types of lines are drawn :
 * one small (3 pixels?) representing the state of the process and the second
 * type is thicker (10 pixels?) representing on which CPU a process is running
 * (and this only in running state).
 *
 * Extremums of the lines :
 * x_min : time of the last event context for this process kept in memory.
 * x_max : time of the current event.
 * y : middle of the process in the process list. The process is found in the
 * list, therefore is it's position in pixels.
 *
 * The choice of lines'color is defined by the context of the last event for this
 * process.
 */
int draw_event_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	ControlFlowData *control_flow_data = Event_Request->Control_Flow_Data;
	//static int i=0;

	//i++;
	//g_critical("%i", i);
	LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttEvent *e;
  e = tfc->e;

	/* Temp dump */
#ifdef DONTSHOW
	GString *string = g_string_new("");;
	gboolean field_names = TRUE, state = TRUE;

  lttv_event_to_string(e, tfc->tf, string, TRUE, field_names, tfs);
  g_string_append_printf(string,"\n");  

  if(state) {
    g_string_append_printf(string, " %s",
        g_quark_to_string(tfs->process->state->s));
  }

  g_info("%s",string->str);

	g_string_free(string, TRUE);
	
	/* End of text dump */
#endif //DONTSHOW

	/* Add process to process list (if not present) and get drawing "y" from
	 * process position */
	guint pid = tfs->process->pid;
	LttTime birth = tfs->process->creation_time;
	gchar *name = strdup(g_quark_to_string(tfs->process->name));
	guint y = 0, height = 0, pl_height = 0;
	HashedProcessData *Hashed_Process_Data = NULL;

	ProcessList *process_list =
		guicontrolflow_get_process_list(Event_Request->Control_Flow_Data);
	
	if(processlist_get_process_pixels(process_list,
					pid,
					&birth,
					&y,
					&height,
					&Hashed_Process_Data) == 1)
	{
		/* Process not present */
		processlist_add(process_list,
				pid,
				&birth,
				name,
				&pl_height,
				&Hashed_Process_Data);
		g_free(name);
		drawing_insert_square( Event_Request->Control_Flow_Data->Drawing, y, height);
	}
	
	/* Find pixels corresponding to time of the event. If the time does
	 * not fit in the window, show a warning, not supposed to happend. */
	guint x = 0;
	guint width = control_flow_data->Drawing->Drawing_Area_V->allocation.width;

	LttTime time = ltt_event_time(e);

	LttTime window_end = ltt_time_add(control_flow_data->Time_Window.time_width,
												control_flow_data->Time_Window.start_time);

	
	convert_time_to_pixels(
			control_flow_data->Time_Window.start_time,
			window_end,
			time,
			width,
			&x);
	
	assert(x <= width);
	
	/* Finally, draw what represents the event. */

	GdkColor color = { 0, 0xffff, 0x0000, 0x0000 };
	PropertiesArc prop_arc;
	prop_arc.color = &color;
	prop_arc.size = 10;
	prop_arc.filled = TRUE;
	prop_arc.position = OVER;

	DrawContext *draw_context = Hashed_Process_Data->draw_context;
	draw_context->Current->modify_over->x = x;
	draw_context->Current->modify_over->y = y;
	draw_context->drawable = control_flow_data->Drawing->Pixmap;
	draw_context->pango_layout = control_flow_data->Drawing->pango_layout;
	GtkWidget *widget = control_flow_data->Drawing->Drawing_Area_V;
	//draw_context->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
	draw_context->gc = widget->style->black_gc;
	
	//draw_arc((void*)&prop_arc, (void*)draw_context);
	//test_draw_item(control_flow_data->Drawing, control_flow_data->Drawing->Pixmap);
	
	//GdkColor colorfg = { 0, 0xffff, 0x0000, 0x0000 };
	GdkColor colorfg;
	colorfg.red = 0xffff;
	colorfg.green = 0x0000;
	colorfg.blue = 0x0000;
	//GdkColor colorbg = { 0, 0xffff, 0x0000, 0xffff };
	GdkColor colorbg;
	colorbg.red = 0x0000;
	colorbg.green = 0x0000;
	colorbg.blue = 0x0000;

	PropertiesText prop_text;
	prop_text.foreground = &colorfg;
	prop_text.background = &colorbg;
	prop_text.size = 10;
	prop_text.position = OVER;

	/* Print status of the process : U, WF, WC, E, W, R */
	if(tfs->process->state->s == LTTV_STATE_UNNAMED)
		prop_text.Text = "U";
	else if(tfs->process->state->s == LTTV_STATE_WAIT_FORK)
		prop_text.Text = "WF";
	else if(tfs->process->state->s == LTTV_STATE_WAIT_CPU)
		prop_text.Text = "WC";
	else if(tfs->process->state->s == LTTV_STATE_EXIT)
		prop_text.Text = "E";
	else if(tfs->process->state->s == LTTV_STATE_WAIT)
		prop_text.Text = "W";
	else if(tfs->process->state->s == LTTV_STATE_RUN)
		prop_text.Text = "R";
	else
		prop_text.Text = "U";
	
	draw_text((void*)&prop_text, (void*)draw_context);
	
	return 0;
}


int draw_after_hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	ControlFlowData *control_flow_data = Event_Request->Control_Flow_Data;

	LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

	
  LttEvent *e;
  e = tfc->e;

	if(strcmp(ltt_eventtype_name(ltt_event_eventtype(e)),"schedchange") == 0)
	{
		g_critical("schedchange!");
		
		/* Add process to process list (if not present) and get drawing "y" from
		 * process position */
		guint pid_out, pid_in;
		LttvProcessState *process_out, *process_in;
		LttTime birth;
		guint y_in = 0, y_out = 0, height = 0, pl_height = 0;

		ProcessList *process_list =
			guicontrolflow_get_process_list(Event_Request->Control_Flow_Data);


		LttField *f = ltt_event_field(e);
		LttField *element;
		element = ltt_field_member(f,0);
		pid_out = ltt_event_get_long_unsigned(e,element);
		element = ltt_field_member(f,1);
		pid_in = ltt_event_get_long_unsigned(e,element);
		g_critical("out : %u  in : %u", pid_out, pid_in);


		/* Find process pid_out in the list... */
		process_out = lttv_state_find_process(tfs, pid_out);
		g_critical("out : %s",g_quark_to_string(process_out->state->s));

		birth = process_out->creation_time;
		gchar *name = strdup(g_quark_to_string(process_out->name));
		HashedProcessData *Hashed_Process_Data_out = NULL;

		if(processlist_get_process_pixels(process_list,
						pid_out,
						&birth,
						&y_out,
						&height,
						&Hashed_Process_Data_out) == 1)
		{
		/* Process not present */
		processlist_add(process_list,
				pid_out,
				&birth,
				name,
				&pl_height,
				&Hashed_Process_Data_out);
		processlist_get_process_pixels(process_list,
						pid_out,
						&birth,
						&y_out,
						&height,
						&Hashed_Process_Data_out);
		drawing_insert_square( Event_Request->Control_Flow_Data->Drawing, y_out, height);
		}

		g_free(name);
		
		/* Find process pid_in in the list... */
		process_in = lttv_state_find_process(tfs, pid_in);
		g_critical("in : %s",g_quark_to_string(process_in->state->s));

		birth = process_in->creation_time;
		name = strdup(g_quark_to_string(process_in->name));
		HashedProcessData *Hashed_Process_Data_in = NULL;

		if(processlist_get_process_pixels(process_list,
						pid_in,
						&birth,
						&y_in,
						&height,
						&Hashed_Process_Data_in) == 1)
		{
		/* Process not present */
			processlist_add(process_list,
				pid_in,
				&birth,
				name,
				&pl_height,
				&Hashed_Process_Data_in);
			processlist_get_process_pixels(process_list,
						pid_in,
						&birth,
						&y_in,
						&height,
						&Hashed_Process_Data_in);

			drawing_insert_square( Event_Request->Control_Flow_Data->Drawing, y_in, height);
		}
		g_free(name);


		/* Find pixels corresponding to time of the event. If the time does
		 * not fit in the window, show a warning, not supposed to happend. */
		guint x = 0;
		guint width = control_flow_data->Drawing->Drawing_Area_V->allocation.width;

		LttTime time = ltt_event_time(e);

		LttTime window_end = ltt_time_add(control_flow_data->Time_Window.time_width,
													control_flow_data->Time_Window.start_time);

		
		convert_time_to_pixels(
				control_flow_data->Time_Window.start_time,
				window_end,
				time,
				width,
				&x);
		
		assert(x <= width);
		
		/* draw what represents the event for outgoing process. */

		DrawContext *draw_context_out = Hashed_Process_Data_out->draw_context;
		draw_context_out->Current->modify_over->x = x;
		draw_context_out->Current->modify_over->y = y_out;
		draw_context_out->drawable = control_flow_data->Drawing->Pixmap;
		draw_context_out->pango_layout = control_flow_data->Drawing->pango_layout;
		GtkWidget *widget = control_flow_data->Drawing->Drawing_Area_V;
		//draw_context_out->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
		draw_context_out->gc = widget->style->black_gc;
		
		//draw_arc((void*)&prop_arc, (void*)draw_context_out);
		//test_draw_item(control_flow_data->Drawing, control_flow_data->Drawing->Pixmap);
		
		GdkColor colorfg_out = { 0, 0xffff, 0x0000, 0x0000 };
		GdkColor colorbg_out = { 0, 0xffff, 0xffff, 0xffff };
		PropertiesText prop_text_out;
		prop_text_out.foreground = &colorfg_out;
		prop_text_out.background = &colorbg_out;
		prop_text_out.size = 10;
		prop_text_out.position = OVER;

		/* Print status of the process : U, WF, WC, E, W, R */
		if(process_out->state->s == LTTV_STATE_UNNAMED)
			prop_text_out.Text = "U";
		else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
			prop_text_out.Text = "WF";
		else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
			prop_text_out.Text = "WC";
		else if(process_out->state->s == LTTV_STATE_EXIT)
			prop_text_out.Text = "E";
		else if(process_out->state->s == LTTV_STATE_WAIT)
			prop_text_out.Text = "W";
		else if(process_out->state->s == LTTV_STATE_RUN)
			prop_text_out.Text = "R";
		else
			prop_text_out.Text = "U";
		
		draw_text((void*)&prop_text_out, (void*)draw_context_out);

		/* Finally, update the drawing context of the pid_in. */

		DrawContext *draw_context_in = Hashed_Process_Data_in->draw_context;
		draw_context_in->Current->modify_over->x = x;
		draw_context_in->Current->modify_over->y = y_in;
		draw_context_in->drawable = control_flow_data->Drawing->Pixmap;
		draw_context_in->pango_layout = control_flow_data->Drawing->pango_layout;
		widget = control_flow_data->Drawing->Drawing_Area_V;
		//draw_context_in->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
		draw_context_in->gc = widget->style->black_gc;
		
		//draw_arc((void*)&prop_arc, (void*)draw_context_in);
		//test_draw_item(control_flow_data->Drawing, control_flow_data->Drawing->Pixmap);
		
		GdkColor colorfg_in = { 0, 0x0000, 0xffff, 0x0000 };
		GdkColor colorbg_in = { 0, 0xffff, 0xffff, 0xffff };
		PropertiesText prop_text_in;
		prop_text_in.foreground = &colorfg_in;
		prop_text_in.background = &colorbg_in;
		prop_text_in.size = 10;
		prop_text_in.position = OVER;

		/* Print status of the process : U, WF, WC, E, W, R */
		if(process_in->state->s == LTTV_STATE_UNNAMED)
			prop_text_in.Text = "U";
		else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
			prop_text_in.Text = "WF";
		else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
			prop_text_in.Text = "WC";
		else if(process_in->state->s == LTTV_STATE_EXIT)
			prop_text_in.Text = "E";
		else if(process_in->state->s == LTTV_STATE_WAIT)
			prop_text_in.Text = "W";
		else if(process_in->state->s == LTTV_STATE_RUN)
			prop_text_in.Text = "R";
		else
			prop_text_in.Text = "U";
		
		draw_text((void*)&prop_text_in, (void*)draw_context_in);
		
	}

	return 0;
}




void update_time_window_hook(void *hook_data, void *call_data)
{
	ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
	TimeWindow *Old_Time_Window = 
		guicontrolflow_get_time_window(control_flow_data);
	TimeWindow *New_Time_Window = ((TimeWindow*)call_data);
	
	/* Two cases : zoom in/out or scrolling */
	
	/* In order to make sure we can reuse the old drawing, the scale must
	 * be the same and the new time interval being partly located in the
	 * currently shown time interval. (reuse is only for scrolling)
	 */

	g_info("Old time window HOOK : %u, %u to %u, %u",
			Old_Time_Window->start_time.tv_sec,
			Old_Time_Window->start_time.tv_nsec,
			Old_Time_Window->time_width.tv_sec,
			Old_Time_Window->time_width.tv_nsec);

	g_info("New time window HOOK : %u, %u to %u, %u",
			New_Time_Window->start_time.tv_sec,
			New_Time_Window->start_time.tv_nsec,
			New_Time_Window->time_width.tv_sec,
			New_Time_Window->time_width.tv_nsec);

	if( New_Time_Window->time_width.tv_sec == Old_Time_Window->time_width.tv_sec
	&& New_Time_Window->time_width.tv_nsec == Old_Time_Window->time_width.tv_nsec)
	{
		/* Same scale (scrolling) */
		g_info("scrolling");
		LttTime *ns = &New_Time_Window->start_time;
		LttTime *os = &Old_Time_Window->start_time;
		LttTime old_end = ltt_time_add(Old_Time_Window->start_time,
																		Old_Time_Window->time_width);
		LttTime new_end = ltt_time_add(New_Time_Window->start_time,
																		New_Time_Window->time_width);
		//if(ns<os+w<ns+w)
		//if(ns<os+w && os+w<ns+w)
		//if(ns<old_end && os<ns)
		if(ltt_time_compare(*ns, old_end) == -1
				&& ltt_time_compare(*os, *ns) == -1)
		{
			g_info("scrolling near right");
			/* Scroll right, keep right part of the screen */
			guint x = 0;
			guint width = control_flow_data->Drawing->Drawing_Area_V->allocation.width;
			convert_time_to_pixels(
					*os,
					old_end,
					*ns,
					width,
					&x);

		  /* Copy old data to new location */
		  gdk_draw_drawable (control_flow_data->Drawing->Pixmap,
				  control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
				  control_flow_data->Drawing->Pixmap,
				  x, 0,
				  0, 0,
				  -1, -1);
			
			convert_time_to_pixels(
					*ns,
					new_end,
					old_end,
					width,
					&x);

			*Old_Time_Window = *New_Time_Window;
			/* Clear the data request background, but not SAFETY */
			gdk_draw_rectangle (control_flow_data->Drawing->Pixmap,
	 				control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      x+SAFETY, 0,
		      control_flow_data->Drawing->width - x,	// do not overlap
		      control_flow_data->Drawing->height+SAFETY);
			/* Get new data for the rest. */
		  drawing_data_request(control_flow_data->Drawing,
					&control_flow_data->Drawing->Pixmap,
					x, 0,
					control_flow_data->Drawing->width - x,
				 	control_flow_data->Drawing->height);
	
			drawing_refresh(control_flow_data->Drawing,
					0, 0,
					control_flow_data->Drawing->width,
					control_flow_data->Drawing->height);


		} else { 
			//if(ns<os<ns+w)
			//if(ns<os && os<ns+w)
			//if(ns<os && os<new_end)
			if(ltt_time_compare(*ns,*os) == -1
					&& ltt_time_compare(*os,new_end) == -1)
			{
				g_info("scrolling near left");
				/* Scroll left, keep left part of the screen */
				guint x = 0;
				guint width = control_flow_data->Drawing->Drawing_Area_V->allocation.width;
				convert_time_to_pixels(
						*ns,
						new_end,
						*os,
						width,
						&x);
	
			  /* Copy old data to new location */
			  gdk_draw_drawable (control_flow_data->Drawing->Pixmap,
				  	control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
					  control_flow_data->Drawing->Pixmap,
					  0, 0,
					  x, 0,
					  -1, -1);
	
				*Old_Time_Window = *New_Time_Window;

				/* Clean the data request background */
				gdk_draw_rectangle (control_flow_data->Drawing->Pixmap,
	 				control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      0, 0,
		      x,	// do not overlap
		      control_flow_data->Drawing->height+SAFETY);
				/* Get new data for the rest. */
			  drawing_data_request(control_flow_data->Drawing,
						&control_flow_data->Drawing->Pixmap,
						0, 0,
						x,
					 	control_flow_data->Drawing->height);
		
				drawing_refresh(control_flow_data->Drawing,
						0, 0,
						control_flow_data->Drawing->width,
						control_flow_data->Drawing->height);
				
			} else {
				g_info("scrolling far");
				/* Cannot reuse any part of the screen : far jump */
				*Old_Time_Window = *New_Time_Window;
				
				
				gdk_draw_rectangle (control_flow_data->Drawing->Pixmap,
	 				control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      0, 0,
		      control_flow_data->Drawing->width+SAFETY,	// do not overlap
		      control_flow_data->Drawing->height+SAFETY);

			 	drawing_data_request(control_flow_data->Drawing,
						&control_flow_data->Drawing->Pixmap,
						0, 0,
						control_flow_data->Drawing->width,
					 	control_flow_data->Drawing->height);
		
				drawing_refresh(control_flow_data->Drawing,
						0, 0,
						control_flow_data->Drawing->width,
						control_flow_data->Drawing->height);
			}
		}
	} else {
		/* Different scale (zoom) */
		g_info("zoom");

		*Old_Time_Window = *New_Time_Window;
	
		gdk_draw_rectangle (control_flow_data->Drawing->Pixmap,
	 				control_flow_data->Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      0, 0,
		      control_flow_data->Drawing->width+SAFETY,	// do not overlap
		      control_flow_data->Drawing->height+SAFETY);

	
	  drawing_data_request(control_flow_data->Drawing,
				&control_flow_data->Drawing->Pixmap,
				0, 0,
				control_flow_data->Drawing->width,
			 	control_flow_data->Drawing->height);
	
		drawing_refresh(control_flow_data->Drawing,
				0, 0,
				control_flow_data->Drawing->width,
				control_flow_data->Drawing->height);
	}

	
}

void update_current_time_hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	LttTime* Current_Time = 
		guicontrolflow_get_current_time(Control_Flow_Data);
	*Current_Time = *((LttTime*)call_data);
	g_info("New Current time HOOK : %u, %u", Current_Time->tv_sec,
							Current_Time->tv_nsec);

	/* If current time is inside time interval, just move the highlight
	 * bar */

	/* Else, we have to change the time interval. We have to tell it
	 * to the main window. */
	/* The time interval change will take care of placing the current
	 * time at the center of the visible area */
	
}

