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
#include <lttv/processTrace.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <lttv/menu.h>
#include <lttv/toolbar.h>

#include "interface.h"
#include "support.h"
#include <lttv/mainWindow.h>
#include "callbacks.h"

/* global variable */
//LttvTracesetStats * gTracesetContext = NULL;
//static LttvTraceset * traceset;
WindowCreationData  gWinCreationData;

/** Array containing instanced objects. */
GSList * Main_Window_List = NULL ;

LttvHooks
  *main_hooks;

/* Initial trace from command line */
LttvTrace *gInit_Trace = NULL;

static char *a_trace;

void lttv_trace_option(void *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  gInit_Trace = lttv_trace_new(trace);
}

/*****************************************************************************
 *                 Functions for module loading/unloading                    *
 *****************************************************************************/
/**
 * plugin's init function
 *
 * This function initializes the GUI.
 */

static gboolean Window_Creation_Hook(void *hook_data, void *call_data)
{
  WindowCreationData* Window_Creation_Data = (WindowCreationData*)hook_data;

  g_critical("GUI Window_Creation_Hook()");
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&(Window_Creation_Data->argc), &(Window_Creation_Data->argv));

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory ("pixmaps");
  add_pixmap_directory ("modules/gui/mainWin/pixmaps");

  constructMainWin(NULL, Window_Creation_Data, TRUE);

  gtk_main ();

  return FALSE;
}

G_MODULE_EXPORT void init(LttvModule *self, int argc, char *argv[]) {

  LttvAttributeValue value;
 
  // Global attributes only used for interaction with main() here.
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  g_critical("GUI init()");
  
  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  g_assert((main_hooks = *(value.v_pointer)) != NULL);

  gWinCreationData.argc = argc;
  gWinCreationData.argv = argv;
  
  lttv_hooks_add(main_hooks, Window_Creation_Hook, &gWinCreationData);

}

void
mainWindow_free(mainWindow * mw)
{ 
  guint i, nb, ref_count;
  LttvTrace * trace;

  if(mw){

g_critical("begin remove");
    lttv_hooks_destroy(mw->Traceset_Info->before_traceset);
    lttv_hooks_destroy(mw->Traceset_Info->after_traceset);
    lttv_hooks_destroy(mw->Traceset_Info->before_trace);
    lttv_hooks_destroy(mw->Traceset_Info->after_trace);
    lttv_hooks_destroy(mw->Traceset_Info->before_tracefile);
    lttv_hooks_destroy(mw->Traceset_Info->after_tracefile);
    lttv_hooks_destroy(mw->Traceset_Info->before_event);
    lttv_hooks_destroy(mw->Traceset_Info->after_event);
g_critical("end remove");
    
    if(mw->Traceset_Info->path != NULL)
      g_free(mw->Traceset_Info->path);
    if(mw->Traceset_Info->TracesetContext != NULL){
      lttv_context_fini(LTTV_TRACESET_CONTEXT(mw->Traceset_Info->TracesetContext));
      g_object_unref(mw->Traceset_Info->TracesetContext);
    }
    if(mw->Traceset_Info->traceset != NULL) {
      nb = lttv_traceset_number(mw->Traceset_Info->traceset);
      for(i = 0 ; i < nb ; i++) {
	trace = lttv_traceset_get(mw->Traceset_Info->traceset, i);
	ref_count = lttv_trace_get_ref_number(trace);
	if(ref_count <= 1)
	  ltt_trace_close(lttv_trace(trace));
      }
    }

    lttv_traceset_destroy(mw->Traceset_Info->traceset); 

    g_object_unref(mw->Attributes);

    g_free(mw->Traceset_Info);
    mw->Traceset_Info = NULL;
      
    Main_Window_List = g_slist_remove(Main_Window_List, mw);

    g_hash_table_destroy(mw->hash_menu_item);
    g_hash_table_destroy(mw->hash_toolbar_item);
    
    g_free(mw);
    mw = NULL;
  }
}

void
mainWindow_Destructor(mainWindow * mw)
{
  if(GTK_IS_WIDGET(mw->MWindow)){
    gtk_widget_destroy(mw->MWindow);
    //    gtk_widget_destroy(mw->HelpContents);
    //    gtk_widget_destroy(mw->AboutBox);    
    mw = NULL;
  }
  //mainWindow_free called when the object mw in the widget is unref.
  //mainWindow_free(mw);
}


void main_window_destroy_walk(gpointer data, gpointer user_data)
{
  mainWindow_Destructor((mainWindow*)data);
}



/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {

  LttvAttributeValue value;  
  LttvTrace *trace;

  lttv_option_remove("trace");

  lttv_hooks_remove_data(main_hooks, Window_Creation_Hook, &gWinCreationData);

  g_critical("GUI destroy()");

  if(Main_Window_List){
    g_slist_foreach(Main_Window_List, main_window_destroy_walk, NULL );
    g_slist_free(Main_Window_List);
  }
  
}




