/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers and XangXiu Yang
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

/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <lttvwindow/menu.h>
#include <lttvwindow/toolbar.h>

#include "interface.h"
#include "support.h"
#include <lttvwindow/mainwindow.h>
#include "callbacks.h"
#include <ltt/trace.h>


/** Array containing instanced objects. */
GSList * g_main_window_list = NULL ;

LttvHooks
  *main_hooks;

/* Initial trace from command line */
LttvTrace *g_init_trace = NULL;

static char *a_trace;

void lttv_trace_option(void *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  g_init_trace = lttv_trace_new(trace);
}

/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the GUI.
 */

static gboolean window_creation_hook(void *hook_data, void *call_data)
{
  g_debug("GUI window_creation_hook()");
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&lttv_argc, &lttv_argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory ("pixmaps");
  add_pixmap_directory ("../modules/gui/main/pixmaps");

  construct_main_window(NULL);

  gtk_main ();

  return FALSE;
}

static void init() {

  LttvAttributeValue value;
 
  // Global attributes only used for interaction with main() here.
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  g_debug("GUI init()");
  
  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  g_assert((main_hooks = *(value.v_pointer)) != NULL);

  lttv_hooks_add(main_hooks, window_creation_hook, NULL);

}

void
main_window_free(MainWindow * mw)
{ 
  if(mw){
    while(mw->tab){
      lttv_state_remove_event_hooks(
           (LttvTracesetState*)mw->tab->traceset_info->traceset_context);
      mw->tab = mw->tab->next;
    }
    g_object_unref(mw->attributes);
    g_main_window_list = g_slist_remove(g_main_window_list, mw);

    g_hash_table_destroy(mw->hash_menu_item);
    g_hash_table_destroy(mw->hash_toolbar_item);
    
    g_free(mw);
    mw = NULL;
  }
}

void
main_window_destructor(MainWindow * mw)
{
  if(GTK_IS_WIDGET(mw->mwindow)){
    gtk_widget_destroy(mw->mwindow);
    //    gtk_widget_destroy(mw->HelpContents);
    //    gtk_widget_destroy(mw->AboutBox);    
    mw = NULL;
  }
  //main_window_free called when the object mw in the widget is unref.
  //main_window_free(mw);
}


void main_window_destroy_walk(gpointer data, gpointer user_data)
{
  main_window_destructor((MainWindow*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {

  LttvAttributeValue value;  
  LttvTrace *trace;

  lttv_option_remove("trace");

  lttv_hooks_remove_data(main_hooks, window_creation_hook, NULL);

  g_debug("GUI destroy()");

  if(g_main_window_list){
    g_slist_foreach(g_main_window_list, main_window_destroy_walk, NULL );
    g_slist_free(g_main_window_list);
  }
  
}


LTTV_MODULE("lttvwindow", "Viewer main window", \
    "Viewer with multiple windows, tabs and panes for graphical modules", \
	    init, destroy, "stats")
