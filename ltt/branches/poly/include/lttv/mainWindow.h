#ifndef _MAIN_WINDOW_
#define _MAIN_WINDOW_

#include <gtk/gtk.h>

#include <ltt/ltt.h>
#include <lttv/attribute.h>
#include <lttv/traceset.h>
#include <lttv/processTrace.h>

#include <lttv/common.h>
#include <lttv/gtkcustom.h>
#include <lttv/hook.h>
#include <lttv/stats.h>

typedef struct _WindowCreationData {
	int argc;
	char ** argv;
} WindowCreationData;

typedef struct _TimeWindow {
	LttTime startTime;
	LttTime Time_Width;
} TimeWindow;

typedef struct _TracesetInfo {
	gchar* path;
	LttvHooks 
	  *before_traceset,
	  *after_traceset,
	  *before_trace,
	  *after_trace,
	  *before_tracefile,
	  *after_tracefile,
	  *before_event,
	  *after_event;
        //FIXME? TracesetContext and stats in same or different variable ?
	LttvTracesetStats * TracesetContext;
	LttvTraceset * traceset;
} TracesetInfo ;


struct _mainWindow{
  GtkWidget*      MWindow;            /* Main Window */

  /* Status bar information */
  //  guint         MainSBarContextID;    /* Context ID of main status bar */
  //  guint         BegTimeSBarContextID; /* Context ID of BegTime status bar */
  //  guint         EndTimeSBarContextID; /* Context ID of EndTime status bar */
  
  /* Child windows */
  //openTracesetWindow*  OpenTracesetWindow;/* Window to get prof and proc file*/
  //viewTimeFrameWindow* ViewTimeFrameWindow;/*Window to select time frame */
  //gotoEventWindow*     GotoEventWindow; /*search for event description*/
  //openFilterWindow*    OpenFilterWindow; /* Open a filter selection window */
  GtkWidget*           HelpContents;/* Window to display help contents */
  GtkWidget*           AboutBox;    /* Window  about information */
 
  //  lttv_trace_filter * filter; /* trace filter associated with the window */

  /* Traceset related information */
  TracesetInfo * Traceset_Info; 
  /* Attributes for trace reading hooks local to the main window */
  LttvIAttribute * Attributes;
  
  tab * Tab;
  tab * CurrentTab;

  WindowCreationData * winCreationData; 

  GHashTable * hash_menu_item;
  GHashTable * hash_toolbar_item;
};


struct _tab{
  GtkWidget * label;
  GtkCustom * custom;
   
  // startTime is the left of the visible area. Corresponds to the scrollbar
  // value.
  // Time_Width is a zoom dependant value (corresponding to page size)
  TimeWindow Time_Window;
  
  // The current time is the time selected in the visible area by the user,
  // not the scrollbar value.
  LttTime currentTime;
  LttvIAttribute * Attributes;

  struct _tab * Next;
  mainWindow  * mw;
};

/**
 * Remove menu and toolbar item when a module unloaded
 */
void main_window_remove_menu_item(lttv_constructor view_constructor);
void main_window_remove_toolbar_item(lttv_constructor view_constructor);

#endif /* _MAIN_WINDOW_ */


