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

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "processlist.h"
#include "drawing.h"
#include "drawitem.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

/* Preallocated Size of the index_to_pixmap array */
#define ALLOCATE_PROCESSES 1000

/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/

//static inline guint get_cpu_number_from_name(GQuark name);
  
/* Enumeration of the columns */
enum
{
  PROCESS_COLUMN,
  PID_COLUMN,
  PPID_COLUMN,
  CPU_COLUMN,
  BIRTH_S_COLUMN,
  BIRTH_NS_COLUMN,
  TRACE_COLUMN,
  N_COLUMNS
};


gint process_sort_func  ( GtkTreeModel *model,
        GtkTreeIter *it_a,
        GtkTreeIter *it_b,
        gpointer user_data)
{
  gchar *a_name;
  guint a_pid, a_ppid, a_cpu;
  gulong a_birth_s, a_birth_ns;
  gulong a_trace;

  gchar *b_name;
  guint b_pid, b_ppid, b_cpu;
  gulong b_birth_s, b_birth_ns;
  gulong b_trace;

  gtk_tree_model_get(model,
           it_a,
           0, &a_name,
           1, &a_pid,
           2, &a_ppid,
           3, &a_cpu,
           4, &a_birth_s,
           5, &a_birth_ns,
           6, &a_trace,
           -1);

  gtk_tree_model_get(model,
           it_b,
           0, &b_name,
           1, &b_pid,
           2, &b_ppid,
           3, &b_cpu,
           4, &b_birth_s,
           5, &b_birth_ns,
           6, &b_trace,
           -1);

  
  /* Order by PID */
  if(a_pid == 0 &&  b_pid == 0) {
    /* If 0, order by CPU */
    if(a_cpu > b_cpu) return 1;
    if(a_cpu < b_cpu) return -1;

  } else { /* if not 0, order by pid */

    if(a_pid > b_pid) return 1;
    if(a_pid < b_pid) return -1;
  }

  /* Order by birth second */

  if(a_birth_s > b_birth_s) return 1;
  if(a_birth_s < b_birth_s) return -1;
  

  /* Order by birth nanosecond */
  if(a_birth_ns > b_birth_ns) return 1;
  if(a_birth_ns < b_birth_ns) return -1;
  
  /* Order by trace_num */
  if(a_trace > b_trace) return 1;
  if(a_trace < b_trace) return -1;

  return 0;

}

static guint process_list_hash_fct(gconstpointer key)
{
  guint pid = ((const ProcessInfo*)key)->pid;
  return ((pid>>8 ^ pid>>4 ^ pid>>2 ^ pid) ^ ((const ProcessInfo*)key)->cpu);
}

/* If hash is good, should be different */
static gboolean process_list_equ_fct(gconstpointer a, gconstpointer b)
{
  const ProcessInfo *pa = (const ProcessInfo*)a;
  const ProcessInfo *pb = (const ProcessInfo*)b;
  
  gboolean ret = TRUE;

  if(likely(pa->pid != pb->pid))
    ret = FALSE;
  else if(likely((pa->pid == 0 && (pa->cpu != pb->cpu))))
    ret = FALSE;
  else if(unlikely(ltt_time_compare(pa->birth, pb->birth) != 0))
    ret = FALSE;
  else if(unlikely(pa->trace_num != pb->trace_num))
    ret = FALSE;

  return ret;
}

void destroy_hash_key(gpointer key);

void destroy_hash_data(gpointer data);


static void update_index_to_pixmap_each(ProcessInfo *key,
                                        HashedProcessData *value,
                                        ProcessList *process_list)
{
  guint array_index = processlist_get_index_from_data(process_list, value);
  
  g_assert(array_index < process_list->index_to_pixmap->len);

  GdkPixmap **pixmap = 
    (GdkPixmap**)&g_ptr_array_index(process_list->index_to_pixmap, array_index);

  *pixmap = value->pixmap;
}


static void update_index_to_pixmap(ProcessList *process_list)
{
  g_ptr_array_set_size(process_list->index_to_pixmap,
                       g_hash_table_size(process_list->process_hash));
  g_hash_table_foreach(process_list->process_hash, 
                       (GHFunc)update_index_to_pixmap_each,
                       process_list);
}


