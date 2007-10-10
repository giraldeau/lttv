/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/trace.h>

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/tracecontext.h>
#include <lttv/hook.h>
#include <lttv/state.h>
#include <lttv/stats.h>

#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/lttv_plugin_tab.h>

#include "hGuiStatisticInsert.xpm"

#define PATH_LENGTH        256  /* CHECK */
#define MAX_NUMBER_EVENT "max_number_event"

//???????????????6
//static GPtrArray  * statistic_traceset;

/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_statistic_viewer_data_list = NULL ;

typedef struct _StatisticViewerData StatisticViewerData;

static void request_background_data(StatisticViewerData *svd);
GtkWidget *guistatistic_get_widget(StatisticViewerData *svd);

//! Statistic Viewer's constructor hook
GtkWidget *h_gui_statistic(LttvPlugin *plugin);
//! Statistic Viewer's constructor
StatisticViewerData *gui_statistic(LttvPluginTab *ptab);
//! Statistic Viewer's destructor
void gui_statistic_destructor(StatisticViewerData *statistic_viewer_data);

static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);

void statistic_destroy_hash_key(gpointer key);
void statistic_destroy_hash_data(gpointer data);

void show_traceset_stats(StatisticViewerData * statistic_viewer_data);
void show_tree(StatisticViewerData * statistic_viewer_data,
         LttvAttribute* stats,  GtkTreeIter* parent);
void show_statistic(StatisticViewerData * statistic_viewer_data,
         LttvAttribute* stats, GtkTextBuffer* buf);


gboolean statistic_traceset_changed(void * hook_data, void * call_data);
//void statistic_add_context_hooks(StatisticViewerData * statistic_viewer_data, 
//           LttvTracesetContext * tsc);
//void statistic_remove_context_hooks(StatisticViewerData *statistic_viewer_data, 
//           LttvTracesetContext * tsc);

//gboolean statistic_insert_traceset_stats(void * stats);

enum
{
   NAME_COLUMN,
   N_COLUMNS
};

struct _StatisticViewerData{
  Tab *tab;
  LttvPluginTab *ptab;
  //LttvTracesetStats * stats;
  int                 size;

  //gboolean     shown;       //indicate if the statistic is shown or not
  //char *       filter_key;

  GtkWidget    * hpaned_v;
  GtkTreeStore * store_m;
  GtkWidget    * tree_v;

  //scroll window containing Tree View
  GtkWidget * scroll_win_tree;

  GtkWidget    * text_v;  
  //scroll window containing Text View
  GtkWidget * scroll_win_text;

  // Selection handler 
  GtkTreeSelection *select_c;
  
  //hash 
  GHashTable *statistic_hash;

  guint background_info_waiting;
};




/* Action to do when background computation completed.
 *
 * Eventually, will have to check that every requested traces are finished
 * before doing the redraw. It will save unnecessary processor usage.
 */

static gint background_ready(void *hook_data, void *call_data)
{
  StatisticViewerData *svd = (StatisticViewerData *)hook_data;
  Tab *tab = svd->tab;
  LttvTrace *trace = (LttvTrace*)call_data;

  svd->background_info_waiting--;

  if(svd->background_info_waiting == 0) {
    g_message("statistics viewer : background computation data ready.");

    gtk_tree_store_clear (svd->store_m);

    lttv_stats_sum_traceset(lttvwindow_get_traceset_stats(tab),
      ltt_time_infinite);
    show_traceset_stats(svd);
  }

  return 0;
}

/* Request background computation. Verify if it is in progress or ready first.
 *
 * Right now, for all loaded traces.
 *
 * Later : must be only for each trace in the tab's traceset.
 */
