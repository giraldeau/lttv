/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2005 Michel Dagenais
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
   consist in AND, OR and NOT nested expressions, forming a tree with 
   simple relations as leaves. The simple relations test is a field
   in an event is equal, not equal, smaller, smaller or equal, larger, or
   larger or equal to a specified value. 
*/

/*
 *  YET TO BE ANSWERED
 *  - nothing for now
 */

#include <lttv/filter.h>

/*
  read_token

  read_expression
    ( read expr )
    simple expr [ op expr ]

  read_simple_expression
    read_field_path [ rel value ]

  read_field_path
    read_field_component [. field path]

  read_field_component
    name [ \[ value \] ]

  data struct:
  and/or(left/right)
  not(child)
  op(left/right)
  path(component...) -> field
*/

GQuark
  LTTV_FILTER_TRACE,
  LTTV_FILTER_TRACESET,
  LTTV_FILTER_TRACEFILE,
  LTTV_FILTER_STATE,
  LTTV_FILTER_EVENT,
  LTTV_FILTER_NAME,
  LTTV_FILTER_CATEGORY,
  LTTV_FILTER_TIME,
  LTTV_FILTER_TSC,
  LTTV_FILTER_PID,
  LTTV_FILTER_PPID,
  LTTV_FILTER_C_TIME,
  LTTV_FILTER_I_TIME,
  LTTV_FILTER_P_NAME,
  LTTV_FILTER_EX_MODE,
  LTTV_FILTER_EX_SUBMODE,
  LTTV_FILTER_P_STATUS,
  LTTV_FILTER_CPU;
  

/**
 *  Parse through filtering field hierarchy as specified 
 *  by user.  This function compares each value to 
 *  predetermined quarks
 *  @param fp The field path list
 *  @return success/failure of operation
 */
gboolean
parse_field_path(GList* fp) {

  GString* f = g_list_first(fp)->data; 
  
  if(g_quark_try_string(f->str) == LTTV_FILTER_EVENT) {
//    parse_subfield(fp, LTTV_FILTER_EVENT);   

  } else if(g_quark_try_string(f->str) == LTTV_FILTER_TRACEFILE) {
    
  } else if(g_quark_try_string(f->str) == LTTV_FILTER_TRACE) {

  } else if(g_quark_try_string(f->str) == LTTV_FILTER_STATE) {

  } else {
    g_warning("Unrecognized field in filter string");
    return FALSE;
  }
  return TRUE;  
}

/**
 * 	Add an filtering option to the current tree
 * 	@param expression Current expression to parse
 * 	@return success/failure of operation
 */
gboolean
parse_simple_expression(GString* expression) {
	
	unsigned i;

  
  

}

/**
 * 	Creates a new lttv_filter
 * 	@param expression filtering options string
 * 	@param t pointer to the current LttvTrace
 * 	@return the current lttv_filter or NULL if error
 */
lttv_filter_t*
lttv_filter_new(char *expression, LttvTraceState *tcs) {

  g_print("filter::lttv_filter_new()\n");		/* debug */

  unsigned 	
    i, 
    p_nesting=0,	/* parenthesis nesting value */
    b=0;	/* current breakpoint in expression string */
	
  /* temporary values */
  GString *a_field_component = g_string_new(""); 
  GList *a_field_path = NULL;
  lttv_simple_expression a_simple_expression;
  
  /*	
   *  1. parse expression
   *  2. construct binary tree
   *  3. return corresponding filter
   */

  /*
   * 	Binary tree memory allocation
   * 	- based upon a preliminary block size
   */
//  gulong size = (strlen(expression)/AVERAGE_EXPRESSION_LENGTH)*MAX_FACTOR;
//  tree = g_malloc(size*sizeof(lttv_filter_tree));
    
  /*
   *	Parse entire expression and construct
   *	the binary tree.  There are two steps 
   *	in browsing that string
   *	  1. finding boolean ops ( &,|,^,! ) and parenthesis
   *	  2. finding simple expressions
   *	    - field path ( separated by dots )
   *	    - op ( >, <, =, >=, <=, !=)
   *	    - value ( integer, string ... )
   *	To spare computing time, the whole 
   *	string is parsed in this loop for a 
   *	O(n) complexity order.
   */
  for(i=0;i<strlen(expression);i++) {
    g_print("%s\n",a_field_component->str);
    switch(expression[i]) {
      /*
       *   logical operators
       */
      case '&':   /* and */
      case '|':   /* or */
      case '^':   /* xor */
        g_list_append( a_field_path, a_field_component );
        a_field_component = g_string_new("");
        break;
      case '!':   /* not, or not equal (math op) */
        if(expression[i+1] == '=') {  /* != */
          a_simple_expression.op = LTTV_FIELD_NE;
          i++;
        } else {  /* ! */
          g_print("%s\n",a_field_component);
          a_field_component = g_string_new("");
        }
        break;
      case '(':   /* start of parenthesis */
      case '[':
      case '{':
        p_nesting++;      /* incrementing parenthesis nesting value */
        break;
      case ')':   /* end of parenthesis */
      case ']':
      case '}':
        p_nesting--;      /* decrementing parenthesis nesting value */
        break;

      /*	
       *	mathematic operators
       */
      case '<':   /* lower, lower or equal */
        if(expression[i+1] == '=') { /* <= */
          i++;
          a_simple_expression.op = LTTV_FIELD_LE;                  
        } else a_simple_expression.op = LTTV_FIELD_LT;
        break;
      case '>':   /* higher, higher or equal */
        if(expression[i+1] == '=') {  /* >= */
          i++;
          a_simple_expression.op = LTTV_FIELD_GE;                  
        } else a_simple_expression.op = LTTV_FIELD_GT;
        break;
      case '=':   /* equal */
        a_simple_expression.op = LTTV_FIELD_EQ;
        break;
      /*
       *  Field concatening caracter
       */
      case '.':   /* dot */
        g_list_append( a_field_path, a_field_component );
        a_field_component = g_string_new("");
        break;
      default:    /* concatening current string */
        g_string_append_c(a_field_component,expression[i]); 				
    }
  }


  
  if( p_nesting>0 ) { 
    g_warning("Wrong filtering options, the string\n\"%s\"\n\
        is not valid due to parenthesis incorrect use",expression);	
    return NULL;
  }
}

