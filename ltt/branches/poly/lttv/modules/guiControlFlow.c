/*! \defgroup guiEvents libguiControlFlow: The GUI ControlFlow display plugin */
/*\@{*/

/*! \file guiControlFlow.c
 * \brief Graphical plugin for showing control flow of a trace.
 *
 * This plugin adds a Control Flow Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling gtkTraceSet's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * This plugin uses the gdk library to draw the events and gtk to interact
 * with the user.
 *
 * Author : Mathieu Desnoyers, June 2003
 */

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/hook.h>
#include <lttv/module.h>

//#include "guiControlFlow.h"
#include "icons/hGuiControlFlowInsert.xpm"

#include "gtktreeprivate.h"

//FIXME by including ltt.h
#include <time.h>
typedef time_t ltt_time;

typedef struct _DrawingAreaInfo {

	guint height, width;

} DrawingAreaInfo ;

typedef struct _ControlFlowData {

	GtkWidget *Drawing_Area_V;
	GtkWidget *Drawing_Scrolled_Window_VC;
	GtkAdjustment *VAdjust_Draw_C;
	
	GtkWidget *Process_List_VC;
	GtkWidget *Process_Scrolled_Window_VC;
	
  /* Model containing list data */
	GtkListStore *Store_M;
	
	GtkWidget *HBox_V;

  GtkAdjustment *VAdjust_C ;
	
	guint First_Event, Last_Event;
	ltt_time Begin_time, End_Time;
	
	/* Drawing Area Info */
	DrawingAreaInfo Drawing_Area_Info;
	
	/* TEST DATA, TO BE READ FROM THE TRACE */
	gint Number_Of_Events ;
	guint Currently_Selected_Event  ;
	gboolean Selected_Event ;
	guint Number_Of_Process;

} ControlFlowData ;


/** Array containing instanced objects. Used when module is unloaded */
static GSList *sControl_Flow_Data_List = NULL ;


//! Control Flow Viewer's constructor hook
GtkWidget *hGuiControlFlow(GtkWidget *pmParentWindow);
//! Control Flow Viewer's constructor
ControlFlowData *GuiControlFlow(void);
//! Control Flow Viewer's destructor
void GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data);


static int Event_Selected_Hook(void *hook_data, void *call_data);

//void Tree_V_set_cursor(ControlFlowData *Control_Flow_Data);
//void Tree_V_get_cursor(ControlFlowData *Control_Flow_Data);

/* Prototype for selection handler callback */
//static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
//static void Tree_V_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
//static void Tree_V_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
//static void Tree_V_cursor_changed_cb (GtkWidget *widget, gpointer data);
//static void Tree_V_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);

static void expose_event_cb (GtkWidget *widget, GdkEventExpose *expose, gpointer data);

void add_test_process(ControlFlowData *Control_Flow_Data);

static void get_test_data(guint Event_Number, guint List_Height, 
									 ControlFlowData *Control_Flow_Data);

void add_test_data(ControlFlowData *Control_Flow_Data);
void test_draw(ControlFlowData *Control_Flow_Data);

void Drawing_Area_Init(ControlFlowData *Control_Flow_Data);




/**
 * plugin's init function
 *
 * This function initializes the Control Flow Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init() {
	g_critical("GUI ControlFlow Viewer init()");

	/* Register the toolbar insert button */
	//ToolbarItemReg(guiEventsInsert_xpm, "Insert Control Flow Viewer", guiEvent);

	/* Register the menu item insert entry */
	//MenuItemReg("/", "Insert Control Flow Viewer", guiEvent);

}

