#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gmodule.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <lttv/mainWindow.h>
#include <lttv/menu.h>
#include <lttv/toolbar.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/module.h>
#include <lttv/gtkdirsel.h>
#include <lttv/iattribute.h>

#define PATH_LENGTH          256
#define DEFAULT_TIME_WIDTH_S   1

//extern LttvTracesetContext * gTracesetContext;
extern LttTrace *gInit_Trace ;


/** Array containing instanced objects. */
extern GSList * Main_Window_List;

static int gWinCount = 0;

mainWindow * get_window_data_struct(GtkWidget * widget);
char * get_unload_module(char ** loaded_module_name, int nb_module);
char * get_remove_trace(char ** all_trace_name, int nb_trace);
char * get_selection(char ** all_name, int nb, char *title, char * column_title);
void * create_tab(GtkWidget* parent, GtkNotebook * notebook, char * label);

void insertView(GtkWidget* widget, view_constructor constructor);

enum
{
  MODULE_COLUMN,
  N_COLUMNS
};


void
insertViewTest(GtkMenuItem *menuitem, gpointer user_data)
{
  guint val = 20;
  insertView((GtkWidget*)menuitem, (view_constructor)user_data);
  //  selected_hook(&val);
}


/* internal functions */
void insertView(GtkWidget* widget, view_constructor constructor)
{
  GtkCustom * custom;
  mainWindow * mwData;  
  GtkWidget * viewer;

  mwData = get_window_data_struct(widget);
  if(!mwData->CurrentTab) return;
  custom = mwData->CurrentTab->custom;

  viewer = (GtkWidget*)constructor(mwData);
  if(viewer)
  {
    gtk_custom_widget_add(custom, viewer); 
    // Added by MD
    //    g_object_unref(G_OBJECT(viewer));
  }
}

void get_label_string (GtkWidget * text, gchar * label) 
{
  GtkEntry * entry = (GtkEntry*)text;
  if(strlen(gtk_entry_get_text(entry))!=0)
    strcpy(label,gtk_entry_get_text(entry)); 
}

void get_label(GtkWindow * mw, gchar * str, gchar* dialogue_title, gchar * label_str)
{
  GtkWidget * dialogue;
  GtkWidget * text;
  GtkWidget * label;
  gint id;

  dialogue = gtk_dialog_new_with_buttons(dialogue_title,NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 

  label = gtk_label_new(label_str);
  gtk_widget_show(label);

  text = gtk_entry_new();
  gtk_widget_show(text);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), label,TRUE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), text,FALSE, FALSE,0);

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
      get_label_string(text,str);
      gtk_widget_destroy(dialogue);
      break;
    case GTK_RESPONSE_REJECT:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }
}

mainWindow * get_window_data_struct(GtkWidget * widget)
{
  GtkWidget * mw;
  mainWindow * mwData;

  mw = lookup_widget(widget, "MWindow");
  if(mw == NULL){
    g_printf("Main window does not exist\n");
    return;
  }
  
  mwData = (mainWindow *) g_object_get_data(G_OBJECT(mw),"mainWindow");
  if(mwData == NULL){
    g_printf("Main window data does not exist\n");
    return;
  }
  return mwData;
}

void createNewWindow(GtkWidget* widget, gpointer user_data, gboolean clone)
{
  mainWindow * parent = get_window_data_struct(widget);

  if(clone){
    g_printf("Clone : use the same traceset\n");
    constructMainWin(parent, NULL, FALSE);
  }else{
    g_printf("Empty : traceset is set to NULL\n");
    constructMainWin(NULL, parent->winCreationData, FALSE);
  }
}

void move_up_viewer(GtkWidget * widget, gpointer user_data)
{
  mainWindow * mw = get_window_data_struct(widget);
  if(!mw->CurrentTab) return;
  gtk_custom_widget_move_up(mw->CurrentTab->custom);
}

void move_down_viewer(GtkWidget * widget, gpointer user_data)
{
  mainWindow * mw = get_window_data_struct(widget);
  if(!mw->CurrentTab) return;
  gtk_custom_widget_move_down(mw->CurrentTab->custom);
}