static void update_pixmap_size_each(ProcessInfo *key,
                                    HashedProcessData *value,
                                    guint width)
{
  GdkPixmap *old_pixmap = value->pixmap;

  value->pixmap = 
        gdk_pixmap_new(old_pixmap,
                       width,
                       value->height,
                       -1);

  gdk_pixmap_unref(old_pixmap);
}


void update_pixmap_size(ProcessList *process_list, guint width)
{
  g_hash_table_foreach(process_list->process_hash, 
                       (GHFunc)update_pixmap_size_each,
                       (gpointer)width);
}


typedef struct _CopyPixmap {
  GdkDrawable *dest;
  GdkGC *gc;
  GdkDrawable *src;
  gint xsrc, ysrc, xdest, ydest, width, height;
} CopyPixmap;

static void copy_pixmap_region_each(ProcessInfo *key,
                                    HashedProcessData *value,
                                    CopyPixmap *cp)
{
  GdkPixmap *src = cp->src;
  GdkPixmap *dest = cp->dest;
  
  if(dest == NULL)
    dest = value->pixmap;
  if(src == NULL)
    src = value->pixmap;

  gdk_draw_drawable (dest,
      cp->gc,
      src,
      cp->xsrc, cp->ysrc,
      cp->xdest, cp->ydest,
      cp->width, cp->height);
}




void copy_pixmap_region(ProcessList *process_list, GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height)
{
  CopyPixmap cp = { dest, gc, src, xsrc, ysrc, xdest, ydest, width, height };
  
  g_hash_table_foreach(process_list->process_hash, 
                       (GHFunc)copy_pixmap_region_each,
                       &cp);
}



typedef struct _RectanglePixmap {
  gboolean filled;
  gint x, y, width, height;
  GdkGC *gc;
} RectanglePixmap;

static void rectangle_pixmap_each(ProcessInfo *key,
                                  HashedProcessData *value,
                                  RectanglePixmap *rp)
{
  if(rp->height == -1)
    rp->height = value->height;
      
  gdk_draw_rectangle (value->pixmap,
      rp->gc,
      rp->filled,
      rp->x, rp->y,
      rp->width, rp->height);
}




void rectangle_pixmap(ProcessList *process_list, GdkGC *gc,
    gboolean filled, gint x, gint y, gint width, gint height)
{
  RectanglePixmap rp = { filled, x, y, width, height, gc };
  
  g_hash_table_foreach(process_list->process_hash, 
                       (GHFunc)rectangle_pixmap_each,
                       &rp);
}


/* Renders each pixmaps into on big drawable */
void copy_pixmap_to_screen(ProcessList *process_list,
    GdkDrawable *dest,
    GdkGC *gc,
    gint x, gint y,
    gint width, gint height)
{
  if(process_list->index_to_pixmap->len == 0) return;
  guint cell_height = process_list->cell_height;

  //cell_height = 24; //FIXME
  /* Get indexes */
  gint begin = floor(y/(double)cell_height);
  gint end = MIN(ceil((y+height)/(double)cell_height),
                 process_list->index_to_pixmap->len);
  gint i;

  for(i=begin; i<end; i++) {
    g_assert(i<process_list->index_to_pixmap->len);
    /* Render the pixmap to the screen */
    GdkPixmap *pixmap = 
      (GdkPixmap*)g_ptr_array_index(process_list->index_to_pixmap, i);

    gdk_draw_drawable (dest,
        gc,
        pixmap,
        x, 0,
        x, i*cell_height,
        width, cell_height);

  }
  
  
}









