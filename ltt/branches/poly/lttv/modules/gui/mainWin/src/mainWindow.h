#ifndef _MAIN_WINDOW_
#define _MAIN_WINDOW_

#include <gtk/gtk.h>

#include <lttv/gtkcustom.h>
#include <ltt/ltt.h>
#include <lttv/attribute.h>
#include <lttv/traceset.h>
#include <lttv/processTrace.h>


typedef struct _TimeInterval{
  LttTime startTime;
  LttTime endTime;  
} TimeInterval;


typedef struct _systemView systemView;
typedef struct _tab tab;

typedef struct _mainWindow{
  GtkWidget*      MWindow;            /* Main Window */
  systemView *    SystemView;         /* System view displayed in this window*/

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
 
  LttvTracesetContext * traceset_context; 
  LttvTraceset * traceset;  /* trace set associated with the window */
  //  lttv_trace_filter * filter; /* trace filter associated with the window */
  

  tab * Tab;
  tab * CurrentTab;
  LttvIAttribute * Attributes;
 
} mainWindow;

typedef GtkWidget * (*view_constructor)(mainWindow * main_win);

struct _systemView{
  gpointer             EventDB;
  gpointer           * SystemInfo;
  gpointer           * Options;
  mainWindow         * Window;
  struct _systemView * Next;
};

struct _tab{
  GtkWidget * label;
  GtkCustom * custom;

  LttTime traceStartTime;
  LttTime traceEndTime;
  LttTime startTime;
  LttTime endTime;
  LttTime currentTime;
  LttvIAttribute * Attributes;

  struct _tab * Next;
};

#endif /* _MAIN_WINDOW_ */


