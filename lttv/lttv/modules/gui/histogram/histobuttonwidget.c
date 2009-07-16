/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Parisa Heidari
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "histobuttonwidget.h"
#include "histodrawing.h"
#include "histodrawitem.h"

extern void histogram_show(HistoControlFlowData *histocontrol_flow_data,
			   guint draw_begin, guint draw_end);

#ifndef g_info
#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#endif
#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

/* Preallocated Size of the index_to_pixmap array */
#define ALLOCATE_PROCESSES 1000

/*****************************************************************************
 *                                       *
 *****************************************************************************/
static GtkWidget *xpm_label_box( gchar     *xpm_filename,
                                 gchar     *label_text );
static gboolean gplus( GtkWidget *widget,gpointer user_data)
{
  HistoControlFlowData *histo_cfd =  (HistoControlFlowData *)user_data;
  //histo_cfd->vertical_ratio =histo_cfd->vertical_ratio * (1.0/2.0);  
  if(histo_cfd->max_height>1)
  {
  	histo_cfd->max_height /= 2;
   //just redraw.horizontal scale is not changed so Array's data are valid.
  	histogram_show(histo_cfd ,0,histo_cfd->number_of_process->len);
  }
  else
	g_warning("Zoom more than 1 event is impossible");
	
  histo_drawing_update_vertical_ruler(histo_cfd->drawing);//, TimeWindow *time_window);
  return 0;
}

static gboolean gMinus( GtkWidget *widget,
                   gpointer user_data )
{
  HistoControlFlowData *histo_cfd = (HistoControlFlowData *)user_data;
  histo_cfd->max_height *= 2;
 
  //just redraw.horizontal scale is not changed so Array's data are valid.
  histogram_show(histo_cfd ,0,histo_cfd->number_of_process->len);
  histo_drawing_update_vertical_ruler(histo_cfd->drawing);//, TimeWindow *time_window);
  return 0;
}

static gboolean gFit( GtkWidget *widget,
                   gpointer user_data )
{
  /*find the maximum value and put max_height equal with this maximum*/
  HistoControlFlowData *histo_cfd = (HistoControlFlowData *)user_data;
  gint i=1,x;
  guint maximum;
  maximum =g_array_index(histo_cfd->number_of_process,guint,i);
  for (i=1; i < histo_cfd->number_of_process-> len ;i++)
  {
  	x=g_array_index(histo_cfd->number_of_process,guint,i);
	maximum=MAX(x,maximum);
  }
  if (maximum >0)
  {
  	histo_cfd->max_height=maximum;
        histogram_show (histo_cfd,0,histo_cfd->number_of_process->len);
  }
  histo_drawing_update_vertical_ruler(histo_cfd->drawing);
  
  return 0;
}
/* Create a new hbox with an image and a label packed into it
 * and return the box. */

static GtkWidget *xpm_label_box( gchar* xpm_filename,
                                 gchar     *label_text )
{
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *image;

    GdkPixbuf *pixbufP;
    //GError **error;
    /* Create box for image and label */
    box = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 1);

    /* Now on to the image stuff */
        
    pixbufP = gdk_pixbuf_new_from_xpm_data((const char **)&xpm_filename);
    image =  gtk_image_new_from_pixbuf(pixbufP);

    /* Create a label for the button */
    label = gtk_label_new (label_text);

    /* Pack the image and label into the box */
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 1);

    gtk_widget_show (image);
    gtk_widget_show (label);

    return box;
}

