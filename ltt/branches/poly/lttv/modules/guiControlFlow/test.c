

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



void add_test_process(ControlFlowData *Control_Flow_Data)
{
	GtkTreeIter iter;
	int i;
	gchar *process[] = { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };

	for(i=0; i<Control_Flow_Data->Number_Of_Process; i++)
	{
	  /* Add a new row to the model */
		gtk_list_store_append (Control_Flow_Data->Store_M, &iter);
		gtk_list_store_set (	Control_Flow_Data->Store_M, &iter,
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

