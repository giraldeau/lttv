#include <stdio.h>

#include <ltt/LTTTypes.h> 
#include "parser.h"
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

char *ltt_eventtype_name(ltt_eventtype *et)
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

char *ltt_eventtype_description(ltt_eventtype *et)
{
  return et->description;
}

/*****************************************************************************
 *Function name
 *    ltt_eventtype_type : get the type of the event type
 *Input params
 *    et                 : an  event type   
 *Return value
 *    ltt_type *         : the type of the event type
 ****************************************************************************/

ltt_type *ltt_eventtype_type(ltt_eventtype *et)
{
  return et->root_field->field_type;
}

/*****************************************************************************
 *Function name
 *    ltt_type_name  : get the name of the type
 *Input params
 *    t              : a type   
 *Return value
 *    char *         : the name of the type
 ****************************************************************************/

char *ltt_type_name(ltt_type *t)
{
  return t->element_name;
}

/*****************************************************************************
 *Function name
 *    ltt_type_class : get the type class of the type
 *Input params
 *    t              : a type   
 *Return value
 *    ltt_type_enum  : the type class of the type
 ****************************************************************************/

ltt_type_enum ltt_type_class(ltt_type *t)
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

unsigned ltt_type_size(ltt_tracefile * tf, ltt_type *t)
{
  if(t->type_class==LTT_STRUCT || t->type_class==LTT_ARRAY || 
     t->type_class==LTT_STRING) return 0;

  if(t->type_class == LTT_FLOAT){
    return floatSizes[t->size];
  }else{
    if(t->size < sizeof(intSizes)/sizeof(unsigned))
      return intSizes[t->size];
    else{
      ltt_arch_size size = tf->trace_header->arch_size;
      if(size == LTT_LP32)
	return sizeof(int16_t);
      else if(size == LTT_ILP32 || size == LTT_LP64)
	return sizeof(int32_t);
      else if(size == LTT_ILP64)
	return sizeof(int64_t);	
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
 *    ltt_type              : the type of nested element of array or sequence
 ****************************************************************************/

ltt_type *ltt_type_element_type(ltt_type *t)
{
  if(t->type_class != LTT_ARRAY || t->type_class != LTT_SEQUENCE)
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

unsigned ltt_type_element_number(ltt_type *t)
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

unsigned ltt_type_member_number(ltt_type *t)
{
  if(t->type_class != LTT_STRUCT)
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
 *    ltt_type *           : the type of structure member
 ****************************************************************************/

ltt_type *ltt_type_member_type(ltt_type *t, unsigned i)
{
  if(t->type_class != LTT_STRUCT) return NULL;
  if(i > t->element_number || i == 0 ) return NULL;
  return t->element_type[i-1];
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

char *ltt_enum_string_get(ltt_type *t, unsigned i)
{  
  if(t->type_class != LTT_ENUM) return NULL;
  if(i > t->element_number || i == 0 ) return NULL;
  return t->enum_strings[i-1];
}

/*****************************************************************************
 *Function name
 *    ltt_field_element : obtain the field of nested elements for arrays and
 *                        sequence 
 *Input params
 *    f                 : a field   
 *Return value
 *    ltt_field *       : the field of the nested element
 ****************************************************************************/

ltt_field *ltt_field_element(ltt_field *f)
{
  if(f->field_type->type_class != LTT_ARRAY ||
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
 *    ltt_field *       : the field of the nested element
 ****************************************************************************/

ltt_field *ltt_field_member(ltt_field *f, unsigned i)
{
  if(f->field_type->type_class != LTT_STRUCT) return NULL;
  if(i==0 || i>f->field_type->element_number) return NULL;
  return f->child[i-1];
}

/*****************************************************************************
 *Function name
 *    ltt_field_type  : obtain the type of the field 
 *Input params
 *    f               : a field   
 *Return value
 *    ltt_tyoe *      : the type of field
 ****************************************************************************/

ltt_type *ltt_field_type(ltt_field *f)
{
  return f->field_type;
}

