/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2005 Michel Dagenais and Simon Bouvier-Zappa
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

/*! \file lttv/lttv/filter.h
 *  \brief Defines the core filter of application 
 *
 *  A filter expression consists in nested AND, OR and NOT expressions
 *  involving boolean relation (>, >=, =, !=, <, <=) between event fields and 
 *  specific values. It is compiled into an efficient data structure which
 *  is used in functions to check if a given event or tracefile satisfies the
 *  filter.
 * 
 *  The grammar for filters is:
 * 
 *  filter = expression
 * 
 *  expression = "(" expression ")" | "!" expression | 
 *              expression "&&" expression | expression "||" expression |
 *              simpleExpression
 *
 *  simpleExpression = fieldPath op value
 *
 *  fieldPath = fieldComponent [ "." fieldPath ]
 *
 *  fieldComponent = name [ "[" integer "]" ]
 *
 *  value = integer | double | string 
 */


#include <lttv/traceset.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/module.h>
#include <ltt/ltt.h>
#include <ltt/time.h>
#include <ltt/event.h>

/* structures prototypes */
typedef enum _LttvStructType LttvStructType; 
typedef enum _LttvFieldType LttvFieldType; 
typedef enum _LttvExpressionOp LttvExpressionOp;
typedef enum _LttvTreeElement LttvTreeElement;
typedef enum _LttvLogicalOp LttvLogicalOp;

typedef union _LttvFieldValue LttvFieldValue;

typedef struct _LttvSimpleExpression LttvSimpleExpression;
typedef struct _LttvFilterTree LttvFilterTree;

#ifndef LTTVFILTER_TYPE_DEFINED
typedef struct _LttvFilter LttvFilter;
#define LTTVFILTER_TYPE_DEFINED
#endif

/**
 * @enum _LttvStructType
 * @brief The lttv structures
 *
 * the LttvStructType enumerates 
 * the possible structures for the 
 * lttv core filter
 */
enum _LttvStructType {
  LTTV_FILTER_TRACE,                /**< trace (LttTrace) */
  LTTV_FILTER_TRACESET,             /**< traceset */
  LTTV_FILTER_TRACEFILE,            /**< tracefile (LttTracefile) */
  LTTV_FILTER_EVENT,                /**< event (LttEvent) */
  LTTV_FILTER_STATE                 /**< state (LttvProcessState) */
};

/**
 * @enum _LttvFieldType
 * @brief Possible fields for the structures
 *
 * the LttvFieldType enum consists on 
 * all the hardcoded structures and 
 * their appropriate fields on which 
 * filters can be applied.
 */
enum _LttvFieldType {
  LTTV_FILTER_TRACE_NAME,             /**< trace.name (char*) */
  LTTV_FILTER_TRACEFILE_NAME,         /**< tracefile.name (char*) */
  LTTV_FILTER_STATE_PID,              /**< state.pid (guint) */
  LTTV_FILTER_STATE_PPID,             /**< state.ppid (guint) */
  LTTV_FILTER_STATE_CT,               /**< state.creation_time (double) */
  LTTV_FILTER_STATE_IT,               /**< state.insertion_time (double) */
  LTTV_FILTER_STATE_P_NAME,           /**< state.process_name (char*) */
  LTTV_FILTER_STATE_T_BRAND,          /**< state.thread_brand (char*) */
  LTTV_FILTER_STATE_EX_MODE,          /**< state.execution_mode (LttvExecutionMode) */
  LTTV_FILTER_STATE_EX_SUBMODE,       /**< state.execution_submode (LttvExecutionSubmode) */
  LTTV_FILTER_STATE_P_STATUS,         /**< state.process_status (LttvProcessStatus) */
  LTTV_FILTER_STATE_CPU,              /**< state.cpu (?last_cpu?) */
  LTTV_FILTER_EVENT_NAME,             /**< event.name (char*) */
  LTTV_FILTER_EVENT_SUBNAME,          /**< event.subname (char*) */
  LTTV_FILTER_EVENT_CATEGORY,         /**< FIXME: not implemented */
  LTTV_FILTER_EVENT_TIME,             /**< event.time (double) */
  LTTV_FILTER_EVENT_TSC,              /**< event.tsc (double) */
  LTTV_FILTER_EVENT_TARGET_PID,       /**< event.target_pid (guint) */
  LTTV_FILTER_EVENT_FIELD,            /**< dynamic field, specified in facility */
  LTTV_FILTER_UNDEFINED               /**< undefined field */
};
  
/**
 *   @enum _LttvExpressionOp
 *  @brief Contains possible operators
 *
 *  This enumeration defines the 
 *  possible operator used to compare 
 *  right and left member in simple 
 *  expression
 */
enum _LttvExpressionOp
{ 
  LTTV_FIELD_EQ,                      /**< equal */
  LTTV_FIELD_NE,                      /**< not equal */
  LTTV_FIELD_LT,                      /**< lower than */
  LTTV_FIELD_LE,                      /**< lower or equal */
  LTTV_FIELD_GT,                      /**< greater than */
  LTTV_FIELD_GE                        /**< greater or equal */
};

/**
 *  @union _LttvFieldValue
 *  @brief Contains possible field values
 *
 *  This particular union defines the 
 *  possible set of values taken by the 
 *  right member of a simple expression.  
 *  It is used for comparison whithin the 
 *  'operators' functions
 */
union _LttvFieldValue {
  GQuark  v_quark;                    /**< GQuark */
  guint64 v_uint64;                   /**< unsigned int of 64 bytes */
  guint32 v_uint32;                   /**< unsigned int of 32 bytes */
  guint16 v_uint16;                   /**< unsigned int of 16 bytes */
  guint16 v_uint;                     /**< unsigned int */
  double v_double;                    /**< double */
  char* v_string;                     /**< string */
  LttTime v_ltttime;                  /**< LttTime */
  struct {
    GQuark q[2];
  } v_quarks;
};

