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

extern systemView * gSysView;
extern LttvTracesetContext * gTracesetContext;

/** Array containing instanced objects. */
extern GSList * Main_Window_List;

mainWindow * get_window_data_struct(GtkWidget * widget);

/* test part */
void insertView(GtkWidget* widget, view_constructor constructor);

void
on_textview1_grab_focus                     (GtkTextView     *text_view,
					  gpointer         user_data)
{
  GtkWidget * widget;
  GtkCustom * custom = (GtkCustom*)user_data;
  widget = gtk_widget_get_parent((GtkWidget*)text_view);
  widget = gtk_widget_get_parent(widget);
  gtk_custom_set_focus((GtkWidget*)custom, (gpointer)widget);
}

void
insertViewTest(GtkMenuItem *menuitem, gpointer user_data)
{
  guint val = 20;
  insertView((GtkWidget*)menuitem, (view_constructor)user_data);
  //  selected_hook(&val);
}

void
on_insert_viewer_test_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *scrolledwindow1, *textview1, *label;
  static int count = 0;
  char str[64];
  GtkCustom * custom;
  GtkTextBuffer* buf;
  
  mainWindow * mwData;  
  mwData = get_window_data_struct((GtkWidget*)menuitem);
  if(!mwData->CurrentTab) return;
  custom = mwData->CurrentTab->custom;

  sprintf(str,"label : %d",++count);
  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  label = gtk_label_new(str);
  gtk_widget_show(label);

  gtk_custom_widget_add(custom, scrolledwindow1);
  gtk_widget_set_size_request ((GtkWidget*)scrolledwindow1, 800, 100);
  
  textview1 = gtk_text_view_new ();
  gtk_widget_show (textview1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), textview1);
  buf =  gtk_text_view_get_buffer((GtkTextView*)textview1);
  sprintf(str,"text view : %d",count);
  gtk_text_buffer_set_text(buf,str, -1);
  
  g_signal_connect ((gpointer) textview1, "grab_focus",
		    G_CALLBACK (on_textview1_grab_focus), custom);
}

/* end of test part */


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
    gtk_custom_widget_add(custom, viewer);  
}

void get_label_string (GtkWidget * text, gchar * label) 
{
  GtkEntry * entry = (GtkEntry*)text;
  if(strlen(gtk_entry_get_text(entry))!=0)
    strcpy(label,gtk_entry_get_text(entry)); 
}

