#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include <gtk/gtk.h>
#include <lttv/state.h>
#include <ltt/ltt.h>

/* The process list
 *
 * Tasks :
 * Create a process list
 * contains the data for the process list
 * tells the height of the process list widget
 * provides methods to add/remove process from the list
 * 	note : the sync with drawing is left to the caller.
 * provides helper function to convert a process unique identifier to
 * 	pixels (in height).
 */

typedef struct _ProcessList ProcessList;

ProcessList *ProcessList_construct(void);
void ProcessList_destroy(ProcessList *Process_List);
GtkWidget *ProcessList_getWidget(ProcessList *Process_List);

// out : success (0) and height
int ProcessList_add(ProcessList *Process_List, guint pid, LttTime *birth,
		guint *height);
// out : success (0) and height
int ProcessList_remove(ProcessList *Process_List, guint pid, LttTime *birth);

guint ProcessList_get_pixels(ProcessList *Process_List);

// Returns 0 on success
gint ProcessList_get_process_pixels(ProcessList *Process_List,
				guint pid, LttTime *birth,
				guint *x, guint *height);
#endif // _PROCESS_LIST_H