void destroy_walk(gpointer data, gpointer user_data)
{
	GuiControlFlow_Destructor((ControlFlowData*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
	g_critical("GUI Control Flow Viewer destroy()");
	int i;

	ControlFlowData *Control_Flow_Data;
	
	g_critical("GUI Event Viewer destroy()");

	g_slist_foreach(sControl_Flow_Data_List, destroy_walk, NULL );
	
	/* Unregister the toolbar insert button */
	//ToolbarItemUnreg(hGuiEvents);

	/* Unregister the menu item insert entry */
	//MenuItemUnreg(hGuiEvents);
}

/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param pmParentWindow A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
hGuiControlFlow(GtkWidget *pmParentWindow)
{
	ControlFlowData* Control_Flow_Data = GuiControlFlow() ;

	return Control_Flow_Data->HBox_V ;
	
}



/* Enumeration of the columns */
enum
{
	PROCESS_COLUMN,
	N_COLUMNS
};



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

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	ControlFlowData* Control_Flow_Data = g_new(ControlFlowData,1) ;
	Control_Flow_Data->Drawing_Area_V = gtk_drawing_area_new ();

	/* TEST DATA, TO BE READ FROM THE TRACE */
	Control_Flow_Data->Number_Of_Events = 1000 ;
	Control_Flow_Data->Currently_Selected_Event = FALSE  ;
	Control_Flow_Data->Selected_Event = 0;
	Control_Flow_Data->Number_Of_Process = 10;

	/* FIXME register Event_Selected_Hook */
	


	/* Create the Process list */
	Control_Flow_Data->Store_M = gtk_list_store_new ( N_COLUMNS,
																										G_TYPE_STRING);


	Control_Flow_Data->Process_List_VC = gtk_tree_view_new_with_model (GTK_TREE_MODEL (Control_Flow_Data->Store_M));

	g_object_unref (G_OBJECT (Control_Flow_Data->Store_M));

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC), FALSE);

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
	/* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Process",
																											renderer,
                                                      "text", PROCESS_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 0.0);
	gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Control_Flow_Data->Process_List_VC), column);



	/* Create the two scrolled windows with the same adjustment */
	Control_Flow_Data->Process_Scrolled_Window_VC = gtk_scrolled_window_new(NULL, NULL);

	Control_Flow_Data->VAdjust_C = gtk_scrolled_window_get_vadjustment(
										GTK_SCROLLED_WINDOW(Control_Flow_Data->Process_Scrolled_Window_VC));

	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(Control_Flow_Data->Process_Scrolled_Window_VC),
			GTK_POLICY_NEVER,
		//	GTK_POLICY_NEVER);
			GTK_POLICY_AUTOMATIC);
	


	Control_Flow_Data->Drawing_Scrolled_Window_VC = gtk_scrolled_window_new(NULL,Control_Flow_Data->VAdjust_C);
	//Control_Flow_Data->Drawing_Scrolled_Window_VC = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(Control_Flow_Data->Drawing_Scrolled_Window_VC),
			GTK_POLICY_NEVER,
		//	GTK_POLICY_NEVER);
			GTK_POLICY_AUTOMATIC);
	
	Control_Flow_Data->VAdjust_Draw_C = gtk_scrolled_window_get_vadjustment(
										GTK_SCROLLED_WINDOW(Control_Flow_Data->Drawing_Scrolled_Window_VC));

	gtk_container_add(GTK_CONTAINER(Control_Flow_Data->Process_Scrolled_Window_VC),
							Control_Flow_Data->Process_List_VC);


	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Control_Flow_Data->Drawing_Scrolled_Window_VC),
							Control_Flow_Data->Drawing_Area_V);

	
	Control_Flow_Data->HBox_V = gtk_hbox_new(0, 0);



	
	/* Pack the list and the drawing area in the hbox */

	gtk_box_pack_start(GTK_BOX(Control_Flow_Data->HBox_V), Control_Flow_Data->Process_Scrolled_Window_VC, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(Control_Flow_Data->HBox_V), Control_Flow_Data->Drawing_Scrolled_Window_VC, TRUE, TRUE, 0);


	g_signal_connect (G_OBJECT (Control_Flow_Data->Drawing_Area_V), "expose_event",  
	                  G_CALLBACK (expose_event_cb), Control_Flow_Data);


	
