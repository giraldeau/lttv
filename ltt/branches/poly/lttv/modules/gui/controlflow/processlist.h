#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include <gtk/gtk.h>
#include <lttv/state.h>
#include <ltt/ltt.h>

#include "drawitem.h"

/* The process list
 *
 * Tasks :
 * Create a process list
 * contains the data for the process list
 * tells the height of the process list widget
 * provides methods to add/remove process from the list
 *  note : the sync with drawing is left to the caller.
 * provides helper function to convert a process unique identifier to
 *  pixels (in height).
 *
 * //FIXME : connect the scrolled window adjustment with the list.
 */

typedef struct _ProcessInfo {
  
  guint pid;
  LttTime birth;

} ProcessInfo;

typedef struct _HashedProcessData {
  
  GtkTreeRowReference *row_ref;
  DrawContext *draw_context;

} HashedProcessData;
  
struct _ProcessList {
  
  GtkWidget *process_list_widget;
  GtkListStore *list_store;

  /* A hash table by PID to speed up process position find in the list */
  GHashTable *process_hash;
  
  guint number_of_process;
};


typedef struct _ProcessList ProcessList;

ProcessList *processlist_construct(void);
void processlist_destroy(ProcessList *process_list);
GtkWidget *processlist_get_widget(ProcessList *process_list);

// out : success (0) and height
int processlist_add(ProcessList *process_list, guint pid, LttTime *birth,
    gchar *name, guint *height, HashedProcessData **hashed_process_data);
// out : success (0) and height
int processlist_remove(ProcessList *process_list, guint pid, LttTime *birth);

guint processlist_get_height(ProcessList *process_list);

// Returns 0 on success
gint processlist_get_process_pixels(ProcessList *process_list,
        guint pid, LttTime *birth,
        guint *y, guint *height,
        HashedProcessData **hashed_process_data);

gint processlist_get_pixels_from_data(  ProcessList *process_list,
          ProcessInfo *process_info,
          HashedProcessData *hashed_process_data,
          guint *y,
          guint *height);

#endif // _PROCESS_LIST_H