static void request_background_data(StatisticViewerData *svd)
{
  gint num_traces = lttvwindowtraces_get_number();
  gint i;
  LttvTrace *trace;
  GtkTextBuffer* buf;

  LttvHooks *background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, svd,
      LTTV_PRIO_DEFAULT);
  svd->background_info_waiting = num_traces;
  buf = gtk_text_view_get_buffer((GtkTextView*)svd->text_v);
  gtk_text_buffer_set_text(buf,"", -1);
  
  for(i=0;i<num_traces;i++) {
    trace = lttvwindowtraces_get_trace(i);

    if(lttvwindowtraces_get_ready(g_quark_from_string("stats"),trace)==FALSE) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("stats"),
                                          trace) == FALSE) {
        /* We first remove requests that could have been done for the same
         * information. Happens when two viewers ask for it before servicing
         * starts.
         */
        if(!lttvwindowtraces_background_request_find(trace, "stats"))
          lttvwindowtraces_background_request_queue(
              main_window_get_widget(svd->tab), trace, "stats");
        lttvwindowtraces_background_notify_queue(svd,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 background_ready_hook);
      } else { /* in progress */
        lttvwindowtraces_background_notify_current(svd,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   background_ready_hook);
      
      }
    } else {
      /* ready */
      lttv_hooks_call(background_ready_hook, NULL);
    }
  }

  if(num_traces == 0) {
    svd->background_info_waiting = 1;
    lttv_hooks_call(background_ready_hook, NULL);
  }
  lttv_hooks_destroy(background_ready_hook);
}


GtkWidget *guistatistic_get_widget(StatisticViewerData *svd)
{
  return svd->hpaned_v;
}


void
gui_statistic_destructor(StatisticViewerData *statistic_viewer_data)
{
  Tab *tab = statistic_viewer_data->tab;

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(guistatistic_get_widget(statistic_viewer_data))){
    g_info("widget still exists");
  }
  if(tab != NULL) {
    lttvwindow_unregister_traceset_notify(statistic_viewer_data->tab,
                                          statistic_traceset_changed,
                                          statistic_viewer_data);
  }
  lttvwindowtraces_background_notify_remove(statistic_viewer_data);

  g_hash_table_destroy(statistic_viewer_data->statistic_hash);
  g_statistic_viewer_data_list =
    g_slist_remove(g_statistic_viewer_data_list, statistic_viewer_data);
  g_free(statistic_viewer_data);
}


/**
 * Statistic Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_gui_statistic(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  StatisticViewerData* statistic_viewer_data = gui_statistic(ptab) ;

  if(statistic_viewer_data)
    return guistatistic_get_widget(statistic_viewer_data);
  else return NULL;
  
}

#if 0
gboolean statistic_insert_traceset_stats(void * stats)
{
  int i, len;
  gpointer s;

  len = statistic_traceset->len;
  for(i=0;i<len;i++){
    s = g_ptr_array_index(statistic_traceset, i);
    if(s == stats) break;    
  }
  if(i==len){
    g_ptr_array_add(statistic_traceset, stats);
    return TRUE;
  }
  return FALSE;
}
#endif //0

/**
 * Statistic Viewer's constructor
 *
 * This constructor is used to create StatisticViewerData data structure.
 * @return The Statistic viewer data created.
 */
