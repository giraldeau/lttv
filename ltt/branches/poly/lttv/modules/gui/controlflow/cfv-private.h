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

#ifndef _CFV_PRIVATE_H
#define _CFV_PRIVATE_H


struct _ControlFlowData {

  GtkWidget *scrolled_window;
  MainWindow *mw;

  GtkWidget *h_paned;

  ProcessList *process_list;

  Drawing_t *drawing;
  GtkAdjustment *v_adjust ;
  
  /* Shown events information */
//  TimeWindow time_window;
//  LttTime current_time;
  
  //guint currently_Selected_Event  ;
  guint number_of_process;

  /* hooks for trace read */
  LttvHooks *event;
  LttvHooks *after_event;
  LttvHooks *after_traceset;
  EventRequest *event_request;

} ;


#endif //_CFV_PRIVATE_H
