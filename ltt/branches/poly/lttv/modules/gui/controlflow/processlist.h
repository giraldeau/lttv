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
  guint cpu;
  guint ppid;
  LttTime birth;
  guint trace_num;

 // gint height_cache;

} ProcessInfo;

typedef struct _HashedProcessData {
  
  GtkTreeIter y_iter; // Access quickly to y pos.
 // DrawContext *draw_context;
  /* Information on current drawing */
  struct {
    guint over;
    gboolean over_used;    /* inform the user that information is incomplete */
    gboolean over_marked;  /* inform the user that information is incomplete */
    guint middle;
    gboolean middle_used;  /* inform the user that information is incomplete */
    gboolean middle_marked;/* inform the user that information is incomplete */
    guint under;
    gboolean under_used;   /* inform the user that information is incomplete */
    gboolean under_marked; /* inform the user that information is incomplete */
  } x; /* last x position saved by after state update */

  LttTime next_good_time; /* precalculate the next time where the next
                             pixel is.*/
  // FIXME : add info on last event ?

} HashedProcessData;
  
struct _ProcessList {
  
  GtkWidget *process_list_widget;
  GtkListStore *list_store;
  GtkWidget *button; /* one button of the tree view */

  /* A hash table by PID to speed up process position find in the list */
  GHashTable *process_hash;
  
  guint number_of_process;
  gint cell_height_cache;

  /* Current process, one per cpu */
  HashedProcessData **current_hash_data;

};


typedef struct _ProcessList ProcessList;

ProcessList *processlist_construct(void);
void processlist_destroy(ProcessList *process_list);
GtkWidget *processlist_get_widget(ProcessList *process_list);

void processlist_clear(ProcessList *process_list);

// out : success (0) and height
/* CPU num is only used for PID 0 */
int processlist_add(ProcessList *process_list, guint pid, guint cpu, guint ppid,
    LttTime *birth, guint trace_num, const gchar *name, guint *height,
    ProcessInfo **process_info,
    HashedProcessData **hashed_process_data);
// out : success (0) and height
int processlist_remove(ProcessList *process_list, guint pid, guint cpu, 
    LttTime *birth, guint trace_num);





static inline gint get_cell_height(ProcessList *process_list,
                                   GtkTreeView *tree_view)
{
  gint height = process_list->cell_height_cache;
  if(height != -1) return height;
  else {
    GtkTreeViewColumn *Column = gtk_tree_view_get_column(tree_view, 0);
  
    gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL,
                                       &process_list->cell_height_cache);
  }
  return process_list->cell_height_cache;
}



static inline guint processlist_get_height(ProcessList *process_list)
{
  return get_cell_height(process_list,
                         (GtkTreeView*)process_list->process_list_widget)
        * process_list->number_of_process ;
}


static inline HashedProcessData *processlist_get_process_data( 
          ProcessList *process_list,
          guint pid, guint cpu, LttTime *birth, guint trace_num)
{
  ProcessInfo process_info;
  gint *path_indices;
  GtkTreePath *tree_path;

  process_info.pid = pid;
  if(pid == 0)
    process_info.cpu = cpu;
  else
    process_info.cpu = 0;
  process_info.birth = *birth;
  process_info.trace_num = trace_num;

  return  (HashedProcessData*)g_hash_table_lookup(
                process_list->process_hash,
                &process_info);
}


static inline gint processlist_get_pixels_from_data(  ProcessList *process_list,
          HashedProcessData *hashed_process_data,
          guint *y,
          guint *height)
{
  gint *path_indices;
  GtkTreePath *tree_path;

  tree_path = gtk_tree_model_get_path((GtkTreeModel*)process_list->list_store,
                    &hashed_process_data->y_iter);
  path_indices =  gtk_tree_path_get_indices (tree_path);

  *height = get_cell_height(process_list,
      (GtkTreeView*)process_list->process_list_widget);
  *y = *height * path_indices[0];
  gtk_tree_path_free(tree_path);

  return 0; 

}



#endif // _PROCESS_LIST_H
