/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

#include <lttv/hook.h>
#include <lttv/module.h>
#include <lttv/lttv.h>
#include <lttv/iattribute.h>
#include <lttv/attribute.h>
#include <lttv/option.h>
#include <lttv/traceset.h>
#include <ltt/trace.h>
#include <stdio.h>
#include <string.h>


/* The main program maintains a few central data structures and relies
   on modules for the rest. These data structures may be accessed by modules
   through an exported API */

static LttvIAttribute *attributes;

static LttvHooks
  *before_options,
  *after_options,
  *before_main,
  *after_main;

static char 
  *a_module,
  *a_module_path;

static gboolean
  a_verbose,
  a_debug,
  a_fatal;

gboolean lttv_profile_memory;

int lttv_argc;

char **lttv_argv;

static void lttv_module_option(void *hook_data);

static void lttv_module_path_option(void *hook_data);

static void lttv_verbose(void *hook_data);

static void lttv_debug(void *hook_data);

static void lttv_event_debug(void *hook_data);

static void lttv_fatal(void *hook_data);

static void lttv_help(void *hook_data);

/* This is the handler to specify when we dont need all the debugging 
   messages. It receives the message and does nothing. */

void ignore_and_drop_message(const gchar *log_domain, GLogLevelFlags log_level,
    const gchar *message, gpointer user_data)
{
}


/* Since everything is done in modules, the main program only takes care
   of the infrastructure. */

int main(int argc, char **argv)
{

  int i;

  char 
    *profile_memory_short_option = "-M",
    *profile_memory_long_option = "--memory";

  gboolean profile_memory = FALSE;

  LttvAttributeValue value;

  lttv_argc = argc;
  lttv_argv = argv;

  /* Before anything else, check if memory profiling is requested */

  for(i = 1 ; i < argc ; i++) {
    if(*(argv[i]) != '-') break;
    if(strcmp(argv[i], profile_memory_short_option) == 0 || 
       strcmp(argv[i], profile_memory_long_option) == 0) {
      g_mem_set_vtable(glib_mem_profiler_table);
      g_message("Memory summary before main");
      g_mem_profile();
      profile_memory = TRUE;
      break;
    }
  }


  /* Initialize glib and by default ignore info and debug messages */

  g_type_init();
  //g_type_init_with_debug_flags (G_TYPE_DEBUG_OBJECTS | G_TYPE_DEBUG_SIGNALS);
  g_log_set_handler(NULL, G_LOG_LEVEL_INFO, ignore_and_drop_message, NULL);
  g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, ignore_and_drop_message, NULL);


  /* Have an attributes subtree to store hooks to be registered by modules. */

  attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));

  before_options = lttv_hooks_new();
  after_options = lttv_hooks_new();
  before_main = lttv_hooks_new();
  after_main = lttv_hooks_new();


  /* Create a number of hooks lists */

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/options/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_options;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/options/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_options;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_main;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_main;


  /* Initialize the command line options processing */

  GError *error = NULL;

  LttvModule *module_module = lttv_module_require("module", &error);
  if(error != NULL) g_error("%s", error->message);
  LttvModule *module_option = lttv_module_require("option", &error);
  if(error != NULL) g_error("%s", error->message);

  /* Initialize the module loading */

  lttv_library_path_add(PACKAGE_PLUGIN_DIR);


  /* Add some built-in options */

  lttv_option_add("module",'m', "load a module", "name of module to load", 
      LTTV_OPT_STRING, &a_module, lttv_module_option, NULL);
 
  lttv_option_add("modules-path", 'L', 
      "add a directory to the module search path", 
      "directory to add to the path", LTTV_OPT_STRING, &a_module_path, 
      lttv_module_path_option, NULL);
	
  lttv_option_add("help",'h', "basic help", "none", 
      LTTV_OPT_NONE, NULL, lttv_help, NULL);

  a_verbose = FALSE; 
  lttv_option_add("verbose",'v', "print information messages", "none", 
      LTTV_OPT_NONE, NULL, lttv_verbose, NULL);
 
  a_debug = FALSE;
  lttv_option_add("debug",'d', "print debugging messages", "none", 
      LTTV_OPT_NONE, NULL, lttv_debug, NULL);

  /* use --edebug, -e conflicts with filter. Problem with option parsing when we
   * reparse the options with different number of arguments. */
  lttv_option_add("edebug",'e', "print event debugging", "none", 
      LTTV_OPT_NONE, NULL, lttv_event_debug, NULL);

  a_fatal = FALSE;
  lttv_option_add("fatal",'f', "make critical messages fatal",
                  "none", 
      LTTV_OPT_NONE, NULL, lttv_fatal, NULL);
 
  lttv_profile_memory = FALSE;
  lttv_option_add(profile_memory_long_option + 2, 
      profile_memory_short_option[1], "print memory information", "none", 
      LTTV_OPT_NONE, &lttv_profile_memory, NULL, NULL);


  /* Process the options */
 
  lttv_hooks_call(before_options, NULL);
  lttv_option_parse(argc, argv);
  lttv_hooks_call(after_options, NULL);


  /* Memory profiling to be useful must be activated as early as possible */

  if(profile_memory != lttv_profile_memory) 
    g_error("Memory profiling options must appear before other options");


  /* Do the main work */

  lttv_hooks_call(before_main, NULL);
  lttv_hooks_call(after_main, NULL);


  /* Clean up everything */

  lttv_module_release(module_option);
  lttv_module_release(module_module);

  lttv_hooks_destroy(before_options);
  lttv_hooks_destroy(after_options);
  lttv_hooks_destroy(before_main);
  lttv_hooks_destroy(after_main);
  g_object_unref(attributes);

  if(profile_memory) {
    g_message("Memory summary after main");
    g_mem_profile();
  }
  return 0;
}


LttvAttribute *lttv_global_attributes()
{
  return (LttvAttribute*)attributes;
}


void lttv_module_option(void *hook_data)
{
  GError *error = NULL;

  lttv_module_require(a_module, &error);
  if(error != NULL) g_error("%s", error->message);
}


void lttv_module_path_option(void *hook_data)
{
  lttv_library_path_add(a_module_path);
}


void lttv_verbose(void *hook_data)
{
  g_log_set_handler(NULL, G_LOG_LEVEL_INFO, g_log_default_handler, NULL);
  g_info("Logging set to include INFO level messages");
}

void lttv_debug(void *hook_data)
{
  g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, g_log_default_handler, NULL);
  g_info("Logging set to include DEBUG level messages");
}

void lttv_event_debug(void *hook_data)
{
  ltt_event_debug(1);
  g_info("Output event detailed debug");
}

void lttv_fatal(void *hook_data)
{
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  //g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
  g_info("Critical log from glib will abort execution");
}

void lttv_help(void *hook_data)
{
  printf("Linux Trace Toolkit Visualizer " VERSION "\n");
  printf("\n");
  lttv_option_show_help();
  printf("\n");
}

/* 

- Define formally traceset/trace in the GUI for the user and decide how
   trace/traceset sharing goes in the application.

- Use appropriately the new functions in time.h

- remove the separate tracefiles (control/per cpu) arrays/loops in context.

- split processTrace into context.c and processTrace.c

- check spelling conventions.

- get all the copyright notices.

- remove all the warnings.

- get all the .h files properly doxygen commented to produce useful documents.

- have an intro/architecture document.

- write a tutorial */