#ifdef DEBUG
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                          G_CALLBACK (expose_event_callback), Control_Flow_Data);



  Control_Flow_Data->VAdjust_C = gtk_range_get_adjustment(GTK_RANGE(Control_Flow_Data->VScroll_VC));

	g_signal_connect (G_OBJECT (Control_Flow_Data->VAdjust_C), "value-changed",
	                  G_CALLBACK (v_scroll_cb),
	                  Control_Flow_Data);
	/* Set the upper bound to the last event number */
	Control_Flow_Data->VAdjust_C->lower = 0;
	Control_Flow_Data->VAdjust_C->upper = Control_Flow_Data->Number_Of_Events;
	Control_Flow_Data->VAdjust_C->value = 0;
	Control_Flow_Data->VAdjust_C->step_increment = 1;
	Control_Flow_Data->VAdjust_C->page_increment = 
				 Control_Flow_Data->VTree_Adjust_C->upper;
	Control_Flow_Data->VAdjust_C->page_size =
				 Control_Flow_Data->VTree_Adjust_C->upper;
	g_critical("value : %u",Control_Flow_Data->VTree_Adjust_C->upper);




	/* Add the object's information to the module's array */
  g_slist_append(sControl_Flow_Data_List, Control_Flow_Data);

	Control_Flow_Data->First_Event = -1 ;
	Control_Flow_Data->Last_Event = 0 ;

	Control_Flow_Data->Num_Visible_Events = 1;

	/* Test data */
	get_test_data((int)Control_Flow_Data->VAdjust_C->value,
									 Control_Flow_Data->Num_Visible_Events, 
									 Control_Flow_Data);

	/* Set the Selected Event */
	//Tree_V_set_cursor(Control_Flow_Data);
#endif //DEBUG	


	g_signal_connect (G_OBJECT (Control_Flow_Data->VAdjust_C), "value-changed",
	                  G_CALLBACK (v_scroll_cb),
	                  Control_Flow_Data);
	g_signal_connect (G_OBJECT (Control_Flow_Data->VAdjust_Draw_C), "value-changed",
	                  G_CALLBACK (v_scroll_cb),
	                  Control_Flow_Data);




	add_test_process(Control_Flow_Data);



	/* Set the size of the drawing area */
	Drawing_Area_Init(Control_Flow_Data);

	


	gtk_widget_show(Control_Flow_Data->Drawing_Area_V);
	gtk_widget_show(Control_Flow_Data->Process_List_VC);
	gtk_widget_show(Control_Flow_Data->Process_Scrolled_Window_VC);
	gtk_widget_show(Control_Flow_Data->Drawing_Scrolled_Window_VC);
	gtk_widget_show(Control_Flow_Data->HBox_V);


	test_draw(Control_Flow_Data);

	return Control_Flow_Data;

}

gint get_cell_height(GtkTreeView *TreeView)
{
	gint height, width;
	GtkTreeViewColumn *Column = gtk_tree_view_get_column(TreeView, 0);
	GList *Render_List = gtk_tree_view_column_get_cell_renderers(Column);
	GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
	
	gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL, &height);
	g_critical("cell 0 height : %u",height);
	
	return height;
}


void Drawing_Area_Init(ControlFlowData *Control_Flow_Data)
{
	DrawingAreaInfo *Drawing_Area_Info = &Control_Flow_Data->Drawing_Area_Info;
	guint w;
	//GdkWindow *Tree_View_Window = gtk_tree_view_get_bin_window(
	//													GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC));
	//GdkRectangle visible_rect ;
	w = 500;
	

	//gdk_drawable_get_size(Tree_View_Window, NULL, &h);
	//gdk_drawable_get_size(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC)->priv->bin_window, NULL, &h);
	//gtk_widget_get_size_request(GTK_WIDGET(Control_Flow_Data->Process_Scrolled_Window_VC), NULL, &h);
	//gdk_window_get_geometry(Tree_View_Window, NULL, NULL, &w, &h, NULL);
	//gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC), &visible_rect);
	//h = visible_rect.height;
	/* 4 is probably the 2 pixels that the outside cells does not have */
	Drawing_Area_Info->height =
		get_cell_height(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC))
				* Control_Flow_Data->Number_Of_Process - 4 ;
	
	gtk_widget_set_size_request (Control_Flow_Data->Drawing_Area_V,
					w,
					Drawing_Area_Info->height);

	
}


