/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang, Mathieu Desnoyers
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

#include <stdio.h>
#include <glib.h>

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/type.h>

static unsigned intSizes[] = {
  sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t), 
  sizeof(short) };

typedef enum _intSizesNames { SIZE_INT8, SIZE_INT16, SIZE_INT32,
                              SIZE_INT64, SIZE_SHORT, INT_SIZES_NUMBER }
                   intSizesNames;


static unsigned floatSizes[] = {
  0, 0, sizeof(float), sizeof(double), 0, sizeof(float), sizeof(double) };

#define FLOAT_SIZES_NUMBER 7


/*****************************************************************************
 *Function name
 *    ltt_eventtype_name : get the name of the event type
 *Input params
 *    et                 : an  event type   
 *Return value
 *    char *             : the name of the event type
 ****************************************************************************/

gchar *ltt_eventtype_name(LttEventType *et)
{
  return et->name;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_description : get the description of the event type
 *Input params
 *    et                 : an  event type   
 *Return value
 *    char *             : the description of the event type
 ****************************************************************************/

gchar *ltt_eventtype_description(LttEventType *et)
{
  return et->description;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_facility : get the facility which contains the event type
 *Input params
 *    et                     : an  event type   
 *Return value
 *    LttFacility *          : the facility
 ****************************************************************************/

LttFacility *ltt_eventtype_facility(LttEventType *et)
{
  return et->facility;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_relative_id : get the relative id of the event type
 *Input params
 *    et                        : an  event type   
 *Return value
 *    unsigned                  : the relative id
 ****************************************************************************/

unsigned ltt_eventtype_relative_id(LttEventType *et)
{
  return et->index;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_id : get the id of the event type
 *Input params
 *    et               : an  event type   
 *Return value
 *    unsigned         : the id
 ****************************************************************************/

unsigned ltt_eventtype_id(LttEventType *et)
{
  return et->facility->base_id + et->index;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_type : get the type of the event type
 *Input params
 *    et                 : an  event type   
 *Return value
 *    LttType *         : the type of the event type
 ****************************************************************************/

LttType *ltt_eventtype_type(LttEventType *et)
{
  if(unlikely(!et->root_field)) return NULL;
  else return et->root_field->field_type;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_field : get the root filed of the event type
 *Input params
 *    et                  : an  event type   
 *Return value
 *    LttField *          : the root filed of the event type
 ****************************************************************************/

LttField *ltt_eventtype_field(LttEventType *et)
{
  return et->root_field;
}

/*****************************************************************************
 *Function name
 *    ltt_type_name  : get the name of the type
 *Input params
 *    t              : a type   
 *Return value
 *    char *         : the name of the type
 ****************************************************************************/

gchar *ltt_type_name(LttType *t)
{
  return t->element_name;
}

/*****************************************************************************
 *Function name
 *    ltt_type_class : get the type class of the type
 *Input params
 *    t              : a type   
 *Return value
 *    LttTypeEnum  : the type class of the type
 ****************************************************************************/

LttTypeEnum ltt_type_class(LttType *t)
{
  return t->type_class;
}

/*****************************************************************************
 *Function name
 *    ltt_type_size : obtain the type size. The size is the number of bytes 
 *                    for primitive types (INT, UINT, FLOAT, ENUM), or the 
 *                    size for the unsigned integer length count for sequences
 *Input params
 *    tf            : trace file
 *    t             : a type   
 *Return value
 *    unsigned      : the type size
 *    returns 0 if erroneous, and show a critical warning message.
 ****************************************************************************/

unsigned ltt_type_size(LttTrace * trace, LttType *t)
{
  unsigned size;
  if(unlikely(t->type_class==LTT_STRUCT || t->type_class==LTT_ARRAY || 
              t->type_class==LTT_STRING || t->type_class==LTT_UNION)) {
    size = 0;
  } else {
    if(t->type_class == LTT_FLOAT){
      size = floatSizes[t->size];
    }else{
      if(likely(t->size < INT_SIZES_NUMBER))
        size = intSizes[t->size];
      else{
        LttArchSize archsize = trace->system_description->size;
        if(archsize == LTT_LP32){
          if(t->size == 5) size = intSizes[SIZE_INT16];
          else size = intSizes[SIZE_INT32];
        }
        else if(archsize == LTT_ILP32 || archsize == LTT_LP64){
          if(t->size == 5) size = intSizes[SIZE_INT32];
          else{
            if(archsize == LTT_ILP32) size = intSizes[SIZE_INT32];
            else size = intSizes[SIZE_INT64];
          }
        }
        else if(archsize == LTT_ILP64) size = intSizes[SIZE_INT64];
      }
    }
  }

  return size;
}

/*****************************************************************************
 *Function name
 *    ltt_type_element_type : obtain the type of nested elements for arrays 
 *                            and sequences 
 *Input params
 *    t                     : a type   
 *Return value
 *    LttType              : the type of nested element of array or sequence
 ****************************************************************************/

LttType *ltt_type_element_type(LttType *t)
{
  LttType *element_type;

  if(unlikely(t->type_class != LTT_ARRAY && t->type_class != LTT_SEQUENCE))
    element_type = NULL;
  else
    element_type = t->element_type[0];

  return element_type;
}

/*****************************************************************************
 *Function name
 *    ltt_type_element_number : obtain the number of elements for arrays 
 *Input params
 *    t                       : a type   
 *Return value
 *    unsigned                : the number of elements for arrays
 ****************************************************************************/

unsigned ltt_type_element_number(LttType *t)
{
  unsigned ret = 0;

  if(likely(t->type_class == LTT_ARRAY))
    ret = t->element_number;

  return ret;
}

/*****************************************************************************
 *Function name
 *    ltt_type_member_number : obtain the number of data members for structure 
 *Input params
 *    t                      : a type   
 *Return value
 *    unsigned               : the number of members for structure
 ****************************************************************************/

unsigned ltt_type_member_number(LttType *t)
{
  unsigned ret = 0;
  
  if(likely(t->type_class == LTT_STRUCT || t->type_class == LTT_UNION))
    ret =t->element_number;

  return ret;
}

/*****************************************************************************
 *Function name
 *    ltt_type_member_type : obtain the type of a data member in a structure 
 *                           or union.
 *Input params
 *    t                    : a type   
 *    i                    : index of the member
 *Return value
 *    LttType *           : the type of structure member
 ****************************************************************************/

LttType *ltt_type_member_type(LttType *t, unsigned i, gchar ** name)
{
  LttType *member_type = NULL;

  if(unlikely(  (t->type_class != LTT_STRUCT
                 && t->type_class != LTT_UNION)
              ||
                (i >= t->element_number)
             )) {
      *name = NULL;
  } else {
    *name = t->element_type[i]->element_name;
    member_type = t->element_type[i];
  }

  return member_type;
}

/*****************************************************************************
 *Function name
 *    ltt_enum_string_get : for enumerations, obtain the symbolic string 
 *                          associated with a value (0 to n - 1 for an 
 *                          enumeration of n elements) 
 *Input params
 *    t                   : a type   
 *    i                   : index of the member
 *Return value
 *    char *              : symbolic string associated with a value
 ****************************************************************************/

char *ltt_enum_string_get(LttType *t, unsigned i)
{ 
  gchar *string = NULL;
  
  if(likely(t->type_class == LTT_ENUM && i < t->element_number))
    string = t->enum_strings[i];

  return string;
}

/*****************************************************************************
 *Function name
 *    ltt_field_element : obtain the field of nested elements for arrays and
 *                        sequence 
 *Input params
 *    f                 : a field   
 *Return value
 *    LttField *       : the field of the nested element
 ****************************************************************************/

LttField *ltt_field_element(LttField *f)
{
  LttField *nest = NULL;
  
  if(likely(f->field_type->type_class == LTT_ARRAY ||
              f->field_type->type_class == LTT_SEQUENCE))
    nest = f->child[0];

  return nest;
}

/*****************************************************************************
 *Function name
 *    ltt_field_member  : obtain the field of data members for structure
 *Input params
 *    f                 : a field   
 *    i                 : index of member field
 *Return value
 *    LttField *       : the field of the nested element
 ****************************************************************************/

LttField *ltt_field_member(LttField *f, unsigned i)
{
  LttField *field_member;

  if(unlikely(   f->field_type->type_class != LTT_STRUCT
                 && f->field_type->type_class != LTT_UNION)
              || i >= f->field_type->element_number )
    field_member = NULL;
  else
    field_member = f->child[i];

  return field_member;
}

/*****************************************************************************
 *Function name
 *    ltt_field_type  : obtain the type of the field 
 *Input params
 *    f               : a field   
 *Return value
 *    ltt_tyoe *      : the type of field
 ****************************************************************************/

LttType *ltt_field_type(LttField *f)
{
  if(unlikely(!f))return NULL;
  return f->field_type;
}

int ltt_field_size(LttField * f)
{
  if(unlikely(!f))return 0;
  return f->field_size;
}
