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

/*! \file lttv/lttv/filter.c
 *  \brief Defines the core filter of application
 *
 *  consist in AND, OR and NOT nested expressions, forming a tree with 
 *  simple relations as leaves. The simple relations test if a field
 *  in an event is equal, not equal, smaller, smaller or equal, larger, or
 *  larger or equal to a specified value.
 *
 *  Fields specified in a simple expression can take following 
 *  values
 *
 *  \verbatim
 *  LttvTracefileContext{} 
 *  |->event\ 
 *  | |->name (String, converted to GQuark) (channel.event)
 *  | |->subname (String, converted to GQuark)
 *  | |->category (String, not yet implemented)
 *  | |->time (LttTime)
 *  | |->tsc (LttCycleCount --> uint64)
 *  | |->target_pid (target PID of the event)
 *  | |->fields
 *  |   |->"channel name"
 *  |     |->"event name"
 *  |       |->"field name"
 *  |         |->"sub-field name"
 *  |           |->...
 *  |             |->"leaf-field name" (field type)
 *  |->channel (or tracefile)
 *  | |->name (String, converted to GQuark)
 *  |->trace
 *  | |->name (String, converted to GQuark)
 *  |->state
 *    |->pid (guint)
 *    |->ppid (guint)
 *    |->creation_time (LttTime)
 *    |->insertion_time (LttTime)
 *    |->process_name (String, converted to GQuark)
 *    |->thread_brand (String, converted to GQuark)
 *    |->execution_mode (LttvExecutionMode)
 *    |->execution_submode (LttvExecutionSubmode)
 *    |->process_status (LttvProcessStatus)
 *    |->cpu (guint)
 *  \endverbatim
 */

/*
 *  \todo 
 *  - refine switch of expression in multiple uses functions
 *  - remove the idle expressions in the tree 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#define TEST
#ifdef TEST
#include <time.h>
#include <sys/time.h>
#endif

#include <lttv/lttv.h>
#include <lttv/filter.h>
#include <ltt/trace.h>
#include <stdlib.h>
#include <string.h>

/**
 * @fn LttvSimpleExpression* lttv_simple_expression_new()
 * 
 * Constructor for LttvSimpleExpression
 * @return pointer to new LttvSimpleExpression
 */
LttvSimpleExpression* 
lttv_simple_expression_new() {

  LttvSimpleExpression* se = g_new(LttvSimpleExpression,1);

  se->field = LTTV_FILTER_UNDEFINED;
  se->op = NULL;
  se->offset = 0;

  return se;
}

/*
 * Keeps the array order.
 */
static inline gpointer ltt_g_ptr_array_remove_index_slow(GPtrArray *fp,
    int index)
{
  gpointer ptr;
  int i;

  if (fp->len == 0)
    return NULL;

  ptr = g_ptr_array_index(fp, index);
  for (i = index; i < fp->len - 1; i++) {
    g_ptr_array_index(fp, i) = g_ptr_array_index(fp, i + 1);
  }
  g_ptr_array_remove_index(fp, fp->len - 1);
  return ptr;
}

/**
 *  @fn gboolean lttv_simple_expression_assign_field(GPtrArray*,LttvSimpleExpression*)
 * 
 *  Parse through filtering field hierarchy as specified 
 *  by user.  This function compares each value to 
 *  predetermined quarks
 *  @param fp The field path list
 *  @param se current simple expression
 *  @return success/failure of operation
 */
gboolean
lttv_simple_expression_assign_field(GPtrArray* fp, LttvSimpleExpression* se) {

  GString* f = NULL;

  if(fp->len < 2) return FALSE;
  g_assert((f=ltt_g_ptr_array_remove_index_slow(fp,0)));


  /*
   * Parse through the specified 
   * hardcoded fields.
   *
   * Take note however that the 
   * 'event' subfields might change 
   * depending on values specified 
   * in core.xml file.  Hence, if 
   * none of the subfields in the 
   * array match the hardcoded 
   * subfields, it will be considered 
   * as a dynamic field
   */

  if(!g_strcasecmp(f->str,"trace") ) {
    /*
     * Possible values:
     *  trace.name
     */
    g_string_free(f,TRUE);
    f=ltt_g_ptr_array_remove_index_slow(fp,0);
    if(!g_strcasecmp(f->str,"name")) {
      se->field = LTTV_FILTER_TRACE_NAME;    
    }
  } else if(!g_strcasecmp(f->str,"traceset") ) {
    /* 
     * FIXME: not yet implemented !
     */
  } else if(!g_strcasecmp(f->str,"tracefile")
            || !g_strcasecmp(f->str,"channel") ) {
    /*
     * Possible values:
     *  tracefile.name
     *  channel.name
     */
    g_string_free(f,TRUE);
    f=ltt_g_ptr_array_remove_index_slow(fp,0);
    if(!g_strcasecmp(f->str,"name")) {
      se->field = LTTV_FILTER_TRACEFILE_NAME;
    }
  } else if(!g_strcasecmp(f->str,"state") ) {
    /*
     * Possible values:
     *  state.pid
     *  state.ppid
     *  state.creation_time
     *  state.insertion_time
     *  state.process_name
     *  state.thread_brand
     *  state.execution_mode
     *  state.execution_submode
     *  state.process_status
     *  state.cpu
     */
    g_string_free(f,TRUE);
    f=ltt_g_ptr_array_remove_index_slow(fp,0);
    if(!g_strcasecmp(f->str,"pid") ) { 
      se->field = LTTV_FILTER_STATE_PID; 
    }
    else if(!g_strcasecmp(f->str,"ppid") ) { 
      se->field = LTTV_FILTER_STATE_PPID; 
    }
    else if(!g_strcasecmp(f->str,"creation_time") ) {
      se->field = LTTV_FILTER_STATE_CT;
    }
    else if(!g_strcasecmp(f->str,"insertion_time") ) {
      se->field = LTTV_FILTER_STATE_IT;
    }
    else if(!g_strcasecmp(f->str,"process_name") ) {
      se->field = LTTV_FILTER_STATE_P_NAME;
    }
    else if(!g_strcasecmp(f->str,"thread_brand") ) {
      se->field = LTTV_FILTER_STATE_T_BRAND;
    }
    else if(!g_strcasecmp(f->str,"execution_mode") ) {
      se->field = LTTV_FILTER_STATE_EX_MODE;
    }
    else if(!g_strcasecmp(f->str,"execution_submode") ) {
      se->field = LTTV_FILTER_STATE_EX_SUBMODE;
    }
    else if(!g_strcasecmp(f->str,"process_status") ) {
      se->field = LTTV_FILTER_STATE_P_STATUS;
    }
    else if(!g_strcasecmp(f->str,"cpu") ) {
      se->field = LTTV_FILTER_STATE_CPU;
    }
  } else if(!g_strcasecmp(f->str,"event") ) {
    /*
     * Possible values:
     *  event.name
     *  event.channel
     *  event.category
     *  event.time
     *  event.tsc
     *  event.target_pid
     *  event.field
     */
    g_string_free(f,TRUE);
    f=ltt_g_ptr_array_remove_index_slow(fp,0);

    if(!g_strcasecmp(f->str,"name") ) {
      se->field = LTTV_FILTER_EVENT_NAME;
    }
    else if(!g_strcasecmp(f->str,"subname") ) {
      se->field = LTTV_FILTER_EVENT_SUBNAME;
    }
    else if(!g_strcasecmp(f->str,"category") ) {
      /*
       * FIXME: Category not yet functional in lttv
       */
      se->field = LTTV_FILTER_EVENT_CATEGORY;
    }
    else if(!g_strcasecmp(f->str,"time") ) {
      se->field = LTTV_FILTER_EVENT_TIME;
    }
    else if(!g_strcasecmp(f->str,"tsc") ) {
      se->field = LTTV_FILTER_EVENT_TSC;
    }
    else if(!g_strcasecmp(f->str,"target_pid") ) {
      se->field = LTTV_FILTER_EVENT_TARGET_PID;
    }
    else if(!g_strcasecmp(f->str,"field") ) {
      se->field = LTTV_FILTER_EVENT_FIELD;
      g_string_free(f,TRUE);
      f=ltt_g_ptr_array_remove_index_slow(fp,0);

    } else {
      //g_string_free(f,TRUE);
      //f=ltt_g_ptr_array_remove_index_slow(fp,0);
      g_warning("Unknown event filter subtype %s", f->str);
    }
  } else {
    g_string_free(f,TRUE);
    f=ltt_g_ptr_array_remove_index_slow(fp,0);

    g_warning("Unrecognized field in filter string");
  }

  /* free memory for last string */
  g_string_free(f,TRUE);

  /* array should be empty */
  g_assert(fp->len == 0);
 
  if(se->field == LTTV_FILTER_UNDEFINED) {
    g_warning("The specified field was not recognized !");
    return FALSE;
  }  
  return TRUE;  
}

