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

#ifndef FILTER_H
#define FILTER_H

#include <lttv/traceset.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/module.h>
#include <ltt/ltt.h>
#include <ltt/event.h>

#define AVERAGE_EXPRESSION_LENGTH 6
#define MAX_FACTOR 1.5

/* A filter expression consists in nested AND, OR and NOT expressions
   involving boolean relation (>, >=, =, !=, <, <=) between event fields and 
   specific values. It is compiled into an efficient data structure which
   is used in functions to check if a given event or tracefile satisfies the
   filter.

   The grammar for filters is:

   filter = expression

   expression = "(" expression ")" | "!" expression | 
                expression "&&" expression | expression "||" expression |
                simpleExpression

   simpleExpression = fieldPath op value

   fieldPath = fieldComponent [ "." fieldPath ]

   fieldComponent = name [ "[" integer "]" ]

   value = integer | double | string 

*/

extern GQuark
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
 * 	@enum lttv_expression_op
 */
typedef enum _lttv_expression_op
{ 
  LTTV_FIELD_EQ,	/** equal */
  LTTV_FIELD_NE,	/** not equal */
  LTTV_FIELD_LT,	/** lower than */
  LTTV_FIELD_LE,	/** lower or equal */
  LTTV_FIELD_GT,	/** greater than */
  LTTV_FIELD_GE		/** greater or equal */
} lttv_expression_op;

typedef enum _lttv_expression_type
{ 
  LTTV_EXPRESSION,
  LTTV_SIMPLE_EXPRESSION,
  LTTV_EXPRESSION_OP,
  LTTV_UNDEFINED_EXPRESSION
} lttv_expression_type;

typedef enum _lttv_tree_element {
  LTTV_TREE_IDLE,
  LTTV_TREE_NODE,
  LTTV_TREE_LEAF
} lttv_tree_element;

typedef struct _lttv_simple_expression
{ 
  char *field_name;
  lttv_expression_op op;
  char *value;
} lttv_simple_expression;

typedef enum _lttv_logical_op {
    LTTV_LOGICAL_OR = 1,         /* 1 */
    LTTV_LOGICAL_AND = 1<<1,     /* 2 */
    LTTV_LOGICAL_NOT = 1<<2,     /* 4 */
    LTTV_LOGICAL_XOR = 1<<3      /* 8 */
} lttv_logical_op;
    
/*
 * Ah .. that's my tree
 */
//typedef struct _lttv_expression 
//{ 
//  gboolean simple_expression;
//  int op;
//  lttv_expression_type type;
//  union {
//    struct lttv_expression *e;
 //   lttv_field_relation *se;  /* --> simple expression */
//  } e;
//} lttv_expression;

typedef struct _lttv_expression {
  lttv_expression_type type;
  union {
    lttv_simple_expression *se;
    int op;
  } e;
} lttv_expression;

typedef struct _lttv_filter_tree {
//	lttv_expression* node;
  int node;
  lttv_tree_element left;
  lttv_tree_element right;
  union {
    struct lttv_filter_tree* t;
    lttv_simple_expression* leaf;
  } l_child;
  union {
    struct lttv_filter_tree* t;
    lttv_simple_expression* leaf;
  } r_child;
} lttv_filter_tree;

/**
 * @struct lttv_filter
 * ( will later contain a binary tree of filtering options )
 */
typedef struct _lttv_filter_t {
	lttv_filter_tree* tree;	
} lttv_filter_t;


lttv_simple_expression* lttv_simple_expression_new();

lttv_filter_tree* lttv_filter_tree_new();

void lttv_filter_tree_destroy(lttv_filter_tree* tree);

lttv_filter* lttv_filter_clone(lttv_filter* tree);

void lttv_filter_tree_add_node(GPtrArray* stack, lttv_filter_tree* subtree, lttv_logical_op op);

/* Parse field path contained in list */
gboolean parse_field_path(GPtrArray* fp);

gboolean parse_simple_expression(GString* expression);

/* Compile the filter expression into an efficient data structure */
lttv_filter_tree *lttv_filter_new(char *expression, LttvTraceState *tfs);

void lttv_filter_destroy(lttv_filter* filter);

/* Check if the tracefile or event satisfies the filter. The arguments are
   declared as void * to allow these functions to be used as hooks. */

gboolean lttv_filter_tracefile(lttv_filter_tree *filter, LttTracefile *tracefile);

gboolean lttv_filter_tracestate(lttv_filter_t *filter, LttvTraceState *tracestate);

gboolean lttv_filter_event(lttv_filter_t *filter, LttEvent *event);

#endif // FILTER_H