void delete_viewer(GtkWidget * widget, gpointer user_data)
{
  mainWindow * mw = get_window_data_struct(widget);
  if(!mw->CurrentTab) return;
  gtk_custom_widget_delete(mw->CurrentTab->custom);
}

void open_traceset(GtkWidget * widget, gpointer user_data)
{
  char ** dir;
  gint id;
  LttvTraceset * traceset;
  mainWindow * mwData = get_window_data_struct(widget);
  GtkFileSelection * fileSelector = 
    (GtkFileSelection *)gtk_file_selection_new("Select a traceset");

  gtk_file_selection_hide_fileop_buttons(fileSelector);
  
  id = gtk_dialog_run(GTK_DIALOG(fileSelector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (fileSelector);
      traceset = lttv_traceset_load(dir[0]);
      g_printf("Open a trace set %s\n", dir[0]); 
      //Not finished yet
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)fileSelector);
      break;
  }

}

void add_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  char * dir;
  gint id;
  mainWindow * mwData = get_window_data_struct(widget);
  GtkDirSelection * fileSelector = (GtkDirSelection *)gtk_dir_selection_new("Select a trace");
  gtk_dir_selection_hide_fileop_buttons(fileSelector);
  
  id = gtk_dialog_run(GTK_DIALOG(fileSelector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (fileSelector);
      trace = ltt_trace_open(dir);
      if(trace == NULL) g_critical("cannot open trace %s", dir);
      trace_v = lttv_trace_new(trace);
      traceset = mwData->Traceset_Info->traceset;
      if(mwData->Traceset_Info->TracesetContext != NULL){
	lttv_context_fini(LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext));
	g_object_unref(mwData->Traceset_Info->TracesetContext);
      }
      lttv_traceset_add(traceset, trace_v);
      mwData->Traceset_Info->TracesetContext =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
      lttv_context_init(
	LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext),traceset);      
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)fileSelector);
      break;
  }

  g_printf("add a trace to a trace set\n");
}

void remove_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  gint i, nb_trace;
  char ** name, *remove_trace_name;
  mainWindow * mwData = get_window_data_struct(widget);
  
  nb_trace =lttv_traceset_number(mwData->Traceset_Info->traceset); 
  name = g_new(char*,nb_trace);
  for(i = 0; i < nb_trace; i++){
    trace_v = lttv_traceset_get(mwData->Traceset_Info->traceset, i);
    trace = lttv_trace(trace_v);
    name[i] = trace->pathname;
  }

  remove_trace_name = get_remove_trace(name, nb_trace);

  if(remove_trace_name){
    for(i=0; i<nb_trace; i++){
      if(strcmp(remove_trace_name,name[i]) == 0){
	traceset = mwData->Traceset_Info->traceset;
	if(mwData->Traceset_Info->TracesetContext != NULL){
	  lttv_context_fini(LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext));
	  g_object_unref(mwData->Traceset_Info->TracesetContext);
	}
	lttv_traceset_remove(traceset, i);
	mwData->Traceset_Info->TracesetContext =
	  g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
	lttv_context_init(
	  LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext),traceset);      
	break;
      }
    }
  }

  g_free(name);
  g_printf("remove a trace from a trace set\n");
}

void save(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save\n");
}

void save_as(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save as\n");
}

void zoom_in(GtkWidget * widget, gpointer user_data)
{
  g_printf("Zoom in\n");
}

void zoom_out(GtkWidget * widget, gpointer user_data)
{
  g_printf("Zoom out\n");
}

void zoom_extended(GtkWidget * widget, gpointer user_data)
{
  g_printf("Zoom extended\n");
}

void go_to_time(GtkWidget * widget, gpointer user_data)
{
  g_printf("Go to time\n");
}

void show_time_frame(GtkWidget * widget, gpointer user_data)
{
  g_printf("Show time frame\n");  
}


/* callback function */

void
on_empty_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  createNewWindow((GtkWidget*)menuitem, user_data, FALSE);
}


void
on_clone_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  createNewWindow((GtkWidget*)menuitem, user_data, TRUE);
}


