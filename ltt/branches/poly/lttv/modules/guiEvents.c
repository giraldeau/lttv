//*! \defgroup GuiEvents libGuiEvents: The GUI Events display plugin */
/*\@{*/

/*! \file GuiEvents.c
 * \brief Graphical plugin for showing events.
 *
 * This plugin lists all the events contained in the current time interval
 * in a list.
 * 
 * This plugin adds a Events Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling gtkTraceSet's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * Coding standard :
 * pm : parameter
 * l  : local
 * g  : global
 * s  : static
 * h  : hook
 * 
 * Author : Karim Yaghmour
 *          Integrated to LTTng by Mathieu Desnoyers, June 2003
 */

#include <math.h>

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/module.h>
//#include <lttv/gtkTraceSet.h>
#include "mw_api.h"
#include "gtktreeprivate.h"

#include "icons/hGuiEventsInsert.xpm"

/** Array containing instanced objects. Used when module is unloaded */
static GSList *sEvent_Viewer_Data_List = NULL ;

typedef struct _EventViewerData {

	/* Model containing list data */
  GtkListStore *Store_M;

	GtkWidget *HBox_V;
	/* Widget to display the data in a columned list */
  GtkWidget *Tree_V;
  GtkAdjustment *VTree_Adjust_C ;
	GdkWindow *TreeWindow ;
	
	/* Vertical scrollbar and it's adjustment */
	GtkWidget *VScroll_VC;
  GtkAdjustment *VAdjust_C ;
	
	/* Selection handler */
	GtkTreeSelection *Select_C;
	
	guint Num_Visible_Events;
	guint First_Event, Last_Event;

} EventViewerData ;

//! Event Viewer's constructor hook
GtkWidget *hGuiEvents(GtkWidget *pmParentWindow);
//! Event Viewer's constructor
EventViewerData *GuiEvents(void);
//! Event Viewer's destructor
void GuiEvents_Destructor(EventViewerData *Event_Viewer_Data);

/* Prototype for selection handler callback */
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
static void Tree_V_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
static void Tree_V_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
static void Tree_V_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void Tree_V_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);

static void get_test_data(guint Event_Number, guint List_Height, 
									 EventViewerData *Event_Viewer_Data);

void add_test_data(EventViewerData *Event_Viewer_Data);

/* TEST DATA, TO BE READ FROM THE TRACE */
static int Number_Of_Events = 1000;

/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init() {
	g_critical("GUI Event Viewer init()");

	/* Register the toolbar insert button */
	//ToolbarItemReg(hGuiEventsInsert_xpm, "Insert Event Viewer", hGuiEvents);

	/* Register the menu item insert entry */
	//MenuItemReg("/", "Insert Event Viewer", hGuiEvents);
}

void destroy_walk(gpointer data, gpointer user_data)
{
	GuiEvents_Destructor((EventViewerData*)data);
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
	int i;

	EventViewerData *Event_Viewer_Data;
	
	g_critical("GUI Event Viewer destroy()");

	g_slist_foreach(sEvent_Viewer_Data_List, destroy_walk, NULL );
	
	/* Unregister the toolbar insert button */
	//ToolbarItemUnreg(hGuiEvents);

	/* Unregister the menu item insert entry */
	//MenuItemUnreg(hGuiEvents);
}

/* Enumeration of the columns */
enum
{
	CPUID_COLUMN,
	EVENT_COLUMN,
	TIME_COLUMN,
	PID_COLUMN,
	ENTRY_LEN_COLUMN,
	EVENT_DESCR_COLUMN,
	N_COLUMNS
};


/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param pmParentWindow A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
hGuiEvents(GtkWidget *pmParentWindow)
{
	EventViewerData* Event_Viewer_Data = GuiEvents() ;

	return Event_Viewer_Data->HBox_V ;
	
}

