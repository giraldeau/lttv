#include <gtk/gtk.h>

#include <lttv/gtkcustom.h>
//#include "gtkintl.h"
#include <lttv/mainWindow.h>
#include <lttv/gtkTraceSet.h>

static void gtk_custom_class_init (GtkCustomClass    *klass);
static void gtk_custom_init       (GtkCustom         *custom);


static void     gtk_custom_size_request   (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void     gtk_custom_size_allocate  (GtkWidget      *widget,
					   GtkAllocation  *allocation);

void gtk_custom_scroll_value_changed (GtkRange *range, gpointer custom);

GType
gtk_custom_get_type (void)
{
  static GType custom_type = 0;

  if (!custom_type)
    {
      static const GTypeInfo custom_info =
      {
	sizeof (GtkCustomClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_custom_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkCustom),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_custom_init,
	NULL,		/* value_table */
      };

      custom_type = g_type_register_static (GTK_TYPE_PANED, "GtkCustom", 
					 &custom_info, 0);
    }

  return custom_type;
}

static void
gtk_custom_class_init (GtkCustomClass *class)
{
  GtkWidgetClass *widget_class;
  
  widget_class = (GtkWidgetClass *) class;

  widget_class->size_request = gtk_custom_size_request;
  widget_class->size_allocate = gtk_custom_size_allocate;
}

static void
gtk_custom_init (GtkCustom *custom)
{
  GtkWidget * button;

  GTK_WIDGET_SET_FLAGS (custom, GTK_NO_WINDOW);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (custom), FALSE);
  
  custom->firstPane    = NULL;
  custom->lastPane     = NULL;
  custom->focusedPane  = NULL;
  custom->numChildren  = 0;

  custom->vbox         = NULL;
  //  custom->scrollWindow = NULL;
  //  custom->viewport     = NULL;
  custom->hScrollbar   = NULL;
}


GtkWidget* gtk_custom_new ()
{
  return GTK_WIDGET (g_object_new (gtk_custom_get_type (), NULL));
}


void gtk_custom_set_focus (GtkWidget * widget, gpointer user_data)
{
  GtkCustom * custom = (GtkCustom*) widget;
  GtkPaned * pane;
  if(!custom->firstPane) return;
  

  pane = custom->firstPane;
  while(1){
    if((GtkWidget*)pane == (GtkWidget*)user_data){
      custom->focusedPane = pane;
      break;
    }
    if(pane == custom->lastPane){
      custom->focusedPane = NULL;
      break;
    }
    pane = (GtkPaned*)pane->child1;
  }
}