void get_label(GtkWindow * mw, gchar * str)
{
  GtkWidget * dialogue;
  GtkWidget * text;
  GtkWidget * label;
  gint id;

  strcpy(str,"Page");     //default label

  dialogue = gtk_dialog_new_with_buttons("Get the name of the tab",mw,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 

  label = gtk_label_new("Please input tab's name");
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
  GtkWidget  * mw = NULL; /* Main window */
  systemView * sv = NULL; /* System view */
  systemView * newSv;     /* New system view displayed in the new window */
  GtkWidget  * newWindow; /* New generated main window */
  mainWindow * newMWindow;/* New main window structure */

  //test
  int i;
  GtkWidget * ToolMenuTitle_menu, *insert_view, *pixmap;
  LttvMenus * menu;
  LttvToolbars * toolbar;
  lttv_menu_closure *menuItem;
  lttv_toolbar_closure *toolbarItem;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  view_constructor constructor;
  GdkPixbuf *pixbuf;
  //end

  mw = lookup_widget (widget, "MWindow");
  if(mw == NULL){
    g_printf("Can not find main window\n");
    return;
  }
  
  sv = (systemView *)g_object_get_data(G_OBJECT(mw),"systemView");
  if(sv == NULL){
    g_printf("Can not find system view\n");
    return;
  }  
    
  newMWindow = g_new(mainWindow, 1);

  /* Add the object's information to the module's array */
  Main_Window_List = g_slist_append(Main_Window_List, mw);

  newWindow  = create_MWindow();
  gtk_widget_show (newWindow);
  
  
  newSv = g_new(systemView, 1);
  while(sv->Next) sv = sv->Next;
  sv->Next = newSv;

  newSv->EventDB = NULL;
  newSv->SystemInfo = NULL;
  newSv->Options  = NULL;
  newSv->Next = NULL;
  newSv->Window = newMWindow;

  newMWindow->MWindow = newWindow;
  newMWindow->Tab = NULL;
  newMWindow->CurrentTab = NULL;
  newMWindow->SystemView = newSv;
  newMWindow->Attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  newMWindow->traceset_context = LTTV_TRACESET_CONTEXT(gTracesetContext);
  newMWindow->traceset = (LTTV_TRACESET_CONTEXT(gTracesetContext))->ts;
  g_object_ref(gTracesetContext);

  //test yxx
  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(menu){
    for(i=0;i<menu->len;i++){
      menuItem = &g_array_index(menu, lttv_menu_closure, i);
      constructor = menuItem->con;
      ToolMenuTitle_menu = lookup_widget(newMWindow->MWindow,"ToolMenuTitle_menu");
      insert_view = gtk_menu_item_new_with_mnemonic (menuItem->menuText);
      gtk_widget_show (insert_view);
      gtk_container_add (GTK_CONTAINER (ToolMenuTitle_menu), insert_view);
      g_signal_connect ((gpointer) insert_view, "activate",
			G_CALLBACK (insertViewTest),
			constructor);  
    }
  }
  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar){
    for(i=0;i<toolbar->len;i++){
      toolbarItem = &g_array_index(toolbar, lttv_toolbar_closure, i);
      constructor = toolbarItem->con;
      ToolMenuTitle_menu = lookup_widget(newMWindow->MWindow,"MToolbar2");
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
    }
  }
  //end
  
  g_object_set_data(G_OBJECT(newWindow), "systemView", (gpointer)newSv);    
  g_object_set_data(G_OBJECT(newWindow), "mainWindow", (gpointer)newMWindow);    

  if(clone){
    g_printf("Clone : use the same traceset\n");
  }else{
    g_printf("Empty : traceset is set to NULL\n");
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
  g_printf("Open a trace set\n");
}

void add_trace(GtkWidget * widget, gpointer user_data)
{
  g_printf("add a trace to a trace set\n");
}

void remove_trace(GtkWidget * widget, gpointer user_data)
{
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
  GList * list;
  gchar label[64];

  tab * tmpTab;
  GtkWidget * pane;
  mainWindow * mwData;
  GtkNotebook * notebook = (GtkNotebook *)lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }

  mwData = get_window_data_struct((GtkWidget*)menuitem);

  tmpTab = mwData->Tab;
  while(tmpTab && tmpTab->Next) tmpTab = tmpTab->Next;
  if(!tmpTab){
    mwData->CurrentTab = NULL;
    tmpTab = g_new(tab,1);
    tmpTab->traceStartTime.tv_sec  = 0;
    tmpTab->traceStartTime.tv_nsec = 0;
    tmpTab->traceEndTime.tv_sec    = G_MAXULONG;
    tmpTab->traceEndTime.tv_nsec   = G_MAXULONG;
    tmpTab->startTime.tv_sec       = 0;
    tmpTab->startTime.tv_nsec      = 0;
    tmpTab->endTime.tv_sec         = G_MAXULONG;
    tmpTab->endTime.tv_nsec        = G_MAXULONG;
    tmpTab->currentTime.tv_sec     = 0;
    tmpTab->currentTime.tv_nsec    = 0;
    mwData->Tab = tmpTab;    
  }else{
    tmpTab->Next = g_new(tab,1);
    tmpTab = tmpTab->Next;
  }
  if(mwData->CurrentTab){
    tmpTab->traceStartTime = mwData->CurrentTab->traceStartTime;
    tmpTab->traceEndTime   = mwData->CurrentTab->traceEndTime;
    tmpTab->startTime      = mwData->CurrentTab->startTime;
    tmpTab->endTime        = mwData->CurrentTab->endTime;
    tmpTab->currentTime    = mwData->CurrentTab->currentTime;
  }
  tmpTab->Attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  //  mwData->CurrentTab = tmpTab;
  tmpTab->custom = (GtkCustom*)gtk_custom_new();
  gtk_widget_show((GtkWidget*)tmpTab->custom);
  tmpTab->Next = NULL;    

  get_label((GtkWindow*)mwData->MWindow, label);
  tmpTab->label = gtk_label_new (label);
  gtk_widget_show (tmpTab->label);

  gtk_notebook_append_page(notebook, (GtkWidget*)tmpTab->custom, tmpTab->label); 
  
  list = gtk_container_get_children(GTK_CONTAINER(notebook));
  gtk_notebook_set_current_page(notebook,g_list_length(list)-1);
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
  g_printf("Close\n");
}


void
on_close_tab_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Close tab\n");
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
  on_MWindow_destroy(GTK_OBJECT(menuitem), user_data);
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
  g_printf("Load module\n");
}


void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Unload module\n");
}


void
on_add_module_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Add module search path\n");
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
  systemView * sv = gSysView;
  gint count = 0;
  while(sv->Next){
    g_printf("There are : %d windows\n",++count);
    sv = sv->Next;
  }
  g_printf("There are : %d windows\n",++count);
  
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

