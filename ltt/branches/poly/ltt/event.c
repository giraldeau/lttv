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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include <asm/types.h>
#include <linux/byteorder/swab.h>

#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/event.h>
#include <ltt/trace.h>
#include <ltt/ltt-types.h>

LttEvent *ltt_event_new()
{
  return g_new(LttEvent, 1);
}

void ltt_event_destroy(LttEvent *event)
{
  g_free(event);
}


#if 0
/* Use get_field_type_size instead */
/*****************************************************************************
 *Function name
 *    ltt_event_refresh_fields   : refresh fields of an event 
 *Input params
 *    offsetRoot      : offset from the root
 *    offsetParent    : offset from the parent
 *    fld             : field
 *    evD             : event data
 *    reverse_byte_order : 1 or 0
 *Return value
 *    int             : size of the field
 ****************************************************************************/

int ltt_event_refresh_fields(int offsetRoot,int offsetParent, 
			     LttField * fld, void *evD, gboolean reverse_byte_order)
{
  int size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  switch(type->type_class) {
    case LTT_ARRAY:
      element_number = (int) type->element_number;
      if(fld->field_fixed == 0){// has string or sequence
        size = 0;
        for(i=0;i<element_number;i++){
          size += ltt_event_refresh_fields(offsetRoot+size,size, 
             fld->child[0], evD+size, reverse_byte_order);
        }
      }else size = fld->field_size;
      break;

    case LTT_SEQUENCE:
      size1 = fld->sequ_number_size;
      element_number = getIntNumber(reverse_byte_order,size1,evD);
      type->element_number = element_number;
      if(fld->element_size > 0){
        size = element_number * fld->element_size;
      }else{//sequence has string or sequence
        size = 0;
        for(i=0;i<element_number;i++){
          size += ltt_event_refresh_fields(offsetRoot+size+size1,size+size1, 
                   fld->child[0], evD+size+size1, reverse_byte_order);
        }	
        size += size1;
      }
      break;

    case LTT_STRING:
      size = strlen((gchar*)evD) + 1; //include end : '\0'
      break;

    case LTT_STRUCT:
      element_number = (int) type->element_number;
      if(fld->field_fixed == 0){
        offset1 = offsetRoot;
        offset2 = 0;
        for(i=0;i<element_number;i++){
          size=ltt_event_refresh_fields(offset1,offset2,
                               fld->child[i],evD+offset2, reverse_byte_order);
          offset1 += size;
          offset2 += size;
        }      
        size = offset2;
      }else size = fld->field_size;
      break;
      
    case LTT_UNION:
      size = fld->field_size;
      break;

    default:
      size = fld->field_size;
  }

#if 0
  if(type->type_class != LTT_STRUCT && type->type_class != LTT_ARRAY &&
     type->type_class != LTT_SEQUENCE && type->type_class != LTT_STRING){
    size = fld->field_size;
  }else if(type->type_class == LTT_ARRAY){
    element_number = (int) type->element_number;
    if(fld->field_fixed == 0){// has string or sequence
      size = 0;
      for(i=0;i<element_number;i++){
	      size += ltt_event_refresh_fields(offsetRoot+size,size, 
					 fld->child[0], evD+size);
      }
    }else size = fld->field_size;
  }else if(type->type_class == LTT_SEQUENCE){
    size1 = fld->sequ_number_size;
    element_number = getIntNumber(size1,evD);
    type->element_number = element_number;
    if(fld->element_size > 0){
      size = element_number * fld->element_size;
    }else{//sequence has string or sequence
      size = 0;
      for(i=0;i<element_number;i++){
	      size += ltt_event_refresh_fields(offsetRoot+size+size1,size+size1, 
				       	 fld->child[0], evD+size+size1);
      }	
      size += size1;
    }
  }else if(type->type_class == LTT_STRING){
    size = strlen((char*)evD) + 1; //include end : '\0'
  }else if(type->type_class == LTT_STRUCT){
    element_number = (int) type->element_number;
    if(fld->field_fixed == 0){
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	      size=ltt_event_refresh_fields(offset1,offset2,
                                      fld->child[i],evD+offset2);
      	offset1 += size;
      	offset2 += size;
      }      
      size = offset2;
    }else size = fld->field_size;
  }
