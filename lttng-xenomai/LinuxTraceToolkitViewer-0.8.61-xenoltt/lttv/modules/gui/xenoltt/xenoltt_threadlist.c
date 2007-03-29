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

#include "xenoltt_threadlist.h"
#include "xenoltt_drawing.h"
#include "xenoltt_drawitem.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

/* Preallocated Size of the index_to_pixmap array */
#define ALLOCATE_PROCESSES 1000

/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/


gint thread_sort_func  ( GtkTreeModel *model,
        GtkTreeIter *it_a,
        GtkTreeIter *it_b,
        gpointer user_data)
{
  gchar *a_name;
  guint a_prio, a_period, a_cpu;
  gulong a_birth_s, a_birth_ns;
  gulong a_trace;

  gchar *b_name;
  guint b_prio, b_period, b_cpu;
  gulong b_birth_s, b_birth_ns;
  gulong b_trace;

  gtk_tree_model_get(model,
           it_a,
           THREAD_COLUMN, &a_name,
           PRIO_COLUMN, &a_prio,
           PERIOD_COLUMN, &a_period,
           CPU_COLUMN, &a_cpu,
           BIRTH_S_COLUMN, &a_birth_s,
           BIRTH_NS_COLUMN, &a_birth_ns,
           TRACE_COLUMN, &a_trace,
           -1);

  gtk_tree_model_get(model,
           it_b,
           THREAD_COLUMN, &b_name,
           PRIO_COLUMN, &b_prio,
           PERIOD_COLUMN, &b_period,
           CPU_COLUMN, &b_cpu,
           BIRTH_S_COLUMN, &b_birth_s,
           BIRTH_NS_COLUMN, &b_birth_ns,
           TRACE_COLUMN, &b_trace,
           -1);


  /* Order by PRIORITY */
  if(a_prio == 0 &&  b_prio == 0) {
    if(a_prio > b_prio) return -1;
    if(a_prio < b_prio) return 1;
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

static guint thread_list_hash_fct(gconstpointer key)
{
  guint address = ((const XenoThreadInfo*)key)->address;
  return ((address>>8 ^ address>>4 ^ address>>2 ^ address) ^ ((const XenoThreadInfo*)key)->cpu);
}

/* If hash is good, should be different */
static gboolean thread_list_equ_fct(gconstpointer a, gconstpointer b)
{
  const XenoThreadInfo *pa = (const XenoThreadInfo*)a;
  const XenoThreadInfo *pb = (const XenoThreadInfo*)b;

  gboolean ret = TRUE;

  if(likely(pa->address != pb->address))
    ret = FALSE;
  if(likely((pa->address == 0)))
    ret = FALSE;
  if(likely((pa->address == 0 && (pa->cpu != pb->cpu))))
    ret = FALSE;
  if(unlikely(ltt_time_compare(pa->thread_birth, pb->thread_birth) != 0))
    ret = FALSE; 
  if(unlikely(pa->trace_num != pb->trace_num))
    ret = FALSE;

  return ret;
}

void destroy_hash_key(gpointer key);

void destroy_hash_data(gpointer data);


gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
  XenoLTTData *xenoltt_data =
      (XenoLTTData*)g_object_get_data(
                G_OBJECT(widget),
                "xenoltt_data");
  XenoLtt_Drawing_t *drawing = xenoltt_data->drawing;
	unsigned int cell_height =
		get_cell_height(GTK_TREE_VIEW(xenoltt_data->thread_list->thread_list_widget));

  switch(event->direction) {
    case GDK_SCROLL_UP:
      gtk_adjustment_set_value(xenoltt_data->v_adjust,
        gtk_adjustment_get_value(xenoltt_data->v_adjust) - cell_height);
      break;
    case GDK_SCROLL_DOWN:
      gtk_adjustment_set_value(xenoltt_data->v_adjust,
        gtk_adjustment_get_value(xenoltt_data->v_adjust) + cell_height);
      break;
    default:
      g_error("should only scroll up and down.");
  }
	return TRUE;
}


static void update_index_to_pixmap_each(XenoThreadInfo *key,
                                        HashedThreadData *value,
                                        ThreadList *thread_list)
{
  guint array_index = threadlist_get_index_from_data(thread_list, value);

  g_assert(array_index < thread_list->index_to_pixmap->len);

  GdkPixmap **pixmap =
    (GdkPixmap**)&g_ptr_array_index(thread_list->index_to_pixmap, array_index);

  *pixmap = value->pixmap;
}


void update_index_to_pixmap(ThreadList *thread_list)
{
  g_ptr_array_set_size(thread_list->index_to_pixmap,
                       g_hash_table_size(thread_list->thread_hash));
  g_hash_table_foreach(thread_list->thread_hash,
                       (GHFunc)update_index_to_pixmap_each,
                       thread_list);
}


static void update_pixmap_size_each(XenoThreadInfo *key,
                                    HashedThreadData *value,
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


void update_pixmap_size(ThreadList *thread_list, guint width)
{
  g_hash_table_foreach(thread_list->thread_hash,
                       (GHFunc)update_pixmap_size_each,
                       (gpointer)width);
}


typedef struct _CopyPixmap {
  GdkDrawable *dest;
  GdkGC *gc;
  GdkDrawable *src;
  gint xsrc, ysrc, xdest, ydest, width, height;
} CopyPixmap;

static void copy_pixmap_region_each(XenoThreadInfo *key,
                                    HashedThreadData *value,
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




void copy_pixmap_region(ThreadList *thread_list, GdkDrawable *dest,
    GdkGC *gc, GdkDrawable *src,
    gint xsrc, gint ysrc,
    gint xdest, gint ydest, gint width, gint height)
{
  CopyPixmap cp = { dest, gc, src, xsrc, ysrc, xdest, ydest, width, height };

  g_hash_table_foreach(thread_list->thread_hash,
                       (GHFunc)copy_pixmap_region_each,
                       &cp);
}



typedef struct _RectanglePixmap {
  gboolean filled;
  gint x, y, width, height;
  GdkGC *gc;
} RectanglePixmap;

static void rectangle_pixmap_each(XenoThreadInfo *key,
                                  HashedThreadData *value,
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




void rectangle_pixmap(ThreadList *thread_list, GdkGC *gc,
    gboolean filled, gint x, gint y, gint width, gint height)
{
  RectanglePixmap rp = { filled, x, y, width, height, gc };

  g_hash_table_foreach(thread_list->thread_hash,
                       (GHFunc)rectangle_pixmap_each,
                       &rp);
}


/* Renders each pixmaps into on big drawable */
void copy_pixmap_to_screen(ThreadList *thread_list,
    GdkDrawable *dest,
    GdkGC *gc,
    gint x, gint y,
    gint width, gint height)
{
  if(thread_list->index_to_pixmap->len == 0) return;
  guint cell_height = thread_list->cell_height;

  /* Get indexes */
  gint begin = floor(y/(double)cell_height);
  gint end = MIN(ceil((y+height)/(double)cell_height),
                 thread_list->index_to_pixmap->len);
  gint i;

  for(i=begin; i<end; i++) {
    g_assert(i<thread_list->index_to_pixmap->len);
    /* Render the pixmap to the screen */
    GdkPixmap *pixmap =
      //(GdkPixmap*)g_ptr_array_index(thread_list->index_to_pixmap, i);
      GDK_PIXMAP(g_ptr_array_index(thread_list->index_to_pixmap, i));

    gdk_draw_drawable (dest,
        gc,
        pixmap,
        x, 0,
        x, i*cell_height,
        width, cell_height);

  }


}









ThreadList *threadlist_construct(void)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  ThreadList* thread_list = g_new(ThreadList,1);

  thread_list->number_of_thread = 0;

  thread_list->current_hash_data = NULL;

  /* Create the Xenomai Thread list */
  thread_list->list_store = gtk_list_store_new (  N_COLUMNS,
              G_TYPE_STRING,
              G_TYPE_UINT,
              G_TYPE_UINT,
              G_TYPE_UINT,
              G_TYPE_ULONG,
              G_TYPE_ULONG,
              G_TYPE_ULONG);


  thread_list->thread_list_widget =
    gtk_tree_view_new_with_model
    (GTK_TREE_MODEL (thread_list->list_store));

  g_object_unref (G_OBJECT (thread_list->list_store));

  gtk_tree_sortable_set_default_sort_func(
      GTK_TREE_SORTABLE(thread_list->list_store),
      thread_sort_func,
      NULL,
      NULL);


  gtk_tree_sortable_set_sort_column_id(
      GTK_TREE_SORTABLE(thread_list->list_store),
      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      GTK_SORT_ASCENDING);


  thread_list->thread_hash = g_hash_table_new_full(
      thread_list_hash_fct, thread_list_equ_fct,
      destroy_hash_key, destroy_hash_data
      );


  gtk_tree_view_set_headers_visible(
    GTK_TREE_VIEW(thread_list->thread_list_widget), TRUE);

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  thread_list->renderer = renderer;

	gint vertical_separator;
	gtk_widget_style_get (GTK_WIDGET (thread_list->thread_list_widget),
			"vertical-separator", &vertical_separator,
			NULL);
  gtk_cell_renderer_get_size(renderer,
      GTK_WIDGET(thread_list->thread_list_widget),
      NULL,
      NULL,
      NULL,
      NULL,
      &thread_list->cell_height);

#if GTK_CHECK_VERSION(2,4,15)
  guint ypad;
  g_object_get(G_OBJECT(renderer), "ypad", &ypad, NULL);

  thread_list->cell_height += ypad;
#endif
  thread_list->cell_height += vertical_separator;


  /* Column 1 representing the Xenomai Task name */
  column = gtk_tree_view_column_new_with_attributes ( "Task",
                renderer,
                "text",
                THREAD_COLUMN,
                NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

  thread_list->button = column->button;

  /* Column 1 representing the priority of the task */
  column = gtk_tree_view_column_new_with_attributes ( "Priority",
                renderer,
                "text",
                PRIO_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

//  gtk_tree_view_column_set_visible(column, 0);

  /* Column 1 representing the period of the task */
  column = gtk_tree_view_column_new_with_attributes ( "Period",
                renderer,
                "text",
                PERIOD_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

//  gtk_tree_view_column_set_visible(column, 0);

  column = gtk_tree_view_column_new_with_attributes ( "CPU",
                renderer,
                "text",
                CPU_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

  column = gtk_tree_view_column_new_with_attributes ( "Birth sec",
                renderer,
                "text",
                BIRTH_S_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

  //gtk_tree_view_column_set_visible(column, 0);

  column = gtk_tree_view_column_new_with_attributes ( "Birth nsec",
                renderer,
                "text",
                BIRTH_NS_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);

  column = gtk_tree_view_column_new_with_attributes ( "TRACE",
                renderer,
                "text",
                TRACE_COLUMN,
                NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (thread_list->thread_list_widget), column);


  //gtk_tree_view_column_set_visible(column, 0);

  g_object_set_data_full(
      G_OBJECT(thread_list->thread_list_widget),
      "thread_list_Data",
      thread_list,
      (GDestroyNotify)threadlist_destroy);

  thread_list->index_to_pixmap = g_ptr_array_sized_new(ALLOCATE_PROCESSES);

  return thread_list;
}

void threadlist_destroy(ThreadList *thread_list)
{
  g_debug("threadlist_destroy %p", thread_list);
  g_hash_table_destroy(thread_list->thread_hash);
  thread_list->thread_hash = NULL;
  g_ptr_array_free(thread_list->index_to_pixmap, TRUE);

  g_free(thread_list);
  g_debug("threadlist_destroy end");
}

static gboolean remove_hash_item(XenoThreadInfo *thread_Info,
                                 HashedThreadData *hashed_thread_data,
                                 ThreadList *thread_list)
{
  GtkTreeIter iter;

  iter = hashed_thread_data->y_iter;

  gtk_list_store_remove (thread_list->list_store, &iter);
  gdk_pixmap_unref(hashed_thread_data->pixmap);

  if(likely(thread_list->current_hash_data != NULL)) {
    if(likely(hashed_thread_data ==
                thread_list->current_hash_data[thread_Info->trace_num][thread_Info->cpu]))
      thread_list->current_hash_data[thread_Info->trace_num][thread_Info->cpu] = NULL;
  }
  return TRUE; /* remove the element from the hash table */
}

void threadlist_clear(ThreadList *thread_list)
{
  g_info("threadlist_clear %p", thread_list);

  g_hash_table_foreach_remove(thread_list->thread_hash,
                              (GHRFunc)remove_hash_item,
                              (gpointer)thread_list);
  thread_list->number_of_thread = 0;
  update_index_to_pixmap(thread_list);
}


GtkWidget *threadlist_get_widget(ThreadList *thread_list)
{
  return thread_list->thread_list_widget;
}


void destroy_hash_key(gpointer key)
{
  g_free(key);
}

void destroy_hash_data(gpointer data)
{
  g_free(data);
}


void threadlist_set_name(ThreadList *thread_list,
    GQuark name,
    HashedThreadData *hashed_thread_data)
{
  gtk_list_store_set (  thread_list->list_store, &hashed_thread_data->y_iter,
        THREAD_COLUMN, g_quark_to_string(name),
        -1);
}

void threadlist_set_prio(ThreadList *thread_list,
    guint prio,
    HashedThreadData *hashed_thread_data)
{
  gtk_list_store_set (  thread_list->list_store, &hashed_thread_data->y_iter,
        PRIO_COLUMN, prio,
        -1);
}

void threadlist_set_period(ThreadList *thread_list,
    guint period,
    HashedThreadData *hashed_thread_data)
{
  gtk_list_store_set (  thread_list->list_store, &hashed_thread_data->y_iter,
        PERIOD_COLUMN, period,
        -1);
}


int threadlist_add(  ThreadList *thread_list,
      XenoLtt_Drawing_t *drawing,
      guint address,
      guint prio,
      guint cpu,
      guint period,
      LttTime *thread_birth,
      guint trace_num,
      GQuark name,
      guint *height,
      XenoThreadInfo **pm_thread_Info,
      HashedThreadData **pm_hashed_thread_data)
{
  XenoThreadInfo *thread_Info = g_new(XenoThreadInfo, 1);
  HashedThreadData *hashed_thread_data = g_new(HashedThreadData, 1);
  *pm_hashed_thread_data = hashed_thread_data;
  *pm_thread_Info = thread_Info;

  thread_Info->address = address;
  thread_Info->prio = prio;
  if(address == 0)
    thread_Info->cpu = cpu;
  else
    thread_Info->cpu = 0;
  thread_Info->period = period;
  thread_Info->thread_birth = *thread_birth;
  thread_Info->trace_num = trace_num;

  /* When we create it from before state update, we are sure that the
   * last event occured before the beginning of the global area.
   *
   * If it is created after state update, this value (0) will be
   * overriden by the new state before anything is drawn.
   */
  hashed_thread_data->x.over = 0;
  hashed_thread_data->x.over_used = FALSE;
  hashed_thread_data->x.over_marked = FALSE;
  hashed_thread_data->x.middle = 0;
  hashed_thread_data->x.middle_used = FALSE;
  hashed_thread_data->x.middle_marked = FALSE;
  hashed_thread_data->x.under = 0;
  hashed_thread_data->x.under_used = FALSE;
  hashed_thread_data->x.under_marked = FALSE;
  hashed_thread_data->next_good_time = ltt_time_zero;
  /* Add a new row to the model */
  gtk_list_store_append ( thread_list->list_store,
                          &hashed_thread_data->y_iter);

  gtk_list_store_set (  thread_list->list_store, &hashed_thread_data->y_iter,
        THREAD_COLUMN, g_quark_to_string(name),
        PRIO_COLUMN, prio,
        PERIOD_COLUMN, period,
        CPU_COLUMN, cpu,
        BIRTH_S_COLUMN, thread_birth->tv_sec,
        BIRTH_NS_COLUMN, thread_birth->tv_nsec,
        TRACE_COLUMN, trace_num,
        -1);
  
  g_hash_table_insert(thread_list->thread_hash,
        (gpointer)thread_Info,
        (gpointer)hashed_thread_data);

  thread_list->number_of_thread++;

  hashed_thread_data->height = thread_list->cell_height;

  g_assert(hashed_thread_data->height != 0);

  *height = hashed_thread_data->height * thread_list->number_of_thread;

  hashed_thread_data->pixmap =
        gdk_pixmap_new(drawing->drawing_area->window,
                       drawing->alloc_width,
                       hashed_thread_data->height,
                       -1);

  // Clear the image
  gdk_draw_rectangle (hashed_thread_data->pixmap,
        drawing->drawing_area->style->black_gc,
        TRUE,
        0, 0,
        drawing->alloc_width,
        hashed_thread_data->height);

  update_index_to_pixmap(thread_list);


  return 0;
}

int threadlist_remove( ThreadList *thread_list,
      guint address,
      guint cpu,
      LttTime *thread_birth, 
      guint trace_num)
{
  XenoThreadInfo thread_Info;
  HashedThreadData *hashed_thread_data;
  GtkTreeIter iter;

  thread_Info.address = address;
  if(address == 0)
    thread_Info.cpu = cpu;
  else
    thread_Info.cpu = 0;
  thread_Info.thread_birth = *thread_birth; 
  thread_Info.trace_num = trace_num;


  hashed_thread_data =
    (HashedThreadData*)g_hash_table_lookup(
          thread_list->thread_hash,
          &thread_Info);
  if(likely(hashed_thread_data != NULL))
  {
    iter = hashed_thread_data->y_iter;

    gtk_list_store_remove (thread_list->list_store, &iter);

    g_hash_table_remove(thread_list->thread_hash,
        &thread_Info);

    if(likely(thread_list->current_hash_data != NULL)) {
      if(likely(hashed_thread_data == thread_list->current_hash_data[trace_num][cpu])) {
        thread_list->current_hash_data[trace_num][cpu] = NULL;
      }
    }

    gdk_pixmap_unref(hashed_thread_data->pixmap);

    update_index_to_pixmap(thread_list);

    thread_list->number_of_thread--;

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
