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

#ifndef _MAIN_WINDOW_PRIVATE_
#define _MAIN_WINDOW_PRIVATE_

#include <gtk/gtk.h>

#include <ltt/ltt.h>
#include <lttv/attribute.h>
#include <lttv/traceset.h>
#include <lttv/tracecontext.h>
#include <lttv/hook.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
//#include <lttvwindow/gtkmultivpaned.h>
#include <lttvwindow/mainwindow.h>

#define SCROLL_STEP_PER_PAGE 10.0

struct _TracesetInfo {
  //FIXME? TracesetContext and stats in same or different variable ?
  LttvTracesetStats * traceset_context;
  LttvTraceset * traceset;
};

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
  
  //Tab * tab;
  //Tab * current_tab;

};


struct _Tab{
  GtkWidget *label;
  GtkWidget *top_widget;
  GtkWidget *vbox; /* contains viewer_container and scrollbar */
  //GtkWidget *multivpaned;
  GtkWidget *viewer_container;
  GtkWidget *scrollbar;

  /* Paste zones */
  GtkTooltips *tooltips;
  
  /* time bar */
  GtkWidget *MTimebar;
  GtkWidget *MEventBox1a;
  GtkWidget *MText1a;
  GtkWidget *MEventBox1b;
  GtkWidget *MText1b;
  GtkWidget *MEntry1;
  GtkWidget *MText2;
  GtkWidget *MEntry2;
  GtkWidget *MText3a;
  GtkWidget *MEventBox3b;
  GtkWidget *MText3b;
  GtkWidget *MEntry3;
  GtkWidget *MText4;
  GtkWidget *MEntry4;
  GtkWidget *MText5a;
  GtkWidget *MEventBox5b;
  GtkWidget *MText5b;
  GtkWidget *MEntry5;
  GtkWidget *MText6;
  GtkWidget *MEntry6;
  GtkWidget *MText7;
  GtkWidget *MEventBox8;
  GtkWidget *MText8;
  GtkWidget *MEntry7;
  GtkWidget *MText9;
  GtkWidget *MEntry8;
  GtkWidget *MText10;
   
  // startTime is the left of the visible area. Corresponds to the scrollbar
  // value.
  // Time_Width is a zoom dependant value (corresponding to page size)
  TimeWindow time_window;
  gboolean time_manager_lock;
  
  // The current time is the time selected in the visible area by the user,
  // not the scrollbar value.
  LttTime current_time;
  gboolean current_time_manager_lock;

  LttvIAttribute * attributes;

  //struct _Tab * next;
  MainWindow  * mw;

  /* Traceset related information */
  TracesetInfo * traceset_info; 

  /* Filter to apply to the tab's traceset */
  LttvFilter *filter;

  /* A list of time requested for the next process trace */
  GSList *events_requests;
  gboolean events_request_pending;
  LttvAttribute *interrupted_state;
  gboolean stop_foreground;
};

#endif /* _MAIN_WINDOW_PRIVATE_ */