void
on_tab_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gchar label[PATH_LENGTH];
  GtkNotebook * notebook = (GtkNotebook *)lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }

  strcpy(label,"Page");
  get_label(NULL, label,"Get the name of the tab","Please input tab's name");

  create_tab ((GtkWidget*)menuitem, notebook, label);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)menuitem, user_data);
}


void
on_close_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  mainWindow * mwData = get_window_data_struct((GtkWidget*)menuitem);
  mainWindow_Destructor(mwData);  
}


void
on_close_tab_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  int count = 0;
  GtkWidget * notebook;
  tab * tmp;
  mainWindow * mwData = get_window_data_struct((GtkWidget*)menuitem);
  notebook = lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  
  if(mwData->Tab == mwData->CurrentTab){
    //    tmp = mwData->CurrentTab;
    //    mwData->Tab = mwData->CurrentTab->Next;
    g_printf("The default TAB can not be deleted\n");
    return;
  }else{
    tmp = mwData->Tab;
    while(tmp != mwData->CurrentTab){
      tmp = tmp->Next;
      count++;
    }
  }

  gtk_notebook_remove_page((GtkNotebook*)notebook, count);  
}


void
on_add_trace_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)menuitem, user_data);
}


void
on_remove_trace_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)menuitem, user_data);
}


void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save((GtkWidget*)menuitem, user_data);
}


void
on_save_as_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)menuitem, user_data);
}


void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_main_quit ();
}


void
on_cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Cut\n");
}


void
on_copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Copye\n");
}


void
on_paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Paste\n");
}


void
on_delete_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Delete\n");
}


void
on_zoom_in_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_out_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_extended_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)menuitem, user_data); 
}


void
on_go_to_time_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)menuitem, user_data); 
}


void
on_show_time_frame_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)menuitem, user_data); 
}


void
on_move_viewer_up_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)menuitem, user_data);
}


void
on_move_viewer_down_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)menuitem, user_data);
}


void
on_remove_viewer_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)menuitem, user_data);
}


void
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  char ** dir;
  gint id;
  char str[PATH_LENGTH], *str1;
  mainWindow * mwData = get_window_data_struct((GtkWidget*)menuitem);
  GtkFileSelection * fileSelector = (GtkFileSelection *)gtk_file_selection_new("Select a module");
  gtk_file_selection_hide_fileop_buttons(fileSelector);
  
  str[0] = '\0';
  id = gtk_dialog_run(GTK_DIALOG(fileSelector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (fileSelector);
      sprintf(str,dir[0]);
      str1 = strrchr(str,'/');
      if(str1)str1++;
      else{
	str1 = strrchr(str,'\\');
	str1++;
      }
      if(mwData->winCreationData)
	lttv_module_load(str1, mwData->winCreationData->argc,mwData->winCreationData->argv);
      else
	lttv_module_load(str1, 0,NULL);
      g_slist_foreach(Main_Window_List, insertMenuToolbarItem, NULL);
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)fileSelector);
      break;
  }
  g_printf("Load module: %s\n", str);
}


void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  int i;
  char **name, *unload_module_name;
  guint nb;
  LttvModule ** modules, *module;
  mainWindow * mwData = get_window_data_struct((GtkWidget*)menuitem);
  
  modules = lttv_module_list(&nb);
  name  = g_new(char*, nb);
  for(i=0;i<nb;i++){
    module = modules[i];
    name[i] = lttv_module_name(module);
  }

  unload_module_name =get_unload_module(name,nb);
  
  if(unload_module_name){
    for(i=0;i<nb;i++){
      if(strcmp(unload_module_name, name[i]) == 0){
	lttv_module_unload(modules[i]);
	break;
      }
    }    
  }

  g_free(name);
}


void
on_add_module_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkDirSelection * fileSelector = (GtkDirSelection *)gtk_dir_selection_new("Select module path");
  char * dir;
  gint id;

  mainWindow * mwData = get_window_data_struct((GtkWidget*)menuitem);

  id = gtk_dialog_run(GTK_DIALOG(fileSelector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (fileSelector);
      lttv_module_path_add(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)fileSelector);
      break;
  }
}