void expose_event_cb (GtkWidget *widget, GdkEventExpose *expose, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;

	g_critical("expose");

	/* When redrawing, use widget->allocation.width to get the width of
	 * drawable area. */
	Control_Flow_Data->Drawing_Area_Info.width = widget->allocation.width;
	
	test_draw(Control_Flow_Data);

	//gdk_draw_arc (widget->window,
  //              widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
  //              TRUE,
  //              //0, 0, widget->allocation.width, widget->allocation.height,
  //              0, 0, widget->allocation.width,
	//							Control_Flow_Data->Drawing_Area_Info.height,
  //              0, 64 * 360);

	
	//Drawing_Area_Init(Control_Flow_Data);


}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	GtkTreePath *Tree_Path;

	g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
	
	//get_test_data((int)adjustment->value, Control_Flow_Data->Num_Visible_Events, 
	//								 Control_Flow_Data);
	
	

}





void add_test_process(ControlFlowData *Control_Flow_Data)
{
	GtkTreeIter iter;
	int i;
	gchar *process[] = { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };

	for(i=0; i<Control_Flow_Data->Number_Of_Process; i++)
	{
	  /* Add a new row to the model */
		gtk_list_store_append (Control_Flow_Data->Store_M, &iter);
		gtk_list_store_set (Control_Flow_Data->Store_M, &iter,
												PROCESS_COLUMN, process[i],
												-1);
	}
							
}
	


void test_draw(ControlFlowData *Control_Flow_Data)
{
	/* Draw event states using available height, Number of process, cell height
	 * (don't forget to remove two pixels at beginning and end).
	 * For horizontal : use width, Time_Begin, Time_End.
	 * This function calls the reading library to get the draw_hook called 
	 * for the desired period of time. */
	
	DrawingAreaInfo *Drawing_Area_Info = &Control_Flow_Data->Drawing_Area_Info;

	
}


/*void Tree_V_set_cursor(ControlFlowData *Control_Flow_Data)
{
	GtkTreePath *path;

	if(Control_Flow_Data->Selected_Event && Control_Flow_Data->First_Event != -1)
	{
		gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C,
														 Control_Flow_Data->Currently_Selected_Event);
		
		path = gtk_tree_path_new_from_indices(
								Control_Flow_Data->Currently_Selected_Event-
								Control_Flow_Data->First_Event,
								-1);

		gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
}

void Tree_V_get_cursor(ControlFlowData *Control_Flow_Data)
{
	GtkTreePath *path;
	gint *indices;
	
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), &path, NULL);
	indices = gtk_tree_path_get_indices(path);

	if(indices != NULL)
	{
		Control_Flow_Data->Selected_Event = TRUE;
		Control_Flow_Data->Currently_Selected_Event =
					Control_Flow_Data->First_Event + indices[0];

	} else {
		Control_Flow_Data->Selected_Event = FALSE;
		Control_Flow_Data->Currently_Selected_Event = 0;
	}
	g_critical("DEBUG : Event Selected : %i , num: %u", Control_Flow_Data->Selected_Event,  Control_Flow_Data->Currently_Selected_Event) ;

	gtk_tree_path_free(path);

}
*/