/**
 * Event Viewer's constructor
 *
 * This constructor is used to create EventViewerData data structure.
 * @return The Event viewer data created.
 */
EventViewerData *
GuiEvents(void)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	EventViewerData* Event_Viewer_Data = g_new(EventViewerData,1) ;
	
	
	/* Create a model for storing the data list */
  Event_Viewer_Data->Store_M = gtk_list_store_new (N_COLUMNS,       /* Total number of columns */
                                            G_TYPE_INT,      /* CPUID                  */
																						G_TYPE_STRING,   /* Event                   */
            	                              G_TYPE_INT,      /* Time                    */
              	                            G_TYPE_INT,      /* PID                     */
                	                          G_TYPE_INT,      /* Entry length            */
                  	                        G_TYPE_STRING);  /* Event's description     */
	
	/* Create the viewer widget for the columned list */
	Event_Viewer_Data->Tree_V = gtk_tree_view_new_with_model (GTK_TREE_MODEL (Event_Viewer_Data->Store_M));
		
	g_signal_connect (G_OBJECT (Event_Viewer_Data->Tree_V), "size-allocate",
	                  G_CALLBACK (Tree_V_size_allocate_cb),
	                  Event_Viewer_Data);
		g_signal_connect (G_OBJECT (Event_Viewer_Data->Tree_V), "size-request",
	                  G_CALLBACK (Tree_V_size_request_cb),
	                  Event_Viewer_Data);
	
		g_signal_connect (G_OBJECT (Event_Viewer_Data->Tree_V), "cursor-changed",
	                  G_CALLBACK (Tree_V_cursor_changed_cb),
	                  Event_Viewer_Data);
	
		g_signal_connect (G_OBJECT (Event_Viewer_Data->Tree_V), "move-cursor",
	                  G_CALLBACK (Tree_V_move_cursor_cb),
	                  Event_Viewer_Data);
// Use on each column!
//gtk_tree_view_column_set_sizing(Event_Viewer_Data->Tree_V, GTK_TREE_VIEW_COLUMN_FIXED);
	
	/* The view now holds a reference.  We can get rid of our own
	 * reference */
	g_object_unref (G_OBJECT (Event_Viewer_Data->Store_M));
	

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
	/* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPUID",
																											renderer,
                                                      "text", CPUID_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 0.0);
	gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event",
																											renderer,
                                                      "text", EVENT_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 0.0);
	gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time",
																											renderer,
                                                      "text", TIME_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("PID",
																											renderer,
                                                      "text", PID_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Entry Length",
																											renderer,
                                                      "text", ENTRY_LEN_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_fixed_width (column, 60);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event's Description",
																											renderer,
                                                      "text", EVENT_DESCR_COLUMN,
                                                      NULL);
	gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V), column);


	/* Setup the selection handler */
	Event_Viewer_Data->Select_C = gtk_tree_view_get_selection (GTK_TREE_VIEW (Event_Viewer_Data->Tree_V));
	gtk_tree_selection_set_mode (Event_Viewer_Data->Select_C, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (Event_Viewer_Data->Select_C), "changed",
	                  G_CALLBACK (tree_selection_changed_cb),
	                  Event_Viewer_Data);
	
  Event_Viewer_Data->HBox_V = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(Event_Viewer_Data->HBox_V), Event_Viewer_Data->Tree_V, TRUE, TRUE, 0);

  /* Create vertical scrollbar and pack it */
  Event_Viewer_Data->VScroll_VC = gtk_vscrollbar_new(NULL);
  gtk_box_pack_start(GTK_BOX(Event_Viewer_Data->HBox_V), Event_Viewer_Data->VScroll_VC, FALSE, TRUE, 0);
	
  /* Get the vertical scrollbar's adjustment */
  Event_Viewer_Data->VAdjust_C = gtk_range_get_adjustment(GTK_RANGE(Event_Viewer_Data->VScroll_VC));
	Event_Viewer_Data->VTree_Adjust_C = gtk_tree_view_get_vadjustment(
															GTK_TREE_VIEW (Event_Viewer_Data->Tree_V));

	g_signal_connect (G_OBJECT (Event_Viewer_Data->VAdjust_C), "value-changed",
	                  G_CALLBACK (v_scroll_cb),
	                  Event_Viewer_Data);
	/* Set the upper bound to the last event number */
	Event_Viewer_Data->VAdjust_C->lower = 0;
	Event_Viewer_Data->VAdjust_C->upper = Number_Of_Events;
	Event_Viewer_Data->VAdjust_C->value = 0;
	Event_Viewer_Data->VAdjust_C->step_increment = 1;
	Event_Viewer_Data->VAdjust_C->page_increment = 
				 Event_Viewer_Data->VTree_Adjust_C->upper;
	Event_Viewer_Data->VAdjust_C->page_size =
				 Event_Viewer_Data->VTree_Adjust_C->upper;
	g_critical("value : %u",Event_Viewer_Data->VTree_Adjust_C->upper);
  /*  Raw event trace */
  gtk_widget_show(Event_Viewer_Data->HBox_V);
  gtk_widget_show(Event_Viewer_Data->Tree_V);
  gtk_widget_show(Event_Viewer_Data->VScroll_VC);

	/* Add the object's information to the module's array */
  g_slist_append(sEvent_Viewer_Data_List, Event_Viewer_Data);

	Event_Viewer_Data->First_Event = -1 ;
	Event_Viewer_Data->Last_Event = 0 ;

	Event_Viewer_Data->Num_Visible_Events = 1;

	/* Test data */
	get_test_data((int)Event_Viewer_Data->VAdjust_C->value,
									 Event_Viewer_Data->Num_Visible_Events, 
									 Event_Viewer_Data);

	return Event_Viewer_Data;
}

