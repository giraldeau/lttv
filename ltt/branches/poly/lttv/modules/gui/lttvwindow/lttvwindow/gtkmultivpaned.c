/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#include <gtk/gtk.h>

#include <lttvwindow/gtkmultivpaned.h>
//#include "gtkintl.h"
#include <lttvwindow/mainwindow.h>
#include <lttvwindow/viewer.h>

static void gtk_multi_vpaned_class_init (GtkMultiVPanedClass    *klass);
static void gtk_multi_vpaned_init       (GtkMultiVPaned         *multi_vpaned);


static void     gtk_multi_vpaned_size_request   (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void     gtk_multi_vpaned_size_allocate  (GtkWidget      *widget,
					   GtkAllocation  *allocation);

void gtk_multi_vpaned_scroll_value_changed (GtkAdjustment *adjust, gpointer multi_vpaned);

gboolean gtk_multi_vpaned_destroy(GtkObject       *object,
                                  gpointer           user_data)
{
  GtkMultiVPaned * multi_vpaned = (GtkMultiVPaned * )object;
  while(multi_vpaned->num_children){
    gtk_multi_vpaned_widget_delete(multi_vpaned);
  }    
  return FALSE;
}

GType
gtk_multi_vpaned_get_type (void)
{
  static GType multi_vpaned_type = 0;

  if (!multi_vpaned_type)
    {
      static const GTypeInfo multi_vpaned_info =
      {
	sizeof (GtkMultiVPanedClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_multi_vpaned_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkMultiVPaned),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_multi_vpaned_init,
	NULL,		/* value_table */
      };

      multi_vpaned_type = g_type_register_static (GTK_TYPE_PANED, "GtkMultiVPaned", 
					 &multi_vpaned_info, 0);
    }

  return multi_vpaned_type;
}

static void
gtk_multi_vpaned_class_init (GtkMultiVPanedClass *class)
{
  GtkWidgetClass *widget_class;
  
  widget_class = (GtkWidgetClass *) class;

  widget_class->size_request = gtk_multi_vpaned_size_request;
  widget_class->size_allocate = gtk_multi_vpaned_size_allocate;
}

static void
gtk_multi_vpaned_init (GtkMultiVPaned * multi_vpaned)
{
  GtkWidget * button;

  GTK_WIDGET_SET_FLAGS (multi_vpaned, GTK_NO_WINDOW);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (multi_vpaned), FALSE);
  
  multi_vpaned->first_pane    = NULL;
  multi_vpaned->last_pane     = NULL;
  multi_vpaned->focused_pane  = NULL;
  multi_vpaned->iter          = NULL;
  multi_vpaned->num_children  = 0;

  multi_vpaned->vbox         = NULL;
  //  multi_vpaned->scrollWindow = NULL;
  //  multi_vpaned->viewport     = NULL;
  multi_vpaned->hscrollbar   = NULL;
}


GtkWidget* gtk_multi_vpaned_new ()
{
  GtkWidget * widget = GTK_WIDGET (g_object_new (gtk_multi_vpaned_get_type (), NULL));
  g_signal_connect(G_OBJECT(widget), "destroy",
		   G_CALLBACK(gtk_multi_vpaned_destroy),NULL);
  
  return widget;
}

GtkWidget * gtk_multi_vpaned_get_widget(GtkMultiVPaned * multi_vpaned)
{
  if(multi_vpaned->focused_pane == NULL)return NULL;
  return (GtkWidget*)multi_vpaned->focused_pane->child2;
}

GtkWidget * gtk_multi_vpaned_get_first_widget(GtkMultiVPaned * multi_vpaned)
{
  if(multi_vpaned->first_pane == NULL)return NULL;
  multi_vpaned->iter = multi_vpaned->first_pane;
  return multi_vpaned->first_pane->child2;
}