#ifdef DEBUG
void Tree_V_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data)
{
	GtkTreePath *path; // = gtk_tree_path_new();
	gint *indices;
	gdouble value;
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), &path, NULL);
	if(path == NULL)
	{
		/* No prior cursor, put it at beginning of page and let the execution do */
		path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
	}
		
	indices = gtk_tree_path_get_indices(path);

	g_critical("DEBUG : move cursor step : %u , int : %i , indice : %i", (guint)arg1, arg2, indices[0]) ;

	value = gtk_adjustment_get_value(Control_Flow_Data->VAdjust_C);

	if(arg1 == GTK_MOVEMENT_DISPLAY_LINES)
	{
		/* Move one line */
		if(arg2 == 1)
		{
			/* move one line down */
			if(indices[0] == Control_Flow_Data->Num_Visible_Events - 1)
			{
				if(value + Control_Flow_Data->Num_Visible_Events <= 
														Control_Flow_Data->Number_Of_Events -1)
				{
					g_critical("need 1 event down") ;
					Control_Flow_Data->Currently_Selected_Event += 1;
					gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C, value+1);
					//gtk_tree_path_free(path);
					//path = gtk_tree_path_new_from_indices(Control_Flow_Data->Num_Visible_Events-1, -1);
					//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
					g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
				}
			}
		} else {
			/* Move one line up */
			if(indices[0] == 0)
			{
				if(value - 1 >= 0 )
				{
					g_critical("need 1 event up") ;
					Control_Flow_Data->Currently_Selected_Event -= 1;
					gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C, value-1);
					//gtk_tree_path_free(path);
					//path = gtk_tree_path_new_from_indices(0, -1);
					//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
					g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
				}
		
			}
		}

	}

	if(arg1 == GTK_MOVEMENT_PAGES)
	{
		/* Move one page */
		if(arg2 == 1)
		{
			if(Control_Flow_Data->Num_Visible_Events == 1)
				value += 1 ;
			/* move one page down */
			if(value + Control_Flow_Data->Num_Visible_Events-1 <= 
											Control_Flow_Data->Number_Of_Events )
			{
				g_critical("need 1 page down") ;

				Control_Flow_Data->Currently_Selected_Event += Control_Flow_Data->Num_Visible_Events-1;
				gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C,
																value+(Control_Flow_Data->Num_Visible_Events-1));
				//gtk_tree_path_free(path);
				//path = gtk_tree_path_new_from_indices(0, -1);
				//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
				g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
			}
		} else {
			/* Move one page up */
			if(Control_Flow_Data->Num_Visible_Events == 1)
					value -= 1 ;

			if(indices[0] < Control_Flow_Data->Num_Visible_Events - 2 )
			{
				if(value - (Control_Flow_Data->Num_Visible_Events-1) >= 0)
				{
					g_critical("need 1 page up") ;
					
					Control_Flow_Data->Currently_Selected_Event -= Control_Flow_Data->Num_Visible_Events-1;

					gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C,
																	value-(Control_Flow_Data->Num_Visible_Events-1));
					//gtk_tree_path_free(path);
					//path = gtk_tree_path_new_from_indices(0, -1);
					//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
					g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");

				} else {
					/* Go to first Event */
					g_critical("need 1 page up") ;

					Control_Flow_Data->Currently_Selected_Event == 0 ;
					gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C,
																	0);
					//gtk_tree_path_free(path);
					//path = gtk_tree_path_new_from_indices(0, -1);
					//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
					g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");

				}
			}
			
		}

	}

	if(arg1 == GTK_MOVEMENT_BUFFER_ENDS)
	{
		/* Move to the ends of the buffer */
		if(arg2 == 1)
		{
			/* move end of buffer */
			g_critical("End of buffer") ;
			Control_Flow_Data->Currently_Selected_Event = Control_Flow_Data->Number_Of_Events-1 ;
			gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C, 
					Control_Flow_Data->Number_Of_Events -
						Control_Flow_Data->Num_Visible_Events);
			//gtk_tree_path_free(path);
			//path = gtk_tree_path_new_from_indices(Control_Flow_Data->Num_Visible_Events-1, -1);
			//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
			g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		} else {
			/* Move beginning of buffer */
			g_critical("Beginning of buffer") ;
			Control_Flow_Data->Currently_Selected_Event = 0 ;
			gtk_adjustment_set_value(Control_Flow_Data->VAdjust_C, 0);
			//gtk_tree_path_free(path);
			//path = gtk_tree_path_new_from_indices(0, -1);
			//gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), path, NULL, FALSE);
			g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		}

	}


	gtk_tree_path_free(path);
}