ButtonWidget *histo_buttonwidget_construct(HistoControlFlowData *histocontrol_flow_data)
{
  GtkWidget *boxPlus, *boxMinus , *boxfit;//containing text and image for each button.

  ButtonWidget *buttonwidget = g_new(ButtonWidget,1);
  buttonwidget->histo_control_flow_data = histocontrol_flow_data;
  /* Put + and - on the vbox and assign related functions to each button */
  buttonwidget-> vbox1 = gtk_vbox_new (FALSE, 0);
  
// Add 2 buttons on the vbox 
//  buttonwidget ->buttonP = gtk_button_new_with_mnemonic ("+");
//  buttonwidget->buttonM = gtk_button_new_with_mnemonic ("-");
// Instead, add 2 button with image and text:

  buttonwidget ->buttonP =gtk_button_new ();
  buttonwidget ->buttonM =gtk_button_new ();
  buttonwidget ->buttonFit =gtk_button_new ();

/* This calls our box creating function */
  boxPlus = xpm_label_box ("stock_zoom_in_24.xpm", "vertical");
  boxMinus = xpm_label_box ("stock_zoom_out_24.xpm", "vertical");
  boxfit = xpm_label_box ("stock_zoom_fit_24.xpm", "vertical");

/* Pack and show all widgets */
  gtk_widget_show (boxPlus);
  gtk_widget_show (boxMinus);
  gtk_widget_show (boxfit);

  gtk_container_add (GTK_CONTAINER (buttonwidget -> buttonP), boxPlus);
  gtk_container_add (GTK_CONTAINER (buttonwidget -> buttonM), boxMinus);
  gtk_container_add (GTK_CONTAINER (buttonwidget -> buttonFit), boxfit);

  gtk_box_pack_start (GTK_BOX (buttonwidget->vbox1),buttonwidget->buttonP, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonwidget->vbox1),buttonwidget->buttonM, TRUE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (buttonwidget->vbox1),buttonwidget->buttonFit, TRUE, FALSE, 0);

    /* When the button receives the "clicked" signal, it will call the
     * function gplus() passing it NULL as its argument.  The gplus()
     * function is defined above . */

    g_signal_connect (G_OBJECT (buttonwidget ->buttonP), "clicked",
		G_CALLBACK (gplus), (gpointer)histocontrol_flow_data);
    g_signal_connect (G_OBJECT ( buttonwidget->buttonM), "clicked",
		G_CALLBACK (gMinus), (gpointer)histocontrol_flow_data);
    g_signal_connect (G_OBJECT ( buttonwidget->buttonFit), "clicked",
		G_CALLBACK (gFit), (gpointer)histocontrol_flow_data);

  gtk_widget_show (buttonwidget -> vbox1);
  gtk_widget_show (buttonwidget ->buttonP);
  gtk_widget_show (buttonwidget ->buttonM);
  gtk_widget_show (buttonwidget ->buttonFit);

  return buttonwidget;
}

void histo_buttonwidget_destroy(ButtonWidget *buttonwidget)
{
  g_debug("buttonwidget_destroy %p", buttonwidget);
  
  g_free(buttonwidget);
  g_debug("buttonwidget_destroy end");
}

GtkWidget *histo_buttonwidget_get_widget(ButtonWidget *button_widget)
{
  return button_widget->vbox1;
}



void histo_rectangle_pixmap (GdkGC *gc,
    gboolean filled, gint x, gint y, gint width, gint height,
                                  histoDrawing_t *value)
{
  if(height == -1)
    height = value->drawing_area->allocation.height;
  if(width == -1)
    height = value->drawing_area->allocation.width; 
  gdk_draw_rectangle (value->pixmap,
      gc,
      filled,
      x, y,
      width, height);
}

//This could be usefull if a vertical scroll bar is added to viewer:
void histo_copy_pixmap_region(histoDrawing_t *drawing,GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height)
{

  if(dest == NULL)
    dest = drawing->pixmap;
  if(src == NULL)
    src = drawing->pixmap;

  gdk_draw_drawable (dest,gc,src,xsrc, ysrc,
      xdest, ydest,width, height);
}

void histo_update_pixmap_size(histoDrawing_t *value,
                                    guint width)
{
  GdkPixmap *old_pixmap = value->pixmap;

  value->pixmap = 
        gdk_pixmap_new(old_pixmap,
                       width,
                       value->height,
                       -1);

  gdk_pixmap_unref(old_pixmap);
}

