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

/* The text filter facility will prompt for user filter option 
 * and transmit them to the lttv core */

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

static char
  *a_file_name = NULL,
  *a_filter_string = NULL;

static LttvHooks
  *before_traceset,
  *event_hook;

static FILE *a_file;

/**
 * analyses user filter options and issues the filter string
 * to the core
 * @param hook_data the hook data
 * @param call_data the call data
 */
static void parse_filter_options(void *hook_data, void *call_data) {

	char* parsed_string=NULL;	/* the string compiled from user's input */ 
	
	/* debug */
	g_print("textFilter::parse_filter_options\n");	
	
	/* recovering hooks data */
	LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

	LttvTracefileState *tfs = (LttvTracefileState *)call_data;

//	LttvTrace* lt = tfc->t_context->vt;
	
	/* hook header */
	g_info("TextFilter options parser");
	
	/*
	 * 	User may specify filtering options through static file
	 * 	and/or command line string.  From these sources, an 
	 * 	option string is rebuilded and sent to the filter core
	 */
	if(a_file_name != NULL) { 	/* -f switch in use */
		a_file = fopen(a_file_name, "r"); 
		if(a_file == NULL) 
			g_error("textFilter::parse_filter_content() cannot open file %s", a_file_name);

		fscanf(a_file,"%s",parsed_string);
		
		fclose(a_file);
	}
	if(a_filter_string != NULL) 	/* -s switch in use */
		parsed_string = a_filter_string;	

	if(parsed_string==NULL) 
		g_warning("textFilter::parser_filter_options() no filtering options specified !"); 

	/* 	send the filtering string to the core	*/
	lttv_filter_new(parsed_string,tfs);	
	
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
  
  g_print("textFilter::init()");	/* debug */
	
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init textFilter.c");
  
  a_filter_string = NULL;
  lttv_option_add("string", 's', 
      "filters a string issued by the user on the command line", 
      "string", 
      LTTV_OPT_STRING, &a_filter_string, NULL, NULL);

  a_file_name = NULL;
  lttv_option_add("filename", 'f', 
      "browse the filter options contained in specified file", 
      "file name", 
      LTTV_OPT_STRING, &a_file_name, NULL, NULL);
  
  /*
   * 	Note to myself !
   * 	LttvAttributeValue::v_pointer is a gpointer* --> void**
   *	see union LttvAttributeValue for more info 
   */
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value));
  g_assert((event_hook = *(value.v_pointer)) != NULL);
  lttv_hooks_add(event_hook, filter_event_content, NULL, LTTV_PRIO_DEFAULT);

  g_assert(lttv_iattribute_find_by_path(attributes,"hooks/trace/before",
	LTTV_POINTER, &value));
  g_assert((before_traceset = *(value.v_pointer)) != NULL);
  lttv_hooks_add(before_traceset, parse_filter_options, NULL, LTTV_PRIO_DEFAULT);

  
}

/**
 * 	Destroy the current module
 */
static void destroy() {
  g_info("Destroy textFilter");

  lttv_option_remove("string");

  lttv_option_remove("filename");

  lttv_hooks_remove_data(event_hook, filter_event_content, NULL);

  lttv_hooks_remove_data(before_traceset, parse_filter_options, NULL);
  
}


LTTV_MODULE("textFilter", "Filters traces", \
	    "Filter the trace following commands issued by user input", \
	    init, destroy, "batchAnalysis", "option")