void Tree_V_cursor_changed_cb (GtkWidget *widget, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) data;

	g_critical("DEBUG : cursor change");
	/* On cursor change, modify the currently selected event by calling
	 * the right API function */
	Tree_V_get_cursor(Control_Flow_Data);
}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	GtkTreePath *Tree_Path;

	g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
	
	get_test_data((int)adjustment->value, Control_Flow_Data->Num_Visible_Events, 
									 Control_Flow_Data);
	
	
	if(Control_Flow_Data->Currently_Selected_Event
								>= Control_Flow_Data->First_Event
								&&
		 Control_Flow_Data->Currently_Selected_Event
		 						<= Control_Flow_Data->Last_Event
								&&
		 Control_Flow_Data->Selected_Event)
	{

		Tree_Path = gtk_tree_path_new_from_indices(
								Control_Flow_Data->Currently_Selected_Event-
								Control_Flow_Data->First_Event,
								-1);

		gtk_tree_view_set_cursor(GTK_TREE_VIEW(Control_Flow_Data->Tree_V), Tree_Path,
																NULL, FALSE);
		gtk_tree_path_free(Tree_Path);
	}


}

gint get_cell_height(GtkTreeView *TreeView)
{
	gint height, width;
	GtkTreeViewColumn *Column = gtk_tree_view_get_column(TreeView, 0);
	GList *Render_List = gtk_tree_view_column_get_cell_renderers(Column);
	GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
	
	gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL, &height);
	g_critical("cell 0 height : %u",height);
	
	return height;
}

void Tree_V_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	gint Cell_Height = get_cell_height(GTK_TREE_VIEW(Control_Flow_Data->Tree_V));
	gint Last_Num_Visible_Events = Control_Flow_Data->Num_Visible_Events;
	gdouble Exact_Num_Visible;
	
	g_critical("size-allocate");

	Exact_Num_Visible = ( alloc->height -
			TREE_VIEW_HEADER_HEIGHT (GTK_TREE_VIEW(Control_Flow_Data->Tree_V)) )
				/ (double)Cell_Height ;

	Control_Flow_Data->Num_Visible_Events = ceil(Exact_Num_Visible) ;

	g_critical("number of events shown : %u",Control_Flow_Data->Num_Visible_Events);
	g_critical("ex number of events shown : %f",Exact_Num_Visible);

	Control_Flow_Data->VAdjust_C->page_increment = 
				 floor(Exact_Num_Visible);
	Control_Flow_Data->VAdjust_C->page_size =
				 floor(Exact_Num_Visible);

	if(Control_Flow_Data->Num_Visible_Events != Last_Num_Visible_Events)
	{
		get_test_data((int)Control_Flow_Data->VAdjust_C->value,
										 Control_Flow_Data->Num_Visible_Events, 
										 Control_Flow_Data);
	}


}

void Tree_V_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data)
{
	gint h;
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	gint Cell_Height = get_cell_height(GTK_TREE_VIEW(Control_Flow_Data->Tree_V));
	
	g_critical("size-request");

	h = Cell_Height + TREE_VIEW_HEADER_HEIGHT
											(GTK_TREE_VIEW(Control_Flow_Data->Tree_V));
	requisition->height = h;
	
}

#endif //DEBUG

