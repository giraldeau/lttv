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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <ltt/ltt.h>
#include <gtk/gtk.h>
#include <lttvwindow/lttvfilter.h>

#define MAX_NUMBER_EVENT     "MAX_NUMBER_EVENT"
#define TRACESET_TIME_SPAN   "TRACESET_TIME_SPAN"

typedef struct _MainWindow MainWindow;
typedef struct _Tab Tab;

/* constructor of the viewer */
typedef GtkWidget * (*lttvwindow_viewer_constructor)
                (MainWindow * main_window, LttvTracesetSelector * s, char *key);

typedef struct _TimeWindow {
	LttTime start_time;
	LttTime time_width;
} TimeWindow;

#endif // COMMON_H