/**
 *  @fn gboolean lttv_simple_expression_assign_operator(LttvSimpleExpression*,LttvExpressionOp)
 * 
 *  Sets the function pointer for the current
 *  Simple Expression
 *  @param se current simple expression
 *  @param op current operator
 *  @return success/failure of operation
 */
gboolean 
lttv_simple_expression_assign_operator(LttvSimpleExpression* se, LttvExpressionOp op) {
     
  switch(se->field) {
     /* 
      * string
      */
     case LTTV_FILTER_TRACE_NAME:
     case LTTV_FILTER_TRACEFILE_NAME:
     case LTTV_FILTER_STATE_P_NAME:
     case LTTV_FILTER_STATE_T_BRAND:
     case LTTV_FILTER_EVENT_SUBNAME:
     case LTTV_FILTER_STATE_EX_MODE:
     case LTTV_FILTER_STATE_EX_SUBMODE:
     case LTTV_FILTER_STATE_P_STATUS:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_quark;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_quark;
           break;
         default:
           g_warning("Error encountered in operator assignment = or != expected");
           return FALSE;
       }
       break;
     /*
      * two strings.
      */
     case LTTV_FILTER_EVENT_NAME:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_quarks;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_quarks;
           break;
         default:
           g_warning("Error encountered in operator assignment = or != expected");
           return FALSE;
       }
     /* 
      * integer
      */
     case LTTV_FILTER_EVENT_TSC:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_uint64;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_uint64;
           break;
         case LTTV_FIELD_LT:
           se->op = lttv_apply_op_lt_uint64;
           break;
         case LTTV_FIELD_LE:
           se->op = lttv_apply_op_le_uint64;
           break;
         case LTTV_FIELD_GT:
           se->op = lttv_apply_op_gt_uint64;
           break;
         case LTTV_FIELD_GE:
           se->op = lttv_apply_op_ge_uint64;
           break;
         default:
           g_warning("Error encountered in operator assignment");
           return FALSE;
       }
       break;
     /* 
      * unsigned integers
      */
     case LTTV_FILTER_STATE_CPU:
     case LTTV_FILTER_STATE_PID:
     case LTTV_FILTER_STATE_PPID:
     case LTTV_FILTER_EVENT_TARGET_PID:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_uint;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_uint;
           break;
         case LTTV_FIELD_LT:
           se->op = lttv_apply_op_lt_uint;
           break;
         case LTTV_FIELD_LE:
           se->op = lttv_apply_op_le_uint;
           break;
         case LTTV_FIELD_GT:
           se->op = lttv_apply_op_gt_uint;
           break;
         case LTTV_FIELD_GE:
           se->op = lttv_apply_op_ge_uint;
           break;
         default:
           g_warning("Error encountered in operator assignment");
           return FALSE;
       }
       break;

     /*
      * Enums
      * Entered as string, converted to enum
      * 
      * can only be compared with 'equal' or 'not equal' operators
      *
      * unsigned int of 16 bits are used here since enums 
      * should not go over 2^16-1 values
      */
//      case /*NOTHING*/:
//       switch(op) {
//         case LTTV_FIELD_EQ:
//           se->op = lttv_apply_op_eq_uint16;
//           break;
//         case LTTV_FIELD_NE:
//           se->op = lttv_apply_op_ne_uint16;
//           break;
//         default:
//           g_warning("Error encountered in operator assignment = or != expected");
//           return FALSE;
//       }
//       break;
     /*
      * Ltttime
      */
     case LTTV_FILTER_STATE_CT:
     case LTTV_FILTER_STATE_IT:
     case LTTV_FILTER_EVENT_TIME:
       switch(op) {
         case LTTV_FIELD_EQ:
           se->op = lttv_apply_op_eq_ltttime;
           break;
         case LTTV_FIELD_NE:
           se->op = lttv_apply_op_ne_ltttime;
           break;
         case LTTV_FIELD_LT:
           se->op = lttv_apply_op_lt_ltttime;
           break;
         case LTTV_FIELD_LE:
           se->op = lttv_apply_op_le_ltttime;
           break;
         case LTTV_FIELD_GT:
           se->op = lttv_apply_op_gt_ltttime;
           break;
         case LTTV_FIELD_GE:
           se->op = lttv_apply_op_ge_ltttime;
           break;
         default:
           g_warning("Error encountered in operator assignment");
           return FALSE;
       }
       break;
     default:
       g_warning("Error encountered in operator assignation ! Field type:%i",se->field);
       return FALSE;
   }
  
  return TRUE;

}

/**
 *  @fn gboolean lttv_simple_expression_assign_value(LttvSimpleExpression*,char*)
 *
 *  Assign the value field to the current LttvSimpleExpression
 *  @param se pointer to the current LttvSimpleExpression
 *  @param value string value for simple expression
 */