void Tree_V_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data)
{
	GtkTreePath *path; // = gtk_tree_path_new();
	gint *indices;
	gdouble value;
	EventViewerData *Event_Viewer_Data = (EventViewerData*)data;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), &path, NULL);
	if(path == NULL)
	{
		/* No prior cursor, put it at beginning of page and let the execution do */
		path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
	}
		
	indices = gtk_tree_path_get_indices(path);

	g_critical("DEBUG : move cursor step : %u , int : %i , indice : %i", (guint)arg1, arg2, indices[0]) ;

	value = gtk_adjustment_get_value(Event_Viewer_Data->VAdjust_C);

	if(arg1 == GTK_MOVEMENT_DISPLAY_LINES)
	{
		/* Move one line */
		if(arg2 == 1)
		{
			/* move one line down */
			if(indices[0] == Event_Viewer_Data->Num_Visible_Events - 1)
			{
				if(value + Event_Viewer_Data->Num_Visible_Events <= Number_Of_Events -1)
				{
					g_critical("need 1 event down") ;
					gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C, value+1);
					gtk_tree_path_free(path);
					path = gtk_tree_path_new_from_indices(Event_Viewer_Data->Num_Visible_Events-1, -1);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
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
					gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C, value-1);
					gtk_tree_path_free(path);
					path = gtk_tree_path_new_from_indices(0, -1);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
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
			if(Event_Viewer_Data->Num_Visible_Events == 1)
				value += 1 ;
			/* move one page down */
			if(value + Event_Viewer_Data->Num_Visible_Events-1 <= Number_Of_Events )
			{
				g_critical("need 1 page down") ;

				gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C,
																value+(Event_Viewer_Data->Num_Visible_Events-1));
				gtk_tree_path_free(path);
				path = gtk_tree_path_new_from_indices(0, -1);
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
				g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
			}
		} else {
			/* Move one page up */
			if(Event_Viewer_Data->Num_Visible_Events == 1)
					value -= 1 ;

			if(indices[0] < Event_Viewer_Data->Num_Visible_Events - 2 )
			{
				if(value - (Event_Viewer_Data->Num_Visible_Events-1) >= 0)
				{
					g_critical("need 1 page up") ;
					

					gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C,
																	value-(Event_Viewer_Data->Num_Visible_Events-1));
					gtk_tree_path_free(path);
					path = gtk_tree_path_new_from_indices(0, -1);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
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
			gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C, Number_Of_Events-Event_Viewer_Data->Num_Visible_Events);
			gtk_tree_path_free(path);
			path = gtk_tree_path_new_from_indices(Event_Viewer_Data->Num_Visible_Events-1, -1);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
			g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		} else {
			/* Move beginning of buffer */
			g_critical("Beginning of buffer") ;
			gtk_adjustment_set_value(Event_Viewer_Data->VAdjust_C, 0);
			gtk_tree_path_free(path);
			path = gtk_tree_path_new_from_indices(0, -1);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V), path, NULL, FALSE);
			g_signal_stop_emission_by_name(G_OBJECT(widget), "move-cursor");
		}

	}



	gtk_tree_path_free(path);

}

