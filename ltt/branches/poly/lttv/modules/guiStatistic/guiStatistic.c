#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/module.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/processTrace.h>
#include <lttv/hook.h>
#include <lttv/common.h>
#include <lttv/state.h>
#include <lttv/stats.h>

#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>

#include <string.h>

#include "../icons/hGuiStatisticInsert.xpm"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

#define PATH_LENGTH        256

static LttvModule *Main_Win_Module;

/** Array containing instanced objects. Used when module is unloaded */
static GSList *gStatistic_Viewer_Data_List = NULL ;

typedef struct _StatisticViewerData StatisticViewerData;

//! Statistic Viewer's constructor hook
GtkWidget *hGuiStatistic(mainWindow *pmParentWindow);
//! Statistic Viewer's constructor
StatisticViewerData *GuiStatistic(mainWindow *pmParentWindow);
//! Statistic Viewer's destructor
void GuiStatistic_Destructor(StatisticViewerData *Statistic_Viewer_Data);
void GuiStatistic_free(StatisticViewerData *Statistic_Viewer_Data);

void grab_focus(GtkWidget *widget, gpointer data);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);

void statistic_destroy_hash_key(gpointer key);
void statistic_destroy_hash_data(gpointer data);

void get_traceset_stats(StatisticViewerData * Statistic_Viewer_Data);
void show_traceset_stats(StatisticViewerData * Statistic_Viewer_Data);
void show_tree(StatisticViewerData * Statistic_Viewer_Data,
	       LttvAttribute* stats,  GtkTreeIter* parent);
void show_statistic(StatisticViewerData * Statistic_Viewer_Data,
		    LttvAttribute* stats, GtkTextBuffer* buf);


enum
{
   NAME_COLUMN,
   N_COLUMNS
};

struct _StatisticViewerData{
  mainWindow * mw;
  LttvTracesetStats * stats;

  GtkWidget    * HPaned_V;
  GtkTreeStore * Store_M;
  GtkWidget    * Tree_V;

  //scroll window containing Tree View
  GtkWidget * Scroll_Win_Tree;

  GtkWidget    * Text_V;  
  //scroll window containing Text View
  GtkWidget * Scroll_Win_Text;

  // Selection handler 
  GtkTreeSelection *Select_C;
  
  //hash 
  GHashTable *Statistic_Hash;
};


/**
 * plugin's init function
 *
 * This function initializes the Statistic Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init(LttvModule *self, int argc, char *argv[]) {

  Main_Win_Module = lttv_module_require(self, "mainwin", argc, argv);
  
  if(Main_Win_Module == NULL){
      g_critical("Can't load Statistic Viewer : missing mainwin\n");
      return;
  }
	
  g_critical("GUI Statistic Viewer init()");
  
  /* Register the toolbar insert button */
  ToolbarItemReg(hGuiStatisticInsert_xpm, "Insert Statistic Viewer", hGuiStatistic);
  
  /* Register the menu item insert entry */
  MenuItemReg("/", "Insert Statistic Viewer", hGuiStatistic);
  
}

void statistic_destroy_walk(gpointer data, gpointer user_data)
{
  GuiStatistic_Destructor((StatisticViewerData*)data);
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
  int i;
  
  g_critical("GUI Statistic Viewer destroy()");

  if(gStatistic_Viewer_Data_List){
    g_slist_foreach(gStatistic_Viewer_Data_List, statistic_destroy_walk, NULL );    
    g_slist_free(gStatistic_Viewer_Data_List);
  }

  /* Unregister the toolbar insert button */
  ToolbarItemUnreg(hGuiStatistic);
	
  /* Unregister the menu item insert entry */
  MenuItemUnreg(hGuiStatistic);
}


void
GuiStatistic_free(StatisticViewerData *Statistic_Viewer_Data)
{ 
  g_critical("GuiStatistic_free()");
  if(Statistic_Viewer_Data){
    g_hash_table_destroy(Statistic_Viewer_Data->Statistic_Hash);
    gStatistic_Viewer_Data_List = g_slist_remove(gStatistic_Viewer_Data_List, Statistic_Viewer_Data);
    g_warning("Delete Statistic data\n");
    g_free(Statistic_Viewer_Data);
  }
}

void
GuiStatistic_Destructor(StatisticViewerData *Statistic_Viewer_Data)
{
  g_critical("GuiStatistic_Destructor()");
  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(Statistic_Viewer_Data->HPaned_V)){
    gtk_widget_destroy(Statistic_Viewer_Data->HPaned_V);
    Statistic_Viewer_Data = NULL;
  }
  //GuiStatistic_free(Statistic_Viewer_Data);
}


/**
 * Statistic Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param pmParentWindow A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
hGuiStatistic(mainWindow * pmParentWindow)
{
  StatisticViewerData* Statistic_Viewer_Data = GuiStatistic(pmParentWindow) ;

  if(Statistic_Viewer_Data)
    return Statistic_Viewer_Data->HPaned_V;
  else return NULL;
	
}

/**
 * Statistic Viewer's constructor
 *
 * This constructor is used to create StatisticViewerData data structure.
 * @return The Statistic viewer data created.
 */