StatisticViewerData *
gui_statistic(LttvPluginTab *ptab)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  StatisticViewerData* statistic_viewer_data = g_new(StatisticViewerData,1);
  Tab *tab = ptab->tab;
  statistic_viewer_data->tab  = tab;
  statistic_viewer_data->ptab  = ptab;
 // statistic_viewer_data->stats  =
 //         lttvwindow_get_traceset_stats(statistic_viewer_data->tab);
 // statistic_viewer_data->calculate_stats = 
 //   statistic_insert_traceset_stats((void *)statistic_viewer_data->stats);

  lttvwindow_register_traceset_notify(statistic_viewer_data->tab,
                                      statistic_traceset_changed,
                                      statistic_viewer_data);
 
  statistic_viewer_data->statistic_hash = g_hash_table_new_full(g_str_hash,
                                                  g_str_equal,
                                                  statistic_destroy_hash_key,
                                                  NULL);

  statistic_viewer_data->hpaned_v  = gtk_hpaned_new();
  statistic_viewer_data->store_m = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING);
  statistic_viewer_data->tree_v  = 
    gtk_tree_view_new_with_model (
        GTK_TREE_MODEL (statistic_viewer_data->store_m));
  g_object_unref (G_OBJECT (statistic_viewer_data->store_m));

  // Setup the selection handler
  statistic_viewer_data->select_c = gtk_tree_view_get_selection (GTK_TREE_VIEW (statistic_viewer_data->tree_v));
  gtk_tree_selection_set_mode (statistic_viewer_data->select_c, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (statistic_viewer_data->select_c), "changed",
        G_CALLBACK (tree_selection_changed_cb),
        statistic_viewer_data);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Statistic Name",
                 renderer,
                 "text", NAME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  //  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (statistic_viewer_data->tree_v), column);


  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (statistic_viewer_data->tree_v), FALSE);

  statistic_viewer_data->scroll_win_tree = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(statistic_viewer_data->scroll_win_tree), 
         GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (statistic_viewer_data->scroll_win_tree), statistic_viewer_data->tree_v);
  gtk_paned_pack1(GTK_PANED(statistic_viewer_data->hpaned_v),statistic_viewer_data->scroll_win_tree, TRUE, FALSE);
  gtk_paned_set_position(GTK_PANED(statistic_viewer_data->hpaned_v), 160);

  statistic_viewer_data->scroll_win_text = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(statistic_viewer_data->scroll_win_text), 
         GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  statistic_viewer_data->text_v = gtk_text_view_new ();
  
  gtk_text_view_set_editable(GTK_TEXT_VIEW(statistic_viewer_data->text_v),FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(statistic_viewer_data->text_v),FALSE);
  gtk_container_add (GTK_CONTAINER (statistic_viewer_data->scroll_win_text), statistic_viewer_data->text_v);
  gtk_paned_pack2(GTK_PANED(statistic_viewer_data->hpaned_v), statistic_viewer_data->scroll_win_text, TRUE, FALSE);

  gtk_container_set_border_width(
      GTK_CONTAINER(statistic_viewer_data->hpaned_v), 1);
  
  gtk_widget_show(statistic_viewer_data->scroll_win_tree);
  gtk_widget_show(statistic_viewer_data->scroll_win_text);
  gtk_widget_show(statistic_viewer_data->tree_v);
  gtk_widget_show(statistic_viewer_data->text_v);
  gtk_widget_show(statistic_viewer_data->hpaned_v);

  g_object_set_data_full(
      G_OBJECT(guistatistic_get_widget(statistic_viewer_data)),
      "statistic_viewer_data",
      statistic_viewer_data,
      (GDestroyNotify)gui_statistic_destructor);

  /* Add the object's information to the module's array */
  g_statistic_viewer_data_list = g_slist_append(
      g_statistic_viewer_data_list,
      statistic_viewer_data);

  request_background_data(statistic_viewer_data);
 
  return statistic_viewer_data;
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*)data;
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(statistic_viewer_data->store_m);
  gchar *event;
  GtkTextBuffer* buf;
  gchar * str;
  GtkTreePath * path;
  GtkTextIter   text_iter;
  LttvAttribute * stats;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, NAME_COLUMN, &event, -1);

      path = gtk_tree_model_get_path(GTK_TREE_MODEL(model),&iter);
      str = gtk_tree_path_to_string (path);
      stats = (LttvAttribute*)g_hash_table_lookup (statistic_viewer_data->statistic_hash,str);
      g_free(str);
      
      buf =  gtk_text_view_get_buffer((GtkTextView*)statistic_viewer_data->text_v);
      gtk_text_buffer_set_text(buf,"Statistic for  '", -1);
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, event, strlen(event));      
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, "' :\n\n",5);
      
      show_statistic(statistic_viewer_data, stats, buf);

      g_free (event);
    }
}

void statistic_destroy_hash_key(gpointer key)
{
  g_free(key);
}

#ifdef DEBUG
#include <stdio.h>
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#endif //DEBUG

