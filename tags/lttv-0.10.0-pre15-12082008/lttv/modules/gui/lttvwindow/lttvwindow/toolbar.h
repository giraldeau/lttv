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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <lttvwindow/lttvwindow.h>
#include <gtk/gtk.h>

typedef GArray LttvToolbars;

typedef struct _LttvToolbarClosure {
  lttvwindow_viewer_constructor con;
  char * tooltip;
  char ** pixmap;
  GtkWidget *widget;
} LttvToolbarClosure;

LttvToolbars *lttv_toolbars_new();

void lttv_toolbars_destroy(LttvToolbars *h);

LttvToolbarClosure lttv_toolbars_add(LttvToolbars *h,
                       lttvwindow_viewer_constructor f,
                       char* tooltip,
                       char ** pixmap,
                       GtkWidget *widget);

GtkWidget *lttv_toolbars_remove(LttvToolbars *h, lttvwindow_viewer_constructor f);

unsigned lttv_toolbars_number(LttvToolbars *h);

#endif // TOOLBAR_H