GtkWidget * gtk_multi_vpaned_get_next_widget(GtkMultiVPaned * multi_vpaned)
{
  if(multi_vpaned->iter != multi_vpaned->last_pane){
    multi_vpaned->iter = (GtkPaned *)multi_vpaned->iter->child1;
    return multi_vpaned->iter->child2;
  }else {
    return NULL;
  }
}

void gtk_multi_vpaned_set_data(GtkMultiVPaned * multi_vpaned,char * key, gpointer value)
{
  g_object_set_data(G_OBJECT(multi_vpaned->focused_pane), key, value);    
}

gpointer gtk_multi_vpaned_get_data(GtkMultiVPaned * multi_vpaned,char * key)
{
  if(multi_vpaned->focused_pane == NULL)return NULL;
  return g_object_get_data(G_OBJECT(multi_vpaned->focused_pane), key);
}

void gtk_multi_vpaned_set_focus (GtkWidget * widget, GtkPaned* paned)
{
  GtkMultiVPaned * multi_vpaned = GTK_MULTI_VPANED(widget);
  GtkPaned * pane;
  if(!multi_vpaned->first_pane) return;
  

  pane = multi_vpaned->first_pane;
  while(1){
    if((GtkWidget*)pane == (GtkWidget*)user_data){
      multi_vpaned->focused_pane = pane;
      break;
    }
    if(pane == multi_vpaned->last_pane){
      multi_vpaned->focused_pane = NULL;
      break;
    }
    pane = (GtkPaned*)pane->child1;
  }
}

void gtk_multi_vpaned_set_adjust(GtkMultiVPaned * multi_vpaned, const TimeWindow *time_window, gboolean first_time)
{
  //TimeWindow time_window = multi_vpaned->mw->current_tab->time_window;
  TimeInterval *time_span;
  double len, start;

  
  if(first_time){
    time_span = LTTV_TRACESET_CONTEXT(multi_vpaned->mw->current_tab->traceset_info->
				      traceset_context)->Time_Span ;
  
    multi_vpaned->hadjust->lower = ltt_time_to_double(time_span->startTime) * 
                             NANOSECONDS_PER_SECOND;
    multi_vpaned->hadjust->value = multi_vpaned->hadjust->lower;
    multi_vpaned->hadjust->upper = ltt_time_to_double(time_span->endTime) *
                             NANOSECONDS_PER_SECOND;
  }

  /* Page increment of whole visible area */
  if(multi_vpaned->hadjust == NULL){
    g_warning("Insert a viewer first");
    return;
  }

  start = ltt_time_to_double(time_window->start_time) * NANOSECONDS_PER_SECOND;
  len = multi_vpaned->hadjust->upper - multi_vpaned->hadjust->lower;

  multi_vpaned->hadjust->page_increment = ltt_time_to_double(
		        time_window->time_width) * NANOSECONDS_PER_SECOND;

  //if(multi_vpaned->hadjust->page_increment >= len )
  //  multi_vpaned->hadjust->value = multi_vpaned->hadjust->lower;
  //if(start + multi_vpaned->hadjust->page_increment >= multi_vpaned->hadjust->upper )
  //  multi_vpaned->hadjust->value = start;
  multi_vpaned->hadjust->value = start;

  /* page_size to the whole visible area will take care that the
     * scroll value + the shown area will never be more than what is
     * in the trace. */
  multi_vpaned->hadjust->page_size = multi_vpaned->hadjust->page_increment;
  multi_vpaned->hadjust->step_increment = multi_vpaned->hadjust->page_increment / 10;

  gtk_adjustment_changed (multi_vpaned->hadjust);

}

