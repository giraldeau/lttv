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

#include "processlist.h"
#include "drawitem.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)


/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/

/* Enumeration of the columns */
enum
{
  PROCESS_COLUMN,
  PID_COLUMN,
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
    // Final condition
    //if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
    //{
    //  g_value_unset(&a);
    //  g_value_unset(&b);
    //  return 0;
    //}

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

guint hash_fct(gconstpointer key)
{
  return ((ProcessInfo*)key)->pid;
}

gboolean equ_fct(gconstpointer a, gconstpointer b)
{
  if(((ProcessInfo*)a)->pid != ((ProcessInfo*)b)->pid)
    return 0;
//  g_critical("compare %u and %u",((ProcessInfo*)a)->pid,((ProcessInfo*)b)->pid);
  if(((ProcessInfo*)a)->birth.tv_sec != ((ProcessInfo*)b)->birth.tv_sec)
    return 0;
//  g_critical("compare %u and %u",((ProcessInfo*)a)->birth.tv_sec,((ProcessInfo*)b)->birth.tv_sec);

  if(((ProcessInfo*)a)->birth.tv_nsec != ((ProcessInfo*)b)->birth.tv_nsec)
    return 0;
//  g_critical("compare %u and %u",((ProcessInfo*)a)->birth.tv_nsec,((ProcessInfo*)b)->birth.tv_nsec);

  if(((ProcessInfo*)a)->trace_num != ((ProcessInfo*)b)->trace_num)
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

  /* Create the Process list */
  process_list->list_store = gtk_list_store_new (  N_COLUMNS,
              G_TYPE_STRING,
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
      hash_fct, equ_fct,
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

  column = gtk_tree_view_column_new_with_attributes ( "PID",
                renderer,
                "text",
                PID_COLUMN,
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
  g_info("processlist_destroy %p", process_list);
  g_hash_table_destroy(process_list->process_hash);
  process_list->process_hash = NULL;

  g_free(process_list);
  g_info("processlist_destroy end");
}

GtkWidget *processlist_get_widget(ProcessList *process_list)
{
  return process_list->process_list_widget;
}



gint get_cell_height(GtkTreeView *tree_view)
{
  gint height;
  GtkTreeViewColumn *Column = gtk_tree_view_get_column(tree_view, 0);
  //GList *Render_List = gtk_tree_view_column_get_cell_renderers(Column);
  //GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
  
  //g_list_free(Render_List);
  gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL, &height);
  //g_critical("cell 0 height : %u",height);
  
  return height;
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
      LttTime *birth,
      guint trace_num,
      const gchar *name,
      guint *height,
      HashedProcessData **pm_hashed_process_data)
{
  GtkTreeIter iter ;
  ProcessInfo *Process_Info = g_new(ProcessInfo, 1);
  HashedProcessData *hashed_process_data = g_new(HashedProcessData, 1);
  *pm_hashed_process_data = hashed_process_data;
  
  Process_Info->pid = pid;
  Process_Info->birth = *birth;
  Process_Info->trace_num = trace_num;
  
  hashed_process_data->draw_context = g_new(DrawContext, 1);
  hashed_process_data->draw_context->drawable = NULL;
  hashed_process_data->draw_context->gc = NULL;
  hashed_process_data->draw_context->pango_layout = NULL;
  hashed_process_data->draw_context->current = g_new(DrawInfo,1);
  hashed_process_data->draw_context->current->over = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->over->x = -1;
  hashed_process_data->draw_context->current->over->y = -1;
  hashed_process_data->draw_context->current->middle = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->middle->x = -1;
  hashed_process_data->draw_context->current->middle->y = -1;
  hashed_process_data->draw_context->current->under = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->under->x = -1;
  hashed_process_data->draw_context->current->under->y = -1;
  hashed_process_data->draw_context->current->modify_over = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->modify_over->x = -1;
  hashed_process_data->draw_context->current->modify_over->y = -1;
  hashed_process_data->draw_context->current->modify_middle = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->modify_middle->x = -1;
  hashed_process_data->draw_context->current->modify_middle->y = -1;
  hashed_process_data->draw_context->current->modify_under = g_new(ItemInfo,1);
  hashed_process_data->draw_context->current->modify_under->x = -1;
  hashed_process_data->draw_context->current->modify_under->y = -1;
  hashed_process_data->draw_context->current->status = LTTV_STATE_UNNAMED;
  hashed_process_data->draw_context->previous = g_new(DrawInfo,1);
  hashed_process_data->draw_context->previous->over = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->over->x = -1;
  hashed_process_data->draw_context->previous->over->y = -1;
  hashed_process_data->draw_context->previous->middle = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->middle->x = -1;
  hashed_process_data->draw_context->previous->middle->y = -1;
  hashed_process_data->draw_context->previous->under = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->under->x = -1;
  hashed_process_data->draw_context->previous->under->y = -1;
  hashed_process_data->draw_context->previous->modify_over = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->modify_over->x = -1;
  hashed_process_data->draw_context->previous->modify_over->y = -1;
  hashed_process_data->draw_context->previous->modify_middle = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->modify_middle->x = -1;
  hashed_process_data->draw_context->previous->modify_middle->y = -1;
  hashed_process_data->draw_context->previous->modify_under = g_new(ItemInfo,1);
  hashed_process_data->draw_context->previous->modify_under->x = -1;
  hashed_process_data->draw_context->previous->modify_under->y = -1;
  hashed_process_data->draw_context->previous->status = LTTV_STATE_UNNAMED;
  
  /* Add a new row to the model */
  gtk_list_store_append ( process_list->list_store, &iter);
  //g_critical ( "iter before : %s", gtk_tree_path_to_string (
  //    gtk_tree_model_get_path (
  //        GTK_TREE_MODEL(process_list->list_store),
  //        &iter)));
  gtk_list_store_set (  process_list->list_store, &iter,
        PROCESS_COLUMN, name,
        PID_COLUMN, pid,
        BIRTH_S_COLUMN, birth->tv_sec,
        BIRTH_NS_COLUMN, birth->tv_nsec,
        TRACE_COLUMN, trace_num,
        -1);
  hashed_process_data->row_ref = gtk_tree_row_reference_new (
      GTK_TREE_MODEL(process_list->list_store),
      gtk_tree_model_get_path(
        GTK_TREE_MODEL(process_list->list_store),
        &iter));
  g_hash_table_insert(  process_list->process_hash,
        (gpointer)Process_Info,
        (gpointer)hashed_process_data);
  
  //g_critical ( "iter after : %s", gtk_tree_path_to_string (
  //    gtk_tree_model_get_path (
  //        GTK_TREE_MODEL(process_list->list_store),
  //        &iter)));
  process_list->number_of_process++;

  *height = get_cell_height(GTK_TREE_VIEW(process_list->process_list_widget))
        * process_list->number_of_process ;
  
  
  return 0;
  
}

int processlist_remove( ProcessList *process_list,
      guint pid,
      LttTime *birth,
      guint trace_num)
{
  ProcessInfo Process_Info;
  gint *path_indices;
  HashedProcessData *hashed_process_data;
  GtkTreeIter iter;
  
  Process_Info.pid = pid;
  Process_Info.birth = *birth;
  Process_Info.trace_num = trace_num;


  if(hashed_process_data = 
    (HashedProcessData*)g_hash_table_lookup(
          process_list->process_hash,
          &Process_Info))
  {
    GtkTreePath *tree_path;

    tree_path = gtk_tree_row_reference_get_path(
                    hashed_process_data->row_ref);

    gtk_tree_model_get_iter (
        GTK_TREE_MODEL(process_list->list_store),
        &iter, tree_path);
 
    gtk_tree_path_free(tree_path);

    gtk_list_store_remove (process_list->list_store, &iter);
    
    g_free(hashed_process_data->draw_context->previous->modify_under);
    g_free(hashed_process_data->draw_context->previous->modify_middle);
    g_free(hashed_process_data->draw_context->previous->modify_over);
    g_free(hashed_process_data->draw_context->previous->under);
    g_free(hashed_process_data->draw_context->previous->middle);
    g_free(hashed_process_data->draw_context->previous->over);
    g_free(hashed_process_data->draw_context->previous);
    g_free(hashed_process_data->draw_context->current->modify_under);
    g_free(hashed_process_data->draw_context->current->modify_middle);
    g_free(hashed_process_data->draw_context->current->modify_over);
    g_free(hashed_process_data->draw_context->current->under);
    g_free(hashed_process_data->draw_context->current->middle);
    g_free(hashed_process_data->draw_context->current->over);
    g_free(hashed_process_data->draw_context->current);
    g_free(hashed_process_data->draw_context);
    g_free(hashed_process_data);

    g_hash_table_remove(process_list->process_hash,
        &Process_Info);
    
    process_list->number_of_process--;

    return 0; 
  } else {
    return 1;
  }
}


guint processlist_get_height(ProcessList *process_list)
{
  return get_cell_height(GTK_TREE_VIEW(process_list->process_list_widget))
        * process_list->number_of_process ;
}


gint processlist_get_process_pixels(  ProcessList *process_list,
          guint pid, LttTime *birth, guint trace_num,
          guint *y,
          guint *height,
          HashedProcessData **pm_hashed_process_data)
{
  ProcessInfo Process_Info;
  gint *path_indices;
  GtkTreePath *tree_path;
  HashedProcessData *hashed_process_data = NULL;

  Process_Info.pid = pid;
  Process_Info.birth = *birth;
  Process_Info.trace_num = trace_num;

  if(hashed_process_data = 
    (HashedProcessData*)g_hash_table_lookup(
          process_list->process_hash,
          &Process_Info))
  {
    tree_path = gtk_tree_row_reference_get_path(
                    hashed_process_data->row_ref);
    path_indices =  gtk_tree_path_get_indices (tree_path);

    *height = get_cell_height(
        GTK_TREE_VIEW(process_list->process_list_widget));
    *y = *height * path_indices[0];
    *pm_hashed_process_data = hashed_process_data;
    gtk_tree_path_free(tree_path);
    
    return 0; 
  } else {
    *pm_hashed_process_data = hashed_process_data;
    return 1;
  }

}


gint processlist_get_pixels_from_data(  ProcessList *process_list,
          ProcessInfo *process_info,
          HashedProcessData *hashed_process_data,
          guint *y,
          guint *height)
{
  gint *path_indices;
  GtkTreePath *tree_path;

  tree_path = gtk_tree_row_reference_get_path(
                  hashed_process_data->row_ref);
  path_indices =  gtk_tree_path_get_indices (tree_path);

  *height = get_cell_height(
      GTK_TREE_VIEW(process_list->process_list_widget));
  *y = *height * path_indices[0];
  gtk_tree_path_free(tree_path);

  return 0; 

}