gboolean 
lttv_simple_expression_assign_value(LttvSimpleExpression* se, char* value) {

  unsigned i;
  gboolean is_double = FALSE;
  LttTime t = ltt_time_zero;
  GString* v;
  guint string_len;
  
  switch(se->field) {
     /* 
      * Strings
      * entered as strings, converted to Quarks
      */
     case LTTV_FILTER_TRACE_NAME:
     case LTTV_FILTER_TRACEFILE_NAME:
     case LTTV_FILTER_STATE_P_NAME:
     case LTTV_FILTER_STATE_T_BRAND:
     case LTTV_FILTER_EVENT_SUBNAME:
     case LTTV_FILTER_STATE_EX_MODE:
     case LTTV_FILTER_STATE_EX_SUBMODE:
     case LTTV_FILTER_STATE_P_STATUS:
      // se->value.v_string = value;
       se->value.v_quark = g_quark_from_string(value);
       g_free(value);
       break;
     /*
      * Two strings.
      */
     case LTTV_FILTER_EVENT_NAME:
       {
         /* channel.event */
         char *end = strchr(value, '.');
         if (end) {
           *end = '\0';
           end++;
           se->value.v_quarks.q[0] = g_quark_from_string(value);
           se->value.v_quarks.q[1] = g_quark_from_string(end);
         } else {
           se->value.v_quarks.q[0] = (GQuark)0;
           se->value.v_quarks.q[1] = g_quark_from_string(value);
         }
         g_free(value);
       }
       break;
     /* 
      * integer -- supposed to be uint64
      */
     case LTTV_FILTER_EVENT_TSC:
       se->value.v_uint64 = atoi(value);
       g_free(value);
       break;
     /*
      * unsigned integers
      */
     case LTTV_FILTER_STATE_PID:
     case LTTV_FILTER_STATE_PPID:
     case LTTV_FILTER_STATE_CPU:
     case LTTV_FILTER_EVENT_TARGET_PID:
       se->value.v_uint = atoi(value);
       g_free(value);
       break;
     /*
      * LttTime
      */
     case LTTV_FILTER_STATE_CT:
     case LTTV_FILTER_STATE_IT:
     case LTTV_FILTER_EVENT_TIME:
       //se->value.v_double = atof(value);
       /*
        * parsing logic could be optimised,
        * but as for now, simpler this way
        */
       v = g_string_new("");
       string_len = strlen(value);
       for(i=0;i<string_len;i++) {
          if(value[i] == '.') { 
              /* cannot specify number with more than one '.' */
              if(is_double) return FALSE; 
              else is_double = TRUE;
              t.tv_sec = atoi(v->str);
              g_string_free(v,TRUE);
              v = g_string_new("");
          } else v = g_string_append_c(v,value[i]);
       }
       /* number can be integer or double */
       if(is_double) t.tv_nsec = atoi(v->str);
       else {
         t.tv_sec = atoi(v->str);
         t.tv_nsec = 0;
       }
       
       g_string_free(v,TRUE);
       
       se->value.v_ltttime = t;
       g_free(value);
       break;
     default:
       g_warning("Error encountered in value assignation ! Field type = %i",se->field);
       g_free(value);
       return FALSE;
   }
  
  return TRUE;
  
}

/**
 *  @fn void lttv_simple_expression_destroy(LttvSimpleExpression*)
 *
 *  Disallocate memory for the current 
 *  simple expression
 *  @param se pointer to the current LttvSimpleExpression
 */
void
lttv_simple_expression_destroy(LttvSimpleExpression* se) {
  
 // g_free(se->value);
//  switch(se->field) {
//     case LTTV_FILTER_TRACE_NAME:
//     case LTTV_FILTER_TRACEFILE_NAME:
//     case LTTV_FILTER_STATE_P_NAME:
//     case LTTV_FILTER_EVENT_NAME:
//       g_free(se->value.v_string);
//       break;
//  }
  g_free(se);

}

/**
 *  @fn gint lttv_struct_type(gint)
 * 
 *  Finds the structure type depending 
 *  on the fields in parameters
 *  @params ft Field of the current structure
 *  @return LttvStructType enum or -1 for error
 */