StatisticViewerData *
GuiStatistic(mainWindow *pmParentWindow)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  StatisticViewerData* Statistic_Viewer_Data = g_new(StatisticViewerData,1);

  Statistic_Viewer_Data->mw     = pmParentWindow;
  Statistic_Viewer_Data->stats  = getTracesetStats(Statistic_Viewer_Data->mw);

  Statistic_Viewer_Data->Statistic_Hash = g_hash_table_new_full(g_str_hash, g_str_equal,
								statistic_destroy_hash_key,
								statistic_destroy_hash_data);

  Statistic_Viewer_Data->HPaned_V  = gtk_hpaned_new();
  Statistic_Viewer_Data->Store_M = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING);
  Statistic_Viewer_Data->Tree_V  = gtk_tree_view_new_with_model (GTK_TREE_MODEL (Statistic_Viewer_Data->Store_M));
  g_object_unref (G_OBJECT (Statistic_Viewer_Data->Store_M));

  g_signal_connect (G_OBJECT (Statistic_Viewer_Data->Tree_V), "grab-focus",
		    G_CALLBACK (grab_focus),
		    Statistic_Viewer_Data);

  // Setup the selection handler
  Statistic_Viewer_Data->Select_C = gtk_tree_view_get_selection (GTK_TREE_VIEW (Statistic_Viewer_Data->Tree_V));
  gtk_tree_selection_set_mode (Statistic_Viewer_Data->Select_C, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (Statistic_Viewer_Data->Select_C), "changed",
		    G_CALLBACK (tree_selection_changed_cb),
		    Statistic_Viewer_Data);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Statistic Name",
						     renderer,
						     "text", NAME_COLUMN,
						     NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  //  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (Statistic_Viewer_Data->Tree_V), column);


  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (Statistic_Viewer_Data->Tree_V), FALSE);

  Statistic_Viewer_Data->Scroll_Win_Tree = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Statistic_Viewer_Data->Scroll_Win_Tree), 
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (Statistic_Viewer_Data->Scroll_Win_Tree), Statistic_Viewer_Data->Tree_V);
  gtk_paned_pack1(GTK_PANED(Statistic_Viewer_Data->HPaned_V),Statistic_Viewer_Data->Scroll_Win_Tree, TRUE, FALSE);
  gtk_paned_set_position(GTK_PANED(Statistic_Viewer_Data->HPaned_V), 160);

  Statistic_Viewer_Data->Scroll_Win_Text = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Statistic_Viewer_Data->Scroll_Win_Text), 
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  Statistic_Viewer_Data->Text_V = gtk_text_view_new ();
  g_signal_connect (G_OBJECT (Statistic_Viewer_Data->Text_V), "grab-focus",
		    G_CALLBACK (grab_focus),
		    Statistic_Viewer_Data);
  
  gtk_text_view_set_editable(GTK_TEXT_VIEW(Statistic_Viewer_Data->Text_V),FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(Statistic_Viewer_Data->Text_V),FALSE);
  gtk_container_add (GTK_CONTAINER (Statistic_Viewer_Data->Scroll_Win_Text), Statistic_Viewer_Data->Text_V);
  gtk_paned_pack2(GTK_PANED(Statistic_Viewer_Data->HPaned_V), Statistic_Viewer_Data->Scroll_Win_Text, TRUE, FALSE);

  gtk_widget_show(Statistic_Viewer_Data->Scroll_Win_Tree);
  gtk_widget_show(Statistic_Viewer_Data->Scroll_Win_Text);
  gtk_widget_show(Statistic_Viewer_Data->Tree_V);
  gtk_widget_show(Statistic_Viewer_Data->Text_V);
  gtk_widget_show(Statistic_Viewer_Data->HPaned_V);

  g_object_set_data_full(
			G_OBJECT(Statistic_Viewer_Data->HPaned_V),
			"Statistic_Viewer_Data",
			Statistic_Viewer_Data,
			(GDestroyNotify)GuiStatistic_free);

  /* Add the object's information to the module's array */
  gStatistic_Viewer_Data_List = g_slist_append(
			gStatistic_Viewer_Data_List,
			Statistic_Viewer_Data);

  get_traceset_stats(Statistic_Viewer_Data);

  return Statistic_Viewer_Data;
}