#endif //0
  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  fld->fixed_root      = (offsetRoot==-1)   ? 0 : 1;
  fld->fixed_parent    = (offsetParent==-1) ? 0 : 1;
  fld->field_size      = size;

  return size;
}
#endif //0


/*****************************************************************************
 *Function name
 *    ltt_event_eventtype_id: get event type id 
 *                            (base id + position of the event)
 *Input params
 *    e                     : an instance of an event type   
 *Return value
 *    unsigned              : event type id
 ****************************************************************************/

unsigned ltt_event_eventtype_id(const LttEvent *e)
{
  return (unsigned) e->event_id;
}

/*****************************************************************************
 *Function name
 *    ltt_event_facility : get the facility of the event
 *Input params
 *    e                  : an instance of an event type   
 *Return value
 *    LttFacility *     : the facility of the event
 ****************************************************************************/

LttFacility *ltt_event_facility(const LttEvent *e)
{
  LttTrace * trace = e->tracefile->trace;
  unsigned id = e->facility_id;
  LttFacility *facility = ltt_trace_facility_by_id(trace,id);
  
  g_assert(facility->exists);

  return facility;
}

/*****************************************************************************
 *Function name
 *    ltt_event_facility_id : get the facility id of the event
 *Input params
 *    e                  : an instance of an event type   
 *Return value
 *    unsigned          : the facility of the event
 ****************************************************************************/

unsigned ltt_event_facility_id(const LttEvent *e)
{
  return e->facility_id;
}

/*****************************************************************************
 *Function name
 *    ltt_event_eventtype : get the event type of the event
 *Input params
 *    e                   : an instance of an event type   
 *Return value
 *    LttEventType *     : the event type of the event
 ****************************************************************************/

LttEventType *ltt_event_eventtype(const LttEvent *e)
{
  LttFacility* facility = ltt_event_facility(e);
  if(!facility) return NULL;
  return &g_array_index(facility->events, LttEventType, e->event_id);
}

/*****************************************************************************
 *Function name
 *    ltt_event_field : get the root field of the event
 *Input params
 *    e               : an instance of an event type   
 *Return value
 *    LttField *      : the root field of the event
 ****************************************************************************/

