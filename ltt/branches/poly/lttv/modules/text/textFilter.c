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

/* 
 * The text filter facility will prompt for user filter option 
 * and transmit them to the lttv core 
 */

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
#include <ltt/type.h>
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <stdio.h>

/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

/*
 *  YET TO BE ANSWERED !
 *  - why does this module need dependency with batchAnalysis ?
 */

static char 
  *a_file_name = NULL,
  *a_string = NULL;

static GString
  *a_filter_string = NULL;


static LttvHooks
  *before_traceset,
  *event_hook;

static FILE *a_file;


/**
 * filters the file input from user
 * @param hook_data the hook data
 * @return success/failure of operation
 */
void filter_analyze_file(void *hook_data) {

  g_print("textFilter::filter_analyze_file\n");
 
  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
	a_file = fopen(a_file_name, "r"); 
	if(a_file == NULL) { 
		g_warning("file %s does not exist", a_file_name);
    return;
  }
  
  char* tmp;
  fscanf(a_file,"%s",tmp);
    
  if(!a_filter_string->len) {
    g_string_append(a_filter_string,tmp);
  }
  else {
    g_string_append(a_filter_string,"&"); /*conjonction between expression*/
    g_string_append(a_filter_string,tmp);
  }
  
  fclose(a_file);
}

/**
 * filters the string input from user
 * @param hook_data the hook data
 * @return success/failure of operation
 */
void filter_analyze_string(void *hook_data) {

  g_print("textFilter::filter_analyze_string\n");

  a_filter_string = g_string_new("");
  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
//  if(!a_filter_string->len) {
    g_string_append(a_filter_string,a_string);
    lttv_filter_new(a_filter_string->str,NULL);
//  }
//  else {
//    g_string_append(a_filter_string,"&"); /*conjonction between expression*/
//    g_string_append(a_filter_string,a_string);
//  }

}

/**
 * filter to current event depending on the 
 * filter options tree
 * @param hook_data the hook data
 * @param call_data the call data
 * @return success/error of operation
 */
static gboolean filter_event_content(void *hook_data, void *call_data) {

	g_print("textFilter::filter_event_content\n");	/* debug */
}

/**
 * 	initialize the new module
 */
static void init() {
  
  g_print("textFilter::init()\n");	/* debug */

  a_filter_string = g_string_new("");
  
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init textFilter.c");
  
  a_string = NULL;
  lttv_option_add("string", 's', 
      "filters a string issued by the user on the command line", 
      "string", 
      LTTV_OPT_STRING, &a_string, filter_analyze_string, NULL);
  // add function to call for option
  
  a_file_name = NULL;
  lttv_option_add("filename", 'f', 
      "browse the filter options contained in specified file", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, filter_analyze_file, NULL);
  
  /*
   * 	Note to myself !
   * 	LttvAttributeValue::v_pointer is a gpointer* --> void**
   *	see union LttvAttributeValue for more info 
   */
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value));
  g_assert((event_hook = *(value.v_pointer)) != NULL);
  lttv_hooks_add(event_hook, filter_event_content, NULL, LTTV_PRIO_DEFAULT);

//  g_assert(lttv_iattribute_find_by_path(attributes,"hooks/trace/before",
//	LTTV_POINTER, &value));
//  g_assert((before_traceset = *(value.v_pointer)) != NULL);
//  lttv_hooks_add(before_traceset, parse_filter_options, NULL, LTTV_PRIO_DEFAULT);

  
}

/**
 * 	Destroy the current module
 */
static void destroy() {
  g_info("Destroy textFilter");

  lttv_option_remove("string");

  lttv_option_remove("filename");

  lttv_hooks_remove_data(event_hook, filter_event_content, NULL);

//  lttv_hooks_remove_data(before_traceset, parse_filter_options, NULL);
  
}


LTTV_MODULE("textFilter", "Filters traces", \
	    "Filter the trace following commands issued by user input", \
	    init, destroy, "batchAnalysis", "option")

