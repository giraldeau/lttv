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

#include "processlist.h"
#include "drawitem.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)


/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/

static __inline guint get_cpu_number_from_name(GQuark name);
  
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
  GValue a, b;

  memset(&a, 0, sizeof(GValue));
  memset(&b, 0, sizeof(GValue));
  
  /* Order by PID */
  gtk_tree_model_get_value( model,
          it_a,
          PID_COLUMN,
          &a);

  gtk_tree_model_get_value( model,
          it_b,
          PID_COLUMN,
          &b);

  if(G_VALUE_TYPE(&a) == G_TYPE_UINT
    && G_VALUE_TYPE(&b) == G_TYPE_UINT )
  {
    {

      if(g_value_get_uint(&a) == 0 &&  g_value_get_uint(&b) == 0) {

        GValue cpua, cpub;

        memset(&cpua, 0, sizeof(GValue));
        memset(&cpub, 0, sizeof(GValue));
       
        /* If 0, order by CPU */
        gtk_tree_model_get_value( model,
                it_a,
                CPU_COLUMN,
                &cpua);

        gtk_tree_model_get_value( model,
                it_b,
                CPU_COLUMN,
                &cpub);

        if(G_VALUE_TYPE(&cpua) == G_TYPE_UINT
          && G_VALUE_TYPE(&cpub) == G_TYPE_UINT )
        {
          if(g_value_get_uint(&cpua) > g_value_get_uint(&cpub))
          {
            g_value_unset(&cpua);
            g_value_unset(&cpub);
            return 1;
          }
          if(g_value_get_uint(&cpua) < g_value_get_uint(&cpub))
          {
            g_value_unset(&cpua);
            g_value_unset(&cpub);
            return 0;
          }
        }

        g_value_unset(&cpua);
        g_value_unset(&cpub);

      } else { /* if not 0, order by pid */
      
        if(g_value_get_uint(&a) > g_value_get_uint(&b))
        {
          g_value_unset(&a);
          g_value_unset(&b);
          return 1;
        }
        if(g_value_get_uint(&a) < g_value_get_uint(&b))
        {
          g_value_unset(&a);
          g_value_unset(&b);
          return 0;
        }
      }
    }
  }

  g_value_unset(&a);
  g_value_unset(&b);


  /* Order by birth second */
  gtk_tree_model_get_value( model,
          it_a,
          BIRTH_S_COLUMN,
          &a);

  gtk_tree_model_get_value( model,
          it_b,
          BIRTH_S_COLUMN,
          &b);


  if(G_VALUE_TYPE(&a) == G_TYPE_ULONG
    && G_VALUE_TYPE(&b) == G_TYPE_ULONG )
  {
    if(g_value_get_ulong(&a) > g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 1;
    }
    if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 0;
    }

  }

  g_value_unset(&a);
  g_value_unset(&b);

  /* Order by birth nanosecond */
  gtk_tree_model_get_value( model,
          it_a,
          BIRTH_NS_COLUMN,
          &a);

  gtk_tree_model_get_value( model,
          it_b,
          BIRTH_NS_COLUMN,
          &b);


  if(G_VALUE_TYPE(&a) == G_TYPE_ULONG
    && G_VALUE_TYPE(&b) == G_TYPE_ULONG )
  {
    if(g_value_get_ulong(&a) > g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 1;
    }
    if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 0;
    }

  }
  
  g_value_unset(&a);
  g_value_unset(&b);

  /* Order by trace_num */
  gtk_tree_model_get_value( model,
          it_a,
          TRACE_COLUMN,
          &a);

  gtk_tree_model_get_value( model,
          it_b,
          TRACE_COLUMN,
          &b);

  if(G_VALUE_TYPE(&a) == G_TYPE_ULONG
    && G_VALUE_TYPE(&b) == G_TYPE_ULONG )
  {
    if(g_value_get_ulong(&a) > g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 1;
    }
    if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
    {
      g_value_unset(&a);
      g_value_unset(&b);
      return 0;
    }

  }

  return 0;

}

static guint process_list_hash_fct(gconstpointer key)
{
  guint pid = ((ProcessInfo*)key)->pid;
  return ((pid>>8 ^ pid>>4 ^ pid>>2 ^ pid) ^ ((ProcessInfo*)key)->cpu);
}

static gboolean process_list_equ_fct(gconstpointer a, gconstpointer b)
{
  const ProcessInfo *pa = (const ProcessInfo*)a;
  const ProcessInfo *pb = (const ProcessInfo*)b;
  
  if(pa->pid != pb->pid)
    return 0;

  if((pa->pid == 0 && (pa->cpu != pb->cpu)))
    return 0;

  if(pa->birth.tv_sec != pb->birth.tv_sec)
    return 0;

  if(pa->birth.tv_nsec != pb->birth.tv_nsec)
    return 0;

  if(pa->trace_num != pb->trace_num)
    return 0;

  return 1;
}

void destroy_hash_key(gpointer key);

void destroy_hash_data(gpointer data);




