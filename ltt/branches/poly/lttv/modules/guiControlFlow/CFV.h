#ifndef _CFV_H
#define _CFV_H

#include <gtk/gtk.h>
#include "lttv/common.h"
#include "lttv/mainWindow.h"
#include "Process_List.h"

typedef struct _ControlFlowData ControlFlowData;

/* Control Flow Data constructor */
ControlFlowData *GuiControlFlow(void);
void
GuiControlFlow_Destructor_Full(ControlFlowData *Control_Flow_Data);
void
GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data);
GtkWidget *GuiControlFlow_get_Widget(ControlFlowData *Control_Flow_Data);
ProcessList *GuiControlFlow_get_Process_List(ControlFlowData *Control_Flow_Data);
TimeWindow *GuiControlFlow_get_Time_Window(ControlFlowData *Control_Flow_Data);
LttTime *GuiControlFlow_get_Current_Time(ControlFlowData *Control_Flow_Data);


#endif // _CFV_H
