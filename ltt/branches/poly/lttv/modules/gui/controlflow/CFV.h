#ifndef _CFV_H
#define _CFV_H

#include <gtk/gtk.h>
#include <lttv/common.h>
#include <lttv/mainWindow.h>
#include "Process_List.h"

typedef struct _ControlFlowData ControlFlowData;

/* Control Flow Data constructor */
ControlFlowData *guicontrolflow(void);
void
guicontrolflow_destructor_full(ControlFlowData *control_flow_data);
void
guicontrolflow_destructor(ControlFlowData *control_flow_data);
GtkWidget *guicontrolflow_get_widget(ControlFlowData *control_flow_data);
ProcessList *guicontrolflow_get_process_list(ControlFlowData *control_flow_data);
TimeWindow *guicontrolflow_get_time_window(ControlFlowData *control_flow_data);
LttTime *guicontrolflow_get_current_time(ControlFlowData *control_flow_data);


#endif // _CFV_H
