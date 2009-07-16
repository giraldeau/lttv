/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers
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



#ifndef _CFV_H
#define _CFV_H

#include <gtk/gtk.h>
#include <lttvwindow/mainwindow.h>
#include <lttv/filter.h>
#include <lttvwindow/lttv_plugin_tab.h>

extern GQuark LTT_NAME_CPU;

#ifndef TYPE_DRAWING_T_DEFINED
#define TYPE_DRAWING_T_DEFINED
typedef struct _Drawing_t Drawing_t;
#endif //TYPE_DRAWING_T_DEFINED

#ifndef TYPE_CONTROLFLOWDATA_DEFINED
#define TYPE_CONTROLFLOWDATA_DEFINED
typedef struct _ControlFlowData ControlFlowData;
#endif //TYPE_CONTROLFLOWDATA_DEFINED

#ifndef TYPE_PROCESSLIST_DEFINED
#define TYPE_PROCESSLIST_DEFINED
typedef struct _ProcessList ProcessList;
#endif //TYPE_PROCESSLIST_DEFINED

struct _ControlFlowData {

  GtkWidget *top_widget;
  Tab *tab;
  LttvPluginTab *ptab;
  
  GtkWidget *hbox;
  GtkWidget *toolbar; /* Vbox that contains the viewer's toolbar */
  GtkToolItem *button_prop; /* Properties button. */
  GtkToolItem *button_filter; /* Properties button. */
  GtkWidget *box; /* box that contains the hpaned. necessary for it to work */
  GtkWidget *h_paned;

  ProcessList *process_list;

  Drawing_t *drawing;
  GtkAdjustment *v_adjust ;

  /* Shown events information */
//  TimeWindow time_window;
//  LttTime current_time;
  
  //guint currently_Selected_Event  ;
  guint number_of_process;
  guint background_info_waiting; /* Number of background requests waited for
                                    in order to have all the info ready. */

  LttvFilter *filter;

} ;

/* Control Flow Data constructor */
ControlFlowData *guicontrolflow(LttvPluginTab *ptab);
void
guicontrolflow_destructor_full(gpointer data);
void
guicontrolflow_destructor(gpointer data);

static inline GtkWidget *guicontrolflow_get_widget(
                                     ControlFlowData *control_flow_data)
{
  return control_flow_data->top_widget ;
}

static inline ProcessList *guicontrolflow_get_process_list
    (ControlFlowData *control_flow_data)
{
    return control_flow_data->process_list ;
}

ControlFlowData *resourceview(LttvPluginTab *ptab);

#endif // _CFV_H
