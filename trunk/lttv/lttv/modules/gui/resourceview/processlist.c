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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "processlist.h"
#include "drawing.h"
#include "drawitem.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
//#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

/* Preallocated Size of the index_to_pixmap array */
#define ALLOCATE_PROCESSES 1000

/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/


gint resource_sort_func  ( GtkTreeModel *model,
        GtkTreeIter *it_a,
        GtkTreeIter *it_b,
        gpointer user_data)
{
  gchar *a_name;
  gchar *b_name;

  gtk_tree_model_get(model, it_a, NAME_COLUMN, &a_name, -1);

  gtk_tree_model_get(model, it_b, NAME_COLUMN, &b_name, -1);

  return strcmp(a_name, b_name);
}

static guint ru_numeric_hash_fct(gconstpointer key)
{
  ResourceUniqueNumeric *ru = (ResourceUniqueNumeric *)key;
  int tmp = (ru->trace_num << 8) ^ ru->id;

  return g_int_hash(&tmp);
}

static gboolean ru_numeric_equ_fct(gconstpointer a, gconstpointer b)
{
  const ResourceUniqueNumeric *pa = (const ResourceUniqueNumeric *)a;
  const ResourceUniqueNumeric  *pb = (const ResourceUniqueNumeric *)b;
  
  if(pa->id == pb->id && pa->trace_num == pb->trace_num)
    return TRUE;

  return FALSE;
}

void destroy_hash_key(gpointer key);

void destroy_hash_data(gpointer data);


gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
  ControlFlowData *control_flow_data = 
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "resourceview_data");
  Drawing_t *drawing = control_flow_data->drawing;
	unsigned int cell_height =
		get_cell_height(GTK_TREE_VIEW(control_flow_data->process_list->process_list_widget));

  switch(event->direction) {
    case GDK_SCROLL_UP:
      gtk_adjustment_set_value(control_flow_data->v_adjust,
        gtk_adjustment_get_value(control_flow_data->v_adjust) - cell_height);
      break;
    case GDK_SCROLL_DOWN:
      gtk_adjustment_set_value(control_flow_data->v_adjust,
        gtk_adjustment_get_value(control_flow_data->v_adjust) + cell_height);
      break;
    default:
      g_error("should only scroll up and down.");
  }
	return TRUE;
}

void expand_event(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *arg2, gpointer user_data)
{
  ControlFlowData *resourceview_data = 
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(treeview),
                "resourceview_data");
  ProcessList *process_list = (ProcessList *) user_data;
  ResourceUnique *rup;
  HashedResourceData *hrd;
  gboolean result;

  GtkTreeModel *model;
  GtkTreeIter child;

  /* Determine which trace has been expanded */
  model = gtk_tree_view_get_model(treeview);

  /* mark each of the trace's resources invisible */
  result = gtk_tree_model_iter_children(model, &child, iter);

  /* for each child of the collapsed row */
  while(result) {
    /* hide the item */
    gtk_tree_model_get(model, &child, DATA_COLUMN, &hrd, -1);
    hrd->hidden=0;

    /* find next */
    result = gtk_tree_model_iter_next(model, &child);
  }

  update_index_to_pixmap(process_list);

  gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
}

void collapse_event(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *arg2, gpointer user_data)
{
  ControlFlowData *resourceview_data = 
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(treeview),
                "resourceview_data");
  ProcessList *process_list = (ProcessList *) user_data;
  ResourceUnique *rup;
  HashedResourceData *hrd;
  gboolean result;

  GtkTreeModel *model;
  GtkTreeIter child;

  /* Determine which trace has been expanded */
  model = gtk_tree_view_get_model(treeview);

  /* mark each of the trace's resources invisible */
  result = gtk_tree_model_iter_children(model, &child, iter);

  /* for each child of the collapsed row */
  while(result) {
    char *name;
    /* hide the item */
    gtk_tree_model_get(model, &child, NAME_COLUMN, &name, DATA_COLUMN, &hrd, -1);
    hrd->hidden=1;

    /* find next */
    result = gtk_tree_model_iter_next(model, &child);
  }

  update_index_to_pixmap(process_list);

  gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
}

