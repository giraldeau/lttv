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

typedef struct _ltt_time_interval
{
	ltt_time time_begin, time_end;
} ltt_time_interval;


/*****************************************************************************
 *                         Definition of structures                          *
 *****************************************************************************/

typedef struct _DrawingAreaInfo {

	guint height, width;

} DrawingAreaInfo ;

typedef struct _ControlFlowData {

	GtkWidget *Drawing_Area_V;
	GtkWidget *Scrolled_Window_VC;
	
	GtkWidget *Process_List_VC;
	
  /* Model containing list data */
	GtkListStore *Store_M;
	
	GtkWidget *HBox_V;
	GtkWidget *Inside_HBox_V;

  GtkAdjustment *VAdjust_C ;
	
	/* Trace information */
	TraceSet *Trace_Set;
	TraceStatistics *Trace_Statistics;
	
	/* Shown events information */
	guint First_Event, Last_Event;
	ltt_time Begin_Time, End_Time;
	
	
	/* Drawing Area Info */
	DrawingAreaInfo Drawing_Area_Info;

	/* TEST DATA, TO BE READ FROM THE TRACE */
	gint Number_Of_Events ;
	guint Currently_Selected_Event  ;
	gboolean Selected_Event ;
	guint Number_Of_Process;

} ControlFlowData ;

/* Structure used to store and use information relative to one events refresh
 * request. Typically filled in by the expose event callback, then passed to the
 * library call, then used by the drawing hooks. Then, once all the events are
 * sent, it is freed by the hook called after the reading.
 */
typedef struct _EventRequest
{
	ControlFlowData *Control_Flow_Data;
	ltt_time time_begin, time_end;
	/* Fill the Events_Context during the initial expose, before calling for
	 * events.
	 */
	GArray Events_Context; //FIXME
} EventRequest ;


/** Array containing instanced objects. Used when module is unloaded */
static GSList *sControl_Flow_Data_List = NULL ;


/*****************************************************************************
 *                         Function prototypes                               *
 *****************************************************************************/
//! Control Flow Viewer's constructor hook
GtkWidget *hGuiControlFlow(GtkWidget *pmParentWindow);
//! Control Flow Viewer's constructor
ControlFlowData *GuiControlFlow(void);
//! Control Flow Viewer's destructor
void GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data);


static int Event_Selected_Hook(void *hook_data, void *call_data);

static lttv_hooks
	*Draw_Before_Hooks,
	*Draw_Event_Hooks,
	*Draw_After_Hooks;

Draw_Before_Hook(void *hook_data, void *call_data)
Draw_Event_Hook(void *hook_data, void *call_data)
Draw_After_Hook(void *hook_data, void *call_data)


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



/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
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

	/* Setup the hooks */
	Draw_Before_Hooks = lttv_hooks_new();
	Draw_Event_Hooks = lttv_hooks_new();
	Draw_After_Hooks = lttv_hooks_new();
	
	lttv_hooks_add(Draw_Before_Hooks, Draw_Before_Hook, NULL);
	lttv_hooks_add(Draw_Event_Hooks, Draw_Event_Hook, NULL);
	lttv_hooks_add(Draw_After_Hooks, Draw_After_Hook, NULL);
	
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
	
	lttv_hooks_destroy(Draw_Before_Hooks);
	lttv_hooks_destroy(Draw_Event_Hooks);
	lttv_hooks_destroy(Draw_After_Hooks);
	
	/* Unregister the toolbar insert button */
	//ToolbarItemUnreg(hGuiEvents);

	/* Unregister the menu item insert entry */
	//MenuItemUnreg(hGuiEvents);
}


