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

/*
 *  TODO
 *  - specify wich hook function will be used to call the core filter
 */

static char 
  *a_file_name = NULL,
  *a_string = NULL;

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
 
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
  GString* a_file_content = g_string_new("");
  a_file = fopen(a_file_name, "r"); 
	if(a_file == NULL) { 
		g_warning("file %s does not exist", a_file_name);
    return;
  }
  
  char* line = NULL;
  size_t len = 0;

  while(!feof(a_file)) {
    len = getline(&line,&len,a_file);
    g_string_append(a_file_content,line);
  }
  free(line);
  
  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));

  if(((GString*)*(value.v_pointer))->len != 0) g_string_append_c((GString*)*(value.v_pointer),'&');
  g_string_append((GString*)*(value.v_pointer),a_file_content->str);
  
  fclose(a_file);
}

/**
 * filters the string input from user
 * @param hook_data the hook data
 * @return success/failure of operation
 */
void filter_analyze_string(void *hook_data) {

  g_print("textFilter::filter_analyze_string\n");

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  
  /*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
/*  if(a_filter_string==NULL) {
    a_filter_string = g_string_new("");
    g_string_append(a_filter_string,a_string);
  }
  else {
    g_string_append(a_filter_string,"&"); 
    g_string_append(a_filter_string,a_string);
  }
*/
  
  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));

  if(((GString*)*(value.v_pointer))->len != 0) g_string_append_c((GString*)*(value.v_pointer),'&');
  g_string_append((GString*)*(value.v_pointer),a_string);

//  LttvFilter* filter = lttv_filter_new();
//  lttv_filter_append_expression(filter,a_string);
}

/**
 * 	initialize the new module
 */
static void init() {
  
  g_print("textFilter::init()\n");	/* debug */

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

}

/**
 * 	Destroy the current module
 */
static void destroy() {
  g_info("Destroy textFilter");

  lttv_option_remove("expression");

  lttv_option_remove("filename");

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes, "filter/expression",
      LTTV_POINTER, &value));
 
  g_string_free((GString*)*(value.v_pointer),TRUE);
  
}


LTTV_MODULE("textFilter", "Filters traces", \
	    "Filter the trace following commands issued by user input", \
	    init, destroy, "option")

