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


   consist in AND, OR and NOT nested expressions, forming a tree with 
   simple relations as leaves. The simple relations test is a field
   in an event is equal, not equal, smaller, smaller or equal, larger, or
   larger or equal to a specified value. */

typedef enum _lttv_expression_op
{ LTTV_FIELD_EQ,
  LTTV_FIELD_NE,
  LTTV_FIELD_LT,
  LTTV_FIELD_LE,
  LTTV_FIELD_GT,
  LTTV_FIELD_GE
} lttv_expression_op;

typedef _lttv_simple_expression
{ lttv_expression_op op;
  char *field_name;
  char *value;
} lttv_simple_expression;

typedef _lttv_expression_type
{ LTTV_EXPRESSION,
  LTTV_SIMPLE_EXPRESSION

}
typedef struct _lttv_expression 
{ bool or;
  bool not;
  bool simple_expression;
  union 
  { lttv_expression *e;
    lttv_field_relation *se;
  } e;
} lttv_expression;

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