void get_test_data(guint Event_Number, guint List_Height, 
									 ControlFlowData *Control_Flow_Data)
{
	//GtkTreeIter iter;
	int i;
	//GtkTreeModel *model = GTK_TREE_MODEL(Control_Flow_Data->Store_M);
	//GtkTreePath *Tree_Path;
	gchar *test_string;

//	if(Event_Number > Control_Flow_Data->Last_Event ||
//		 Event_Number + List_Height-1 < Control_Flow_Data->First_Event ||
//		 Control_Flow_Data->First_Event == -1)
	{
		/* no event can be reused, clear and start from nothing */
		//gtk_list_store_clear(Control_Flow_Data->Store_M);
		for(i=Event_Number; i<Event_Number+List_Height; i++)
		{
			if(i>=Control_Flow_Data->Number_Of_Events) break;
		  /* Add a new row to the model */
		//	gtk_list_store_append (Control_Flow_Data->Store_M, &iter);
		//	gtk_list_store_set (Control_Flow_Data->Store_M, &iter,
		//											CPUID_COLUMN, 0,
		//		  								EVENT_COLUMN, "event irq",
		//										  TIME_COLUMN, i,
		//										  PID_COLUMN, 100,
		//										  ENTRY_LEN_COLUMN, 17,
		//										  EVENT_DESCR_COLUMN, "Detailed information",
		//											-1);
		}
	}
#ifdef DEBUG //do not use this, it's slower and broken
//	} else {
		/* Some events will be reused */
		if(Event_Number < Control_Flow_Data->First_Event)
		{
			/* scrolling up, prepend events */
			Tree_Path = gtk_tree_path_new_from_indices
										(Event_Number+List_Height-1 -
										 Control_Flow_Data->First_Event + 1,
										 -1);
			for(i=0; i<Control_Flow_Data->Last_Event-(Event_Number+List_Height-1);
																				i++)
			{
				/* Remove the last events from the list */
				if(gtk_tree_model_get_iter(model, &iter, Tree_Path))
					gtk_list_store_remove(Control_Flow_Data->Store_M, &iter);
			}

			for(i=Control_Flow_Data->First_Event-1; i>=Event_Number; i--)
			{
				if(i>=Control_Flow_Data->Number_Of_Events) break;
				/* Prepend new events */
				gtk_list_store_prepend (Control_Flow_Data->Store_M, &iter);
				gtk_list_store_set (Control_Flow_Data->Store_M, &iter,
														CPUID_COLUMN, 0,
					  								EVENT_COLUMN, "event irq",
													  TIME_COLUMN, i,
													  PID_COLUMN, 100,
													  ENTRY_LEN_COLUMN, 17,
													  EVENT_DESCR_COLUMN, "Detailed information",
														-1);
			}
		} else {
			/* Scrolling down, append events */
			for(i=Control_Flow_Data->First_Event; i<Event_Number; i++)
			{
				/* Remove these events from the list */
				gtk_tree_model_get_iter_first(model, &iter);
				gtk_list_store_remove(Control_Flow_Data->Store_M, &iter);
			}
			for(i=Control_Flow_Data->Last_Event+1; i<Event_Number+List_Height; i++)
			{
				if(i>=Control_Flow_Data->Number_Of_Events) break;
				/* Append new events */
				gtk_list_store_append (Control_Flow_Data->Store_M, &iter);
				gtk_list_store_set (Control_Flow_Data->Store_M, &iter,
														CPUID_COLUMN, 0,
					  								EVENT_COLUMN, "event irq",
													  TIME_COLUMN, i,
													  PID_COLUMN, 100,
													  ENTRY_LEN_COLUMN, 17,
													  EVENT_DESCR_COLUMN, "Detailed information",
														-1);
			}

		}
	}
#endif //DEBUG
	Control_Flow_Data->First_Event = Event_Number ;
	Control_Flow_Data->Last_Event = Event_Number+List_Height-1 ;
	


}
	
#ifdef DEBUG
void add_test_data(ControlFlowData *Control_Flow_Data)
{
	GtkTreeIter iter;
	int i;
	
	for(i=0; i<10; i++)
	{
	  /* Add a new row to the model */
		gtk_list_store_append (Control_Flow_Data->Store_M, &iter);
		gtk_list_store_set (Control_Flow_Data->Store_M, &iter,
												CPUID_COLUMN, 0,
			  								EVENT_COLUMN, "event irq",
											  TIME_COLUMN, i,
											  PID_COLUMN, 100,
											  ENTRY_LEN_COLUMN, 17,
											  EVENT_DESCR_COLUMN, "Detailed information",
												-1);
	}
							
}
	
