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

/**
 * @enum LttvFieldType
 * @brief Structures and their fields
 *
 * the LttvFieldType enum consists on 
 * all the hardcoded structures and 
 * their appropriate fields on which 
 * filters can be applied.
 */
enum _LttvFieldType {
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
  LTTV_FILTER_CPU
} LttvFieldType;
  
/**
 * 	@enum LttvExpressionOp
 */
typedef enum _LttvExpressionOp
{ 
  LTTV_FIELD_EQ,	  /** equal */
  LTTV_FIELD_NE,	  /** not equal */
  LTTV_FIELD_LT,	  /** lower than */
  LTTV_FIELD_LE,	  /** lower or equal */
  LTTV_FIELD_GT,	  /** greater than */
  LTTV_FIELD_GE		  /** greater or equal */
} LttvExpressionOp;

/**
 * @enum LttvTreeElement
 * @brief element types for the tree nodes
 *
 * LttvTreeElement defines the possible 
 * types of nodes which build the LttvFilterTree.  
 */
typedef enum _LttvTreeElement {
  LTTV_TREE_IDLE,   /** this node does nothing */
  LTTV_TREE_NODE,   /** this node contains a logical operator */
  LTTV_TREE_LEAF    /** this node is a leaf and contains a simple expression */
} LttvTreeElement;

/**
 * @enum LttvSimpleExpression
 * @brief simple expression structure
 *
 * An LttvSimpleExpression is the base 
 * of all filtering operations.  It also 
 * populates the leaves of the
 * LttvFilterTree.  Each expression 
 * consists basically in a structure 
 * field, an operator and a specific 
 * value.
 */
typedef struct _LttvSimpleExpression
{ 
  char *field_name;
//  LttvExpressionOp op;
  gboolean (*op)();
  char *value;
} LttvSimpleExpression;

/**
 * @enum LttvLogicalOp
 * @brief logical operators
 * 
 * Contains the possible values taken 
 * by logical operator used to link 
 * simple expression.  Values are 
 * AND, OR, XOR or NOT
 */
typedef enum _LttvLogicalOp {
    LTTV_LOGICAL_OR = 1,         /* 1 */
    LTTV_LOGICAL_AND = 1<<1,     /* 2 */
    LTTV_LOGICAL_NOT = 1<<2,     /* 4 */
    LTTV_LOGICAL_XOR = 1<<3      /* 8 */
} LttvLogicalOp;
    
/**
 *  @struct LttvFilterTree
 *  The filtering tree is used to represent the 
 *  expression string in its entire hierarchy 
 *  composed of simple expressions and logical 
 *  operators
 */
typedef struct _LttvFilterTree {
  int node;                         /** value of LttvLogicalOp */
  LttvTreeElement left;
  LttvTreeElement right;
  union {
    struct LttvFilter* t;
    LttvSimpleExpression* leaf;
  } l_child;
  union {
    struct LttvFilter* t;
    LttvSimpleExpression* leaf;
  } r_child;
} LttvFilterTree;

/**
 * @struct lttv_filter
 * Contains a binary tree of filtering options along 
 * with the expression itself.
 */
typedef struct _LttvFilter {
  char *expression;
  LttvFilterTree *head;
}

/*
 * General Data Handling functions
 */

LttvSimpleExpression* lttv_simple_expression_new();

void lttv_filter_tree_add_node(GPtrArray* stack, LttvFilter* subtree, LttvLogicalOp op);

gboolean parse_field_path(GPtrArray* fp);

gboolean parse_simple_expression(GString* expression);

/*
 * Logical operators functions
 */

gboolean lttv_apply_op_eq();

gboolean lttv_apply_op_ne();

gboolean lttv_apply_op_lt();

gboolean lttv_apply_op_le();

gboolean lttv_apply_op_gt();

gboolean lttv_apply_op_ge();

/*
 * Cloning
 */

LttvFilterTree* lttv_filter_tree_clone(LttvFilterTree* tree);

LttvFilter* lttv_filter_clone(LttvFilter* filter);

/*
 * Constructors/Destructors
 */

/* LttvFilter */
LttvFilter *lttv_filter_new(char *expression, LttvTraceState *tfs);

void lttv_filter_destroy(LttvFilter* filter);

/* LttvFilterTree */
LttvFilterTree* lttv_filter_tree_new();

void lttv_filter_tree_destroy(LttvFilterTree* tree);


/*
 *  Hook functions
 *  
 *  These hook functions will be the one called when filtering 
 *  an event, a trace, a state, etc.
 */

/* Check if the tracefile or event satisfies the filter. The arguments are
   declared as void * to allow these functions to be used as hooks. */

gboolean lttv_filter_tracefile(LttvFilter *filter, LttTracefile *tracefile);

gboolean lttv_filter_tracestate(LttvFilter *filter, LttvTraceState *tracestate);

gboolean lttv_filter_event(LttvFilter *filter, LttEvent *event);

#endif // FILTER_H