static gboolean update_index_to_pixmap_each (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, UpdateIndexPixmapArg *arg) 
{
  guint array_index = arg->count;
  HashedResourceData *hdata;
  gchar *name;
  
  gtk_tree_model_get(model, iter, NAME_COLUMN, &name, DATA_COLUMN, &hdata, -1);
  
  g_assert(array_index < arg->process_list->index_to_pixmap->len);
  
  if(hdata->hidden == 0) {
    GdkPixmap **pixmap =
      (GdkPixmap**)&g_ptr_array_index(arg->process_list->index_to_pixmap, array_index);
    *pixmap = hdata->pixmap;

    arg->count++;
  }

  return FALSE;
}

void update_index_to_pixmap(ProcessList *process_list)
{
  int i, items=0;
  UpdateIndexPixmapArg arg;

  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    items += g_hash_table_size(process_list->restypes[i].hash_table);
  }

  /* we don't know the exact number of items there will be,
   * so set an upper bound */
  g_ptr_array_set_size(process_list->index_to_pixmap, items);

  arg.count = 0;
  arg.process_list = process_list;

  /* If cell_height is still 0, the only element in the tree is a temporary
   * element that has no pixmap, see also processlist_construct() */
  if (process_list->cell_height != 0) {
    gtk_tree_model_foreach(GTK_TREE_MODEL(process_list->list_store),
      (GtkTreeModelForeachFunc)update_index_to_pixmap_each, &arg);
  }

  /* now that we know the exact number of items, set it */
  g_ptr_array_set_size(process_list->index_to_pixmap, arg.count);
}


static void update_pixmap_size_each(void *key,
                                    HashedResourceData *value,
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
  int i;
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(process_list->restypes[i].hash_table,
			 (GHFunc)update_pixmap_size_each,
			 GUINT_TO_POINTER(width));
  }
}


typedef struct _CopyPixmap {
  GdkDrawable *dest;
  GdkGC *gc;
  GdkDrawable *src;
  gint xsrc, ysrc, xdest, ydest, width, height;
} CopyPixmap;

static void copy_pixmap_region_each(void *key,
                                    HashedResourceData *value,
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
  int i;
  CopyPixmap cp = { dest, gc, src, xsrc, ysrc, xdest, ydest, width, height };
  
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(process_list->restypes[i].hash_table, 
                       (GHFunc)copy_pixmap_region_each,
                       &cp);
  }
}



typedef struct _RectanglePixmap {
  gboolean filled;
  gint x, y, width, height;
  GdkGC *gc;
} RectanglePixmap;

static void rectangle_pixmap_each(void *key,
                                  HashedResourceData *value,
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
  int i;
  RectanglePixmap rp = { filled, x, y, width, height, gc };
  
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_foreach(process_list->restypes[i].hash_table, 
                       (GHFunc)rectangle_pixmap_each,
                       &rp);
  }
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

  /* Get indexes */
  gint begin = floor(y/(double)cell_height);
  gint end = MIN(ceil((y+height)/(double)cell_height),
                 process_list->index_to_pixmap->len);
  gint i;

  for(i=begin; i<end; i++) {
    g_assert(i<process_list->index_to_pixmap->len);
    /* Render the pixmap to the screen */
    GdkPixmap *pixmap = 
      //(GdkPixmap*)g_ptr_array_index(process_list->index_to_pixmap, i);
      GDK_PIXMAP(g_ptr_array_index(process_list->index_to_pixmap, i));

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
  process_list->list_store = gtk_tree_store_new (  N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);

  process_list->process_list_widget = 
    gtk_tree_view_new_with_model
    (GTK_TREE_MODEL (process_list->list_store));
  g_object_set(process_list->process_list_widget, "enable-tree-lines", TRUE, NULL);

  g_object_unref (G_OBJECT (process_list->list_store));

  gtk_tree_sortable_set_default_sort_func(
      GTK_TREE_SORTABLE(process_list->list_store),
      resource_sort_func,
      NULL,
      NULL);
 

  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(process_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);

  gtk_tree_view_set_headers_visible(
    GTK_TREE_VIEW(process_list->process_list_widget), TRUE);

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  process_list->renderer = renderer;

  g_signal_connect(process_list->process_list_widget, "row-expanded", G_CALLBACK(expand_event), process_list);
  g_signal_connect(process_list->process_list_widget, "row-collapsed", G_CALLBACK(collapse_event), process_list);

  /* Add a temporary row to the model to get the cell size when the first
  * real process is added. */
  GtkTreeIter iter;
  GtkTreePath *path;
  path = gtk_tree_path_new_first();
  gtk_tree_model_get_iter (gtk_tree_view_get_model(GTK_TREE_VIEW(
    process_list->process_list_widget)), &iter, path);
  gtk_tree_store_append(process_list->list_store, &iter, NULL);
  gtk_tree_path_free(path);

  process_list->cell_height = 0;	// not ready to get size yet.

  column = gtk_tree_view_column_new_with_attributes ( "Resource",
                renderer,
                "text",
                NAME_COLUMN,
                NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (process_list->process_list_widget), column);
  
  process_list->button = column->button;
  
  g_object_set_data_full(
      G_OBJECT(process_list->process_list_widget),
      "process_list_Data",
      process_list,
      (GDestroyNotify)processlist_destroy);

  process_list->index_to_pixmap = g_ptr_array_sized_new(ALLOCATE_PROCESSES);

  process_list->restypes[RV_RESOURCE_MACHINE].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  process_list->restypes[RV_RESOURCE_CPU].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  process_list->restypes[RV_RESOURCE_IRQ].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  process_list->restypes[RV_RESOURCE_SOFT_IRQ].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  process_list->restypes[RV_RESOURCE_TRAP].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  process_list->restypes[RV_RESOURCE_BDEV].hash_table = g_hash_table_new(ru_numeric_hash_fct, ru_numeric_equ_fct);
  
  return process_list;
}