void gtk_custom_widget_add(GtkCustom * custom, GtkWidget * widget1)
{
  GtkPaned * tmpPane; 
  GtkWidget * w;
  TimeWindow Time_Window;
  LttTime      time;
  TimeInterval *Time_Span;
  
  g_return_if_fail(GTK_IS_CUSTOM(custom));
  g_object_ref(G_OBJECT(widget1));
 

  if(!custom->firstPane){
    custom->firstPane = (GtkPaned *)gtk_vpaned_new();
    custom->lastPane = custom->firstPane;

    custom->hScrollbar = gtk_hscrollbar_new (NULL);
    gtk_widget_show(custom->hScrollbar);

    custom->hAdjust = gtk_range_get_adjustment(GTK_RANGE(custom->hScrollbar));
    GetTimeWindow(custom->mw,&Time_Window);
    GetCurrentTime(custom->mw,&time);
    Time_Span = LTTV_TRACESET_CONTEXT(custom->mw->Traceset_Info->TracesetContext)->Time_Span ;

    custom->hAdjust->lower =  ltt_time_to_double(Time_Span->startTime) * 
        NANOSECONDS_PER_SECOND;
    custom->hAdjust->value = custom->hAdjust->lower;
    custom->hAdjust->upper = ltt_time_to_double(Time_Span->endTime) *
        NANOSECONDS_PER_SECOND;
    /* Page increment of whole visible area */
    custom->hAdjust->page_increment = ltt_time_to_double(
        Time_Window.Time_Width) * NANOSECONDS_PER_SECOND;
    /* page_size to the whole visible area will take care that the
     * scroll value + the shown area will never be more than what is
     * in the trace. */
    custom->hAdjust->page_size = custom->hAdjust->page_increment;
    custom->hAdjust->step_increment = custom->hAdjust->page_increment / 10;

    gtk_range_set_update_policy (GTK_RANGE(custom->hScrollbar),
                                  GTK_UPDATE_DISCONTINUOUS);
    g_signal_connect(G_OBJECT(custom->hScrollbar), "value-changed",
		     G_CALLBACK(gtk_custom_scroll_value_changed), custom);

    custom->vbox = gtk_vbox_new(FALSE,0);
    gtk_widget_show(custom->vbox);
    
    //    custom->viewport = gtk_viewport_new (NULL,NULL);
    //    gtk_widget_show(custom->viewport);
    
    //    gtk_container_add(GTK_CONTAINER(custom->viewport), (GtkWidget*)custom->vbox);
    gtk_box_pack_end(GTK_BOX(custom->vbox),(GtkWidget*)custom->hScrollbar,FALSE,FALSE,0);
    gtk_box_pack_end(GTK_BOX(custom->vbox),(GtkWidget*)custom->firstPane,TRUE,TRUE,0);
    
    //    custom->scrollWindow = gtk_scrolled_window_new (NULL, NULL);
    //    gtk_widget_show(custom->scrollWindow);
    //    gtk_container_add (GTK_CONTAINER (custom->scrollWindow), (GtkWidget*)custom->viewport);    
    //    gtk_paned_pack1(GTK_PANED(custom), (GtkWidget*)custom->scrollWindow,FALSE, TRUE);

    gtk_paned_pack1(GTK_PANED(custom), (GtkWidget*)custom->vbox,FALSE, TRUE);
  }else{
    tmpPane = custom->lastPane;
    custom->lastPane = (GtkPaned *)gtk_vpaned_new();
    gtk_paned_pack1 (tmpPane,(GtkWidget*)custom->lastPane, FALSE,TRUE);
  }
  gtk_widget_show((GtkWidget *)custom->lastPane);  

  gtk_paned_pack2 (custom->lastPane,widget1, TRUE, TRUE);      
  custom->focusedPane = custom->lastPane;
  custom->numChildren++;
  
}

void gtk_custom_widget_delete(GtkCustom * custom)
{
  GtkPaned * tmp, *prev, *next;

  if(!custom->focusedPane) return;
 
  tmp = (GtkPaned*)custom->focusedPane->child2; //widget in vpaned
  g_object_unref(G_OBJECT(tmp));

  if(custom->focusedPane == custom->firstPane &&
     custom->focusedPane == custom->lastPane){
    //    gtk_container_remove(GTK_CONTAINER(custom),(GtkWidget*)custom->scrollWindow);
    gtk_container_remove(GTK_CONTAINER(custom),(GtkWidget*)custom->vbox);
    custom->firstPane = NULL;
    custom->lastPane = NULL;
    custom->focusedPane = NULL;
  }else if(custom->focusedPane == custom->firstPane &&
	   custom->focusedPane != custom->lastPane){
    next = (GtkPaned*)custom->firstPane->child1;
    g_object_ref(G_OBJECT(next));
    gtk_container_remove(GTK_CONTAINER(custom->firstPane),(GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(custom->vbox),(GtkWidget*)custom->firstPane);
    custom->firstPane = next;
    gtk_box_pack_end(GTK_BOX(custom->vbox),(GtkWidget*)custom->firstPane,TRUE,TRUE,0);
    custom->focusedPane = custom->firstPane;
    g_object_unref(G_OBJECT(next));
  }else if(custom->focusedPane != custom->firstPane &&
	   custom->focusedPane == custom->lastPane){
    tmp = custom->lastPane;
    custom->lastPane = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)custom->lastPane);
    custom->focusedPane = custom->lastPane;
    gtk_container_remove(GTK_CONTAINER(custom->lastPane),(GtkWidget*)tmp);
  }else{
    tmp = custom->focusedPane;
    prev = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)tmp);
    next = (GtkPaned*)tmp->child1;
    g_object_ref(G_OBJECT(next));
    gtk_container_remove(GTK_CONTAINER(custom->focusedPane),(GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(prev),(GtkWidget*)custom->focusedPane);
    gtk_paned_pack1(prev, (GtkWidget*)next, FALSE, TRUE);
    custom->focusedPane = next;
    g_object_unref(G_OBJECT(next));
  }

  custom->numChildren--;
}


