/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gmodule.h>

#include "interface.h"
#include "support.h"
#include "mainWindow.h"
#include "callbacks.h"

/* global variable */
systemView * gSysView;

typedef view_constructor (* constructor)();
constructor get_constructor = NULL;
typedef void (*call_Event_Selected_Hook)(void * call_data);
call_Event_Selected_Hook selected_hook = NULL;
GModule *gm;
view_constructor gConstructor = NULL;

int
main (int argc, char *argv[])
{
  GModule *gm;
  GtkWidget * ToolMenuTitle_menu, *insert_view;
  GtkWidget *window1;
  mainWindow * mw = g_new(mainWindow, 1);
  gSysView = g_new(systemView, 1);

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory ("pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  window1 = create_MWindow ();
  gtk_widget_show (window1);

  mw->MWindow = window1;
  mw->SystemView = gSysView;
  mw->Tab = NULL;
  mw->CurrentTab = NULL;
  //  mw->Attributes = lttv_attributes_new();

  //test

  gm = g_module_open("/home1/yangxx/poly/lttv/modules/libguiEvents.la",0);
  printf("Main : the address of gm : %d\n", gm);
  if(!g_module_symbol(gm, "get_constructor", (gpointer)&get_constructor)){
    g_error("can not get constructor\n");
  }  
  if(!g_module_symbol(gm, "call_Event_Selected_Hook", (gpointer)&selected_hook)){
    g_error("can not get selected hook\n");
  }  

  gConstructor = get_constructor();
  ToolMenuTitle_menu = lookup_widget(mw->MWindow,"ToolMenuTitle_menu");
  insert_view = gtk_menu_item_new_with_mnemonic (_("insert_view"));
  gtk_widget_show (insert_view);
  gtk_container_add (GTK_CONTAINER (ToolMenuTitle_menu), insert_view);
  g_signal_connect ((gpointer) insert_view, "activate",
                    G_CALLBACK (insertViewTest),
                    NULL);  
  //end

  gSysView->EventDB = NULL;
  gSysView->SystemInfo = NULL;
  gSysView->Options  = NULL;
  gSysView->Window = mw;
  gSysView->Next = NULL;

  g_object_set_data(G_OBJECT(window1), "systemView", (gpointer)gSysView);
  g_object_set_data(G_OBJECT(window1), "mainWindow", (gpointer)mw);

  gtk_main ();
  return 0;
}