void Tree_V_cursor_changed_cb (GtkWidget *widget, gpointer data)
{
	g_critical("DEBUG : cursor change");

}


void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	EventViewerData *Event_Viewer_Data = (EventViewerData*)data;
	g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
	
	get_test_data((int)adjustment->value, Event_Viewer_Data->Num_Visible_Events, 
									 Event_Viewer_Data);
	
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
	EventViewerData *Event_Viewer_Data = (EventViewerData*)data;
	gint Cell_Height = get_cell_height(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V));
	gint Last_Num_Visible_Events = Event_Viewer_Data->Num_Visible_Events;
	gdouble Exact_Num_Visible;
	
	g_critical("size-allocate");

	Exact_Num_Visible = ( alloc->height -
			TREE_VIEW_HEADER_HEIGHT (GTK_TREE_VIEW(Event_Viewer_Data->Tree_V)) )
				/ (double)Cell_Height ;

	Event_Viewer_Data->Num_Visible_Events = ceil(Exact_Num_Visible) ;

	g_critical("number of events shown : %u",Event_Viewer_Data->Num_Visible_Events);
	g_critical("ex number of events shown : %f",Exact_Num_Visible);

	Event_Viewer_Data->VAdjust_C->page_increment = 
				 floor(Exact_Num_Visible);
	Event_Viewer_Data->VAdjust_C->page_size =
				 floor(Exact_Num_Visible);

	if(Event_Viewer_Data->Num_Visible_Events != Last_Num_Visible_Events)
	{
		get_test_data((int)Event_Viewer_Data->VAdjust_C->value,
										 Event_Viewer_Data->Num_Visible_Events, 
										 Event_Viewer_Data);
	}


}

void Tree_V_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data)
{
	gint h;
	EventViewerData *Event_Viewer_Data = (EventViewerData*)data;
	gint Cell_Height = get_cell_height(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V));
	
	g_critical("size-request");

	h = Cell_Height + TREE_VIEW_HEADER_HEIGHT
											(GTK_TREE_VIEW(Event_Viewer_Data->Tree_V));
	requisition->height = h;
	
}