void gtk_custom_widget_move_up(GtkCustom * custom)
{
  GtkWidget* upWidget, *downWidget;
  GtkPaned * prev,*next, *prevPrev;

  if(custom->lastPane == custom->focusedPane) return;

  // move VPane
  prev = (GtkPaned*)custom->focusedPane->child1;
  g_object_ref(G_OBJECT(prev));
  gtk_container_remove(GTK_CONTAINER(custom->focusedPane),(GtkWidget*)prev);

  if(prev == custom->lastPane){
    prevPrev = NULL;
    custom->lastPane = custom->focusedPane;
  }else{    
    prevPrev = (GtkPaned*)prev->child1;
    g_object_ref(G_OBJECT(prevPrev));
    gtk_container_remove(GTK_CONTAINER(prev),(GtkWidget*)prevPrev);   
  }

  g_object_ref(G_OBJECT(custom->focusedPane));
  if(custom->firstPane == custom->focusedPane){
    gtk_container_remove(GTK_CONTAINER(custom->vbox),(GtkWidget*)custom->focusedPane);
    gtk_box_pack_end(GTK_BOX(custom->vbox),(GtkWidget*)prev,TRUE,TRUE,0);
    custom->firstPane = prev;
  }else{
    next = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)custom->focusedPane);
    gtk_container_remove(GTK_CONTAINER(next),(GtkWidget*)custom->focusedPane);
    gtk_paned_pack1(GTK_PANED(next), (GtkWidget*)prev, FALSE,TRUE);
  }
  gtk_paned_pack1(GTK_PANED(prev),(GtkWidget*)custom->focusedPane, FALSE,TRUE);
  if(prevPrev)
    gtk_paned_pack1(GTK_PANED(custom->focusedPane),(GtkWidget*)prevPrev, FALSE,TRUE);

  g_object_unref(G_OBJECT(prev));
  if(prevPrev) g_object_unref(G_OBJECT(prevPrev));
  g_object_unref(G_OBJECT(custom->focusedPane));
}


void gtk_custom_widget_move_down(GtkCustom * custom)
{
  GtkWidget* upWidget, *downWidget;
  GtkPaned * prev,*next, *nextNext;

  if(custom->firstPane == custom->focusedPane) return;

  //move VPane
  next = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)custom->focusedPane);
  g_object_ref(G_OBJECT(next));

  if(custom->lastPane == custom->focusedPane){
    prev = NULL;
    custom->lastPane = next;
  }else{
    prev = (GtkPaned*)custom->focusedPane->child1;
    g_object_ref(G_OBJECT(prev));
    gtk_container_remove(GTK_CONTAINER(custom->focusedPane),(GtkWidget*)prev);    
  }

  g_object_ref(G_OBJECT(custom->focusedPane));
  gtk_container_remove(GTK_CONTAINER(next),(GtkWidget*)custom->focusedPane);
  
  if(next == custom->firstPane){
    custom->firstPane = custom->focusedPane;
    gtk_container_remove(GTK_CONTAINER(custom->vbox),(GtkWidget*)next);       
    gtk_box_pack_end(GTK_BOX(custom->vbox),(GtkWidget*)custom->focusedPane,TRUE,TRUE,0);
  }else{    
    nextNext = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(nextNext),(GtkWidget*)next);       
    gtk_paned_pack1(nextNext, (GtkWidget*)custom->focusedPane, FALSE, TRUE);
  }
  gtk_paned_pack1(custom->focusedPane,(GtkWidget*)next, FALSE,TRUE);
  if(prev)
    gtk_paned_pack1(next,(GtkWidget*)prev, FALSE,TRUE);

  if(prev)g_object_unref(G_OBJECT(prev));
  g_object_unref(G_OBJECT(next));
  g_object_unref(G_OBJECT(custom->focusedPane));
}

void gtk_custom_scroll_value_changed(GtkRange *range, gpointer custom_arg)
{
  LttTime time;
  GtkCustom * custom = (GtkCustom*)custom_arg;
  gdouble value = gtk_range_get_value(range);
  time = ltt_time_from_double(value / NANOSECONDS_PER_SECOND);
  SetCurrentTime(custom->mw, &time);
  g_warning("The current time is second :%d, nanosecond : %d\n", time.tv_sec, time.tv_nsec);
}