void
on_color_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Color\n");
}


void
on_filter_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Filter\n");
}


void
on_save_configuration_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Save configuration\n");
}


void
on_content_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Content\n");
}


void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("About...\n");
}


void
on_button_new_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  createNewWindow((GtkWidget*)button, user_data, FALSE);
}


void
on_button_open_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)button, user_data);
}


void
on_button_add_trace_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)button, user_data);
}


void
on_button_remove_trace_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)button, user_data);
}


void
on_button_save_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  save((GtkWidget*)button, user_data);
}


void
on_button_save_as_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)button, user_data);
}


void
on_button_zoom_in_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)button, user_data); 
}


void
on_button_zoom_out_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)button, user_data); 
}


void
on_button_zoom_extended_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)button, user_data); 
}


void
on_button_go_to_time_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)button, user_data); 
}


void
on_button_show_time_frame_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)button, user_data); 
}


void
on_button_move_up_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)button, user_data);
}


void
on_button_move_down_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)button, user_data);
}


void
on_button_delete_viewer_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)button, user_data);
}

void
on_MWindow_destroy                     (GtkObject       *object,
                                        gpointer         user_data)
{
  mainWindow *Main_Window = (mainWindow*)user_data;
  
  g_printf("There are : %d windows\n",g_slist_length(Main_Window_List));

  gWinCount--;
  if(gWinCount == 0)
    gtk_main_quit ();
}


void
on_MNotebook_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
  mainWindow * mw = get_window_data_struct((GtkWidget*)notebook);
  tab * Tab = mw->Tab;
 
  while(page_num){
    Tab = Tab->Next;
    page_num--;
  }
  mw->CurrentTab = Tab;
}

char * get_remove_trace(char ** all_trace_name, int nb_trace)
{
  return get_selection(all_trace_name, nb_trace, 
		       "Select a trace", "Trace pathname");
}
char * get_unload_module(char ** loaded_module_name, int nb_module)
{
  return get_selection(loaded_module_name, nb_module, 
		       "Select an unload module", "Module pathname");
}

char * get_selection(char ** loaded_module_name, int nb_module,
		     char *title, char * column_title)
{
  GtkWidget         * dialogue;
  GtkWidget         * scroll_win;
  GtkWidget         * tree;
  GtkListStore      * store;
  GtkTreeViewColumn * column;
  GtkCellRenderer   * renderer;
  GtkTreeSelection  * select;
  GtkTreeIter         iter;
  gint                id, i;
  char              * unload_module_name = NULL;

  dialogue = gtk_dialog_new_with_buttons(title,
					 NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 
  gtk_window_set_default_size((GtkWindow*)dialogue, 500, 200);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show ( scroll_win);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  store = gtk_list_store_new (N_COLUMNS,G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));
  gtk_widget_show ( tree);
  g_object_unref (G_OBJECT (store));
		
  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes (column_title,
						     renderer,
						     "text", MODULE_COLUMN,
						     NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_fixed_width (column, 150);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  gtk_container_add (GTK_CONTAINER (scroll_win), tree);  

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), scroll_win,TRUE, TRUE,0);

  for(i=0;i<nb_module;i++){
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, MODULE_COLUMN,loaded_module_name[i],-1);
  }

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      if (gtk_tree_selection_get_selected (select, (GtkTreeModel**)&store, &iter)){
	  gtk_tree_model_get ((GtkTreeModel*)store, &iter, MODULE_COLUMN, &unload_module_name, -1);
      }
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }

  return unload_module_name;
}

void main_window_destroy_hash_key(gpointer key)
{
  g_free(key);
}

void main_window_destroy_hash_data(gpointer data)
{
}