void gtk_multi_vpaned_widget_add(GtkMultiVPaned * multi_vpaned, GtkWidget * widget1)
{
  GtkPaned * tmpPane; 
  GtkWidget * w;
  
  g_return_if_fail(GTK_IS_MULTI_VPANED(multi_vpaned));
  g_object_ref(G_OBJECT(widget1));
 

  if(!multi_vpaned->first_pane){
    multi_vpaned->first_pane = (GtkPaned *)gtk_vpaned_new();
    multi_vpaned->last_pane = multi_vpaned->first_pane;

    multi_vpaned->hscrollbar = gtk_hscrollbar_new (NULL);
    gtk_widget_show(multi_vpaned->hscrollbar);

    multi_vpaned->hadjust = gtk_range_get_adjustment(GTK_RANGE(multi_vpaned->hscrollbar));
    gtk_multi_vpaned_set_adjust(multi_vpaned, &multi_vpaned->mw->current_tab->time_window, TRUE);

    gtk_range_set_update_policy (GTK_RANGE(multi_vpaned->hscrollbar),
																 GTK_UPDATE_CONTINUOUS);
																 //changed by Mathieu Desnoyers, was :
                                 // GTK_UPDATE_DISCONTINUOUS);
    g_signal_connect(G_OBJECT(multi_vpaned->hadjust), "value-changed",
		     G_CALLBACK(gtk_multi_vpaned_scroll_value_changed), multi_vpaned);
    g_signal_connect(G_OBJECT(multi_vpaned->hadjust), "changed",
		     G_CALLBACK(gtk_multi_vpaned_scroll_value_changed), multi_vpaned);


    multi_vpaned->vbox = gtk_vbox_new(FALSE,0);
    gtk_widget_show(multi_vpaned->vbox);
    
    //    multi_vpaned->viewport = gtk_viewport_new (NULL,NULL);
    //    gtk_widget_show(multi_vpaned->viewport);
    
    //    gtk_container_add(GTK_CONTAINER(multi_vpaned->viewport), (GtkWidget*)multi_vpaned->vbox);
    gtk_box_pack_end(GTK_BOX(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->hscrollbar,FALSE,FALSE,0);
    gtk_box_pack_end(GTK_BOX(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->first_pane,TRUE,TRUE,0);
    
    //    multi_vpaned->scrollWindow = gtk_scrolled_window_new (NULL, NULL);
    //    gtk_widget_show(multi_vpaned->scrollWindow);
    //    gtk_container_add (GTK_CONTAINER (multi_vpaned->scrollWindow), (GtkWidget*)multi_vpaned->viewport);    
    //    gtk_paned_pack1(GTK_PANED(multi_vpaned), (GtkWidget*)multi_vpaned->scrollWindow,FALSE, TRUE);

    gtk_paned_pack1(GTK_PANED(multi_vpaned), (GtkWidget*)multi_vpaned->vbox,FALSE, TRUE);
  }else{
    tmpPane = multi_vpaned->last_pane;
    multi_vpaned->last_pane = (GtkPaned *)gtk_vpaned_new();
    gtk_paned_pack1 (tmpPane,(GtkWidget*)multi_vpaned->last_pane, FALSE,TRUE);
  }
  gtk_widget_show((GtkWidget *)multi_vpaned->last_pane);  

  gtk_paned_pack2 (multi_vpaned->last_pane,widget1, TRUE, TRUE);      
  multi_vpaned->focused_pane = multi_vpaned->last_pane;
  multi_vpaned->num_children++;
  
}

void gtk_multi_vpaned_widget_delete(GtkMultiVPaned * multi_vpaned)
{
  GtkPaned * tmp, *prev, *next;

  if(!multi_vpaned->focused_pane) return;
 
  tmp = (GtkPaned*)multi_vpaned->focused_pane->child2; //widget in vpaned
  g_object_unref(G_OBJECT(tmp));

  if(multi_vpaned->focused_pane == multi_vpaned->first_pane &&
     multi_vpaned->focused_pane == multi_vpaned->last_pane){
    //    gtk_container_remove(GTK_CONTAINER(multi_vpaned),(GtkWidget*)multi_vpaned->scrollWindow);
    gtk_container_remove(GTK_CONTAINER(multi_vpaned),(GtkWidget*)multi_vpaned->vbox);
    multi_vpaned->first_pane = NULL;
    multi_vpaned->last_pane = NULL;
    multi_vpaned->focused_pane = NULL;
  }else if(multi_vpaned->focused_pane == multi_vpaned->first_pane &&
	   multi_vpaned->focused_pane != multi_vpaned->last_pane){
    next = (GtkPaned*)multi_vpaned->first_pane->child1;
    g_object_ref(G_OBJECT(next));
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->first_pane),(GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->first_pane);
    multi_vpaned->first_pane = next;
    gtk_box_pack_end(GTK_BOX(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->first_pane,TRUE,TRUE,0);
    multi_vpaned->focused_pane = multi_vpaned->first_pane;
    g_object_unref(G_OBJECT(next));
  }else if(multi_vpaned->focused_pane != multi_vpaned->first_pane &&
	   multi_vpaned->focused_pane == multi_vpaned->last_pane){
    tmp = multi_vpaned->last_pane;
    multi_vpaned->last_pane = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)multi_vpaned->last_pane);
    multi_vpaned->focused_pane = multi_vpaned->last_pane;
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->last_pane),(GtkWidget*)tmp);
  }else{
    tmp = multi_vpaned->focused_pane;
    prev = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)tmp);
    next = (GtkPaned*)tmp->child1;
    g_object_ref(G_OBJECT(next));
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->focused_pane),(GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(prev),(GtkWidget*)multi_vpaned->focused_pane);
    gtk_paned_pack1(prev, (GtkWidget*)next, FALSE, TRUE);
    multi_vpaned->focused_pane = next;
    g_object_unref(G_OBJECT(next));
  }

  multi_vpaned->num_children--;
}