gint
lttv_struct_type(gint ft) {
  
    switch(ft) {
        case LTTV_FILTER_TRACE_NAME:
            return LTTV_FILTER_TRACE;
            break;
        case LTTV_FILTER_TRACEFILE_NAME:
            return LTTV_FILTER_TRACEFILE;
            break;
        case LTTV_FILTER_STATE_PID:
        case LTTV_FILTER_STATE_PPID:
        case LTTV_FILTER_STATE_CT:
        case LTTV_FILTER_STATE_IT:
        case LTTV_FILTER_STATE_P_NAME:
        case LTTV_FILTER_STATE_T_BRAND:
        case LTTV_FILTER_STATE_EX_MODE:
        case LTTV_FILTER_STATE_EX_SUBMODE:
        case LTTV_FILTER_STATE_P_STATUS:
        case LTTV_FILTER_STATE_CPU:
            return LTTV_FILTER_STATE;
            break;
        case LTTV_FILTER_EVENT_NAME:
        case LTTV_FILTER_EVENT_SUBNAME:
        case LTTV_FILTER_EVENT_CATEGORY:
        case LTTV_FILTER_EVENT_TIME:
        case LTTV_FILTER_EVENT_TSC:
        case LTTV_FILTER_EVENT_TARGET_PID:
        case LTTV_FILTER_EVENT_FIELD:
            return LTTV_FILTER_EVENT;
            break;
        default:
            return -1;
    }
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint(const gpointer v1, LttvFieldValue v2) {

  guint* r = (guint*) v1;
  return (*r == v2.v_uint);
  
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint64(const gpointer v1, LttvFieldValue v2) {

  guint64* r = (guint64*) v1;
  return (*r == v2.v_uint64);
  
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r == v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_eq_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r == v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_eq_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r == v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_eq_string(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_string(const gpointer v1, LttvFieldValue v2) {
  char* r = (char*) v1;
  return (!g_strcasecmp(r,v2.v_string));
}

/**
 *  @fn gboolean lttv_apply_op_eq_quark(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_quark(const gpointer v1, LttvFieldValue v2) {
  GQuark* r = (GQuark*) v1;
  return (*r == v2.v_quark);
}

/**
 *  @fn gboolean lttv_apply_op_eq_quarks(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_quarks(const gpointer v1, LttvFieldValue v2) {
  GQuark *r1 = (GQuark *) v1;
  GQuark *r2 = r1 + 1;
  if (likely(*r1 != (GQuark)0) && *r1 != v2.v_quarks.q[0])
    return 0;
  if (*r2 != v2.v_quarks.q[1])
    return 0;
  return 1;
}

/**
 *  @fn gboolean lttv_apply_op_eq_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_eq_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
  return ltt_time_compare(*r, v2.v_ltttime)==0?1:0;
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint(const gpointer v1, LttvFieldValue v2) {
  guint* r = (guint*) v1;
  return (*r != v2.v_uint);
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint64(const gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r != v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r != v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_ne_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r != v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_ne_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r != v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_ne_string(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_string(const gpointer v1, LttvFieldValue v2) {
  char* r = (char*) v1;
  return (g_strcasecmp(r,v2.v_string));
}

/**
 *  @fn gboolean lttv_apply_op_ne_quark(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_quark(const gpointer v1, LttvFieldValue v2) {
  GQuark* r = (GQuark*) v1;
  return (*r != v2.v_quark);
}

/**
 *  @fn gboolean lttv_apply_op_ne_quarks(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'equal' operator to the
 *  specified structure and value
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_quarks(const gpointer v1, LttvFieldValue v2) {
  GQuark *r1 = (GQuark *) v1;
  GQuark *r2 = r1 + 1;
  if ((*r1 == (GQuark)0 || *r1 == v2.v_quarks.q[0]) && *r2 == v2.v_quarks.q[1])
    return 0;
  else
    return 1;
}

/**
 *  @fn gboolean lttv_apply_op_ne_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'not equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ne_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
  return ltt_time_compare(*r, v2.v_ltttime)!=0?1:0;
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint(const gpointer v1, LttvFieldValue v2) {
  guint* r = (guint*) v1;
  return (*r < v2.v_uint);
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint64(const gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r < v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r < v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_lt_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r < v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_lt_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r < v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_lt_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_lt_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
//  return ((r->tv_sec < v2.v_ltttime.tv_sec) || ((r->tv_sec == v2.v_ltttime.tv_sec) && (r->tv_nsec < v2.v_ltttime.tv_nsec)));
  return ltt_time_compare(*r, v2.v_ltttime)==-1?1:0;
}

/**
 *  @fn gboolean lttv_apply_op_le_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint(const gpointer v1, LttvFieldValue v2) {
  guint* r = (guint*) v1;
  return (*r <= v2.v_uint);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint64(const gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r <= v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r <= v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_le_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r <= v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_le_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r <= v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_le_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'lower or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_le_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
//  return ((r->tv_sec < v2.v_ltttime.tv_sec) || ((r->tv_sec == v2.v_ltttime.tv_sec) && (r->tv_nsec <= v2.v_ltttime.tv_nsec)));
  return ltt_time_compare(*r, v2.v_ltttime)<1?1:0;
}


/**
 *  @fn gboolean lttv_apply_op_gt_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint(const gpointer v1, LttvFieldValue v2) {
  guint* r = (guint*) v1;
  return (*r > v2.v_uint);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint64(const gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r > v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r > v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_gt_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r > v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_gt_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r > v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_gt_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater than' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_gt_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
//  return ((r->tv_sec > v2.v_ltttime.tv_sec) || ((r->tv_sec == v2.v_ltttime.tv_sec) && (r->tv_nsec > v2.v_ltttime.tv_nsec)));
  return ltt_time_compare(*r, v2.v_ltttime)==1?1:0;
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint(const gpointer v1, LttvFieldValue v2) {
  guint* r = (guint*) v1;
  return (*r >= v2.v_uint);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint64(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint64(const gpointer v1, LttvFieldValue v2) {
  guint64* r = (guint64*) v1;
  return (*r >= v2.v_uint64);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint32(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint32(const gpointer v1, LttvFieldValue v2) {
  guint32* r = (guint32*) v1;
  return (*r >= v2.v_uint32);
}

/**
 *  @fn gboolean lttv_apply_op_ge_uint16(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_uint16(const gpointer v1, LttvFieldValue v2) {
  guint16* r = (guint16*) v1;
  return (*r >= v2.v_uint16);
}

/**
 *  @fn gboolean lttv_apply_op_ge_double(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_double(const gpointer v1, LttvFieldValue v2) {
  double* r = (double*) v1;
  return (*r >= v2.v_double);
}

/**
 *  @fn gboolean lttv_apply_op_ge_ltttime(gpointer,LttvFieldValue) 
 * 
 *  Applies the 'greater or equal' operator to the
 *  specified structure and value 
 *  @param v1 left member of comparison
 *  @param v2 right member of comparison
 *  @return success/failure of operation
 */
gboolean lttv_apply_op_ge_ltttime(const gpointer v1, LttvFieldValue v2) {
  LttTime* r = (LttTime*) v1;
//  return ((r->tv_sec > v2.v_ltttime.tv_sec) || ((r->tv_sec == v2.v_ltttime.tv_sec) && (r->tv_nsec >= v2.v_ltttime.tv_nsec)));
  return ltt_time_compare(*r, v2.v_ltttime)>-1?1:0;
}



/**
 *  Makes a copy of the current filter tree
 *  @param tree pointer to the current tree
 *  @return new copy of the filter tree
 */
LttvFilterTree*
lttv_filter_tree_clone(const LttvFilterTree* tree) {

  LttvFilterTree* newtree = lttv_filter_tree_new();  

  newtree->node = tree->node;
 
  newtree->left = tree->left;
  if(newtree->left == LTTV_TREE_NODE) {
    newtree->l_child.t = lttv_filter_tree_clone(tree->l_child.t);
  } else if(newtree->left == LTTV_TREE_LEAF) {
    newtree->l_child.leaf = lttv_simple_expression_new();
    newtree->l_child.leaf->field = tree->l_child.leaf->field;
    newtree->l_child.leaf->offset = tree->l_child.leaf->offset;
    newtree->l_child.leaf->op = tree->l_child.leaf->op;
    /* FIXME: special case for string copy ! */
    newtree->l_child.leaf->value = tree->l_child.leaf->value;
  }
 
  newtree->right = tree->right;
  if(newtree->right == LTTV_TREE_NODE) {
    newtree->r_child.t = lttv_filter_tree_clone(tree->r_child.t);
  } else if(newtree->right == LTTV_TREE_LEAF) {
    newtree->r_child.leaf = lttv_simple_expression_new();
    newtree->r_child.leaf->field = tree->r_child.leaf->field;
    newtree->r_child.leaf->offset = tree->r_child.leaf->offset;
    newtree->r_child.leaf->op = tree->r_child.leaf->op;
    newtree->r_child.leaf->value = tree->r_child.leaf->value;
  }
  
  return newtree;
  
}

/**
 *  Makes a copy of the current filter
 *  @param filter pointer to the current filter
 *  @return new copy of the filter
 */
LttvFilter*
lttv_filter_clone(const LttvFilter* filter) {
 
  if(!filter) return NULL;

  LttvFilter* newfilter = g_new(LttvFilter,1); 

  strcpy(newfilter->expression,filter->expression);

  newfilter->head = lttv_filter_tree_clone(filter->head);
  
  return newfilter;
    
}


/**
 *  @fn LttvFilter* lttv_filter_new()
 * 
 *   Creates a new LttvFilter
 *   @return the current LttvFilter or NULL if error
 */
LttvFilter*
lttv_filter_new() {

  LttvFilter* filter = g_new(LttvFilter,1);
  filter->expression = NULL;
  filter->head = NULL;

  return filter;
    
}

/**
 *  @fn gboolean lttv_filter_update(LttvFilter*)
 * 
 *  Updates the current LttvFilter by building 
 *  its tree based upon the expression string
 *  @param filter pointer to the current LttvFilter
 *  @return Failure/Success of operation
 */
gboolean
lttv_filter_update(LttvFilter* filter) {
    
//  g_print("filter::lttv_filter_new()\n");    /* debug */
  
  if(filter->expression == NULL) return FALSE;
  
  int  
    i, 
    p_nesting=0,  /* parenthesis nesting value */
    not=0;
  guint expression_len;
    
  /* trees */
  LttvFilterTree
    *tree = lttv_filter_tree_new(),   /* main tree */
    *subtree = NULL,                  /* buffer for subtrees */
    *t1,                              /* buffer #1 */
    *t2,                              /* buffer #2 */
    *t3;                              /* buffer #3 */

  /* 
   * the filter
   * If the tree already exists, 
   * destroy it and build a new one
   */
  if(filter->head != NULL) lttv_filter_tree_destroy(filter->head);
  filter->head = NULL;    /* will be assigned at the end */
 
  /*
   * Tree Stack
   * each element of the list
   * is a sub tree created 
   * by the use of parenthesis in the 
   * global expression.  The final tree 
   * will be the one left at the root of 
   * the list
   */
  GPtrArray *tree_stack = g_ptr_array_new();
  g_ptr_array_add( tree_stack,(gpointer) tree );
  
  /* temporary values */
  GString *a_field_component = g_string_new(""); 
  GString *a_string_spaces = g_string_new(""); 
  GPtrArray *a_field_path = g_ptr_array_new(); 
  
  /* simple expression buffer */
  LttvSimpleExpression* a_simple_expression = lttv_simple_expression_new(); 

  gint nest_quotes = 0;
  
  /*
   *  Parse entire expression and construct
   *  the binary tree.  There are two steps 
   *  in browsing that string
   *    1. finding boolean ops " &,|,^,! " and parenthesis " {,(,[,],),} "
   *    2. finding simple expressions
   *      - field path ( separated by dots )
   *      - op ( >, <, =, >=, <=, !=)
   *      - value ( integer, string ... )
   *  To spare computing time, the whole 
   *  string is parsed in this loop for a 
   *  O(n) complexity order.
   *
   *  When encountering logical op &,|,^
   *    1. parse the last value if any
   *    2. create a new tree
   *    3. add the expression (simple exp, or exp (subtree)) to the tree
   *    4. concatenate this tree with the current tree on top of the stack
   *  When encountering math ops >,>=,<,<=,=,!=
   *    1. add to op to the simple expression
   *    2. concatenate last field component to field path
   *  When encountering concatening ops .
   *    1. concatenate last field component to field path
   *  When encountering opening parenthesis (,{,[
   *    1. create a new subtree on top of tree stack
   *  When encountering closing parenthesis ),},]
   *    1. add the expression on right child of the current tree
   *    2. the subtree is completed, allocate a new subtree
   *    3. pop the tree value from the tree stack
   */
 
#ifdef TEST
  struct timeval starttime;
  struct timeval endtime;
  gettimeofday(&starttime, NULL);
#endif
  
  expression_len = strlen(filter->expression);
  for(i=0;i<expression_len;i++) {
    // debug
//    g_print("%c\n ",filter->expression[i]);
    if(nest_quotes) {
      switch(filter->expression[i]) {
        case '\\' :
          if(filter->expression[i+1] == '\"') {
            i++;
          }
          break;
        case '\"':
          nest_quotes = 0;
          i++;
          break;
      }
      if(a_string_spaces->len != 0) {
        a_field_component = g_string_append(
          a_field_component, a_string_spaces->str);
        a_string_spaces = g_string_set_size(a_string_spaces, 0);
      }
      a_field_component = g_string_append_c(a_field_component,
        filter->expression[i]);
      continue;
    }

    switch(filter->expression[i]) {
      /*
       *   logical operators
       */
      case '&':   /* and */
    
        /* get current tree in tree stack */
        t1 = (LttvFilterTree*)g_ptr_array_index(tree_stack,tree_stack->len-1);

        /* get current node at absolute right */
        while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_AND;
        t1->right = LTTV_TREE_NODE;
        t1->r_child.t = t2;
        if(not) {   /* add not operator to tree */
          t3 = lttv_filter_tree_new();
          t3->node = LTTV_LOGICAL_NOT;
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = t3;
          t2 = t3;
          not = 0;
        }
        if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
        } else {  /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          a_field_component = g_string_new("");
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
        }
        break;
      
      case '|':   /* or */
      
        t1 = (LttvFilterTree*)g_ptr_array_index(tree_stack,tree_stack->len-1);
         while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_OR;
        t1->right = LTTV_TREE_NODE;
        t1->r_child.t = t2;
        if(not) { // add not operator to tree
          t3 = lttv_filter_tree_new();
          t3->node = LTTV_LOGICAL_NOT;
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = t3;
          t2 = t3;
          not = 0;
       }
       if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
       } else {    /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          a_field_component = g_string_new("");
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new();
        }
        break;
      
      case '^':   /* xor */
        
        t1 = (LttvFilterTree*)g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) {
          g_assert(t1->right == LTTV_TREE_NODE);
          t1 = t1->r_child.t;
        }
        t2 = lttv_filter_tree_new();
        t2->node = LTTV_LOGICAL_XOR;
        t1->right = LTTV_TREE_NODE;
        t1->r_child.t = t2;
        if(not) { // add not operator to tree
          t3 = lttv_filter_tree_new();
          t3->node = LTTV_LOGICAL_NOT;
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = t3;
          t2 = t3;
          not = 0;
        }
        if(subtree != NULL) {   /* append subtree to current tree */
          t2->left = LTTV_TREE_NODE;
          t2->l_child.t = subtree;
          subtree = NULL;
        } else {    /* append a simple expression */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          a_field_component = g_string_new("");
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
          t2->left = LTTV_TREE_LEAF;
          t2->l_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
        }
        break;
      
      case '!':   /* not, or not equal (math op) */
        
        if(filter->expression[i+1] == '=') {  /* != */
          g_ptr_array_add( a_field_path,(gpointer) a_field_component );
          lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
          a_field_component = g_string_new("");         
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_NE);
          i++;
        } else {  /* ! */
          not=1;
        }
        break;
      
      case '(':   /* start of parenthesis */
      case '[':
      case '{':
        
        p_nesting++;      /* incrementing parenthesis nesting value */
        t1 = lttv_filter_tree_new();
        if(not) { /* add not operator to tree */
          t3 = lttv_filter_tree_new();
          t3->node = LTTV_LOGICAL_NOT;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t3;
          not = 0;
        }
        g_ptr_array_add( tree_stack,(gpointer) t1 );
        break;
      
      case ')':   /* end of parenthesis */
      case ']':
      case '}':
        
        p_nesting--;      /* decrementing parenthesis nesting value */
        if(p_nesting<0 || tree_stack->len<2) {
          g_warning("Wrong filtering options, the string\n\"%s\"\n\
                     is not valid due to parenthesis incorrect use",filter->expression);  
          return FALSE;
        }
  
        /* there must at least be the root tree left in the array */
        g_assert(tree_stack->len>0);
 
        t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
        while(t1->right != LTTV_TREE_IDLE) {
           t1 = t1->r_child.t;
        }
        if(not) { // add not operator to tree
          g_print("ici");
          t3 = lttv_filter_tree_new();
          t3->node = LTTV_LOGICAL_NOT;
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = t3;
          t1 = t3;
          not = 0;
        }
        if(subtree != NULL) {   /* append subtree to current tree */
          t1->right = LTTV_TREE_NODE;
          t1->r_child.t = subtree;
          subtree = g_ptr_array_index(tree_stack,tree_stack->len-1);
          g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        } else {    /* assign subtree as current tree */
          lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
          a_field_component = g_string_new("");
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
          t1->right = LTTV_TREE_LEAF;
          t1->r_child.leaf = a_simple_expression;
          a_simple_expression = lttv_simple_expression_new(); 
          subtree = g_ptr_array_remove_index(tree_stack,tree_stack->len-1);
        }
        break;

      /*  
       *  mathematic operators
       */
      case '<':   /* lower, lower or equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        g_string_free(a_string_spaces, TRUE);
        a_string_spaces = g_string_new("");
        if(filter->expression[i+1] == '=') { /* <= */
          i++;
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_LE);
        } else lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_LT);
       break;
      
      case '>':   /* higher, higher or equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );   
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        g_string_free(a_string_spaces, TRUE);
        a_string_spaces = g_string_new("");
        if(filter->expression[i+1] == '=') {  /* >= */
          i++;
          lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_GE);
        } else lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_GT);
       break;
      
      case '=':   /* equal */
        
        g_ptr_array_add( a_field_path,(gpointer) a_field_component );
        lttv_simple_expression_assign_field(a_field_path,a_simple_expression);
        a_field_component = g_string_new("");         
        g_string_free(a_string_spaces, TRUE);
        a_string_spaces = g_string_new("");
        lttv_simple_expression_assign_operator(a_simple_expression,LTTV_FIELD_EQ);
        break;
        
      /*
       *  Field concatening caracter
       */
      case '.':   /* dot */
        
        /*
         * divide field expression into elements 
         * in a_field_path array.
         *
         * A dot can also be present in double values
         */
        if(a_simple_expression->field == LTTV_FILTER_UNDEFINED) {
          g_ptr_array_add( a_field_path,(gpointer) a_field_component );
          a_field_component = g_string_new("");
          g_string_free(a_string_spaces, TRUE);
          a_string_spaces = g_string_new("");
        } else {
          /* Operator found, we are in the value field */
          g_string_append_c(a_field_component, filter->expression[i]);
        }
        break;
      case ' ':   /* keep spaces that are within a field component */
        if(a_field_component->len == 0) break; /* ignore */
        else 
          a_string_spaces = g_string_append_c(a_string_spaces,
                                              filter->expression[i]);

      case '\n':  /* ignore */
        break;
      case '\"':
               nest_quotes?(nest_quotes=0):(nest_quotes=1);
               break;
      default:    /* concatening current string */
              if(a_string_spaces->len != 0) {
                a_field_component = g_string_append(
                    a_field_component, a_string_spaces->str);
                a_string_spaces = g_string_set_size(a_string_spaces, 0);
              }
              a_field_component = g_string_append_c(a_field_component,
                    filter->expression[i]);
    }
  }

  /*
   * Preliminary check to see
   * if tree was constructed correctly
   */
  if( p_nesting>0 ) { 
    g_warning("Wrong filtering options, the string\n\"%s\"\n\
        is not valid due to parenthesis incorrect use",filter->expression);  
    return FALSE;
  }
 
  if(tree_stack->len != 1) /* only root tree should remain */ 
    return FALSE;
  
  /*  
   *  processing last element of expression   
   */
  t1 = g_ptr_array_index(tree_stack,tree_stack->len-1);
  while(t1->right != LTTV_TREE_IDLE) {
    g_assert(t1->right == LTTV_TREE_NODE);
    t1 = t1->r_child.t;
  }
  if(not) { // add not operator to tree
     t3 = lttv_filter_tree_new();
     t3->node = LTTV_LOGICAL_NOT;
     t1->right = LTTV_TREE_NODE;
     t1->r_child.t = t3;
     t1 = t3;
     not = 0;
  }
  if(subtree != NULL) {  /* add the subtree */
    t1->right = LTTV_TREE_NODE;
    t1->r_child.t = subtree;
    subtree = NULL;
  } else {  /* add a leaf */
    lttv_simple_expression_assign_value(a_simple_expression,g_string_free(a_field_component,FALSE)); 
    a_field_component = NULL;
    g_string_free(a_string_spaces, TRUE);
    a_string_spaces = NULL;
    t1->right = LTTV_TREE_LEAF;
    t1->r_child.leaf = a_simple_expression;
    a_simple_expression = NULL;
  }
  
  
  /* free the pointer array */
  g_assert(a_field_path->len == 0);
  g_ptr_array_free(a_field_path,TRUE);

  /* free the tree stack -- but keep the root tree */
  filter->head = ltt_g_ptr_array_remove_index_slow(tree_stack,0);
  g_ptr_array_free(tree_stack,TRUE);
  
  /* free the field buffer if allocated */
  if(a_field_component != NULL) g_string_free(a_field_component,TRUE); 
   if(a_string_spaces != NULL) g_string_free(a_string_spaces, TRUE);

  /* free the simple expression buffer if allocated */
  if(a_simple_expression != NULL) lttv_simple_expression_destroy(a_simple_expression);
  
  g_assert(filter->head != NULL); /* tree should exist */
  g_assert(subtree == NULL); /* remaining subtree should be included in main tree */
 
