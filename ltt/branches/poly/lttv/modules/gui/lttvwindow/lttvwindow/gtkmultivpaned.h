
/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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

#ifndef __GTK_MULTI_VPANED_H__
#define __GTK_MULTI_VPANED_H__


#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <lttvwindow/mainwindow.h> // for TimeWindow

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_MULTI_VPANED            (gtk_multi_vpaned_get_type ())
#define GTK_MULTI_VPANED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MULTI_VPANED, GtkMultiVPaned))
#define GTK_MULTI_VPANED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_MULTI_VPANED, GtkMultiVPanedClass))
#define GTK_IS_MULTI_VPANED(obj   )      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_MULTI_VPANED))
#define GTK_IS_MULTI_VPANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MULTI_VPANED))
#define GTK_MULTI_VPANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_MULTI_VPANED, GtkMultiVPanedClass))

typedef struct _GtkMultiVPaned	 GtkMultiVPaned;
typedef struct _GtkMultiVPanedClass   GtkMultiVPanedClass;

struct _GtkMultiVPaned
{
  GtkPaned container;

  /*< public >*/
  GtkPaned * first_pane;
  GtkPaned * last_pane;
  GtkPaned * focused_pane;
  GtkPaned * iter;
  guint num_children;

  GtkWidget * vbox;
  //  GtkWidget * scrollWindow;
  //  GtkWidget * viewport;
  GtkWidget * hscrollbar;  
  GtkAdjustment *hadjust;
};

struct _GtkMultiVPanedClass
{
  GtkPanedClass parent_class;
};


GType	   gtk_multi_vpaned_get_type (void) G_GNUC_CONST;
GtkWidget* gtk_multi_vpaned_new (void);

void gtk_multi_vpaned_set_focus (GtkWidget * widget, GtkPaned *Paned);     
void gtk_multi_vpaned_widget_add(GtkMultiVPaned * multi_vpaned, GtkWidget * widget1);
void gtk_multi_vpaned_widget_delete(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_widget_move_up(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_widget_move_down(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_set_adjust(GtkMultiVPaned * multi_vpaned, const TimeWindow * time_window, gboolean first_time);
void gtk_multi_vpaned_set_data(GtkMultiVPaned * multi_vpaned, char * key, gpointer value);
gpointer gtk_multi_vpaned_get_data(GtkMultiVPaned * multi_vpaned, char * key);
GtkWidget * gtk_multi_vpaned_get_widget(GtkMultiVPaned * multi_vpaned);
GtkWidget * gtk_multi_vpaned_get_first_widget(GtkMultiVPaned * multi_vpaned);
GtkWidget * gtk_multi_vpaned_get_next_widget(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_set_scroll_value(GtkMultiVPaned * multi_vpaned, double value);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MULTI_VPANED_H__ */