LttField *ltt_event_field(LttEvent *e)
{
  LttField * field;
  LttEventType * event_type = ltt_event_eventtype(e);
  if(unlikely(!event_type)) return NULL;
  field = event_type->root_field;
  if(unlikely(!field)) return NULL;

  get_field_type_size(e->tracefile, event_type, 0, 0,
      field, e->data);
  
  return field;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the time of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttTime       : the time of the event
 ****************************************************************************/

LttTime ltt_event_time(const LttEvent *e)
{
  return e->event_time;
}

/*****************************************************************************
 *Function name
 *    ltt_event_time : get the cycle count of the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    LttCycleCount  : the cycle count of the event
 ****************************************************************************/

LttCycleCount ltt_event_cycle_count(const LttEvent *e)
{
  return e->tsc;
}



/*****************************************************************************
 *Function name
 *    ltt_event_position_get : get the event position data
 *Input params
 *    e                  : an instance of an event type   
 *    ep                 : a pointer to event's position structure
 *    tf                 : tracefile pointer
 *    block              : current block
 *    offset             : current offset
 *    tsc                : current tsc
 ****************************************************************************/
void ltt_event_position_get(LttEventPosition *ep, LttTracefile **tf,
        guint *block, guint *offset, guint64 *tsc)
{
  *tf = ep->tracefile;
  *block = ep->block;
  *offset = ep->offset;
  *tsc = ep->tsc;
}


/*****************************************************************************
 *Function name
 *    ltt_event_position : get the event's position
 *Input params
 *    e                  : an instance of an event type   
 *    ep                 : a pointer to event's position structure
 ****************************************************************************/

void ltt_event_position(LttEvent *e, LttEventPosition *ep)
{
  ep->tracefile = e->tracefile;
  ep->block = e->block;
  ep->offset = e->offset;
  ep->tsc = e->tsc;
}

LttEventPosition * ltt_event_position_new()
{
  return g_new(LttEventPosition, 1);
}


/*****************************************************************************
 * Function name
 *    ltt_event_position_compare : compare two positions
 *    A NULL value is infinite.
 * Input params
 *    ep1                    : a pointer to event's position structure
 *    ep2                    : a pointer to event's position structure
 * Return
 *    -1 is ep1 < ep2
 *    1 if ep1 > ep2
 *    0 if ep1 == ep2
 ****************************************************************************/


gint ltt_event_position_compare(const LttEventPosition *ep1,
                                const LttEventPosition *ep2)
{
  if(ep1 == NULL && ep2 == NULL)
      return 0;
  if(ep1 != NULL && ep2 == NULL)
      return -1;
  if(ep1 == NULL && ep2 != NULL)
      return 1;

   if(ep1->tracefile != ep2->tracefile)
    g_error("ltt_event_position_compare on different tracefiles makes no sense");
   
  if(ep1->block < ep2->block)
    return -1;
  if(ep1->block > ep2->block)
    return 1;
  if(ep1->offset < ep2->offset)
    return -1;
  if(ep1->offset > ep2->offset)
    return 1;
  return 0;
}

/*****************************************************************************
 * Function name
 *    ltt_event_position_copy : copy position
 * Input params
 *    src                    : a pointer to event's position structure source
 *    dest                   : a pointer to event's position structure dest
 * Return
 *    void
 ****************************************************************************/
void ltt_event_position_copy(LttEventPosition *dest,
                             const LttEventPosition *src)
{
  if(src == NULL)
    dest = NULL;
  else
    *dest = *src;
}



LttTracefile *ltt_event_position_tracefile(LttEventPosition *ep)
{
  return ep->tracefile;
}

/*****************************************************************************
 *Function name
 *    ltt_event_cpu_i: get the cpu id where the event happens
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    unsigned       : the cpu id
 ****************************************************************************/

unsigned ltt_event_cpu_id(LttEvent *e)
{
  return e->tracefile->cpu_num;
}

/*****************************************************************************
 *Function name
 *    ltt_event_data : get the raw data for the event
 *Input params
 *    e              : an instance of an event type   
 *Return value
 *    void *         : pointer to the raw data for the event
 ****************************************************************************/

void *ltt_event_data(LttEvent *e)
{
  return e->data;
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_number
 *                   : The number of elements in a sequence field is specific
 *                     to each event. This function returns the number of 
 *                     elements for an array or sequence field in an event.
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *Return value
 *    unsigned       : the number of elements for an array/sequence field
 ****************************************************************************/
guint64 ltt_event_field_element_number(LttEvent *e, LttField *f)
{
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return 0;
  
  if(f->field_type->type_class == LTT_ARRAY)
    return f->field_type->element_number;
  return ltt_get_uint(LTT_GET_BO(e->tracefile), f->sequ_number_size,
      e + f->offset_root);
}

/*****************************************************************************
 *Function name
 *    ltt_event_field_element_select
 *                   : Set the currently selected element for a sequence or
 *                     array field
 *                     O(1) if fields are of fixed size, else O(n) if fields are
 *                     of variable size.
 *Input params
 *    e              : an instance of an event type
 *    f              : a field of the instance
 *    i              : the ith element (0, ...)
 ****************************************************************************/
void ltt_event_field_element_select(LttEvent *e, LttField *f, unsigned i)
{
  unsigned element_number;
  LttField *field;
  unsigned int k;
  size_t size;
  LttEventType *event_type;
 
  if(f->field_type->type_class != LTT_ARRAY &&
     f->field_type->type_class != LTT_SEQUENCE)
    return ;

  element_number  = ltt_event_field_element_number(e,f);
  event_type = ltt_event_eventtype(e);
  /* Sanity check for i : 0..n-1 only, and must be lower or equal element_number
   */
  if(i >= element_number) return;
  
  field = f->child[0];
  
  if(f->field_type->type_class == LTT_SEQUENCE)
    size = f->sequ_number_size;
  else
    size = 0;
  
  if(field->fixed_size == FIELD_FIXED) {
      size += field->field_size * i;

      get_field_type_size(e->tracefile, event_type,
                  f->offset_root+size, size, field, e->data);

  } else {
    for(k=0;k<=i;k++){
      size += get_field_type_size(e->tracefile, event_type,
                  f->offset_root+size, size, field, e->data);
    }
  }
  f->current_element = i;
}

/*****************************************************************************
 * These functions extract data from an event after architecture specific
 * conversions
 ****************************************************************************/
guint32 ltt_event_get_unsigned(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  LttTypeEnum t = f->field_type->type_class;

  g_assert(t == LTT_UINT || t == LTT_ENUM);

  if(f->field_size == 1){
    guint8 x = *(guint8 *)(e->data + f->offset_root);
    return (guint32) x;    
  }else if(f->field_size == 2){
    return (guint32)ltt_get_uint16(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 4){
    return (guint32)ltt_get_uint32(reverse_byte_order, e->data + f->offset_root);
  }
#if 0
  else if(f->field_size == 8){
    guint64 x = *(guint64 *)(e->data + f->offset_root);
    if(e->tracefile->trace->my_arch_endian == LTT_LITTLE_ENDIAN)
      return (unsigned int) (revFlag ? GUINT64_FROM_BE(x): x);    
    else
      return (unsigned int) (revFlag ? GUINT64_FROM_LE(x): x);    
  }
#endif //0
  g_critical("ltt_event_get_unsigned : field size %i unknown", f->field_size);
  return 0;
}

gint32 ltt_event_get_int(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  g_assert(f->field_type->type_class == LTT_INT);

  if(f->field_size == 1){
    gint8 x = *(gint8 *)(e->data + f->offset_root);
    return (gint32) x;    
  }else if(f->field_size == 2){
    return (gint32)ltt_get_int16(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 4){
    return (gint32)ltt_get_int32(reverse_byte_order, e->data + f->offset_root);
  }
#if 0
  else if(f->field_size == 8){
    gint64 x = *(gint64 *)(e->data + f->offset_root);
    if(e->tracefile->trace->my_arch_endian == LTT_LITTLE_ENDIAN)
      return (int) (revFlag ? GINT64_FROM_BE(x): x);    
    else
      return (int) (revFlag ? GINT64_FROM_LE(x): x);    
  }
#endif //0
  g_critical("ltt_event_get_int : field size %i unknown", f->field_size);
  return 0;
}

guint64 ltt_event_get_long_unsigned(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  LttTypeEnum t = f->field_type->type_class;

  g_assert(t == LTT_UINT || t == LTT_ENUM 
      || t == LTT_ULONG || LTT_SIZE_T || LTT_OFF_T || LTT_POINTER);

  if(f->field_size == 1){
    guint8 x = *(guint8 *)(e->data + f->offset_root);
    return (guint64) x;    
  }else if(f->field_size == 2){
    return (guint64)ltt_get_uint16(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 4){
    return (guint64)ltt_get_uint32(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 8){
    return ltt_get_uint64(reverse_byte_order, e->data + f->offset_root);
  }
  g_critical("ltt_event_get_long_unsigned : field size %i unknown", f->field_size);
  return 0;
}

gint64 ltt_event_get_long_int(LttEvent *e, LttField *f)
{
  //int revFlag = e->tracefile->trace->my_arch_endian == 
  //              e->tracefile->trace->system_description->endian ? 0:1;
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  g_assert( f->field_type->type_class == LTT_INT
      || f->field_type->type_class == LTT_LONG
      || f->field_type->type_class == LTT_SSIZE_T);

  if(f->field_size == 1){
    gint8 x = *(gint8 *)(e->data + f->offset_root);
    return (gint64) x;    
  }else if(f->field_size == 2){
    return (gint64)ltt_get_int16(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 4){
    return (gint64)ltt_get_int32(reverse_byte_order, e->data + f->offset_root);
  }else if(f->field_size == 8){
    return ltt_get_int64(reverse_byte_order, e->data + f->offset_root);
  }
  g_critical("ltt_event_get_long_int : field size %i unknown", f->field_size);
  return 0;
}

float ltt_event_get_float(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  g_assert(f->field_type->type_class == LTT_FLOAT && f->field_size == 4);

  if(reverse_byte_order == 0) return *(float *)(e->data + f->offset_root);
  else{
    guint32 aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 4);
    aInt = ___swab32(aInt);
    return ((float)aInt);
  }
}

double ltt_event_get_double(LttEvent *e, LttField *f)
{
  gboolean reverse_byte_order = LTT_GET_BO(e->tracefile);

  g_assert(f->field_type->type_class == LTT_FLOAT && f->field_size == 8);

  if(reverse_byte_order == 0) return *(double *)(e->data + f->offset_root);
  else{
    guint64 aInt;
    memcpy((void*)&aInt, e->data + f->offset_root, 8);
    aInt = ___swab64(aInt);
    return ((double)aInt);
  }
}

/*****************************************************************************
 * The string obtained is only valid until the next read from
 * the same tracefile.
 ****************************************************************************/
char *ltt_event_get_string(LttEvent *e, LttField *f)
{
  g_assert(f->field_type->type_class == LTT_STRING);

  return (gchar*)g_strdup((gchar*)(e->data + f->offset_root));
}


/*****************************************************************************
 *Function name
 *    get_field_type_size : set the fixed and dynamic sizes of the field type
 *    from the data read.
 *Input params 
 *    tf              : tracefile
 *    event_type      : event type
 *    offset_root     : offset from the root
 *    offset_parent   : offset from the parent
 *    field           : field
 *    data            : a pointer to the event data.
 *Returns the field type size.
 ****************************************************************************/
size_t get_field_type_size(LttTracefile *tf, LttEventType *event_type,
    off_t offset_root, off_t offset_parent,
    LttField *field, void *data)
{
  size_t size = 0;
  guint i;
  LttType *type;
  
  g_assert(field->fixed_root != FIELD_UNKNOWN);
  g_assert(field->fixed_parent != FIELD_UNKNOWN);
  g_assert(field->fixed_size != FIELD_UNKNOWN);

  field->offset_root = offset_root;
  field->offset_parent = offset_parent;
  
  type = field->field_type;

  switch(type->type_class) {
    case LTT_INT:
    case LTT_UINT:
    case LTT_FLOAT:
    case LTT_ENUM:
    case LTT_POINTER:
    case LTT_LONG:
    case LTT_ULONG:
    case LTT_SIZE_T:
    case LTT_SSIZE_T:
    case LTT_OFF_T:
      g_assert(field->fixed_size == FIELD_FIXED);
      size = field->field_size;
      break;
    case LTT_SEQUENCE:
      {
        gint seqnum = ltt_get_uint(LTT_GET_BO(tf),
                        field->sequ_number_size,
                        data + offset_root);

        if(field->child[0]->fixed_size == FIELD_FIXED) {
          size = field->sequ_number_size + 
            (seqnum * get_field_type_size(tf, event_type,
                                          offset_root, offset_parent,
                                          field->child[0], data));
        } else {
          size += field->sequ_number_size;
          for(i=0;i<seqnum;i++) {
            size_t child_size;
            child_size = get_field_type_size(tf, event_type,
                                    offset_root, offset_parent,
                                    field->child[0], data);
            offset_root += child_size;
            offset_parent += child_size;
            size += child_size;
          }
        }
        field->field_size = size;
      }
      break;
    case LTT_STRING:
      size = strlen((char*)(data+offset_root)) + 1;// length + \0
      field->field_size = size;
      break;
    case LTT_ARRAY:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        for(i=0;i<field->field_type->element_number;i++) {
          size_t child_size;
          child_size = get_field_type_size(tf, event_type,
                                  offset_root, offset_parent,
                                  field->child[0], data);
          offset_root += child_size;
          offset_parent += child_size;
          size += child_size;
        }
        field->field_size = size;
      }
      break;
    case LTT_STRUCT:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        size_t current_root_offset = offset_root;
        size_t current_offset = 0;
        size_t child_size = 0;
        for(i=0;i<type->element_number;i++) {
          child_size = get_field_type_size(tf,
                     event_type, current_root_offset, current_offset, 
                     field->child[i], data);
          current_offset += child_size;
          current_root_offset += child_size;
          
        }
        size = current_offset;
        field->field_size = size;
      }
      break;
    case LTT_UNION:
      if(field->fixed_size == FIELD_FIXED)
        size = field->field_size;
      else {
        size_t current_root_offset = field->offset_root;
        size_t current_offset = 0;
        for(i=0;i<type->element_number;i++) {
          size = get_field_type_size(tf, event_type,
                 current_root_offset, current_offset, 
                 field->child[i], data);
          size = max(size, field->child[i]->field_size);
        }
        field->field_size = size;
      }
      break;
  }

  return size;
}


