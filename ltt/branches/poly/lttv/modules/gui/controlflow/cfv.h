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
#include "processlist.h"

typedef struct _ControlFlowData ControlFlowData;

/* Control Flow Data constructor */
ControlFlowData *guicontrolflow(void);
void
guicontrolflow_destructor_full(ControlFlowData *control_flow_data);
void
guicontrolflow_destructor(ControlFlowData *control_flow_data);
__inline GtkWidget *guicontrolflow_get_widget(
                                       ControlFlowData *control_flow_data);
__inline ProcessList *guicontrolflow_get_process_list(
                                       ControlFlowData *control_flow_data);


#endif // _CFV_H