void processlist_destroy(ProcessList *process_list)
{
  int i;

  g_debug("processlist_destroy %p", process_list);
  
  for(i=0; i<RV_RESOURCE_COUNT; i++) {
    g_hash_table_destroy(process_list->restypes[i].hash_table);
    process_list->restypes[i].hash_table = NULL;
  }
  g_ptr_array_free(process_list->index_to_pixmap, TRUE);

  g_free(process_list);
  g_debug("processlist_destroy end");
}

static gboolean remove_hash_item(void *key,
                                 HashedResourceData *hashed_process_data,
                                 ProcessList *process_list)
{
  GtkTreeIter iter;

  iter = hashed_process_data->y_iter;

  gtk_tree_store_remove (process_list->list_store, &iter);
  gdk_pixmap_unref(hashed_process_data->pixmap);

// TODO pmf: check this; might be needed
//  if(likely(process_list->current_hash_data != NULL)) {
//    if(likely(hashed_process_data ==
//                process_list->current_hash_data[process_info->trace_num][process_info->cpu]))
//      process_list->current_hash_data[process_info->trace_num][process_info->cpu] = NULL;
//  }
  return TRUE; /* remove the element from the hash table */
}

void processlist_clear(ProcessList *process_list)
{
  int i;

  g_info("processlist_clear %p", process_list);

  for(i=RV_RESOURCE_COUNT-1; i>=0; i--) {
    g_hash_table_foreach_remove(process_list->restypes[i].hash_table,
                                (GHRFunc)remove_hash_item,
                                (gpointer)process_list);
  }
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

GQuark make_cpu_name(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  GQuark name;
  gchar *str;

  str = g_strdup_printf("CPU%u", id);
  name = g_quark_from_string(str);
  g_free(str);

  return name;
}

GQuark make_irq_name(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  GQuark name;
  gchar *str;

  str = g_strdup_printf("IRQ %u", id);
  name = g_quark_from_string(str);
  g_free(str);

  return name;
}

GQuark make_soft_irq_name(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  GQuark name;
  gchar *str;

  str = g_strdup_printf("SOFTIRQ %u", id);
  name = g_quark_from_string(str);
  g_free(str);

  return name;
}

GQuark make_trap_name(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  GQuark name;
  gchar *str;

  str = g_strdup_printf("Trap %u", id);
  name = g_quark_from_string(str);
  g_free(str);

  return name;
}

GQuark make_bdev_name(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  GQuark name;
  gchar *str;

  str = g_strdup_printf("Block (%u,%u)", MAJOR(id), MINOR(id));
  name = g_quark_from_string(str);
  g_free(str);

  return name;
}

HashedResourceData *resourcelist_obtain_machine(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  ResourceUniqueNumeric *ru = g_new(ResourceUniqueNumeric, 1);
  HashedResourceData *data = g_new(HashedResourceData, 1);
  
  /* Prepare hash key */
  ru->trace_num = trace_num;
  ru->id = id;
  
  /* Search within hash table */
  GHashTable *ht = resourceview_data->process_list->restypes[RV_RESOURCE_MACHINE].hash_table;
  data = g_hash_table_lookup(ht, ru);
  
  /* If not found in hash table, add it */
  if(data == NULL) {
    GQuark name;

    data = g_malloc(sizeof(HashedResourceData));
    /* Prepare hashed data */
    data->type = RV_RESOURCE_MACHINE;
    data->x.over = 0;
    data->x.over_used = FALSE;
    data->x.over_marked = FALSE;
    data->x.middle = 0; // last 
    data->x.middle_used = FALSE;
    data->x.middle_marked = FALSE;
    data->x.under = 0;
    data->x.under_used = FALSE;
    data->x.under_marked = FALSE;
    data->next_good_time = ltt_time_zero;
    data->hidden = 0;

    if (resourceview_data->process_list->cell_height == 0) {
        GtkTreePath *path;
        GdkRectangle rect;
        GtkTreeIter iter;

        path = gtk_tree_path_new_first();
        gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(
            resourceview_data->process_list->process_list_widget)),
            &iter, path);
        gtk_tree_view_get_background_area(GTK_TREE_VIEW(
            resourceview_data->process_list->process_list_widget),
            path, NULL, &rect);
        gtk_tree_store_remove(resourceview_data->process_list->list_store,
            &iter);
        gtk_tree_path_free(path);
        resourceview_data->process_list->cell_height = rect.height;
    }

    data->height = resourceview_data->process_list->cell_height;
    data->pixmap = 
        gdk_pixmap_new(resourceview_data->drawing->drawing_area->window,
                       resourceview_data->drawing->alloc_width,
                       data->height,
                       -1);
    g_assert(data->pixmap);

    gdk_draw_rectangle (data->pixmap,
        resourceview_data->drawing->drawing_area->style->black_gc,
        TRUE,
        0, 0,
        resourceview_data->drawing->alloc_width,
        data->height);

    /* add to hash table */
    g_hash_table_insert(ht, ru, data);
    resourceview_data->process_list->number_of_process++; // TODO: check

    /* add to process list */
    {
      gchar *str;
      str = g_strdup_printf("Trace %u", id);
      name = g_quark_from_string(str);
      g_free(str);
    }

    gtk_tree_store_append(resourceview_data->process_list->list_store, &data->y_iter, NULL);
    gtk_tree_store_set(resourceview_data->process_list->list_store, &data->y_iter,
         NAME_COLUMN, g_quark_to_string(name), DATA_COLUMN, data,
         -1);

    update_index_to_pixmap(resourceview_data->process_list);

    int heightall = data->height * resourceview_data->process_list->number_of_process;

    gtk_widget_set_size_request(resourceview_data->drawing->drawing_area,
                              -1,
                              heightall);

    gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
  }

  /* expand the newly added machine */
  {
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(resourceview_data->process_list->process_list_widget));
    GtkTreePath *path;
    path = gtk_tree_model_get_path(model, &data->y_iter);

    gtk_tree_view_expand_row(GTK_TREE_VIEW(resourceview_data->process_list->process_list_widget), path, FALSE);

    gtk_tree_path_free(path);
  }

  return data;
}

