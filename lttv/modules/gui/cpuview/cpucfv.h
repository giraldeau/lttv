/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Parisa heidari (inspired from CFV by Mathieu Desnoyers)
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



#ifndef _HISTOCFV_H
#define _HISTOCFV_H

#include <gtk/gtk.h>
#include <lttvwindow/mainwindow.h>
#include <lttvwindow/lttv_plugin_tab.h>

extern GQuark LTT_NAME_CPU;


#ifndef TYPE_cpuDrawing_t_DEFINED
#define TYPE_cpuDrawing_t_DEFINED
typedef struct _cpuDrawing_t cpuDrawing_t;
#endif //TYPE_cpuDrawing_t_DEFINED

#ifndef TYPE_ButtonWidget_DEFINED
#define TYPE_ButtonWidget_DEFINED
typedef struct _ButtonWidget ButtonWidget;
#endif //TYPE_ButtonWidget_DEFINED

#ifndef TYPE_CpuControlFlowData_DEFINED
#define TYPE_CpuControlFlowData_DEFINED
typedef struct _CpuControlFlowData CpuControlFlowData;
#endif //TYPE_CpuControlFlowData_DEFINED



struct _CpuControlFlowData {

  GtkWidget *top_widget;//The hbox containing buttons and drawing area.
  LttvPluginTab *ptab;
  Tab *tab; 
  GtkWidget *box;
  GtkWidget *ev_box;//for cpugram
  ButtonWidget *buttonwidget;

  cpuDrawing_t *drawing;
  //GtkAdjustment *v_adjust ;//may be used later for scrollbar

  /* Shown events information */
//  TimeWindow time_window;
//  LttTime current_time;

  //guint currently_Selected_Event  ;
  GArray  *number_of_process;//number of events
  guint background_info_waiting; /* Number of background requests waited for
                                    in order to have all the info ready. */
// For cpugram
  guint max_height;

  LttvFilter *cpu_main_win_filter;
  gboolean chunk_has_begun;
} ;

/* Control Flow Data constructor */
CpuControlFlowData *guicpucontrolflow(LttvPluginTab *ptab);
void
guicpucontrolflow_destructor_full(CpuControlFlowData *cpu_control_flow_data);
void
guicpucontrolflow_destructor(CpuControlFlowData *cpu_control_flow_data);

static inline GtkWidget *guicpucontrolflow_get_widget(
                         CpuControlFlowData *cpu_control_flow_data)
{
  return cpu_control_flow_data->top_widget ;
}

#endif // _HISTOCFV_H
