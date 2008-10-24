#ifndef _EVENTS_H
#define _EVENTS_H

#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttv_plugin_tab.h>

typedef struct _EventViewerData EventViewerData;

struct _EventViewerData {

  Tab * tab;
  LttvPluginTab *ptab;
  LttvHooks  * event_hooks;

  /* previous value is used to determine if it is a page up/down or
   * step up/down, in which case we move of a certain amount of events (one or
   * the number of events shown on the screen) instead of changing begin time.
   */
  double       previous_value;

  //scroll window containing Tree View
  GtkWidget * scroll_win;

  /* Model containing list data */
  GtkListStore *store_m;

  GPtrArray *pos; /* Array of LttvTracesetContextPosition * */
 
  GtkWidget *top_widget;
  GtkWidget *hbox_v;
  /* Widget to display the data in a columned list */
  GtkWidget *tree_v;
  GtkAdjustment *vtree_adjust_c ;
  GtkWidget *button; /* a button of the header, used to get the header_height */
  gint header_height;
  
  /* Vertical scrollbar and its adjustment */
  GtkWidget *vscroll_vc;
  GtkAdjustment *vadjust_c;
  
  /* Selection handler */
  GtkTreeSelection *select_c;
  
  gint num_visible_events;
  
  LttvTracesetContextPosition *currently_selected_position;
  gboolean update_cursor; /* Speed optimisation : do not update cursor when 
                             unnecessary */
  gboolean report_position; /* do not report position when in current_time
                               update */

  LttvTracesetContextPosition *first_event;  /* Time of the first event shown */
  LttvTracesetContextPosition *last_event;  /* Time of the first event shown */

  LttvTracesetContextPosition *current_time_get_first; 

  LttvFilter *main_win_filter;

  gint background_info_waiting;

  guint32 last_tree_update_time; /* To filter out repeat keys */

  guint num_events;  /* Number of events processed */

  LttvFilter *filter;
  GtkWidget *toolbar;
  GtkToolItem *button_filter;

  guint init_done;
};

extern gint evd_redraw_notify(void *hook_data, void *call_data);

#endif //EVENTS_H
