

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
	
	Control_Flow_Data = guicontrolflow();
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
  
	//h_guievents_destructor(ListViewer);

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


/* Event_Hook.c tests */

void test_draw_item(Drawing_t *Drawing,
			GdkPixmap *Pixmap) 
{
	PropertiesIcon properties_icon;
	DrawContext draw_context;
	
	DrawInfo current, previous;
	ItemInfo over, middle, under, modify_over, modify_middle, modify_under;

	int i=0,j=0;
	
	//for(i=0; i<1024;i=i+15)
	{
	//	for(j=0;j<768;j=j+15)
		{
			over.x = i;
			over.y = j;

			current.modify_over = &over;
	
			draw_context.drawable = Pixmap;
			draw_context.gc = Drawing->Drawing_Area_V->style->black_gc;

			draw_context.Current = &current;
			draw_context.Previous = NULL;
	
			properties_icon.icon_name = g_new(char, MAX_PATH_LEN);
			strncpy(properties_icon.icon_name, 
				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/mini-display.xpm",
				MAX_PATH_LEN);
			properties_icon.width = -1;
			properties_icon.height = -1;
			properties_icon.position = OVER;
			draw_icon(&properties_icon, &draw_context);
			g_free(properties_icon.icon_name);
		}
	}

}

#ifdef NOTUSE
/* NOTE : no drawing data should be sent there, since the drawing widget
 * has not been initialized */
void send_test_drawing(ProcessList *Process_List,
			Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			gint x, gint y, // y not used here?
		  gint width,
			gint height) // height won't be used here ?
{
	int i,j;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *FontDesc;// = pango_font_description_new();
	gint Font_Size;

	//icon
	//GdkBitmap *mask = g_new(GdkBitmap, 1);
	//GdkPixmap *icon_pixmap = g_new(GdkPixmap, 1);
	GdkGC * gc;
	// rectangle
	GdkColor color = { 0, 0xffff, 0x0000, 0x0000 };
	
	gc = gdk_gc_new(Pixmap);
	/* Sent text data */
	layout = gtk_widget_create_pango_layout(Drawing->Drawing_Area_V,
			NULL);
	context = pango_layout_get_context(layout);
	FontDesc = pango_context_get_font_description(context);
	Font_Size = pango_font_description_get_size(FontDesc);
	pango_font_description_set_size(FontDesc, Font_Size-3*PANGO_SCALE);
	
	


	LttTime birth;
	birth.tv_sec = 12000;
	birth.tv_nsec = 55500;
	g_info("we have : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	processlist_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	
	g_info("we draw : x : %u, y : %u, width : %u, height : %u", x, y, width, height);
	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	pango_layout_set_text(layout, "Test", -1);
	gdk_draw_layout(Pixmap, Drawing->Drawing_Area_V->style->black_gc,
			0, y+height, layout);

	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	processlist_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_info("y : %u, height : %u", y, height);

	

	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);

	/* Draw rectangle (background color) */
	gdk_gc_copy(gc, Drawing->Drawing_Area_V->style->black_gc);
	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_draw_rectangle(Pixmap, gc,
					TRUE,
					x, y, width, height);

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	
	/* Draw arc */
	gdk_draw_arc(Pixmap, Drawing->Drawing_Area_V->style->black_gc,
							TRUE, 100, y, height/2, height/2, 0, 360*64);

	g_info("y : %u, height : %u", y, height);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		processlist_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		

		drawing_draw_line(
			Drawing, Pixmap, x,
			y+(height/2), x + width, y+(height/2),
			Drawing->Drawing_Area_V->style->black_gc);

		g_critical("y : %u, height : %u", y, height);

	}

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	

	drawing_draw_line(
		Drawing, Pixmap, x,
		y+(height/2), x + width, y+(height/2),
		Drawing->Drawing_Area_V->style->black_gc);

	g_info("y : %u, height : %u", y, height);
	

	/* IMPORTANT : This action uses the cpu heavily! */
	//icon_pixmap = gdk_pixmap_create_from_xpm(Pixmap, &mask, NULL,