#ifdef TEST
  gettimeofday(&endtime, NULL);

  /* Calcul du temps de l'algorithme */
  double time1 = starttime.tv_sec + (starttime.tv_usec/1000000.0);
  double time2 = endtime.tv_sec + (endtime.tv_usec/1000000.0);
// g_print("Tree build took %.10f ms for strlen of %i\n",(time2-time1)*1000,strlen(filter->expression));
  g_print("%.10f %i\n",(time2-time1)*1000,strlen(filter->expression));
#endif
  
  /* debug */
  g_debug("+++++++++++++++ BEGIN PRINT ++++++++++++++++\n");
  lttv_print_tree(filter->head,0) ;
  g_debug("+++++++++++++++ END PRINT ++++++++++++++++++\n");
  
  /* success */
  return TRUE;

}

/**
 *  @fn void lttv_filter_destroy(LttvFilter*)
 * 
 *  Destroy the current LttvFilter
 *  @param filter pointer to the current LttvFilter
 */
void
lttv_filter_destroy(LttvFilter* filter) {
  
  if(!filter) return;

  if(filter->expression)
    g_free(filter->expression);
  if(filter->head)
    lttv_filter_tree_destroy(filter->head);
  g_free(filter);
  
}

/**
 *  @fn LttvFilterTree* lttv_filter_tree_new()
 * 
 *  Assign a new tree for the current expression
 *  or sub expression
 *  @return pointer of LttvFilterTree
 */