#endif //DEBUG
void
GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data)
{
	guint index;

	/* May already been done by GTK window closing */
	if(GTK_IS_WIDGET(Control_Flow_Data->HBox_V))
		gtk_widget_destroy(Control_Flow_Data->HBox_V);
	
	/* Destroy the Tree View */
	//gtk_widget_destroy(Control_Flow_Data->Tree_V);
	
  /*  Clear raw event list */
	//gtk_list_store_clear(Control_Flow_Data->Store_M);
	//gtk_widget_destroy(GTK_WIDGET(Control_Flow_Data->Store_M));

	g_slist_remove(sControl_Flow_Data_List,Control_Flow_Data);
}

//FIXME : call hGuiEvents_Destructor for corresponding data upon widget destroy

#ifdef DEBUG
static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
				ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
        GtkTreeIter iter;
				GtkTreeModel *model = GTK_TREE_MODEL(Control_Flow_Data->Store_M);
        gchar *Event;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, EVENT_COLUMN, &Event, -1);

                g_print ("Event selected :  %s\n", Event);

                g_free (Event);
        }
}
#endif //DEBUG


int Event_Selected_Hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	guint *Event_Number = (guint*) call_data;

	g_critical("DEBUG : event selected by main window : %u", *Event_Number);
	
//	Control_Flow_Data->Currently_Selected_Event = *Event_Number;
//	Control_Flow_Data->Selected_Event = TRUE ;
	
//	Tree_V_set_cursor(Control_Flow_Data);

}



static void destroy_cb( GtkWidget *widget,
		                        gpointer   data )
{ 
	    gtk_main_quit ();
}



int main(int argc, char **argv)
{
	GtkWidget *Window;
	GtkWidget *CF_Viewer;
	GtkWidget *VBox_V;
	GtkWidget *HScroll_VC;
	ControlFlowData *Control_Flow_Data;
	guint ev_sel = 444 ;
	/* Horizontal scrollbar and it's adjustment */
	GtkWidget *VScroll_VC;
  GtkAdjustment *VAdjust_C ;
	
	/* Initialize i18n support */
  gtk_set_locale ();

  /* Initialize the widget set */
  gtk_init (&argc, &argv);

	init();

  Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (Window), ("Test Window"));
	
	g_signal_connect (G_OBJECT (Window), "destroy",
			G_CALLBACK (destroy_cb), NULL);


  VBox_V = gtk_vbox_new(0, 0);
	gtk_container_add (GTK_CONTAINER (Window), VBox_V);

  //ListViewer = hGuiEvents(Window);
  //gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, TRUE, TRUE, 0);

  //ListViewer = hGuiEvents(Window);
  //gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, FALSE, TRUE, 0);
	
	Control_Flow_Data = GuiControlFlow();
	CF_Viewer = Control_Flow_Data->HBox_V;
  gtk_box_pack_start(GTK_BOX(VBox_V), CF_Viewer, TRUE, TRUE, 0);

  /* Create horizontal scrollbar and pack it */
  HScroll_VC = gtk_hscrollbar_new(NULL);
  gtk_box_pack_start(GTK_BOX(VBox_V), HScroll_VC, FALSE, TRUE, 0);
	
	
  gtk_widget_show (HScroll_VC);
  gtk_widget_show (VBox_V);
	gtk_widget_show (Window);

	//Event_Selected_Hook(Control_Flow_Data, &ev_sel);
	
	gtk_main ();

	g_critical("main loop finished");
  
	//hGuiEvents_Destructor(ListViewer);

	//g_critical("GuiEvents Destructor finished");
	destroy();
	
	return 0;
}

/*\@}*/