ProcessList *processlist_construct(void)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  ProcessList* process_list = g_new(ProcessList,1);
  
  process_list->number_of_process = 0;
  process_list->cell_height_cache = -1;

  process_list->current_process_info = NULL;
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

  gtk_tree_sortable_set_sort_func(
      GTK_TREE_SORTABLE(process_list->list_store),
      PID_COLUMN,
      process_sort_func,
      NULL,
      NULL);
  
  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(process_list->list_store),
      PID_COLUMN,
      GTK_SORT_ASCENDING);
  
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

  return process_list;
}

void processlist_destroy(ProcessList *process_list)
{
  g_debug("processlist_destroy %p", process_list);
  g_hash_table_destroy(process_list->process_hash);
  process_list->process_hash = NULL;

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

  if(process_info == process_list->current_process_info)
    process_list->current_process_info = NULL;
  if(hashed_process_data == process_list->current_hash_data)
    process_list->current_hash_data = NULL;

  return TRUE; /* remove the element from the hash table */
}

void processlist_clear(ProcessList *process_list)
{
  g_info("processlist_clear %p", process_list);

  g_hash_table_foreach_remove(process_list->process_hash,
                              (GHRFunc)remove_hash_item,
                              (gpointer)process_list);
  process_list->number_of_process = 0;
}


GtkWidget *processlist_get_widget(ProcessList *process_list)
{
  return process_list->process_list_widget;
}



static __inline gint get_cell_height(ProcessList *process_list, GtkTreeView *tree_view)
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

void destroy_hash_key(gpointer key)
{
  g_free(key);
}

void destroy_hash_data(gpointer data)
{
  g_free(data);
}

int processlist_add(  ProcessList *process_list,
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
        CPU_COLUMN, get_cpu_number_from_name(cpu),
        BIRTH_S_COLUMN, birth->tv_sec,
        BIRTH_NS_COLUMN, birth->tv_nsec,
        TRACE_COLUMN, trace_num,
        -1);
#if 0
  hashed_process_data->row_ref = gtk_tree_row_reference_new (
      GTK_TREE_MODEL(process_list->list_store),
      gtk_tree_model_get_path(
        GTK_TREE_MODEL(process_list->list_store),
        &iter));
#endif //0
  g_hash_table_insert(process_list->process_hash,
        (gpointer)Process_Info,
        (gpointer)hashed_process_data);
  
  //g_critical ( "iter after : %s", gtk_tree_path_to_string (
  //    gtk_tree_model_get_path (
  //        GTK_TREE_MODEL(process_list->list_store),
  //        &iter)));
  process_list->number_of_process++;

  *height = get_cell_height(process_list,
                            GTK_TREE_VIEW(process_list->process_list_widget))
        * process_list->number_of_process ;
  
  return 0;
}

int processlist_remove( ProcessList *process_list,
      guint pid,
      guint cpu,
      LttTime *birth,
      guint trace_num)
{
  ProcessInfo process_info;
  gint *path_indices;
  HashedProcessData *hashed_process_data;
  GtkTreeIter iter;
  
  process_info.pid = pid;
  if(pid == 0)
    process_info.cpu = cpu;
  else
    process_info.cpu = 0;
  process_info.birth = *birth;
  process_info.trace_num = trace_num;


  if(hashed_process_data = 
    (HashedProcessData*)g_hash_table_lookup(
          process_list->process_hash,
          &process_info))
  {
    iter = hashed_process_data->y_iter;

    gtk_list_store_remove (process_list->list_store, &iter);
    
    g_hash_table_remove(process_list->process_hash,
        &process_info);

    if(hashed_process_data == process_list->current_hash_data) {
      process_list->current_process_info = NULL;
      process_list->current_hash_data = NULL;
    }

    process_list->number_of_process--;

    return 0; 
  } else {
    return 1;
  }
}


guint processlist_get_height(ProcessList *process_list)
{
  return get_cell_height(process_list,
                         (GtkTreeView*)process_list->process_list_widget)
        * process_list->number_of_process ;
}


__inline gint processlist_get_process_pixels(  ProcessList *process_list,
          guint pid, guint cpu, LttTime *birth, guint trace_num,
          guint *y,
          guint *height,
          HashedProcessData **pm_hashed_process_data)
{
  ProcessInfo process_info;
  gint *path_indices;
  GtkTreePath *tree_path;
  HashedProcessData *hashed_process_data = NULL;

  process_info.pid = pid;
  if(pid == 0)
    process_info.cpu = cpu;
  else
    process_info.cpu = 0;
  process_info.birth = *birth;
  process_info.trace_num = trace_num;

  if(hashed_process_data = 
    (HashedProcessData*)g_hash_table_lookup(
          process_list->process_hash,
          &process_info))
  {
    tree_path = gtk_tree_model_get_path(
                    (GtkTreeModel*)process_list->list_store,
                    &hashed_process_data->y_iter);
    path_indices =  gtk_tree_path_get_indices (tree_path);

    *height = get_cell_height(process_list,
        (GtkTreeView*)process_list->process_list_widget);
    *y = *height * path_indices[0];
    *pm_hashed_process_data = hashed_process_data;
    gtk_tree_path_free(tree_path);
    
    return 0; 
  } else {
    *pm_hashed_process_data = hashed_process_data;
    return 1;
  }

}


__inline gint processlist_get_pixels_from_data(  ProcessList *process_list,
          ProcessInfo *process_info,
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

static __inline guint get_cpu_number_from_name(GQuark name)
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

