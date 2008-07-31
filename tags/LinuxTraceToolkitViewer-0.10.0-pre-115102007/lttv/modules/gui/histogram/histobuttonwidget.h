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

#ifndef _HISTOBUTTONWIDGET_H
#define _HISTOBUTTONWIDGET_H

#include <gtk/gtk.h>
#include <lttv/state.h>
#include <ltt/ltt.h>
#include "histocfv.h"
#include "histodrawitem.h"


/* The ButtonWidget
 *
 * Tasks :
 * Create a widget
 * containing 3 buttons zoomIn,zoonOut and zoomFit to change the vertical scale.
 *
 */
#ifndef TYPE_ButtonWidget_DEFINED
#define TYPE_ButtonWidget_DEFINED
typedef struct _ButtonWidget ButtonWidget;
#endif //TYPE_ButtonWidget_DEFINED

#ifndef TYPE_HistoControlFlowData_DEFINED
#define TYPE_HistoControlFlowData_DEFINED
typedef struct _HistoControlFlowData HistoControlFlowData;
#endif //TYPE_HistoControlFlowData_DEFINED

struct _ButtonWidget {
  
  GtkWidget *buttonP;
  GtkWidget *buttonM;
  GtkWidget *buttonFit;

  GtkWidget *vbox1;//buttons are placed on this vbox 

  GtkWidget *hbox;//Parent Widget containing buttons and drawing area. 
  HistoControlFlowData *histo_control_flow_data;

};


void histo_copy_pixmap_region(histoDrawing_t *drawing,GdkDrawable *dest, 
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height);

void histo_rectangle_pixmap (GdkGC *gc,gboolean filled, gint x, gint y, 
			gint width, gint height,histoDrawing_t *value);

ButtonWidget *histo_buttonwidget_construct(HistoControlFlowData *histocontrol_flow_data);

void histo_buttonwidget_destroy(ButtonWidget *buttonwidget);


static gboolean gplus( GtkWidget *widget,gpointer user_data);//assigned to zoomIn
static gboolean gMinus( GtkWidget *widget,gpointer user_data );//assigned to zoomOut
static gboolean gFit( GtkWidget *widget,gpointer user_data );//assigned to zoomFit

GtkWidget *histo_buttonwidget_get_widget(ButtonWidget *button_widget);
void histo_update_pixmap_size(histoDrawing_t *value,
                                    guint width);
#endif //_HISTOBUTTONWIDGET_H 
