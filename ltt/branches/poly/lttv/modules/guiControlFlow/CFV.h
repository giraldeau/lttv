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
guicontrolflow_destructor_full(ControlFlowData *Control_Flow_Data);
void
guicontrolflow_destructor(ControlFlowData *Control_Flow_Data);
GtkWidget *guicontrolflow_get_widget(ControlFlowData *Control_Flow_Data);
ProcessList *guicontrolflow_get_process_list(ControlFlowData *Control_Flow_Data);
TimeWindow *guicontrolflow_get_time_window(ControlFlowData *Control_Flow_Data);
LttTime *guicontrolflow_get_current_time(ControlFlowData *Control_Flow_Data);


#endif // _CFV_H