void gtk_multi_vpaned_widget_move_up(GtkMultiVPaned * multi_vpaned)
{
  GtkWidget* upWidget, *downWidget;
  GtkPaned * prev,*next, *prevPrev;

  if(multi_vpaned->last_pane == multi_vpaned->focused_pane) return;

  // move VPane
  prev = (GtkPaned*)multi_vpaned->focused_pane->child1;
  g_object_ref(G_OBJECT(prev));
  gtk_container_remove(GTK_CONTAINER(multi_vpaned->focused_pane),(GtkWidget*)prev);

  if(prev == multi_vpaned->last_pane){
    prevPrev = NULL;
    multi_vpaned->last_pane = multi_vpaned->focused_pane;
  }else{    
    prevPrev = (GtkPaned*)prev->child1;
    g_object_ref(G_OBJECT(prevPrev));
    gtk_container_remove(GTK_CONTAINER(prev),(GtkWidget*)prevPrev);   
  }

  g_object_ref(G_OBJECT(multi_vpaned->focused_pane));
  if(multi_vpaned->first_pane == multi_vpaned->focused_pane){
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->focused_pane);
    gtk_box_pack_end(GTK_BOX(multi_vpaned->vbox),(GtkWidget*)prev,TRUE,TRUE,0);
    multi_vpaned->first_pane = prev;
  }else{
    next = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)multi_vpaned->focused_pane);
    gtk_container_remove(GTK_CONTAINER(next),(GtkWidget*)multi_vpaned->focused_pane);
    gtk_paned_pack1(GTK_PANED(next), (GtkWidget*)prev, FALSE,TRUE);
  }
  gtk_paned_pack1(GTK_PANED(prev),(GtkWidget*)multi_vpaned->focused_pane, FALSE,TRUE);
  if(prevPrev)
    gtk_paned_pack1(GTK_PANED(multi_vpaned->focused_pane),(GtkWidget*)prevPrev, FALSE,TRUE);

  g_object_unref(G_OBJECT(prev));
  if(prevPrev) g_object_unref(G_OBJECT(prevPrev));
  g_object_unref(G_OBJECT(multi_vpaned->focused_pane));
}