void insertMenuToolbarItem(mainWindow * mw, gpointer user_data)
{
  int i;
  GdkPixbuf *pixbuf;
  view_constructor constructor;
  LttvMenus * menu;
  LttvToolbars * toolbar;
  lttv_menu_closure *menuItem;
  lttv_toolbar_closure *toolbarItem;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  GtkWidget * ToolMenuTitle_menu, *insert_view, *pixmap, *tmp;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(menu){
    for(i=0;i<menu->len;i++){
      menuItem = &g_array_index(menu, lttv_menu_closure, i);
      tmp = g_hash_table_lookup(mw->hash_menu_item, g_strdup(menuItem->menuText));
      if(tmp)continue;
      constructor = menuItem->con;
      ToolMenuTitle_menu = lookup_widget(mw->MWindow,"ToolMenuTitle_menu");
      insert_view = gtk_menu_item_new_with_mnemonic (menuItem->menuText);
      gtk_widget_show (insert_view);
      gtk_container_add (GTK_CONTAINER (ToolMenuTitle_menu), insert_view);
      g_signal_connect ((gpointer) insert_view, "activate",
			G_CALLBACK (insertViewTest),
			constructor);  
      g_hash_table_insert(mw->hash_menu_item, g_strdup(menuItem->menuText),
			  insert_view);
    }
  }

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar){
    for(i=0;i<toolbar->len;i++){
      toolbarItem = &g_array_index(toolbar, lttv_toolbar_closure, i);
      tmp = g_hash_table_lookup(mw->hash_toolbar_item, g_strdup(toolbarItem->tooltip));
      if(tmp)continue;
      constructor = toolbarItem->con;
      ToolMenuTitle_menu = lookup_widget(mw->MWindow,"MToolbar2");
      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)toolbarItem->pixmap);
      pixmap = gtk_image_new_from_pixbuf(pixbuf);
      insert_view = gtk_toolbar_append_element (GTK_TOOLBAR (ToolMenuTitle_menu),
						GTK_TOOLBAR_CHILD_BUTTON,
						NULL,
						"",
						toolbarItem->tooltip, NULL,
						pixmap, NULL, NULL);
      gtk_label_set_use_underline (GTK_LABEL (((GtkToolbarChild*) (g_list_last (GTK_TOOLBAR (ToolMenuTitle_menu)->children)->data))->label), TRUE);
      gtk_widget_show (insert_view);
      gtk_container_set_border_width (GTK_CONTAINER (insert_view), 1);
      g_signal_connect ((gpointer) insert_view, "clicked",G_CALLBACK (insertViewTest),constructor);       
      g_hash_table_insert(mw->hash_toolbar_item, g_strdup(toolbarItem->tooltip),
			  insert_view);
    }
  }
}

