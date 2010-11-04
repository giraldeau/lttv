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
#include "cpucfv.h"
#include "cpudrawitem.h"


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

#ifndef TYPE_CpuControlFlowData_DEFINED
#define TYPE_CpuControlFlowData_DEFINED
typedef struct _CpuControlFlowData CpuControlFlowData;
#endif //TYPE_CpuControlFlowData_DEFINED

struct _ButtonWidget {
  
  GtkWidget *buttonP;
  GtkWidget *buttonM;
  GtkWidget *buttonFit;

  GtkWidget *vbox1;//buttons are placed on this vbox 

  GtkWidget *hbox;//Parent Widget containing buttons and drawing area. 
  CpuControlFlowData *cpu_control_flow_data;

};


void cpu_copy_pixmap_region(cpuDrawing_t *drawing,GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height);

void cpu_rectangle_pixmap (GdkGC *gc,gboolean filled, gint x, gint y,
			gint width, gint height,cpuDrawing_t *value);

ButtonWidget *cpu_buttonwidget_construct(CpuControlFlowData *cpucontrol_flow_data);

void cpu_buttonwidget_destroy(ButtonWidget *buttonwidget);


static gboolean gplus( GtkWidget *widget,gpointer user_data);//assigned to zoomIn
static gboolean gMinus( GtkWidget *widget,gpointer user_data );//assigned to zoomOut
static gboolean gFit( GtkWidget *widget,gpointer user_data );//assigned to zoomFit

GtkWidget *cpu_buttonwidget_get_widget(ButtonWidget *button_widget);
void cpu_update_pixmap_size(cpuDrawing_t *value,
                                    guint width);
#endif //_HISTOBUTTONWIDGET_H 