void gtk_multi_vpaned_widget_move_down(GtkMultiVPaned * multi_vpaned)
{
  GtkWidget* upWidget, *downWidget;
  GtkPaned * prev,*next, *nextNext;

  if(multi_vpaned->first_pane == multi_vpaned->focused_pane) return;

  //move VPane
  next = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)multi_vpaned->focused_pane);
  g_object_ref(G_OBJECT(next));

  if(multi_vpaned->last_pane == multi_vpaned->focused_pane){
    prev = NULL;
    multi_vpaned->last_pane = next;
  }else{
    prev = (GtkPaned*)multi_vpaned->focused_pane->child1;
    g_object_ref(G_OBJECT(prev));
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->focused_pane),(GtkWidget*)prev);    
  }

  g_object_ref(G_OBJECT(multi_vpaned->focused_pane));
  gtk_container_remove(GTK_CONTAINER(next),(GtkWidget*)multi_vpaned->focused_pane);
  
  if(next == multi_vpaned->first_pane){
    multi_vpaned->first_pane = multi_vpaned->focused_pane;
    gtk_container_remove(GTK_CONTAINER(multi_vpaned->vbox),(GtkWidget*)next);       
    gtk_box_pack_end(GTK_BOX(multi_vpaned->vbox),(GtkWidget*)multi_vpaned->focused_pane,TRUE,TRUE,0);
  }else{    
    nextNext = (GtkPaned*)gtk_widget_get_parent((GtkWidget*)next);
    gtk_container_remove(GTK_CONTAINER(nextNext),(GtkWidget*)next);       
    gtk_paned_pack1(nextNext, (GtkWidget*)multi_vpaned->focused_pane, FALSE, TRUE);
  }
  gtk_paned_pack1(multi_vpaned->focused_pane,(GtkWidget*)next, FALSE,TRUE);
  if(prev)
    gtk_paned_pack1(next,(GtkWidget*)prev, FALSE,TRUE);

  if(prev)g_object_unref(G_OBJECT(prev));
  g_object_unref(G_OBJECT(next));
  g_object_unref(G_OBJECT(multi_vpaned->focused_pane));
}

void gtk_multi_vpaned_set_scroll_value(GtkMultiVPaned * multi_vpaned, double value)
{
  gtk_adjustment_set_value(multi_vpaned->hadjust, value);
  //g_signal_stop_emission_by_name(G_OBJECT(multi_vpaned->hscrollbar), "value-changed");  
}

void gtk_multi_vpaned_scroll_value_changed(GtkAdjustment *adjust, gpointer multi_vpaned_arg)
{
  TimeWindow time_window;
  TimeInterval *time_span;
  LttTime time;
  GtkMultiVPaned * multi_vpaned = (GtkMultiVPaned*)multi_vpaned_arg;
  gdouble value = gtk_adjustment_get_value(adjust);
  gdouble upper, lower, ratio;

  time_window = multi_vpaned->mw->current_tab->time_window;

  time_span = LTTV_TRACESET_CONTEXT(multi_vpaned->mw->current_tab->traceset_info->
				    traceset_context)->Time_Span ;
  lower = multi_vpaned->hadjust->lower;
  upper = multi_vpaned->hadjust->upper;
  ratio = (value - lower) / (upper - lower);
  
  time = ltt_time_sub(time_span->endTime, time_span->startTime);
  time = ltt_time_mul(time, (float)ratio);
  time = ltt_time_add(time_span->startTime, time);

  time_window.start_time = time;

  time = ltt_time_sub(time_span->endTime, time);
  if(ltt_time_compare(time,time_window.time_width) < 0){
    time_window.time_width = time;
  }
  set_time_window(multi_vpaned->mw, &time_window);
  // done in expose now call_pending_read_hooks(multi_vpaned->mw);
}


static void
gtk_multi_vpaned_size_request (GtkWidget      *widget,
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
gtk_multi_vpaned_size_allocate (GtkWidget     *widget,
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

