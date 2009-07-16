/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Simon Bouvier-Zappa 
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

/*! \file lttv/modules/text/textFilter.c
 *  \brief Textual prompt for user filtering expression.
 *  
 *  The text filter facility will prompt for user filter option 
 *  and transmit them to the lttv core.  User can either specify 
 *  a filtering string with the command line or/and specify a 
 *  file containing filtering expressions.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/trace.h>

/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

/*
 *  TODO
 *  - specify wich hook function will be used to call the core filter
 */

static char 
  *a_file_name = NULL,
  *a_string = NULL;

static LttvHooks __attribute__ ((__unused__))
  *before_traceset,
  *event_hook;

/**
 * filters the file input from user
 * @param hook_data the hook data, unused
 */
void filter_analyze_file(void *hook_data) {

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
  if(!g_file_test(a_file_name,G_FILE_TEST_EXISTS)) {
    g_warning("file %s does not exist", a_file_name);
    return;
  }

  char* a_file_content = NULL;

  g_file_get_contents(a_file_name,&a_file_content,NULL,NULL);
  
  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));

  if(((GString*)*(value.v_pointer))->len != 0)
      g_string_append_c((GString*)*(value.v_pointer),'&');
     g_string_append_c((GString*)*(value.v_pointer),'(');
    g_string_append((GString*)*(value.v_pointer),a_file_content);
    g_string_append_c((GString*)*(value.v_pointer),')');
  
}

/**
 * filters the string input from user
 * @param hook_data the hook data, unused
 */
void filter_analyze_string(void *hook_data) {

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));

  if(((GString*)*(value.v_pointer))->len != 0)
      g_string_append_c((GString*)*(value.v_pointer),'&');
    g_string_append_c((GString*)*(value.v_pointer),'(');
    g_string_append((GString*)*(value.v_pointer),a_string);
    g_string_append_c((GString*)*(value.v_pointer),')');

}

/**
 * Output all filter commands on console 
 * @param hook_data the hook data
 */
void filter_list_commands(void *hook_data) {

  g_print("[field] [op] [value]\n\n");

  g_print("*** Possible fields ***\n");
  g_print("event.name (string) (channel_name.event_name)\n");
  g_print("event.subname (string)\n");
  g_print("event.category (string)\n");
  g_print("event.time (double)\n");
  g_print("event.tsc (integer)\n");
  g_print("event.target_pid (integer)\n");
  g_print("event.field.channel_name.event_name.field_name.subfield_name (field_type)\n");
  g_print("channel.name (string)\n");
  g_print("trace.name (string)\n");
  g_print("state.pid (integer)\n");
  g_print("state.ppid (integer)\n");
  g_print("state.creation_time (double)\n");
  g_print("state.insertion_time (double)\n");
  g_print("state.process_name (string)\n");
  g_print("state.thread_brand (string)\n");
  g_print("state.execution_mode (string)\n");
  g_print("state.execution_submode (string)\n");
  g_print("state.process_status (string)\n");
  g_print("state.cpu (string)\n\n");
  
  g_print("*** Possible operators ***\n");
  g_print("equal '='\n");
  g_print("not equal '!='\n");
  g_print("greater '>'\n");
  g_print("greater or equal '>='\n");
  g_print("lower '<'\n");
  g_print("lower or equal '<='\n");
 
  g_print("*** Possible values ***\n");
  g_print("string, integer, double");
}

/**
 * 	initialize the new module
 */
static void init() {
  
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));
  
  *(value.v_pointer) = g_string_new("");

  g_info("Init textFilter.c");
 
  a_string = NULL;
  lttv_option_add("expression", 'e', 
      "filters a string issued by the user on the command line", 
      "string", 
      LTTV_OPT_STRING, &a_string, filter_analyze_string, NULL);
  // add function to call for option
  
  a_file_name = NULL;
  lttv_option_add("filename", 'f', 
      "browse the filter options contained in specified file", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, filter_analyze_file, NULL);

  lttv_option_add("list", 'l',
      "list all possible filter commands for module",
      "list commands",
      LTTV_OPT_NONE, NULL, filter_list_commands, NULL);
  
}

/**
 * 	Destroy the current module
 */
static void destroy() {
  g_info("Destroy textFilter");

  lttv_option_remove("expression");

  lttv_option_remove("filename");

  lttv_option_remove("list");

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));
 
  g_string_free((GString*)*(value.v_pointer),TRUE);
  
}


LTTV_MODULE("textFilter", "Filters traces", \
	    "Filter the trace following commands issued by user input", \
	    init, destroy, "option")

