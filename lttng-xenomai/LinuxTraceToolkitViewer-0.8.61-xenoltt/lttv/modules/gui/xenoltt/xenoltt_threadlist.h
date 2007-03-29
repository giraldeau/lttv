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



#ifndef _THREAD_LIST_H
#define _THREAD_LIST_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <lttv/state.h>
#include <ltt/ltt.h>

#include "xenoltt_drawitem.h"

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
  THREAD_COLUMN,
  PRIO_COLUMN,
  PERIOD_COLUMN,
  CPU_COLUMN,
  BIRTH_S_COLUMN,
  BIRTH_NS_COLUMN,
  TRACE_COLUMN,
  N_COLUMNS
};


typedef struct _XenoThreadInfo {
  
  gulong address;
  guint prio;
  guint cpu;
  guint period;
  LttTime thread_birth;
  guint trace_num;

} XenoThreadInfo;

typedef struct _HashedThreadData {
 
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

  gulong timer_address;
  
} HashedThreadData;
  
struct _ThreadList {
  
  GtkWidget *thread_list_widget;
  GtkListStore *list_store;
  GtkWidget *button; /* one button of the tree view */
  GtkCellRenderer *renderer;

  /* A hash table by PID to speed up process position find in the list */
  GHashTable *thread_hash;
  
  guint number_of_thread;
  gint cell_height;

  /* Current process pointer, one per cpu, one per trace */
  HashedThreadData ***current_hash_data;

  /* Array containing index -> pixmap correspondance. Must be updated
   * every time the process list is reordered, process added or removed */
  GPtrArray * index_to_pixmap;

};


typedef struct _ThreadList ThreadList;


#ifndef TYPE_XENOLTT_DRAWING_T_DEFINED
#define TYPE_XENOLTT_DRAWING_T_DEFINED
typedef struct _XenoLtt_Drawing_t XenoLtt_Drawing_t;
#endif //TYPE_XENOLTT_DRAWING_T_DEFINED

ThreadList *threadlist_construct(void);
void threadlist_destroy(ThreadList *thread_list);
GtkWidget *threadlist_get_widget(ThreadList *thread_list);

void threadlist_clear(ThreadList *thread_list);

// out : success (0) and height
/* CPU num is only used for PID 0 */
int threadlist_add(ThreadList *thread_list, XenoLtt_Drawing_t * drawing, 
    guint address24, guint prio, guint cpu, guint period,
    /*LttTime *process_birth,*/ LttTime *thread_birth, guint trace_num, GQuark name, guint *height,
    XenoThreadInfo **thread_info,
    HashedThreadData **hashed_thread_data);
// out : success (0) and height
int threadlist_remove(ThreadList *thread_list, guint pid, guint cpu, 
     LttTime *birth, guint trace_num);

/* Set the name of a xenomai thread */
void threadlist_set_name(ThreadList *thread_list,
    GQuark name,
    HashedThreadData *hashed_thread_data);

/* Set the period of a thread */
void threadlist_set_period(ThreadList *thread_list,
    guint period,
    HashedThreadData *hashed_thread_data);

/* Set the priority of a thread */
void threadlist_set_prio(ThreadList *thread_list,
    guint prio,
    HashedThreadData *hashed_thread_data);


/* Synchronize the list at the left and the drawing */
void update_index_to_pixmap(ThreadList *thread_list);

/* Update the width of each pixmap buffer for each process */
void update_pixmap_size(ThreadList *thread_list, guint width);


/* Put src and/or dest to NULL to copy from/to the each PID specific pixmap */
void copy_pixmap_region(ThreadList *thread_list, GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height);

/* If height is -1, the height of each pixmap is used */
void rectangle_pixmap(ThreadList *thread_list, GdkGC *gc,
    gboolean filled, gint x, gint y, gint width, gint height);

/* Renders each pixmaps into on big drawable */
void copy_pixmap_to_screen(ThreadList *thread_list,
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

static inline guint threadlist_get_height(ThreadList *thread_list)
{
  return thread_list->cell_height * thread_list->number_of_thread ;
}


static inline HashedThreadData *threadlist_get_thread_data( 
          ThreadList *thread_list,
          gulong address, guint cpu, LttTime *thread_birth, guint trace_num)
{
  XenoThreadInfo thread_info;

  thread_info.address = address;
  if(address == 0)
    thread_info.cpu = cpu;
  else
    thread_info.cpu = ANY_CPU;
  thread_info.thread_birth = *thread_birth; 
  thread_info.trace_num = trace_num;

  return  (HashedThreadData*)g_hash_table_lookup(thread_list->thread_hash,&thread_info);
}


static gboolean find_timer(gpointer key, gpointer value, gpointer user_data){
  const HashedThreadData *pa = (const HashedThreadData*)value;
  const gulong pb = (const gulong)user_data;

  return likely(pa->timer_address == pb);
}
     
static inline XenoThreadInfo *threadlist_get_thread_from_timer( 
          ThreadList *thread_list,gulong timer_address)
{
  return  (HashedThreadData*)g_hash_table_find(thread_list->thread_hash,find_timer,timer_address);
}

static inline gint threadlist_get_pixels_from_data(  ThreadList *thread_list,
          HashedThreadData *hashed_thread_data,
          guint *y,
          guint *height)
{
  gint *path_indices;
  GtkTreePath *tree_path;

  tree_path = gtk_tree_model_get_path((GtkTreeModel*)thread_list->list_store,
                    &hashed_thread_data->y_iter);
  path_indices =  gtk_tree_path_get_indices (tree_path);

  *height = get_cell_height((GtkTreeView*)thread_list->thread_list_widget);
  *y = *height * path_indices[0];
  gtk_tree_path_free(tree_path);

  return 0; 

}

static inline guint threadlist_get_index_from_data(ThreadList *thread_list,
          HashedThreadData *hashed_thread_data)
{
  gint *path_indices;
  GtkTreePath *tree_path;
  guint ret;

  tree_path = gtk_tree_model_get_path((GtkTreeModel*)thread_list->list_store,
                    &hashed_thread_data->y_iter);
  path_indices =  gtk_tree_path_get_indices (tree_path);

  ret = path_indices[0];

  gtk_tree_path_free(tree_path);

  return ret;
}



#endif // _THREAD_LIST_H