/*****************************************************************************
 *                     Control Flow Viewer class implementation              *
 *****************************************************************************/


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



	Control_Flow_Data->Inside_HBox_V = gtk_hbox_new(0, 0);

	gtk_box_pack_start(GTK_BOX(Control_Flow_Data->Inside_HBox_V), Control_Flow_Data->Process_List_VC, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(Control_Flow_Data->Inside_HBox_V), Control_Flow_Data->Drawing_Area_V, TRUE, TRUE, 0);


	Control_Flow_Data->VAdjust_C = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,	/* Value */
																										0.0,	/* Lower */
																										0.0,	/* Upper */
																										0.0,			/* Step inc. */
																										0.0,			/* Page inc. */
																										0.0	));	/* page size */
	

	Control_Flow_Data->Scrolled_Window_VC = gtk_scrolled_window_new (NULL,
																						Control_Flow_Data->VAdjust_C);
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(Control_Flow_Data->Scrolled_Window_VC) ,
																	GTK_POLICY_NEVER,
																	GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Control_Flow_Data->Scrolled_Window_VC),
										Control_Flow_Data->Inside_HBox_V);
	
	
	g_signal_connect (G_OBJECT (Control_Flow_Data->Drawing_Area_V), "expose_event",  
	                  G_CALLBACK (expose_event_cb), Control_Flow_Data);


	
	g_signal_connect (G_OBJECT (Control_Flow_Data->VAdjust_C), "value-changed",
	                  G_CALLBACK (v_scroll_cb),
	                  Control_Flow_Data);

	add_test_process(Control_Flow_Data);



	/* Set the size of the drawing area */
	Drawing_Area_Init(Control_Flow_Data);

	/* Get trace statistics */
	Control_Flow_Data->Trace_Statistics = get_trace_statistics(Trace);


	gtk_widget_show(Control_Flow_Data->Drawing_Area_V);
	gtk_widget_show(Control_Flow_Data->Process_List_VC);
	gtk_widget_show(Control_Flow_Data->Inside_HBox_V);
	gtk_widget_show(Control_Flow_Data->Scrolled_Window_VC);

	test_draw(Control_Flow_Data);

	return Control_Flow_Data;

}

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



/*****************************************************************************
 *                              Drawing functions                            *
 *****************************************************************************/

typedef enum 
{
	RED,
	GREEN,
	BLUE,
	WHITE,
	BLACK

} ControlFlowColors;

/* Vector of unallocated colors */
static GdkColor CF_Colors [] = 
{
	{ 0, 0xffff, 0x0000, 0x0000 },	// RED
	{ 0, 0x0000, 0xffff, 0x0000 },	// GREEN
	{ 0, 0x0000, 0x0000, 0xffff },	// BLUE
	{ 0, 0xffff, 0xffff, 0xffff },	// WHITE
	{ 0, 0x0000, 0x0000, 0x0000 }		// BLACK
};


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


/* get_time_from_pixels
 *
 * Get the time interval from window time and pixels, and pixels requested. This
 * function uses TimeMul, which should only be used if the float value is lower
 * that 4, and here it's always lower than 1, so it's ok.
 */
void get_time_from_pixels(guint area_x, guint area_width,
													guint window_width,
													ltt_time &window_time_begin, ltt_time &window_time_end,
													ltt_time &time_begin, ltt_time &time_end)
{
	ltt_time window_time_interval;
	
	TimeSub(window_time_interval, window_time_end, window_time_begin);

	
	TimeMul(time_begin, window_time_interval, (area_x/(float)window_width));
	TimeAdd(time_begin, window_time_begin, time_begin);
	
	TimeMul(time_end, window_time_interval, (area_width/(float)window_width));
	TimeAdd(time_end, time_begin, time_end);
	
}

void Drawing_Area_Init(ControlFlowData *Control_Flow_Data)
{
	DrawingAreaInfo *Drawing_Area_Info = &Control_Flow_Data->Drawing_Area_Info;
	guint w;

	w = 500;
	

	Drawing_Area_Info->height =
		get_cell_height(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC))
				* Control_Flow_Data->Number_Of_Process ;
	
	gtk_widget_modify_bg(Control_Flow_Data->Drawing_Area_V,
											 GTK_STATE_NORMAL,
											 &CF_Colors[BLACK]);
											 
	
	gtk_widget_set_size_request (Control_Flow_Data->Drawing_Area_V,
					w,
					Drawing_Area_Info->height);
	
	
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


/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/
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