void get_test_data(guint Event_Number, guint List_Height, 
									 EventViewerData *Event_Viewer_Data)
{
	GtkTreeIter iter;
	int i;
	GtkTreeModel *model = GTK_TREE_MODEL(Event_Viewer_Data->Store_M);
	GtkTreePath *Tree_Path;
	gchar *test_string;

//	if(Event_Number > Event_Viewer_Data->Last_Event ||
//		 Event_Number + List_Height-1 < Event_Viewer_Data->First_Event ||
//		 Event_Viewer_Data->First_Event == -1)
	{
		/* no event can be reused, clear and start from nothing */
		gtk_list_store_clear(Event_Viewer_Data->Store_M);
		for(i=Event_Number; i<Event_Number+List_Height; i++)
		{
			if(i>=Number_Of_Events) break;
		  /* Add a new row to the model */
			gtk_list_store_append (Event_Viewer_Data->Store_M, &iter);
			gtk_list_store_set (Event_Viewer_Data->Store_M, &iter,
													CPUID_COLUMN, 0,
				  								EVENT_COLUMN, "event irq",
												  TIME_COLUMN, i,
												  PID_COLUMN, 100,
												  ENTRY_LEN_COLUMN, 17,
												  EVENT_DESCR_COLUMN, "Detailed information",
													-1);
		}
	}
#ifdef DEBUG //do not use this, it's slower and broken
//	} else {
		/* Some events will be reused */
		if(Event_Number < Event_Viewer_Data->First_Event)
		{
			/* scrolling up, prepend events */
			Tree_Path = gtk_tree_path_new_from_indices
										(Event_Number+List_Height-1 -
										 Event_Viewer_Data->First_Event + 1,
										 -1);
			for(i=0; i<Event_Viewer_Data->Last_Event-(Event_Number+List_Height-1);
																				i++)
			{
				/* Remove the last events from the list */
				if(gtk_tree_model_get_iter(model, &iter, Tree_Path))
					gtk_list_store_remove(Event_Viewer_Data->Store_M, &iter);
			}

			for(i=Event_Viewer_Data->First_Event-1; i>=Event_Number; i--)
			{
				if(i>=Number_Of_Events) break;
				/* Prepend new events */
				gtk_list_store_prepend (Event_Viewer_Data->Store_M, &iter);
				gtk_list_store_set (Event_Viewer_Data->Store_M, &iter,
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
			for(i=Event_Viewer_Data->First_Event; i<Event_Number; i++)
			{
				/* Remove these events from the list */
				gtk_tree_model_get_iter_first(model, &iter);
				gtk_list_store_remove(Event_Viewer_Data->Store_M, &iter);
			}
			for(i=Event_Viewer_Data->Last_Event+1; i<Event_Number+List_Height; i++)
			{
				if(i>=Number_Of_Events) break;
				/* Append new events */
				gtk_list_store_append (Event_Viewer_Data->Store_M, &iter);
				gtk_list_store_set (Event_Viewer_Data->Store_M, &iter,
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
	Event_Viewer_Data->First_Event = Event_Number ;
	Event_Viewer_Data->Last_Event = Event_Number+List_Height-1 ;
	
}
	

void add_test_data(EventViewerData *Event_Viewer_Data)
{
	GtkTreeIter iter;
	int i;
	
	for(i=0; i<10; i++)
	{
	  /* Add a new row to the model */
		gtk_list_store_append (Event_Viewer_Data->Store_M, &iter);
		gtk_list_store_set (Event_Viewer_Data->Store_M, &iter,
												CPUID_COLUMN, 0,
			  								EVENT_COLUMN, "event irq",
											  TIME_COLUMN, i,
											  PID_COLUMN, 100,
											  ENTRY_LEN_COLUMN, 17,
											  EVENT_DESCR_COLUMN, "Detailed information",
												-1);
	}
							
}
	

void
GuiEvents_Destructor(EventViewerData *Event_Viewer_Data)
{
	guint index;

	/* May already been done by GTK window closing */
	if(GTK_IS_WIDGET(Event_Viewer_Data->HBox_V))
		gtk_widget_destroy(Event_Viewer_Data->HBox_V);
	
	/* Destroy the Tree View */
	//gtk_widget_destroy(Event_Viewer_Data->Tree_V);
	
  /*  Clear raw event list */
	//gtk_list_store_clear(Event_Viewer_Data->Store_M);
	//gtk_widget_destroy(GTK_WIDGET(Event_Viewer_Data->Store_M));

	g_slist_remove(sEvent_Viewer_Data_List,Event_Viewer_Data);
}

//FIXME : call hGuiEvents_Destructor for corresponding data upon widget destroy

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
				EventViewerData *Event_Viewer_Data = (EventViewerData*)data;
        GtkTreeIter iter;
				GtkTreeModel *model = GTK_TREE_MODEL(Event_Viewer_Data->Store_M);
        gchar *Event;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
                gtk_tree_model_get (model, &iter, EVENT_COLUMN, &Event, -1);

                g_print ("Event selected :  %s\n", Event);

                g_free (Event);
        }
}




/* Imported code from LTT 0.9.6pre2 tracevisualizer */
#ifdef DEBUG

/******************************************************************
 * Function :
 *    WDI_gtk_clist_set_last_row_data_full()
 * Description :
 *    Appends data to the last row of a GtkClist.
 * Parameters :
 * Return values :
 *    NONE.
 * History :
 *    J.H.D., 27/08/99, Initial typing.
 * Note :
 *    Based on gtk_clist_set_row_data_full() version 1.2.3.
 *    Much faster than using gtk_clist_set_row_data_full().
 ******************************************************************/
static void WDI_gtk_clist_set_last_row_data_full(GtkCList*         pmClist,
					  gpointer          pmData,
					  GtkDestroyNotify  pmDestroy)
{
  GtkCListRow *pClistRow;

  g_return_if_fail (pmClist != NULL);
  g_return_if_fail (GTK_IS_CLIST (pmClist));
  g_return_if_fail (pmClist->row_list_end != NULL);

  pClistRow = pmClist->row_list_end->data;
  pClistRow->data    = pmData;
  pClistRow->destroy = pmDestroy;
}


/******************************************************************
 * Function :
 *    SHRTEventSelect()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTEventSelect(GtkWidget*      pmCList,
		     gint            pmRow,
		     gint            pmColumn,
		     GdkEventButton* pmEvent,
		     gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Store the selected event */
  pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), pmRow);
  pSysView->Window->EventSelected = TRUE;
}