LttvFilterTree* 
lttv_filter_tree_new() {
  LttvFilterTree* tree;

  tree = g_new(LttvFilterTree,1);
  tree->node = 0; //g_new(lttv_expression,1);
  tree->left = LTTV_TREE_IDLE;
  tree->right = LTTV_TREE_IDLE;
  tree->r_child.t = NULL;
  tree->l_child.t = NULL;
  
  return tree;
}

/**
 *  @fn void lttv_filter_append_expression(LttvFilter*,char*)
 * 
 *  Append a new expression to the expression 
 *  defined in the current filter
 *  @param filter pointer to the current LttvFilter
 *  @param expression string that must be appended
 *  @return Success/Failure of operation
 */
gboolean 
lttv_filter_append_expression(LttvFilter* filter, const char *expression) {

  if(expression == NULL) return FALSE;
  if(filter == NULL) return FALSE;
  if(expression[0] == '\0') return FALSE;  /* Empty expression */

  GString* s = g_string_new("");
  if(filter->expression != NULL) {
    s = g_string_append(s,filter->expression);
    s = g_string_append_c(s,'&');
  }
  s = g_string_append(s,expression);
 
  g_free(filter->expression);
  filter->expression = g_string_free(s,FALSE);
  
  /* TRUE if construction of tree proceeded without errors */
  return lttv_filter_update(filter);
  
}

