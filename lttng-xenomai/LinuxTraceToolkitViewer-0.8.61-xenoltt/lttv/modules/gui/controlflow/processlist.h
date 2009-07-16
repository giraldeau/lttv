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
#include <gdk/gdk.h>
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
 */


/* Enumeration of the columns */
enum
{
  PROCESS_COLUMN,
  BRAND_COLUMN,
  PID_COLUMN,
  TGID_COLUMN,
  PPID_COLUMN,
  CPU_COLUMN,
  BIRTH_S_COLUMN,
  BIRTH_NS_COLUMN,
  TRACE_COLUMN,
  N_COLUMNS
};


typedef struct _ProcessInfo {
  
  guint pid;
  guint tgid;
  guint cpu;
  guint ppid;
  LttTime birth;
  guint trace_num;

 // gint height_cache;

} ProcessInfo;

typedef struct _HashedProcessData {
 
  GdkPixmap *pixmap;  // Pixmap slice containing drawing buffer for the PID
  gint height; // height of the pixmap
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

} HashedProcessData;
  
struct _ProcessList {
  
  GtkWidget *process_list_widget;
  GtkListStore *list_store;
  GtkWidget *button; /* one button of the tree view */
  GtkCellRenderer *renderer;

  /* A hash table by PID to speed up process position find in the list */
  GHashTable *process_hash;
  
  guint number_of_process;
  gint cell_height;

  /* Current process pointer, one per cpu, one per trace */
  HashedProcessData ***current_hash_data;

  /* Array containing index -> pixmap correspondance. Must be updated
   * every time the process list is reordered, process added or removed */
  GPtrArray * index_to_pixmap;

};


typedef struct _ProcessList ProcessList;


#ifndef TYPE_DRAWING_T_DEFINED
#define TYPE_DRAWING_T_DEFINED
typedef struct _Drawing_t Drawing_t;
#endif //TYPE_DRAWING_T_DEFINED

ProcessList *processlist_construct(void);
void processlist_destroy(ProcessList *process_list);
GtkWidget *processlist_get_widget(ProcessList *process_list);

void processlist_clear(ProcessList *process_list);

// out : success (0) and height
/* CPU num is only used for PID 0 */
int processlist_add(ProcessList *process_list, Drawing_t * drawing, 
    guint pid, guint tgid, guint cpu, guint ppid,
    LttTime *birth, guint trace_num, GQuark name, GQuark brand, guint *height,
    ProcessInfo **process_info,
    HashedProcessData **hashed_process_data);
// out : success (0) and height
int processlist_remove(ProcessList *process_list, guint pid, guint cpu, 
    LttTime *birth, guint trace_num);

/* Set the name of a process */
void processlist_set_name(ProcessList *process_list,
    GQuark name,
    HashedProcessData *hashed_process_data);

void processlist_set_brand(ProcessList *process_list,
    GQuark brand,
    HashedProcessData *hashed_process_data);

/* Set the ppid of a process */
void processlist_set_tgid(ProcessList *process_list,
    guint tgid,
    HashedProcessData *hashed_process_data);
void processlist_set_ppid(ProcessList *process_list,
    guint ppid,
    HashedProcessData *hashed_process_data);


/* Synchronize the list at the left and the drawing */
void update_index_to_pixmap(ProcessList *process_list);

/* Update the width of each pixmap buffer for each process */
void update_pixmap_size(ProcessList *process_list, guint width);


/* Put src and/or dest to NULL to copy from/to the each PID specific pixmap */
void copy_pixmap_region(ProcessList *process_list, GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height);

/* If height is -1, the height of each pixmap is used */
void rectangle_pixmap(ProcessList *process_list, GdkGC *gc,
    gboolean filled, gint x, gint y, gint width, gint height);

/* Renders each pixmaps into on big drawable */
void copy_pixmap_to_screen(ProcessList *process_list,
    GdkDrawable *dest,
    GdkGC *gc,
    gint x, gint y,
    gint width, gint height);


static inline gint get_cell_height(GtkTreeView *TreeView)
{
  gint height;
  GtkTreeViewColumn *column = gtk_tree_view_get_column(TreeView, 0);
  
  gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height);

  gint vertical_separator;
  gtk_widget_style_get (GTK_WIDGET (TreeView),
      "vertical-separator", &vertical_separator,
      NULL);
  height += vertical_separator;

  return height;
}

static inline guint processlist_get_height(ProcessList *process_list)
{
  return process_list->cell_height * process_list->number_of_process ;
}


static inline HashedProcessData *processlist_get_process_data( 
          ProcessList *process_list,
          guint pid, guint cpu, LttTime *birth, guint trace_num)
{
  ProcessInfo process_info;

  process_info.pid = pid;
  if(pid == 0)
    process_info.cpu = cpu;
  else
    process_info.cpu = ANY_CPU;
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

  *height = get_cell_height((GtkTreeView*)process_list->process_list_widget);
  *y = *height * path_indices[0];
  gtk_tree_path_free(tree_path);

  return 0; 

}

static inline guint processlist_get_index_from_data(ProcessList *process_list,
          HashedProcessData *hashed_process_data)
{
  gint *path_indices;
  GtkTreePath *tree_path;
  guint ret;

  tree_path = gtk_tree_model_get_path((GtkTreeModel*)process_list->list_store,
                    &hashed_process_data->y_iter);
  path_indices =  gtk_tree_path_get_indices (tree_path);

  ret = path_indices[0];

  gtk_tree_path_free(tree_path);

  return ret;
}



#endif // _PROCESS_LIST_H