/******************************************************************
 * Function :
 *    SHRTEventButtonPress()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTEventButtonPress(GtkWidget*      pmCList,
			  GdkEventButton* pmEvent,
			  gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */
  gint         row, column;     /* The clicked row and column */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* if we have a right-click event */
  if(pmEvent->button == 3)
    /* If we clicked on an item, get its row and column values */
    if(gtk_clist_get_selection_info(GTK_CLIST(pmCList), pmEvent->x, pmEvent->y, &row, &column))
      {
      /* Highlight the selected row */
      gtk_clist_select_row(GTK_CLIST(pmCList), row, column);

      /* Store the selected event */
      pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), row);
      pSysView->Window->EventSelected = TRUE;

      /* Display the popup menu */
      gtk_menu_popup(GTK_MENU(pSysView->Window->RawEventPopup),
		     NULL, NULL, NULL, NULL,
		     pmEvent->button, GDK_CURRENT_TIME);
      }
}


/******************************************************************
 * Function :
 *    SHRTVAdjustValueChanged()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
static void SHRTVAdjustValueChanged(GtkAdjustment*  pmVAdjust,
			     gpointer        pmData)
{
  event        lEvent;          /* Event used for searching */
  guint32      lPosition;       /* The position to scroll to */
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Is there an event database? */
  if(pSysView->EventDB == NULL)
    return;

  /* Set the pointer to the first event */
  if(pSysView->EventDB->TraceStart == NULL)
    return;

  /* Are we closer to the beginning? */
  if((pmVAdjust->value - (pmVAdjust->upper / 2)) < 0)
    {
    /* Set the navigation pointer to the beginning of the list */
    lEvent =  pSysView->EventDB->FirstEvent;

    /* Calculate distance from beginning */
    lPosition = (guint32) pmVAdjust->value;

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventNext(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }
  else
    {
    /* Set the navigation pointer to the end of the list */
    lEvent = pSysView->EventDB->LastEvent;

    /* Calculate distance from end */
    lPosition = (guint32) (pmVAdjust->upper - pmVAdjust->value);

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventPrev(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }

  /* Fill the event list according to what was found */
  WDFillEventList(pSysView->Window->RTCList,
		  pSysView->EventDB,
		  pSysView->System,
		  &lEvent,
		  &(pSysView->Window->LastSelectedEvent));
}



/******************************************************************
 * Function :
 *    WDConnectSignals()
 * Description :
 *    Attaches signal handlers to the window items.
 * Parameters :
 *    pmSysView, System view for which signals have to be connected
 * Return values :
 *    NONE
 * History :
 * Note :
 *    This function attaches a pointer to the main window during
 *    the connect. This means that the handlers will get a pointer
 *    to the window in the data argument.
 ******************************************************************/
static void WDConnectSignals(systemView* pmSysView)
{
  /* Raw event Popup menu */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawGotoProcess),
		     "activate",
		     GTK_SIGNAL_FUNC(SHGotoProcAnalysis),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawViewEvent),
		     "activate",
		     GTK_SIGNAL_FUNC(SHViewEventInEG),
		     pmSysView);

  /* Set event list callbacks */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "select_row",
		     GTK_SIGNAL_FUNC(SHRTEventSelect),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "button-press-event",
		     GTK_SIGNAL_FUNC(SHRTEventButtonPress),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTVAdjust),
		     "value-changed",
		     GTK_SIGNAL_FUNC(SHRTVAdjustValueChanged),
		     pmSysView);


}