void constructMainWin(mainWindow * parent, WindowCreationData * win_creation_data,
		      gboolean first_window)
{
  g_critical("constructMainWin()");
  GtkWidget  * newWindow; /* New generated main window */
  mainWindow * newMWindow;/* New main window structure */
  GtkNotebook * notebook;
  LttvIAttribute *attributes =
	  LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  LttvAttributeValue value;
         
  newMWindow = g_new(mainWindow, 1);

  // Add the object's information to the module's array 
  Main_Window_List = g_slist_append(Main_Window_List, newMWindow);


  newWindow  = create_MWindow();
  gtk_widget_show (newWindow);
    
  newMWindow->Attributes = attributes;
  
  newMWindow->Traceset_Info = g_new(TracesetInfo,1);
  newMWindow->Traceset_Info->path = NULL ;


  newMWindow->Traceset_Info->before_traceset = lttv_hooks_new();
  newMWindow->Traceset_Info->after_traceset = lttv_hooks_new();
  newMWindow->Traceset_Info->before_trace = lttv_hooks_new();
  newMWindow->Traceset_Info->after_trace = lttv_hooks_new();
  newMWindow->Traceset_Info->before_tracefile = lttv_hooks_new();
  newMWindow->Traceset_Info->after_tracefile = lttv_hooks_new();
  newMWindow->Traceset_Info->before_event = lttv_hooks_new();
  newMWindow->Traceset_Info->after_event = lttv_hooks_new();

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->before_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->after_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->before_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->after_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->before_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->after_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->before_event;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = newMWindow->Traceset_Info->after_event;
 
  
  newMWindow->MWindow = newWindow;
  newMWindow->Tab = NULL;
  newMWindow->CurrentTab = NULL;
  newMWindow->Attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  if(parent){
    newMWindow->Traceset_Info->traceset = 
        lttv_traceset_copy(parent->Traceset_Info->traceset);
    
//FIXME copy not implemented in lower level
    newMWindow->Traceset_Info->TracesetContext =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    lttv_context_init(
	LTTV_TRACESET_CONTEXT(newMWindow->Traceset_Info->TracesetContext),
	newMWindow->Traceset_Info->traceset);
  //newMWindow->traceset_context = parent->traceset_context;
    newMWindow->winCreationData = parent->winCreationData;
  }else{
    newMWindow->Traceset_Info->traceset = lttv_traceset_new();

    /* Add the command line trace */
    if(gInit_Trace != NULL && first_window)
      lttv_traceset_add(newMWindow->Traceset_Info->traceset, gInit_Trace);
    /* NOTE : the context must be recreated if we change the traceset,
     * ie : adding/removing traces */
    newMWindow->Traceset_Info->TracesetContext =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    lttv_context_init(
	LTTV_TRACESET_CONTEXT(newMWindow->Traceset_Info->TracesetContext),
	newMWindow->Traceset_Info->traceset);

    newMWindow->winCreationData = win_creation_data;
  }

  newMWindow->hash_menu_item = g_hash_table_new_full (g_str_hash, g_str_equal,
					      main_window_destroy_hash_key, 
					      main_window_destroy_hash_data);
  newMWindow->hash_toolbar_item = g_hash_table_new_full (g_str_hash, g_str_equal,
					      main_window_destroy_hash_key, 
					      main_window_destroy_hash_data);

  insertMenuToolbarItem(newMWindow, NULL);
  
  g_object_set_data(G_OBJECT(newWindow), "mainWindow", (gpointer)newMWindow);    

  //create a default tab
  notebook = (GtkNotebook *)lookup_widget(newMWindow->MWindow, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  //for now there is no name field in LttvTraceset structure
  //Use "Traceset" as the label for the default tab
  create_tab(newMWindow->MWindow, notebook,"Traceset");

  g_object_set_data_full(
			G_OBJECT(newMWindow->MWindow),
			"Main_Window_Data",
			newMWindow,
			(GDestroyNotify)mainWindow_free);

  gWinCount++;
}

void Tab_Destructor(tab *Tab)
{
  if(Tab->Attributes)
    g_object_unref(Tab->Attributes);  

  if(Tab->mw->Tab == Tab){
    Tab->mw->Tab = Tab->Next;
  }else{
    tab * tmp1, *tmp = Tab->mw->Tab;
    while(tmp != Tab){
      tmp1 = tmp;
      tmp = tmp->Next;
    }
    tmp1->Next = Tab->Next;
  }
  g_free(Tab);
}

void * create_tab(GtkWidget* parent, GtkNotebook * notebook, char * label)
{
  GList * list;
  tab * tmpTab;
  mainWindow * mwData;
  LttTime TmpTime;

  mwData = get_window_data_struct(parent);
  tmpTab = mwData->Tab;
  while(tmpTab && tmpTab->Next) tmpTab = tmpTab->Next;
  if(!tmpTab){
    mwData->CurrentTab = NULL;
    tmpTab = g_new(tab,1);
    mwData->Tab = tmpTab;    
  }else{
    tmpTab->Next = g_new(tab,1);
    tmpTab = tmpTab->Next;
  }
  if(mwData->CurrentTab){
 // Will have to read directly at the main window level, as we want
 // to be able to modify a traceset on the fly.
 //   tmpTab->traceStartTime = mwData->CurrentTab->traceStartTime;
 //   tmpTab->traceEndTime   = mwData->CurrentTab->traceEndTime;
    tmpTab->Time_Window      = mwData->CurrentTab->Time_Window;
    tmpTab->currentTime    = mwData->CurrentTab->currentTime;
  }else{
 // Will have to read directly at the main window level, as we want
 // to be able to modify a traceset on the fly.
  //  getTracesetTimeSpan(mwData,&tmpTab->traceStartTime, &tmpTab->traceEndTime);
    tmpTab->Time_Window.startTime   = 
	    LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext)->Time_Span->startTime;
    if(DEFAULT_TIME_WIDTH_S <
              LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext)->Time_Span->endTime.tv_sec)
      TmpTime.tv_sec = DEFAULT_TIME_WIDTH_S;
    else
      TmpTime.tv_sec =
              LTTV_TRACESET_CONTEXT(mwData->Traceset_Info->TracesetContext)->Time_Span->endTime.tv_sec;
    TmpTime.tv_nsec = 0;
    tmpTab->Time_Window.Time_Width = TmpTime ;
    tmpTab->currentTime.tv_sec = TmpTime.tv_sec / 2;
    tmpTab->currentTime.tv_nsec = 0 ;
  }
  tmpTab->Attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  //  mwData->CurrentTab = tmpTab;
  tmpTab->custom = (GtkCustom*)gtk_custom_new();
  tmpTab->custom->mw = mwData;
  gtk_widget_show((GtkWidget*)tmpTab->custom);
  tmpTab->Next = NULL;    
  tmpTab->mw   = mwData;

  tmpTab->label = gtk_label_new (label);
  gtk_widget_show (tmpTab->label);

  g_object_set_data_full(
           G_OBJECT(tmpTab->custom),
           "Tab_Info",
	   tmpTab,
	   (GDestroyNotify)Tab_Destructor);
  
  gtk_notebook_append_page(notebook, (GtkWidget*)tmpTab->custom, tmpTab->label);  
  list = gtk_container_get_children(GTK_CONTAINER(notebook));
  gtk_notebook_set_current_page(notebook,g_list_length(list)-1);
}

