#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

typedef struct _ProcessList ProcessList;

ProcessList *ProcessList_construct(void);
void ProcessList_destroy(ProcessList *Process_List);
GtkWidget *ProcessList_getWidget(ProcessList *Process_List);

//int ProcessList_add(Process *myproc, ProcessList *Process_List, guint *height);
//int ProcessList_remove(Process *myproc, ProcessList *Process_List);
#endif // _PROCESS_LIST_H
