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
   consist in AND, OR and NOT nested expressions, forming a tree with 
   simple relations as leaves. The simple relations test is a field
   in an event is equal, not equal, smaller, smaller or equal, larger, or
   larger or equal to a specified value. */

#include <lttv/filter.h>

typedef enum _lttv_expression_op
{ LTTV_FIELD_EQ,
  LTTV_FIELD_NE,
  LTTV_FIELD_LT,
  LTTV_FIELD_LE,
  LTTV_FIELD_GT,
  LTTV_FIELD_GE
} lttv_expression_op;

typedef enum _lttv_expression_type
{ 
  LTTV_EXPRESSION,
  LTTV_SIMPLE_EXPRESSION
} lttv_expression_type;

typedef struct _lttv_simple_expression
{ 
  lttv_expression_op op;
  char *field_name;
  char *value;
} lttv_simple_expression;

typedef struct _lttv_expression 
{ 
  gboolean or;
  gboolean not;
  gboolean simple_expression;
//  union e 
//  { 
//	lttv_expression *e;
//    	lttv_field_relation *se;
//  };
} lttv_expression;

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

/**
 * 	create a new lttv_filter
 * 	@param expression filtering options string
 * 	@param t pointer to the current LttvTrace
 * 	@return the current lttv_filter
 */
lttv_filter*
lttv_filter_new(char *expression, LttvTrace *t) {

	g_print("filter::lttv_filter_new()\n");		/* debug */

	/*
	 * 	1. parse expression
	 *	2. construct binary tree
	 * 	3. return corresponding filter
	 */

	
}

gboolean
lttv_filter_tracefile(lttv_filter *filter, void *tracefile) {

}

gboolean
lttv_filter_event(lttv_filter *filter, void *event) {

}