//				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/move_message.xpm");
	//				"/home/compudj/local/share/LinuxTraceToolkit/pixmaps/mini-display.xpm");

	//		gdk_gc_set_clip_mask(Drawing->Drawing_Area_V->style->black_gc, mask);

//	for(i=x;i<x+width;i=i+15)
//	{
//		for(j=0;j<height*20;j=j+15)
//		{
			
			/* Draw icon */
			//gdk_gc_copy(gc, Drawing->Drawing_Area_V->style->black_gc);
//			gdk_gc_set_clip_origin(Drawing->Drawing_Area_V->style->black_gc, i, j);
//			gdk_draw_drawable(Pixmap, 
//					Drawing->Drawing_Area_V->style->black_gc,
//					icon_pixmap,
//					0, 0, i, j, -1, -1);

//		}
//	}

	test_draw_item(Drawing,Pixmap);
	
	//gdk_gc_set_clip_origin(Drawing->Drawing_Area_V->style->black_gc, 0, 0);
	//gdk_gc_set_clip_mask(Drawing->Drawing_Area_V->style->black_gc, NULL);

	//g_free(icon_pixmap);
	//g_free(mask);






	pango_font_description_set_size(FontDesc, Font_Size);
	g_object_unref(layout);
	g_free(gc);
}

void send_test_process(ProcessList *Process_List, Drawing_t *Drawing)
{
	guint height, y;
	int i;
	ProcessInfo Process_Info = {10000, 12000, 55600};
	//ProcessInfo Process_Info = {156, 14000, 55500};
	GtkTreeRowReference *got_RowRef;

	LttTime birth;

	if(Process_List->Test_Process_Sent) return;

	birth.tv_sec = 12000;
	birth.tv_nsec = 55500;

	processlist_add(Process_List,
			1,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					1,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 14000;
	birth.tv_nsec = 55500;

	processlist_add(Process_List,
			156,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					156,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	birth.tv_sec = 12000;
	birth.tv_nsec = 55700;

	processlist_add(Process_List,
			10,
			&birth,
			&height);
	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//drawing_insert_square( Drawing, height, 5);

	for(i=0; i<10; i++)
	{
		birth.tv_sec = i*12000;
		birth.tv_nsec = i*55700;

		processlist_add(Process_List,
				i,
				&birth,
				&height);
		processlist_get_process_pixels(Process_List,
						i,
						&birth,
						&y,
						&height);
		drawing_insert_square( Drawing, y, height);
	
	//	g_critical("y : %u, height : %u", y, height);
	
	}
	//g_critical("height : %u", height);

	birth.tv_sec = 12000;
	birth.tv_nsec = 55600;

	processlist_add(Process_List,
			10,
			&birth,
			&y);
	processlist_get_process_pixels(Process_List,
					10,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	processlist_add(Process_List,
			10000,
			&birth,
			&height);
	processlist_get_process_pixels(Process_List,
					10000,
					&birth,
					&y,
					&height);
	drawing_insert_square( Drawing, y, height);
	
	//g_critical("y : %u, height : %u", y, height);
	
	//drawing_insert_square( Drawing, height, 5);
	//g_critical("height : %u", height);


	processlist_get_process_pixels(Process_List,
				10000,
				&birth,
				&y, &height);
	processlist_remove( 	Process_List,
				10000,
				&birth);

	drawing_remove_square( Drawing, y, height);
	
	if(got_RowRef = 
		(GtkTreeRowReference*)g_hash_table_lookup(
					Process_List->Process_Hash,
					&Process_Info))
	{
		g_critical("key found");
		g_critical("position in the list : %s",
			gtk_tree_path_to_string (
			gtk_tree_row_reference_get_path(
				(GtkTreeRowReference*)got_RowRef)
			));
		
	}

	Process_List->Test_Process_Sent = TRUE;

}
#endif//NOTUSE