ProcessList *processlist_construct(void)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  ProcessList* process_list = g_new(ProcessList,1);
  
  process_list->number_of_process = 0;

  process_list->current_hash_data = NULL;

  /* Create the Process list */
  process_list->list_store = gtk_list_store_new (  N_COLUMNS,
              G_TYPE_STRING,
              G_TYPE_UINT,
              G_TYPE_UINT,
              G_TYPE_UINT,
              G_TYPE_ULONG,
              G_TYPE_ULONG,
              G_TYPE_ULONG);


  process_list->process_list_widget = 
    gtk_tree_view_new_with_model
    (GTK_TREE_MODEL (process_list->list_store));

  g_object_unref (G_OBJECT (process_list->list_store));

  gtk_tree_sortable_set_default_sort_func(
      GTK_TREE_SORTABLE(process_list->list_store),
      process_sort_func,
      NULL,
      NULL);
  
  process_list->process_hash = g_hash_table_new_full(
      process_list_hash_fct, process_list_equ_fct,
      destroy_hash_key, destroy_hash_data
      );
  
  
  gtk_tree_view_set_headers_visible(
    GTK_TREE_VIEW(process_list->process_list_widget), TRUE);

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  process_list->renderer = renderer;

  gtk_cell_renderer_get_size(renderer,
      GTK_WIDGET(process_list->process_list_widget),
      NULL,
      NULL,
      NULL,
      NULL,
      &process_list->cell_height);
  
  guint ypad;
  g_object_get(G_OBJECT(renderer), "ypad", &ypad, NULL);

  process_list->cell_height += ypad;
  
  column = gtk_tree_view_column_new_with_attributes ( "Process",
                renderer,
                "text",
                PROCESS_COLUMN,
                NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);
  
  process_list->button = column->button;
  
  column = gtk_tree_view_column_new_with_attributes ( "PID",
                renderer,
                "text",
                PID_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);

  column = gtk_tree_view_column_new_with_attributes ( "PPID",
                renderer,
                "text",
                PPID_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);
  
  column = gtk_tree_view_column_new_with_attributes ( "CPU",
                renderer,
                "text",
                CPU_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);

  column = gtk_tree_view_column_new_with_attributes ( "Birth sec",
                renderer,
                "text",
                BIRTH_S_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);

  //gtk_tree_view_column_set_visible(column, 0);
  //
  column = gtk_tree_view_column_new_with_attributes ( "Birth nsec",
                renderer,
                "text",
                BIRTH_NS_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);

  column = gtk_tree_view_column_new_with_attributes ( "TRACE",
                renderer,
                "text",
                TRACE_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);


  //gtk_tree_view_column_set_visible(column, 0);
  
  g_object_set_data_full(
      G_OBJECT(process_list->process_list_widget),
      "process_list_Data",
      process_list,
      (GDestroyNotify)processlist_destroy);

  process_list->index_to_pixmap = g_ptr_array_sized_new(ALLOCATE_PROCESSES);
  
  return process_list;
}

void processlist_destroy(ProcessList *process_list)
{
  g_debug("processlist_destroy %p", process_list);
  g_hash_table_destroy(process_list->process_hash);
  process_list->process_hash = NULL;
  g_ptr_array_free(process_list->index_to_pixmap, TRUE);

  g_free(process_list);
  g_debug("processlist_destroy end");
}

static gboolean remove_hash_item(ProcessInfo *process_info,
                                 HashedProcessData *hashed_process_data,
                                 ProcessList *process_list)
{
  GtkTreeIter iter;

  iter = hashed_process_data->y_iter;

  gtk_list_store_remove (process_list->list_store, &iter);
  gdk_pixmap_unref(hashed_process_data->pixmap);

  if(likely(process_list->current_hash_data != NULL)) {
    if(likely(hashed_process_data ==
                process_list->current_hash_data[process_info->cpu]))
      process_list->current_hash_data[process_info->cpu] = NULL;
  }
  return TRUE; /* remove the element from the hash table */
}

void processlist_clear(ProcessList *process_list)
{
  g_info("processlist_clear %p", process_list);

  g_hash_table_foreach_remove(process_list->process_hash,
                              (GHRFunc)remove_hash_item,
                              (gpointer)process_list);
  process_list->number_of_process = 0;
  update_index_to_pixmap(process_list);
}


GtkWidget *processlist_get_widget(ProcessList *process_list)
{
  return process_list->process_list_widget;
}


void destroy_hash_key(gpointer key)
{
  g_free(key);
}

void destroy_hash_data(gpointer data)
{
  g_free(data);
}