void remove_menu_item(gpointer main_win, gpointer user_data)
{
  mainWindow * mw = (mainWindow *) main_win;
  lttv_menu_closure *menuItem = (lttv_menu_closure *)user_data;
  GtkWidget * ToolMenuTitle_menu, *insert_view;

  ToolMenuTitle_menu = lookup_widget(mw->MWindow,"ToolMenuTitle_menu");
  insert_view = (GtkWidget*)g_hash_table_lookup(mw->hash_menu_item,
						menuItem->menuText);
  if(insert_view){
    g_hash_table_remove(mw->hash_menu_item, menuItem->menuText);
    gtk_container_remove (GTK_CONTAINER (ToolMenuTitle_menu), insert_view);
  }
}

void remove_toolbar_item(gpointer main_win, gpointer user_data)
{
  mainWindow * mw = (mainWindow *) main_win;
  lttv_toolbar_closure *toolbarItem = (lttv_toolbar_closure *)user_data;
  GtkWidget * ToolMenuTitle_menu, *insert_view;


  ToolMenuTitle_menu = lookup_widget(mw->MWindow,"MToolbar2");
  insert_view = (GtkWidget*)g_hash_table_lookup(mw->hash_toolbar_item,
						toolbarItem->tooltip);
  if(insert_view){
    g_hash_table_remove(mw->hash_toolbar_item, toolbarItem->tooltip);
    gtk_container_remove (GTK_CONTAINER (ToolMenuTitle_menu), insert_view);
  }
}

/**
 * Remove menu and toolbar item when a module unloaded
 */

void main_window_remove_menu_item(lttv_constructor constructor)
{
  int i;
  LttvMenus * menu;
  lttv_menu_closure *menuItem;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(menu){
    for(i=0;i<menu->len;i++){
      menuItem = &g_array_index(menu, lttv_menu_closure, i);
      if(menuItem->con != constructor) continue;
      if(Main_Window_List){
	g_slist_foreach(Main_Window_List, remove_menu_item, menuItem);
      }
      break;
    }
  }
  
}

void main_window_remove_toolbar_item(lttv_constructor constructor)
{
  int i;
  LttvToolbars * toolbar;
  lttv_toolbar_closure *toolbarItem;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar){
    for(i=0;i<toolbar->len;i++){
      toolbarItem = &g_array_index(toolbar, lttv_toolbar_closure, i);
      if(toolbarItem->con != constructor) continue;
      if(Main_Window_List){
	g_slist_foreach(Main_Window_List, remove_toolbar_item, toolbarItem);
      }
      break;
    }
  }
}