/******************************************************************
 * Function :
 *    WDFillEventList()
 * Description :
 *    Fills the window's event list using the trace database.
 * Parameters :
 *    pmList, The list to be filled.
 *    pmTraceDB, The database of events.
 *    pmSystem, The system to which this list belongs.
 *    pmEvent, Event from which we start drawing.
 *    pmSelectedEvent, Event selected if any.
 * Return values :
 *    NONE.
 * History :
 *    K.Y., 18/06/99, Initial typing.
 * Note :
 ******************************************************************/
static void WDFillEventList(GtkWidget*  pmList,
		     db*         pmTraceDB,
		     systemInfo* pmSystem,
		     event*      pmEvent,
		     event*      pmSelectedEvent)
{
  gint                i = 0;                              /* Generic index */
  event               lEvent;                             /* Generic event */
  gchar               lTimeStr[TIME_STR_LEN];             /* Time of event */
  static gchar*       lString[RTCLIST_NB_COLUMNS]={'\0'}; /* Strings describing event */
  process*            pProcess;                           /* Generic process pointer */
#if SUPP_RTAI
  RTAItask*           pTask = NULL;                       /* Generic task pointer */
#endif /* SUPP_RTAI */
  eventDescription    lEventDesc;                         /* Description of event */

  /* Did we allocate space for strings */
  if(lString[0] == NULL)
    /* Allocate space for strings */
    for (i = 0;  i < RTCLIST_NB_COLUMNS - 1; i++)
      lString[i] = (char*) g_malloc(MW_DEFAULT_STRLEN);

  /* Allocate space for description string */
  lString[RTCLIST_NB_COLUMNS - 1] = (char*) g_malloc(MW_LONG_STRLEN);

  /* If no event was supplied, start at the beginning */
  if(pmEvent == NULL)
    lEvent = pmTraceDB->FirstEvent;
  else
    lEvent = *pmEvent;

  /* Freeze and clear clist */
  gtk_clist_freeze(GTK_CLIST(pmList));
  gtk_clist_clear(GTK_CLIST(pmList));

  /* Reset index */
  i = 0;

  /* Go through the event list */
  do
    {
    /* Get the event description */
    DBEventDescription(pmTraceDB, &lEvent, TRUE, &lEventDesc);

    /* Get the event's process */
    pProcess = DBEventProcess(pmTraceDB, &lEvent, pmSystem, FALSE);

#if SUPP_RTAI
    /* Does this trace contain RTAI information */
    if(pmTraceDB->SystemType == TRACE_SYS_TYPE_RTAI_LINUX)
      /* Get the RTAI task to which this event belongs */
      pTask = RTAIDBEventTask(pmTraceDB, &lEvent, pmSystem, FALSE);
#endif /* SUPP_RTAI */

    /* Set the event's entry in the list of raw events displayed */
    sRawEventsDisplayed[i] = lEvent;

    /* Add text describing the event */
    /*  The CPU ID */
    if(pmTraceDB->LogCPUID == TRUE)
      snprintf(lString[0], MW_DEFAULT_STRLEN, "%d", lEventDesc.CPUID);
    else
      snprintf(lString[0], MW_DEFAULT_STRLEN, "0");

    /*  The event ID */
    snprintf(lString[1], MW_DEFAULT_STRLEN, "%s", pmTraceDB->EventString(pmTraceDB, lEventDesc.ID, &lEvent));

    /*  The event's time of occurence */
    DBFormatTimeInReadableString(lTimeStr,
				 lEventDesc.Time.tv_sec,
				 lEventDesc.Time.tv_usec);    
    snprintf(lString[2], MW_DEFAULT_STRLEN, "%s", lTimeStr);

    /* Is this an RT event */
    if(lEventDesc.ID <= TRACE_MAX)
      {
      /*  The PID of the process to which the event belongs */
      if(pProcess != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "%d", pProcess->PID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "N/A");
      }
#if SUPP_RTAI
    else
      {
      /*  The TID of the task to which the event belongs */
      if(pTask != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:%d", pTask->TID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:N/A");
      }
#endif /* SUPP_RTAI */

    /*  The size of the entry */
    snprintf(lString[4], MW_DEFAULT_STRLEN, "%d", lEventDesc.Size);

    /*  The string describing the event */
    snprintf(lString[5], MW_LONG_STRLEN, "%s", lEventDesc.String);

    /* Insert the entry into the list */
    gtk_clist_append(GTK_CLIST(pmList), lString);

    /* Set the row's data to point to the current event */
    WDI_gtk_clist_set_last_row_data_full(GTK_CLIST(pmList), (gpointer) &(sRawEventsDisplayed[i]), NULL);

    /* Was this the last selected event */
    if(DBEventsEqual(lEvent, (*pmSelectedEvent)))
      gtk_clist_select_row(GTK_CLIST(pmList), i, 0);

    /* Go to next row */
    i++;
    } while((DBEventNext(pmTraceDB, &lEvent) == TRUE) && (i < RTCLIST_NB_ROWS));

  /* Resize the list's length */
  gtk_widget_queue_resize(pmList);

  /* Thaw the clist */
  gtk_clist_thaw(GTK_CLIST(pmList));
}

#endif //DEBUG

static void destroy_cb( GtkWidget *widget,
		                        gpointer   data )
{ 
	    gtk_main_quit ();
}



int main(int argc, char **argv)
{
	GtkWidget *Window;
	GtkWidget *ListViewer;
	GtkWidget *VBox_V;
	EventViewerData *Event_Viewer_Data;

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

  ListViewer = hGuiEvents(Window);
  gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, TRUE, TRUE, 0);

  //ListViewer = hGuiEvents(Window);
  //gtk_box_pack_start(GTK_BOX(VBox_V), ListViewer, FALSE, TRUE, 0);

  gtk_widget_show (VBox_V);
	gtk_widget_show (Window);

	gtk_main ();

	g_critical("main loop finished");
  
	//hGuiEvents_Destructor(ListViewer);

	//g_critical("GuiEvents Destructor finished");
	destroy();
	
	return 0;
}


/*\@}*/