/**
 * 	Apply the filter to a specific trace
 * 	@param filter the current filter applied
 * 	@param tracefile the trace to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_tracefile(lttv_filter_t *filter, LttTracefile *tracefile) {

  
  
  /* test */
/*  int i, nb;
  char *f_name, *e_name;

  char* field = "cpu";
   
  LttvTraceHook h;

  LttEventType *et;

  LttType *t;

  GString *fe_name = g_string_new("");

  nb = ltt_trace_eventtype_number(tcs->parent.t);
  g_print("NB:%i\n",nb);
  for(i = 0 ; i < nb ; i++) {
    et = ltt_trace_eventtype_get(tcs->parent.t, i);
    e_name = ltt_eventtype_name(et);
    f_name = ltt_facility_name(ltt_eventtype_facility(et));
    g_string_printf(fe_name, "%s.%s", f_name, e_name);
    g_print("facility:%s and event:%s\n",f_name,e_name);
  }
 */
}

gboolean
lttv_filter_tracestate(lttv_filter_t *filter, LttvTraceState *tracestate) {

}

/**
 * 	Apply the filter to a specific event
 * 	@param filter the current filter applied
 * 	@param event the event to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_event(lttv_filter_t *filter, LttEvent *event) {

}

/**
 *  Initializes the filter module and specific values
 */
static void module_init()
{

  /* 
   *  Quarks initialization
   *  for hardcoded filtering options
   *
   *  TODO: traceset has no yet been defined
   */
  
  /* top fields */
  LTTV_FILTER_EVENT = g_quark_from_string("event"); 
  LTTV_FILTER_TRACE = g_quark_from_string("trace"); 
  LTTV_FILTER_TRACESET = g_quark_from_string("traceset"); 
  LTTV_FILTER_STATE = g_quark_from_string("state"); 
  LTTV_FILTER_TRACEFILE = g_quark_from_string("tracefile"); 
 
  /* event.name, tracefile.name, trace.name */
  LTTV_FILTER_NAME = g_quark_from_string("name");

  /* event sub fields */
  LTTV_FILTER_CATEGORY = g_quark_from_string("category"); 
  LTTV_FILTER_TIME = g_quark_from_string("time"); 
  LTTV_FILTER_TSC = g_quark_from_string("tsc"); 

  /* state sub fields */
  LTTV_FILTER_PID = g_quark_from_string("pid"); 
  LTTV_FILTER_PPID = g_quark_from_string("ppid"); 
  LTTV_FILTER_C_TIME = g_quark_from_string("creation_time");  
  LTTV_FILTER_I_TIME = g_quark_from_string("insertion_time"); 
  LTTV_FILTER_P_NAME = g_quark_from_string("process_name"); 
  LTTV_FILTER_EX_MODE = g_quark_from_string("execution_mode");  
  LTTV_FILTER_EX_SUBMODE = g_quark_from_string("execution_submode");
  LTTV_FILTER_P_STATUS = g_quark_from_string("process_status");
  LTTV_FILTER_CPU = g_quark_from_string("cpu");
  
}

/**
 *  Destroys the filter module and specific values
 */
static void module_destroy() 
{
}


LTTV_MODULE("filter", "Filters traceset and events", \
    "Filters traceset and events specifically to user input", \
    module_init, module_destroy)