/**
 *  @fn void lttv_filter_clear_expression(LttvFilter*)
 * 
 *  Clear the filter expression from the 
 *  current filter and sets its pointer to NULL
 *  @param filter pointer to the current LttvFilter
 */
void 
lttv_filter_clear_expression(LttvFilter* filter) {
  
  if(filter->expression != NULL) {
    g_free(filter->expression);
    filter->expression = NULL;
  }
  
}

/**
 *  @fn void lttv_filter_tree_destroy(LttvFilterTree*)
 * 
 *  Destroys the tree and his sub-trees
 *  @param tree Tree which must be destroyed
 */
void 
lttv_filter_tree_destroy(LttvFilterTree* tree) {
 
  if(tree == NULL) return;

  if(tree->left == LTTV_TREE_LEAF) lttv_simple_expression_destroy(tree->l_child.leaf);
  else if(tree->left == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->l_child.t);

  if(tree->right == LTTV_TREE_LEAF) lttv_simple_expression_destroy(tree->r_child.leaf);
  else if(tree->right == LTTV_TREE_NODE) lttv_filter_tree_destroy(tree->r_child.t);

//  g_free(tree->node);
  g_free(tree);
}

/**
 *  Global parsing function for the current
 *  LttvFilterTree
 *  @param t pointer to the current LttvFilterTree
 *  @param event current LttEvent, NULL if not used
 *  @param tracefile current LttTracefile, NULL if not used
 *  @param trace current LttTrace, NULL if not used
 *  @param state current LttvProcessState, NULL if not used
 *  @param context current LttvTracefileContext, NULL if not used
 *  @return response of filter
 */
gboolean
lttv_filter_tree_parse(
        const LttvFilterTree* t,
        const LttEvent* event,
        const LttTracefile* tracefile,
        const LttTrace* trace,
        const LttvTracefileContext* context,
	const LttvProcessState* state,
	const LttvTraceContext* tc
        /*,...*/) 
{

   /*
   *  Each tree is parsed in inorder.
   *  This way, it's possible to apply the left filter of the 
   *  tree, then decide whether or not the right branch should 
   *  be parsed depending on the linking logical operator
   *
   *  Each node consists in a
   *  1. logical operator
   *  2. left child ( node or simple expression )
   *  3. right child ( node or simple expression )
   *  
   *  When the child is a simple expression, we must 
   *  before all determine if the expression refers to 
   *  a structure which is whithin observation ( not NULL ). 
   *    -If so, the expression is evaluated.
   *    -If not, the result is set to TRUE since this particular 
   *     operation does not interfere with the lttv structure
   *
   *  The result of each simple expression will directly 
   *  affect the next branch.  This way, depending on 
   *  the linking logical operator, the parser will decide 
   *  to explore or not the next branch.
   *  1. AND OPERATOR
   *     -If result of left branch is 0 / FALSE
   *      then don't explore right branch and return 0;
   *     -If result of left branch is 1 / TRUE then explore
   *  2. OR OPERATOR
   *     -If result of left branch is 1 / TRUE
   *      then don't explore right branch and return 1;
   *     -If result of left branch is 0 / FALSE then explore
   *  3. XOR OPERATOR
   *     -Result of left branch will not affect exploration of 
   *      right branch
   */
    
  gboolean lresult = FALSE, rresult = FALSE;

  LttvTraceState *ts = NULL;
  LttvTracefileState *tfs = (LttvTracefileState*)context;
  if(tc)
    ts = (LttvTraceState*)tc;
  else if(context)
    ts = (LttvTraceState*)context->t_context;

  if(tfs) {
    guint cpu = tfs->cpu;
    if(ts)
      state = ts->running_process[cpu];
  }
  
  /*
   * Parse left branch
   */
  if(t->left == LTTV_TREE_NODE) {
      lresult = lttv_filter_tree_parse(t->l_child.t,event,tracefile,trace,context,NULL,NULL);
  }
  else if(t->left == LTTV_TREE_LEAF) {
      lresult = lttv_filter_tree_parse_branch(t->l_child.leaf,event,tracefile,trace,state,context);
  }
   
  /*
   * Parse linking operator
   * make a cutoff if possible
   */
  if((t->node & LTTV_LOGICAL_OR) && lresult == TRUE) return TRUE;
  if((t->node & LTTV_LOGICAL_AND) && lresult == FALSE) return FALSE;

  /*
   * Parse right branch
   */
  if(t->right == LTTV_TREE_NODE) {
      rresult = lttv_filter_tree_parse(t->r_child.t,event,tracefile,trace,context,NULL,NULL);
  }
  else if(t->right == LTTV_TREE_LEAF) {
      rresult = lttv_filter_tree_parse_branch(t->r_child.leaf,event,tracefile,trace,state,context);
  }

  
  /*
   * Apply and return the 
   * logical link between the 
   * two operation
   */
  switch(t->node) {
    case LTTV_LOGICAL_OR: return (lresult | rresult);
    case LTTV_LOGICAL_AND: return (lresult & rresult);
    case LTTV_LOGICAL_NOT: 
      return (t->left==LTTV_TREE_LEAF)?!lresult:((t->right==LTTV_TREE_LEAF)?!rresult:TRUE);
    case LTTV_LOGICAL_XOR: return (lresult ^ rresult);
    case 0: return (rresult);
    default: 
      /*
       * This case should never be 
       * parsed, if so, this subtree
       * is cancelled !
       */
      return TRUE;
  }
  
}