HashedResourceData *resourcelist_obtain_generic(ControlFlowData *resourceview_data, gint res_type, guint trace_num, guint id, GQuark (*make_name_func)(ControlFlowData *resourceview_data, guint trace_num, guint id))
{
  ResourceUniqueNumeric *ru = g_new(ResourceUniqueNumeric, 1);
  HashedResourceData *data = g_new(HashedResourceData, 1);
  
  /* Prepare hash key */
  ru->ru.type = &(resourceview_data->process_list->restypes[res_type]);
  ru->trace_num = trace_num;
  ru->id = id;
  
  /* Search within hash table */
  GHashTable *ht = resourceview_data->process_list->restypes[res_type].hash_table;
  data = g_hash_table_lookup(ht, ru);
  
  /* If not found in hash table, add it */
  if(data == NULL) {
    GQuark name;
    HashedResourceData *parent;

    /* Find the parent machine */
    parent = resourcelist_obtain_machine(resourceview_data, trace_num, trace_num);

    /* Prepare hashed data */
    data = g_malloc(sizeof(HashedResourceData));

    data->type = res_type;
    data->x.over = 0;
    data->x.over_used = FALSE;
    data->x.over_marked = FALSE;
    data->x.middle = 0; // last 
    data->x.middle_used = FALSE;
    data->x.middle_marked = FALSE;
    data->x.under = 0;
    data->x.under_used = FALSE;
    data->x.under_marked = FALSE;
    data->next_good_time = ltt_time_zero;

    if (resourceview_data->process_list->cell_height == 0) {
        GtkTreePath *path;
        GdkRectangle rect;
        GtkTreeIter iter;

        path = gtk_tree_path_new_first();
        gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(
            resourceview_data->process_list->process_list_widget)), &iter,
            path);
        gtk_tree_view_get_background_area(GTK_TREE_VIEW(
            resourceview_data->process_list->process_list_widget), path,
            NULL, &rect);
        gtk_tree_store_remove(resourceview_data->process_list->list_store,
            &iter);
        gtk_tree_path_free(path);
        resourceview_data->process_list->cell_height = rect.height;
    }

    data->height = resourceview_data->process_list->cell_height;
    data->pixmap = 
        gdk_pixmap_new(resourceview_data->drawing->drawing_area->window,
                       resourceview_data->drawing->alloc_width,
                       data->height,
                       -1);

    gdk_draw_rectangle (data->pixmap,
        resourceview_data->drawing->drawing_area->style->black_gc,
        TRUE,
        0, 0,
        resourceview_data->drawing->alloc_width,
        data->height);

    /* add to hash table */
    g_hash_table_insert(ht, ru, data);
    resourceview_data->process_list->number_of_process++; // TODO: check

    /* add to process list */
    name = make_name_func(resourceview_data, trace_num, id);

    gtk_tree_store_append(resourceview_data->process_list->list_store, &data->y_iter, &parent->y_iter);
    gtk_tree_store_set(resourceview_data->process_list->list_store, &data->y_iter,
         NAME_COLUMN, g_quark_to_string(name), DATA_COLUMN, data,
         -1);

    /* Determine if we should add it hidden or not */
    {
      gboolean result;
      GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(resourceview_data->process_list->process_list_widget));
      GtkTreeIter parent_iter;

      result = gtk_tree_model_iter_parent(model, &parent_iter, &data->y_iter);
      GtkTreePath *path = gtk_tree_model_get_path(model, &parent_iter);
      data->hidden = gtk_tree_view_row_expanded(GTK_TREE_VIEW(resourceview_data->process_list->process_list_widget), path)?0:1;
      gtk_tree_path_free(path);
    }


    update_index_to_pixmap(resourceview_data->process_list);

    int heightall = data->height * resourceview_data->process_list->number_of_process;

    gtk_widget_set_size_request(resourceview_data->drawing->drawing_area,
                              -1,
                              heightall);

    gtk_widget_queue_draw(resourceview_data->drawing->drawing_area);
  }

  return data;
}

HashedResourceData *resourcelist_obtain_cpu(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  return resourcelist_obtain_generic(resourceview_data, RV_RESOURCE_CPU, trace_num, id, make_cpu_name);
}

HashedResourceData *resourcelist_obtain_irq(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  return resourcelist_obtain_generic(resourceview_data, RV_RESOURCE_IRQ, trace_num, id, make_irq_name);
}

HashedResourceData *resourcelist_obtain_soft_irq(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  return resourcelist_obtain_generic(resourceview_data, RV_RESOURCE_SOFT_IRQ, trace_num, id, make_soft_irq_name);
}

HashedResourceData *resourcelist_obtain_trap(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  return resourcelist_obtain_generic(resourceview_data, RV_RESOURCE_TRAP, trace_num, id, make_trap_name);
}

HashedResourceData *resourcelist_obtain_bdev(ControlFlowData *resourceview_data, guint trace_num, guint id)
{
  return resourcelist_obtain_generic(resourceview_data, RV_RESOURCE_BDEV, trace_num, id, make_bdev_name);
}