void show_traceset_stats(StatisticViewerData * statistic_viewer_data)
{
  Tab *tab = statistic_viewer_data->tab;
  int i, nb;
  LttvTraceset *ts;
  LttvTraceStats *tcs;
  LttSystemDescription *desc;
  LttvTracesetStats * tscs = lttvwindow_get_traceset_stats(tab);
  gchar * str, trace_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = statistic_viewer_data->store_m;

  if(tscs->stats == NULL) return;
#ifdef DEBUG
  lttv_attribute_write_xml(tscs->stats, stdout, 1, 4);
#endif //DEBUG
  
  ts = tscs->parent.parent.ts;
  nb = lttv_traceset_number(ts);
  if(nb == 0) return;

  gtk_tree_store_append (store, &iter, NULL);  
  gtk_tree_store_set (store, &iter,
          NAME_COLUMN, "Traceset statistics",
          -1);  
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
  str = gtk_tree_path_to_string (path);
  g_hash_table_insert(statistic_viewer_data->statistic_hash,
          (gpointer)str, tscs->stats);
  show_tree(statistic_viewer_data, tscs->stats, &iter);

  //show stats for all traces
  for(i = 0 ; i < nb ; i++) {
    tcs = (LttvTraceStats *)(LTTV_TRACESET_CONTEXT(tscs)->traces[i]);
#if 0 //FIXME
    desc = ltt_trace_system_description(tcs->parent.parent.t);    
    LttTime start_time = ltt_trace_system_description_trace_start_time(desc);
    sprintf(trace_str, "Trace on system %s at time %lu.%09lu", 
            ltt_trace_system_description_node_name(desc),
            start_time.tv_sec,
            start_time.tv_nsec);
#endif //0
    sprintf(trace_str, g_quark_to_string(ltt_trace_name(tcs->parent.parent.t)));
    gtk_tree_store_append (store, &iter, NULL);  
    gtk_tree_store_set (store, &iter,NAME_COLUMN,trace_str,-1);  
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    str = gtk_tree_path_to_string (path);
    g_hash_table_insert(statistic_viewer_data->statistic_hash,
      (gpointer)str,tcs->stats);
    show_tree(statistic_viewer_data, tcs->stats, &iter);
#ifdef DEBUG
    lttv_attribute_write_xml(tcs->stats, stdout, 3, 4);
#endif //DEBUG
  }
}

void show_tree(StatisticViewerData * statistic_viewer_data,
         LttvAttribute* stats,  GtkTreeIter* parent)
{
  int i, nb;
  LttvAttribute *subtree;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
	gboolean is_named;
  gchar * str, dir_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = statistic_viewer_data->store_m;

  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value, &is_named);
    switch(type) {
     case LTTV_GOBJECT:
        if(LTTV_IS_ATTRIBUTE(*(value.v_gobject))) {
          subtree = (LttvAttribute *)*(value.v_gobject);
          if(is_named)
            sprintf(dir_str, "%s", g_quark_to_string(name));
          else
            sprintf(dir_str, "%u", name);
          gtk_tree_store_append (store, &iter, parent);  
          gtk_tree_store_set (store, &iter,NAME_COLUMN,dir_str,-1);  
          path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
          str = gtk_tree_path_to_string (path);
          g_hash_table_insert(statistic_viewer_data->statistic_hash,
            (gpointer)str, subtree);
          show_tree(statistic_viewer_data, subtree, &iter);
        }
        break;
      default:
  break;
    }
  }    
}

void show_statistic(StatisticViewerData * statistic_viewer_data,
        LttvAttribute* stats, GtkTextBuffer* buf)
{
  int i, nb , flag;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
	gboolean is_named;
  gchar type_name[PATH_LENGTH], type_value[PATH_LENGTH];
  GtkTextIter   text_iter;
  
  flag = 0;
  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value, &is_named);
		if(is_named)
	    sprintf(type_name,"%s", g_quark_to_string(name));
		else
	    sprintf(type_name,"%u", name);
    type_value[0] = '\0';
    switch(type) {
      case LTTV_INT:
        sprintf(type_value, " :  %d\n", *value.v_int);
        break;
      case LTTV_UINT:
        sprintf(type_value, " :  %u\n", *value.v_uint);
        break;
      case LTTV_LONG:
        sprintf(type_value, " :  %ld\n", *value.v_long);
        break;
      case LTTV_ULONG:
        sprintf(type_value, " :  %lu\n", *value.v_ulong);
        break;
      case LTTV_FLOAT:
        sprintf(type_value, " :  %f\n", (double)*value.v_float);
        break;
      case LTTV_DOUBLE:
        sprintf(type_value, " :  %f\n", *value.v_double);
        break;
      case LTTV_TIME:
        sprintf(type_value, " :  %10lu.%09lu\n", value.v_time->tv_sec, 
            value.v_time->tv_nsec);
        break;
      case LTTV_POINTER:
        sprintf(type_value, " :  POINTER\n");
        break;
      case LTTV_STRING:
        sprintf(type_value, " :  %s\n", *value.v_string);
        break;
      default:
        break;
    }
    if(strlen(type_value)){
      flag = 1;
      strcat(type_name,type_value);
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, type_name, strlen(type_name));
    }
  }

  if(flag == 0){
    sprintf(type_value, "No statistic information in this directory.\nCheck in subdirectories please.\n");
    gtk_text_buffer_get_end_iter(buf, &text_iter);
    gtk_text_buffer_insert(buf, &text_iter, type_value, strlen(type_value));

  }
}