/**
 *  This function parses a particular branch of the tree
 *  @param se pointer to the current LttvSimpleExpression
 *  @param event current LttEvent, NULL if not used
 *  @param tracefile current LttTracefile, NULL if not used
 *  @param trace current LttTrace, NULL if not used
 *  @param state current LttvProcessState, NULL if not used
 *  @param context current LttvTracefileContext, NULL if not used
 *  @return response of filter
 */
gboolean 
lttv_filter_tree_parse_branch(
        const LttvSimpleExpression* se,
        const LttEvent* event,
        const LttTracefile* tracefile,
        const LttTrace* trace,
        const LttvProcessState* state,
        const LttvTracefileContext* context) {

    LttvFieldValue v;
    v = se->value;
    switch(se->field) {
        case LTTV_FILTER_TRACE_NAME:
            if(trace == NULL) return TRUE;
            else {
                GQuark quark = ltt_trace_name(trace);
                return se->op((gpointer)&quark,v);
            }
            break;
        case LTTV_FILTER_TRACEFILE_NAME:
            if(tracefile == NULL) return TRUE;
            else {
                GQuark quark = ltt_tracefile_name(tracefile);
                return se->op((gpointer)&quark,v);
            }
            break;
        case LTTV_FILTER_STATE_PID:
            if(state == NULL) return TRUE;
            else return se->op((gpointer)&state->pid,v);
            break;
        case LTTV_FILTER_STATE_PPID:
            if(state == NULL) return TRUE;
            else return se->op((gpointer)&state->ppid,v);
            break;
        case LTTV_FILTER_STATE_CT:
            if(state == NULL) return TRUE;
            else {
              return se->op((gpointer)&state->creation_time,v);
            }
            break;
        case LTTV_FILTER_STATE_IT:
            if(state == NULL) return TRUE;
            else {
              return se->op((gpointer)&state->insertion_time,v);
            }
            break;
        case LTTV_FILTER_STATE_P_NAME:
            if(state == NULL) return TRUE;
            else {
              GQuark quark = state->name;
              return se->op((gpointer)&quark,v);
            }
            break;
        case LTTV_FILTER_STATE_T_BRAND:
            if(state == NULL) return TRUE;
            else {
              GQuark quark = state->brand;
              return se->op((gpointer)&quark,v);
            }
            break;
        case LTTV_FILTER_STATE_EX_MODE:
            if(state == NULL) return TRUE;
            else return se->op((gpointer)&state->state->t,v);
            break;
        case LTTV_FILTER_STATE_EX_SUBMODE:
            if(state == NULL) return TRUE;
            else return se->op((gpointer)&state->state->n,v);
            break;
        case LTTV_FILTER_STATE_P_STATUS:
            if(state == NULL) return TRUE;
            else return se->op((gpointer)&state->state->s,v);
            break;
        case LTTV_FILTER_STATE_CPU:
            if(state == NULL) return TRUE;
            else {
              return se->op((gpointer)&state->cpu,v);
            }
            break;
        case LTTV_FILTER_EVENT_NAME:
            if(event == NULL) return TRUE;
            else {
              struct marker_info *info;
              GQuark qtuple[2];
	      LttTracefile *tf = context->tf;
              qtuple[0] = ltt_tracefile_name(tracefile);
              info = marker_get_info_from_id(tf->mdata, event->event_id);
              g_assert(info != NULL);
              qtuple[1] = info->name;
              return se->op((gpointer)qtuple,v);
            }
            break;
        case LTTV_FILTER_EVENT_SUBNAME:
            if(event == NULL) return TRUE;
            else {
              struct marker_info *info;
	      LttTracefile *tf = context->tf;
              info = marker_get_info_from_id(tf->mdata, event->event_id);
              g_assert(info != NULL);
              GQuark quark = info->name;
              return se->op((gpointer)&quark,v);
            }
            break;
        case LTTV_FILTER_EVENT_CATEGORY:
            /*
             * TODO: Not yet implemented
             */
            return TRUE;
            break;
        case LTTV_FILTER_EVENT_TIME:
            if(event == NULL) return TRUE;
            else {
                LttTime time = ltt_event_time(event);
                return se->op((gpointer)&time,v);
            }
            break;
        case LTTV_FILTER_EVENT_TSC:
            if(event == NULL) return TRUE;
            else {
              LttCycleCount count = ltt_event_cycle_count(event);
              return se->op((gpointer)&count,v);
            }
            break;
        case LTTV_FILTER_EVENT_TARGET_PID:
            if(context == NULL) return TRUE;
            else {
              guint target_pid =
		      lttv_state_get_target_pid((LttvTracefileState*)context);
              return se->op((gpointer)&target_pid,v);
            }
            break;
        case LTTV_FILTER_EVENT_FIELD:
            /*
             * TODO: Use the offset to 
             * find the dynamic field 
             * in the event struct
             */
            return TRUE; 
        default:
            /*
             * This case should never be 
             * parsed, if so, the whole 
             * filtering is cancelled
             */
            g_warning("Error while parsing the filter tree");
            return TRUE;
    }

    /* should never get here */
    return TRUE;
    
}



/**
 *  Debug function.  Prints tree memory allocation.
 *  @param t the pointer to the current LttvFilterTree
 */
void
lttv_print_tree(const LttvFilterTree* t, const int count) {

  g_debug("node:%p lchild:%p rchild:%p depth:%i\n",t, //t->l_child.t,t->r_child.t);
          (t->left==LTTV_TREE_NODE)?t->l_child.t:NULL,
          (t->right==LTTV_TREE_NODE)?t->r_child.t:NULL,
          count);
  g_debug("logic operator: %s\n",(t->node&1)?"OR":((t->node&2)?"AND":((t->node&4)?"NOT":((t->node&8)?"XOR":"IDLE"))));
  g_debug("|-> left branch %p is a %s\n",t->l_child.t,(t->left==LTTV_TREE_NODE)?"NODE":((t->left==LTTV_TREE_LEAF)?"LEAF":"IDLE"));
  if(t->left == LTTV_TREE_LEAF) {
    g_debug("| |-> field type number: %i\n",t->l_child.leaf->field);
    g_debug("| |-> offset is: %i\n",t->l_child.leaf->offset);
    g_debug("| |-> operator function is: %p\n",t->l_child.leaf->op);
  }
  g_debug("|-> right branch %p is a %s\n",t->r_child.t,(t->right==LTTV_TREE_NODE)?"NODE":((t->right==LTTV_TREE_LEAF)?"LEAF":"IDLE"));
  if(t->right == LTTV_TREE_LEAF) {
    g_debug("| |-> field type number: %i\n",t->r_child.leaf->field);
    g_debug("| |-> offset is: %i\n",t->r_child.leaf->offset);
    g_debug("| |-> operator function is: %p\n",t->r_child.leaf->op);
  }

  if(t->left == LTTV_TREE_NODE) lttv_print_tree(t->l_child.t,count+1);
  if(t->right == LTTV_TREE_NODE) lttv_print_tree(t->r_child.t,count+1);
}

/**
 *  @fn static void module_init()
 * 
 *  Initializes the filter module and specific values
 */
static void module_init()
{

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



