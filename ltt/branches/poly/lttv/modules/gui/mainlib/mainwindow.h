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

#ifndef _MAIN_WINDOW_
#define _MAIN_WINDOW_

#include <gtk/gtk.h>

#include <ltt/ltt.h>
#include <lttv/attribute.h>
#include <lttv/traceset.h>
#include <lttv/tracecontext.h>

#include <lttv/common.h>
#include <lttv/gtkmultivpaned.h>
#include <lttv/hook.h>
#include <lttv/stats.h>

typedef struct _TracesetInfo {
  //FIXME? TracesetContext and stats in same or different variable ?
  LttvTracesetStats * traceset_context;
  LttvTraceset * traceset;
} TracesetInfo ;


struct _MainWindow{
  GtkWidget*      mwindow;            /* Main Window */
  int             window_width;

  /* Status bar information */
  //  guint         MainSBarContextID;    /* Context ID of main status bar */
  //  guint         BegTimeSBarContextID; /* Context ID of BegTime status bar */
  //  guint         EndTimeSBarContextID; /* Context ID of EndTime status bar */
  
  /* Child windows */
  //openTracesetWindow*  OpenTracesetWindow;/* Window to get prof and proc file*/
  //viewTimeFrameWindow* ViewTimeFrameWindow;/*Window to select time frame */
  //gotoEventWindow*     GotoEventWindow; /*search for event description*/
  //openFilterWindow*    OpenFilterWindow; /* Open a filter selection window */
  GtkWidget*           help_contents;/* Window to display help contents */
  GtkWidget*           about_box;    /* Window  about information */
 
  //  lttv_trace_filter * filter; /* trace filter associated with the window */

  /* Attributes for trace reading hooks local to the main window */
  LttvIAttribute * attributes;
  
  Tab * tab;
  Tab * current_tab;

  GHashTable * hash_menu_item;
  GHashTable * hash_toolbar_item;
};


struct _Tab{
  GtkWidget * label;
  GtkMultiVPaned * multi_vpaned;
   
  // startTime is the left of the visible area. Corresponds to the scrollbar
  // value.
  // Time_Width is a zoom dependant value (corresponding to page size)
  TimeWindow time_window;
  
  // The current time is the time selected in the visible area by the user,
  // not the scrollbar value.
  LttTime current_time;
  LttvIAttribute * attributes;

  struct _Tab * next;
  MainWindow  * mw;

  /* Traceset related information */
  TracesetInfo * traceset_info; 
};

/**
 * Remove menu and toolbar item when a module unloaded
 */
void main_window_remove_menu_item(lttv_constructor view_constructor);
void main_window_remove_toolbar_item(lttv_constructor view_constructor);

#endif /* _MAIN_WINDOW_ */