int processlist_add(  ProcessList *process_list,
      Drawing_t *drawing,
      guint pid,
      guint cpu,
      guint ppid,
      LttTime *birth,
      guint trace_num,
      const gchar *name,
      guint *height,
      ProcessInfo **pm_process_info,
      HashedProcessData **pm_hashed_process_data)
{
  ProcessInfo *Process_Info = g_new(ProcessInfo, 1);
  HashedProcessData *hashed_process_data = g_new(HashedProcessData, 1);
  *pm_hashed_process_data = hashed_process_data;
  *pm_process_info = Process_Info;
  
  Process_Info->pid = pid;
  if(pid == 0)
    Process_Info->cpu = cpu;
  else
    Process_Info->cpu = 0;
  Process_Info->ppid = ppid;
  Process_Info->birth = *birth;
  Process_Info->trace_num = trace_num;

  /* When we create it from before state update, we are sure that the
   * last event occured before the beginning of the global area.
   *
   * If it is created after state update, this value (0) will be
   * overriden by the new state before anything is drawn.
   */
  hashed_process_data->x.over = 0;
  hashed_process_data->x.over_used = FALSE;
  hashed_process_data->x.over_marked = FALSE;
  hashed_process_data->x.middle = 0;
  hashed_process_data->x.middle_used = FALSE;
  hashed_process_data->x.middle_marked = FALSE;
  hashed_process_data->x.under = 0;
  hashed_process_data->x.under_used = FALSE;
  hashed_process_data->x.under_marked = FALSE;
  hashed_process_data->next_good_time = ltt_time_zero;
 
  /* Add a new row to the model */
  gtk_list_store_append ( process_list->list_store,
                          &hashed_process_data->y_iter);

  gtk_list_store_set (  process_list->list_store, &hashed_process_data->y_iter,
        PROCESS_COLUMN, name,
        PID_COLUMN, pid,
        PPID_COLUMN, ppid,
        CPU_COLUMN, cpu,
        BIRTH_S_COLUMN, birth->tv_sec,
        BIRTH_NS_COLUMN, birth->tv_nsec,
        TRACE_COLUMN, trace_num,
        -1);
  //gtk_tree_view_set_model(GTK_TREE_VIEW(process_list->process_list_widget),
  //                        GTK_TREE_MODEL(process_list->list_store));
  //gtk_container_resize_children(GTK_CONTAINER(process_list->process_list_widget));
  
  g_hash_table_insert(process_list->process_hash,
        (gpointer)Process_Info,
        (gpointer)hashed_process_data);
  
  process_list->number_of_process++;

  hashed_process_data->height = process_list->cell_height;

  //hashed_process_data->height = 24; // FIXME
  g_assert(hashed_process_data->height != 0);

  *height = hashed_process_data->height * process_list->number_of_process;

  hashed_process_data->pixmap = 
        gdk_pixmap_new(drawing->drawing_area->window,
                       drawing->alloc_width,
                       hashed_process_data->height,
                       -1);
  
  // Clear the image
  gdk_draw_rectangle (hashed_process_data->pixmap,
        drawing->drawing_area->style->black_gc,
        TRUE,
        0, 0,
        drawing->alloc_width,
        hashed_process_data->height);

  update_index_to_pixmap(process_list);


  return 0;
}

int processlist_remove( ProcessList *process_list,
      guint pid,
      guint cpu,
      LttTime *birth,
      guint trace_num)
{
  ProcessInfo process_info;
  HashedProcessData *hashed_process_data;
  GtkTreeIter iter;
  
  process_info.pid = pid;
  if(pid == 0)
    process_info.cpu = cpu;
  else
    process_info.cpu = 0;
  process_info.birth = *birth;
  process_info.trace_num = trace_num;


  hashed_process_data = 
    (HashedProcessData*)g_hash_table_lookup(
          process_list->process_hash,
          &process_info);
  if(likely(hashed_process_data != NULL))
  {
    iter = hashed_process_data->y_iter;

    gtk_list_store_remove (process_list->list_store, &iter);
    
    g_hash_table_remove(process_list->process_hash,
        &process_info);

    if(likely(process_list->current_hash_data != NULL)) {
      if(likely(hashed_process_data == process_list->current_hash_data[cpu])) {
        process_list->current_hash_data[cpu] = NULL;
      }
    }
    
    gdk_pixmap_unref(hashed_process_data->pixmap);
    
    update_index_to_pixmap(process_list);

    process_list->number_of_process--;

    return 0; 
  } else {
    return 1;
  }
}


#if 0
static inline guint get_cpu_number_from_name(GQuark name)
{
  const gchar *string;
  char *begin;
  guint cpu;

  string = g_quark_to_string(name);

  begin = strrchr(string, '/');
  begin++;

  g_assert(begin != '\0');

  cpu = strtoul(begin, NULL, 10);

  return cpu;
}
#endif //0