/**
 * @enum _LttvTreeElement
 * @brief element types for the tree nodes
 *
 * LttvTreeElement defines the possible 
 * types of nodes which build the LttvFilterTree.  
 */
enum _LttvTreeElement {
  LTTV_TREE_IDLE,                     /**< this node does nothing */
  LTTV_TREE_NODE,                     /**< this node contains a logical operator */
  LTTV_TREE_LEAF                      /**< this node is a leaf and contains a simple expression */
};


/**
 * @struct _LttvSimpleExpression
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
struct _LttvSimpleExpression
{ 
  gint field;                               /**< left member of simple expression */                  
  gint offset;                              /**< offset used for dynamic fields */
  gboolean (*op)(gpointer,LttvFieldValue);  /**< operator of simple expression */
  LttvFieldValue value;                     /**< right member of simple expression */
};

/**
 * @enum _LttvLogicalOp
 * @brief logical operators
 * 
 * Contains the possible values taken 
 * by logical operator used to link 
 * simple expression.  Values are 
 * AND, OR, XOR or NOT
 */
enum _LttvLogicalOp {
    LTTV_LOGICAL_OR = 1,              /**< OR (1) */
    LTTV_LOGICAL_AND = 1<<1,          /**< AND (2) */
    LTTV_LOGICAL_NOT = 1<<2,          /**< NOT (4) */
    LTTV_LOGICAL_XOR = 1<<3           /**< XOR (8) */
};
    
/**
 *  @struct _LttvFilterTree
 *  @brief The filtering tree
 *  
 *  The filtering tree is used to represent the 
 *  expression string in its entire hierarchy 
 *  composed of simple expressions and logical 
 *  operators
 */
struct _LttvFilterTree {
  int node;                         /**< value of LttvLogicalOp */
  LttvTreeElement left;             /**< nature of left branch (node/leaf) */
  LttvTreeElement right;            /**< nature of right branch (node/leaf) */
  union {
    LttvFilterTree* t;              
    LttvSimpleExpression* leaf;     
  } l_child;                        /**< left branch of tree */
  union {
    LttvFilterTree* t;              
    LttvSimpleExpression* leaf;     
  } r_child;                        /**< right branch of tree */
};

/**
 * @struct _LttvFilter
 * @brief The filter
 * 
 * Contains a binary tree of filtering options along 
 * with the expression itself.
 */
struct _LttvFilter {
  char *expression;                 /**< filtering expression string */
  LttvFilterTree *head;             /**< tree associated to expression */
};

/*
 * Simple Expression
 */
LttvSimpleExpression* lttv_simple_expression_new();

gboolean lttv_simple_expression_assign_field(GPtrArray* fp, LttvSimpleExpression* se);

gboolean lttv_simple_expression_assign_operator(LttvSimpleExpression* se, LttvExpressionOp op);

gboolean lttv_simple_expression_assign_value(LttvSimpleExpression* se, char* value);

void lttv_simple_expression_destroy(LttvSimpleExpression* se);


/*
 * Logical operators functions
 */

gboolean lttv_apply_op_eq_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_string(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_quark(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_quarks(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_eq_ltttime(const gpointer v1, LttvFieldValue v2);

gboolean lttv_apply_op_ne_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_string(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_quark(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_quarks(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ne_ltttime(const gpointer v1, LttvFieldValue v2);

gboolean lttv_apply_op_lt_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_lt_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_lt_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_lt_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_lt_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_lt_ltttime(const gpointer v1, LttvFieldValue v2);

gboolean lttv_apply_op_le_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_le_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_le_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_le_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_le_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_le_ltttime(const gpointer v1, LttvFieldValue v2);

gboolean lttv_apply_op_gt_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_gt_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_gt_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_gt_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_gt_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_gt_ltttime(const gpointer v1, LttvFieldValue v2);

gboolean lttv_apply_op_ge_uint(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ge_uint64(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ge_uint32(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ge_uint16(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ge_double(const gpointer v1, LttvFieldValue v2);
gboolean lttv_apply_op_ge_ltttime(const gpointer v1, LttvFieldValue v2);

/*
 * Cloning
 */

LttvFilterTree* lttv_filter_tree_clone(const LttvFilterTree* tree);

LttvFilter* lttv_filter_clone(const LttvFilter* filter);

/* 
 * LttvFilter 
 */
LttvFilter *lttv_filter_new();

gboolean lttv_filter_update(LttvFilter* filter);

void lttv_filter_destroy(LttvFilter* filter);

gboolean lttv_filter_append_expression(LttvFilter* filter, const char *expression);

void lttv_filter_clear_expression(LttvFilter* filter);

/*
 * LttvFilterTree 
 */
LttvFilterTree* lttv_filter_tree_new();

void lttv_filter_tree_destroy(LttvFilterTree* tree);

gboolean lttv_filter_tree_parse(
        const LttvFilterTree* t,
        const LttEvent* event,
        const LttTracefile* tracefile,
        const LttTrace* trace,
        const LttvTracefileContext* context,
	const LttvProcessState* pstate,
	const LttvTraceContext* tc);

gboolean lttv_filter_tree_parse_branch(
        const LttvSimpleExpression* se,
        const LttEvent* event,
        const LttTracefile* tracefile,
        const LttTrace* trace,
        const LttvProcessState* state,
        const LttvTracefileContext* context);

/*
 *  Debug functions
 */
void lttv_print_tree(const LttvFilterTree* t, const int count);

#endif // FILTER_H

