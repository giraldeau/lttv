#ifndef _CFV_H
#define _CFV_H

#include <gtk/gtk.h>

typedef struct _ControlFlowData ControlFlowData;

/* Control Flow Data constructor */
ControlFlowData *GuiControlFlow(void);
void
GuiControlFlow_Destructor_Full(ControlFlowData *Control_Flow_Data);
void
GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data);
GtkWidget *GuiControlFlow_get_Widget(ControlFlowData *Control_Flow_Data);

#endif // _CFV_H