int Event_Selected_Hook(void *hook_data, void *call_data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*) hook_data;
	guint *Event_Number = (guint*) call_data;

	g_critical("DEBUG : event selected by main window : %u", *Event_Number);
	
//	Control_Flow_Data->Currently_Selected_Event = *Event_Number;
//	Control_Flow_Data->Selected_Event = TRUE ;
	
//	Tree_V_set_cursor(Control_Flow_Data);

}


/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in Event_Request.
 */
Draw_Before_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	EventsContext Events_Context = (EventsContext*)call_data;
	
	Event_Request->Events_Context = Events_Context;
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
Draw_Event_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;

}


Draw_After_Hook(void *hook_data, void *call_data)
{
	EventRequest *Event_Request = (EventRequest*)hook_data;
	
	g_free(Event_Request);
}

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

#ifdef DEBUG
void test_draw() {
	gint cell_height = get_cell_height(GTK_TREE_VIEW(Control_Flow_Data->Process_List_VC));
	GdkGC *GC = gdk_gc_new(widget->window);
	GdkColor color = CF_Colors[GREEN];
	
	gdk_color_alloc (gdk_colormap_get_system () , &color);
	
	g_critical("expose");

	/* When redrawing, use widget->allocation.width to get the width of
	 * drawable area. */
	Control_Flow_Data->Drawing_Area_Info.width = widget->allocation.width;
	
	test_draw(Control_Flow_Data);
	
	gdk_gc_copy(GC,widget->style->white_gc);
	gdk_gc_set_foreground(GC,&color);
	
	//gdk_draw_arc (widget->window,
  //              widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
  //              TRUE,
  //              //0, 0, widget->allocation.width, widget->allocation.height,
  //              0, 0, widget->allocation.width,
	//							Control_Flow_Data->Drawing_Area_Info.height,
  //              0, 64 * 360);

	
	//Drawing_Area_Init(Control_Flow_Data);
	
	// 2 pixels for the box around the drawing area, 1 pixel for off-by-one
	// (starting from 0)
	//gdk_gc_copy (&GC, widget->style->fg_gc[GTK_WIDGET_STATE (widget)]);

	gdk_gc_set_line_attributes(GC,12, GDK_LINE_SOLID, GDK_CAP_NOT_LAST,GDK_JOIN_MITER);
	
	gdk_draw_line (widget->window,
                 GC,
								 0, (cell_height-1)/2,
								 widget->allocation.width, (cell_height-1)/2);

	color = CF_Colors[BLUE];
	
	gdk_color_alloc (gdk_colormap_get_system () , &color);
	
	gdk_gc_set_foreground(GC,&color);


		gdk_gc_set_line_attributes(GC,3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST,GDK_JOIN_MITER);
	
	gdk_draw_line (widget->window,
                 GC,
								 0, (cell_height-1)/2,
								 widget->allocation.width,(cell_height-1)/2);
	





	g_object_unref(GC);
	
	//gdk_colormap_alloc_colors(gdk_colormap_get_system(), TRUE, 
		
	//gdk_gc_set_line_attributes(GC,5, GDK_LINE_SOLID, GDK_CAP_NOT_LAST,GDK_JOIN_MITER);
	//gdk_gc_set_foreground(GC, 

	//gdk_draw_line (widget->window,
  //               GC,
	//							 0, (2*cell_height)-2-1,
	//							 50, (2*cell_height)-2-1);

}
#endif //DEBUG

void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	ControlFlowData *Control_Flow_Data = (ControlFlowData*)data;
	GtkTreePath *Tree_Path;

	g_critical("DEBUG : scroll signal, value : %f", adjustment->value);
	
	//get_test_data((int)adjustment->value, Control_Flow_Data->Num_Visible_Events, 
	//								 Control_Flow_Data);
	
	

}






static void destroy_cb( GtkWidget *widget,
		                        gpointer   data )
{ 
	    gtk_main_quit ();
}



/*****************************************************************************
 *                                test routines                              *
 *****************************************************************************/


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
	CF_Viewer = Control_Flow_Data->Scrolled_Window_VC;
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