void grab_focus(GtkWidget *widget, gpointer data)
{
  StatisticViewerData *Statistic_Viewer_Data = (StatisticViewerData *)data;
  mainWindow * mw = Statistic_Viewer_Data->mw;
  SetFocusedPane(mw, gtk_widget_get_parent(Statistic_Viewer_Data->HPaned_V));
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  StatisticViewerData *Statistic_Viewer_Data = (StatisticViewerData*)data;
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(Statistic_Viewer_Data->Store_M);
  gchar *Event;
  GtkTextBuffer* buf;
  gchar * str;
  GtkTreePath * path;
  GtkTextIter   text_iter;
  LttvAttribute * stats;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, NAME_COLUMN, &Event, -1);

      path = gtk_tree_model_get_path(GTK_TREE_MODEL(model),&iter);
      str = gtk_tree_path_to_string (path);
      stats = (LttvAttribute*)g_hash_table_lookup (Statistic_Viewer_Data->Statistic_Hash,str);
      g_free(str);
      
      buf =  gtk_text_view_get_buffer((GtkTextView*)Statistic_Viewer_Data->Text_V);
      gtk_text_buffer_set_text(buf,"Statistic for  '", -1);
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, Event, strlen(Event));      
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, "' :\n\n",5);
      
      show_statistic(Statistic_Viewer_Data, stats, buf);

      g_free (Event);
    }
}

void statistic_destroy_hash_key(gpointer key)
{
  g_free(key);
}

void statistic_destroy_hash_data(gpointer data)
{
  //  g_free(data);
}

void get_traceset_stats(StatisticViewerData * Statistic_Viewer_Data)
{
  LttTime start, end;

  start.tv_sec = 0;
  start.tv_nsec = 0;
  end.tv_sec = G_MAXULONG;
  end.tv_nsec = G_MAXULONG;
  
  stateAddEventHooks(Statistic_Viewer_Data->mw);
  statsAddEventHooks(Statistic_Viewer_Data->mw);

  processTraceset(Statistic_Viewer_Data->mw, start, end, G_MAXULONG);
  
  stateRemoveEventHooks(Statistic_Viewer_Data->mw);
  statsRemoveEventHooks(Statistic_Viewer_Data->mw);

  //establish tree view for stats
  show_traceset_stats(Statistic_Viewer_Data);
}

void show_traceset_stats(StatisticViewerData * Statistic_Viewer_Data)
{
  int i, nb;
  LttvTraceset *ts;
  LttvTraceStats *tcs;
  LttSystemDescription *desc;
  LttvTracesetStats * tscs = Statistic_Viewer_Data->stats;
  gchar * str, trace_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = Statistic_Viewer_Data->Store_M;

  if(tscs->stats == NULL) return;

  gtk_tree_store_append (store, &iter, NULL);  
  gtk_tree_store_set (store, &iter,
		      NAME_COLUMN, "Traceset statistics",
		      -1);  
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),	&iter);
  str = gtk_tree_path_to_string (path);
  g_hash_table_insert(Statistic_Viewer_Data->Statistic_Hash,
		      (gpointer)str, tscs->stats);
  show_tree(Statistic_Viewer_Data, tscs->stats, &iter);

  //show stats for all traces
  ts = tscs->parent.parent.ts;
  nb = lttv_traceset_number(ts);
  
  for(i = 0 ; i < nb ; i++) {
    tcs = (LttvTraceStats *)(LTTV_TRACESET_CONTEXT(tscs)->traces[i]);
    desc = ltt_trace_system_description(tcs->parent.parent.t);    
    sprintf(trace_str, "Trace on system %s at time %d secs", 
      	    desc->node_name,desc->trace_start.tv_sec);
    
    gtk_tree_store_append (store, &iter, NULL);  
    gtk_tree_store_set (store, &iter,NAME_COLUMN,trace_str,-1);  
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    str = gtk_tree_path_to_string (path);
    g_hash_table_insert(Statistic_Viewer_Data->Statistic_Hash,
			(gpointer)str,tcs->stats);
    show_tree(Statistic_Viewer_Data, tcs->stats, &iter);
  }
}

void show_tree(StatisticViewerData * Statistic_Viewer_Data,
	       LttvAttribute* stats,  GtkTreeIter* parent)
{
  int i, nb;
  LttvAttribute *subtree;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
  gchar * str, dir_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = Statistic_Viewer_Data->Store_M;

  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value);
    switch(type) {
     case LTTV_GOBJECT:
        if(LTTV_IS_ATTRIBUTE(*(value.v_gobject))) {
	  sprintf(dir_str, "%s", g_quark_to_string(name));
          subtree = (LttvAttribute *)*(value.v_gobject);
	  gtk_tree_store_append (store, &iter, parent);  
	  gtk_tree_store_set (store, &iter,NAME_COLUMN,dir_str,-1);  
	  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	  str = gtk_tree_path_to_string (path);
	  g_hash_table_insert(Statistic_Viewer_Data->Statistic_Hash,
			      (gpointer)str, subtree);
          show_tree(Statistic_Viewer_Data, subtree, &iter);
        }
        break;
      default:
	break;
    }
  }    
}

void show_statistic(StatisticViewerData * Statistic_Viewer_Data,
		    LttvAttribute* stats, GtkTextBuffer* buf)
{
  int i, nb , flag;
  LttvAttribute *subtree;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
  gchar type_name[PATH_LENGTH], type_value[PATH_LENGTH];
  GtkTextIter   text_iter;
  
  flag = 0;
  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value);
    sprintf(type_name,"%s", g_quark_to_string(name));
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
        sprintf(type_value, " :  %10u.%09u\n", value.v_time->tv_sec, 
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