static void
gtk_custom_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkPaned *paned = GTK_PANED (widget);
  GtkRequisition child_requisition;

  requisition->width = 0;
  requisition->height = 0;

  if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
    {
      gtk_widget_size_request (paned->child1, &child_requisition);

      requisition->height = child_requisition.height;
      requisition->width = child_requisition.width;
    }

  if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
    {
      gtk_widget_size_request (paned->child2, &child_requisition);

      requisition->width = MAX (requisition->width, child_requisition.width);
      requisition->height += child_requisition.height;
    }

  requisition->height += GTK_CONTAINER (paned)->border_width * 2;
  requisition->width += GTK_CONTAINER (paned)->border_width * 2;
  
  if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1) &&
      paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
    {
      gint handle_size;

      gtk_widget_style_get (widget, "handle_size", &handle_size, NULL);
      requisition->height += handle_size;
    }

}

static void
gtk_custom_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkPaned *paned = GTK_PANED (widget);
  gint border_width = GTK_CONTAINER (paned)->border_width;

  widget->allocation = *allocation;

  if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1) &&
      paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
    {
      GtkRequisition child1_requisition;
      GtkRequisition child2_requisition;
      GtkAllocation child1_allocation;
      GtkAllocation child2_allocation;
      gint handle_size;
      
      gtk_widget_style_get (widget, "handle_size", &handle_size, NULL);

      gtk_widget_get_child_requisition (paned->child1, &child1_requisition);
      gtk_widget_get_child_requisition (paned->child2, &child2_requisition);
    
      gtk_paned_compute_position (paned,
				  MAX (1, widget->allocation.height
				       - handle_size
				       - 2 * border_width),
				  child1_requisition.height,
				  child2_requisition.height);

      paned->handle_pos.x = widget->allocation.x + border_width;
      paned->handle_pos.y = widget->allocation.y + paned->child1_size + border_width;
      paned->handle_pos.width = MAX (1, (gint) widget->allocation.width - 2 * border_width);
      paned->handle_pos.height = handle_size;
      
      if (GTK_WIDGET_REALIZED (widget))
	{
	  if (GTK_WIDGET_MAPPED (widget))
	    gdk_window_show (paned->handle);
	  gdk_window_move_resize (paned->handle,
				  paned->handle_pos.x,
				  paned->handle_pos.y,
				  paned->handle_pos.width,
				  handle_size);
	}

      child1_allocation.width = child2_allocation.width = MAX (1, (gint) allocation->width - border_width * 2);
      child1_allocation.height = MAX (1, paned->child1_size);
      child1_allocation.x = child2_allocation.x = widget->allocation.x + border_width;
      child1_allocation.y = widget->allocation.y + border_width;
      
      child2_allocation.y = child1_allocation.y + paned->child1_size + paned->handle_pos.height;
      child2_allocation.height = MAX (1, widget->allocation.y + widget->allocation.height - child2_allocation.y - border_width);
      
      if (GTK_WIDGET_MAPPED (widget) &&
	  paned->child1->allocation.height < child1_allocation.height)
	{
	  gtk_widget_size_allocate (paned->child2, &child2_allocation);
	  gtk_widget_size_allocate (paned->child1, &child1_allocation);
	}
      else
	{
	  gtk_widget_size_allocate (paned->child1, &child1_allocation);
	  gtk_widget_size_allocate (paned->child2, &child2_allocation);
	}
    }
  else
    {
      GtkAllocation child_allocation;

      if (GTK_WIDGET_REALIZED (widget))      
	gdk_window_hide (paned->handle);

      if (paned->child1)
	gtk_widget_set_child_visible (paned->child1, TRUE);
      if (paned->child2)
	gtk_widget_set_child_visible (paned->child2, TRUE);

      child_allocation.x = widget->allocation.x + border_width;
      child_allocation.y = widget->allocation.y + border_width;
      child_allocation.width = MAX (1, allocation->width - 2 * border_width);
      child_allocation.height = MAX (1, allocation->height - 2 * border_width);
      
      if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
	gtk_widget_size_allocate (paned->child1, &child_allocation);
      else if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
	gtk_widget_size_allocate (paned->child2, &child_allocation);
    }
}