gboolean statistic_traceset_changed(void * hook_data, void * call_data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*) hook_data;
  
  request_background_data(statistic_viewer_data);

  return FALSE;
}

#if 0
void statistic_add_context_hooks(StatisticViewerData * statistic_viewer_data, 
           LttvTracesetContext * tsc)
{
  gint i, j, nbi, nb_tracefile;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  LttvTracesetSelector  * ts_s;
  LttvTraceSelector     * t_s;
  LttvTracefileSelector * tf_s;
  gboolean selected;

  ts_s = (LttvTracesetSelector*)g_object_get_data(G_OBJECT(statistic_viewer_data->hpaned_v), 
              statistic_viewer_data->filter_key);

  //if there are hooks for traceset, add them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_trace_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, add them here

    nb_tracefile = ltt_trace_control_tracefile_number(trace) +
        ltt_trace_per_cpu_tracefile_number(trace);
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      tfc = tc->tracefiles[j];
      
      //if there are hooks for tracefile, add them here
      //      lttv_tracefile_context_add_hooks(tfc, NULL,NULL,NULL,NULL,
      //               statistic_viewer_data->before_event_hooks,NULL);
    }
  }  

  lttv_stats_add_event_hooks(LTTV_TRACESET_STATS(tsc));
  
}

void statistic_remove_context_hooks(StatisticViewerData * statistic_viewer_data, 
        LttvTracesetContext * tsc)
{
  gint i, j, nbi, nb_tracefile;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  LttvTracesetSelector  * ts_s;
  LttvTraceSelector     * t_s;
  LttvTracefileSelector * tf_s;
  gboolean selected;

  ts_s = (LttvTracesetSelector*)g_object_get_data(G_OBJECT(statistic_viewer_data->hpaned_v), 
              statistic_viewer_data->filter_key);

  //if there are hooks for traceset, remove them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_trace_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, remove them here

    nb_tracefile = ltt_trace_control_tracefile_number(trace) +
        ltt_trace_per_cpu_tracefile_number(trace);
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_tracefile_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      tfc = tc->tracefiles[j];
      
      //if there are hooks for tracefile, remove them here
      //      lttv_tracefile_context_remove_hooks(tfc, NULL,NULL,NULL,NULL,
      //            statistic_viewer_data->before_event_hooks,NULL);
    }
  }

  lttv_stats_remove_event_hooks(LTTV_TRACESET_STATS(tsc));
}
#endif //0

/**
 * plugin's init function
 *
 * This function initializes the Statistic Viewer functionnality through the
 * gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guistatistics",
                                  "/",
                                  "Insert Statistic Viewer",
                                  hGuiStatisticInsert_xpm,
                                  "Insert Statistic Viewer",
                                  h_gui_statistic);
}

void statistic_destroy_walk(gpointer data, gpointer user_data)
{
  StatisticViewerData *svd = (StatisticViewerData*)data;

  g_debug("CFV.c : statistic_destroy_walk, %p", svd);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guistatistic_get_widget(svd)))
    gtk_widget_destroy(guistatistic_get_widget(svd));
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  
  g_slist_foreach(g_statistic_viewer_data_list, statistic_destroy_walk, NULL );    
  g_slist_free(g_statistic_viewer_data_list);

  lttvwindow_unregister_constructor(h_gui_statistic);
  
}


LTTV_MODULE("guistatistics", "Statistics viewer", \
    "Graphical module to view statistics about processes, CPUs and systems", \
    init, destroy, "lttvwindow")

