/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/type.h>

static unsigned intSizes[] = {
  sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t), 
  sizeof(short) };

static unsigned floatSizes[] = {
  0, 0, sizeof(float), sizeof(double), 0, sizeof(float), sizeof(double) };


/*****************************************************************************
 *Function name
 *    ltt_eventtype_name : get the name of the event type
 *Input params
 *    et                 : an  event type   
 *Return value
 *    char *             : the name of the event type
 ****************************************************************************/

char *ltt_eventtype_name(LttEventType *et)
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

char *ltt_eventtype_description(LttEventType *et)
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
  if(!et->root_field) return NULL;
  return et->root_field->field_type;
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

char *ltt_type_name(LttType *t)
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
 ****************************************************************************/

unsigned ltt_type_size(LttTrace * trace, LttType *t)
{
  if(t->type_class==LTT_STRUCT || t->type_class==LTT_ARRAY || 
     t->type_class==LTT_STRING) return 0;

  if(t->type_class == LTT_FLOAT){
    return floatSizes[t->size];
  }else{
    if(t->size < sizeof(intSizes)/sizeof(unsigned))
      return intSizes[t->size];
    else{
      LttArchSize size = trace->system_description->size;
      if(size == LTT_LP32){
	if(t->size == 5)return sizeof(int16_t);
	else return sizeof(int32_t);
      }
      else if(size == LTT_ILP32 || size == LTT_LP64){
	if(t->size == 5)return sizeof(int32_t);
	else{
	  if(size == LTT_ILP32) return sizeof(int32_t);
	  else return sizeof(int64_t);
	}
      }
      else if(size == LTT_ILP64)return sizeof(int64_t);	      
    }
  }
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
  if(t->type_class != LTT_ARRAY && t->type_class != LTT_SEQUENCE)
    return NULL;
  return t->element_type[0];
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
  if(t->type_class != LTT_ARRAY)
    return 0;
  return t->element_number;
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
  if(t->type_class != LTT_STRUCT && t->type_class != LTT_UNION)
    return 0;
  return t->element_number;  
}

/*****************************************************************************
 *Function name
 *    ltt_type_member_type : obtain the type of a data members in a structure 
 *Input params
 *    t                    : a type   
 *    i                    : index of the member
 *Return value
 *    LttType *           : the type of structure member
 ****************************************************************************/

LttType *ltt_type_member_type(LttType *t, unsigned i, char ** name)
{
  if(t->type_class != LTT_STRUCT){*name == NULL; return NULL;}
  if(i >= t->element_number || i < 0 ){*name = NULL; return NULL;}
  *name = t->element_type[i]->element_name;
  return t->element_type[i];
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
  if(t->type_class != LTT_ENUM) return NULL;
  if(i >= t->element_number || i < 0 ) return NULL;
  return t->enum_strings[i];
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
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return NULL;

  return f->child[0];
}

/*****************************************************************************
 *Function name
 *    ltt_field_member  : obtain the filed of data members for structure
 *Input params
 *    f                 : a field   
 *    i                 : index of member field
 *Return value
 *    LttField *       : the field of the nested element
 ****************************************************************************/

LttField *ltt_field_member(LttField *f, unsigned i)
{
  if(f->field_type->type_class != LTT_STRUCT) return NULL;
  if(i < 0 || i >= f->field_type->element_number) return NULL;
  return f->child[i];
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
  if(!f)return NULL;
  return f->field_type;
}

int ltt_field_size(LttField * f)
{
  if(!f)return 0;
  return f->field_size;
}
